/*********************************************************************************************************************** Copyright (C) 2012 Continental Automotive Systems, Inc.** Author: Petrica.Manoila@continental-corporation.com** Implementation of backup process** This Source Code Form is subject to the terms of the Mozilla Public* License, v. 2.0. If a copy of the MPL was not distributed with this* file, You can obtain one at http://mozilla.org/MPL/2.0/.** Date       Author             Reason* 2012.11.27 uidu0250           CSP_WZ#1280:  Initial version***********************************************************************************************************************/#include "persComTypes.h"
#include "stdio.h"
#include "string.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>

#include "dlt/dlt.h"

#include "persistence_admin_service.h"
#include "test_PAS.h"
#include "test_pas_resource_config_add.h"

#include "persComDbAccess.h"

#define File_t PersistenceResourceType_file
#define Key_t PersistenceResourceType_key

/* L&T context */
#define LT_HDR                          "TEST_PAS_CONF >> "
DLT_IMPORT_CONTEXT                      (persAdminSvcDLTCtx)
static char g_msg[512] ;


#define RESOURCE_PATH "/tmp/var/resourceConfig"
#define RESOURCE_PATH_ RESOURCE_PATH"/"

#define RESOURCE_ARCHIVE_PATHNAME_1 "/tmp/PAS/resource1.tar.gz"
#define RESOURCE_ARCHIVE_PATHNAME_2 "/tmp/PAS/resource2.tar.gz"

/**********************************************************************************************************************************************
 *********************************************** Expected - public *****************************************************************************
 *********************************************************************************************************************************************/
