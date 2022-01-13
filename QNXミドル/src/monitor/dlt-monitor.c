/**
 * @licence app begin@
 * Copyright (C) 2016  Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * This file is part of GENIVI Project Dlt - Diagnostic Log and Trace monitor apps.
 *
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author Manikandan Chockalingam <Manikandan.Chockalingam@in.bosch.com> ADIT 2016
 *
 * \file dlt-monitor.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <mqueue.h>
#include <signal.h>
#include <dlfcn.h>
#include <errno.h>
#include <ctype.h>

#include "dlt.h"
#include "dlt_common.h"
#include "dlt_config_file_parser.h"
#include "dlt-monitor.h"


DLT_DECLARE_CONTEXT(sm_context);

SystemMonitor sm; /*SystemMonitor global structure*/

typedef struct
{
    DltMonitorCollectorEvent event;
    char *name;
}MsgDataBuffer;

#define MONITOR_MAJOR_VERSION "0"
#define MONITOR_MINOR_VERSION "1"
#define GENERAL_BASE_NAME "General"
#define COLLECTOR_BASE_NAME "DataCollector"
#define DLT_SYSMON_CONFIG_PATH CONFIGURATION_FILES_DIR "/dlt-sysmon.conf"

static int local_verbose;

int get_verbosity(void)
{
    return local_verbose;
}

void set_verbosity(int v)
{
    local_verbose = !!v;
}

/**
 * @brief Checks monitor application name
 *
 * @param sm pointer to system monitor structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_app_name(SystemMonitor *sm, char *value)
{
    pr_function_verbose();

    if (sm == NULL || value == NULL)
    {
        return -1;
    }

    strncpy(sm->appid, value, DLT_ID_SIZE);
    if (sm->appid == NULL)
    {
        fprintf(stderr, "apid not defined\n");
        return -1;
    }
    sm->appid[DLT_ID_SIZE] = 0;
    return 0;
}

/**
 * @brief Checks monitor application context id
 *
 * @param sm pointer to system monitor structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_ctid(SystemMonitor *sm, char *value)
{
    pr_function_verbose();

    if (sm == NULL || value == NULL)
    {
        return -1;
    }

    strncpy(sm->ctid, value, DLT_ID_SIZE);
    if (sm->ctid == NULL)
    {
        fprintf(stderr, "ctid not defined\n");
        return -1;
    }

    sm->ctid[DLT_ID_SIZE] = 0;
    return 0;
}

/**
 * @brief Checks monitor application acquisition type
 *
 * @param sm pointer to system monitor structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_acquisition_type(SystemMonitor *sm, char *value)
{
    pr_function_verbose();

    if (sm == NULL || value == NULL)
    {
        return -1;
    }

    sm->accquisition_type = strdup(value);
    if (sm->accquisition_type == NULL)
    {
        fprintf(stderr, "Cannot duplicate string for acquisition type\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Checks monitor application update interval
 *
 * @param sm pointer to system monitor structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_update_interval(SystemMonitor *sm, char *value)
{
    pr_function_verbose();

    char *endptr = NULL;

    if (sm == NULL || value == NULL)
    {
        return -1;
    }

    sm->update_interval = strtol(value, &endptr, 10);
    if (sm->update_interval > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * Expected entries for a system monitor configuration
 * Caution: after changing entries here,
 * dlt_monitor_check_param needs to be updated as well
 * */
static DltMonitorConf monitor_cfg_entries[MONITOR_CONF_COUNT] =
{
    [MONITOR_CONF_APP_NAME] = {
        .key = "ApplicationName",
        .func = dlt_monitor_check_app_name,
        .is_opt = 0 },
    [MONITOR_CONF_CTID] = {
        .key = "ContextName",
        .func = dlt_monitor_check_ctid,
        .is_opt = 0 },
    [MONITOR_CONF_ACQ_TYPE] = {
        .key = "StartDataAcquisition",
        .func = dlt_monitor_check_acquisition_type,
        .is_opt = 0 },
    [MONITOR_CONF_UPDATE_INTERVAL] = {
        .key = "UpdateInterval",
        .func = dlt_monitor_check_update_interval,
        .is_opt = 0 }
};

/**
 * Check if DltMonitorConf configuration parameter is valid.
 *
 * @param sm pointer to system monitor structure
 * @param ctype monitor configuration type
 * @param value specified property value from configuration file
 * @return 0 on success, -1 otherwise
 */
static int dlt_monitor_check_param(SystemMonitor *sm,
                                   DltMonitorConfType ctype,
                                   char *value)
{
    pr_function_verbose();

    if (sm == NULL || value == NULL)
    {
        return -1;
    }

    if (ctype < MONITOR_CONF_COUNT)
    {
        return monitor_cfg_entries[ctype].func(sm, value);
    }
    return -1;
}

