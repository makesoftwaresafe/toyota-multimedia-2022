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
#include "stdio.h"
#include "string.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include "persistence_admin_service.h"
#include "test_PAS.h"
#include "test_pas_data_backup_recovery.h"

/* compress/uncompress */
#include "archive.h"
#include "archive_entry.h"

#define READ_BUFFER_LENGTH (16384)

#define PATH_ABS_MAX_SIZE  ( 512)

#define BACKUP_FORMAT      (".arch.tar.gz")

#define File_t PersistenceResourceType_file
#define Key_t PersistenceResourceType_key

static sint_t persadmin_compress(pstr_t compressTo, pstr_t compressFrom) ;

//===================================================================================================================
// BACKUP CONTENT
//===================================================================================================================
entryRctInit_s RCT_public_backup_content[] =
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
} ;    


entryDataInit_s dataKeysPublic_backup_content[] =
{
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingK",                                      PersistencePolicy_wt, 0, 0, "Data>>/pubSettingK"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",        PersistencePolicy_wt, 2, 1, "Data>>/pubSettingB::user2::seat1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",        PersistencePolicy_wt, 2, 2, "Data>>/pubSettingB::user2:seat2"},
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingL",                                      PersistencePolicy_wt, 0, 0, "Data>>/pubSettingL"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/KBL",                                  PersistencePolicy_wt, 1, 0, "Data>>/pubSetting/KBL::user1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/KBL",                                  PersistencePolicy_wt, 2, 0, "Data>>/pubSetting/KBL::user2"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/KBL",                                  PersistencePolicy_wt, 3, 0, "Data>>/pubSetting/KBL::user3"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/KBL",                                  PersistencePolicy_wt, 4, 0, "Data>>/pubSetting/KBL::user4"},
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                      PersistencePolicy_wc, 0, 0, "Data>>/pubSettingD"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",        PersistencePolicy_wc, 2, 1, "Data>>/pubSettingE::user2:seat1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",        PersistencePolicy_wc, 2, 2, "Data>>/pubSettingE::user2:seat2"},
    {0,  PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                      PersistencePolicy_wc, 0, 0, "Data>>/pubSettingF"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                                  PersistencePolicy_wc, 1, 0, "Data>>/pubSetting/DEF::user1"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                                  PersistencePolicy_wc, 1, 0, "Data>>/pubSetting/DEF::user2"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                                  PersistencePolicy_wc, 3, 0, "Data>>/pubSetting/DEF::user3"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                                  PersistencePolicy_wc, 4, 0, "Data>>/pubSetting/DEF::user4"},
    {0,  PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/XYZ",                                  PersistencePolicy_wc, 4, 0, "Data>>/pubSetting/XYZ::user4"}
} ;


entryDataInit_s dataFilesPublic_mnt_c_backup_content[] =
{
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_NODE_FOLDER_NAME"/doc1.txt",                                        PersistencePolicy_wt, 0, 0, "File>>/doc1.txt"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_NODE_FOLDER_NAME"/Docs/doc2.txt",                                   PersistencePolicy_wt, 0, 0, "File>>/Docs/doc2.txt"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/1/docK.txt",                                      PersistencePolicy_wc, 1, 0, "File>>/docK.txt::user1"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2/docK.txt",                                      PersistencePolicy_wc, 2, 0, "File>>/docK.txt::user2"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2/docT.txt",                                      PersistencePolicy_wc, 2, 0, "File>>/docT.txt::user2"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/3/docK.txt",                                      PersistencePolicy_wc, 3, 0, "File>>/docK.txt::user3"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/4/docK.txt",                                      PersistencePolicy_wc, 4, 0, "File>>/docK.txt::user4"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",    PersistencePolicy_wc, 2, 1, "File>>/docB.txt::user2:seat1"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",    PersistencePolicy_wc, 2, 2, "File>>/docB.txt::user2:seat2"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",    PersistencePolicy_wc, 2, 3, "File>>/docB.txt::user2:seat3"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",    PersistencePolicy_wc, 2, 4, "File>>/docB.txt::user2:seat4"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",    PersistencePolicy_wc, 2, 4, "File>>/docC.txt::user2:seat4"}
};

