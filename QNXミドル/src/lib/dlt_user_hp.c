/**
 * @licence app begin@
 * Copyright (C) 2013 - 2015  Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * DLT HP log functionality source file
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author
 * \author
 *
 * \file: dlt_user_hp.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */
#include <stdlib.h>
#include <string.h>
#include <signal.h>

#include <sys/time.h>
#include <math.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <unistd.h>

#include <stdio.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <time.h>
#include <dirent.h>
#include <syslog.h>

#include "dlt_user.h"
#include "dlt_user_shared.h"
#include "dlt_user_shared_cfg.h"
#include "dlt_user_cfg.h"

extern DltUser dlt_user;
extern sem_t dlt_mutex;
static int dlt_hp_ctid_cnt = 0;
static int dlt_nw_trace_hp_cfg = -1;
DLT_DECLARE_CONTEXT(context_injection_callback);

int dlt_ext_injection_macro_callback(uint32_t service_id, void *data,
        uint32_t length);

void dlt_ext_set_ringbuf_header(DltExtBuff *pDltExtBuff)
{
    DltExtBuffHeader *pBufHeader;

    if (pDltExtBuff == NULL)
        return;

    /* initialize ring buffer */
    pBufHeader = (DltExtBuffHeader *) pDltExtBuff->addr;
    pBufHeader->write_count = 0;
    pBufHeader->size = pDltExtBuff->size;

    /* initialize common log header */
    memcpy(pBufHeader->ecuid, dlt_user.ecuID, sizeof(pBufHeader->ecuid));
    pBufHeader->htyp = DLT_HTYP_PROTOCOL_VERSION1;
    /* send ecu id */
    if (dlt_user.with_ecu_id)
    {
        pBufHeader->htyp |= DLT_HTYP_WEID;
    }
    /* send timestamp */
    if (dlt_user.with_timestamp)
    {
        pBufHeader->htyp |= DLT_HTYP_WTMS;
    }
    /* send session id */
    if (dlt_user.with_session_id)
    {
        pBufHeader->htyp |= DLT_HTYP_WSID;
    }
    if (dlt_user.verbose_mode)
    {
        /* In verbose mode, send extended header */
        pBufHeader->htyp |= DLT_HTYP_UEH;
    }
    else
    {
        /* In non-verbose, send extended header if desired */
        if (dlt_user.use_extende_header_for_non_verbose)
            pBufHeader->htyp |= DLT_HTYP_UEH;
    }
#if (BYTE_ORDER==BIG_ENDIAN)
    pBufHeader->htyp |= DLT_HTYP_MSBF;
#endif
    pBufHeader->msin = DLT_EXT_LOG_HEADER_MSIN;
    pBufHeader->noar = DLT_EXT_LOG_HEADER_NOAR;
    memcpy(pBufHeader->apid, dlt_user.appID, sizeof(pBufHeader->apid));
    memcpy(pBufHeader->ctid, pDltExtBuff->ctid, sizeof(pBufHeader->ctid));
    pBufHeader->type_info_header = DLT_TYPE_INFO_STRG | DLT_SCOD_ASCII;
    pBufHeader->type_info_payload = DLT_TYPE_INFO_RAWD;

    return;
}

int dlt_ext_check_extension(char *filename, char *extension)
{
    int ret = 0;
    int body_len;
    int ext_len;

    ext_len = strlen(extension);
    body_len = strlen(filename) - ext_len;
    if (body_len < 0)
    {
        ret = 1;
    }
    else
    {
        filename += body_len;
        if (strncmp(filename, extension, ext_len) != 0)
        {
            ret = 1;
        }
    }
    return ret;
}

