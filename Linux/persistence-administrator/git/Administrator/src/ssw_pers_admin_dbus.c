/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Implementation of DBus for Persistence Administration Service
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013-09-17 uidu0250			CSP_WZ#5633:  Wait for NSM to start before registration.
* 2013-03-09 uidu0250			CSP_WZ#4480:  Added persadmin_QuitDBusMainLoop function
* 2013-04-04 uidu0250			CSP_WZ#2739:  Moved PAS<->PCL communication to common part under PersCommonIPC.
* 2013.03.26 uidu0250			CSP_WZ#3171:  Update PAS registration to NSM
* 2013.02.15 uidu0250	        CSP_WZ#8849:  Modified PAS<->PCL communication interface
* 2013.02.04 uidu0250	        CSP_WZ#2211:  Rework for QAC compliancy
* 2012.11.28 uidu0250	        CSP_WZ#1280:  Updated implementation according to the updated org.genivi.persistence.admin interface
* 2012.11.13 uidu0250           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/ 


#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <pthread.h>
#include <dlt.h>

#include "persComErrors.h"

#include "ssw_pers_admin_dbus.h"
#include "ssw_pers_admin_service.h"
#include "NodeStateTypes.h"
#include "NodeStateConsumer.h"
#include "NodeStateLifecycleConsumer.h"


/**********************************************************************************************************************
*
* Local defines, macros, constants and type definitions.
*
**********************************************************************************************************************/

#define		PAS_SHUTDOWN_TIMEOUT				60000	/* ms */

/**********************************************************************************************************************
*
* Global variables. Initialization of global contexts.
*
**********************************************************************************************************************/
DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);

static GMainLoop          						*g_pMainLoop          		= NIL;
static GDBusConnection    						*g_pBusConnection     		= NIL;
static NodeStateLifeCycleConsumerSkeleton		*g_pNSLCSkeleton			= NIL;
static NodeStateConsumerProxy					*g_pNSCProxy				= NIL;

static volatile bool_t							g_bDBusConnInit				= false;
static bool_t									g_bRegisteredToNSM			= false;	/* registration to NSM status */


/**********************************************************************************************************************
*
* Prototypes for local functions (see implementation for description)
*
**********************************************************************************************************************/

static void 	OnBusAcquired_cb(	GDBusConnection *connection, const gchar     *name, gpointer user_data);
static void 	OnNameAcquired_cb(	GDBusConnection *connection, const gchar     *name, gpointer user_data);
static void 	OnNameLost_cb(	GDBusConnection *connection, const gchar     *name, gpointer     user_data);
static void 	OnNameAppeared(	GDBusConnection *connection, const gchar 	*name, const gchar 	*name_owner, gpointer 	user_data);


/* LifecycleRequest */
static gboolean OnHandleLifecycleRequest (  NodeStateLifeCycleConsumer 	*object,
											GDBusMethodInvocation 		*invocation,
											guint 						arg_Request,
											guint 						arg_RequestId);

/* ExportNSMConsumerIF */
static bool_t		ExportNSMConsumerIF(GDBusConnection    						*connection);



/**********************************************************************************************************************
*
* The function is called when a connection to the D-Bus could be established.
* According to the documentation the objects should be exported here.
*
* \param connection:  Connection, which was acquired
* \param name:        Bus name
* \param user_data:   Optionally user data
*
* \return void
*
**********************************************************************************************************************/
static void OnBusAcquired_cb(	GDBusConnection *connection,
								const gchar     *name,
								gpointer         user_data)
{
	bool_t	bRetVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Successfully connected to D-Bus."));

	/* Store the connection. */
	g_pBusConnection = connection;

	/* Export the org.genivi.NodeStateManager.LifeCycleConsumer interface over DBus trough the PAS object */
	bRetVal = ExportNSMConsumerIF(connection);
	if(true == bRetVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Successfully connected to D-Bus and exported object."));
	}
}



/**********************************************************************************************************************
*
* The function is called when the "bus name" could be acquired on the D-Bus.
*
* \param connection:  Connection over which the bus name was acquired
* \param name:        Acquired bus name
* \param user_data:   Optionally user data
*
* \return void
*
**********************************************************************************************************************/

static void OnNameAcquired_cb(	GDBusConnection *connection,
        						const gchar     *name,
        						gpointer         user_data)
{
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Successfully obtained D-Bus name:");
												DLT_STRING(name));

	/* DBus connection initialized */
	g_bDBusConnInit = true;
}



