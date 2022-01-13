/**
 * @licence app begin@
 * Copyright (C) 2013 - 2015  Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * DLT Get HP log functionality source file.
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author : Syed Hameed shameed@jp.adit-jv.com
 *
 * \file: dlt-get-hplog.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <ctype.h>

#include "dlt.h"
#include "dlt_client.h"
#include "dlt_user_hp.h"
#include "dlt-get-hplog.h"
#include "dlt_protocol.h"
#include "dlt_client.h"
#include "dlt-control-common.h"
#include "dlt_daemon_connection_types.h"

#define DLT_GET_HPLOG_DBG_LOG(level, format, ...)   printf(format, ##__VA_ARGS__);

DltClient dltclient;
static char conv_list_file[DLT_MAX_FILEPATH_LEN+1];
static char output_file_path[DLT_MAX_FILEPATH_LEN+1];
static char tmp_file_path[DLT_MAX_FILEPATH_LEN+1];
static DltGetHplogConvList ConvList;
static DltGetHplogInjectionInfoList InjectionList;
static char opt1 = DLT_GET_HPLOG_OPT1_NONE;
static char opt2 = DLT_GET_HPLOG_OPT2_NONE;
static char opt3 = DLT_GET_HPLOG_OPT3_NONE;
unsigned int g_send_injection_msg = 0;

/*--------------------------------------------------------------------------------------------*/
/* common modules                                                                             */
/*--------------------------------------------------------------------------------------------*/
void usage()
{
    printf(
            "-----------------------------------------------------------------------------------------------\n");
    printf("Usage: dlt-get-hplog [option1] [option2] [option3]\n");
    printf(" Get hp trace log and Convert to DLT log.\n");
    printf(
            "-----------------------------------------------------------------------------------------------\n");
    printf(" Option1:\n");
    printf(
            "   -i                 Get snap-shot HP trace about Option2 specific.\n");
    printf(
            "   -a                 Get immediate HP trace about Option2 specific.\n");
    printf("   -p <Pid>           Get immediate HP trace about <Pid>\n");
    printf("   -c                 Convert HP trace about Option2 specific.\n");
    printf(" Option2:\n");
    printf(
            "   -t <Apid>,<Ctid>   target DLT-ID specific. don't use if opt1 is '-p'.\n");
    printf(
            "   -f <ListFilepath>  target DLT-ID specific list file path. don't use if opt1 is '-p' or '-c'.\n");
    printf(" Option3:\n");
    printf("   -o <OutFilepath>   Output file path.\n");
    printf(
            "-----------------------------------------------------------------------------------------------\n");
}

char *dlt_get_ring_buffer_get_file_name(char *Filepath)
{
    char *p;

    for (p = strchr(Filepath, '\0'); p >= Filepath; p--)
    {
        if (*p == '/')
        {
            return (p + 1);
        }
    }
    return Filepath;
}

int dlt_get_ring_buffer_check_extension(char *filename, char *extension)
{
    int ret = 0;
    int body_len;

    if ((filename ==  NULL) || (extension == NULL))
        return 1;

    body_len = strlen(filename) - strlen(extension);
    if (body_len < 0)
    {
        ret = 1;
    }
    else
    {
        filename += body_len;
        if (strcmp(filename, extension) != 0)
        {
            ret = 1;
        }
    }
    return ret;
}

int dlt_get_ring_buffer_parse_filename(char *filename, char *ctid, char *apid,
        int *pid)
{
    int ret = 0;
    char *rp;
    uint8_t count;
    char pid_ascii[DLT_EXT_PID_MAX_LEN + 1];

    /* dlt_ctid_apid_pid.xxx */
    /* skip to ctid */
    rp = filename + sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX) - 1
            + sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1;

    /* get ctid */
    for (count = 0; count < DLT_ID_SIZE; count++)
    {
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);
        if (ret == 0)
        {
            break;
        }
        ctid[count] = *rp;
        rp++;
    }
    if (count == DLT_ID_SIZE)
    {
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);
        if (ret != 0)
        {
            /* CTID length invalid */
            return 1;
        }
    }
    ctid[count] = 0;
    rp += (sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);

    /* get apid */
    for (count = 0; count < DLT_ID_SIZE; count++)
    {
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);
        if (ret == 0)
        {
            break;
        }
        apid[count] = *rp;
        rp++;
    }
    if (count == DLT_ID_SIZE)
    {
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);
        if (ret != 0)
        {
            /* APID length invalid */
            return 1;
        }
    }
    apid[count] = 0;
    rp += (sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);

    /* get pid */
    for (count = 0; count < DLT_EXT_PID_MAX_LEN; count++)
    {
        ret = memcmp(rp, DLT_EXT_CTID_LOG_PERIOD,
                sizeof(DLT_EXT_CTID_LOG_PERIOD) - 1);
        if (ret == 0)
        {
            break;
        }
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_BKNUM_DELI,
                sizeof(DLT_EXT_CTID_LOG_FILE_BKNUM_DELI) - 1);
        if (ret == 0)
        {
            /* this file is backup */
            return 1;
        }
        pid_ascii[count] = *rp;
        rp++;
    }
    if (count == DLT_EXT_PID_MAX_LEN)
    {
        ret = memcmp(rp, DLT_EXT_CTID_LOG_PERIOD,
                sizeof(DLT_EXT_CTID_LOG_PERIOD) - 1);
        if (ret != 0)
        {
            /* PID length invalid or backup file */
            return 1;
        }
    }
    pid_ascii[count] = 0;
    *pid = atoi(pid_ascii);

    return 0;
}


/*--------------------------------------------------------------------------------------------*/
/* file convert modules                                                                       */
/*--------------------------------------------------------------------------------------------*/
uint16_t write_from_buf(int fd_to, void **rp, int len, void *rp_head,
        uint32_t size)
{
    int remain;
    uint16_t value = 0;

    if (len != 0)
    {
        remain = (int) (size - (*rp - rp_head));

        /* cast first 2byte value to uint16_t */
        if (remain == 1)
        {
            value = ((((*(char *) rp_head) << 8) & 0xFF00)
                    | ((*(char *) (*rp)) & 0x00FF));
        }
        else
        {
            value = ((((*(char *) ((*rp) + 1)) << 8) & 0xFF00)
                    | ((*(char *) (*rp)) & 0x00FF));
        }

        /* write cyclic read data, and rp go ahead */
        if (len > remain)
        {
            write(fd_to, *rp, remain);
            write(fd_to, rp_head, (len - remain));
            *rp = rp_head + (len - remain);
        }
        else
        {
            write(fd_to, *rp, len);
            *rp += len;
        }
    }
    return value;
}

int dlt_get_ring_buffer_check_Storageheader(char *rp_head, uint32_t size,
        char *rp)
{
    int remain;
    int ret = 1;
    char DltStorageheader[4] = { 0x44, 0x4C, 0x54, 0x01 }; /* DLT^A */

    remain = size - (int) (rp - rp_head);
    /* search for Storage Header first word */
    if (*rp == DltStorageheader[0])
    {
        switch (remain)
        {
        case 3:
            if ((*(rp + 1) == DltStorageheader[1])
                    && (*(rp + 2) == DltStorageheader[2])
                    && (*(rp_head) == DltStorageheader[3]))
            {
                ret = 0;
            }
            break;
        case 2:
            if ((*(rp + 1) == DltStorageheader[1])
                    && (*(rp_head) == DltStorageheader[2])
                    && (*(rp_head + 1) == DltStorageheader[3]))
            {
                ret = 0;
            }
            break;
        case 1:
            if ((*(rp_head) == DltStorageheader[1])
                    && (*(rp_head + 1) == DltStorageheader[2])
                    && (*(rp_head + 2) == DltStorageheader[3]))
            {
                ret = 0;
            }
            break;
        default:
            ret = memcmp(rp, DltStorageheader, DLT_ID_SIZE);
            break;
        }
    }
    return ret;
}