int dlt_ext_parse_filename(char *filename, char *ctid, char *apid, int *pid,
        int *bknum)
{
    int ret = 0;
    char *rp;
    uint8_t count;
    char apid_old[DLT_ID_SIZE + 1];
    char ctid_old[DLT_ID_SIZE + 1];
    char pid_ascii[DLT_EXT_PID_MAX_LEN + 1];
    char bknum_ascii[DLT_EXT_BKNUM_LEN + 1];
    int bknum_detect = 0;

    /* dlt_ctid_apid_pid_bknum.xxx */
    /* skip to ctid */
    rp = filename + sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX) - 1
            + sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1;

    /* get ctid */
    for (count = 0; count < DLT_ID_SIZE + 1; count++)
    {
        if (*rp == 0)
        {
            return DLT_RETURN_ERROR;
        }
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);
        if (ret == 0)
        {
            break;
        }
        ctid_old[count] = *rp;
        rp++;
    }
    if (count == DLT_ID_SIZE + 1)
    {
        return DLT_RETURN_ERROR;
    }
    ctid_old[count] = 0;
    rp += (sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);

    /* get apid */
    for (count = 0; count < DLT_ID_SIZE + 1; count++)
    {
        if (*rp == 0)
            return DLT_RETURN_ERROR;
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_DELIMITER,
                sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);
        if (ret == 0)
        {
            break;
        }
        apid_old[count] = *rp;
        rp++;
    }
    if (count == DLT_ID_SIZE + 1)
        return DLT_RETURN_ERROR;
    apid_old[count] = 0;
    rp += (sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1);

    /* check pid */
    for (count = 0; count < DLT_EXT_PID_MAX_LEN + 1; count++)
    {
        if (*rp == 0)
            return DLT_RETURN_ERROR;
        ret = memcmp(rp, DLT_EXT_CTID_LOG_PERIOD,
                sizeof(DLT_EXT_CTID_LOG_PERIOD) - 1);
        if (ret == 0)
        {
            bknum_detect = 0; /* Rear char is not backup number */
            break;
        }
        ret = memcmp(rp, DLT_EXT_CTID_LOG_FILE_BKNUM_DELI,
                sizeof(DLT_EXT_CTID_LOG_FILE_BKNUM_DELI) - 1);
        if (ret == 0)
        {
            bknum_detect = 1; /* Rear char is backup number */
            break;
        }
        pid_ascii[count] = *rp;
        rp++;
    }
    if (count == DLT_EXT_PID_MAX_LEN + 1)
        return DLT_RETURN_ERROR;
    pid_ascii[count] = 0;
    *pid = atoi(pid_ascii);
    rp += (sizeof(DLT_EXT_CTID_LOG_FILE_BKNUM_DELI) - 1);

    if (bknum_detect == 1)
    {
        /* get backup number */
        for (count = 0; count < DLT_EXT_BKNUM_LEN + 1; count++)
        {
            if (*rp == 0)
                return DLT_RETURN_ERROR;
            ret = memcmp(rp, DLT_EXT_CTID_LOG_PERIOD,
                    sizeof(DLT_EXT_CTID_LOG_PERIOD) - 1);
            if (ret == 0)
            {
                break;
            }
            bknum_ascii[count] = *rp;
            rp++;
        }
        if (count == DLT_EXT_BKNUM_LEN + 1)
            return DLT_RETURN_ERROR;
        bknum_ascii[count] = 0;
        *bknum = atoi(bknum_ascii);
    }
    else
    {
        *bknum = -1;
    }

    if ((strncmp(ctid, ctid_old, DLT_ID_SIZE) == 0)
            && (strncmp(apid, apid_old, DLT_ID_SIZE) == 0))
    {
        return DLT_RETURN_OK;
    }

    return DLT_RETURN_ERROR;
}

