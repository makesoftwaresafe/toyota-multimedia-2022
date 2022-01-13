/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Petrica.Manoila@continental-corporation.com
*
* Implementation of recovery process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.03.20 uidu0250			CSP_WZ#2250:  Provide compress/uncompress functionality 
* 2013.02.15 uidu0250	        CSP_WZ#8849:  Fix persadmin_restore_all_data issue for restoring inexistent apps
* 2013.02.07 uidu0250	        CSP_WZ#2220:  Changed implementation of persadmin_restore_check_RCT_compatibility to remove Helplibs usage
* 2013.02.04 uidu0250	        CSP_WZ#2211:  Modified the implementation to be QAC compliant
* 2013.01.10 uidu0250	        CSP_WZ#2060:  Adapted last database_helper modifications
* 2013.01.10 uidu0250	        CSP_WZ#2060:  Switched to common defines from pers_data_organization_if.h
* 2013.01.04 uidu0250	        CSP_WZ#2060:  Switched from using CRC32 to using CRC16 provided by HelpLibs
* 2012.12.11 uidu0250	        CSP_WZ#1925:  Added RCT compatibility check
* 2012.12.11 uidu0250	        CSP_WZ#1925:  Fixed data_backup_restore issues.
* 2012.12.05 uidu0250	        CSP_WZ#1925:  Ignore the new applications from the backup content for PersASSelectionType_All.
* 2012.11.16 uidu0250           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/ 

/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include "string.h"
#include "stdlib.h"
#include <dlt/dlt.h>

#include "persComDataOrg.h"
#include "persistence_admin_service.h"
#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_database_helper.h"
#include "ssw_pers_admin_common.h"
#include "ssw_pers_admin_service.h"


/* ---------- local defines, macros, constants and type definitions ------------ */
#define PAS_ITEM_LIST_DELIMITER         					'\0'


/* path prefix for destination local cached database/folder: backup_path/Data/mnt-c/<appId>/<database_name or folder_name> */
#define PAS_SRC_LOCAL_CACHE_PATH_FORMAT 					"%s"PERS_ORG_LOCAL_CACHE_PATH_FORMAT

/* path prefix for destination shared group cached database/folder: backup_path/Data/mnt-c/shared/group/<group_no>/<database_name or folder_name>  */
#define PAS_SRC_SHARED_GROUP_CACHE_PATH_STRING_FORMAT 		"%s"PERS_ORG_SHARED_CACHE_PATH_STRING_FORMAT

#define PAS_SHARED_PUBLIC_PATH								PERS_ORG_SHARED_FOLDER_NAME"/"PERS_ORG_PUBLIC_FOLDER_NAME

#define PAS_SHARED_GROUP_PATH								PERS_ORG_SHARED_FOLDER_NAME"/"PERS_ORG_GROUP_FOLDER_NAME

#define	PAS_MAX_LIST_DB_SIZE								(32 * 1024)

#define SUCCESS_CODE              							(0)
#define	GENERIC_ERROR_CODE									PAS_FAILURE;

#define LT_HDR                          					"RESTORE >>"

/* ----------global variables. initialization of global contexts ------------ */
DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);


/* ---------------------- local functions ---------------------------------- */
/**
 * \brief Allow recovery of data from backup on application level ->  (node+user).(application)
 *
 * \param backup_data_path full path of the data to be restored (above /Data folder)
 * \param appId the application identifier
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_appl_data(  pstr_t             backup_data_path,
                      	  	  	  	  	  pstr_t             appId );


/**
 * \brief Allow recovery of data from backup on user level ->  (user).(application+shared)
 *
 * \param backup_data_path full path of the data to be restored (above /Data folder)
 * \param user_no the user ID
 * \param seat_no the seat ID
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_user_data(pstr_t             	backup_data_path,
										uint32_t       		user_no,
										uint32_t    		seat_no );


/**
 * \brief Allow recovery of all data from backup ->  (node+user).(application+shared)
 *
 * \param backup_data_path full path of the data to be restored (above /data folder)
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_all_data(   pstr_t             backup_data_path );


/**
 * \brief Allow recovery of data backup on application level for the node section
 *
 * \param backupDataPath backup root location
 * \param appId the application identifier
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_appl_node(pstr_t    backupDataPath,
                    					pstr_t    appId );

/**
 * \brief Allow recovery of data backup on application level for the user section (all or a specific user)
 *
 * \param backupDataPath backup root location
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_appl_user(  pstr_t          	backupDataPath,
										  pstr_t          	appId,
										  uint32_t   		user_no,
										  uint32_t	  		seat_no);

/**
 * \brief Allow recovery of local user data for the selected user_no (all or a specific user)
 *
 * \param backupDataPath backup root location
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_user_local_data(	pstr_t             		backupDataPath,
												uint32_t     			user_no,
												uint32_t				seat_no);

/**
 * \brief Allow recovery of shared user data for the selected user_no (all or a specific user)
 *
 * \param type restore type
 * \param backupDataPath backup root location
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_user_shared_data(	PersASSelectionType_e 	type,
												pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no);

/**
 * \brief Allow recovery of user public files for the selected user_no (all or a specific user)
 *
 * \param backupDataPath backup root location
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_user_public_files(pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no);
/**
 * \brief Allow recovery of user group files for the selected user_no (all or a specific user)
 *
 * \param backupDataPath backup root location
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_user_group_files(	pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no);

/**
 * \brief Allow recovery of shared data not related to user
 *
 * \param backupDataPath backup root location
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_non_user_shared_data(	pstr_t             	backupDataPath);


/**
 * \brief Allow recovery of local keys for the selected applicationId and/or user_no (all or a specific user)
 *
 * \param type recovery type
 * \param backupDataPath backup root location
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_local_keys( 	PersASSelectionType_e 	type,
											pstr_t          		backupDataPath,
											pstr_t          		appId,
											uint32_t       			user_no,
											uint32_t				seat_no);


/**
 * \brief Allow recovery of shared keys for the selected applicationId and/or user_no (all or a specific user)
 *
 * \param type recovery type
 * \param backupDataPath backup root location
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
static long persadmin_restore_shared_keys( 	PersASSelectionType_e 	type,
											pstr_t          		backupDataPath,
											pstr_t          		appId,
											uint32_t       			user_no,
											uint32_t				seat_no);


/**
 * \brief Checks if the RCT in the source path has the same content as the RCT in the destination path
 *
 * \param sourceRCTPath path of the source RCT file parent folder
 * \param destRCTPath path of the destination RCT file parent folder
 *
 * \return true if the two RCT files have the same content, false otherwise
 */