uint8_t dlt_get_ring_buffer_convert_log_write(void *rp_head,
        DltExtBuffHeader *pBuffHead, int fd_to, void *rp)
{
    int remain;
    uint16_t header_len;
    uint16_t payload_len;
    uint16_t ret;
    uint8_t mcnt = 0;
    uint32_t size;

    size = pBuffHead->size - sizeof(DltExtBuffHeader);

    do
    {
       remain = size - (int)(rp - rp_head);
       if(remain <= 0)
       {
           rp = rp_head;
           remain = size;
       }
        /* check Storageheader */
        ret = dlt_get_ring_buffer_check_Storageheader(rp_head, size, rp);
        if (ret)
        {
            /* next trace is not correct, finish to convert */
            DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                    " next trace have no Storage Header.. convert stop!\n");
            break;
        }

        /* make DltStorageHeader */
        ret = write_from_buf(fd_to, &rp, DLT_ID_SIZE, rp_head, size); /* pattern */
        ret = write_from_buf(fd_to, &rp, sizeof(int32_t), rp_head, size); /* seconds */
        ret = write_from_buf(fd_to, &rp, sizeof(int32_t), rp_head, size); /* microseconds */
        write(fd_to, pBuffHead->ecuid, DLT_ID_SIZE); /* ecu */

        /* make DltStandardHeader */
        write(fd_to, &pBuffHead->htyp, sizeof(uint8_t)); /* hytp */
        write(fd_to, &mcnt, sizeof(uint8_t)); /* mcnt */
        ret = write_from_buf(fd_to, &rp, sizeof(uint16_t), rp_head, size); /* len */

        /* make DltStandardHeaderExtra */
        if (DLT_IS_HTYP_WEID(pBuffHead->htyp))
        {
            write(fd_to, pBuffHead->ecuid, DLT_ID_SIZE); /* ecu */
        }
        if (DLT_IS_HTYP_WSID(pBuffHead->htyp))
        {
            ret = write_from_buf(fd_to, &rp, sizeof(uint32_t), rp_head, size); /* seid */
        }
        if (DLT_IS_HTYP_WTMS(pBuffHead->htyp))
        {
            ret = write_from_buf(fd_to, &rp, sizeof(uint32_t), rp_head, size); /* tsmp */
        }

        /* make DltExtenderHeader */
        write(fd_to, &pBuffHead->msin, sizeof(uint8_t)); /* msin */
        write(fd_to, &pBuffHead->noar, sizeof(uint8_t)); /* noar */
        write(fd_to, pBuffHead->apid, DLT_ID_SIZE); /* apid */
        write(fd_to, pBuffHead->ctid, DLT_ID_SIZE); /* ctid */

        /* make UserLog */
        write(fd_to, &pBuffHead->type_info_header,
                sizeof(uint32_t)); /* type_info */
        header_len = write_from_buf(fd_to, &rp, sizeof(uint16_t), rp_head,
                size); /* header len */
        if (header_len > DLT_EXT_BUF_LOGMAX_HP1)
        {
            /* rollback file offset to head of this trace */
            lseek(fd_to, -(DLT_EXT_SEEK_LEN_TO_HEADER_LEN), SEEK_CUR);
            break;
        }
        ret = write_from_buf(fd_to, &rp, header_len, rp_head, size); /* header data */
        write(fd_to, &pBuffHead->type_info_payload,
                sizeof(uint32_t)); /* type_info */
        payload_len = write_from_buf(fd_to, &rp, sizeof(uint16_t), rp_head,
                size); /* payload len */
        if (payload_len > DLT_EXT_BUF_LOGMAX_HP1)
        {
            /* rollback file offset to head of this trace */
            lseek(fd_to, -(DLT_EXT_SEEK_LEN_TO_PAYLOAD_LEN(header_len)),
                    SEEK_CUR);
            break;
        }
        ret = write_from_buf(fd_to, &rp, payload_len, rp_head, size); /* payload data */

        mcnt++;
    }
    while (rp != (void *) (rp_head + pBuffHead->write_count));

    return mcnt;
}

int dlt_get_ring_buffer_search_Storageheader(char *rp_head, uint32_t size,
        char *rp, char **record_head_p)
{
    uint32_t count;
    int remain;
    int ret;

    remain = size - (int)(rp - rp_head);

    for(count=0; count<size; count++)
    {
        if(remain <= 0)
        {
           rp = rp_head;
           remain = size;
        }
        /* check Storageheader */
        ret = dlt_get_ring_buffer_check_Storageheader(rp_head, size, rp);
        if (ret == 0)
        {
            /* Storage Header detect! */
            *record_head_p = rp;
            return 0;
        }
        rp++;
        remain--;
    }
    /* Storage Header lost.. */
    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, " Storage Header lost..\n");
    return -1;
}

uint8_t dlt_get_ring_buffer_convert_log(int fd_to, char *filepath)
{
    int fd_from;
    void *addr = NULL;
    void *rp_head = NULL;
    void *rp;
    char *record_head_p;
    DltExtBuffHeader BuffHeader;
    int ret = 1;
    ssize_t read_ret;
    uint8_t conv_cnt = 0;

    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, " convert %s start\n", filepath);

    sleep(DLT_INJECTION_WAIT);

    /* mmap convert file */
    fd_from = open(filepath, O_RDONLY);
    if (fd_from != -1)
    {
        /* Buffer Header read */
        read_ret = read(fd_from, &BuffHeader, sizeof(BuffHeader));
        /* mmap */
        addr = mmap(NULL, BuffHeader.size, PROT_READ, MAP_SHARED, fd_from, 0);
        if ((read_ret == -1) || (addr != MAP_FAILED))
        {
            ret = 0;
            rp_head = addr + sizeof(BuffHeader);
        }
        close(fd_from);
    }
    if (ret)
    {
        /* open or mmap convert file failed */
        DLT_GET_HPLOG_DBG_LOG(HP_LOG_ERR, "  -> open or mmap failed..\n");
        return conv_cnt;
    }

    /* search latest record head */
    rp = (char *) (rp_head + BuffHeader.write_count);
    ret = dlt_get_ring_buffer_search_Storageheader((char *) rp_head,
            (BuffHeader.size - sizeof(BuffHeader)), (char *) rp,
            &record_head_p);
    if (ret == 0)
    {
        /* convert record to DLT log format */
        conv_cnt = dlt_get_ring_buffer_convert_log_write(rp_head, &BuffHeader,
                fd_to, record_head_p);
    }
    else
    {
        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "  -> Storage Header lost..\n");
    }

    /* delete convert file */
    munmap(addr, BuffHeader.size);
    unlink(filepath);

    return conv_cnt;
}

