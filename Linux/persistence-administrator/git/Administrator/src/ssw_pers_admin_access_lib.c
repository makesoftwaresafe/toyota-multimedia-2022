/**********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Implementation of functions declared in persistence_admin_service.h
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.04.15 uidu0250 			CSP_WZ#3424:  Add IF extension for "restore to default"
* 2013.03.19 uidn3591           CSP_WZ#3061:  Update PAS access library according with persAdmin IF Version 2.0.0
* 2012.11.23 uidn3591           CSP_WZ#1280:  Minor bug fixes
* 2012.11.16 uidl9757           CSP_WZ#1280:  persadmin_response_msg_s: rename bResult to result and change type to long
* 2012.11.12 uidl9757           CSP_WZ#1280:  Created
*
**********************************************************************************************************************/

/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include "string.h"
#include "stdio.h"
#include "stdlib.h"
#include <stddef.h>
#include <unistd.h>
#include <sys/file.h>
#include "fcntl.h"
#include <errno.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <semaphore.h>

#include "persistence_admin_service.h"
#include "ssw_pers_admin_access_lib.h"

/* ---------------------- local functions ---------------------------------- */
static bool_t persadmin_lockSyncSem(sem_t ** ppSynchSem_inout) ;
static bool_t persadmin_unlockSyncSem(sem_t ** ppSynchSem_inout) ;

/* ---------------------- local variables ---------------------------------- */


