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
 * \file dlt_daemon_filter_backend.h
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt_daemon_filter_backend.h                                   **
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

#ifndef _DLT_DAEMON_FILTER_BACKEND_H
#define _DLT_DAEMON_FILTER_BACKEND_H

#ifndef HAS_PROPRIETARY_FILTER_BACKEND /* only function stubs */
/**
 * @brief Init proprietary filter backend communication
 *
 * @param daemon_local DltDaemonLocal structure pointer
 * @param curr_filter_level default filter level
 * @return 0 on success, -1 otherwise
 */
static inline int dlt_daemon_filter_backend_init(DltDaemonLocal *daemon_local,
                                                 int curr_filter_level,
                                                 int verbose)
{
    (void) daemon_local;
    (void) curr_filter_level;
    (void) verbose;
    return 0;
}

/**
 * @brief Deinit proprietary filter backend communication
 *
 * @param daemon_local DltDaemonLocal structure pointer
 * @return 0 on success, -1 otherwise
 */
static inline int dlt_daemon_filter_backend_deinit(DltDaemonLocal *daemon_local,
                                                   int verbose)
{
    (void) daemon_local;
    (void) verbose;
    return 0;
}

/**
 * @brief proprietary filter backend dispatch wrapper function
 *
 * @param daemon_local  DltDaemonLocal structure pointer
 * @param verbose       verbose flag
 * @return 0 on success, -1 otherwise
 */
static inline int dlt_daemon_filter_backend_dispatch(
    DltDaemonLocal *daemon_local,
    int *verbose)
{
    (void) daemon_local;
    (void) verbose;
    return 0;
}
#else
/**
 * @brief Init proprietary filter backend communication
 *
 * @param daemon_local DltDaemonLocal structure pointer
 * @param curr_filter_level default filter level
 * @return 0 on success, -1 otherwise
 */
int dlt_daemon_filter_backend_init(DltDaemonLocal *daemon_local,
                                   int curr_filter_level,
                                   int verbose);

/**
 * @brief Deinit proprietary filter backend communication
 *
 * @param daemon_local DltDaemonLocal structure pointer
 * @return 0 on success, -1 otherwise
 */
int dlt_daemon_filter_backend_deinit(DltDaemonLocal *daemon_local, int verbose);

/**
 * @brief proprietary filter backend dispatch wrapper function
 *
 * @param daemon_local  DltDaemonLocal structure pointer
 * @param verbose       verbose flag
 * @return 0 on success, -1 otherwise
 */
int dlt_daemon_filter_backend_dispatch(DltDaemonLocal *daemon_local,
                                       int *verbose);
#endif /* HAS_PROPRIETARY_FILTER_BACKEND */
#endif