int dlt_get_ring_buffer_do_convert(void)
{
    DIR *dir;
    struct dirent *dent;
    char log_file_path[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1];
    struct stat sb;
    int ret;
    int count;
    char apid[DLT_ID_SIZE + 1];
    char ctid[DLT_ID_SIZE + 1];
    int pid;
    int injection_retry = 0;
    int fd_to;
    char target_apid_wild;
    char target_ctid_wild;
    int target_detect;
    int no_more_convert;
    int conv_num;
    int all_conv_num = 0;

    /* create tmp file */
    fd_to = open(tmp_file_path, O_CREAT | O_RDWR | O_TRUNC, S_IRWXU | S_IRWXO);
    if (fd_to == -1)
    {
        /* create tmp file failed */
        return -1;
    }

    for (count = 0; count < ConvList.list_count; count++)
    {
        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                              " target AP:%s CT:%s\n",
                              ConvList.apid[count],
                              ConvList.ctid[count]);
        /* target id wildcard check*/
        if (!strncmp(ConvList.apid[count], DLT_EXT_WILD_CARD, DLT_ID_SIZE))
        {
            target_apid_wild = 1;
        }
        else
        {
            target_apid_wild = 0;
        }
        if (!strncmp(ConvList.ctid[count], DLT_EXT_WILD_CARD, DLT_ID_SIZE))
        {
            target_ctid_wild = 1;
        }
        else
        {
            target_ctid_wild = 0;
        }
        conv_num = 0;

        dir = opendir(DLT_EXT_CTID_LOG_DIRECTORY);
        if (dir == NULL)
        {
            DLT_GET_HPLOG_DBG_LOG(HP_LOG_ERR, "opendir failed..\n");
            return -1;
        }
        while ((dent = readdir(dir)) != NULL )
        {
            no_more_convert = 0;

            snprintf(log_file_path, DLT_EXT_CTID_LOG_FILEPATH_LEN + 1, "%s%s",
                    DLT_EXT_CTID_LOG_DIRECTORY, dent->d_name);
            if ((stat(log_file_path, &sb) < 0) && (errno != ENOENT))
            {
                break;
            }
            if (!S_ISREG(sb.st_mode))
            {
                continue;
            }

            /* check extention and backup*/
            if (opt1 == DLT_GET_HPLOG_OPT1_INJECTION)
            {
                ret = dlt_get_ring_buffer_check_extension(dent->d_name,
                        DLT_EXT_CTID_FIX_FILE_EXTENSION);
            }
            else
            {
                ret = dlt_get_ring_buffer_check_extension(dent->d_name,
                        DLT_EXT_CTID_LOG_FILE_EXTENSION);
            }
            if (ret == 1)
            {
                continue;
            }
            ret = dlt_get_ring_buffer_parse_filename(dent->d_name, ctid, apid,
                    &pid);
            if (ret == 1)
            {
                continue;
            }

            DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                  " target file detect!%s\n",
                                  dent->d_name);

            if (opt1 == DLT_GET_HPLOG_OPT1_KILL_PID)
            {
                /* matching for target PID */
                if (ConvList.pid_value[count] == pid)
                {
                    target_detect = 1;
                }
            }
            else
            {
                /* matching for target APID and CTID */
                if (target_apid_wild)
                {
                    if (target_ctid_wild)
                    {
                        target_detect = 1;
                    }
                    else
                    {
                        if ((!strncmp(ConvList.ctid[count], ctid, DLT_ID_SIZE)))
                        {
                            target_detect = 1;
                        }
                    }
                }
                else
                {
                    if ((!strncmp(ConvList.apid[count], apid, DLT_ID_SIZE)))
                    {
                        if (target_ctid_wild)
                        {
                            target_detect = 1;
                        }
                        else
                        {
                            if ((!strncmp(ConvList.ctid[count], ctid,
                                    DLT_ID_SIZE)))
                            {
                                target_detect = 1;
                                no_more_convert = 1;
                            }
                        }
                    }
                }
            }
            if (target_detect)
            {
                target_detect = 0;
                injection_retry = 0;
                conv_num++;
                /* convert it to dlt log */
                all_conv_num += dlt_get_ring_buffer_convert_log(fd_to,
                        log_file_path);
                if (no_more_convert)
                {
                    no_more_convert = 0;
                    break;
                }
            }
        }
        closedir(dir);

        /* only INJECTION */
        if (opt1 == DLT_GET_HPLOG_OPT1_INJECTION)
        {
            if ((!conv_num) || (target_apid_wild || target_ctid_wild))
            {
                if (injection_retry)
                {
                    if (target_apid_wild || target_ctid_wild)
                    {
                        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                "APID:%s CTID:%s there is no convert file remain, convert finish\n",
                                ConvList.apid[count],
                                ConvList.ctid[count]);
                    }
                    else
                    {
                        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                "APID:%s CTID:%s trace convert failed..\n",
                                ConvList.apid[count],
                                ConvList.ctid[count]);
                    }
                    injection_retry = 0;
                }
                else
                {
                    if (target_apid_wild || target_ctid_wild)
                    {
                        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                "APID:%s CTID:%s wait for a while, because trace file may be creating now\n",
                                ConvList.apid[count],
                                ConvList.ctid[count]);
                    }
                    else
                    {
                        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                "APID:%s CTID:%s trace file is not fix, wait for a while\n",
                                ConvList.apid[count],
                                ConvList.ctid[count]);
                    }
                    sleep(DLT_INJECTION_WAIT);
                    count--;
                    injection_retry = 1;
                }
            }
        }
    }

    /* close tmp dlt file */
    close(fd_to);

    if (all_conv_num)
    {
        rename(tmp_file_path, output_file_path);
    }
    else
    {
        unlink(tmp_file_path);
    }

    return all_conv_num;
}


/*--------------------------------------------------------------------------------------------*/
/* KILL modules                                                                               */
/*--------------------------------------------------------------------------------------------*/
int dlt_get_ring_buffer_kill(void)
{
    int count;
    DIR *dir;
    struct dirent *dent;
    char log_file_path[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1];
    struct stat sb;
    int ret;
    char apid[DLT_ID_SIZE + 1];
    char ctid[DLT_ID_SIZE + 1];
    int pid;
    char target_apid_wild;
    char target_ctid_wild;

    for (count = 0; count < ConvList.list_count; count++)
    {
        /* target id wildcard check*/
        if (!strncmp(ConvList.apid[count], DLT_EXT_WILD_CARD, DLT_ID_SIZE))
        {
            target_apid_wild = 1;
        }
        else
        {
            target_apid_wild = 0;
        }
        if (!strncmp(ConvList.ctid[count], DLT_EXT_WILD_CARD, DLT_ID_SIZE))
        {
            target_ctid_wild = 1;
        }
        else
        {
            target_ctid_wild = 0;
        }

        dir = opendir(DLT_EXT_CTID_LOG_DIRECTORY);
        if (dir == NULL)
        {
            return -1;
        }

        while ((dent = readdir(dir)) != NULL )
        {
            snprintf(log_file_path, DLT_EXT_CTID_LOG_FILEPATH_LEN + 1, "%s%s",
                    DLT_EXT_CTID_LOG_DIRECTORY, dent->d_name);
            if ((stat(log_file_path, &sb) < 0) && (errno != ENOENT))
            {
                break;
            }
            if (!S_ISREG(sb.st_mode))
            {
                continue;
            }

            /* check extention(if not ".log", then check next file) */
            ret = dlt_get_ring_buffer_check_extension(dent->d_name,
                    DLT_EXT_CTID_LOG_FILE_EXTENSION);
            if (ret == 1)
            {
                continue;
            }

            /* check matching for target CTID and APID */
            ret = dlt_get_ring_buffer_parse_filename(dent->d_name, ctid, apid,
                    &pid);
            if (ret == 1)
            {
                continue;
            }
            if (opt1 == DLT_GET_HPLOG_OPT1_KILL_PID)
            {
                if (ConvList.pid_value[count] == pid)
                {
                    ret = kill(pid, SIGKILL);
                }
            }
            else
            {
                if (target_apid_wild)
                {
                    if (target_ctid_wild)
                    {
                        ret = kill(pid, SIGKILL);
                    }
                    else
                    {
                        if ((!strncmp(ConvList.ctid[count], ctid, DLT_ID_SIZE)))
                        {
                            ret = kill(pid, SIGKILL);
                        }
                    }
                }
                else
                {
                    if ((!strncmp(ConvList.apid[count], apid, DLT_ID_SIZE)))
                    {
                        if (target_ctid_wild)
                        {
                            ret = kill(pid, SIGKILL);
                        }
                        else
                        {
                            if ((!strncmp(ConvList.ctid[count], ctid,
                                    DLT_ID_SIZE)))
                            {
                                ret = kill(pid, SIGKILL);
                                break;
                            }
                        }
                    }
                }
            }
        }
        closedir(dir);
    }
    return 0;
}