void dlt_ext_backup_ringbuf(DltExtBuff *pDltExtBuff)
{
    DIR *dir;
    struct dirent *dent;
    struct stat sb;
    int ret;
    int pid;
    int bknum;
    int bknum_max = -1;
    int target_detect = 0;
    int log_len;
    char log_file_path[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1] = {'\0'};
    char apid[DLT_ID_SIZE + 1] = {'\0'};
    char ctid[DLT_ID_SIZE + 1] = {'\0'};
    char target_log_extention[] = "log";
    char rename_file[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1] = {'\0'};
    char rename_filepath[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1] ={'\0'};
    char old_log_filepath[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1] ={'\0'};

    dlt_set_id(ctid, pDltExtBuff->ctid);
    dlt_set_id(apid, dlt_user.appID);

    dir = opendir(DLT_EXT_CTID_LOG_DIRECTORY);
    if (dir == NULL)
    {
        return;
    }

    while ((dent = readdir(dir)) != NULL )
    {
        snprintf(log_file_path, DLT_EXT_CTID_LOG_FILEPATH_LEN + 1, "%s%s",
                DLT_EXT_CTID_LOG_DIRECTORY, dent->d_name);
        if ((stat(log_file_path, &sb) < 0) && (errno != ENOENT))
        {
            dlt_log(LOG_DEBUG, " Don't find file ERROR\n");
            continue;
        }
        if (!S_ISREG(sb.st_mode))
        {
            dlt_log(LOG_DEBUG, " No file\n");
            continue;
        }

        /* check extention(if not ".log", then check next file) */
        ret = dlt_ext_check_extension(dent->d_name, target_log_extention);
        if (ret == 1)
        {
            dlt_log(LOG_DEBUG, " Not log file\n");
            continue;
        }

        /* check matching for target CTID and APID and get pid and backup number */
        ret = dlt_ext_parse_filename(dent->d_name, ctid, apid, &pid, &bknum);
        if (ret == DLT_RETURN_OK)
        {
            dlt_log(LOG_DEBUG, "Same Ctid and Apid, previous log file\n");
            if (bknum_max < bknum)
            {
                bknum_max = bknum;
            }
            if (bknum == -1)
            {
                target_detect = 1;
                log_len = strlen(dent->d_name);
                strncpy(rename_file, dent->d_name,
                        DLT_EXT_CTID_LOG_FILEPATH_LEN);
                if (DLT_EXT_CTID_LOG_FILEPATH_LEN
                        < (log_len
                                - (sizeof(DLT_EXT_CTID_LOG_FILE_EXTENSION) - 1)))
                {
                    dlt_log(LOG_ERR, "MAX length of filename is over\n");
                    return;
                }
                rename_file[log_len
                        - (sizeof(DLT_EXT_CTID_LOG_FILE_EXTENSION) - 1)] = 0;
            }
        }
    }
    closedir(dir);

    if (target_detect != 0)
    {
        snprintf(rename_filepath, DLT_EXT_CTID_LOG_FILEPATH_LEN + 1, "%s%s%s",
                DLT_EXT_CTID_LOG_DIRECTORY, rename_file,
                DLT_EXT_CTID_LOG_FILE_EXTENSION);
        if (bknum_max == DLT_EXT_BKNUM_MAX)
        {
            dlt_log(LOG_DEBUG, " Previous log filename remove!\n");
            remove(rename_filepath);
        }
        else
        {
            dlt_log(LOG_DEBUG, " Previous log filename change!\n");
            bknum_max = bknum_max + 1;
            snprintf(old_log_filepath, DLT_EXT_CTID_LOG_FILEPATH_LEN + 1,
                    "%s%s_%d%s", DLT_EXT_CTID_LOG_DIRECTORY, rename_file,
                    bknum_max, DLT_EXT_CTID_LOG_FILE_EXTENSION);
            rename(rename_filepath, old_log_filepath);
        }
    }
    return;
}

void dlt_ext_get_log_filepath(char *filepath_p, char *filename_p,
        char *extention_p)
{
    filepath_p[0] = 0;
    strncat(filepath_p, DLT_EXT_CTID_LOG_DIRECTORY,
            sizeof(DLT_EXT_CTID_LOG_DIRECTORY) - 1);
    strncat(filepath_p, filename_p, strlen(filename_p));
    strncat(filepath_p, extention_p, strlen(extention_p));
    return;
}

