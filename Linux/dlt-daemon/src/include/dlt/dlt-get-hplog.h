/**
 * @licence app begin@
 * Copyright (C) 2013 - 2015  Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * DLT HP log functionality header file.
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
 * \file: dlt-get-hplog.h
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt-get-hplog                                                 **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Syed Hameed shameed@jp.adit-jv.com                            **
**                                                                            **
**  PURPOSE   :                                                               **
**                                                                            **
**  REMARKS   :                                                               **
**                                                                            **
**  PLATFORM DEPENDANT [yes/no]: yes                                          **
**                                                                            **
**  TO BE CHANGED BY USER [yes/no]: no                                        **
**                                                                            **
*******************************************************************************/

/*******************************************************************************
**                      Author Identity                                       **
********************************************************************************
**                                                                            **
** Initials     Name                       Company                            **
** --------     -------------------------  ---------------------------------- **
**                                                                            **
**                                                                            **
*******************************************************************************/

#ifndef DLT_GET_HPLOG_H
#define DLT_GET_HPLOG_H

typedef enum
{
    DLT_GET_HPLOG_OPT1_NONE = 0,      /* default */
    DLT_GET_HPLOG_OPT1_INJECTION,     /* INJECTION request */
    DLT_GET_HPLOG_OPT1_KILL_DLT_ID,   /* kill request(APID CTID) */
    DLT_GET_HPLOG_OPT1_KILL_PID,      /* kill request(PID) */
    DLT_GET_HPLOG_OPT1_CONVERT        /* Log convert request */
} DltGetHplogOpt1;

typedef enum
{
    DLT_GET_HPLOG_OPT2_NONE = 0,      /* default */
    DLT_GET_HPLOG_OPT2_DLT_ID,        /* DLT ID */
    DLT_GET_HPLOG_OPT2_LIST           /* PID */
} DltGetHplogOpt2;

typedef enum
{
    DLT_GET_HPLOG_OPT3_NONE = 0,      /* default */
    DLT_GET_HPLOG_OPT3_ENABLE         /* output file specific */
} DltGetHplogOpt3;

typedef enum
{
    DLT_GET_HPLOG_PARSE_ID_FAIL = -1, /* ID len over, list count over */
    DLT_GET_HPLOG_PARSE_ID_OK,        /* parse OK */
    DLT_GET_HPLOG_PARSE_ID_EOF,       /* ID and EOF detect */
    DLT_GET_HPLOG_PARSE_EOF           /* only EOF detect */
} DltGetHplogParseIdResult;

#define DLT_EXT_WILD_CARD_TO_API    "*"
#define DLT_EXT_WILD_CARD           "----"
#define DLT_EXT_SLASH               "/"

#define DLT_HP_LOG_APID_NUM_MAX   150
#define DLT_HP_LOG_DATA_MAX       800    /* CTID MAX is same */

#define DLT_RECEIVE_TEXTBUFSIZE  ( ( DLT_HP_LOG_APID_NUM_MAX * 6 \
                                   + DLT_HP_LOG_DATA_MAX * 6      )\
                                   * 3 + 20 )                             /* Size of buffer for text output */
#define DLT_MAX_FILEPATH_LEN        1023
#define DLT_CONV_LIST_MAX_COUNT     256
#define DLT_SERVICE_ID_MAX_LEN      5
#define DLT_INJECTION_WAIT          1

#define DLT_EXT_CTID_DLT_FILE_EXTENSION     ".dlt"
#define DLT_EXT_CTID_TMP_FILE_EXTENSION     ".tmp"

#define DLT_EXT_SEEK_LEN_TO_HEADER_LEN              DLT_ID_SIZE + sizeof(int32_t) + sizeof(int32_t) + DLT_ID_SIZE + \
                                                    sizeof(uint8_t) + sizeof(uint8_t) + sizeof(uint16_t) + \
                                                    DLT_ID_SIZE + sizeof(uint32_t) + \
                                                    sizeof(uint8_t) + sizeof(uint8_t) + DLT_ID_SIZE + DLT_ID_SIZE + \
                                                    sizeof(uint32_t) + sizeof(uint16_t)
#define DLT_EXT_SEEK_LEN_TO_PAYLOAD_LEN(head_len)   DLT_EXT_SEEK_LEN_TO_HEADER_LEN + head_len + \
                                                    sizeof(uint32_t) + sizeof(uint16_t)

typedef struct {
    int         list_count;
    char        apid[DLT_CONV_LIST_MAX_COUNT][DLT_ID_SIZE+1];
    char        ctid[DLT_CONV_LIST_MAX_COUNT][DLT_ID_SIZE+1];
    int16_t     pid_value[DLT_CONV_LIST_MAX_COUNT];
} DltGetHplogConvList;

typedef struct {
    char        used;
    char        apid[DLT_ID_SIZE+1];
    int         ctid_count;
    char        ctid[DLT_EXT_CTID_MAX][DLT_ID_SIZE+1];
} InjectionInfo;

typedef struct {
    int           info_list_count;
    InjectionInfo info[DLT_CONV_LIST_MAX_COUNT];
} DltGetHplogInjectionInfoList;

/* if tools not merge to dlt-daemon */
#define HP_LOG_EMERG   0
#define HP_LOG_ALERT   1
#define HP_LOG_CRIT    2
#define HP_LOG_ERR     3
#define HP_LOG_WARNING 4
#define HP_LOG_NOTICE  5
#define HP_LOG_INFO    6
#define HP_LOG_DEBUG   7

#endif /* DLT_GET_HPLOG_H */
