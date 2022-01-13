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
 * \author Anitha Ammaji Baggam <anithaammaji.baggam@in.bosch.com> ADIT 2016
 *
 * \file dlt-monitor-filesystem.c
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

#define NAME "FILESYS"

static pthread_t fileSystem_monitor_thread;
static int g_run_loop = 1; /*condition variable for the data collector thread*/
static int g_update_interval;
static char *filepath = "/tmp";
static int g_interval_changed = 0;

#define NO_OF_FIELDS_FOR_DF_COMMAND_OUTPUT 6

DLT_DECLARE_CONTEXT(fs_context);
#define MILLISEC_PER_SECOND 1000

typedef struct
{
    char *filesystem;
    char *size;
    char *used;
    char *available;
    char *used_percentage;
    char *mounted_on;
}FsInfo;

/**
 * @brief fills filepath to collector private data
 *
 * @param dc pointer to data collector structure
 * @param key collector config type
 * @param value read by conf file parser
 * @return 0 always
 */
int dlt_monitor_fs_check_file_path(DataCollector *dc, char *key, char *value)
{
    if (dc == NULL || key == NULL || value == NULL)
    {
        fprintf(stderr,"invalid arg\n");
    }
    dc->private_data = strdup(value);
    return 0;
}

ConfigOption optional_configuration_entries_fs[1] =
{
    [0] = {
        .key = "FilesPath",
        .opt = 0,
        .validate = dlt_monitor_fs_check_file_path,
        .next = NULL}
};

/**
 * @brief callback filesystem data collector specific injection
 *
 * @param service_id injection service id
 * @param data injection data
 *   Data Expected: <INFO_TYPE> <PID>
 * @param length
 * @return 0 on sucess -1 otherwise
 */