static bool_t persadmin_restore_check_RCT_compatibility(pstr_t        sourceRCTPath,
                          	  	  	  	  	  	  		pstr_t        destRCTPath );


/**
 * \brief Allow recovery of keys for the selected applicationId and/or user_no (all or a specific user)
 *
 * \param sourceRootPath root location
 * \param applicationId the application identifier
 *
 * \return true if the application exists in the specified sourceRootPath, false otherwise
 */
static bool_t persadmin_restore_check_if_app_exists(	pstr_t        sourceRootPath,
                          	  	  	  	  	  	  	  	pstr_t        appId );

/**
 * \brief Forms the path (by appending) according to the provided user_no and seat_no
 *
 * \param parentUserPath an application/group/public path
 * \param parentUserPathLength the size of the parentUserPath in characters. Should be able to accommodate the additional user/seat info
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return void
 */
static void persadmin_restore_set_user_path(pstr_t    		parentUserPath,
											uint_t			parentUserPathLength,
											uint32_t   		user_no,
											uint32_t		seat_no);

/**
 * \brief Allow recovery of data from backup on different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param backup_name name of the backup to allow identification
 * \param applicationID the application identifier
 * \param user_no the user ID
 * \param seat_no the seat number (seat 0 to 3)
 *
 * \return positive value: number of bytes restored; negative value: error code
 */
long persadmin_data_backup_recovery(PersASSelectionType_e     	type,
                      	  	  	  	char*             			backup_name,
                      	  	  	  	char*             			applicationID,
                      	  	  	  	unsigned int         		user_no,
                      	  	  	  	unsigned int         		seat_no)
{
	sint_t  retVal	= SUCCESS_CODE;
	str_t	pUncompressFromPath	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t	pUncompressToPath	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t	pDataRootPath    	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if((NIL == backup_name) || (NIL == applicationID))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_data_backup_recovery call."));
		return GENERIC_ERROR_CODE;
	}
	else
	{
		(void)memset(pUncompressFromPath, 0, sizeof(pUncompressFromPath));
		(void)snprintf(pUncompressFromPath, sizeof(pUncompressFromPath), "%s%s", BACKUP_LOCATION, backup_name);

		(void)memset(pUncompressToPath, 0, sizeof(pUncompressToPath));
		(void)snprintf(pUncompressToPath, sizeof(pUncompressToPath), "%s", BACKUP_LOCATION);

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("Backup file path:"),
													DLT_STRING(pUncompressFromPath));

		/* Check if the specified backup file exists */
		if( persadmin_check_if_file_exists(pUncompressFromPath, false) < 0)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Backup file does not exist:"),
														DLT_STRING(pUncompressFromPath));
			return GENERIC_ERROR_CODE;
		}

		/* Extract backup data in the same location as the backup file */
		retVal = persadmin_uncompress(pUncompressFromPath, pUncompressToPath);
		if(retVal < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Failed to extract backup content from:"),
														DLT_STRING(pUncompressFromPath),
														DLT_STRING("to"),
														DLT_STRING(pUncompressToPath));
			return retVal;
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
														DLT_STRING("Successfully extracted backup content to:"),
														DLT_STRING(pUncompressToPath));
		}

		/* remove trailing '/' */
		(void)memset(pUncompressToPath, 0, sizeof(pUncompressToPath));
		(void)snprintf(pUncompressToPath, strlen(BACKUP_LOCATION) * sizeof(*pUncompressToPath), "%s", BACKUP_LOCATION);

		switch(type)
		{

		case PersASSelectionType_All:
		{
			retVal = persadmin_restore_all_data(  pUncompressToPath );
		}
		break;

		case PersASSelectionType_User:
		{
			retVal = persadmin_restore_user_data(	pUncompressToPath,
													user_no,
													seat_no);
		}
		  break;

		case PersASSelectionType_Application:
		{
			retVal = persadmin_restore_appl_data( 	pUncompressToPath,
													applicationID );
		}
		break;

		default:
			retVal = GENERIC_ERROR_CODE;
		break; // redundant
		}

		if(SUCCESS_CODE <= retVal)
		{
			/* <backupDataPath>/Data */
			(void)snprintf(pDataRootPath, sizeof(pDataRootPath), "%s", BACKUP_LOCATION);
			(void)snprintf(pDataRootPath + strlen(BACKUP_LOCATION) - 1, (sizeof(pDataRootPath) - strlen(BACKUP_LOCATION) + 1) * sizeof(*pDataRootPath), "%s", PERS_ORG_ROOT_PATH);
			if( 0 > persadmin_delete_folder(pDataRootPath))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
															DLT_STRING("Failed to delete folder :"),
															DLT_STRING(pDataRootPath));
			}
			else
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
															DLT_STRING("Successfully deleted folder:"),
															DLT_STRING(pDataRootPath));
			}
		}
	}

    return retVal;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//---------------------------------------------------------------
static void persadmin_restore_set_user_path(pstr_t    		parentUserPath,
											uint_t			parentUserPathLength,
											uint32_t   		user_no,
											uint32_t		seat_no)
{
	pstr_t       	pTempSrcUserPath    = NIL;

	pTempSrcUserPath = (pstr_t)malloc(parentUserPathLength * sizeof(*parentUserPath));/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pTempSrcUserPath)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
													DLT_STRING("Failed to allocate memory for user path."));
	}
	else
	{
		if(NIL != parentUserPath)
		{
			// all users & all seats;
			if( (PERSIST_SELECT_ALL_USERS == user_no) && (PERSIST_SELECT_ALL_SEATS == seat_no) )
			{
				 /* <parentUserPath>/user */
				(void)snprintf(pTempSrcUserPath, parentUserPathLength, "%s%s", parentUserPath, gUser);
			}
			// only one user & all seats;
			if( (PERSIST_SELECT_ALL_USERS != user_no) && (PERSIST_SELECT_ALL_SEATS == seat_no) )
			{
				/* <parentUserPath>/user/<user> */
				(void)snprintf(pTempSrcUserPath, parentUserPathLength, "%s%s%d", parentUserPath, gUser, user_no);
			}
			// only one user & one seat;
			if( (PERSIST_SELECT_ALL_USERS != user_no) && (PERSIST_SELECT_ALL_SEATS != seat_no) )
			{
				/* <parentUserPath>/user/<user_no>/seat/<seat_no> */
				(void)snprintf(pTempSrcUserPath, parentUserPathLength, "%s%s%d%s%d", parentUserPath, gUser, user_no, gSeat, seat_no);
			}

			(void)snprintf(parentUserPath, parentUserPathLength, "%s", pTempSrcUserPath);
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
														DLT_STRING("Invalid parameter in persadmin_restore_set_user_path call."));
		}

		free(pTempSrcUserPath);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	}
}