/**
 * \brief Allow creation of a backup on different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param backup_name name of the backup to allow identification
 * \param applicationID the application identifier
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes written; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataBackupCreate(PersASSelectionType_e type, const char* backup_name, const char* applicationID, unsigned int user_no, unsigned int seat_no)
{
    pstr_t FuncName = "persAdminDataBackupCreate:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if((NIL != backup_name) && (NIL != applicationID))
    {
        if(((type >= PersASSelectionType_All) && (type < PersASSelectionType_LastEntry)) /*&& (user_no <= 4) && (seat_no <= 4)*/)
        {
            sint_t len1 = strlen(backup_name) ;
            sint_t len2 = strlen(applicationID) ;
            if((len1 >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH) || (len2 >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH))
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_INVALID_PARAMETER ;
                printf("%s Invalid params - path too long (len1=%d len2=%d)\n", FuncName, len1, len2) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_INVALID_PARAMETER ;
            printf("%s Invalid params - type=%d user_no=%d seat_no=%d\n", FuncName, type, user_no, seat_no) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - null pointer passed\n", FuncName) ;
    }

    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s %d <%s> <%s> %d %d\n", 
            FuncName, type, backup_name, applicationID, user_no, seat_no) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_DataBackupCreate ;
        sRequest.type = type ;
        sRequest.user_no = user_no ;
        sRequest.seat_no = seat_no ;
        strcpy(sRequest.path1, backup_name) ;
        strcpy(sRequest.path2, applicationID) ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
 * \brief Allow recovery of from backup on different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param backup_name name of the backup to allow identification
 * \param applicationID the application identifier
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes restored; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataBackupRecovery(PersASSelectionType_e type, const char* backup_name, const char* applicationID, unsigned int user_no, unsigned int seat_no)
{
    pstr_t FuncName = "persAdminDataBackupRecovery:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if((NIL != backup_name) && (NIL != applicationID))
    {
        if(((type >= PersASSelectionType_All) && (type < PersASSelectionType_LastEntry)) /*&& (user_no <= 4) && (seat_no <= 4)*/)
        {
            sint_t len1 = strlen(backup_name) ;
            sint_t len2 = strlen(applicationID) ;
            if((len1 >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH) || (len2 >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH))
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_INVALID_PARAMETER ;
                printf("%s Invalid params - path too long (len1=%d len2=%d)\n", FuncName, len1, len2) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_INVALID_PARAMETER ;
            printf("%s Invalid params - type=%d user_no=%d seat_no=%d\n", FuncName, type, user_no, seat_no) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - null pointer passed\n", FuncName) ;
    }

    if(bEverythingOK)
    {        
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s %d <%s> <%s> %d %d\n", 
            FuncName, type, backup_name, applicationID, user_no, seat_no) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_DataBackupRecovery ;
        sRequest.type = type;
        sRequest.user_no = user_no ;
        sRequest.seat_no = seat_no ;
        strcpy(sRequest.path1, backup_name) ;
        strcpy(sRequest.path2, applicationID) ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
* \brief Allow restore of values from default on different level (application, user or complete)
*
* \param type represents the data to restore: all, application, user
* \param defaultSource source of the default to allow reset
* \param applicationID the application identifier
* \param user_no the user ID
* \param seat_no the seat number (seat 0 to 3)
*
* \return positive value: number of bytes restored; negative value: error code (\ref PAS_RETURNS)
*
* \since V2.1.0
*/
long persAdminDataRestore(PersASSelectionType_e type, PersASDefaultSource_e defaultSource, const char* applicationID, unsigned int user_no, unsigned int seat_no)
{
	pstr_t 		FuncName 		= "persAdminDataRestore:";
	bool_t 		bEverythingOK 	= true;

	persadmin_request_msg_s 	sRequest;
	persadmin_response_msg_s 	sResponse;
	mqd_t 	mqdMsgQueueRequest 	= -1;
	mqd_t 	mqdMsgQueueResponse = -1;
	bool_t 	bSemLocked 			= false;
	sem_t * pSynchSem 			= NIL;
	long 	result 				= -1;
	long 	errorCode 			= PAS_FAILURE;

	/* check params */
	if(NIL != applicationID)
	{
		if(((type >= PersASSelectionType_All) && (type < PersASSelectionType_LastEntry)) &&
		   ((defaultSource >= PersASDefaultSource_Factory) && (defaultSource < PersASDefaultSource_LastEntry)))
		{
			sint_t appNameLength = strlen(applicationID);
			if(appNameLength >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH)
			{
				bEverythingOK = false;
				errorCode = PAS_FAILURE_INVALID_PARAMETER;
				printf("%s Invalid params - path too long (appNameLength=%d)\n", FuncName, appNameLength);
			}
		}
		else
		{
			bEverythingOK = false;
			errorCode = PAS_FAILURE_INVALID_PARAMETER ;
			printf("%s Invalid params - type=%d defaultSource=%d user_no=%d seat_no=%d\n", FuncName, type, defaultSource, user_no, seat_no);
		}
	}
	else
	{
		bEverythingOK = false;
		errorCode = PAS_FAILURE_INVALID_PARAMETER;
		printf("%s Invalid params - null pointer passed\n", FuncName);
	}

	if(bEverythingOK)
	{
		if(persadmin_lockSyncSem(&pSynchSem))
		{
			bSemLocked = true ;
		}
		else
		{
			bEverythingOK = false ;
			errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
		}
	}

	if(bEverythingOK)
	{
		printf("%s %d %d <%s> %d %d\n",
			FuncName, type, defaultSource, applicationID, user_no, seat_no) ;
		mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
		if(mqdMsgQueueRequest >= 0)
		{
			mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
			if(mqdMsgQueueResponse < 0)
			{
				bEverythingOK = false ;
				errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
				printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
			}
		}
		else
		{
			bEverythingOK = false ;
			errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
			printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;
		}
	}

	if(bEverythingOK)
	{
		sRequest.eRequest 		= PAS_Req_DataRestoreToDefault ;
		sRequest.type 			= type;
		sRequest.defaultSource	= defaultSource;
		sRequest.user_no 		= user_no ;
		sRequest.seat_no 		= seat_no ;
		strcpy(sRequest.path2, applicationID) ;

		if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
		{
			/* request sent, now wait for response... */
			sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
			if(sizeof(sResponse) == responseSize)
			{
				result = sResponse.result ;
				printf("%s request result=%ld !!!\n", FuncName, result) ;
			}
			else
			{
				bEverythingOK = false ;
				errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
				printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
			}
		}
	}

	if(mqdMsgQueueRequest >= 0)
	{
		mq_close(mqdMsgQueueRequest) ;
	}
	if(mqdMsgQueueResponse >= 0)
	{
		mq_close(mqdMsgQueueResponse) ;
	}

	if(bSemLocked)
	{
		persadmin_unlockSyncSem(&pSynchSem) ;
	}

	return (bEverythingOK ? result : (errorCode)) ;
}


/**
 * \brief Allow to identify and prepare the data to allow an export from system
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param dst_folder name of the destination folder for the data
 *
 * \return positive value: number of bytes written; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataFolderExport(PersASSelectionType_e type, const char* dst_folder)
{
    pstr_t FuncName = "persAdminDataFolderExport:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if(NIL != dst_folder)
    {
        if((type >= PersASSelectionType_All) && (type < PersASSelectionType_LastEntry))
        {
            sint_t len = strlen(dst_folder) ;
            if(len >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_INVALID_PARAMETER ;
                printf("%s Invalid params - path too long (len=%d)\n", FuncName, len) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_INVALID_PARAMETER ;
            printf("%s Invalid params - type=%d\n", FuncName, type) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - null pointer passed\n", FuncName) ;
    }

    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s %d <%s>\n", 
            FuncName, type, dst_folder) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_DataFolderExport ;
        sRequest.type = type ;
        strcpy(sRequest.path1, dst_folder) ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
 * \brief Allow the import of data from specified folder to the system respecting different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param src_folder name of the source folder of the data
 *
 * \return positive value: number of bytes imported; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminDataFolderImport(PersASSelectionType_e type, const char* src_folder)
{
    pstr_t FuncName = "persAdminDataFolderImport:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if(NIL != src_folder)
    {
        if((type >= PersASSelectionType_All) && (type < PersASSelectionType_LastEntry))
        {
            sint_t len = strlen(src_folder) ;
            if(len >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_INVALID_PARAMETER ;
                printf("%s Invalid params - path too long (len=%d)\n", FuncName, len) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_INVALID_PARAMETER ;
            printf("%s Invalid params - type=%d\n", FuncName, type) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - null pointer passed\n", FuncName) ;
    }

    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s %d <%s>\n", 
            FuncName, type, src_folder) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_DataFolderImport ;
        sRequest.type = type ;
        strcpy(sRequest.path1, src_folder) ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
 * \brief Allow to extend the configuration for persistency of data from specified level (application, user).
 *        Used during new persistency entry installation
 *
 * \param resource_file name of the persistency resource configuration to integrate
 *
 * \return positive value: number of modified entries in the resource configuration; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminResourceConfigAdd(const char* resource_file)
{
    pstr_t FuncName = "persAdminResourceConfigAdd:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if(NIL != resource_file)
    {
        sint_t len = strlen(resource_file) ;
        if(len >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH)
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_INVALID_PARAMETER ;
            printf("%s Invalid params - path too long (len=%d)\n", FuncName, len) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - null pointer passed\n", FuncName) ;
    }

    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s<%s>\n", 
            FuncName, resource_file) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_ResourceConfigAdd ;
        strcpy(sRequest.path1, resource_file) ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
 * \brief Allow the modification of the resource properties from data (key-values and files)
 *
 * \param resource_file name of the persistency resource configuration to integrate
 *
 * \return positive value: number of modified properties in the resource configuration; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminResourceConfigChangeProperties(const char* resource_file)
{
    pstr_t FuncName = "persAdminResourceConfigChangeProperties:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if(NIL != resource_file)
    {
        sint_t len = strlen(resource_file) ;
        if(len >= PERSADMIN_ACCESSLIB_REQUEST_MAX_PATH)
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_INVALID_PARAMETER ;
            printf("%s Invalid params - path too long (len=%d)\n", FuncName, len) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - null pointer passed\n", FuncName) ;
    }

    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s<%s>\n", 
            FuncName, resource_file) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_ResourceConfigChangeProperties ;
        strcpy(sRequest.path1, resource_file) ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
 * \brief Allow the copy of user related data between different users
 *
 * \param src_user_no the user ID source
 * \param src_seat_no the seat number source (seat 0 to 3)
 * \param dest_user_no the user ID destination
 * \param dest_seat_no the seat number destination (seat 0 to 3)
 *
 * \return positive value: number of bytes copied; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminUserDataCopy(unsigned int src_user_no, unsigned int src_seat_no, unsigned int dest_user_no, unsigned int dest_seat_no)
{
    pstr_t FuncName = "persAdminUserDataCopy:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if((src_user_no <= 4) && (src_seat_no <= 4) && (dest_user_no <= 4) && (dest_seat_no <= 4))
    {

    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - src_user_no=%d src_seat_no=%d dest_user_no=%d dest_seat_no=%d \n", FuncName, src_user_no, src_seat_no, dest_user_no, dest_seat_no) ;
    }


    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s %d %d %d %d\n", 
            FuncName, src_user_no, src_seat_no, dest_user_no, dest_seat_no) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_DataBackupCreate ;
        sRequest.src_user_no = src_user_no ;
        sRequest.src_seat_no = src_seat_no ;
        sRequest.dest_user_no = dest_user_no ;
        sRequest.dest_seat_no = dest_seat_no ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}

/**
 * \brief Delete the user related data from persistency containers
 *
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes deleted; negative value: error code (\ref PAS_RETURNS)
 */
long persAdminUserDataDelete(unsigned int user_no, unsigned int seat_no)
{
    pstr_t FuncName = "persAdminUserDataDelete:" ;
    bool_t bEverythingOK = true ;
    persadmin_request_msg_s sRequest ;
    persadmin_response_msg_s sResponse ;
    mqd_t mqdMsgQueueRequest = -1 ;
    mqd_t mqdMsgQueueResponse = -1 ;
    bool_t bSemLocked = false ;
    sem_t * pSynchSem = NIL ;
    long result = -1 ;
    long errorCode = PAS_FAILURE ;

    /* check params */
    if((user_no <= 4) && (seat_no <= 4))
    {

    }
    else
    {
        bEverythingOK = false ;
        errorCode = PAS_FAILURE_INVALID_PARAMETER ;
        printf("%s Invalid params - user_no=%d seat_no=%d \n", FuncName, user_no, seat_no) ;
    }


    if(bEverythingOK)
    {
        if(persadmin_lockSyncSem(&pSynchSem))
        {
            bSemLocked = true ;
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
        }
    }

    if(bEverythingOK)
    {
        printf("%s %d %d\n", 
            FuncName, user_no, seat_no) ;
        mqdMsgQueueRequest = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, O_WRONLY);
        if(mqdMsgQueueRequest >= 0)
        {
            mqdMsgQueueResponse = mq_open(PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, O_RDONLY);
            if(mqdMsgQueueResponse < 0)
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_RESPONSE, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
            printf("%s mq_open(%s) errno=<%s>\n", FuncName, PERSADMIN_ACCESSLIB_MSG_QUEUE_REQUEST, strerror(errno)) ;            
        }
    }

    if(bEverythingOK)
    {
        sRequest.eRequest = PAS_Req_UserDataDelete ;
        sRequest.user_no = user_no ;
        sRequest.seat_no = seat_no ;

        if(0 == mq_send(mqdMsgQueueRequest, (char*)&sRequest, sizeof(sRequest), 0))
        {
            /* request sent, now wait for response... */
            sint_t responseSize = mq_receive(mqdMsgQueueResponse, (char*)&sResponse, sizeof(sResponse), NULL) ;
            if(sizeof(sResponse) == responseSize)
            {
                result = sResponse.result ;
                printf("%s request result=%ld !!!\n", FuncName, result) ;
            }
            else
            {
                bEverythingOK = false ;
                errorCode = PAS_FAILURE_OS_RESOURCE_ACCESS ;
                printf("%s mq_receive returned %d (expected %d)\n", FuncName, responseSize, sizeof(sResponse)) ;
            }
        }
    }

    if(mqdMsgQueueRequest >= 0)
    {
        mq_close(mqdMsgQueueRequest) ;
    }
    if(mqdMsgQueueResponse >= 0)
    {
        mq_close(mqdMsgQueueResponse) ;
    }

    if(bSemLocked)
    {
        persadmin_unlockSyncSem(&pSynchSem) ;
    }

    return (bEverythingOK ? result : (errorCode)) ;
}


static bool_t persadmin_lockSyncSem(sem_t ** ppSynchSem_inout)
{
    pstr_t FuncName = "persadmin_lockSyncSem:" ;
    bool_t bEverythingOK = true ;

    if(NIL == *ppSynchSem_inout)
    {
        *ppSynchSem_inout = sem_open(PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE, O_RDONLY) ;
        if(NIL != *ppSynchSem_inout)
        {
            if(0 != sem_wait(*ppSynchSem_inout))
            {
                bEverythingOK = false ;
                printf("%s sem_wait() errno=<%s> !!!\n", FuncName, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            printf("%s sem_open(%s, O_RDONLY)() errno=<%s> !!!\n", FuncName, PERSADMIN_ACCESSLIB_SYNC_SEMAPHORE, strerror(errno)) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        printf("%s pSynchSem != NULL !!!\n", FuncName) ;
    }
    
    return bEverythingOK ;
}

static bool_t persadmin_unlockSyncSem(sem_t ** ppSynchSem_inout)
{
    pstr_t FuncName = "persadmin_unlockSyncSem:" ;
    bool_t bEverythingOK = true ;

    if(NIL != *ppSynchSem_inout)
    {  
        if(0 == sem_post(*ppSynchSem_inout))
        {
            if(0 == sem_close(*ppSynchSem_inout))
            {
                *ppSynchSem_inout = NIL ;
            }
            else
            {
                bEverythingOK = false ;
                printf("%s sem_close() errno=<%s> !!!\n", FuncName, strerror(errno)) ;
            }
        }
        else
        {
            bEverythingOK = false ;
            printf("%s sem_post() errno=<%s> !!!\n", FuncName, strerror(errno)) ;
        }
    }
    else
    {
        bEverythingOK = false ;
        printf("%s pSynchSem is NULL !!!\n", FuncName) ;
    }

    return bEverythingOK ;
}

