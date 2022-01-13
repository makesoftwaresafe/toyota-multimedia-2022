/*********************************************************************************************************************
*
* Copyright (C) 2012 Continental Automotive Systems, Inc.
*
* Author: Ionut.Ieremie@continental-corporation.com
*
* Implementation of backup process
*
* This Source Code Form is subject to the terms of the Mozilla Public
* License, v. 2.0. If a copy of the MPL was not distributed with this
* file, You can obtain one at http://mozilla.org/MPL/2.0/.
*
* Date       Author                   Reason
  2015.10.20 Cosmin Cernat            Fixed buffer overflow issue.
                                      Extension of the function persadmin_serialize_data() call with handover of the buffer with its size
  2013.04.15 Petrica Manoila          CSP_WZ#3424:  Add IF extension for "restore to default"
  2013.01.24 Petrica Manoila          CSP_WZ#2246:  Added additional test cases for persAdminDataBackupRecovery
  2012.12.11 Petrica Manoila          CSP_WZ#1280:  Added test cases for persAdminDataBackupRecovery
  2012.11.23 Ana Chisca, Alin Liteanu CSP_WZ#1280:  Added test cases for persAdminDataBackupCreate & persAdminUserDataDelete
  2012.11.21 Ionut Ieremie            CSP_WZ#1280:  Created (only framework and a dummy test case)
*
**********************************************************************************************************************/

/* ---------------------- include files  --------------------------------- */
#include "persComTypes.h"
#include "stdio.h"
#include "string.h"
#include "malloc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include <dlt/dlt.h>
#include <dlt/dlt_user.h>

#include "persComErrors.h"
#include "persComDataOrg.h"
#include "persComDbAccess.h"
#include "persComRct.h"

#include "ssw_pers_admin_files_helper.h"
#include "ssw_pers_admin_dbus.h"

#include "persistence_admin_service.h"

#include "test_PAS.h"


#include "test_pas_check_data_after_reset.h"
#include "test_pas_data_backup_recovery.h"
#include "test_pas_data_restore_default.h"
#include "test_pas_resource_config_add.h"
#include "test_pas_data_import.h"
#include "test_pas_check_data_after_backup_application.h"
#include "test_pas_check_data_after_delete_user2_data.h"
#include "test_pas_check_data_after_backup_create_all.h"
#include "test_pas_check_data_after_backup_user_all.h"
#include "test_pas_check_data_after_backup_user2_seat_all.h"

/* L&T context */
#define LT_HDR                          "TEST_PAS >> "

//static   DLT_DECLARE_CONTEXT(testPersAdminDLTCtx);
DLT_DECLARE_CONTEXT(persAdminSvcDLTCtx) ;
#define testPersAdminDLTCtx persAdminSvcDLTCtx
str_t g_msg[512] ;

#define File_t PersistenceResourceType_file
#define Key_t PersistenceResourceType_key


