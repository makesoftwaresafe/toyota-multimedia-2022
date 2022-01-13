/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila[at]continental-corporation.com
*
* Implementation for Persistence Administration Service
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2014.09.11 uidu0250			OvipRbt#8870: Fixed timeout calculation in WDOG retriggering mechanism.
* 2013.10.07 uidu0250			CSP_WZ#5965:  Fixed timeout calculation for mq_timedreceive
* 2013.09.23 uidl9757           CSP_WZ#5781:  Watchdog timeout of pas-daemon
* 2013.09.17 uidu0250			CSP_WZ#5633:  Ignore requests received before registration to NSM
* 2013.09.03 uidu0250			CSP_WZ#4480:  Implement watchdog functionality
* 2013.05.23 uidu0250			CSP_WZ#3831:  Adapt PAS to be started as a service by systemd
* 2013.04.19 uidu0250  			CSP_WZ#3424:  Add PAS IF extension for "restore to default"
* 2013.04.04 uidu0250			CSP_WZ#2739:  Using PersCommonIPC for requests to PCL
* 2013.03.26 uidu0250			CSP_WZ#3171:  Update PAS registration to NSM
* 2012.11.16 uidl9757           CSP_WZ#1280:  persadmin_response_msg_s: rename bResult to result and change type to long
* 2012.11.13 uidu0250	        CSP_WZ#1280:  Implemented DBus registration/notification mechanism
* 2012.11.12 uidl9757           CSP_WZ#1280:  Created (only stubs for access lib part introduced)
*
**********************************************************************************************************************/

/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include <stddef.h>
#include <unistd.h>
#include <sys/file.h>
#include "fcntl.h"
#include <errno.h>
#include <sys/stat.h>
#include <sys/time.h>				/* used for watchdog re-triggering */
#include <mqueue.h>
#include <semaphore.h>
#include <dlt.h>
#include <pthread.h>
#include <signal.h>
#include <syslog.h>					/* Syslog messages */
#include <systemd/sd-daemon.h>		/* Systemd wdog    */

#include "persComErrors.h"

#include "persistence_admin_service.h"
#include "ssw_pers_admin_access_lib.h"
#include "ssw_pers_admin_pcl.h"
#include "ssw_pers_admin_dbus.h"
#include "ssw_pers_admin_pcl.h"
#include "ssw_pers_admin_service.h"


/* ---------- local defines, macros, constants and type definitions ------------ */

/* Definition of the process name for this application. Used for logging. */
#define 	PERSISTENCE_ADMIN_PROC_NAME 		"PersistenceAdminService"

#define 	LT_HDR                      		"PAS >>"

#define		INIT_PROC_PID						1
#define		ONE_SEC_IN_US						(1000000U)	/* microseconds */

#define		OS_SUCCESS_CODE						0
#define 	OS_ERROR_CODE               		(-1)
#define		OS_INVALID_HANDLE					(-1)

#define		DAEMONIZE_FAIL_RET_VAL				(-1)
#define		DAEMONIZE_SUCCESS_RET_VAL			1
#define		DAEMONIZE_PARENT_RET_VAL			0


#define		PAS_PID_FILE_NAME					"pers_admin_svc.pid"
#define		PAS_PID_FILE_PATH					"/tmp/" PAS_PID_FILE_NAME
#define		PAS_SYSPROC_PATH					"/proc"
#define		PAS_PID_MAX_SIZE					10
#define		PAS_PID_PROC_PATH_MAX_SIZE			(PAS_PID_MAX_SIZE + 6)	/* PAS_PID_MAX_SIZE + strlen(PAS_SYSPROC_PATH) + 1 */

#define 	PAS_WATCHDOG_USEC_DEFAULT_VALUE		(5 * ONE_SEC_IN_US)		/* microseconds */
#define		PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT	(50000U)   				/* 50 ms in microseconds */


/* ----------global variables. initialization of global contexts ------------ */

DLT_DECLARE_CONTEXT(persAdminSvcDLTCtx);

static sint32_t   		g_hPASPidFile         		= OS_INVALID_HANDLE;

static bool_t			g_bOpInProgress				= false;
static bool_t			g_bShutdownInProgress		= false;
static	volatile bool_t g_bRunAccessLibThread		= true;

static pthread_mutex_t 		shutdownMtx;
static pthread_cond_t		shutdownCond;


