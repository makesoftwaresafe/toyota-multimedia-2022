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
 * \file dlt-monitor-memory.c
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

static pthread_t memory_monitor_thread;
static int g_run_loop = 1; /* condition variable for the data collector thread */
static int g_update_interval;
static int g_interval_changed = 0;

DLT_DECLARE_CONTEXT(mem_context);

#define NAME "MEM"
#define MILLISEC_PER_SECOND 1000

typedef struct
{
    char *mem_total;
    char *mem_free;
    char *swap_total;
    char *swap_free;
}MemInfo;

/**
 * @brief callback memory data collector specific injection
 *
 * @param service_id injection service id
 * @param data injection data
 *   Data Expected: <INFO_TYPE> <PID>
 * @param length
 * @return 0 on sucess -1 otherwise
 */
int dlt_monitor_memory_info_injection_callback(uint32_t service_id,
                                           void *data,
                                           uint32_t length)
{
    int interval = 0;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (data == NULL)
    {
        return -1;
    }

    DLT_LOG(mem_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    if (sscanf((char*)data, "I%*s %u", &interval))
    {
        if (interval > 0)
        {
            g_update_interval = interval; /*to update data collection interval*/
            g_interval_changed = 1;
            DLT_LOG(mem_context, DLT_LOG_VERBOSE, DLT_UINT32(g_update_interval));
        }
        else
        {
            DLT_LOG(mem_context, DLT_LOG_ERROR, DLT_STRING("invalid interval"));
        }
    }

    return 0;
}

/**
 * @brief to initiate memory info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int init(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        DLT_LOG(mem_context, DLT_LOG_ERROR, DLT_STRING("Collector Invalid"));
        return -1;
    }

    if (collector->state == COLLECTOR_NOT_RUNNING)
    {
        DLT_REGISTER_CONTEXT(mem_context,
                             collector->ctid,
                             "Memory Data Collector");

        DLT_REGISTER_INJECTION_CALLBACK(mem_context,
                                        collector->injection_service_id,
                                        dlt_monitor_memory_info_injection_callback);

        collector->state = COLLECTOR_INITIALISED;

        DLT_LOG(mem_context,
                DLT_LOG_VERBOSE,
                DLT_STRING("Initialize"),
                DLT_STRING(collector->name));
    }

    return 0;
}

/**
 * @brief thread to collect and send memory related info to dlt
 */
static void *dlt_memory_monitor(void *collector)
{
    int ret = -1;
    char *str = NULL;
    FILE *file_p = NULL;
    DataCollector *data_collector = NULL;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        DLT_LOG(mem_context, DLT_LOG_ERROR, DLT_STRING("Collector Invalid"));
        pthread_exit(&ret);
    }

    data_collector = (DataCollector *)collector;
    str = (char*)malloc(BUF_MAX);
    if (str == NULL)
    {
        DLT_LOG(mem_context, DLT_LOG_ERROR, DLT_STRING("malloc failed."));
        data_collector->state = COLLECTOR_NOT_RUNNING;
        pthread_exit(&ret);
    }

    while (g_run_loop)
    {
        char delim[4] = " \r\n";
        MemInfo mem_info = {NULL, NULL, NULL, NULL};
        int complete_flag = 0;
        int retry_cnt = 0;

        if (g_interval_changed)
        {
            data_collector->update_interval = g_update_interval;
            g_interval_changed = 0;
        }

        file_p = fopen("/proc/meminfo", "r");

        if (!file_p)
        {
            if (retry_cnt == MAX_RETRY_COUNT)
            {
                DLT_LOG(mem_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("dlt-memory-monitor, abnormal exit status."),
                        DLT_STRING(strerror(errno)));
                break;
            }
            else
            {
                DLT_LOG(mem_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("fopen failed, retrying"),
                        DLT_STRING(strerror(errno)));
                retry_cnt++;
            }
        }
        else
        {
            DLT_LOG(mem_context, DLT_LOG_INFO, DLT_STRING("BEG"));

            while (fgets(str, BUF_MAX, file_p) != NULL)
            {
                char *token = NULL;
                char *save_ptr = NULL;

                /* Expected output
                 *  MemTotal:     81204 KB
                 *  MemFree:      61300 KB
                 */
                /* get the first token */
                token = strtok_r(str, delim, &save_ptr);
                if (strncmp(token, "MemTotal:", strlen("MemTotal:")) == 0)
                {
                    token = strtok_r(NULL, delim, &save_ptr);
                    mem_info.mem_total = strdup(token);
                }
                else if (strncmp(token, "MemFree:", strlen("MemFree:")) == 0)
                {
                    token = strtok_r(NULL, delim, &save_ptr);
                    mem_info.mem_free = strdup(token);
                }
                else if (strncmp(token, "SwapTotal:", strlen("SwapTotal:")) == 0)
                {
                    token = strtok_r(NULL, delim, &save_ptr);
                    mem_info.swap_total = strdup(token);
                }
                else if (strncmp(token, "SwapFree:", strlen("SwapFree:")) == 0)
                {
                    token = strtok_r(NULL, delim, &save_ptr);
                    mem_info.swap_free = strdup(token);
                    complete_flag = 1;
                }
                else
                {
                //do nothing
                }

                if (complete_flag)
                {
                    DLT_LOG(mem_context,
                            DLT_LOG_INFO,
                            DLT_STRING(mem_info.mem_total),
                            DLT_STRING(mem_info.mem_free),
                            DLT_STRING(mem_info.swap_total),
                            DLT_STRING(mem_info.swap_free));
                    complete_flag = 0;
                }
            }

            if (mem_info.mem_total)
            {
                free(mem_info.mem_total);
            }
            if (mem_info.mem_free)
            {
                free(mem_info.mem_free);
            }
            if (mem_info.swap_total)
            {
                free(mem_info.swap_total);
            }
            if (mem_info.swap_free)
            {
                free(mem_info.swap_free);
            }

            DLT_LOG(mem_context, DLT_LOG_INFO, DLT_STRING("END"));
            fclose(file_p);
        }
        usleep(g_update_interval * MILLISEC_PER_SECOND);
    }

    free(str);
    data_collector->state = COLLECTOR_NOT_RUNNING;
    return NULL;
}