//---------------------------------------------------------------
static long persadmin_restore_appl_node(pstr_t    backupDataPath,
                    					pstr_t    appId )			// could use an 'extended' app path
{
	sint_t    	retVal;
	long	  	bytesRestored		= 0;
	str_t	  	pNodeSourcePath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pNodeDestPath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if((NIL == backupDataPath) || (NIL == appId))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_appl_node call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore node content for:"),
												DLT_STRING(appId),
												DLT_STRING("from"),
												DLT_STRING(backupDataPath));

	/* app node source path */

	/* <backupDataPath>/Data/mnt-c/<appId>/node */
	(void)snprintf(pNodeSourcePath, sizeof(pNodeSourcePath), PAS_SRC_LOCAL_CACHE_PATH_FORMAT, backupDataPath, appId, gNode);


	/* app node dest path */

	/* /Data/mnt-c/<appId>/node */
	(void)snprintf(pNodeDestPath, sizeof(pNodeDestPath), gLocalCachePath, appId, gNode);

	/* erase node content */
	retVal = persadmin_delete_folder(pNodeDestPath);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_delete_folder call failed with error code:"),
													DLT_INT(retVal),
													DLT_STRING("for"),
													DLT_STRING(pNodeDestPath));
	}

	/* copy node content */
	retVal = persadmin_copy_folder(	pNodeSourcePath,
									pNodeDestPath,
									PersadminFilterAll,
									true);

	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_copy_folder call failed with error code:"),
													DLT_INT(retVal),
													DLT_STRING("for source:"),
													DLT_STRING(pNodeSourcePath),
													DLT_STRING("and destination:"),
													DLT_STRING(pNodeDestPath));
		return GENERIC_ERROR_CODE;
	}

	bytesRestored += retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully node content for:"),
												DLT_STRING(appId),
												DLT_STRING("from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored."));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/



//-----------------------------------------------------------------------
static long persadmin_restore_appl_user(  pstr_t          	backupDataPath,
										  pstr_t          	appId,
										  uint32_t 			user_no,
										  uint32_t 			seat_no)
{
	sint_t  retVal;
	long	bytesRestored		= 0;
	str_t	pUserSourcePath   	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t   pUserDestPath   	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];


	if((NIL == backupDataPath) || (NIL == appId))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_appl_user call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore user content for App:"),
												DLT_STRING(appId),
												DLT_STRING("User:"),
												DLT_UINT8(user_no),
												DLT_STRING("Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("from"),
												DLT_STRING(backupDataPath));

	/* user source path */

	/* <backupDataPath>/Data/mnt-c/<appId>/user/<user_no>/seat/<seat_no> */
	(void)snprintf(pUserSourcePath, sizeof(pUserSourcePath), PAS_SRC_LOCAL_CACHE_PATH_FORMAT, backupDataPath, appId, "");
	persadmin_restore_set_user_path(	pUserSourcePath,
										sizeof(pUserSourcePath)/sizeof(str_t),
										user_no,
										seat_no);

	/* user dest path */

	/* /Data/mnt-c/<appId>/user/<user_no>/seat/<seat_no> */
	(void)snprintf(pUserDestPath, sizeof(pUserDestPath), gLocalCachePath, appId, "");
	persadmin_restore_set_user_path(pUserDestPath,
									sizeof(pUserDestPath)/sizeof(str_t),
									user_no,
									seat_no);

	/* erase user content */
	retVal = persadmin_delete_folder(pUserDestPath);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_delete_folder call failed with error code:"),
													DLT_INT(retVal),
													DLT_STRING("for"),
													DLT_STRING(pUserDestPath));
	}

	/* copy user content */
	retVal = persadmin_copy_folder( pUserSourcePath,
									pUserDestPath,
									PersadminFilterAll,
									true);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_copy_folder call failed with error code:"),
													DLT_INT(retVal),
													DLT_STRING("from"),
													DLT_STRING(pUserSourcePath),
													DLT_STRING("to"),
													DLT_STRING(pUserDestPath));
		return GENERIC_ERROR_CODE;
	}

	bytesRestored += retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user content for App:"),
												DLT_STRING(appId),
												DLT_STRING("User:"),
												DLT_UINT8(user_no),
												DLT_STRING("Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored."));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//-------------------------------------------------------------------------------------