/**
 * @brief Checks data collector name
 *
 * @param dc pointer to data collector structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_collector_name(DataCollector *dc, char *value)
{
    pr_function_verbose();

    if (dc == NULL || value == NULL)
    {
        return -1;
    }

    dc->name = strdup(value);
    if (dc->name == NULL)
    {
        fprintf(stderr, "collector name not defined\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Checks data collector context id
 *
 * @param dc pointer to data collector structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_collector_ctid(DataCollector *dc, char *value)
{
    pr_function_verbose();

    if (dc == NULL || value == NULL)
    {
        return -1;
    }

    strncpy(dc->ctid, value, DLT_ID_SIZE);
    if (dc->ctid == NULL)
    {
        fprintf(stderr, "Cannot allocate memory for section ctid\n");
        return -1;
    }
    dc->ctid[DLT_ID_SIZE] = 0;
    return 0;
}

/**
 * @brief Checks data collector enable type
 *
 * @param dc pointer to data collector structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_collector_enable_type(DataCollector *dc,
                                            char *value)
{
    pr_function_verbose();

    char *endptr = NULL;

    if (dc == NULL || value == NULL)
    {
        return -1;
    }

    dc->enable = strtol(value, &endptr, 10);
    if (dc->enable > 0)
    {
        return 0;
    }
    return -1;
}

/**
 * @brief Checks data collector injection service id
 *
 * @param dc pointer to data collector structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_collector_injection_service_id(DataCollector *dc,
                                                     char *value)
{
    pr_function_verbose();

    char *endptr = NULL;

    if (dc == NULL || value == NULL)
    {
        return -1;
    }

    dc->injection_service_id = strtol(value, &endptr, 10);

    if (errno == ERANGE)
    {
        fprintf(stderr, "service id out of range\n");
        return -1;
    }
    return 0;
}

/**
 * @brief Checks data collector update interval
 *
 * @param dc pointer to data collector structure
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_check_collector_update_interval(DataCollector *dc,
                                                char *value)
{
    pr_function_verbose();

    char *endptr = NULL;

    if (dc == NULL || value == NULL)
    {
        return -1;
    }

    dc->update_interval = strtol(value, &endptr, 10);

    if (dc->update_interval > 0)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

/**
 * Expected entries for a data collector configuration
 * Caution: after changing entries here,
 * dlt_monitor_check_collector_param needs to be updated as well
 */
static DltCollectorConf collector_cfg_entries[COLLECTOR_CONF_COUNT] =
{
    [COLLECTOR_NAME] = {
        .key = "Name",
        .func = dlt_monitor_check_collector_name,
        .is_opt = 0 },
    [COLLECTOR_CTID] = {
        .key = "ContextName",
        .func = dlt_monitor_check_collector_ctid,
        .is_opt = 0 },
    [COLLECTOR_ENABLE] = {
        .key = "Enable",
        .func = dlt_monitor_check_collector_enable_type,
        .is_opt = 0 },
    [COLLECTOR_INJECTION_SERVICE_ID] = {
        .key = "InjectionServiceIDs",
        .func = dlt_monitor_check_collector_injection_service_id,
        .is_opt = 0 },
    [COLLECTOR_UPDATE_INTERVAL] = {
        .key = "UpdateInterval",
        .func = dlt_monitor_check_collector_update_interval,
        .is_opt = 0 }
};

/**
 * @brief Checks data collector config
 *
 * @param dc pointer to data collector structure
 * @param ctype collector config type
 * @param value read by conf file parser
 * @return 0 on success, -1 otherwise
 */
static int dlt_monitor_check_collector_param(DataCollector *dc,
                                             DltCollectorConfType ctype,
                                             char *value)
{
    pr_function_verbose();

    if (dc == NULL || value == NULL)
    {
        return -1;
    }

    if (ctype < COLLECTOR_CONF_COUNT)
    {
        return collector_cfg_entries[ctype].func(dc, value);
    }
    return -1;
}

/**
 * @brief reads system monitor configuration
 *
 * @param sm system monitor structure
 * @return 0 on success, -1 otherwise
 */
