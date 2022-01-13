/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Implementation of PCL communication gate for Persistence Administration Service
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.10.24 uidu0250			CSP_WZ#6327:  Change error handling for missing PCL clients
* 2013.10.02 uidu0250			CSP_WZ#5927:  Update values for IPC request flags to match PCL v7
* 2013.04.04 uidu0250			CSP_WZ#2739:  Using PersCommonIPC for requests to PCL
* 2013.02.18 uidu0250	        CSP_WZ#8849:  Block DBus client registration during Sync&Lock
* 2013.02.05 uidu0250	        CSP_WZ#2211:  Fixed QAC deviations
* 2013.02.05 uidu0250	        CSP_WZ#2211:  Perform rollback in case of lock&sync failed
* 2012.11.28 uidu0250	        CSP_WZ#1280:  Added an additional param. to persadmin_ChangePersistenceMode to specify the need for request confirmations
* 2012.11.13 uidu0250           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/ 


#include <stdlib.h>
#include <string.h>
#include <gio/gio.h>
#include <pthread.h>
#include <dlt.h>

#include "persComErrors.h"
#include "persComIpc.h"
#include "ssw_pers_admin_pcl.h"


/* ---------- local defines, macros, constants and type definitions ------------ */

//#define TEST_MODE_ENABLED

#ifdef TEST_MODE_ENABLED
#define		REG_FLAG_SET						1
#endif // TEST_MODE_ENABLED

#define		PAS_NOTIF_CONF_WAIT_STEP			5	// ms

/* Override persComIpc.h flags to mach the PCL v.7 implementation */
#define PERSISTENCE_MODE_LOCK_PCL           (0x0001U)   	/* as defined by PCL */
#define PERSISTENCE_MODE_UNLOCK_PCL         (0x0002U)   	/* as defined by PCL */
#define PERSISTENCE_MODE_SYNC_PCL           (0x0010U)   	/* as defined by PCL */
#define	PERSISTENCE_PCL_RESPONSE_PENDING	(0x0001U)		/* as defined by PCL */
#define PERSISTENCE_PCL_STATUS_OK			(0x0002U)		/* as defined by PCL */

typedef struct PAS_NOTIF_CLIENT_INFO_
{
	sint32_t							clientId;
	uint32_t							notificationFlag;
	uint32_t							timeoutMs;
	bool_t								bClientNotified;
	bool_t								bRequestConfirmed;
	struct PAS_NOTIF_CLIENT_INFO_	*	pNextClient;

}PAS_NOTIF_CLIENT_INFO, *PPAS_NOTIF_CLIENT_INFO;


/* ----------global variables. initialization of global contexts ------------ */

DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);

static PPAS_NOTIF_CLIENT_INFO					pPersRegClients				= NIL;	 /* list of clients registered for notifications */
static pthread_mutex_t 							persRegClientsListMtx;	 		 	 /* mutex for the access to the list of registered clients */

static volatile bool_t							g_bBlockClientReg			= false; /* flag indicating if the DBus client registration is blocked */
static volatile bool_t							g_bWaitForNotifConf			= false; /* flag indicating there is a notification sent and confirmations are waited */
static volatile int32_t							g_s32RequestId				= 0;	 /* used to identify requests */
static uint32_t									g_ReqAdminPersMode			= 0;	 /* currently requested persistence admin mode */

static uint32_t									persistenceMode				= 0;	 /* current state of the persistence from PAS p.o.v. */


#ifdef TEST_MODE_ENABLED
static pthread_t           						g_hThread            		= 0;
static int 										notifThreadFlag;
static pthread_cond_t 							notifThreadFlagCV;
static pthread_mutex_t 							notifThreadFlagMtx;
#endif // TEST_MODE_ENABLED


/* ---------------------- local functions ---------------------------------- */

/* Register callback */
static int persIpcRegisterCB(	int    			clientID,
								unsigned int 	flags,
								unsigned int 	timeout);

/* Un-register callback */
static int	persIpcUnRegisterCB(int    			clientID,
								unsigned int 	flags);

/* PersAdminRequestCompleted callback */
static int	persIpcRequestCompletedCB(	int				clientID,
										int 			requestID,
										unsigned int 	status);

/* Change persistence mode */
static sint_t persadmin_ChangePersistenceMode(	uint32_t	u32PersMode, bool_t		bWaitForConfirmation);

/* Notify all PCL clients */
static sint_t NotifyRegisteredClients( uint32_t	u32PersMode	);

