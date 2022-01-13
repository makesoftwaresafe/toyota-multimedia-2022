/************************************************************************
* @file: IRaHotplugReceiverShadow.h
*
* @version: 1.1
*
* IRaHotplugReceiverShadow class parse the D-Bus messages from Routing Adapter
* to Application (Hotpug) and delivers the parsed parameters to interface
* IRaHotplugReceive implemented by the Application (Hotplug)
* 
* @component: platform/audiomanager
*
* @author: Nrusingh Dash <ndash@jp.adit-jv.com>
*
* @copyright (c) 2010, 2011 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* @see <related items>
*
* @history
*
***********************************************************************/
#ifndef _I_RA_HOTPLUG_RECEIVER_SHADOW_H_
#define _I_RA_HOTPLUG_RECEIVER_SHADOW_H_

#include <string>
#include <map>
#include <dbus/dbus.h>
#include <IRaHotplugReceive.h>
#include <CAmSocketHandler.h>

namespace am {
class CDBusSender;
class CDBusReceiver;
class CAmDbusWrapper;
}

class IRaHotplugReceiverShadow
{
public:
	IRaHotplugReceiverShadow();
	virtual ~IRaHotplugReceiverShadow();
	am::am_Error_e init(am::CAmSocketHandler *socketHandler,
						IRaHotplugReceive* hotplugReceiver,
						std::string busName,
						std::string busObjectPath,
						std::string interfaceName,
						std::string interfaceObjectPath,
						DBusBusType type);
	void confirmHotplugReady(DBusConnection *conn, DBusMessage *msg);
	void asyncRegisterSink(DBusConnection *conn, DBusMessage *msg);
	void asyncDeregisterSink(DBusConnection *conn, DBusMessage *msg);
	void asyncRegisterSource(DBusConnection *conn, DBusMessage *msg);
	void asyncDeregisterSource(DBusConnection *conn, DBusMessage *msg);
	void ackSetSinkVolumeChange(DBusConnection *conn, DBusMessage *msg);
	void ackSetSourceVolumeChange(DBusConnection *conn, DBusMessage *msg);
	void ackSetSourceState(DBusConnection *conn, DBusMessage *msg);
	void ackSetSinkSoundProperties(DBusConnection *conn, DBusMessage *msg);
	void ackSetSinkSoundProperty(DBusConnection *conn, DBusMessage *msg);
	void ackSetSourceSoundProperties(DBusConnection *conn, DBusMessage *msg);
	void ackSetSourceSoundProperty(DBusConnection *conn, DBusMessage *msg);
	void hookInterruptStatusChange(DBusConnection *conn, DBusMessage *msg);
	void hookSinkAvailablityStatusChange(DBusConnection *conn, DBusMessage *msg);
	void hookSourceAvailablityStatusChange(DBusConnection *conn, DBusMessage *msg);
	void hookSinkNotificationDataChange(DBusConnection *conn, DBusMessage *msg);
	void hookSourceNotificationDataChange(DBusConnection *conn, DBusMessage *msg);
	void asyncUpdateSink(DBusConnection *conn, DBusMessage *msg);
	void asyncUpdateSource(DBusConnection *conn, DBusMessage *msg);
	void sendIntrospection(DBusConnection* conn, DBusMessage* msg);
	//DBusHandlerResult receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data);
	DBusHandlerResult receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg);
	void getConnection(DBusConnection * &connection);

private:
	typedef void (IRaHotplugReceiverShadow::*CallBackMethod)(DBusConnection *connection, DBusMessage *message);
	typedef std::map<std::string, CallBackMethod> functionMap_t;
	IRaHotplugReceive *mpHotplugReceive;
	functionMap_t mFunctionMap;
	am::CDBusSender *mpDBusSender;
	am::CDBusReceiver *mpDBusReceiver;
	am::CAmDbusWrapper *mpDBusWrapper;
	am::CAmSocketHandler *mpSocketHandler;
	functionMap_t createMap(void);
};

#endif // _I_RA_HOTPLUG_RECEIVER_SHADOW_H_