/* ---------------------- local functions declarations ---------------------------------- */

/**
 * \brief  Access lib thread. Thread responsible to perform (on behalf of the clients)
 * the requests available in persistence_admin_service.h
 *
 * \param  arg:	thread arguments
 *
 * \return NIL
 **/
static void *   persadmin_AccessLibThread               (void *arg);

/**
 * \brief  Process the specified request (from the requests available in persistence_admin_service.h)
 *
 * \param  psRequest:	request details
 * \param  pResult_out: request result
 *
 * \return true if successful, false otherwise
 **/
static bool_t   persadmin_ProcessRequest                (persadmin_request_msg_s* psRequest, long* pResult_out);

/**
 * \brief  The function is called from main and turns the current process into a daemon.
 *
 * \param  pProcessName:	Specifies the name of the process to be daemonized.
 *
 * \return 0 : parent process returned; positive value: success; negative value: error
 **/
static sint_t   persadmin_DaemonizeProcess(pstr_t pProcessName);

/**
 * \brief  Run main daemon actions.
 *
 * \return 0 : success; negative value: error
 **/
static sint_t 	persadmin_RunDaemon(void);

/**
 * \brief  Signal handler.
 *
 * \param  signum :	signal identifier
 *
 * \return void
 **/
static void 	persadmin_HandleTerm(int signum);

/**
 * \brief  The function is called from main. It checks if the  unlocks the PID file and closes the opened handles.
 *
 * \return true if the process is already running, false otherwise
 **/
static bool_t 	persadmin_IsAlreadyRunning(void);

/**
 * \brief  The function is called from main and writes the child PID to be used by systemd
 *
 * \return true if successful, false otherwise
 **/
static bool_t 	persadmin_LockPidFile(void);

/**
 * \brief  The function is called from main. It unlocks the PID file and closes the opened handles.
 *
 * \return void
 **/
static void 	persadmin_EndPASProcess(void);

/**
 * \brief Get re-trigger rate from environment variable (from .service file)
 *
 * \return re-trigger rate (in seconds)
 **/
static uint32_t persadmin_GetWdogRetriggerRate(void);


/* ---------------------- public functions definition ---------------------------------- */

/**
 * \brief Set PAS shutdown state
 *
 * \param state the shutdown state : true if the shutdown is pending, false otherwise
 *
 * \return : void
 */
void persadmin_set_shutdown_state(bool_t state)
{
	if(OS_SUCCESS_CODE != pthread_mutex_lock(&shutdownMtx))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR);
													DLT_STRING("Failed to lock the shutdown mutex. errno =");
													DLT_INT(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
	}
	else
	{
		if(true == state)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Shutdown request received. Waiting for active operations to complete..."));

			/* wait for any pending operation */
			while (true == g_bOpInProgress) {
				(void)pthread_cond_wait(&shutdownCond, &shutdownMtx);
			}

			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Completed all running operations before shutdown."));
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("Shutdown canceled notification received."));
		}

		g_bShutdownInProgress = state;

		(void)pthread_mutex_unlock(&shutdownMtx);
	}
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief  Allow the modification of the resource properties from data (key-values and files)
 *
 * \param  resource_config_file name of the persistency resource configuration to integrate
 *
 * \return positive value: number of modified properties in the resource configuration; negative value: error code
 */
long persadmin_resource_config_change_properties(char* resource_config_file)
{
    printf("persadmin_resource_config_change_properties(%s) \n",
        resource_config_file) ;
    printf("persadmin_resource_config_change_properties - NOT IMPLEMENTED \n") ;

    return -1 ;
}


/**
 * \brief Allow the copy of user related data between different users
 *
 * \param src_user_no the user ID source
 * \param src_seat_no the seat number source (seat 0 to 3)
 * \param dest_user_no the user ID destination
 * \param dest_seat_no the seat number destination (seat 0 to 3)
 *
 * \return positive value: number of bytes copied; negative value: error code
 */
long persadmin_user_data_copy(unsigned int src_user_no, unsigned int src_seat_no, unsigned int dest_user_no, unsigned int dest_seat_no)
{
    printf("persadmin_user_data_copy(%d %d %d %d) \n",
        src_user_no, src_seat_no, dest_user_no, dest_seat_no) ;
    printf("persadmin_user_data_copy - NOT IMPLEMENTED \n") ;

    return -1 ;
}


/* ---------------------- local functions definition ---------------------- */