/*--------------------------------------------------------------------------------------------*/
/* INJECTTION modules                                                                         */
/*--------------------------------------------------------------------------------------------*/

/**
 * @brief Prepare message body to be send to DLT Daemon
 *
 * @return Pointer ot DltControlMsgBody, NULL otherwise
 */
DltControlMsgBody *dlt_msg_injection_prepare_message_body(uint32_t serviceid, uint8_t *buffer, uint32_t size)
{
    int offset = 0;
    DltControlMsgBody *mb = calloc(1, sizeof(DltControlMsgBody));
    if (mb == NULL)
    {
        return NULL;
    }

    mb->data = calloc(1, sizeof(uint32_t) + sizeof(uint32_t) + size);
    if (mb->data == NULL)
    {
        free(mb);
        return NULL;
    }


    memcpy(mb->data, &serviceid, sizeof(serviceid));
    offset+=sizeof(uint32_t);

    memcpy(mb->data + offset, &size, sizeof(size));
    offset+=sizeof(uint32_t);

    memcpy(mb->data + offset, buffer, size);

    mb->size = sizeof(uint32_t) + sizeof(uint32_t) + size;

    return mb;
}

void dlt_get_ring_buffer_injection_fire(void)
{
    int ret = 0;
    int info_count;

    /* prepare GetlogInfo message body */
    DltControlMsgBody *msg_body = NULL;

    for (info_count = 0; info_count < InjectionList.info_list_count;
                info_count++)
    {
        if (InjectionList.info[info_count].used == 1)
        {
            msg_body = dlt_msg_injection_prepare_message_body(DLT_EX_INJECTION_CODE,
                           (uint8_t *) InjectionList.info[info_count].ctid[0],
                           (DLT_EXT_CTID_MAX * (DLT_ID_SIZE + 1)));
            if (msg_body == NULL)
            {
                pr_error("Failed to prepare injection message body \n");
                return;
            }

            ret = dlt_control_send_injection_message(msg_body,
                                            InjectionList.info[info_count].apid,
                                            DLT_CT_EXT_CB,
                                            get_timeout());
            if (ret != 0)
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "INJECTION fire failed..\n");
            }

            free(msg_body->data);
            free(msg_body);
        }
    }

    return;
}

void dlt_get_ring_buffer_set_injection_info(char *apid, char *ctid)
{
    int ret;
    int info_count;
    int index;

    /* check same apid still entry */
    for (info_count = 0; info_count < DLT_CONV_LIST_MAX_COUNT; info_count++)
    {
        if (InjectionList.info[info_count].used == 1)
        {
            ret = strncmp(InjectionList.info[info_count].apid, apid,
                    DLT_ID_SIZE + 1);
            if (ret == 0)
            {
                /* apid still entry, then ctid add */
                index = InjectionList.info[info_count].ctid_count;
                if (index < DLT_EXT_CTID_MAX)
                {
                    strncpy(InjectionList.info[info_count].ctid[index], ctid,
                            DLT_ID_SIZE + 1);
                    InjectionList.info[info_count].ctid_count++;
                    return;
                }
                else
                {
                    /* ctid still entry full */
                    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                            "ctid still entry full at injection info..\n");
                    return;
                }
            }
        }
    }

    /* try to entry this apid new */
    for (info_count = 0; info_count < DLT_CONV_LIST_MAX_COUNT; info_count++)
    {
        if (InjectionList.info[info_count].used != 1)
        {
            /* entry this apid and ctid */
            InjectionList.info[info_count].used = 1;
            strncpy(InjectionList.info[info_count].apid, apid, DLT_ID_SIZE + 1);
            memset(InjectionList.info[info_count].ctid[0], 0,
                    (DLT_EXT_CTID_MAX * (DLT_ID_SIZE + 1)));
            strncpy(InjectionList.info[info_count].ctid[0], ctid,
                    DLT_ID_SIZE + 1);
            InjectionList.info[info_count].ctid_count = 1;
            /* count up injection num */
            InjectionList.info_list_count++;
            return;
        }
    }

    /* apid still entry full */
    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
            "apid still entry full at injection info..\n");
    return;
}

void dlt_get_ring_buffer_conv_ascii_to_id(char *rp, int *rp_count, char *wp,
        int len)
{
    char number16[6]={0};
    char *endptr;
    int count;

    if ((rp == NULL) || (rp_count == NULL) || (wp == NULL))
    {
        return;
    }
        /* ------------------------------------------------------
           from: [72 65 6d 6f ] -> to: [0x72,0x65,0x6d,0x6f,0x00]
           ------------------------------------------------------ */
        number16[0] = '+';
        number16[1] = '0';
        number16[2] = 'x';
        for (count = 0; count < (len - 1); count++)
        {
            number16[3] = *(rp + *rp_count + 0);
            number16[4] = *(rp + *rp_count + 1);
            *(wp + count) = strtol(number16, &endptr, 16);
            *rp_count += 3;
        }
        *(wp + count) = 0;
        return;
}

uint16_t dlt_get_ring_buffer_conv_ascii_to_uint16_t(char *rp, int *rp_count)
{
    char num_work[8];
    char *endptr;

    /* ------------------------------------------------------
     from: [89 13 ] -> to: ['+0x'1389\0] -> to num
     ------------------------------------------------------ */
    num_work[0] = '+';
    num_work[1] = '0';
    num_work[2] = 'x';
    num_work[3] = *(rp + *rp_count + 3);
    num_work[4] = *(rp + *rp_count + 4);
    num_work[5] = *(rp + *rp_count + 0);
    num_work[6] = *(rp + *rp_count + 1);
    num_work[7] = 0;
    *rp_count += 6;

    return strtol(num_work, &endptr, 16);
}