static int dlt_monitor_read_configurations(SystemMonitor *sm)
{
    pr_function_verbose();

    DltConfigFile *file = NULL;
    int num_of_sections = 0;
    int sec = 0;
    int ret = 0;
    int invalid = 0;
    DataCollector *collector = NULL;
    DataCollector *collector_tmp = NULL;
    int num_of_collectors = 0;

    if (sm == NULL)
    {
        return -1;
    }
    /* read configuration file */
    file = dlt_config_file_init(sm->conf_file);
    if (file == NULL)
    {
        fprintf(stderr, "file handle null\n");
        return -1;
    }
    ret = dlt_config_file_get_num_sections(file, &num_of_sections);

    if (ret != 0)
    {
        dlt_config_file_release(file);
        fprintf(stderr, "Invalid number of sections in configuration file\n");
        return -1;
    }

    /* not considering the general section */
    num_of_collectors = num_of_sections - 1;

    if (num_of_collectors > 0)
    {
        /* allocating memory for storing the collector configuration temporarily */
        collector = malloc(sizeof(DataCollector)*(num_of_collectors));
        if (collector == NULL)
        {
            dlt_config_file_release(file);
            fprintf(stderr, "cannot allocate memory for collector\n");
            return -1;
        }
        collector_tmp = collector;
    }
    else
    {
        fprintf(stderr, "collectors not mentioned in config file\n");
        return -1;
    }
    while (sec < num_of_sections)
    {
        char sec_name[DLT_CONFIG_FILE_ENTRY_MAX_LEN] = {0};

        if ((ret = dlt_config_file_get_section_name(file, sec, sec_name)) == -1)
        {
            fprintf(stderr, "Failed to read section name\n");
            break;
        }

        if (strstr(sec_name, GENERAL_BASE_NAME) != NULL)
        {
            DltMonitorConfType type = MONITOR_CONF_APP_NAME;
            char value[DLT_CONFIG_FILE_ENTRY_MAX_LEN] = {0};

            for (type = 0; type < MONITOR_CONF_COUNT; type++)
            {
                ret = dlt_config_file_get_value(file,
                                                sec_name,
                                                monitor_cfg_entries[type].key,
                                                value);
                if ((ret != 0) ||
                    ((dlt_monitor_check_param(sm, type, value) == -1) &&
                    (monitor_cfg_entries[type].is_opt == 0)))
                {
                    /*incase of system monitor configuration error
                                  *exiting system monitor by returning error here
                                  */
                    fprintf(stderr, "system monitor configuration is invalid\n");
                    return -1;
                }
            }
        }
        else if (strstr(sec_name, COLLECTOR_BASE_NAME) != NULL)
        {
            DltCollectorConfType type = COLLECTOR_NAME;
            char value[DLT_CONFIG_FILE_ENTRY_MAX_LEN] = {0};

            /*getting collector configuration entries and storing it temporarily*/
            for (type = 0; type < COLLECTOR_CONF_COUNT; type++)
            {
                ret = dlt_config_file_get_value(file,
                                                sec_name,
                                                collector_cfg_entries[type].key,
                                                value);
                if ((ret != 0) ||
                    ((dlt_monitor_check_collector_param(collector, type, value) == -1) &&
                    (collector_cfg_entries[type].is_opt == 0)))
                {
                    invalid = 1;
                    fprintf(stderr, "collector configuration is invalid\n");
                    break;
                }
            }
            if (invalid == 0)
            {
                /*if the collector configuration entries are valid, then collector count is incremented*/
                collector++;
                sm->num_of_collectors++;
            }
            else
            {
                invalid = 0;
            }
        }
        else /* unknown section */
        {
            fprintf(stderr, "unknown section \n");
        }
        sec++;
    }
    if (sm->num_of_collectors > 0)
    {
        /* storing the read data in system monitor structure*/
        sm->collectors = malloc(sizeof(DataCollector)*(sm->num_of_collectors));
        if (sm->collectors == NULL)
        {
            dlt_config_file_release(file);
            fprintf(stderr, "cannot allocate memory for collector");
            return -1;
        }
        memcpy(sm->collectors, collector_tmp, (sizeof(DataCollector)*sm->num_of_collectors));
    }
    else
    {
        fprintf(stderr, "No vaid collector configuration found\n");
        return -1;
    }
    free(collector_tmp);
    dlt_config_file_release(file);
    return ret;
}

/**
 * @brief gets the collector interface api's
 *
 * @param sm system monitor structure
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_get_data_collectors(SystemMonitor *sm)
{
    pr_function_verbose();

    char *libprefix = "lib";
    char libname[NAME_MAX] = {0};
    char *libsuffix = ".so";
    int i = 0;
    int ret = 0;
    DataCollector *collector = NULL;
    void *data_collector_handle = NULL;

    if (sm == NULL || sm->collectors == NULL)
    {
        return -1;
    }

    collector = sm->collectors;

    for (i = 0; i < sm->num_of_collectors; i++)
    {
        strcpy(libname, libprefix);
        strcat(libname, collector->name);
        strcat(libname, libsuffix);

        data_collector_handle = dlopen(libname, RTLD_NOW);

        if (data_collector_handle == NULL)
        {
              DLT_LOG(sm_context, DLT_LOG_ERROR, DLT_STRING("dlopen failed"));
              ret = -1;
              break;
        }
        collector->init = dlsym(data_collector_handle, "init");
        collector->collect = dlsym(data_collector_handle, "collect");
        collector->cleanup = dlsym(data_collector_handle, "cleanup");
        collector->get_additional_parameter = dlsym(data_collector_handle,
                                                    "get_additional_parameter");

        if ((collector->init == NULL) ||
            (collector->collect == NULL) ||
            (collector->cleanup == NULL) ||
            (collector->get_additional_parameter == NULL))
        {
        /* if any interface is not available in the data collector, then just print an error*/
            DLT_LOG(sm_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("collector interface(s) null"));
        }
        collector++;
    }
    return ret;
}