static bool_t persadmin_ProcessRequest(persadmin_request_msg_s* psRequest, long* pResult_out)
{
    bool_t 	bEverythingOK 	= true;
    bool_t	bLockedMtx		= false;
    long result 			= -1 ;

    /* check if PAS is registered to NSM */
	if(false == persadmin_IsRegisteredToNSM())
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, DLT_STRING("Cannot process request. Not registered to NSM yet..."));

		bEverythingOK = false;
	}


	/* check if a shutdown is in progress */
	if(true == bEverythingOK)
	{
		if(OS_SUCCESS_CODE != pthread_mutex_lock(&shutdownMtx))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR);
														DLT_STRING("Failed to lock the shutdown mutex. errno =");
														DLT_INT(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
			bEverythingOK = false;
		}
		else
		{
			bLockedMtx = true;
		}
	}

	if(true == bEverythingOK)
	{
		if(true == g_bShutdownInProgress)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, DLT_STRING("Cannot process request. Shutdown is in progress..."));

			*pResult_out = PAS_FAILURE_OS_RESOURCE_ACCESS;

			bEverythingOK = false;
		}
	}

	if(true == bEverythingOK)
	{
		g_bOpInProgress = true;

		(void)pthread_mutex_unlock(&shutdownMtx);
		bLockedMtx = false;

		switch(psRequest->eRequest)
		{
			case PAS_Req_DataBackupCreate:
			{
				result = persadmin_data_backup_create(psRequest->type, psRequest->path1, psRequest->path2, psRequest->user_no, psRequest->seat_no) ;
				break ;
			}
			case PAS_Req_DataBackupRecovery:
			{
				result = persadmin_data_backup_recovery(psRequest->type, psRequest->path1, psRequest->path2, psRequest->user_no, psRequest->seat_no) ;
				break ;
			}
			case PAS_Req_DataRestoreToDefault:
			{
				result = persadmin_data_restore_to_default(psRequest->type, psRequest->defaultSource, psRequest->path2, psRequest->user_no, psRequest->seat_no);
				break;
			}
			case PAS_Req_DataFolderExport:
			{
				result = persadmin_data_folder_export(psRequest->type, psRequest->path1) ;
				break ;
			}
			case PAS_Req_DataFolderImport:
			{
				result = persadmin_data_folder_import(psRequest->type, psRequest->path1) ;
				break ;
			}
			case PAS_Req_ResourceConfigAdd:
			{
				result = persadmin_resource_config_add(psRequest->path1) ;
				break ;
			}
			case PAS_Req_ResourceConfigChangeProperties:
			{
				result = persadmin_resource_config_change_properties(psRequest->path1) ;
				break ;
			}
			case PAS_Req_UserDataCopy:
			{
				result = persadmin_user_data_copy(psRequest->src_user_no, psRequest->src_seat_no, psRequest->dest_user_no, psRequest->dest_seat_no) ;
				break ;
			}
			case PAS_Req_UserDataDelete:
			{
				result = persadmin_user_data_delete(psRequest->user_no, psRequest->seat_no) ;
				break ;
			}
			default:
			{
				/* shall never happen */
				bEverythingOK = false ;
				break ;
			}
		}

		/* operation finished */
		if(OS_SUCCESS_CODE != pthread_mutex_lock(&shutdownMtx))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR);
														DLT_STRING("Failed to lock the shutdown mutex. errno =");
														DLT_INT(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
			bEverythingOK = false;
		}
		else
		{
			bLockedMtx = true;

			g_bOpInProgress = false;

			/* notify any waiting thread */
			(void)pthread_cond_broadcast(&shutdownCond);
		}
    }

	if(true == bLockedMtx)
	{
		(void)pthread_mutex_unlock(&shutdownMtx);
		bLockedMtx = false;
	}

    *pResult_out = bEverythingOK ? result : PAS_FAILURE ;

    return bEverythingOK;

}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static void * persadmin_AccessLibThread(void *arg)
{
    bool_t 	bEverythingOK 		= true;

    sem_t * pSyncSem 			= NIL;
    mqd_t mqdMsgQueueRequest 	= OS_INVALID_HANDLE;
    mqd_t mqdMsgQueueResponse 	= OS_INVALID_HANDLE;

    persadmin_request_msg_s sRequest;
    persadmin_response_msg_s sResponse;

    struct timeval 		timestamp 				= {0};
    struct timespec		tsTimestamp;
	struct timespec		tsWdogTimestamp;
	struct timespec	*	pTimestamp				= NIL;

	uint32_t 			u32RetriggerRate 		= 0;
	bool_t				bWDogTimeoutSet 		= false;

    struct mq_attr mq_attr_request;
    struct mq_attr mq_attr_response;
    mq_attr_request.mq_maxmsg = 1; /* max 1 message in the queue */
    mq_attr_request.mq_msgsize = sizeof(persadmin_request_msg_s); 
    mq_attr_response.mq_maxmsg = 1; /* max 1 message in the queue */
    mq_attr_response.mq_msgsize = sizeof(persadmin_response_msg_s); 


    /* first initialize the resources */
    pSyncSem = sem_open(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE, O_CREAT|O_EXCL, S_IRWXU|S_IRWXG|S_IRWXO, 1) ;
    {
        if(NIL == pSyncSem)
        {
        	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
														DLT_STRING("Failed to create semaphore");
														DLT_STRING(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
														DLT_STRING(". Error :");
														DLT_STRING(strerror(errno)));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
			bEverythingOK = false;
        }
        else
        {
        	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,	DLT_STRING(LT_HDR);
														DLT_STRING("Semaphore");
														DLT_STRING(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
														DLT_STRING("created successfully."));
        }
    }

    if(bEverythingOK)
    {
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_CREAT|O_EXCL|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO, &mq_attr_request);
        if(OS_INVALID_HANDLE == mqdMsgQueueRequest)
        {
        	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
														DLT_STRING("Failed to open request queue");
														DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
														DLT_STRING(". Error :");
														DLT_STRING(strerror(errno)));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
        	bEverythingOK = false;
        }
        else
        {
        	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,	DLT_STRING(LT_HDR);
														DLT_STRING("Request queue");
														DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
														DLT_STRING("created successfully."));
        }
    }

    if(bEverythingOK)
    {
        mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_CREAT|O_EXCL|O_RDWR, S_IRWXU|S_IRWXG|S_IRWXO, &mq_attr_response);
        if(OS_INVALID_HANDLE == mqdMsgQueueResponse)
        {
        	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
														DLT_STRING("Failed to open response queue");
														DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
														DLT_STRING(". Error :");
														DLT_STRING(strerror(errno)));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
        	bEverythingOK = false;
        }
        else
        {
        	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,	DLT_STRING(LT_HDR);
														DLT_STRING("Response queue");
														DLT_STRING(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
														DLT_STRING("created successfully."));
        }
    }

    if(bEverythingOK)
    {
    	/* get watchdog retrigger rate */
    	u32RetriggerRate = persadmin_GetWdogRetriggerRate();

        /* everything is initialized now, so we can process requests */
        while(true == g_bRunAccessLibThread)
        {
        	sint_t	sErrCode;

        	if(OS_ERROR_CODE == gettimeofday(&timestamp, NULL))
        	{
        		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
															DLT_STRING("gettimeofday call failed. Error :");
															DLT_INT(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
        		break;
        	}
        	else
        	{
        		if(false == bWDogTimeoutSet)
				{
        			/* determine timeout moment in time based on the re-trigger rate */
        			tsWdogTimestamp.tv_sec 	= timestamp.tv_sec + (__time_t)((u32RetriggerRate / ONE_SEC_IN_US) + ( (uint64_t)timestamp.tv_usec + (u32RetriggerRate % ONE_SEC_IN_US))/ONE_SEC_IN_US );
        			tsWdogTimestamp.tv_nsec	= (((uint64_t)timestamp.tv_usec + (u32RetriggerRate % ONE_SEC_IN_US)) % ONE_SEC_IN_US) * 1000;
        			bWDogTimeoutSet = true;
				}

        		/* determine access lib timeout moment in time */
        		tsTimestamp.tv_sec  = timestamp.tv_sec + (__time_t)((PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT / ONE_SEC_IN_US) + ( (uint64_t)timestamp.tv_usec + (PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT % ONE_SEC_IN_US))/ONE_SEC_IN_US );
        		tsTimestamp.tv_nsec = (((uint64_t)timestamp.tv_usec + (PAS_ACCESS_LIB_REQ_QUEUE_TIMEOUT % ONE_SEC_IN_US)) % ONE_SEC_IN_US) * 1000;


        		if(	(tsTimestamp.tv_sec < tsWdogTimestamp.tv_sec) ||
        			((tsTimestamp.tv_sec == tsWdogTimestamp.tv_sec) && (tsTimestamp.tv_nsec < tsWdogTimestamp.tv_nsec)))
        		{
        			pTimestamp = &tsTimestamp;
        		}
        		else
        		{
        			pTimestamp = &tsWdogTimestamp;
        		}
        	}

			/* wait for request */
			sErrCode = mq_timedreceive(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), NIL, pTimestamp);

			switch(sErrCode)
			{
				case OS_ERROR_CODE:
				{
					if(ETIMEDOUT == errno)/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
					{
						/* check if watchdog timeout was active */
						if(pTimestamp == &tsWdogTimestamp)
						{
							/* re-trigger watchdog */
							persadmin_RetriggerWatchdog();

							/* reset watchdog timeout */
							bWDogTimeoutSet = false;
						}
					}
					else
					{
						DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
																	DLT_STRING("Invalid request received.");
																	DLT_STRING("Expected");
																	DLT_INT(sizeof(sRequest));
																	DLT_STRING("bytes"));
						g_bRunAccessLibThread = false;
					}
				}
				break;

				case sizeof(sRequest):
				{
					long 	requestResult = -1;
					sint_t	sFctRetVal;

					/* re-trigger watchdog */
					persadmin_RetriggerWatchdog();

					/* reset watchdog timeout */
					bWDogTimeoutSet = false;

					/* request received in the right format */
					sFctRetVal = persadmin_SendLockAndSyncRequestToPCL();
					if(PERS_COM_SUCCESS != sFctRetVal)
					{
						DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
																	DLT_STRING("persadmin_SendLockAndSyncRequestToPCL call failed. Error :");
																	DLT_INT(sFctRetVal));
						requestResult = PAS_FAILURE;
					}
					else
					{
						if(false == persadmin_ProcessRequest(&sRequest, &requestResult))
						{
							DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
																		DLT_STRING("persadmin_ProcessRequest call failed."));
						}

						(void)persadmin_SendUnlockRequestToPCL();
					}

					sResponse.result = requestResult;

					if(OS_ERROR_CODE == mq_send(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), 0))
					{
						DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
																	DLT_STRING("Failed to send response. Error :");
																	DLT_INT(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

						g_bRunAccessLibThread = false;
					}
				}
				break;

				default:
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR);
																DLT_STRING("Invalid request received. Returned");
																DLT_INT(sErrCode);
																DLT_STRING("bytes. Expected");
																DLT_INT(sizeof(sRequest));
																DLT_STRING("bytes"));
					g_bRunAccessLibThread = false;
				}
				break;
			}
        }
    }

    /* close handles */
    if(NIL != pSyncSem)
    {
    	(void)sem_close(pSyncSem);
    	(void)sem_unlink(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE);
    }

    if(OS_INVALID_HANDLE != mqdMsgQueueRequest)	/* && OS_ERROR_CODE != mqdMsgQueueRequest */
    {
    	(void)mq_close(mqdMsgQueueRequest);
    	(void)mq_unlink(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST);
    }

    if(OS_INVALID_HANDLE != mqdMsgQueueResponse) /* && OS_ERROR_CODE != mqdMsgQueueResponse */
    {
    	(void)mq_close(mqdMsgQueueResponse);
    	(void)mq_unlink(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE);
    }

    return NIL;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static sint_t persadmin_DaemonizeProcess (pstr_t pProcessName)
{
	pid_t	pid, sid;
	sint_t	sRetVal 		= DAEMONIZE_SUCCESS_RET_VAL;

	if(NIL == pProcessName)
	{
		syslog(LOG_ERR, "PAS >> Invalid argument in persadmin_DaemonizeProcess call.");

		sRetVal = DAEMONIZE_FAIL_RET_VAL;
	}

	if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
	{
		syslog(LOG_INFO, "PAS >> Daemon process %s starting.", pProcessName);

		/* Fork off the parent process */
		pid = fork();
		if (pid < 0)
		{
			syslog(LOG_ERR, "PAS >> Unable to fork, code=%d (%s).", errno, strerror(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

			sRetVal = DAEMONIZE_FAIL_RET_VAL;
		}
		else
		{
			if(pid > 0)
			{
				syslog(LOG_ERR, "PAS >> Child(daemon) process created successfully with PID=%d.", pid);

				sRetVal = DAEMONIZE_PARENT_RET_VAL;
			}
		}

		if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
		{
			/* Ignore or perform the default action for some of the signals in the child process */
			(void)signal(SIGCHLD,SIG_DFL); /* A child process dies */
			(void)signal(SIGTSTP,SIG_IGN); /* Various TTY signals */
			(void)signal(SIGTTOU,SIG_IGN);
			(void)signal(SIGTTIN,SIG_IGN);
			(void)signal(SIGHUP, SIG_IGN); /* Ignore hangup signal */
			(void)signal(SIGTERM,SIG_DFL); /* Die on SIGTERM */

			syslog(LOG_INFO, "PAS >> Child signals ignored.");

			/* Change the file mode mask */
			(void)umask(0);

			syslog(LOG_INFO, "PAS >> File mode mask changed.");

			/* Get a unique Session ID from the kernel */
			sid = setsid();
			if (OS_ERROR_CODE == sid)
			{
				syslog(LOG_ERR, "PAS >> Unable to get a unique session id. Error code %d (%s).", errno, strerror(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

				sRetVal = DAEMONIZE_FAIL_RET_VAL;
			}
			else
			{
				syslog(LOG_INFO, "PAS >> New SID for child process: %d.", sid);
			}
		}

		if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
		{
			/* Change the current working directory.
			 * This prevents the current directory from being locked; hence not being able to remove it.
			 * The root directory cannot be unmounted.
			 */
			if ((chdir("/")) < 0)
			{
				syslog(LOG_ERR, "PAS >> Unable to change directory to root path. Error code %d (%s)", errno, strerror(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/

				sRetVal = DAEMONIZE_FAIL_RET_VAL;
			}
			else
			{
				syslog(LOG_INFO, "PAS >> Current working directory changed for child process.");
			}
		}

		if(DAEMONIZE_SUCCESS_RET_VAL == sRetVal)
		{
			/* Redirect standard files to /dev/null */
			(void)freopen( "/dev/null", "r", stdin);
			(void)freopen( "/dev/null", "w", stdout);
			(void)freopen( "/dev/null", "w", stderr);

			syslog(LOG_INFO, "PAS >> Standard file descriptors redirected to /dev/null.");
		}
	}

	return sRetVal;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static sint_t persadmin_RunDaemon(void)
{
	sint_t					sRetVal				= EXIT_SUCCESS;
	sint_t 					retVal 				= PERS_COM_SUCCESS;
	bool_t					bSyncObjInitialized	= false;
	bool_t					bLockedPIDFile		= false;
	struct 	sigaction 		action;

	/* Initialize the logging interface */
	DLT_REGISTER_APP("PERS", "Persistence Administration Service");
	DLT_REGISTER_CONTEXT(persAdminSvcDLTCtx, "PADM", "Persistence Administration Service Context");

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING(PERSISTENCE_ADMIN_PROC_NAME);
												DLT_STRING(" starting..."));

	/* Set SIGTERM handler */
	(void)memset(&action, 0, sizeof(struct sigaction));
	action.sa_handler = &persadmin_HandleTerm;
	(void)sigaction(SIGTERM, &action, NULL);

	/* Lock the PID file */
	if(false == persadmin_LockPidFile())
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to lock the PID file."));

		sRetVal = EXIT_FAILURE;
	}
	else
	{
		bLockedPIDFile = true;
	}

	if(EXIT_FAILURE != sRetVal)
	{
		/* Initialize synchronization objects */
		if(OS_SUCCESS_CODE == pthread_mutex_init (&shutdownMtx, NULL))
		{
			if(OS_SUCCESS_CODE == pthread_cond_init (&shutdownCond, NULL))
			{
				bSyncObjInitialized = true;
			}
			else
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to init shutdown sync. obj. (cond)"));

				(void)pthread_mutex_destroy(&shutdownMtx);

				sRetVal = EXIT_FAILURE;
			}
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to init shutdown sync. obj. (mtx)"));

			sRetVal = EXIT_FAILURE;
		}

		if(true == bSyncObjInitialized)
		{
			pthread_t		g_hAccessLibThread    = 0;

			/* Create the AccessLib thread */
			if(OS_SUCCESS_CODE != pthread_create(&g_hAccessLibThread, NIL, &persadmin_AccessLibThread, NIL))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to create AccessLib thread."));
			}
			else
			{
				/* Init PAS IPC protocol */
				retVal = persadmin_InitIpc();
				if(PERS_COM_SUCCESS != retVal)
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING("Failed to init PAS IPC component."));

					sRetVal = EXIT_FAILURE;
				}
				else
				{
					/* Notify systemd that PAS is ready */
					(void)sd_notify(0, "READY=1");

					/* Init DBus connection and run the PAS main loop */
					persadmin_RunDBusMainLoop();
				}

				/* signal the AccessLib thread to stop */
				g_bRunAccessLibThread = false;

				/* Wait for the AccessLib thread */
				(void)pthread_join(g_hAccessLibThread, NIL);

				/* Close synchronization objects */
				(void)pthread_mutex_destroy(&shutdownMtx);
				(void)pthread_cond_destroy(&shutdownCond);
			}
		}
	}

	if(true == bLockedPIDFile)
	{
		/* Unlock PID file and close handles */
		persadmin_EndPASProcess();
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING(PERSISTENCE_ADMIN_PROC_NAME);
												DLT_STRING(" stopped."));
	DLT_UNREGISTER_CONTEXT(persAdminSvcDLTCtx);
	DLT_UNREGISTER_APP();

	return sRetVal;

}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static bool_t persadmin_LockPidFile(void)
{
	str_t 	strPid[PAS_PID_MAX_SIZE];
	bool_t	bRetVal = true;

	g_hPASPidFile = open(PAS_PID_FILE_PATH, O_RDWR|O_CREAT, 0640);
	if (OS_INVALID_HANDLE == g_hPASPidFile)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to create PID file:");
													DLT_STRING(PAS_PID_FILE_PATH));
		bRetVal = false;
	}
	else
	{
		if (OS_INVALID_HANDLE == lockf(g_hPASPidFile, F_TLOCK, 0))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to lock PID file:");
														DLT_STRING(PAS_PID_FILE_PATH));
			bRetVal = false;
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Locked PID file:");
														DLT_STRING(PAS_PID_FILE_PATH));

			/* write PID to be used by systemd */
			(void)sprintf(strPid, "%d", getpid());
			(void)write(g_hPASPidFile, strPid, strlen(strPid));
		}
	}

	return bRetVal;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static void persadmin_EndPASProcess(void)
{
	/* unlock PID file */
	if(OS_INVALID_HANDLE != g_hPASPidFile)
	{
		if(OS_INVALID_HANDLE == lockf(g_hPASPidFile, F_ULOCK, 0))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to unlock PID file:");
														DLT_STRING(PAS_PID_FILE_PATH));
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Unlocked PID file:");
														DLT_STRING(PAS_PID_FILE_PATH));
		}
	}

	/* close PID file */
	if(OS_INVALID_HANDLE == close(g_hPASPidFile))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to close PID file:");
													DLT_STRING(PAS_PID_FILE_PATH));
	}
	
	g_hPASPidFile = OS_INVALID_HANDLE;

	/* remove PID file */
	if(OS_INVALID_HANDLE == remove(PAS_PID_FILE_PATH))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to remove PID file:");
													DLT_STRING(PAS_PID_FILE_PATH));
	}
}