static long persadmin_restore_non_user_shared_data(	pstr_t             	backupDataPath)
{
	sint_t    	retVal;
	long	  	bytesRestored			= 0;
	str_t    	pGroupRootSourcePath	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pExtendedAppId   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];	// group path
	sint_t    	listBuffSize    		= 0;
	pstr_t    	pStrList      			= NIL;
	pstr_t    	pItemName     			= NIL;
	sint_t    	outBuffSize     		= 0;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore non-user shared data from"),
												DLT_STRING(backupDataPath));


	/* --- public file/folder restore --- */
	retVal = persadmin_restore_appl_node(	backupDataPath,
											PAS_SHARED_PUBLIC_PATH );
	if(retVal < 0)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_appl_node call failed with error code:"),
													DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}
	bytesRestored += retVal;


	/* --- group file/folder restore --- */

	/* <backupDataPath>/Data/mnt-c/shared/group */
	(void)snprintf(pGroupRootSourcePath, sizeof(pGroupRootSourcePath), "%s%s", backupDataPath, PERS_ORG_SHARED_GROUP_CACHE_PATH);


	/* Check all groups */
	listBuffSize = persadmin_list_folder_get_size(  pGroupRootSourcePath,
													PersadminFilterFolders,
													false );
	if(listBuffSize < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_folder_get_size call failed with error code:"),
													DLT_INT(listBuffSize));
		return GENERIC_ERROR_CODE;
	}
	if(listBuffSize > 0)
	{
		pStrList = NIL;
		pStrList = (pstr_t)malloc((uint_t)listBuffSize); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		if(NIL == pStrList)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Error allocating memory for list of folders."));
			return GENERIC_ERROR_CODE;
		}
		(void)memset(pStrList, 0, (uint_t)listBuffSize);

		outBuffSize = persadmin_list_folder(  	pGroupRootSourcePath,
												pStrList,
												listBuffSize,
												PersadminFilterFolders,
												false);
		if(outBuffSize < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Error obtaining the list of folders."));
			free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pStrList = NIL;
			return GENERIC_ERROR_CODE;
		}

		pItemName = pStrList;
		while(listBuffSize > 0)
		{
			if(0 == strlen(pItemName))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("Invalid folder item found."));
				break;
			}

			/* Restore the node content for every group */

			/* shared/group/<group_id> */
			(void)snprintf(pExtendedAppId, sizeof(pExtendedAppId), "%s/%s/%s", PERS_ORG_SHARED_FOLDER_NAME, PERS_ORG_GROUP_FOLDER_NAME, pItemName);

			retVal = persadmin_restore_appl_node(	backupDataPath,
													pExtendedAppId);
			if(retVal < 0)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_restore_appl_node call failed with error code:"),
															DLT_INT(retVal));
				free(pStrList);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pStrList = NIL;
				return GENERIC_ERROR_CODE;
			}

			bytesRestored += retVal;

			listBuffSize -= ((sint_t)strlen(pItemName) + 1) * (sint_t)sizeof(*pItemName);
			pItemName += (strlen(pItemName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
		}

		free(pStrList);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pStrList = NIL;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore non-user shared data from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//--------------------------------------------------------------------------
static long persadmin_restore_user_public_files(pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no)
{
	sint_t    	retVal;
	long	  	bytesRestored			= 0;
	str_t		pPublicSourcePath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pPublicDestPath			[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pUserSourcePath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pUserDestPath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];


	if(NIL == backupDataPath)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_user_public_files call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore user public files from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));


	/* --- public RCT compatibility check --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("public RCT compatibility check..."));

	/* <backupDataPath>/Data/mnt-c/shared/public */
	(void)snprintf(pPublicSourcePath, sizeof(pPublicSourcePath), "%s%s", backupDataPath, PERS_ORG_SHARED_PUBLIC_CACHE_PATH);

	/* /Data/mnt-c/shared/public */
	(void)snprintf(pPublicDestPath, sizeof(pPublicDestPath), "%s", PERS_ORG_SHARED_PUBLIC_CACHE_PATH);


	if(false == persadmin_restore_check_RCT_compatibility(pPublicSourcePath, pPublicDestPath))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Incompatible public RCT files:"),
													DLT_STRING(pPublicSourcePath),
													DLT_STRING("<->"),
													DLT_STRING(pPublicDestPath));
		return GENERIC_ERROR_CODE;
	}

	/* --- public file/folder restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("public file/folder restore..."));

	/* <backupDataPath>/Data/mnt-c/shared/public/user/<user_no>/seat/<seat_no> */
	(void)snprintf(pUserSourcePath, sizeof(pUserSourcePath), "%s%s", backupDataPath, PERS_ORG_SHARED_PUBLIC_CACHE_PATH);
	persadmin_restore_set_user_path(pUserSourcePath,
									sizeof(pUserSourcePath)/sizeof(str_t),
									user_no,
									seat_no);

	/* /Data/mnt-c/shared/public/user/<user_no>/seat/<seat_no> */
	(void)snprintf(pUserDestPath, sizeof(pUserDestPath), "%s", PERS_ORG_SHARED_PUBLIC_CACHE_PATH);
	persadmin_restore_set_user_path(pUserDestPath,
									sizeof(pUserDestPath)/sizeof(str_t),
									user_no,
									seat_no);

	/* erase user content */
	retVal = persadmin_delete_folder(pUserDestPath);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_delete_folder call failed with error code:"),
													DLT_INT(retVal));
	}

	/* copy user content */
	retVal = persadmin_copy_folder(	pUserSourcePath,
									pUserDestPath,
									PersadminFilterAll,
									true);
	if(retVal < 0)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_copy_folder call failed with error code:"),
													DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}

	bytesRestored += retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user public files from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

