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
 * \file dlt-monitor-process.h
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#ifndef DLT_MONITOR_PROCESS_H_
#define DLT_MONITOR_PROCESS_H_

#define NO_OF_FIELDS_FOR_PS_COMMAND_OUTPUT 20  /* Max entries for process info */
#define NO_OF_FIELDS_FOR_TOP_COMMAND_OUTPUT 12 /* Max entries for thread info */
#define NO_OF_FIELDS_FOR_USER_COMMAND_OUTPUT 7 /* Max entries for user info */
#define NO_OF_FIELDS_FOR_LSOF_COMMAND_OUTPUT 3 /* Max entries for open files info */
#define NO_OF_FIELDS_FOR_PMAP_COMMAND_OUTPUT 6 /* Max entries for memmap info */
#define MILLISEC_PER_SECOND 1000
#define NAME "PROC"
#define TOP_COMMAND "top -H -p %u -n 1 -b > %s/thread.txt"
#define USER_COMMAND "ps -p %u -o user,suser,ruser,euser,fsuser,comm,pid > %s/usr.txt"
#define LSOF_COMMAND "lsof | grep -w %u > %s/files.txt"
#define MMAP_COMMAND "pmap -x %u > %s/mmap.txt"
#define PROCESS_COMMAND "ps -e -o %s -o user,stat,vsz,rss,vsz,sz,rss,%s,cpu,etime,ni,pid,label:5,%s,wchan:45,f,pri,ppid,cmd > %s/proc.txt"

    typedef enum {
        PP_PROC_NAME = 0,
        PP_USER,
        PP_STATUS,
        PP_VMEM,
        PP_RMEM,
        PP_WMEM,
        PP_SMEM,
        PP_XSMEM,
        PP_PCPU,
        PP_CPU_TIME,
        PP_ELAPSED,
        PP_NICE,
        PP_ID,
        PP_SEC_CONTEXT,
        PP_MEM,
        PP_WCHAN,
        PP_CGRP,
        PP_PRIORITY,
        PP_PPID,
        PP_CMDLINE, /* this should be the penultimate entry */
        PP_MAX_ENTRIES = NO_OF_FIELDS_FOR_PS_COMMAND_OUTPUT
    } ProcessParams;
    typedef enum {
        TP_PID = 0,
        TP_USER,
        TP_PRIORITY,
        TP_NICE,
        TP_VMEM,
        TP_RMEM,
        TP_SMEM,
        TP_STATE,
        TP_PCPU,
        TP_MEMORY,
        TP_TIME,
        TP_PROC_NAME,
        TP_MAX_ENTRIES = NO_OF_FIELDS_FOR_TOP_COMMAND_OUTPUT
    } ThreadParams;
    typedef enum {
        UP_USER = 0,
        UP_SUSER,
        UP_RUSER,
        UP_EUSER,
        UP_FSUSER,
        UP_PROC_NAME,
        UP_PID,
        UP_MAX_ENTRIES = NO_OF_FIELDS_FOR_USER_COMMAND_OUTPUT
    } UserParams;
    typedef enum {
        FP_PID = 0,
        FP_PROC_NAME,
        FP_FILE_NAME,
        FP_MAX_ENTRIES = NO_OF_FIELDS_FOR_LSOF_COMMAND_OUTPUT
    } FileParams;
    typedef enum {
        MP_ADDRESS = 0,
        MP_KBYTES,
        MP_RSS,
        MP_DIRTY,
        MP_MODE,
        MP_MAPPING,
        MP_MAX_ENTRIES = NO_OF_FIELDS_FOR_PMAP_COMMAND_OUTPUT
    } Mmap_Params;

#endif
