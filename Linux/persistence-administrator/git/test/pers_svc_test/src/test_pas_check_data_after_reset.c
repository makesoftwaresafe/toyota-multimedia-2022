/*********************************************************************************************************************** Copyright (C) 2012 Continental Automotive Systems, Inc.** Author: Petrica.Manoila@continental-corporation.com** Implementation of backup process** This Source Code Form is subject to the terms of the Mozilla Public* License, v. 2.0. If a copy of the MPL was not distributed with this* file, You can obtain one at http://mozilla.org/MPL/2.0/.** Date       Author             Reason* 2012.11.27 uidu0250           CSP_WZ#1280:  Initial version***********************************************************************************************************************/#include "persComTypes.h"
#include "stdio.h"


#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include "ssw_pers_admin_files_helper.h"

#include "test_PAS.h"
#include "test_pas_check_data_after_reset.h"

#define File_t PersistenceResourceType_file
#define Key_t PersistenceResourceType_key

expected_key_data_RCT_s expectedKeyData_shared_public_RCT_AfterReset[13] =
{
    {"pubSettingA",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,   
                        {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingB",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,
                        {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingC",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,   
                        {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/ABC",   PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,   
                        {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingD",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,  
                        {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingE",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true, 
                        {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingF",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,   
                        {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/DEF",   PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,
                        {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"doc1.txt",         PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,
                        {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,
                        {PersistencePolicy_wt,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"docA.txt",         PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true,
                        {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"Docs/docB.txt",    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, true, 
                        {PersistencePolicy_wc,  PersistenceStorage_shared, File_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/DEE",   PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_RCT_NAME, false,
                        {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t,  S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}}
};

expected_key_data_localDB_s expectedKeyData_shared_public_localDB_AfterReset[18] =
{
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSettingA",               sizeof("Data>>/pubSettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSettingB::user2::seat1", sizeof("Data>>/pubSettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSettingB::user2:seat2",  sizeof("Data>>/pubSettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                                    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSettingC",               sizeof("Data>>/pubSettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSetting/ABC::user1",     sizeof("Data>>/pubSetting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSetting/ABC::user2",     sizeof("Data>>/pubSetting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSetting/ABC::user3",     sizeof("Data>>/pubSetting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,    "Data>>/pubSetting/ABC::user4",     sizeof("Data>>/pubSetting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSettingD",               sizeof("Data>>/pubSettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSettingE::user2:seat1",  sizeof("Data>>/pubSettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",      PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSettingE::user2:seat2",  sizeof("Data>>/pubSettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSettingF",               sizeof("Data>>/pubSettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSetting/DEF::user1",     sizeof("Data>>/pubSetting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSetting/DEF::user2",     sizeof("Data>>/pubSetting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSetting/DEF::user3",     sizeof("Data>>/pubSetting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,    "Data>>/pubSetting/DEF::user4",     sizeof("Data>>/pubSetting/DEF::user4")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     false,   NIL,                                0},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  false,   NIL,                                0}
};

expected_file_data_s expectedFileData_shared_public_AfterReset[11] =
{
    { PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/doc1.txt",                                           true, "File>>/doc1.txt",                sizeof("File>>/doc1.txt")},
    { PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/Docs/doc2.txt",                                      true, "File>>/Docs/doc2.txt",           sizeof("File>>/Docs/doc2.txt")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/1/docA.txt",                                      true, "File>>/docA.txt::user1",         sizeof("File>>/docA.txt::user1")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2/docA.txt",                                      true, "File>>/docA.txt::user2",         sizeof("File>>/docA.txt::user2")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/3/docA.txt",                                      true, "File>>/docA.txt::user3",         sizeof("File>>/docA.txt::user3")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/4/docA.txt",                                      true, "File>>/docA.txt::user4",         sizeof("File>>/docA.txt::user4")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",    true, "File>>/docB.txt::user2:seat1",   sizeof("File>>/docB.txt::user2:seat1")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",    true, "File>>/docB.txt::user2:seat2",   sizeof("File>>/docB.txt::user2:seat2")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",    true, "File>>/docB.txt::user2:seat3",   sizeof("File>>/docB.txt::user2:seat3")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",    true, "File>>/docB.txt::user2:seat4",   sizeof("File>>/docB.txt::user2:seat4")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/1"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",    false,NIL,                              0},
} ;

#if 0
bool_t Test_DataAfterReset(int ceva, void* pAltceva) ;

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

const testcase_s const* pTC_testDataAfterReset = &tc_testDataAfterReset ;
#endif

/**************************************************************************************************************
*****************************************    ADD TEST CASES HERE   ********************************************
**************************************************************************************************************/
bool_t Test_DataAfterReset(int ceva, void* pAltceva)
{
    bool_t bEverythingOK = true ;

    //bEverythingOK = ResetReferenceData() ;
    //printf("\nTest_DataAfterReset: ResetReferenceData() - %s \n", bEverythingOK ? "OK" : "FAILED") ;
    printf("\nTest_DataAfterReset: Data was reset... \n") ;

    return bEverythingOK ;
}
