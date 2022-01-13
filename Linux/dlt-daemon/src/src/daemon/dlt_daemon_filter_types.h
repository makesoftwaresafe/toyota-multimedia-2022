/*
 * @licence app begin@
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * This file is part of GENIVI Project DLT - Diagnostic Log and Trace.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License (MPL), v. 2.0.
 * If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*!
 * \author
 * Christoph Lipka <clipka@jp.adit-jv.com>
 *
 * \copyright Copyright © 2016 Advanced Driver Information Technology. \n
 * License MPL-2.0: Mozilla Public License version 2.0 http://mozilla.org/MPL/2.0/.
 *
 * \file dlt_daemon_filter_types.h
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt_daemon_filter_types.h                                     **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Christoph Lipka clipka@jp.adit-jv.com                         **
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
**  cl          Christoph Lipka            ADIT                               **
*******************************************************************************/


#ifndef _DLT_DAEMON_FILTER_TYPES_H
#define _DLT_DAEMON_FILTER_TYPES_H

#include "dlt_config_file_parser.h"
#include "dlt_daemon_connection_types.h"
#include "dlt_protocol.h"
#include "dlt_common.h"

#define DLT_FILTER_CONFIG_FILE CONFIGURATION_FILES_DIR "/dlt_message_filter.conf"

#define DLT_FILTER_BASIC_CONTROL_MESSAGE_MASK 0xCFFFFFFF

/* whatever connection always available: timers, msg, unix socket control (, app) */
#define DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK ( \
    DLT_CON_MASK_APP_MSG | \
    DLT_CON_MASK_APP_CONNECT | \
    DLT_CON_MASK_ONE_S_TIMER | \
    DLT_CON_MASK_SIXTY_S_TIMER | \
    DLT_CON_MASK_SYSTEMD_TIMER | \
    DLT_CON_MASK_CONTROL_CONNECT | \
    DLT_CON_MASK_CONTROL_MSG)

#define DLT_FILTER_CLIENT_ALL       "ALL"
#define DLT_FILTER_CLIENT_NONE      "NONE"

#define DLT_FILTER_LEVEL_MIN                0
#define DLT_FILTER_LEVEL_MAX                100
#define DLT_FILTER_CONFIGURATIONS_MAX       DLT_FILTER_LEVEL_MAX + 1
#define DLT_FILTER_INJECTION_CONFIG_MAX     50

/* get bit at position n */
#define BIT(x,n) (((x) >> (n)) & 1)
/* set bit at position n */
#define SET_BIT(x,n) ((x) |= (1 << n))

/* Structure defines flags for AUTOSAR Service IDs and user defined Service IDs.
 * The services ID are defined in dlt_protocol.h
*/
typedef struct
{
    unsigned char upper[DLT_NUM_USER_SERVICE_ID]; /* 0xF01..DLT_SERVICE_ID_USER_LAST_ENTRY */
    unsigned char lower[DLT_NUM_SERVICE_ID]; /* 0x01..DLT_SERVICE_ID_LAST_ENTRY */
} DltServiceIdFlag;

typedef struct
{
    char *name;         /* name of injection */
    char *apid;         /* application identifier */
    char *ctid;         /* context identifier */
    char *ecuid;        /* node identifier */
    int num_sevice_ids; /* number of service ids */
    int *service_ids;   /* list of service ids */
} DltInjectionConfig;

typedef struct DltFilterConfiguration
{
    char *name;                /* name of filter configuration */
    unsigned int level_min;    /* minimum level id of filter configuration*/
    unsigned int level_max;    /* maximum level id of filter configuration */
    unsigned int client_mask;  /* Mask of allowed clients */
    DltServiceIdFlag ctrl_mask;/* Mask of allowed control messages */
    char **injections;         /* list of injection messages names */
    int num_injections;        /* number of injections */
    struct DltFilterConfiguration *next; /* for multiple filter configuration
                                          * using linked list */
} DltFilterConfiguration;

typedef struct
{
    char *name; /* name of filter configuration */
    char *backend; /* name of filter control backend */
    unsigned int default_level; /* default level of message filter */
    DltFilterConfiguration *head; /* head pointer of configs */
    DltFilterConfiguration *configs;
    DltFilterConfiguration *current; /* active configuration */
    DltInjectionConfig injections[DLT_FILTER_INJECTION_CONFIG_MAX]; /* list of injections */
    int num_injections; /* number of specified injections */
} DltMessageFilter;

#endif
