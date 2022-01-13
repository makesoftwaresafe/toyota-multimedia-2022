/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ana.Chisca@continental-corporation.com
*
* Implementation of backup process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.03.20 uidu0250			CSP_WZ#2250:  Provide compress/uncompress functionality
* 2013.02.28 uidn3591           CSP_WZ#2250:  QAC + provide compress functionality
* 2013.01.08 uidn3591           CSP_WZ#2060:  Update according with latest fixes in database_helper
* 2012.11.22 uidn3591           CSP_WZ#1280:  Minor bug fixing
* 2012.11.19 uidn3591           CSP_WZ#1280:  Merge updates
* 2012.11.16 uidn3591           CSP_WZ#1280:  Initial version
*
**********************************************************************************************************************/

#include <stddef.h>
#include <string.h>
#include <stdlib.h>

#include "dlt/dlt.h"

#include "persistence_admin_service.h"
#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_database_helper.h"
#include "ssw_pers_admin_common.h"
#include "ssw_pers_admin_service.h"
#include "persComDataOrg.h"
#include "persComTypes.h"



/* L&T context */
#define LT_HDR                          "BCKUP >> "
DLT_IMPORT_CONTEXT                      (persAdminSvcDLTCtx)

#define E_INVALID_PARAMS                PAS_FAILURE_INVALID_PARAMETER
#define E_TO_BE_DEFINED                 PAS_FAILURE
#define E_OK                            PAS_SUCCESS


#define PATH_RELATIVE_MAX_SIZE          ( PERS_ORG_MAX_LENGTH_PATH_FILENAME + 1 )
#define PATH_ABS_MAX_SIZE               ( 512)
#define PATHS_DB_MAX_SIZE               (PATH_ABS_MAX_SIZE *   4)

#define NAME_APP_MAX_SIZE               (  64)
#define NAMES_APP_MAX_SIZE              (NAME_APP_MAX_SIZE * 100)

#define IS_VALID_APPL(applicationID)    ((NIL != (applicationID)) && (0 != (*(applicationID))))
#define IS_VALID_USER(user_no)          ((PERSIST_SELECT_ALL_USERS == (user_no)) || ((1 <= (user_no)) && (4 >= (user_no))))
#define IS_VALID_SEAT(seat_no)          ((PERSIST_SELECT_ALL_SEATS == (seat_no)) || ((1 <= (seat_no)) && (4 >= (seat_no))))

const char*                             gSharedPublicRelativePath   = PERS_ORG_SHARED_FOLDER_NAME PERS_ORG_PUBLIC_FOLDER_NAME_;
const char*                             gSharedGroupRelativePath    = PERS_ORG_SHARED_FOLDER_NAME PERS_ORG_GROUP_FOLDER_NAME_ "/";


static long                             persadmin_priv_data_backup_create_all                       (pstr_t backup_name);
static long                             persadmin_priv_data_backup_create_application               (pstr_t backup_name, pstr_t applicationID, uint32_t user_no, uint32_t seat_no);
static long                             persadmin_priv_data_backup_create_application_cached        (pstr_t backup_name, pstr_t applicationID, uint32_t user_no, uint32_t seat_no);
static long                             persadmin_priv_data_backup_create_application_write_through (pstr_t backup_name, pstr_t applicationID, uint32_t user_no, uint32_t seat_no);
static long                             persadmin_priv_data_backup_create_user_all                  (pstr_t backup_name, uint32_t user_no, uint32_t seat_no);
static long                             persadmin_priv_data_backup_create_user_all_cached           (pstr_t backup_name, uint32_t user_no, uint32_t seat_no, pstr_t pchNamesApp);
static long                             persadmin_priv_data_backup_create_user_all_write_through    (pstr_t backup_name, uint32_t user_no, uint32_t seat_no, pstr_t pchNamesApp);
static long                             persadmin_priv_data_backup_create_user_all_databases        (pstr_t backup_name, uint32_t user_no, uint32_t seat_no, pstr_t pchNamesApp);

static long                             persadmin_priv_data_backup_create_node                      (pconststr_t backup_name, pconststr_t pchBckupPathSrc, pconststr_t pchLocalPublicGroup);
static long                             persadmin_priv_data_backup_create_user                      (pconststr_t backup_name, pconststr_t pchBckupPathSrc, pconststr_t pchLocalPublicGroup, uint32_t user_no, uint32_t seat_no);
static long                             persadmin_priv_data_backup_create_databases                 (pconststr_t backup_name, PersASSelectionType_e type,  pconststr_t applicationID, PersistenceStorage_e eStorage, uint32_t user_no, uint32_t seat_no);

static long                             persadmin_data_backup_create_user_all_local_for_storage     (pconststr_t backup_name, pstr_t pchNamesAppTemp, pconststr_t pchPathTemp, uint32_t user_no, uint32_t seat_no);
static long                             persadmin_data_backup_create_user_all_shared_for_storage    (pconststr_t backup_name, pconststr_t pchLocalPath, pconststr_t pchSharedPublicPath, pconststr_t pchSharedGroupPath, uint32_t user_no, uint32_t seat_no);

static void                             persadmin_priv_data_backup_archive                          (PersASSelectionType_e type, pstr_t backup_name, pstr_t applicationID);

/* return positive value: number of bytes written; negative value: error code 
 */