//--------------------------------------------------------------------------
static long persadmin_restore_user_group_files(	pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no)
{
	sint_t    	retVal;
	long	  	bytesRestored			= 0;
	str_t    	pGroupRootSourcePath	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pGroupSourcePath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pGroupDestPath			[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pUserSourcePath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pUserDestPath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	pstr_t    	pStrList      			= NIL;
	pstr_t    	pItemName     			= NIL;
	sint_t    	listBuffSize    		= 0;
	sint_t    	outBuffSize     		= 0;

	if(NIL == backupDataPath)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_user_group_files call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore user group files from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* <backupDataPath>/Data/mnt-c/shared/group */
	(void)snprintf(pGroupRootSourcePath, sizeof(pGroupRootSourcePath), "%s%s", backupDataPath, PERS_ORG_SHARED_GROUP_CACHE_PATH);

	/* Check all groups */
	listBuffSize = persadmin_list_folder_get_size(  pGroupRootSourcePath,
													PersadminFilterFolders,
													false );
	if(listBuffSize < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_folder_get_size call failed with error code:"),
													DLT_INT(listBuffSize));
		return GENERIC_ERROR_CODE;
	}
	if(listBuffSize > 0)
	{
		pStrList = NIL;
		pStrList = (pstr_t)malloc((uint_t)listBuffSize); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		if(NIL == pStrList)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Error allocating memory for list of folders."));
			return GENERIC_ERROR_CODE;
		}
		(void)memset(pStrList, 0, (uint_t)listBuffSize);

		outBuffSize = persadmin_list_folder(  	pGroupRootSourcePath,
												pStrList,
												listBuffSize,
												PersadminFilterFolders,
												false);
		if(outBuffSize < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Error obtaining the list of folders."));
			free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pStrList = NIL;
			return GENERIC_ERROR_CODE;
		}

		pItemName = pStrList;
		while(listBuffSize > 0)
		{
			if(0 == strlen(pItemName))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("Invalid group name found."));
				break;
			}

			/* Restore the user content for every group */
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,	DLT_STRING(LT_HDR),
														DLT_STRING("restore user content for group"),
														DLT_STRING(pItemName),
														DLT_STRING(" ..."));

			/* --- group RCT compatibility check --- */
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
														DLT_STRING("RCT compatibility check for group"),
														DLT_STRING(pItemName),
														DLT_STRING("..."));

			/* <backupDataPath>/Data/mnt-c/shared/group/<group_id> */
			(void)snprintf(pGroupSourcePath, sizeof(pGroupSourcePath), PAS_SRC_SHARED_GROUP_CACHE_PATH_STRING_FORMAT, backupDataPath, pItemName, "");

			/*/Data/mnt-c/shared/group/<group_id> */
			(void)snprintf(pGroupDestPath, sizeof(pGroupDestPath), PERS_ORG_SHARED_CACHE_PATH_STRING_FORMAT, pItemName, "");

			if(false == persadmin_restore_check_RCT_compatibility(pGroupSourcePath, pGroupDestPath))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("Incompatible group RCT files:"),
															DLT_STRING(pGroupSourcePath),
															DLT_STRING("<->"),
															DLT_STRING(pGroupDestPath));
				free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pStrList = NIL;
				return GENERIC_ERROR_CODE;
			}

			/* <backupDataPath>/Data/mnt-c/shared/group/<group_id>/user/<user_no>/seat/<seat_no> */
			(void)snprintf(pUserSourcePath, sizeof(pUserSourcePath), PAS_SRC_SHARED_GROUP_CACHE_PATH_STRING_FORMAT, backupDataPath, pItemName, "");
			persadmin_restore_set_user_path(pUserSourcePath,
											sizeof(pUserSourcePath) / sizeof(str_t),
											user_no,
											seat_no);

			/* /Data/mnt-c/shared/group/<group_id>/user/<user_no>/seat/<seat_no> */
			(void)snprintf(pUserDestPath, sizeof(pUserDestPath), PERS_ORG_SHARED_CACHE_PATH_STRING_FORMAT, pItemName, "");
			persadmin_restore_set_user_path(pUserDestPath,
											sizeof(pUserDestPath) / sizeof(str_t),
											user_no,
											seat_no);

			/* erase user content */
			retVal = persadmin_delete_folder(pUserDestPath);
			if(retVal < SUCCESS_CODE)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_delete_folder call failed with error code:"),
															DLT_INT(retVal));
			}

			/* copy user content */
			retVal = persadmin_copy_folder( pUserSourcePath,
											pUserDestPath,
											PersadminFilterAll,
											true);

			if(retVal < 0)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_copy_folder call failed with error code:"),
															DLT_INT(retVal));
				free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pStrList = NIL;
				return GENERIC_ERROR_CODE;
			}

			bytesRestored += retVal;

			listBuffSize -= ((sint_t)strlen(pItemName) + 1) * (sint_t)sizeof(*pItemName);
			pItemName += (strlen(pItemName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
		}

		free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pStrList = NIL;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user group files from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//--------------------------------------------------------------------------
static long persadmin_restore_user_local_data(	pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no)
{
	sint_t    	siRetVal      		= 0;
	sint_t		listBuffSize		= 0;
	long    	retVal        		= 0;
	long    	bytesRestored  		= 0;
	pstr_t    	pAppNameList    	= NIL;
	pstr_t    	pAppName      		= NIL;
	str_t		pAppSourcePath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pAppDestPath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];


	if(NIL == backupDataPath)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_user_local_data call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore user local data from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* Get list of application names */
	siRetVal = persadmin_list_application_folders_get_size(backupDataPath);
	if(siRetVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders_get_size call failed with error code:"),
													DLT_INT(siRetVal));
		return GENERIC_ERROR_CODE;
	}

	if(0 == siRetVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
													DLT_STRING("No application found."));
		return siRetVal;
	}

	listBuffSize = siRetVal;

	pAppNameList = NIL;
	pAppNameList = (pstr_t) malloc((uint_t)listBuffSize); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pAppNameList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for App name list."));
		return GENERIC_ERROR_CODE;
	}
	(void)memset(pAppNameList, 0, (uint_t)listBuffSize);

	siRetVal = persadmin_list_application_folders(backupDataPath,
												  pAppNameList,
												  listBuffSize );

	if((siRetVal <= SUCCESS_CODE) || (siRetVal > listBuffSize))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders call failed with error code:"),
													DLT_INT(siRetVal));
		free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pAppNameList = NIL;
		return GENERIC_ERROR_CODE;
	}

	listBuffSize = siRetVal;
	pAppName  = pAppNameList;
	while(listBuffSize > 0)
	{
		if(0 == strlen(pAppName))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Invalid app name found."));
			break;
		}

		/* <backupDataPath>/Data/mnt-c/<appId> */
		(void)snprintf(pAppSourcePath, sizeof(pAppSourcePath), PAS_SRC_LOCAL_CACHE_PATH_FORMAT, backupDataPath, pAppName, "");

		/* /Data/mnt-c/<appId> */
		(void)snprintf(pAppDestPath, sizeof(pAppDestPath), PERS_ORG_LOCAL_CACHE_PATH_FORMAT, pAppName, "");

		/* --- RCT compatibility check --- */
		if(false == persadmin_restore_check_RCT_compatibility(pAppSourcePath, pAppDestPath))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Incompatible App RCT files:"),
														DLT_STRING(pAppSourcePath),
														DLT_STRING("<->"),
														DLT_STRING(pAppDestPath));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return GENERIC_ERROR_CODE;
		}


		/* user_no for every app */
		retVal = persadmin_restore_appl_user( 	backupDataPath,
												pAppName,
												user_no,
												seat_no);
		if(retVal < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_appl_user call failed with error code:"),
														DLT_INT(retVal));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return GENERIC_ERROR_CODE;
		}
		bytesRestored += retVal;


		/* user_no for local keys */
		retVal = persadmin_restore_local_keys( 	PersASSelectionType_User,
												backupDataPath,
												pAppName,
												user_no,
												seat_no);

		if(retVal < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_local_keys call failed with error code:"),
														DLT_INT(retVal));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return GENERIC_ERROR_CODE;
		}
		bytesRestored += retVal;


		/* links to groups and public */
		siRetVal = persadmin_import_links(pAppSourcePath,
										  pAppDestPath );

		if(retVal < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_import_links call failed with error code:"),
														DLT_INT(retVal));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return GENERIC_ERROR_CODE;
		}
		bytesRestored += siRetVal;

		listBuffSize -= ((sint_t)strlen(pAppName) + 1) * (sint_t)sizeof(*pAppName);
		pAppName += (strlen(pAppName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}

	free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pAppNameList = NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user local data from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

//--------------------------------------------------------------------------
static long persadmin_restore_user_shared_data(	PersASSelectionType_e 	type,
												pstr_t             		backupDataPath,
												uint32_t       			user_no,
												uint32_t				seat_no)
{
	sint_t    	retVal;
	long	  	bytesRestored			= 0;


	if(NIL == backupDataPath)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_user_shared_data call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore (of type"),
												DLT_INT(type),
												DLT_STRING(") user shared data from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));


	/* --- public user files restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("public file/folder restore..."));

	retVal = persadmin_restore_user_public_files(	backupDataPath,
													user_no,
													seat_no);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_user_public_files call failed with error code:"),
													DLT_INT(retVal));
	}

	bytesRestored += retVal;


	/* --- group user files restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("group file/folder restore..."));

	retVal = persadmin_restore_user_group_files(backupDataPath,
												user_no,
												seat_no);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_user_group_files call failed with error code:"),
													DLT_INT(retVal));
	}

	bytesRestored += retVal;


	/* --- shared key-value restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("restore shared keys for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("..."));

	retVal = persadmin_restore_shared_keys(	type,
											backupDataPath,
											"",
											user_no,
											seat_no);

	if(retVal < 0)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_shared_keys call failed with error code:"),
													DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}

	bytesRestored += retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user shared data from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//-------------------------------------------------------------------------
static long persadmin_restore_local_keys( 	PersASSelectionType_e 	type,
											pstr_t          		backupDataPath,
											pstr_t          		appId,
											uint32_t       			user_no,
											uint32_t				seat_no)
{
	sint_t    	siRetVal;
	long	  	bytesRestored		= 0;
	str_t		pDBFileName     	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pDBFileName_		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pDestDBPath     	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	pstr_t    	pSourceDBPath   	= NIL;
	pstr_t    	pDBList       		= NIL;

	if((NIL == backupDataPath) || (NIL == appId))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_local_keys call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore local keys from "),
												DLT_STRING(backupDataPath),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	// alloc. a fixed size for DB list
	pDBList = NIL;
	pDBList = (pstr_t) malloc(PAS_MAX_LIST_DB_SIZE); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pDBList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for DB list."));
		return GENERIC_ERROR_CODE;
	}
	(void)memset(pDBList, 0, PAS_MAX_LIST_DB_SIZE);


	siRetVal = persadmin_get_all_db_paths_with_names( backupDataPath,
													  appId,
													  PersistenceStorage_local,
													  pDBList,
													  PAS_MAX_LIST_DB_SIZE);

	if(siRetVal <= 0)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_get_all_db_paths_with_names call failed with error code:"),
													DLT_INT(siRetVal));
		free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pDBList = NIL;
		return GENERIC_ERROR_CODE;
	}

	pSourceDBPath   = pDBList;
	while(PAS_ITEM_LIST_DELIMITER != (*pSourceDBPath))
	{
		(void)memset(pDBFileName, 0, sizeof(pDBFileName));

		siRetVal = persadmin_get_filename(pSourceDBPath,
										  pDBFileName,
										  sizeof(pDBFileName));

		if(siRetVal < 0)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_get_filename call failed with error code:"),
														DLT_INT(siRetVal));
			free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pDBList = NIL;
			return GENERIC_ERROR_CODE;
		}

		(void)snprintf(pDBFileName_, sizeof(pDBFileName_), "/%s", pDBFileName);

		/* Form a local path */
		(void)snprintf(pDestDBPath, sizeof(pDestDBPath), PERS_ORG_LOCAL_CACHE_PATH_FORMAT, appId, pDBFileName_);

		// delete the keys in the destination DB only if the source DB exists
		if(SUCCESS_CODE == persadmin_check_if_file_exists(	pSourceDBPath, false))
		{
			// remove current keys before transferring keys from backup content
			siRetVal = persadmin_delete_keys_by_filter(	type,
														pDestDBPath,
														user_no,
														seat_no);
			if(siRetVal < SUCCESS_CODE)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_delete_keys_by_filter call failed with error code:"),
															DLT_INT(siRetVal));
			}
		}

		// transfer the keys from the backup content
		if(SUCCESS_CODE == persadmin_check_if_file_exists(	pSourceDBPath, false))
		{
			siRetVal = persadmin_copy_keys_by_filter( 	type,
														pDestDBPath,
														pSourceDBPath,
														user_no,
														seat_no );
			if(siRetVal < SUCCESS_CODE)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_copy_keys_by_filter call failed with error code:"),
															DLT_INT(siRetVal));
				free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pDBList = NIL;
				return GENERIC_ERROR_CODE;
			}

			bytesRestored += siRetVal;
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
														DLT_STRING("Source database does not exist (ignore):"),
														DLT_STRING(pSourceDBPath));
		}

		pSourceDBPath += (strlen(pSourceDBPath) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}

	free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pDBList=NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully local keys from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

