/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Implementation of resource_cong_add funtionality
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author             Reason
* 2013.10.23 uidl9757           CSP_WZ#6300:  Installation of the default values in context of the custom keys not working
* 2013.09.27 uidl9757           CSP_WZ#5781:  Fix memory leakage
* 2013.09.23 uidl9757           CSP_WZ#5781:  Watchdog timeout of pas-daemon
* 2013.06.07 uidl9757           CSP_WZ#4260:  Variable sContext of type pas_cfg_instFoldCtx_s is not initialized.
* 2013.04.12 uidl9757           CSP_WZ#3450:  Fix SCC warnings
* 2013.04.02 uidl9757           CSP_WZ#2798:  Fix some of review findings
* 2013.03.21 uidl9757           CSP_WZ#2798:  Complete rework to allow configuration based on json files
* 2012.12.13 uidl9757           CSP_WZ#1280:  Update according to new detailed description:
                                              https://workspace1.conti.de/content/00002483/Team%20Documents/01%20Technical%20Documentation
                                              /04%20OIP/20_SW_Packages/03_Persistence/Administration_Service
                                              /Configure%20and%20initialize%20data%20and%20policy%20for%20new%20application%20-%20questions.doc
* 2012.11.29 uidl9757           CSP_WZ#1280:  Created
*
**********************************************************************************************************************/ 

/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include "stdio.h" /*DG C8MR2R-MISRA-C:2004 Rule 20.9-SSW_Administrator_0007*/
#include "string.h"
#include "stdlib.h"
#include <stddef.h>
#include <unistd.h>
#include <sys/file.h>
#include "fcntl.h"
#include <errno.h>
#include <sys/stat.h>
#include "dlt.h"


#include "ssw_pers_admin_service.h"

#include "persComErrors.h"
#include "persComDataOrg.h"
#include "persComDbAccess.h"
#include "persComRct.h"

#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_common.h"
#include "persistence_admin_service.h"

#include "ssw_pers_admin_config.h"


/******************************************************************************************************
********************************   Local defines  *****************************************************
******************************************************************************************************/
/* !!!!!! TO BE EXPOSED IN A INTERFACE !!!! */
#define PERSADMIN_MIN_USER_NO 1
#define PERSADMIN_NO_OF_USERS 4
#define PERSADMIN_MAX_USER_NO ((PERSADMIN_MIN_USER_NO)+(PERSADMIN_NO_OF_USERS)-1)
#define PERSADMIN_MIN_SEAT_NO 1
#define PERSADMIN_NO_OF_SEATS 4
#define PERSADMIN_MAX_SEAT_NO ((PERSADMIN_MIN_SEAT_NO)+(PERSADMIN_NO_OF_SEATS)-1)

/* folder where the input resource is un-compressed */
#define PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME "/tmp/PAS/cfg/"
/* the root folder of the un-compressed input */
#define PERSADM_CFG_SOURCE_ROOT_FOLDER_PATH PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_ROOT_FOLDER_NAME

/* root folder - configuration data for shared public */
#define PERSADM_CFG_SOURCE_PUBLIC_FOLDER_PATH PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_PUBLIC_FOLDER_PATHNAME
/* root folder - configuration data for shared group */
#define PERSADM_CFG_SOURCE_GROUPS_FOLDER_PATH PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_GROUPS_FOLDER_PATHNAME
/* root folder - configuration data for applications */
#define PERSADM_CFG_SOURCE_APPS_FOLDER_PATH PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME PERSADM_CFG_RESOURCE_APPS_FOLDER_PATHNAME

/* root folder - configurations for key type resources */
#define PERSADM_CFG_SOURCE_KEY_TYPE_DATA_FOLDER_RELATIVE_PATH "/"PERSADM_CFG_RESOURCE_ROOT_KEYDATA_FOLDER_NAME
/* root folder - configurations for file type resources */
#define PERSADM_CFG_SOURCE_FILE_TYPE_DATA_FOLDER_RELATIVE_PATH "/"PERSADM_CFG_RESOURCE_ROOT_FILEDATA_FOLDER_NAME

/******************************************************************************************************
********************************   Local macros  ******************************************************
******************************************************************************************************/


/******************************************************************************************************
********************************   Local structures and enums  ****************************************
******************************************************************************************************/

/*-----------------------------------------------------------------------------------------------------
Install folder related (common for public/group/app)
-----------------------------------------------------------------------------------------------------*/
/* to handle information about availablity of files */
typedef struct pas_conf_fileInfo_s_
{
    bool_t  bIsAvailable ;                          /* the file indicated by pathname exists */
    str_t   pathname[PERSADMIN_MAX_PATH_LENGHT];    /* pathname */
    sint_t  handler ;                               /* open handler to file (only if needed) */
}pas_conf_fileInfo_s ;

/* structure to make easier to obtain the context for destination install folder
*  allows to use a loop instead of individual calls */
typedef struct pas_conf_helpDestContextEntry_s_
{
    bool_t                      bIsFolder ;             /* specify if we deal with a file or a folder */
    pas_conf_fileInfo_s*        psFileAvailability ;    /* availability (in the install folder) of file/folder specified by filename */
    pstr_t                      filename ;              /* file or folder name (not the complete path) */
}pas_conf_helpDestContextEntry_s ;

/* structure to make easier to obtain the context for source install folder
*  allows to use a loop instead of individual calls */
typedef struct pas_conf_helpSrcContextEntry_s_
{
    bool_t                      bIsFolder ;             /* specify if we deal with a file or a folder */
    pas_conf_fileInfo_s*        psFileAvailability ;    /* availability (in the install folder) of file/folder specified by filename */
    pstr_t                      filename ;              /* file or folder name (not the complete path) */
    PersAdminCfgFileTypes_e     eCfgFileType ;          /* file type */
    bool_t                      bPresenceIsMandatory ;  /* true if presence of the file is mandatory */
}pas_conf_helpSrcContextEntry_s ;

/* the context for destination(public/group/app) install folder */
typedef struct pas_cfg_destInstallFolderContext_s_
{
    pas_conf_fileInfo_s   sInstallFolder ;            /* destination install folder */    
    pas_conf_fileInfo_s   sRctDB ;                    /* destination RCT DB */
    pas_conf_fileInfo_s   sCachedDB ;                 /* destination cached DB */
    pas_conf_fileInfo_s   sWtDB ;                     /* destination un-cached DB */
    pas_conf_fileInfo_s   sFactoryDefaultDB ;         /* destination factory-default DB */
    pas_conf_fileInfo_s   sConfigurableDefaultDB ;    /* destination configurable-default DB */
}pas_cfg_destInstallFolderContext_s ;


/* the context for source(public/group/app) install folder */
typedef struct pas_cfg_srcInstallFolderContext_s_
{
    pas_conf_fileInfo_s   sInstallFolder ;                /* source install folder */
    pas_conf_fileInfo_s   sFiledataFolder ;               /* source filedata folder */    
    pas_conf_fileInfo_s   sKeydataFolder ;                /* source keydata folder */ 
    pas_conf_fileInfo_s   sRctFile ;                      /* source RCT configuration file */
    pas_conf_fileInfo_s   sFactoryDefaultDataFile ;       /* source factory-default data configuration file */
    pas_conf_fileInfo_s   sConfigurableDefaultDataFile ;  /* source configurable-default data configuration file */
    pas_conf_fileInfo_s   sInitialDataFile ;              /* source initial non-default data configuration file */
    pas_conf_fileInfo_s   sExceptionsFile ;               /* source install exceptions file */  
}pas_cfg_srcInstallFolderContext_s ;

/* info about list (items in list are '\0' separated) */
typedef struct pas_conf_listOfItems_s_
{
    sint_t  sizeOfList ;    /* size of list in bytes */
    pstr_t  pList ;         /* pointer to the list */
}pas_conf_listOfItems_s;

/* handle together the lists for an install folder (public/group/app) */
typedef struct pas_conf_listsForInstallFolder_s_
{
    pas_conf_listOfItems_s  sSrcRCT ;               /* list of resources in source RCT */
    pas_conf_listOfItems_s  sDestRCT ;              /* list of resources in destination RCT */
    pas_conf_listOfItems_s  sObsoleteResources ;    /* list of resources no longer present in source RCT */
}pas_conf_listsForInstallFolder_s;

/* top structure for an install folder (public/group/app) */
typedef struct pas_cfg_instFoldCtx_s_
{
    PersAdminCfgInstallRules_e          eInstallRule ;  /* install rule for the public/grup/app */
    pas_cfg_destInstallFolderContext_s  sDestContext ;  /* context for destination folder */
    pas_cfg_srcInstallFolderContext_s   sSrcContext ;   /* context for source folder */
    pas_conf_listsForInstallFolder_s    sLists ;        /* lists for install folder (source and destination) */
}pas_cfg_instFoldCtx_s ;

/*-----------------------------------------------------------------------------------------------------
Resource related
-----------------------------------------------------------------------------------------------------*/
/* the types of configuration possible for a resource */
typedef enum
{
    pas_cfg_configType_update = 0,  /* (over)write configuration for the resource */
    pas_cfg_configType_dontTouch,   /* don't touch the configuration for the resource */
    pas_cfg_configType_delete       /* remove the configuration for the resource */
}pas_cfg_configTypes_e ;

/* context for key's policy change */
typedef struct pas_conf_keyPolicyChangeCtx_s_
{
    PersistenceConfigurationKey_s*  psSrcConfig ;   /* new(source)configuration for resource */
    PersistenceConfigurationKey_s*  psDestConfig ;  /* old(destination)configuration for resource */
    sint_t                          hDestCachedDB ; /* handler to local cached DB */
    sint_t                          hDestWtDB ;     /* handler to local un-cached DB */
}pas_conf_keyPolicyChangeCtx_s ;

/*-----------------------------------------------------------------------------------------------------
Default-data related
-----------------------------------------------------------------------------------------------------*/
/* for an easier management of file handlers for default data */
typedef struct pas_conf_handlersForDefault_s_
{
    sint_t hDestRctDB ;             /* handler to destination RCT DB */
    sint_t hDestDefaultDataDB ;     /* handler to destination default-data DB */
    sint_t hSrcRctFile ;            /* handler to source RCT configuration file */
    sint_t hSrcDefaultDataFile ;    /* handler to source default-data configuration file */
    sint_t hSrcExceptions ;         /* handler to source install-exceptions configuration file */
}pas_conf_handlersForDefault_s ;

/*-----------------------------------------------------------------------------------------------------
Non-default-data related
-----------------------------------------------------------------------------------------------------*/
/* for an easier management of file handlers for non-default data */
typedef struct pas_conf_handlersForNonDefault_s_
{
    sint_t hDestRctDB ;             /* handler to destination RCT DB */
    sint_t hDestCachedDataDB ;      /* handler to destination cached-data DB */
    sint_t hDestWtDataDB ;          /* handler to destination writethrough-data DB */
    sint_t hSrcRctFile ;            /* handler to source RCT configuration file */
    sint_t hSrcExceptions ;         /* handler to source install-exceptions configuration file */
}pas_conf_handlersForNonDefault_s ;

/*-----------------------------------------------------------------------------------------------------
Look-Up table related
-----------------------------------------------------------------------------------------------------*/
/* types of data in regards to origin (default, runtime) */
typedef enum pas_cfg_dataTypes_e
{
    pas_cfg_dataTypes_DefaultFactory = 0,   /* factory-default */
    pas_cfg_dataTypes_DefaultConfigurable,  /* configurable-default */
    pas_cfg_dataTypes_NonDefault,           /* non-default (initial or runtime)*/
    /* add new entries here */
    pas_cfg_dataTypes_LastEntry
}pas_cfg_dataTypes_e ;

/* entry in Look-Up table for install rules/exceptions */
typedef struct pas_cfg_lookUpEntry_s_
{
    pas_cfg_configTypes_e   eDefaultConfigType ;                                        /* default rule to be applied on resources */
    bool_t                  affectedDataLookUp[pas_cfg_dataTypes_LastEntry] ;           /* look-up table for data affected by install rule */
    bool_t                  exceptionsLookUp[PersAdminCfgInstallExceptions_LastEntry] ; /* look-up table for exceptions allowed for an install rule */
}pas_cfg_lookUpEntry_s ;



/******************************************************************************************************
********************************   Local variables  ***************************************************
******************************************************************************************************/

/* Look-Up table for relations between install rules, affected data and install exceptions */
static pas_cfg_lookUpEntry_s g_LookUpRules[PersAdminCfgInstallRules_LastEntry] = 
{
        /* ********************** **************************    **********************************    *********************************
           **** install rule **** *** default config type **    ******** affected data  **********    *******  allowed exceptions *****
           ********************** **************************    **********************************    *********************************                                
                                                                factory     config    non-default      update    dontTouch     delete
                                                                default     default                                                      */
                                    
           /*     NewInstall   */ {pas_cfg_configType_update,   {true,      true,       true},          {false,     false,      false}  },
           /*     Uninstall    */ {pas_cfg_configType_delete,   {true,      true,       true},          {false,     false,      false}  },
           /*     DontTouch    */ {pas_cfg_configType_dontTouch,{false,     false,      false},         {false,     false,      false}  },            
           /*     UpdateAll    */ {pas_cfg_configType_update,   {true,      true,       true},          {false,     true,       false}  },           
/* UpdateAllSkipDefaultFactory */ {pas_cfg_configType_update,   {false,     true,       true},          {false,     true,       false}  },
/* UpdateAllSkipDefaultConfig  */ {pas_cfg_configType_update,   {true,      false,      true},          {false,     true,       false}  },
    /* UpdateAllSkipDefaultAll */ {pas_cfg_configType_update,   {false,     false,      true},          {false,     true,       false}  },
       /* UpdateDefaultFactory */ {pas_cfg_configType_update,   {true,      false,      false},         {false,     true,       false}  },
        /* UpdateDefaultConfig */ {pas_cfg_configType_update,   {false,     true,       false},         {false,     true,       false}  },
           /* UpdateDefaultAll */ {pas_cfg_configType_update,   {true,      true,       false},         {false,     true,       false}  },
       /* UpdateSetOfResources */ {pas_cfg_configType_dontTouch,{true,      true,       true},          {true,      false,      true}   },
        /* UninstallNonDefault */ {pas_cfg_configType_delete,   {false,     false,      true},          {false,     true,       false}  }       
} ;


/******************************************************************************************************
********************************   DLT related  *******************************************************
******************************************************************************************************/
/* L&T context */
#define LT_HDR                          "CONF >> "
DLT_IMPORT_CONTEXT                      (persAdminSvcDLTCtx)

static str_t g_msg[512] ; /* 512 bytes should be enough ; any way snprintf is used */

/******************************************************************************************************
********************************   Local functions  ***************************************************
******************************************************************************************************/

/*-----------------------------------------------------------------------------------------------------
Top level
-----------------------------------------------------------------------------------------------------*/
static bool_t pas_conf_checkSource(void) ;

/*-----------------------------------------------------------------------------------------------------
Public-data related
-----------------------------------------------------------------------------------------------------*/
static sint_t pas_conf_configureDataForPublic(void) ;
static bool_t pas_conf_removeDataForPublic(bool_t bPrepareForNewInstall) ;

