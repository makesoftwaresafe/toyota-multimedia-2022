/**
 * @licence app begin@
 * Copyright (C) 2015, 2016  Advanced Driver Information Technology.
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
 * \author Syed Hameed <shameed@jp.adit-jv.com> ADIT 2015
 * \author Christoph Lipka <clipka@jp.adit-jv.com> ADIT 2016
 *
 * \file dlt-blockmode-ctrl.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt-blockmode-ctrl.c                                          **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Syed Hameed <shameed@jp.adit-jv.com>                          **
**            : Christoph Lipka <clipka@jp.adit-jv.com>                       **
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
**  SH          Syed Hameed                 ADIT                              **
**  CL          Christoph Lipka             ADIT                              **
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

#define UNDEFINED 999
#define MAX_RESPONSE_LENGTH 32

#define DLT_BLOCKING "BLOCKING"
#define DLT_NON_BLOCKING "NON-BLOCKING"

static struct BlockmodeOptions {
    char apid[DLT_ID_SIZE]; /**< set BM to specific application */
    unsigned int command; /**< filter control command */
    unsigned int mode; /**< new filter configuration level */
    long timeout; /**< Default timeout */
} g_options = {
    .command = UNDEFINED,
    .mode = UNDEFINED,
};

static void set_apid(char *apid)
{
    if (apid == NULL)
    {
        pr_error("-a needs argument. Please refer -h for usage information\n");
        exit(-1);
    }

    pr_verbose("Set APID to '%s", apid);
    dlt_set_id(g_options.apid, apid);
}

static char *get_apid(void)
{
    return g_options.apid;
}

static unsigned int get_command(void)
{
    return g_options.command;
}

static void set_command(unsigned int c)
{
    g_options.command = c;
}

static unsigned int get_mode(void)
{
    return g_options.mode;
}

static void set_mode(unsigned int m)
{
    if ((m == DLT_MODE_NON_BLOCKING) || (m == DLT_MODE_BLOCKING))
    {
        g_options.mode = m;
        set_command(DLT_SERVICE_ID_SET_BLOCK_MODE);
    }
    else
    {
        pr_error("Block mode (%u) invalid\n", m);
        exit(-1);
    }
}

/**
 * @brief Print filter status information
 *
 * @param info DltServiceGetCurrentFilterInfo
 */
static void dlt_print_blockmode_status(DltServiceGetBlockMode *info)
{
    char *mode;

    if (info == NULL)
    {
        pr_error("%s: Unexpected argument (NULL)\n", __func__);
    }

    if (info->mode == DLT_MODE_NON_BLOCKING)
    {
        mode = DLT_NON_BLOCKING;
    }
    else if (info->mode == DLT_MODE_BLOCKING)
    {
        mode = DLT_BLOCKING;
    }
    else
    {
        pr_error("Unexpected mode received: %u\n", info->mode);
        return;
    }

    if (strncmp(info->apid, DLT_ALL_APPLICATIONS, DLT_ID_SIZE) == 0)
    {
        fprintf(stdout, "Daemon BlockMode status: %s\n", mode);
    }
    else
    {
        fprintf(stdout,
                "Application '%.4s' BlockMode status: %s\n",
                info->apid, mode);
    }
}

/**
 * @brief Analyze received DLT Daemon response
 *
 * This function checks the received message. In particular, it checks the
 * answer string 'service(<ID>, {ok, error, perm_denied})'. In any case the
 * g_callback_return variable will be set as well which is evaluated in the
 * main function after the communication thread returned.
 *
 * @param message   Received DLT Message
 * @return 0 if daemon returns 'ok' message, -1 otherwise
 */
static int dlt_blockmode_analyze_response(char *answer, void *payload, int len)
{
    int ret = -1;
    char resp_ok[MAX_RESPONSE_LENGTH] = { 0 };
    char resp_perm_denied[MAX_RESPONSE_LENGTH] = { 0 };
    char resp_error[MAX_RESPONSE_LENGTH] = { 0 };

    if (answer == NULL || payload == NULL)
    {
        return -1;
    }

    snprintf(resp_ok,
             MAX_RESPONSE_LENGTH,
             "service(%u), ok",
             get_command());

    snprintf(resp_perm_denied,
             MAX_RESPONSE_LENGTH,
             "service(%u), perm_denied",
             get_command());

    snprintf(resp_error,
             MAX_RESPONSE_LENGTH,
             "service(%u), error",
             get_command());

    pr_verbose("Response received: '%s'\n", answer);
    pr_verbose("Response expected: '%s'\n", resp_ok);

    if (strncmp(answer, resp_ok, strlen(resp_ok)) == 0)
    {
        ret = 0;

        if (get_command() == DLT_SERVICE_ID_GET_BLOCK_MODE)
        {
            if ((int)sizeof(DltServiceGetBlockMode) > len)
            {
                pr_error("Received payload is smaller than expected\n");
                pr_verbose("Expected: %lu,\nreceived: %d\n",
                           (unsigned long) sizeof(DltServiceGetBlockMode),
                           len);
                ret = -1;
            }
            else
            {
                DltServiceGetBlockMode *info =
                    (DltServiceGetBlockMode *) (payload);
                if (info == NULL)
                {
                    pr_error("Received response is NULL\n");
                    return -1;
                }

                dlt_print_blockmode_status(info);
            }
        }
    }
    else if (strncmp(answer, resp_perm_denied, strlen(resp_perm_denied)) == 0)
    {
        fprintf(stdout,
                "Permission denied response received\n"
                "Check if client is connected or AllowBlockMode is enabled in "
                "dlt.conf\n");
    }
    else if (strncmp(answer, resp_error, strlen(resp_error)) == 0)
    {
            pr_verbose("Error response received \n");
            DltServiceGetBlockMode *info =
                                (DltServiceGetBlockMode *) (payload);
            if (info == NULL)
            {
                pr_error("Received response is NULL\n");
                return -1;
            }
            fprintf(stdout,
                    "Error response received\n");
    }
    else
    {
        pr_verbose("Invalid response received \n");
    }

    return ret;
}