static bool_t persadmin_IsAlreadyRunning(void)
{
	sint32_t 		hFile 		= OS_INVALID_HANDLE;
	struct 	stat 	sStat;
	str_t 			strPid[PAS_PID_MAX_SIZE];
	str_t			strProcPath[PAS_PID_PROC_PATH_MAX_SIZE];
	uint32_t 		u32PidVal 	= 0;
	bool_t			bRetVal 	= true;

	/* check PID file */
	hFile = stat(PAS_PID_FILE_PATH, &sStat);
	if (OS_INVALID_HANDLE == hFile)
	{
		/* PAS process not running. The PID file does not exists */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Failed to open PAS PID file. Process is not running already. errno =");
													DLT_INT(errno));/*DG C8MR2R-MISRA-C:2004 Rule 20.5-SSW_Administrator_0003*/
		bRetVal =  false;
	}
	else
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Found PAS PID file");
													DLT_STRING(PAS_PID_FILE_PATH));

		/* PID file exists, check if the process with the specified PID is running */
		hFile = open(PAS_PID_FILE_PATH, O_RDONLY);
		if(OS_INVALID_HANDLE == hFile)
		{
			/* the process might be running */
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING("Failed to open file");
														DLT_STRING(PAS_PID_FILE_PATH));
		}
		else
		{
			/* read PID and check process status */
			(void)memset(strPid, 0, sizeof(strPid));
			(void)read(hFile, strPid, (PAS_PID_MAX_SIZE - 1));
			(void)sscanf(strPid, "%d", &u32PidVal);
			(void)close(hFile);

			(void)sprintf(strProcPath, "%s/%d", PAS_SYSPROC_PATH, u32PidVal);

			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING("Checking status for process");
														DLT_STRING(strProcPath));

			hFile = stat(strProcPath, &sStat);
			if (OS_INVALID_HANDLE == hFile)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING("File '");
											DLT_STRING(strProcPath);
											DLT_STRING("' exists, but process with PID:");
											DLT_STRING(strPid);
											DLT_STRING(" is not running."));
				bRetVal =  false;
			}
		}
	}

	return bRetVal;

}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static uint32_t persadmin_GetWdogRetriggerRate(void)
{
	const char	*sWdogUSec   	= NIL;
	uint32_t   	u32WdogUSec 	= 0;

	sWdogUSec = getenv("WATCHDOG_USEC");
	if(NIL == sWdogUSec)
	{
		/* use default watchdog re-trigger rate */
		u32WdogUSec = PAS_WATCHDOG_USEC_DEFAULT_VALUE;

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING(LT_HDR);
													DLT_STRING("Using default watchdog re-trigger rate :");
													DLT_UINT32(u32WdogUSec / 1000);
													DLT_STRING("ms."));
	}
	else
	{
		u32WdogUSec = strtoul(sWdogUSec, NULL, 10);

		/* The min. valid value for systemd is 1 s => WATCHDOG_USEC at least needs to contain 1.000.000 us */
		if(u32WdogUSec < ONE_SEC_IN_US)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR);
														DLT_STRING("Error: Invalid WDOG config: WATCHDOG_USEC:");
														DLT_STRING(sWdogUSec);
														DLT_STRING(".Using default watchdog re-trigger rate :");
														DLT_UINT32(PAS_WATCHDOG_USEC_DEFAULT_VALUE / 1000);
														DLT_STRING("ms"));

			u32WdogUSec = PAS_WATCHDOG_USEC_DEFAULT_VALUE;
		}
		else
		{
			u32WdogUSec /= 2;
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING(LT_HDR);
														DLT_STRING("WDOG retrigger rate [ms]:");
														DLT_UINT32(u32WdogUSec/1000));
		}
	}

	return u32WdogUSec;
}


