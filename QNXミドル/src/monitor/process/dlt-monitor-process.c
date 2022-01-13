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
 * \file dlt-monitor-process.c
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
#include "dlt-monitor-process.h"

static pthread_t process_monitor_thread;
static int g_run_loop = 1; /* condition variable for the data collector thread */
static int g_update_interval;
static char *filepath = "/tmp";
static int g_interval_changed = 0;

DLT_DECLARE_CONTEXT(ps_context);

/**
 * @brief fills filepath to collector private data
 *
 * @param dc pointer to data collector structure
 * @param key collector config type
 * @param value read by conf file parser
 * @return 0 always
 */
int dlt_monitor_process_check_file_path(DataCollector *dc, char *key, char *value)
{
    if (dc == NULL || key == NULL || value == NULL)
    {
        fprintf(stderr,"invalid arg\n");
    }
    dc->private_data = strdup(value);
    return 0;
}

ConfigOption optional_configuration_entries[1] =
{
    [0] = {
        .key = "FilesPath",
        .opt = 0,
        .validate = dlt_monitor_process_check_file_path,
        .next = NULL}
};

/**
 * @brief process the request for thread related info
 *
 * @param pid process id
 * @return 0 on sucess -1 on failure
 */
int dlt_monitor_process_thread_info(int pid)
{
    char str[BUF_MAX] = {0};
    char buf[NAME_MAX] = {0};
    int retval = 0;
    char filename[NAME_MAX] = {0};
    int line_cnt = 0;

    pr_verbose("%s: %s\n", NAME, __func__);

    snprintf(buf, NAME_MAX, TOP_COMMAND, pid, filepath);
    retval = system(buf);
    if (retval != 0)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("Top syscal failure"),
                DLT_INT(retval));
        return retval; /* Leaving function */
    }

    snprintf(filename, NAME_MAX, "%s/thread.txt", filepath);
    FILE *file_p = fopen(filename, "r");
    if (!file_p)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("couldnt read thread info"),
                DLT_STRING(strerror(errno)));
        return -1; /* Leaving function */
    }

    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("BEG"), DLT_STRING("THREAD"));

    while (fgets(str, BUF_MAX, file_p) != NULL)
    {
        char *token = NULL;
        char s[4] = " \r\n";
        /* array of pointers to hold the fields of each line of ps output */
        char *fields[NO_OF_FIELDS_FOR_TOP_COMMAND_OUTPUT] = {NULL};
        char *save_ptr = NULL;
        int i = 0;
        /* Expected ouput:
         *
         * top - 01:08:08 up  1:08,  0 users,  load average: 0.00, 0.01, 0.05
         * Threads:   1 total,   0 running,   1 sleeping,   0 stopped,   0 zombie
         * %Cpu(s):  0.0 us,  0.1 sy,  0.0 ni, 99.8 id,  0.0 wa,  0.0 hi,  0.0 si,  0.0 st
         * KiB Mem :  1799464 total,  1747468 free,    14364 used,    37632 buff/cache
         * KiB Swap:        0 total,        0 free,        0 used.  1753528 avail Mem
         *
         *  PID USER      PR  NI    VIRT    RES    SHR S %CPU %MEM     TIME+ COMMAND
         *   1 root      20   0   13312   2888   2008 S  0.0  0.2   0:03.15 systemd
         */

        /* ignoring first 6 lines which are not of our interest */
        if (line_cnt > 5)
        {
            token = strtok_r(str, s, &save_ptr); /* get the first token */
            /* walk through other tokens */
            while ((token != NULL) && (i < NO_OF_FIELDS_FOR_TOP_COMMAND_OUTPUT))
            {
                fields[i] = strdup(token);
                token = strtok_r(NULL, s, &save_ptr); /* tokenizing further */
                i++;
            }
            if (fields[TP_PID] && fields[TP_USER] && fields[TP_PRIORITY] &&
                fields[TP_NICE] && fields[TP_VMEM] && fields[TP_RMEM] &&
                fields[TP_SMEM] && fields[TP_STATE] && fields[TP_PCPU] &&
                fields[TP_MEMORY] && fields[TP_TIME] && fields[TP_PROC_NAME])
            {
                DLT_LOG(ps_context,
                        DLT_LOG_INFO,
                        DLT_STRING(fields[TP_PID]),
                        DLT_STRING(fields[TP_USER]),
                        DLT_STRING(fields[TP_PRIORITY]),
                        DLT_STRING(fields[TP_NICE]),
                        DLT_STRING(fields[TP_VMEM]),
                        DLT_STRING(fields[TP_RMEM]),
                        DLT_STRING(fields[TP_SMEM]),
                        DLT_STRING(fields[TP_STATE]),
                        DLT_STRING(fields[TP_PCPU]),
                        DLT_STRING(fields[TP_MEMORY]),
                        DLT_STRING(fields[TP_TIME]),
                        DLT_STRING(fields[TP_PROC_NAME]));
            }
            for (i = 0; i < NO_OF_FIELDS_FOR_TOP_COMMAND_OUTPUT; i++)
            {
                if (fields[i])
                {
                    free(fields[i]);
                }
            }
       }
       line_cnt++;
    }
    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("END"), DLT_STRING("THREAD"));
    fclose(file_p);
    return retval;
}