/* Wait for confirmations from all notified clients */
static sint_t WaitForNotificationConfirmations(void);

//static void ReleaseClientInfoList(PPAS_NOTIF_CLIENT_INFO	* ppClientInfoList);

#ifdef TEST_MODE_ENABLED
static void* 	ClientNotificationThread(void *lpParam);
#endif // TEST_MODE_ENABLED


/**
 * \brief Init the PAS IPC module
 *
 * \return 0 for success, negative value for error (see persComErrors.h)
 */
sint_t	persadmin_InitIpc(void)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;
	PersAdminPASInitInfo_s 	pInitInfo;

	/* Init synchronization objects */
	if(0 != pthread_mutex_init (&persRegClientsListMtx, NIL))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to initialize mutex."));
		return PERS_COM_FAILURE;
	}

	#ifdef TEST_MODE_ENABLED
	/* Create the notification thread*/
	pthread_create(&g_hThread, NIL, ClientNotificationThread, NIL);
	#endif // TEST_MODE_ENABLED

	pInitInfo.pRegCB 			= persIpcRegisterCB;
	pInitInfo.pUnRegCB			= persIpcUnRegisterCB;
	pInitInfo.pReqCompleteCB	= persIpcRequestCompletedCB;

	retVal = persIpcInitPAS(&pInitInfo);
	if(PERS_COM_SUCCESS != retVal)
	{
		/* Error: the connection could not be established. */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to establish IPC protocol for PAS with error code"),
													DLT_INT(retVal));
	}
	else
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Successfully established IPC protocol for PAS."));
	}

	#ifdef TEST_MODE_ENABLED
	/* Wait the client notification thread */
	//pthread_join(g_hThread, NIL);
	#endif // TEST_MODE_ENABLED

	return retVal;
}


/**
 * \brief PCL client registration callback
 *
 * \param clientID                      [in] unique identifier assigned to the registered client
 * \param flags                         [in] flags specifying the notifications to register for (bitfield using any of the flags : ::PERSISTENCE_MODE_LOCK, ::PERSISTENCE_MODE_SYNC and ::PERSISTENCE_MODE_UNLOCK)
 * \param timeout                       [in] maximum time needed to process any supported request (in milliseconds)
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */
static int persIpcRegisterCB(	int    			clientID,
								unsigned int 	flags,
								unsigned int 	timeout)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;
	PPAS_NOTIF_CLIENT_INFO	pClientInfo			= NIL;
	PPAS_NOTIF_CLIENT_INFO	pNewClientInfo		= NIL;

	if(true == g_bBlockClientReg)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Client registration failed for client with ID :"),
													DLT_INT(clientID));
		return PERS_COM_FAILURE;
	}

	/* Acquire mutex on the list of registered clients */
	if(0 != pthread_mutex_lock (&persRegClientsListMtx))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to lock mutex."));
		return PERS_COM_FAILURE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Registering client with ID :"),
												DLT_INT(clientID));

	pClientInfo  = pPersRegClients;

	while((NIL != pClientInfo) && (NIL != pClientInfo->pNextClient))
	{
		pClientInfo = pClientInfo->pNextClient;
	}


	pNewClientInfo = NIL;
	pNewClientInfo = (PPAS_NOTIF_CLIENT_INFO)malloc(sizeof(*pNewClientInfo)); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pNewClientInfo)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Error allocating memory for client info data."));
		(void)pthread_mutex_unlock (&persRegClientsListMtx);
		return PERS_COM_ERR_MALLOC;
	}
	else
	{
		(void)memset(pNewClientInfo, 0, sizeof(*pNewClientInfo));
	}


	if(NIL == pPersRegClients)
	{
		pPersRegClients = pNewClientInfo;
	}
	else
	{
		pClientInfo->pNextClient = pNewClientInfo;
	}

	pNewClientInfo->clientId = clientID;

	/* each registered application is notified according to the registration mask */
	pNewClientInfo->notificationFlag = flags;

	/* each application will register using a specified timeout (associated to the max duration of the requested operation) */
	pNewClientInfo->timeoutMs = timeout;

	pNewClientInfo->bClientNotified 	= false;
	pNewClientInfo->bRequestConfirmed	= false;

	/* Release list mutex */
	(void)pthread_mutex_unlock (&persRegClientsListMtx);

	#ifdef TEST_MODE_ENABLED
	/* Notify the client notification thread */
	pthread_mutex_lock (&notifThreadFlagMtx);
	notifThreadFlag = REG_FLAG_SET;
	pthread_cond_signal (&notifThreadFlagCV);
	pthread_mutex_unlock (&notifThreadFlagMtx);
	#endif // TEST_MODE_ENABLED

	return retVal;
}