/**
 * @brief to start memory data collection to DLT
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int collect(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        DLT_LOG(mem_context, DLT_LOG_ERROR, DLT_STRING("Collector Invalid"));
        return -1;
    }

    if (collector->state == COLLECTOR_NOT_RUNNING)
    {
        collector->init(collector);
        g_run_loop = 1;
    }

    g_update_interval = collector->update_interval;
    collector->state = COLLECTOR_RUNNING;
    if (pthread_create(&memory_monitor_thread,
                       NULL,
                       dlt_memory_monitor,
                       collector) != 0)
    {
        DLT_LOG(mem_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot create thread to communicate with DLT daemon"),
                DLT_STRING(strerror(errno)));
        collector->state = COLLECTOR_INITIALISED;
        return -1;
    }
    DLT_LOG(mem_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));

    return 0;
}

/**
 * @brief to cleanup memory info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    int ret = -1;

    pr_verbose("%s: %s\n", NAME, __func__);

    DLT_LOG(mem_context, DLT_LOG_DEBUG, DLT_STRING("cleanup called"));

    if (collector == NULL)
    {
        DLT_LOG(mem_context, DLT_LOG_ERROR, DLT_STRING("Collector Invalid"));
        return -1;
    }

    if (collector->state == COLLECTOR_RUNNING)
    {
        g_run_loop = 0;
        ret = pthread_cancel(memory_monitor_thread);
        if (ret)
        {
            DLT_LOG(mem_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("unable to cancel the thread"),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            ret = pthread_join(memory_monitor_thread, NULL);
            if (ret)
            {
                DLT_LOG(mem_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("unable to join the thread"),
                        DLT_STRING(strerror(errno)));
            }
        }
    }

    DLT_LOG(mem_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    collector->state = COLLECTOR_NOT_RUNNING;

    DLT_UNREGISTER_CONTEXT(mem_context);

    return 0;
}
