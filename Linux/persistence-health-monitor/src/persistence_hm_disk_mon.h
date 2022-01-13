#ifndef PERSISTENCE_HM_DISK_MON_H_
#define PERSISTENCE_HM_DISK_MON_H_


/******************************************************************************
 * Project         Persistency
 * (c) copyright   2014
 * Company         XS Embedded GmbH
 *****************************************************************************/
/******************************************************************************
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a  copy of the MPL was not distributed
 * with this file, You can obtain one at http://mozilla.org/MPL/2.0/.
******************************************************************************/
 /**
 * @file           persistence_hm_disk_mon.c
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor disk free space monitor.
 * @see
 */



int startMonitorThread();

void freeRbTree();


#endif /* PERSISTENCE_HM_DISK_MON_H_ */
