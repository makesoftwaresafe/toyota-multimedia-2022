/**
 * @licence app begin@
 * Copyright (C) 2013 - 2015  Advanced Driver Information Technology.
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
 * \author Anitha.BA <anithaammaji.baggam@in.bosch.com> ADIT 2015
 * \author Frederic Berat <fberat@de.adit-jv.com> ADIT 2015
 *
 * \file dlt-logstorage-prop.c
 * For further information see http://www.genivi.org/.
 * @licence end@
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt-logstorage-prop.c                                         **
**                                                                            **
**  TARGET    : linux                                                         **
**                                                                            **
**  PROJECT   : DLT                                                           **
**                                                                            **
**  AUTHOR    : Anitha.B.A  anithaammaji.baggam@in.bosch.com                  **
**              Frederic Berat fberat@de.adit-jv.com                          **
**                                                                            **
**  PURPOSE   : For handling the proprietary automounter interaction with DLT **
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
**  BA          Anitha                     ADIT                               **
**  fb          Frederic Berat             ADIT                               **
*******************************************************************************/

#define pr_fmt(fmt) "Automounter control: "fmt

#include <errno.h>
#include <mntent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "dlt-logstorage-prop.h"
#include "dlt-control-common.h"
#include "dlt-logstorage-common.h"
#include "dlt-logstorage-ctrl.h"
#include "dlt-logstorage-list.h"

#include "automounter/automounter_api.h"
#include "automounter/automounter_api_ctrl.h"
#include "automounter/automounter_api_info.h"
#include "automounter/automounter_api_events.h"

/** @brief Ask automounter to remount the partition
 *
 * As of now this function is only called in order to remount RW.
 * We only warn on error, the result doesn't really matters.
 *
 * @param mount_point The mount point to be remounted
 * @param mount_options The option for the remount
 */
static void dlt_logstorage_remount(const char *mount_point,
                                   const char *mount_options)
{
    error_code_t result = RESULT_OK;

    if (!mount_point || !mount_options)
    {
        pr_error("Bad arguments for remount (%p %p).\n",
                 mount_point,
                 mount_options);
        return;
    }

    pr_verbose("Ask for %s remount %s.\n", mount_point, mount_options);

    result = automounter_api_remount_partition_by_mountpoint(mount_point,
                                                             mount_options,
                                                             -1,
                                                             NULL);

    switch (result)
    {
    case RESULT_NOT_MOUNTED:
        pr_error("Partition not mounted.\n");
        break;
    case RESULT_INVALID:
        pr_error("Given mount point not found.\n");
        break;
    case RESULT_NORESOURCE:
        pr_error("No resource for remounting.\n");
        break;
    default:

        if (result != RESULT_OK)
        {
            pr_error("Unknown error while executing the remount command.\n");
        }

        break;
    }

    return;
}

/** @brief Calls the automounter event dispatcher
 *
 * @return 0
 */
static int logstorage_am_callback(void)
{
    automounter_api_dispatch_event();
    return 0;
}

/** @brief Acts on successful connection to automounter
 *
 * Once the connection is established, we ask for a snapshot of the currently
 * mounted devices.
 */
static void on_establish_connection_success(void)
{
    pr_verbose("Connection established. Asking for snapshot.\n");
    /* Request for all mounted devices and partitions */
    automounter_api_get_snapshot(SNAPSHOT_MOUNTED_PARTITIONS_ONLY, NULL);
}

/** @brief Acts on connection failure
 *
 * Asks for application exit on connection failure.
 */
static void on_establish_connection_failure(void)
{
    pr_error("Connection to automounter failed. Exiting.\n");
    dlt_logstorage_exit();
}

/** @brief Acts on connection lost
 *
 * Tries to reconnect to the automounter if the connection as been lost.
 * TODO: Send unmount event for all known devices ?
 */
static void on_connection_lost(void)
{
    pr_verbose("Connection to automounter lost.\nTrying to reconnect.\n");

    if (automounter_api_try_connect() != RESULT_OK)
    {
        pr_error("Unable to reconnect with automounter.\n");
        dlt_logstorage_exit();
        return;
    }
}

/** @brief Acts on MOUNT events
 *
 * Checks whether if a configuration file is available,
 * sends a message to DLT if it's the case.
 *
 * @param part_info The partition info
 * @param device_info The device info
 */