//-------------------------------------------------------------------------
static long persadmin_restore_shared_keys( 	PersASSelectionType_e 	type,
											pstr_t          		backupDataPath,
											pstr_t          		appId,
											uint32_t       			user_no,
											uint32_t				seat_no)
{
	sint_t    	siRetVal;
	long	  	bytesRestored		= 0;
	str_t		pDBFileName     	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pDBFileName_		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pDestDBPath     	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	pstr_t    	pSourceDBPath   	= NIL;
	pstr_t    	pDBList       		= NIL;
	pstr_t	  	pSharedSubstr		= NIL;

	if((NIL == backupDataPath) || (NIL == appId))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_shared_keys call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore shared keys from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	// alloc. a fixed size for DB list
	pDBList = NIL;
	pDBList = (pstr_t) malloc(PAS_MAX_LIST_DB_SIZE); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pDBList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for DB list."));
		return GENERIC_ERROR_CODE;
	}
	(void)memset(pDBList, 0, PAS_MAX_LIST_DB_SIZE);


	siRetVal = persadmin_get_all_db_paths_with_names( backupDataPath,
													  appId,
													  PersistenceStorage_shared,
													  pDBList,
													  PAS_MAX_LIST_DB_SIZE);

	if(siRetVal <= 0)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_get_all_db_paths_with_names call failed with error code:"),
													DLT_INT(siRetVal));
		free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pDBList = NIL;
		return GENERIC_ERROR_CODE;
	}

	pSourceDBPath   = pDBList;
	while(PAS_ITEM_LIST_DELIMITER != (*pSourceDBPath))
	{
		(void)memset(pDBFileName, 0, sizeof(pDBFileName));

		siRetVal = persadmin_get_filename(pSourceDBPath,
										  pDBFileName,
										  sizeof(pDBFileName));

		if(siRetVal < 0)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_get_filename call failed with error code:"),
														DLT_INT(siRetVal));
			free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pDBList = NIL;
			return GENERIC_ERROR_CODE;
		}

		(void)snprintf(pDBFileName_, sizeof(pDBFileName_), "/%s", pDBFileName);

		/* Form a shared path */
		pSharedSubstr = NIL;
		/* public DB */
		pSharedSubstr = strstr(pSourceDBPath, PAS_SHARED_PUBLIC_PATH);
		if(NIL == pSharedSubstr)
		{
			/* group DB */
			pSharedSubstr =  strstr(pSourceDBPath, PAS_SHARED_GROUP_PATH);
		}

		if(NIL != pSharedSubstr)
		{
			(void)snprintf(pDestDBPath, sizeof(pDestDBPath), "%s%s", PERS_ORG_LOCAL_APP_CACHE_PATH_, pSharedSubstr);
		}

		// transfer the keys from the backup content
		if(SUCCESS_CODE == persadmin_check_if_file_exists(	pSourceDBPath, false))
		{
			siRetVal = persadmin_copy_keys_by_filter( 	type,
														pDestDBPath,
														pSourceDBPath,
														user_no,
														seat_no );
			if(siRetVal < SUCCESS_CODE)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_copy_keys_by_filter call failed with error code:"),
															DLT_INT(siRetVal));
				free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pDBList = NIL;
				return GENERIC_ERROR_CODE;
			}

			bytesRestored += siRetVal;
		}
		else
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
														DLT_STRING("Source database does not exist (ignore):"),
														DLT_STRING(pSourceDBPath));
		}

		pSourceDBPath += (strlen(pSourceDBPath) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}

	free(pDBList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pDBList=NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully shared keys from"),
												DLT_STRING(backupDataPath),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/



//-------------------------------------------------------------------------------
static bool_t persadmin_restore_check_if_app_exists(pstr_t        sourceRootPath,
                          	  	  	  	  	  	  	pstr_t        appId )
{
	str_t    pAppPath  		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if((NIL == sourceRootPath) || (NIL == appId))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_check_if_app_exists call."));
		return false;
	}

	/* <sourceRootPath>/Data/mnt-c/<appId> */
	(void)snprintf(pAppPath, sizeof(pAppPath), PAS_SRC_LOCAL_CACHE_PATH_FORMAT, sourceRootPath, appId, "");

	if(SUCCESS_CODE == persadmin_check_if_file_exists(	pAppPath, true))
	{
		return true;
	}

	return false;
}