int dlt_ext_check_log_dir(void)
{
    int ret = DLT_RETURN_OK;
    struct stat sb;

    if (stat(DLT_EXT_CTID_LOG_DIRECTORY, &sb) < 0)
    {
        if (stat(DLT_EXT_CTID_DLT_DIRECTORY, &sb) < 0)
        {
            if (mkdir(DLT_EXT_CTID_DLT_DIRECTORY, DLT_EXT_CTID_MKDIR_MODE) < 0)
            {
                dlt_log(LOG_ERR, "mkdir fail..\n");
                ret = DLT_RETURN_ERROR;
            }
            else
            {
                if (mkdir(DLT_EXT_CTID_LOG_DIRECTORY, DLT_EXT_CTID_MKDIR_MODE)
                        < 0)
                {
                    dlt_log(LOG_ERR, "mkdir fail..\n");
                    ret = DLT_RETURN_ERROR;
                }
            }
        }
        else
        {
            if (mkdir(DLT_EXT_CTID_LOG_DIRECTORY, DLT_EXT_CTID_MKDIR_MODE) < 0)
            {
                dlt_log(LOG_ERR, "mkdir fail..\n");
                ret = DLT_RETURN_ERROR;
            }
        }
    }
    return ret;
}

int dlt_ext_make_ringbuf(DltExtBuff *pDltExtBuff)
{
    int ret;
    char log_filepath[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1];
    int fd;
    int ret_val;
    const char c = '\0';
    ssize_t write_ret;
    void *addr = NULL;

    ret = dlt_ext_check_log_dir();
    if (ret != DLT_RETURN_OK)
    {
        return ret;
    }

    dlt_ext_get_log_filepath(log_filepath, pDltExtBuff->log_filename,
            DLT_EXT_CTID_LOG_FILE_EXTENSION);

    /* if there are log files of same apid and same ctid, create backup log file */
    dlt_ext_backup_ringbuf(pDltExtBuff);

    ret = DLT_RETURN_ERROR;
    fd = open(log_filepath, O_CREAT | O_RDWR, S_IRWXU | S_IRWXO);
    if (fd != -1)
    {
        ret_val = lseek(fd, (pDltExtBuff->size - 1), SEEK_SET);
        if (ret_val != -1)
        {
            write_ret = write(fd, &c, 1);
            if (write_ret != -1)
            {
                addr = mmap(NULL, pDltExtBuff->size, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
                if (addr != MAP_FAILED)
                {
                    ret = DLT_RETURN_OK;
                    pDltExtBuff->addr = addr;
                }
            }
        }
        close(fd);
    }
    if (ret != DLT_RETURN_OK)
    {
        dlt_log(LOG_ERR, "log file operation failed..\n");
    }
    return ret;
}

void dlt_ext_decide_log_filename(DltExtBuff *pDltExtBuff)
{
    char apid_str[DLT_ID_SIZE + 1];
    char pid_str[DLT_EXT_PID_MAX_LEN + 1];

    /* get string of user apid and pid */
    memset(apid_str, 0, sizeof(apid_str));
    dlt_set_id(apid_str, dlt_user.appID);
    sprintf(pid_str, "%d", getpid());

    /* log file name decide */
    pDltExtBuff->log_filename[0] = 0;
    strncat(pDltExtBuff->log_filename, DLT_EXT_CTID_LOG_FILE_PREFIX,
            sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX) - 1); /* dlt    */
    strncat(pDltExtBuff->log_filename, DLT_EXT_CTID_LOG_FILE_DELIMITER,
            sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1); /* _      */
    strncat(pDltExtBuff->log_filename, pDltExtBuff->ctid,
            strlen(pDltExtBuff->ctid)); /* <ctid> */
    strncat(pDltExtBuff->log_filename, DLT_EXT_CTID_LOG_FILE_DELIMITER,
            sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1); /* _      */
    strncat(pDltExtBuff->log_filename, apid_str, strlen(apid_str)); /* <apid> */
    strncat(pDltExtBuff->log_filename, DLT_EXT_CTID_LOG_FILE_DELIMITER,
            sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER) - 1); /* _      */
    strncat(pDltExtBuff->log_filename, pid_str, strlen(pid_str)); /* <pid>  */
    return;
}

