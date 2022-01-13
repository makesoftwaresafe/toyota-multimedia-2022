/************************************************************************
* @file: CAppHotplugSenderShadow.h
*
* @version: 1.1
*
* CAppHotplugSenderShadow class recives the D-Bus messages recived from
* Application (Hotplug) to Routing Adapter. Parses the D-Bus message paramters
* and delivers them to interface IRaHotplugSend
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
#ifndef __C_APP_HOTPLUG_SENDER_SHADOW_H__
#define __C_APP_HOTPLUG_SENDER_SHADOW_H__
#include <map>
#include <string>
#include <dbus/dbus.h>
#include <IRaHotplugSend.h>
#include <CAmSocketHandler.h>

namespace am {
class CDBusSender;
class CDBusReceiver;
class CAmDbusWrapper;
}

class CAppHotplugSenderShadow
{
public:
	CAppHotplugSenderShadow();
	virtual ~CAppHotplugSenderShadow();
	am::am_Error_e init(am::CAmSocketHandler *socketHandler,
						IRaHotplugSend* hotplugSender,
						std::string busName,
						std::string busObjcetpath,
						std::string interfaceName,
						std::string interfaceObjectPath,
						DBusBusType type);
	void asyncSetSinkVolume(DBusConnection *conn, DBusMessage *msg);
	void asyncSetSourceVolume(DBusConnection *conn, DBusMessage *msg);
	void asyncSetSourceState(DBusConnection *conn, DBusMessage *msg);
	void asyncSetSinkSoundProperties(DBusConnection *conn, DBusMessage *msg);
	void asyncSetSinkSoundProperty(DBusConnection *conn, DBusMessage *msg);
	void asyncSetSourceSoundProperties(DBusConnection *conn, DBusMessage *msg);
	void asyncSetSourceSoundProperty(DBusConnection *conn, DBusMessage *msg);
	void ackRegisterSink(DBusConnection *conn, DBusMessage *msg);
	void ackDeregisterSink(DBusConnection *conn, DBusMessage *msg);
	void ackRegisterSource(DBusConnection *conn, DBusMessage *msg);
	void ackDeregisterSource(DBusConnection *conn, DBusMessage *msg);
	void ackUpdateSource(DBusConnection *conn, DBusMessage *msg);
	void ackUpdateSink(DBusConnection *conn, DBusMessage *msg);
	void sendIntrospection(DBusConnection* conn, DBusMessage* msg);
	DBusHandlerResult receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg);
	void getConnection(DBusConnection *& connection);

private:
	typedef void (CAppHotplugSenderShadow::*CallBackMethod)(DBusConnection *connection, DBusMessage *message);
	typedef std::map<std::string, CallBackMethod> functionMap_t;
	IRaHotplugSend *mpHotplugSend;
	functionMap_t mFunctionMap;
	am::CDBusSender *mpDBusSender;
	am::CDBusReceiver *mpDBusReceiver;
	am::CAmSocketHandler *mpSocketHandler;
	am::CAmDbusWrapper *mpDBusWrapper;
	functionMap_t createMap(void);
};
#endif /* __C_APP_HOTPLUG_SENDER_SHADOW_H__ */
