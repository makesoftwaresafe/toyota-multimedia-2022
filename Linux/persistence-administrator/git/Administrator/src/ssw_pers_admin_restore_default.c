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
* 2013.04.15 uidu0250 			CSP_WZ#3424:  Initial version
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

#define PAS_SHARED_PUBLIC_PATH								PERS_ORG_SHARED_FOLDER_NAME"/"PERS_ORG_PUBLIC_FOLDER_NAME


#define	PAS_MAX_LIST_DB_SIZE								(32 * 1024)

#define LT_HDR                          					"RESTORE DEFAULT>>"



/* ----------global variables. initialization of global contexts ------------ */
DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);


/* ---------------------- local functions ---------------------------------- */
/**
 * \brief Allow recovery of default data on application level ->  (node+user).(application)
 *
 * \param defaultSource source of the default to allow reset
 * \param appId the application identifier
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_appl_data(PersASDefaultSource_e defaultSource,
                      	  	  	  	  	  	  	  pstr_t             	appId );


/**
 * \brief Allow recovery of default data on user level ->  (user).(application+shared)
 *
 * \param defaultSource source of the default to allow reset
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_user_data(	PersASDefaultSource_e 	defaultSource,
													pstr_t             		appId,
													uint32_t       			user_no,
													uint32_t    			seat_no );


/**
 * \brief Allow restore all data to default values ->  (node+user).(application+shared)
 *
 * \param defaultSource source of the default to allow reset
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_all_data(   PersASDefaultSource_e 	defaultSource );


/**
 * \brief Allow recovery of default data on application level for the node section
 *
 * \param appId the application identifier
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_appl_node(  pstr_t             	appId );


/**
 * \brief Allow recovery of default data on application level for the user section (all or a specific user)
 *
 * \param defaultSource source of the default to allow reset
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_appl_user(	PersASDefaultSource_e 	defaultSource,
													pstr_t          		appId,
													uint32_t   				user_no,
													uint32_t	  			seat_no);

/**
 * \brief Allow recovery of default local user data for the selected user_no (all or a specific user)
 *
 * \param defaultSource source of the default to allow reset
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_user_local_data(PersASDefaultSource_e 	defaultSource,
														pstr_t          		appId,
														uint32_t     			user_no,
														uint32_t				seat_no);

/**
 * \brief Allow recovery of default shared user data for the selected user_no (all or a specific user)
 *
 * \param type restore type
 * \param defaultSource source of the default to allow reset
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_user_shared_data(	PersASSelectionType_e 	type,
															PersASDefaultSource_e 	defaultSource,
															uint32_t       			user_no,
															uint32_t				seat_no);

/**
 * \brief Allow recovery of default user public files for the selected user_no (all or a specific user)
 *
 * \param defaultSource source of the default to allow reset
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_user_public_files(	PersASDefaultSource_e 	defaultSource,
															uint32_t       			user_no,
															uint32_t				seat_no);

/**
 * \brief Allow recovery of default user group files for the selected user_no (all or a specific user)
 *
 * \param defaultSource source of the default to allow reset
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_user_group_files(	PersASDefaultSource_e 	defaultSource,
															uint32_t       			user_no,
															uint32_t				seat_no);

/**
 * \brief Allow restore of default shared data not related to user
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_non_user_shared_data(void);


/**
 * \brief Allow restore of default local keys for the selected applicationId and/or user_no (all or a specific user)
 *
 * \param type recovery type
 * \param defaultSource source of the default to allow reset
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_local_keys(	PersASSelectionType_e 	type,
													PersASDefaultSource_e 	defaultSource,
													pstr_t          		appId,
													uint32_t       			user_no,
													uint32_t				seat_no);


/**
 * \brief Allow restore of default shared keys for the selected applicationId and/or user_no (all or a specific user)
 *
 * \param type recovery type
 * \param defaultSource source of the default to allow reset
 * \param appId the application identifier
 * \param user_no the user ID
 * \param seat_no the seat ID
 *
 * \return 0 for success; negative value: error code
 */
static sint_t persadmin_restore_default_shared_keys(PersASSelectionType_e 	type,
													PersASDefaultSource_e 	defaultSource,
													pstr_t          		appId,
													uint32_t       			user_no,
													uint32_t				seat_no);