/**
 * @brief parse the config file and trigger validation
 *
 * @param sm system monitor structure
 * @param dc data collector structure
 * @param config_option ConfigOption structure
 * @return 0 on success, -1 otherwise
 */
static int dlt_monitor_parse_and_validate(SystemMonitor *sm,
                                          DataCollector *dc,
                                          ConfigOption *config_option)
{
    pr_function_verbose();

    DltConfigFile *file = NULL;
    int num_of_sections = 0;
    int sec = 0;
    int ret = 0;
    ConfigOption *cfg_opt_tmp = NULL;

    if (sm == NULL || dc == NULL || config_option == NULL)
    {
        return -1;
    }

    cfg_opt_tmp = config_option;

    /* read configuration file */
    file = dlt_config_file_init(sm->conf_file);
    if (file == NULL)
    {
        DLT_LOG(sm_context, DLT_LOG_ERROR, DLT_STRING("file handle null"));
        return -1;
    }
    ret = dlt_config_file_get_num_sections(file, &num_of_sections);

    if (ret != 0)
    {
        dlt_config_file_release(file);
        DLT_LOG(sm_context,
                DLT_LOG_ERROR,
                DLT_STRING("Invalid number of sections in configuration file"));
        return -1;
    }
    while (sec < num_of_sections)
    {
        char sec_name[DLT_CONFIG_FILE_ENTRY_MAX_LEN] = {0};

        if ((ret = dlt_config_file_get_section_name(file, sec, sec_name)) == -1)
        {
            DLT_LOG(sm_context,
                    DLT_LOG_WARN,
                    DLT_STRING("Failed to read section name"));
            break;
        }
        if (strstr(sec_name, COLLECTOR_BASE_NAME) != NULL)
        {
            char value[DLT_CONFIG_FILE_ENTRY_MAX_LEN] = {0};

            ret = dlt_config_file_get_value(file,
                                            sec_name,
                                            collector_cfg_entries[COLLECTOR_NAME].key,
                                            value);
            if (strcmp(value, dc->name) == 0)
            {
                while (cfg_opt_tmp != NULL)
                {
                    ret = dlt_config_file_get_value(file,
                                                    sec_name,
                                                    cfg_opt_tmp->key,
                                                    value);
                    if ((ret != 0) ||
                        ((cfg_opt_tmp->validate(dc, cfg_opt_tmp->key, value) == -1) &&
                        (cfg_opt_tmp->opt == 0)))
                    {
                        DLT_LOG(sm_context,
                                DLT_LOG_WARN,
                                DLT_STRING("collector configuration is invalid"));
                        break;
                    }
                    cfg_opt_tmp = cfg_opt_tmp->next;
                }
            }
        }
        sec++;
    }
    return 0;
}

/**
 * @brief gets the collector specifc parameters
 *
 * @param sm system monitor structure
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_get_collector_specific_param(SystemMonitor *sm)
{
    pr_function_verbose();

    int i = 0;
    DataCollector *collector = NULL;
    ConfigOption *config_option = NULL;

    if (sm == NULL || sm->collectors == NULL)
    {
        return -1;
    }

    collector = sm->collectors;

    for (i = 0; i < sm->num_of_collectors; i++)
    {
        if (collector->get_additional_parameter)
        {
            collector->get_additional_parameter(&config_option);
            if (config_option == NULL)
            {
                DLT_LOG(sm_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("config_option null"));
                return -1;
            }
            else
            {
                if (dlt_monitor_parse_and_validate(sm, collector, config_option) == -1)
                {
                    DLT_LOG(sm_context,
                            DLT_LOG_ERROR,
                            DLT_STRING("error in parsing/validation"));
                    return -1;
                }
            }
        }
        collector++;
    }
    return 0;
}

/**
 * @brief initialize all available data collectors
 *
 * @param sm system monitor structure
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_init_data_collectors(SystemMonitor *sm)
{
    pr_function_verbose();

    int i = 0;
    DataCollector *collector = NULL;

    if (sm == NULL || sm->collectors == NULL)
    {
        return -1;
    }

    collector = sm->collectors;

    for (i = 0; i < sm->num_of_collectors; i++)
    {
        if (collector->init)
        {
            collector->init(collector);
        }
        collector++;
    }
    return 0;
}

/**
 * @brief cleanup all available data collectors
 *
 * @param sm system monitor structure
 * @return 0 on success, -1 otherwise
 */