/**
 * @brief process the request for different user info
 *
 * @param pid process id
 * @return 0 on sucess -1 on syscall failure
 */
int dlt_monitor_process_user_info(int pid)
{
    char str[BUF_MAX] = {0};
    char buf[NAME_MAX] = {0};
    int retval = 0;
    char filename[NAME_MAX] = {0};

    pr_verbose("%s: %s\n", NAME, __func__);

    snprintf(buf, NAME_MAX, USER_COMMAND, pid, filepath);
    retval = system(buf);
    if (retval != 0)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("ps syscal failure"),
                DLT_INT(retval));
        return retval; /* Leaving function */
    }

    snprintf(filename, NAME_MAX, "%s/usr.txt", filepath);
    FILE *file_p = fopen(filename, "r");
    if (!file_p)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("couldnt read user info"),
                DLT_STRING(strerror(errno)));
        return -1; /* Leaving function */
    }

    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("BEG"), DLT_STRING("USER"));

    while (fgets(str, BUF_MAX, file_p) != NULL)
    {
        char *token = NULL;
        char s[4] = " \r\n";
        /* array of pointers to hold the fields of each line of ps output */
        char *fields[NO_OF_FIELDS_FOR_USER_COMMAND_OUTPUT] = {NULL};
        int i = 0;
        char *save_ptr = NULL;

        /* Expected output:
         * USER     SUSER    RUSER    EUSER    FSUSER   COMMAND           PID
         * root     root     root     root     root     systemd             1
         */
        /* get the first token */
        token = strtok_r(str, s, &save_ptr);
        /* walk through other tokens */
        while ((token != NULL) && (i < NO_OF_FIELDS_FOR_USER_COMMAND_OUTPUT))
        {
            fields[i] = strdup(token);
            token = strtok_r(NULL, s, &save_ptr);
            i++;
        }
        if (fields[UP_USER] && fields[UP_SUSER] && fields[UP_RUSER] &&
            fields[UP_EUSER] && fields[UP_FSUSER] && fields[UP_PROC_NAME] &&
            fields[UP_PID])
        {
            DLT_LOG(ps_context,
                    DLT_LOG_INFO,
                    DLT_STRING(fields[UP_USER]),
                    DLT_STRING(fields[UP_SUSER]),
                    DLT_STRING(fields[UP_RUSER]),
                    DLT_STRING(fields[UP_EUSER]),
                    DLT_STRING(fields[UP_FSUSER]),
                    DLT_STRING(fields[UP_PROC_NAME]),
                    DLT_STRING(fields[UP_PID]));
        }
        for (i = 0; i < NO_OF_FIELDS_FOR_USER_COMMAND_OUTPUT; i++)
        {
            if (fields[i])
            {
                free(fields[i]);
            }
        }
    }
    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("END"), DLT_STRING("USER"));
    fclose(file_p);
    return retval;
}

/**
 * @brief process the request for list of open files
 *
 * @param pid process id
 * @return 0 on sucess -1 on syscall failure
 */
