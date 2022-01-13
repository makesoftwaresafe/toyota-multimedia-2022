#ifndef PERSISTENCE_HM_DBUS_MESSAGE_H_
#define PERSISTENCE_HM_DBUS_MESSAGE_H_

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
 * @file           persistence_hm_dbus_message.h
 * @ingroup        Persistence Health Monitor
 * @author         Ingo Huerner
 * @brief          Implementation of the persistence health monitor dbus message dispatching.
 * @see
 */

#include <dbus/dbus.h>


DBusHandlerResult checkPersPhmMsg(DBusConnection * connection, DBusMessage * message, void * user_data);

DBusHandlerResult msg_persFsCheck(DBusConnection *connection, DBusMessage *message);

DBusHandlerResult msg_persFsCheckAndRecover(DBusConnection *connection, DBusMessage *message);

DBusHandlerResult msg_persFsMount(DBusConnection *connection, DBusMessage *message);

DBusHandlerResult msg_persFsUnMount(DBusConnection *connection, DBusMessage *message);

DBusHandlerResult msg_persFsCreatePartition(DBusConnection *connection, DBusMessage *message);


#endif /* PERSISTENCE_HM_DBUS_MESSAGE_H_ */