expected_key_data_RCT_s expected_RCT_public[13] =
{
    //pubSettingA changed policy (wt -> wc)
    {"pubSettingA",                PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingB",                PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingC",                PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/ABC",             PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSettingD",                PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    //pubSettingE changed policy (wc -> wt)
    {"pubSettingE",                PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    //pubSettingF removed
    {"pubSettingF",                PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, false,  {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"pubSetting/DEF",             PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    //new_pubSetting_1 added
    {"new_pubSetting_1",           PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_shared,Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"doc1.txt",                   PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",              PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"docA.txt",                   PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}},
    {"Docs/docB.txt",              PERS_ORG_SHARED_PUBLIC_CACHE_PATH"/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_shared,File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"public"}, {NIL},{NIL}}}
} ;

/* data after phase 1 */
expected_key_data_localDB_s expectedKeyData_public[29] = 
{
    //pubSettingA changed policy (wt -> wc)
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                   PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        false,   
            "Data>>/pubSettingA",                           sizeof("Data>>/pubSettingA")},
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                   PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    true,  
            "Data>>/pubSettingA",                           sizeof("Data>>/pubSettingA")},
    {"pubSettingA",                         PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           true,  
            "FactoryDefault : pubSettingA : orig",          sizeof("FactoryDefault : pubSettingA : orig")},
    {"pubSettingA",                         PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      true,  
            "ConfigurableDefault : pubSettingA : orig",     sizeof("ConfigurableDefault : pubSettingA : orig")},

    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",          PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSettingB::user2::seat1",             sizeof("Data>>/pubSettingB::user2::seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",          PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSettingB::user2:seat2",              sizeof("Data>>/pubSettingB::user2:seat2")},
    //added factory default value in config file
    {"pubSettingB",                         PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           true,  
            "FactoryDefault : pubSettingB : new",           sizeof("FactoryDefault : pubSettingB : new")},

    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                   PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSettingC",                           sizeof("Data>>/pubSettingC")},

    {PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSetting/ABC::user1",                 sizeof("Data>>/pubSetting/ABC::user1")},
    {PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSetting/ABC::user2",                 sizeof("Data>>/pubSetting/ABC::user2")},
    {PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSetting/ABC::user3",                 sizeof("Data>>/pubSetting/ABC::user3")},
    {PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSetting/ABC::user4",                 sizeof("Data>>/pubSetting/ABC::user4")},            
    {"pubSetting/ABC",                      PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           true,  
            "FactoryDefault : pubSetting/ABC : orig",       sizeof("FactoryDefault : pubSetting/ABC : orig")},
    #if 0 //configurable-default values changed by config file
    {"pubSetting/ABC",                      PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      true,  
            "ConfigurableDefault : pubSetting/ABC : orig",  sizeof("ConfigurableDefault : pubSetting/ABC : orig")},
    #endif
    {"pubSetting/ABC",                      PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      true,  
            "ConfigurableDefault : pubSetting/ABC : new",   sizeof("ConfigurableDefault : pubSetting/ABC : new")},

    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                   PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    true,   
            "Data>>/pubSettingD",                           sizeof("Data>>/pubSettingD")},
    #if 0 //factory-default value changed by config file
    {"pubSettingD",                         PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           true,  
            "FactoryDefault : pubSettingD : orig",          sizeof("FactoryDefault : pubSettingD : orig")},
    #endif
    {"pubSettingD",                         PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           true,  
            "FactoryDefault : pubSettingD : new",           sizeof("FactoryDefault : pubSettingD : new")},
    //configurable-default value added by config file
    {"pubSettingD",                         PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      true,  
            "ConfigurableDefault : pubSettingD : new",      sizeof("ConfigurableDefault : pubSettingD : new")},

    //pubSettingE changed policy (wc -> wt)
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",          PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSettingE::user2:seat1",              sizeof("Data>>/pubSettingE::user2:seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",          PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    false,   
            "Data>>/pubSettingE::user2:seat1",              sizeof("Data>>/pubSettingE::user2:seat1")}, 
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",          PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        true,   
            "Data>>/pubSettingE::user2:seat2",              sizeof("Data>>/pubSettingE::user2:seat2")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",          PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    false,   
            "Data>>/pubSettingE::user2:seat2",              sizeof("Data>>/pubSettingE::user2:seat2")}, 

    //pubSettingF removed
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                   PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        false,   
            "Data>>/pubSettingF",                           sizeof("Data>>/pubSettingF")},    
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                   PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    false,   
            "Data>>/pubSettingF",                           sizeof("Data>>/pubSettingF")},  

    {PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    true,   
            "Data>>/pubSetting/DEF::user1",                 sizeof("Data>>/pubSetting/DEF::user1")},  
    {PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    true,   
            "Data>>/pubSetting/DEF::user2",                 sizeof("Data>>/pubSetting/DEF::user2")},  
    {PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    true,   
            "Data>>/pubSetting/DEF::user3",                 sizeof("Data>>/pubSetting/DEF::user3")},  
    {PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    true,   
            "Data>>/pubSetting/DEF::user4",                 sizeof("Data>>/pubSetting/DEF::user4")},  

    //new_pubSetting_1 - added factory-default value in config file
    {"new_pubSetting_1",                    PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           true,  
            "FactoryDefault : new_pubSetting_1 : new",      sizeof("FactoryDefault : new_pubSetting_1 : new")},
    //new_pubSetting_1 - added configurable-default value in config file
    {"new_pubSetting_1",                    PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      true,  
            "ConfigurableDefault : new_pubSetting_1 : new", sizeof("ConfigurableDefault : new_pubSetting_1 : new")}
} ;


/* data after phase 2 - uninstall non-default data (pubSettingB excepted) */
expected_key_data_localDB_s expectedKeyData_public_phase2[29] = 
{
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSettingA",                                       sizeof("Data>>/pubSettingA")},
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingA",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSettingA",                                       sizeof("Data>>/pubSettingA")},
    {"pubSettingA",                                                             PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           
            true,   "FactoryDefault : pubSettingA : orig",                      sizeof("FactoryDefault : pubSettingA : orig")},
    {"pubSettingA",                                                             PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      
            true,   "ConfigurableDefault : pubSettingA : orig",                 sizeof("ConfigurableDefault : pubSettingA : orig")},

    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingB",    PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            true,   "Data>>/pubSettingB::user2::seat1",                         sizeof("Data>>/pubSettingB::user2::seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingB",    PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            true,   "Data>>/pubSettingB::user2:seat2",                          sizeof("Data>>/pubSettingB::user2:seat2")},
    {"pubSettingB",                                                             PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           
            true,   "FactoryDefault : pubSettingB : new",                       sizeof("FactoryDefault : pubSettingB : new")},

    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingC",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSettingC",                                       sizeof("Data>>/pubSettingC")},

    {PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSetting/ABC::user1",                             sizeof("Data>>/pubSetting/ABC::user1")},
    {PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSetting/ABC::user2",                             sizeof("Data>>/pubSetting/ABC::user2")},
    {PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSetting/ABC::user3",                             sizeof("Data>>/pubSetting/ABC::user3")},
    {PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/ABC",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSetting/ABC::user4",                             sizeof("Data>>/pubSetting/ABC::user4")},            
    {"pubSetting/ABC",                                                          PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           
            true,   "FactoryDefault : pubSetting/ABC : orig",                   sizeof("FactoryDefault : pubSetting/ABC : orig")},

    {"pubSetting/ABC",                                                          PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      
            true,   "ConfigurableDefault : pubSetting/ABC : new",               sizeof("ConfigurableDefault : pubSetting/ABC : new")},

    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingD",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSettingD",                                       sizeof("Data>>/pubSettingD")},

    {"pubSettingD",                                                             PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           
            true,   "FactoryDefault : pubSettingD : new",                       sizeof("FactoryDefault : pubSettingD : new")},
    {"pubSettingD",                                                             PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      
            true,   "ConfigurableDefault : pubSettingD : new",                  sizeof("ConfigurableDefault : pubSettingD : new")},

    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",    PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSettingE::user2:seat1",                          sizeof("Data>>/pubSettingE::user2:seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/pubSettingE",    PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSettingE::user2:seat1",                          sizeof("Data>>/pubSettingE::user2:seat1")}, 
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",    PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSettingE::user2:seat2",                          sizeof("Data>>/pubSettingE::user2:seat2")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/pubSettingE",    PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSettingE::user2:seat2",                          sizeof("Data>>/pubSettingE::user2:seat2")}, 

    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_WT_DB_NAME_,                        
            false,  "Data>>/pubSettingF",                                       sizeof("Data>>/pubSettingF")},    
    {PERS_ORG_NODE_FOLDER_NAME_"/pubSettingF",                                  PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSettingF",                                       sizeof("Data>>/pubSettingF")},  

    {PERS_ORG_USER_FOLDER_NAME_"1/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSetting/DEF::user1",                             sizeof("Data>>/pubSetting/DEF::user1")},  
    {PERS_ORG_USER_FOLDER_NAME_"2/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSetting/DEF::user2",                             sizeof("Data>>/pubSetting/DEF::user2")},  
    {PERS_ORG_USER_FOLDER_NAME_"3/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSetting/DEF::user3",                             sizeof("Data>>/pubSetting/DEF::user3")},  
    {PERS_ORG_USER_FOLDER_NAME_"4/pubSetting/DEF",                              PERS_ORG_SHARED_PUBLIC_WT_PATH""PERS_ORG_LOCAL_CACHE_DB_NAME_,                    
            false,  "Data>>/pubSetting/DEF::user4",                             sizeof("Data>>/pubSetting/DEF::user4")},  

    {"new_pubSetting_1",                                                        PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME,           
            true,   "FactoryDefault : new_pubSetting_1 : new",                  sizeof("FactoryDefault : new_pubSetting_1 : new")},
    {"new_pubSetting_1",                                                        PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME,      
            true,   "ConfigurableDefault : new_pubSetting_1 : new",             sizeof("ConfigurableDefault : new_pubSetting_1 : new")}
};

expected_file_data_s expectedFileData_public[18] =
{
/* factory-default - installed via config */
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""doc1.txt",                            true, "File>>/doc1.txt factory-default",            sizeof("File>>/doc1.txt factory-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""docA.txt",                            true, "File>>/docA.txt factory-default",            sizeof("File>>/docA.txt factory-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""Docs/doc2.txt",                       true, "File>>/Docs/doc2.txt factory-default",       sizeof("File>>/Docs/doc2.txt factory-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""Docs/docB.txt",                       true, "File>>/Docs/docB.txt factory-default",       sizeof("File>>/Docs/docB.txt factory-default")-1},
/* configurable-default - installed via config */
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""doc1.txt",                     true, "File>>/doc1.txt configurable-default",       sizeof("File>>/doc1.txt configurable-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""docA.txt",                     true, "File>>/docA.txt configurable-default",       sizeof("File>>/docA.txt configurable-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""Docs/doc2.txt",                true, "File>>/Docs/doc2.txt configurable-default",  sizeof("File>>/Docs/doc2.txt configurable-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""Docs/docB.txt",                true, "File>>/Docs/docB.txt configurable-default",  sizeof("File>>/Docs/docB.txt configurable-default")-1},
/* non-default - pre-installed */
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_NODE_FOLDER_NAME"/""doc1.txt",                                    true, "File>>/doc1.txt",                            sizeof("File>>/doc1.txt")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_NODE_FOLDER_NAME"/""Docs/doc2.txt",                               true, "File>>/Docs/doc2.txt",                       sizeof("File>>/Docs/doc2.txt")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/1/docA.txt",                                    true, "File>>/docA.txt::user1",                     sizeof("File>>/docA.txt::user1")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2/docA.txt",                                    true, "File>>/docA.txt::user2",                     sizeof("File>>/docA.txt::user2")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/3/docA.txt",                                    true, "File>>/docA.txt::user3",                     sizeof("File>>/docA.txt::user3")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/4/docA.txt",                                    true, "File>>/docA.txt::user4",                     sizeof("File>>/docA.txt::user4")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat1",               sizeof("File>>/docB.txt::user2:seat1")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat2",               sizeof("File>>/docB.txt::user2:seat2")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat3",               sizeof("File>>/docB.txt::user2:seat3")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  true, "File>>/docB.txt::user2:seat4",               sizeof("File>>/docB.txt::user2:seat4")}
};

/* data after phase 2 - uninstall non-default data (docA.txt excepted) */
expected_file_data_s expectedFileData_public_phase2[18] =
{
/* factory-default - installed via config */
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""doc1.txt",                            true, "File>>/doc1.txt factory-default",            sizeof("File>>/doc1.txt factory-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""docA.txt",                            true, "File>>/docA.txt factory-default",            sizeof("File>>/docA.txt factory-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""Docs/doc2.txt",                       true, "File>>/Docs/doc2.txt factory-default",       sizeof("File>>/Docs/doc2.txt factory-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_DEFAULT_DATA_FOLDER_NAME"/""Docs/docB.txt",                       true, "File>>/Docs/docB.txt factory-default",       sizeof("File>>/Docs/docB.txt factory-default")-1},
/* configurable-default - installed via config */
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""doc1.txt",                     true, "File>>/doc1.txt configurable-default",       sizeof("File>>/doc1.txt configurable-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""docA.txt",                     true, "File>>/docA.txt configurable-default",       sizeof("File>>/docA.txt configurable-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""Docs/doc2.txt",                true, "File>>/Docs/doc2.txt configurable-default",  sizeof("File>>/Docs/doc2.txt configurable-default")-1},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME"/""Docs/docB.txt",                true, "File>>/Docs/docB.txt configurable-default",  sizeof("File>>/Docs/docB.txt configurable-default")-1},
/* non-default - pre-installed */
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_NODE_FOLDER_NAME"/""doc1.txt",                                    false,"File>>/doc1.txt",                            sizeof("File>>/doc1.txt")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_NODE_FOLDER_NAME"/""Docs/doc2.txt",                               false,"File>>/Docs/doc2.txt",                       sizeof("File>>/Docs/doc2.txt")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/1/docA.txt",                                    true, "File>>/docA.txt::user1",                     sizeof("File>>/docA.txt::user1")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2/docA.txt",                                    true, "File>>/docA.txt::user2",                     sizeof("File>>/docA.txt::user2")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/3/docA.txt",                                    true, "File>>/docA.txt::user3",                     sizeof("File>>/docA.txt::user3")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/4/docA.txt",                                    true, "File>>/docA.txt::user4",                     sizeof("File>>/docA.txt::user4")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",  false,"File>>/docB.txt::user2:seat1",               sizeof("File>>/docB.txt::user2:seat1")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",  false,"File>>/docB.txt::user2:seat2",               sizeof("File>>/docB.txt::user2:seat2")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",  false,"File>>/docB.txt::user2:seat3",               sizeof("File>>/docB.txt::user2:seat3")},
    {PERS_ORG_SHARED_PUBLIC_WT_PATH"/"PERS_ORG_USER_FOLDER_NAME"/2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",  false,"File>>/docB.txt::user2:seat4",               sizeof("File>>/docB.txt::user2:seat4")}
};

/**********************************************************************************************************************************************
 *********************************************** Expected - group/10 *****************************************************************************
 *********************************************************************************************************************************************/
expected_key_data_RCT_s expected_RCT_group_10[12] =
{
    {"gr10_SettingA",       PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t, S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingB",       PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingC",       PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_Setting/ABC",    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingD",       PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingE",       PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_SettingF",       PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_Setting/DEF",    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_1.txt",          PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"Docs/gr10_A.txt",     PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"gr10_2.txt",          PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}},
    {"Docs/gr10_B.txt",     PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group10"}, {NIL},{NIL}}}
} ;

expected_key_data_localDB_s expectedKeyData_group_10[24] = 
{
/* non-default data : pre-installed */
    {PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingA",                                    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_SettingA",                     sizeof("Data>>/gr10_SettingA")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingB",      PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_SettingB::user2::seat1",       sizeof("Data>>/gr10_SettingB::user2::seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingB",      PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_SettingB::user2:seat2",        sizeof("Data>>/gr10_SettingB::user2:seat2")},
    //no longer in - deleted by installException
    {PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingC",                                    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               false,   
            "Data>>/gr10_SettingC",                     sizeof("Data>>/gr10_SettingC")},
    {PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/ABC",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_Setting/ABC::user1",           sizeof("Data>>/gr10_Setting/ABC::user1")},
    {PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/ABC",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_Setting/ABC::user2",           sizeof("Data>>/gr10_Setting/ABC::user2")},
    {PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/ABC",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_Setting/ABC::user3",           sizeof("Data>>/gr10_Setting/ABC::user3")},
    {PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/ABC",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_WT_DB_NAME_,               true,   
            "Data>>/gr10_Setting/ABC::user4",           sizeof("Data>>/gr10_Setting/ABC::user4")},
    {PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingD",                                    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_SettingD",                     sizeof("Data>>/gr10_SettingD")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr10_SettingE",      PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_SettingE::user2:seat1",        sizeof("Data>>/gr10_SettingE::user2:seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr10_SettingE",      PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_SettingE::user2:seat2",        sizeof("Data>>/gr10_SettingE::user2:seat2")},
    {PERS_ORG_NODE_FOLDER_NAME_"/gr10_SettingF",                                    PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_SettingF",                     sizeof("Data>>/gr10_SettingF")},
    {PERS_ORG_USER_FOLDER_NAME_"1/gr10_Setting/DEF",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_Setting/DEF::user1",           sizeof("Data>>/gr10_Setting/DEF::user1")},
    {PERS_ORG_USER_FOLDER_NAME_"2/gr10_Setting/DEF",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_Setting/DEF::user2",           sizeof("Data>>/gr10_Setting/DEF::user2")},
    {PERS_ORG_USER_FOLDER_NAME_"3/gr10_Setting/DEF",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_Setting/DEF::user3",           sizeof("Data>>/gr10_Setting/DEF::user3")},
    {PERS_ORG_USER_FOLDER_NAME_"4/gr10_Setting/DEF",                                PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CACHE_DB_NAME_,            true,   
            "Data>>/gr10_Setting/DEF::user4",           sizeof("Data>>/gr10_Setting/DEF::user4")},
/* factory-default data : pre-installed */
    //not updated because factory-default data not available in config file
    {"gr10_SettingA",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,   
            "FactoryDefault : gr10_SettingA : orig",    sizeof("FactoryDefault : gr10_SettingA : orig")},
    {"gr10_SettingB",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,   
            "FactoryDefault : gr10_SettingB : orig",    sizeof("FactoryDefault : gr10_SettingB : orig")},
    //deleted by config file
    {"gr10_SettingC",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        false,   
            "FactoryDefault : gr10_SettingC : orig",    sizeof("FactoryDefault : gr10_SettingC : orig")},
    {"gr10_Setting/ABC",                      PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,   
            "FactoryDefault : gr10_Setting/ABC : orig", sizeof("FactoryDefault : gr10_Setting/ABC : orig")},
/* configurable-default data : pre-installed */
    #if 0 //updated by config file
    {"gr10_SettingA",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,  true,   
            "ConfigurableDefault : gr10_SettingA : orig",       sizeof("ConfigurableDefault : gr10_SettingA : orig")},
    #endif 
    {"gr10_SettingA",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,   
            "ConfigurableDefault : gr10_SettingA : new",        sizeof("ConfigurableDefault : gr10_SettingA : new")},
    {"gr10_SettingB",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,   
            "ConfigurableDefault : gr10_SettingB : orig",       sizeof("ConfigurableDefault : gr10_SettingB : orig")},
    //deleted by config file
    {"gr10_SettingC",                         PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   false,   
            "ConfigurableDefault : gr10_SettingC : orig",       sizeof("ConfigurableDefault : gr10_SettingC : orig")},
    {"gr10_Setting/ABC",                      PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,   
            "ConfigurableDefault : gr10_Setting/ABC : orig",    sizeof("ConfigurableDefault : gr10_Setting/ABC : orig")}
} ;

expected_file_data_s expectedFileData_group_10[18] =
{
/* pre-installed */
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_NODE_FOLDER_NAME_"/gr10_1.txt",                                         true, "File>>gr10_>>/gr10_1.txt",                                   sizeof("File>>gr10_>>/gr10_1.txt")},
    #if 0 //updated by config file
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_A.txt",                                    true, "File>>gr10_>>/Docs/gr10_A.txt",                              sizeof("File>>gr10_>>/Docs/gr10_A.txt")},
    #endif
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_NODE_FOLDER_NAME_"/Docs/gr10_A.txt",                                    false,"File>>gr10_>>/Docs/gr10_A.txt",                              sizeof("File>>gr10_>>/Docs/gr10_A.txt")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"1/gr10_2.txt",                                     true, "File>>gr10_>>/gr10_2.txt::user1",                            sizeof("File>>gr10_>>/gr10_2.txt::user1")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2/gr10_2.txt",                                     true, "File>>gr10_>>/gr10_2.txt::user2",                            sizeof("File>>gr10_>>/gr10_2.txt::user2")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"3/gr10_2.txt",                                     true, "File>>gr10_>>/gr10_2.txt::user3",                            sizeof("File>>gr10_>>/gr10_2.txt::user3")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"4/gr10_2.txt",                                     true, "File>>gr10_>>/gr10_2.txt::user4",                            sizeof("File>>gr10_>>/gr10_2.txt::user4")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/gr10_B.txt",   true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat1",                 sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat1")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/gr10_B.txt",   true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat2",                 sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat2")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/gr10_B.txt",   true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat3",                 sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat3")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/gr10_B.txt",   true, "File>>gr10_>>/Docs/gr10_B.txt::user2:seat4",                 sizeof("File>>gr10_>>/Docs/gr10_B.txt::user2:seat4")},

    /* factory-default data */
    #if 0 //updated by config file
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr10_1.txt",                                 true, "File>>gr10_>>/gr10_1.txt factory-default : orig",            sizeof("File>>gr10_>>/gr10_1.txt factory-default : orig")},
    #endif
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr10_1.txt",                                 true, "File>>gr10_>>/gr10_1.txt factory-default : new",             sizeof("File>>gr10_>>/gr10_1.txt factory-default : new")},
    //deleted by config file
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_A.txt",                            false, "File>>gr10_>>/Docs/gr10_A.txt factory-default : orig",      sizeof("File>>gr10_>>/Docs/gr10_A.txt factory-default : orig")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr10_2.txt",                              true, "File>>gr10_>>/gr10_2.txt factory-default : orig",            sizeof("File>>gr10_>>/gr10_2.txt factory-default : orig")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_B.txt",                         true, "File>>gr10_>>/Docs/gr10_B.txt factory-default : orig",       sizeof("File>>gr10_>>/Docs/gr10_B.txt factory-default : orig")},
    /* configurable-default data */
    #if 0 //updated by config file
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr10_1.txt",                          true, "File>>gr10_>>/gr10_1.txt configurable-default : orig",       sizeof("File>>gr10_>>/gr10_1.txt configurable-default : orig")},
    #endif
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr10_1.txt",                          true, "File>>gr10_>>/gr10_1.txt configurable-default : new",        sizeof("File>>gr10_>>/gr10_1.txt configurable-default : new")},
    //deleted by config file
    {PERS_ORG_SHARED_GROUP_WT_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_A.txt",                     false,"File>>gr10_>>/Docs/gr10_A.txt configurable-default : orig",  sizeof("File>>gr10_>>/Docs/gr10_A.txt configurable-default : orig")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr10_2.txt",                       true, "File>>gr10_>>/gr10_2.txt configurable-default : orig",       sizeof("File>>gr10_>>/gr10_2.txt configurable-default : orig")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"10"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/Docs/gr10_B.txt",                  true, "File>>gr10_>>/Docs/gr10_B.txt configurable-default : orig",  sizeof("File>>gr10_>>/Docs/gr10_B.txt configurable-default : orig")},

};

/**********************************************************************************************************************************************
 *********************************************** Expected - group/20 *****************************************************************************
 *********************************************************************************************************************************************/


expected_key_data_RCT_s expected_RCT_group_20[12] =
{
    {"gr20_SettingA",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingB",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingC",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_Setting/ABC",    PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingD",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingE",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_SettingF",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"gr20_Setting/DEF",    PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"doc1.txt",            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"Docs/doc2.txt",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"docA.txt",            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}},
    {"Docs/docB.txt",       PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group20"},{NIL},{NIL}}}
} ;

expected_key_data_localDB_s expectedKeyData_group_20[16] = 
{
/* non-default data : pre-installed */
    {PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingA", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_SettingA",                                             sizeof("Data>>/gr20_SettingA")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingB",           
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_SettingB::user2::seat1",                               sizeof("Data>>/gr20_SettingB::user2::seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingB",           
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_SettingB::user2:seat2",                                sizeof("Data>>/gr20_SettingB::user2:seat2")},
    {PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingC",                                         
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_SettingC",                                             sizeof("Data>>/gr20_SettingC")},
    {PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/ABC",                                     
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_Setting/ABC::user1",                                   sizeof("Data>>/gr20_Setting/ABC::user1")},
    {PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/ABC",                                     
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_Setting/ABC::user2",                                   sizeof("Data>>/gr20_Setting/ABC::user2")},
    {PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/ABC",                                     
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_Setting/ABC::user3",                                   sizeof("Data>>/gr20_Setting/ABC::user3")},
    {PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/ABC",                                     
            PERS_ORG_SHARED_GROUP_WT_PATH_"20"PERS_ORG_LOCAL_WT_DB_NAME_,       true,
            "Data>>/gr20_Setting/ABC::user4",                                   sizeof("Data>>/gr20_Setting/ABC::user4")},
    {PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingD",                                         
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_SettingD",                                             sizeof("Data>>/gr20_SettingD")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/gr20_SettingE",           
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_SettingE::user2:seat1",                                sizeof("Data>>/gr20_SettingE::user2:seat1")},
    {PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/gr20_SettingE",           
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_SettingE::user2:seat2",                                sizeof("Data>>/gr20_SettingE::user2:seat2")},
    {PERS_ORG_NODE_FOLDER_NAME_"/gr20_SettingF",                                         
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_SettingF",                                             sizeof("Data>>/gr20_SettingF")},
    {PERS_ORG_USER_FOLDER_NAME_"1/gr20_Setting/DEF",                                     
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_Setting/DEF::user1",                                   sizeof("Data>>/gr20_Setting/DEF::user1")},
    {PERS_ORG_USER_FOLDER_NAME_"2/gr20_Setting/DEF",                                     
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_Setting/DEF::user2",                                   sizeof("Data>>/gr20_Setting/DEF::user2")},
    {PERS_ORG_USER_FOLDER_NAME_"3/gr20_Setting/DEF",                                     
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_Setting/DEF::user3",                                   sizeof("Data>>/gr20_Setting/DEF::user3")},
    {PERS_ORG_USER_FOLDER_NAME_"4/gr20_Setting/DEF",                                     
            PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_LOCAL_CACHE_DB_NAME_, true,
            "Data>>/gr20_Setting/DEF::user4",                                   sizeof("Data>>/gr20_Setting/DEF::user4")}
};
expected_file_data_s expectedFileData_group_20[10] =
{
/* pre-installed */
    {PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                          true,   "File>>gr20_>>/doc1.txt",               sizeof("File>>gr20_>>/doc1.txt")},
    {PERS_ORG_SHARED_GROUP_WT_PATH_"20" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                     true,   "File>>gr20_>>/Docs/doc2.txt",          sizeof("File>>gr20_>>/Docs/doc2.txt")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                       true,   "File>>gr20_>>/docA.txt::user1",        sizeof("File>>gr20_>>/docA.txt::user1")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                       true,   "File>>gr20_>>/docA.txt::user2",        sizeof("File>>gr20_>>/docA.txt::user2")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                       true,   "File>>gr20_>>/docA.txt::user3",        sizeof("File>>gr20_>>/docA.txt::user3")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                       true,   "File>>gr20_>>/docA.txt::user4",        sizeof("File>>gr20_>>/docA.txt::user4")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt",     true,   "File>>gr20_>>/docB.txt::user2:seat1",  sizeof("File>>gr20_>>/docB.txt::user2:seat1")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt",     true,   "File>>gr20_>>/docB.txt::user2:seat2",  sizeof("File>>gr20_>>/docB.txt::user2:seat2")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt",     true,   "File>>gr20_>>/docB.txt::user2:seat3",  sizeof("File>>gr20_>>/docB.txt::user2:seat3")},
    {PERS_ORG_SHARED_GROUP_CACHE_PATH_"20"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt",     true,   "File>>gr20_>>/docB.txt::user2:seat4",  sizeof("File>>gr20_>>/docB.txt::user2:seat4")}
};

/**********************************************************************************************************************************************
 *********************************************** Expected - group/30 *****************************************************************************
 *********************************************************************************************************************************************/
/* Group 30 - new install based exclusively on configuration input (json fles) */
expected_key_data_RCT_s expected_RCT_group_30[6] =
{
    {"gr30_SettingA",       PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group30"},{NIL},{NIL}}},
    {"gr30_SettingB",       PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group30"},{NIL},{NIL}}},
    {"gr30_SettingC",       PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group30"},{NIL},{NIL}}},
    {"gr30_SettingD",       PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group30"},{NIL},{NIL}}},
    {"gr30_1.txt",          PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group30"},{NIL},{NIL}}},
    {"gr30_2.txt",          PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"group30"},{NIL},{NIL}}},
} ;

expected_key_data_localDB_s expectedKeyData_group_30[8] = 
{
/* factory-default data : installed via configuration input */
    {"gr30_SettingA", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,       true,
            "gr30_SettingA : FactoryDefault : orig",                                         sizeof("gr30_SettingA : FactoryDefault : orig")},
    {"gr30_SettingB", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,       true,
            "gr30_SettingB : FactoryDefault : orig",                                         sizeof("gr30_SettingB : FactoryDefault : orig")},
    {"gr30_SettingC", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,       true,
            "gr30_SettingC : FactoryDefault : orig",                                         sizeof("gr30_SettingC : FactoryDefault : orig")},
    {"gr30_SettingD", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,       true,
            "gr30_SettingD : FactoryDefault : orig",                                         sizeof("gr30_SettingD : FactoryDefault : orig")},
/* configurable-default data : installed via configuration input */
    {"gr30_SettingA", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,  true,
            "gr30_SettingA : ConfigurableDefault : orig",                                    sizeof("gr30_SettingA : ConfigurableDefault : orig")},
    {"gr30_SettingB", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,  true,
            "gr30_SettingB : ConfigurableDefault : orig",                                    sizeof("gr30_SettingB : ConfigurableDefault : orig")},
    {"gr30_SettingC", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,  true,
            "gr30_SettingC : ConfigurableDefault : orig",                                    sizeof("gr30_SettingC : ConfigurableDefault : orig")},
    {"gr30_SettingD", 
            PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,  true,
            "gr30_SettingD : ConfigurableDefault : orig",                                    sizeof("gr30_SettingD : ConfigurableDefault : orig")},
};

expected_file_data_s expectedFileData_group_30[4] =
{
/* factory-default data : installed via configuration input */
    {PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr30_1.txt",         true,   
            "File>>gr30_>>/gr30_1.txt factory-default : orig",                                  sizeof("File>>gr30_>>/gr30_1.txt factory-default : orig")},
    {PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/gr30_2.txt",         true,   
            "File>>gr30_>>/gr30_2.txt factory-default : orig",                                  sizeof("File>>gr30_>>/gr30_2.txt factory-default : orig")},
/* configurable-default data : installed via configuration input */
    {PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr30_1.txt",  true,   
            "File>>gr30_>>/gr30_1.txt configurable-default : orig",                             sizeof("File>>gr30_>>/gr30_1.txt configurable-default : orig")},
    {PERS_ORG_SHARED_GROUP_WT_PATH_"30"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/gr30_2.txt",  true,   
            "File>>gr30_>>/gr30_2.txt configurable-default : orig",                             sizeof("File>>gr30_>>/gr30_2.txt configurable-default : orig")},
};

/**********************************************************************************************************************************************
 *********************************************** Expected - App30 First Phase *****************************************************************
 *********************************************************************************************************************************************/
/* Group 30 - new install based exclusively on configuration input (json fles) */
expected_key_data_RCT_s expected_RCT_App30_Phase_1[6] =
{
    {"App30_SettingA",       PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App30"},{NIL},{NIL}}},
    {"App30_SettingB",       PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App30"},{NIL},{NIL}}},
    {"App30_SettingC",       PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App30"},{NIL},{NIL}}},
    {"App30_SettingD",       PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, Key_t ,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App30"},{NIL},{NIL}}},
    {"App30_1.txt",          PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wt,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App30"},{NIL},{NIL}}},
    {"App30_2.txt",          PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_RCT_NAME_,   true,   {PersistencePolicy_wc,  PersistenceStorage_shared, File_t,S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App30"},{NIL},{NIL}}},
} ;

expected_key_data_localDB_s expectedKeyData_App30_Phase_1[8] = 
{
/* factory-default data : installed via configuration input */
    {"App30_SettingA", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,
            "App30_SettingA : FactoryDefault : orig",                                         sizeof("App30_SettingA : FactoryDefault : orig")},
    {"App30_SettingB", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,
            "App30_SettingB : FactoryDefault : orig",                                         sizeof("App30_SettingB : FactoryDefault : orig")},
    {"App30_SettingC", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,
            "App30_SettingC : FactoryDefault : orig",                                         sizeof("App30_SettingC : FactoryDefault : orig")},
    {"App30_SettingD", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_FACTORY_DEFAULT_DB_NAME_,        true,
            "App30_SettingD : FactoryDefault : orig",                                         sizeof("App30_SettingD : FactoryDefault : orig")},
/* configurable-default data : installed via configuration input */
    {"App30_SettingA", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,
            "App30_SettingA : ConfigurableDefault : orig",                                    sizeof("App30_SettingA : ConfigurableDefault : orig")},
    {"App30_SettingB", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,
            "App30_SettingB : ConfigurableDefault : orig",                                    sizeof("App30_SettingB : ConfigurableDefault : orig")},
    {"App30_SettingC", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,
            "App30_SettingC : ConfigurableDefault : orig",                                    sizeof("App30_SettingC : ConfigurableDefault : orig")},
    {"App30_SettingD", 
            PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_LOCAL_CONFIGURABLE_DEFAULT_DB_NAME_,   true,
            "App30_SettingD : ConfigurableDefault : orig",                                    sizeof("App30_SettingD : ConfigurableDefault : orig")},
};

expected_file_data_s expectedFileData_App30_Phase_1[4] =
{
/* factory-default data : installed via configuration input */
    {PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/App30_1.txt",        true,   
            "File>>App30_>>/App30_1.txt factory-default : orig",                                sizeof("File>>App30_>>/App30_1.txt factory-default : orig")},
    {PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/App30_2.txt",        true,   
            "File>>App30_>>/App30_2.txt factory-default : orig",                                sizeof("File>>App30_>>/App30_2.txt factory-default : orig")},
/* configurable-default data : installed via configuration input */
    {PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/App30_1.txt", true,   
            "File>>App30_>>/App30_1.txt configurable-default : orig",                           sizeof("File>>App30_>>/App30_1.txt configurable-default : orig")},
    {PERS_ORG_LOCAL_APP_WT_PATH_"App30"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/App30_2.txt", true,   
            "File>>App30_>>/App30_2.txt configurable-default : orig",                           sizeof("File>>App30_>>/App30_2.txt configurable-default : orig")},
};


/***********************************************************************************************************************/
expected_key_data_RCT_s expectedKeyData_RCT_resConfAdd_1[17] =
{
    {"App1_SettingA",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingB",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingC",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_Setting/ABC",            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingD",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_SettingE",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    /* no longer available in the new RCT */
    {"App1_SettingF",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t,          S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_Setting/DEF",            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"doc1.txt",                    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"Docs/doc2.txt",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"docA.txt",                    PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    /* no longer available in the new RCT */
    {"Docs/docB.txt",               PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, false,  {PersistencePolicy_wc,  PersistenceStorage_local, File_t,          S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_NewSetting1",            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_NewSetting2",            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wt,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_NewSettingA",            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_NewSettingB/BAU",        PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}},
    {"App1_NewSettingC",            PERS_ORG_LOCAL_APP_WT_PATH_"App1/"PERS_ORG_RCT_NAME, true,   {PersistencePolicy_wc,  PersistenceStorage_local, Key_t,           S_IRWXU|S_IRWXG|S_IRWXO, 64, {"App1"}, {NIL},{NIL}}}
};

expected_key_data_localDB_s expectedKeyData_localDB_resConfAdd_1[41] = 
{
    {"App1_SettingA",                                                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DEFAULT_DB_NAME_,        true,   "Data>>/App1_SettingA::DEFAULT",              sizeof("Data>>/App1_SettingA::DEFAULT")},
    {"App1_SettingB",                                                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     true,   "Data>>/App1_SettingB::DEFAULT",              sizeof("Data>>/App1_SettingB::DEFAULT")},
    {"App1_SettingB",                                                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DEFAULT_DB_NAME_,        false,  "Data>>/App1_SettingB::DEFAULT",              sizeof("Data>>/App1_SettingB::DEFAULT")},
    {"App1_SettingF",                                                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     false,  NIL,                                          0},


    {"App1_NewSetting1",                                                                    PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DEFAULT_DB_NAME_,        true,   "Data>>/App1_NewSetting1::DEFAULT",           sizeof("Data>>/App1_NewSetting1::DEFAULT")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_NewSetting1",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_NewSetting1::user1",             sizeof("Data>>/App1_NewSetting1::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_NewSetting1",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_NewSetting1::user2",             sizeof("Data>>/App1_NewSetting1::user2")},


    {"App1_NewSetting2",                                                                    PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DEFAULT_DB_NAME_,        true,   "Data>>/App1_NewSetting2::DEFAULT",           sizeof("Data>>/App1_NewSetting2::DEFAULT")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_NewSetting2",          PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_NewSetting2::user2::seat1",      sizeof("Data>>/App1_NewSetting2::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_NewSetting2",          PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_NewSetting2::user2::seat2",      sizeof("Data>>/App1_NewSetting2::user2::seat2")},


    {"App1_NewSettingA",                                                                    PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     true,   "Data>>/App1_NewSettingA::DEFAULT",           sizeof("Data>>/App1_NewSettingA::DEFAULT")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_NewSettingA",                                        PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_NewSettingA::node",              sizeof("Data>>/App1_NewSettingA::node")},


    {"App1_NewSettingB/BAU",                                                                PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     true,   "Data>>/App1_NewSettingB/BAU::DEFAULT",       sizeof("Data>>/App1_NewSettingB/BAU::DEFAULT")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_NewSettingB/BAU::user2::seat3",  sizeof("Data>>/App1_NewSettingB/BAU::user2::seat3")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DEFAULT_DB_NAME_,        false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"1"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"1"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_NewSettingB/BAU::user1::seat2",  sizeof("Data>>/App1_NewSettingB/BAU::user1::seat2")},
    { PERS_ORG_USER_FOLDER_NAME_"1"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DEFAULT_DB_NAME_,        false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"1"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_NewSettingB/BAU",      PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                false,  NIL,                                          0},

    

    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingA",                                           PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_SettingA",                       sizeof("Data>>/App1_SettingA")},
    /* policy changed for App1_SettingB */
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",             PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingB",             PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_SettingB::user2::seat1",         sizeof("Data>>/App1_SettingB::user2::seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",             PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingB",             PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_SettingB::user2:seat2",          sizeof("Data>>/App1_SettingB::user2:seat2")},

    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingC",                                           PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_SettingC",                       sizeof("Data>>/App1_SettingC")},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/ABC",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_Setting/ABC::user1",             sizeof("Data>>/App1_Setting/ABC::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/ABC",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_Setting/ABC::user2",             sizeof("Data>>/App1_Setting/ABC::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/ABC",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_Setting/ABC::user3",             sizeof("Data>>/App1_Setting/ABC::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/ABC",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_WT_DB_NAME_,                true,   "Data>>/App1_Setting/ABC::user4",             sizeof("Data>>/App1_Setting/ABC::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingD",                                           PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_SettingD",                       sizeof("Data>>/App1_SettingD")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/App1_SettingE",             PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_SettingE::user2:seat1",          sizeof("Data>>/App1_SettingE::user2:seat1")},
    { PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/App1_SettingE",             PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_SettingE::user2:seat2",          sizeof("Data>>/App1_SettingE::user2:seat2")},
    /* no longer available in the new RCT */
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_SettingF",                                           PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             false,  NIL,                                          0},
    { PERS_ORG_USER_FOLDER_NAME_"1/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_Setting/DEF::user1",             sizeof("Data>>/App1_Setting/DEF::user1")},
    { PERS_ORG_USER_FOLDER_NAME_"2/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_Setting/DEF::user2",             sizeof("Data>>/App1_Setting/DEF::user2")},
    { PERS_ORG_USER_FOLDER_NAME_"3/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_Setting/DEF::user3",             sizeof("Data>>/App1_Setting/DEF::user3")},
    { PERS_ORG_USER_FOLDER_NAME_"4/App1_Setting/DEF",                                       PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             true,   "Data>>/App1_Setting/DEF::user4",             sizeof("Data>>/App1_Setting/DEF::user4")},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_NewSettingC",                                        PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DB_NAME_,             false,  NIL,                                          0},
    { PERS_ORG_NODE_FOLDER_NAME_"/App1_NewSettingC",                                        PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_LOCAL_CACHE_DEFAULT_DB_NAME_,     false,  NIL,                                          0}
};


expected_file_data_s expectedKeyData_files_resConfAdd_1[16] =
{
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_CONFIG_DEFAULT_DATA_FOLDER_NAME_"/doc1.txt",                        true, "File>>App1>>NewData>>/doc1.txt::CONFIGURABLE-DEFAULT",   sizeof("File>>App1>>NewData>>/doc1.txt::CONFIGURABLE-DEFAULT")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/doc1.txt",                               true, "File>>App1>>NewData>>/doc1.txt::DEFAULT",                sizeof("File>>App1>>NewData>>/doc1.txt::DEFAULT")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/doc2.txt",                          true, "File>>App1>>NewData>>/Docs/doc2.txt::DEFAULT",           sizeof("File>>App1>>NewData>>/Docs/doc2.txt::DEFAULT")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/docA.txt",                               true, "File>>App1>>NewData>>/docA.txt::DEFAULT",                sizeof("File>>App1>>NewData>>/docA.txt::DEFAULT")},
    /* no longer available in the new RCT */
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/docB.txt",                          false,NIL,                                                      0},


    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/doc1.txt",                                      true, "File>>App1>>NewData>>/doc1.txt",                         sizeof("File>>App1>>NewData>>/doc1.txt")},
    { PERS_ORG_LOCAL_APP_WT_PATH_"App1" PERS_ORG_NODE_FOLDER_NAME_"/Docs/doc2.txt",                                 true, "File>>App1>>NewData>>/Docs/doc2.txt",                    sizeof("File>>App1>>NewData>>/Docs/doc2.txt")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"1/docA.txt",                                   true, "File>>App1>>NewData>>/docA.txt::user1",                  sizeof("File>>App1>>NewData>>/docA.txt::user1")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2/docA.txt",                                   true, "File>>App1>>/docA.txt::user2",                           sizeof("File>>App1>>/docA.txt::user2")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"3/docA.txt",                                   true, "File>>App1>>NewData>>/docA.txt::user3",                  sizeof("File>>App1>>NewData>>/docA.txt::user3")},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"4/docA.txt",                                   true, "File>>App1>>/docA.txt::user4",                           sizeof("File>>App1>>/docA.txt::user4")},

    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_DEFAULT_DATA_FOLDER_NAME_"/Docs/docB.txt",                       false,NIL, 0},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"1/Docs/docB.txt", false,NIL, 0},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"2/Docs/docB.txt", false,NIL, 0},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"3/Docs/docB.txt", false,NIL, 0},
    { PERS_ORG_LOCAL_APP_CACHE_PATH_"App1"PERS_ORG_USER_FOLDER_NAME_"2"PERS_ORG_SEAT_FOLDER_NAME_"4/Docs/docB.txt", false,NIL, 0}
} ;

bool_t Test_ResourceConfigAdd_1(int ceva, void* pAltceva)
{
    bool_t bEverythingOK = true ;

    if(bEverythingOK)
    {
        long result = persAdminResourceConfigAdd(RESOURCE_ARCHIVE_PATHNAME_1) ;
        sprintf(g_msg, "Test_ResourceConfigAdd_1: persAdminResourceConfigAdd(%s) returned %ld", RESOURCE_ARCHIVE_PATHNAME_1, result) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    return bEverythingOK ;
}

bool_t Test_ResourceConfigAdd_2(int ceva, void* pAltceva)
{
    bool_t bEverythingOK = true ;

    if(bEverythingOK)
    {
        long result = persAdminResourceConfigAdd(RESOURCE_ARCHIVE_PATHNAME_2) ;
        sprintf(g_msg, "Test_ResourceConfigAdd_1: persAdminResourceConfigAdd(%s) returned %ld", RESOURCE_ARCHIVE_PATHNAME_2, result) ;
        DLT_LOG(persAdminSvcDLTCtx, DLT_LOG_INFO, DLT_STRING(LT_HDR), DLT_STRING(g_msg));
    }

    return bEverythingOK ;
}