int dlt_monitor_process_open_files_info(int pid)
{
    char str[BUF_MAX] = {0};
    char buf[NAME_MAX] = {0};
    int retval = 0;
    char filename[NAME_MAX] = {0};

    pr_verbose("%s: %s\n", NAME, __func__);

    snprintf(buf, NAME_MAX, LSOF_COMMAND, pid, filepath);
    retval = system(buf);
    if (retval != 0)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("lsof syscal failure"),
                DLT_INT(retval));
        return retval; /* Leaving function */
    }

    snprintf(filename, NAME_MAX, "%s/files.txt", filepath);
    FILE *file_p = fopen(filename, "r");
    if (!file_p)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("couldnt read list of open files"),
                DLT_STRING(strerror(errno)));
        return -1; /* Leaving function */
    }

    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("BEG"), DLT_STRING("LSOF"));

    while (fgets(str, BUF_MAX, file_p) != NULL)
    {
        char *token = NULL;
        /* lsof utillity formats o/p with tab spacing */
        char s[4] = "\t\r\n";
        /* array of pointers to hold the fields of each line of ps output */
        char *fields[NO_OF_FIELDS_FOR_LSOF_COMMAND_OUTPUT] = {NULL};
        int i = 0;
        char *save_ptr = NULL;

        /* Expected ouput:
         * 1   /lib/systemd/systemd    /dev/kmsg
         * 1   /lib/systemd/systemd    anon_inode:[eventpoll]
         */
        /* get the first token */
        token = strtok_r(str, s, &save_ptr);
        /* walk through other tokens */
        while ((token != NULL) && (i < NO_OF_FIELDS_FOR_LSOF_COMMAND_OUTPUT))
        {
            fields[i] = strdup(token);
            token = strtok_r(NULL, s, &save_ptr);
            i++;
        }
        if (fields[FP_PID] && fields[FP_PROC_NAME] && fields[FP_FILE_NAME])
        {
            DLT_LOG(ps_context,
                    DLT_LOG_INFO,
                    DLT_STRING(fields[FP_PID]),
                    DLT_STRING(fields[FP_PROC_NAME]),
                    DLT_STRING(fields[FP_FILE_NAME]));
        }
        for (i = 0; i < NO_OF_FIELDS_FOR_LSOF_COMMAND_OUTPUT; i++)
        {
            if (fields[i])
            {
                free(fields[i]);
            }
        }
    }
    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("END"), DLT_STRING("LSOF"));
    fclose(file_p);
    return retval;
}

/**
 * @brief process the request for memory map info
 *
 * @param pid process id
 * @return 0 on sucess -1 on syscall failure
 */
int dlt_monitor_process_memory_map(int pid)
{
    char str[BUF_MAX] = {0};
    char buf[NAME_MAX] = {0};
    int retval = 0;
    char filename[NAME_MAX] = {0};

    pr_verbose("%s: %s\n", NAME, __func__);

    snprintf(buf, NAME_MAX, MMAP_COMMAND, pid, filepath);
    retval = system(buf);
    if (retval != 0)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("pmap syscal failure"),
                DLT_INT(retval));
        return retval;
    }

    snprintf(filename, NAME_MAX, "%s/mmap.txt", filepath);
    FILE *file_p = fopen(filename, "r");
    if (!file_p)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("couldnt get memory map"),
                DLT_STRING(strerror(errno)));
        return -1;
    }

    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("BEG"), DLT_STRING("MMAP"));

    while (fgets(str, BUF_MAX, file_p) != NULL)
    {
        char *token = NULL;
        char s[4] = " \r\n";
        /* array of pointers to hold the fields of each line of ps output */
        char *fields[NO_OF_FIELDS_FOR_PMAP_COMMAND_OUTPUT] = {NULL};
        int i = 0;
        char *save_ptr = NULL;

        /* Expected ouput:
         * Address   Kbytes     RSS   Dirty Mode  Mapping
         * 762d8000       4       0       0 -----   [ anon ]
         * 762d9000    8188       8       8 rw---   [ anon ]
         * 76ad8000      12       8       0 r-x-- libattr.so.1.1.0
         * 76adb000      64       0       0 ----- libattr.so.1.1.0
         */
        /* get the first token */
        token = strtok_r(str, s, &save_ptr);
        /* walk through other tokens */
        while ((token != NULL) && (i < NO_OF_FIELDS_FOR_PMAP_COMMAND_OUTPUT))
        {
            fields[i] = strdup(token);
            if (i < MP_MODE)
            {
                token = strtok_r(NULL, s, &save_ptr);
            }
            else
            {
                token = strtok_r(NULL, "\r\n", &save_ptr);
            }
            i++;
        }
        if (fields[MP_ADDRESS] && fields[MP_KBYTES] && fields[MP_RSS] &&
            fields[MP_DIRTY] && fields[MP_MODE] && fields[MP_MAPPING])
        {
            DLT_LOG(ps_context,
                    DLT_LOG_INFO,
                    DLT_STRING(fields[MP_ADDRESS]),
                    DLT_STRING(fields[MP_KBYTES]),
                    DLT_STRING(fields[MP_RSS]),
                    DLT_STRING(fields[MP_DIRTY]),
                    DLT_STRING(fields[MP_MODE]),
                    DLT_STRING(fields[MP_MAPPING]));
        }
        for (i = 0; i < NO_OF_FIELDS_FOR_PMAP_COMMAND_OUTPUT; i++)
        {
            if (fields[i])
            {
                free(fields[i]);
            }
        }
    }
    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("END"), DLT_STRING("MMAP"));
    fclose(file_p);
    return retval;
}

