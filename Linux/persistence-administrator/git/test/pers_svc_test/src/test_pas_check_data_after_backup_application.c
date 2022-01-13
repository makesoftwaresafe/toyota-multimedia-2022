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
#include "string.h"
#include "stdio.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include "test_PAS.h"
#include "ssw_pers_admin_files_helper.h"
#include "test_pas_check_data_after_backup_application.h"
#include "persistence_admin_service.h"

expected_key_data_localDB_s expectedKeyData_shared_public_localDB_AfterBackupApplication[16] =
{   
/* App1 */
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingA"                ,      sizeof("Data>>/App1_SettingA"               )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",  "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingB::user2::seat1"  ,      sizeof("Data>>/App1_SettingB::user2::seat1" )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",  "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingB::user2:seat2"   ,      sizeof("Data>>/App1_SettingB::user2:seat2"  )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingC"                ,      sizeof("Data>>/App1_SettingC"               )},
        { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user1"      ,      sizeof("Data>>/App1_Setting/ABC::user1"     )},
        { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user2"      ,      sizeof("Data>>/App1_Setting/ABC::user2"     )},
        { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user3"      ,      sizeof("Data>>/App1_Setting/ABC::user3"     )},
        { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user4"      ,      sizeof("Data>>/App1_Setting/ABC::user4"     )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingD"                ,      sizeof("Data>>/App1_SettingD"               )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",  "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingE::user2:seat1"   ,      sizeof("Data>>/App1_SettingE::user2:seat1"  )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",  "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingE::user2:seat2"   ,      sizeof("Data>>/App1_SettingE::user2:seat2"  )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingF"                ,      sizeof("Data>>/App1_SettingF"               )},
        { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user1"      ,      sizeof("Data>>/App1_Setting/DEF::user1"     )},
        { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user2"      ,      sizeof("Data>>/App1_Setting/DEF::user2"     )},
        { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user3"      ,      sizeof("Data>>/App1_Setting/DEF::user3"     )},
        { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                            "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user4"      ,      sizeof("Data>>/App1_Setting/DEF::user4"     )}
};

expected_file_data_s expectedFileData_shared_public_AfterBackupApplication[10] =
{    
/* App1 */
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                    true, "File>>App1>>/doc1.txt"              ,     sizeof("File>>App1>>/doc1.txt"                      )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                               true, "File>>App1>>/Docs/doc2.txt"         ,     sizeof("File>>App1>>/Docs/doc2.txt"                 )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                    true, "File>>App1>>/docA.txt::user1"       ,     sizeof("File>>App1>>/docA.txt::user1"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                    true, "File>>App1>>/docA.txt::user2"       ,     sizeof("File>>App1>>/docA.txt::user2"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                    true, "File>>App1>>/docA.txt::user3"       ,     sizeof("File>>App1>>/docA.txt::user3"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                    true, "File>>App1>>/docA.txt::user4"       ,     sizeof("File>>App1>>/docA.txt::user4"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  true, "File>>App1>>/docB.txt::user2:seat1" ,     sizeof("File>>App1>>/docB.txt::user2:seat1"         )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  true, "File>>App1>>/docB.txt::user2:seat2" ,     sizeof("File>>App1>>/docB.txt::user2:seat2"         )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  true, "File>>App1>>/docB.txt::user2:seat3" ,     sizeof("File>>App1>>/docB.txt::user2:seat3"         )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>App1>>/docB.txt::user2:seat4" ,     sizeof("File>>App1>>/docB.txt::user2:seat4"         )}
} ;

/**************************************************************************************************************
*****************************************    ADD TEST CASES HERE   ********************************************
**************************************************************************************************************/
bool_t Test_DataAfterBackupCreateApplication(int ceva, void* pAltceva)
{
    long                    sResult                 = 0;
    PersASSelectionType_e   eSelection              = PersASSelectionType_LastEntry;
    char                    pchBackupName           [MAX_PATH_SIZE];
    char                    pchApplicationID        [MAX_APPLICATION_NAME_SIZE];
    int                     iBackupNameSize         = sizeof(pchBackupName);
    int                     iApplicationNameSize    = sizeof(pchApplicationID);

    // reset;
    memset(pchBackupName,    0, iBackupNameSize);
    memset(pchApplicationID, 0, iApplicationNameSize);

    // selection application : valid application, all users, all seats;

    // create input data;
    snprintf(pchBackupName,    iBackupNameSize,      "%s", BACKUP_NAME);
    snprintf(pchApplicationID, iApplicationNameSize, "%s", APPLICATION_NAME);
    eSelection = PersASSelectionType_Application;
 
    persadmin_delete_folder(BACKUP_NAME);
    // persAdminDataBackupCreate(PersASSelectionType_Application, "/tmp/backup", "App1", 0xFF, 0xFF);
    sResult = persAdminDataBackupCreate(eSelection, pchBackupName, pchApplicationID, PERSIST_SELECT_ALL_USERS, PERSIST_SELECT_ALL_SEATS);
    // expected result : backup is created for the specified application, all users & seats (local);

    // some info;
    printf("\n Test_DataAfterBackupCreateApplication: persAdminDataBackupCreate(application) - %ld \n", sResult) ;

    return true ;
}
