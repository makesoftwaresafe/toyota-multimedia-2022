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
 * \file dlt-monitor-io.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include <dlt.h>
#include <dlt_common.h>
#include "../dlt-monitor.h"

static pthread_t io_monitor_thread;
static int g_run_loop = 1; /* condition variable for the data collector thread */
static int g_update_interval;
static char *filepath = "/tmp";
static int g_interval_changed = 0;

DLT_DECLARE_CONTEXT(io_context);

#define NAME "IO"
#define NO_OF_FIELDS_FOR_IO_COMMAND_OUTPUT 6
#define MILLISEC_PER_SECOND 1000
#define BUF_MAX 1024

typedef enum {
    DEV_NAME = 0,
    TPS,
    KB_READ_SEC,
    KB_WRITE_SEC,
    KB_READ,
    KB_WRITE,
    MAX_ENTRIES = NO_OF_FIELDS_FOR_IO_COMMAND_OUTPUT
} io_Params;

/**
 * @brief fills filepath to collector private data
 *
 * @param dc pointer to data collector structure
 * @param key collector config type
 * @param value read by conf file parser
 * @return 0 always
 */
int dlt_monitor_io_check_file_path(DataCollector *dc, char *key, char *value)
{
    if (dc == NULL || key == NULL || value == NULL)
    {
        fprintf(stderr,"invalid arg\n");
    }
    dc->private_data = strdup(value);
    return 0;
}

ConfigOption optional_configuration_entries_io[1] =
{
    [0] = {
        .key = "FilesPath",
        .opt = 0,
        .validate = dlt_monitor_io_check_file_path,
        .next = NULL}
};

/**
 * @brief callback io data collector specific injection
 *
 * @param service_id injection service id
 * @param data injection data
 *   Data Expected: <INFO_TYPE> <PID>
 * @param length
 * @return 0 on sucess -1 on syscall failure
 */