int dlt_ext_ringbuf_init(DltContext *handle, const char *contextid,
        DltTraceBufType trace_buf_type)
{
    int ret = DLT_RETURN_ERROR;
    DltExtBuff *pDltExtBuff;

    /* get ringbuffer info table and link to injection_table */
    pDltExtBuff = malloc(sizeof(DltExtBuff));
    if (pDltExtBuff == NULL)
    {
        return ret;
    }
    dlt_user.dlt_ll_ts[handle->log_level_pos].DltExtBuff_ptr = pDltExtBuff;

    /* Store context id and filename */
    pDltExtBuff->ctid[4] = 0;
    dlt_set_id(pDltExtBuff->ctid, contextid);
    dlt_ext_decide_log_filename(pDltExtBuff);

    /* ring buffer size decide */
    if (trace_buf_type == DLT_TRACE_BUF_SMALL)
    {
        pDltExtBuff->size = DLT_EXT_BUF_SIZE_SMALL * dlt_nw_trace_hp_cfg;
    }
    else
    {
        pDltExtBuff->size = DLT_EXT_BUF_SIZE_LARGE * dlt_nw_trace_hp_cfg;
    }

    /* create empty file and mmap */
    ret = dlt_ext_make_ringbuf(pDltExtBuff);
    if (ret != DLT_RETURN_OK)
    {
        dlt_user.dlt_ll_ts[handle->log_level_pos].DltExtBuff_ptr =
                (DltExtBuff*) NULL;
        free(pDltExtBuff);
        return ret;
    }

    /* set ringbuf header */
    dlt_ext_set_ringbuf_header(pDltExtBuff);

    return ret;
}

int dlt_register_context_hp(DltContext *handle,
                            const char *contextid,
                            const char *description,
                            DltTraceBufType trace_buf_type)
{
    char *env_p;
    int ret = DLT_RETURN_ERROR;

    /* get configuration at once */
    DLT_SEM_LOCK();
    if(dlt_nw_trace_hp_cfg == -1)
    {
        env_p = getenv(DLT_NW_TRACE_HP_CFG_ENV);
        if(env_p != NULL)
        {
            dlt_nw_trace_hp_cfg = atoi(env_p);
            if((dlt_nw_trace_hp_cfg > DLT_EXT_BUF_MAX_NUM)
                || (dlt_nw_trace_hp_cfg < DLT_EXT_BUF_DEFAULT_NUM))
            {
                dlt_log(LOG_DEBUG,"configuration invalid value, set default.\n");
                dlt_nw_trace_hp_cfg = DLT_EXT_BUF_DEFAULT_NUM;
            }
        }
        else
        {
            dlt_nw_trace_hp_cfg = DLT_EXT_BUF_DEFAULT_NUM;
        }
    }
    DLT_SEM_FREE();

    if(dlt_nw_trace_hp_cfg != 0)
    {
        DLT_SEM_LOCK();
        /* check hp ctid limit */
        if(dlt_hp_ctid_cnt >= DLT_EXT_CTID_MAX)
        {
            DLT_SEM_FREE();
            dlt_log(LOG_ERR,"no more entry new hp ctid -> return.\n");
            return ret;
        }
        dlt_hp_ctid_cnt++;
        DLT_SEM_FREE();

        /* check first API call */
        if(dlt_hp_ctid_cnt == 1)
        {
            dlt_log(LOG_DEBUG,"first API call\n");
            /* at first API call, regist INJECTION callback */
            dlt_register_context(&context_injection_callback,
                                 DLT_CT_EXT_CB,
                                 "DLT Extention API Injection Callback");
            dlt_register_injection_callback(&context_injection_callback,
                                            DLT_EX_INJECTION_CODE,
                                            dlt_ext_injection_macro_callback);
        }
        /* register context */
        ret = dlt_register_context(handle, contextid, description);
        if(ret == DLT_RETURN_OK)
        {
            /* make ringbuffer */
            ret = dlt_ext_ringbuf_init(handle, contextid, trace_buf_type);
            if(ret != DLT_RETURN_OK)
            {
                /* count down hp context */
                dlt_hp_ctid_cnt--;
            }
        }
    }
    else
    {
        /* register context */
        ret = dlt_register_context(handle, contextid, description);
    }
    return ret;
}

