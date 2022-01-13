/*
 * @licence app begin@
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * This file is part of GENIVI Project DLT - Diagnostic Log and Trace.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*!
 * \author Onkar Palkar onkar.palkar@wipro.com
 *
 * \copyright Copyright © 2016 Advanced Driver Information Technology.
 *
 * License MPL-2.0: Mozilla Public License version 2.0 http://mozilla.org/MPL/2.0/.
 *
 * \file gtest_dlt_daemon_filter.cpp
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: gtest_dlt_daemon_filter.cpp                                   **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Onkar Palkar onkar.palkar@wipro.com                           **
**  PURPOSE   : Unit test filter                                              **
**                                                                            **
**  REMARKS   :                                                               **
**                                                                            **
**  PLATFORM DEPENDANT [yes/no]: yes                                          **
**                                                                            **
**  TO BE CHANGED BY USER [yes/no]: no                                        **
**                                                                            **
*******************************************************************************/

/*******************************************************************************
**                      Author Identity                                       **
********************************************************************************
**                                                                            **
** Initials     Name                       Company                            **
** --------     -------------------------  ---------------------------------- **
**  op          Onkar Palkar               Wipro                              **
*******************************************************************************/

#include <gtest/gtest.h>
extern "C"
{

    #include "dlt_daemon_filter_types.h"
    #include "dlt_daemon_filter.h"
    #include "gtest_common.h"
    #include "string.h"
    #include <search.h>
}

/* Begin Method: dlt_daemon_filter::t_enable_all*/
TEST(t_enable_all, normal)
{
    DltServiceIdFlag flags;
    memset(&flags, 0, sizeof(DltServiceIdFlag));

    EXPECT_EQ(0, flags.upper[0]);
    EXPECT_EQ(DLT_RETURN_OK, enable_all(&flags));
    EXPECT_EQ(255, flags.upper[0]);
}

TEST(t_enable_all, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, enable_all(NULL));
}

/* Begin Method: dlt_daemon_filter::t_init_flags*/
TEST(t_init_flags, normal)
{
    DltServiceIdFlag flags;
    memset(&flags, 0, sizeof(DltServiceIdFlag));

    EXPECT_EQ(0, flags.upper[0]);
    EXPECT_EQ(DLT_RETURN_OK, enable_all(&flags));
    EXPECT_EQ(255, flags.upper[0]);
    EXPECT_EQ(DLT_RETURN_OK, init_flags(&flags));
    EXPECT_EQ(0, flags.upper[0]);
}

TEST(t_init_flags, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, init_flags(NULL));
}

/* Begin Method: dlt_daemon_filter::t_set_bit*/
TEST(t_set_bit, normal)
{
    DltServiceIdFlag flags;
    memset(&flags, 0, sizeof(DltServiceIdFlag));

    EXPECT_EQ(DLT_RETURN_OK, set_bit(&flags, DLT_SERVICE_ID_SET_LOG_LEVEL));
    EXPECT_EQ(2, flags.lower[0]);
    EXPECT_EQ(DLT_RETURN_OK, set_bit(&flags, DLT_SERVICE_ID_SET_TRACE_STATUS));
    EXPECT_EQ(6, flags.lower[0]);
    EXPECT_EQ(DLT_RETURN_OK, set_bit(&flags, DLT_SERVICE_ID_GET_LOG_INFO));
    EXPECT_EQ(14, flags.lower[0]);
    EXPECT_EQ(DLT_RETURN_OK, set_bit(&flags, DLT_SERVICE_ID_GET_SOFTWARE_VERSION));
    EXPECT_EQ(8, flags.lower[2]);
}

TEST(t_set_bit, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, set_bit(NULL, 0));
}

/* Begin Method: dlt_daemon_filter::t_bit*/
TEST(t_bit, normal)
{
    DltServiceIdFlag flags;
    memset(&flags, 0, sizeof(DltServiceIdFlag));

    EXPECT_EQ(0, bit(&flags, DLT_SERVICE_ID_SET_LOG_LEVEL));
    EXPECT_EQ(DLT_RETURN_OK, set_bit(&flags, DLT_SERVICE_ID_SET_LOG_LEVEL));
    EXPECT_EQ(1, bit(&flags, DLT_SERVICE_ID_SET_LOG_LEVEL));
}

TEST(t_bit, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, bit(NULL, 0));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_name*/
TEST(t_dlt_daemon_filter_name, normal)
{
    char value[] = "Internal";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_name(&mf, &config, value));

    free(config.name);
    config.name = NULL;
}