int dlt_monitor_io_info_injection_callback(uint32_t service_id,
                                           void *data,
                                           uint32_t length)
{
    int interval = 0;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (data == NULL)
    {
        return -1;
    }
    DLT_LOG(io_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    if (sscanf((char*)data, "I%*s %u", &interval))
    {
        g_update_interval = interval; /*to update data collection interval*/
        g_interval_changed = 1;
        DLT_LOG(io_context, DLT_LOG_VERBOSE, DLT_UINT32(g_update_interval));
    }

    return 0;
}

/**
 * @brief to initiate io info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int init(DataCollector *collector)
{
    int ret = -1;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        return ret;
    }
    if (collector->state == COLLECTOR_NOT_RUNNING)
    {
        DLT_REGISTER_CONTEXT(io_context,
                             collector->ctid,
                             "Filesystem Data Collector");

        DLT_REGISTER_INJECTION_CALLBACK(io_context,
                                        collector->injection_service_id,
                                        dlt_monitor_io_info_injection_callback);

        if (collector->private_data)
        {
            filepath = collector->private_data;
            ret = mkdir(filepath, S_IRUSR | S_IWUSR );
            if ((ret == -1) && (errno != EEXIST))
            {
                DLT_LOG(io_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("Failed to create configured folder"),
                        DLT_STRING(filepath));
                return ret;
            }
        }

        collector->state = COLLECTOR_INITIALISED;
        DLT_LOG(io_context,
                DLT_LOG_VERBOSE,
                DLT_STRING("Initialize"),
                DLT_STRING(collector->name));
    }
    return 0;
}

/**
 * @brief thread to collect and send io related info to dlt
 */
static void *dlt_io_monitor(void *collector)
{
    int ret = -1;
    char *str = NULL;
    DataCollector *data_collector = NULL;
    char buf[NAME_MAX] = {0};
    char filename[NAME_MAX] = {0};

    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        pthread_exit(&ret);
        DLT_LOG(io_context, DLT_LOG_ERROR, DLT_STRING("invalid collector."));
    }
    data_collector = (DataCollector *)collector;

    str = (char*)malloc(MAX_BUF_SIZE);
    if (str == NULL)
    {
        DLT_LOG(io_context, DLT_LOG_ERROR, DLT_STRING("malloc failed."));
        data_collector->state = COLLECTOR_NOT_RUNNING;
        pthread_exit(&ret);
    }

    snprintf(buf, NAME_MAX, "iostat -dk > %s/io.txt",filepath);
    snprintf(filename, NAME_MAX, "%s/io.txt",filepath);

    while (g_run_loop)
    {
        int line_cnt = 0;
        char s[4] = " \r\n";

        if (g_interval_changed)
        {
            data_collector->update_interval = g_update_interval;
            g_interval_changed = 0;
        }

        ret = system(buf);
        if (ret != 0)
        {
            DLT_LOG(io_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("io syscal failure"),
                    DLT_INT(ret));
            pthread_exit(&ret);
        }

        FILE *file_p = fopen(filename, "r");

        if (!file_p)
        {
            DLT_LOG(io_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("dlt-io-monitor, abnormal exit status."),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            DLT_LOG(io_context, DLT_LOG_INFO, DLT_STRING("BEG"));
            while (fgets(str, MAX_BUF_SIZE, file_p) != NULL)
            {
                char *token = NULL;
                /* array of pointers to hold the fields of each line of iostat output */
                char *fields[NO_OF_FIELDS_FOR_IO_COMMAND_OUTPUT] = {NULL};
                int i = 0;
                char *save_ptr = NULL;

                if (line_cnt > 1)
                {
                    /* Expected output
                     * Device:            tps    kB_read/s    kB_wrtn/s    kB_read    kB_wrtn
                     * mmcblk0           0.42         1.46        11.64      38236     304054
                     * mmcblk0p1         0.00         0.02         0.00        576          0
                     */
                    /* get the first token */
                    token = strtok_r(str, s, &save_ptr);
                    /* walk through other tokens */
                    while ((token != NULL) && (i < NO_OF_FIELDS_FOR_IO_COMMAND_OUTPUT))
                    {
                        fields[i] = strdup(token);
                        token = strtok_r(NULL, s, &save_ptr);
                        i++;
                    }
                    if (fields[DEV_NAME] && fields[TPS] && fields[KB_READ_SEC] &&
                        fields[KB_WRITE_SEC] && fields[KB_READ] && fields[KB_WRITE])
                    {
                        DLT_LOG(io_context,
                                DLT_LOG_INFO,
                                DLT_STRING(fields[DEV_NAME]),
                                DLT_STRING(fields[TPS]),
                                DLT_STRING(fields[KB_READ_SEC]),
                                DLT_STRING(fields[KB_WRITE_SEC]),
                                DLT_STRING(fields[KB_READ]),
                                DLT_STRING(fields[KB_WRITE]));
                    }
                    for (i = 0; i < NO_OF_FIELDS_FOR_IO_COMMAND_OUTPUT; i++)
                    {
                        if (fields[i])
                        {
                            free(fields[i]);
                        }
                    }
                }
                line_cnt++;
            }
            fclose(file_p);
            DLT_LOG(io_context, DLT_LOG_INFO, DLT_STRING("END"));
        }

        usleep(g_update_interval * MILLISEC_PER_SECOND);
    }
    free(str);
    data_collector->state = COLLECTOR_NOT_RUNNING;
    return NULL;
}

/**
 * @brief to start io data collection to DLT
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int collect(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        return -1;
    }

    if (collector->state == COLLECTOR_NOT_RUNNING)
    {
        if (collector->init(collector) == 0)
        {
            g_run_loop = 1;
        }
        else
        {
            return -1;
        }
    }
    g_update_interval = collector->update_interval;
    collector->state = COLLECTOR_RUNNING;
    if (pthread_create(&io_monitor_thread,
                       NULL,
                       dlt_io_monitor,
                       collector) != 0)
    {
        DLT_LOG(io_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot create thread to communicate with DLT daemon"),
                DLT_STRING(strerror(errno)));
        collector->state = COLLECTOR_INITIALISED;
        return -1;
    }
    DLT_LOG(io_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));

    return 0;
}

/**
 * @brief provides pointer to additional params structure
 *
 * @param list config option list ptr
 * @return 0 always
 */
int get_additional_parameter(ConfigOption **list)
{
    *list = optional_configuration_entries_io;
    return 0;
}

/**
 * @brief to cleanup io info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    int ret = -1;

    pr_verbose("%s: %s\n", NAME, __func__);

    DLT_LOG(io_context,
            DLT_LOG_DEBUG,
            DLT_STRING("cleanup called"));
    if (collector == NULL)
    {
        return -1;
    }
    if (collector->state == COLLECTOR_RUNNING)
    {
        g_run_loop = 0;
        ret = pthread_cancel(io_monitor_thread);
        if (ret)
        {
            DLT_LOG(io_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("unable to cancel the thread"),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            ret = pthread_join(io_monitor_thread, NULL);
            if (ret)
            {
                DLT_LOG(io_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("unable to join the thread"),
                        DLT_STRING(strerror(errno)));
            }
        }
    }

    DLT_LOG(io_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    collector->state = COLLECTOR_NOT_RUNNING;

    DLT_UNREGISTER_CONTEXT(io_context);

    return 0;
}