/* Callback function */
void dlt_ext_fix_hp_log(DltExtBuff *pDltExtBuff, uint8_t count)
{
    int ret = DLT_RETURN_ERROR;
    char log_filepath[DLT_EXT_CTID_LOG_FILEPATH_LEN + 1];
    char fix_filepath[DLT_EXT_CTID_FIX_FILEPATH_LEN + 1];

    DLT_SEM_LOCK();
    /* munmap ringbuffer */
    munmap(pDltExtBuff->addr, pDltExtBuff->size);

    /* change extension */
    dlt_ext_get_log_filepath(log_filepath, pDltExtBuff->log_filename,
            DLT_EXT_CTID_LOG_FILE_EXTENSION);
    dlt_ext_get_log_filepath(fix_filepath, pDltExtBuff->log_filename,
            DLT_EXT_CTID_FIX_FILE_EXTENSION);
    rename(log_filepath, fix_filepath);

    /* create next empty file and mmap */
    ret = dlt_ext_make_ringbuf(pDltExtBuff);
    if (ret != DLT_RETURN_OK)
    {
        dlt_user.dlt_ll_ts[count].DltExtBuff_ptr = (DltExtBuff*) NULL;
        free(pDltExtBuff);
        DLT_SEM_FREE();
        return;
    }

    /* set ringbuf header */
    dlt_ext_set_ringbuf_header(pDltExtBuff);
    DLT_SEM_FREE();
    return;
}

int dlt_ext_injection_macro_callback(uint32_t service_id, void *data,
        uint32_t length)
{
    int ret;
    char *ctid_p;
    uint8_t count;
    uint32_t remain;
    char target_ctid[DLT_ID_SIZE + 1];
    uint8_t read_count;
    DltExtBuff *pDltExtBuff;

    /* check service_id */
    if (service_id != DLT_EX_INJECTION_CODE)
    {
        dlt_log(LOG_ERR, "service_id is invalid -> return.\n");
        return DLT_RETURN_ERROR;
    }

    /* check callback parameters */
    if ((data == NULL ) || (length == 0))
    {
        dlt_log(LOG_ERR, "parameter fail -> return.\n");
        return DLT_RETURN_ERROR;
    }

    /* target ctid check */
    ctid_p = (char *) data;
    remain = length;
    if (ctid_p[0] == DLT_EXT_CTID_WILDCARD)
    {
        /* wildcard */
        dlt_log(LOG_DEBUG, "all ctid stop\n");

        for (count = 0; count < dlt_user.dlt_ll_ts_num_entries; count++)
        {
            if ((dlt_user.dlt_ll_ts[count].nrcallbacks == 0)
                    && (dlt_user.dlt_ll_ts[count].DltExtBuff_ptr != NULL ))
            {
                pDltExtBuff = dlt_user.dlt_ll_ts[count].DltExtBuff_ptr;
                dlt_ext_fix_hp_log(pDltExtBuff, count);
            }
        }
    }
    else
    {
        do
        {
            if (ctid_p[0] == '\0')
            {
                remain--;
                ctid_p++;
            }
            else
            {
                read_count = 0;
                do
                {
                    remain--;
                    read_count++;
                    if (ctid_p[read_count] == '\0')
                    {
                        break;
                    }
                }
                while (remain);
                if (read_count <= DLT_ID_SIZE)
                {
                    memset(target_ctid, 0, (DLT_ID_SIZE + 1));
                    memcpy(target_ctid, ctid_p, read_count);
                    /* check target ctid running */
                    for (count = 0; count < dlt_user.dlt_ll_ts_num_entries;
                            count++)
                    {
                        if ((dlt_user.dlt_ll_ts[count].nrcallbacks == 0)
                                && (dlt_user.dlt_ll_ts[count].DltExtBuff_ptr
                                        != NULL ))
                        {
                            pDltExtBuff =
                                    dlt_user.dlt_ll_ts[count].DltExtBuff_ptr;
                            ret = strncmp(target_ctid, pDltExtBuff->ctid,
                                    DLT_ID_SIZE);
                            if (ret == 0)
                            {
                                dlt_log(LOG_DEBUG, "target ctid stop\n");
                                dlt_ext_fix_hp_log(pDltExtBuff, count);
                            }
                        }
                    }
                }
                ctid_p += read_count;
            }
        }
        while (remain);
    }
    return DLT_RETURN_OK;
}