TEST(t_dlt_daemon_filter_name, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_name(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_name(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_name(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_name(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_level*/
TEST(t_dlt_daemon_filter_level, normal)
{
    char value[] = "50";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(0, config.level_max);
    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_level(&mf, &config, value));
    EXPECT_EQ(50, config.level_max);
}

TEST(t_dlt_daemon_filter_level, abnormal_1)
{
    /*Default Level is invalid*/
    char value[] = "110";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(0, config.level_max);
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_level(&mf, &config, value));
}

TEST(t_dlt_daemon_filter_level, abnormal_2)
{
    /*Default Level is not a number*/
    char value[] = "not_a_number";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(0, config.level_max);
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_level(&mf, &config, value));
}

TEST(t_dlt_daemon_filter_level, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_level(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_level(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_level(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_level(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_control_mask*/
TEST(t_dlt_daemon_filter_control_mask, normal_1)
{
    /* check wildcard */
    char value[] = "*";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_control_mask(&mf, &config, value));
    EXPECT_EQ(255, config.ctrl_mask.upper[0]);
}

TEST(t_dlt_daemon_filter_control_mask, normal_2)
{
    /* check for no client specifier */
    char value[] = DLT_FILTER_CLIENT_NONE;
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_control_mask(&mf, &config, value));
}

TEST(t_dlt_daemon_filter_control_mask, normal_3)
{
    /* check for no client specifier */
    char value[] = "0x03, 0x13, 0xf09";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_control_mask(&mf, &config, value));
    EXPECT_EQ(8, config.ctrl_mask.lower[0]);
    EXPECT_EQ(8, config.ctrl_mask.lower[2]);
    EXPECT_EQ(2, config.ctrl_mask.upper[1]);
}

TEST(t_dlt_daemon_filter_control_mask, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_control_mask(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_control_mask(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_control_mask(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_control_mask(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_client_mask*/
TEST(t_dlt_daemon_filter_client_mask, normal_1)
{
    /* check wildcard */
    char value[] = "*";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_client_mask(&mf, &config, value));
    EXPECT_EQ(DLT_CON_MASK_ALL, config.client_mask);
}

TEST(t_dlt_daemon_filter_client_mask, normal_2)
{
    /* check for no client specifier */
    char value[] = DLT_FILTER_CLIENT_NONE;
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_client_mask(&mf, &config, value));
}

TEST(t_dlt_daemon_filter_client_mask, normal_3)
{
    /* list of allowed clients given */
    char value[] = "Serial,TCP";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_client_mask(&mf, &config, value));
    EXPECT_EQ(2046, config.client_mask);
}

TEST(t_dlt_daemon_filter_client_mask, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_client_mask(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_client_mask(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_client_mask(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_client_mask(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_injections*/
TEST(t_dlt_daemon_filter_injections, normal_1)
{
    /*value is a komma separated list of injections or '*' or NONE */
    char value[] = "*";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_injections(&mf, &config, value));
    EXPECT_EQ(-1, config.num_injections);
}

TEST(t_dlt_daemon_filter_injections, normal_2)
{
    /*value is a komma separated list of injections or '*' or NONE */
    char value[] = DLT_FILTER_CLIENT_NONE;
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_injections(&mf, &config, value));
    EXPECT_EQ(0, config.num_injections);
}

TEST(t_dlt_daemon_filter_injections, normal_3)
{
    /*at least one specific injection is given */
    int i;
    char value[] = "aaa,bbb,ccc";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_injections(&mf, &config, value));
    EXPECT_EQ(3, config.num_injections);
    ASSERT_STREQ("aaa", config.injections[0]);
    ASSERT_STREQ("bbb", config.injections[1]);
    ASSERT_STREQ("ccc", config.injections[2]);
    for(i = 0; i < config.num_injections ; i++)
    {
        free(config.injections[i]);
        config.injections[i] = NULL;
    }
}

TEST(t_dlt_daemon_filter_injections, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltFilterConfiguration config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltFilterConfiguration));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_injections(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_injections(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_injections(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_filter_injections(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_setup_filter_section*/
TEST(t_dlt_daemon_setup_filter_section, normal_1)
{
    int i;
    char sec_name[] = "Filter";
    char keys_0[] = "Name";
    char keys_1[] = "Level";
    char keys_2[] = "Clients";
    char keys_3[] = "ControlMessages";
    char keys_4[] = "Injections";
    char *keys[5] = {keys_0, keys_1, keys_2, keys_3, keys_4};
    char data_0[] = "off";
    char data_1[] = "1";
    char data_2[] = "None";
    char data_3[] = "None";
    char data_4[] = "None";
    char *data[5] = {data_0, data_1, data_2, data_3, data_4};
    DltMessageFilter mf;
    DltConfigFile config;
    DltConfigKeyData **tmp = NULL;
    DltConfigKeyData *n = NULL;
    DltConfigKeyData *n1 = NULL;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltConfigFile));
    config.sections = (DltConfigFileSection*)calloc(1, sizeof(DltConfigFileSection));

    if (config.sections != NULL)
    {
        config.num_sections = 1;
        config.sections->name = sec_name;

        for(i = 0; i < 5; i++)
        {
            if (config.sections->list == NULL)
            {
                config.sections->list = (DltConfigKeyData*) malloc(sizeof(DltConfigKeyData));
                if (config.sections->list == NULL)
                {
                    continue;
                }
                tmp = &config.sections->list;
            }
            else
            {
                tmp = &config.sections->list;

                while ((*tmp) != NULL)
                {
                    tmp = &(*tmp)->next;
                }

                *tmp = (DltConfigKeyData*) malloc(sizeof(DltConfigKeyData));

                if (*tmp == NULL)
                {
                    continue;
                }
            }
            (*tmp)->key = strdup(keys[i]);
            (*tmp)->data = strdup(data[i]);
            (*tmp)->next = NULL;
        }
        if (config.sections->list != NULL)
        {
            EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_setup_filter_section(&mf, &config, sec_name));

            n = config.sections->list;
            while (n != NULL)
            {
                n1 = n;
                n = n->next;
                free(n1->key);
                free(n1->data);
                free(n1);
            }
        }
        free(config.sections);
    }
}

TEST(t_dlt_daemon_setup_filter_section, normal_2)
{
    int i;
    char sec_name[] = "Filter";
    char keys_0[] = "Name";
    char keys_1[] = "Level";
    char keys_2[] = "Clients";
    char keys_3[] = "ControlMessages";
    char keys_4[] = "Injections";
    char *keys[5] = {keys_0, keys_1, keys_2, keys_3, keys_4};
    char data_0[] = "off";
    char data_1[] = "1";
    char data_2[] = "None";
    char data_3[] = "None";
    char data_4[] = "None";
    char *data[5] = {data_0, data_1, data_2, data_3, data_4};
    char name[] = "internal";
    DltMessageFilter mf;
    DltConfigFile config;
    DltConfigKeyData **tmp = NULL;
    DltConfigKeyData *n = NULL;
    DltConfigKeyData *n1 = NULL;
    DltFilterConfiguration *conf;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltConfigFile));
    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    conf->name = strdup(name);
    conf->level_min = 0;
    conf->level_max = 19;
    init_flags(&conf->ctrl_mask);
    conf->client_mask = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;
    conf->num_injections = 0;
    mf.configs = conf;
    mf.head = conf;

    config.sections = (DltConfigFileSection*)calloc(1, sizeof(DltConfigFileSection));

    if (config.sections != NULL)
    {
        config.num_sections = 1;
        config.sections->name = sec_name;

        for(i = 0; i < 5; i++)
        {
            if (config.sections->list == NULL)
            {
                config.sections->list = (DltConfigKeyData*) malloc(sizeof(DltConfigKeyData));
                if (config.sections->list == NULL)
                {
                    continue;
                }
                tmp = &config.sections->list;
            }
            else
            {
                tmp = &config.sections->list;

                while ((*tmp) != NULL)
                {
                    tmp = &(*tmp)->next;
                }

                *tmp = (DltConfigKeyData*) malloc(sizeof(DltConfigKeyData));

                if (*tmp == NULL)
                {
                    continue;
                }
            }
            (*tmp)->key = strdup(keys[i]);
            (*tmp)->data = strdup(data[i]);
            (*tmp)->next = NULL;
        }
        if (config.sections->list != NULL)
        {
            EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_setup_filter_section(&mf, &config, sec_name));

            n = config.sections->list;
            while (n != NULL)
            {
                n1 = n;
                n = n->next;
                free(n1->key);
                free(n1->data);
                free(n1);
            }
        }
        free(config.sections);
    }
    free(conf->name);
    free(conf);
}

TEST(t_dlt_daemon_setup_filter_section, nullpointer)
{
    // NULL-Pointer, expect -1
    char sec_name[DLT_CONFIG_FILE_ENTRY_MAX_LEN + 1] = {'\0'};
    DltMessageFilter mf;
    DltConfigFile config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltConfigFile));

    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_setup_filter_section(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_setup_filter_section(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_setup_filter_section(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_setup_filter_section(NULL, NULL, sec_name));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_set_injection_service_ids*/
TEST(t_dlt_daemon_set_injection_service_ids, normal_1)
{
    /* value is a komma separated list of injections or '*' or NONE */
    char value[] = "*";
    DltInjectionConfig config;
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_set_injection_service_ids(&config.service_ids,
                                             &config.num_sevice_ids,
                                             value));
    EXPECT_EQ(-1, config.num_sevice_ids);
}

TEST(t_dlt_daemon_set_injection_service_ids, normal_2)
{
    /* value is a komma separated list of injections or '*' or NONE */
    char value[] = DLT_FILTER_CLIENT_NONE;
    DltInjectionConfig config;
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_set_injection_service_ids(&config.service_ids,
                                             &config.num_sevice_ids,
                                             value));
    EXPECT_EQ(0, config.num_sevice_ids);
}

TEST(t_dlt_daemon_set_injection_service_ids, normal_3)
{
    /* at least one specific service id is given */
    char value[] = "10,11";
    DltInjectionConfig config;
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_set_injection_service_ids(&config.service_ids,
                                             &config.num_sevice_ids,
                                             value));
    EXPECT_EQ(10, config.service_ids[0]);
    EXPECT_EQ(11, config.service_ids[1]);

    free(config.service_ids);
    config.service_ids = NULL;
}

TEST(t_dlt_daemon_set_injection_service_ids, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "*";
    int service_ids = 100;
    DltInjectionConfig config;
    memset(&config, 0, sizeof(DltInjectionConfig));
    config.service_ids = &service_ids;
    config.num_sevice_ids = 1;

    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_set_injection_service_ids(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_set_injection_service_ids(&config.service_ids, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_set_injection_service_ids(NULL, &config.num_sevice_ids, NULL));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_set_injection_service_ids(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_find_injection_by_name*/
TEST(t_dlt_daemon_filter_find_injection_by_name, normal)
{
    char name[] = "injection";
    char apid[] = "100";
    char ctid[] = "200";
    DltInjectionConfig *ret;
    DltInjectionConfig injections;
    memset(&injections, 0, sizeof(DltInjectionConfig));
    injections.name = name;
    injections.apid = apid;
    injections.ctid = ctid;

    ret = dlt_daemon_filter_find_injection_by_name(&injections, name);
    ASSERT_STREQ(apid, ret->apid);
    ASSERT_STREQ(ctid, ret->ctid);
}

TEST(t_dlt_daemon_filter_find_injection_by_name, nullpointer)
{
    // NULL-Pointer, expect -1
    char name[] = "injection";
    DltInjectionConfig injections;
    memset(&injections, 0, sizeof(DltInjectionConfig));
    DltInjectionConfig *ret;

    ret = dlt_daemon_filter_find_injection_by_name(NULL, NULL);
    EXPECT_EQ(NULL, ret);
    ret = dlt_daemon_filter_find_injection_by_name(NULL, name);
    EXPECT_EQ(NULL, ret);
    ret  = dlt_daemon_filter_find_injection_by_name(&injections, NULL);
    EXPECT_EQ(NULL, ret);
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_find_injection*/
TEST(t_dlt_daemon_filter_find_injection, normal_1)
{
    int num_injections = 2;
    char i1[] = "i1";
    char i2[] = "i2";
    char *injections[] = {i1, i2};

    EXPECT_EQ(0, dlt_daemon_filter_find_injection(num_injections, injections, i1));
}

TEST(t_dlt_daemon_filter_find_injection, normal_2)
{
    int num_injections = 2;
    char i1[] = "i1";
    char i2[] = "i2";
    char i3[] = "i3";
    char *injections[] = {i1, i2};

    EXPECT_EQ(-1, dlt_daemon_filter_find_injection(num_injections, injections, i3));
}

TEST(t_dlt_daemon_filter_find_injection, nullpointer)
{
    char i1[] = "i1";
    char i2[] = "i2";
    char *injections[] = {i1, i2};

    EXPECT_EQ(-1, dlt_daemon_filter_find_injection(0, NULL, NULL));
    EXPECT_EQ(-1, dlt_daemon_filter_find_injection(0, NULL, i1));
    EXPECT_EQ(-1, dlt_daemon_filter_find_injection(0, injections, NULL));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_injection_name*/
TEST(t_dlt_daemon_injection_name, normal)
{
    char value[] = "Internal";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_injection_name(&mf, &config, value));
    ASSERT_STREQ(config.name, value);

    free(config.name);
}

TEST(t_dlt_daemon_injection_name, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_name(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_name(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_name(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_name(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_injection_apid*/
TEST(t_dlt_daemon_injection_apid, normal)
{
    char value[] = "100";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_injection_apid(&mf, &config, value));
    ASSERT_STREQ(config.apid, value);

    free(config.apid);
}

TEST(t_dlt_daemon_injection_apid, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_apid(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_apid(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_apid(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_apid(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_injection_ctid*/
TEST(t_dlt_daemon_injection_ctid, normal)
{
    char value[] = "100";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_injection_ctid(&mf, &config, value));
    ASSERT_STREQ(config.ctid, value);

    free(config.ctid);
}

TEST(t_dlt_daemon_injection_ctid, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ctid(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ctid(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ctid(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ctid(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_injection_ecu_id*/
TEST(t_dlt_daemon_injection_ecu_id, normal)
{
    char value[] = "100";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_injection_ecu_id(&mf, &config, value));
    ASSERT_STREQ(config.ecuid, value);

    free(config.ecuid);
}

TEST(t_dlt_daemon_injection_ecu_id, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "Internal";
    DltMessageFilter mf;
    DltInjectionConfig config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ecu_id(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ecu_id(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ecu_id(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_ecu_id(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_injection_service_id*/
TEST(t_dlt_daemon_injection_service_id, normal)
{
    char value[] = "*";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));
    DltInjectionConfig config;
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_injection_service_id(&mf,
                                             &config,
                                             value));
    EXPECT_EQ(-1, config.num_sevice_ids);
}

TEST(t_dlt_daemon_injection_service_id, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "*";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));
    DltInjectionConfig config;
    memset(&config, 0, sizeof(DltInjectionConfig));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_service_id(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_service_id(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_service_id(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_injection_service_id(NULL, NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_get_name*/
TEST(t_dlt_daemon_get_name, normal)
{
    char value[] = "ALD-backend-filter";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_get_name(&mf, value));
    ASSERT_STREQ(mf.name, value);

    free(mf.name);
}

TEST(t_dlt_daemon_get_name, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "ALD-backend-filter";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_name(NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_name(&mf, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_name(NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_get_default_level*/
TEST(t_dlt_daemon_get_default_level, normal)
{
    /* value is a komma separated list of injections or '*' or NONE */
    char value[] = "40";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_get_default_level(&mf, value));
    EXPECT_EQ(40, mf.default_level);
}

TEST(t_dlt_daemon_get_default_level, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "40";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_default_level(NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_default_level(&mf, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_default_level(NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_get_backend*/
TEST(t_dlt_daemon_get_backend, normal)
{
    char value[] = "ALD";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_get_backend(&mf, value));
    ASSERT_STREQ(mf.backend, value);

    free(mf.backend);
}

TEST(t_dlt_daemon_get_backend, nullpointer)
{
    // NULL-Pointer, expect -1
    char value[] = "ALD";
    DltMessageFilter mf;
    memset(&mf, 0, sizeof(DltMessageFilter));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_name(NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_name(&mf, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_get_name(NULL, value));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_setup_filter_properties*/
TEST(t_dlt_daemon_setup_filter_properties, normal)
{
    int i;
    char sec_name[] = "General";
    char keys_0[] = "Name";
    char keys_1[] = "DefaultLevel";
    char keys_2[] = "Backend";
    char *keys[3] = {keys_0, keys_1, keys_2};
    char data_0[] = "ALD-backend-filter";
    char data_1[] = "40";
    char data_2[] = "ALD";
    char *data[3] = {data_0, data_1, data_2};
    DltMessageFilter mf;
    DltConfigFile config;
    DltConfigKeyData **tmp = NULL;
    DltConfigKeyData *n = NULL;
    DltConfigKeyData *n1 = NULL;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltConfigFile));
    config.sections = (DltConfigFileSection*)calloc(1, sizeof(DltConfigFileSection));

    if (config.sections != NULL)
    {
        config.num_sections = 1;
        config.sections->name = sec_name;

        for(i = 0; i < 3; i++)
        {
            if (config.sections->list == NULL)
            {
                config.sections->list = (DltConfigKeyData*) malloc(sizeof(DltConfigKeyData));
                if (config.sections->list == NULL)
                {
                    continue;
                }
                tmp = &config.sections->list;
            }
            else
            {
                tmp = &config.sections->list;

                while ((*tmp) != NULL)
                {
                    tmp = &(*tmp)->next;
                }

                *tmp = (DltConfigKeyData*) malloc(sizeof(DltConfigKeyData));

                if (*tmp == NULL)
                {
                    continue;
                }
            }
            (*tmp)->key = strdup(keys[i]);
            (*tmp)->data = strdup(data[i]);
            (*tmp)->next = NULL;
        }
        if (config.sections->list != NULL)
        {
            EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_setup_filter_properties(&mf, &config, sec_name));

            n = config.sections->list;
            while (n != NULL)
            {
                n1 = n;
                n = n->next;
                free(n1->key);
                free(n1->data);
                free(n1);
            }
        }
        free(config.sections);
    }
}

TEST(t_dlt_daemon_setup_filter_properties, nullpointer)
{
    // NULL-Pointer, expect -1
    char sec_name[DLT_CONFIG_FILE_ENTRY_MAX_LEN + 1] = {'\0'};
    DltMessageFilter mf;
    DltConfigFile config;
    memset(&mf, 0, sizeof(DltMessageFilter));
    memset(&config, 0, sizeof(DltConfigFile));

    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_setup_filter_properties(NULL, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_setup_filter_properties(&mf, NULL, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_setup_filter_properties(NULL, &config, NULL));
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_setup_filter_properties(NULL, NULL, sec_name));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_add_closed_filter*/
TEST(t_dlt_daemon_add_closed_filter, normal)
{
    char name[]= "Internal";
    unsigned int min = 0;
    DltFilterConfiguration *config;

    config = dlt_daemon_add_closed_filter(name, min);
    if (config != NULL)
    {
        ASSERT_STREQ(name, config->name);
        EXPECT_EQ(min, config->level_min);
        EXPECT_EQ(DLT_FILTER_LEVEL_MAX, config->level_max);
        EXPECT_EQ(DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK, config->client_mask);
        EXPECT_EQ(0, config->num_injections);

        free(config->name);
        free(config);
    }
}

TEST(t_dlt_daemon_add_closed_filter, nullpointer)
{
    EXPECT_EQ(NULL, dlt_daemon_add_closed_filter(NULL, 0));
}

/* Begin Method: dlt_daemon_filter::t_dlt_filter_control_compare_flags*/
TEST(t_dlt_filter_control_compare_flags, normal_1)
{
    DltServiceIdFlag flag1;
    DltServiceIdFlag flag2;

    init_flags(&flag1);
    init_flags(&flag2);

    EXPECT_EQ(DLT_RETURN_OK, dlt_filter_control_compare_flags(&flag1, &flag2));
}

TEST(t_dlt_filter_control_compare_flags, normal_2)
{
    DltServiceIdFlag flag1;
    DltServiceIdFlag flag2;

    init_flags(&flag1);
    init_flags(&flag2);
    enable_all(&flag1);
    enable_all(&flag2);

    EXPECT_EQ(DLT_RETURN_OK, dlt_filter_control_compare_flags(&flag1, &flag2));
}

TEST(t_dlt_filter_control_compare_flags, normal_3)
{
    int id1 = DLT_SERVICE_ID_SET_LOG_LEVEL;
    DltServiceIdFlag flag1;
    DltServiceIdFlag flag2;

    init_flags(&flag1);
    init_flags(&flag2);
    set_bit(&flag1, id1);
    set_bit(&flag2, id1);

    EXPECT_EQ(DLT_RETURN_OK, dlt_filter_control_compare_flags(&flag1, &flag2));
}

TEST(t_dlt_filter_control_compare_flags, abnormal_1)
{
    DltServiceIdFlag flag1;
    DltServiceIdFlag flag2;

    init_flags(&flag1);
    init_flags(&flag2);
    enable_all(&flag2);

    EXPECT_EQ(DLT_RETURN_ERROR, dlt_filter_control_compare_flags(&flag1, &flag2));
}

TEST(t_dlt_filter_control_compare_flags, abnormal_2)
{
    int id1 = DLT_SERVICE_ID_SET_LOG_LEVEL;
    int id2 = DLT_SERVICE_ID_GET_LOG_INFO;
    DltServiceIdFlag flag1;
    DltServiceIdFlag flag2;

    init_flags(&flag1);
    init_flags(&flag2);
    set_bit(&flag1, id1);
    set_bit(&flag2, id2);

    EXPECT_EQ(DLT_RETURN_ERROR, dlt_filter_control_compare_flags(&flag1, &flag2));
}

TEST(t_dlt_filter_control_compare_flags, nullpointer)
{
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_filter_control_compare_flags(NULL, NULL));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_check_each_client_mask*/
TEST(t_dlt_daemon_check_each_client_mask, normal_1)
{
    unsigned int check_client = DLT_CON_MASK_CLIENT_MSG_SERIAL;
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                          DLT_CON_MASK_ALL;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_each_client_mask(conf, next_conf, check_client));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_each_client_mask, normal_2)
{
    unsigned int check_client = DLT_CON_MASK_CLIENT_MSG_SERIAL;
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                          DLT_CON_MASK_CLIENT_MSG_SERIAL;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_each_client_mask(conf, next_conf, check_client));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_each_client_mask, normal_3)
{
    unsigned int check_client = DLT_CON_MASK_CLIENT_CONNECT;
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                          DLT_CON_MASK_ALL;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_TRUE, dlt_daemon_check_each_client_mask(conf, next_conf, check_client));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_each_client_mask, normal_4)
{
    unsigned int check_client = DLT_CON_MASK_CLIENT_CONNECT;
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_TRUE, dlt_daemon_check_each_client_mask(conf, next_conf, check_client));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_dlt_daemon_check_each_client_mask, nullpointer)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));

    if ((conf != NULL) && (next_conf != NULL))
    {
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_each_client_mask(NULL, NULL, 0));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_each_client_mask(conf, NULL, 0));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_each_client_mask(NULL, next_conf, 0));

        free(conf);
        free(next_conf);
    }
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_check_client_mask*/
TEST(t_dlt_daemon_check_client_mask, normal_1)
{
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_client_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_client_mask, normal_2)
{
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_ALL;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_client_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_client_mask, normal_3)
{
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                          DLT_CON_MASK_ALL;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_client_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_client_mask, normal_4)
{
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                          DLT_CON_MASK_CLIENT_MSG_SERIAL;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_client_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_client_mask, normal_5)
{
    unsigned int client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                          DLT_CON_MASK_CLIENT_MSG_SERIAL;
    unsigned int next_client = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK |
                               DLT_CON_MASK_CLIENT_CONNECT |
                               DLT_CON_MASK_CLIENT_MSG_SERIAL;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->name = strdup("1");
        conf->client_mask = client;
        next_conf->name = strdup("2");
        next_conf->client_mask = next_client;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_client_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_client_mask, nullpointer)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));

    if ((conf != NULL) && (next_conf != NULL))
    {
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_client_mask(NULL, NULL));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_client_mask(conf, NULL));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_client_mask(next_conf, NULL));

        free(conf);
        free(next_conf);
    }
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_check_ctrl_mask*/
TEST(t_dlt_daemon_check_ctrl_mask, normal_1)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        init_flags(&conf->ctrl_mask);
        init_flags(&next_conf->ctrl_mask);

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_ctrl_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_ctrl_mask, normal_2)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        init_flags(&conf->ctrl_mask);
        init_flags(&next_conf->ctrl_mask);
        enable_all(&conf->ctrl_mask);
        enable_all(&next_conf->ctrl_mask);

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_ctrl_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_ctrl_mask, normal_3)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        init_flags(&conf->ctrl_mask);
        init_flags(&next_conf->ctrl_mask);
        enable_all(&conf->ctrl_mask);

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_ctrl_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_ctrl_mask, normal_4)
{
    int id1 = DLT_SERVICE_ID_SET_LOG_LEVEL;
    int id2 = DLT_SERVICE_ID_GET_LOG_INFO;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        init_flags(&conf->ctrl_mask);
        init_flags(&next_conf->ctrl_mask);
        set_bit(&conf->ctrl_mask, id1);
        set_bit(&next_conf->ctrl_mask, id2);

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_ctrl_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_ctrl_mask, normal_5)
{
    int id1 = DLT_SERVICE_ID_SET_LOG_LEVEL;
    int id2 = DLT_SERVICE_ID_GET_LOG_INFO;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        init_flags(&conf->ctrl_mask);
        init_flags(&next_conf->ctrl_mask);
        set_bit(&conf->ctrl_mask, id1);
        set_bit(&next_conf->ctrl_mask, id1);
        set_bit(&next_conf->ctrl_mask, id2);

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_ctrl_mask(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_ctrl_mask, nullpointer)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));

    if ((conf != NULL) && (next_conf != NULL))
    {
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_ctrl_mask(NULL, NULL));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_ctrl_mask(conf, NULL));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_ctrl_mask(NULL, next_conf));

        free(conf);
        free(next_conf);
    }
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_check_injections*/
TEST(t_dlt_daemon_check_injections, normal_1)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->num_injections = 0;
        next_conf->num_injections = 0;

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_injections(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_injections, normal_2)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->num_injections = -1;
        next_conf->num_injections = -1;

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_injections(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_injections, normal_3)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->num_injections = -1;
        next_conf->num_injections = 0;

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_injections(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_injections, normal_4)
{
    int ret = 0;
    int num_injections1 = 2;
    int num_injections2 = 1;
    char i1[] = "i1";
    char i2[] = "i2";
    char *injections1[] = {i1, i2};
    char *injections2[] = {i1};
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));

    conf->name = strdup("Injection1");
    conf->level_min = DLT_FILTER_LEVEL_MIN;
    conf->level_max = DLT_FILTER_LEVEL_MAX;
    init_flags(&conf->ctrl_mask);
    conf->client_mask = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;

    next_conf->name = strdup("Injection2");
    next_conf->level_min = DLT_FILTER_LEVEL_MIN;
    next_conf->level_max = DLT_FILTER_LEVEL_MAX;
    init_flags(&conf->ctrl_mask);
    next_conf->client_mask = DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK;

    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->num_injections = num_injections1;
        conf->injections = injections1;
        next_conf->num_injections = num_injections2;
        next_conf->injections = injections2;

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        ret = dlt_daemon_check_injections(conf, next_conf);
        EXPECT_EQ(DLT_RETURN_OK, ret);

        free(conf->name);
        free(next_conf->name);
        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_injections, normal_5)
{
    int num_injections1 = 1;
    int num_injections2 = 2;
    char i1[] = "i1";
    char i2[] = "i2";
    char *injections1[] = {i1};
    char *injections2[] = {i1, i2};
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->num_injections = num_injections1;
        conf->injections = injections1;
        next_conf->num_injections = num_injections2;
        next_conf->injections = injections2;

        conf->name = strdup("1");
        next_conf->name = strdup("2");

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_injections(conf, next_conf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_injections, nullpointer)
{
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));

    if ((conf != NULL) && (next_conf != NULL))
    {
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_injections(NULL, NULL));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_injections(conf, NULL));
        EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_injections(NULL, next_conf));

        free(conf);
        free(next_conf);
    }
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_check_filter_level*/
TEST(t_dlt_daemon_check_filter_level, normal)
{
    int level_min1 = 0;
    int level_max1 = 19;
    int level_min2 = 20;
    int level_max2 = DLT_FILTER_LEVEL_MAX;
    DltMessageFilter mf;
    DltFilterConfiguration *conf;
    DltFilterConfiguration *next_conf;

    memset(&mf, 0, sizeof(DltMessageFilter));
    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    next_conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if ((conf != NULL) && (next_conf != NULL))
    {
        conf->level_min = level_min1;
        conf->level_max = level_max1;
        next_conf->level_min = level_min2;
        next_conf->level_max = level_max2;
        conf->next = next_conf;
        mf.configs = conf;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_check_filter_level(&mf));

        free(conf);
        free(next_conf);
    }
}

TEST(t_dlt_daemon_check_filter_level, nullpointer)
{
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_check_filter_level(NULL));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_set_default_level*/
TEST(t_dlt_daemon_set_default_level, normal)
{
    int level = 10;
    int level_min = 0;
    int level_max = 19;
    DltMessageFilter mf;
    DltFilterConfiguration *conf;

    memset(&mf, 0, sizeof(DltMessageFilter));
    conf = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if (conf != NULL)
    {
        conf->level_min = level_min;
        conf->level_max = level_max;
        mf.configs = conf;
        mf.default_level = level;

        EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_set_default_level(&mf));

        free(conf);
    }
}

TEST(t_dlt_daemon_set_default_level, nullpointer)
{
    EXPECT_EQ(DLT_RETURN_WRONG_PARAMETER, dlt_daemon_set_default_level(NULL));
}

TEST(t_dlt_daemon_prepare_message_filter, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_prepare_message_filter(NULL, 0));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_is_connection_allowed*/
TEST(t_dlt_daemon_filter_is_connection_allowed, normal)
{
    DltMessageFilter filter;
    memset(&filter, 0, sizeof(DltMessageFilter));

    filter.current = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if(filter.current != NULL)
    {
        filter.current->client_mask = 0X0055;
        EXPECT_EQ(4, dlt_daemon_filter_is_connection_allowed(&filter, DLT_CONNECTION_CLIENT_MSG_TCP));
        free(filter.current);
    }
}

TEST(t_dlt_daemon_filter_is_connection_allowed, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_connection_allowed(NULL, DLT_CONNECTION_CLIENT_MSG_SERIAL));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_is_control_allowed*/
TEST(t_dlt_daemon_filter_is_control_allowed, normal)
{
    DltMessageFilter filter;
    memset(&filter, 0, sizeof(DltMessageFilter));

    filter.current = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if(filter.current != NULL)
    {
        EXPECT_EQ(DLT_RETURN_OK, set_bit(&filter.current->ctrl_mask, DLT_SERVICE_ID_SET_LOG_LEVEL));
        EXPECT_EQ(1, dlt_daemon_filter_is_control_allowed(&filter, DLT_SERVICE_ID_SET_LOG_LEVEL));
        free(filter.current);
    }
}

TEST(t_dlt_daemon_filter_is_control_allowed, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_control_allowed(NULL, DLT_SERVICE_ID_SET_LOG_LEVEL));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_is_injection_allowed*/
TEST(t_dlt_daemon_filter_is_injection_allowed, normal)
{
    DltMessageFilter filter;
    memset(&filter, 0, sizeof(DltMessageFilter));
    char name[] = "injection";
    char apid[] = "1234";
    char ctid[] = "5678";
    char ecuid[] = "9012";
    int service_id = 10;
    char* ptr_name = name;
    filter.current = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if(filter.current != NULL)
    {
        /* no injection allowed */
        filter.current->num_injections = 0;
        EXPECT_EQ(0, dlt_daemon_filter_is_injection_allowed(&filter, apid, ctid, ecuid, service_id));
        /* all allowed */
        filter.current->num_injections = -1;
        EXPECT_EQ(1, dlt_daemon_filter_is_injection_allowed(&filter, apid, ctid, ecuid, service_id));
        /* check application identifier, context identifier, node identifier
         * and injection id (service id). Every entry must be valid to allow
         * that injection message */
        filter.current->num_injections = 1;
        filter.current->injections = &ptr_name;
        filter.injections[0].name = name;
        filter.injections[0].apid = apid;
        filter.injections[0].ctid = ctid;
        filter.injections[0].ecuid = ecuid;
        filter.injections[0].service_ids = &service_id;
        filter.injections[0].num_sevice_ids = 1;
        EXPECT_EQ(1, dlt_daemon_filter_is_injection_allowed(&filter, apid, ctid, ecuid, service_id));

        free(filter.current);
    }
}

TEST(t_dlt_daemon_filter_is_injection_allowed, nullpointer)
{
    // NULL-Pointer, expect -1
    DltMessageFilter filter;
    memset(&filter, 0, sizeof(DltMessageFilter));
    char apid[] = "1234";
    char ctid[] = "5678";
    char ecuid[] = "9012";
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_injection_allowed(NULL, NULL, NULL, NULL, 0));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_injection_allowed(&filter, NULL, NULL, NULL, 0));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_injection_allowed(NULL, apid, NULL, NULL, 0));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_injection_allowed(NULL, NULL, ctid, NULL, 0));
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_is_injection_allowed(NULL, NULL, NULL, ecuid, 0));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_change_filter_level*/
TEST(t_dlt_daemon_filter_change_filter_level, normal)
{
    int level = 0;
    char name[] = "injection";
    DltDaemonLocal daemon_local;
    DltFilterConfiguration configs;
    memset(&daemon_local, 0, sizeof(DltDaemonLocal));
    memset(&configs, 0, sizeof(DltFilterConfiguration));
    configs.name = name;
    configs.level_min = 0;
    configs.level_max = 0;
    daemon_local.pFilter.configs = &configs;
    daemon_local.pFilter.current = (DltFilterConfiguration*)calloc(1, sizeof(DltFilterConfiguration));
    if(daemon_local.pFilter.current != NULL)
    {
        daemon_local.pEvent.connections = (DltConnection*)calloc(1, sizeof(DltConnection));
        if(daemon_local.pEvent.connections != NULL)
        {
            daemon_local.pEvent.connections->receiver = (DltReceiver*)calloc(1, sizeof(DltReceiver));
            if(daemon_local.pEvent.connections->receiver != NULL)
            {
                daemon_local.pEvent.connections->receiver->fd = 1;
                daemon_local.pEvent.connections->type = DLT_CONNECTION_CLIENT_CONNECT;
                daemon_local.pEvent.connections->status = INACTIVE;

                EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_change_filter_level(&daemon_local, level, 0));
                ASSERT_STREQ(daemon_local.pFilter.configs->name, daemon_local.pFilter.current->name);

                free(daemon_local.pEvent.connections->receiver);
                free(daemon_local.pEvent.connections);
            }
        }
    }
}

