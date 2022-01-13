/**
 * @licence app begin@
 * Copyright (C) 2016  Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch and DENSO.
 *
 * This file is part of GENIVI Project Dlt - Diagnostic Log and Trace console apps.
 *
 *
 * \copyright
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL was not distributed with
 * this file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 *
 * \author Christoph Lipka <clipka@jp.adit-jv.com> ADIT 2016
 *
 * \file dlt-filter-ctrl.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt-filter-ctrl.c                                             **
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include "dlt_protocol.h"
#include "dlt_client.h"
#include "dlt-control-common.h"
#include "dlt_daemon_connection_types.h"
#include "dlt_daemon_filter_types.h"

/* define borders for min and max filter configuration level */
#define LEVEL_MIN   0
#define LEVEL_MAX 100
#define UNDEFINED 999

#define DLT_MAX_RESPONSE_LENGTH 32

#define DLT_MAX_STRING_SIZE 1024

static struct LogstorageOptions {
    unsigned int command; /**< filter control command */
    unsigned int level; /**< new filter configuration level */
    long timeout; /**< Default timeout */
} g_options = {
    .command = UNDEFINED,
    .level = UNDEFINED,
};

static unsigned int get_command(void)
{
    return g_options.command;
}

static void set_command(unsigned int c)
{
    g_options.command = c;
}

static unsigned int  get_level(void)
{
    return g_options.level;
}

static void set_level(unsigned int l)
{
    if (l > LEVEL_MAX)
    {
        pr_error("Filter level invalid\n");
        exit(-1);
    }

    g_options.level = l;
    set_command(DLT_SERVICE_ID_SET_FILTER_LEVEL);
}

/**
 * @brief Get bit in Control Message bit mask
 *
 * @param flags DltServiceIdFlags
 * @param id    set bit for given id
 * @return 0,1 on success, -1 otherwise
 */
static int bit(DltServiceIdFlag *flags, int id)
{
    int is_upper;
    int byte_pos;
    int bit_pos;
    int tmp;

    if (flags == NULL)
    {
        pr_error("%s: invalid arguments\n", __FUNCTION__);
        return -1;
    }

    if ((id <= DLT_SERVICE_ID) || (id >= DLT_USER_SERVICE_ID_LAST_ENTRY) ||
       (id >= DLT_SERVICE_ID_LAST_ENTRY && id <= DLT_USER_SERVICE_ID))
    {
        pr_error("Given ID = %d is invalid\n", id);
        return -1;
    }

    is_upper = id & 0xF00;
    tmp = id & 0xFF;

    byte_pos = tmp >> 3; //  tmp / 8;
    bit_pos = tmp & 7; // tmp % 8;

    if (is_upper)
    {
        return BIT(flags->upper[byte_pos], bit_pos);
    }
    else
    {
        return BIT(flags->lower[byte_pos], bit_pos);
    }
}

/**
 * @brief Convert level range to string.
 *
 * @param str       String to store clients in
 * @param level_min Minimum filter level
 * @param level_max Maximum filter level
 */
DLT_STATIC void dlt_filter_level_range_to_string(char *str, unsigned int level_min, unsigned int level_max)
{
    if (str == NULL)
    {
        return;
    }

    if (level_min == level_max)
    {
        sprintf(str, "%d", level_min);
    }
    else
    {
        if (level_max == DLT_FILTER_LEVEL_MAX)
        {
            sprintf(str, "%d%s", level_min, "-");
        }
        else
        {
            sprintf(str, "%d%s%d", level_min, "-", level_max);
        }
    }
}

/**
 * @brief Convert mask to human readable string.
 *
 * @param str   String to store clients in
 * @param mask Client mask
 */