/**
 * \brief PCL client un-registration callback
 *
 * \param clientID                      [in] unique identifier assigned to the registered client
 * \param flags                         [in] flags specifying the notifications to un-register from (bitfield using any of the flags : ::PERSISTENCE_MODE_LOCK, ::PERSISTENCE_MODE_SYNC and ::PERSISTENCE_MODE_UNLOCK)
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */
static int	persIpcUnRegisterCB(int    			clientID,
								unsigned int 	flags)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;
	PPAS_NOTIF_CLIENT_INFO	pClientInfo			= NIL;
	PPAS_NOTIF_CLIENT_INFO	pPrevClientInfo		= NIL;

	// Acquire mutex on list
	if(0 != pthread_mutex_lock (&persRegClientsListMtx))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to lock mutex."));
		return PERS_COM_FAILURE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Un-registering client with ID :"),
												DLT_INT(clientID));

	pClientInfo  = pPersRegClients;

	while(NIL != pClientInfo)
	{
		if(clientID == pClientInfo->clientId)
		{
			break;
		}

		pPrevClientInfo = pClientInfo;
		pClientInfo = pClientInfo->pNextClient;
	}

	if(NIL != pClientInfo)
	{
		/* remove specified client info */
		if(NIL == pPrevClientInfo)
		{
			pPersRegClients = NIL;
		}
		else
		{
			pPrevClientInfo->pNextClient = pClientInfo->pNextClient;
		}

		free(pClientInfo);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	}

	/* Release list mutex */
	(void)pthread_mutex_unlock (&persRegClientsListMtx);

	return retVal;
}


/**
 * \brief PCL request confirmation callback
 *
 * \param clientID                      [in] unique identifier assigned to the registered client
 * \param requestID                     [in] unique identifier of the request sent by PAS. Should have the same value
 *                                           as the parameter requestID specified by PAS when calling 
                                             \ref persIpcSendRequestToPCL
 * \param status                        [in] the status of the request processed by PCL
 *                                           - In case of success: bitfield using any of the flags, depending on the request : ::PERSISTENCE_STATUS_LOCKED.
 *                                           - In case of error: the sum of ::PERSISTENCE_STATUS_ERROR and an error code \ref PERS_COM_IPC_DEFINES_ERROR is returned.
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */     
static int	persIpcRequestCompletedCB(	int				clientID,
										int 			requestID,
										unsigned int 	status)
{
	sint_t 					retVal 			= PERS_COM_SUCCESS;
	bool_t					bFailed			= false;
	PPAS_NOTIF_CLIENT_INFO	pClientInfo		= NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("PersistenceAdminRequestCompleted called by client with ID :"),
												DLT_INT(clientID));

	/* Check if the requestId is still valid */
	if(requestID != g_s32RequestId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING("PersistenceAdminRequestCompleted ignored notification with different RequestId."));

		/* ignore notification */
		return PERS_COM_ERR_INVALID_PARAM;
	}


	/* Acquire mutex on the list of registered clients */
	if(0 != pthread_mutex_lock (&persRegClientsListMtx))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to lock mutex."));
		return PERS_COM_FAILURE;
	}

	/* Check if the sender is in the current list of notified clients */
	pClientInfo  = pPersRegClients;

	while(NIL != pClientInfo)
	{
		if(clientID == pClientInfo->clientId)
		{
			break;
		}

		pClientInfo = pClientInfo->pNextClient;
	}

	if(NIL == pClientInfo)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Invalid client confirmed the request. ID :"),
													DLT_INT(pClientInfo->clientId));
		(void)pthread_mutex_unlock (&persRegClientsListMtx);
		return PERS_COM_FAILURE;
	}

	/* Check if this client should confirm the persistence mode change (by the registered notification mask) */
	if(0 == ((pClientInfo->notificationFlag) & g_ReqAdminPersMode))
	{
		/* Ignore client if a different state than the one specified by his mask was confirmed */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Invalid client confirmed the request. ID :"),
													DLT_INT(clientID));
	}else
	{
		if(PERSISTENCE_STATUS_ERROR == (PERSISTENCE_STATUS_ERROR & status))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("PersAdminRequestCompleted called by client with ID :"),
														DLT_INT(pClientInfo->clientId),
														DLT_STRING("reported the error code:"),
														DLT_UINT8((uint8_t)status));
			/* Stop waiting for notification confirmations */
			/* Mark the failed notification */
			/* The persAdminMode should be switched back to the previous value */
			/* Stop the thread checking the list of notified clients */
			g_bWaitForNotifConf = false;

			(void)pthread_mutex_unlock (&persRegClientsListMtx);
			return PERS_COM_FAILURE;
		}
		else
		{
			/* Check if the client status reflects the performed request */

			bFailed = false;

			/* Check request-status associations */

			if((PERSISTENCE_MODE_LOCK_PCL == (PERSISTENCE_MODE_LOCK_PCL & g_ReqAdminPersMode)) &&
			   (PERSISTENCE_MODE_LOCK_PCL == (PERSISTENCE_MODE_LOCK_PCL & pClientInfo->notificationFlag)) &&
			   (PERSISTENCE_STATUS_LOCKED != (PERSISTENCE_STATUS_LOCKED & status)))
			{
				bFailed = true;
			}

			/* additional checks should be added here if new persistence mode values/status values are added */

			if(true == bFailed)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("PersAdminRequestCompleted called by client with ID :"),
															DLT_INT(pClientInfo->clientId),
															DLT_STRING("reported an invalid status."));

				/* Stop waiting for notification confirmations */
				/* Mark the failed notification */
				/* The persAdminMode should be switched back to the previous value */
				/* Stop the thread checking the list of notified clients */
				g_bWaitForNotifConf = false;

				(void)pthread_mutex_unlock (&persRegClientsListMtx);
				return PERS_COM_FAILURE;

			}
			else
			{
				pClientInfo->bRequestConfirmed = true;
			}
		}
	}

	/* Release mutex on the list of clients notified */
	(void)pthread_mutex_unlock (&persRegClientsListMtx);

	return retVal;
}


