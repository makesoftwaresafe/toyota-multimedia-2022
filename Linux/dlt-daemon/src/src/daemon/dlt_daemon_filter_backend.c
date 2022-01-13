/*
 * @licence app begin@
 * SPDX license identifier: MPL-2.0
 *
 * Copyright (C) 2015 Advanced Driver Information Technology.
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
 * \copyright Copyright © 2015 Advanced Driver Information Technology. \n
 * License MPL-2.0: Mozilla Public License version 2.0 http://mozilla.org/MPL/2.0/.
 *
 * \file dlt_daemon_filter_backend.c
 */

#include <syslog.h>
#include <sys/poll.h>

#include <adit-components/ald_plugin.h>

#include "dlt_common.h"
#include "dlt-daemon.h"
#include "dlt-daemon_cfg.h"
#include "dlt_daemon_filter_types.h"
#include "dlt_daemon_filter.h"
#include "dlt_daemon_event_handler.h"
#include "dlt_daemon_connection.h"
#include "dlt_daemon_connection_types.h"

/**
 * @brief Callback for security level changed event
 *
 * @param level     new security level
 * @param ptr1      pointer to DltDaemonLocal
 * @param ptr2      pointer to verbose flag
 */
void dlt_daemon_filter_backend_level_changed(unsigned int level,
                                             void *ptr1,
                                             void *ptr2)
{
    if ((ptr1 == NULL) || (ptr2 == NULL))
    {
        dlt_vlog(LOG_CRIT, "%s: has wrong paramters\n", __func__);
        return;
    }

    PRINT_FUNCTION_VERBOSE(*((int *)ptr2));

    DltDaemonLocal *daemon_local = (DltDaemonLocal *) ptr1;
    DltFilterConfiguration *curr = daemon_local->pFilter.current;

    /* Nothing need to be done if the received level is included in
     * the current level range */
    if ((level >= curr->level_min) && (level <= curr->level_max))
    {
        return;
    }

    int ret = dlt_daemon_filter_change_filter_level(daemon_local,
                                                    level,
                                                    daemon_local->flags.vflag);

    if (ret != 0)
    {
        dlt_log(LOG_CRIT, "Changing filter level failed!\n");
    }
}

/**
 * @brief Callback for backend connection established event
 *
 * @param ptr1 pointer to DltDaemonLocal
 * @param ptr2 pointer to verbose flag
 */
void dlt_daemon_filter_backend_connected(void *ptr1, void *ptr2)
{
    if ((ptr1 == NULL) || (ptr2 == NULL))
    {
        dlt_vlog(LOG_CRIT, "%s: wrong parameters\n", __func__);
        return;
    }

    PRINT_FUNCTION_VERBOSE(*((int *)ptr2));

    /* when connected, retrieve filter level; 1 == true */
    unsigned int updated_filter_level = ald_plugin_get_security_level(1);
    dlt_daemon_filter_backend_level_changed(updated_filter_level,
                                               ptr1,
                                               ptr2);
}

/**
 * @brief Callback for backend connection lost event
 *
 * @param ptr1 pointer to DltDaemonLocal
 * @param ptr2 pointer to verbose flag
 */
void dlt_daemon_filter_backend_disconnected(void *ptr1, void *ptr2)
{
    if ((ptr1 == NULL) || (ptr2 == NULL))
    {
        dlt_vlog(LOG_CRIT, "%s: wrong parameters\n", __func__);
        return;
    }

    PRINT_FUNCTION_VERBOSE(*((int *)ptr2));

    /* when connection to ALD is lost, the default level need to be set again
     * for this we need to access the filter pointer */

    DltDaemonLocal *daemon_local = (DltDaemonLocal *) ptr1;
    unsigned int default_filter_level = (unsigned int)
                                        daemon_local->pFilter.default_level;
    dlt_daemon_filter_backend_level_changed(default_filter_level,
                                               ptr1,
                                               ptr2);
}

static const ald_plugin_callbacks_t ald_plugin_callbacks =
{
    .connection_clbk = dlt_daemon_filter_backend_connected,
    .disConnection_clbk = dlt_daemon_filter_backend_disconnected,
    .calculate_log_level_clbk = dlt_daemon_filter_backend_level_changed,
    .cmd_received_clbk = NULL /* not interested in receiving commands */
};

int dlt_daemon_filter_backend_init(DltDaemonLocal *daemon_local,
                                   int curr_filter_level,
                                   int verbose)
{
    int fd = -1;
    PRINT_FUNCTION_VERBOSE(verbose);

    if (daemon_local == NULL)
        return -1;

    int ret = ald_plugin_init(&ald_plugin_callbacks, curr_filter_level);

    if (ret != 0)
    {
        dlt_log(LOG_ERR, "ALD plugin could not be initialized\n");
        return ret;
    }

    fd = ald_plugin_get_pollfd();

    ret = dlt_connection_create(daemon_local,
                         &daemon_local->pEvent,
                         fd,
                         POLLIN,
                         DLT_CONNECTION_FILTER);

    if (ret != 0)
    {
        dlt_log(LOG_ERR, "Filter backend connection creation failed\n");
        return ret;
    }

    return 0;
}

int dlt_daemon_filter_backend_deinit(DltDaemonLocal *daemon_local, int verbose)
{
    PRINT_FUNCTION_VERBOSE(verbose);

    if (daemon_local == NULL)
    {
        return -1;
    }

    return ald_plugin_deinit(daemon_local, &verbose);
}

int dlt_daemon_filter_backend_dispatch(DltDaemonLocal *daemon_local,
                                       int *verbose)
{
    if ((daemon_local == NULL) || (verbose == NULL))
    {
        return -1;
    }

    PRINT_FUNCTION_VERBOSE(*verbose);

    return ald_plugin_dispatch(daemon_local, verbose);
}