/* Trace functions */
int dlt_ext_parameter_check(DltContext *handle,
        DltNetworkTraceType nw_trace_type, uint16_t header_len, void *header,
        uint16_t payload_len, void *payload)
{
    if (handle == NULL)
    {
        dlt_log(LOG_ERR, "handle error\n");
        return DLT_RETURN_ERROR;
    }

    if ((nw_trace_type != DLT_NW_TRACE_HP0)
            && (nw_trace_type != DLT_NW_TRACE_HP1))
    {
        dlt_log(LOG_ERR, "nw_trace_type error\n");
        return DLT_RETURN_ERROR;
    }

    if (header_len != 0)
    {
        if (header == NULL)
        {
            dlt_log(LOG_ERR, "header error\n");
            return DLT_RETURN_ERROR;
        }
    }

    if (payload_len != 0)
    {
        if (payload == NULL)
        {
            dlt_log(LOG_ERR, "payload error\n");
            return DLT_RETURN_ERROR;
        }
    }
    return DLT_RETURN_OK;
}

void dlt_ext_write_ring_buff_set_data(void *RingBuffHead_p,
        uint32_t RingBuffSize, uint32_t *write_count, void *data_p,
        uint32_t data_len)
{
    void *wp;
    uint32_t remain_buff_size;
    void *divide_data_p;
    uint32_t divide_data_len;

    wp = RingBuffHead_p + *write_count;
    remain_buff_size = RingBuffSize - *write_count;

    if (data_len > remain_buff_size)
    {
        dlt_log(LOG_DEBUG, "step over end of buf!\n");

        /* set the first half of data to bottom */
        memcpy(wp, data_p, remain_buff_size);
        wp = RingBuffHead_p;
        divide_data_p = data_p + remain_buff_size;
        divide_data_len = data_len - remain_buff_size;

        /* set the remain data to top */
        memcpy(wp, divide_data_p, divide_data_len);
        *write_count = divide_data_len;
    }
    else
    {
        /* set normally */
        memcpy(wp, data_p, data_len);
        *write_count += data_len;
    }

    return;
}