static void on_state_mounted(const partition_info_t *part_info,
                             const device_info_t *device_info)
{
    if ((part_info == NULL) || (device_info == NULL))
    {
        return;
    }

    if (!dlt_logstorage_check_config_file((char *)part_info->mount_point))
    {
        pr_verbose("No config file in %s\n", part_info->mount_point);
        return;
    }

    if (part_info->mounted_writable == false)
    {
        pr_verbose("Asking for %s to be remounted.\n", part_info->mount_point);
        dlt_logstorage_remount(part_info->mount_point, MNTOPT_RW);
        return;
    }

    logstorage_store_dev_info(part_info->mount_src,
                              part_info->mount_point);

    if (dlt_logstorage_send_event(EVENT_MOUNTED,
                                  (char *)part_info->mount_point))
    {
        pr_error("Can't send mount event for %s to DLT.\n",
                 part_info->mount_point);
        return;
    }

    return;
}

/** @brief Acts on UNMOUNT events
 *
 * Checks whether if athe device is in the internal list,
 * sends a message to DLT if it's the case.
 *
 * @param part_info The partition info
 * @param device_info The device info
 */
static void on_state_unmounting(const partition_info_t *part_info,
                                const device_info_t *device_info)
{
    char *mnt_point = NULL;

    if ((part_info == NULL) || (device_info == NULL))
    {
        return;
    }

    mnt_point = logstorage_delete_dev_info(part_info->mount_src);

    pr_verbose("Getting unmount event for %s.\n", part_info->mount_src);

    if (mnt_point)
    {
        if (dlt_logstorage_send_event(EVENT_UNMOUNTING, mnt_point))
        {
            pr_error("Can't send unmount event for %s to DLT.\n", mnt_point);
        }
    }

    free(mnt_point);

    return;
}

/** @brief Acts on partition info update
 *
 * This function is expected to be called as a result of the snapshot asked
 * when the connection has been established.
 *
 * @param part_info The partition info
 * @param device_info The device info
 * @param request_id Unused
 */
static void on_update_partition_info(const partition_info_t *part_info,
                                     const device_info_t *device_info,
                                     int request_id)
{
    /* Satisfying lint */
    (void)request_id;

    if ((part_info == NULL) || (device_info == NULL))
    {
        return;
    }

    if (part_info->state == PARTITION_MOUNTED)
    {
        return on_state_mounted(part_info, device_info);
    }

    return;
}

/* The list of callbacks provided to automounter */
automounter_api_callbacks_t event_callbacks = {
    .on_establish_connection_success = on_establish_connection_success,
    .on_connection_lost = on_connection_lost,
    .on_establish_connection_failure = on_establish_connection_failure,
    .on_partition_mounted = on_state_mounted,
    .on_partition_unmounting = on_state_unmounting,
    .on_partition_invalid = on_state_unmounting,
    .on_update_partition_info = on_update_partition_info
};

/* The strings that can be used as a command parameter for daemonizing */
static char *possible_types[] = {
    "am",
    "automounter",
    "prop",
    "proprietary",
    NULL
};

/** @brief Check whether if the user wants to have automounter handling
 *
 * This function compares the argument with the list of allowed strings.
 * If the argument matches, the that means that the user wants to use
 * automounter as a trigger for mount/unmount events.
 *
 * @param type The string to be compared
 *
 * @return 1 if the user wants automounter, 0 either.
 */
int check_proprietary_handling(char *type)
{
    int i = 0;

    while (possible_types[i])
    {
        if (strncmp(type, possible_types[i], strlen(possible_types[i])) == 0)
        {
            return 1;
        }

        i++;
    }

    return 0;
}

/** @brief Clean-up the autmounter connection
 *
 * This disconnects the API and deinit it.
 */
int dlt_logstorage_prop_deinit(void)
{
    automounter_api_disconnect();
    automounter_api_deinit();

    return 0;
}

/** @brief Initialize the automounter API
 *
 * Initialize the API and registers the callbacks.
 *
 * @return 0 on success, -1 either.
 */
int dlt_logstorage_prop_init(void)
{
    DltLogstorageCtrl *lctrl = get_logstorage_control();

    pr_verbose("Initializing.\n");

    if (!lctrl)
    {
        pr_error("Not able to get logstorage control instance.\n");
        dlt_logstorage_exit();
        return -1;
    }

    if (automounter_api_init("automounterctl",
                             LOGGER_LEVEL_ERROR,
                             true) != RESULT_OK)
    {
        pr_error("Error in initializing automounter.\n");
        return -1;
    }

    automounter_api_register_callbacks(&event_callbacks);

    if (automounter_api_try_connect() != RESULT_OK)
    {
        pr_error("Connection to automounter failed.\n");
        return -1;
    }

    lctrl->fd = automounter_api_get_pollfd();

    if (lctrl->fd == -1)
    {
        pr_error("Failed to get automounter fd, exiting.\n");
        dlt_logstorage_exit();
        return -1;
    }

    lctrl->callback = logstorage_am_callback;

    return 0;
}
