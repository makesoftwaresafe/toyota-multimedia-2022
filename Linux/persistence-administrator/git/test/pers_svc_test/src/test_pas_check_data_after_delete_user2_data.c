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
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include "persistence_admin_service.h"

#include "test_PAS.h"
#include "test_pas_check_data_after_delete_user2_data.h"

expected_key_data_localDB_s expectedKeyData_shared_public_localDB_AfterDeleteUser2Data[80] =
{   
    //public
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,  "Data>>/pubSettingA",               sizeof("Data>>/pubSettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     false, "Data>>/pubSettingB::user2::seat1", sizeof("Data>>/pubSettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     false, "Data>>/pubSettingB::user2:seat2",  sizeof("Data>>/pubSettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,  "Data>>/pubSettingC",               sizeof("Data>>/pubSettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,  "Data>>/pubSetting/ABC::user1",     sizeof("Data>>/pubSetting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,  "Data>>/pubSetting/ABC::user2",     sizeof("Data>>/pubSetting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,  "Data>>/pubSetting/ABC::user3",     sizeof("Data>>/pubSetting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_WT_DB_NAME,     true,  "Data>>/pubSetting/ABC::user4",     sizeof("Data>>/pubSetting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,  "Data>>/pubSettingD",               sizeof("Data>>/pubSettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  false, "Data>>/pubSettingE::user2:seat1",  sizeof("Data>>/pubSettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",    PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  false, "Data>>/pubSettingE::user2:seat2",  sizeof("Data>>/pubSettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,  "Data>>/pubSettingF",               sizeof("Data>>/pubSettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,  "Data>>/pubSetting/DEF::user1",     sizeof("Data>>/pubSetting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,  "Data>>/pubSetting/DEF::user2",     sizeof("Data>>/pubSetting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,  "Data>>/pubSetting/DEF::user3",     sizeof("Data>>/pubSetting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_SHARED_CACHE_DB_NAME,  true,  "Data>>/pubSetting/DEF::user4",     sizeof("Data>>/pubSetting/DEF::user4")},
    
    //GR10
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingA",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr10_SettingA",               sizeof("Data>>/gr10_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB",  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    false, "Data>>/gr10_SettingB::user2::seat1", sizeof("Data>>/gr10_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingB",  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    false, "Data>>/gr10_SettingB::user2:seat2",  sizeof("Data>>/gr10_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingC",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr10_SettingC",               sizeof("Data>>/gr10_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr10_Setting/ABC::user1",     sizeof("Data>>/gr10_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr10_Setting/ABC::user2",     sizeof("Data>>/gr10_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr10_Setting/ABC::user3",     sizeof("Data>>/gr10_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr10_Setting/ABC::user4",     sizeof("Data>>/gr10_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingD",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr10_SettingD",               sizeof("Data>>/gr10_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingE",  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, false, "Data>>/gr10_SettingE::user2:seat1",  sizeof("Data>>/gr10_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingE",  PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, false, "Data>>/gr10_SettingE::user2:seat2",  sizeof("Data>>/gr10_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingF",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr10_SettingF",               sizeof("Data>>/gr10_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr10_Setting/DEF::user1",     sizeof("Data>>/gr10_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr10_Setting/DEF::user2",     sizeof("Data>>/gr10_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr10_Setting/DEF::user3",     sizeof("Data>>/gr10_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"10/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr10_Setting/DEF::user4",     sizeof("Data>>/gr10_Setting/DEF::user4")},

    //GR20
    { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingA",                                PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr20_SettingA",               sizeof("Data>>/gr20_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingB",  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    false, "Data>>/gr20_SettingB::user2::seat1", sizeof("Data>>/gr20_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingB",  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    false, "Data>>/gr20_SettingB::user2:seat2",  sizeof("Data>>/gr20_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingC",                                PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr20_SettingC",               sizeof("Data>>/gr20_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr20_Setting/ABC::user1",     sizeof("Data>>/gr20_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr20_Setting/ABC::user2",     sizeof("Data>>/gr20_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr20_Setting/ABC::user3",     sizeof("Data>>/gr20_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/ABC",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_WT_DB_NAME,    true,  "Data>>/gr20_Setting/ABC::user4",     sizeof("Data>>/gr20_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingD",                                PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr20_SettingD",               sizeof("Data>>/gr20_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingE",  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, false, "Data>>/gr20_SettingE::user2:seat1",  sizeof("Data>>/gr20_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingE",  PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, false, "Data>>/gr20_SettingE::user2:seat2",  sizeof("Data>>/gr20_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingF",                                PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr20_SettingF",               sizeof("Data>>/gr20_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr20_Setting/DEF::user1",     sizeof("Data>>/gr20_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr20_Setting/DEF::user2",     sizeof("Data>>/gr20_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr20_Setting/DEF::user3",     sizeof("Data>>/gr20_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/DEF",                            PERS_ORG_SHARED_GROUP_WT_PATH_"20/"PERS_ORG_SHARED_CACHE_DB_NAME, true,  "Data>>/gr20_Setting/DEF::user4",     sizeof("Data>>/gr20_Setting/DEF::user4")},
    
    //App1
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App1_SettingA",               sizeof("Data>>/App1_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     false, "Data>>/App1_SettingB::user2::seat1", sizeof("Data>>/App1_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     false, "Data>>/App1_SettingB::user2:seat2",  sizeof("Data>>/App1_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App1_SettingC",               sizeof("Data>>/App1_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App1_Setting/ABC::user1",     sizeof("Data>>/App1_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App1_Setting/ABC::user2",     sizeof("Data>>/App1_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App1_Setting/ABC::user3",     sizeof("Data>>/App1_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App1_Setting/ABC::user4",     sizeof("Data>>/App1_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App1_SettingD",               sizeof("Data>>/App1_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  false, "Data>>/App1_SettingE::user2:seat1",  sizeof("Data>>/App1_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",  PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  false, "Data>>/App1_SettingE::user2:seat2",  sizeof("Data>>/App1_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App1_SettingF",               sizeof("Data>>/App1_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App1_Setting/DEF::user1",     sizeof("Data>>/App1_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App1_Setting/DEF::user2",     sizeof("Data>>/App1_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App1_Setting/DEF::user3",     sizeof("Data>>/App1_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App1_Setting/DEF::user4",     sizeof("Data>>/App1_Setting/DEF::user4")},
    
    //App2
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingA",                                PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App2_SettingA",               sizeof("Data>>/App2_SettingA")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingB",  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     false, "Data>>/App2_SettingB::user2::seat1", sizeof("Data>>/App2_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingB",  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     false, "Data>>/App2_SettingB::user2:seat2",  sizeof("Data>>/App2_SettingB::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingC",                                PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App2_SettingC",               sizeof("Data>>/App2_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App2_Setting/ABC::user1",     sizeof("Data>>/App2_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App2_Setting/ABC::user2",     sizeof("Data>>/App2_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App2_Setting/ABC::user3",     sizeof("Data>>/App2_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/ABC",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_WT_DB_NAME,     true,  "Data>>/App2_Setting/ABC::user4",     sizeof("Data>>/App2_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingD",                                PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App2_SettingD",               sizeof("Data>>/App2_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App2_SettingE",  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  false, "Data>>/App2_SettingE::user2:seat1",  sizeof("Data>>/App2_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App2_SettingE",  PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  false, "Data>>/App2_SettingE::user2:seat2",  sizeof("Data>>/App2_SettingE::user2:seat2")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App2_SettingF",                                PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App2_SettingF",               sizeof("Data>>/App2_SettingF")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App2_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App2_Setting/DEF::user1",     sizeof("Data>>/App2_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App2_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App2_Setting/DEF::user2",     sizeof("Data>>/App2_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App2_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App2_Setting/DEF::user3",     sizeof("Data>>/App2_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App2_Setting/DEF",                            PERS_ORG_LOCAL_APP_WT_PATH_"App2/"PERS_ORG_LOCAL_CACHE_DB_NAME,  true,  "Data>>/App2_Setting/DEF::user4",     sizeof("Data>>/App2_Setting/DEF::user4")}
};

expected_file_data_s expectedFileData_shared_public_AfterDeleteUser2Data[50] =
{    
    //public
    { PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/doc1.txt",                                           true,  "File>>/doc1.txt",              sizeof("File>>/doc1.txt")},
    { PERS_ORG_SHARED_PUBLIC_WT_PATH_ PERS_ORG_NODE_FOLDER_NAME"/Docs/doc2.txt",                                      true,  "File>>/Docs/doc2.txt",         sizeof("File>>/Docs/doc2.txt")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/1/docA.txt",                                      true,  "File>>/docA.txt::user1",       sizeof("File>>/docA.txt::user1")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2/docA.txt",                                      true,  "File>>/docA.txt::user2",       sizeof("File>>/docA.txt::user2")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/3/docA.txt",                                      true,  "File>>/docA.txt::user3",       sizeof("File>>/docA.txt::user3")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/4/docA.txt",                                      true,  "File>>/docA.txt::user4",       sizeof("File>>/docA.txt::user4")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",    false, "File>>/docB.txt::user2:seat1", sizeof("File>>/docB.txt::user2:seat1")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",    false, "File>>/docB.txt::user2:seat2", sizeof("File>>/docB.txt::user2:seat2")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",    false, "File>>/docB.txt::user2:seat3", sizeof("File>>/docB.txt::user2:seat3")},
    { PERS_ORG_SHARED_PUBLIC_CACHE_PATH_ PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",    false, "File>>/docB.txt::user2:seat4", sizeof("File>>/docB.txt::user2:seat4")},
    
    //group10
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                     true,  "File>>gr10_>>/gr10_1.txt"                   ,  sizeof("File>>gr10_>>/gr10_1.txt"                     )},
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10" PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_A.txt",                                true,  "File>>gr10_>>/Docs/gr10_A.txt"              ,  sizeof("File>>gr10_>>/Docs/gr10_A.txt"                )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/gr10_2.txt",                                  true,  "File>>gr10_>>/gr10_2.txt::user1"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user1"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/gr10_2.txt",                                  true,  "File>>gr10_>>/gr10_2.txt::user2"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user2"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/gr10_2.txt",                                  true,  "File>>gr10_>>/gr10_2.txt::user3"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user3"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/gr10_2.txt",                                  true,  "File>>gr10_>>/gr10_2.txt::user4"            ,  sizeof("File>>gr10_>>/gr10_2.txt::user4"              )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",false, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat1"        )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",false, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat2"        )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",false, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat3"        )},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",false, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4" ,  sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat4"        )},


    //group20
    { PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      true,  "File>>gr20_>>/doc1.txt",              sizeof("File>>gr20_>>/doc1.txt")},
    { PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 true,  "File>>gr20_>>/Docs/doc2.txt",         sizeof("File>>gr20_>>/Docs/doc2.txt")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                   true,  "File>>gr20_>>/docA.txt::user1",       sizeof("File>>gr20_>>/docA.txt::user1")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                   true,  "File>>gr20_>>/docA.txt::user2",       sizeof("File>>gr20_>>/docA.txt::user2")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                   true,  "File>>gr20_>>/docA.txt::user3",       sizeof("File>>gr20_>>/docA.txt::user3")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                   true,  "File>>gr20_>>/docA.txt::user4",       sizeof("File>>gr20_>>/docA.txt::user4")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt", false, "File>>gr20_>>/docB.txt::user2:seat1", sizeof("File>>gr20_>>/docB.txt::user2:seat1")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt", false, "File>>gr20_>>/docB.txt::user2:seat2", sizeof("File>>gr20_>>/docB.txt::user2:seat2")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt", false, "File>>gr20_>>/docB.txt::user2:seat3", sizeof("File>>gr20_>>/docB.txt::user2:seat3")},
    { PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt", false, "File>>gr20_>>/docB.txt::user2:seat4", sizeof("File>>gr20_>>/docB.txt::user2:seat4")},
    
    //Appl1
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                       true,  "File>>App1>>/doc1.txt",              sizeof("File>>App1>>/doc1.txt")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                  true,  "File>>App1>>/Docs/doc2.txt",         sizeof("File>>App1>>/Docs/doc2.txt")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                    true,  "File>>App1>>/docA.txt::user1",       sizeof("File>>App1>>/docA.txt::user1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                    true,  "File>>App1>>/docA.txt::user2",       sizeof("File>>App1>>/docA.txt::user2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                    true,  "File>>App1>>/docA.txt::user3",       sizeof("File>>App1>>/docA.txt::user3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                    true,  "File>>App1>>/docA.txt::user4",       sizeof("File>>App1>>/docA.txt::user4")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  false, "File>>App1>>/docB.txt::user2:seat1", sizeof("File>>App1>>/docB.txt::user2:seat1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  false, "File>>App1>>/docB.txt::user2:seat2", sizeof("File>>App1>>/docB.txt::user2:seat2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  false, "File>>App1>>/docB.txt::user2:seat3", sizeof("File>>App1>>/docB.txt::user2:seat3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  false, "File>>App1>>/docB.txt::user2:seat4", sizeof("File>>App1>>/docB.txt::user2:seat4")},
    
    //Appl2
    { PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                       true,  "File>>App2>>/doc1.txt",              sizeof("File>>App2>>/doc1.txt")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App2" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                  true,  "File>>App2>>/Docs/doc2.txt",         sizeof("File>>App2>>/Docs/doc2.txt")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                    true,  "File>>App2>>/docA.txt::user1",       sizeof("File>>App2>>/docA.txt::user1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                    true,  "File>>App2>>/docA.txt::user2",       sizeof("File>>App2>>/docA.txt::user2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                    true,  "File>>App2>>/docA.txt::user3",       sizeof("File>>App2>>/docA.txt::user3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                    true,  "File>>App2>>/docA.txt::user4",       sizeof("File>>App2>>/docA.txt::user4")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  false, "File>>App2>>/docB.txt::user2:seat1", sizeof("File>>App2>>/docB.txt::user2:seat1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  false, "File>>App2>>/docB.txt::user2:seat2", sizeof("File>>App2>>/docB.txt::user2:seat2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  false, "File>>App2>>/docB.txt::user2:seat3", sizeof("File>>App2>>/docB.txt::user2:seat3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App2"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  false, "File>>App2>>/docB.txt::user2:seat4", sizeof("File>>App2>>/docB.txt::user2:seat4")}
} ;

/**************************************************************************************************************
*****************************************    ADD TEST CASES HERE   ********************************************
**************************************************************************************************************/
bool_t Test_DataAfterDeleteUser2Data(int ceva, void* pAltceva)
{
    bool_t bEverythingOK = true ;

    if ( 0 > persAdminUserDataDelete(2, 1) )
    {
        bEverythingOK = false;
    }
    
    if ( bEverythingOK )
    {
        if ( 0 > persAdminUserDataDelete(2, 2) )
        {
            bEverythingOK = false;
        }
    }
    
    if ( bEverythingOK )
    {
        if ( 0 > persAdminUserDataDelete(2, 3) )
        {
            bEverythingOK = false;
        }
    }
    
    if ( bEverythingOK )
    {
        if ( 0 > persAdminUserDataDelete(2, 4) )
        {
            bEverythingOK = false;
        }
    }

    printf("\nTest_DataAfterDeleteUser2Data: DeleteUser2Data() - %s \n", bEverythingOK ? "OK" : "FAILED") ;

    return bEverythingOK ;
}