long persadmin_data_backup_create(PersASSelectionType_e type, char* backup_name, char* applicationID, unsigned int user_no, unsigned int seat_no)
{
    /* number of written bytes; */
    sint64_t sResult = 0;

    if( (NIL == backup_name) ||
        (PersASSelectionType_LastEntry <= type) || (PersASSelectionType_All > type) )
    {
        /* set error; */
        sResult = E_INVALID_PARAMS;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("data_backup_create -"), DLT_STRING("invalid input parameters"));
    }
    else
    {
        switch( type )
        {
            /* main flow complete backup (system recovery point); */
            /* select all data/files: (node+user).(all applications+shared); */
            /* ignore ApplID, userNumber & seatNumber; */
            case PersASSelectionType_All:
            {
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("data_backup_create -"), DLT_STRING("All"));
                /* create backup; */
                sResult = persadmin_priv_data_backup_create_all(backup_name);

                break;
            }
            
            /* alternative flow application backup (application recovery point); */
            /* select application data/files: (node+user).(application); */
            case PersASSelectionType_Application:
            {
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("data_backup_create -"), DLT_STRING("Application"));
                /* create backup; */
                sResult = persadmin_priv_data_backup_create_application(backup_name, applicationID, user_no, seat_no);

                break;
            }
            
            /* alternative flow application backup (user recovery point); */
            /* select user data/files: (user).(all applications+shared); */
            case PersASSelectionType_User:
            {
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("data_backup_create -"), DLT_STRING("User"));
                /* create backup; */
                sResult = persadmin_priv_data_backup_create_user_all(backup_name, user_no, seat_no);

                break;
            }
            
            default:
            {
                /* nothing to do */
                break;
            }
        }
    }

    /* everything ok; */
    if( 0 <= sResult )
    {
        /* compute the number of bytes in /backup folder & return the result; */
        sResult = persadmin_get_folder_size(backup_name);
        /* compress backup (e.g. *.tar.gz) no matter the result; */
        persadmin_priv_data_backup_archive(type, backup_name, applicationID);
    }
    
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("data_backup_create -"), (NIL != backup_name) ? DLT_STRING(backup_name) : DLT_STRING(""), DLT_INT(sResult), DLT_STRING("bytes"));

    /* return number of written bytes; */
    return sResult;
    
} /* persadmin_data_backup_create() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_all(pstr_t backup_name)
{
    sint64_t    sResult         = 0;
    bool_t      bCanContinue    = true;
    str_t       pchBckupPathDst [PATH_ABS_MAX_SIZE];
    sint_t      s32SizeBckup    = sizeof(pchBckupPathDst);
    
    /* reset; */
    (void)memset(pchBckupPathDst, 0, (size_t)s32SizeBckup);
    
    /* backup the shared and application data; */

    /* append root path to backup name -> /backup_name/Data; */
    (void)snprintf(pchBckupPathDst, (size_t)s32SizeBckup, "%s%s", backup_name, gRootPath);

    /* TODO: check if folder exists; */
    /* copy content of source (/Data) to destination (/backup); */
    sResult = persadmin_copy_folder(gRootPath, pchBckupPathDst, PersadminFilterAll, true);
    if( 0 > sResult )
    {
        bCanContinue = false;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_all -"), DLT_STRING("persadmin_copy_folder"),
                            DLT_STRING(gRootPath), DLT_STRING("to"), DLT_STRING(pchBckupPathDst), DLT_STRING("ERR"), DLT_INT(sResult));
    }
    else
    {
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_all -"), DLT_STRING("backed up"),
                            DLT_STRING(gRootPath), DLT_STRING("to"), DLT_STRING(pchBckupPathDst));
    }

    /* TODO: filter destination folder of files which do not correspond with the definition in BackupFileList.info; */
    /* persadmin_priv_filter_folder(); */

    /* return result; */
    return sResult;
    
} /* persadmin_priv_data_backup_create_all() */