int dlt_monitor_cleanup_data_collectors(SystemMonitor *sm)
{
    pr_function_verbose();

    int i = 0;
    DataCollector *collector = NULL;

    if (sm == NULL || sm->collectors == NULL)
    {
        return -1;
    }

    collector = sm->collectors;

    for (i = 0; i < sm->num_of_collectors; i++)
    {
        if ((collector->state == COLLECTOR_RUNNING ||
             collector->state == COLLECTOR_INITIALISED) &&
             collector->cleanup)
        {
            collector->cleanup(collector);
        }
        collector++;
    }
    return 0;
}

/* exit handled in case of proper termination of the app
*/
void dlt_monitor_atexit_handler(void)
{
    pr_function_verbose();

    mqd_t dlt_monitor_queue_handle;
    int i = 0;
    DataCollector *collector = NULL;

    dlt_monitor_cleanup_data_collectors(&sm);
    dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_WRONLY);
    if (dlt_monitor_queue_handle > 0)
    {
        mq_close(dlt_monitor_queue_handle);
        mq_unlink(QUEUE_NAME);
    }
    free(sm.accquisition_type);
    collector = sm.collectors;

    for (i = 0; i < sm.num_of_collectors; i++)
    {
        if (collector->private_data)
        {
            free(collector->private_data);
        }
        free(collector->name);
        collector++;
    }
    free(sm.collectors);
    DLT_UNREGISTER_CONTEXT(sm_context);
    DLT_UNREGISTER_APP();
}

/* exit handled in case of signals
*/
void dlt_monitor_exit_handler(int sig)
{
    pr_function_verbose();

    mqd_t dlt_monitor_queue_handle;
    MsgDataBuffer msg_data_buffer;

    switch (sig)
    {
        case SIGHUP:
        case SIGTERM:
        case SIGINT:
        case SIGQUIT:
        {
            dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_WRONLY);

            msg_data_buffer.event = DLT_STOP_SYSTEM_MONITOR;

            if (mq_send(dlt_monitor_queue_handle,
                        (char *)&msg_data_buffer,
                        sizeof(MsgDataBuffer),
                        0))
            {
                fprintf(stderr, "msg send failure\n");
            }
            break;
        }
        default:
        {
            break;
        }
    } /* switch */

}

/**
 * @brief callback to start all available data collectors
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 always
 */
int dlt_monitor_injection_callback_start_all_collectors(uint32_t service_id,
                                                     void *data,
                                                     uint32_t length)
{
    pr_function_verbose();

    mqd_t dlt_monitor_queue_handle;
    MsgDataBuffer msg_data_buffer;

    DLT_LOG(sm_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_WRONLY);

    msg_data_buffer.event = DLT_MONITOR_START_ALL_COLLECTORS;

    if (mq_send(dlt_monitor_queue_handle,
                (char *)&msg_data_buffer,
                sizeof(MsgDataBuffer),
                0))
    {
        fprintf(stderr, "msg send failure\n");
    }
    return 0;
}

/**
 * @brief callback to stop all available data collectors
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 always
 */
int dlt_monitor_injection_callback_stop_all_collectors(uint32_t service_id,
                                                    void *data,
                                                    uint32_t length)
{
    pr_function_verbose();

    mqd_t dlt_monitor_queue_handle;
    MsgDataBuffer msg_data_buffer;

    DLT_LOG(sm_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_WRONLY);
    if (strcmp(data, "TERM") == 0)
    {
        msg_data_buffer.event = DLT_STOP_SYSTEM_MONITOR;
    }
    else
    {
        msg_data_buffer.event = DLT_MONITOR_STOP_ALL_COLLECTORS;
    }
    if (mq_send(dlt_monitor_queue_handle,
                (char *)&msg_data_buffer,
                sizeof(MsgDataBuffer),
                0))
    {
        fprintf(stderr, "msg send failure\n");
    }
    return 0;
}

/**
 * @brief callback to start data collector by mentioning collector name
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 always
 */