static entryRctInit_s RCT_public_init[] =
{
    {"pubSettingA",                true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSettingB",                true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSettingC",                true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSetting/ABC",             true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSettingD",                true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSettingE",                true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSettingF",                true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"pubSetting/DEF",             true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"},      {NIL},{NIL}}}
} ;

static entryDataInit_s dataKeysPublicInit[] =
{
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                      PersistencePolicy_wt, 0, 0, "Data>>/pubSettingA"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",        PersistencePolicy_wt, 2, 1, "Data>>/pubSettingB::user2::seat1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",        PersistencePolicy_wt, 2, 2, "Data>>/pubSettingB::user2:seat2"},
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                                      PersistencePolicy_wt, 0, 0, "Data>>/pubSettingC"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",                                  PersistencePolicy_wt, 1, 0, "Data>>/pubSetting/ABC::user1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",                                  PersistencePolicy_wt, 2, 0, "Data>>/pubSetting/ABC::user2"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",                                  PersistencePolicy_wt, 3, 0, "Data>>/pubSetting/ABC::user3"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",                                  PersistencePolicy_wt, 4, 0, "Data>>/pubSetting/ABC::user4"},
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                      PersistencePolicy_wc, 0, 0, "Data>>/pubSettingD"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",        PersistencePolicy_wc, 2, 1, "Data>>/pubSettingE::user2:seat1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",        PersistencePolicy_wc, 2, 2, "Data>>/pubSettingE::user2:seat2"},
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                      PersistencePolicy_wc, 0, 0, "Data>>/pubSettingF"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                                  PersistencePolicy_wc, 1, 0, "Data>>/pubSetting/DEF::user1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                                  PersistencePolicy_wc, 2, 0, "Data>>/pubSetting/DEF::user2"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                                  PersistencePolicy_wc, 3, 0, "Data>>/pubSetting/DEF::user3"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                                  PersistencePolicy_wc, 4, 0, "Data>>/pubSetting/DEF::user4"}
} ;


static entryDataInit_s dataFilesPublicInit[] =
{
    {0,  PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/doc1.txt",                                          PersistencePolicy_wt, 0, 0, "File>>/doc1.txt"},
    {0,  PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/Docs/doc2.txt",                                     PersistencePolicy_wt, 0, 0, "File>>/Docs/doc2.txt"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/1/docA.txt",                                     PersistencePolicy_wc, 1, 0, "File>>/docA.txt::user1"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2/docA.txt",                                     PersistencePolicy_wc, 2, 0, "File>>/docA.txt::user2"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/3/docA.txt",                                     PersistencePolicy_wc, 3, 0, "File>>/docA.txt::user3"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/4/docA.txt",                                     PersistencePolicy_wc, 4, 0, "File>>/docA.txt::user4"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",   PersistencePolicy_wc, 2, 1, "File>>/docB.txt::user2:seat1"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",   PersistencePolicy_wc, 2, 2, "File>>/docB.txt::user2:seat2"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",   PersistencePolicy_wc, 2, 3, "File>>/docB.txt::user2:seat3"},
    {0,  PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",   PersistencePolicy_wc, 2, 4, "File>>/docB.txt::user2:seat4"}
};

static defaultDataInit_s factoryDefaultInitKey[] =
{
    {"pubSettingA",     "FactoryDefault : pubSettingA : orig"       },
    {"pubSettingD",     "FactoryDefault : pubSettingD : orig"       },
    {"pubSetting/ABC",  "FactoryDefault : pubSetting/ABC : orig"    }    
};

static defaultDataInit_s configurableDefaultInitKey[] =
{
    {"pubSettingA",     "ConfigurableDefault : pubSettingA : orig"       },
    {"pubSetting/ABC",  "ConfigurableDefault : pubSetting/ABC : orig"    }    
};

static dataInit_s sSharedPubDataInit =
{
    PERS_ORG_SHARED_PUBLIC_WT_PATH_,
    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME,
    dbType_RCT,
    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,   
    RCT_public_init,
    sizeof(RCT_public_init)/sizeof(RCT_public_init[0]),
    dataKeysPublicInit,
    sizeof(dataKeysPublicInit)/sizeof(dataKeysPublicInit[0]),
    dataFilesPublicInit,
    sizeof(dataFilesPublicInit)/sizeof(dataFilesPublicInit[0]),

    "/Data/mnt-wt/shared/public"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,
    factoryDefaultInitKey,
    sizeof(factoryDefaultInitKey)/sizeof(factoryDefaultInitKey[0]),
    "/Data/mnt-wt/shared/public"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,
    configurableDefaultInitKey,
    sizeof(configurableDefaultInitKey)/sizeof(configurableDefaultInitKey[0])    
} ;


/**********************************************************************************************************************************************
 ***************************************** Group 10 *******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_group10_init[] =
{
    {"gr10_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_1.txt",                 false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"Docs/gr10_A.txt",            false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"gr10_2.txt",                 false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}},
    {"Docs/gr10_B.txt",            false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"},{NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_Group10_Init[] =
{
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingA",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr10_SettingA"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/gr10_SettingB::user2::seat1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/gr10_SettingB::user2:seat2"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingC",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr10_SettingC"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/ABC",                                     PersistencePolicy_wt, 1, 0, "Data>>/gr10_Setting/ABC::user1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/ABC",                                     PersistencePolicy_wt, 2, 0, "Data>>/gr10_Setting/ABC::user2"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/ABC",                                     PersistencePolicy_wt, 3, 0, "Data>>/gr10_Setting/ABC::user3"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/ABC",                                     PersistencePolicy_wt, 4, 0, "Data>>/gr10_Setting/ABC::user4"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr10_SettingD"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/gr10_SettingE::user2:seat1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/gr10_SettingE::user2:seat2"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr10_SettingF"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/gr10_Setting/DEF::user1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/gr10_Setting/DEF::user2"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/gr10_Setting/DEF::user3"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/gr10_Setting/DEF::user4"}
} ;

static defaultDataInit_s dataKeys_Group10_FactoryDefaultInit[] =
{
    {"gr10_SettingA",     "FactoryDefault : gr10_SettingA : orig"       },
    {"gr10_SettingB",     "FactoryDefault : gr10_SettingB : orig"       },
    {"gr10_SettingC",     "FactoryDefault : gr10_SettingC : orig"    },  
    {"gr10_Setting/ABC",  "FactoryDefault : gr10_Setting/ABC : orig"    }
};

static defaultDataInit_s dataKeys_Group10_ConfigurableDefaultInit[] =
{
    {"gr10_SettingA",     "ConfigurableDefault : gr10_SettingA : orig"       },
    {"gr10_SettingB",     "ConfigurableDefault : gr10_SettingB : orig"       },
    {"gr10_SettingC",     "ConfigurableDefault : gr10_SettingC : orig"    },  
    {"gr10_Setting/ABC",  "ConfigurableDefault : gr10_Setting/ABC : orig"    }
};

static entryDataInit_s dataFiles_Group10_Init[] =
{
    {0x10,  PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                           PersistencePolicy_wt, 0, 0, "File>>gr10_>>/gr10_1.txt"},
    {0x10,  PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_A.txt",                                      PersistencePolicy_wt, 0, 0, "File>>gr10_>>/Docs/gr10_A.txt"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/gr10_2.txt",                                        PersistencePolicy_wc, 1, 0, "File>>gr10_>>/gr10_2.txt::user1"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/gr10_2.txt",                                        PersistencePolicy_wc, 2, 0, "File>>gr10_>>/gr10_2.txt::user2"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/gr10_2.txt",                                        PersistencePolicy_wc, 3, 0, "File>>gr10_>>/gr10_2.txt::user3"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/gr10_2.txt",                                        PersistencePolicy_wc, 4, 0, "File>>gr10_>>/gr10_2.txt::user4"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",      PersistencePolicy_wc, 2, 1, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",      PersistencePolicy_wc, 2, 2, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",      PersistencePolicy_wc, 2, 3, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3"},
    {0x10,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",      PersistencePolicy_wc, 2, 4, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4"},
    /* factory-default data */
    {0x10, PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr10_1.txt",           PersistencePolicy_wt, 0, 0, "File>>gr10_>>/gr10_1.txt factory-default : orig"},
    {0x10, PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_A.txt",      PersistencePolicy_wt, 0, 0, "File>>gr10_>>/Docs/gr10_A.txt factory-default : orig"},
    {0x10, PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr10_2.txt",            PersistencePolicy_wc, 0, 0, "File>>gr10_>>/gr10_2.txt factory-default : orig"},
    {0x10, PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_B.txt",       PersistencePolicy_wc, 0, 0, "File>>gr10_>>/Docs/gr10_B.txt factory-default : orig"},
    /* configurable-default data */
    {0x10, PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr10_1.txt",       PersistencePolicy_wt, 0, 0, "File>>gr10_>>/gr10_1.txt configurable-default : orig"},
    {0x10, PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_A.txt",  PersistencePolicy_wt, 0, 0, "File>>gr10_>>/Docs/gr10_A.txt configurable-default : orig"},
    {0x10, PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr10_2.txt",        PersistencePolicy_wc, 0, 0, "File>>gr10_>>/gr10_2.txt configurable-default : orig"},
    {0x10, PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_B.txt",   PersistencePolicy_wc, 0, 0, "File>>gr10_>>/Docs/gr10_B.txt configurable-default : orig"},
};


static dataInit_s sShared_Group10_DataInit =
{
    PERS_ORG_SHARED_GROUP_WT_PATH_"10/",
    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,   
    RCT_group10_init,
    sizeof(RCT_group10_init)/sizeof(RCT_group10_init[0]),
    dataKeys_Group10_Init,
    sizeof(dataKeys_Group10_Init)/sizeof(dataKeys_Group10_Init[0]),
    dataFiles_Group10_Init,
    sizeof(dataFiles_Group10_Init)/sizeof(dataFiles_Group10_Init[0]),
    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,
    dataKeys_Group10_FactoryDefaultInit,
    sizeof(dataKeys_Group10_FactoryDefaultInit)/sizeof(dataKeys_Group10_FactoryDefaultInit[0]),
    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,
    dataKeys_Group10_ConfigurableDefaultInit,
    sizeof(dataKeys_Group10_ConfigurableDefaultInit)/sizeof(dataKeys_Group10_ConfigurableDefaultInit[0]) 
} ;


/**********************************************************************************************************************************************
 ***************************************** Group 20 *******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_group20_init[] =
{
    {"gr20_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_Group20_Init[] =
{
    {0x20,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingA",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr20_SettingA"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/gr20_SettingB::user2::seat1"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/gr20_SettingB::user2:seat2"},
    {0x20,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingC",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr20_SettingC"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/ABC",                                     PersistencePolicy_wt, 1, 0, "Data>>/gr20_Setting/ABC::user1"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/ABC",                                     PersistencePolicy_wt, 2, 0, "Data>>/gr20_Setting/ABC::user2"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/ABC",                                     PersistencePolicy_wt, 3, 0, "Data>>/gr20_Setting/ABC::user3"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/ABC",                                     PersistencePolicy_wt, 4, 0, "Data>>/gr20_Setting/ABC::user4"},
    {0x20,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr20_SettingD"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/gr20_SettingE::user2:seat1"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/gr20_SettingE::user2:seat2"},
    {0x20,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr20_SettingF"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/gr20_Setting/DEF::user1"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/gr20_Setting/DEF::user2"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/gr20_Setting/DEF::user3"},
    {0x20,  PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/gr20_Setting/DEF::user4"}
};

static entryDataInit_s dataFiles_Group20_Init[] =
{
    {0x20,  PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                           PersistencePolicy_wt, 0, 0, "File>>gr20_>>/doc1.txt"},
    {0x20,  PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                      PersistencePolicy_wt, 0, 0, "File>>gr20_>>/Docs/doc2.txt"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                        PersistencePolicy_wc, 1, 0, "File>>gr20_>>/docA.txt::user1"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                        PersistencePolicy_wc, 2, 0, "File>>gr20_>>/docA.txt::user2"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                        PersistencePolicy_wc, 3, 0, "File>>gr20_>>/docA.txt::user3"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                        PersistencePolicy_wc, 4, 0, "File>>gr20_>>/docA.txt::user4"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>gr20_>>/docB.txt::user2:seat1"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>gr20_>>/docB.txt::user2:seat2"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>gr20_>>/docB.txt::user2:seat3"},
    {0x20,  PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>gr20_>>/docB.txt::user2:seat4"}
};


static dataInit_s sShared_Group20_DataInit =
{
    PERS_ORG_SHARED_GROUP_WT_PATH_"20/",
    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,   
    RCT_group20_init,
    sizeof(RCT_group20_init)/sizeof(RCT_group20_init[0]),
    dataKeys_Group20_Init,
    sizeof(dataKeys_Group20_Init)/sizeof(dataKeys_Group20_Init[0]),
    dataFiles_Group20_Init,
    sizeof(dataFiles_Group20_Init)/sizeof(dataFiles_Group20_Init[0]),
    NIL,
    NIL,
    0,
    NIL,
    NIL,
    0
} ;

/**********************************************************************************************************************************************
 ***************************************** App1 *******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_App1_init[] =
{
    {"App1_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"App1_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"},{NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_App1_Init[] =
{
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                         PersistencePolicy_wt, 0, 0, "Data>>/App1_SettingA"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/App1_SettingB::user2::seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/App1_SettingB::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                         PersistencePolicy_wt, 0, 0, "Data>>/App1_SettingC"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                                     PersistencePolicy_wt, 1, 0, "Data>>/App1_Setting/ABC::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                                     PersistencePolicy_wt, 2, 0, "Data>>/App1_Setting/ABC::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                                     PersistencePolicy_wt, 3, 0, "Data>>/App1_Setting/ABC::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                                     PersistencePolicy_wt, 4, 0, "Data>>/App1_Setting/ABC::user4"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/App1_SettingD"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/App1_SettingE::user2:seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/App1_SettingE::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/App1_SettingF"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/App1_Setting/DEF::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/App1_Setting/DEF::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/App1_Setting/DEF::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/App1_Setting/DEF::user4"}
} ;

static entryDataInit_s dataFiles_App1_Init[] =
{
    {0xFF,  PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                           PersistencePolicy_wt, 0, 0, "File>>App1>>/doc1.txt"},
    {0xFF,  PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                      PersistencePolicy_wt, 0, 0, "File>>App1>>/Docs/doc2.txt"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                        PersistencePolicy_wc, 1, 0, "File>>App1>>/docA.txt::user1"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                        PersistencePolicy_wc, 2, 0, "File>>App1>>/docA.txt::user2"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                        PersistencePolicy_wc, 3, 0, "File>>App1>>/docA.txt::user3"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                        PersistencePolicy_wc, 4, 0, "File>>App1>>/docA.txt::user4"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>App1>>/docB.txt::user2:seat1"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>App1>>/docB.txt::user2:seat2"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>App1>>/docB.txt::user2:seat3"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docB.txt::user2:seat4"}
};


static dataInit_s s_App1_DataInit =
{
    PERS_ORG_LOCAL_APP_WT_PATH_"App1/",
    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,
    dbType_local,
    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,
    dbType_local,   
    RCT_App1_init,
    sizeof(RCT_App1_init)/sizeof(RCT_App1_init[0]),
    dataKeys_App1_Init,
    sizeof(dataKeys_App1_Init)/sizeof(dataKeys_App1_Init[0]),
    dataFiles_App1_Init,
    sizeof(dataFiles_App1_Init)/sizeof(dataFiles_App1_Init[0]),
    NIL,
    NIL,
    0,
    NIL,
    NIL,
    0
} ;

/**********************************************************************************************************************************************
 ***************************************** App2*******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_App2_init[] =
{
    {"App2_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"App2_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"},{NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_App2_Init[] =
{
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingA",                                         PersistencePolicy_wt, 0, 0, "Data>>/App2_SettingA"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/App2_SettingB::user2::seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/App2_SettingB::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingC",                                         PersistencePolicy_wt, 0, 0, "Data>>/App2_SettingC"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/ABC",                                     PersistencePolicy_wt, 1, 0, "Data>>/App2_Setting/ABC::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/ABC",                                     PersistencePolicy_wt, 2, 0, "Data>>/App2_Setting/ABC::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/ABC",                                     PersistencePolicy_wt, 3, 0, "Data>>/App2_Setting/ABC::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/ABC",                                     PersistencePolicy_wt, 4, 0, "Data>>/App2_Setting/ABC::user4"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/App2_SettingD"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/App2_SettingE::user2:seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/App2_SettingE::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/App2_SettingF"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/App2_Setting/DEF::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/App2_Setting/DEF::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/App2_Setting/DEF::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/App2_Setting/DEF::user4"}
} ;

static entryDataInit_s dataFiles_App2_Init[] =
{
    {0xFF,  PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                           PersistencePolicy_wt, 0, 0, "File>>App2>>/doc1.txt"},
    {0xFF,  PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                      PersistencePolicy_wt, 0, 0, "File>>App2>>/Docs/doc2.txt"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                        PersistencePolicy_wc, 1, 0, "File>>App2>>/docA.txt::user1"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                        PersistencePolicy_wc, 2, 0, "File>>App2>>/docA.txt::user2"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                        PersistencePolicy_wc, 3, 0, "File>>App2>>/docA.txt::user3"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                        PersistencePolicy_wc, 4, 0, "File>>App2>>/docA.txt::user4"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>App2>>/docB.txt::user2:seat1"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>App2>>/docB.txt::user2:seat2"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>App2>>/docB.txt::user2:seat3"},
    {0xFF,  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docB.txt::user2:seat4"}
};  


static dataInit_s s_App2_DataInit =
{
     PERS_ORG_LOCAL_APP_WT_PATH_"App2/",
     PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_RCT_NAME,
    dbType_RCT,
     PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,
    dbType_local,
     PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,
    dbType_local,   
    RCT_App2_init,
    sizeof(RCT_App2_init)/sizeof(RCT_App2_init[0]),
    dataKeys_App2_Init,
    sizeof(dataKeys_App2_Init)/sizeof(dataKeys_App2_Init[0]),
    dataFiles_App2_Init,
    sizeof(dataFiles_App2_Init)/sizeof(dataFiles_App2_Init[0]),
    NIL,
    NIL,
    0,
    NIL,
    NIL,
    0
} ;



#if 0
expected_key_data_localDB_s expectedKeyDataAfterExportAll[] =
{
    /* add here ... */
};
expected_file_data_s expectedFileDataAfterExportAll[] =
{
    /* add here ... */
}
#endif

/**************************************************************************************************************
*****************************************    ADD TEST CASES HERE   ********************************************
**************************************************************************************************************/

testcase_s tc_testDataAfterReset =
{
    Test_DataAfterReset,     
    0,  
    NIL,    
    "Check reference data structure after initialization",
    expectedKeyData_shared_public_RCT_AfterReset,
    sizeof(expectedKeyData_shared_public_RCT_AfterReset)/sizeof(expectedKeyData_shared_public_RCT_AfterReset[0]),
    expectedKeyData_shared_public_localDB_AfterReset,
    sizeof(expectedKeyData_shared_public_localDB_AfterReset)/sizeof(expectedKeyData_shared_public_localDB_AfterReset[0]),
    expectedFileData_shared_public_AfterReset,
    sizeof(expectedFileData_shared_public_AfterReset)/sizeof(expectedFileData_shared_public_AfterReset[0])
};

testcase_s tc_testResourceConfigAdd_public =
{
    Test_ResourceConfigAdd_1,
    0,  
    NIL,    
    "Resource Config Add : Configure public",
    expected_RCT_public,
    sizeof(expected_RCT_public)/sizeof(expected_RCT_public[0]),
    expectedKeyData_public,
    sizeof(expectedKeyData_public)/sizeof(expectedKeyData_public[0]),
    expectedFileData_public,
    sizeof(expectedFileData_public)/sizeof(expectedFileData_public[0]),
    false
};

testcase_s tc_testResourceConfigAdd_group_10 =
{
    NIL, //skip it
    0,  
    NIL,    
    "Resource Config Add : Configure group 10",
    expected_RCT_group_10,
    sizeof(expected_RCT_group_10)/sizeof(expected_RCT_group_10[0]),
    expectedKeyData_group_10,
    sizeof(expectedKeyData_group_10)/sizeof(expectedKeyData_group_10[0]),
    expectedFileData_group_10,
    sizeof(expectedFileData_group_10)/sizeof(expectedFileData_group_10[0]),
    true //skip data reset
};

testcase_s tc_testResourceConfigAdd_group_20 =
{
    NIL, //skip it
    0,  
    NIL,    
    "Resource Config Add : Configure group 20",
    expected_RCT_group_20,
    sizeof(expected_RCT_group_20)/sizeof(expected_RCT_group_20[0]),
    expectedKeyData_group_20,
    sizeof(expectedKeyData_group_20)/sizeof(expectedKeyData_group_20[0]),
    expectedFileData_group_20,
    sizeof(expectedFileData_group_20)/sizeof(expectedFileData_group_20[0]),
    true //skip data reset
};

testcase_s tc_testResourceConfigAdd_group_30 =
{
    NIL, //skip it
    0,  
    NIL,    
    "Resource Config Add : Configure new group 30",
    expected_RCT_group_30,
    sizeof(expected_RCT_group_30)/sizeof(expected_RCT_group_30[0]),
    expectedKeyData_group_30,
    sizeof(expectedKeyData_group_30)/sizeof(expectedKeyData_group_30[0]),
    expectedFileData_group_30,
    sizeof(expectedFileData_group_30)/sizeof(expectedFileData_group_30[0]),
    true //skip data reset
};

testcase_s tc_testResourceConfigAdd_App30 =
{
    NIL, //skip it
    0,  
    NIL,    
    "Resource Config Add : Configure new App30",
    expected_RCT_App30_Phase_1,
    sizeof(expected_RCT_App30_Phase_1)/sizeof(expected_RCT_App30_Phase_1[0]),
    expectedKeyData_App30_Phase_1,
    sizeof(expectedKeyData_App30_Phase_1)/sizeof(expectedKeyData_App30_Phase_1[0]),
    expectedFileData_App30_Phase_1,
    sizeof(expectedFileData_App30_Phase_1)/sizeof(expectedFileData_App30_Phase_1[0]),
    true //skip data reset
};

testcase_s tc_testResourceConfigAdd_public_phase_2 =
{
    Test_ResourceConfigAdd_2,
    0,  
    NIL,    
    "Resource Config Add : Configure public - Phase 2",
    expected_RCT_public,
    sizeof(expected_RCT_public)/sizeof(expected_RCT_public[0]),
    expectedKeyData_public_phase2,
    sizeof(expectedKeyData_public_phase2)/sizeof(expectedKeyData_public_phase2[0]),
    expectedFileData_public_phase2,
    sizeof(expectedFileData_public_phase2)/sizeof(expectedFileData_public_phase2[0]),
    true //skip data reset
};

testcase_s tc_testDataAfterDeleteUser2Data =
{
    Test_DataAfterDeleteUser2Data,
    0,
    NIL,
    "Check user data delete for user 2 data",
    NIL,
    0,
    expectedKeyData_shared_public_localDB_AfterDeleteUser2Data,
    sizeof(expectedKeyData_shared_public_localDB_AfterDeleteUser2Data)/sizeof(expectedKeyData_shared_public_localDB_AfterDeleteUser2Data[0]),
    expectedFileData_shared_public_AfterDeleteUser2Data,
    sizeof(expectedFileData_shared_public_AfterDeleteUser2Data)/sizeof(expectedFileData_shared_public_AfterDeleteUser2Data[0]),
    false
};

testcase_s tc_testDataAfterBackupCreateAll =
{
    Test_DataAfterBackupCreateAll,
    0,
    NIL,
    "Check data backup create all",
    NIL,
    0,
    expectedKeyData_shared_public_localDB_AfterBackupCreateAll,
    sizeof(expectedKeyData_shared_public_localDB_AfterBackupCreateAll)/sizeof(expectedKeyData_shared_public_localDB_AfterBackupCreateAll[0]),
    expectedFileData_shared_public_AfterBackupCreateAll,
    sizeof(expectedFileData_shared_public_AfterBackupCreateAll)/sizeof(expectedFileData_shared_public_AfterBackupCreateAll[0]),
    false
};

testcase_s tc_testDataAfterBackupCreateApplication =
{
    Test_DataAfterBackupCreateApplication,
    0,
    NIL,
    "Check data backup create application",
    NIL,
    0,
    expectedKeyData_shared_public_localDB_AfterBackupApplication,
    sizeof(expectedKeyData_shared_public_localDB_AfterBackupApplication)/sizeof(expectedKeyData_shared_public_localDB_AfterBackupApplication[0]),
    expectedFileData_shared_public_AfterBackupApplication,
    sizeof(expectedFileData_shared_public_AfterBackupApplication)/sizeof(expectedFileData_shared_public_AfterBackupApplication[0]),
    false
};

testcase_s tc_testDataAfterBackupCreateUserAll =
{
    Test_DataAfterBackupCreateUserAll,
    0,
    NIL,
    "Check data backup create user all",
    NIL,
    0,
    expectedKeyData_shared_public_localDB_AfterBackupUserAll,
    sizeof(expectedKeyData_shared_public_localDB_AfterBackupUserAll)/sizeof(expectedKeyData_shared_public_localDB_AfterBackupUserAll[0]),
    expectedFileData_shared_public_AfterBackupUserAll,
    sizeof(expectedFileData_shared_public_AfterBackupUserAll)/sizeof(expectedFileData_shared_public_AfterBackupUserAll[0]),
    false
};

testcase_s tc_testDataAfterBackupCreateUser2SeatAll =
{
    Test_DataAfterBackupCreateUser2SeatAll,
    0,
    NIL,
    "Check data backup create user 2 seat all",
    NIL,
    0,
    expectedKeyData_shared_public_localDB_AfterBackupUser2SeatAll,
    sizeof(expectedKeyData_shared_public_localDB_AfterBackupUser2SeatAll)/sizeof(expectedKeyData_shared_public_localDB_AfterBackupUser2SeatAll[0]),
    expectedFileData_shared_public_AfterBackupUser2SeatAll,
    sizeof(expectedFileData_shared_public_AfterBackupUser2SeatAll)/sizeof(expectedFileData_shared_public_AfterBackupUser2SeatAll[0]),
    false
};

testcase_s tc_test_Recover_App1 =
{
    Test_Recover_App1,
    0,
    NIL,
    "Check reference data structure after App1 recovery",
    NIL,
    0,
    expected_key_data_after_restore_App1,
    sizeof(expected_key_data_after_restore_App1)/sizeof(expected_key_data_after_restore_App1[0]),
    expected_file_data_after_restore_App1,
    sizeof(expected_file_data_after_restore_App1)/sizeof(expected_file_data_after_restore_App1[0]),
    false
};

testcase_s tc_test_Recover_User1 =
{
    Test_Recover_User1,
    0,
    NIL,
    "Check reference data structure after User1 recovery",
    NIL,
    0,
    expected_App1_key_data_after_restore_User1,
    sizeof(expected_App1_key_data_after_restore_User1)/sizeof(expected_App1_key_data_after_restore_User1[0]),
    expected_App1_file_data_after_restore_User1,
    sizeof(expected_App1_file_data_after_restore_User1)/sizeof(expected_App1_file_data_after_restore_User1[0]),
    false
};

testcase_s tc_test_Recover_All =
{
    Test_Recover_All,
    0,
    NIL,
    "Check reference data structure after All recovery",
    NIL,
    0,
    expected_key_data_after_restore_All,
    sizeof(expected_key_data_after_restore_All)/sizeof(expected_key_data_after_restore_All[0]),
    expected_file_data_after_restore_All,
    sizeof(expected_file_data_after_restore_All)/sizeof(expected_file_data_after_restore_All[0]),
    false
};

testcase_s tc_test_Recover_Users =
{
    Test_Recover_Users,
    0,
    NIL,
    "Check reference data structure after All recovery",
    NIL,
    0,
    expected_key_data_after_restore_Users,
    sizeof(expected_key_data_after_restore_Users)/sizeof(expected_key_data_after_restore_Users[0]),
    expected_file_data_after_restore_Users,
    sizeof(expected_file_data_after_restore_Users)/sizeof(expected_file_data_after_restore_Users[0]),
    false
};

testcase_s tc_test_Recover_All_InitialContent =
{
    Test_Recover_All_InitialContent,
    0,
    NIL,
    "Check reference data structure after All InitialContent recovery",
    NIL,
    0,
    expected_key_data_after_restore_All_InitialContent,
    sizeof(expected_key_data_after_restore_All_InitialContent)/sizeof(expected_key_data_after_restore_All_InitialContent[0]),
    expected_file_data_after_restore_All_InitialContent,
    sizeof(expected_file_data_after_restore_All_InitialContent)/sizeof(expected_file_data_after_restore_All_InitialContent[0]),
    false
};

testcase_s tc_test_Recover_App1_InitialContent_From_All =
{
    Test_Recover_App1_InitialContent_From_All,
    0,
    NIL,
    "Check reference data structure after App1 InitialContent recovery from All",
    NIL,
    0,
    expected_key_data_after_restore_App1_InitialContent,
    sizeof(expected_key_data_after_restore_App1_InitialContent)/sizeof(expected_key_data_after_restore_App1_InitialContent[0]),
    expected_file_data_after_restore_App1_InitialContent,
    sizeof(expected_file_data_after_restore_App1_InitialContent)/sizeof(expected_file_data_after_restore_App1_InitialContent[0]),
    false
};

testcase_s tc_test_Recover_App1_InitialContent_From_App1 =
{
    Test_Recover_App1_InitialContent_From_App1,
    0,
    NIL,
    "Check reference data structure after App1 InitialContent recovery from App1",
    NIL,
    0,
    expected_key_data_after_restore_App1_InitialContent,
    sizeof(expected_key_data_after_restore_App1_InitialContent)/sizeof(expected_key_data_after_restore_App1_InitialContent[0]),
    expected_file_data_after_restore_App1_InitialContent,
    sizeof(expected_file_data_after_restore_App1_InitialContent)/sizeof(expected_file_data_after_restore_App1_InitialContent[0]),
    false
};

testcase_s tc_test_Recover_User1_InitialContent_From_All =
{
    Test_Recover_User1_InitialContent_From_All,
    0,
    NIL,
    "Check reference data structure after User1 InitialContent recovery from All",
    NIL,
    0,
    expected_key_data_after_restore_User1_InitialContent,
    sizeof(expected_key_data_after_restore_User1_InitialContent)/sizeof(expected_key_data_after_restore_User1_InitialContent[0]),
    expected_file_data_after_restore_User1_InitialContent,
    sizeof(expected_file_data_after_restore_User1_InitialContent)/sizeof(expected_file_data_after_restore_User1_InitialContent[0]),
    false
};

testcase_s tc_test_Recover_User1_InitialContent_From_User1 =
{
    Test_Recover_User1_InitialContent_From_User1,
    0,
    NIL,
    "Check reference data structure after User1 InitialContent recovery from User1",
    NIL,
    0,
    expected_key_data_after_restore_User1_InitialContent,
    sizeof(expected_key_data_after_restore_User1_InitialContent)/sizeof(expected_key_data_after_restore_User1_InitialContent[0]),
    expected_file_data_after_restore_User1_InitialContent,
    sizeof(expected_file_data_after_restore_User1_InitialContent)/sizeof(expected_file_data_after_restore_User1_InitialContent[0]),
    false
};

testcase_s tc_test_Recover_User2_Seat1_InitialContent_From_All =
{
    Test_Recover_User2_Seat1_InitialContent_From_All,
    0,
    NIL,
    "Check reference data structure after User2 Seat1 InitialContent recovery from All",
    NIL,
    0,
    expected_key_data_after_restore_User2_Seat1_InitialContent,
    sizeof(expected_key_data_after_restore_User2_Seat1_InitialContent)/sizeof(expected_key_data_after_restore_User2_Seat1_InitialContent[0]),
    expected_file_data_after_restore_User2_Seat1_InitialContent,
    sizeof(expected_file_data_after_restore_User2_Seat1_InitialContent)/sizeof(expected_file_data_after_restore_User2_Seat1_InitialContent[0]),
    false
};

testcase_s tc_test_Recover_User2_Seat1_InitialContent_From_User2_Seat1 =
{
    Test_Recover_User2_Seat1_InitialContent_From_User2_Seat1,
    0,
    NIL,
    "Check reference data structure after User2 Seat1 InitialContent recovery from User2 Seat1",
    NIL,
    0,
    expected_key_data_after_restore_User2_Seat1_InitialContent,
    sizeof(expected_key_data_after_restore_User2_Seat1_InitialContent)/sizeof(expected_key_data_after_restore_User2_Seat1_InitialContent[0]),
    expected_file_data_after_restore_User2_Seat1_InitialContent,
    sizeof(expected_file_data_after_restore_User2_Seat1_InitialContent)/sizeof(expected_file_data_after_restore_User2_Seat1_InitialContent[0]),
    false
};

testcase_s tc_test_Restore_Factory_Default_App1 =
{
	Test_Restore_Factory_Default_App1,
    0,
    NIL,
    "Check reference data structure after restore to factory default of App1",
    NIL,
    0,
    expected_key_data_after_restore_default_App1,
    sizeof(expected_key_data_after_restore_default_App1)/sizeof(expected_key_data_after_restore_default_App1[0]),
    expected_file_data_after_restore_default_App1,
    sizeof(expected_file_data_after_restore_default_App1)/sizeof(expected_file_data_after_restore_default_App1[0]),
    false
};

testcase_s tc_test_Restore_Configurable_Default_App1 =
{
	Test_Restore_Configurable_Default_App1,
    0,
    NIL,
    "Check reference data structure after restore to configurable default of App1",
    NIL,
    0,
    expected_key_data_after_restore_default_App1,
    sizeof(expected_key_data_after_restore_default_App1)/sizeof(expected_key_data_after_restore_default_App1[0]),
    expected_file_data_after_restore_default_App1,
    sizeof(expected_file_data_after_restore_default_App1)/sizeof(expected_file_data_after_restore_default_App1[0]),
    false
};

testcase_s tc_test_Restore_Configurable_Default_User1 =
{
	Test_Restore_Configurable_Default_User1,
    0,
    NIL,
    "Check reference data structure after restore to configurable default of User1",
    NIL,
    0,
    expected_key_data_after_restore_default_User1,
    sizeof(expected_key_data_after_restore_default_User1)/sizeof(expected_key_data_after_restore_default_User1[0]),
    expected_file_data_after_restore_default_User1,
    sizeof(expected_file_data_after_restore_default_User1)/sizeof(expected_file_data_after_restore_default_User1[0]),
    false
};

testcase_s tc_test_Restore_Configurable_Default_User2Seat1 =
{
	Test_Restore_Configurable_Default_User2Seat1,
    0,
    NIL,
    "Check reference data structure after restore to configurable default of User2 Seat1",
    NIL,
    0,
    expected_key_data_after_restore_default_User2Seat1,
    sizeof(expected_key_data_after_restore_default_User2Seat1)/sizeof(expected_key_data_after_restore_default_User2Seat1[0]),
    expected_file_data_after_restore_default_User2Seat1,
    sizeof(expected_file_data_after_restore_default_User2Seat1)/sizeof(expected_file_data_after_restore_default_User2Seat1[0]),
    false
};

testcase_s tc_test_Restore_Configurable_Default_User2App1 =
{
	Test_Restore_Configurable_Default_User2App1,
    0,
    NIL,
    "Check reference data structure after restore to configurable default of User2 App1",
    NIL,
    0,
    expected_key_data_after_restore_default_User2App1,
    sizeof(expected_key_data_after_restore_default_User2App1)/sizeof(expected_key_data_after_restore_default_User2App1[0]),
    expected_file_data_after_restore_default_User2App1,
    sizeof(expected_file_data_after_restore_default_User2App1)/sizeof(expected_file_data_after_restore_default_User2App1[0]),
    false
};

testcase_s tc_test_Restore_Factory_Default_All =
{
	Test_Restore_Factory_Default_All,
    0,
    NIL,
    "Check reference data structure after restore to factory default All content",
    NIL,
    0,
    expected_key_data_after_restore_default_All,
    sizeof(expected_key_data_after_restore_default_All)/sizeof(expected_key_data_after_restore_default_All[0]),
    expected_file_data_after_restore_default_All,
    sizeof(expected_file_data_after_restore_default_All)/sizeof(expected_file_data_after_restore_default_All[0]),
    false
};

testcase_s tc_test_Restore_Configurable_Default_All =
{
	Test_Restore_Configurable_Default_All,
    0,
    NIL,
    "Check reference data structure after restore to configurable default All content",
    NIL,
    0,
    expected_key_data_after_restore_default_All,
    sizeof(expected_key_data_after_restore_default_All)/sizeof(expected_key_data_after_restore_default_All[0]),
    expected_file_data_after_restore_default_All,
    sizeof(expected_file_data_after_restore_default_All)/sizeof(expected_file_data_after_restore_default_All[0]),
    false
};


testcase_s tc_testImportApp =
{
    Test_import_all_app,
    0,
    NIL,
    "Import all _ app",
    NIL,
    0,
    expected_key_data_after_import_app_all,
    sizeof(expected_key_data_after_import_app_all)/sizeof(expected_key_data_after_import_app_all[0]),
    expected_file_data_after_import_app_all,
    sizeof(expected_file_data_after_import_app_all)/sizeof(expected_file_data_after_import_app_all[0]),
    false
};

testcase_s tc_testImportAll =
{
    Test_import_all_all,
    0,
    NIL,
    "Import all _ all",
    NIL,
    0,
    expected_key_data_after_import_all_all,
    sizeof(expected_key_data_after_import_all_all)/sizeof(expected_key_data_after_import_all_all[0]),
    expected_file_data_after_import_all_all,
    sizeof(expected_file_data_after_import_all_all)/sizeof(expected_file_data_after_import_all_all[0]),
    false
};

testcase_s tc_testImportUser =
{
    Test_import_all_user,
    0,
    NIL,
    "Import all _ user",
    NIL,
    0,
    expected_key_data_after_import_all_user,
    sizeof(expected_key_data_after_import_all_user)/sizeof(expected_key_data_after_import_all_user[0]),
    expected_file_data_after_import_all_user,
    sizeof(expected_file_data_after_import_all_user)/sizeof(expected_file_data_after_import_all_user[0]),
    false
};




/**************************************************************************************************************
*****************************************    ADD TEST CASES HERE   ********************************************
**************************************************************************************************************/

testcase_s* testCases[] =
{
    /* add here test cases */

	/* ResourceConfigAdd */
    /* don't change the order - start */
    &tc_testResourceConfigAdd_public
    ,&tc_testResourceConfigAdd_group_10
    ,&tc_testResourceConfigAdd_group_20
    ,&tc_testResourceConfigAdd_group_30
    ,&tc_testResourceConfigAdd_App30
    ,&tc_testResourceConfigAdd_public_phase_2
    /* don't change the order - end */


    /* Delete */
    ,&tc_testDataAfterDeleteUser2Data

    /* Backup */
    ,&tc_testDataAfterBackupCreateAll
    ,&tc_testDataAfterBackupCreateApplication
    ,&tc_testDataAfterBackupCreateUserAll
    ,&tc_testDataAfterBackupCreateUser2SeatAll

    /* Recovery */
    ,&tc_test_Recover_App1
    ,&tc_test_Recover_User1
    ,&tc_test_Recover_All
    ,&tc_test_Recover_Users
    ,&tc_test_Recover_All_InitialContent
    ,&tc_test_Recover_App1_InitialContent_From_All
    ,&tc_test_Recover_App1_InitialContent_From_App1
    ,&tc_test_Recover_User1_InitialContent_From_All
    ,&tc_test_Recover_User1_InitialContent_From_User1
    ,&tc_test_Recover_User2_Seat1_InitialContent_From_All
    ,&tc_test_Recover_User2_Seat1_InitialContent_From_User2_Seat1

    /* Restore default */
    ,&tc_test_Restore_Factory_Default_App1
    ,&tc_test_Restore_Configurable_Default_App1
    ,&tc_test_Restore_Configurable_Default_User1
    ,&tc_test_Restore_Factory_Default_All
    ,&tc_test_Restore_Configurable_Default_All
    ,&tc_test_Restore_Configurable_Default_User2Seat1
    ,&tc_test_Restore_Configurable_Default_User2App1

    /* Import */
    ,&tc_testImportApp
    ,&tc_testImportAll
    ,&tc_testImportUser
} ;



/**********************************************************************************************************************************************
 *********************************************************************************************************************************************/



typedef struct
{
    void*       dbHandle ;
    pstr_t      dbPath ;
    dbType_e    dbType ; 
}db_handle_s;



static bool_t ResetReferenceData(void) ;
static bool_t CreateFileWithData(pstr_t filePath, pstr_t data, sint_t dataSize) ;
static bool_t ExecuteTestCase(testcase_s* psTestCase) ;

static sint_t persadmin_serialize_data(PersistenceConfigurationKey_s pc, char* buffer, int size) ;

static bool_t CreateFileWithData(pstr_t filePath, pstr_t data, sint_t dataSize)
{
    bool_t bEverythingOK = true ;
    str_t folderPath[256] ;

    if((NIL == filePath) || (NIL == data) || (dataSize < 0))
    {
        bEverythingOK = false ;
    }

    if(bEverythingOK)
    {
        sint_t result = persadmin_get_folder_path(filePath, folderPath, sizeof(folderPath)) ;
        if(result >= 0)
        {
            result = persadmin_create_folder(folderPath) ;
            if(result < 0)
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK)
    {
        FILE *pFile = fopen(filePath, "wb") ;
        if(NIL != pFile)
        {
            if(dataSize != fwrite (data , 1 , dataSize , pFile ))
            {
                bEverythingOK = false ;
            }
            fclose (pFile);
        }
        else
        {
            bEverythingOK = false ;
        }
    }

    return bEverythingOK ;
}

bool_t InitDataFolder(dataInit_s* psDataInit)
{
    bool_t bEverythingOK = true ;
    sint_t i = 0 ;

    if(0 > persadmin_create_folder(psDataInit->installFolderPath))
    {
        bEverythingOK = false ;
    }
    #if 0
    else
    {
        str_t buffer[1] ;
        if(! CreateFileWithData(psDataInit->RCT_pathname, buffer, 0))
        {
            bEverythingOK = false ;
        }
    }
    #endif

    if(bEverythingOK)
    {
        if(     (NIL != psDataInit->RctInitTab)
            &&  (NIL != psDataInit->RCT_pathname)
            &&  (psDataInit->RctDBtype)
        )
        {
            sint_t rctHandler = persComRctOpen(psDataInit->RCT_pathname, true) ;
            if(rctHandler >= 0)
            {
                for(i = 0 ; i < psDataInit->noEntriesRctInitTab ; i++)
                {
                    str_t buffer[64] ;
                    persadmin_serialize_data(psDataInit->RctInitTab[i].sRctEntry, buffer, sizeof(buffer)) ;
                    psDataInit->RctInitTab[i].sRctEntry.type = 
                        psDataInit->RctInitTab[i].bIsKey ? PersistenceResourceType_key : PersistenceResourceType_file ;

                    if(0 > persComRctWrite(rctHandler, psDataInit->RctInitTab[i].resourceID, &psDataInit->RctInitTab[i].sRctEntry))
                    {
                        bEverythingOK = false ;
                        sprintf(g_msg, "persComRctWrite(<%s> <%s> <%s>) FAILED", psDataInit->RCT_pathname, psDataInit->RctInitTab[i].resourceID, buffer) ;
                        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));                        
                        break ;
                    }
                    else
                    {
                        sprintf(g_msg, "persComRctWrite(<%s> <%s> <%s> type=%d) done", 
                                psDataInit->RCT_pathname, psDataInit->RctInitTab[i].resourceID, buffer, psDataInit->RctInitTab[i].sRctEntry.type) ;
                        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
                    }
                }
                persComRctClose(rctHandler) ;
            }
            else
            {
                bEverythingOK = false ;
                sprintf(g_msg, "persComRctOpen(<<%s>>, true) FAILED", psDataInit->RCT_pathname) ;
                DLT_LOG(testPersAdminDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
            }
        }
    }

    if(bEverythingOK)
    {
        if(NIL != psDataInit->dataKeysInitTab)
        {
            sint_t wcDbHandler = persComDbOpen(psDataInit->wcDBpathname, true) ;
            sint_t wtDbHandler = persComDbOpen(psDataInit->wtDBpathname, true) ;
            sprintf(g_msg, "InitDataFolder: wcDbHandler=<%d> wtDbHandler=<%d>", wcDbHandler, wtDbHandler) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            for(i = 0 ; i < psDataInit->noEntriesDataKeysInitTab; i++)
            {
                pstr_t dbPath = ( PersistencePolicy_wc == psDataInit->dataKeysInitTab[i].policy) ? psDataInit->wcDBpathname : psDataInit->wtDBpathname ;
                sint_t dbHandler = ( PersistencePolicy_wc == psDataInit->dataKeysInitTab[i].policy) ? wcDbHandler : wtDbHandler ;
                if((dbHandler >= 0) && (psDataInit->dataKeysInitTab[i].data))
                {

                    if(0 > persComDbWriteKey(dbHandler, psDataInit->dataKeysInitTab[i].resourceID, psDataInit->dataKeysInitTab[i].data, (strlen(psDataInit->dataKeysInitTab[i].data)+1)))
                    {
                        bEverythingOK = false ;
                        sprintf(g_msg, "persComDbWriteKey(<%s> <%s> %d) FAILED", dbPath, psDataInit->dataKeysInitTab[i].resourceID, (strlen(psDataInit->dataKeysInitTab[i].data)+1)) ;
                        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
                        break ;
                    }
                    else
                    {
                        sprintf(g_msg, "persComDbWriteKey(<%s> <%s> %d) done", dbPath, psDataInit->dataKeysInitTab[i].resourceID, (strlen(psDataInit->dataKeysInitTab[i].data)+1)) ;
                        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    }
                }
            }
            persComDbClose(wcDbHandler) ;
            persComDbClose(wtDbHandler) ;
        }
    }

    if(bEverythingOK)
    {
        if(NIL != psDataInit->dataFilesInitTab)
        {
            for(i = 0 ; i < psDataInit->noEntriesDataFilesInitTab; i++)
            {
                sprintf(g_msg, "CreateFileWithData(%s)...", psDataInit->dataFilesInitTab[i].resourceID) ;
                DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                if(!CreateFileWithData(psDataInit->dataFilesInitTab[i].resourceID, psDataInit->dataFilesInitTab[i].data, (strlen(psDataInit->dataFilesInitTab[i].data)+1)))
                {
                    bEverythingOK = false ;
                    sprintf(g_msg, "CreateFileWithData(%s) FAILED", psDataInit->dataFilesInitTab[i].resourceID) ;
                    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                    break ;
                }
            }
        }
    }

    if(bEverythingOK)
    {
        typedef struct
        {
            pstr_t              dbPathname ;
            defaultDataInit_s*  psDefaultData ;
            sint_t              iNoOfEntries ;
        }defaultDataInfo_s ;

        defaultDataInfo_s sDefaultDataTab[] =
        {
            {psDataInit->factoryDefaultDBpathname,  psDataInit->factoryDefaultInitTab, psDataInit->noEntriesFactoryDefaultInitTab},
            {psDataInit->configurableDefaultDBpathname,  psDataInit->configurableDefaultInitTab, psDataInit->noEntriesConfigurableDefaultInitTab}
        };

        for(i = 0 ; i < sizeof(sDefaultDataTab)/sizeof(sDefaultDataTab[0]) ; i++)
        {
            if( (NIL != sDefaultDataTab[i].dbPathname) && (NIL != sDefaultDataTab[i].psDefaultData))
            {
                sint_t hDefaultDB = persComDbOpen(sDefaultDataTab[i].dbPathname, true) ;
                if(hDefaultDB >= 0)
                {
                    sint_t j = 0 ;
                    for(j = 0 ; j < sDefaultDataTab[i].iNoOfEntries ; j++)
                    {
                        if(0 > persComDbWriteKey(hDefaultDB, sDefaultDataTab[i].psDefaultData[j].pResourceID, 
                                sDefaultDataTab[i].psDefaultData[j].data, (strlen(sDefaultDataTab[i].psDefaultData[j].data)+1)))
                        {
                            bEverythingOK = false ;
                            sprintf(g_msg, "persComDbWriteKey(<%s> <%s> %d) FAILED", 
                                sDefaultDataTab[i].dbPathname, sDefaultDataTab[i].psDefaultData[j].pResourceID, (strlen(sDefaultDataTab[i].psDefaultData[j].data)+1)) ;
                            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
                            break ;
                        }
                        else
                        {
                            sprintf(g_msg, "persComDbWriteKey(<%s> <%s> %d) done", 
                                sDefaultDataTab[i].dbPathname, sDefaultDataTab[i].psDefaultData[j].pResourceID, (strlen(sDefaultDataTab[i].psDefaultData[j].data)+1)) ;
                            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
                        }
                    }

                    persComDbClose(hDefaultDB) ;
                }
                else
                {
                    bEverythingOK = false ;
                    sprintf(g_msg, "InitDataFolder: persComDbOpen(%s) failed", sDefaultDataTab[i].dbPathname) ;
                    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
                }
            }
        }
        
    }


    return bEverythingOK ;
}

sint_t DeleteFolderContent(pstr_t folderPath)
{
    bool_t bEverythingOK = true ;
    sint_t bytesDeleted = 0 ;
    pstr_t listingBuffer = NIL ;

    sint_t neededBufferSize = persadmin_list_folder_get_size(folderPath, PersadminFilterAll, false) ;
    if(neededBufferSize > 0)
    {
        listingBuffer = (pstr_t)malloc(neededBufferSize) ;
        if(NIL != listingBuffer)
        {
            if(neededBufferSize != persadmin_list_folder(folderPath, listingBuffer, neededBufferSize, PersadminFilterAll, false))
            {
                bEverythingOK = false ;
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }

    if(bEverythingOK)
    {
        str_t completePath[256] ;
        sint_t posInBuffer = 0 ;
        sint_t posRelativePath ;
        strcpy(completePath, folderPath) ;
        posRelativePath = strlen(completePath) ;
        if('/' != completePath[posRelativePath-1])
        {
            strcat(completePath, "/") ;
            posRelativePath = strlen(completePath) ;
        }
        while(posInBuffer < neededBufferSize)
        {
            sint_t len = strlen(listingBuffer + posInBuffer) ;
            sint_t bytesDeletedLocal = -1 ;
            strcpy(completePath + posRelativePath, listingBuffer + posInBuffer) ;
            if(0 == persadmin_check_if_file_exists(completePath, true))
            {
                bytesDeletedLocal = persadmin_delete_folder(completePath) ;
            }
            else
            {
                if(0 == persadmin_check_if_file_exists(completePath, false))
                {
                    bytesDeletedLocal = persadmin_delete_file(completePath) ;
                }
                else
                {
                    bEverythingOK = false ;
                }
            }
            if(bytesDeletedLocal >= 0)
            {
                bytesDeleted += bytesDeletedLocal ;
            }
            else
            {
                bEverythingOK = false ;
            }
            posInBuffer += (len + 1) ;
        }
    }

    if(NIL != listingBuffer)
    {
        free(listingBuffer) ;
    }

    return bEverythingOK ? bytesDeleted : (-1) ;
}


sint_t DeleteFolder(pstr_t folderPath)
{
	sint_t bytesDeleted;

	bytesDeleted = DeleteFolderContent(folderPath);
	if(bytesDeleted < 0)
	{
		sprintf(g_msg, "DeleteFolderContent(%s) returned %d", folderPath, bytesDeleted);
		DLT_LOG(testPersAdminDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
		return bytesDeleted;
	}

	if(0 == remove(folderPath))
	{
		(void)snprintf(g_msg, sizeof(g_msg), "deleted >>%s<<", folderPath) ;
	}
	else
	{
		(void)snprintf(g_msg, sizeof(g_msg), "DeleteFolder: remove(%s) errno=<%s>", folderPath, strerror(errno));
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_ERROR, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
	}

	return bytesDeleted;
}

sint_t CheckIfFileExists(pstr_t pathname, bool_t bIsFolder)
{
    bool_t bEverythingOK = true ;
    if(NIL == pathname)
    {
        bEverythingOK = false ;
        (void)snprintf(g_msg, sizeof(g_msg), "persadmin_check_if_file_exist: NIL pathname") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }
    else
    {
        if('/' != pathname[0])
        {
            bEverythingOK = false ;
            (void)snprintf(g_msg, sizeof(g_msg), "persadmin_check_if_file_exist: not an absolute path(%s)", pathname) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_DEBUG, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
        }
    }

    if(bEverythingOK)
    {
        struct stat sb;

        if (0 == lstat(pathname, &sb))
        {
            /* pathname exist*/
            if(bIsFolder)
            {
                /* check if it is a foler */
                if( ! S_ISDIR(sb.st_mode))
                {
                    /* not a folder */
                    bEverythingOK = false ;
                }
            }
            else
            {
                /* check if it is a file */
                if(S_ISDIR(sb.st_mode))
                {
                    /* it is a folder */
                    bEverythingOK = false ;
                }
            }
        }
        else
        {
            bEverythingOK = false ;
        }
    }

    return bEverythingOK ? 0 : PAS_FAILURE ;
}

static bool_t ResetReferenceData(void)
{
    bool_t bEverythingOK = true ;
    pstr_t referenceDataPath =  PERS_ORG_LOCAL_APP_WT_PATH_  ;

    sint_t result = DeleteFolderContent(referenceDataPath) ;
    sprintf(g_msg, "DeleteFolderContent(%s) returned %d", referenceDataPath, result) ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    if(result < 0)
    {
        bEverythingOK = false ;
    }

    if(bEverythingOK)
    {
        dataInit_s* sDataInit[] = 
        {
            &sSharedPubDataInit,
            &sShared_Group10_DataInit,
            &sShared_Group20_DataInit,
            &s_App1_DataInit,
            &s_App2_DataInit
        };

        sint_t i = 0 ;
        for(i = 0 ; i < sizeof(sDataInit)/sizeof(sDataInit[0]) ; i++)
        {
            if(! InitDataFolder(sDataInit[i]))
            {
                bEverythingOK = false ;
            }
        }
    }

    return bEverythingOK ;
}


static bool_t CheckExpectedDataKeyLocalDB(expected_key_data_localDB_s* pExpectedData)
{
    bool_t bEverythingOK = true ;

    str_t dataBuffer[256] ;
    sint_t readSize = sizeof(dataBuffer) ;

    sint_t dbHandler = persComDbOpen(pExpectedData->dbPath, false) ;
    if(dbHandler >= 0)
    {
        readSize = persComDbReadKey(dbHandler, pExpectedData->key, dataBuffer, sizeof(dataBuffer)) ;
        persComDbClose(dbHandler) ;

        if(readSize >= 0)
        {
            sprintf(g_msg, "Found <%s> in %s :: size = %d data=<%s>",
                pExpectedData->key, pExpectedData->dbPath, readSize, dataBuffer) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            if(pExpectedData->bExpectedToExist)
            {
                if(readSize == pExpectedData->expectedDataSize)
                {
                    if(0 == memcmp(dataBuffer, pExpectedData->expectedData, pExpectedData->expectedDataSize))
                    {
                        sprintf(g_msg, "\t\t...as expected") ;
                    }
                    else
                    {
                        bEverythingOK = false ;
                        sprintf(g_msg, "\t\t...FAILURE - expected size = %d data=<%s>", pExpectedData->expectedDataSize,  pExpectedData->expectedData) ;
                    } 
                }
                else
                {
                    bEverythingOK = false ;
                    sprintf(g_msg, "\t\t...FAILURE - expected size = %d data=<%s>", pExpectedData->expectedDataSize,  pExpectedData->expectedData) ;
                }
            }
            else
            {
                bEverythingOK = false ;
                sprintf(g_msg, "\t\t...FAILURE - expected to not find key") ;
            }
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
        else
        {
            sprintf(g_msg, "Failed to find <%s> in %s",
                pExpectedData->key, pExpectedData->dbPath) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            if( ! pExpectedData->bExpectedToExist)
            {
                sprintf(g_msg, "\t\t...as expected") ;
                DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            }
            else
            {
                bEverythingOK = false ;
                sprintf(g_msg, "\t\t...FAILURE - expected size = %d data=<%s>", pExpectedData->expectedDataSize,  pExpectedData->expectedData) ;
                DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            }
                
        }
    }
    else
    {
        sprintf(g_msg, "persComDbOpen(%s) returned <%d>", pExpectedData->dbPath, dbHandler) ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        bEverythingOK = false ;
    }
    

    return bEverythingOK ;
}

/* copied here from PCL */
static sint_t persadmin_serialize_data(PersistenceConfigurationKey_s pc, char* buffer, int size)
{
   sint_t rval = 0;
   rval = snprintf(buffer, size, "%d %d %d %s",
                                               pc.policy, pc.storage, pc.max_size,
                                               pc.reponsible);

   //printf("persadmin_serialize_data: %s \n", buffer);
   return rval;
}


static bool_t CheckExpectedDataKeyRCT(expected_key_data_RCT_s* pExpectedData)
{
    bool_t bEverythingOK = true ;
    PersistenceConfigurationKey_s sFoundConfig ;
    str_t serializedFound[256] ;
    str_t serializedExpected[256] ;

    sint_t rctHandler = persComRctOpen(pExpectedData->dbPath, false);
    if(rctHandler >= 0)
    {
        if(sizeof(PersistenceConfigurationKey_s) == persComRctRead(rctHandler, pExpectedData->key, &sFoundConfig))
        {
            persadmin_serialize_data(sFoundConfig, serializedFound, sizeof(serializedFound)) ;
            persadmin_serialize_data(pExpectedData->sExpectedConfig, serializedExpected, sizeof(serializedExpected)) ;
            sprintf(g_msg, "Found <%s> in %s :: config=<%s>",
                pExpectedData->key, pExpectedData->dbPath, serializedFound) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            if(pExpectedData->bExpectedToExist)
            {
                if(0 == strcmp(serializedFound, serializedExpected))
                {
                    sprintf(g_msg, "\t\t...as expected") ;
                }
                else
                {
                    bEverythingOK = false ;
                    sprintf(g_msg, "\t\t...FAILURE - expected config=<%s>", serializedExpected) ;
                }
            }
            else
            {
                    bEverythingOK = false ;
                    sprintf(g_msg, "\t\t...FAILURE - expected to not find key") ;
            }
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
        else
        {
            sprintf(g_msg, "Failed to find <%s> in %s",
                pExpectedData->key, pExpectedData->dbPath) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            if( ! pExpectedData->bExpectedToExist)
            {
                sprintf(g_msg, "\t\t...as expected") ;
            }
            else
            {
                bEverythingOK = false ;
                sprintf(g_msg, "\t\t...FAILURE - expected config=<%s>", serializedExpected) ;
            }
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));             
        }

        persComRctClose(rctHandler) ;
    }

    return bEverythingOK ;
}

static bool_t GetFileContent(pstr_t absPath, pstr_t dataBuffer_out, sint_t* pSize_inout)
{
    bool_t bEverythingOK = true ;

    FILE* pFile = fopen(absPath, "rb") ;

    if(NIL != pFile)
    {
        sint_t readSize = fread(dataBuffer_out, 1, *pSize_inout, pFile);
        if( (readSize >= 0) && (readSize < *pSize_inout))
        {
            *pSize_inout = readSize ;
        }
        else
        {
            if(readSize >= *pSize_inout)
            {
                bEverythingOK = false ;
                sprintf(g_msg, "GetFileContent(%s) buffer too small (%d)", absPath, *pSize_inout) ;
            }
            else
            {
                bEverythingOK = false ;
            }
        }

        fclose(pFile) ;
    }

    return bEverythingOK ;
}

static bool_t CheckExpectedDataFile(expected_file_data_s* pExpectedData)
{
    bool_t bEverythingOK = true ;

    str_t dataBuffer[256] ;
    sint_t size = sizeof(dataBuffer) ;

    if(0 <= persadmin_check_if_file_exists(pExpectedData->filename, false))
    {
        /* file found */
        memset(dataBuffer, 0x0, sizeof(dataBuffer)) ;
        if(GetFileContent(pExpectedData->filename, dataBuffer, &size))
        {
            sprintf(g_msg, "Found <%s> :: size = %d data=<%s>",
                    pExpectedData->filename, size, dataBuffer) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            if(pExpectedData->bExpectedToExist)
            {
                if(size == pExpectedData->expectedDataSize)
                {
                    if(0 == memcmp(dataBuffer, pExpectedData->expectedData, pExpectedData->expectedDataSize))
                    {
                        sprintf(g_msg, "\t\t...as expected") ;
                    }
                    else
                    {
                        bEverythingOK = false ;
                        sprintf(g_msg, "\t\t...FAILURE - expected size = %d data=<%s>", pExpectedData->expectedDataSize,  pExpectedData->expectedData) ;
                    }
                }
                else
                {
                    bEverythingOK = false ;
                    sprintf(g_msg, "\t\t...FAILURE - expected size = %d data=<%s>", pExpectedData->expectedDataSize,  pExpectedData->expectedData) ;
                }
            }
            else
            {
                    bEverythingOK = false ;
                    sprintf(g_msg, "\t\t...FAILURE - expected to not find file") ;
            }
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
        else
        {
            bEverythingOK = false ;
            sprintf(g_msg, "GetFileContent(%s) failed", pExpectedData->filename) ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
    }
    else
    {
        sprintf(g_msg, "Failed to find <%s>", pExpectedData->filename) ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        if( ! pExpectedData->bExpectedToExist)
        {
            sprintf(g_msg, "\t\t...as expected") ;
        }
        else
        {
            bEverythingOK = false ;
            sprintf(g_msg, "\t\t...FAILURE - expected size = %d data=<%s>", pExpectedData->expectedDataSize,  pExpectedData->expectedData) ;
        }
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    }    

    return bEverythingOK ;
}


static bool_t ExecuteTestCase(testcase_s* psTestCase)
{
    bool_t bTestResult = true ;
    sprintf(g_msg, "ExecuteTestCase: %s", psTestCase->testCaseDescription) ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 

    if( ! psTestCase->bSkipDataReset)
    {
        sprintf(g_msg, "First init reference data...") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        if(ResetReferenceData())
        {
            sprintf(g_msg, "ResetReferenceData - Done") ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
        else
        {
            bTestResult = false ;
            sprintf(g_msg, "ResetReferenceData - Failed") ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
    }
    else
    {
        sprintf(g_msg, "Skip ResetReferenceData") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }


    if(bTestResult)
    {
        if(NIL != psTestCase->TestCaseFunction)
        {
            sprintf(g_msg, "Call test case function...") ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
            bTestResult = psTestCase->TestCaseFunction(psTestCase->iParam, psTestCase->pvoidParam) ;
            sprintf(g_msg, "Test case function returned with %s", bTestResult ? "SUCCESS" : "FAILURE") ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
        else
        {
            sprintf(g_msg, "Skip calling test case function...") ;
            DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        }
    }

    if(bTestResult)
    {
        sint_t i = 0 ;
        sint_t noOfFailedTests = 0 ;
        sint_t noOfSuccessfulTests = 0 ;

        sprintf(g_msg, "+++++++++++++++++++++ Check data key RCT START ... +++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        for(i = 0 ; i < psTestCase->noItemsInExpectedKeyDataRCT; i++)
        {
            if( ! CheckExpectedDataKeyRCT(&psTestCase->pExpectedKeyDataRCT[i]))
            {
                noOfFailedTests++ ;
                bTestResult = false ;
            }
            else
            {
                noOfSuccessfulTests++ ;
            }
        }
        sprintf(g_msg, "++++++++ Check data key RCT SUMMARY: tests ; %d executed %d success %d failed +++++++++++++", 
                        noOfFailedTests + noOfSuccessfulTests, noOfSuccessfulTests, noOfFailedTests) ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 

        noOfFailedTests = 0 ;
        noOfSuccessfulTests = 0 ;
        sprintf(g_msg, "+++++++++++++++++++++ Check data key localDB START ... +++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        for(i = 0 ; i < psTestCase->noItemsInExpectedKeyDataLocalDB ; i++)
        {
            if( ! CheckExpectedDataKeyLocalDB(&psTestCase->pExpectedKeyDataLocalDB[i]))
            {
                noOfFailedTests++ ;
                bTestResult = false ;
            }
            else
            {
                noOfSuccessfulTests++ ;
            }
        }
        sprintf(g_msg, "++++++++ Check data key localDB SUMMARY: tests ; %d executed %d success %d failed +++++++++++++", 
                        noOfFailedTests + noOfSuccessfulTests, noOfSuccessfulTests, noOfFailedTests) ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 

        noOfFailedTests = 0 ;
        noOfSuccessfulTests = 0 ;
        sprintf(g_msg, "+++++++++++++++++++++ Check data file START ... +++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        for(i = 0 ; i < psTestCase->noItemsInExpectedFileData; i++)
        {
            if( ! CheckExpectedDataFile(&psTestCase->pExpectedFileData[i]))
            {
                noOfFailedTests++ ;
                bTestResult = false ;
            }
            else
            {
                noOfSuccessfulTests++ ;
            }
        }
        sprintf(g_msg, "++++++++ Check data file SUMMARY: tests ; %d executed %d success %d failed +++++++++++++", 
                        noOfFailedTests + noOfSuccessfulTests, noOfSuccessfulTests, noOfFailedTests) ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    }

    return bTestResult ;
}


bool_t ExecuteTestCases(sint_t        *pNoOfTestCases,
                        sint_t        *pNoOfTestCasesSuccessful,
                        sint_t        *pNoOfTestCasesFailed )
{
    bool_t bTestsResult = true ;
    sint_t noOfTestCasesSuccessful = 0 ;
    sint_t noOfTestCasesFailed = 0 ;
    sint_t i = 0 ;
    for(i = 0 ; i < sizeof(testCases)/sizeof(testCases[0]) ; i++)
    {
        bool_t bCurrentTestcaseResult = true ;
        sprintf(g_msg, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        sprintf(g_msg, "++++++++++++++++++++++++  Test Case No %d Started...  +++++++++++++++++++++++++++++++++++++++", i) ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        sprintf(g_msg, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        if(ExecuteTestCase(testCases[i]))
        {
            noOfTestCasesSuccessful++ ;
        }
        else
        {
            bCurrentTestcaseResult = false ;
            noOfTestCasesFailed++ ;
            bTestsResult = false ;
        }
        sprintf(g_msg, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        sprintf(g_msg, "++++++++++++++++++++++++  Test Case No %d Completed %s  +++++++++++++++++++++++++++++++++++++++", i, bCurrentTestcaseResult ? "OK" : "with EROORS") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
        sprintf(g_msg, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") ;
        DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    }

    sprintf(g_msg, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    sprintf(g_msg, "++++++++ SUMMARY: Test cases <%d executed> <%d ok> <%d failed>  ++++++++++++++++++++++++++++", noOfTestCasesSuccessful+noOfTestCasesFailed, noOfTestCasesSuccessful, noOfTestCasesFailed) ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    sprintf(g_msg, "++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++") ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg)); 
    if(NULL != pNoOfTestCases)
    {
        *pNoOfTestCases = sizeof(testCases)/sizeof(testCases[0]);
    }

    if(NULL != pNoOfTestCasesSuccessful)
    {
        *pNoOfTestCasesSuccessful = noOfTestCasesSuccessful;
    }

    if(NULL != pNoOfTestCasesFailed)
    {
        *pNoOfTestCasesFailed = noOfTestCasesFailed;
    }

    return bTestsResult ;
}





// ===============================



// ===============================


int main(void)
{
    char context[16] ;
    char contextID[16] ;
    char appID[16] ;
    pid_t pid = getpid() ;

    sprintf(context, "ID_%d", pid) ;
    sprintf(contextID, "CONTEXT_%d", pid) ;
    sprintf(appID, "APP_%d", pid) ;
    DLT_REGISTER_APP(appID,"PAS");
    //DLT_REGISTER_CONTEXT(testPersAdminDLTCtx,"TestPAS", contextID);
    DLT_REGISTER_CONTEXT(persAdminSvcDLTCtx,"TestPAS", contextID);
    

    //sprintf(g_msg, "\n\n++++++++++ Test PAS  - START +++++++++++++++++\n\n") ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("++++++++++ Test PAS  - START +++++++++++++++++"));

    ExecuteTestCases(NULL, NULL, NULL) ;

    //sprintf(g_msg, "\n\n++++++++++ Test PAS  - FINISH ++++++++++++++++\n\n") ;
    DLT_LOG(testPersAdminDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING("++++++++++ Test PAS  - FINISH ++++++++++++++++"));
    /* Init and run the main DBus loop */
    persadmin_RunDBusMainLoop();

    return 1 ;
}