void dlt_ext_write_ring_buff(DltExtBuff *pDltExtBuff,
                             DltNetworkTraceType nw_trace_type,
                             uint16_t header_len,
                             void *header,
                             uint16_t payload_len,
                             void *payload)
{
    DltExtBuffHeader *pBufHeader;
    void *RingBuffHead_p;
    uint32_t RingBuffSize;
    char DltStorageheader[4] = { 0x44, 0x4C, 0x54, 0x01 };
    struct timeval tv;
    struct timezone tz;
    uint16_t ringbuf_log_len;
    uint32_t seid;
    uint32_t tsmp;
    uint16_t maxlen;
    uint16_t write_header_len;
    uint16_t write_payload_len;

    pBufHeader = (DltExtBuffHeader *) pDltExtBuff->addr;
    RingBuffHead_p = ((void*) pBufHeader + sizeof(DltExtBuffHeader));
    RingBuffSize = pDltExtBuff->size - sizeof(DltExtBuffHeader);

    /* get pid */
    seid = getpid();
    seid = DLT_HTOBE_32(seid);

    /* get current time */
    gettimeofday(&tv, &tz);
    tsmp = dlt_uptime();
    tsmp = DLT_HTOBE_32(tsmp);

    /* check nw_trace_type */
    if (nw_trace_type == DLT_NW_TRACE_HP0)
    {
        maxlen = DLT_EXT_BUF_LOGMAX_HP0;
    }
    else
    {
        maxlen = DLT_EXT_BUF_LOGMAX_HP1;
    }

    /* calculate length */
    if (header_len > maxlen)
    {
        write_header_len = maxlen;
        write_payload_len = 0;
    }
    else if ((header_len + payload_len) > maxlen)
    {
        write_header_len = header_len;
        write_payload_len = maxlen - header_len;
    }
    else
    {
        write_header_len = header_len;
        write_payload_len = payload_len;
    }
    ringbuf_log_len = (sizeof(DltStandardHeader)
            + DLT_STANDARD_HEADER_EXTRA_SIZE(pBufHeader->htyp)
            + sizeof(DltExtendedHeader) + sizeof(uint32_t) + sizeof(header_len)
            + write_header_len + sizeof(uint32_t) + sizeof(payload_len)
            + write_payload_len);
    ringbuf_log_len = (((((ringbuf_log_len) >> 8) & 0xff)
            | (((ringbuf_log_len) << 8) & 0xff00)));

    /* Set DltStorageHeader */
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, DltStorageheader,
            sizeof(DltStorageheader));
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, &tv.tv_sec, sizeof(int32_t));
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, &tv.tv_usec, sizeof(int32_t));

    /* Set DltStandardHeader */
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, &ringbuf_log_len,
            sizeof(ringbuf_log_len));

    /* Set DltStandardHeaderExtra */
    if (DLT_IS_HTYP_WSID(pBufHeader->htyp))
    {
        dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
                &pBufHeader->write_count, &seid, sizeof(seid));
    }
    if (DLT_IS_HTYP_WTMS(pBufHeader->htyp))
    {
        dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
                &pBufHeader->write_count, &tsmp, sizeof(tsmp));
    }

    /* Set User LogData */
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, &write_header_len,
            sizeof(write_header_len));
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, header, write_header_len);
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, &write_payload_len,
            sizeof(write_payload_len));
    dlt_ext_write_ring_buff_set_data(RingBuffHead_p, RingBuffSize,
            &pBufHeader->write_count, payload, write_payload_len);

    return;
}

void dlt_user_trace_network_hp(DltContext *handle,
        DltNetworkTraceType nw_trace_type, uint16_t header_len, void *header,
        uint16_t payload_len, void *payload)
{
    DltExtBuff *pDltExtBuff;
    int ret = 0;

    /* DLT_TRACE_NETWORK() */
    if (header == 0)
    {
        ret = dlt_user_trace_network(handle, nw_trace_type, payload_len,
                payload, 0, 0);
    }
    else
    {
        nw_trace_type |= DLT_NW_TRACE_ASCII_OUT;
        ret = dlt_user_trace_network(handle, nw_trace_type, header_len, header,
                0, 0);
        nw_trace_type &= ~DLT_NW_TRACE_ASCII_OUT;
        ret = dlt_user_trace_network(handle, nw_trace_type, payload_len,
                payload, 0, 0);
    }
    if (ret != 0)
    {
        return;
    }

    pDltExtBuff = dlt_user.dlt_ll_ts[handle->log_level_pos].DltExtBuff_ptr;
    if (pDltExtBuff != NULL)
    {
        /* check paramters */
        if (dlt_ext_parameter_check(handle, nw_trace_type, header_len, header,
                payload_len, payload) != DLT_RETURN_OK)
        {
            return;
        }
        /* ring buffer write */
        DLT_SEM_LOCK();
        dlt_ext_write_ring_buff(pDltExtBuff, nw_trace_type, header_len, header,
                payload_len, payload);
        DLT_SEM_FREE();
    }
    return;
}
