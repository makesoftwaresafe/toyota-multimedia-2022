#include "persComTypes.h"
#include "stdio.h"
#include "string.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include "ssw_pers_admin_files_helper.h"
#include "test_PAS.h"
#include "test_pas_check_data_after_backup_create_all.h"
#include "persistence_admin_service.h"

expected_key_data_localDB_s expectedKeyData_shared_public_localDB_AfterBackupCreateAll[80] =
{   
/* shared public */        
        { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSettingA"                  ,      sizeof("Data>>/pubSettingA"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",  "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSettingB::user2::seat1"    ,      sizeof("Data>>/pubSettingB::user2::seat1"   )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",  "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSettingB::user2:seat2"     ,      sizeof("Data>>/pubSettingB::user2:seat2"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                                "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSettingC"                  ,      sizeof("Data>>/pubSettingC"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSetting/ABC::user1"        ,      sizeof("Data>>/pubSetting/ABC::user1"       )},
        { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSetting/ABC::user2"        ,      sizeof("Data>>/pubSetting/ABC::user2"       )},
        { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSetting/ABC::user3"        ,      sizeof("Data>>/pubSetting/ABC::user3"       )},
        { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,         true,   "Data>>/pubSetting/ABC::user4"        ,      sizeof("Data>>/pubSetting/ABC::user4"       )},
        { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSettingD"                  ,      sizeof("Data>>/pubSettingD"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",  "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSettingE::user2:seat1"     ,      sizeof("Data>>/pubSettingE::user2:seat1"    )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",  "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSettingE::user2:seat2"     ,      sizeof("Data>>/pubSettingE::user2:seat2"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSettingF"                  ,      sizeof("Data>>/pubSettingF"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSetting/DEF::user1"        ,      sizeof("Data>>/pubSetting/DEF::user1"       )},
        { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSetting/DEF::user2"        ,      sizeof("Data>>/pubSetting/DEF::user2"       )},
        { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSetting/DEF::user3"        ,      sizeof("Data>>/pubSetting/DEF::user3"       )},
        { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                            "/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,      true,   "Data>>/pubSetting/DEF::user4"        ,      sizeof("Data>>/pubSetting/DEF::user4"       )},
/* shared group 10 */
        { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingA",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_SettingA"                ,      sizeof("Data>>/gr10_SettingA"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB" ,  "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_SettingB::user2::seat1"  ,      sizeof("Data>>/gr10_SettingB::user2::seat1"   )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingB" ,  "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_SettingB::user2:seat2"   ,      sizeof("Data>>/gr10_SettingB::user2:seat2"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingC",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_SettingC"                ,      sizeof("Data>>/gr10_SettingC"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_Setting/ABC::user1"      ,      sizeof("Data>>/gr10_Setting/ABC::user1"       )},
        { PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_Setting/ABC::user2"      ,      sizeof("Data>>/gr10_Setting/ABC::user2"       )},
        { PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_Setting/ABC::user3"      ,      sizeof("Data>>/gr10_Setting/ABC::user3"       )},
        { PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr10_Setting/ABC::user4"      ,      sizeof("Data>>/gr10_Setting/ABC::user4"       )},
        { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingD",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_SettingD"                ,      sizeof("Data>>/gr10_SettingD"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingE",   "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_SettingE::user2:seat1"   ,      sizeof("Data>>/gr10_SettingE::user2:seat1"    )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingE",   "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_SettingE::user2:seat2"   ,      sizeof("Data>>/gr10_SettingE::user2:seat2"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingF",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_SettingF"                ,      sizeof("Data>>/gr10_SettingF"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_Setting/DEF::user1"      ,      sizeof("Data>>/gr10_Setting/DEF::user1"       )},
        { PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_Setting/DEF::user2"      ,      sizeof("Data>>/gr10_Setting/DEF::user2"       )},
        { PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_Setting/DEF::user3"      ,      sizeof("Data>>/gr10_Setting/DEF::user3"       )},
        { PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr10_Setting/DEF::user4"      ,      sizeof("Data>>/gr10_Setting/DEF::user4"       )},
/* shared group 20 */
        { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingA",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_SettingA"                ,      sizeof("Data>>/gr20_SettingA"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingB",   "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_SettingB::user2::seat1"  ,      sizeof("Data>>/gr20_SettingB::user2::seat1"   )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingB",   "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_SettingB::user2:seat2"   ,      sizeof("Data>>/gr20_SettingB::user2:seat2"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingC",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_SettingC"                ,      sizeof("Data>>/gr20_SettingC"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_Setting/ABC::user1"      ,      sizeof("Data>>/gr20_Setting/ABC::user1"       )},
        { PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_Setting/ABC::user2"      ,      sizeof("Data>>/gr20_Setting/ABC::user2"       )},
        { PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_Setting/ABC::user3"      ,      sizeof("Data>>/gr20_Setting/ABC::user3"       )},
        { PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/ABC",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,       true,   "Data>>/gr20_Setting/ABC::user4"      ,      sizeof("Data>>/gr20_Setting/ABC::user4"       )},
        { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingD",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_SettingD"                ,      sizeof("Data>>/gr20_SettingD"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingE" ,  "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_SettingE::user2:seat1"   ,      sizeof("Data>>/gr20_SettingE::user2:seat1"    )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingE" ,  "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_SettingE::user2:seat2"   ,      sizeof("Data>>/gr20_SettingE::user2:seat2"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingF",                                 "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_SettingF"                ,      sizeof("Data>>/gr20_SettingF"                 )},
        { PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_Setting/DEF::user1"      ,      sizeof("Data>>/gr20_Setting/DEF::user1"       )},
        { PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_Setting/DEF::user2"      ,      sizeof("Data>>/gr20_Setting/DEF::user2"       )},
        { PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_Setting/DEF::user3"      ,      sizeof("Data>>/gr20_Setting/DEF::user3"       )},
        { PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/DEF",                             "/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME,    true,   "Data>>/gr20_Setting/DEF::user4"      ,      sizeof("Data>>/gr20_Setting/DEF::user4"       )},
/* App1 */
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingA"                ,      sizeof("Data>>/App1_SettingA"               )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingB::user2::seat1"  ,      sizeof("Data>>/App1_SettingB::user2::seat1" )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingB::user2:seat2"   ,      sizeof("Data>>/App1_SettingB::user2:seat2"  )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_SettingC"                ,      sizeof("Data>>/App1_SettingC"               )},
        { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user1"      ,      sizeof("Data>>/App1_Setting/ABC::user1"     )},
        { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user2"      ,      sizeof("Data>>/App1_Setting/ABC::user2"     )},
        { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user3"      ,      sizeof("Data>>/App1_Setting/ABC::user3"     )},
        { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App1_Setting/ABC::user4"      ,      sizeof("Data>>/App1_Setting/ABC::user4"     )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingD"                ,      sizeof("Data>>/App1_SettingD"               )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingE::user2:seat1"   ,      sizeof("Data>>/App1_SettingE::user2:seat1"  )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingE::user2:seat2"   ,      sizeof("Data>>/App1_SettingE::user2:seat2"  )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_SettingF"                ,      sizeof("Data>>/App1_SettingF"               )},
        { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user1"      ,      sizeof("Data>>/App1_Setting/DEF::user1"     )},
        { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user2"      ,      sizeof("Data>>/App1_Setting/DEF::user2"     )},
        { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user3"      ,      sizeof("Data>>/App1_Setting/DEF::user3"     )},
        { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App1_Setting/DEF::user4"      ,      sizeof("Data>>/App1_Setting/DEF::user4"     )},
/* App2*/        
        { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingA",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_SettingA"                ,      sizeof( "Data>>/App2_SettingA"              )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_SettingB::user2::seat1"  ,      sizeof( "Data>>/App2_SettingB::user2::seat1")},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_SettingB::user2:seat2"   ,      sizeof( "Data>>/App2_SettingB::user2:seat2" )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingC",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_SettingC"                ,      sizeof( "Data>>/App2_SettingC"              )},
        { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_Setting/ABC::user1"      ,      sizeof( "Data>>/App2_Setting/ABC::user1"    )},
        { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_Setting/ABC::user2"      ,      sizeof( "Data>>/App2_Setting/ABC::user2"    )},
        { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_Setting/ABC::user3"      ,      sizeof( "Data>>/App2_Setting/ABC::user3"    )},
        { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/ABC",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,                  true,   "Data>>/App2_Setting/ABC::user4"      ,      sizeof( "Data>>/App2_Setting/ABC::user4"    )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_SettingD"                ,      sizeof( "Data>>/App2_SettingD"              )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_SettingE::user2:seat1"   ,      sizeof( "Data>>/App2_SettingE::user2:seat1" )},
        { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",   "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_SettingE::user2:seat2"   ,      sizeof( "Data>>/App2_SettingE::user2:seat2" )},
        { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                 "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_SettingF"                ,      sizeof( "Data>>/App2_SettingF"              )},
        { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_Setting/DEF::user1"      ,      sizeof( "Data>>/App2_Setting/DEF::user1"    )},
        { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_Setting/DEF::user2"      ,      sizeof( "Data>>/App2_Setting/DEF::user2"    )},
        { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_Setting/DEF::user3"      ,      sizeof( "Data>>/App2_Setting/DEF::user3"    )},
        { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                             "/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,               true,   "Data>>/App2_Setting/DEF::user4"      ,      sizeof( "Data>>/App2_Setting/DEF::user4"    )}
};

expected_file_data_s expectedFileData_shared_public_AfterBackupCreateAll[50] =
{    
/* shared public */
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH  PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                    true, "File>>/doc1.txt"                     ,    sizeof("File>>/doc1.txt"                            )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH  PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                               true, "File>>/Docs/doc2.txt"                ,    sizeof("File>>/Docs/doc2.txt"                       )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                    true, "File>>/docA.txt::user1"              ,    sizeof("File>>/docA.txt::user1"                     )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                    true, "File>>/docA.txt::user2"              ,    sizeof("File>>/docA.txt::user2"                     )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                    true, "File>>/docA.txt::user3"              ,    sizeof("File>>/docA.txt::user3"                     )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                    true, "File>>/docA.txt::user4"              ,    sizeof("File>>/docA.txt::user4"                     )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat1"        ,    sizeof("File>>/docB.txt::user2:seat1"               )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat2"        ,    sizeof("File>>/docB.txt::user2:seat2"               )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat3"        ,    sizeof("File>>/docB.txt::user2:seat3"               )},
        {"/tmp/backup"PERS_ORG_SHARED_PUBLIC_WT_PATH PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat4"        ,    sizeof("File>>/docB.txt::user2:seat4"               )},
/* shared group 10 */
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                  true, "File>>gr10_>>/gr10_1.txt"                   ,  sizeof("File>>gr10_>>/gr10_1.txt"                     )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_A.txt",                             true, "File>>gr10_>>/Docs/gr10_A.txt"              ,  sizeof("File>>gr10_>>/Docs/gr10_A.txt"                )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user1"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user1"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user2"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user2"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user3"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user3"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/gr10_2.txt",                                  true, "File>>gr10_>>/gr10_2.txt::user4"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user4"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat1"        )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat2"        )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat3"        )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat4"        )},
/* shared group 20 */           
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt" ,                                   true, "File>>gr20_>>/doc1.txt"              ,    sizeof("File>>gr20_>>/doc1.txt"                     )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                               true, "File>>gr20_>>/Docs/doc2.txt"         ,    sizeof("File>>gr20_>>/Docs/doc2.txt"                )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                    true, "File>>gr20_>>/docA.txt::user1"       ,    sizeof("File>>gr20_>>/docA.txt::user1"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                    true, "File>>gr20_>>/docA.txt::user2"       ,    sizeof("File>>gr20_>>/docA.txt::user2"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                    true, "File>>gr20_>>/docA.txt::user3"       ,    sizeof("File>>gr20_>>/docA.txt::user3"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                    true, "File>>gr20_>>/docA.txt::user4"       ,    sizeof("File>>gr20_>>/docA.txt::user4"              )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  true, "File>>gr20_>>/docB.txt::user2:seat1" ,    sizeof("File>>gr20_>>/docB.txt::user2:seat1"        )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  true, "File>>gr20_>>/docB.txt::user2:seat2" ,    sizeof("File>>gr20_>>/docB.txt::user2:seat2"        )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  true, "File>>gr20_>>/docB.txt::user2:seat3" ,    sizeof("File>>gr20_>>/docB.txt::user2:seat3"        )},
        {"/tmp/backup"PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>gr20_>>/docB.txt::user2:seat4" ,    sizeof("File>>gr20_>>/docB.txt::user2:seat4"        )},
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
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>App1>>/docB.txt::user2:seat4" ,     sizeof("File>>App1>>/docB.txt::user2:seat4"         )},
/* App2*/          
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                    true, "File>>App2>>/doc1.txt"       ,            sizeof("File>>App2>>/doc1.txt"                      )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                               true, "File>>App2>>/Docs/doc2.txt"  ,            sizeof("File>>App2>>/Docs/doc2.txt"                 )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                    true, "File>>App2>>/docA.txt::user1",            sizeof("File>>App2>>/docA.txt::user1"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                    true, "File>>App2>>/docA.txt::user2",            sizeof("File>>App2>>/docA.txt::user2"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                    true, "File>>App2>>/docA.txt::user3",            sizeof("File>>App2>>/docA.txt::user3"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                    true, "File>>App2>>/docA.txt::user4",            sizeof("File>>App2>>/docA.txt::user4"               )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  true, "File>>App2>>/docB.txt::user2:seat1" ,     sizeof("File>>App2>>/docB.txt::user2:seat1"         )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  true, "File>>App2>>/docB.txt::user2:seat2" ,     sizeof("File>>App2>>/docB.txt::user2:seat2"         )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  true, "File>>App2>>/docB.txt::user2:seat3" ,     sizeof("File>>App2>>/docB.txt::user2:seat3"         )},
        {"/tmp/backup"PERS_ORG_LOCAL_APP_WT_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>App2>>/docB.txt::user2:seat4" ,     sizeof("File>>App2>>/docB.txt::user2:seat4"         )}
} ;

/**************************************************************************************************************
*****************************************    ADD TEST CASES HERE   ********************************************
**************************************************************************************************************/
bool_t Test_DataAfterBackupCreateAll(int ceva, void* pAltceva)
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

    // selection all : all applications, all users, all seats;

    // create input data;
    snprintf(pchBackupName,    iBackupNameSize,      "%s", BACKUP_NAME);
    snprintf(pchApplicationID, iApplicationNameSize, "%s", NO_APPLICATION);
    eSelection = PersASSelectionType_All;
 
    persadmin_delete_folder(BACKUP_NAME);
    // persAdminDataBackupCreate(PersASSelectionType_All, "/tmp/backup", "", 0xFF, 0xFF);
    sResult = persAdminDataBackupCreate(eSelection, pchBackupName, pchApplicationID, PERSIST_SELECT_ALL_USERS, PERSIST_SELECT_ALL_SEATS);
    // expected result : backup is created for all applications, users & seats (local + shared);

    // some info;
    printf("\n Test_BackupCreateAll: persAdminDataBackupCreate(all) - %ld \n", sResult) ;

    return true ;
}
