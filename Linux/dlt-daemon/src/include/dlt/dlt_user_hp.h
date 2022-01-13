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
 * \file: dlt_user_hp.h
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt_user_hp.h                                                 **
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
**  sh          Syed Hameed                ADIT                               **
*******************************************************************************/
#ifndef DLT_USER_EXT_H
#define DLT_USER_EXT_H

/* network trace type extend for ASCII output */
#define DLT_NW_TRACE_ASCII_OUT 0x80

/* about external api */
#define DLT_NW_TRACE_HP_CFG_ENV             "DLT_NW_TRACE_HP_CFG"
#define DLT_CT_EXT_CB                       "0EXT"
#define DLT_EX_INJECTION_CODE               4096
#define DLT_EXT_CTID_MAX                    7

/* about INJECTION */
#define DLT_EXT_CTID_WILDCARD               0x2A

/* about string length */
#ifdef __linux__
#define DLT_EXT_PID_MAX_LEN                 5
#else
#define DLT_EXT_PID_MAX_LEN                 10
#endif
#define DLT_EXT_BKNUM_LEN                   3

/* about ring buffer */
#define DLT_EXT_BUF_SIZE_SMALL              4096
#define DLT_EXT_BUF_SIZE_LARGE              8192
#define DLT_EXT_BUF_LOGMAX_HP0              100
#define DLT_EXT_BUF_LOGMAX_HP1              200
#define DLT_EXT_BUF_MAX_NUM                 5
#define DLT_EXT_BUF_DEFAULT_NUM             0

/* about log file */
#define DLT_EXT_CTID_MKDIR_MODE             (0755)
#define DLT_EXT_CTID_DLT_DIRECTORY          "/tmp/log/dlt"
#define DLT_EXT_CTID_LOG_DIRECTORY          "/tmp/log/dlt/hp/"
#define DLT_EXT_CTID_LOG_FILE_PREFIX        "dlt"
#define DLT_EXT_CTID_LOG_FILE_DELIMITER     "_IDis"
#define DLT_EXT_CTID_LOG_FILE_BKNUM_DELI    "_"
#define DLT_EXT_CTID_LOG_PERIOD             "."
#define DLT_EXT_CTID_LOG_FILE_EXTENSION     ".log"
#define DLT_EXT_CTID_FIX_FILE_EXTENSION     ".fix"
#define DLT_EXT_BKNUM_MAX                   999
#define DLT_EXT_CTID_LOG_FILENAME_LEN       (sizeof(DLT_EXT_CTID_LOG_FILE_PREFIX)-1    \
                                            +sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER)-1 \
                                            +DLT_ID_SIZE                               \
                                            +sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER)-1 \
                                            +DLT_ID_SIZE                               \
                                            +sizeof(DLT_EXT_CTID_LOG_FILE_DELIMITER)-1 \
                                            +DLT_EXT_PID_MAX_LEN)                      \
                                            +sizeof(DLT_EXT_CTID_LOG_FILE_BKNUM_DELI)-1 \
                                            +DLT_EXT_BKNUM_LEN
#define DLT_EXT_CTID_LOG_FILEPATH_LEN       (sizeof(DLT_EXT_CTID_LOG_DIRECTORY)-1      \
                                            +DLT_EXT_CTID_LOG_FILENAME_LEN             \
                                            +sizeof(DLT_EXT_CTID_LOG_FILE_EXTENSION)-1)
#define DLT_EXT_CTID_LOG_OLD_FILEPATH_LEN   (sizeof(DLT_EXT_CTID_LOG_DIRECTORY)-1      \
                                            +DLT_EXT_CTID_LOG_FILENAME_LEN             \
                                            +sizeof(DLT_EXT_CTID_LOG_OLD_FILE_EXTENSION)-1)
#define DLT_EXT_CTID_FIX_FILEPATH_LEN       (sizeof(DLT_EXT_CTID_LOG_DIRECTORY)-1      \
                                            +DLT_EXT_CTID_LOG_FILENAME_LEN             \
                                            +sizeof(DLT_EXT_CTID_FIX_FILE_EXTENSION)-1)

/* about log header */
#define DLT_EXT_LOG_HEADER_HYTP             (DLT_HTYP_WEID|DLT_HTYP_WTMS|DLT_HTYP_PROTOCOL_VERSION1|DLT_HTYP_UEH)
#define DLT_EXT_LOG_HEADER_MSIN             ((DLT_TYPE_NW_TRACE << DLT_MSIN_MSTP_SHIFT)          \
                                            |(DLT_NW_TRACE_USER_DEFINED0 << DLT_MSIN_MTIN_SHIFT) \
                                            |DLT_MSIN_VERB )
#define DLT_EXT_LOG_HEADER_NOAR             2

/**
 * Definitions for type of trace type/DLT_TRACE_NETWORK_HP()
 */
/* network trace type redefine */
#define DLT_NW_TRACE_HP0 DLT_NW_TRACE_USER_DEFINED0
#define DLT_NW_TRACE_HP1 DLT_NW_TRACE_USER_DEFINED1

/**
 * Definitions for type of ring buffer/DLT_REGISTER_CONTEXT_HP()
 */
typedef enum
{
        DLT_TRACE_BUF_SMALL = 0x01,
        DLT_TRACE_BUF_LARGE
} DltTraceBufType;

typedef struct {
    char        ctid[DLT_ID_SIZE+1];
    void        *addr;
    uint32_t    size;
    char        log_filename[DLT_EXT_CTID_LOG_FILENAME_LEN+1];
} DltExtBuff;

typedef struct {
    uint32_t write_count;
    uint32_t size;
    char ecuid[DLT_ID_SIZE];
    char htyp;
    char msin;
    char noar;
    char reserve[1];
    char apid[DLT_ID_SIZE];
    char ctid[DLT_ID_SIZE];
    uint32_t type_info_header;
    uint32_t type_info_payload;
} DltExtBuffHeader;

#endif /* DLT_USER_EXT_H */