/**
 * \brief Change persistence mode
 *
 * \param u32PersMode                   [in] flags specifying the new persistence mode (bitfield using any of the flags : ::PERSISTENCE_MODE_LOCK, ::PERSISTENCE_MODE_SYNC and ::PERSISTENCE_MODE_UNLOCK)
 * \param bWaitForConfirmation          [in] if true the call will return after waiting for confirmations from all the notified clients, if false it will return after notifying
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */     
static sint_t persadmin_ChangePersistenceMode(uint32_t	u32PersMode, bool_t	bWaitForConfirmation)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;
	uint32_t				tmpReqAdminPersMode	= g_ReqAdminPersMode;

	g_ReqAdminPersMode = u32PersMode;

	/* Notify the registered clients */
	retVal = NotifyRegisteredClients(u32PersMode);
	if(PERS_COM_SUCCESS != retVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed notifying registered clients with error code : "),
													DLT_INT(retVal));
	}

	if((PERS_COM_SUCCESS == retVal) && (true == bWaitForConfirmation))
	{
		/* Wait for confirmations from the notified clients */
		retVal = WaitForNotificationConfirmations();
		if(PERS_COM_SUCCESS != retVal)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed waiting for notified clients with error code : "),
														DLT_INT(retVal));
		}
	}

	if(PERS_COM_SUCCESS != retVal)
	{
		g_ReqAdminPersMode = tmpReqAdminPersMode;
	}

	return retVal;
}


