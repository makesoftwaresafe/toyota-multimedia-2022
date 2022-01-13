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
 * \file dlt-monitor-network.c
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

static pthread_t network_monitor_thread;
static int g_run_loop = 1; /* condition variable for the data collector thread */
static int g_update_interval;
static int g_interval_changed = 0;

DLT_DECLARE_CONTEXT(net_context);

#define NAME "NET"
#define MILLISEC_PER_SECOND 1000
#define BUF_MAX 1024

typedef enum {
    NET_IF = 0,
    RX_BYTES,
    RX_PACKETS,
    RX_ERRORS,
    RX_DROP,
    RX_FIFO,
    RX_FRAME,
    RX_COMPRESSED,
    RX_MULTICAST,
    TX_BYTES,
    TX_PACKETS,
    TX_ERRORS,
    TX_DROP,
    TX_FIFO,
    TX_COLLS,
    TX_CARRIER,
    TX_COMPRESSED,
    NO_OF_FIELDS_FOR_NETWORK_COMMAND_OUTPUT
} network_Params;


/**
 * @brief callback network data collector specific injection
 *
 * @param service_id injection service id
 * @param data injection data
 *   Data Expected: <INFO_TYPE> <PID>
 * @param length
 * @return 0 on sucess -1 otherwise
 */
int dlt_monitor_network_info_injection_callback(uint32_t service_id,
                                           void *data,
                                           uint32_t length)
{
    int interval = 0;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (data == NULL)
    {
        return -1;
    }

    DLT_LOG(net_context,
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
            DLT_LOG(net_context, DLT_LOG_VERBOSE, DLT_UINT32(g_update_interval));
        }
        else
        {
            DLT_LOG(net_context, DLT_LOG_ERROR, DLT_STRING("invalid interval"));
        }
    }

    return 0;
}

/**
 * @brief to initiate network info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int init(DataCollector *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        return -1;
    }

    if (collector->state == COLLECTOR_NOT_RUNNING)
    {
        DLT_REGISTER_CONTEXT(net_context,
                             collector->ctid,
                             "Network Data Collector");

        DLT_REGISTER_INJECTION_CALLBACK(net_context,
                                        collector->injection_service_id,
                                        dlt_monitor_network_info_injection_callback);

        collector->state = COLLECTOR_INITIALISED;

        DLT_LOG(net_context,
                DLT_LOG_VERBOSE,
                DLT_STRING("Initialize"),
                DLT_STRING(collector->name));
    }

    return 0;
}

/**
 * @brief thread to collect and send network related info to dlt
 */
