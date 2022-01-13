/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Implementation of backup process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2012.11.27 uidu0250           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#include "stdio.h"
#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <pthread.h>

#include "test_PAS.h"
#include "PasModuleTestGen.h"



/**********************************************************************************************************************
*
* Global variables. Initialization of global contexts.
*
**********************************************************************************************************************/
static GMainLoop          						*g_pMainLoop          		= NULL;
static GDBusConnection    						*g_pBusConnection     		= NULL;
static OipPersistenceAdmintestframeworkSkeleton	*g_pTFSkeleton				= NULL;
static volatile bool_t							g_bDBusConnInit				= false;

static pthread_mutex_t 							testOpMtx;	 		 		 // run test cases - operation mutex



/**********************************************************************************************************************
*
* Prototypes for local functions (see implementation for description)
*
**********************************************************************************************************************/

static void 	OnBusAcquired_cb(GDBusConnection *pConnection, const gchar* sName, gpointer pUserData);
static void 	OnNameAcquired_cb(GDBusConnection *pConnection, const gchar* sName, gpointer pUserData);
static void 	OnNameLost_cb(GDBusConnection *pConnection, const gchar* sName, gpointer pUserData);

/* ExecuteTestCases */
static gboolean OnHandleExecuteTestCases (	OipPersistenceAdmintestframework 	*object,
											GDBusMethodInvocation 				*invocation);



/**********************************************************************************************************************
*
* The function is called when a connection to the D-Bus could be established.
* According to the documentation the objects should be exported here.
*
* @param pConnection: Connection, which was acquired
* @param sName:       Bus name
* @param pUserData:   Optionally user data
*
* @return void
*
**********************************************************************************************************************/
static void OnBusAcquired_cb(GDBusConnection *pConnection, const gchar* sName, gpointer pUserData)
{
  GError *pError = NULL;

  /* Store the connection. */
  g_pBusConnection = pConnection;

  /* Create real object to offer on the DBus */
  g_pTFSkeleton = (OipPersistenceAdmintestframeworkSkeleton*) oip_persistence_admintestframework_skeleton_new();

  g_signal_connect(g_pTFSkeleton, "handle-execute-test-cases", G_CALLBACK(OnHandleExecuteTestCases), pUserData);

  /* Attach interfaces to the objects and export them */
  if(TRUE == g_dbus_interface_skeleton_export(	G_DBUS_INTERFACE_SKELETON(g_pTFSkeleton),
                                      	  	  	g_pBusConnection,
                                      	  	  	PERSISTENCE_ADMIN_TF_OBJ_PATH,
                                      	  	  	&pError))
  {
	  printf("Successfully connected to D-Bus and exported object.\n");
  }
  else
  {
	  /* Error: the PersistenceAdminService TF interface could not be exported. */
	  printf("Failed to export PersistenceAdminService TF object.\n");
	  g_main_loop_quit(g_pMainLoop);
  }
  fflush(stdout);
}



/**********************************************************************************************************************
*
* The function is called when the "bus name" could be acquired on the D-Bus.
*
* @param pConnection: Connection over which the bus name was acquired
* @param sName:       Acquired bus name
* @param pUserData:   Optionally user data
*
* @return void
*
**********************************************************************************************************************/
static void OnNameAcquired_cb(GDBusConnection *pConnection, const gchar* sName, gpointer pUserData)
{
	printf("Successfully obtained D-Bus name: %s\n", sName);

	/* DBus connection initialized */
	g_bDBusConnInit = true;

	fflush(stdout);
}



/**********************************************************************************************************************
*
* The function is called if either no connection to D-Bus could be established or
* the bus name could not be acquired.
*
* @param pConnection: Connection. If it is NULL, no D-Bus connection could be established.
*                     Otherwise the bus name was lost.
* @param sName:       Bus name
* @param pUserData:   Optionally user data
*
* @return void
*
**********************************************************************************************************************/
static void OnNameLost_cb(GDBusConnection *pConnection, const gchar* sName, gpointer pUserData)
{
  if(pConnection == NULL)
  {
	  /* Error: the connection could not be established. */
	  printf("Failed to establish D-Bus connection.");
  }
  else
  {
	  /* Error: connection established, but name not obtained. This might be a second instance of the application */
	  printf("Failed to obtain bus name %s\n", sName);
  }
  fflush(stdout);

  /* In both cases leave the main loop. */
  g_main_loop_quit(g_pMainLoop);
}



/**********************************************************************************************************************
*
* Handler for ExecuteTestCases.
* Signature based on generated code.
*
**********************************************************************************************************************/
static gboolean OnHandleExecuteTestCases (	OipPersistenceAdmintestframework 	*object,
											GDBusMethodInvocation 				*invocation)
{
	sint_t		noOfTestCases			= 0;
	sint_t		noOfTestCasesSuccessful = 0;
	sint_t		noOfTestCasesFailed		= 0;

	// Acquire op mutex
	pthread_mutex_lock (&testOpMtx);

	ExecuteTestCases(	&noOfTestCases,
						&noOfTestCasesSuccessful,
						&noOfTestCasesFailed );

	// Release list mutex
	pthread_mutex_unlock (&testOpMtx);

	oip_persistence_admintestframework_complete_execute_test_cases(	object,
																	invocation,
																	noOfTestCases,
																	noOfTestCasesSuccessful,
																	noOfTestCasesFailed);

	return(TRUE);
}


/**********************************************************************************************************************
*
* Initialize DBus registration mechanism
*
**********************************************************************************************************************/
void persadmin_RunDBusMainLoop()
{
	uint u32ConnectionId = 0;

	/* Initialize glib */
	g_type_init();

	/* Init synchronization objects */
	pthread_mutex_init (&testOpMtx, NULL);

	/* Create the main loop */
	g_pMainLoop = g_main_loop_new(NULL, FALSE);


	/* Connect to the D-Bus. Obtain a bus name to offer PAS objects */
	u32ConnectionId = g_bus_own_name( PERSISTENCE_ADMIN_TF_BUS_TYPE
									, PERSISTENCE_ADMIN_TF_BUS_NAME
									, G_BUS_NAME_OWNER_FLAGS_NONE
									, &OnBusAcquired_cb
									, &OnNameAcquired_cb
									, &OnNameLost_cb
									, NULL
									, NULL);

	/* The main loop is only canceled if the Node is completely shut down or the D-Bus connection fails */
	g_main_loop_run(g_pMainLoop);

	/* If the main loop returned, clean up. Release bus name and main loop */
	g_bus_unown_name(u32ConnectionId);
	g_main_loop_unref(g_pMainLoop);


	/* Release the (created) skeleton object */
	if(NULL != g_pTFSkeleton)
	{
		g_object_unref(g_pTFSkeleton);
	}
}