/**
 * \brief Notify all clients registered for the u32PersMode change
 *
 * \param u32PersMode                   [in] flags specifying the new persistence mode (bitfield using any of the flags : ::PERSISTENCE_MODE_LOCK, ::PERSISTENCE_MODE_SYNC and ::PERSISTENCE_MODE_UNLOCK)
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */
static sint_t NotifyRegisteredClients( uint32_t	u32PersMode	)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;
	sint_t					commRetVal			= PERS_COM_SUCCESS;
	PPAS_NOTIF_CLIENT_INFO	pClientInfo			= NIL;

	/* Generate a new request id */
	++g_s32RequestId;


	if(0 != pthread_mutex_lock (&persRegClientsListMtx))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to lock mutex."));
		return PERS_COM_FAILURE;
	}

	/* Notify asynchronously every registered client */
	pClientInfo  = pPersRegClients;

	while(NIL != pClientInfo)
	{
		pClientInfo->bClientNotified 	= false;
		pClientInfo->bRequestConfirmed	= false;

		/* Skip client registered for other notifications */
		if(0 != (((uint32_t)pClientInfo->notificationFlag) & u32PersMode))
		{
			/* Synchronous call to "PersistenceAdminRequest" */
			commRetVal = persIpcSendRequestToPCL(	pClientInfo->clientId,
													g_s32RequestId,
													u32PersMode);
			if((PERS_COM_SUCCESS != commRetVal) &&
			   (PERSISTENCE_PCL_RESPONSE_PENDING 	!= commRetVal) &&
			   (PERSISTENCE_PCL_STATUS_OK			!= commRetVal))
			{
				if(PERS_COM_IPC_ERR_PCL_NOT_AVAILABLE == commRetVal)
				{
					/* Considered that any missing client was notified and confirmed (to avoid triggering a timeout) */
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, 	DLT_STRING("Failed to notify client with ID :"),
																DLT_INT(pClientInfo->clientId),
																DLT_STRING(".Client not available."));
					pClientInfo->bRequestConfirmed = true;
				}
				else
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("PersistenceAdminRequest call failed for client with ID :"),
																DLT_INT(pClientInfo->clientId),
																DLT_STRING("Flags="),
																DLT_INT(u32PersMode),
																DLT_STRING("RequestId="),
																DLT_INT(g_s32RequestId));
					(void)pthread_mutex_unlock (&persRegClientsListMtx);
					return commRetVal;
				}
			}
			else
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("Notified client with ID :"),
															DLT_INT(pClientInfo->clientId),
															DLT_STRING("Flags="),
															DLT_INT(u32PersMode),
															DLT_STRING("RequestId="),
															DLT_INT(g_s32RequestId));
			}

			pClientInfo->bClientNotified = true;
		}

		pClientInfo = pClientInfo->pNextClient;
	}

	/* Release mutex on the list of clients notified */
	(void)pthread_mutex_unlock (&persRegClientsListMtx);

	return retVal;
}


/**
 * \brief Wait for confirmations from all the notified clients
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */
static sint_t WaitForNotificationConfirmations(void)
{
	sint_t					retVal				= PERS_COM_SUCCESS;
	uint32_t				currentTimeout		= 0;
	bool_t					bAllClientsConfirmed= true;
	PPAS_NOTIF_CLIENT_INFO	pClientInfo			= NIL;
	bool_t					bContinue			= false;

	struct timespec 		ts;


	ts.tv_sec	= 0;
	ts.tv_nsec	= (PAS_NOTIF_CONF_WAIT_STEP * 1000000);	/* PAS_NOTIF_CONF_WAIT_STEP ms */


	/* Wait for notification confirmations */
	currentTimeout 		= 0;
	g_bWaitForNotifConf = true;

	while(true == g_bWaitForNotifConf)
	{
		/* Acquire mutex on the current list of registered clients */
		if(0 != pthread_mutex_lock (&persRegClientsListMtx))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to lock mutex."));
			return PERS_COM_FAILURE;
		}

		/* Check the status of every registered client notified */
		bAllClientsConfirmed 	= true;
		pClientInfo 			= pPersRegClients;
		while(NIL != pClientInfo)
		{
			bContinue	= false;

			/* ignore clients not notified */
			if(false == pClientInfo->bClientNotified)
			{
				pClientInfo = pClientInfo->pNextClient;
				bContinue	= true;	// used instead of continue
			}

			/* skip clients that confirmed the request */
			if(false == bContinue)
			{
				if(true == pClientInfo->bRequestConfirmed)
				{
					pClientInfo = pClientInfo->pNextClient;
					bContinue	= true;	// used instead of continue
				}
			}

			if(false == bContinue)
			{
				/* consider that the client did not manage to handle the notification if the timeout was exceeded */
				if(currentTimeout > pClientInfo->timeoutMs)
				{
					g_bWaitForNotifConf	= false;
				}

				bAllClientsConfirmed = false;
				break;
			}
		}

		if(false == g_bWaitForNotifConf)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Timeout when trying to notify client with ID :"),
														DLT_INT(pClientInfo->clientId),
														DLT_STRING(" and timeout = "),
														DLT_UINT(pClientInfo->timeoutMs));
			retVal = PERS_COM_FAILURE;
		}

		/* Release mutex on the list of clients notified */
		(void)pthread_mutex_unlock (&persRegClientsListMtx);

		if(true == bAllClientsConfirmed)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("Notified all registered clients."));
			break;
		}

		/* Wait only when not in timeout for any of the clients */
		if(true == g_bWaitForNotifConf)
		{
			/* sleep */
			(void)nanosleep(&ts, NIL);

			currentTimeout += PAS_NOTIF_CONF_WAIT_STEP;
		}
	}

	return retVal;
}