int dlt_monitor_filesystem_info_injection_callback(uint32_t service_id,
                                           void *data,
                                           uint32_t length)
{
    int interval = 0;

    pr_verbose("%s: %s\n", NAME, __func__);

    if (data == NULL)
    {
        return -1;
    }

    DLT_LOG(fs_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    if (sscanf((char*)data, "I%*s %u", &interval))
    {
        if (interval > 0)
        {
            g_update_interval = interval;
            g_interval_changed = 1;
            DLT_LOG(fs_context, DLT_LOG_VERBOSE, DLT_UINT32(g_update_interval));
        }
        else
        {
            DLT_LOG(fs_context, DLT_LOG_ERROR, DLT_STRING("invalid interval"));
        }
    }

    return 0;
}

/**
 * @brief to initiate filesysteminfo info collector
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
        DLT_REGISTER_CONTEXT(fs_context,
                             collector->ctid,
                             "Filesystem Data Collector");

        DLT_REGISTER_INJECTION_CALLBACK(fs_context,
                                        collector->injection_service_id,
                                        dlt_monitor_filesystem_info_injection_callback);

        if (collector->private_data)
        {
            filepath = collector->private_data;
            ret = mkdir(filepath, S_IRUSR | S_IWUSR );
            if ((ret == -1) && (errno != EEXIST))
            {
                DLT_LOG(fs_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("Failed to create configured folder"),
                        DLT_STRING(filepath));
                return ret;
            }
        }
        collector->state = COLLECTOR_INITIALISED;
        DLT_LOG(fs_context,
                DLT_LOG_VERBOSE,
                DLT_STRING("Initialize"),
                DLT_STRING(collector->name));
    }
    return 0;
}

/**
 * @brief thread to collect and send filesystem info to dlt
 */
static void *dlt_filesystem_monitor(void *collector)
{
    pr_verbose("%s: %s\n", NAME, __func__);

    int ret = -1;
    char *str = NULL;
    DataCollector *data_collector = NULL;
    char buf[NAME_MAX] = {0};
    char filename[NAME_MAX] = {0};

    if (collector == NULL)
    {
        pthread_exit(&ret);
    }
    data_collector = (DataCollector *)collector;

    str = (char*)malloc(MAX_BUF_SIZE);
    if (str == NULL)
    {
        DLT_LOG(fs_context, DLT_LOG_ERROR, DLT_STRING("malloc failed."));
        return (void *) -1;
    }

    snprintf(buf, NAME_MAX, "df -hP > %s/fs.txt",filepath);
    snprintf(filename, NAME_MAX, "%s/fs.txt",filepath);

    while (g_run_loop)
    {
        if (g_interval_changed)
        {
            data_collector->update_interval = g_update_interval;
            g_interval_changed = 0;
        }
        ret = system(buf);

        if (ret != 0)
        {
            DLT_LOG(fs_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("io syscal failure"),
                    DLT_INT(ret));
            pthread_exit(&ret);
        }

        FILE *file_p = fopen(filename, "r");

        if (!file_p)
        {
            DLT_LOG(fs_context, DLT_LOG_ERROR,
                    DLT_STRING("dlt-FS-monitor, error in getting df o/p from file."),
                    DLT_STRING("df -hP"));
        }
        else
        {
            DLT_LOG(fs_context, DLT_LOG_INFO, DLT_STRING("BEG"));
            while (fgets(str, MAX_BUF_SIZE, file_p) != NULL)
            {
                char *token = NULL;
                char s[4] = " \r\n";
                /*array of pointers to hold the df fields of each line
                * Filesystem Size Used Available Use% Mounted on*/
                char *fields[NO_OF_FIELDS_FOR_DF_COMMAND_OUTPUT] = {NULL};
                int i = 0;
                int count = 0;
                char *save_ptr = NULL;
                /* get the first token */
                token = strtok_r(str, s, &save_ptr);
                /* walk through other tokens */
                while (token != NULL && count < NO_OF_FIELDS_FOR_DF_COMMAND_OUTPUT)
                {
                    count++;
                    fields[i] = strdup(token);
                    token = strtok_r(NULL, s, &save_ptr);
                    i++;
                }
                if (fields[0] != NULL && fields[1] != NULL &&
                    fields[2] != NULL && fields[3] != NULL &&
                    fields[4] != NULL && fields[5] != NULL)
                {
                    DLT_LOG(fs_context,
                            DLT_LOG_INFO,
                            DLT_STRING(fields[0]),
                            DLT_STRING(fields[1]),
                            DLT_STRING(fields[2]),
                            DLT_STRING(fields[3]),
                            DLT_STRING(fields[4]),
                            DLT_STRING(fields[5]));
                }
                for (i = 0; i < NO_OF_FIELDS_FOR_DF_COMMAND_OUTPUT; i++)
                {
                    if (fields[i])
                    {
                        free(fields[i]);
                    }
                }
            }
            fclose(file_p);
            DLT_LOG(fs_context, DLT_LOG_INFO, DLT_STRING("END"));
        }
        usleep(g_update_interval * MILLISEC_PER_SECOND);
    }/*end of while (g_run_loop)*/
    if (str != NULL )
    {
        free(str);
    }
    data_collector->state = COLLECTOR_NOT_RUNNING;
    return NULL;
}

/**
 * @brief to send adit product info to DLT
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
    if (pthread_create(&fileSystem_monitor_thread,
                       NULL,
                       dlt_filesystem_monitor,
                       collector) != 0)
    {
        DLT_LOG(fs_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot create thread to communicate with DLT daemon"));
        return -1;
    }
    DLT_LOG(fs_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));
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
    *list = optional_configuration_entries_fs;
    return 0;
}

/**
 * @brief to cleanup filesystem info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    int ret = -1;

    pr_verbose("%s: %s\n", NAME, __func__);

    DLT_LOG(fs_context,
            DLT_LOG_DEBUG,
            DLT_STRING("cleanup called"));
    if (collector == NULL)
    {
        return -1;
    }
    if (collector->state == COLLECTOR_RUNNING)
    {
        g_run_loop = 0;
        ret = pthread_cancel(fileSystem_monitor_thread);
        if (ret)
        {
            DLT_LOG(fs_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("unable to cancel the thread"),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            ret = pthread_join(fileSystem_monitor_thread, NULL);
            if (ret)
            {
                DLT_LOG(fs_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("unable to join the thread"),
                        DLT_STRING(strerror(errno)));
            }
        }
    }

    DLT_LOG(fs_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    collector->state = COLLECTOR_NOT_RUNNING;

    DLT_UNREGISTER_CONTEXT(fs_context);

    return 0;
}