/**
 * @brief callback process data collector specific injection
 *
 * @param service_id injection service id
 * @param data injection data
 *   Data Expected: <INFO_TYPE> <PID>
 * @param length
 * @return 0 on sucess -1 on syscall failure
 */
int dlt_monitor_process_info_injection_callback(uint32_t service_id,
                                void *data,
                                uint32_t length)
{
    int value = 0;
    char *inj_token = NULL;
    char *inj_info = NULL;
    char *endptr = NULL;
    int retval = 0;
    char *save_ptr = NULL;

    pr_verbose("%s: %s\n", NAME, __func__);

    if ((data == NULL) || (length <= 1))
    {
        return -1;
    }
    DLT_LOG(ps_context,
            DLT_LOG_VERBOSE,
            DLT_UINT32(service_id),
            DLT_STRING(data),
            DLT_UINT32(length));

    /* Expected data from injection
     * <INFO_TYPE> <PID>
     * eg: THREAD 120
     */
    inj_token = strtok_r((char*)data, " ", &save_ptr);
    inj_info = strdup(inj_token); /* storing type of info requested */
    inj_token = strtok_r(NULL, " \r\n", &save_ptr);
    if (inj_token != NULL)
    {
        value = strtol((char*)inj_token, &endptr, 10);/* getting PID/Interval */
    }
    else
    {
        DLT_LOG(ps_context, DLT_LOG_WARN, DLT_STRING("Unsupported Request"));
        free(inj_info);
        return -1;
    }
    DLT_LOG(ps_context, DLT_LOG_VERBOSE, DLT_UINT32(value), DLT_STRING(inj_info));

    if (strncmp(inj_info, "THREAD", strlen(inj_info)) == 0)
    {
        retval = dlt_monitor_process_thread_info(value);
    }
    else if (strncmp(inj_info, "USER", strlen(inj_info)) == 0)
    {
        retval = dlt_monitor_process_user_info(value);
    }
    else if (strncmp(inj_info, "FILES", strlen(inj_info)) == 0)
    {
        retval = dlt_monitor_process_open_files_info(value);
    }
    else if (strncmp(inj_info, "MMAP", strlen(inj_info)) == 0)
    {
        retval = dlt_monitor_process_memory_map(value);
    }
    else if (strncmp(inj_info, "INT", strlen(inj_info)) == 0)
    {
        g_update_interval = value;
        g_interval_changed = 1;
    }
    else
    {
        DLT_LOG(ps_context, DLT_LOG_WARN, DLT_STRING("Unsupported Request"));
    }
    free(inj_info);
    return retval;
}

/**
 * @brief to initiate process info collector
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
        DLT_REGISTER_CONTEXT(ps_context,
                             collector->ctid,
                             "Filesystem Data Collector");
        DLT_REGISTER_INJECTION_CALLBACK(ps_context,
                                        collector->injection_service_id,
                                        dlt_monitor_process_info_injection_callback);

        if (collector->private_data)
        {
            filepath = collector->private_data;
            ret = mkdir(filepath, S_IRUSR | S_IWUSR );
            if ((ret == -1) && (errno != EEXIST))
            {
                DLT_LOG(ps_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("Failed to create configured folder"),
                        DLT_STRING(filepath));
                return ret;
            }
        }

        collector->state = COLLECTOR_INITIALISED;
        DLT_LOG(ps_context,
                DLT_LOG_VERBOSE,
                DLT_STRING("Initialize"),
                DLT_STRING(collector->name));
    }
    return 0;
}

/**
 * @brief thread to collect and send process related info to dlt
 */
