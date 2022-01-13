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
#include "test_pas_data_import.h"

#define LT_HDR                          "TEST_PAS >> "

DLT_IMPORT_CONTEXT(persAdminSvcDLTCtx);

#define PATH_ABS_MAX_SIZE  ( 512)

expected_key_data_localDB_s expected_key_data_after_import_app_all[] =
{
// TO BE USED WHEN RCT logic is implemented
//    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingK",                     PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App1_SettingK"                                          ,   sizeof("Data>>/App1_SettingK"                           )},
//    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingL",                     PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App1_SettingL"                                          ,   sizeof("Data>>/App1_SettingL"                           )},
//    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App1_Setting/KBL::user1"                                ,   sizeof("Data>>/App1_Setting/KBL::user1"                 )},
//    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App1_Setting/KBL::user2"                                ,   sizeof("Data>>/App1_Setting/KBL::user2"                 )},
//    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App1_Setting/KBL::user3"                                ,   sizeof("Data>>/App1_Setting/KBL::user3"                 )},
//    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App1_Setting/KBL::user4"                                ,   sizeof("Data>>/App1_Setting/KBL::user4"                 )},
//    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingK",                     PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App2_SettingK"                                          ,   sizeof("Data>>/App2_SettingK"                           )},
//    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingL",                     PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App2_SettingL"                                          ,   sizeof("Data>>/App2_SettingL"                           )},
//    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App2_Setting/KBL::user1"                                ,   sizeof("Data>>/App2_Setting/KBL::user1"                 )},
//    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App2_Setting/KBL::user2"                                ,   sizeof("Data>>/App2_Setting/KBL::user2"                 )},
//    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App2_Setting/KBL::user3"                                ,   sizeof("Data>>/App2_Setting/KBL::user3"                 )},
//    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/KBL",                 PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME                 ,   true,   "Data>>/App2_Setting/KBL::user4"                                ,   sizeof("Data>>/App2_Setting/KBL::user4"                 )},

	{ PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true,   "Data>>/App1_SettingB::user2::seat1"                            ,   sizeof("Data>>/App1_SettingB::user2::seat1"             )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,        true,   "Data>>/App1_SettingB::user2:seat2"                             ,   sizeof("Data>>/App1_SettingB::user2:seat2"              )},
	{ PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                           PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_SettingD"                                          ,   sizeof("Data>>/App1_SettingD"                           )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_SettingE::user2:seat1"                             ,   sizeof("Data>>/App1_SettingE::user2:seat1"              )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_SettingE::user2:seat2"                             ,   sizeof("Data>>/App1_SettingE::user2:seat2"              )},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                           PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_SettingF"                                          ,   sizeof("Data>>/App1_SettingF"                           )},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_Setting/DEF::user1"                                ,   sizeof("Data>>/App1_Setting/DEF::user1"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_Setting/DEF::user2"                                ,   sizeof("Data>>/App1_Setting/DEF::user2"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_Setting/DEF::user3"                                ,   sizeof("Data>>/App1_Setting/DEF::user3"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App1_Setting/DEF::user4"                                ,   sizeof("Data>>/App1_Setting/DEF::user4"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,        true,   "Data>>/App2_SettingB::user2::seat1"                            ,   sizeof("Data>>/App2_SettingB::user2::seat1"             )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,        true,   "Data>>/App2_SettingB::user2:seat2"                             ,   sizeof("Data>>/App2_SettingB::user2:seat2"              )},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                           PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_SettingD"                                          ,   sizeof("Data>>/App2_SettingD"                           )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_SettingE::user2:seat1"                             ,   sizeof("Data>>/App2_SettingE::user2:seat1"              )},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",             PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_SettingE::user2:seat2"                             ,   sizeof("Data>>/App2_SettingE::user2:seat2"              )},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                           PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_SettingF"                                          ,   sizeof("Data>>/App2_SettingF"                           )},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_Setting/DEF::user1"                                ,   sizeof("Data>>/App2_Setting/DEF::user1"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_Setting/DEF::user2"                                ,   sizeof("Data>>/App2_Setting/DEF::user2"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_Setting/DEF::user3"                                ,   sizeof("Data>>/App2_Setting/DEF::user3"                 )},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                                       PERS_ORG_LOCAL_APP_CACHE_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,     true,   "Data>>/App2_Setting/DEF::user4"                                ,   sizeof("Data>>/App2_Setting/DEF::user4"                 )},

	//common data, not imported for type "application"
    //public key
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingB",           	                                PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,     	    false,   "Data>>/pubSettingB",               							   sizeof("Data>>/pubSettingB")},
    //group key
    { PERS_ORG_USER_FOLDER_NAME_"1"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB",             PERS_ORG_SHARED_GROUP_CACHE_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       false,   "Data>>/gr10_SettingB::user1::seat1"                        ,   sizeof ("Data>>/gr10_SettingB::user1::seat1"        )},
};