entryDataInit_s dataFilesPublic_mnt_wt_backup_content[] =
{
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      PersistencePolicy_wt, 0, 0, "File>>/doc1.txt"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 PersistencePolicy_wt, 0, 0, "File>>/Docs/doc2.txt"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                      PersistencePolicy_wc, 1, 0, "File>>/docK.txt::user1"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                      PersistencePolicy_wc, 2, 0, "File>>/docK.txt::user2"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2/docT.txt",                                      PersistencePolicy_wc, 2, 0, "File>>/docT.txt::user2"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                      PersistencePolicy_wc, 3, 0, "File>>/docK.txt::user3"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                      PersistencePolicy_wc, 4, 0, "File>>/docK.txt::user4"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",    PersistencePolicy_wc, 2, 1, "File>>/docB.txt::user2:seat1"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",    PersistencePolicy_wc, 2, 2, "File>>/docB.txt::user2:seat2"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",    PersistencePolicy_wc, 2, 3, "File>>/docB.txt::user2:seat3"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",    PersistencePolicy_wc, 2, 4, "File>>/docB.txt::user2:seat4"},
    {0, BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",    PersistencePolicy_wc, 2, 4, "File>>/docC.txt::user2:seat4"}
};


dataInit_s sSharedPubData_mnt_c_backup_content =
{
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ ,
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_public_backup_content,
    sizeof(RCT_public_backup_content)/sizeof(RCT_public_backup_content[0]),
    dataKeysPublic_backup_content,
    sizeof(dataKeysPublic_backup_content)/sizeof(dataKeysPublic_backup_content[0]),
    dataFilesPublic_mnt_c_backup_content,
    sizeof(dataFilesPublic_mnt_c_backup_content)/sizeof(dataFilesPublic_mnt_c_backup_content[0])
} ;

dataInit_s sSharedPubData_mnt_wt_backup_content =
{
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ ,
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_public_backup_content,
    sizeof(RCT_public_backup_content)/sizeof(RCT_public_backup_content[0]),
    dataKeysPublic_backup_content,
    sizeof(dataKeysPublic_backup_content)/sizeof(dataKeysPublic_backup_content[0]),
    dataFilesPublic_mnt_wt_backup_content,
    sizeof(dataFilesPublic_mnt_wt_backup_content)/sizeof(dataFilesPublic_mnt_wt_backup_content[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** group 10 *******************************************************************************************
 *********************************************************************************************************************************************/
entryRctInit_s RCT_group10_backup_content[] =
{
    {"gr10_SettingA",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingB",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingC",              true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_Setting/ABC",           true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingD",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingE",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingF",              true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_Setting/DEF",           true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_1.txt",                 false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"Docs/gr10_A.txt",            false,  {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_2.txt",                 false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"Docs/gr10_B.txt",            false,  {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}}
} ;


entryDataInit_s dataKeys_group10_backup_content[] =
{
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingK",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr10_SettingK"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/gr10_SettingB::user2::seat1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/gr10_SettingB::user2:seat2"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingL",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr10_SettingL"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/KBL",                                     PersistencePolicy_wt, 1, 0, "Data>>/gr10_Setting/KBL::user1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/KBL",                                     PersistencePolicy_wt, 2, 0, "Data>>/gr10_Setting/KBL::user2"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/KBL",                                     PersistencePolicy_wt, 3, 0, "Data>>/gr10_Setting/KBL::user3"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/KBL",                                     PersistencePolicy_wt, 4, 0, "Data>>/gr10_Setting/KBL::user4"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr10_SettingD"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingZ",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr10_SettingZ"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/gr10_SettingE::user2:seat1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/gr10_SettingE::user2:seat2"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr10_SettingF"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/gr10_Setting/DEF::user1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/gr10_Setting/DEF::user2"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/gr10_Setting/DEF::user3"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/gr10_Setting/DEF::user4"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/PRT",                                     PersistencePolicy_wc, 4, 0, "Data>>/gr10_Setting/PRT::user4"}
} ;


entryDataInit_s dataFiles_group10_mnt_c_backup_content[] =
{
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                       PersistencePolicy_wt, 0, 0, "File>>gr10_>>/gr10_1.txt"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_2.txt",                                  PersistencePolicy_wt, 0, 0, "File>>gr10_>>/Docs/gr10_2.txt"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                         PersistencePolicy_wc, 1, 0, "File>>gr10_>>/docK.txt::user1"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                         PersistencePolicy_wc, 2, 0, "File>>gr10_>>/docK.txt::user2"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/docT.txt",                                         PersistencePolicy_wc, 2, 0, "File>>gr10_>>/docK.txt::user2"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                         PersistencePolicy_wc, 3, 0, "File>>gr10_>>/docK.txt::user3"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                         PersistencePolicy_wc, 4, 0, "File>>gr10_>>/docK.txt::user4"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 1, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 2, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 3, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 4, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docT.txt",       PersistencePolicy_wc, 2, 4, "File>>gr10_>>/docB.txt::user2:seat4"}
};


entryDataInit_s dataFiles_group10_mnt_wt_backup_content[] =
{
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                       PersistencePolicy_wt, 0, 0, "File>>gr10_>>/gr10_1.txt"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_2.txt",                                  PersistencePolicy_wt, 0, 0, "File>>gr10_>>/Docs/gr10_2.txt"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                         PersistencePolicy_wc, 1, 0, "File>>gr10_>>/docK.txt::user1"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                         PersistencePolicy_wc, 2, 0, "File>>gr10_>>/docK.txt::user2"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/docT.txt",                                         PersistencePolicy_wc, 2, 0, "File>>gr10_>>/docK.txt::user2"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                         PersistencePolicy_wc, 3, 0, "File>>gr10_>>/docK.txt::user3"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                         PersistencePolicy_wc, 4, 0, "File>>gr10_>>/docK.txt::user4"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 1, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 2, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 3, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",     PersistencePolicy_wc, 2, 4, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4"},
    {0x10, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docT.txt",       PersistencePolicy_wc, 2, 4, "File>>gr10_>>/docB.txt::user2:seat4"}
};

dataInit_s sShared_Group10_Data_mnt_c_backup_content =
{
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10/",
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_group10_backup_content,
    sizeof(RCT_group10_backup_content)/sizeof(RCT_group10_backup_content[0]),
    dataKeys_group10_backup_content,
    sizeof(dataKeys_group10_backup_content)/sizeof(dataKeys_group10_backup_content[0]),
    dataFiles_group10_mnt_c_backup_content,
    sizeof(dataFiles_group10_mnt_c_backup_content)/sizeof(dataFiles_group10_mnt_c_backup_content[0])
} ;


dataInit_s sShared_Group10_Data_mnt_wt_backup_content =
{
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10/",
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_group10_backup_content,
    sizeof(RCT_group10_backup_content)/sizeof(RCT_group10_backup_content[0]),
    dataKeys_group10_backup_content,
    sizeof(dataKeys_group10_backup_content)/sizeof(dataKeys_group10_backup_content[0]),
    dataFiles_group10_mnt_wt_backup_content,
    sizeof(dataFiles_group10_mnt_wt_backup_content)/sizeof(dataFiles_group10_mnt_wt_backup_content[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** group 20 *******************************************************************************************
 *********************************************************************************************************************************************/
entryRctInit_s RCT_group20_backup_content[] =
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


entryDataInit_s dataKeys_group20_backup_content[] =
{
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingK",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr20_SettingK"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/gr20_SettingB::user2::seat1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/gr20_SettingB::user2:seat2"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingL",                                         PersistencePolicy_wt, 0, 0, "Data>>/gr20_SettingL"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/KBL",                                     PersistencePolicy_wt, 1, 0, "Data>>/gr20_Setting/KBL::user1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/KBL",                                     PersistencePolicy_wt, 2, 0, "Data>>/gr20_Setting/KBL::user2"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/KBL",                                     PersistencePolicy_wt, 3, 0, "Data>>/gr20_Setting/KBL::user3"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/KBL",                                     PersistencePolicy_wt, 4, 0, "Data>>/gr20_Setting/KBL::user4"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr20_SettingD"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingZ",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr20_SettingZ"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/gr20_SettingE::user2:seat1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/gr20_SettingE::user2:seat2"},
    {0x10,  PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/gr20_SettingF"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/gr20_Setting/DEF::user1"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/gr20_Setting/DEF::user2"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/gr20_Setting/DEF::user3"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/gr20_Setting/DEF::user4"},
    {0x10,  PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/PRT",                                     PersistencePolicy_wc, 4, 0, "Data>>/gr20_Setting/PRT::user4"}
} ;

entryDataInit_s dataFiles_group20_mnt_c_backup_content[] =
{
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                         PersistencePolicy_wt, 0, 0, "File>>gr20_>>/doc1.txt"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                    PersistencePolicy_wt, 0, 0, "File>>gr20_>>/Docs/doc2.txt"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                         PersistencePolicy_wc, 1, 0, "File>>gr20_>>/docK.txt::user1"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                         PersistencePolicy_wc, 2, 0, "File>>gr20_>>/docK.txt::user2"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                         PersistencePolicy_wc, 3, 0, "File>>gr20_>>/docK.txt::user3"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                         PersistencePolicy_wc, 4, 0, "File>>gr20_>>/docK.txt::user4"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",       PersistencePolicy_wc, 2, 1, "File>>gr20_>>/docB.txt::user2:seat1"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",       PersistencePolicy_wc, 2, 2, "File>>gr20_>>/docB.txt::user2:seat2"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",       PersistencePolicy_wc, 2, 3, "File>>gr20_>>/docB.txt::user2:seat3"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",       PersistencePolicy_wc, 2, 4, "File>>gr20_>>/docB.txt::user2:seat4"}
};

entryDataInit_s dataFiles_group20_mnt_wt_backup_content[] =
{
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                         PersistencePolicy_wt, 0, 0, "File>>gr20_>>/doc1.txt"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                    PersistencePolicy_wt, 0, 0, "File>>gr20_>>/Docs/doc2.txt"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                         PersistencePolicy_wc, 1, 0, "File>>gr20_>>/docK.txt::user1"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                         PersistencePolicy_wc, 2, 0, "File>>gr20_>>/docK.txt::user2"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                         PersistencePolicy_wc, 3, 0, "File>>gr20_>>/docK.txt::user3"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                         PersistencePolicy_wc, 4, 0, "File>>gr20_>>/docK.txt::user4"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",       PersistencePolicy_wc, 2, 1, "File>>gr20_>>/docB.txt::user2:seat1"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",       PersistencePolicy_wc, 2, 2, "File>>gr20_>>/docB.txt::user2:seat2"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",       PersistencePolicy_wc, 2, 3, "File>>gr20_>>/docB.txt::user2:seat3"},
    {0x20, BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",       PersistencePolicy_wc, 2, 4, "File>>gr20_>>/docB.txt::user2:seat4"}
};

dataInit_s sShared_Group20_Data_mnt_c_backup_content =
{
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20/",
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_CACHE_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_group20_backup_content,
    sizeof(RCT_group20_backup_content)/sizeof(RCT_group20_backup_content[0]),
    dataKeys_group20_backup_content,
    sizeof(dataKeys_group20_backup_content)/sizeof(dataKeys_group20_backup_content[0]),
    dataFiles_group20_mnt_c_backup_content,
    sizeof(dataFiles_group20_mnt_c_backup_content)/sizeof(dataFiles_group20_mnt_c_backup_content[0])
} ;


dataInit_s sShared_Group20_Data_mnt_wt_backup_content =
{
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20/",
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_group20_backup_content,
    sizeof(RCT_group20_backup_content)/sizeof(RCT_group20_backup_content[0]),
    dataKeys_group20_backup_content,
    sizeof(dataKeys_group20_backup_content)/sizeof(dataKeys_group20_backup_content[0]),
    dataFiles_group20_mnt_wt_backup_content,
    sizeof(dataFiles_group20_mnt_wt_backup_content)/sizeof(dataFiles_group20_mnt_wt_backup_content[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** App1 *******************************************************************************************
 *********************************************************************************************************************************************/
entryRctInit_s RCT_App1_backup_content[] =
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


entryDataInit_s dataKeys_App1_backup_content[] =
{
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingK",                                         PersistencePolicy_wt, 0, 0, "Data>>/App1_SettingK"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/App1_SettingB::user2::seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/App1_SettingB::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingL",                                         PersistencePolicy_wt, 0, 0, "Data>>/App1_SettingL"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/KBL",                                     PersistencePolicy_wt, 1, 0, "Data>>/App1_Setting/KBL::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/KBL",                                     PersistencePolicy_wt, 2, 0, "Data>>/App1_Setting/KBL::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/KBL",                                     PersistencePolicy_wt, 3, 0, "Data>>/App1_Setting/KBL::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/KBL",                                     PersistencePolicy_wt, 4, 0, "Data>>/App1_Setting/KBL::user4"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/App1_SettingD"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/App1_SettingE::user2:seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/App1_SettingE::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/App1_SettingF"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/App1_Setting/DEF::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/App1_Setting/DEF::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/App1_Setting/DEF::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/App1_Setting/DEF::user4"}
} ;

entryDataInit_s dataFiles_App1_mnt_c_backup_content[] =
{
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                        PersistencePolicy_wt, 0, 0, "File>>App1>>/doc1.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                   PersistencePolicy_wt, 0, 0, "File>>App1>>/Docs/doc2.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                        PersistencePolicy_wc, 1, 0, "File>>App1>>/docK.txt::user1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                        PersistencePolicy_wc, 2, 0, "File>>App1>>/docK.txt::user2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                        PersistencePolicy_wc, 3, 0, "File>>App1>>/docK.txt::user3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                        PersistencePolicy_wc, 4, 0, "File>>App1>>/docK.txt::user4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>App1>>/docB.txt::user2:seat1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>App1>>/docB.txt::user2:seat2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>App1>>/docB.txt::user2:seat3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docB.txt::user2:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docC.txt::user2:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docD.txt::user2:seat4"}
};


entryDataInit_s dataFiles_App1_mnt_wt_backup_content[] =
{
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                        PersistencePolicy_wt, 0, 0, "File>>App1>>/doc1.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                   PersistencePolicy_wt, 0, 0, "File>>App1>>/Docs/doc2.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                        PersistencePolicy_wc, 1, 0, "File>>App1>>/docK.txt::user1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                        PersistencePolicy_wc, 2, 0, "File>>App1>>/docK.txt::user2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                        PersistencePolicy_wc, 3, 0, "File>>App1>>/docK.txt::user3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                        PersistencePolicy_wc, 4, 0, "File>>App1>>/docK.txt::user4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>App1>>/docB.txt::user2:seat1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>App1>>/docB.txt::user2:seat2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>App1>>/docB.txt::user2:seat3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docB.txt::user2:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docC.txt::user2:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt",      PersistencePolicy_wc, 2, 4, "File>>App1>>/docD.txt::user2:seat4"}
};


dataInit_s s_App1_Data_mnt_c_backup_content =
{
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/",
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,
    dbType_local,
    RCT_App1_backup_content,
    sizeof(RCT_App1_backup_content)/sizeof(RCT_App1_backup_content[0]),
    dataKeys_App1_backup_content,
    sizeof(dataKeys_App1_backup_content)/sizeof(dataKeys_App1_backup_content[0]),
    dataFiles_App1_mnt_c_backup_content,
    sizeof(dataFiles_App1_mnt_c_backup_content)/sizeof(dataFiles_App1_mnt_c_backup_content[0])
};


dataInit_s s_App1_Data_mnt_wt_backup_content =
{
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1/",
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,
    dbType_local,
    RCT_App1_backup_content,
    sizeof(RCT_App1_backup_content)/sizeof(RCT_App1_backup_content[0]),
    dataKeys_App1_backup_content,
    sizeof(dataKeys_App1_backup_content)/sizeof(dataKeys_App1_backup_content[0]),
    dataFiles_App1_mnt_wt_backup_content,
    sizeof(dataFiles_App1_mnt_wt_backup_content)/sizeof(dataFiles_App1_mnt_wt_backup_content[0])
} ;

/**********************************************************************************************************************************************
 ***************************************** App2*******************************************************************************************
 *********************************************************************************************************************************************/
entryRctInit_s RCT_App2_backup_content[] =
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


entryDataInit_s dataKeys_App2_backup_content[] =
{
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingK",                                         PersistencePolicy_wt, 0, 0, "Data>>/App2_SettingK"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",           PersistencePolicy_wt, 2, 1, "Data>>/App2_SettingB::user2::seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",           PersistencePolicy_wt, 2, 2, "Data>>/App2_SettingB::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingL",                                         PersistencePolicy_wt, 0, 0, "Data>>/App2_SettingL"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/KBL",                                     PersistencePolicy_wt, 1, 0, "Data>>/App2_Setting/KBL::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/KBL",                                     PersistencePolicy_wt, 2, 0, "Data>>/App2_Setting/KBL::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/KBL",                                     PersistencePolicy_wt, 3, 0, "Data>>/App2_Setting/KBL::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/KBL",                                     PersistencePolicy_wt, 4, 0, "Data>>/App2_Setting/KBL::user4"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                         PersistencePolicy_wc, 0, 0, "Data>>/App2_SettingD"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",           PersistencePolicy_wc, 2, 1, "Data>>/App2_SettingE::user2:seat1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",           PersistencePolicy_wc, 2, 2, "Data>>/App2_SettingE::user2:seat2"},
    {0xFF,  PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                         PersistencePolicy_wc, 0, 0, "Data>>/App2_SettingF"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                                     PersistencePolicy_wc, 1, 0, "Data>>/App2_Setting/DEF::user1"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                                     PersistencePolicy_wc, 2, 0, "Data>>/App2_Setting/DEF::user2"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                                     PersistencePolicy_wc, 3, 0, "Data>>/App2_Setting/DEF::user3"},
    {0xFF,  PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                                     PersistencePolicy_wc, 4, 0, "Data>>/App2_Setting/DEF::user4"}
} ;


entryDataInit_s dataFiles_App2_mnt_c_backup_content[] =
{
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                        PersistencePolicy_wt, 0, 0, "File>>App2>>/doc1.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                   PersistencePolicy_wt, 0, 0, "File>>App2>>/Docs/doc2.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                        PersistencePolicy_wc, 1, 0, "File>>App2>>/docK.txt::user1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                        PersistencePolicy_wc, 2, 0, "File>>App2>>/docK.txt::user2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                        PersistencePolicy_wc, 3, 0, "File>>App2>>/docK.txt::user3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                        PersistencePolicy_wc, 4, 0, "File>>App2>>/docK.txt::user4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>App2>>/docB.txt::user2:seat1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>App2>>/docB.txt::user2:seat2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>App2>>/docB.txt::user2:seat3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docB.txt::user2:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docC.txt::userC:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docD.txt::userC:seat4"}
};


entryDataInit_s dataFiles_App2_mnt_wt_backup_content[] =
{
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                        PersistencePolicy_wt, 0, 0, "File>>App2>>/doc1.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                   PersistencePolicy_wt, 0, 0, "File>>App2>>/Docs/doc2.txt"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                        PersistencePolicy_wc, 1, 0, "File>>App2>>/docK.txt::user1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                        PersistencePolicy_wc, 2, 0, "File>>App2>>/docK.txt::user2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                        PersistencePolicy_wc, 3, 0, "File>>App2>>/docK.txt::user3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                        PersistencePolicy_wc, 4, 0, "File>>App2>>/docK.txt::user4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",      PersistencePolicy_wc, 2, 1, "File>>App2>>/docB.txt::user2:seat1"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",      PersistencePolicy_wc, 2, 2, "File>>App2>>/docB.txt::user2:seat2"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",      PersistencePolicy_wc, 2, 3, "File>>App2>>/docB.txt::user2:seat3"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docB.txt::user2:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docC.txt::userC:seat4"},
    {0xFF, BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt",      PersistencePolicy_wc, 2, 4, "File>>App2>>/docD.txt::userC:seat4"}
};


dataInit_s s_App2_Data_mnt_c_backup_content =
{
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/",
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,
    dbType_local,
    RCT_App2_backup_content,
    sizeof(RCT_App2_backup_content)/sizeof(RCT_App2_backup_content[0]),
    dataKeys_App2_backup_content,
    sizeof(dataKeys_App2_backup_content)/sizeof(dataKeys_App2_backup_content[0]),
    dataFiles_App2_mnt_c_backup_content,
    sizeof(dataFiles_App2_mnt_c_backup_content)/sizeof(dataFiles_App2_mnt_c_backup_content[0])
};


dataInit_s s_App2_Data_mnt_wt_backup_content =
{
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2/",
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_RCT_NAME,
    dbType_RCT,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_SHARED_WT_DB_NAME,
    dbType_local,
    BACKUP_FOLDER PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_SHARED_CACHE_DB_NAME,
    dbType_local,
    RCT_App2_backup_content,
    sizeof(RCT_App2_backup_content)/sizeof(RCT_App2_backup_content[0]),
    dataKeys_App2_backup_content,
    sizeof(dataKeys_App2_backup_content)/sizeof(dataKeys_App2_backup_content[0]),
    dataFiles_App2_mnt_wt_backup_content,
    sizeof(dataFiles_App2_mnt_wt_backup_content)/sizeof(dataFiles_App2_mnt_wt_backup_content[0])
};

//reset backup content
bool_t ResetBackupContent(PersASSelectionType_e type, char* applicationID)
{
	sint_t sResult 				= 0;
    bool_t bEverythingOK 		= true ;
    pstr_t referenceDataPath 	= BACKUP_CONTENT_FOLDER;

    str_t  pchPathCompressTo   [PATH_ABS_MAX_SIZE];
    str_t  pchPathCompressFrom [PATH_ABS_MAX_SIZE];

    DeleteFolder(referenceDataPath);

    // the current implementation performs the restore process twice due to the
    // fact that the backup generates a mirrored content (for mnt-c and mnt-wt)

    if(bEverythingOK)
    {
    	sint_t i = 0 ;

    	// mnt_c
        dataInit_s* sDataInit[] =
        {
            &sSharedPubData_mnt_c_backup_content,
            &sShared_Group10_Data_mnt_c_backup_content,
            &sShared_Group20_Data_mnt_c_backup_content,
            &s_App1_Data_mnt_c_backup_content,
            &s_App2_Data_mnt_c_backup_content
        };

        for(i = 0 ; i < sizeof(sDataInit)/sizeof(sDataInit[0]) ; i++)
        {
            if(! InitDataFolder(sDataInit[i]))
            {
                bEverythingOK = false ;
            }
        }
    }

    if(bEverythingOK)
	{
		sint_t i = 0 ;

		// mnt_wt
		dataInit_s* sDataInit[] =
		{
			&sSharedPubData_mnt_wt_backup_content,
			&sShared_Group10_Data_mnt_wt_backup_content,
			&sShared_Group20_Data_mnt_wt_backup_content,
			&s_App1_Data_mnt_wt_backup_content,
			&s_App2_Data_mnt_wt_backup_content
		};

		for(i = 0 ; i < sizeof(sDataInit)/sizeof(sDataInit[0]) ; i++)
		{
			if(! InitDataFolder(sDataInit[i]))
			{
				bEverythingOK = false ;
			}
		}
	}

    /* compress the source folder and delete it afterwards */
    (void)snprintf(pchPathCompressFrom, sizeof(pchPathCompressFrom), "%s", BACKUP_CONTENT_FOLDER);

	/* create the tar name; */
	switch( type )
	{
		case PersASSelectionType_Application:
		{
			(void)snprintf(pchPathCompressTo, sizeof(pchPathCompressTo), "%s%s%s", BACKUP_FOLDER_, applicationID, BACKUP_FORMAT);
			break;
		}
		case PersASSelectionType_User:
		{
			(void)snprintf(pchPathCompressTo, sizeof(pchPathCompressTo), "%s%s%s", BACKUP_FOLDER_, "user", BACKUP_FORMAT);
			break;
		}
		case PersASSelectionType_All:
		{
			(void)snprintf(pchPathCompressTo, sizeof(pchPathCompressTo), "%s%s%s", BACKUP_FOLDER_, "all", BACKUP_FORMAT);
			break;
		}
		default:
		{
			bEverythingOK = false;
			/* nothing to do */
			break;
		}
	}

	if(true == bEverythingOK)
	{
		/* return 0 for success, negative value otherwise; */
		sResult = persadmin_compress(pchPathCompressTo, pchPathCompressFrom);
		if( 0 > sResult )
		{
			bEverythingOK = false;
		}

		/* remove the initial folder content */
		DeleteFolder(referenceDataPath);
	}

    return bEverythingOK ;
}


/**
 * @brief Saves files together into a single archive.
 * @usage persadmin_compress("/path/to/compress/to/archive_name.tgz", "/path/from/where/to/compress")
 * @return 0 for success, negative value otherwise.
 */
static sint_t persadmin_compress(pstr_t compressTo, pstr_t compressFrom)
{
    uint8_t              buffer         [READ_BUFFER_LENGTH];
    str_t  			     pchParentPath	[PATH_ABS_MAX_SIZE];
	pstr_t				 pchStrPos		= NIL;
	struct archive       *psArchive     = NIL;
    struct archive       *psDisk        = NIL;
    struct archive_entry *psEntry       = NIL;
    sint_t               s32Result      = ARCHIVE_OK;
    sint_t               s32Length      = 0;
    sint_t               fd;
    sint_t				 s32ParentPathLength	= 0;


    if( (NIL == compressTo) ||
        (NIL == compressFrom) )
    {
        s32Result = ARCHIVE_FAILED;
        printf("persadmin_compress - invalid parameters \n");
    }

    if( ARCHIVE_OK == s32Result )
    {
        printf("persadmin_compress - create <%s> from <%s>\n", compressTo, compressFrom);
        psArchive = archive_write_new();
        if( NIL == psArchive )
        {
            s32Result = ARCHIVE_FAILED;
            printf("persadmin_compress - archive_write_new ERR\n");
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        /* this in turn calls archive_write_add_filter_gzip; */
        s32Result = archive_write_set_compression_gzip(psArchive);
        if( ARCHIVE_OK != s32Result )
        {
            printf("persadmin_compress - archive_write_set_compression_gzip ERR %d\n", s32Result);
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        /* portable archive exchange; */
        archive_write_set_format_pax(psArchive);
        compressTo = (strcmp(compressTo, "-") == 0) ? NIL : compressTo;
        s32Result = archive_write_open_filename(psArchive, compressTo);
        if( ARCHIVE_OK != s32Result )
        {
            printf("persadmin_compress - archive_write_open_filename ERR %d\n", s32Result);
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        psDisk = archive_read_disk_new();
        if( NIL == psDisk )
        {
            s32Result = ARCHIVE_FAILED;
            printf("persadmin_compress - archive_read_disk_new ERR\n");
        }
    }

    if( ARCHIVE_OK == s32Result )
    {
        archive_read_disk_set_standard_lookup(psDisk);
        s32Result = archive_read_disk_open(psDisk, compressFrom);
        if( ARCHIVE_OK != s32Result )
        {
            printf("persadmin_compress - archive_read_disk_new ERR %s\n", archive_error_string(psDisk));
        }
    }

    memset(pchParentPath, 0, sizeof(pchParentPath));
    snprintf(pchParentPath, sizeof(pchParentPath), compressFrom);
    pchStrPos = strrchr(pchParentPath, '/');
    if(NIL != pchStrPos)
    {
    	*pchStrPos = '\0';
    }
    s32ParentPathLength = strlen(pchParentPath);


    while( ARCHIVE_OK == s32Result )
    {
        psEntry = archive_entry_new();
        s32Result = archive_read_next_header2(psDisk, psEntry);

        switch( s32Result )
        {
            case ARCHIVE_EOF:
            {
                /* nothing else to do; */
                break;
            }
            case ARCHIVE_OK:
            {
            	str_t 	pstrTemp[PATH_ABS_MAX_SIZE];
            	pstr_t	p = archive_entry_pathname(psEntry);
				if(NIL != p)
				{
                	/* remove parent section and save relative pathnames */
					memset(pstrTemp, 0, sizeof(pstrTemp));
					snprintf(pstrTemp, sizeof(pstrTemp), "%s", p + (s32ParentPathLength + 1));
					archive_entry_copy_pathname(psEntry, pstrTemp);
				}

                archive_read_disk_descend(psDisk);
                s32Result = archive_write_header(psArchive, psEntry);
                if( ARCHIVE_OK > s32Result)
                {
                    printf("persadmin_compress - archive_write_header ERR %s\n", archive_error_string(psArchive));
                }
                if( ARCHIVE_FATAL == s32Result )
                {
                    /* exit; */
                    printf("persadmin_compress - archive_write_header ERR FATAL\n");
                }
                if( ARCHIVE_FAILED < s32Result )
                {
#if 0
                    /* Ideally, we would be able to use
                     * the same code to copy a body from
                     * an archive_read_disk to an
                     * archive_write that we use for
                     * copying data from an archive_read
                     * to an archive_write_disk.
                     * Unfortunately, this doesn't quite
                     * work yet. */
                    persadmin_copy_data(psDisk, psArchive);
#else

                    /* For now, we use a simpler loop to copy data
                     * into the target archive. */
                    fd = open(archive_entry_sourcepath(psEntry), O_RDONLY);
                    s32Length = read(fd, buffer, READ_BUFFER_LENGTH);
                    while( s32Length > 0 )
                    {
                        archive_write_data(psArchive, buffer, s32Length);
                        s32Length = read(fd, buffer, READ_BUFFER_LENGTH);
                    }
                    close(fd);
#endif
                }

                break;
            }
            default:
            {
                printf("persadmin_compress - archive_read_next_header2 ERR %s\n", archive_error_string(psDisk));
                /* exit; */
                break;
            }
        }

        if( NIL != psEntry )
        {
            archive_entry_free(psEntry);
        }
    }

    /* perform cleaning operations; */
    if( NIL != psDisk )
    {
        archive_read_close(psDisk);
        archive_read_free(psDisk);
    }

    if( NIL != psArchive )
    {
        archive_write_close(psArchive);
        archive_write_free(psArchive);
    }

    /* overwrite result; */
    s32Result = (s32Result == ARCHIVE_EOF) ? ARCHIVE_OK : s32Result;
    /* return result; */
    return s32Result;

} /* persadmin_compress() */