static void dlt_filter_client_mask_to_string(char *str, unsigned int mask)
{
    if (str == NULL)
    {
        return;
    }

    if (mask == DLT_FILTER_CLIENT_CONNECTION_DEFAULT_MASK)
    {
        strncpy(str, "\t" DLT_FILTER_CLIENT_NONE, strlen("\t" DLT_FILTER_CLIENT_NONE));
    }
    else if (mask == DLT_CON_MASK_ALL)
    {
        strncpy(str, "\t" DLT_FILTER_CLIENT_ALL, strlen("\t" DLT_FILTER_CLIENT_ALL));
    }
    else
    {
        if (mask & DLT_CON_MASK_CLIENT_CONNECT)
        {
            strcat(str, "\tTCP\n");
        }

        if (mask & DLT_CON_MASK_CLIENT_MSG_SERIAL)
        {
            strcat(str, "\tSerial\n");
        }

        if (mask & DLT_CON_MASK_CLIENT_MSG_OFFLINE_TRACE)
        {
            strcat(str, "\tOfflineTrace\n");
        }

        if (mask & DLT_CON_MASK_CLIENT_MSG_OFFLINE_LOGSTORAGE)
        {
            strcat(str, "\tOfflineLogstorage\n");
        }
    }
}

/**
 * @brief Compare none or all flags and convert to human readable string.
 *
 * @param str   String to store control message names
 * @param flag1 DltServiceIdFlag
 * @param flag2 DltServiceIdFlag (expected none or all)
 * @param text  String to print
 *
 * @return 0 if equal, otherwise -1
 */
DLT_STATIC int dlt_filter_control_mask_compare_flags_to_string(char *str,
                                                           DltServiceIdFlag *flag1,
                                                           DltServiceIdFlag *flag2,
                                                           char *text)
{
    if ((str == NULL) || (flag1 == NULL) || (flag2 == NULL) || (text == NULL))
    {
        return -1;
    }

    if ((memcmp(flag1->upper, flag2->upper, sizeof(flag1->upper)) == 0) &&
        (memcmp(flag1->lower, flag2->lower, sizeof(flag1->lower)) == 0))
    {
        strcat(str, "\t");
        strcat(str, text);
        strcat(str, "\n");
        return 0;
    }

    return -1;
}

/**
 * @brief Convert control message mask to human readable string.
 *
 * @param str   String to store control message names
 * @param mask Control mask
 */
static void dlt_filter_control_mask_to_string(char *str, DltServiceIdFlag *flags)
{
    DltServiceIdFlag none = { 0 };
    DltServiceIdFlag all;
    memset(&all, 0xFF, sizeof(DltServiceIdFlag));

    int i = 0;

    if ((str == NULL) || (flags == NULL))
    {
        return;
    }

    if ((dlt_filter_control_mask_compare_flags_to_string(str,
                                                         flags,
                                                         &none,
                                                         DLT_FILTER_CLIENT_NONE) != 0))
    {
        if (dlt_filter_control_mask_compare_flags_to_string(str,
                                                            flags,
                                                            &all,
                                                            DLT_FILTER_CLIENT_ALL) != 0)
        {
            for (i = DLT_SERVICE_ID_SET_LOG_LEVEL; i < DLT_SERVICE_ID_LAST_ENTRY; i++)
            {
                if (bit(flags, i) == 1)
                {
                    strcat(str, "\t");
                    strcat(str, dlt_get_service_name(i));
                    strcat(str, "\n");
                }
            }

            for (i = DLT_SERVICE_ID_UNREGISTER_CONTEXT; i < DLT_USER_SERVICE_ID_LAST_ENTRY; i++)
            {
                if (bit(flags, i) == 1)
                {
                    strcat(str, "\t");
                    strcat(str, dlt_get_service_name(i));
                    strcat(str, "\n");
                }
            }
        }
    }
}

/**
 * @brief Print filter status information
 *
 * @param info DltServiceGetCurrentFilterInfo
 */