static void *dlt_process_monitor(void *collector)
{
    int ret = -1;
    int error_found = 1;
    char *str = NULL;
    DataCollector *data_collector = NULL;
    char buf[NAME_MAX] = {0};
    char filename[NAME_MAX] = {0};
    char *cmd_string = "\"%c|\"";
    char *cpu_string = "%cpu";
    char *mem_string = "%mem";

    pr_verbose("%s: %s\n", NAME, __func__);

    if (collector == NULL)
    {
        pthread_exit(&ret);
    }
    data_collector = (DataCollector *)collector;

    str = (char*)malloc(MAX_BUF_SIZE);
    if (str == NULL)
    {
        DLT_LOG(ps_context, DLT_LOG_ERROR, DLT_STRING("malloc failed."));
        data_collector->state = COLLECTOR_NOT_RUNNING;
        pthread_exit(&ret);
    }

    snprintf(buf, NAME_MAX, PROCESS_COMMAND, cmd_string, cpu_string, mem_string, filepath);
    snprintf(filename, NAME_MAX, "%s/proc.txt",filepath);

    while (g_run_loop)
    {
        if (g_interval_changed)
        {
            data_collector->update_interval = g_update_interval;
            g_interval_changed = 0;
        }
         /* ps output is formatted in a way which we can tokenize
          * '|' charecter is added for process name to avoid missing entries with space
          * cmd is added as last parameter to avoid unneccessary handling for
          * process arguments */
        ret = system(buf);
        if (ret != 0)
        {
            DLT_LOG(ps_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("ps syscal failure"),
                    DLT_INT(ret));
            pthread_exit(&ret);
        }

        FILE *file_p = fopen(filename, "r");

        if (!file_p)
        {
            DLT_LOG(ps_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("dlt-process-monitor, abnormal exit status."),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("BEG"));
            while (fgets(str, MAX_BUF_SIZE, file_p) != NULL)
            {
                char *token = NULL;
                char s[4] = " \r\n";
                /* array of pointers to hold the fields of each line of ps output */
                char *fields[NO_OF_FIELDS_FOR_PS_COMMAND_OUTPUT] = {NULL};
                int i = 0;
                char *save_ptr = NULL;
                /* Expected output
                 * COMMAND     |USER     STAT    VSZ   RSS    VSZ    SZ   RSS %CPU CPU     ELAPSED  NI   PID LABEL %MEM WCHAN             F PRI  PPID CMD
                 * systemd     |root     Ss    13312  2888  13312  3328  2888  0.0   -    02:02:48   0     1 -      0.1 SyS_epoll_wait    4  19     0 /sbin/init
                 * kthreadd    |root     S         0     0      0     0     0  0.0   -    02:02:48   0     2 -      0.0 kthreadd          1  19     0 [kthreadd]
                 * ksoftirqd/0 |root     S         0     0      0     0     0  0.0   -    02:02:48   0     3 -      0.0 smpboot_thread_fn 1  19     2 [ksoftirqd/0]
                 */
                /* get the first token */
                token = strtok_r(str, "|", &save_ptr);
                /* walk through other tokens */
                while ((token != NULL) && (i < NO_OF_FIELDS_FOR_PS_COMMAND_OUTPUT))
                {
                    fields[i] = strdup(token);
                    if (i < PP_CMDLINE-1)
                    {
                        token = strtok_r(NULL, s, &save_ptr);
                    }
                    else
                    {
                        token = strtok_r(NULL, "\r\n", &save_ptr);
                    }
                    i++;
                }
                if (fields[PP_PROC_NAME] && fields[PP_USER] && fields[PP_STATUS] &&
                    fields[PP_VMEM] && fields[PP_RMEM] && fields[PP_WMEM] &&
                    fields[PP_SMEM] && fields[PP_XSMEM] && fields[PP_PCPU] &&
                    fields[PP_CPU_TIME] && fields[PP_ELAPSED] && fields[PP_NICE] &&
                    fields[PP_ID] && fields[PP_SEC_CONTEXT] && fields[PP_MEM] &&
                    fields[PP_WCHAN] && fields[PP_CGRP] && fields[PP_PRIORITY] &&
                    fields[PP_PPID] && fields[PP_CMDLINE])
                {
                    DLT_LOG(ps_context,
                            DLT_LOG_INFO,
                            DLT_STRING(fields[PP_PROC_NAME]),
                            DLT_STRING(fields[PP_USER]),
                            DLT_STRING(fields[PP_STATUS]),
                            DLT_STRING(fields[PP_VMEM]),
                            DLT_STRING(fields[PP_RMEM]),
                            DLT_STRING("N/A"),/* WMEM-not implemented */
                            DLT_STRING(fields[PP_SMEM]),
                            DLT_STRING("N/A"),/* XSMEM-not implemented */
                            DLT_STRING(fields[PP_PCPU]),
                            DLT_STRING(fields[PP_CPU_TIME]),
                            DLT_STRING(fields[PP_ELAPSED]),
                            DLT_STRING(fields[PP_NICE]),
                            DLT_STRING(fields[PP_ID]),
                            DLT_STRING(fields[PP_CMDLINE]),
                            DLT_STRING(fields[PP_SEC_CONTEXT]),
                            DLT_STRING(fields[PP_MEM]),
                            DLT_STRING(fields[PP_WCHAN]),
                            DLT_STRING("N/A"),/* CGRP-not implemented */
                            DLT_STRING(fields[PP_PRIORITY]),
                            DLT_STRING(fields[PP_PPID]));
                    error_found = 0;
                }
                else
                {
                    /* setting error flag if data is not as expected which in
                     * turn will exit this thread */
                    DLT_LOG(ps_context,
                            DLT_LOG_ERROR,
                            DLT_STRING("ps command output format mismatch"));
                    error_found = 1;
                }
                for (i = 0; i < NO_OF_FIELDS_FOR_PS_COMMAND_OUTPUT; i++)
                {
                    if (fields[i])
                    {
                        free(fields[i]);
                    }
                }
            }
            fclose(file_p);
            DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("END"));
        }

        if (error_found)
        {
            free(str);
            data_collector->state = COLLECTOR_NOT_RUNNING;
            pthread_exit(&ret);
        }
        usleep(g_update_interval * MILLISEC_PER_SECOND);
    }
    free(str);
    data_collector->state = COLLECTOR_NOT_RUNNING;
    return 0;
}

