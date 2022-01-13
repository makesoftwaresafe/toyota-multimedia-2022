/************************************************************************
* @file: CAppHotplugSenderShadow.cpp
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
#include <dbus/dbus.h>
#include <dlt/dlt.h>
#include <assert.h>
#include <fstream>
#include <stdexcept>
#include "CDBusSender.h"
#include "CDBusReceiver.h"
#include "CAmDbusWrapper.h"
#include "CAppHotplugSenderShadow.h"
#include "Log.h"

DLT_IMPORT_CONTEXT(raContext)

#define HOTPLUG_SEND_DBUS_INTROSPECTION_FILE "/usr/share/audiomanager/HotplugSender.xml"
#define HOTPLUG_SEND_NODE "hotplugsend"
static DBusObjectPathVTable gObjectPathVTable;

static DBusHandlerResult receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    LOG_FN_ENTRY(raContext);
    assert(conn != NULL);
    assert(msg != NULL);
    assert(user_data != NULL);
    CAppHotplugSenderShadow* reference = (CAppHotplugSenderShadow*) ((user_data));
    LOG_FN_EXIT(raContext);
    return (reference->receiveCallbackDelegate(conn, msg));
}

CAppHotplugSenderShadow::CAppHotplugSenderShadow() :
	mpHotplugSend(0),
	mFunctionMap(createMap()),
	mpDBusSender(NULL),
	mpDBusReceiver(NULL),
	mpDBusWrapper(NULL)
{
    LOG_FN_ENTRY(raContext);
    LOG_FN_EXIT(raContext);
}

CAppHotplugSenderShadow::~CAppHotplugSenderShadow()
{
    LOG_FN_ENTRY(raContext);
	if (mpDBusWrapper != NULL)
	{
		delete mpDBusWrapper;
		mpDBusWrapper = NULL;
	}
	if (mpDBusReceiver != NULL)
	{
		delete mpDBusReceiver;
		mpDBusReceiver = NULL;
	}
	if (mpDBusSender != NULL)
	{
		delete mpDBusSender;
		mpDBusSender = NULL;
	}

    LOG_FN_EXIT(raContext);
}

am::am_Error_e CAppHotplugSenderShadow::init(CAmSocketHandler *socketHandler,
											IRaHotplugSend* hotplugSender,
											std::string busName,
											std::string busObjectPath,
											std::string interfaceName,
											std::string interfaceObjectPath,
											DBusBusType type)
{
	LOG_FN_ENTRY(raContext);
	if (socketHandler == NULL || hotplugSender == NULL)
	{
		LOG_ERROR(raContext, DLT_STRING("CAppHotplugSenderShadow::init mainLoopHandler == NULL || hotplugSender == NULL"));
		return am::E_UNKNOWN;
	}

	mpDBusSender = new CDBusSender();
	if (mpDBusSender == NULL)
	{
		DLT_LOG(raContext, DLT_LOG_ERROR, DLT_STRING("CAppHotplugSenderShadow::init mpDBusSender == NULL"));
		return am::E_UNKNOWN;
	}

	mpDBusReceiver = new CDBusReceiver();
	if (mpDBusReceiver == NULL)
	{
		DLT_LOG(raContext, DLT_LOG_ERROR, DLT_STRING("CAppHotplugSenderShadow::init mpDBusReceiver == NULL"));
		return am::E_UNKNOWN;
	}

	mpHotplugSend = hotplugSender;
	mpSocketHandler = socketHandler;
	gObjectPathVTable.message_function = receiveCallback;
	DBusConnection* connection;
	mpDBusWrapper = new CAmDbusWrapper(mpSocketHandler, type, busName.c_str(), busObjectPath.c_str());

	if (mpDBusWrapper == NULL)
	{
		LOG_ERROR(raContext, DLT_STRING("CAppHotplugSenderShadow::setHotplugReceiver mpDBusWrapper == NULL"));
		return am::E_UNKNOWN;
	}

	mpDBusWrapper->getDBusConnection(connection);
	if (connection == NULL)
	{
		LOG_ERROR(raContext, DLT_STRING("mpDBusWrapper->getDBusConnection connection == NULL"));
		return am::E_UNKNOWN;
	}
	mpDBusSender->setPathPrefix(busObjectPath,busName);
	LOG_DEBUG(raContext, DLT_STRING(busName.c_str()),
    			DLT_STRING(" interfaceName: "),
    			DLT_STRING(interfaceName.c_str()),
    			DLT_STRING(" interfaceObjectPath: "),
    			DLT_STRING(interfaceObjectPath.c_str()));

	mpDBusSender->setDBusConnection(connection);
	mpDBusReceiver->setDBusConnection(connection);
	if (connection == NULL)
	{
		LOG_ERROR(raContext, DLT_STRING("mDBusReceiver.setDBusConnection connection == NULL"));
	}

	std::string path(HOTPLUG_SEND_NODE);
	{
		assert(hotplugSender != NULL);
	}
	mpDBusWrapper->registerCallback(&gObjectPathVTable, path, this, busObjectPath);

	LOG_FN_EXIT(raContext);
	return am::E_OK;
}

void CAppHotplugSenderShadow::asyncSetSinkVolume(DBusConnection *conn, DBusMessage *msg)
{
	(void) ((conn));
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
	am::am_volume_t volume(mpDBusReceiver->getInt());
	am::am_CustomRampType_t ramp(static_cast<am::am_CustomRampType_t>(mpDBusReceiver->getInt()));
	am::am_time_t rampTime(mpDBusReceiver->getUInt());
	mpHotplugSend->asyncSetSinkVolume(handle, sinkID, volume, ramp, rampTime);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::asyncSetSourceVolume(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	am::am_volume_t volume(mpDBusReceiver->getInt());
	am::am_CustomRampType_t ramp(static_cast<am::am_CustomRampType_t>(mpDBusReceiver->getInt()));
	am::am_time_t rampTime(mpDBusReceiver->getUInt());
	mpHotplugSend->asyncSetSourceVolume(handle, sourceID, volume, ramp, rampTime);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::asyncSetSourceState(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	am::am_SourceState_e state(static_cast<am::am_SourceState_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->asyncSetSourceState(handle, sourceID, state);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::asyncSetSinkSoundProperties(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
	std::vector<am::am_SoundProperty_s> soundProperties;
	mpDBusReceiver->getListSoundProperties(soundProperties);
	mpHotplugSend->asyncSetSinkSoundProperties(handle, sinkID, soundProperties);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::asyncSetSinkSoundProperty(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
	am::am_SoundProperty_s soundProperty;
	mpDBusReceiver->getSoundProperty(soundProperty);
	mpHotplugSend->asyncSetSinkSoundProperty(handle, sinkID, soundProperty);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::asyncSetSourceSoundProperties(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	std::vector<am::am_SoundProperty_s> soundProperties;
	mpDBusReceiver->getListSoundProperties(soundProperties);
	mpHotplugSend->asyncSetSourceSoundProperties(handle, sourceID, soundProperties);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::asyncSetSourceSoundProperty(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	uint16_t handle(mpDBusReceiver->getUInt());
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	am::am_SoundProperty_s soundProperty;
	mpDBusReceiver->getSoundProperty(soundProperty);
	mpHotplugSend->asyncSetSourceSoundProperty(handle, sourceID, soundProperty);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::ackRegisterSink(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	std::string sinkName(mpDBusReceiver->getString());
	am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
	am::am_Error_e error(static_cast<am::am_Error_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->ackRegisterSink(sinkName, sinkID, error);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::ackDeregisterSink(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
	am::am_Error_e error(static_cast<am::am_Error_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->ackDeregisterSink(sinkID, error);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::ackRegisterSource(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	std::string sourceName(mpDBusReceiver->getString());
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	am::am_Error_e error(static_cast<am::am_Error_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->ackRegisterSource(sourceName, sourceID, error);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::ackDeregisterSource(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
    mpDBusReceiver->initReceive(msg);
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	am::am_Error_e error(static_cast<am::am_Error_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->ackDeregisterSource(sourceID, error);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::ackUpdateSource(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
	mpDBusReceiver->initReceive(msg);
	am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	am::am_Error_e error(static_cast<am::am_Error_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->ackUpdateSource(sourceID, error);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::ackUpdateSink(DBusConnection *conn, DBusMessage *msg)
{
	(void) (conn);
	LOG_FN_ENTRY(raContext);
	mpDBusReceiver->initReceive(msg);
	am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
	am::am_Error_e error(static_cast<am::am_Error_e>(mpDBusReceiver->getInt()));
	mpHotplugSend->ackUpdateSink(sinkID, error);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void CAppHotplugSenderShadow::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
	LOG_FN_ENTRY(raContext);
	assert(conn != NULL);
	assert(msg != NULL);
	DBusMessage *reply;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;

	reply = dbus_message_new_method_return(msg);
	std::string fullpath(HOTPLUG_SEND_DBUS_INTROSPECTION_FILE);
	std::ifstream introspection_file(fullpath.c_str(), std::ifstream::in);
	if (!introspection_file)
	{
		LOG_ERROR(raContext, DLT_STRING("AppHotplugSenderShadow::sendIntrospection could not load xml file "), DLT_STRING(fullpath.c_str()));
		throw std::runtime_error("AppHotplugSenderShadow::sendIntrospection could not load introspection XML");
	}

	std::string introspect((std::istreambuf_iterator<char>(introspection_file)), std::istreambuf_iterator<char>());
	const char *string = introspect.c_str();

	// add arguments to the reply
	dbus_message_iter_init_append(reply, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &string))
	{
		LOG_ERROR(raContext, DLT_STRING("DBus handler out of memory"));
	}

	// send the reply and flush the connection
	if (!dbus_connection_send(conn, reply, &serial))
	{
		LOG_ERROR(raContext, DLT_STRING("DBus handler out of memory"));
	}

	// free the reply
	dbus_message_unref(reply);
	LOG_FN_EXIT(raContext);
}

DBusHandlerResult CAppHotplugSenderShadow::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
{
	LOG_FN_ENTRY(raContext);
	if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "Introspect"))
	{
		sendIntrospection(conn, msg);
		LOG_FN_EXIT(raContext);
		return (DBUS_HANDLER_RESULT_HANDLED);
	}

	functionMap_t::iterator iter = mFunctionMap.begin();
	std::string k(dbus_message_get_member(msg));
	LOG_DEBUG(raContext, DLT_STRING(k.c_str()));
	iter = mFunctionMap.find(k);
	if (iter != mFunctionMap.end())
	{
		std::string p(iter->first);
		CallBackMethod cb = iter->second;
		(this->*cb)(conn, msg);
		LOG_FN_EXIT(raContext);
		return (DBUS_HANDLER_RESULT_HANDLED);
	}

	LOG_FN_EXIT(raContext);
	return (DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

void CAppHotplugSenderShadow::getConnection(DBusConnection *& connection)
{
	LOG_FN_ENTRY(raContext);
	mpDBusWrapper->getDBusConnection(connection);
	LOG_FN_EXIT(raContext);
}

CAppHotplugSenderShadow::functionMap_t CAppHotplugSenderShadow::createMap(void)
{
	LOG_FN_ENTRY(raContext);
	functionMap_t m;

	m["asyncSetSinkVolume"] = &CAppHotplugSenderShadow::asyncSetSinkVolume;
	m["asyncSetSourceVolume"] = &CAppHotplugSenderShadow::asyncSetSourceVolume;
	m["asyncSetSourceState"] = &CAppHotplugSenderShadow::asyncSetSourceState;
	m["asyncSetSinkSoundProperties"] = &CAppHotplugSenderShadow::asyncSetSinkSoundProperties;
	m["asyncSetSinkSoundProperty"] = &CAppHotplugSenderShadow::asyncSetSinkSoundProperty;
	m["asyncSetSourceSoundProperties"] = &CAppHotplugSenderShadow::asyncSetSourceSoundProperties;
	m["asyncSetSourceSoundProperty"] = &CAppHotplugSenderShadow::asyncSetSourceSoundProperty;
	m["ackRegisterSink"] = &CAppHotplugSenderShadow::ackRegisterSink;
	m["ackDeregisterSink"] = &CAppHotplugSenderShadow::ackDeregisterSink;
	m["ackRegisterSource"] = &CAppHotplugSenderShadow::ackRegisterSource;
	m["ackDeregisterSource"] = &CAppHotplugSenderShadow::ackDeregisterSource;
	m["ackUpdateSource"] = &CAppHotplugSenderShadow::ackUpdateSource;
	m["ackUpdateSink"] = &CAppHotplugSenderShadow::ackUpdateSink;

	LOG_FN_EXIT(raContext);
	return m;
}