/**********************************************************************************************************************
*
* The function is called if either no connection to D-Bus could be established or
* the bus name could not be acquired.
*
* \param connection:  Connection. If it is NIL, no D-Bus connection could be established.
*                     Otherwise the bus name was lost.
* \param name:        Bus name
* \param user_data:   Optionally user data
*
* \return void
*
**********************************************************************************************************************/
static void OnNameLost_cb(	GDBusConnection *connection,
							const gchar     *name,
							gpointer         user_data )
{
	if(connection == NIL)
	{
		/* Error: the connection could not be established. */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to establish D-Bus connection."));
	}
	else
	{
		/* Error: connection established, but name not obtained. This might be a second instance of the application */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to obtain bus name ");
													DLT_STRING(name);
													DLT_STRING("."));
	}

	/* In both cases leave the main loop. */
	g_main_loop_quit(g_pMainLoop);

	/* DBus connection lost */
	g_bDBusConnInit = false;
}


/**********************************************************************************************************************
*
* Invoked when the name being watched is known to have an owner.
*
* \param connection:  The GDBusConnection the name is being watched on.
* \param name:        The name being watched.
* \param name_owner:  Unique name of the owner of the name being watched.
* \param user_data:   Optionally user data
*
* \return void
*
**********************************************************************************************************************/
static void OnNameAppeared(	GDBusConnection *connection,
                            const gchar 	*name,
                            const gchar 	*name_owner,
                            gpointer 		user_data )
{
	GError				*pError 		= NIL;
	gboolean			gbRetVal;
	gint				gErrorCode		= NsmErrorStatus_NotSet;

	if(false == g_bRegisteredToNSM)
	{
		/* Register to NSM as shutdown client */
		g_pNSCProxy = NIL;
		g_pNSCProxy = (NodeStateConsumerProxy *) node_state_consumer_proxy_new_sync(g_pBusConnection,
																					G_DBUS_PROXY_FLAGS_NONE,
																					NSM_BUS_NAME,
																					NSM_CONSUMER_OBJECT,
																					NIL,
																					&pError);
		if(NIL == g_pNSCProxy)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("Failed to create proxy for NSC. Error :"),
														DLT_STRING(pError->message));
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to register to NSM as shutdown client."));
			g_error_free (pError);
		}
		else
		{
			/* Synchronous call to "RegisterShutdownClient" */
			gbRetVal = node_state_consumer_call_register_shutdown_client_sync(	(NodeStateConsumer *)g_pNSCProxy,
																				PERSISTENCE_ADMIN_SVC_BUS_NAME,
																				PERSISTENCE_ADMIN_SVC_OBJ_PATH,
																				NSM_SHUTDOWNTYPE_NORMAL|NSM_SHUTDOWNTYPE_FAST,
																				PAS_SHUTDOWN_TIMEOUT,
																				&gErrorCode,
																				NIL,
																				&pError);
			if(FALSE == gbRetVal)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("RegisterShutdownClient call failed. Error :"),
															DLT_STRING(pError->message));
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to register to NSM as shutdown client."));

				g_error_free (pError);
				g_object_unref(g_pNSCProxy);
				g_pNSCProxy = NIL;
			}
			else
			{
				if(NsmErrorStatus_Ok != gErrorCode)
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to register to NSM as shutdown client. Error code :"),
																DLT_INT(gErrorCode));
					g_object_unref(g_pNSCProxy);
					g_pNSCProxy = NIL;
				}
				else
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Successfully registered to NSM as shutdown client."));

					g_bRegisteredToNSM = true;
				}
			}
		}
	}
}

/**********************************************************************************************************************
*
* Handler for LifecycleRequest.
* Signature based on generated code.
*
**********************************************************************************************************************/
static gboolean OnHandleLifecycleRequest (  NodeStateLifeCycleConsumer 	*object,
											GDBusMethodInvocation 		*invocation,
											guint 						arg_Request,
											guint 						arg_RequestId)
{
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Received NSM shutdown notification."));

	/* send a NsmErrorStatus_ResponsePending response and wait for PAS to
	 * finish any operation in progress */
	node_state_life_cycle_consumer_complete_lifecycle_request(	object,
																invocation,
																NsmErrorStatus_ResponsePending);


	if( (0 != (NSM_SHUTDOWNTYPE_NORMAL & arg_Request)) ||
		(0 != (NSM_SHUTDOWNTYPE_FAST & arg_Request)))
	{
		/* set shutdown state and wait for the current operation to finish*/
		/* Any other PAS request is denied after this point
		 * (unless a cancel shutdown notification is received)
		 */
		(void)persadmin_set_shutdown_state(true);
	}
	else
	{
		if(0 != (NSM_SHUTDOWNTYPE_RUNUP & arg_Request))
		{
			(void)persadmin_set_shutdown_state(false);
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, 	DLT_STRING("Invalid NSM notification received:"),
														DLT_UINT(arg_Request));
		}
	}


	/* Notify NSM that the request was finished  */
	node_state_consumer_call_lifecycle_request_complete((NodeStateConsumer *)g_pNSCProxy,
														arg_RequestId,
														NsmErrorStatus_Ok,
														NIL,
														NIL,
														NIL);

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Finished preparing for shutdown."));

	return (TRUE);
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/