/**
 * @brief Prepare message body to be send to DLT Daemon
 *
 * @return Pointer ot DltControlMsgBody, NULL otherwise
 */
static DltControlMsgBody *dlt_blockmode_prepare_message_body()
{
    DltControlMsgBody *mb = calloc(1, sizeof(DltControlMsgBody));
    if (mb == NULL)
    {
        return NULL;
    }

    if (get_command() == DLT_SERVICE_ID_SET_BLOCK_MODE)
    {
        mb->data = calloc(1, sizeof(DltServiceSetBlockMode));
        if (mb->data == NULL)
        {
            free(mb);
            return NULL;
        }
        mb->size = sizeof(DltServiceSetBlockMode);
        DltServiceSetBlockMode *serv = (DltServiceSetBlockMode *)
            mb->data;
        serv->service_id = DLT_SERVICE_ID_SET_BLOCK_MODE;
        serv->mode = get_mode();
        dlt_set_id(serv->apid, get_apid());
    }
    else /* Check BlockMode status */
    {
        mb->data = calloc(1, sizeof(DltServiceGetBlockMode));
        if (mb->data == NULL)
        {
            free(mb);
            return NULL;
        }

        mb->size = sizeof(DltServiceGetBlockMode);
        DltServiceGetBlockMode *serv = (DltServiceGetBlockMode *)
            mb->data;
        serv->service_id = DLT_SERVICE_ID_GET_BLOCK_MODE;
        serv->mode = 0;
        dlt_set_id(serv->apid, get_apid());
    }

    return mb;
}

/**
 * @brief Send a single command to DLT daemon and wait for response
 *
 * @return 0 on success, -1 on error
 */
static int dlt_blockmode_ctrl_single_request(void)
{
    int ret = -1;
    DltControlMsgBody *msg_body = NULL;

    /* Initializing the communication with the daemon */
    if (dlt_control_init(dlt_blockmode_analyze_response,
                         get_ecuid(),
                         get_verbosity()) != 0)
    {
        pr_error("Failed to initialize connection with the daemon.\n");
        return ret;
    }

    /* prepare message body */
    msg_body = dlt_blockmode_prepare_message_body();

    if (msg_body == NULL)
    {
        pr_error("Data for Dlt Message body is NULL\n");
        return ret;
    }

    ret = dlt_control_send_message(msg_body, get_timeout());

    free(msg_body->data);
    free(msg_body);

    dlt_control_deinit();

    return ret;
}

static void usage(char *name)
{
    if (name == NULL)
    {
        return;
    }

    printf("Usage: %s [options]\n", name);
    printf("Send a trigger to DLT daemon to enable/disable the block mode "
           "or get current BlockMode status\n");
    printf("\n");
    printf("Options:\n");
    printf("  -a         Specify specific application (optional)\n");
    printf("  -e         Enable Block mode (mandatory)\n");
    printf("  -d         Disable Block mode (mandatory)\n");
    printf("  -s         Show current BlockMode setting\n");
    printf("  -h         Usage\n");
    printf("  -t         Specify connection timeout (Default: %ds)\n",
           DLT_CTRL_TIMEOUT);
    printf("  -v         Set verbose flag (Default:%d)\n", get_verbosity());
    exit(0);
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

    /* Get command line arguments */
    opterr = 0;

    while ((c = getopt(argc, argv, "a:edsvt:h")) != -1)
    {
        switch(c)
        {
        case 'a':
            set_apid(optarg);
            break;
        case 't':
            set_timeout(strtol(optarg,NULL, 10));
            break;
        case 'e':
            set_mode(DLT_MODE_BLOCKING);
            break;
        case 'd':
            set_mode(DLT_MODE_NON_BLOCKING);
            break;
        case 's':
            set_command(DLT_SERVICE_ID_GET_BLOCK_MODE);
            break;
        case 'h':
            usage(argv[0]);
            break;
        case 'v':
            set_verbosity(1);
            pr_verbose("Now in verbose mode.\n");
            break;
        case '?':
            if (isprint(optopt))
            {
                pr_error("Unknown option -%c.\n", optopt);
            }
            else
            {
                pr_error("Unknown option character \\x%x.\n", optopt);
            }
            usage(argv[0]);
            return -1;
        default:
            pr_error("Try %s -h for more information.\n", argv[0]);
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

    set_ecuid(DLT_CTRL_DEFAULT_ECUID);
    set_timeout(DLT_CTRL_TIMEOUT);
    set_apid(DLT_ALL_APPLICATIONS);

    /* Get command line arguments */
    if (parse_args(argc, argv) != 0)
    {
        return -1;
    }

    if (get_command() == UNDEFINED)
    {
        pr_error("Neither -e nor -d specified!\n");
        usage(argv[0]);
        return -1;
    }

    pr_verbose("Sending command to DLT daemon.\n");
    /* one shot request */
    ret = dlt_blockmode_ctrl_single_request();

    pr_verbose("Exiting.\n");

    return ret;
}
