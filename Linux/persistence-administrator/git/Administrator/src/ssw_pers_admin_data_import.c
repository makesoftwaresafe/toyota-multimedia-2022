/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Liviu.Ciocanta@continental-corporation.com
*
* Implementation of import process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2012.11.15 uidn3504        	CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/ 

#include <dirent.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>
#include <errno.h>
#include <dlt.h>
#include <stdlib.h>

#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_common.h"
#include "ssw_pers_admin_service.h"
#include "persistence_admin_service.h"
#include "persComDataOrg.h"


#define USER_ALL    PERSIST_SELECT_ALL_USERS
#define SEAT_ALL    PERSIST_SELECT_ALL_SEATS
#define MAX_PATH    (PERS_ORG_MAX_LENGTH_PATH_FILENAME + 1)//256
#define MAX_APP_NO  256

//moved to "ssw_pers_admin_common.h"
//extern const char StringTerminator;

DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);

/**
 * \brief Allow the import of data from specified folder to the system respecting different level (application, user or complete)
 *
 * \param type represent the quality of the data to backup: all, application, user
 * \param src_folder name of the source folder of the data
 *
 * \return positive value: number of bytes imported; negative value: error code
 */
long persadmin_data_folder_import(PersASSelectionType_e type, pconststr_t src_folder)
{
	long lRetVal = 0;
	str_t appName[MAX_PATH];

	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO,
					DLT_STRING("->persadmin_data_folder_import : type = "), DLT_INT(type),
					DLT_STRING(" , src_folder = "), DLT_STRING(src_folder));

	switch (type)
	{
		case PersASSelectionType_Application:
		{
			int appListSize = persadmin_list_application_folders_get_size(src_folder);
			pstr_t list = NIL;

			DLT_LOG_STRING_INT(persAdminSvcDLTCtx, DLT_LOG_INFO, "size of src_folder="
					,appListSize);
//			DLT_LOG_STRING(persAdminSvcDLTCtx, DLT_LOG_INFO, "list of apps :");

			if(appListSize > 0)
			{
				list = (pstr_t)malloc((unsigned int)appListSize*sizeof(str_t));/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
			}
			else
			{
				DLT_LOG_STRING_INT(persAdminSvcDLTCtx, DLT_LOG_ERROR, "NEGATIVE size of src_folder="
									,appListSize);
			}

			if(NIL != list )
			{
				//str_t list[ appListSize ];

				appListSize = persadmin_list_application_folders(src_folder, list, appListSize);

				if(appListSize > 0)
				{
					int totalSize = 0;
					char * crtApp = list;
					lRetVal = 0;

					while ((*crtApp != '\0') && (lRetVal >=0))
					{
						totalSize += lRetVal;
						(void)strcpy(appName, crtApp);
						DLT_LOG_STRING(persAdminSvcDLTCtx, DLT_LOG_INFO, appName );
						lRetVal = persadmin_data_backup_recovery(type, src_folder, appName, USER_ALL, SEAT_ALL);
						crtApp+=strlen(appName)+sizeof(StringTerminator); //QAC - TODO: add deviation grant.
					}
					//add last result
					if(lRetVal >= 0)
					{
						lRetVal += totalSize;
					}
				}
				free(list);/*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
				list = NIL;
			}
			else
			{
				DLT_LOG_STRING(persAdminSvcDLTCtx, DLT_LOG_ERROR, "Could not allocate App. list!");
			}
			break;
		}
		case PersASSelectionType_All://fall-through
		case PersASSelectionType_User:
		{
			DLT_LOG_STRING_INT(persAdminSvcDLTCtx, DLT_LOG_INFO,
					"Calling persadmin_data_backup_recovery, type=%d",type);
			lRetVal = persadmin_data_backup_recovery(type, src_folder, "", USER_ALL, SEAT_ALL);
			break;
		}
		default:
		{
			lRetVal = PAS_FAILURE;
			break;
		}
	}

	return lRetVal;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