static long persadmin_priv_data_backup_create_application(pstr_t backup_name, pstr_t applicationID, uint32_t user_no, uint32_t seat_no)
{
    sint_t      sResult         = 0;

    /* check input parameters; */
    if( (!IS_VALID_APPL(applicationID)) ||
        (!IS_VALID_USER(user_no))       ||
        (!IS_VALID_SEAT(seat_no)))
    {
        /* set result; */
        sResult = E_INVALID_PARAMS;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("invalid input parameters"), DLT_INT(user_no), DLT_INT(seat_no));
    }

    /* backup application data: /Node & /User(s); */

    else
    {
        /* CACHED; */
        sResult = persadmin_priv_data_backup_create_application_cached(backup_name, applicationID, user_no, seat_no);
        if( 0 <= sResult )
        {
            /* WRITE-THROUGH; */
            sResult = persadmin_priv_data_backup_create_application_write_through(backup_name, applicationID, user_no, seat_no);
        }
        if( 0 <= sResult )
        {
            /* common for CACHED and WRITE-THROUGH - request data bases, filter & copy them at the needed location; */
            sResult = persadmin_priv_data_backup_create_databases(backup_name, PersASSelectionType_Application, applicationID, PersistenceStorage_local, user_no, seat_no);
            if( 0 > sResult )
            {
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_databases"),
                        DLT_STRING("ERR"), DLT_INT(sResult));
            }
        }
    }

    /* TODO: filter destination folder of files which do not correspond with the definition in BackupFileList.info; */
    /* persadmin_priv_filter_folder(); */
    
    /* return result; */
    return sResult;
    
} /* persadmin_priv_data_backup_create_application() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_application_cached(pstr_t backup_name, pstr_t applicationID, uint32_t user_no, uint32_t seat_no)
{
    sint_t      sResult         	= 0;
    str_t       pchPathTemp     	[PATH_ABS_MAX_SIZE];
    str_t		pchLocalCachePath	[PATH_ABS_MAX_SIZE];
    sint_t      s32SizePathTemp 	= sizeof(pchPathTemp);
    sint_t		s32SizeCachePath	= sizeof(pchLocalCachePath);

    /* copy folders; */

    /* create source path; */
    (void)memset(pchPathTemp, 0, (size_t)s32SizePathTemp);
    (void)snprintf(pchPathTemp, (size_t)s32SizePathTemp, gLocalCachePath, "", "");

    sResult = persadmin_priv_data_backup_create_node(backup_name, pchPathTemp, applicationID);
    if( 0 > sResult )
    {
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_node"),
                DLT_STRING(pchPathTemp), DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
    }

    if( 0 <= sResult )
    {
        sResult = persadmin_priv_data_backup_create_user(backup_name, pchPathTemp, applicationID, user_no, seat_no);
        if( 0 > sResult )
        {
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_user"),
                    DLT_STRING(pchPathTemp), DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
        }

        if(0 <= sResult)
        {
        	/* copy RCT file */

        	/* /Data/mnt-c/\<appId\>/\<database_name\> */
        	(void)memset(pchLocalCachePath, 0, (size_t)s32SizeCachePath);
        	(void)snprintf(pchLocalCachePath, (size_t)s32SizeCachePath, gLocalCachePath, applicationID, gResTableCfg);

        	/* \<backup_name\>/Data/mnt-c/\<appId\>/\<database_name\> */
            (void)memset(pchPathTemp, 0, (size_t)s32SizePathTemp);
            (void)snprintf(pchPathTemp, (size_t)s32SizePathTemp, "%s%s", backup_name, pchLocalCachePath);

            sResult = persadmin_copy_file(pchLocalCachePath, pchPathTemp);
            if( 0 > sResult )
            {
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_copy_file"),
						DLT_STRING(pchLocalCachePath), DLT_STRING(pchPathTemp), DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
            }
        }
    }

    return sResult;

} /* persadmin_priv_data_backup_create_application_cached() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_application_write_through(pstr_t backup_name, pstr_t applicationID, uint32_t user_no, uint32_t seat_no)
{
    sint_t      sResult         	= 0;
    str_t       pchPathTemp     	[PATH_ABS_MAX_SIZE];
    str_t		pchLocalWtPath		[PATH_ABS_MAX_SIZE];
    sint_t      s32SizePathTemp 	= sizeof(pchPathTemp);
    sint_t		s32SizeLocalWtPath	= sizeof(pchLocalWtPath);


    /* copy folders; */

    /* create source path; */
    (void)memset(pchPathTemp, 0, (size_t)s32SizePathTemp);
    (void)snprintf(pchPathTemp, (size_t)s32SizePathTemp, gLocalWtPath, "", "");

    sResult = persadmin_priv_data_backup_create_node(backup_name, pchPathTemp, applicationID);
    if( 0 > sResult )
    {
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_node"),
                DLT_STRING(pchPathTemp), DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
    }

    if( 0 <= sResult )
    {
        sResult = persadmin_priv_data_backup_create_user(backup_name, pchPathTemp, applicationID, user_no, seat_no);
        if( 0 > sResult )
        {
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_user"),
                    DLT_STRING(pchPathTemp), DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
        }

        if(0 <= sResult)
		{
        	/* /Data/mnt-wt/\<appId\>/\<database_name\> */
			(void)memset(pchLocalWtPath, 0, (size_t)s32SizeLocalWtPath);
			(void)snprintf(pchLocalWtPath, (size_t)s32SizeLocalWtPath, gLocalWtPath, applicationID, gResTableCfg);
			
			/* \<backup_name\>/Data/mnt-wt/\<appId\>/\<database_name\> */
			(void)memset(pchPathTemp, 0, (size_t)s32SizePathTemp);
			(void)snprintf(pchPathTemp, (size_t)s32SizePathTemp, "%s%s", backup_name, pchLocalWtPath);

			sResult = persadmin_copy_file(pchLocalWtPath, pchPathTemp);
			if( 0 > sResult )
			{
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_copy_file"),
						DLT_STRING(pchLocalWtPath), DLT_STRING(pchPathTemp), DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
			}
		}
    }

    return sResult;

} /* persadmin_priv_data_backup_create_application_write_through() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_user_all(pstr_t backup_name, uint32_t user_no, uint32_t seat_no)
{
    sint_t  sResult      = 0;
    bool_t  bCanContinue = true;
    str_t*  pchNamesApp  = NIL;

    /* check input parameters; */
    if( (!IS_VALID_USER(user_no)) || (!IS_VALID_SEAT(seat_no)) )
    {
        /* set result; */
        sResult = E_INVALID_PARAMS;
        /* set can continue; */
        bCanContinue = false;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("invalid input parameters"), DLT_INT(user_no), DLT_INT(seat_no));
    }

    /* backup shared + application data: /User(s); */

    if( true == bCanContinue )
    {
        /* common for CACHED and WRITE-THROUGH - get all application names; */
        sResult = persadmin_list_application_folders_get_size("");
        if( 0 >= sResult )
        {
            /* set can continue; */
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("persadmin_list_application_folders_get_size"),
                    DLT_STRING("ERR"), DLT_INT(sResult));
        }
    }

    if( true == bCanContinue )
    {
        pchNamesApp = (str_t*) malloc((size_t)sResult * sizeof(str_t)); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        if( NIL == pchNamesApp )
        {
            /* set result; */
            sResult = E_TO_BE_DEFINED;
            /* set can continue; */
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("can not allocate"),
                    DLT_INT(sResult), DLT_STRING("bytes"));
        }
        else
        {
            sResult = persadmin_list_application_folders("", pchNamesApp, sResult);
            if( 0 > sResult )
            {
                bCanContinue = false;
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("persadmin_list_application_folders"),
                        DLT_STRING("ERR"), DLT_INT(sResult));
            }
        }
    }

    if( true == bCanContinue )
    {
        /* CACHED; */
        sResult = persadmin_priv_data_backup_create_user_all_cached(backup_name, user_no, seat_no, pchNamesApp);
        if( 0 <= sResult )
        {
            /* WRITE-THROUGH; */
            sResult = persadmin_priv_data_backup_create_user_all_write_through(backup_name, user_no, seat_no, pchNamesApp);
        }
        if( 0 <= sResult )
        {
            /* common for CACHED and WRITE-THROUGH - save local & shared data bases; */
            sResult = persadmin_priv_data_backup_create_user_all_databases(backup_name, user_no, seat_no, pchNamesApp);
        }
    }

    /* free the memory; */
    if( NIL != pchNamesApp )
    {
        free(pchNamesApp); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pchNamesApp = NIL;
    }

    /* TODO: filter entire destination path and delete files, whose extensions are not in the BackupFileList.info; */

    /* return result; */
    return sResult;

} /* persadmin_priv_data_backup_create_user_all() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_user_all_cached(pstr_t backup_name, uint32_t user_no, uint32_t seat_no, pstr_t pchNamesApp)
{
    sint_t  sResult           = 0;
    bool_t  bCanContinue      = true;

    str_t  pchLocalCachePath  [PERS_ORG_MAX_LENGTH_PATH_FILENAME]; /* strlen(gLocalCachePath) + 1 */
    sint_t s32Size            = (sint_t) sizeof(pchLocalCachePath);
    str_t* pchNamesAppTemp    = pchNamesApp;

    /* create source path; */
    (void)memset(pchLocalCachePath, 0, (size_t)s32Size);
    (void)snprintf(pchLocalCachePath, (size_t)s32Size, gLocalCachePath, "", "");

    /*                                                               ("/backup", "applicationID", "/Data/mnt-c", , ); */
    sResult = persadmin_data_backup_create_user_all_local_for_storage(backup_name, pchNamesAppTemp, pchLocalCachePath, user_no, seat_no);
    if( 0 > sResult )
    {
        bCanContinue = false;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("persadmin_data_backup_create_user_all_local_for_storage"),
                DLT_STRING(pchLocalCachePath), DLT_STRING("ERR"), DLT_INT(sResult));
    }

    if( true == bCanContinue )
    {
        str_t  pchSharedPublicCachePath [PERS_ORG_MAX_LENGTH_PATH_FILENAME]; /* strlen(gSharedPublicCachePath) + 1 */
        s32Size = (sint_t) sizeof(pchSharedPublicCachePath);

        /* create source path; */
        (void)memset(pchSharedPublicCachePath, 0, (size_t)s32Size);
        (void)snprintf(pchSharedPublicCachePath, (size_t)s32Size, gSharedPublicCachePath, "");

        /*                                                                ("/backup", ","/Data/mnt-c", "/Data/mnt-c/shared/public", "/Data/mnt-c/shared/group/", , ) */
        sResult = persadmin_data_backup_create_user_all_shared_for_storage(backup_name, pchLocalCachePath, pchSharedPublicCachePath, gSharedCachePathRoot, user_no, seat_no);
        if( 0 > sResult )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage"),
                    DLT_STRING(pchLocalCachePath), DLT_STRING("ERR"), DLT_INT(sResult));
        }
    }

    return sResult;

} /* persadmin_priv_data_backup_create_user_all_cached() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_user_all_write_through(pstr_t backup_name, uint32_t user_no, uint32_t seat_no, pstr_t pchNamesApp)
{
    sint_t  sResult           = 0;
    bool_t  bCanContinue      = true;

    str_t  pchLocalWtPath     [PERS_ORG_MAX_LENGTH_PATH_FILENAME]; /* strlen(gLocalWtPath) + 1 */
    sint_t s32Size            = (sint_t)sizeof(pchLocalWtPath);
    str_t* pchNamesAppTemp    = pchNamesApp;

    /* create source path; */
    (void)memset(pchLocalWtPath, 0, (size_t)s32Size);
    (void)snprintf(pchLocalWtPath, (size_t)s32Size, gLocalWtPath, "", "");

    /*                                                               ("/backup", "applicationID", "/Data/mnt-wt", , ); */
    sResult = persadmin_data_backup_create_user_all_local_for_storage(backup_name, pchNamesAppTemp, pchLocalWtPath, user_no, seat_no);
    if( 0 > sResult )
    {
        bCanContinue = false;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("persadmin_data_backup_create_user_all_local_for_storage"),
                DLT_STRING(pchLocalWtPath), DLT_STRING("ERR"), DLT_INT(sResult));
    }

    if( true == bCanContinue )
    {
        str_t  pchSharedPublicWtPath[PERS_ORG_MAX_LENGTH_PATH_FILENAME]; /* strlen(gSharedPublicWtPath) + 1 */
        s32Size = (sint_t)sizeof(pchSharedPublicWtPath);

        /* create source path; */
        (void)memset(pchSharedPublicWtPath, 0, (size_t)s32Size);
        (void)snprintf(pchSharedPublicWtPath, (size_t)s32Size, gSharedPublicWtPath, "");

        /*                                                                ("/backup", ","/Data/mnt-wt", "/Data/mnt-wt/shared/public", "/Data/mnt-wt/shared/group/", , ) */
        sResult = persadmin_data_backup_create_user_all_shared_for_storage(backup_name, pchLocalWtPath, pchSharedPublicWtPath, gSharedCachePathRoot, user_no, seat_no);
        if( 0 > sResult )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user_all -"), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage"),
                    DLT_STRING(pchLocalWtPath), DLT_STRING("ERR"), DLT_INT(sResult));
        }
    }

    return sResult;

} /* persadmin_priv_data_backup_create_user_all_write_through() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_user_all_databases(pstr_t backup_name, uint32_t user_no, uint32_t seat_no, pstr_t pchNamesApp)
{
    sint_t  sResult           = 0;
    bool_t  bCanContinue      = true;

    str_t* pchNamesAppTemp    = pchNamesApp;

    /* go through all applications and save local data; */
    while( (0 != *pchNamesAppTemp) && (true == bCanContinue) )
    {
        /* save & filter local databases; */
        sResult = persadmin_priv_data_backup_create_databases(backup_name, PersASSelectionType_User, pchNamesAppTemp, PersistenceStorage_local, user_no, seat_no);
        if( 0 > sResult )
        {
          bCanContinue = false;
          /* some info; */
          DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_databases - local"),
                 DLT_STRING(pchNamesAppTemp), DLT_STRING("ERR"), DLT_INT(sResult));
        }

        if( true == bCanContinue )
        {
            /* move on to the next data base; */
            pchNamesAppTemp += (strlen(pchNamesAppTemp) + 1 /* \0 */);
        }
    }

    if( true == bCanContinue )
    {
        /* go through all applications and save shared data; */
        sResult = persadmin_priv_data_backup_create_databases(backup_name, PersASSelectionType_User, "", PersistenceStorage_shared, user_no, seat_no);
        if( 0 > sResult )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_application -"), DLT_STRING("persadmin_priv_data_backup_create_databases - shared"),
                 DLT_STRING("ERR"), DLT_INT(sResult));
        }
    }

    return sResult;

} /* persadmin_priv_data_backup_create_user_all_databases() */


