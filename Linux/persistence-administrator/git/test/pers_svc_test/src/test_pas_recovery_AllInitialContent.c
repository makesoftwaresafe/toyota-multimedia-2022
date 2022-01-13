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

#include "persComTypes.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include <dlt/dlt.h>
#include <dlt/dlt_user.h>

#include "test_PAS.h"
#include "persistence_admin_service.h"
#include "test_pas_data_backup_recovery.h"

DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);

#define LT_HDR                          "TEST_PAS >> "

#define PATH_ABS_MAX_SIZE  ( 512)

#define File_t PersistenceResourceType_file
#define Key_t PersistenceResourceType_key
//===================================================================================================================
// INIT
//===================================================================================================================

/**********************************************************************************************************************************************
***************************************** public *******************************************************************************************
*********************************************************************************************************************************************/

static entryRctInit_s RCT_public_init[] =
{
    {"pubSettingA",                true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingB",                true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingC",                true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/ABC",             true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingD",                true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingE",                true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingF",                true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/DEF",             true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}}
};

static entryDataInit_s dataKeysPublicInit[] = {};

static entryDataInit_s dataFilesPublicInit[] = {};

static dataInit_s sSharedPubDataInit =
{
     PERS_ORG_SHARED_PUBLIC_WT_PATH_ ,
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
    sizeof(dataFilesPublicInit)/sizeof(dataFilesPublicInit[0])
};

/**********************************************************************************************************************************************
 ***************************************** Group 10 *******************************************************************************************
 *********************************************************************************************************************************************/

static entryRctInit_s RCT_group10_init[] =
{
    {"gr10_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_1.txt",                 false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"Docs/gr10_A.txt",            false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_2.txt",                 false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"Docs/gr10_B.txt",            false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}}} ;

static entryDataInit_s dataKeys_Group10_Init[] = {};

static entryDataInit_s dataFiles_Group10_Init[] = {};

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
    sizeof(dataFiles_Group10_Init)/sizeof(dataFiles_Group10_Init[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** Group 20 *******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_group20_init[] =
{
    {"gr20_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"gr20_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"}, {NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_Group20_Init[] = {};

static entryDataInit_s dataFiles_Group20_Init[] ={};

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
    sizeof(dataFiles_Group20_Init)/sizeof(dataFiles_Group20_Init[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** App1 *******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_App1_init[] =
{
    {"App1_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_App1_Init[] = {} ;

static entryDataInit_s dataFiles_App1_Init[] = {};

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
    sizeof(dataFiles_App1_Init)/sizeof(dataFiles_App1_Init[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** App2*******************************************************************************************
 *********************************************************************************************************************************************/
static entryRctInit_s RCT_App2_init[] =
{
    {"App2_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"App2_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"doc1.txt",                   false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",              false,  {PersistencePolicy_wt,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"docA.txt",                   false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}},
    {"Docs/docB.txt",              false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App2"}, {NIL},{NIL}}}
} ;

static entryDataInit_s dataKeys_App2_Init[] = {};

static entryDataInit_s dataFiles_App2_Init[] = {};

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
    sizeof(dataFiles_App2_Init)/sizeof(dataFiles_App2_Init[0])
} ;


//===================================================================================================================
// BACKUP CONTENT
//===================================================================================================================
// the backup content is obtained trough the backup process

//===================================================================================================================
// EXPECTED
//===================================================================================================================

expected_key_data_localDB_s expected_key_data_after_restore_All_InitialContent[16 + 16 + 16 + 32+ 32] =
{
    /**********************************************************************************************************************************************
    ***************************************** public *******************************************************************************************
    *********************************************************************************************************************************************/ 
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                     PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true, "Data>>/pubSettingA",               sizeof("Data>>/pubSettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",       PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true, "Data>>/pubSettingB::user2::seat1", sizeof("Data>>/pubSettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",       PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true, "Data>>/pubSettingB::user2:seat2",  sizeof("Data>>/pubSettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                                     PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true, "Data>>/pubSettingC",               sizeof("Data>>/pubSettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",                                 PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,"Data>>/pubSetting/ABC::user1",      sizeof("Data>>/pubSetting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",                                 PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,"Data>>/pubSetting/ABC::user2",      sizeof("Data>>/pubSetting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",                                 PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,"Data>>/pubSetting/ABC::user3",      sizeof("Data>>/pubSetting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",                                 PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,"Data>>/pubSetting/ABC::user4",      sizeof("Data>>/pubSetting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                     PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSettingD",               sizeof("Data>>/pubSettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",       PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSettingE::user2:seat1",  sizeof("Data>>/pubSettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",       PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSettingE::user2:seat2",  sizeof("Data>>/pubSettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                     PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSettingF",               sizeof("Data>>/pubSettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                                 PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSetting/DEF::user1",     sizeof("Data>>/pubSetting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                                 PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSetting/DEF::user2",     sizeof("Data>>/pubSetting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                                 PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSetting/DEF::user3",     sizeof("Data>>/pubSetting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                                 PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/pubSetting/DEF::user4",     sizeof("Data>>/pubSetting/DEF::user4")},


    /**********************************************************************************************************************************************
    ***************************************** Group 10 *******************************************************************************************
    *********************************************************************************************************************************************/
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingA",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_SettingA",                 sizeof("Data>>/gr10_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB",    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_SettingB::user2::seat1",   sizeof("Data>>/gr10_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingB",    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_SettingB::user2:seat2",    sizeof("Data>>/gr10_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingC",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_SettingC",                 sizeof("Data>>/gr10_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_Setting/ABC::user1",       sizeof("Data>>/gr10_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_Setting/ABC::user2",       sizeof("Data>>/gr10_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_Setting/ABC::user3",       sizeof("Data>>/gr10_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,      true, "Data>>/gr10_Setting/ABC::user4",       sizeof("Data>>/gr10_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingD",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_SettingD",                 sizeof("Data>>/gr10_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingE",    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_SettingE::user2:seat1",    sizeof("Data>>/gr10_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingE",    PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_SettingE::user2:seat2",    sizeof("Data>>/gr10_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingF",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_SettingF",                 sizeof("Data>>/gr10_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_Setting/DEF::user1",       sizeof("Data>>/gr10_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_Setting/DEF::user2",       sizeof("Data>>/gr10_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_Setting/DEF::user3",       sizeof("Data>>/gr10_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,   true, "Data>>/gr10_Setting/DEF::user4",       sizeof("Data>>/gr10_Setting/DEF::user4")},

    /**********************************************************************************************************************************************
	***************************************** Group 20 *******************************************************************************************
	*********************************************************************************************************************************************/
	{ PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingA",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_SettingA",                 sizeof("Data>>/gr20_SettingA")},
	{ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingB",    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_SettingB::user2::seat1",   sizeof("Data>>/gr20_SettingB::user2::seat1")},
	{ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingB",    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_SettingB::user2:seat2",    sizeof("Data>>/gr20_SettingB::user2:seat2")},
	{ PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingC",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_SettingC",                 sizeof("Data>>/gr20_SettingC")},
	{ PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_Setting/ABC::user1",       sizeof("Data>>/gr20_Setting/ABC::user1")},
	{ PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_Setting/ABC::user2",       sizeof("Data>>/gr20_Setting/ABC::user2")},
	{ PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_Setting/ABC::user3",       sizeof("Data>>/gr20_Setting/ABC::user3")},
	{ PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/ABC",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true, "Data>>/gr20_Setting/ABC::user4",       sizeof("Data>>/gr20_Setting/ABC::user4")},
	{ PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingD",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_SettingD",                 sizeof("Data>>/gr20_SettingD")},
	{ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingE",    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_SettingE::user2:seat1",    sizeof("Data>>/gr20_SettingE::user2:seat1")},
	{ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingE",    PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_SettingE::user2:seat2",    sizeof("Data>>/gr20_SettingE::user2:seat2")},
	{ PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingF",                                  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_SettingF",                 sizeof("Data>>/gr20_SettingF")},
	{ PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_Setting/DEF::user1",       sizeof("Data>>/gr20_Setting/DEF::user1")},
	{ PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_Setting/DEF::user2",       sizeof("Data>>/gr20_Setting/DEF::user2")},
	{ PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_Setting/DEF::user3",       sizeof("Data>>/gr20_Setting/DEF::user3")},
	{ PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/DEF",                              PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true, "Data>>/gr20_Setting/DEF::user4",       sizeof("Data>>/gr20_Setting/DEF::user4")},

    /**********************************************************************************************************************************************
    ***************************************** App1 *******************************************************************************************
    *********************************************************************************************************************************************/    
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App1_SettingA",                  sizeof("Data>>/App1_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true, "Data>>/App1_SettingB::user2::seat1",   sizeof("Data>>/App1_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true, "Data>>/App1_SettingB::user2:seat2",    sizeof("Data>>/App1_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App1_SettingC",                  sizeof("Data>>/App1_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App1_Setting/ABC::user1",        sizeof("Data>>/App1_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App1_Setting/ABC::user2",        sizeof("Data>>/App1_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App1_Setting/ABC::user3",        sizeof("Data>>/App1_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App1_Setting/ABC::user4",        sizeof("Data>>/App1_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_SettingD",                 sizeof("Data>>/App1_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_SettingE::user2:seat1",    sizeof("Data>>/App1_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_SettingE::user2:seat2",    sizeof("Data>>/App1_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_SettingF",                 sizeof("Data>>/App1_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_Setting/DEF::user1",       sizeof("Data>>/App1_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_Setting/DEF::user2",       sizeof("Data>>/App1_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_Setting/DEF::user3",       sizeof("Data>>/App1_Setting/DEF::user4")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App1_Setting/DEF::user4",       sizeof("Data>>/App1_Setting/DEF::user3")},

    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_SettingA",                 sizeof("Data>>/App1_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_SettingB::user2::seat1",   sizeof("Data>>/App1_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_SettingB::user2:seat2",    sizeof("Data>>/App1_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_SettingC",                 sizeof("Data>>/App1_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_Setting/ABC::user1",       sizeof("Data>>/App1_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_Setting/ABC::user2",       sizeof("Data>>/App1_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_Setting/ABC::user3",       sizeof("Data>>/App1_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App1_Setting/ABC::user4",       sizeof("Data>>/App1_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false,"Data>>/App1_SettingD",                 sizeof("Data>>/App1_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false,"Data>>/App1_SettingE::user2:seat1",    sizeof("Data>>/App1_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false,"Data>>/App1_SettingE::user2:seat2",    sizeof("Data>>/App1_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false,"Data>>/App1_SettingF",                 sizeof("Data>>/App1_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false,"Data>>/App1_Setting/DEF::user1",       sizeof("Data>>/App1_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false,"Data>>/App1_Setting/DEF::user2",       sizeof("Data>>/App1_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App1_Setting/DEF::user3",      sizeof("Data>>/App1_Setting/DEF::user4")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App1_Setting/DEF::user4",      sizeof("Data>>/App1_Setting/DEF::user3")},


    /**********************************************************************************************************************************************
    ***************************************** App2*******************************************************************************************
    *********************************************************************************************************************************************/
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingA",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App2_SettingA",                 sizeof("Data>>/App1_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",    PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true, "Data>>/App2_SettingB::user2::seat1",   sizeof("Data>>/App2_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",    PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true, "Data>>/App2_SettingB::user2:seat2",    sizeof("Data>>/App2_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingC",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App2_SettingC",                 sizeof("Data>>/App2_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App2_Setting/ABC::user1",       sizeof("Data>>/App2_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App2_Setting/ABC::user2",       sizeof("Data>>/App2_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App2_Setting/ABC::user3",       sizeof("Data>>/App2_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         true,"Data>>/App2_Setting/ABC::user4",       sizeof("Data>>/App2_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_SettingD",                 sizeof("Data>>/App2_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_SettingE::user2:seat1",    sizeof("Data>>/App2_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_SettingE::user2:seat2",    sizeof("Data>>/App2_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_SettingF",                 sizeof("Data>>/App2_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_Setting/DEF::user1",       sizeof("Data>>/App2_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_Setting/DEF::user2",       sizeof("Data>>/App2_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_Setting/DEF::user3",       sizeof("Data>>/App2_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   true, "Data>>/App2_Setting/DEF::user4",       sizeof("Data>>/App2_Setting/DEF::user4")},

    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingA",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App2_SettingA",                 sizeof("Data>>/App1_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false, "Data>>/App2_SettingB::user2::seat1",   sizeof("Data>>/App2_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",    PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false, "Data>>/App2_SettingB::user2:seat2",    sizeof("Data>>/App2_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingC",                                  PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App2_SettingC",                 sizeof("Data>>/App2_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App2_Setting/ABC::user1",       sizeof("Data>>/App2_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App2_Setting/ABC::user2",       sizeof("Data>>/App2_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App2_Setting/ABC::user3",       sizeof("Data>>/App2_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/ABC",                              PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,   false,"Data>>/App2_Setting/ABC::user4",       sizeof("Data>>/App2_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_SettingD",                 sizeof("Data>>/App2_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",    PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_SettingE::user2:seat1",    sizeof("Data>>/App2_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",    PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_SettingE::user2:seat2",    sizeof("Data>>/App2_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_SettingF",                 sizeof("Data>>/App2_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_Setting/DEF::user1",       sizeof("Data>>/App2_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_Setting/DEF::user2",       sizeof("Data>>/App2_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_Setting/DEF::user3",       sizeof("Data>>/App2_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,         false, "Data>>/App2_Setting/DEF::user4",       sizeof("Data>>/App2_Setting/DEF::user4")}
} ;

expected_file_data_s expected_file_data_after_restore_All_InitialContent[10 + 10 + 10 + 10 + 10] =
{
    /**********************************************************************************************************************************************
    ***************************************** public *******************************************************************************************
    *********************************************************************************************************************************************/ 
	{ PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/doc1.txt",                                         true, "File>>/doc1.txt"              , sizeof("File>>/doc1.txt")},
    { PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/Docs/doc2.txt",                                    true, "File>>/Docs/doc2.txt"         , sizeof("File>>/Docs/doc2.txt")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/1/docA.txt",                                    true, "File>>/docA.txt::user1"       , sizeof("File>>/docA.txt::user1")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2/docA.txt",                                    true, "File>>/docA.txt::user2"       , sizeof("File>>/docA.txt::user2")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/3/docA.txt",                                    true, "File>>/docA.txt::user3"       , sizeof("File>>/docA.txt::user3")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/4/docA.txt",                                    true, "File>>/docA.txt::user4"       , sizeof("File>>/docA.txt::user4")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat1" , sizeof("File>>/docB.txt::user2:seat1")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat2" , sizeof("File>>/docB.txt::user2:seat2")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat3" , sizeof("File>>/docB.txt::user2:seat3")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat4" , sizeof("File>>/docB.txt::user2:seat4")},


    /**********************************************************************************************************************************************
	***************************************** Group 10 *******************************************************************************************
	*********************************************************************************************************************************************/
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                  true, "File>>gr10_>>/gr10_1.txt"                   ,  sizeof("File>>gr10_>>/gr10_1.txt"                     )},
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_A.txt",                             true, "File>>gr10_>>/Docs/gr10_A.txt"              ,  sizeof("File>>gr10_>>/Docs/gr10_A.txt"                )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user1"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user1"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user2"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user2"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user3"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user3"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user4"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user4"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat1"        )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat2"        )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat3"        )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat4"        )},


    /**********************************************************************************************************************************************
	***************************************** Group 20 *******************************************************************************************
	*********************************************************************************************************************************************/
	{ PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      true, "File>>gr20_>>/doc1.txt"       ,        sizeof("File>>gr20_>>/doc1.txt")},
	{ PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 true, "File>>gr20_>>/Docs/doc2.txt"  ,        sizeof("File>>gr20_>>/Docs/doc2.txt")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                   true, "File>>gr20_>>/docA.txt::user1",        sizeof("File>>gr20_>>/docA.txt::user1")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                   true, "File>>gr20_>>/docA.txt::user2",        sizeof("File>>gr20_>>/docA.txt::user2")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                   true, "File>>gr20_>>/docA.txt::user3",        sizeof("File>>gr20_>>/docA.txt::user3")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                   true, "File>>gr20_>>/docA.txt::user4",        sizeof("File>>gr20_>>/docA.txt::user4")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt", true, "File>>gr20_>>/docB.txt::user2:seat1" , sizeof("File>>gr20_>>/docB.txt::user2:seat1")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt", true, "File>>gr20_>>/docB.txt::user2:seat2" , sizeof("File>>gr20_>>/docB.txt::user2:seat2")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt", true, "File>>gr20_>>/docB.txt::user2:seat3" , sizeof("File>>gr20_>>/docB.txt::user2:seat3")},
	{ PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt", true, "File>>gr20_>>/docB.txt::user2:seat4" , sizeof("File>>gr20_>>/docB.txt::user2:seat4")},


	/**********************************************************************************************************************************************
    ***************************************** App1 *******************************************************************************************
    *********************************************************************************************************************************************/
	{ PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      true, "File>>App1>>/doc1.txt"              , sizeof("File>>App1>>/doc1.txt"                )},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 true, "File>>App1>>/Docs/doc2.txt"         , sizeof("File>>App1>>/Docs/doc2.txt"           )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                   true, "File>>App1>>/docA.txt::user1"       , sizeof("File>>App1>>/docA.txt::user1"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                   true, "File>>App1>>/docA.txt::user2"       , sizeof("File>>App1>>/docA.txt::user2"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                   true, "File>>App1>>/docA.txt::user3"       , sizeof("File>>App1>>/docA.txt::user3"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                   true, "File>>App1>>/docA.txt::user4"       , sizeof("File>>App1>>/docA.txt::user4"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt", true, "File>>App1>>/docB.txt::user2:seat1" , sizeof("File>>App1>>/docB.txt::user2:seat1"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt", true, "File>>App1>>/docB.txt::user2:seat2" , sizeof("File>>App1>>/docB.txt::user2:seat2"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt", true, "File>>App1>>/docB.txt::user2:seat3" , sizeof("File>>App1>>/docB.txt::user2:seat3"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt", true, "File>>App1>>/docB.txt::user2:seat4" , sizeof("File>>App1>>/docB.txt::user2:seat4"   )},


    /**********************************************************************************************************************************************
    ***************************************** App2*******************************************************************************************
    *********************************************************************************************************************************************/
    { PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      true, "File>>App2>>/doc1.txt"              , sizeof("File>>App2>>/doc1.txt")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 true, "File>>App2>>/Docs/doc2.txt"         , sizeof("File>>App2>>/Docs/doc2.txt")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                   true, "File>>App2>>/docA.txt::user1"       , sizeof("File>>App2>>/docA.txt::user1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                   true, "File>>App2>>/docA.txt::user2"       , sizeof("File>>App2>>/docA.txt::user2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                   true, "File>>App2>>/docA.txt::user3"       , sizeof("File>>App2>>/docA.txt::user3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                   true, "File>>App2>>/docA.txt::user4"       , sizeof("File>>App2>>/docA.txt::user4")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt", true, "File>>App2>>/docB.txt::user2:seat1" , sizeof("File>>App2>>/docB.txt::user2:seat1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt", true, "File>>App2>>/docB.txt::user2:seat2" , sizeof("File>>App2>>/docB.txt::user2:seat2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt", true, "File>>App2>>/docB.txt::user2:seat3" , sizeof("File>>App2>>/docB.txt::user2:seat3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt", true, "File>>App2>>/docB.txt::user2:seat4" , sizeof("File>>App2>>/docB.txt::user2:seat4")}
} ;


//===================================================================================================================

static bool_t EraseReferenceData(void)
{
    bool_t bEverythingOK = true ;
    pstr_t referenceDataPath =  PERS_ORG_LOCAL_APP_CACHE_PATH_  ;

    sint_t result = DeleteFolderContent(referenceDataPath) ;
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


bool_t Test_Recover_All_InitialContent(sint_t type, void* pv)
{
    bool_t bEverythingOK = true ;
    long lTemp ;

    str_t  pchBackupFilePath [PATH_ABS_MAX_SIZE];

    /* Create backup */
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Create backup to : "),
        															DLT_STRING(BACKUP_FOLDER),
        															DLT_STRING("..."));
    lTemp = persAdminDataBackupCreate(		PersASSelectionType_All,
    										BACKUP_FOLDER,
    										"",
    										USER_DONT_CARE,
    										SEAT_DONT_CARE);

    bEverythingOK = (lTemp >= 0) ? true : false ;

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Test_Recover_All_InitialContent: persAdminDataBackupCreate() - "),
    																DLT_STRING(bEverythingOK ? "OK" : "FAILED"));

    if(true == bEverythingOK)
    {
		DeleteFolder(BACKUP_CONTENT_FOLDER);


		/* Reset the destination content (except the RCT files) */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Erase reference data..."));

		bEverythingOK = EraseReferenceData();

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Test_Recover_All_InitialContent: EraseReferenceData() - "),
																		DLT_STRING(bEverythingOK ? "OK" : "FAILED"));
    }

    if(true == bEverythingOK)
    {
		(void)snprintf(pchBackupFilePath, sizeof(pchBackupFilePath), "%s%s", "all", BACKUP_FORMAT);

		/* Restore content */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Restore backup from : "),
																		DLT_STRING(pchBackupFilePath),
																		DLT_STRING("..."));

		lTemp = persAdminDataBackupRecovery(   PersASSelectionType_All,
												       pchBackupFilePath,
												       "",
												       USER_DONT_CARE,
												       SEAT_DONT_CARE);
		bEverythingOK = (lTemp >= 0) ? true : false ;

		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Test_Recover_All_InitialContent: data_backup_recovery() - "),
																		DLT_STRING(bEverythingOK ? "OK" : "FAILED"));
    }

    return bEverythingOK ;
} /* Test_Recover_All_InitialContent */
