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
 * \file dlt-monitor-cpu.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>

#include "dlt.h"
#include "dlt_common.h" /* for dlt_get_version() */
#include "../dlt-monitor.h"

#define NAME "CPU"

DLT_DECLARE_CONTEXT(cpu_context);
#define BUF_MAX 1024
#define MAX_CPU 128
#define CPU_NAME_LEN 6
#define MILLISEC_PER_SECOND 1000
#define DLT_MONITOR_SERVICE_ID_SEND_CPU_CORES 0X2005

typedef struct
{
    char cpu_name[CPU_NAME_LEN];
    unsigned long long int user_mode; /*Time spent in user mode*/
    unsigned long long int nice; /*Time spent in user mode with low priority*/
    unsigned long long int system_mode; /*Time spent in system mode*/
    unsigned long long int idle_time; /*Time spent in idle task*/
    unsigned long long int iowait; /*Time waiting for I/O to complete*/
    unsigned long long int irq; /*Time servicing interrupts*/
    unsigned long long int softirq; /*Time servicing softirqs*/
    unsigned long long int steal; /*Time spent in other OSes when in virtual env*/
    unsigned long long int guest; /*Time spent running a virtual CPU for guest OS*/
    unsigned long long int guest_nice; /*Time spent running niced guest */
}CpuData;

typedef struct
{
    char *processor;
    char *model;
    char *bogo_mips;
    char *features;
}CpuInfo;

typedef struct
{
    char *hardware;
    char *revision;
    char *serial;
}HardwareInfo;

static pthread_t cpu_monitor_thread;
static int g_run_loop = 1; /*condition variable for the data collector thread*/
static int g_update_interval;
static int g_interval_changed = 0;

int dlt_monitor_cpu_injection_callback(uint32_t service_id, void *data, uint32_t length);

/**
 * @brief to read /proc/stat file and fill in usage info
 *
 * @param fp file pointer to /proc/stat
 * @param data CpuData pointer
 * @return 0 on success, -1 otherwise
 */
static int read_fields (FILE *fp, CpuData *data)
{
    int retval = 0;
    char buffer[BUF_MAX] = {0};

    if (fp == NULL || data == NULL)
    {
        return -1;
    }

    if (!fgets(buffer, BUF_MAX, fp))
    {
        DLT_LOG(cpu_context, DLT_LOG_ERROR, DLT_STRING("cannot read file"));
    }
    /* line starts with cp and a string. This is to handle cpu, cpu[0-9]+ */
    retval = sscanf (buffer, "cp%*s %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu %Lu",
                              &data->user_mode,
                              &data->nice,
                              &data->system_mode,
                              &data->idle_time,
                              &data->iowait,
                              &data->irq,
                              &data->softirq,
                              &data->steal,
                              &data->guest,
                              &data->guest_nice);
    if (retval == 0)
    {
        return -1;
    }
    if (retval < 4) /* Atleast 4 fields is to be read */
    {
        DLT_LOG(cpu_context,
                DLT_LOG_ERROR,
                DLT_STRING("Error reading /proc/stat cpu field"));
        return -1;
    }

    return 0;
}

/**
 * @brief thread to collect and send cpu usage data to dlt
 */