static long persadmin_priv_data_backup_create_node(pconststr_t backup_name, pconststr_t pchBckupPathSrc, pconststr_t pchLocalPublicGroup)
{
    sint_t      sResult             = 0;
    str_t       pchPathTempSrc      [PATH_ABS_MAX_SIZE];
    str_t       pchPathTempDst      [PATH_ABS_MAX_SIZE];
    sint_t      s32SizePathTemp     = PATH_ABS_MAX_SIZE;

    /* /Node; */

    /* reset; */
    (void)memset(pchPathTempSrc, 0, (size_t)s32SizePathTemp);
    (void)memset(pchPathTempDst, 0, (size_t)s32SizePathTemp);

    /* create source; */
    (void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s", pchBckupPathSrc, pchLocalPublicGroup, gNode);

    /* check if /Node folder exists; */
    if( 0 == persadmin_check_if_file_exists(pchPathTempSrc, true) )
    {
        /* create destination; */
        (void)snprintf(pchPathTempDst, (size_t)s32SizePathTemp, "%s%s", backup_name, pchPathTempSrc);

        /* copy /Node to destination; */
        sResult = persadmin_copy_folder(pchPathTempSrc, pchPathTempDst, PersadminFilterAll, true);
        if( 0 > sResult )
        {
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_node -"), DLT_STRING("persadmin_copy_folder"),
                    DLT_STRING(pchPathTempSrc), DLT_STRING("ERR"),DLT_INT(sResult));
        }
        else
        {
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_node -"), DLT_STRING("backed up"),
                                DLT_STRING(pchPathTempSrc), DLT_STRING("to"), DLT_STRING(pchPathTempDst));
        }
    }
    else
    {
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_node -"),
                            DLT_STRING(pchPathTempSrc), DLT_STRING("does not exist"));
    }

    /* return result; */
    return sResult;

} /* persadmin_priv_data_backup_create_node() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_user(pconststr_t backup_name, pconststr_t pchBckupPathSrc, pconststr_t pchLocalPublicGroup, uint32_t user_no, uint32_t seat_no)
{
    sint_t      sResult             = 0;
    str_t       pchPathTempSrc      [PATH_ABS_MAX_SIZE];
    str_t       pchPathTempDst      [PATH_ABS_MAX_SIZE];
    sint_t      s32SizePathTemp     = PATH_ABS_MAX_SIZE;

    /* /User(s); */

    /* reset; */
    (void)memset(pchPathTempSrc, 0, (size_t)s32SizePathTemp);
    (void)memset(pchPathTempDst, 0, (size_t)s32SizePathTemp);

    /* create source; */
    /* all users & all seats; */
    if( (PERSIST_SELECT_ALL_USERS == user_no) && (PERSIST_SELECT_ALL_SEATS == seat_no) )
    {
        (void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s", pchBckupPathSrc, pchLocalPublicGroup, gUser);
    }
    /* only one user & all seats; */
    if( (PERSIST_SELECT_ALL_USERS != user_no) && (PERSIST_SELECT_ALL_SEATS == seat_no) )
    {
        (void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s%d", pchBckupPathSrc, pchLocalPublicGroup, gUser, user_no);
    }
    /* only one user & one seat; */
    if( (PERSIST_SELECT_ALL_USERS != user_no) && (PERSIST_SELECT_ALL_SEATS != seat_no) )
    {
        (void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s%d%s%d", pchBckupPathSrc, pchLocalPublicGroup, gUser, user_no, gSeat, seat_no);
    }

    if( 0 == persadmin_check_if_file_exists(pchPathTempSrc, true) )
    {
        /* create destination; */
        (void)snprintf(pchPathTempDst, (size_t)s32SizePathTemp, "%s%s", backup_name, pchPathTempSrc);

        /* copy /User(s) to destination; */
        sResult = persadmin_copy_folder(pchPathTempSrc, pchPathTempDst, PersadminFilterAll, true);
        if( 0 > sResult )
        {
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user -"), DLT_STRING("persadmin_copy_folder"),
                    DLT_STRING(pchPathTempSrc), DLT_STRING("ERR"),DLT_INT(sResult));
        }
        else
        {
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user -"), DLT_STRING("backed up"),
                                DLT_STRING(pchPathTempSrc), DLT_STRING("to"), DLT_STRING(pchPathTempDst));
        }
    }
    else
    {
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_user -"),
                            DLT_STRING(pchPathTempSrc), DLT_STRING("does not exist"));
    }

    /* return result; */
    return sResult;

} /* persadmin_priv_data_backup_create_user() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_priv_data_backup_create_databases(pconststr_t backup_name, PersASSelectionType_e type, pconststr_t applicationID, PersistenceStorage_e eStorage, uint32_t user_no, uint32_t seat_no)
{
    sint_t      sResult         = 0;
    bool_t      bCanContinue    = true;
    str_t       pchDBPaths      [PATHS_DB_MAX_SIZE];
    sint_t      s32SizeDBPaths  = sizeof(pchDBPaths);

    /* reset; */
    (void)memset(pchDBPaths, 0, (size_t)s32SizeDBPaths);

    if( (NIL == applicationID) ||
        (PersistenceStorage_LastEntry <= eStorage) || (PersistenceStorage_local > eStorage) )
    {
        /* set error; */
        sResult = E_INVALID_PARAMS;
        /* set can continue; */
        bCanContinue = false;
        /* some info; */
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_databases -"), DLT_STRING("invalid input parameters"));
    }

    if( true == bCanContinue )
    {
        /* request data bases; */
        sResult = persadmin_get_all_db_paths_with_names("", applicationID, eStorage, pchDBPaths, s32SizeDBPaths);
        if( 0 > sResult )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_databases -"), DLT_STRING("persadmin_get_all_db_paths_with_names"),
                    DLT_STRING(applicationID), DLT_STRING("ERR"), DLT_INT(sResult));
        }
    }

    if( true == bCanContinue )
    {
        str_t    pchPathTemp     [PATH_ABS_MAX_SIZE];
        sint_t   s32SizePathTemp = sizeof(pchPathTemp);
        str_t*   pchDB           = pchDBPaths;

        /* save the actual data size; */
        s32SizeDBPaths = sResult;

        /* go through all data bases & update them; */
        while( (0 != *pchDB) && (true == bCanContinue) )
        {
            /* reset; */
            (void)memset(pchPathTemp, 0, (size_t)s32SizePathTemp);
            /* create destination path; */
            (void)snprintf(pchPathTemp, (size_t)s32SizePathTemp, "%s%s", backup_name, pchDB);

            /* filter destination data base based on the user/seat information; */
            sResult = persadmin_copy_keys_by_filter(type, pchPathTemp, pchDB, user_no, seat_no);

            if( 0 > sResult )
            {
                bCanContinue = false;
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_create_databases -"), DLT_STRING("persadmin_get_all_db_paths_with_names"),
                            DLT_STRING(pchPathTemp), DLT_STRING("ERR"), DLT_INT(sResult));
            }

            if( true == bCanContinue )
            {
                /* move on to the next data base; */
                pchDB += (strlen(pchDB) + 1 /* \0 */);
            }
        }
    }

    /* return result; */
    return sResult;

} /* persadmin_priv_data_backup_create_databases() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 7-SSW_Administrator_0004*/