TEST(t_dlt_daemon_filter_change_filter_level, nullpointer)
{
    // NULL-Pointer, expect -1
    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_change_filter_level(NULL, 0, 0));
}

/* Begin Method: dlt_daemon_filter::t_dlt_daemon_filter_process_filter_control_messages*/
TEST(t_dlt_daemon_filter_process_filter_control_messages, normal)
{
    DltDaemonLocal daemon_local;
    DltDaemon daemon;
    DltReceiver receiver;
    memset(&daemon_local, 0, sizeof(DltDaemonLocal));
    memset(&daemon, 0, sizeof(DltDaemon));
    memset(&receiver, 0, sizeof(DltReceiver));

    EXPECT_EQ(DLT_RETURN_OK, dlt_daemon_filter_process_filter_control_messages(&daemon, &daemon_local, &receiver, 0));
}

TEST(t_dlt_daemon_filter_process_filter_control_messages, nullpointer)
{
    // NULL-Pointer, expect -1
    DltDaemon daemon;
    DltReceiver receiver;

    EXPECT_EQ(DLT_RETURN_ERROR, dlt_daemon_filter_process_filter_control_messages(&daemon, NULL, &receiver, 0));
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    ::testing::FLAGS_gtest_break_on_failure = false;
//    ::testing::FLAGS_gtest_filter = "t_dlt_daemon_filter_change_filter_level*";
    return RUN_ALL_TESTS();
}