static void *dlt_cpu_monitor(void *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    FILE *fp = NULL;
    CpuData data;
    int cpus = 0;
    int count = 0;
    int ret = -1;
    char str[CPU_NAME_LEN] = {0};
    DataCollector *data_collector = NULL;

    if (collector == NULL)
    {
        pthread_exit(&ret);
    }

    data_collector = collector;

    fp = fopen("/proc/stat", "r");
    if (fp == NULL)
    {
        DLT_LOG(cpu_context, DLT_LOG_ERROR, DLT_STRING("cannot open file"));
        pthread_exit(&ret);
    }

    while (read_fields(fp, &data) != -1)
    {
        cpus++;
    }
    data_collector->state = COLLECTOR_RUNNING;
    while (g_run_loop)
    {
        if (g_interval_changed)
        {
            data_collector->update_interval = g_update_interval;
            g_interval_changed = 0;
        }
        fseek(fp, 0, SEEK_SET);
        fflush(fp);
        DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("BEG"), DLT_STRING("DATA"));

        for (count = 0; count < cpus; count++)
        {
            if (read_fields(fp, &data) == -1)
            {
                return 0;
            }
            if (count == 0)
            {
                DLT_LOG(cpu_context,
                        DLT_LOG_INFO,
                        DLT_STRING("CPU"),
                        DLT_UINT64(data.user_mode),
                        DLT_UINT64(data.nice),
                        DLT_UINT64(data.system_mode),
                        DLT_UINT64(data.idle_time),
                        DLT_UINT64(data.iowait),
                        DLT_UINT64(data.irq),
                        DLT_UINT64(data.softirq),
                        DLT_UINT64(data.steal),
                        DLT_UINT64(data.guest),
                        DLT_UINT64(data.guest_nice));
            }
            else
            {
                snprintf(str, CPU_NAME_LEN, "CPU%d", count-1);
                DLT_LOG(cpu_context,
                        DLT_LOG_INFO,
                        DLT_STRING(str),
                        DLT_UINT64(data.user_mode),
                        DLT_UINT64(data.nice),
                        DLT_UINT64(data.system_mode),
                        DLT_UINT64(data.idle_time),
                        DLT_UINT64(data.iowait),
                        DLT_UINT64(data.irq),
                        DLT_UINT64(data.softirq),
                        DLT_UINT64(data.steal),
                        DLT_UINT64(data.guest),
                        DLT_UINT64(data.guest_nice));
            }
        }

        DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("END"), DLT_STRING("DATA"));

        usleep(g_update_interval * MILLISEC_PER_SECOND);
    }

    data_collector->state = COLLECTOR_NOT_RUNNING;
    fclose(fp);
    return 0;
}

/**
 * @brief callback cpu data collector specific injection
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 always
 */
int dlt_monitor_cpu_injection_callback(uint32_t service_id,
                                void *data,
                                uint32_t length)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    int retval = 0;
    int interval = 0;

    DLT_LOG(cpu_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    retval = sscanf((char*)data, "I%*s %u", &interval);

    if (retval == 0)
    {
        g_run_loop = 0; /*to stop data collection*/
    }
    else
    {
        g_update_interval = interval; /*to update data collection interval*/
        g_interval_changed = 1;
        DLT_LOG(cpu_context, DLT_LOG_VERBOSE, DLT_UINT32(g_update_interval));
    }

    return 0;
}

/**
 * @brief callback to send number of cpu related information
 *
 * @param service_id injection service id
 * @param data injection data
 * @param length
 * @return 0 on success -1 on failure
 */