/**
 * \brief Forms the user path (by appending) according to the provided user_no and seat_no
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
* \brief Allow restore of default values on different level (application, user or complete)
*
* \param type represents the quality of the data to backup: all, application, user
* \param defaultSource source of the default data to reset
* \param applicationID the application identifier
* \param user_no the user ID
* \param seat_no the seat number (seat 0 to 3)
*
* \return 0 for success; negative value: error code (\ref PAS_RETURNS)
*/
long persadmin_data_restore_to_default(	PersASSelectionType_e 	type,
										PersASDefaultSource_e 	defaultSource,
										const char* 			applicationID,
										unsigned int 			user_no,
										unsigned int 			seat_no)
{
	if(NIL == applicationID)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_data_restore_to_default call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	switch(type)
	{

	case PersASSelectionType_All:
	{
		return (long)persadmin_restore_default_all_data(  defaultSource );
	}
	break;

	case PersASSelectionType_User:
	{
		return (long)persadmin_restore_default_user_data(defaultSource,
														 (pstr_t)applicationID,
														 (uint32_t)user_no,
														 (uint32_t)seat_no);
	}
	  break;

	case PersASSelectionType_Application:
	{
		return (long)persadmin_restore_default_appl_data(	defaultSource,
															(pstr_t)applicationID );
	}
	break;

	default:
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_data_restore_to_default call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	break; // redundant
	}
}


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
static sint_t persadmin_restore_default_appl_node(  pstr_t             	appId ) // could use an 'extended' app path
{
	sint_t    	retVal;
	str_t		pNodeDestPath		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if(NIL == appId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_appl_node call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default node content for:"),
												DLT_STRING(appId));


	/* app node dest path */

	/* /Data/mnt-c/<appId>/node */
	(void)snprintf(pNodeDestPath, sizeof(pNodeDestPath), gLocalCachePath, appId, gNode);

	/* erase node content */
	if(PAS_SUCCESS == persadmin_check_if_file_exists(pNodeDestPath, true))
	{
		retVal = persadmin_delete_folder(pNodeDestPath);
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_delete_folder call failed with error code:"),
														DLT_INT(retVal),
														DLT_STRING("for"),
														DLT_STRING(pNodeDestPath));
			return retVal;
		}
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default node content for:"),
												DLT_STRING(appId),
												DLT_STRING("."));

	return PAS_SUCCESS;
}


