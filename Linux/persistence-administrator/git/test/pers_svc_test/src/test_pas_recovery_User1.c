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
#include <stdio.h>

#include <dlt/dlt.h>
#include <dlt/dlt_user.h>

#include "test_PAS.h"
#include "persistence_admin_service.h"
#include "test_pas_data_backup_recovery.h"

DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);

#define LT_HDR                          "TEST_PAS >> "

#define PATH_ABS_MAX_SIZE  ( 512)

//===================================================================================================================
// INIT
//===================================================================================================================
// using default structure offered by test framework


//===================================================================================================================
// BACKUP CONTENT
//===================================================================================================================
// using a common backup content structure


//===================================================================================================================
// EXPECTED
//===================================================================================================================
expected_key_data_localDB_s expected_App1_key_data_after_restore_User1[24] =
{
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_SettingA",                 sizeof("Data>>/App1_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_SettingB::user2::seat1",   sizeof("Data>>/App1_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_SettingB::user2:seat2",    sizeof("Data>>/App1_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_SettingC",                 sizeof("Data>>/App1_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        false,"Data>>/App1_Setting/ABC::user1",       sizeof("Data>>/App1_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_Setting/ABC::user2",       sizeof("Data>>/App1_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_Setting/ABC::user3",       sizeof("Data>>/App1_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_Setting/ABC::user4",       sizeof("Data>>/App1_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingK",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        false,"Data>>/App1_SettingK",                 sizeof("Data>>/App1_SettingK")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingL",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        false,"Data>>/App1_SettingL",                 sizeof("Data>>/App1_SettingL")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/KBL",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true, "Data>>/App1_Setting/KBL::user1",       sizeof("Data>>/App1_Setting/KBL::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/KBL",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        false,"Data>>/App1_Setting/KBL::user2",       sizeof("Data>>/App1_Setting/KBL::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/KBL",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        false,"Data>>/App1_Setting/KBL::user3",       sizeof("Data>>/App1_Setting/KBL::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/KBL",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        false,"Data>>/App1_Setting/KBL::user4",       sizeof("Data>>/App1_Setting/KBL::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_SettingD",                 sizeof("Data>>/App1_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_SettingE::user2:seat1",    sizeof("Data>>/App1_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_SettingE::user2:seat2",    sizeof("Data>>/App1_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_SettingF",                 sizeof("Data>>/App1_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_Setting/DEF::user1",       sizeof("Data>>/App1_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/KKK",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     false,"Data>>/App1_Setting/DEF::user2",       sizeof("Data>>/App1_Setting/KKK::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_Setting/DEF::user2",       sizeof("Data>>/App1_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_Setting/DEF::user3",       sizeof("Data>>/App1_Setting/DEF::user4")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true, "Data>>/App1_Setting/DEF::user4",       sizeof("Data>>/App1_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/XYZ",                              PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     false,"Data>>/App1_Setting/DEF::user4",       sizeof("Data>>/App1_Setting/XYZ::user4")},
};

expected_file_data_s expected_App1_file_data_after_restore_User1[16] =
{
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      true,  "File>>App1>>/doc1.txt"              , sizeof("File>>App1>>/doc1.txt"                )},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 true,  "File>>App1>>/Docs/doc2.txt"         , sizeof("File>>App1>>/Docs/doc2.txt"           )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                   false, "File>>App1>>/docA.txt::user1"       , sizeof("File>>App1>>/docA.txt::user1"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                   true,  "File>>App1>>/docA.txt::user2"       , sizeof("File>>App1>>/docA.txt::user2"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                   true,  "File>>App1>>/docA.txt::user3"       , sizeof("File>>App1>>/docA.txt::user3"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                   true,  "File>>App1>>/docA.txt::user4"       , sizeof("File>>App1>>/docA.txt::user4"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                                   true,  "File>>App1>>/docK.txt::user1"       , sizeof("File>>App1>>/docK.txt::user1"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                                   false, "File>>App1>>/docK.txt::user2"       , sizeof("File>>App1>>/docK.txt::user2"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                                   false, "File>>App1>>/docK.txt::user3"       , sizeof("File>>App1>>/docK.txt::user3"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                                   false, "File>>App1>>/docK.txt::user4"       , sizeof("File>>App1>>/docK.txt::user4"         )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt", true,  "File>>App1>>/docB.txt::user2:seat1" , sizeof("File>>App1>>/docB.txt::user2:seat1"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt", true,  "File>>App1>>/docB.txt::user2:seat2" , sizeof("File>>App1>>/docB.txt::user2:seat2"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt", true,  "File>>App1>>/docB.txt::user2:seat3" , sizeof("File>>App1>>/docB.txt::user2:seat3"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt", true,  "File>>App1>>/docB.txt::user2:seat4" , sizeof("File>>App1>>/docB.txt::user2:seat4"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt", false, "File>>App1>>/docC.txt::user2:seat4" , sizeof("File>>App1>>/docC.txt::user2:seat4"   )},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt", false, "File>>App1>>/docD.txt::user2:seat4" , sizeof("File>>App1>>/docD.txt::user2:seat4"   )}
};
//===================================================================================================================

bool_t Test_Recover_User1(sint_t type, void* pv)
{

    bool_t bEverythingOK = true ;

    str_t  pchBackupFilePath [PATH_ABS_MAX_SIZE];

    /* Reset the backup data for every test */
    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Reset backup content..."));

    bEverythingOK = ResetBackupContent(PersASSelectionType_User, NULL) ;

    DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Test_Recover_User1: ResetBackupContent() - "),
            														DLT_STRING(bEverythingOK ? "OK" : "FAILED"));

    if(true == bEverythingOK)
    {
		(void)snprintf(pchBackupFilePath, sizeof(pchBackupFilePath), "%s%s", "user", BACKUP_FORMAT);

		/* Restore content */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Restore backup from : "),
																		DLT_STRING(pchBackupFilePath),
																		DLT_STRING("..."));

    	bEverythingOK = persAdminDataBackupRecovery(   PersASSelectionType_User,
														pchBackupFilePath,
														"",
														0x01,
                                            			PERSIST_SELECT_ALL_SEATS);

    	DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Test_Recover_User1: persAdminDataBackupRecovery() - "),
        																DLT_STRING(bEverythingOK ? "OK" : "FAILED"));
    }

    return bEverythingOK ;
} /* Test_Recover_User1 */