/**********************************************************************************************************************
*
* Function that exports the org.genivi.NodeStateManager.LifeCycleConsumer interface over DBus trough the PAS object.
*
* \param connection:  Connection over which the interface should be exported
*
* \return true if successful, false otherwise
*
**********************************************************************************************************************/
static bool_t		ExportNSMConsumerIF(GDBusConnection    						*connection)
{
	GError *pError = NIL;

	/* Create object to offer on the DBus (for NSMConsumer) */
	g_pNSLCSkeleton			= (NodeStateLifeCycleConsumerSkeleton*) node_state_life_cycle_consumer_skeleton_new();

	(void)g_signal_connect(g_pNSLCSkeleton, "handle-lifecycle-request", G_CALLBACK(&OnHandleLifecycleRequest), NIL);

	/* Attach interface to the object and export it */
	if(FALSE == g_dbus_interface_skeleton_export(	G_DBUS_INTERFACE_SKELETON(g_pNSLCSkeleton),
													g_pBusConnection,
													PERSISTENCE_ADMIN_SVC_OBJ_PATH,
													&pError))
	{
		/* Error: NSM Consumer object could not be exported. */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to export LifeCycleConsumer interface. Error :"),
													DLT_STRING(pError->message));
		g_error_free(pError);
		return false;
	}

	return true;
}


/**
 * \brief Starts the main DBus loop. Returns when the DBus connection is closed
 *
 * \return void
 */
void persadmin_RunDBusMainLoop(void)
{
	uint32_t				u32ConnectionId;
	guint 					watcherId;
	GBusNameWatcherFlags 	flags =  G_BUS_NAME_WATCHER_FLAGS_NONE;

	/* Initialize GLib */
	g_type_init();			/* deprecated. Since GLib 2.36, the type system is initialized automatically and this function does nothing.*/

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Creating a new DBus connection..."));

	/* Connect to the D-Bus. Obtain a bus name to offer PAS objects */
	u32ConnectionId = g_bus_own_name( PERSISTENCE_ADMIN_SVC_BUS_TYPE
									, PERSISTENCE_ADMIN_SVC_BUS_NAME
									, G_BUS_NAME_OWNER_FLAGS_NONE
									, &OnBusAcquired_cb
									, &OnNameAcquired_cb
									, &OnNameLost_cb
									, NIL
									, NIL);

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("D-Bus connection Id: "), DLT_UINT32(u32ConnectionId));

	/* Watch the NSM bus name */
	watcherId = g_bus_watch_name (	PERSISTENCE_ADMIN_SVC_BUS_TYPE, /* NSM uses the same bus type as PAS  */
									NSM_BUS_NAME,
									flags,
									&OnNameAppeared,
									NIL,
									NIL,
									NIL);

	/* Create the main loop */
	g_pMainLoop = g_main_loop_new(NIL, FALSE);

	/* The main loop is only canceled if the Node is completely shut down or the D-Bus connection fails */
	g_main_loop_run(g_pMainLoop);

	g_bus_unwatch_name (watcherId);

	/* If the main loop returned, clean up. Release bus name and main loop */
	g_bus_unown_name(u32ConnectionId);

	g_main_loop_unref(g_pMainLoop);
	g_pMainLoop = NIL;

	/* Release the (created) skeleton object */
	if(NIL != g_pNSLCSkeleton)
	{
		g_object_unref(g_pNSLCSkeleton);
		g_pNSLCSkeleton = NIL;
	}

	/* Release the (created) proxy object */
	if(NIL != g_pNSCProxy)
	{
		g_object_unref(g_pNSCProxy);
		g_pNSCProxy = NIL;
	}
}


/**
 * \brief Quits the main DBus loop
 *
 * \return void
 */
void persadmin_QuitDBusMainLoop(void)
{
	if(true == g_bDBusConnInit)
	{
		if(NIL != g_pMainLoop)
		{
			g_main_loop_quit(g_pMainLoop);
			g_bDBusConnInit = false;
		}
	}
}


/**
 * \brief Check if the registration to NSM was performed successfully.
 *
 * \return true if registered to NSM, false otherwise
 */
bool_t 	persadmin_IsRegisteredToNSM(void)
{
	return g_bRegisteredToNSM;
}