static void dlt_filter_print_filter_status(DltServiceGetCurrentFilterInfo *info)
{
	char level_range[DLT_MAX_STRING_SIZE] = {'\0'};
    char clients[DLT_MAX_STRING_SIZE] = {'\0'};
    char control[DLT_MAX_STRING_SIZE] = {'\0'};
    DltServiceIdFlag flags;

    if (info == NULL)
    {
        fprintf(stderr,
                "Wrong arguments for %s\n"
                "Cannot print filter configuration\n", __func__);
        return;
    }

    memcpy(&flags.lower, info->ctrl_mask_lower, sizeof(flags.lower));
    memcpy(&flags.upper, info->ctrl_mask_upper, sizeof(flags.upper));

    dlt_filter_level_range_to_string(level_range,
                                     info->level_min,
                                     info->level_max);
    dlt_filter_client_mask_to_string(clients, info->client_mask);
    dlt_filter_control_mask_to_string(control, &flags);

    fprintf(stdout, "-----------------------------\n");
    fprintf(stdout, "Current filter configuration:\n");
    fprintf(stdout, "-----------------------------\n");
    fprintf(stdout, "Name:\n");
    fprintf(stdout, "\t%s\n", info->name);
    fprintf(stdout, "Level:\n");
    fprintf(stdout, "\t%s\n", level_range);
    fprintf(stdout, "Clients:\n");
    fprintf(stdout, "%s\n", clients);
    fprintf(stdout, "Control:\n");
    fprintf(stdout, "%s\n", control);
    fprintf(stdout, "Injections:\n");
    fprintf(stdout, "\t%s\n", info->injections);
    fprintf(stdout, "-----------------------------\n");
}

/**
 * @brief Analyze received DLT Daemon response
 *
 * This function checks the received message. In particular, it checks the
 * answer string 'service(<ID>, {ok, error, perm_denied})'. In any casae the
 * g_callback_return variable will be set as well which is evaluated in the
 * main function after the communication thread returned.
 *
 * @param message   Received DLT Message
 * @return 0 if daemon returns 'ok' message, -1 otherwise
 */
static int dlt_filter_analyze_response(char *answer, void *payload, int len)
{
    int ret = -1;
    char resp_ok[DLT_MAX_RESPONSE_LENGTH] = { 0 };
    DltServiceGetCurrentFilterInfo *info = NULL;

    if ((answer == NULL) || (payload == NULL))
    {
        return -1;
    }

    snprintf(resp_ok, DLT_MAX_RESPONSE_LENGTH, "service(%u), ok", get_command());

    pr_verbose("Response received: '%s', Expected: '%s'\n", answer, resp_ok);

    if (strncmp(answer, resp_ok, strlen(resp_ok)) == 0)
    {
        ret = 0;

        if (get_command() == DLT_SERVICE_ID_GET_FILTER_STATUS)
        {
            if ((int)sizeof(DltServiceGetCurrentFilterInfo) > len)
            {
                pr_error("Received payload is smaller than expected\n");
                pr_verbose("Expected: %lu; received: %d\n",
                           (unsigned long) sizeof(DltServiceGetCurrentFilterInfo),
                           len);

                ret = -1;
            }
            else
            {
                info = (DltServiceGetCurrentFilterInfo *) payload;
                if (info == NULL)
                {
                    pr_error("Received response is NULL\n");
                    return -1;
                }

                dlt_filter_print_filter_status(info);
            }
        }
    }

    return ret;
}

/**
 * @brief Prepare message body to be send to DLT Daemon
 *
 * @return Pointer ot DltControlMsgBody, NULL otherwise
 */
static DltControlMsgBody *dlt_filter_prepare_message_body(void)
{
    DltControlMsgBody *mb = calloc(1, sizeof(*mb));

    if (mb == NULL)
    {
        return NULL;
    }

    if (get_command() == DLT_SERVICE_ID_SET_FILTER_LEVEL)
    {
        DltServiceSetFilterLevel *serv = NULL;

        mb->data = calloc(1, sizeof(*serv));

        if (mb->data == NULL)
        {
            free(mb);
            return NULL;
        }

        mb->size = sizeof(DltServiceSetFilterLevel);
        serv = (DltServiceSetFilterLevel *) mb->data;
        serv->service_id = DLT_SERVICE_ID_SET_FILTER_LEVEL;
        serv->level = get_level();
    }
    else
    {
        DltServiceGetCurrentFilterInfo *serv = NULL;

        mb->data = calloc(1, sizeof(*serv));
        if (mb->data == NULL)
        {
            free(mb);
            return NULL;
        }

        mb->size = sizeof(DltServiceGetCurrentFilterInfo);
        serv = (DltServiceGetCurrentFilterInfo *) mb->data;
        serv->service_id = DLT_SERVICE_ID_GET_FILTER_STATUS;
    }

    return mb;
}