//---------------------------------------------------------------------------------
static bool_t persadmin_restore_check_RCT_compatibility(pstr_t        sourceRCTPath,
                          	  	  	  	  	  	  		pstr_t        destRCTPath )
{
	str_t    	pSrcRCTPath  		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pDestRCTPath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if((NIL == sourceRCTPath) || (NIL == destRCTPath))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_check_RCT_compatibility call."));
		return false;
	}


	/* path to source RCT file */
	(void)snprintf(pSrcRCTPath, sizeof(pSrcRCTPath), "%s%s", sourceRCTPath, gResTableCfg);

	/* path to dest RCT file */
	(void)snprintf(pDestRCTPath, sizeof(pDestRCTPath), "%s%s", destRCTPath, gResTableCfg);

	return 	persadmin_check_for_same_file_content(pSrcRCTPath, pDestRCTPath);
}


//---------------------------------------------------------------------------
static long persadmin_restore_appl_data(  pstr_t             backup_data_path,
                      	  	  	  	  	  pstr_t             appId )
{
	long    	retVal        		= SUCCESS_CODE;
	long    	bytesRestored   	= 0;
	str_t    	pAppSourcePath    	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t   	pAppDestPath    	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if((NIL == backup_data_path) || (NIL == appId))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_appl_data call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore app data from"),
												DLT_STRING(backup_data_path),
												DLT_STRING("for App:"),
												DLT_STRING(appId));


	/* <backupDataPath>/Data/mnt-c/<appId> */
	(void)snprintf(pAppSourcePath, sizeof(pAppSourcePath), PAS_SRC_LOCAL_CACHE_PATH_FORMAT, backup_data_path, appId, "");

	/* /Data/mnt-c/<appId> */
	(void)snprintf(pAppDestPath, sizeof(pAppDestPath), PERS_ORG_LOCAL_CACHE_PATH_FORMAT, appId, "");


	/* --- RCT compatibility check --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("RCT compatibility check for App:"),
												DLT_STRING(appId),
												DLT_STRING("..."));

	if(false == persadmin_restore_check_RCT_compatibility(pAppSourcePath, pAppDestPath))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Incompatible App RCT files."));
		return GENERIC_ERROR_CODE;
	}


	/* --- local file/folder restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local file/folder restore..."));


	/* Check if the specified application data exists */
	if(true == persadmin_restore_check_if_app_exists(	(pstr_t)backup_data_path,
														(pstr_t)appId) )
	{
		/* restore node content */
		retVal = persadmin_restore_appl_node( 	backup_data_path,
												(pstr_t)appId);
		if(retVal < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_appl_node call failed with error code:"),
														DLT_INT(retVal));
			return GENERIC_ERROR_CODE;
		}
		else
		{
			bytesRestored += retVal;
		}

		/* restore user content (for all users) */
		retVal = persadmin_restore_appl_user(	backup_data_path,
												(pstr_t)appId,
												PERSIST_SELECT_ALL_USERS,
												PERSIST_SELECT_ALL_SEATS);
		if(retVal < SUCCESS_CODE)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_appl_user call failed with error code:"),
														DLT_INT(retVal));
			return GENERIC_ERROR_CODE;
		}
		else
		{
			bytesRestored += retVal;
		}
	}


	/* --- local key-value restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local key/value restore..."));

	retVal = persadmin_restore_local_keys(	PersASSelectionType_Application,
											backup_data_path,
											appId,
											PERSIST_SELECT_ALL_USERS,
											PERSIST_SELECT_ALL_SEATS);

	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_local_keys call failed with error code:"),
													DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}
	else
	{
		bytesRestored += retVal;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully app data from"),
												DLT_STRING(backup_data_path),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/



//---------------------------------------------------------------------------
static long persadmin_restore_user_data(pstr_t             	backup_data_path,
										uint32_t       		user_no,
										uint32_t    		seat_no )
{
	long    	retVal        		= 0;
	long    	bytesRestored  		= 0;


	if(NIL == backup_data_path)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_user_data call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore user data from"),
												DLT_STRING(backup_data_path),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* --- local user data restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local user data restore..."));

	retVal = persadmin_restore_user_local_data(	backup_data_path,
												user_no,
												seat_no);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_user_local_data call failed with error code:"),
												  	DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}

	bytesRestored += retVal;


	/* --- shared user data restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("shared user data restore..."));

	retVal = persadmin_restore_user_shared_data(PersASSelectionType_User,
												backup_data_path,
	                      	  	  	  	  		user_no,
	                      	  	  	  	  		seat_no);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_user_shared_data call failed with error code:"),
													DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}

	bytesRestored += retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user data from"),
												DLT_STRING(backup_data_path),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/



//---------------------------------------------------------------------------
static long persadmin_restore_all_data(   pstr_t             backup_data_path )
{
	sint_t		siRetVal      		= 0;
	sint_t		listBuffSize		= 0;
	long    	retVal        		= 0;
	long    	bytesRestored   	= 0;
	pstr_t    	pAppNameList    	= NIL;
	pstr_t    	pAppName      		= NIL;
	str_t    	pAppSourcePath    	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pAppDestPath    	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];


	if(NIL == backup_data_path)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_all_data call."));
		return GENERIC_ERROR_CODE;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore all data from"),
												DLT_STRING(backup_data_path));


	/* --- local data restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local data restore..."));

	/* Get list of backup application names */
	/* Only the existing applications will be restored, no new applications will be added from the backup content */
	siRetVal = persadmin_list_application_folders_get_size(backup_data_path);
	if(siRetVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders_get_size call failed with error code:"),
							  	  	  	  	  	  	DLT_INT(siRetVal));
		return GENERIC_ERROR_CODE;
	}

	if(0 == siRetVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
													DLT_STRING("No application found."));
		return siRetVal;
	}

	listBuffSize = siRetVal;

	pAppNameList = (pstr_t) malloc((uint_t)listBuffSize); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pAppNameList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for App name list."));
		return GENERIC_ERROR_CODE;
	}
	(void)memset(pAppNameList, 0, (uint_t)listBuffSize);

	siRetVal = persadmin_list_application_folders(backup_data_path,
												  pAppNameList,
												  listBuffSize );

	if((siRetVal < SUCCESS_CODE) || (siRetVal > listBuffSize))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders call failed with error code:"),
													DLT_INT(siRetVal));
		free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pAppNameList = NIL;
		return GENERIC_ERROR_CODE;
	}

	listBuffSize = siRetVal;
	pAppName  = pAppNameList;
	while(listBuffSize > 0)
	{
		if(0 == strlen(pAppName))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Invalid app name found."));
			break;
		}

		/* Check if the application exists. Skip the application otherwise */
		/* /Data/mnt-c/<appId> */
		(void)snprintf(pAppDestPath, sizeof(pAppDestPath), PERS_ORG_LOCAL_CACHE_PATH_FORMAT, pAppName, "");

		if(true == persadmin_restore_check_if_app_exists("", pAppName))
		{
			/* local app data */
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
														DLT_STRING("local app data restore..."));

			retVal = persadmin_restore_appl_data( 	backup_data_path,
													pAppName );
			if(retVal < SUCCESS_CODE)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_restore_appl_data call failed with error code:"),
															DLT_INT(retVal));
				free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pAppNameList = NIL;
				return GENERIC_ERROR_CODE;
			}
			bytesRestored += retVal;


			/* links to groups and public */
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
														DLT_STRING("restore links to groups and public..."));

			/* <backupDataPath>/Data/mnt-c/<appId> */
			(void)snprintf(pAppSourcePath, sizeof(pAppSourcePath), PAS_SRC_LOCAL_CACHE_PATH_FORMAT, backup_data_path, pAppName, "");

			siRetVal = persadmin_import_links(  pAppSourcePath,
												pAppDestPath );
			if(retVal < SUCCESS_CODE)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_import_links call failed with error code:"),
															DLT_INT(retVal));
				free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pAppNameList = NIL;
				return GENERIC_ERROR_CODE;
			}
			bytesRestored += retVal;
		}

		listBuffSize -= ((sint_t)strlen(pAppName) + 1) * (sint_t)sizeof(*pAppName);
		pAppName += (strlen(pAppName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}
	free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pAppNameList = NIL;


	/* non-user shared data */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("restore non-user shared data..."));

	retVal = persadmin_restore_non_user_shared_data(backup_data_path);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_non_user_shared_data call failed with error code:"),
												  	DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}
	bytesRestored += retVal;


	/* user shared data */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("restore user shared data..."));

	retVal = persadmin_restore_user_shared_data(	PersASSelectionType_All,
													backup_data_path,
													PERSIST_SELECT_ALL_USERS,
													PERSIST_SELECT_ALL_SEATS);
	if(retVal < SUCCESS_CODE)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_user_shared_data call failed with error code:"),
												  	DLT_INT(retVal));
		return GENERIC_ERROR_CODE;
	}
	bytesRestored += retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully all data from"),
												DLT_STRING(backup_data_path),
												DLT_STRING("."),
												DLT_INT64(bytesRestored),
												DLT_STRING("bytes restored"));

	return bytesRestored;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/