int16_t dlt_get_ring_buffer_conv_ascii_to_int16_t(char *rp, int *rp_count)
{
    char num_work[6];
    char *endptr;

    if ((rp == NULL) || (rp_count == NULL))
    {
        return -1;
    }
    /* ------------------------------------------------------
       from: [89 ] -> to: ['0x'89\0] -> to num
       ------------------------------------------------------ */
    num_work[0] = '0';
    num_work[1] = 'x';
    num_work[2] = *(rp + *rp_count + 0);
    num_work[3] = *(rp + *rp_count + 1);
    num_work[4] = 0;
    *rp_count += 3;

    return (signed char)strtol(num_work, &endptr, 16);
}

int dlt_get_ring_buffer_make_injection_info(char *resp_text)
{
    int ret;
    int list_count;
    char *rp;
    int rp_count;
    int rp_reg_ctid_count;
    uint16_t reg_apid_count;
    uint16_t reg_ctid_count;
    uint16_t reg_apid_num;
    uint16_t reg_ctid_num;
    uint16_t reg_ctid_dsc_len;
    char reg_apid[DLT_ID_SIZE + 1];
    char reg_ctid[DLT_ID_SIZE + 1];
    char target_apid_is_wild;
    char target_apid_reg_INJECTION;
    int service_opt = 7;
    char *reg_ctid_dsc;
    /* ------------------------------------------------------
     get_log_info data structure(all data is ascii)

     get_log_info, aa, bb bb cc cc cc cc dd dd ee ee ee ee ..
     ~~  ~~~~~ ~~~~~~~~~~~ ~~~~~ ~~~~~~~~~~~~~~
     cc cc cc cc dd dd ee ee ee ee ..
     ~~~~~~~~~~~ ~~~~~ ~~~~~~~~~~~~~~
     aa         : get mode (fix value at 0x03)
     bb bb      : list num of apid (little endian)
     cc cc cc cc: apid
     dd dd      : list num of ctid (little endian)
     ee ee ee ee: ctid
     ------------------------------------------------------ */

    /* checck all target convert list */
    for (list_count = 0; list_count < ConvList.list_count; list_count++)
    {
        /* check target apid wild */
        ret = strncmp(ConvList.apid[list_count], DLT_EXT_WILD_CARD,
                DLT_ID_SIZE + 1);
        if (ret == 0)
        {
            target_apid_is_wild = 1;
        }
        else
        {
            target_apid_is_wild = 0;
        }

        /* rp set header */
        rp = (resp_text + 18);
        rp_count = 0;
        /* get reg_apid_num */
        reg_apid_num = dlt_get_ring_buffer_conv_ascii_to_uint16_t(rp,
                &rp_count);

        /* search for target apid */
        for (reg_apid_count = 0; reg_apid_count < reg_apid_num;
                reg_apid_count++)
        {

            /* get reg_apid */
            dlt_get_ring_buffer_conv_ascii_to_id(rp, &rp_count, reg_apid,
                    DLT_ID_SIZE + 1);
            /* get reg_ctid_num of current reg_apid */
            reg_ctid_num = dlt_get_ring_buffer_conv_ascii_to_uint16_t(rp,
                    &rp_count);

            if (target_apid_is_wild)
            {
                ret = 0;
            }
            else
            {
                ret = strncmp(ConvList.apid[list_count], reg_apid,
                        DLT_ID_SIZE + 1);
            }
            if (ret != 0)
            {
                /* check next reg_apid */
                rp_count += reg_ctid_num * 12; /* one ctid ASCII num include delimiter */
            }
            else
            {
                /* check target apid regist INJECTION */
                target_apid_reg_INJECTION = 0;
                rp_reg_ctid_count = rp_count;
                for (reg_ctid_count = 0; reg_ctid_count < reg_ctid_num;
                        reg_ctid_count++)
                {
                    /* get reg_ctid */
                    dlt_get_ring_buffer_conv_ascii_to_id(rp, &rp_reg_ctid_count,
                            reg_ctid, DLT_ID_SIZE + 1);
                    /* check target ctid */
                    ret = strncmp(DLT_CT_EXT_CB, reg_ctid, DLT_ID_SIZE + 1);
                    if (ret == 0)
                    {
                        /* target apid regist INJECTION */
                        target_apid_reg_INJECTION = 1;
                        break;
                    }
                    else
                    {
                        /* check next reg_ctid */
                        ;
                    }
                }
                if (!target_apid_reg_INJECTION)
                {
                    if (target_apid_is_wild)
                    {
                        /* check next reg_apid apid */
                        rp_count += reg_ctid_num * 12; /* one ctid ASCII num include delimiter */
                        continue;
                    }
                    else
                    {
                        /* check next target apid */
                        break;
                    }
                }

                /* check target ctid wild */
                ret = strncmp(ConvList.ctid[list_count], DLT_EXT_WILD_CARD,
                        DLT_ID_SIZE + 1);
                if (ret == 0)
                {
                    dlt_get_ring_buffer_set_injection_info(reg_apid,
                            DLT_EXT_WILD_CARD_TO_API);
                }
                else
                {
                    /* search for target ctid */
                    rp_reg_ctid_count = rp_count;
                    for (reg_ctid_count = 0; reg_ctid_count < reg_ctid_num;
                            reg_ctid_count++)
                    {
                        /* get reg_ctid */
                        dlt_get_ring_buffer_conv_ascii_to_id(rp,
                                &rp_reg_ctid_count, reg_ctid, DLT_ID_SIZE + 1);
                        /* check target ctid */
                        ret = strncmp(ConvList.ctid[list_count], reg_ctid,
                                DLT_ID_SIZE + 1);
                        if (ret == 0)
                        {
                            /* target ctid registed */
                            dlt_get_ring_buffer_set_injection_info(reg_apid,
                                    reg_ctid);
                            break;
                        }
                        dlt_get_ring_buffer_conv_ascii_to_int16_t(rp, &rp_reg_ctid_count);
                        dlt_get_ring_buffer_conv_ascii_to_int16_t(rp, &rp_reg_ctid_count);
                        /* Description Information */
                        if (service_opt == 7)
                        {
                            reg_ctid_dsc_len = dlt_get_ring_buffer_conv_ascii_to_uint16_t(rp, &rp_reg_ctid_count);
                            reg_ctid_dsc = (char *)malloc(sizeof(char) * reg_ctid_dsc_len + 1);
                            if (reg_ctid_dsc == 0)
                            {
                                return -1;
                            }
                            dlt_get_ring_buffer_conv_ascii_to_id(rp, &rp_reg_ctid_count, reg_ctid_dsc, reg_ctid_dsc_len+1);
                        }

                    }
                }
                if (target_apid_is_wild)
                {
                    /* check next reg_apid apid */
                    rp_count += reg_ctid_num * 12; /* one ctid ASCII num include delimiter */
                }
                else
                {
                    /* check next target apid */
                    break;
                }
            }
        }
    }

    if (InjectionList.info_list_count)
    {
        /* need INJECTION fire */
        ret = 0;
    }
    else
    {
        /* no need INJECTION fire */
        ret = -1;
    }

    return ret;
}

int dlt_get_ring_buffer_send_injection(char *resp_text)
{
    int ret;
    /* make injection info */
    ret = dlt_get_ring_buffer_make_injection_info(resp_text);

    return ret;
}

