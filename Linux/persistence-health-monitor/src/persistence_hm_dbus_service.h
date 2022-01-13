#ifndef PERSISTENCE_HM_DBUS_SERVICE_H_
#define PERSISTENCE_HM_DBUS_SERVICE_H_

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
 * @file           persistence_hm_dbus_service.h
 * @ingroup        Persistence health monitor
 * @author         Ingo Huerner
 * @brief          Header of the persistence health monitor dbus service.
 * @see
 */

#include <dbus/dbus.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <sys/timerfd.h>

/// command definitions for main loop
typedef enum ECmd
{
	/// command none
   CMD_NONE = 0,
   /// quit command
   CMD_QUIT,
   ///  request dbus name
   CMD_REQUEST_NAME
} tCmd;


/// command data union definition
typedef union MainLoopData_u_{

	/// message structure
	struct {
		/// dbus mainloop command
		uint32_t cmd;
		/// unsigned int parameters
		uint32_t params[4];
		/// string parameter
		char string[64];
	} message;

	/// the message payload
	char payload[128];
} MainLoopData_u;


/// mutex to make sure main loop is running
extern pthread_mutex_t gDbusInitializedMtx;
/// dbus init conditional variable
extern pthread_cond_t  gDbusInitializedCond;
/// dbus pending mutex
extern pthread_mutex_t gDbusPendingRegMtx;
/// dbus mainloop conditional variable
extern pthread_mutex_t gMainCondMtx;
/// dbus mainloop mutex
extern pthread_t gMainLoopThread;

/*
/// persistence administrator consumer dbus interface
extern const char* gDbusPersHmInterface;
/// persistence administrator consumer dbus
extern const char* gDbusPersHmPath;
/// persistence administrator consumer dbus interface message
extern const char* gDbusPersHmMsg;
/// persistence administrator dbus
extern const char* gDbusPersAdminInterface;
/// persistence administrator dbus path
extern const char* gPersHmPath;
*/


/**
 * @brief DBus main loop to dispatch events and dbus messages
 *
 * @param vtable the function pointer tables for '/org/genivi/persistence/adminconsumer' messages
 * @param vtableFallback the fallback function pointer tables
 * @param conn the dbus connection
 *
 * @return 0
 */
int mainLoop(DBusObjectPathVTable vtable, DBusObjectPathVTable vtableFallback, void* userData);


/**
 * @brief Setup the dbus main dispatching loop
 *
 * @return 0
 */
int setup_dbus_mainloop(void);


/**
 * @brief deliver message to mainloop (blocking)
 *        The function blocks until the message has
 *        been delivered to the mainloop
 *
 * @param payload the message to deliver to the mainloop (command and data)
 *
 * @return 0
 */
int deliverToMainloop(MainLoopData_u* payload);


/**
 * @brief deliver message to mainloop (non blocking)
 *        The function does N O T  block until the message has
 *        been delivered to the mainloop
 *
 * @param payload the message to deliver to the mainloop (command and data)
 *
 * @return 0
 */
int deliverToMainloop_NM(MainLoopData_u* payload);


#endif /* PERSISTENCE_HM_DBUS_SERVICE_H_ */