static long persadmin_data_backup_create_user_all_local_for_storage(pconststr_t backup_name, pstr_t pchNamesAppTemp, pconststr_t pchPathTemp, uint32_t user_no, uint32_t seat_no)
{
    sint_t      sResult           = 0;
    bool_t      bCanContinue      = true;
    str_t  		pchPathTempSrc    [PATH_ABS_MAX_SIZE];
	str_t  		pchPathTempDst    [PATH_ABS_MAX_SIZE];
	sint_t 		s32SizePathTemp   = PATH_ABS_MAX_SIZE;

    /* go through all applications and save local data; */
    while( (0 != *pchNamesAppTemp) && (true == bCanContinue) )
    {
        /* copy folders; */
        /* persadmin_priv_data_backup_create_user("/backup", "/Data/mnt-c/", "applicationID", user_no, seat_no); */
        /* persadmin_priv_data_backup_create_user("/backup", "/Data/mnt-wt/", "applicationID", user_no, seat_no); */
        sResult = persadmin_priv_data_backup_create_user(backup_name, pchPathTemp, pchNamesAppTemp, user_no, seat_no);

        if( 0 > sResult )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_local_for_storage -"), DLT_STRING("persadmin_priv_data_backup_create_user"),
                  DLT_STRING(pchPathTemp), DLT_STRING(pchNamesAppTemp), DLT_STRING("ERR"), DLT_INT(sResult));
        }

        if(true == bCanContinue)
        {
        	/* copy RCT file */

			/* /Data/mnt-c/\<appId\>/\<database_name\> */
        	/* /Data/mnt-wt/\<appId\>/\<database_name\> */
			(void)memset(pchPathTempSrc, 0, (size_t)s32SizePathTemp);
			(void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s", pchPathTemp, pchNamesAppTemp, gResTableCfg);

			/* \<backup_name\>/Data/mnt-c/\<appId\>/\<database_name\> */
			/* \<backup_name\>/Data/mnt-wt/\<appId\>/\<database_name\> */
			(void)memset(pchPathTempDst, 0, (size_t)s32SizePathTemp);
			(void)snprintf(pchPathTempDst, (size_t)s32SizePathTemp, "%s%s", backup_name, pchPathTempSrc);

			sResult = persadmin_copy_file(pchPathTempSrc, pchPathTempDst);
			if( 0 > sResult )
			{
				bCanContinue = false;
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_local_for_storage -"), DLT_STRING("persadmin_copy_file"),
						DLT_STRING(pchPathTempSrc), DLT_STRING(pchPathTempDst), DLT_STRING("ERR"), DLT_INT(sResult));
			}
        }

        if( true == bCanContinue )
        {
            /* export links for each application; */

            /* reset; */
            (void)memset(pchPathTempSrc, 0, (size_t)s32SizePathTemp);
            (void)memset(pchPathTempDst, 0, (size_t)s32SizePathTemp);
            /* /Data/mnt-c/applicationID; */
            /* /Data/mnt-wt/applicationID; */
            (void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s", pchPathTemp, pchNamesAppTemp);
            /* /backup/Data/mnt-c/applicationID;  */
            /* /backup/Data/mnt-wt/applicationID; */
            (void)snprintf(pchPathTempDst, (size_t)s32SizePathTemp, "%s%s", backup_name, pchPathTempSrc);

            sResult = persadmin_export_links(pchPathTempSrc, pchPathTempDst);
            if( 0 > sResult )
            {
                bCanContinue = false;
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_local_for_storage -"), DLT_STRING("persadmin_export_links"),
                        DLT_STRING(pchPathTempSrc), DLT_STRING(pchPathTempDst), DLT_STRING("ERR"), DLT_INT(sResult));
            }
        }

        if( true == bCanContinue )
        {
            /* move on to the next data base; */
            pchNamesAppTemp += (strlen(pchNamesAppTemp) + 1 /* \0 */);
        }
    }

    /* return result; */
    return sResult;

} /* persadmin_data_backup_create_user_all_local_for_storage() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


static long persadmin_data_backup_create_user_all_shared_for_storage(pconststr_t backup_name, pconststr_t pchLocalPath, pconststr_t pchSharedPublicPath, pconststr_t pchSharedGroupPath, uint32_t user_no, uint32_t seat_no)
{
    sint_t	sResult      	= 0;
    bool_t 	bCanContinue 	= true;
    str_t  	pchPathTempSrc    [PATH_ABS_MAX_SIZE];
    str_t  	pchPathTempDst    [PATH_ABS_MAX_SIZE];
    sint_t 	s32SizePathTemp = PATH_ABS_MAX_SIZE;

    /* save shared user data; */
    if( 0 == persadmin_check_if_file_exists(pchSharedPublicPath, true) )
    {
        /* persadmin_priv_data_backup_create_user("/backup", "/Data/mnt-c", "/shared/public", user_no, seat_no); */
        /* persadmin_priv_data_backup_create_user("/backup", "/Data/mnt-wt", "/shared/public", user_no, seat_no); */
        sResult = persadmin_priv_data_backup_create_user(backup_name, pchLocalPath, gSharedPublicRelativePath, user_no, seat_no);
        if( 0 > sResult )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage -"), DLT_STRING("persadmin_priv_data_backup_create_user"),
                   DLT_STRING("ERR"), DLT_INT(sResult));
        }

        if(true == bCanContinue)
        {
        	/* copy RCT file */

			/* /Data/mnt-c/shared/public/\<database_name\> */
        	/* /Data/mnt-wt/shared/public/\<database_name\> */
			(void)memset(pchPathTempSrc, 0, (size_t)s32SizePathTemp);
			(void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s", pchLocalPath, gSharedPublicRelativePath, gResTableCfg);

			/* /<backup_name\>/Data/mnt-c/\<appId\>/\<database_name\> */
			/* /<backup_name\>/Data/mnt-wt/\<appId\>/\<database_name\> */
			(void)memset(pchPathTempDst, 0, (size_t)s32SizePathTemp);
			(void)snprintf(pchPathTempDst, (size_t)s32SizePathTemp, "%s%s", backup_name, pchPathTempSrc);

			sResult = persadmin_copy_file(pchPathTempSrc, pchPathTempDst);
			if( 0 > sResult )
			{
				bCanContinue = false;
				DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage -"), DLT_STRING("persadmin_copy_file"),
						DLT_STRING(pchPathTempSrc), DLT_STRING(pchPathTempDst), DLT_STRING("ERR"), DLT_INT(sResult));
			}
        }
    }

    if( (0 == persadmin_check_if_file_exists(pchSharedGroupPath, true)) &&
        (true == bCanContinue))
    {
        pstr_t  pstrBufferTemp              = NIL;
        sint_t  s32SizeTemp                 = 0;
        str_t   pchSharedGroupRelativePath  [PATH_RELATIVE_MAX_SIZE];
        sint_t  s32SizePath                 = sizeof(pchSharedGroupRelativePath);

        /* list all folders; */
        sint_t s32FolderSize    = 0;
        pstr_t pstrBuffer       = NIL;

        /* list source folder; */
        sResult = s32FolderSize = persadmin_list_folder_get_size(pchSharedGroupPath, PersadminFilterFolders, false);

        if( 0 > s32FolderSize )
        {
            bCanContinue = false;
            /* some info; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage -"), DLT_STRING("persadmin_list_folder_get_size"),
                   DLT_STRING("ERR"), DLT_INT(sResult));
        }

        if( true == bCanContinue )
        {
            pstrBuffer = (pstr_t) malloc((size_t)s32FolderSize * sizeof(str_t)); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if( NIL != pstrBuffer )
            {
                /* list folder "/Data/mnt-c/shared/group/"; */
                /* list folder "/Data/mnt-wt/shared/group/"; */
                sResult = persadmin_list_folder(pchSharedGroupPath, pstrBuffer, s32FolderSize, PersadminFilterFolders, false);
                if( s32FolderSize != sResult )
                {
                    bCanContinue = false;
                    /* some info; */
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage -"), DLT_STRING("persadmin_list_folder"),
                           DLT_STRING("ERR"), DLT_INT(sResult), DLT_INT(s32FolderSize));
                }
                else
                {
                    pstrBufferTemp = pstrBuffer;
                }
            }
            else
            {
                bCanContinue = false;
            }
        }

        while( (s32SizeTemp < s32FolderSize) && (true == bCanContinue) )
        {
            /* TODO: optimize so that only groupID is appended -> /shared/group/ is common; */
            /* create source relative path "shared/group/10"; */
            (void)memset(pchSharedGroupRelativePath, 0, (size_t)s32SizePath);
            (void)snprintf(pchSharedGroupRelativePath, (size_t)s32SizePath, "%s%s", gSharedGroupRelativePath, pstrBufferTemp);

            /* for each folder create the relative path "/shared/group/10" & backup user data; */
            /* persadmin_priv_data_backup_create_user("/backup", "/Data/mnt-c", "/shared/group/10", user_no, seat_no); */
            sResult = persadmin_priv_data_backup_create_user(backup_name, pchLocalPath, pchSharedGroupRelativePath, user_no, seat_no);
            if( 0 > sResult )
            {
                bCanContinue = false;
                /* some info; */
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage -"), DLT_STRING("persadmin_priv_data_backup_create_user"),
                       DLT_STRING(pstrBufferTemp), DLT_STRING("ERR"), DLT_INT(sResult));
            }

            if(true == bCanContinue)
			{
				/* copy RCT file */

				/* /Data/mnt-c/shared/group/\<group_id\>\<database_name\> */
            	/* /Data/mnt-wt/shared/group/\<group_id\>\<database_name\> */
				(void)memset(pchPathTempSrc, 0, (size_t)s32SizePathTemp);
				(void)snprintf(pchPathTempSrc, (size_t)s32SizePathTemp, "%s%s%s", pchLocalPath, pchSharedGroupRelativePath, gResTableCfg);


				/* \<backup_name\>/Data/mnt-c/shared/group/\<group_id\>\<database_name\> */
				/* \<backup_name\>/Data/mnt-wt/shared/group/\<group_id\>\<database_name\> */
				(void)memset(pchPathTempDst, 0, (size_t)s32SizePathTemp);
				(void)snprintf(pchPathTempDst, (size_t)s32SizePathTemp, "%s%s", backup_name, pchPathTempSrc);

				sResult = persadmin_copy_file(pchPathTempSrc, pchPathTempDst);
				if( 0 > sResult )
				{
					bCanContinue = false;
					DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("persadmin_data_backup_create_user_all_shared_for_storage -"), DLT_STRING("persadmin_copy_file"),
							DLT_STRING(pchPathTempSrc), DLT_STRING(pchPathTempDst), DLT_STRING("ERR"), DLT_INT(sResult));
				}
			}

            if( true == bCanContinue )
            {
                /* move on to the next group "10", "20" ...; */
                s32SizeTemp += ((sint_t)strlen(pstrBufferTemp) + 1 /* \0 */);
                pstrBufferTemp += s32SizeTemp;
            }
        }

        /* free the memory; */
        if( NIL != pstrBuffer )
        {
            free(pstrBuffer); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            pstrBuffer = NIL;
        }
    }

    /* return result; */
    return sResult;

} /* persadmin_data_backup_create_user_all_shared_for_storage() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 7-SSW_Administrator_0004*/