int dlt_get_ring_buffer_parse_service_id(char *resp_text, int *service_id,
        char *cb_result)
{
    int ret;
    char service_tag[9];
    char get_log_info_tag[13];
    char *rp;
    char service_id_ascii[DLT_SERVICE_ID_MAX_LEN + 1];
    char options_ascii[3];
    int options;
    int count;

    /* check of syntax */
    strncpy(service_tag, "service(", 9);
    ret = memcmp((void *) resp_text, (void *) service_tag,
            sizeof(service_tag) - 1);
    if (ret == 0)
    {
        /* value type, syntax is 'service(4096), ok' */

        /* get sarvice id value */
        ret = -1;
        memset(service_id_ascii, 0, DLT_SERVICE_ID_MAX_LEN + 1);
        rp = resp_text + sizeof(service_tag) - 1;
        for (count = 0; count < DLT_SERVICE_ID_MAX_LEN; count++)
        {
            if (*rp == ')')
            {
                *service_id = atoi(service_id_ascii);
                if (*service_id != 0)
                {
                    ret = 0;
                }
                break;
            }
            service_id_ascii[count] = *rp;
            rp++;
        }
        if (ret == 0)
        {
            /* get callback result */
            rp += 2;
            if ((rp[0] == 'o') && (rp[1] == 'k'))
            {
                *cb_result = 0;
            }
            else
            {
                *cb_result = 1;
            }
        }
    }
    else
    {
        /* ascii type, syntax is 'get_log_info, ..' */

        /* check target id */
        strncpy(get_log_info_tag, "get_log_info", 13);
        ret = memcmp((void *) resp_text, (void *) get_log_info_tag,
                sizeof(get_log_info_tag) - 1);
        if (ret == 0)
        {
            /* check options is same my request */
            options_ascii[0] = *((char*) resp_text + sizeof(get_log_info_tag)
                    + 1);
            options_ascii[1] = *((char*) resp_text + sizeof(get_log_info_tag)
                    + 2);
            options_ascii[2] = 0;
            options = atoi(options_ascii);
            if (options == 7)
            {
                *service_id = DLT_SERVICE_ID_GET_LOG_INFO;
                *cb_result = 0;
            }
            else
            {
                ret = -1;
                *cb_result = 1;
            }
        }
    }
    return ret;
}

int dlt_message_callback(char *resp_text, void *data, int len)
{
    int ret;
    int service_id;
    char cb_result;

    /* parameter check */
    if ((resp_text == NULL) || (len < 0))
    {
        return -1;
    }
    if (data == NULL)
    {
        ;
    }
    /* check service id */
    ret = dlt_get_ring_buffer_parse_service_id(resp_text, &service_id,
            &cb_result);
    if (ret == 0)
    {
        switch (service_id)
        {
        /* GET_LOG_INFO response */
        case DLT_SERVICE_ID_GET_LOG_INFO:
            ret = dlt_get_ring_buffer_send_injection(resp_text);
            if (ret != 0)
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "GET_LOG_INFO result failed..\n");
                g_send_injection_msg = 0;
                return -1;
            }
            g_send_injection_msg = 1;
            break;

            /* INJECTION response */
        case DLT_EX_INJECTION_CODE:
            if (cb_result)
            {
                /* INJECTION success */
                //dlt_client_cleanup(&dltclient, 0);
            }
            else
            {
                /* INJECTION failed.. */
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "INJECTION result failed..\n");
                //dlt_client_cleanup(&dltclient, 0);
            }
            break;

            /* unknown response */
        default:
            break;
        }
    }

    return ret;
}

/**
 * @brief Prepare message body to be send to DLT Daemon
 *
 * @return Pointer ot DltControlMsgBody, NULL otherwise
 */
DltControlMsgBody *dlt_get_loginfo_prepare_message_body()
{
    DltControlMsgBody *mb = calloc(1, sizeof(DltControlMsgBody));
    if (mb == NULL)
    {
        return NULL;
    }

    mb->data = calloc(1, sizeof(DltServiceGetLogInfoRequest));
    if (mb->data == NULL)
    {
        free(mb);
        return NULL;
    }
    mb->size = sizeof(DltServiceGetLogInfoRequest);
    DltServiceGetLogInfoRequest *serv = (DltServiceGetLogInfoRequest *)mb->data;
    serv->service_id = DLT_SERVICE_ID_GET_LOG_INFO;
    serv->options = 7;
    dlt_set_id(serv->apid, "");
    dlt_set_id(serv->ctid, "");
    dlt_set_id(serv->com, "remo");

    return mb;
}


int dlt_get_ring_buffer_do_injection(void)
{
    int ret = 0;

    /* Initializing the communication with the daemon */
    if (dlt_control_init(dlt_message_callback,
                         get_ecuid(),
                         get_verbosity()) != 0)
    {
        pr_error("Failed to initialize connection with the daemon.\n");
        return ret;
    }

    /* prepare GetlogInfo message body */
    DltControlMsgBody *msg_body = NULL;
    msg_body = dlt_get_loginfo_prepare_message_body();

    if (msg_body == NULL)
    {
        pr_error("Data for Dlt GetLogInfo Message body is NULL\n");
        return ret;
    }

    ret = dlt_control_send_message(msg_body, get_timeout());

    free(msg_body->data);
    free(msg_body);

    //dlt_control_deinit();

    return ret;
}


/*--------------------------------------------------------------------------------------------*/
/* option parser modules                                                                      */
/*--------------------------------------------------------------------------------------------*/
int dlt_get_ring_buffer_parse_get_id(FILE *fp, char *id_buf, int len)
{
    int write_count;
    int c;
    char delimiter_detect;
    char eof_detect;

    /* get id */
    write_count = 0;
    delimiter_detect = 0;
    eof_detect = 0;
    do
    {
        c = fgetc(fp);
        switch (c)
        {
        case 0xA: /* LF */
        case 0x20: /* Space */
            if (write_count)
            {
                /* detect delimiter */
                *id_buf = 0;
                id_buf++;
                delimiter_detect = 1;
            }
            else
            {
                /* skip delimiter */
                ;
            }
            break;
        case EOF: /* EOF */
            if (write_count)
            {
                *id_buf = 0;
                write_count++;
                id_buf++;
            }
            eof_detect = 1;
            break;
        default:
            *id_buf = c;
            write_count++;
            id_buf++;
            break;
        }
        if (delimiter_detect || eof_detect)
        {
            break;
        }
    }
    while (write_count < (len + 1));

    if (!(delimiter_detect || eof_detect))
    {
        /* NULL terminate lost */
        return DLT_GET_HPLOG_PARSE_ID_FAIL;
    }
    if (eof_detect)
    {
        if (write_count)
        {
            /* ID and EOF detect */
            return DLT_GET_HPLOG_PARSE_ID_EOF;
        }
        else
        {
            /* only EOF detect */
            return DLT_GET_HPLOG_PARSE_EOF;
        }
    }
    return DLT_GET_HPLOG_PARSE_ID_OK;
}