/**
 * @brief to start process data collection to DLT
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
    if (pthread_create(&process_monitor_thread,
                       NULL,
                       dlt_process_monitor,
                       collector) != 0)
    {
        DLT_LOG(ps_context,
                DLT_LOG_ERROR,
                DLT_STRING("Cannot create thread to communicate with DLT daemon"));
        collector->state = COLLECTOR_INITIALISED;
        return -1;
    }
    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("COLLECT OK"));
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
    *list = optional_configuration_entries;
    return 0;
}

/**
 * @brief to cleanup process info collector
 *
 * @param collector data collector structure
 * @return 0 on success, -1 otherwise
 */
int cleanup(DataCollector *collector)
{
    int ret = -1;

    pr_verbose("%s: %s\n", NAME, __func__);

    DLT_LOG(ps_context,
            DLT_LOG_DEBUG,
            DLT_STRING("cleanup called"));
    if (collector == NULL)
    {
        return -1;
    }
    if (collector->state == COLLECTOR_RUNNING)
    {
        g_run_loop = 0;
        ret = pthread_cancel(process_monitor_thread);
        if (ret)
        {
            DLT_LOG(ps_context,
                    DLT_LOG_ERROR,
                    DLT_STRING("unable to cancel the thread"),
                    DLT_STRING(strerror(errno)));
        }
        else
        {
            ret = pthread_join(process_monitor_thread, NULL);
            if (ret)
            {
                DLT_LOG(ps_context,
                        DLT_LOG_ERROR,
                        DLT_STRING("unable to join the thread"),
                        DLT_STRING(strerror(errno)));
            }
        }
    }

    DLT_LOG(ps_context, DLT_LOG_INFO, DLT_STRING("CLEANUP OK"));
    collector->state = COLLECTOR_NOT_RUNNING;

    DLT_UNREGISTER_CONTEXT(ps_context);

    return 0;
}