/**
 * @brief Send a single command to DLT daemon and wait for response
 *
 * @return 0 on success, -1 on error
 */
static int dlt_filter_ctrl_single_request()
{
    int ret = -1;

    /* Initializing the communication with the daemon */
    if (dlt_control_init(dlt_filter_analyze_response,
                         get_ecuid(),
                         get_verbosity()) != 0)
    {
        pr_error("Failed to initialize connection with the daemon.\n");
        return ret;
    }

    /* prepare message body */
    DltControlMsgBody *msg_body = dlt_filter_prepare_message_body();

    if (msg_body == NULL)
    {
        pr_error("Data for Dlt Message body is NULL\n");
        return -1;
    }

    ret = dlt_control_send_message(msg_body, get_timeout());

    free(msg_body->data);
    free(msg_body);

    dlt_control_deinit();

    return ret;
}

static void usage()
{
    printf("Usage: dlt-filter-ctrl [options]\n");
    printf("Send a trigger to DLT daemon to set the filter configuration "
           "or print filter status information\n");
    printf("\n");
    printf("Options:\n");
    printf("  -e         Set ECU ID (Default: %s\n", DLT_CTRL_DEFAULT_ECUID);
    printf("  -h         Usage\n");
    printf("  -l         Set filter level [0..100]\n");
    printf("  -s         Get filter status information\n");
    printf("  -t         Specify connection timeout (Default: %ds)\n",
            DLT_CTRL_TIMEOUT);
    printf("  -v         Set verbose flag (Default:%d)\n", get_verbosity());
}

/**
 * @brief Parse application arguments
 *
 * The arguments are parsed and saved in static structure for future use.
 *
 * @param argc  amount of arguments
 * @param argv  argument table
 * @return 0 on success, -1 otherwise
 */
static int parse_args(int argc, char *argv[])
{
    int c = 0;
    unsigned long level;
    char *ptr = NULL;

    /* Get command line arguments */
    opterr = 0;

    while ((c = getopt(argc, argv, "t:he:l:sv")) != -1)
    {
        switch(c)
        {
        case 't':
            set_timeout(strtol(optarg, NULL, 10));
            break;
        case 'h':
            usage();
            return -1;
        case 'e':
            set_ecuid(optarg);
            break;
        case 'l':
            level = strtoul(optarg, &ptr, 10);
            if (ptr == optarg)
            {
                printf("Level %s is not a number\n", optarg);
                return -1;
            }
            else
            {
                set_level((unsigned int)level);
            }
            break;
        case 's':
            set_command(DLT_SERVICE_ID_GET_FILTER_STATUS);
            break;
        case 'v':
            set_verbosity(1);
            pr_verbose("Now in verbose mode.\n");
            break;
        case '?':
            if ((optopt == 'c') || (optopt == 'l') || (optopt == 'e') || (optopt == 't'))
            {
                pr_error("Option -%c requires an argument.\n", optopt);
            }
            else if (isprint(optopt))
            {
                pr_error("Unknown option -%c.\n", optopt);
            }
            else
            {
                pr_error("Unknown option character \\x%x.\n", optopt);
            }

            usage();
            return -1;
        default:
            pr_error("Try %s -h for more information.\n", argv[0]);
            usage();
            return -1;
        }
    }

    return 0;
}

/**
 * @brief Entry point
 *
 * Execute the argument parser and call the main feature accordingly
 *
 * @param argc  amount of arguments
 * @param argv  argument table
 * @return 0 on success, -1 otherwise
 */
int main(int argc, char *argv[])
{
    int ret = 0;

    set_ecuid(NULL);
    set_timeout(DLT_CTRL_TIMEOUT);

    /* Get command line arguments */
    if (parse_args(argc, argv) != 0)
    {
        return -1;
    }

    if (get_command() == UNDEFINED)
    {
        pr_error("Neither -l nor -s specified!\n");
        usage();
        return -1;
    }

    pr_verbose("Sending command to DLT daemon.\n");
    /* one shot request */
    ret = dlt_filter_ctrl_single_request();

    pr_verbose("Exiting.\n");
    return ret;
}