expected_file_data_s expected_file_data_after_import_app_all[] =
{
// TO BE USED WHEN RCT logic is implemented
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                            true,       "File>>App1>>/docK.txt::user1"              ,   sizeof("File>>App1>>/docK.txt::user1"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                            true,       "File>>App1>>/docK.txt::user2"              ,   sizeof("File>>App1>>/docK.txt::user2"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                            true,       "File>>App1>>/docK.txt::user3"              ,   sizeof("File>>App1>>/docK.txt::user3"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                            true,       "File>>App1>>/docK.txt::user4"              ,   sizeof("File>>App1>>/docK.txt::user4"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",                true,       "File>>App1>>/docC.txt::user2:seat4"        ,   sizeof("File>>App1>>/docC.txt::user2:seat4"             )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt",                false,      "File>>App1>>/docB.txt::user2:seat4"        ,   sizeof("File>>App1>>/docD.txt::user2:seat4"             )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docK.txt",                            true,       "File>>App2>>/docK.txt::user1"              ,   sizeof("File>>App2>>/docK.txt::user1"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docK.txt",                            true,       "File>>App2>>/docK.txt::user2"              ,   sizeof("File>>App2>>/docK.txt::user2"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docK.txt",                            true,       "File>>App2>>/docK.txt::user3"              ,   sizeof("File>>App2>>/docK.txt::user3"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docK.txt",                            true,       "File>>App2>>/docK.txt::user4"              ,   sizeof("File>>App2>>/docK.txt::user4"                   )   },
//    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docC.txt",                true,       "File>>App2>>/docC.txt::user2:seat4"        ,   sizeof("File>>App2>>/docC.txt::user2:seat4"             )   }
//	  { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docD.txt",                false,      "File>>App2>>/docB.txt::user2:seat4"        ,   sizeof("File>>App2>>/docD.txt::user2:seat4"             )   }

	{ PERS_ORG_LOCAL_APP_CACHE_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                            true,       "File>>App1>>/doc1.txt"                     ,   sizeof("File>>App1>>/doc1.txt"                          )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                       true,       "File>>App1>>/Docs/doc2.txt"                ,   sizeof("File>>App1>>/Docs/doc2.txt"                     )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",          true,       "File>>App1>>/docB.txt::user2:seat1"        ,   sizeof("File>>App1>>/docB.txt::user2:seat1"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",          true,       "File>>App1>>/docB.txt::user2:seat2"        ,   sizeof("File>>App1>>/docB.txt::user2:seat2"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",          true,       "File>>App1>>/docB.txt::user2:seat3"        ,   sizeof("File>>App1>>/docB.txt::user2:seat3"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",          true,       "File>>App1>>/docB.txt::user2:seat4"        ,   sizeof("File>>App1>>/docB.txt::user2:seat4"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                            true,       "File>>App2>>/doc1.txt"                     ,   sizeof("File>>App2>>/doc1.txt"                          )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                       true,       "File>>App2>>/Docs/doc2.txt"                ,   sizeof("File>>App2>>/Docs/doc2.txt"                     )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",          true,       "File>>App2>>/docB.txt::user2:seat1"        ,   sizeof("File>>App2>>/docB.txt::user2:seat1"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",          true,       "File>>App2>>/docB.txt::user2:seat2"        ,   sizeof("File>>App2>>/docB.txt::user2:seat2"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",          true,       "File>>App2>>/docB.txt::user2:seat3"        ,   sizeof("File>>App2>>/docB.txt::user2:seat3"             )   },
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",          true,       "File>>App2>>/docB.txt::user2:seat4"        ,   sizeof("File>>App2>>/docB.txt::user2:seat4"             )   }


};

bool_t Test_import_all_app(sint_t type, void* pv)
{
    bool_t 	bEverythingOK 	= true ;
    long 	impBytes		=-1;
    str_t  pchBackupFilePath [PATH_ABS_MAX_SIZE];

    bEverythingOK = ResetImportData(PersASSelectionType_Application);

    if(true == bEverythingOK)
	{
		(void)snprintf(pchBackupFilePath, sizeof(pchBackupFilePath), "%s%s", "all", BACKUP_FORMAT);

		/* Restore content */
		DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), 	DLT_STRING("Import from:"),
																		DLT_STRING(pchBackupFilePath),
																		DLT_STRING("..."));

		impBytes = persAdminDataFolderImport(PersASSelectionType_Application, pchBackupFilePath);

		bEverythingOK = (impBytes >= 0)?true:false;
	}

    printf("\nTest_import_all - %s \n", bEverythingOK   ? "OK" : "FAILED") ;

    return bEverythingOK ;
}