//-----------------------------------------------------------------------
static sint_t persadmin_restore_default_appl_user(	PersASDefaultSource_e 	defaultSource,
													pstr_t          		appId,
													uint32_t   				user_no,
													uint32_t	  			seat_no)
{
	sint_t  retVal;
	str_t   pUserDestPath   	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t	pConfDefaultPath	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if(NIL == appId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_appl_user call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default user content for App:"),
												DLT_STRING(appId),
												DLT_STRING("User:"),
												DLT_UINT8(user_no),
												DLT_STRING("Seat:"),
												DLT_UINT8(seat_no));

	/* user dest path */
	/* /Data/mnt-c/<appId>/user/<user_no>/seat/<seat_no> */
	(void)snprintf(pUserDestPath, sizeof(pUserDestPath), gLocalCachePath, appId, "");
	persadmin_restore_set_user_path(pUserDestPath,
									sizeof(pUserDestPath)/sizeof(*pUserDestPath),
									user_no,
									seat_no);

	/* erase user content */
	if(PAS_SUCCESS == persadmin_check_if_file_exists(pUserDestPath, true))
	{
		retVal = persadmin_delete_folder(pUserDestPath);
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_delete_folder call failed with error code:"),
														DLT_INT(retVal),
														DLT_STRING("for"),
														DLT_STRING(pUserDestPath));
			return retVal;
		}
	}

	if(PersASDefaultSource_Factory == defaultSource)
	{
		/* delete configurableDefaultUserData folder */

		/* /Data/mnt-c/<appId>/configurableDefaultUserData */
		(void)snprintf(pConfDefaultPath, sizeof(pConfDefaultPath), gLocalCachePath, appId, PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_);

		if(PAS_SUCCESS == persadmin_check_if_file_exists(pConfDefaultPath, true))
		{
			retVal = persadmin_delete_folder(pConfDefaultPath);
			if(retVal < PAS_SUCCESS)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
															DLT_STRING("Failed to delete folder:"),
															DLT_STRING(pConfDefaultPath));
				return retVal;
			}
		}
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default user content for App:"),
												DLT_STRING(appId),
												DLT_STRING("User:"),
												DLT_UINT8(user_no),
												DLT_STRING("Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//-------------------------------------------------------------------------------------
static sint_t persadmin_restore_default_non_user_shared_data(void)
{
	sint_t    	retVal;
	str_t    	pExtendedAppId   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];	// group path
	sint_t    	listBuffSize    		= 0;
	pstr_t    	pStrList      			= NIL;
	pstr_t    	pItemName     			= NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default non-user shared data"));

	/* --- public file/folder restore --- */
	retVal = persadmin_restore_default_appl_node(	PAS_SHARED_PUBLIC_PATH );
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_appl_node call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	/* --- group file/folder restore --- */

	/* Check all groups */
	retVal = persadmin_list_folder_get_size(PERS_ORG_SHARED_GROUP_CACHE_PATH,
											PersadminFilterFolders,
											false );
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_folder_get_size call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	listBuffSize = retVal;

	if(listBuffSize > 0)
	{
		pStrList = NIL;
		pStrList = (pstr_t)malloc((uint_t)(listBuffSize * sizeof(*pStrList))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		if(NIL == pStrList)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Error allocating memory for list of folders."));
			return PAS_FAILURE_OUT_OF_MEMORY;
		}
		(void)memset(pStrList, 0, (uint_t)(listBuffSize * sizeof(*pStrList)));

		retVal = persadmin_list_folder(	PERS_ORG_SHARED_GROUP_CACHE_PATH,
										pStrList,
										listBuffSize,
										PersadminFilterFolders,
										false);
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Error obtaining the list of folders."));
			free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pStrList = NIL;
			return retVal;
		}

		pItemName = pStrList;
		while(listBuffSize > 0)
		{
			if(0 == strlen(pItemName))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("Invalid folder item found"));
				break;
			}

			/* Restore the default node content for every group */

			/* shared/group/<group_id> */
			(void)snprintf(pExtendedAppId, sizeof(pExtendedAppId), "%s/%s/%s", PERS_ORG_SHARED_FOLDER_NAME, PERS_ORG_GROUP_FOLDER_NAME, pItemName);

			retVal = persadmin_restore_default_appl_node(	pExtendedAppId);
			if(retVal < PAS_SUCCESS)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_restore_default_appl_node call failed with error code:"),
															DLT_INT(retVal));
				free(pStrList);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				pStrList = NIL;
				return retVal;
			}

			listBuffSize -= ((sint_t)strlen(pItemName) + 1) * (sint_t)sizeof(*pItemName);
			pItemName += (strlen(pItemName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
		}

		free(pStrList);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pStrList = NIL;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored default non-user shared data."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//--------------------------------------------------------------------------
static sint_t persadmin_restore_default_user_public_files(	PersASDefaultSource_e 	defaultSource,
															uint32_t       			user_no,
															uint32_t				seat_no)
{
	sint_t    	retVal;
	str_t    	pConfDefaultPath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pUserDestPath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default user public files"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* --- public file/folder restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("public file/folder restore..."));

	/* /Data/mnt-c/shared/public/user/<user_no>/seat/<seat_no> */
	(void)snprintf(pUserDestPath, sizeof(pUserDestPath), "%s", PERS_ORG_SHARED_PUBLIC_CACHE_PATH);
	persadmin_restore_set_user_path(pUserDestPath,
									sizeof(pUserDestPath)/sizeof(*pUserDestPath),
									user_no,
									seat_no);

	/* erase user content */
	if(PAS_SUCCESS == persadmin_check_if_file_exists(pUserDestPath, true))
	{
		retVal = persadmin_delete_folder(pUserDestPath);
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_delete_folder call failed with error code:"),
														DLT_INT(retVal));
			return retVal;
		}
	}

	if(PersASDefaultSource_Factory == defaultSource)
	{
		/* delete configurableDefaultUserData folder */

		/* /Data/mnt-c/shared/public/configurableDefaultUserData */
		(void)snprintf(pConfDefaultPath, sizeof(pConfDefaultPath), "%s%s", PERS_ORG_SHARED_PUBLIC_CACHE_PATH, PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_);

		if(PAS_SUCCESS == persadmin_check_if_file_exists(pConfDefaultPath, true))
		{
			retVal = persadmin_delete_folder(pConfDefaultPath);
			if(retVal < PAS_SUCCESS)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
															DLT_STRING("Failed to delete folder:"),
															DLT_STRING(pConfDefaultPath));
				return retVal;
			}
		}
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default user public files"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//--------------------------------------------------------------------------
static sint_t persadmin_restore_default_user_group_files(	PersASDefaultSource_e 	defaultSource,
															uint32_t       			user_no,
															uint32_t				seat_no)
{
	sint_t    	retVal;
	str_t    	pConfDefaultPath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pUserDestPath   		[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	pstr_t    	pStrList      			= NIL;
	pstr_t    	pItemName     			= NIL;
	sint_t    	listBuffSize    		= 0;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default user group files"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* Check all groups */
	retVal = persadmin_list_folder_get_size(PERS_ORG_SHARED_GROUP_CACHE_PATH,
											PersadminFilterFolders,
											false );
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_folder_get_size call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}
	if(0 == retVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
													DLT_STRING("No content to restore for user group files"),
													DLT_STRING("for User:"),
													DLT_UINT8(user_no),
													DLT_STRING("for Seat:"),
													DLT_UINT8(seat_no),
													DLT_STRING("."));
		return retVal;
	}

	listBuffSize = retVal;

	pStrList = NIL;
	pStrList = (pstr_t)malloc((uint_t)(listBuffSize * sizeof(*pStrList))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pStrList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for list of folders."));
		return PAS_FAILURE_OUT_OF_MEMORY;
	}
	(void)memset(pStrList, 0, (uint_t)(listBuffSize * sizeof(*pStrList)));

	retVal = persadmin_list_folder(	PERS_ORG_SHARED_GROUP_CACHE_PATH,
									pStrList,
									listBuffSize,
									PersadminFilterFolders,
									false);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error obtaining the list of folders."));
		free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pStrList = NIL;
		return retVal;
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
													DLT_STRING("restore default user content for group"),
													DLT_STRING(pItemName),
													DLT_STRING(" ..."));

		/* /Data/mnt-c/shared/group/<group_id>/user/<user_no>/seat/<seat_no> */
		(void)snprintf(pUserDestPath, sizeof(pUserDestPath), PERS_ORG_SHARED_CACHE_PATH_STRING_FORMAT, pItemName, "");
		persadmin_restore_set_user_path(pUserDestPath,
										sizeof(pUserDestPath) / sizeof(*pUserDestPath),
										user_no,
										seat_no);

		/* erase user content */
		if(PAS_SUCCESS == persadmin_check_if_file_exists(pUserDestPath, true))
		{
			retVal = persadmin_delete_folder(pUserDestPath);
			if(retVal < PAS_SUCCESS)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
															DLT_STRING("persadmin_delete_folder call failed with error code:"),
															DLT_INT(retVal));
				return retVal;
			}
		}

		if(PersASDefaultSource_Factory == defaultSource)
		{
			/* delete configurableDefaultUserData folder */

			/* /Data/mnt-c/shared/group/<group_id>/configurableDefaultData */
			(void)snprintf(pConfDefaultPath, sizeof(pConfDefaultPath), PERS_ORG_SHARED_CACHE_PATH_STRING_FORMAT, pItemName, PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_);

			if(PAS_SUCCESS == persadmin_check_if_file_exists(pConfDefaultPath, true))
			{
				retVal = persadmin_delete_folder(pConfDefaultPath);
				if(retVal < PAS_SUCCESS)
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
																DLT_STRING("Failed to delete folder:"),
																DLT_STRING(pConfDefaultPath));

					free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
					pStrList = NIL;
					return retVal;
				}
			}
		}

		listBuffSize -= ((sint_t)strlen(pItemName) + 1) * (sint_t)sizeof(*pItemName);
		pItemName += (strlen(pItemName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}

	free(pStrList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pStrList = NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default user group files"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//--------------------------------------------------------------------------
static sint_t persadmin_restore_default_user_local_data(PersASDefaultSource_e 	defaultSource,
														pstr_t          		appId,
														uint32_t     			user_no,
														uint32_t				seat_no)
{
	sint_t    	retVal;
	sint_t		listBuffSize		= 0;
	pstr_t    	pAppNameList    	= NIL;
	pstr_t    	pAppName      		= NIL;


	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default user local data"),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT32(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT32(seat_no));

	/* Restore default values for a specific application only */
	if(0 != strcmp(appId, ""))
	{
		/* user_no for every app */
		retVal = persadmin_restore_default_appl_user(	defaultSource,
														appId,
														user_no,
														seat_no );
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_default_appl_user call failed with error code:"),
														DLT_INT(retVal));
			return retVal;
		}

		/* user_no for local keys */
		retVal = persadmin_restore_default_local_keys( PersASSelectionType_User,
														defaultSource,
														appId,
														user_no,
														seat_no);

		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_default_local_keys call failed with error code:"),
														DLT_INT(retVal));
			return retVal;
		}

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("Restored successfully user local data "),
													DLT_STRING("for App:"),
													DLT_STRING(appId),
													DLT_STRING("for User:"),
													DLT_UINT8(user_no),
													DLT_STRING("for Seat:"),
													DLT_UINT8(seat_no),
													DLT_STRING("."));

		return PAS_SUCCESS;
	}


	/* Get list of application names */
	retVal = persadmin_list_application_folders_get_size("");
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders_get_size call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	if(0 == retVal) /* size of path collection is 0 */
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
													DLT_STRING("No application found."));
		return retVal;
	}

	listBuffSize = retVal;

	pAppNameList = NIL;
	pAppNameList = (pstr_t) malloc((uint_t)(listBuffSize * sizeof(*pAppNameList))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pAppNameList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for App name list."));
		return PAS_FAILURE_OUT_OF_MEMORY;
	}
	(void)memset(pAppNameList, 0, (uint_t)(listBuffSize * sizeof(*pAppNameList)));

	retVal = persadmin_list_application_folders("",
												pAppNameList,
												listBuffSize );

	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders call failed with error code:"),
													DLT_INT(retVal));
		free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pAppNameList = NIL;

		if(retVal > listBuffSize)
		{
			return PAS_FAILURE;
		}

		return retVal;
	}

	listBuffSize = retVal;
	pAppName  = pAppNameList;
	while(listBuffSize > 0)
	{
		if(0 == strlen(pAppName))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("Invalid app name found."));
			break;
		}

		/* user_no for every app */
		retVal = persadmin_restore_default_appl_user(	defaultSource,
														pAppName,
														user_no,
														seat_no );
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_default_appl_user call failed with error code:"),
														DLT_INT(retVal));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return retVal;
		}

		/* user_no for local keys */
		retVal = persadmin_restore_default_local_keys( 	PersASSelectionType_User,
														defaultSource,
														pAppName,
														user_no,
														seat_no);

		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_default_local_keys call failed with error code:"),
														DLT_INT(retVal));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return retVal;
		}

		listBuffSize -= ((sint_t)strlen(pAppName) + 1) * (sint_t)sizeof(*pAppName);
		pAppName += (strlen(pAppName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}

	free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pAppNameList = NIL;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully user local data "),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//--------------------------------------------------------------------------
static sint_t persadmin_restore_default_user_shared_data(	PersASSelectionType_e 	type,
															PersASDefaultSource_e 	defaultSource,
															uint32_t       			user_no,
															uint32_t				seat_no)
{
	sint_t		retVal;

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default user shared data"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* --- public user files restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("public file/folder restore..."));

	retVal = persadmin_restore_default_user_public_files(	defaultSource,
															user_no,
															seat_no);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_user_public_files call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	/* --- group user files restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("group file/folder restore..."));

	retVal = persadmin_restore_default_user_group_files(defaultSource,
														user_no,
														seat_no);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_user_group_files call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	/* --- shared key-value restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("restore default shared keys"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("..."));

	retVal = persadmin_restore_default_shared_keys(	type,
													defaultSource,
													"",
													user_no,
													seat_no);

	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_shared_keys call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default user shared data"),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//-------------------------------------------------------------------------
static sint_t persadmin_restore_default_local_keys(	PersASSelectionType_e 	type,
													PersASDefaultSource_e 	defaultSource,
													pstr_t          		appId,
													uint32_t       			user_no,
													uint32_t				seat_no)
{
	sint_t    	retVal;
	str_t    	pDestDBPath     	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];

	if(NIL == appId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_local_keys call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default local keys for:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	/* cache dest. db. path */
	/* /Data/mnt-c/<appId>/cached.itz */
	(void)snprintf(pDestDBPath, sizeof(pDestDBPath), gLocalCachePath, appId, PERS_ORG_LOCAL_CACHE_DB_NAME_);

	/* delete the keys in the cache dest. DB */
	retVal = persadmin_delete_keys_by_filter(	type,
												pDestDBPath,
												user_no,
												seat_no);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_delete_keys_by_filter call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}


	/* wt dest. db. path */
	/* /Data/mnt-c/<appId>/wt.itz */
	(void)snprintf(pDestDBPath, sizeof(pDestDBPath), gLocalCachePath, appId, PERS_ORG_LOCAL_WT_DB_NAME_);

	/* delete the keys in the wt dest. DB */
	retVal = persadmin_delete_keys_by_filter(	type,
												pDestDBPath,
												user_no,
												seat_no);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_delete_keys_by_filter call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}


	if(PersASDefaultSource_Factory == defaultSource)
	{
		/* delete configurable-default-data.itz */

		/* /Data/mnt-c/<appId>/configurable-default-data.itz */
		(void)snprintf(pDestDBPath, sizeof(pDestDBPath), gLocalCachePath, appId, PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_);

		if(PAS_SUCCESS == persadmin_check_if_file_exists(	pDestDBPath, false))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING(LT_HDR),
														DLT_STRING("Removing file:"),
														DLT_STRING(pDestDBPath));

			retVal = persadmin_delete_file(pDestDBPath);
			if(retVal < PAS_SUCCESS)
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
															DLT_STRING("Failed to delete file:"),
															DLT_STRING(pDestDBPath));
				return retVal;
			}
		}
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default local keys for:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//-------------------------------------------------------------------------
static sint_t persadmin_restore_default_shared_keys(PersASSelectionType_e 	type,
													PersASDefaultSource_e 	defaultSource,
													pstr_t          		appId,
													uint32_t       			user_no,
													uint32_t				seat_no)
{
	sint_t    	retVal;
	str_t		pDBFileName     	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t		pDestDBParentPath	[PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	str_t    	pConfDefaultDBPath  [PERS_ORG_MAX_LENGTH_PATH_FILENAME];
	pstr_t    	pDestDBPath   		= NIL;
	str_t    	pDBList       		[PAS_MAX_LIST_DB_SIZE];

	if(NIL == appId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_shared_keys call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restore default shared keys"),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no));

	retVal = persadmin_get_all_db_paths_with_names( "",
													appId,
													PersistenceStorage_shared,
													pDBList,
													PAS_MAX_LIST_DB_SIZE);

	if(retVal <= 0)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_get_all_db_paths_with_names call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	pDestDBPath   = pDBList;
	while(PAS_ITEM_LIST_DELIMITER != (*pDestDBPath))
	{
		(void)memset(pDBFileName, 0, sizeof(pDBFileName));

		retVal = persadmin_get_filename(pDestDBPath,
										pDBFileName,
										sizeof(pDBFileName));

		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_get_filename call failed with error code:"),
														DLT_INT(retVal));
			return retVal;
		}

		/* filter only the cached.itz and wt.itz databases */
		if(0 != strcmp(pDBFileName, PERS_ORG_SHARED_CACHE_DB_NAME))
		{
			if(0 != strcmp(pDBFileName, PERS_ORG_SHARED_WT_DB_NAME))
			{
				pDestDBPath += (strlen(pDestDBPath) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
				continue;
			}
		}


		/* delete the keys in the dest. DB */
		retVal = persadmin_delete_keys_by_filter(	type,
													pDestDBPath,
													user_no,
													seat_no);
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_delete_keys_by_filter call failed with error code:"),
														DLT_INT(retVal));
			return retVal;
		}


		if(PersASDefaultSource_Factory == defaultSource)
		{
			/* delete configurable-default-data.itz */
			(void)snprintf(pDestDBParentPath, (strlen(pDestDBPath)- strlen(pDBFileName) + 1) * sizeof(*pDestDBParentPath), "%s", pDestDBPath);

			/* default configurable-default-data.itz source db. path */
			(void)snprintf(pConfDefaultDBPath, sizeof(pConfDefaultDBPath), "%s%s", pDestDBParentPath, PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME);

			if(PAS_SUCCESS == persadmin_check_if_file_exists(pConfDefaultDBPath, false))
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, 	DLT_STRING(LT_HDR),
															DLT_STRING("Removing file:"),
															DLT_STRING(pConfDefaultDBPath));

				retVal = persadmin_delete_file(pConfDefaultDBPath);
				if(retVal < PAS_SUCCESS)
				{
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, 	DLT_STRING(LT_HDR),
																DLT_STRING("Failed to delete file:"),
																DLT_STRING(pConfDefaultDBPath));
					return retVal;
				}
			}
		}

		pDestDBPath += (strlen(pDestDBPath) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default shared keys"),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//---------------------------------------------------------------------------
static sint_t persadmin_restore_default_appl_data(PersASDefaultSource_e defaultSource,
                      	  	  	  	  	  	  	  pstr_t             	appId )
{
	sint_t		retVal;

	if(NIL == appId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_appl_data call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	switch(defaultSource)
	{
	case PersASDefaultSource_Factory:

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("Restore factory default app data for App:"),
													DLT_STRING(appId));
		break;

	case PersASDefaultSource_Configurable:
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("Restore configurable default app data for App:"),
													DLT_STRING(appId));
		break;
	default:
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_appl_data call."));
		return PAS_FAILURE_INVALID_PARAMETER;
		break; /* redundant */
	}


	/* --- local file/folder restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local file/folder restore..."));


	/* restore default node content */
	retVal = persadmin_restore_default_appl_node((pstr_t)appId);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_appl_node call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}


	/* restore default user content (for all users) */
	retVal = persadmin_restore_default_appl_user(	defaultSource,
													(pstr_t)appId,
													PERSIST_SELECT_ALL_USERS,
													PERSIST_SELECT_ALL_SEATS);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_appl_user call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}


	/* --- local key-value restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local key/value restore..."));

	retVal = persadmin_restore_default_local_keys(	PersASSelectionType_Application,
													defaultSource,
													appId,
													PERSIST_SELECT_ALL_USERS,
													PERSIST_SELECT_ALL_SEATS);

	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_local_keys call failed with error code:"),
													DLT_INT(retVal));
		return retVal;
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default app data for App:"),
												DLT_STRING(appId),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


//---------------------------------------------------------------------------
static sint_t persadmin_restore_default_user_data(	PersASDefaultSource_e 	defaultSource,
													pstr_t             		appId,
													uint32_t       			user_no,
													uint32_t    			seat_no )
{
	sint_t		retVal;

	if(NIL == appId)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_user_data call."));
		return PAS_FAILURE_INVALID_PARAMETER;
	}

	switch(defaultSource)
	{
	case PersASDefaultSource_Factory:

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Restore factory default user data"),
													DLT_STRING("for App:"),
													DLT_STRING(appId),
													DLT_STRING("for User:"),
													DLT_UINT8(user_no),
													DLT_STRING("for Seat:"),
													DLT_UINT8(seat_no),
													DLT_STRING("not supported."));
		return PAS_FAILURE_OPERATION_NOT_SUPPORTED;
		break; /* redundant */

	case PersASDefaultSource_Configurable:

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("Restore configurable default user data"),
													DLT_STRING("for App:"),
													DLT_STRING(appId),
													DLT_STRING("for User:"),
													DLT_UINT8(user_no),
													DLT_STRING("for Seat:"),
													DLT_UINT8(seat_no));
		break;

	default:
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_user_data call."));
		return PAS_FAILURE_INVALID_PARAMETER;
		break; /* redundant */
	}


	/* --- local user data restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("default local user data restore..."));

	retVal = persadmin_restore_default_user_local_data(	defaultSource,
														appId,
														user_no,
														seat_no);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_user_local_data call failed with error code:"),
												  	DLT_INT(retVal));
		return retVal;
	}

	if(0 == strcmp(appId, ""))
	{
		/* --- shared user data restore --- */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("default shared user data restore..."));

		retVal = persadmin_restore_default_user_shared_data(	PersASSelectionType_User,
																defaultSource,
																user_no,
																seat_no);
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_default_user_shared_data call failed with error code:"),
														DLT_INT(retVal));
			return retVal;
		}
	}

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully default user data"),
												DLT_STRING("for App:"),
												DLT_STRING(appId),
												DLT_STRING("for User:"),
												DLT_UINT8(user_no),
												DLT_STRING("for Seat:"),
												DLT_UINT8(seat_no),
												DLT_STRING("."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/



//---------------------------------------------------------------------------
static sint_t persadmin_restore_default_all_data(   PersASDefaultSource_e 	defaultSource )
{
	sint_t		retVal;
	sint_t		listBuffSize		= 0;
	pstr_t    	pAppNameList    	= NIL;
	pstr_t    	pAppName      		= NIL;


	switch(defaultSource)
	{
	case PersASDefaultSource_Factory:

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Restore all factory default data"));
		break;

	case PersASDefaultSource_Configurable:

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("Restore all configurable default data"));
		break;

	default:
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Invalid parameter in persadmin_restore_default_all_data call."));
		return PAS_FAILURE_INVALID_PARAMETER;
		break; /* redundant */
	}


	/* --- local data restore --- */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("local data restore..."));

	/* Get the list of application names */
	retVal = persadmin_list_application_folders_get_size("");
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders_get_size call failed with error code:"),
							  	  	  	  	  	  	DLT_INT(retVal));
		return retVal;
	}

	if(0 == retVal)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,  	DLT_STRING(LT_HDR),
													DLT_STRING("No application found."));
		return retVal;
	}

	listBuffSize = retVal;

	pAppNameList = (pstr_t) malloc((uint_t)(listBuffSize * sizeof(*pAppNameList))); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	if(NIL == pAppNameList)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("Error allocating memory for apps name list."));
		return PAS_FAILURE_OUT_OF_MEMORY;
	}
	(void)memset(pAppNameList, 0, (uint_t)(listBuffSize * sizeof(*pAppNameList)));

	retVal = persadmin_list_application_folders("",
												pAppNameList,
												listBuffSize );

	if((retVal < PAS_SUCCESS) || (retVal > listBuffSize))
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_list_application_folders call failed with error code:"),
													DLT_INT(retVal));
		free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
		pAppNameList = NIL;
		return retVal;
	}

	pAppName  = pAppNameList;
	while(listBuffSize > 0)
	{
		if(0 == strlen(pAppName))
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN,	DLT_STRING(LT_HDR),
														DLT_STRING("Invalid app name found."));
			break;
		}

		/* local app data */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
													DLT_STRING("local app data restore..."));

		retVal = persadmin_restore_default_appl_data( 	defaultSource,
														pAppName );
		if(retVal < PAS_SUCCESS)
		{
			DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,  DLT_STRING(LT_HDR),
														DLT_STRING("persadmin_restore_default_appl_data call failed with error code:"),
														DLT_INT(retVal));
			free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			pAppNameList = NIL;
			return retVal;
		}

		listBuffSize -= ((sint_t)strlen(pAppName) + 1) * (sint_t)sizeof(*pAppName);
		pAppName += (strlen(pAppName) + 1); // MISRA-C:2004 Rule 17.4 Performing pointer arithmetic. - Rule currently not accepted
	}
	free(pAppNameList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
	pAppNameList = NIL;


	/* non-user shared data */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("restore non-user shared data..."));

	retVal = persadmin_restore_default_non_user_shared_data();
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_non_user_shared_data call failed with error code:"),
												  	DLT_INT(retVal));
		return retVal;
	}

	/* user shared data */
	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("restore user shared data..."));

	retVal = persadmin_restore_default_user_shared_data(	PersASSelectionType_All,
															defaultSource,
															PERSIST_SELECT_ALL_USERS,
															PERSIST_SELECT_ALL_SEATS);
	if(retVal < PAS_SUCCESS)
	{
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR,	DLT_STRING(LT_HDR),
													DLT_STRING("persadmin_restore_default_user_shared_data call failed with error code:"),
												  	DLT_INT(retVal));
		return retVal;
	}


	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG,  DLT_STRING(LT_HDR),
												DLT_STRING("Restored successfully all default data."));

	return PAS_SUCCESS;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/
