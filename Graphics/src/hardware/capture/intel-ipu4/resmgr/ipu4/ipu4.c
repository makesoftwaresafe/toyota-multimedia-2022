/*
* Copyright (c) 2017 QNX Software Systems.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

/*
 * ipu4.c
 *
 * This file contains the main routine of the function which implements the
 * service's initialization.
 */

#include <stdint.h>
#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/procmgr.h>
#include <sys/neutrino.h>
#include <login.h>

#include "ipu4_log.h"
#include "ipu4_resmgr.h"

// Initialize our resource manager
static int initialize_resmgr(void)
{
    int err;

    err = ipu4_resmgr_init();
    if (err != EOK) {
        LOG_ERROR("Failed to initialize our resmgr: err = %d", err);
        return err;
    }

    err = ipu4_resmgr_start();
    if (err != EOK) {
        LOG_ERROR("Failed to start resmgr: err = %d", err);
        return err;
    }
    return EOK;
}

/**
 * Acquire any root abilities that we need to conserve.
 *
 * Here we register to keep any root abilities that we may need.
 * @return EOK on success, failure code on error.
 */
static int acquire_root_abilities(void)
{
#ifdef PROCMGR_AOP_ALLOW
    int rc;

    rc = procmgr_ability(0,
                         PROCMGR_ADN_NONROOT | PROCMGR_AOP_ALLOW | PROCMGR_AOP_LOCK | PROCMGR_AID_MEM_PHYS,
                         PROCMGR_ADN_NONROOT | PROCMGR_AOP_ALLOW | PROCMGR_AOP_LOCK | PROCMGR_AID_INTERRUPT,
                         PROCMGR_ADN_NONROOT | PROCMGR_AOP_ALLOW | PROCMGR_AOP_LOCK | PROCMGR_AID_MAP_FIXED,
                         PROCMGR_AID_EOL);
    if (rc != EOK) {
        LOG_ERROR("Failed to acquire abilities");
        return rc;
    }
#endif
    return EOK;
}

/**
 * Service main routine.
 *
 * This is the main routine of the service: it handles the initialization
 * and start of the routine then waits to exit when we are done.
 * @param[IN] argc Number of command line arguments
 * @param[IN] argv Command line argument values
 * @return EXIT_FAILURE if an error was encountered, EXIT_SUCCESS if no errors
 *         were encountered.
 */
int main(int argc, char *argv[])
{
    int sig;
    sigset_t sigset;
    int opt;
    char *start_cred_param = NULL;
    bool more_options = true;

    pthread_setname_np(0, "IPU4");

    while (more_options) {
        opt = getopt(argc, argv, "U:");
        if (opt != -1) {
            switch (opt) {
            case 'U':
                start_cred_param = optarg;
                break;
            default:
                fprintf(stderr, "Invalid usage.  See use message for help\n");
                return(EXIT_FAILURE);
                break;
            }
        } else {
            if (optind >= argc) {
                more_options = false;
            }
        }
    }
    if (start_cred_param == NULL) {
        fprintf(stderr, "-U command line option is required\n");
        return EXIT_FAILURE;
    }

    if (ipu4_log_init() != EOK) {
        fprintf(stderr, "Unable to initialize logging\n");
        return EXIT_FAILURE;
    }

    // Get IO privileges required by ipu4drv library
    if (ThreadCtl(_NTO_TCTL_IO, 0) == -1) {
        LOG_CRITICAL("ThreadCtl failed, err=%d", errno);
        return EXIT_FAILURE;
    }

    // Initialize our resmgr
    if (initialize_resmgr() != EOK) {
        LOG_CRITICAL("Failed to initialize our resource manager");
        return EXIT_FAILURE;
    }

    // acquire any root capabilities that we may need in the future
    if (acquire_root_abilities() != EOK) {
        LOG_CRITICAL("Unable to acquire root abilities");
        return EXIT_FAILURE;
    }

#ifdef PROCMGR_AOP_ALLOW
    // drop root uid/gid now.
    if (set_ids_from_arg(start_cred_param) != EOK) {
        LOG_CRITICAL("Unable to set uid/gid");
        return EXIT_FAILURE;
    }
#endif

    // wait for shutdown of service
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    sigaddset(&sigset, SIGTERM);
    sigwait(&sigset, &sig);

    LOG_WARNING("Shutting down: received signal %d", sig);

    // Do necessary cleanup
    if (ipu4_resmgr_deinit() != EOK) {
        LOG_WARNING("Failure in trying to de-initialize our resmgr");
    }

    return EXIT_SUCCESS;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/resmgr/ipu4/ipu4.c $ $Rev: 838597 $")
#endif
