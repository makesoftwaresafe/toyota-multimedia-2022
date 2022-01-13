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
 * \file dlt_daemon_filter.h
 */

/*******************************************************************************
**                                                                            **
**  SRC-MODULE: dlt_daemon_filter.h                                           **
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

#ifndef _DLT_DAEMON_FILTER_H
#define _DLT_DAEMON_FILTER_H

#include "dlt_daemon_connection_types.h"
#include "dlt_daemon_filter_types.h"
#include "dlt-daemon.h"

/**
 * @brief callback for events received from a filter backend
 *
 * @param daemon_local  DltDaemonLocal structure pointer
 * @oaram verbose       verbose flag
 * @return 0 on success, -1 otherwise
 */
int dlt_daemon_filter_process_filter_control_messages(
    DltDaemon *daemon,
    DltDaemonLocal *daemon_local,
    DltReceiver *rec,
    int verbose);

/**
 * @brief Change the filter to the given level
 *
 * This function changes the filter configuration to the given filter level.
 * It is called by the DLT Daemon directly when it receives ChangeFilterLevel
 * service request send by a control application.
 *
 * @param daemon_local  DltDaemonLocal structure
 * @param level         New filter level
 * @param verbose       verbose flag
 * @return 0 on success, -1 otherwise
 */
int dlt_daemon_filter_change_filter_level(DltDaemonLocal *daemon_local,
                                          unsigned int level,
                                          int verbose);

/**
  * @brief Check if a connection is allowed in the current filter level
  *
  * This function uses the currently active filter configuration to check if a
  * certain connection type is allowed.
  *
  * @param filter   Message filter
  * @param type     ConnectionType
  * @return > 0 if allowed, 0 if not allowed or -1 on error
  */
int dlt_daemon_filter_is_connection_allowed(DltMessageFilter *filter,
                                           DltConnectionType type);

/**
  * @brief Check if a control message is allowed in the current filter level
  *
  * This function uses the currently active filter configuration to check if
  * the given control message - specified by its identifier - has to be rejected
  * or handled.
  *
  * @param filter   Message filter
  * @param id       Control message identifier
  * @return true > 0 if allowed, 0 if not allowed or -1 on error
  */
int dlt_daemon_filter_is_control_allowed(DltMessageFilter *filter, int id);

/**
 * @brief Check if an injection message is allowed
 *
 * This function uses the currently active filter configuration to check if
 * the received injection message has to be rejected or forwarded to the
 * corresponding application.
 *
 * @param filter    Message filter
 * @param apid      Application identifier of the message receiver
 * @param ctid      Context identifier of the message receiver
 * @param ecuid     Node identifier of the message receiver
 * @param id        Service identifier of the injection message
 * @return 1 if allowed, 0 if not allowed or -1 on error
 */
int dlt_daemon_filter_is_injection_allowed(DltMessageFilter *filter,
                                           char *apid,
                                           char *ctid,
                                           char *ecuid,
                                           int id);

/**
* @brief Initialize the message filter structure
*
* During initialization, the configuration file will be parsed and all
* specified message filter configurations and injection message configurations
* inialized. Furthermore, the specified inital filter level will be set.
*
* @param daemon_local  DaemonLocal
* @param verbose       verbose flag
* @return 0 on success, -1 otherwise
*/
int dlt_daemon_prepare_message_filter(DltDaemonLocal *daemon_local,
                                     int verbose);

/**
* @brief Cleanup the message filter structure
*
* All allocated memory of the message filter will be freed. The pointer will
* be set to NULL before the function returns.
*
* @param daemon_local  DaemonLocal
* @param verbose       verbose flag
*/
void dlt_daemon_cleanup_message_filter(DltDaemonLocal *daemon_local,
                                      int verbose);
#endif