static void persadmin_priv_data_backup_archive(PersASSelectionType_e type, pstr_t backup_name, pstr_t applicationID)
{
    sint_t sResult = 0;

    if( 0 > persadmin_check_if_file_exists(BACKUP_LOCATION, true) )
    {
        sResult = persadmin_create_folder(BACKUP_LOCATION);
    }

    if( 0 == sResult )
    {
        str_t  pchPathCompressTo   [PATH_ABS_MAX_SIZE];
        str_t  pchPathCompressFrom [PATH_ABS_MAX_SIZE];

        (void)snprintf(pchPathCompressFrom, sizeof(pchPathCompressFrom), "%s%s", backup_name, PERS_ORG_ROOT_PATH);

        /* create the tar name; */
        switch( type )
        {
            case PersASSelectionType_Application:
            {
                /* if( IS_VALID_APPL(applicationID) ) already checked in persadmin_priv_data_backup_create_application */
                (void)snprintf(pchPathCompressTo, sizeof(pchPathCompressTo), "%s%s%s", BACKUP_LOCATION, applicationID, BACKUP_FORMAT);
                break;
            }
            case PersASSelectionType_User:
            {
                (void)snprintf(pchPathCompressTo, sizeof(pchPathCompressTo), "%s%s%s", BACKUP_LOCATION, "user", BACKUP_FORMAT);
                break;
            }
            case PersASSelectionType_All:
            {
                (void)snprintf(pchPathCompressTo, sizeof(pchPathCompressTo), "%s%s%s", BACKUP_LOCATION, "all", BACKUP_FORMAT);
                break;
            }
            default:
            {
                /* nothing to do */
                break;
            }
        }

        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_archive -"),
                DLT_STRING("create"),
                DLT_STRING("<"),
                DLT_STRING(pchPathCompressTo),
                DLT_STRING(">"),
                DLT_STRING("from"),
                DLT_STRING("<"),
                DLT_STRING(pchPathCompressFrom),
                DLT_STRING(">"));

        /* usage persadmin_compress("/path/to/compress/to/archive_name.tgz", "/path/from/where/to/compress"); */
        /* return 0 for success, negative value otherwise; */
        sResult = persadmin_compress(pchPathCompressTo, pchPathCompressFrom);
        if( 0 > sResult )
        {
            /* warning; */
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_WARN, DLT_STRING(LT_HDR), DLT_STRING("persadmin_priv_data_backup_archive -"), DLT_INT(sResult));
        }
    }

} /* persadmin_priv_data_backup_archive() */ /*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