int dlt_monitor_cpu_injection_callback_send_cpu_info(uint32_t service_id,
                                               void *data,
                                               uint32_t length)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    FILE *cpuinfo = fopen("/proc/cpuinfo", "rb");
    char arg[BUF_MAX] = {0};
    int count = 0;
    CpuInfo *cpu_info = NULL;
    CpuInfo *tmp_cpu_info = NULL;
    HardwareInfo hardware_info = {NULL, NULL, NULL};
    int i = 0;
    int processor_number = 0;
    char *endptr = NULL;
    char *pch = NULL;

    if (cpuinfo == NULL)
    {
        return -1;
    }
    DLT_LOG(cpu_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    while (fgets(arg, BUF_MAX, cpuinfo) != NULL)
    {
        if (strncmp(arg, "processor", strlen("processor")) == 0)
        {
            count++;
        }
    }

    fseek(cpuinfo, 0, SEEK_SET);

    cpu_info = malloc(count * sizeof(CpuInfo));

    if (cpu_info == NULL)
    {
        DLT_LOG(cpu_context,
                DLT_LOG_ERROR,
                DLT_STRING("cannot allocate memory"));
        return -1;
    }

    DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("BEG"), DLT_STRING("INFO"));

    memset(cpu_info,0,count*sizeof(CpuInfo));
    tmp_cpu_info = cpu_info;

    while (fgets(arg, BUF_MAX, cpuinfo) != NULL)
    {
        if (strncmp(arg, "processor", strlen("processor")) == 0)
        {
            char temp[BUF_MAX] = {0};
            char *save_ptr = NULL;

            strncpy(temp, arg, BUF_MAX);
            pch = strtok_r(temp, ":",  &save_ptr);
            pch = strtok_r(NULL, "\r\n", &save_ptr);
            processor_number = strtol(pch, &endptr, 10);
            tmp_cpu_info = cpu_info + processor_number;
            tmp_cpu_info->processor = strdup(arg);
        }
        else if (strncmp(arg, "model name", strlen("model name")) == 0)
        {
            tmp_cpu_info->model = strdup(arg);
        }
        else if (strncasecmp(arg, "BogoMIPS", strlen("BogoMIPS")) == 0)
        {
            tmp_cpu_info->bogo_mips = strdup(arg);
        }
        else if ((strncmp(arg, "Features", strlen("Features")) == 0) ||
                 (strncmp(arg, "flags", strlen("flags")) == 0))
        {
            tmp_cpu_info->features = strdup(arg);
        }
        else if (strncmp(arg, "Hardware", strlen("Hardware")) == 0)
        {
            hardware_info.hardware = strdup(arg);
        }
        else if (strncmp(arg, "Revision", strlen("Revision")) == 0)
        {
            hardware_info.revision = strdup(arg);
        }
        else if (strncmp(arg, "Serial", strlen("Serial")) == 0)
        {
            hardware_info.serial = strdup(arg);
        }
    }

    tmp_cpu_info = cpu_info;

    for (i = 0; i < count; i++)
    {
        DLT_LOG(cpu_context,
                DLT_LOG_INFO,
                DLT_STRING(tmp_cpu_info->processor),
                DLT_STRING(tmp_cpu_info->model),
                DLT_STRING(tmp_cpu_info->bogo_mips),
                DLT_STRING(tmp_cpu_info->features));
        if (tmp_cpu_info->processor)
        {
            free(tmp_cpu_info->processor);
        }
        if (tmp_cpu_info->model)
        {
            free(tmp_cpu_info->model);
        }
        if (tmp_cpu_info->bogo_mips)
        {
            free(tmp_cpu_info->bogo_mips);
        }
        if (tmp_cpu_info->features)
        {
            free(tmp_cpu_info->features);
        }
        tmp_cpu_info++;
    }

    if ((hardware_info.hardware == NULL) && (hardware_info.revision == NULL) &&
        (hardware_info.serial == NULL))
    {
        DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("No hardware info"));
    }
    else
    {
        DLT_LOG(cpu_context,
                DLT_LOG_INFO,
                DLT_STRING(hardware_info.hardware),
                DLT_STRING(hardware_info.revision),
                DLT_STRING(hardware_info.serial));
    }

    DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("END"), DLT_STRING("INFO"));

    if (hardware_info.hardware)
    {
        free(hardware_info.hardware);
    }
    if (hardware_info.revision)
    {
        free(hardware_info.revision);
    }
    if (hardware_info.serial)
    {
        free(hardware_info.serial);
    }
    if (cpu_info)
    {
        free(cpu_info);
    }

    fclose(cpuinfo);

    return 0;
}

/**
 * @brief to initiate cpu usage data collector
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
        DLT_REGISTER_CONTEXT(cpu_context,
                             collector->ctid,
                             "cpu data collector Context for Logging");
        DLT_REGISTER_INJECTION_CALLBACK(cpu_context,
                                        collector->injection_service_id,
                                        dlt_monitor_cpu_injection_callback);
        DLT_REGISTER_INJECTION_CALLBACK(cpu_context,
                                        DLT_MONITOR_SERVICE_ID_SEND_CPU_CORES,
                                        dlt_monitor_cpu_injection_callback_send_cpu_info);
        collector->state = COLLECTOR_INITIALISED;
        DLT_LOG(cpu_context, DLT_LOG_VERBOSE, DLT_STRING("init cpu collector"));
    }

    return 0;
}

/**
 * @brief to start cpu usage data collector thread
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
        /*enabling it incase of loop stopped due to request via injection*/
        g_run_loop = 1;
    }
    g_update_interval = collector->update_interval;
    collector->state = COLLECTOR_RUNNING;
    if (pthread_create(&cpu_monitor_thread,
                      NULL,
                      dlt_cpu_monitor,
                      collector) != 0)
    {
        DLT_LOG(cpu_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot create thread to communicate with DLT daemon"));
        collector->state = COLLECTOR_INITIALISED;
        return -1;
    }
    DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));
    return 0;
}

/**
 * @brief to cleanup cpu usage data collector thread
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    int ret = 0;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        return -1;
    }

    if (collector->state == COLLECTOR_RUNNING)
    {
        g_run_loop = 0;
        ret = pthread_cancel(cpu_monitor_thread);
        if (ret)
        {
            DLT_LOG(cpu_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("unable to cancel the thread"),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            ret = pthread_join(cpu_monitor_thread, NULL);
            if (ret)
            {
                DLT_LOG(cpu_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("unable to join the thread"),
                        DLT_STRING(strerror(errno)));
            }
        }
    }

    DLT_LOG(cpu_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    DLT_UNREGISTER_CONTEXT(cpu_context);
    collector->state = COLLECTOR_NOT_RUNNING;

    return 0;
}