//static void ReleaseClientInfoList(PPAS_NOTIF_CLIENT_INFO	* ppClientInfoList)
//{
//	PPAS_NOTIF_CLIENT_INFO	pClientInfo		= NIL;
//	PPAS_NOTIF_CLIENT_INFO	pClientInfoList	= NIL;
//
//	if((NIL != ppClientInfoList) && (NIL != *ppClientInfoList))
//	{
//		pClientInfoList	= *ppClientInfoList;
//		pClientInfo  	= pClientInfoList;
//
//		while(NIL != pClientInfo)
//		{
//			pClientInfoList = pClientInfo->pNextClient;
//			free(pClientInfo);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
//			pClientInfo  = pClientInfoList;
//		}
//
//		*ppClientInfoList = NIL;
//	}
//}


/* Client notification thread (test purposes only) */
#ifdef TEST_MODE_ENABLED
static void* 	ClientNotificationThread(void *lpParam)
{
	/* Wait for a DBus client registration */
	pthread_mutex_lock (&notifThreadFlagMtx);
	while (!notifThreadFlag)
	{
		pthread_cond_wait (&notifThreadFlagCV, &notifThreadFlagMtx);
	}
	pthread_mutex_unlock (&notifThreadFlagMtx);


	/* TEST	NOTIFY REGISTERED CLIENTS */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Notifying all registered clients of lock&sync persistence request."));
	persadmin_SendLockAndSyncRequestToPCL();

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Notifying all registered clients of unlock persistence request."));
	persadmin_SendUnlockRequestToPCL();

	return NIL;
}
#endif // TEST_MODE_ENABLED


/**
 * \brief Checks if the memory access is currently locked
 *
 * \return true if memory access is locked, false otherwise
 */
bool_t	persadmin_IsPCLAccessLocked(void)
{
	return (PERSISTENCE_MODE_LOCK_PCL == (PERSISTENCE_MODE_LOCK_PCL & persistenceMode)) ? true : false;
}


/**
 * \brief Locks the memory access and synchronizes the cache for all clients registered to PAS.
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */
sint_t	persadmin_SendLockAndSyncRequestToPCL(void)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;

	/* Check if access is already locked */
	if(	true == persadmin_IsPCLAccessLocked())
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("PCL access already locked."));

		/* Access already locked, return error */
		retVal = PERS_COM_ERR_ACCESS_DENIED;
	}

	/* Block DBus client registration */
	g_bBlockClientReg = true;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("DBus client registration blocked."));

	persistenceMode = (PERSISTENCE_MODE_LOCK_PCL | PERSISTENCE_MODE_SYNC_PCL);

	/* Sync & Lock */
	retVal = persadmin_ChangePersistenceMode(persistenceMode, true);
	if(PERS_COM_SUCCESS != retVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Sync & Lock request failed with error code : "),
													DLT_INT(retVal));

		/* Rollback - unlock access for all notified clients in case of failure */
		persistenceMode = PERSISTENCE_MODE_UNLOCK_PCL;

		(void)persadmin_ChangePersistenceMode(persistenceMode, false);

		/* Unblock DBus client registration */
		g_bBlockClientReg = false;

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("Client registration unblocked."));
	}
	else
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING("Sync & Lock performed."));
	}

	return retVal;
}


/**
 * \brief Unlocks the memory access for all clients registered to PAS.
 *
 * \return 0 for success, negative value for error (see \ref PERS_COM_ERROR_CODES_DEFINES)
 */
sint_t	persadmin_SendUnlockRequestToPCL(void)
{
	sint_t 					retVal 				= PERS_COM_SUCCESS;

	/* Check if access is already unlocked */
	if(	false == persadmin_IsPCLAccessLocked())
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("PCL access already unlocked."));

		/* Access already unlocked, return error */
		retVal = PERS_COM_ERR_ACCESS_DENIED;
	}

	persistenceMode = PERSISTENCE_MODE_UNLOCK_PCL;

	/* Unlock */
	retVal = persadmin_ChangePersistenceMode(persistenceMode, false);
	if(PERS_COM_SUCCESS != retVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Unlock request failed with error code : "),
													DLT_INT(retVal));
	}
	else
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING("Unlock performed."));
	}

	/* Unblock client registration */
	g_bBlockClientReg = false;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING("Client registration unblocked."));

	return retVal;
}