void 	persadmin_RetriggerWatchdog(void)
{

	(void) sd_notify(0, "WATCHDOG=1");

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, 	DLT_STRING(LT_HDR);
												DLT_STRING("Triggered systemd WDOG"));
}


static void persadmin_HandleTerm(int signum)
{
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,	DLT_STRING(LT_HDR);
												DLT_STRING("SIGTERM received..."));

	/* quit DBus main loop */
	(void)persadmin_QuitDBusMainLoop();
}


int main(void)
{
	sint_t					sRetVal				= EXIT_SUCCESS;
	sint_t					sTmpRetVal;

	/* Initialize syslog */
	(void)setlogmask (LOG_UPTO (LOG_DEBUG));
	openlog("PAS", LOG_PID, LOG_USER);

	/* Check if the process is already running */
	if(true == persadmin_IsAlreadyRunning())
	{
		syslog(LOG_INFO, "PAS >> PAS process is already running! Only one instance allowed!");

		sRetVal = EXIT_FAILURE;
	}

	if(EXIT_FAILURE != sRetVal)
	{
		/* Daemonize the current process */
		sTmpRetVal = persadmin_DaemonizeProcess((str_t*)PERSISTENCE_ADMIN_PROC_NAME);
		if(DAEMONIZE_FAIL_RET_VAL == sTmpRetVal)
		{
			syslog(LOG_INFO, "PAS >> persadmin_DaemonizeProcess call failed.");

			sRetVal = EXIT_FAILURE;

		}else
		{
			if(DAEMONIZE_SUCCESS_RET_VAL == sTmpRetVal)
			{
				/* Run daemon actions */
				sRetVal = persadmin_RunDaemon();

			}/* exit with success for parent process */
		}
	}

	/* De-initialize syslog */
	closelog();

    return sRetVal;
}
