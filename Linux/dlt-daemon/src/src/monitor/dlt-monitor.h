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

#ifndef DLT_MONITOR_H_
#define DLT_MONITOR_H_

#include <limits.h> /* for NAME_MAX */
#include <sys/stat.h>

#ifndef pr_fmt
#   define pr_fmt(fmt) fmt
#endif

#define pr_error(fmt, ...) \
    ({ fprintf(stdout, pr_fmt(fmt), ## __VA_ARGS__); fflush(stdout); })
#define pr_verbose(fmt, ...) \
    ({ if (get_verbosity()) { fprintf(stdout, pr_fmt(fmt), ## __VA_ARGS__); fflush(stdout); } })

#define pr_function_verbose() \
    { pr_verbose("%s()\n", __func__); }

int get_verbosity(void);
void set_verbosity(int);

#define MAX_BUF_SIZE 4096
#define QUEUE_NAME "/dlt_monitor_message_queue"

typedef struct DataCollector_st DataCollector;
typedef int (*DataCollector_fnptr)(DataCollector *dc);
typedef struct ConfigOption_st ConfigOption;

/**
 *The structure to handle collector specific parameters
 * the validation function (inside collector library), needed to store information
 * in the collector structure's private data after validation
 */
struct ConfigOption_st
{
    char *key; /* expected key in configuration file */
    int opt;  /* flag to mark a Config optional */
    int (*validate)(DataCollector *dc, char *key, char *val); /* fnptr to validation function*/
    ConfigOption *next; /* pointer to next, set to NULL, if no additional parameter available */
};

struct DataCollector_st
{
    char *name;
    char ctid[DLT_ID_SIZE+1];
    int injection_service_id;
    int state;
    int enable;
    int update_interval;
    int type;
    void *private_data; /*collector specifc information*/
    DataCollector_fnptr init;
    DataCollector_fnptr collect;
    DataCollector_fnptr cleanup;
    int (*get_additional_parameter)(ConfigOption **list);
};

typedef struct SystemMonitor_st SystemMonitor;

struct SystemMonitor_st
{
    char appid[DLT_ID_SIZE+1];
    char ctid[DLT_ID_SIZE+1];
    char conf_file[NAME_MAX]; /*system monitor configuration file*/
    char *accquisition_type; /*current type supported is on demand*/
    int update_interval;
    int num_of_collectors;
    DataCollector *collectors; /*handle for list of available collectors*/
    mqd_t dlt_monitor_queue_handle; /*handle for system monitor msg Q*/
};

typedef enum
{
    COLLECTOR_NOT_RUNNING = 0,
    COLLECTOR_INITIALISED,
    COLLECTOR_RUNNING
}DltMonitorCollectorState;

typedef enum
{
    DLT_MONITOR_NO_EVENT = 0,
    DLT_MONITOR_START_ALL_COLLECTORS,
    DLT_MONITOR_STOP_ALL_COLLECTORS,
    DLT_MONITOR_START_COLLECTOR_BY_NAME,
    DLT_MONITOR_STOP_COLLECTOR_BY_NAME,
    DLT_STOP_SYSTEM_MONITOR
}DltMonitorCollectorEvent;

typedef struct {
    char *key;  /* The configuration key*/
    int (*func)(SystemMonitor *sm, char *value); /* Conf handler */
    int is_opt; /* If the configuration is optional or not */
} DltMonitorConf;

typedef enum {
    MONITOR_CONF_APP_NAME = 0,
    MONITOR_CONF_CTID,
    MONITOR_CONF_ACQ_TYPE,
    MONITOR_CONF_UPDATE_INTERVAL,
    MONITOR_CONF_COUNT
} DltMonitorConfType;

typedef struct {
    char *key;  /* The configuration key*/
    int (*func)(DataCollector *dc, char *value); /* Conf handler */
    int is_opt; /* If the configuration is optional or not */
} DltCollectorConf;

typedef enum {
    COLLECTOR_NAME = 0,
    COLLECTOR_CTID,
    COLLECTOR_ENABLE,
    COLLECTOR_INJECTION_SERVICE_ID,
    COLLECTOR_UPDATE_INTERVAL,
    COLLECTOR_CONF_COUNT
} DltCollectorConfType;

#define DLT_MONITOR_SERVICE_ID_START_ALL_COLLECTORS    0X2000
#define DLT_MONITOR_SERVICE_ID_STOP_ALL_COLLECTORS     0X2001
#define DLT_MONITOR_SERVICE_ID_START_COLLECTOR_BY_NAME 0X2002
#define DLT_MONITOR_SERVICE_ID_STOP_COLLECTOR_BY_NAME  0X2003
#define DLT_MONITOR_SERVICE_ID_SEND_COLLECTOR_DETAILS  0X2004
#define DLT_MONITOR_SERVICE_ID_SEND_MONITOR_VERSION    0X1001

#define BUF_MAX 1024
#define MAX_RETRY_COUNT 10

#endif