/*-----------------------------------------------------------------------------------------------------
Group-data related
-----------------------------------------------------------------------------------------------------*/
static sint_t pas_conf_configureDataForGroups(void) ;
static sint_t pas_conf_configureDataForGroup(constpstr_t  groupID, PersAdminCfgInstallRules_e eInstallRule) ;
static bool_t pas_conf_configureGroupMembers(constpstr_t groupID, pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_addMemberToGroup(constpstr_t groupID, constpstr_t appName) ;
static bool_t pas_conf_identifyAndAddNewMembersToGroup(constpstr_t groupID, constpstr_t pBufNewMembersList, sint_t iSizeNewList, constpstr_t pBufOldMembersList, sint_t iSizeOldList) ;
static bool_t pas_conf_removeGroupMembers(constpstr_t  groupID) ;
static bool_t pas_conf_removeMemberFromGroup(constpstr_t  groupID, constpstr_t appID) ;
static bool_t pas_conf_identifyAndRemoveObsoleteGroupMembers(constpstr_t groupID, constpstr_t pBufNewMembersList, sint_t iSizeNewList, constpstr_t pBufOldMembersList, sint_t iSizeOldList) ;
static bool_t pas_conf_removeDataForGroup(constpstr_t  groupID, bool_t bPrepareForNewInstall) ;
static bool_t pas_conf_isGroupIdValid(constpstr_t  groupID);

/*-----------------------------------------------------------------------------------------------------
Application-data related
-----------------------------------------------------------------------------------------------------*/
static sint_t pas_conf_configureDataForApplications(void) ;
static sint_t pas_conf_configureDataForApp(constpstr_t appID, PersAdminCfgInstallRules_e eInstallRule) ;
static bool_t pas_conf_removeDataForApp(constpstr_t appID, bool_t bPrepareForNewInstall) ;

/*-----------------------------------------------------------------------------------------------------
Install folder related
-----------------------------------------------------------------------------------------------------*/
static bool_t pas_conf_getContextInstallFolder(constpstr_t destInstallFolder,  constpstr_t srcInstallFolder, pas_cfg_instFoldCtx_s* psInstallFolderCtx_inout) ;
static bool_t pas_conf_destroyContextInstallFolder(pas_cfg_instFoldCtx_s* psInstallFolderCtx_inout) ;
static sint_t pas_conf_configureDataForInstallFolder(pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_createListsForInstallFolder( pas_cfg_instFoldCtx_s*  psInstallFolderCtx_inout) ;
static bool_t pas_conf_destroyListsForInstallFolder(pas_cfg_instFoldCtx_s*  psInstallFolderCtx_inout) ;
static sint_t pas_conf_processPolicyChangesForInstallFolder(pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_processPolicyChangesForKey(pstr_t resourceID,  pas_cfg_instFoldCtx_s* const   psInstallFolderCtx, pas_conf_keyPolicyChangeCtx_s* const psKeyPolicyChangeCtx); /*DG C8MR2R-MISRA-C:2004 Rule 5.1-SSW_Administrator_0005*/
static sint_t pas_conf_uninstallObsoleteDataForInstallFolder(pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_uninstallObsoleteResource(constpstr_t resourceID, PersistenceConfigurationKey_s* const psConfig, pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_uninstallObsoleteKey(pstr_t resourceID, PersistenceConfigurationKey_s* const psConfig, pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_uninstallObsoleteFile(pstr_t resourceID, PersistenceConfigurationKey_s* const psConfig, pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static sint_t pas_conf_updateRctForInstallFolder(pas_cfg_instFoldCtx_s* const   psInstallFolderCtx) ;
static sint_t pas_conf_configureForDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx, bool_t bForFactoryDefault) ;
static bool_t pas_conf_setupHandlersForDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx, bool_t bForFactoryDefault, pas_conf_handlersForDefault_s* psHandlers_out) ;
static sint_t pas_conf_configureForNonDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx) ;
static bool_t pas_conf_setupHandlersForNonDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx, pas_conf_handlersForNonDefault_s* psHandlers_out) ;
static bool_t pas_conf_getConfigurationNeededForResource(constpstr_t resourceID, PersAdminCfgInstallRules_e  eRule, sint_t hExceptions, pas_cfg_configTypes_e* peConfigType_out);
static bool_t pas_conf_updateDefaultDataForResource(constpstr_t resourceID, pas_cfg_instFoldCtx_s* const psInstallFolderCtx, pas_conf_handlersForDefault_s* psHandlers, bool_t bForFactoryDefault) ;
static bool_t pas_conf_deleteDefaultDataForResource(constpstr_t resourceID, pas_cfg_instFoldCtx_s* const psInstallFolderCtx, pas_conf_handlersForDefault_s* psHandlers, bool_t bForFactoryDefault) ;
static bool_t pas_conf_deleteNonDefaultDataForResource(constpstr_t resourceID, pas_cfg_instFoldCtx_s* const psInstallFolderCtx, pas_conf_handlersForNonDefault_s* psHandlers) ;
static bool_t pas_conf_deleteNonDefaultDataForKeyTypeResource(constpstr_t resourceID, sint_t hNonDefaultDB) ; /*DG C8MR2R-MISRA-C:2004 Rule 5.1-SSW_Administrator_0005*/
static bool_t pas_conf_deleteNonDefaultDataForFileTypeResource(constpstr_t resourceID, constpstr_t installFolderPathname) ; /*DG C8MR2R-MISRA-C:2004 Rule 5.1-SSW_Administrator_0005*/


/*-----------------------------------------------------------------------------------------------------
Helpers
-----------------------------------------------------------------------------------------------------*/
static bool_t pas_conf_unarchiveResourceFile(constpstr_t resourcePathname) ;
static bool_t pas_conf_deleteFolderContent(pstr_t folderPathName, PersadminFilterMode_e eFilterMode) ;
static sint_t pas_conf_filterResourcesList(pstr_t  resourceID, bool_t  bFilterOnlyNonDefault, bool_t  bIsKeyType, pas_conf_listOfItems_s* const psListUnfiltered, pstr_t  pListFiltered_out);


/******************************************************************************************************
********************************   Implementation  ****************************************************
******************************************************************************************************/


/**
 * @brief Allow to extend the configuration for persistency of data from specified level (application, user).
 *        Used during new persistency entry installation
 *        https://workspace1.conti.de/content/00002483/Team%20Documents/01%20Technical%20Documentation/04%20OIP/20_SW_Packages/03_Persistence/Administration_Service/Configure%20and%20initialize%20data%20and%20policy%20for%20new%20application%20-%20questions.doc
 *
 * @param resource_file name of the persistency resource configuration to integrate
 *
 * @return positive value: number of added/modified/removed entries in RCT for all public/groups/apps ; negative value: error code
 */
long persadmin_resource_config_add(char* resource_file)
{
    bool_t  bEverythingOK = true ;
    sint_t  errorCode = PAS_FAILURE ;

    sint_t  iNoOfInstalledResources = 0 ;

    /* check params */
    if(NIL == resource_file)
    {
        bEverythingOK = false ;
        errorCode = PERS_COM_ERR_INVALID_PARAM ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - NULL param", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        /* un-archive the resource file */
        if( ! pas_conf_unarchiveResourceFile(resource_file))
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_unarchiveResourceFile(%s) FAILED", __FUNCTION__, resource_file) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        /* source available in PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME */
        /* check source validity/consistency */
        if( ! pas_conf_checkSource())
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_checkSource FAILED", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        /* source in PERSADM_CFG_SOURCE_ROOT_FOLDER_PATH is fine */
        sint_t iNoOfInstalledResourcesPartial = 0 ;

        iNoOfInstalledResourcesPartial = pas_conf_configureDataForPublic() ;
        if(iNoOfInstalledResourcesPartial >= 0)
        {
            iNoOfInstalledResources += iNoOfInstalledResourcesPartial ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - %d resources installed for public", __FUNCTION__, iNoOfInstalledResourcesPartial) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
        }

        if(bEverythingOK)
        {
            iNoOfInstalledResourcesPartial = pas_conf_configureDataForGroups() ;
            if(iNoOfInstalledResourcesPartial >= 0)
            {
                iNoOfInstalledResources += iNoOfInstalledResourcesPartial ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - %d resources installed for groups", __FUNCTION__, iNoOfInstalledResourcesPartial) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
                bEverythingOK = false ;
            }
        }

        if(bEverythingOK)
        {
            iNoOfInstalledResourcesPartial = pas_conf_configureDataForApplications() ;
            if(iNoOfInstalledResourcesPartial >= 0)
            {
                iNoOfInstalledResources += iNoOfInstalledResourcesPartial ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - %d resources installed for applications", __FUNCTION__, iNoOfInstalledResourcesPartial) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
                bEverythingOK = false ;
            }
        }
    }

    /* remove the folder with un-compressed input */
    if(0 <= persadmin_check_if_file_exists(PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME, true))
    {
        sint_t iBytesDeleted = persadmin_delete_folder(PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME) ;
        if(iBytesDeleted < 0)
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_delete_folder(%s) returned <%d>",
            __FUNCTION__, PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME, iBytesDeleted) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    (void)snprintf(g_msg, sizeof(g_msg), "%s - result=%d bEverythingOK=%d", __FUNCTION__, iNoOfInstalledResources, bEverythingOK) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    
    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0008*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0009*/


/**
 * \brief Perform consistency check of the input (uncompressed in the default folder)
 * \param none
 * \return true for success, false for error
 */
static bool_t pas_conf_checkSource(void)
{
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING("pas_conf_checkSource: NOT IMPLEMENTED !!!")); 
    return true ;
}


/**
 * \brief Apply configuration for shared public data
 * \param none
 * \return the number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureDataForPublic(void)
{
    bool_t                  bEverythingOK = true ;
    sint_t                  errorCode = PAS_FAILURE ;
    sint_t                  iNoOfInstalledResources = 0 ;    
    pas_cfg_instFoldCtx_s   sContext = {0} ;
    bool_t                  bNothingToDo =   false ; /* set to true in case of PersAdminCfgInstallRules_DontTouch */
    pstr_t                  pInstallRulesPathname = PERSADM_CFG_SOURCE_PUBLIC_FOLDER_PATH"/"PERSADM_CFG_RESOURCE_INSTALL_RULES_FILENAME ;

    sint_t hRules = -1 ;

    /* retrigger watchdog */
    persadmin_RetriggerWatchdog();

    /* initialize */
    (void)memset(&sContext, 0x0, sizeof(sContext));

    (void)snprintf(g_msg, sizeof(g_msg), "%s - started...", __FUNCTION__) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    /* get handler for install rules file */
    hRules = persAdmCfgFileOpen(pInstallRulesPathname, PersAdminCfgFileType_InstallRules) ;

    if(hRules < 0)
    {        
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgFileOpen(%s) Failed err=%d", __FUNCTION__, pInstallRulesPathname, hRules) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    
    if(bEverythingOK)
    {
        /* there should be at most one install rule for "public" */

        sint_t errorLocal = persAdmCfgRulesGetRuleForFolder(hRules, PERS_ORG_PUBLIC_FOLDER_NAME, &sContext.eInstallRule) ;
        if(errorLocal >= 0)
        {
            /* install rule for "public" available */
            (void)snprintf(g_msg, sizeof(g_msg), "%s - install rule for public = <%d>", __FUNCTION__, sContext.eInstallRule) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            if(PersAdminCfgInstallRules_DontTouch == sContext.eInstallRule)
            {
                bNothingToDo = true ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - No configuration needed for public data", __FUNCTION__) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            /* install rule for "public" not available => skip configuration for "public" */
            bNothingToDo = true ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRulesGetRuleForFolder(%s) Failed err=%d => skip it", __FUNCTION__, pInstallRulesPathname, errorLocal) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK && (! bNothingToDo))
    {
        if(     (PersAdminCfgInstallRules_Uninstall == sContext.eInstallRule)
            ||  (PersAdminCfgInstallRules_NewInstall == sContext.eInstallRule))
        {
            /* in case of newInstall/uninstall, the folder must be cleaned/deleted */
            bool_t bPrepareForNewInstall = (PersAdminCfgInstallRules_NewInstall == sContext.eInstallRule) ;
            if( ! pas_conf_removeDataForPublic(bPrepareForNewInstall))
            {
                bEverythingOK = false ;
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_removeDataForPublic(%d)...%s", 
                __FUNCTION__, bPrepareForNewInstall, bEverythingOK ? "OK" : "FAILED") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK && (! bNothingToDo))
    {
        /* let's be sure that "public" folder exists (if no one created it before, or it was deleted) */
        if(0 > persadmin_check_if_file_exists(PERS_ORG_SHARED_PUBLIC_WT_PATH, true))
        {
            if(0 > persadmin_create_folder(PERS_ORG_SHARED_PUBLIC_WT_PATH))
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK && (! bNothingToDo))
    {
        /* get the constext for install folder */
        if(pas_conf_getContextInstallFolder(PERS_ORG_SHARED_PUBLIC_WT_PATH, PERSADM_CFG_SOURCE_PUBLIC_FOLDER_PATH"/"PERS_ORG_PUBLIC_FOLDER_NAME, &sContext))
        {
            iNoOfInstalledResources = pas_conf_configureDataForInstallFolder(&sContext) ;
            if(iNoOfInstalledResources >= 0)
            {
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForInstallFolder returned %d", __FUNCTION__, iNoOfInstalledResources) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
                bEverythingOK = false ;
                errorCode = iNoOfInstalledResources ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForInstallFolder FAILED err=%d", __FUNCTION__, errorCode) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_getContextInstallFolder FAILED", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }

        if( ! pas_conf_destroyContextInstallFolder(&sContext))
        {
            bEverythingOK = false ;
        }
    }


    if(hRules >= 0)
    {
        (void)persAdmCfgFileClose(hRules) ;
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0010*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0011*/

/**
 * \brief Remove data for "public"
 * \param bPrepareForNewInstall [in] if true, the install folder is cleaned, other way it is removed
 * \return true for success, false other way
 */
static bool_t pas_conf_removeDataForPublic(bool_t bPrepareForNewInstall)
{
    bool_t  bEverythingOK = true ;
    pstr_t  pFolderPathName = PERS_ORG_SHARED_PUBLIC_WT_PATH ;

    if(0 <= persadmin_check_if_file_exists(pFolderPathName, true))
    {
        if(bPrepareForNewInstall)
        {
            /* delete the content of "public" folder, but keep the folder */
            if( ! pas_conf_deleteFolderContent(pFolderPathName, PersadminFilterAll))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            /* delete "public" folder */
            if(0 > persadmin_delete_folder(pFolderPathName))
            {
                bEverythingOK = false ;
            }
        }
    }

    return bEverythingOK ;
}


/*****************************************************************************************************************
******************************  Group data related ***************************************************************
*****************************************************************************************************************/

/**
 * \brief Apply configuration for shared group data
 * \param none
 * \return the number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureDataForGroups(void)
{
    bool_t                      bEverythingOK = true ;
    sint_t                      errorCode = PAS_FAILURE ;
    sint_t                      iNoOfInstalledResources = 0 ;    
    PersAdminCfgInstallRules_e  eRule = PersAdminCfgInstallRules_DontTouch ;
    pstr_t                      bufferInstallRules = NIL ;
    sint_t                      iSizeBufInstallRules = 0 ;
    pstr_t                      pInstallRulesPathname = PERSADM_CFG_SOURCE_GROUPS_FOLDER_PATH"/"PERSADM_CFG_RESOURCE_INSTALL_RULES_FILENAME ;
    sint_t                      hRules = -1 ;

    (void)snprintf(g_msg, sizeof(g_msg), "%s - started...", __FUNCTION__) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    /* get handler to install rules file */
    hRules = persAdmCfgFileOpen(pInstallRulesPathname, PersAdminCfgFileType_InstallRules) ;

    if(hRules < 0)
    {        
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgFileOpen(%s) Failed err=%d", __FUNCTION__, pInstallRulesPathname, hRules) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        /* get the list of groups for which install rules are defined */
        iSizeBufInstallRules = persAdmCfgRulesGetSizeFoldersList(hRules) ;
        if(iSizeBufInstallRules > 0)
        {
            bufferInstallRules = (pstr_t)malloc((size_t)iSizeBufInstallRules) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL != bufferInstallRules)
            {
                sint_t iErrLocal = persAdmCfgRulesGetFoldersList(hRules, bufferInstallRules, iSizeBufInstallRules) ;
                if(iErrLocal != iSizeBufInstallRules)
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRulesGetFoldersList returned %d (expected %d)", 
                        __FUNCTION__, iErrLocal, iSizeBufInstallRules) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            if(iSizeBufInstallRules < 0)
            {
                bEverythingOK = false ;
            }
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRulesGetSizeFoldersList returned %d", __FUNCTION__, iSizeBufInstallRules) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        /* for each group in the list, apply configuration */
        pstr_t currentGroupID = NIL ;
        sint_t iPosInBuffer = 0 ;

        while(bEverythingOK && (iPosInBuffer < iSizeBufInstallRules))
        {
            sint_t errorLocal ;
            currentGroupID = bufferInstallRules + iPosInBuffer ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

            if( ! pas_conf_isGroupIdValid(currentGroupID))
            {
                bEverythingOK = false ;
                break ;
            }

            (void)snprintf(g_msg, sizeof(g_msg), "%s - Start configuration for group <<%s>>", __FUNCTION__, currentGroupID) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

            errorLocal = persAdmCfgRulesGetRuleForFolder(hRules, currentGroupID, &eRule) ;
            if(errorLocal >= 0)
            {            
                if(PersAdminCfgInstallRules_DontTouch != eRule)
                {                
                    sint_t iNoOfResourcesInstalledForGroup = pas_conf_configureDataForGroup(currentGroupID, eRule) ;
                    if(iNoOfResourcesInstalledForGroup >= 0)
                    {
                        iNoOfInstalledResources += iNoOfResourcesInstalledForGroup ;
                    }
                    else
                    {
                        bEverythingOK = false ;
                    }
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForGroup(%s) returned %d",
                            __FUNCTION__, currentGroupID, iNoOfResourcesInstalledForGroup) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
                else
                {
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - No configuration needed for group <<%s>>", __FUNCTION__, currentGroupID) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }

            iPosInBuffer += ((sint_t)strlen(currentGroupID) + 1) ; /* 1 <=> '\0'*/
        }        
    }
    
    if(bufferInstallRules != NIL)
    {
        free(bufferInstallRules) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    if(hRules >= 0)
    {
        (void)persAdmCfgFileClose(hRules) ;
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0012*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0013*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0014*/

/**
 * \brief Apply configuration for one group
 * \param groupID       [in] group's name
 * \param eInstallRule  [in] install rule for the group
 * \return the number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureDataForGroup(   constpstr_t                 groupID,
                                                PersAdminCfgInstallRules_e  eInstallRule)
{
    bool_t  bEverythingOK = true ;
    sint_t  errorCode = PAS_FAILURE ;
    sint_t  iNoOfInstalledResources = 0 ;    
    pas_cfg_instFoldCtx_s sContext = {0};

    str_t destInstallFolder[PERSADMIN_MAX_PATH_LENGHT] ;
    str_t srcInstallFolder[PERSADMIN_MAX_PATH_LENGHT] ;

    /* retrigger watchdog */
    persadmin_RetriggerWatchdog();
        
    if(     (PersAdminCfgInstallRules_Uninstall == eInstallRule)
        ||  (PersAdminCfgInstallRules_NewInstall == eInstallRule))
    {
        /* clean/delete the group's folder */
        bool_t bPrepareForNewInstall = (PersAdminCfgInstallRules_NewInstall == eInstallRule) ;
        if( ! pas_conf_removeDataForGroup(groupID, bPrepareForNewInstall))
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_removeDataForGroup(%s, %d)...%s", 
            __FUNCTION__, groupID, bPrepareForNewInstall, bEverythingOK ? "OK" : "FAILED") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        sContext.eInstallRule = eInstallRule ;

        /* set the paths */
        (void)snprintf(destInstallFolder, sizeof(destInstallFolder), "%s/%s",
                PERS_ORG_SHARED_GROUP_WT_PATH, groupID) ;
        (void)snprintf(srcInstallFolder, sizeof(destInstallFolder), "%s/%s",
                PERSADM_CFG_SOURCE_GROUPS_FOLDER_PATH, groupID) ;

        /* create the group's install folder, if not already created */
        if(0 > persadmin_check_if_file_exists(destInstallFolder, true))
        {
            if(0 > persadmin_create_folder(destInstallFolder))
            {
                bEverythingOK = false ;
            }
        }

        if(bEverythingOK)
        {
            if( ! pas_conf_getContextInstallFolder(destInstallFolder, srcInstallFolder, &sContext))
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_getContextInstallFolder FAILED", __FUNCTION__) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        /* make configurations (e.g. create/delete sym-links) for the group's members */
        if( ! pas_conf_configureGroupMembers(groupID, &sContext))
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureGroupMembers ... %s", 
                __FUNCTION__, bEverythingOK ? "OK" : "FAILED") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        /* apply configuration for the group's folder */
        iNoOfInstalledResources = pas_conf_configureDataForInstallFolder(&sContext) ;
        if(iNoOfInstalledResources >= 0)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForInstallFolder returned %d", __FUNCTION__, iNoOfInstalledResources) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
            errorCode = iNoOfInstalledResources ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForInstallFolder FAILED err=%d", __FUNCTION__, errorCode) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if( ! pas_conf_destroyContextInstallFolder(&sContext))
    {
        bEverythingOK = false ;
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0015*/

/**
 * \brief Make configurations (e.g. create/delete sym-links) for the group's members
 * \param groupID             [in] group's name
 * \param psInstallFolderCtx  [in] context for the group
 * \return true for success, false other way
 */
static bool_t pas_conf_configureGroupMembers(constpstr_t groupID, pas_cfg_instFoldCtx_s* const psInstallFolderCtx)
{
    bool_t  bEverythingOK = true ;
    pstr_t  pBufSrcMembersList = NIL ;
    sint_t  iSizeSrcMembersList = 0 ;
    pstr_t  pBufDestMembersList = NIL ;
    sint_t  iSizeDestMembersList = 0 ;
    str_t   destGroupMembersFilePathname[PERSADMIN_MAX_PATH_LENGHT] ;
    str_t   srcGroupMembersFilePathname[PERSADMIN_MAX_PATH_LENGHT] ;
    bool_t  bSrcGroupContentFileExists = false ;

    /* set paths */
    (void)snprintf(destGroupMembersFilePathname, sizeof(destGroupMembersFilePathname), "%s/%s",
            psInstallFolderCtx->sDestContext.sInstallFolder.pathname, 
            PERSADM_CFG_RESOURCE_GROUP_CONTENT_FILENAME) ;
    (void)snprintf(srcGroupMembersFilePathname, sizeof(srcGroupMembersFilePathname), "%s/%s",
            psInstallFolderCtx->sSrcContext.sInstallFolder.pathname, 
            PERSADM_CFG_RESOURCE_GROUP_CONTENT_FILENAME) ;


    /*  Obtain the lists with the old and new group's members.
    *   Compare the new members list with the old ones.
    *   - for each new member => create sym link to group's folder
    *   - for each old member no longer in the list => delete sym link to group's folder */
    if(0 <= persadmin_check_if_file_exists(srcGroupMembersFilePathname, false))
    {
        sint_t hSrcGroupContentFile ;
        
        bSrcGroupContentFileExists = true ;

        hSrcGroupContentFile = persAdmCfgFileOpen(srcGroupMembersFilePathname, PersAdminCfgFileType_GroupContent) ;

        if(hSrcGroupContentFile >= 0)
        {            
            iSizeSrcMembersList = persAdmCfgGroupContentGetSizeMembersList(hSrcGroupContentFile) ;
            if(iSizeSrcMembersList > 0)
            {
                /* don't forget to free it */
                pBufSrcMembersList = (pstr_t)malloc((size_t)iSizeSrcMembersList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
                if(NIL != pBufSrcMembersList)
                {
                    if(iSizeSrcMembersList != persAdmCfgGroupContentGetMembersList(hSrcGroupContentFile, pBufSrcMembersList, iSizeSrcMembersList))
                    {
                        bEverythingOK = false ;
                    }
                }
                else
                {
                    bEverythingOK = false ;
                }
            }

            (void)persAdmCfgFileClose(hSrcGroupContentFile) ;
        }
    }

    if(bEverythingOK)
    {
        sint_t hDestGroupContentFile = persAdmCfgFileOpen(destGroupMembersFilePathname, PersAdminCfgFileType_GroupContent) ;
        if(hDestGroupContentFile >= 0)
        {            
            iSizeDestMembersList = persAdmCfgGroupContentGetSizeMembersList(hDestGroupContentFile) ;
            if(iSizeDestMembersList > 0)
            {
                pBufDestMembersList = (pstr_t)malloc((size_t)iSizeDestMembersList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
                if(NIL != pBufDestMembersList)
                {
                    if(iSizeSrcMembersList != persAdmCfgGroupContentGetMembersList(hDestGroupContentFile, pBufDestMembersList, iSizeDestMembersList))
                    {
                        bEverythingOK = false ;
                    }
                }
                else
                {
                    bEverythingOK = false ;
                }
            }

            (void)persAdmCfgFileClose(hDestGroupContentFile) ;
        }
    }

    if(bEverythingOK)
    {
        /* We have the lists with old and new group members
         * Identify the new members of the group and create the sym links */
        bEverythingOK = pas_conf_identifyAndAddNewMembersToGroup(groupID, pBufSrcMembersList, iSizeSrcMembersList, pBufDestMembersList, iSizeDestMembersList) ;
    }

    if(bEverythingOK)
    {
        /* We have the lists with old and new group members
         * Identify the obsolete members of the group and delete the sym links */
        bEverythingOK = pas_conf_identifyAndRemoveObsoleteGroupMembers(groupID, pBufSrcMembersList, iSizeSrcMembersList, pBufDestMembersList, iSizeDestMembersList) ;
    }

    if(bEverythingOK && bSrcGroupContentFileExists)
    {
        /* keep a copy of the source groupContent file in the group's install folder 
         * In order to be able to get the group's members list easily (without scanning all the applications' folders)*/
        sint_t iBytesCopied = persadmin_copy_file(srcGroupMembersFilePathname, destGroupMembersFilePathname) ;
        if(iBytesCopied < 0)
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_copy_file(<%s>, <%s>) returned <%d>", 
            __FUNCTION__, srcGroupMembersFilePathname, destGroupMembersFilePathname, iBytesCopied) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(NIL != pBufSrcMembersList)
    {
        free(pBufSrcMembersList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }
    if(NIL != pBufDestMembersList)
    {
        free(pBufDestMembersList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0016*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0035*/

/**
 * \brief Make configurations (e.g. create sym-link) for the application that becomes member of the group
 * \param groupID             [in] group's name
 * \param appName             [in] application's name
 * \return true for success, false other way
 */
static bool_t pas_conf_addMemberToGroup(constpstr_t groupID, constpstr_t appName)
{
    bool_t  bEverythingOK = true ;
    str_t   pathLink[PERSADMIN_MAX_PATH_LENGHT] ;
    str_t   pathTarget[PERSADMIN_MAX_PATH_LENGHT] ;

    (void)snprintf(pathTarget, sizeof(pathTarget), "%s%s", PERS_ORG_SHARED_GROUP_WT_PATH_, groupID) ;
    (void)snprintf(pathLink, sizeof(pathLink), "%s/%s/%s%s", 
        PERS_ORG_LOCAL_APP_WT_PATH, appName, PERS_ORG_SHARED_GROUP_SYMLINK_PREFIX, groupID);
    if(0 > persadmin_create_symbolic_link(pathLink, pathTarget))
    {
        bEverythingOK = false ;
    }
    (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_create_symbolic_link(<%s>, <%s>) %s", 
            __FUNCTION__, pathLink, pathTarget, bEverythingOK ? "OK" : "FAILED") ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    return bEverythingOK ;
}

/**
 * \brief Identifies the new members of the group and applies configurations (e.g. create sym-link)
 * \param groupID             [in] group's name
 * \param pBufNewMembersList  [in] buffer with '\0' separated new group's members
 * \param iSizeNewList        [in] sizeof pBufNewMembersList
 * \param pBufOldMembersList  [in] buffer with '\0' separated old group's members
 * \param iSizeOldList        [in] sizeof iSizeOldList
 * \return true for success, false other way
 */
static bool_t pas_conf_identifyAndAddNewMembersToGroup(constpstr_t groupID, constpstr_t pBufNewMembersList, sint_t iSizeNewList, constpstr_t pBufOldMembersList, sint_t iSizeOldList)
{
    bool_t bEverythingOK = true ;
    sint_t iPosInSrcList = 0 ;
    sint_t iPosInDestList = 0 ;
    pstr_t oldAppName = NIL ;
    pstr_t newAppName = NIL ;
    bool_t bIsNewMember = false ;

    while(bEverythingOK && (iPosInSrcList < iSizeNewList))
    {
        bIsNewMember = true ;
        newAppName = pBufNewMembersList + iPosInSrcList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
        iPosInDestList = 0 ;
        
        while(iPosInDestList < iSizeOldList)
        {
            oldAppName = pBufOldMembersList + iPosInDestList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            iPosInDestList += ((sint_t)strlen(oldAppName) + 1) ; /* 1 <=> '\0' */
            if(0 == strcmp(newAppName, oldAppName))
            {
                bIsNewMember = false ;
                break ;
            }
        }

        if(bIsNewMember)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - New member <<%s>> for group <<%s>>", __FUNCTION__, newAppName, groupID) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

            bEverythingOK = pas_conf_addMemberToGroup(groupID, newAppName) ;
        }
        iPosInSrcList += ((sint_t)strlen(newAppName) + 1) ; /* 1 <=> '\0' */
    }

    return bEverythingOK ;
}


/**
 * \brief Make configurations (e.g. delete sym-link) for all the applications that were members of the removed group
 * \param groupID             [in] group's name
 * \return true for success, false other way
 */
static bool_t pas_conf_removeGroupMembers(constpstr_t groupID)
{
    bool_t bEverythingOK = true ;
    pstr_t pMembersList = NIL ;
    sint_t iMemberListSize = 0 ;
    str_t  groupContentPathname[PERSADMIN_MAX_PATH_LENGHT] ;
    sint_t hGroupContent = -1 ;

    /* set path */
    (void)snprintf(groupContentPathname, sizeof(groupContentPathname), "%s/%s/%s",
        PERSADM_CFG_SOURCE_GROUPS_FOLDER_PATH,
        groupID,
        PERSADM_CFG_RESOURCE_GROUP_CONTENT_FILENAME) ;

    /* get the list of existent group's members - first get size of the needed buffer */
    if(persadmin_check_if_file_exists(groupContentPathname, false))
    {
        hGroupContent = persAdmCfgFileOpen(groupContentPathname, PersAdminCfgFileType_GroupContent) ;
        if(hGroupContent >= 0)
        {
            iMemberListSize = persAdmCfgGroupContentGetSizeMembersList(hGroupContent) ;
            if(iMemberListSize < 0)
            {
                bEverythingOK = false ;
            }
        }
    }
    else
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - <%s> does not exist", __FUNCTION__, groupContentPathname) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK && (iMemberListSize > 0))
    {
        /* get the list of existent group's members */
        /* don't forget to free it */
        pMembersList = (pstr_t)malloc((size_t)iMemberListSize) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        if(NIL != pMembersList)
        {
            if(iMemberListSize != persAdmCfgGroupContentGetMembersList(hGroupContent, pMembersList, iMemberListSize))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
        }            
    }

    if(bEverythingOK && (iMemberListSize > 0))
    {
        /* for each group's member in the list, remove the connections to group */
        sint_t iPosInList = 0 ;
        pstr_t pCurrentMember = NIL ;

        while(bEverythingOK && (iPosInList < iMemberListSize))
        {
            pCurrentMember = pMembersList + iPosInList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

            if( ! pas_conf_removeMemberFromGroup(groupID, pCurrentMember))
            {
                bEverythingOK = false ;
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_removeMemberFromGroup(<%s>, <%s>)...%s", 
                __FUNCTION__, groupID, pCurrentMember, bEverythingOK ? "OK" : "FAILED") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

            iPosInList += ((sint_t)strlen(pCurrentMember) + 1) ; /* 1 <=> '\0' */
        }
    }

    if(hGroupContent >= 0)
    {
        (void)persAdmCfgFileClose(hGroupContent) ;
    }

    if(NIL != pMembersList)
    {
        free(pMembersList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief Make configurations (e.g. delete sym-link) for the application that is no longer member of the group
 * \param groupID             [in] group's name
 * \param appID               [in] application's name
 * \return true for success, false other way
 */
static bool_t pas_conf_removeMemberFromGroup(constpstr_t groupID, constpstr_t appID)
{
    bool_t  bEverythingOK = true ;
    str_t   pathLink[PERSADMIN_MAX_PATH_LENGHT] ;

    /* set path */
    (void)snprintf(pathLink, sizeof(pathLink), "%s/%s/%s%s", 
        PERS_ORG_LOCAL_APP_WT_PATH, appID, PERS_ORG_SHARED_GROUP_SYMLINK_PREFIX, groupID);
    if(persadmin_check_if_file_exists(pathLink, false))
    {
        if(! persadmin_delete_file(pathLink))
        {
            bEverythingOK = false ;
        }
    }

    (void)snprintf(g_msg, sizeof(g_msg), "%s - delete <%s> ... %s", 
            __FUNCTION__, pathLink, bEverythingOK ? "OK" : "FAILED") ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));


    return bEverythingOK ;
}


/**
 * \brief Identifies the obsolete members of the group and applies configurations (e.g. delete sym-link)
 * \param groupID             [in] group's name
 * \param pBufNewMembersList  [in] buffer with '\0' separated new group's members
 * \param iSizeNewList        [in] sizeof pBufNewMembersList
 * \param pBufOldMembersList  [in] buffer with '\0' separated old group's members
 * \param iSizeOldList        [in] sizeof iSizeOldList
 * \return true for success, false other way
 */
static bool_t pas_conf_identifyAndRemoveObsoleteGroupMembers(constpstr_t groupID, constpstr_t pBufNewMembersList, sint_t iSizeNewList, constpstr_t pBufOldMembersList, sint_t iSizeOldList)
{
    bool_t  bEverythingOK = true ;
    sint_t iPosInSrcList = 0 ;
    sint_t iPosInDestList = 0 ;
    pstr_t oldAppName = NIL ;
    pstr_t newAppName = NIL ;
    bool_t bIsObsoleteMember = false ;

    while(bEverythingOK && (iPosInDestList < iSizeOldList))
    {
        bIsObsoleteMember = true ;
        oldAppName = pBufOldMembersList + iPosInDestList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
        iPosInSrcList = 0 ;
        
        while(iPosInSrcList < iSizeNewList)
        {
            newAppName = pBufNewMembersList + iPosInSrcList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            iPosInSrcList += ((sint_t)strlen(newAppName) + 1) ; /* 1 <=> '\0' */
            if(0 == strcmp(newAppName, oldAppName))
            {
                bIsObsoleteMember = false ;
                break ;
            }
        }

        if(bIsObsoleteMember)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - Obsolete member <<%s>> for group <<%s>>", __FUNCTION__, oldAppName, groupID) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

            bEverythingOK = pas_conf_removeMemberFromGroup(groupID, oldAppName) ;
        }
        iPosInDestList += ((sint_t)strlen(oldAppName) + 1) ; /* 1 <=> '\0' */
    }

    return bEverythingOK ;
}

/**
 * \brief Remove data for groupID
 * \param groupID               [in] group's name
 * \param bPrepareForNewInstall [in] if true, the group's folder is cleaned, other way it is removed
 * \return true for success, false other way
 */
static bool_t pas_conf_removeDataForGroup(constpstr_t groupID, bool_t bPrepareForNewInstall)
{
    bool_t  bEverythingOK = true ;
    str_t   folderPathName[PERSADMIN_MAX_PATH_LENGHT] ;

    /* set path */
    (void)snprintf(folderPathName, sizeof(folderPathName), "%s%s", PERS_ORG_SHARED_GROUP_WT_PATH_, groupID) ;

    if(0 <= persadmin_check_if_file_exists(folderPathName, true))
    {
        if(bPrepareForNewInstall)
        {
            /* delete the content of group folder, but keep the folder */
            if( ! pas_conf_deleteFolderContent(folderPathName, PersadminFilterAll))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            /* delete "group" folder */
            if(0 > persadmin_delete_folder(folderPathName))
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK)
    {
        if( ! pas_conf_removeGroupMembers(groupID))
        {
            bEverythingOK = false ;
        }
    }

    return bEverythingOK ;
}

/**
 * \brief Check the validity of the group's name
 * \param groupID               [in] group's name
 * \return true is group's name is valid, false other way
 */
static bool_t pas_conf_isGroupIdValid(constpstr_t  groupID)
{
    bool_t bEverythingOK = true ;
    sint_t iGroupID = 0 ;

    /* check the validity of the group name (2 digits hex value < 0x80) */
    if(2 == strlen(groupID))
    {
        if(1 == sscanf(groupID, "%X", &iGroupID))
        {
            if((iGroupID < 0) || (iGroupID >= 0x80))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }
    else
    {
        bEverythingOK = false ;
    }

    if(! bEverythingOK)
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - invalid group ID : <<%s>>", __FUNCTION__, groupID) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    return bEverythingOK ;
}

/*****************************************************************************************************************
******************************  Application data related *********************************************************
*****************************************************************************************************************/

/**
 * \brief Apply configuration for applications's data
 * \param none
 * \return the number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureDataForApplications(void)
{
    bool_t                      bEverythingOK = true ;
    sint_t                      errorCode = PAS_FAILURE ;
    sint_t                      iNoOfInstalledResources = 0 ;    
    PersAdminCfgInstallRules_e  eRule ;
    pstr_t                      bufferInstallRules = NIL ;
    sint_t                      iSizeBufInstallRules = 0 ;
    pstr_t                      pInstallRulesPathname = PERSADM_CFG_SOURCE_APPS_FOLDER_PATH"/"PERSADM_CFG_RESOURCE_INSTALL_RULES_FILENAME ;
    sint_t                      hRules = -1 ;


    (void)snprintf(g_msg, sizeof(g_msg), "%s - started...", __FUNCTION__) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    hRules = persAdmCfgFileOpen(pInstallRulesPathname, PersAdminCfgFileType_InstallRules) ;

    if(hRules < 0)
    {        
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgFileOpen(%s) Failed err=%d", __FUNCTION__, pInstallRulesPathname, hRules) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        /* get the list of install rules for apps */
        iSizeBufInstallRules = persAdmCfgRulesGetSizeFoldersList(hRules) ;
        if(iSizeBufInstallRules > 0)
        {
            bufferInstallRules = (pstr_t)malloc((size_t)iSizeBufInstallRules) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL != bufferInstallRules)
            {
                sint_t iErrLocal = persAdmCfgRulesGetFoldersList(hRules, bufferInstallRules, iSizeBufInstallRules) ;
                if(iErrLocal != iSizeBufInstallRules)
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRulesGetFoldersList returned %d (expected %d)", 
                        __FUNCTION__, iErrLocal, iSizeBufInstallRules) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            if(iSizeBufInstallRules < 0)
            {
                bEverythingOK = false ;
            }
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRulesGetSizeFoldersList returned %d", __FUNCTION__, iSizeBufInstallRules) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        /* for each application, apply configuration */
        pstr_t currentApp = NIL ;
        sint_t iPosInBuffer = 0 ;

        while(bEverythingOK && (iPosInBuffer < iSizeBufInstallRules))
        {
            sint_t errorLocal ;
            currentApp = bufferInstallRules + iPosInBuffer ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

            (void)snprintf(g_msg, sizeof(g_msg), "%s - Start configuration for app <<%s>>", __FUNCTION__, currentApp) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

            errorLocal = persAdmCfgRulesGetRuleForFolder(hRules, currentApp, &eRule) ;
            if(errorLocal >= 0)
            {
                if(PersAdminCfgInstallRules_DontTouch != eRule)
                {
                    sint_t iNoOfResourcesInstalledForApp = pas_conf_configureDataForApp(currentApp, eRule) ;
                    if(iNoOfResourcesInstalledForApp >= 0)
                    {
                        iNoOfInstalledResources += iNoOfResourcesInstalledForApp ;
                    }
                    else
                    {
                        bEverythingOK = false ;
                    }
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForApp(%s) returned %d",
                            __FUNCTION__, currentApp, iNoOfResourcesInstalledForApp) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
                else
                {
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - No configuration needed for app <<%s>>", __FUNCTION__, currentApp) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }

            iPosInBuffer += ((sint_t)strlen(currentApp) + 1) ; /* 1 <=> '\0'*/
        }
        
    }
    
    if(NIL != bufferInstallRules)
    {
        free(bufferInstallRules) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    if(hRules >= 0)
    {
        (void)persAdmCfgFileClose(hRules) ;
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0017*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0018*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0019*/


/**
 * \brief Apply configuration for one application
 * \param appID         [in] application's name
 * \param eInstallRule  [in] install rule for the application
 * \return the number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureDataForApp(constpstr_t appID, PersAdminCfgInstallRules_e eInstallRule)
{
    bool_t                  bEverythingOK = true ;
    sint_t                  errorCode = PAS_FAILURE ;
    sint_t                  iNoOfInstalledResources = 0 ;    
    pas_cfg_instFoldCtx_s   sContext = {0} ;
    str_t                   destInstallFolder[PERSADMIN_MAX_PATH_LENGHT] ;
    str_t                   srcInstallFolder[PERSADMIN_MAX_PATH_LENGHT] ;

    /* retrigger watchdog */
    persadmin_RetriggerWatchdog();

    if(     (PersAdminCfgInstallRules_Uninstall == eInstallRule)
        ||  (PersAdminCfgInstallRules_NewInstall == eInstallRule))
    {
        /* clear/remove the application's folder */
        bool_t bPrepareForNewInstall = (PersAdminCfgInstallRules_NewInstall == eInstallRule) ;
        if( ! pas_conf_removeDataForApp(appID, bPrepareForNewInstall))
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_removeDataForApp(%s, %d)...%s", 
            __FUNCTION__, appID, bPrepareForNewInstall, bEverythingOK ? "OK" : "FAILED") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        sContext.eInstallRule = eInstallRule ;

        /* set paths */
        (void)snprintf(destInstallFolder, sizeof(destInstallFolder), "%s/%s",
                PERS_ORG_LOCAL_APP_WT_PATH, appID) ;
        (void)snprintf(srcInstallFolder, sizeof(destInstallFolder), "%s/%s",
                PERSADM_CFG_SOURCE_APPS_FOLDER_PATH, appID) ;
            
        if(0 > persadmin_check_if_file_exists(destInstallFolder, true))
        {
            if(0 > persadmin_create_folder(destInstallFolder))
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        if( ! pas_conf_getContextInstallFolder(destInstallFolder, srcInstallFolder, &sContext))
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_getContextInstallFolder FAILED", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        /* create symlink to public folder - if not already available */
        str_t pathLink[PERSADMIN_MAX_PATH_LENGHT] ;

        (void)snprintf(pathLink, sizeof(pathLink), "%s/%s", 
            destInstallFolder, PERS_ORG_SHARED_PUBLIC_SYMLINK_NAME);
        if(0 > persadmin_check_if_file_exists(pathLink, false))
        {
            /* symlink does not exist */
            if(0 > persadmin_create_symbolic_link(pathLink, PERS_ORG_SHARED_PUBLIC_WT_PATH))
            {
                bEverythingOK = false ;
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_create_symbolic_link(<%s>, <%s>) %s", 
                    __FUNCTION__, pathLink, PERS_ORG_SHARED_PUBLIC_WT_PATH, bEverythingOK ? "OK" : "FAILED") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK && (PersAdminCfgInstallRules_Uninstall != eInstallRule))
    {
        iNoOfInstalledResources = pas_conf_configureDataForInstallFolder(&sContext) ;
        if(iNoOfInstalledResources >= 0)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForInstallFolder returned %d", __FUNCTION__, iNoOfInstalledResources) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
            errorCode = iNoOfInstalledResources ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureDataForInstallFolder FAILED err=%d", __FUNCTION__, errorCode) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if( ! pas_conf_destroyContextInstallFolder(&sContext))
    {
        bEverythingOK = false ;
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0020*/

/**
 * \brief Remove data for one application
 * \param appID                 [in] application's name
 * \param bPrepareForNewInstall [in] if true, the application's folder is cleaned, other way it is removed
 * \return true for success, false other way
 */
static bool_t pas_conf_removeDataForApp(constpstr_t appID, bool_t bPrepareForNewInstall)
{
    bool_t  bEverythingOK = true ;
    str_t   appFolderPathname[PERSADMIN_MAX_PATH_LENGHT] ;

    /* set path */
    (void)snprintf(appFolderPathname, sizeof(appFolderPathname), "%s%s", PERS_ORG_LOCAL_APP_WT_PATH_, appID) ;

    if(0 <= persadmin_check_if_file_exists(appFolderPathname, true))
    {
        if(bPrepareForNewInstall)
        {
            /* delete the content of the folder(don't delete sym-links), but keep the folder */
            if( ! pas_conf_deleteFolderContent(appFolderPathname, PersadminFilterFoldersAndRegularFiles))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            /* delete "public" folder */
            if(0 > persadmin_delete_folder(appFolderPathname))
            {
                bEverythingOK = false ;
            }
        }
    }

    return bEverythingOK ;
}


/*****************************************************************************************************************
******************************  Folder (public, group, app) data related *****************************************
*****************************************************************************************************************/

/**
 * \brief Get the context for the public/group/app's install folder
 * \param destInstallFolder         [in]    pathname for the destination(install) folder
 * \param srcInstallFolder          [in]    pathname for the source(configuration input) folder
 * \param psInstallFolderCtx_inout  [inout] pointer to the public/group/app's context
 * \return true for success, false other way
 */
static bool_t pas_conf_getContextInstallFolder(constpstr_t destInstallFolder, constpstr_t srcInstallFolder, pas_cfg_instFoldCtx_s* psInstallFolderCtx_inout)
{
    bool_t  bEverythingOK = true ;
    size_t  iIndex = 0 ;

    /*---------------------------------------------------------------------------------------------------------
                                     get context for destination folder        
     ---------------------------------------------------------------------------------------------------------*/
    (void)snprintf( psInstallFolderCtx_inout->sDestContext.sInstallFolder.pathname,
                sizeof(psInstallFolderCtx_inout->sDestContext.sInstallFolder.pathname),
                "%s", destInstallFolder) ;

    (void)snprintf(psInstallFolderCtx_inout->sDestContext.sRctDB.pathname,
                sizeof(psInstallFolderCtx_inout->sDestContext.sRctDB.pathname),
                "%s/%s", destInstallFolder, PERS_ORG_RCT_NAME) ;
    psInstallFolderCtx_inout->sDestContext.sRctDB.handler = persComRctOpen(psInstallFolderCtx_inout->sDestContext.sRctDB.pathname, true) ;
    if(psInstallFolderCtx_inout->sDestContext.sRctDB.handler >= 0)
    {
        psInstallFolderCtx_inout->sDestContext.sRctDB.bIsAvailable = true ;
    }
    else
    {
        bEverythingOK = false ;
    }
    (void)snprintf(g_msg, sizeof(g_msg), "%s - persComRctOpen(%s) returned <%d>", 
            __FUNCTION__, psInstallFolderCtx_inout->sDestContext.sRctDB.pathname, psInstallFolderCtx_inout->sDestContext.sRctDB.handler) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    if(bEverythingOK)
    {
        /* table with the files/folders to be checked in the destination folder */
        pas_conf_helpDestContextEntry_s tab_sDestLocalDBsInfo[] =
        {
            {false,  NIL, /* &psInstallFolderCtx_inout->sDestContext.sCachedDB*/               PERS_ORG_LOCAL_CACHE_DB_NAME},
            {false,  NIL, /* &psInstallFolderCtx_inout->sDestContext.sWtDB*/                   PERS_ORG_LOCAL_WT_DB_NAME},
            {false,  NIL, /* &psInstallFolderCtx_inout->sDestContext.sFactoryDefaultDB*/       PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME},
            {false,  NIL, /* &psInstallFolderCtx_inout->sDestContext.sConfigurableDefaultDB*/  PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME}
        } ;
        tab_sDestLocalDBsInfo[0].psFileAvailability = &psInstallFolderCtx_inout->sDestContext.sCachedDB ;
        tab_sDestLocalDBsInfo[1].psFileAvailability = &psInstallFolderCtx_inout->sDestContext.sWtDB ;
        tab_sDestLocalDBsInfo[2].psFileAvailability = &psInstallFolderCtx_inout->sDestContext.sFactoryDefaultDB ;
        tab_sDestLocalDBsInfo[3].psFileAvailability = &psInstallFolderCtx_inout->sDestContext.sConfigurableDefaultDB ;

        for(iIndex = 0 ; bEverythingOK && (iIndex < sizeof(tab_sDestLocalDBsInfo)/sizeof(tab_sDestLocalDBsInfo[0])) ; iIndex++)
        {
            (void)snprintf( tab_sDestLocalDBsInfo[iIndex].psFileAvailability->pathname,
                            sizeof(tab_sDestLocalDBsInfo[iIndex].psFileAvailability->pathname), 
                            "%s/%s", destInstallFolder, tab_sDestLocalDBsInfo[iIndex].filename) ;
            tab_sDestLocalDBsInfo[iIndex].psFileAvailability->handler = persComDbOpen(tab_sDestLocalDBsInfo[iIndex].psFileAvailability->pathname, true) ;
            if(tab_sDestLocalDBsInfo[iIndex].psFileAvailability->handler >= 0)
            {
                tab_sDestLocalDBsInfo[iIndex].psFileAvailability->bIsAvailable = true ;
            }
            else
            {
                bEverythingOK = false ;
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s - persComDbOpen(%s) returned <%d>", 
                    __FUNCTION__, tab_sDestLocalDBsInfo[iIndex].psFileAvailability->pathname, tab_sDestLocalDBsInfo[iIndex].psFileAvailability->handler) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));        
        }
    }

    /*---------------------------------------------------------------------------------------------------------
                                     get context for source folders        
     ---------------------------------------------------------------------------------------------------------*/
    if(bEverythingOK)
    {
        /* table with the folders to be checked in the source folder */
        pas_conf_helpSrcContextEntry_s tab_sSrcContextEntries[] =
        {
            {true,  NIL, /* &psInstallFolderCtx_inout->sSrcContext.sInstallFolder*/
                "",                                                  PersAdminCfgFileType_LastEntry,               true},
            {true,  NIL, /* &psInstallFolderCtx_inout->sSrcContext.sFiledataFolder*/
                "/"PERSADM_CFG_RESOURCE_ROOT_FILEDATA_FOLDER_NAME,   PersAdminCfgFileType_LastEntry,               true},
            {true,  NIL, /* &psInstallFolderCtx_inout->sSrcContext.sKeydataFolder*/
                "/"PERSADM_CFG_RESOURCE_ROOT_KEYDATA_FOLDER_NAME,    PersAdminCfgFileType_LastEntry,               true}
        };
        tab_sSrcContextEntries[0].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sInstallFolder ;
        tab_sSrcContextEntries[1].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sFiledataFolder ;
        tab_sSrcContextEntries[2].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sKeydataFolder ;

        for(iIndex = 0 ; bEverythingOK && (iIndex < sizeof(tab_sSrcContextEntries)/sizeof(tab_sSrcContextEntries[0])) ; iIndex++)
        {
            (void)snprintf( tab_sSrcContextEntries[iIndex].psFileAvailability->pathname,
                            sizeof(tab_sSrcContextEntries[iIndex].psFileAvailability->pathname), 
                            "%s%s", srcInstallFolder, tab_sSrcContextEntries[iIndex].filename) ;
            if(0 <= persadmin_check_if_file_exists(tab_sSrcContextEntries[iIndex].psFileAvailability->pathname, tab_sSrcContextEntries[iIndex].bIsFolder))
            {
                tab_sSrcContextEntries[iIndex].psFileAvailability->bIsAvailable = true ;  
            }
            else
            {
                tab_sSrcContextEntries[iIndex].psFileAvailability->bIsAvailable = false ; 
                if(tab_sSrcContextEntries[iIndex].bPresenceIsMandatory)
                {
                    bEverythingOK = false ;
                }
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s - %s <<%s>>", __FUNCTION__, 
                    tab_sSrcContextEntries[iIndex].psFileAvailability->bIsAvailable ? "Found" : "Not found",
                    tab_sSrcContextEntries[iIndex].psFileAvailability->pathname) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        /* table with the files to be checked in the source folder */
        pas_conf_helpSrcContextEntry_s tab_sSrcContextEntries[] =
        {
            {false,  NIL,/* &psInstallFolderCtx_inout->sSrcContext.sRctFile */
                PERSADM_CFG_RESOURCE_RCT_FILENAME,                                                                      PersAdminCfgFileType_RCT,               true},
            {false,  NIL,/* &psInstallFolderCtx_inout->sSrcContext.sFactoryDefaultDataFile */
                PERSADM_CFG_RESOURCE_ROOT_KEYDATA_FOLDER_NAME"/"PERSADM_CFG_RESOURCE_FACTORY_DEFAULT_KEYDATA_FILENAME,  PersAdminCfgFileType_Database,          true},
            {false,  NIL,/* &psInstallFolderCtx_inout->sSrcContext.sConfigurableDefaultDataFile */
                PERSADM_CFG_RESOURCE_ROOT_KEYDATA_FOLDER_NAME"/"PERSADM_CFG_RESOURCE_CONFIG_KEYDEFAULT_DATA_FILENAME,   PersAdminCfgFileType_Database,          true},
            {false,  NIL,/* &psInstallFolderCtx_inout->sSrcContext.sInitialDataFile */
                PERSADM_CFG_RESOURCE_ROOT_KEYDATA_FOLDER_NAME"/"PERSADM_CFG_RESOURCE_NON_DEFAULT_KEYDATA_FILENAME,      PersAdminCfgFileType_Database,          false},
            {false,  NIL,/* &psInstallFolderCtx_inout->sSrcContext.sExceptionsFile */
                PERSADM_CFG_RESOURCE_INSTALL_EXCEPTIONS_FILENAME,                                                       PersAdminCfgFileType_InstallExceptions, true}        
        } ;
        /* initialize here to please SCC */
        tab_sSrcContextEntries[0].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sRctFile ;
        tab_sSrcContextEntries[1].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sFactoryDefaultDataFile ;
        tab_sSrcContextEntries[2].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sConfigurableDefaultDataFile ;
        tab_sSrcContextEntries[3].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sInitialDataFile ;
        tab_sSrcContextEntries[4].psFileAvailability = &psInstallFolderCtx_inout->sSrcContext.sExceptionsFile ;

        for(iIndex = 0 ; bEverythingOK && (iIndex < sizeof(tab_sSrcContextEntries)/sizeof(tab_sSrcContextEntries[0])) ; iIndex++)
        {
            (void)snprintf( tab_sSrcContextEntries[iIndex].psFileAvailability->pathname,
                            sizeof(tab_sSrcContextEntries[iIndex].psFileAvailability->pathname), 
                            "%s/%s", srcInstallFolder, tab_sSrcContextEntries[iIndex].filename) ;
            if(0 <= persadmin_check_if_file_exists(tab_sSrcContextEntries[iIndex].psFileAvailability->pathname, tab_sSrcContextEntries[iIndex].bIsFolder))
            {
                tab_sSrcContextEntries[iIndex].psFileAvailability->bIsAvailable = true ;  
                tab_sSrcContextEntries[iIndex].psFileAvailability->handler = 
                        persAdmCfgFileOpen(tab_sSrcContextEntries[iIndex].psFileAvailability->pathname, tab_sSrcContextEntries[iIndex].eCfgFileType) ;
                if(tab_sSrcContextEntries[iIndex].psFileAvailability->handler < 0)
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgFileOpen(<%s>, %d) returned <%d>", 
                        __FUNCTION__, tab_sSrcContextEntries[iIndex].psFileAvailability->pathname,
                        tab_sSrcContextEntries[iIndex].eCfgFileType, tab_sSrcContextEntries[iIndex].psFileAvailability->handler) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));  
            }
            else
            {
                tab_sSrcContextEntries[iIndex].psFileAvailability->bIsAvailable = false ; 
                tab_sSrcContextEntries[iIndex].psFileAvailability->handler = -1 ;
                if(tab_sSrcContextEntries[iIndex].bPresenceIsMandatory)
                {
                    bEverythingOK = false ;
                }
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s - %s <<%s>>", __FUNCTION__, 
                    tab_sSrcContextEntries[iIndex].psFileAvailability->bIsAvailable ? "Found" : "Not found",
                    tab_sSrcContextEntries[iIndex].filename) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    
    }

    /*---------------------------------------------------------------------------------------------------------
                                     create lists        
     ---------------------------------------------------------------------------------------------------------*/
    if(bEverythingOK)
    {
        bEverythingOK = pas_conf_createListsForInstallFolder(psInstallFolderCtx_inout);
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - create lists... %s",
                    __FUNCTION__, bEverythingOK ? "OK" : "FAILED") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0021*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0022*/

/**
 * \brief Close handlers, free dynamicaly allocated memory for the public/group/app's context
 * \param psInstallFolderCtx_inout  [inout] pointer to the public/group/app's context
 * \return true for success, false other way
 */
static bool_t pas_conf_destroyContextInstallFolder(pas_cfg_instFoldCtx_s* psInstallFolderCtx_inout)
{
    bool_t  bEverythingOK = true ;
    size_t  iIndex = 0 ;
    sint_t* tab_pDestHandlers[] = 
    {
        NIL,/* &(psInstallFolderCtx_inout->sDestContext.sCachedDB.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sDestContext.sWtDB.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sDestContext.sFactoryDefaultDB.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sDestContext.sConfigurableDefaultDB.handler) */
    } ;

    sint_t* tab_pSrcHandlers[] = 
    {
        NIL,/* &(psInstallFolderCtx_inout->sSrcContext.sRctFile.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sSrcContext.sFactoryDefaultDataFile.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sSrcContext.sConfigurableDefaultDataFile.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sSrcContext.sInitialDataFile.handler) */
        NIL,/* &(psInstallFolderCtx_inout->sSrcContext.sExceptionsFile.handler) */
    } ;

    /* initialize here to please SCC */
    tab_pDestHandlers[0] = &(psInstallFolderCtx_inout->sDestContext.sCachedDB.handler) ;
    tab_pDestHandlers[1] = &(psInstallFolderCtx_inout->sDestContext.sWtDB.handler) ;
    tab_pDestHandlers[2] = &(psInstallFolderCtx_inout->sDestContext.sFactoryDefaultDB.handler) ;
    tab_pDestHandlers[3] = &(psInstallFolderCtx_inout->sDestContext.sConfigurableDefaultDB.handler) ;

    /* initialize here to please SCC */
    tab_pSrcHandlers[0] = &(psInstallFolderCtx_inout->sSrcContext.sRctFile.handler) ;
    tab_pSrcHandlers[1] = &(psInstallFolderCtx_inout->sSrcContext.sFactoryDefaultDataFile.handler) ;
    tab_pSrcHandlers[2] = &(psInstallFolderCtx_inout->sSrcContext.sConfigurableDefaultDataFile.handler) ;
    tab_pSrcHandlers[3] = &(psInstallFolderCtx_inout->sSrcContext.sInitialDataFile.handler) ;
    tab_pSrcHandlers[4] = &(psInstallFolderCtx_inout->sSrcContext.sExceptionsFile.handler) ;

    /* close destination  handlers */
    if(psInstallFolderCtx_inout->sDestContext.sRctDB.handler >= 0)
    {
        (void)persComRctClose(psInstallFolderCtx_inout->sDestContext.sRctDB.handler) ;
        psInstallFolderCtx_inout->sDestContext.sRctDB.handler = -1 ;
    }

    for(iIndex = 0 ; iIndex < sizeof(tab_pDestHandlers)/sizeof(tab_pDestHandlers[0]) ; iIndex++)
    {
        if(*tab_pDestHandlers[iIndex] >= 0)
        {
            (void)persComDbClose(*tab_pDestHandlers[iIndex]) ;
            *tab_pDestHandlers[iIndex] = -1 ;
        }
    }

    /* close source  handlers */
    for(iIndex = 0 ; iIndex < sizeof(tab_pSrcHandlers)/sizeof(tab_pSrcHandlers[0]) ; iIndex++)
    {
        if(*tab_pSrcHandlers[iIndex] >= 0)
        {
            (void)persAdmCfgFileClose(*tab_pSrcHandlers[iIndex]) ;
            *tab_pSrcHandlers[iIndex] = -1 ;
        }
    }
    
    /* distroy lists */
    bEverythingOK = pas_conf_destroyListsForInstallFolder(psInstallFolderCtx_inout) ;

    return bEverythingOK ;
}


/**
 * \brief Apply configuration for public/group/app's folder
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \return number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureDataForInstallFolder(pas_cfg_instFoldCtx_s* const psInstallFolderCtx)
{
    bool_t  bEverythingOK = true ;
    sint_t  errorCode = PAS_FAILURE ;
    sint_t  iNoOfInstalledResources = 0 ;  
    sint_t  iNoOfInstalledResourcesPartial = 0 ;  

    /* first un-install data for the resources no longer present in the source RCT */
    (void)snprintf(g_msg, sizeof(g_msg), "%s - process obsolete resources ...", __FUNCTION__) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    
    iNoOfInstalledResourcesPartial = pas_conf_uninstallObsoleteDataForInstallFolder(psInstallFolderCtx) ;
    if(iNoOfInstalledResourcesPartial >= 0)
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - Un-installed <%d> obsolete resources",
                __FUNCTION__, iNoOfInstalledResourcesPartial) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        bEverythingOK = false ;
    }
    (void)snprintf(g_msg, sizeof(g_msg), "%s - process obsolete resources ... %s", 
            __FUNCTION__, bEverythingOK ? "OK" : "FAILED") ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));


    /* then process eventual policy changes */
    if(bEverythingOK)
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - process policy changes...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        
        iNoOfInstalledResourcesPartial = pas_conf_processPolicyChangesForInstallFolder(psInstallFolderCtx) ;
        if(iNoOfInstalledResourcesPartial >= 0)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - Changed policy for <%d> resources",
                    __FUNCTION__, iNoOfInstalledResourcesPartial) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - process policy changes ...  %s",
                __FUNCTION__, bEverythingOK ? "OK" : "FAILED") ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    /* Configuration for factory-default data (if affected by the install rule) */
    if(bEverythingOK && (g_LookUpRules[psInstallFolderCtx->eInstallRule].affectedDataLookUp[pas_cfg_dataTypes_DefaultFactory]))
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - start configuration of factory default data...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

        iNoOfInstalledResourcesPartial = pas_conf_configureForDefault(psInstallFolderCtx, true) ;
        if(iNoOfInstalledResourcesPartial >= 0)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureForDefault(factory) returned %d", __FUNCTION__, iNoOfInstalledResources) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureForDefault(factory) FAILED", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }
    else
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - skip configuration of factory default data...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    /* Configuration for configurable-default data (if affected by the install rule) */
    if(bEverythingOK && (g_LookUpRules[psInstallFolderCtx->eInstallRule].affectedDataLookUp[pas_cfg_dataTypes_DefaultConfigurable]))
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - start configuration of configurable default data...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    
        iNoOfInstalledResourcesPartial = pas_conf_configureForDefault(psInstallFolderCtx, false) ;
        if(iNoOfInstalledResourcesPartial >= 0)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureForDefault(configurable) returned %d", __FUNCTION__, iNoOfInstalledResources) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureForDefault(configurable) FAILED", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }
    else
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - skip configuration of configurable default data...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    /* Configuration for non-default data (if affected by the install rule) */
    if(bEverythingOK && (g_LookUpRules[psInstallFolderCtx->eInstallRule].affectedDataLookUp[pas_cfg_dataTypes_NonDefault]))
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - start configuration of normal data...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    
        iNoOfInstalledResourcesPartial = pas_conf_configureForNonDefault(psInstallFolderCtx) ;
        if(iNoOfInstalledResourcesPartial >= 0)
        {
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureForNonDefault returned %d", __FUNCTION__, iNoOfInstalledResources) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_configureForNonDefault FAILED", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }
    else
    {
        (void)snprintf(g_msg, sizeof(g_msg), "%s - skip configuration of normal data...", __FUNCTION__) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK)
    {
        /* update local RCT - to mirror the source RCT */
        sint_t iNoOfUpdatesInRct = pas_conf_updateRctForInstallFolder(psInstallFolderCtx) ;
        if(iNoOfUpdatesInRct >= 0)
        {
            iNoOfInstalledResources = iNoOfUpdatesInRct ;
        }
        else
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_updateRctForInstallFolder returned <%d>", __FUNCTION__, iNoOfUpdatesInRct) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0023*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0024*/

/**
 * \brief Create (allocate and fill) the lists in pas_conf_listsForInstallFolder_s
 * \param psInstallFolderCtx_inout  [inout] pointer to the public/group/app's context
 * \return true for success, false other way
 */
static bool_t pas_conf_createListsForInstallFolder( pas_cfg_instFoldCtx_s*  psInstallFolderCtx_inout)
{
    bool_t  bEverythingOK = true ;

    if( ! psInstallFolderCtx_inout->sSrcContext.sRctFile.bIsAvailable)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - %s NOT available", 
            __FUNCTION__, psInstallFolderCtx_inout->sSrcContext.sRctFile.pathname) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    }

    if(bEverythingOK)
    {
        psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList = persAdmCfgRctGetSizeResourcesList(psInstallFolderCtx_inout->sSrcContext.sRctFile.handler) ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRctGetSizeResourcesList(hSrcRct) returned %d", 
                __FUNCTION__, psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        if(psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList > 0)
        {
            psInstallFolderCtx_inout->sLists.sSrcRCT.pList = (pstr_t)malloc((size_t)psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL != psInstallFolderCtx_inout->sLists.sSrcRCT.pList)
            {
                sint_t iErrLocal = persAdmCfgRctGetResourcesList(   psInstallFolderCtx_inout->sSrcContext.sRctFile.handler,
                                                                    psInstallFolderCtx_inout->sLists.sSrcRCT.pList,
                                                                    psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList) ;
                if(iErrLocal != psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList)
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRctGetResourcesList returned %d (expected %d)", 
                            __FUNCTION__, iErrLocal, psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            if(psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList < 0)
            {/* size of 0 is not an error (even if not much sense to have an empty RCT) */
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK)
    {
        psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList = persComRctGetSizeResourcesList(psInstallFolderCtx_inout->sDestContext.sRctDB.handler) ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persComRctGetSizeResourcesList(hDestRctDB) returned %d", 
                __FUNCTION__, psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        if(psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList > 0)
        {
            psInstallFolderCtx_inout->sLists.sDestRCT.pList = (pstr_t)malloc((size_t)psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL != psInstallFolderCtx_inout->sLists.sDestRCT.pList)
            {
                sint_t iErrLocal = persComRctGetResourcesList(  psInstallFolderCtx_inout->sDestContext.sRctDB.handler, 
                                                                psInstallFolderCtx_inout->sLists.sDestRCT.pList, 
                                                                psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList) ;
                if(iErrLocal != psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList)
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - persComRctGetResourcesList returned %d (expected %d)", 
                            __FUNCTION__, iErrLocal, psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            /* size of 0 is not an error (even if it would be strange) */
            if(psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList < 0)
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK)
    {
        /* create the list of obsolete resources */
        if(psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList > 0)
        {
            psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList = 0 ;
            psInstallFolderCtx_inout->sLists.sObsoleteResources.pList = 
                            (pstr_t)malloc((size_t)psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL != psInstallFolderCtx_inout->sLists.sObsoleteResources.pList)
            {
                sint_t iPosInDestList = 0 ;
                sint_t iPosInSrcList = 0 ;
                sint_t iPosInObsoleteList = 0 ;                
                pstr_t pCurrentDestResource = NIL ;
                pstr_t pCurrentSrcResource = NIL ;
                while(bEverythingOK && (iPosInDestList < psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList))
                {
                    bool_t bCurrentResourceIsObsolete = true ;
                    iPosInSrcList = 0 ;
                    pCurrentDestResource = psInstallFolderCtx_inout->sLists.sDestRCT.pList + iPosInDestList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
                    /* search the resource in the new(source) list */
                    while(iPosInSrcList < psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList)
                    {
                        pCurrentSrcResource = psInstallFolderCtx_inout->sLists.sSrcRCT.pList + iPosInSrcList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

                        iPosInSrcList+= ((sint_t)strlen(pCurrentSrcResource) + 1) ; /* 1 <=> '\0' */
                        
                        if(0 == strcmp(pCurrentDestResource, pCurrentSrcResource))
                        {
                            /* not obsolete resources */
                            bCurrentResourceIsObsolete = false ;
                            break ;
                        }
                    }

                    if(bCurrentResourceIsObsolete)
                    {
                        (void)strcpy(psInstallFolderCtx_inout->sLists.sObsoleteResources.pList+iPosInObsoleteList, pCurrentDestResource) ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
                        psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList+= ((sint_t)strlen(pCurrentDestResource) + 1) ; /* 1 <=> '\0' */
                        iPosInObsoleteList = psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList ;
                        (void)snprintf(g_msg, sizeof(g_msg), "%s - obsolete resource <<%s>> list(size=%d, pos=%d)", 
                                __FUNCTION__, pCurrentDestResource, 
                                psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList, iPosInObsoleteList) ;
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    }

                    iPosInDestList+= ((sint_t)strlen(pCurrentDestResource) + 1) ; /* 1 <=> '\0' */
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            /* no old resources => no obsolete resources */
            (void)snprintf(g_msg, sizeof(g_msg), "%s - No obsolete resources...", __FUNCTION__) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList = 0 ;
            psInstallFolderCtx_inout->sLists.sObsoleteResources.pList = NIL ;
        }
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0025*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0026*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0027*/

/**
 * \brief Distroy (de-allocate) the lists previously created by pas_conf_createListsForInstallFolder
 * \param psInstallFolderCtx_inout  [inout] pointer to the public/group/app's context
 * \return true for success, false other way
 */
static bool_t pas_conf_destroyListsForInstallFolder(pas_cfg_instFoldCtx_s*  psInstallFolderCtx_inout)
{
    if(     (psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList >= 0) 
        &&  (NIL != psInstallFolderCtx_inout->sLists.sSrcRCT.pList))
    {
        free(psInstallFolderCtx_inout->sLists.sSrcRCT.pList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        psInstallFolderCtx_inout->sLists.sSrcRCT.pList = NIL ;
        psInstallFolderCtx_inout->sLists.sSrcRCT.sizeOfList = 0 ;
    }

    if(     (psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList >= 0)
        &&  (NIL != psInstallFolderCtx_inout->sLists.sDestRCT.pList))
    {
        free(psInstallFolderCtx_inout->sLists.sDestRCT.pList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        psInstallFolderCtx_inout->sLists.sDestRCT.pList = NIL ;
        psInstallFolderCtx_inout->sLists.sDestRCT.sizeOfList = 0 ;
    } 

    if(     (psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList >= 0)
        &&  (NIL != psInstallFolderCtx_inout->sLists.sObsoleteResources.pList))
    {
        free(psInstallFolderCtx_inout->sLists.sObsoleteResources.pList); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        psInstallFolderCtx_inout->sLists.sObsoleteResources.pList = NIL ;
        psInstallFolderCtx_inout->sLists.sObsoleteResources.sizeOfList = 0 ;
    } 

    return true ;
}

/**
 * \brief Process policy change for resources of public/group/app
 * \param psInstallFolderCtx  [inout] pointer to the public/group/app's context
 * \return number policy changes performed, or negative value for error
 */
static sint_t pas_conf_processPolicyChangesForInstallFolder(pas_cfg_instFoldCtx_s* const psInstallFolderCtx)
{
    /* identify the key type local resorces affected by policy change
     * for each of these, call pas_conf_processPolicyChangesForKey */
       
    bool_t                          bEverythingOK = true ;
    sint_t                          iNoOfPolicyChanges = 0 ;
    sint_t                          iPosInSrcRctList = 0 ;
    pstr_t                          pCurrentResourceID = NIL ;
    PersistenceConfigurationKey_s   sSrcConfig ;
    PersistenceConfigurationKey_s   sDestConfig ;
    /* pas_conf_getContextInstallFolder assures that all the handlers below are opened */
    sint_t                          hSrcRct = psInstallFolderCtx->sSrcContext.sRctFile.handler ;
    sint_t                          hDestRct = psInstallFolderCtx->sDestContext.sRctDB.handler ;
    sint_t                          hDestCachedDB = psInstallFolderCtx->sDestContext.sCachedDB.handler ;
    sint_t                          hDestWtDB = psInstallFolderCtx->sDestContext.sWtDB.handler ;

    while(bEverythingOK && (iPosInSrcRctList < psInstallFolderCtx->sLists.sSrcRCT.sizeOfList))
    {
        pCurrentResourceID = psInstallFolderCtx->sLists.sSrcRCT.pList + iPosInSrcRctList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

        if(0 <= persAdmCfgRctRead(hSrcRct, pCurrentResourceID, &sSrcConfig))
        {
            if(0 <= persComRctRead(hDestRct, pCurrentResourceID, &sDestConfig))
            {
                if(     (PersistenceResourceType_key == sSrcConfig.type) 
                    &&  (PersistenceResourceType_key == sDestConfig.type)
                    &&  (sSrcConfig.storage < PersistenceStorage_custom) 
                    &&  (sDestConfig.storage < PersistenceStorage_custom)
                    &&  (sDestConfig.policy != sSrcConfig.policy))
                {
                    /* initialize this way to please SCC */
                    pas_conf_keyPolicyChangeCtx_s sKeyPolicyChangeCtx = {0} ;
                    sKeyPolicyChangeCtx.psSrcConfig = &sSrcConfig ;
                    sKeyPolicyChangeCtx.psDestConfig = &sDestConfig ;
                    sKeyPolicyChangeCtx.hDestCachedDB = hDestCachedDB ;
                    sKeyPolicyChangeCtx.hDestWtDB = hDestWtDB ;
                
                    iNoOfPolicyChanges++ ;
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - change policy (%s) for <%s>", 
                        __FUNCTION__,
                        (PersistencePolicy_wc == sSrcConfig.policy) ? "wt -> wc" : "wc -> wt",
                        pCurrentResourceID) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    if( ! pas_conf_processPolicyChangesForKey(pCurrentResourceID, psInstallFolderCtx, &sKeyPolicyChangeCtx))
                    {
                        bEverythingOK = false ;
                    }
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_processPolicyChangesForKey(%s)...%s", 
                        __FUNCTION__, pCurrentResourceID, bEverythingOK ? "OK" : "FAILED") ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }
        }
        
        
        iPosInSrcRctList += ((sint_t)strlen(pCurrentResourceID)+1) ; /* 1 <=> '\0' */
    }

    return bEverythingOK ? iNoOfPolicyChanges : PAS_FAILURE ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0028*/


/**
 * \brief Perform policy change for one resources
 * \param resourceID            [in] resource identifier
 * \param psInstallFolderCtx    [in] pointer to the public/group/app's context
 * \param psKeyPolicyChangeCtx  [in] relevant context for resourceID
 * \return number policy changes performed, or negative value for error
 */
static bool_t pas_conf_processPolicyChangesForKey(  pstr_t                               resourceID, 
                                                    pas_cfg_instFoldCtx_s* const         psInstallFolderCtx, 
                                                    pas_conf_keyPolicyChangeCtx_s* const psKeyPolicyChangeCtx)
{
    /* get the list of entries (related to the key) in e.g. cachedDB and move them into wtDB */
    bool_t  bEverythingOK = true ;
    sint_t hOldDB = -1 ;
    sint_t hNewDB = -1 ;
    sint_t iListUnfilteredSize = 0 ;
    pstr_t pListUnfilteredBuffer = NIL ;
    sint_t iListFilteredSize = 0 ;
    pstr_t pListFilteredBuffer = NIL ;
    
    switch(psKeyPolicyChangeCtx->psSrcConfig->policy)
    {
        case PersistencePolicy_wc:
        {
            hNewDB = psKeyPolicyChangeCtx->hDestCachedDB ;
            hOldDB = psKeyPolicyChangeCtx->hDestWtDB ;
            break ;
        }
        case PersistencePolicy_wt:
        {
            hNewDB = psKeyPolicyChangeCtx->hDestWtDB ;
            hOldDB = psKeyPolicyChangeCtx->hDestCachedDB ;
            break ;
        }
        default:
        {
            bEverythingOK = false ;
            break ;
        }
    }

    if(bEverythingOK)
    {
        /* now it is clear where from to take the data and where to move it
         * but we only have the name of the resourceID in RCT, 
         * not the name of the keys in cached/wt DBs (this contains node/group/user/seat prefix) */
        iListUnfilteredSize = persComDbGetSizeKeysList(hOldDB) ;
        
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persComDbGetSizeKeysList() returned <%d>", 
            __FUNCTION__, iListUnfilteredSize) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

        if(iListUnfilteredSize > 0)
        {
            pListUnfilteredBuffer = (pstr_t)malloc((size_t)iListUnfilteredSize) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL != pListUnfilteredBuffer)
            {
                if(iListUnfilteredSize != persComDbGetKeysList(hOldDB, pListUnfilteredBuffer, iListUnfilteredSize))
                {
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            if(iListUnfilteredSize < 0)
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK && (iListUnfilteredSize > 0))
    {        
        /* pListUnfilteredBuffer contains the names of all the keys, but we have to filter the keys we want to move */
        pListFilteredBuffer = (pstr_t)malloc((size_t)iListUnfilteredSize) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        if(NIL != pListFilteredBuffer)
        {
            /* initialize this way to please SCC */
            pas_conf_listOfItems_s sUnfilteredList = {0} ;
            sUnfilteredList.pList = pListUnfilteredBuffer ;
            sUnfilteredList.sizeOfList = iListUnfilteredSize ;
        
            iListFilteredSize = pas_conf_filterResourcesList(resourceID, false, true, &sUnfilteredList, pListFilteredBuffer);
            if(iListFilteredSize < 0)
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }

    if(bEverythingOK && (iListFilteredSize > 0))
    {
        str_t   keyData[PERS_DB_MAX_SIZE_KEY_DATA] ;
        sint_t  iDataSize ;
        sint_t  iPosInListing = 0 ;
        pstr_t  pOldDbPathname = (PersistencePolicy_wt == psKeyPolicyChangeCtx->psSrcConfig->policy) ? 
                                 psInstallFolderCtx->sDestContext.sCachedDB.pathname : 
                                 psInstallFolderCtx->sDestContext.sWtDB.pathname ;
        pstr_t  pNewDbPathname = (PersistencePolicy_wt == psKeyPolicyChangeCtx->psSrcConfig->policy) ? 
                                 psInstallFolderCtx->sDestContext.sWtDB.pathname : 
                                 psInstallFolderCtx->sDestContext.sCachedDB.pathname ;
        
        while((iPosInListing < iListFilteredSize) && bEverythingOK)
        {
            pstr_t pCurrentKey = pListFilteredBuffer + iPosInListing ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

            iDataSize = persComDbReadKey(hOldDB, pCurrentKey, keyData, sizeof(keyData)) ;
            if(iDataSize >= 0)
            {
                if(0 <= persComDbWriteKey(hNewDB, pCurrentKey, keyData, iDataSize))
                {
                    (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbWriteKey(<%s>, <%s>, %d) done",
                                __FUNCTION__, pNewDbPathname, pCurrentKey, iDataSize) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    if(0 > persComDbDeleteKey(hOldDB, pCurrentKey))
                    {
                        (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbDeleteKey(<%s>, <%s>) failed",
                                    __FUNCTION__, pOldDbPathname, pCurrentKey) ;
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                        bEverythingOK = false ;
                    }
                    else
                    {
                        (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbDeleteKey(<%s>, <%s>) done",
                                    __FUNCTION__, pOldDbPathname, pCurrentKey) ;
                        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    }
                }
                else
                {
                    (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbWriteKey(<%s>, <%s>, %d) failed",
                                __FUNCTION__, pNewDbPathname, pCurrentKey, iDataSize) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    bEverythingOK = false ;
                }
            }

            iPosInListing += ((sint_t)strlen(pCurrentKey)+1); /* 1 <=> '\0' */
        }
    }

    if(NIL != pListUnfilteredBuffer)
    {
        free(pListUnfilteredBuffer) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }
    if(NIL != pListFilteredBuffer)
    {
        free(pListFilteredBuffer) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }    

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 1-SSW_Administrator_0029*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0030*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0031*/

/**
 * \brief Remove the data for obsolete resources of public/group/app
 * \param psInstallFolderCtx  [inout] pointer to the public/group/app's context
 * \return number of obsolete resources removed, or negative value for error
 */
static sint_t pas_conf_uninstallObsoleteDataForInstallFolder(pas_cfg_instFoldCtx_s* const psInstallFolderCtx)
{
    bool_t bEverythingOK = true ;
    sint_t iNoOfRemovedObsoleteResources = 0 ;
    bool_t bObsoleteResourcesAvailable = false ;
    sint_t hDestRct = psInstallFolderCtx->sDestContext.sRctDB.handler ;

    if(psInstallFolderCtx->sLists.sObsoleteResources.sizeOfList > 0)
    {
        /* there are obsolete resources */
        bObsoleteResourcesAvailable = true ;
    }

    if(bObsoleteResourcesAvailable)
    {
        PersistenceConfigurationKey_s sConfig ;
        pstr_t pCurrentObsoleteResource = NIL ;
        sint_t iPosInList = 0 ;
        while(bEverythingOK && (iPosInList < psInstallFolderCtx->sLists.sObsoleteResources.sizeOfList))
        {
            sint_t iResultLocal ;
            pCurrentObsoleteResource = psInstallFolderCtx->sLists.sObsoleteResources.pList + iPosInList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

            iResultLocal = persComRctRead(hDestRct, pCurrentObsoleteResource, &sConfig);
            if((sint_t)sizeof(sConfig) == iResultLocal)
            {
                iNoOfRemovedObsoleteResources++ ;
                bEverythingOK = pas_conf_uninstallObsoleteResource(pCurrentObsoleteResource, &sConfig, psInstallFolderCtx) ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_uninstallObsoleteResource(%s)...%s", 
                    __FUNCTION__, pCurrentObsoleteResource, bEverythingOK ? "OK" : "FAILED") ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
                bEverythingOK = false ;
            }
            iPosInList += ((sint_t)strlen(pCurrentObsoleteResource)+1) ; /* 1 <=> '\0' */
        }
    }

    return bEverythingOK ? iNoOfRemovedObsoleteResources : PAS_FAILURE ;
}


/**
 * \brief Remove the data for one obsolete resource
 * \param resourceID          [in] resource's identifier
 * \param psConfig            [in] resource's configuration
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \return true for success, false otherway
 */
static bool_t pas_conf_uninstallObsoleteResource(   constpstr_t                             resourceID,
                                                    PersistenceConfigurationKey_s* const    psConfig,
                                                    pas_cfg_instFoldCtx_s* const            psInstallFolderCtx)
{
    bool_t bEverythingOK = true ;

    (void)snprintf(g_msg, sizeof(g_msg), "%s - <<%s>> policy=%d type=%d", 
        __FUNCTION__, resourceID, psConfig->policy, psConfig->type) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    switch(psConfig->type)
    {
        case PersistenceResourceType_key:
        {
            bEverythingOK = pas_conf_uninstallObsoleteKey(resourceID, psConfig, psInstallFolderCtx) ;
            break;
        }
        case PersistenceResourceType_file:
        {
            bEverythingOK = pas_conf_uninstallObsoleteFile(resourceID, psConfig, psInstallFolderCtx) ;
            break;
        }
        default:
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - unknown type = %d", __FUNCTION__, psConfig->type) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            break;
        }            
    }

    return bEverythingOK ;
}

/**
 * \brief Remove the data for one obsolete key type resource
 * \param resourceID          [in] resource's identifier
 * \param psConfig            [in] resource's configuration
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \return true for success, false otherway
 */
static bool_t pas_conf_uninstallObsoleteKey(    pstr_t                                  resourceID,
                                                PersistenceConfigurationKey_s* const    psConfig,
                                                pas_cfg_instFoldCtx_s* const            psInstallFolderCtx)
{
    bool_t bEverythingOK = true ;
    bool_t bUninstallSupported = true ;

    if(psConfig->storage >= PersistenceStorage_custom)
    {
        bUninstallSupported = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - not supported for <%s> storage=%d", 
                __FUNCTION__, resourceID, psConfig->storage) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bEverythingOK && bUninstallSupported)
    {
        /* remove the entry from factory-default DB - if available */
        if(psInstallFolderCtx->sDestContext.sFactoryDefaultDB.bIsAvailable)
        {
            sint_t hDefaultDB = psInstallFolderCtx->sDestContext.sFactoryDefaultDB.handler;
            if(hDefaultDB >= 0)
            {
                sint_t iResult = persComDbDeleteKey(hDefaultDB, resourceID) ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbDeleteKey(<%s>, <%s>)returned <%d>",
                            __FUNCTION__, psInstallFolderCtx->sDestContext.sFactoryDefaultDB.pathname, resourceID, iResult) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                if((iResult < 0) && (PERS_COM_ERR_NOT_FOUND != iResult))
                {
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK && bUninstallSupported)
    {
        /* remove the entry from configurable-default DB - if available */
        if(psInstallFolderCtx->sDestContext.sConfigurableDefaultDB.bIsAvailable)
        {
            sint_t hDefaultDB = psInstallFolderCtx->sDestContext.sConfigurableDefaultDB.handler ;
            if(hDefaultDB >= 0)
            {
                sint_t iResult = persComDbDeleteKey(hDefaultDB, resourceID) ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbDeleteKey(<%s>, <%s>)returned <%d>",
                            __FUNCTION__, psInstallFolderCtx->sDestContext.sConfigurableDefaultDB.pathname, resourceID, iResult) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                if((iResult < 0) && (PERS_COM_ERR_NOT_FOUND != iResult))
                {
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK && bUninstallSupported)
    {
        /* remove the entry from non-default DB (cached or wt according to key's policy) */
        sint_t hNonDefaultDB = -1;
        pstr_t pNonDefaultDbPathname = NIL ;
        switch(psConfig->policy)
        {
            case PersistencePolicy_wc:
            {
                hNonDefaultDB = psInstallFolderCtx->sDestContext.sCachedDB.handler ;
                pNonDefaultDbPathname = psInstallFolderCtx->sDestContext.sCachedDB.pathname ;
                break;
            }
            case PersistencePolicy_wt:
            {
                hNonDefaultDB = psInstallFolderCtx->sDestContext.sWtDB.handler ;
                pNonDefaultDbPathname = psInstallFolderCtx->sDestContext.sWtDB.pathname ;
                break;
            }
            default:
            {
                bEverythingOK = false ;
                break;
            }
        }

        if(bEverythingOK)
        {
            if( ! pas_conf_deleteNonDefaultDataForKeyTypeResource(resourceID, hNonDefaultDB))
            {
                bEverythingOK = false ;
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s pas_conf_deleteNonDefaultDataForKeyTypeResource(<%s>, <%s>)...%s",
                        __FUNCTION__, resourceID, pNonDefaultDbPathname, bEverythingOK ? "OK" : "FAILED") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
        else
        {
            bEverythingOK = false ;
        }
    }


    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0032*/

/**
 * \brief Remove the data for one obsolete file type resource
 * \param resourceID          [in] resource's identifier
 * \param psConfig            [in] resource's configuration
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \return true for success, false otherway
 */
static bool_t pas_conf_uninstallObsoleteFile(   pstr_t                                  resourceID,
                                                PersistenceConfigurationKey_s* const    psConfig,
                                                pas_cfg_instFoldCtx_s* const            psInstallFolderCtx)
{
    bool_t  bEverythingOK = true ;
    str_t   filePathname[PERSADMIN_MAX_PATH_LENGHT] ;
    sint_t  iSizeOfListing = 0 ;
    pstr_t  pListingBuffer = NIL ;
    pstr_t  pListingBufferFiltered = NIL ;
    pstr_t  installFolderPathname =  psInstallFolderCtx->sDestContext.sInstallFolder.pathname ; /* just to have shorter expressions */
    bool_t  bUninstallSupported = true ;

    if(psConfig->storage >= PersistenceStorage_custom)
    {
        bUninstallSupported = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - not supported for <%s> storage=%d", 
                __FUNCTION__, resourceID, psConfig->storage) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    if(bUninstallSupported)
    {
        /* get a list of all the files in the application folder */
        iSizeOfListing = persadmin_list_folder_get_size(installFolderPathname, PersadminFilterFilesRegular, true) ;

        (void)snprintf(g_msg, sizeof(g_msg), "%s: persadmin_list_folder_get_size(%s) returned <%d>",
                    __FUNCTION__, installFolderPathname, iSizeOfListing) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

        if(iSizeOfListing > 0)
        {
            pListingBuffer = (pstr_t)malloc((size_t)iSizeOfListing) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            pListingBufferFiltered = (pstr_t)malloc((size_t)iSizeOfListing) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if((NIL != pListingBuffer) && (NIL != pListingBufferFiltered))
            {
                if(iSizeOfListing != persadmin_list_folder(installFolderPathname, pListingBuffer, iSizeOfListing, PersadminFilterFilesRegular, true))
                {
                    /* very strange */
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK && bUninstallSupported && (iSizeOfListing > 0))
    {
        /* we have the list with all the files in the install folder
         * now we have to filter and delete all the files with the same name as resourceID */
        sint_t iSizeOfListFiltered = 0 ;
        /* initialize this way to please SCC */
        pas_conf_listOfItems_s sUnfilteredList = {0} ;
        sUnfilteredList.pList = pListingBuffer ;
        sUnfilteredList.sizeOfList = iSizeOfListing ;
        
        iSizeOfListFiltered = pas_conf_filterResourcesList(resourceID, false, false, &sUnfilteredList, pListingBufferFiltered) ;

        if(iSizeOfListFiltered > 0)
        {
            sint_t iPosInBuffer = 0 ;
            pstr_t pCurrentFilePathname = NIL ;
            while((iPosInBuffer < iSizeOfListFiltered) && bEverythingOK)
            {
                sint_t iDeleteResult ;
                pCurrentFilePathname = pListingBufferFiltered + iPosInBuffer ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

                /* delete the file */
                
                (void)snprintf(g_msg, sizeof(g_msg), "%s: resID=<%s> filename=<%s>", __FUNCTION__, resourceID, pCurrentFilePathname) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                
                (void)snprintf(filePathname, sizeof(filePathname), "%s/%s", installFolderPathname, pCurrentFilePathname) ;
                iDeleteResult = persadmin_delete_file(filePathname) ;
                if(iDeleteResult < 0)
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s persadmin_delete_file(<%s>) returned <%d>", __FUNCTION__, filePathname, iDeleteResult) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

                iPosInBuffer += ((sint_t)strlen(pCurrentFilePathname) + 1) ; /* 1 <=>  '\0' */
            }
        }
        else
        {
            if(iSizeOfListFiltered < 0)
            {
                bEverythingOK = false ;
            }
        }
    }

    if(pListingBuffer)
    {
        free(pListingBuffer) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }   
    if(pListingBufferFiltered)
    {
        free(pListingBufferFiltered) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 6-SSW_Administrator_0033*/

/**
 * \brief Update the existent public/group/app's existent RCT in order to match the input(source) RCT
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \return the number of updated RCT entries, or negative value in case of error
 */
static sint_t pas_conf_updateRctForInstallFolder(pas_cfg_instFoldCtx_s* const   psInstallFolderCtx)
{
    bool_t bEverythingOK = true ;
    sint_t iNoOfRctEntriesModified = 0 ;
    /* pas_conf_getContextInstallFolder assures that all the handlers below are opened */
    sint_t hDestRCT = psInstallFolderCtx->sDestContext.sRctDB.handler ;
    sint_t hSrcRCT = psInstallFolderCtx->sSrcContext.sRctFile.handler;

    if(bEverythingOK)
    {
        /* first remove the obsolete resources */
        sint_t iPosInList = 0 ;
        pstr_t pCurrentResource = NIL ;
        sint_t iCurrentNumberOfResourceProcessed = 0 ;
        while(bEverythingOK && (iPosInList < psInstallFolderCtx->sLists.sObsoleteResources.sizeOfList))
        {
            if(0 == (++iCurrentNumberOfResourceProcessed)%15) /* retrigger watchdog once for every 15 resources processed*/
            {
                /* retrigger watchdog */
                persadmin_RetriggerWatchdog();
            }
        
            pCurrentResource = psInstallFolderCtx->sLists.sObsoleteResources.pList + iPosInList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            if(0 <= persComRctDelete(hDestRCT, pCurrentResource))
            {
                iNoOfRctEntriesModified++ ;
            }
            else
            {
                bEverythingOK = false ;
            }
            (void)snprintf(g_msg, sizeof(g_msg), "%s persComRctDelete(%s) %s", __FUNCTION__,
                    pCurrentResource, bEverythingOK ? "OK" : "FAILED") ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

            iPosInList += ((sint_t)strlen(pCurrentResource) + 1) ; /* 1 <=> '\0'*/
        }
    }

    if(bEverythingOK)
    {
        /* then (over)write the old RCT entries with the new ones */
        sint_t                          iPosInList = 0 ;
        pstr_t                          pCurrentResource = NIL ;
        PersistenceConfigurationKey_s   sOldConfig = {PersistencePolicy_na} ;
        PersistenceConfigurationKey_s   sNewConfig = {PersistencePolicy_na};
        bool_t                          bIsNewRsource = false ; 
        sint_t iCurrentNumberOfResourceProcessed = 0 ;
        while(bEverythingOK && (iPosInList < psInstallFolderCtx->sLists.sSrcRCT.sizeOfList))
        {
            bIsNewRsource = false ;
            pCurrentResource = psInstallFolderCtx->sLists.sSrcRCT.pList + iPosInList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

            if(0 == (++iCurrentNumberOfResourceProcessed)%15) /* retrigger watchdog once for every 15 resources processed*/
            {
                /* retrigger watchdog */
                persadmin_RetriggerWatchdog();
            }
            
            if(0 <= persAdmCfgRctRead(hSrcRCT, pCurrentResource, &sNewConfig))
            {
                sint_t iReadSize = persComRctRead(hDestRCT, pCurrentResource, &sOldConfig) ;
                if(PERS_COM_ERR_NOT_FOUND == iReadSize)
                {
                    bIsNewRsource = true ;
                }
                if( (iReadSize < 0) && (PERS_COM_ERR_NOT_FOUND != iReadSize))
                {
                    bEverythingOK = false ;
                }
            }
            else
            {
                bEverythingOK = false ;
            }

            if(((0 != memcmp(&sNewConfig, &sOldConfig, sizeof(sOldConfig))) || bIsNewRsource)
                && (bEverythingOK) ) /* normally first check, but SCC suspects that memcmp has side effects */
            {
                /* new resource or different configuration => (over)write with the new one */
                if(0 > persComRctWrite(hDestRCT, pCurrentResource, &sNewConfig))
                {
                    bEverythingOK = false ;
                }
                else
                {
                    iNoOfRctEntriesModified++ ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s update of RCT for <%s> %s", __FUNCTION__,
                        pCurrentResource, bEverythingOK ? "OK" : "FAILED") ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            iPosInList += ((sint_t)strlen(pCurrentResource) + 1) ; /* 1 <=> '\0'*/
        }
    }

    return bEverythingOK ? iNoOfRctEntriesModified : PAS_FAILURE ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief Apply configuration on public/group/app's default data
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \param bForFactoryDefault  [in] select data to be affected (factory-default for true, configurable-default for false)
 * \return the number of configured resources, or negative value in case of error
 */
static sint_t pas_conf_configureForDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx, bool_t bForFactoryDefault)
{
    bool_t                              bEverythingOK = true ;
    sint_t                              errorCode = PAS_FAILURE ;
    sint_t                              iNoOfInstalledResources = 0 ;
    pas_conf_handlersForDefault_s       sHandlers ;
    pas_conf_listsForInstallFolder_s*   psLists = &psInstallFolderCtx->sLists; /* only to have shorter expressions */

    (void)memset(&sHandlers, 0x0, sizeof(sHandlers));

    if( ! pas_conf_setupHandlersForDefault(psInstallFolderCtx, bForFactoryDefault,  &sHandlers))
    {
        bEverythingOK = false ;
    }

    if(bEverythingOK)
    {
        sint_t                          iPosInListOfResources = 0 ;
        pstr_t                          pCurrentResourceID = NIL ;
        sint_t                          iCurrentNumberOfResourceProcessed = 0 ;

        /* for each resource in the list:
         *   - find out which kind of configuration to apply (if any)
         *   - apply configuration */
        while(bEverythingOK && (iPosInListOfResources < psLists->sSrcRCT.sizeOfList))
        {
            bool_t                  bSkipCurrentResource = false ;
            pas_cfg_configTypes_e   eConfigTypeForResource = pas_cfg_configType_dontTouch ;

            if(0 == (++iCurrentNumberOfResourceProcessed)%15) /* retrigger watchdog once for every 15 resources processed*/
            {
                /* retrigger watchdog */
                persadmin_RetriggerWatchdog();
            }
            
            pCurrentResourceID = psLists->sSrcRCT.pList + iPosInListOfResources ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            
            if(pas_conf_getConfigurationNeededForResource(pCurrentResourceID, psInstallFolderCtx->eInstallRule, sHandlers.hSrcExceptions, &eConfigTypeForResource))
            {
                if(pas_cfg_configType_dontTouch == eConfigTypeForResource)
                {
                    bSkipCurrentResource = true ;
                }
            }
            else
            {
                bSkipCurrentResource = true ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_getConfigurationNeededForResource(%s) FAILED. Skip configuration...",
                        __FUNCTION__, pCurrentResourceID) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            if( (! bSkipCurrentResource) && (pas_cfg_configType_update == eConfigTypeForResource))
            {
                if(pas_conf_updateDefaultDataForResource(pCurrentResourceID, psInstallFolderCtx, &sHandlers, bForFactoryDefault))
                {
                    iNoOfInstalledResources++ ;
                }
                else
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_updateDefaultDataForResource(<%s>, <%s>) ... %s",
                        __FUNCTION__, pCurrentResourceID,
                        bForFactoryDefault ? "factory" : "configurable",
                        bEverythingOK ? "OK" : "FAILED") ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            if( (! bSkipCurrentResource) && (pas_cfg_configType_delete == eConfigTypeForResource))
            {
                if(pas_conf_deleteDefaultDataForResource(pCurrentResourceID, psInstallFolderCtx, &sHandlers, bForFactoryDefault))
                {
                    iNoOfInstalledResources++ ;
                }
                else
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_deleteDefaultDataForResource(<%s>, <%s>) ... %s",
                        __FUNCTION__, pCurrentResourceID,
                        bForFactoryDefault ? "factory" : "configurable",
                        bEverythingOK ? "OK" : "FAILED") ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            iPosInListOfResources += ((sint_t)strlen(pCurrentResourceID) + 1) ; /* 1 <=> '\0' */
        }
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief Setup handlers to public/group/app's default data accordinf to pas_conf_handlersForDefault_s
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \param bForFactoryDefault  [in] select data to be affected (factory-default for true, configurable-default for false)
 * \param psHandlers_out      [out] pointer to the handlers structure
 * \return true for success, false other way
 */
static bool_t pas_conf_setupHandlersForDefault(  pas_cfg_instFoldCtx_s* const    psInstallFolderCtx, 
                                                bool_t                          bForFactoryDefault,
                                                pas_conf_handlersForDefault_s*  psHandlers_out)
{
    bool_t  bEverythingOK = true ;

    psHandlers_out->hDestRctDB              = psInstallFolderCtx->sDestContext.sRctDB.handler ;
    psHandlers_out->hDestDefaultDataDB      = bForFactoryDefault ? 
                                                psInstallFolderCtx->sDestContext.sFactoryDefaultDB.handler :
                                                psInstallFolderCtx->sDestContext.sConfigurableDefaultDB.handler ;
    psHandlers_out->hSrcRctFile             = psInstallFolderCtx->sSrcContext.sRctFile.handler;
    psHandlers_out->hSrcDefaultDataFile     = bForFactoryDefault ?
                                                psInstallFolderCtx->sSrcContext.sFactoryDefaultDataFile.handler :
                                                psInstallFolderCtx->sSrcContext.sConfigurableDefaultDataFile.handler ;
    psHandlers_out->hSrcExceptions          = psInstallFolderCtx->sSrcContext.sExceptionsFile.handler;


    return bEverythingOK ;
}


/**
 * \brief Configure non-default(initial and run-time) data for install folder
 * \param psInstallFolderCtx    [in] pointer to install folder context
 * \return the number of installed resources, or negative value for error
 */
static sint_t pas_conf_configureForNonDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx)
{
    bool_t                              bEverythingOK = true ;
    sint_t                              errorCode = PAS_FAILURE ;
    sint_t                              iNoOfInstalledResources = 0 ;
    pas_conf_handlersForNonDefault_s    sHandlers = {0} ;
    pas_conf_listsForInstallFolder_s*   psLists = &psInstallFolderCtx->sLists; /* only to have shorter expressions */

    if( ! pas_conf_setupHandlersForNonDefault(psInstallFolderCtx,  &sHandlers))
    {
        bEverythingOK = false ;
    }

    if(bEverythingOK)
    {
        sint_t                          iPosInListOfResources = 0 ;
        pstr_t                          pCurrentResourceID = NIL ;

        /* for each resource in the list:
         *   - find out which kind of configuration to apply (if any)
         *   - apply configuration */
        while(bEverythingOK && (iPosInListOfResources < psLists->sSrcRCT.sizeOfList))
        {
            bool_t                  bSkipCurrentResource = false ;
            pas_cfg_configTypes_e   eConfigTypeForResource = pas_cfg_configType_dontTouch ;
            
            pCurrentResourceID = psLists->sSrcRCT.pList + iPosInListOfResources ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            
            if(pas_conf_getConfigurationNeededForResource(pCurrentResourceID, psInstallFolderCtx->eInstallRule, sHandlers.hSrcExceptions, &eConfigTypeForResource))
            {
                if(pas_cfg_configType_dontTouch == eConfigTypeForResource)
                {
                    bSkipCurrentResource = true ;
                }
            }
            else
            {
                bSkipCurrentResource = true ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_getConfigurationNeededForResource(%s) FAILED. Skip configuration...",
                        __FUNCTION__, pCurrentResourceID) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            if( (! bSkipCurrentResource) && (pas_cfg_configType_update == eConfigTypeForResource))
            {
                (void)snprintf(g_msg, sizeof(g_msg), "%s - update of non-default data for <%s> not (yet) supported",
                        __FUNCTION__, pCurrentResourceID) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            if( (! bSkipCurrentResource) && (pas_cfg_configType_delete == eConfigTypeForResource))
            {
                if(pas_conf_deleteNonDefaultDataForResource(pCurrentResourceID, psInstallFolderCtx, &sHandlers))
                {
                    iNoOfInstalledResources++ ;
                }
                else
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - pas_conf_deleteNonDefaultDataForResource(<%s>) ... %s",
                        __FUNCTION__, pCurrentResourceID, bEverythingOK ? "OK" : "FAILED") ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }

            iPosInListOfResources += ((sint_t)strlen(pCurrentResourceID) + 1) ; /* 1 <=> '\0' */
        }
    }

    return (bEverythingOK ? iNoOfInstalledResources : errorCode) ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief Open handlers to public/group/app's non-default data accordinf to pas_conf_handlersForNonDefault_s
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \param psHandlers_out      [out] pointer to the handlers structure
 * \return true for success, false other way
 */
static bool_t pas_conf_setupHandlersForNonDefault(pas_cfg_instFoldCtx_s* const psInstallFolderCtx, pas_conf_handlersForNonDefault_s* psHandlers_out)
{
    bool_t  bEverythingOK = true ;

    psHandlers_out->hDestRctDB          = psInstallFolderCtx->sDestContext.sRctDB.handler ;
    psHandlers_out->hDestCachedDataDB   = psInstallFolderCtx->sDestContext.sCachedDB.handler ;
    psHandlers_out->hDestWtDataDB       = psInstallFolderCtx->sDestContext.sWtDB.handler ;
    psHandlers_out->hSrcRctFile         = psInstallFolderCtx->sSrcContext.sRctFile.handler ;
    psHandlers_out->hSrcExceptions      = psInstallFolderCtx->sSrcContext.sExceptionsFile.handler ;

    return bEverythingOK ;
}


/**
 * \brief Find out the type of configuration to by performed on the resource
 * \param resourceID          [in] resource's identifier
 * \param eRule               [in] generic(folder level) install rule
 * \param hExceptions         [in] handler to the install-exceptions file
 * \param peConfigType_out    [out]where the type of configuration is returned
 * \return true for success, false other way
 */
static bool_t pas_conf_getConfigurationNeededForResource( constpstr_t                 resourceID, 
                                                          PersAdminCfgInstallRules_e  eRule,
                                                          sint_t                      hExceptions,
                                                          pas_cfg_configTypes_e*      peConfigType_out )
{
    bool_t                          bEverythingOK = true ;
    bool_t                          bExceptionSpecifiedForResource = false ;
    PersAdminCfgInstallExceptions_e eExceptionForResource ;

    if(hExceptions >= 0)
    {
        sint_t iErrLocal = persAdmCfgExcGetExceptionForResource(hExceptions, resourceID, &eExceptionForResource) ;
        if(iErrLocal >= 0)
        {
            bExceptionSpecifiedForResource = true ;           
        }
        else
        {
            if(PAS_FAILURE_NOT_FOUND != iErrLocal)
            {
                bEverythingOK = false ;
                (void)snprintf(g_msg, sizeof(g_msg), "%s unexpected error =<%d>(0x%X) !!!",
                            __FUNCTION__, iErrLocal, iErrLocal) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }            
    }

    if(bEverythingOK && bExceptionSpecifiedForResource)
    {
        /* check if the exception is valid in corespondence to install rule */
        if( ! g_LookUpRules[eRule].exceptionsLookUp[eExceptionForResource])
        {
            /* invalid install exception */
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s unexpected exception <%d> for install rule <%d> !!!",
                        __FUNCTION__, eRule, eExceptionForResource) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        if(bExceptionSpecifiedForResource)
        {
            switch(eExceptionForResource)
            {
                case PersAdminCfgInstallExceptions_Update:
                {
                    *peConfigType_out = pas_cfg_configType_update;
                    break ;
                }
                case PersAdminCfgInstallExceptions_Delete:
                {
                    *peConfigType_out = pas_cfg_configType_delete;
                    break ;
                }
                default: /* i.e. PersAdminCfgInstallExceptions_DontTouch */
                {
                    *peConfigType_out = pas_cfg_configType_dontTouch;
                    break ;
                }
            }
        }
        else
        {
            *peConfigType_out = g_LookUpRules[eRule].eDefaultConfigType ;            
        }
    }
    
    return bEverythingOK ;
}


/**
 * \brief Update default data (factory or configurable) for one resource
 * \param resourceID          [in] resource's identifier
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \param psHandlers          [in] structure with handlers to relevan files
 * \param bForFactoryDefault  [in] select data to be updated (factory-default for true, configurable-default for false)
 * \return true for success, false other way
 */
static bool_t pas_conf_updateDefaultDataForResource(constpstr_t                    resourceID, 
                                                    pas_cfg_instFoldCtx_s* const   psInstallFolderCtx, 
                                                    pas_conf_handlersForDefault_s* psHandlers,
                                                    bool_t                         bForFactoryDefault)
{
    bool_t                        bEverythingOK = true ;
    PersistenceConfigurationKey_s sResourceCfgSrc ;
    int                           iErrLocal ;

    iErrLocal = persAdmCfgRctRead(psHandlers->hSrcRctFile, resourceID, &sResourceCfgSrc) ;
    if(iErrLocal >= 0)
    {
        if(PersistenceResourceType_key == sResourceCfgSrc.type) /* supported also for custom plugins */
        {
            PersAdminCfgKeyDataEntry_s  sDefaultDataEntry ;
            sint_t                      iDefaultDataSize = 0 ;
        
            /* check if there is default data for the key */
            iDefaultDataSize = persAdmCfgReadDefaultData(psHandlers->hSrcDefaultDataFile, resourceID, sDefaultDataEntry.data, sizeof(sDefaultDataEntry.data)) ;
            if(iDefaultDataSize > 0)
            {
                /* (over)write default data */
                sint_t iDbErr = persComDbWriteKey(psHandlers->hDestDefaultDataDB, resourceID, sDefaultDataEntry.data, iDefaultDataSize) ;
                if(iDbErr < 0)
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - persComDbWriteKey(%s) returned %d",
                        __FUNCTION__, resourceID, iDefaultDataSize) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }

        if(    (sResourceCfgSrc.storage < PersistenceStorage_custom ) /* supported only for local/shared storage */
            && (PersistenceResourceType_file == sResourceCfgSrc.type))
        {
            str_t srcDefaultFilePathname[PERSADMIN_MAX_PATH_LENGHT] ;
            str_t destDefaultFilePathname[PERSADMIN_MAX_PATH_LENGHT] ;

            /* set paths */
            (void)snprintf(srcDefaultFilePathname, sizeof(srcDefaultFilePathname), "%s/%s/%s",
                    psInstallFolderCtx->sSrcContext.sFiledataFolder.pathname,
                    bForFactoryDefault ? PERS_ORG_DEFAULT_DATA_FOLDER_NAME : PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME,
                    resourceID) ;
            (void)snprintf(destDefaultFilePathname, sizeof(destDefaultFilePathname), "%s/%s/%s",
                    psInstallFolderCtx->sDestContext.sInstallFolder.pathname,
                    bForFactoryDefault ? PERS_ORG_DEFAULT_DATA_FOLDER_NAME : PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME,
                    resourceID) ;
            if(0 <= persadmin_check_if_file_exists(srcDefaultFilePathname, false))
            {
                /* default data available for the resource */
                sint_t iErrCopy = persadmin_copy_file(srcDefaultFilePathname, destDefaultFilePathname) ;
                if(iErrCopy < 0)
                {
                    bEverythingOK = false ;
                }

               (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_copy_file(<%s>, <%s>) returned %d", 
                        __FUNCTION__, srcDefaultFilePathname, destDefaultFilePathname, iErrCopy) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
               (void)snprintf(g_msg, sizeof(g_msg), "%s - no default data (<%s>) ", 
                        __FUNCTION__, srcDefaultFilePathname) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));                            
            }
        }
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/


/**
 * \brief Remove default data (factory or configurable) for one resource
 * \param resourceID          [in] resource's identifier
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \param psHandlers          [in] structure with handlers to relevan files
 * \param bForFactoryDefault  [in] select data to be removed (factory-default for true, configurable-default for false)
 * \return true for success, false other way
 */
static bool_t pas_conf_deleteDefaultDataForResource(constpstr_t                    resourceID, 
                                                    pas_cfg_instFoldCtx_s* const   psInstallFolderCtx, 
                                                    pas_conf_handlersForDefault_s* psHandlers,
                                                    bool_t                         bForFactoryDefault)
{
    bool_t                          bEverythingOK = true ;
    PersistenceConfigurationKey_s   sResourceCfgSrc ;
    int                             iErrLocal ;

    iErrLocal = persAdmCfgRctRead(psHandlers->hSrcRctFile, resourceID, &sResourceCfgSrc) ;
    if(iErrLocal >= 0)
    {
        if(    (sResourceCfgSrc.storage < PersistenceStorage_custom ) /* supported only for local/shared storage */
            && (PersistenceResourceType_key == sResourceCfgSrc.type))
        {
            sint_t                      iDefaultDataSize = 0 ;
        
            /* check if there is default data for the key */
            iDefaultDataSize = persComDbGetKeySize(psHandlers->hDestDefaultDataDB, resourceID) ;
            if(iDefaultDataSize > 0)
            {
                /* delete default data */
                sint_t iDbErr = persComDbDeleteKey(psHandlers->hDestDefaultDataDB, resourceID) ;
                if( (iDbErr < 0) && (PERS_COM_ERR_NOT_FOUND != iDbErr))
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - persComDbDeleteKey(%s) ... %s",
                        __FUNCTION__, resourceID, bEverythingOK ? "OK" : "FAILED") ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
                (void)snprintf(g_msg, sizeof(g_msg), "%s - no default data to be deleted for <%s>",
                        __FUNCTION__, resourceID) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }

        if(    (sResourceCfgSrc.storage < PersistenceStorage_custom ) /* supported only for local/shared storage */
            && (PersistenceResourceType_file == sResourceCfgSrc.type))
        {
            str_t defaultFilePathname[PERSADMIN_MAX_PATH_LENGHT] ;
            
            (void)snprintf(defaultFilePathname, sizeof(defaultFilePathname), "%s/%s/%s",
                    psInstallFolderCtx->sDestContext.sInstallFolder.pathname,
                    bForFactoryDefault ? PERS_ORG_DEFAULT_DATA_FOLDER_NAME : PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME,
                    resourceID) ;
            if(0 <= persadmin_check_if_file_exists(defaultFilePathname, false))
            {
                /* default data available for the resource => delete it */
                sint_t iErrCopy = persadmin_delete_file(defaultFilePathname) ;
                if(iErrCopy < 0)
                {
                    bEverythingOK = false ;
                }

               (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_delete_file(<%s>) returned %d", 
                        __FUNCTION__, defaultFilePathname, iErrCopy) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
               (void)snprintf(g_msg, sizeof(g_msg), "%s - no default data file (<%s>) to be deleted", 
                        __FUNCTION__, defaultFilePathname) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));                            
            }
        }
    }
    else
    {
        /* should never happen */
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRctRead(%s) failed err=%d",
                __FUNCTION__, resourceID, iErrLocal) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief Remove non-default data (initial or run time) for one resource
 * \param resourceID          [in] resource's identifier
 * \param psInstallFolderCtx  [in] pointer to the public/group/app's context
 * \param psHandlers          [in] structure with handlers to relevan files
 * \return true for success, false other way
 */
static bool_t pas_conf_deleteNonDefaultDataForResource( constpstr_t                         resourceID, 
                                                        pas_cfg_instFoldCtx_s* const        psInstallFolderCtx, 
                                                        pas_conf_handlersForNonDefault_s*   psHandlers)
{
    bool_t                          bEverythingOK = true ;
    PersistenceConfigurationKey_s   sResourceCfgSrc ;
    int                             iErrLocal ;

    iErrLocal = persAdmCfgRctRead(psHandlers->hSrcRctFile, resourceID, &sResourceCfgSrc) ;
    if(iErrLocal >= 0)
    {
        if(    (sResourceCfgSrc.storage < PersistenceStorage_custom ) /* supported only for local/shared storage */
            && (PersistenceResourceType_key == sResourceCfgSrc.type))
        {
            sint_t hNonDefaultDB = (PersistencePolicy_wc == sResourceCfgSrc.policy) ? 
                                    psHandlers->hDestCachedDataDB : psHandlers->hDestWtDataDB ;
            
            bEverythingOK = pas_conf_deleteNonDefaultDataForKeyTypeResource(resourceID, hNonDefaultDB) ;
        }

        if(    (sResourceCfgSrc.storage < PersistenceStorage_custom ) /* supported only for local/shared storage */
            && (PersistenceResourceType_file == sResourceCfgSrc.type))
        {
            bEverythingOK = pas_conf_deleteNonDefaultDataForFileTypeResource(resourceID, psInstallFolderCtx->sDestContext.sInstallFolder.pathname) ;
        }
    }
    else
    {
        /* should never happen */
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persAdmCfgRctRead(%s) failed err=%d",
                __FUNCTION__, resourceID, iErrLocal) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    return bEverythingOK ;
}    

/**
 * \brief Remove the resource's entries from non-default DB
 * \param resourceID          [in] resource's identifier
 * \param hNonDefaultDB       [in] Handler to non-default DB
 * \return true for success, false other way
 *
 * \note The caller must provide hNonDefaultDB according to resource's policy
 */
static bool_t pas_conf_deleteNonDefaultDataForKeyTypeResource(constpstr_t resourceID, sint_t hNonDefaultDB)
{
    bool_t bEverythingOK = true ;
    sint_t iSizeUnfilteredList = 0 ;
    sint_t iSizeFilteredList = 0 ;
    pstr_t pUnfilteredList = NIL ;
    pstr_t pFilteredList = NIL ;
    bool_t bNothingToDeleteFromDB = true ;

    /* get a list of all the keys in psAppDB->pathname related to our resourceID and delete them */
    iSizeUnfilteredList = persComDbGetSizeKeysList(hNonDefaultDB) ;
    
    (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbGetSizeKeysList() returned <%d>",
                __FUNCTION__, iSizeUnfilteredList) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    if(iSizeUnfilteredList > 0)
    {
        bNothingToDeleteFromDB = false ;
        pUnfilteredList = (pstr_t)malloc((size_t)iSizeUnfilteredList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        if(NIL != pUnfilteredList)
        {
            pFilteredList = (pstr_t)malloc((size_t)iSizeUnfilteredList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
            if(NIL == pFilteredList)
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }
    else
    {
        if(iSizeUnfilteredList < 0)
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbGetSizeKeysList() FAILED err=<%d>",
                __FUNCTION__, iSizeUnfilteredList) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK && (! bNothingToDeleteFromDB))
    {
        sint_t iListingSize = persComDbGetKeysList(hNonDefaultDB, pUnfilteredList, iSizeUnfilteredList) ;
        if(iListingSize == iSizeUnfilteredList)
        {
            /* there are keys in the unfiltered list, so let's filter the list upon our key */

            /* initialize this way to please SCC */
            pas_conf_listOfItems_s sUnfilteredList = {0} ;
            sUnfilteredList.pList = pUnfilteredList ;
            sUnfilteredList.sizeOfList = iSizeUnfilteredList ;
            
            iSizeFilteredList = pas_conf_filterResourcesList(resourceID, false, true, &sUnfilteredList, pFilteredList) ;
        }
        else
        {
            /* very strange */
            bEverythingOK = false ;
        }
    }

    if(bEverythingOK && (! bNothingToDeleteFromDB) && (iSizeFilteredList > 0))
    {
        /* there are keys in the filtered list, so delete all of them from psDestDB->pathname */
        sint_t posInList = 0 ;
        pstr_t pCurrentKey = NIL ;
        sint_t iResult ;

        while((posInList < iSizeFilteredList) && bEverythingOK)
        {                 
            pCurrentKey = pFilteredList + posInList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            iResult = persComDbDeleteKey(hNonDefaultDB, pCurrentKey) ;
            
            (void)snprintf(g_msg, sizeof(g_msg), "%s persComDbDeleteKey(<%s>)returned <%d>",
                        __FUNCTION__, pCurrentKey, iResult) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            
            if(iResult < 0)
            {
                bEverythingOK = false ;
            }

            posInList += ((sint_t)strlen(pCurrentKey) + 1) ; /* 1 <=>  '\0' */
        }
    }
    
    if(pUnfilteredList)
    {
        free(pUnfilteredList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }
    if(pFilteredList)
    {
        free(pFilteredList) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief Remove the resource's non-default data files
 * \param resourceID            [in] resource's identifier
 * \param installFolderPathname [in] public/group/app's install folder pathname
 * \return true for success, false other way
 *
 * \note The caller must provide hNonDefaultDB according to resource's policy
 */
static bool_t pas_conf_deleteNonDefaultDataForFileTypeResource(constpstr_t resourceID, constpstr_t installFolderPathname)
{
    bool_t  bEverythingOK = true ;
    str_t   filePathname[PERSADMIN_MAX_PATH_LENGHT] ;
    sint_t  iSizeOfListing = 0 ;
    pstr_t  pListingBuffer = NIL ;
    pstr_t  pListingBufferFiltered = NIL ;



    /* get a list of all the files in the install folder */
    iSizeOfListing = persadmin_list_folder_get_size(installFolderPathname, PersadminFilterFilesRegular, true) ;

    (void)snprintf(g_msg, sizeof(g_msg), "%s: persadmin_list_folder_get_size(%s) returned <%d>",
                __FUNCTION__, installFolderPathname, iSizeOfListing) ;
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    if(iSizeOfListing > 0)
    {
        pListingBuffer = (pstr_t)malloc((size_t)iSizeOfListing) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        pListingBufferFiltered = (pstr_t)malloc((size_t)iSizeOfListing) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        if((NIL != pListingBuffer) && (NIL != pListingBufferFiltered))
        {
            if(iSizeOfListing != persadmin_list_folder(installFolderPathname, pListingBuffer, iSizeOfListing, PersadminFilterFilesRegular, true))
            {
                /* very strange */
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }


    if(bEverythingOK && (iSizeOfListing > 0))
    {
        /* we have the list 
         * now we have to delete all the files with the same name as resourceID
        */
        sint_t iSizeOfListFiltered = 0 ;
        /* initialize this way to please SCC */
        pas_conf_listOfItems_s sUnfilteredList = {0} ;
        sUnfilteredList.pList = pListingBuffer ;
        sUnfilteredList.sizeOfList = iSizeOfListing ;

        iSizeOfListFiltered = pas_conf_filterResourcesList(resourceID, true, false, &sUnfilteredList, pListingBufferFiltered) ;

        if(iSizeOfListFiltered > 0)
        {
            sint_t iPosInBuffer = 0 ;
            pstr_t pCurrentFilePathname = NIL ;
            while((iPosInBuffer < iSizeOfListFiltered) && bEverythingOK)
            {
                sint_t iDeleteResult ;
                pCurrentFilePathname = pListingBufferFiltered + iPosInBuffer ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/

                /* delete the file */
                
                (void)snprintf(g_msg, sizeof(g_msg), "%s: resID=<%s> filename=<%s>", __FUNCTION__, resourceID, pCurrentFilePathname) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                
                (void)snprintf(filePathname, sizeof(filePathname), "%s/%s", installFolderPathname, pCurrentFilePathname) ;
                iDeleteResult = persadmin_delete_file(filePathname) ;
                if(iDeleteResult < 0)
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s persadmin_delete_file(<%s>) returned <%d>", __FUNCTION__, filePathname, iDeleteResult) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

                iPosInBuffer += ((sint_t)strlen(pCurrentFilePathname) + 1) ; /* 1 <=>  PERSADMIN_ITEMS_IN_LIST_SEPARATOR */
            }
        }
        else
        {
            if(iSizeOfListFiltered < 0)
            {
                bEverythingOK = false ;
            }
        }
    }

    if(NIL != pListingBuffer)
    {
        free(pListingBuffer) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }   
    if(NIL != pListingBufferFiltered)
    {
        free(pListingBufferFiltered) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    return bEverythingOK ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief Uncompress into the default folder the archive indicated by resourcePathname
 * \param resourcePathname    [in] absolute pathname for the input archive
 * \return true for success, false otherway
 */
static bool_t pas_conf_unarchiveResourceFile(constpstr_t resourcePathname)
{
    bool_t bEverythingOK = true ;

    if(0 <= persadmin_check_if_file_exists(resourcePathname, false))
    {
        /* create the destination folder (if not already available) */
        if(0 <= persadmin_create_folder(PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME))
        {
            /* clean the folder's content (un-archive does not do this by default) */
            if( ! pas_conf_deleteFolderContent(PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME, PersadminFilterAll))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_create_folder(<%s>) FAILED", 
                __FUNCTION__, resourcePathname) ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }
    else
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "%s - archive <%s> not found", 
            __FUNCTION__, resourcePathname) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));        
    }

    if(bEverythingOK)
    {
        /* un-compress resourcePathname into default  folder */
        sint_t iErrCodeLocal = persadmin_uncompress(resourcePathname, PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME) ;
        if(iErrCodeLocal < 0)
        {
            bEverythingOK = false ;
        }
        (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_uncompress(<%s>, <%s>) returned <%d>", 
            __FUNCTION__, resourcePathname, PERSADM_CFG_UNCOMPRESSED_RESOURCE_FOLDER_PATHNAME, iErrCodeLocal) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));

    }

    return bEverythingOK;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

/**
 * \brief Delete a folder's content according to filter
 * \param folderPathName    [in] absolute pathname to the folder
 * \param eFilterMode       [in] specify which kind of files/folders to be deleted
 * \return true for success, false otherway
 */
static bool_t pas_conf_deleteFolderContent(pstr_t folderPathName, PersadminFilterMode_e eFilterMode)
{
    bool_t  bEverythingOK = true ;
    pstr_t  pListingBuffer = NIL ;
    sint_t  iListingSize = 0 ;

    iListingSize = persadmin_list_folder_get_size(folderPathName, eFilterMode, false);
    if(iListingSize > 0)
    {
        pListingBuffer = (pstr_t)malloc((size_t)iListingSize); /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
        if(NIL == pListingBuffer)
        {
            bEverythingOK = false ;
        }
    }
    else
    {
        if(iListingSize < 0)
        {
            bEverythingOK = false ;
        }
    }    

    if(bEverythingOK && (iListingSize > 0))
    {
        sint_t iListingSizeLocal = persadmin_list_folder(folderPathName, pListingBuffer, iListingSize, eFilterMode, false) ;
        if(iListingSizeLocal != iListingSize)
        {
            bEverythingOK = false ;
        }
    }

    if(bEverythingOK && (iListingSize > 0))
    {
        /* the list of files and folders available in pListingBuffer */
        sint_t  iPosInList = 0 ;
        pstr_t  pCurrentFileOrFolder = NIL ;
        str_t   pathname[PERSADMIN_MAX_PATH_LENGHT] ;
        while(bEverythingOK && (iPosInList < iListingSize))
        {
            pCurrentFileOrFolder = pListingBuffer + iPosInList ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            (void)snprintf(pathname, sizeof(pathname), "%s/%s", folderPathName, pCurrentFileOrFolder) ;

            if(0 <= persadmin_check_if_file_exists(pathname, true))
            {
                /* it is a folder => delete it */
                sint_t iDeletedSize = persadmin_delete_folder(pathname) ;
                if(iDeletedSize < 0)
                {
                    bEverythingOK = false ;
                }
                (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_delete_folder(%s) returned <%d>", 
                    __FUNCTION__, pathname, iDeletedSize) ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
            else
            {
                if(0 <= persadmin_check_if_file_exists(pathname, false))
                {
                    /* it is a file */
                    sint_t iDeletedSize = persadmin_delete_file(pathname) ;
                    if(iDeletedSize < 0)
                    {
                        bEverythingOK = false ;
                    }
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - persadmin_delete_file(%s) returned <%d>", 
                        __FUNCTION__, pathname, iDeletedSize) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
                else
                {
                    bEverythingOK = false ;
                    (void)snprintf(g_msg, sizeof(g_msg), "%s - invalid path <%s>", 
                        __FUNCTION__, pathname) ;
                    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                }
            }

            iPosInList += ((sint_t)strlen(pCurrentFileOrFolder) + 1) ; /* 1 <=> '\0' */
        }
    }

    if(NIL != pListingBuffer)
    {
        free(pListingBuffer) ; /*DG C8MR2R-MISRA-C:2004 Rule 20.4-SSW_Administrator_0002*/
    }

    return bEverythingOK;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/ /*DG C8ISQP-ISQP Metric 11-SSW_Administrator_0034*/

/**
 * \brief From a list with items for all resources, extract only the items related to one resourceID
 * \param resourceID            [in] resource's identifier
 * \param bFilterOnlyNonDefault [in] if true, default entries are not filetered in
 * \param bIsKeyType            [in] resource's type (key for true, file otherway)
 * \param psListUnfiltered      [in] structure(buffer + size) with the items for all the resources
 * \param pListFiltered_out     [in] buffer where to return the list of items related to resourceID
 * \return the actual list size in pListFiltered_out, or negative value in case of error
 */
#define PREFIX_MAX_SIZE 32  /* should be enough for "/user/2/seat/1/" or "configurableDefaultData/" like strings */
static sint_t pas_conf_filterResourcesList( pstr_t                          resourceID,
                                            bool_t                          bFilterOnlyNonDefault,
                                            bool_t                          bIsKeyType, 
                                            pas_conf_listOfItems_s* const   psListUnfiltered, 
                                            pstr_t                          pListFiltered_out)
{
    bool_t              bEverythingOK = true ;
    sint_t              iFilteredBufSize = -1 ;
    str_t               searchString[PERSADMIN_MAX_PATH_LENGHT] = {0} ;
    sint_t              iPosInUnfilteredBuf = 0 ;
    sint_t              iPosInFilteredBuf = 0 ;
                        /* (PERSADMIN_NO_OF_SEATS+1) <=> PERSADMIN_NO_OF_SEATS for "/user/user_no/seat/seat_no/", 1 for "/user/user_no/"  */
                        /* +1][PREFIX_MAX_SIZE] <=> + "node"  */
    static str_t        prefixesKey[((PERSADMIN_NO_OF_USERS)*(PERSADMIN_NO_OF_SEATS+1))+1][PREFIX_MAX_SIZE] ;
                        /* +1+2][PREFIX_MAX_SIZE] <=> + "node", "defaultData", "configurableDefaultData"  */
    static str_t        prefixesFile[((PERSADMIN_NO_OF_USERS)*(PERSADMIN_NO_OF_SEATS+1))+1+2][PREFIX_MAX_SIZE] ;
    static sint_t       iNoOfPrefixesKey = 0;
    static sint_t       iNoOfPrefixesFile = 0;
    static bool_t       bFirstCall = true ;
    sint_t              i, j ; /* indexes to be used in loops */

    if(bFirstCall)
    {
       /**************************************************
        *   The idea is to obtain something like this:
        *
        *   str_t prefixesKey[][PREFIX_MAX_SIZE] = 
        *   {
        *       "/user/1/seat/1/",
        *       "/user/1/seat/2/",
        *       ................
        *       "/user/4/seat/4/",
        *       "/user/1/",
        *       .........
        *       "/user/4/",
        *       "/node"
        *   } ;
        *
        *   str_t prefixesFile[][PREFIX_MAX_SIZE] = 
        *   {
        *       "user/1/seat/1/",
        *       "user/1/seat/2/",
        *       ................
        *       "user/4/seat/4/",
        *       "user/1/",
        *       .........
        *       "user/4/",
        *       "node",
        *       "defaultData",
        *       "configurableDefaultData"
        *   } ;
        ****************************************************/
       
        bFirstCall = false ; /* do it only once */
       
        for(i = PERSADMIN_MIN_USER_NO ; i <= PERSADMIN_MAX_USER_NO ; i++)
        {
            for(j = PERSADMIN_MIN_SEAT_NO ; j <= PERSADMIN_MAX_SEAT_NO ; j++)
            {
                (void)snprintf(&prefixesKey[(i-PERSADMIN_MIN_USER_NO)*PERSADMIN_NO_OF_USERS + (j - PERSADMIN_MIN_SEAT_NO)][0], PREFIX_MAX_SIZE, "%s/%d/%s/%d/", 
                    ("/"PERS_ORG_USER_FOLDER_NAME), i, PERS_ORG_SEAT_FOLDER_NAME, j) ;
                (void)snprintf(&prefixesFile[(i-PERSADMIN_MIN_USER_NO)*PERSADMIN_NO_OF_USERS + (j - PERSADMIN_MIN_SEAT_NO)][0], PREFIX_MAX_SIZE, "%s/%d/%s/%d/", 
                    (PERS_ORG_USER_FOLDER_NAME), i, PERS_ORG_SEAT_FOLDER_NAME, j) ;
            }
        }
        for(i = PERSADMIN_MIN_USER_NO ; i <= PERSADMIN_MAX_USER_NO ; i++)
        {
            (void)snprintf(&prefixesKey[(PERSADMIN_NO_OF_USERS*PERSADMIN_NO_OF_SEATS-1)+i][0], PREFIX_MAX_SIZE, "%s/%d/", 
                        ("/"PERS_ORG_USER_FOLDER_NAME) , i) ;
            (void)snprintf(&prefixesFile[(PERSADMIN_NO_OF_USERS*PERSADMIN_NO_OF_SEATS-1)+i][0], PREFIX_MAX_SIZE, "%s/%d/", 
                        PERS_ORG_USER_FOLDER_NAME , i) ;
        }
        (void)snprintf(&prefixesKey[PERSADMIN_NO_OF_USERS*(PERSADMIN_NO_OF_SEATS+1)][0], PREFIX_MAX_SIZE, "%s/", 
                    PERS_ORG_NODE_FOLDER_NAME_) ;
        (void)snprintf(&prefixesFile[PERSADMIN_NO_OF_USERS*(PERSADMIN_NO_OF_SEATS+1)][0], PREFIX_MAX_SIZE, "%s/", 
                    PERS_ORG_NODE_FOLDER_NAME) ;
        
        /* prefixes relevant only for file type resources */
        (void)snprintf(&prefixesFile[PERSADMIN_NO_OF_USERS*(PERSADMIN_NO_OF_SEATS+1)+1][0], PREFIX_MAX_SIZE, "%s/", PERS_ORG_DEFAULT_DATA_FOLDER_NAME) ;
        (void)snprintf(&prefixesFile[PERSADMIN_NO_OF_USERS*(PERSADMIN_NO_OF_SEATS+1)+2][0], PREFIX_MAX_SIZE, "%s/", PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME) ;

        iNoOfPrefixesKey = sizeof(prefixesKey)/sizeof(prefixesKey[0]) ;
        iNoOfPrefixesFile = sizeof(prefixesFile)/sizeof(prefixesFile[0]) ; 
    }

    while((iPosInUnfilteredBuf < psListUnfiltered->sizeOfList) && bEverythingOK)
    {
        pstr_t pFilenameToBeFiltered = psListUnfiltered->pList + iPosInUnfilteredBuf ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
        sint_t iLengthFilenameToBeFiltered = (sint_t)strlen(pFilenameToBeFiltered) ;
        bool_t bFilenameFilteredIn = false ;
        pstr_t pFoundString = NIL ;
        
        (void)snprintf(g_msg, sizeof(g_msg), "%s pos=%d len=%d <%s>", 
                    __FUNCTION__, iPosInUnfilteredBuf, iLengthFilenameToBeFiltered, pFilenameToBeFiltered)  ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        
        /* scan pFilenameToBeFiltered to see if a sequence matches searchString  */
        pFoundString = strstr(pFilenameToBeFiltered, resourceID) ;
        if(NIL != pFoundString)
        {
            sint_t iNoOfPrefixesToCheck = bIsKeyType ? iNoOfPrefixesKey : iNoOfPrefixesFile ;
            if( (! bIsKeyType) && bFilterOnlyNonDefault)
            {
                /* don't filter in default data */
                iNoOfPrefixesToCheck = iNoOfPrefixesFile - 2 ; /* 2 <=> defaultData and configurableDefaultData */
            }
            
            /* sequence found, make sure that the path is valid and avoid collision with other files with names close to this one */
            for(i = 0 ; i < iNoOfPrefixesToCheck ; i++)
            {
                (void)snprintf(searchString, sizeof(searchString), "%s%s", bIsKeyType ? &prefixesKey[i][0] : &prefixesFile[i][0], resourceID) ;

                (void)snprintf(g_msg, sizeof(g_msg), "%s <%s> vs <%s> vs <%s>", 
                            __FUNCTION__, searchString, resourceID, pFilenameToBeFiltered)  ;
                DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                
                if(0 == strcmp(searchString, pFilenameToBeFiltered))
                {
                    bFilenameFilteredIn = true ;
                    break ;
                }
            }
        }

        if(bFilenameFilteredIn)
        {
            (void)strcpy(pListFiltered_out+iPosInFilteredBuf, pFilenameToBeFiltered) ; /*DG C8MR2R-MISRA-C:2004 Rule 17.4-SSW_Administrator_0006*/
            iPosInFilteredBuf += (iLengthFilenameToBeFiltered + 1) ; /* 1 <=>  '\0' */
            (void)snprintf(g_msg, sizeof(g_msg), "%s Filtered in <%s> <%s> size=%d pos=%d", 
                    __FUNCTION__, pFilenameToBeFiltered, resourceID, strlen(pFilenameToBeFiltered), iPosInFilteredBuf)  ;
            DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }

        iPosInUnfilteredBuf += (iLengthFilenameToBeFiltered + 1) ; /* 1 <=>  '\0' */
    }

    if(bEverythingOK)
    {
        iFilteredBufSize = iPosInFilteredBuf;
    }

    return iFilteredBufSize ;
}/*DG C8ISQP-ISQP Metric 10-SSW_Administrator_0001*/