int dlt_monitor_injection_callback_start_collector_by_name(uint32_t service_id,
                                                        void *data,
                                                        uint32_t length)
{
    pr_function_verbose();

    mqd_t dlt_monitor_queue_handle;
    MsgDataBuffer msg_data_buffer;

    if (data != NULL)
    {
        DLT_LOG(sm_context,
                DLT_LOG_VERBOSE,
                DLT_UINT32(service_id),
                DLT_STRING(data),
                DLT_UINT32(length));

        dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_WRONLY);

        msg_data_buffer.event = DLT_MONITOR_START_COLLECTOR_BY_NAME;
        msg_data_buffer.name = strdup(data);
        if (mq_send(dlt_monitor_queue_handle,
                    (char *)&msg_data_buffer,
                    sizeof(MsgDataBuffer),
                    0))
        {
            fprintf(stderr, "msg send failure\n");
        }
    }
    return 0;
}

/**
 * @brief callback to stop data collector by mentioning collector name
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 always
 */
int dlt_monitor_injection_callback_stop_collector_by_name(uint32_t service_id,
                                                       void *data,
                                                       uint32_t length)
{
    pr_function_verbose();

    mqd_t dlt_monitor_queue_handle;
    MsgDataBuffer msg_data_buffer;

    if (data != NULL)
    {
        DLT_LOG(sm_context,
                DLT_LOG_VERBOSE,
                DLT_UINT32(service_id),
                DLT_STRING(data),
                DLT_UINT32(length));

        dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_WRONLY);

        msg_data_buffer.event = DLT_MONITOR_STOP_COLLECTOR_BY_NAME;
        msg_data_buffer.name = strdup(data);

        if (mq_send(dlt_monitor_queue_handle,
                    (char *)&msg_data_buffer,
                    sizeof(MsgDataBuffer),
                    0))
        {
            fprintf(stderr, "msg send failure\n");
        }
    }
    return 0;
}

/**
 * @brief to start or stop data collector filtering by name
 *
 * @param sm system monitor structure
 * @param msg_data_buffer message buffer
 * @return 0 on success -1 on failure
 */
int dlt_monitor_trigger_collector(SystemMonitor *sm, MsgDataBuffer msg_data_buffer)
{
    pr_function_verbose();

    int i = 0;
    DataCollector *collector = NULL;

    if (sm->collectors == NULL)
    {
        return -1;
    }

    collector = sm->collectors;

    for (i = 0; i < sm->num_of_collectors; i++)
    {
        if (strncmp(collector->name,msg_data_buffer.name, strlen(collector->name)) == 0)
        {
            if (msg_data_buffer.event == DLT_MONITOR_STOP_COLLECTOR_BY_NAME)
            {
                if ((collector->state == COLLECTOR_RUNNING ||
                     collector->state == COLLECTOR_INITIALISED)&&
                     collector->cleanup)
                {
                    pr_verbose("%s: Stop collector: %s\n", __func__, collector->name);
                    collector->cleanup(collector);
                }
            }
            else if (msg_data_buffer.event == DLT_MONITOR_START_COLLECTOR_BY_NAME)
            {
                if ((collector->state == COLLECTOR_NOT_RUNNING ||
                     collector->state == COLLECTOR_INITIALISED) &&
                     collector->collect)
                {
                    pr_verbose("%s: Start collector: %s\n", __func__, collector->name);
                    collector->collect(collector);
                }
            }
            else
            {
                pr_verbose("%s: No valid event\n", __func__);
                DLT_LOG(sm_context, DLT_LOG_WARN, DLT_STRING("Not a valid event"));
            }
        }
        collector++;
    }
    free(msg_data_buffer.name);
    return 0;
}

/**
 * @brief callback to send system monitor version
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 always
 */
int dlt_monitor_injection_callback_send_monitor_version(uint32_t service_id,
                                                     void *data,
                                                     uint32_t length)
{
    pr_function_verbose();

    char buf[NAME_MAX] = {0};

    snprintf(buf,
             NAME_MAX,
             "DLT Monitor Version %s.%s",
             MONITOR_MAJOR_VERSION,
             MONITOR_MINOR_VERSION);
    DLT_LOG(sm_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));
    DLT_LOG(sm_context, DLT_LOG_INFO, DLT_STRING(buf));
    return 0;
}

/**
 * @brief callback to send all avaialble collectors information
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 on success -1 on failure
 */