int dlt_get_ring_buffer_parse_conv_file(void)
{
    FILE * fp;
    int ret;
    int ret_val = 1;
    int list_count;
    char apid[DLT_ID_SIZE + 1];
    char ctid[DLT_ID_SIZE + 1];

    /* open target file */
    fp = fopen(conv_list_file, "r");
    if (fp == NULL)
    {
        return -1;
    }
    /* make convert list */
    list_count = 0;
    do
    {
        /* get apid */
        ret = dlt_get_ring_buffer_parse_get_id(fp, apid, DLT_ID_SIZE);
        if (ret == DLT_GET_HPLOG_PARSE_ID_EOF)
        {
            /* if detect EOF after apid, then parse fail. */
            ret_val = 1;
        }
        else if (ret == DLT_GET_HPLOG_PARSE_EOF)
        {
            /* if detect EOF before apid, then parse finish. */
            ret_val = 0;
        }
        else if (ret == DLT_GET_HPLOG_PARSE_ID_OK)
        {
            /* get ctid */
            ret = dlt_get_ring_buffer_parse_get_id(fp, ctid, DLT_ID_SIZE);
            if (ret == DLT_GET_HPLOG_PARSE_EOF)
            {
                /* if detect EOF before ctid, then parse fail. */
                ret_val = 1;
            }
            else if ((ret == DLT_GET_HPLOG_PARSE_ID_OK)
                    || (ret == DLT_GET_HPLOG_PARSE_ID_EOF))
            {
                /* set apid and ctid to conv list */
                strncpy(ConvList.apid[list_count], apid, DLT_ID_SIZE + 1);
                strncpy(ConvList.ctid[list_count], ctid, DLT_ID_SIZE + 1);
                list_count++;
                if (ret == DLT_GET_HPLOG_PARSE_ID_EOF)
                {
                    ret_val = 0;
                    break;
                }
                if (list_count == DLT_CONV_LIST_MAX_COUNT)
                {
                    /* convert list num over, sopt make list */
                    ret = DLT_GET_HPLOG_PARSE_ID_EOF;
                    ret_val = 0;
                    break;
                }
            }
            else
            {
                /**/
            }
        }
        else
        {
            /**/
        }
    }
    while (ret == DLT_GET_HPLOG_PARSE_ID_OK);

    if (ret_val == 0)
    {
        /* make convert list success */
        ConvList.list_count = list_count;
    }

    /* close target file */
    fclose(fp);

    return ret_val;
}