static void *dlt_network_monitor(void *collector)
{
    int ret = -1;
    char *str = NULL;
    FILE *file_p = NULL;
    DataCollector *data_collector = NULL;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        DLT_LOG(net_context, DLT_LOG_ERROR, DLT_STRING("invalid collector."));
        pthread_exit(&ret);
    }

    data_collector = (DataCollector *)collector;
    str = (char*)malloc(BUF_MAX);
    if (str == NULL)
    {
        DLT_LOG(net_context, DLT_LOG_ERROR, DLT_STRING("malloc failed."));
        data_collector->state = COLLECTOR_NOT_RUNNING;
        pthread_exit(&ret);
    }

    while (g_run_loop)
    {
        int retry_cnt = 0;
        char s[4] = " \r\n";

        if (g_interval_changed)
        {
            data_collector->update_interval = g_update_interval;
            g_interval_changed = 0;
        }

        file_p = fopen("/proc/net/dev", "r");

        if (!file_p)
        {
            if (retry_cnt == 10)
            {
                DLT_LOG(net_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("dlt-network-monitor, abnormal exit status."),
                        DLT_STRING(strerror(errno)));
                break;
            }
            else
            {
                DLT_LOG(net_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("popen failed, retrying"),
                        DLT_STRING(strerror(errno)));
                retry_cnt++;
            }
        }
        else
        {
            DLT_LOG(net_context, DLT_LOG_INFO, DLT_STRING("BEG"));

            while (fgets(str, BUF_MAX, file_p) != NULL)
            {
                char *token = NULL;
                /* array of pointers to hold the fields of each line of /proc/net/dev output */
                char *fields[NO_OF_FIELDS_FOR_NETWORK_COMMAND_OUTPUT] = {NULL};
                int i = 0;
                char *save_ptr = NULL;
                    /* Expected output
                     * Inter-|   Receive                                                |  Transmit
                     * face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
                     *    lo:       0       0    0    0    0     0          0         0        0       0    0    0    0     0       0          0
                      * eth0: 10973508  149162    1    0    1     1          0         0 24321818  150506    0    0    0     0       0          0
                     */
                    /* get the first token */
                    token = strtok_r(str, s, &save_ptr);
                    /* walk through other tokens */
                    while ((token != NULL) && (i < NO_OF_FIELDS_FOR_NETWORK_COMMAND_OUTPUT))
                    {
                        fields[i] = strdup(token);
                        token = strtok_r(NULL, s, &save_ptr);
                        i++;
                    }

                    if (i == NO_OF_FIELDS_FOR_NETWORK_COMMAND_OUTPUT)
                    {
                        DLT_LOG(net_context,
                                DLT_LOG_INFO,
                                DLT_STRING(fields[NET_IF]),
                                DLT_STRING(fields[RX_BYTES]),
                                DLT_STRING(fields[RX_PACKETS]),
                                DLT_STRING(fields[RX_ERRORS]),
                                DLT_STRING(fields[RX_DROP]),
                                DLT_STRING(fields[RX_FIFO]),
                                DLT_STRING(fields[RX_FRAME]),
                                DLT_STRING(fields[RX_COMPRESSED]),
                                DLT_STRING(fields[RX_MULTICAST]),
                                DLT_STRING(fields[TX_BYTES]),
                                DLT_STRING(fields[TX_PACKETS]),
                                DLT_STRING(fields[TX_ERRORS]),
                                DLT_STRING(fields[TX_DROP]),
                                DLT_STRING(fields[TX_FIFO]),
                                DLT_STRING(fields[TX_COLLS]),
                                DLT_STRING(fields[TX_CARRIER]),
                                DLT_STRING(fields[TX_COMPRESSED]));
                    }

                    for (i = 0; i < NO_OF_FIELDS_FOR_NETWORK_COMMAND_OUTPUT; i++)
                    {
                        if (fields[i])
                        {
                            free(fields[i]);
                        }
                    }
            }
            DLT_LOG(net_context, DLT_LOG_INFO, DLT_STRING("END"));
            fclose(file_p);
        }
        usleep(g_update_interval * MILLISEC_PER_SECOND);
    }

    free(str);
    data_collector->state = COLLECTOR_NOT_RUNNING;
    return NULL;
}

/**
 * @brief to start network data collection to DLT
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
        collector->init(collector);
        g_run_loop = 1;
    }

    g_update_interval = collector->update_interval;
    collector->state = COLLECTOR_RUNNING;
    if (pthread_create(&network_monitor_thread,
                       NULL,
                       dlt_network_monitor,
                       collector) != 0)
    {
        DLT_LOG(net_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot create thread to communicate with DLT daemon"),
                DLT_STRING(strerror(errno)));
        collector->state = COLLECTOR_INITIALISED;
        return -1;
    }
    DLT_LOG(net_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));

    return 0;
}

/**
 * @brief to cleanup network info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    int ret = -1;

    pr_verbose("%s: %s\n", NAME, __func__);

    DLT_LOG(net_context,
            DLT_LOG_DEBUG,
            DLT_STRING("cleanup called"));
    if (collector == NULL)
    {
        return -1;
    }

    if (collector->state == COLLECTOR_RUNNING)
    {
        g_run_loop = 0;
        ret = pthread_cancel(network_monitor_thread);
        if (ret)
        {
            DLT_LOG(net_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("unable to cancel the thread"),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            ret = pthread_join(network_monitor_thread, NULL);
            if (ret)
            {
                DLT_LOG(net_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("unable to join the thread"),
                        DLT_STRING(strerror(errno)));
            }
        }
    }

    DLT_LOG(net_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    collector->state = COLLECTOR_NOT_RUNNING;

    DLT_UNREGISTER_CONTEXT(net_context);

    return 0;
}