int dlt_monitor_injection_callback_send_collector_details(uint32_t service_id,
                                                       void *data,
                                                       uint32_t length)
{
    pr_function_verbose();

    int i = 0;
    DataCollector *collector = sm.collectors;

    DLT_LOG(sm_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    DLT_LOG(sm_context,
            DLT_LOG_INFO,
            DLT_STRING("Number of Collectors:"),
            DLT_INT32(sm.num_of_collectors));

    for (i = 0; i < sm.num_of_collectors; i++)
    {
        if (collector == NULL)
        {
            return -1;
        }
        DLT_LOG(sm_context,
                DLT_LOG_INFO,
                DLT_STRING("Ctid:"),
                DLT_STRING(collector->ctid),
                DLT_STRING("Name:"),
                DLT_STRING(collector->name),
                DLT_STRING("State:"),
                DLT_INT32(collector->state),
                DLT_STRING("Service ID:"),
                DLT_INT32(collector->injection_service_id),
                DLT_STRING("Update Interval:"),
                DLT_INT32(collector->update_interval));

        collector++;
    }
    return 0;
}

/**
 * @brief to start all available data collectors
 *
 * @param sm system monitor structure
 * @return 0 on success -1 on failure
 */
int dlt_monitor_start_data_collectors(SystemMonitor *sm)
{
    pr_function_verbose();

    int i = 0;
    DataCollector *collector = NULL;

    if (sm == NULL || sm->collectors == NULL)
    {
        return -1;
    }

    collector = sm->collectors;

    for (i = 0; i < sm->num_of_collectors; i++)
    {
        if ((collector->state == COLLECTOR_NOT_RUNNING ||
             collector->state == COLLECTOR_INITIALISED) &&
             collector->collect)
        {
            collector->collect(collector);
        }
        collector++;
    }
    return 0;
}
/**
 * Print usage information of tool.
 */
void usage(void)
{
    printf("Usage: dlt-monitor [options]\n");
    printf("Options:\n");
    printf("  -h            Usage\n");
    printf("  -c filename   DLT monitor configuration file (Default: "
            CONFIGURATION_FILES_DIR "/dlt-sysmon.conf)\n");
}

/**
 * @brief to read the input args for system monitor
 *
 * @param sm system monitor structure
 * @param argc
 * @param argv
 * @return 0 on success -1 on failure -2 on usage request
 */
int monitor_option_handling(SystemMonitor *sm, int argc, char* argv[])
 {
    int c;

    if (sm == NULL)
    {
        fprintf (stderr, "Invalid parameter passed to monitor_option_handling()\n");
        return -1;
    }

    /* Initialize system monitor structure */
    memset(sm, 0, sizeof(SystemMonitor));

    opterr = 0;

    while ((c = getopt (argc, argv, "hc:v")) != -1)
    {
        switch (c)
        {
            case 'c':
            {
                strncpy(sm->conf_file, optarg, NAME_MAX);
                break;
            }
            case 'h':
            {
                usage();
                return -2; /* return no error */
            }
            case 'v':
            {
                set_verbosity(1);
                break;
            }
            case '?':
            {
                if (optopt == 'c')
                {
                    fprintf (stderr, "Option -%c requires an argument.\n", optopt);
                }
                else if (isprint (optopt))
                {
                    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
                }
                else
                {
                    fprintf (stderr, "Unknown option character `\\x%x'.\n", optopt);
                }
                /* unknown or wrong option used, show usage information and terminate */
                usage();
                return -1;
            }
            default:
            {
                fprintf (stderr, "Invalid option, this should never occur!\n");
                return -1;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[])
{
    MsgDataBuffer msg_data_buffer;
    struct mq_attr attr;
    int ret = 0;

    /* Command line option handling */
    if ((ret = monitor_option_handling(&sm, argc, argv)) < 0)
    {
        if(ret != -2)
        {
            /*if not help requested*/
            fprintf (stderr, "monitor_option_handling() failed!\n");
        }
        return -1;
    }

    if (sm.conf_file[0] == 0)
    {
        strncpy(sm.conf_file, DLT_SYSMON_CONFIG_PATH, NAME_MAX);
    }

    if (dlt_monitor_read_configurations(&sm) == -1)
    {
        /*in case of following errors the system monitor is exited
         *1.configuration file not found/general section entries incomplete/wrong
         *2.no valid data collectors mentioned in conf file
         */
        fprintf(stderr, "Failed to read configurations\n");
        return -1;
    }

    atexit(dlt_monitor_atexit_handler);
    signal(SIGTERM, dlt_monitor_exit_handler); /* software termination signal from kill */
    signal(SIGHUP, dlt_monitor_exit_handler); /* hangup signal */
    signal(SIGQUIT, dlt_monitor_exit_handler);
    signal(SIGINT, dlt_monitor_exit_handler);


    dlt_with_timestamp(1);
    dlt_with_ecu_id(1);
    dlt_verbose_mode();

    DLT_REGISTER_APP(sm.appid, "system monitor application");
    DLT_REGISTER_CONTEXT(sm_context, sm.ctid, "system monitor Context for Logging");

    DLT_REGISTER_INJECTION_CALLBACK(sm_context,
                                    DLT_MONITOR_SERVICE_ID_START_ALL_COLLECTORS,
                                    dlt_monitor_injection_callback_start_all_collectors);
    DLT_REGISTER_INJECTION_CALLBACK(sm_context,
                                    DLT_MONITOR_SERVICE_ID_STOP_ALL_COLLECTORS,
                                    dlt_monitor_injection_callback_stop_all_collectors);
    DLT_REGISTER_INJECTION_CALLBACK(sm_context,
                                    DLT_MONITOR_SERVICE_ID_START_COLLECTOR_BY_NAME,
                                    dlt_monitor_injection_callback_start_collector_by_name);
    DLT_REGISTER_INJECTION_CALLBACK(sm_context,
                                    DLT_MONITOR_SERVICE_ID_STOP_COLLECTOR_BY_NAME,
                                    dlt_monitor_injection_callback_stop_collector_by_name);
    DLT_REGISTER_INJECTION_CALLBACK(sm_context,
                                    DLT_MONITOR_SERVICE_ID_SEND_COLLECTOR_DETAILS,
                                    dlt_monitor_injection_callback_send_collector_details);
    DLT_REGISTER_INJECTION_CALLBACK(sm_context,
                                    DLT_MONITOR_SERVICE_ID_SEND_MONITOR_VERSION,
                                    dlt_monitor_injection_callback_send_monitor_version);

    if (dlt_monitor_get_data_collectors(&sm) != 0)
    {
        DLT_LOG(sm_context, DLT_LOG_ERROR, DLT_STRING("cannot load collector interfaces"));
        return -1;
    }
    if (dlt_monitor_get_collector_specific_param(&sm) != 0)
    {
        DLT_LOG(sm_context, DLT_LOG_ERROR, DLT_STRING("cannot get collector specific param"));
    }
    if (dlt_monitor_init_data_collectors(&sm) != 0)
    {
        DLT_LOG(sm_context, DLT_LOG_ERROR, DLT_STRING("cannot initialize collectors"));
        return -1;
    }

    /* initialize the queue attributes */
    attr.mq_flags = 0;
    attr.mq_maxmsg = 20;
    attr.mq_msgsize = sizeof(MsgDataBuffer);
    attr.mq_curmsgs = 0;

    sm.dlt_monitor_queue_handle = mq_open(QUEUE_NAME, O_CREAT | O_RDONLY, 0644, &attr);

    if (sm.dlt_monitor_queue_handle < 0)
    {
        if(errno == EEXIST)
        {
            fprintf(stderr, "Old message queue exists, trying to delete.\n");
            if(mq_unlink(QUEUE_NAME) < 0)
            {
                fprintf(stderr, "Could not delete existing message queue: %s \n",strerror(errno));
                return -1;
            }
            else /* Retry */
            {
                sm.dlt_monitor_queue_handle = mq_open(QUEUE_NAME,
                                                      O_CREAT | O_RDONLY,
                                                      0644,
                                                      &attr);
            }
        }
    }

    pr_verbose("Entering Mainloop\n");

    msg_data_buffer.event = DLT_MONITOR_NO_EVENT;
    while(1)
    {
        switch (msg_data_buffer.event)
        {
            case DLT_MONITOR_START_ALL_COLLECTORS:
            {
                dlt_monitor_start_data_collectors(&sm);
                msg_data_buffer.event = DLT_MONITOR_NO_EVENT;
                break;
            }
            case DLT_MONITOR_STOP_ALL_COLLECTORS:
            {
                dlt_monitor_cleanup_data_collectors(&sm);
                msg_data_buffer.event = DLT_MONITOR_NO_EVENT;
                break;
            }
            case DLT_MONITOR_START_COLLECTOR_BY_NAME:
            case DLT_MONITOR_STOP_COLLECTOR_BY_NAME:
            {
                dlt_monitor_trigger_collector(&sm,msg_data_buffer);
                msg_data_buffer.event = DLT_MONITOR_NO_EVENT;
                break;
            }
            case DLT_STOP_SYSTEM_MONITOR:
            {
                dlt_monitor_cleanup_data_collectors(&sm);
                DLT_UNREGISTER_CONTEXT(sm_context);
                DLT_UNREGISTER_APP();
                DLT_LOG(sm_context, DLT_LOG_INFO, DLT_STRING("stopping system monitor"));
                return 0;
            }
            default:
            {
                ssize_t bytes_read;
                /* wait for any event to occur through DLT injection */
                bytes_read = mq_receive(sm.dlt_monitor_queue_handle,
                                        (char*)&msg_data_buffer,
                                        sizeof(MsgDataBuffer),
                                        NULL);
                if (bytes_read != sizeof(msg_data_buffer))
                {
                    DLT_LOG(sm_context, DLT_LOG_ERROR, DLT_STRING("error while msg queue"));
                }
                break;
            }
        }
    }

    pr_verbose("Exit program\n");

    return 0;
}