void dlt_get_ring_buffer_parse_option(int argc, char *argv[])
{
    int result;
    int ret;
    char syntax_err_detect = 0;
    char pid_ascii[DLT_EXT_PID_MAX_LEN + 1];
    char *ListFileName;
    char *p;

    do
    {
        result = getopt(argc, argv, "iap:ct:f:o:");
        switch (result)
        {
        /* option1 */
        /* INJECTIN request */
        case 'i':
            if (opt1 == DLT_GET_HPLOG_OPT1_NONE)
            {
                opt1 = DLT_GET_HPLOG_OPT1_INJECTION;
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;
            /* kill request(APID CTID) */
        case 'a':
            if (opt1 == DLT_GET_HPLOG_OPT1_NONE)
            {
                opt1 = DLT_GET_HPLOG_OPT1_KILL_DLT_ID;
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;
            /* kill request(PID) */
        case 'p':
            if (opt1 == DLT_GET_HPLOG_OPT1_NONE)
            {
                if (strlen(optarg) > DLT_EXT_PID_MAX_LEN)
                {
                    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "PID invalid..\n");
                    syntax_err_detect = 1;
                }
                else
                {
                    strncpy(pid_ascii, optarg, DLT_EXT_PID_MAX_LEN + 1);
                    ConvList.pid_value[0] = atoi(optarg);
                    ConvList.list_count = 1;
                    opt1 = DLT_GET_HPLOG_OPT1_KILL_PID;
                }
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;
            /* Log convert request */
        case 'c':
            if (opt1 == DLT_GET_HPLOG_OPT1_NONE)
            {
                opt1 = DLT_GET_HPLOG_OPT1_CONVERT;
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;

            /* option2 */
            /* target specific command parameter(APID CTID) */
        case 't':
            if (opt2 == DLT_GET_HPLOG_OPT2_NONE)
            {
                p = strchr(optarg, ',');
                if (p == NULL)
                {
                    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "no delimiter..\n");
                    syntax_err_detect = 1;
                }
                else
                {
                    /* get target APID */
                    if (((p - optarg) == 0) || ((p - optarg) > DLT_ID_SIZE))
                    {
                        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "APID len over..\n");
                        syntax_err_detect = 1;
                    }
                    else
                    {
                        if (((p - optarg) == 1)
                                && (memcmp(optarg, DLT_EXT_WILD_CARD_TO_API, 1)
                                        == 0))
                        {
                            strncpy(ConvList.apid[0], DLT_EXT_WILD_CARD,
                                    strlen(DLT_EXT_WILD_CARD));
                        }
                        else
                        {
                            strncpy(ConvList.apid[0], optarg, (p - optarg));
                        }
                        /* get target CTID */
                        if ((strlen(p + 1) == 0)
                                || (strlen(p + 1) > DLT_ID_SIZE))
                        {
                            DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                    "CTID len over..\n");
                            syntax_err_detect = 1;
                        }
                        else
                        {
                            if ((strlen(p + 1) == 1)
                                    && (memcmp((p + 1),
                                            DLT_EXT_WILD_CARD_TO_API, 1) == 0))
                            {
                                strncpy(ConvList.ctid[0], DLT_EXT_WILD_CARD,
                                        strlen(DLT_EXT_WILD_CARD));
                            }
                            else
                            {
                                strncpy(ConvList.ctid[0], (p + 1),
                                        strlen(p + 1));
                            }
                            ConvList.list_count = 1;
                            opt2 = DLT_GET_HPLOG_OPT2_DLT_ID;
                        }
                    }
                }
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;
            /* target specific list file */
        case 'f':
            if (opt2 == DLT_GET_HPLOG_OPT2_NONE)
            {
                strncpy(conv_list_file, optarg, DLT_MAX_FILEPATH_LEN);
                /* make convert list */
                ret = dlt_get_ring_buffer_parse_conv_file();
                if (ret)
                {
                    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                            "input file format NG..\n");
                    syntax_err_detect = 1;
                }
                else
                {
                    opt2 = DLT_GET_HPLOG_OPT2_LIST;
                }
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;

            /* option3 */
            /* output file name specific */
        case 'o':
            if (opt3 == DLT_GET_HPLOG_OPT3_NONE)
            {
                opt3 = DLT_GET_HPLOG_OPT3_ENABLE;
                strncpy(output_file_path, optarg, DLT_MAX_FILEPATH_LEN);
            }
            else
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                        "Multiple Option detect..\n");
                syntax_err_detect = 1;
            }
            break;

            /* invalid parameter detect */
        case '?':
            if (optopt == 'p' || optopt == 't' || optopt == 'f'
                    || optopt == 'o')
            {
                DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                      "Option -%c requires an argument.\n",
                                      optopt);
            }
            else
            {
                if (isprint(optopt))
                {
                    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                                          "Unknown option '-%c'.\n",
                                          optopt);
                }
                else
                {
                    DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                            "Unknown option character detect..");
                }
            }
            syntax_err_detect = 1;
            break;
        default:
            break;
        }
    }
    while ((result != -1) && !syntax_err_detect);

    /* option num check */
    if (opt1 == DLT_GET_HPLOG_OPT1_NONE)
    {
        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "option1 needs..\n");
        usage();
        return;
    }
    else if (opt1 == DLT_GET_HPLOG_OPT1_KILL_PID)
    {
        if (opt2 != DLT_GET_HPLOG_OPT2_NONE)
        {
            DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                    "if use -p, then option2 not use..\n");
            opt1 = DLT_GET_HPLOG_OPT1_NONE;
            usage();
            return;
        }
    }
    else if (opt1 == DLT_GET_HPLOG_OPT1_CONVERT)
    {
        if (opt2 != DLT_GET_HPLOG_OPT2_DLT_ID)
        {
            DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                    "if use -c, then option2 use only -t..\n");
            opt1 = DLT_GET_HPLOG_OPT1_NONE;
            usage();
            return;
        }
    }
    else
    {
        if (opt2 == DLT_GET_HPLOG_OPT2_NONE)
        {
            DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG,
                    "if use -i -a, then option2 needs..\n");
            opt1 = DLT_GET_HPLOG_OPT1_NONE;
            usage();
            return;
        }
    }

    if ((argc - optind) != 0)
    {
        syntax_err_detect = 1;
        DLT_GET_HPLOG_DBG_LOG(HP_LOG_DEBUG, "unsuported parameters detect..\n");
    }
    if (syntax_err_detect)
    {
        opt1 = DLT_GET_HPLOG_OPT1_NONE;
        usage();
        return;
    }

    /* if no option3 */
    if (opt3 == DLT_GET_HPLOG_OPT3_NONE)
    {
        if (opt1 == DLT_GET_HPLOG_OPT1_KILL_PID)
        {
            memset(output_file_path, 0, DLT_MAX_FILEPATH_LEN + 1);
            strncat(output_file_path, DLT_EXT_CTID_DLT_DIRECTORY,
                    sizeof(DLT_EXT_CTID_DLT_DIRECTORY));
            strncat(output_file_path, DLT_EXT_SLASH, sizeof(DLT_EXT_SLASH));
            strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_PREFIX,
                    sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX));
            strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                    sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER));
            strncat(output_file_path, pid_ascii, strlen(pid_ascii) + 1);
            strncat(output_file_path, DLT_EXT_CTID_DLT_FILE_EXTENSION,
                    sizeof(DLT_EXT_CTID_DLT_FILE_EXTENSION));
        }
        else if (opt2 == DLT_GET_HPLOG_OPT2_DLT_ID)
        {
            /* option2:-t */
            if (strncmp(ConvList.apid[0], DLT_EXT_WILD_CARD, DLT_ID_SIZE + 1))
            {
                if (strncmp(ConvList.ctid[0], DLT_EXT_WILD_CARD,
                        DLT_ID_SIZE + 1))
                {
                    memset(output_file_path, 0, DLT_MAX_FILEPATH_LEN + 1);
                    strncat(output_file_path, DLT_EXT_CTID_DLT_DIRECTORY,
                            sizeof(DLT_EXT_CTID_DLT_DIRECTORY));
                    strncat(output_file_path, DLT_EXT_SLASH,
                            sizeof(DLT_EXT_SLASH));
                    strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_PREFIX,
                            sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX));
                    strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                            sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER));
                    strncat(output_file_path, ConvList.ctid[0],
                            strlen(ConvList.ctid[0]) + 1);
                    strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                            sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER));
                    strncat(output_file_path, ConvList.apid[0],
                            strlen(ConvList.apid[0]) + 1);
                    strncat(output_file_path, DLT_EXT_CTID_DLT_FILE_EXTENSION,
                            sizeof(DLT_EXT_CTID_DLT_FILE_EXTENSION));
                }
                else
                {
                    memset(output_file_path, 0, DLT_MAX_FILEPATH_LEN + 1);
                    strncat(output_file_path, DLT_EXT_CTID_DLT_DIRECTORY,
                            sizeof(DLT_EXT_CTID_DLT_DIRECTORY));
                    strncat(output_file_path, DLT_EXT_SLASH,
                            sizeof(DLT_EXT_SLASH));
                    strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_PREFIX,
                            sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX));
                    strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                            sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER));
                    strncat(output_file_path, ConvList.apid[0],
                            strlen(ConvList.apid[0]) + 1);
                    strncat(output_file_path, DLT_EXT_CTID_DLT_FILE_EXTENSION,
                            sizeof(DLT_EXT_CTID_DLT_FILE_EXTENSION));
                }
            }
            else
            {
                memset(output_file_path, 0, DLT_MAX_FILEPATH_LEN + 1);
                strncat(output_file_path, DLT_EXT_CTID_DLT_DIRECTORY,
                        sizeof(DLT_EXT_CTID_DLT_DIRECTORY));
                strncat(output_file_path, DLT_EXT_SLASH, sizeof(DLT_EXT_SLASH));
                strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_PREFIX,
                        sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX));
                strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                        sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER));
                strncat(output_file_path, ConvList.ctid[0],
                        strlen(ConvList.ctid[0]) + 1);
                strncat(output_file_path, DLT_EXT_CTID_DLT_FILE_EXTENSION,
                        sizeof(DLT_EXT_CTID_DLT_FILE_EXTENSION));
            }
        }
        else
        {
            /* option2:-f */
            memset(output_file_path, 0, DLT_MAX_FILEPATH_LEN + 1);
            strncat(output_file_path, DLT_EXT_CTID_DLT_DIRECTORY,
                    sizeof(DLT_EXT_CTID_DLT_DIRECTORY));
            strncat(output_file_path, DLT_EXT_SLASH, sizeof(DLT_EXT_SLASH));
            strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_PREFIX,
                    sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX));
            strncat(output_file_path, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                    sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER));
            ListFileName = dlt_get_ring_buffer_get_file_name(conv_list_file);
            strncat(output_file_path, ListFileName, strlen(ListFileName) + 1);
            strncat(output_file_path, DLT_EXT_CTID_DLT_FILE_EXTENSION,
                    sizeof(DLT_EXT_CTID_DLT_FILE_EXTENSION));
        }
    }
    /* make tmp file name */
    tmp_file_path[0] = 0;
    strncat(tmp_file_path, output_file_path, strlen(output_file_path) + 1);
    strncat(tmp_file_path, DLT_EXT_CTID_TMP_FILE_EXTENSION,
            strlen(DLT_EXT_CTID_TMP_FILE_EXTENSION) + 1);
}


/*--------------------------------------------------------------------------------------------*/
/*  main module                                                                               */
/*--------------------------------------------------------------------------------------------*/
int main(int argc, char *argv[])
{
    int result;

    set_timeout(100);

    /* parse command option */
    dlt_get_ring_buffer_parse_option(argc, argv);
    if (opt1 == DLT_GET_HPLOG_OPT1_NONE)
    {
        /* ivalid command option */
        exit(-1);
    }

    /* do operation */
    result = 0;
    switch (opt1)
    {
    case DLT_GET_HPLOG_OPT1_INJECTION:
        result = dlt_get_ring_buffer_do_injection();
        break;
    case DLT_GET_HPLOG_OPT1_KILL_DLT_ID:
    case DLT_GET_HPLOG_OPT1_KILL_PID:
        result = dlt_get_ring_buffer_kill();
        break;
    case DLT_GET_HPLOG_OPT1_CONVERT:
        break;
    default:
        result = -1;
        break;
    }
    if (result == -1)
    {
        /* operation failed */
        exit(-1);
    }

    if(g_send_injection_msg)
    {
        dlt_get_ring_buffer_injection_fire();
    }
    /* file convert */
    result = dlt_get_ring_buffer_do_convert();
    if (result == -1)
    {
        /* convert failed */
        fprintf(stdout, "convert failed..\n");
        exit(-1);
    }
    else if (result == 0)
    {
        fprintf(stdout, "there is no valid hp trace, not convert.\n");
    }
    else
    {
        fprintf(stdout, "convert done! -> [%s]\n", output_file_path);
    }
    return 0;
}
