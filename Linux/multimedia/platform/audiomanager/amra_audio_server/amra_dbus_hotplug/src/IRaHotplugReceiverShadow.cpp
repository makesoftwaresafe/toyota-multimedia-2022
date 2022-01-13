/************************************************************************
* @file: IRaHotplugReceiverShadow.cpp
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
#include <fstream>
#include <assert.h>
#include "IRaHotplugReceiverShadow.h"
#include <dlt/dlt.h>
#include <stdexcept>
#include "CDBusSender.h"
#include "CDBusReceiver.h"
#include "CAmDbusWrapper.h"
#include "Log.h"

DLT_IMPORT_CONTEXT(raContext)
#define HOTPLUG_RECEIVE_NODE "hotplugreceive"
#define HOTPLUG_RECEIVE_DBUS_INTROSPECTION_FILE "/usr/share/audiomanager/HotplugReceiver.xml"
static DBusObjectPathVTable gObjectPathVTable;

static DBusHandlerResult receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    LOG_FN_ENTRY(raContext);
    assert(conn != NULL);
    assert(msg != NULL);
    assert(user_data != NULL);
    IRaHotplugReceiverShadow* reference = (IRaHotplugReceiverShadow*) ((user_data));
    LOG_FN_EXIT(raContext);
    return (reference->receiveCallbackDelegate(conn, msg));
}


IRaHotplugReceiverShadow::IRaHotplugReceiverShadow() :
	mpHotplugReceive(),
	mFunctionMap(createMap()),
	mpDBusSender(NULL),
	mpDBusReceiver(NULL),
	mpDBusWrapper(NULL)
{
    LOG_FN_ENTRY(raContext);
    LOG_FN_EXIT(raContext);
}

IRaHotplugReceiverShadow::~IRaHotplugReceiverShadow()
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

am::am_Error_e IRaHotplugReceiverShadow::init(CAmSocketHandler *socketHandler,
											IRaHotplugReceive* hotplugReceiver,
											std::string busName,
											std::string busObjectPath,
											std::string interfaceName,
											std::string interfaceObjectPath,
											DBusBusType type)
{
    LOG_FN_ENTRY(raContext);
    if (socketHandler == NULL || hotplugReceiver == NULL)
    {
    	LOG_ERROR(raContext, DLT_STRING("IRaHotplugReceiverShadow::init mainLoopHandler == NULL || hotplugReceiver == NULL"));
        LOG_FN_EXIT(raContext);
    	return am::E_UNKNOWN;
    }

    mpDBusSender = new CDBusSender();
    if (mpDBusSender == NULL)
    {
       	LOG_ERROR(raContext, DLT_STRING(" mpDBusSender == NULL "));
        return am::E_UNKNOWN;
    }

    mpDBusReceiver = new CDBusReceiver();
    if (mpDBusReceiver == NULL)
    {
       	LOG_ERROR(raContext, DLT_STRING(" mpDBusSender == NULL "));
        return am::E_UNKNOWN;
    }

    mpHotplugReceive = hotplugReceiver;
    mpSocketHandler = socketHandler;
    gObjectPathVTable.message_function = receiveCallback;
    DBusConnection* connection;
    mpDBusWrapper = new CAmDbusWrapper(mpSocketHandler, type, busName.c_str(), busObjectPath.c_str());
    if (mpDBusWrapper == NULL)
    {
    	LOG_ERROR(raContext, DLT_STRING("IRaHotplugReceiverShadow::setHotplugReceiver mpDBusWrapper == NULL"));
        LOG_FN_EXIT(raContext);
    	return am::E_UNKNOWN;
    }

    mpDBusWrapper->getDBusConnection(connection);
    if (connection == NULL)
    {
    	LOG_ERROR(raContext, DLT_STRING("mpDBusWrapper->getDBusConnection connection == NULL"));
        LOG_FN_EXIT(raContext);
    	return am::E_UNKNOWN;
    }
    mpDBusSender->setPathPrefix(busObjectPath,busName );
    LOG_DEBUG(raContext, DLT_STRING(" busName : "),
    			DLT_STRING(busName.c_str()),
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
    std::string path(HOTPLUG_RECEIVE_NODE);
    {
        assert(hotplugReceiver != NULL);
    }
    mpDBusWrapper->registerCallback(&gObjectPathVTable, path, this, busObjectPath);

    LOG_FN_EXIT(raContext);
    return am::E_OK;
}

void IRaHotplugReceiverShadow::confirmHotplugReady(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle(mpDBusReceiver->getUInt());
    uint16_t error(mpDBusReceiver->getUInt());
    mpHotplugReceive->confirmHotplugReady(handle, (am::am_Error_e) error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::asyncRegisterSink(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_Sink_s sink;
    mpDBusReceiver->getSinkData(sink);
    mpHotplugReceive->asyncRegisterSink(sink);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::asyncDeregisterSink(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t sinkID(mpDBusReceiver->getUInt());
    mpHotplugReceive->asyncDeregisterSink(sinkID);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::asyncRegisterSource(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_Source_s source;
    mpDBusReceiver->getSourceData(source);
    mpHotplugReceive->asyncRegisterSource(source);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::asyncDeregisterSource(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t sourceID(mpDBusReceiver->getUInt());
    mpHotplugReceive->asyncDeregisterSource(sourceID);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSinkVolumeChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle(mpDBusReceiver->getUInt());
    am::am_volume_t volume(mpDBusReceiver->getInt());
    am::am_Error_e error((am::am_Error_e)((mpDBusReceiver->getUInt())));
    mpHotplugReceive->ackSetSinkVolumeChange(handle, volume, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSourceVolumeChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle(mpDBusReceiver->getUInt());
    am::am_volume_t volume(mpDBusReceiver->getInt());
    am::am_Error_e error((am::am_Error_e)((mpDBusReceiver->getUInt())));
    mpHotplugReceive->ackSetSourceVolumeChange(handle, volume, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSourceState(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle(mpDBusReceiver->getUInt());
    am::am_Error_e error((am::am_Error_e)((mpDBusReceiver->getUInt())));
    mpHotplugReceive->ackSetSourceState(handle, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSinkSoundProperties(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle = mpDBusReceiver->getUInt();
    am::am_Error_e error = (am::am_Error_e)((mpDBusReceiver->getUInt()));
    mpHotplugReceive->ackSetSinkSoundProperties(handle, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSinkSoundProperty(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle(mpDBusReceiver->getUInt());
    am::am_Error_e error((am::am_Error_e)((mpDBusReceiver->getUInt())));
    mpHotplugReceive->ackSetSinkSoundProperty(handle, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSourceSoundProperties(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle = mpDBusReceiver->getUInt();
    am::am_Error_e error = (am::am_Error_e)((mpDBusReceiver->getUInt()));
    mpHotplugReceive->ackSetSourceSoundProperties(handle, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::ackSetSourceSoundProperty(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    uint16_t handle(mpDBusReceiver->getUInt());
    am::am_Error_e error((am::am_Error_e)((mpDBusReceiver->getUInt())));
    mpHotplugReceive->ackSetSourceSoundProperty(handle, error);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::hookInterruptStatusChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_sourceID_t sourceID = mpDBusReceiver->getUInt();
    am::am_InterruptState_e interruptState = (am::am_InterruptState_e)((mpDBusReceiver->getUInt()));
    mpHotplugReceive->hookInterruptStatusChange(sourceID, interruptState);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::hookSinkAvailablityStatusChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_sinkID_t sinkID = mpDBusReceiver->getUInt();
    am::am_Availability_s avialabilty;
    mpDBusReceiver->getAvailability(avialabilty);
    mpHotplugReceive->hookSinkAvailablityStatusChange(sinkID, avialabilty);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::hookSourceAvailablityStatusChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_sourceID_t sourceID = mpDBusReceiver->getUInt();
    am::am_Availability_s avialabilty;
    mpDBusReceiver->getAvailability(avialabilty);
    mpHotplugReceive->hookSourceAvailablityStatusChange(sourceID, avialabilty);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::hookSinkNotificationDataChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
    am::am_NotificationPayload_s payload;
    mpDBusReceiver->getNotificationPayload(payload);
    mpHotplugReceive->hookSinkNotificationDataChange(sinkID, payload);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::hookSourceNotificationDataChange(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
    am::am_NotificationPayload_s payload;
    mpDBusReceiver->getNotificationPayload(payload);
    mpHotplugReceive->hookSourceNotificationDataChange(sourceID, payload);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();
    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::asyncUpdateSink(DBusConnection *conn, DBusMessage *msg)
{
    (void) ((conn));
    LOG_FN_ENTRY(raContext);
    assert(mpHotplugReceive != NULL);
    mpDBusReceiver->initReceive(msg);
    am::am_sinkID_t sinkID(mpDBusReceiver->getUInt());
    am::am_sinkClass_t sinkClassID(mpDBusReceiver->getUInt());
    std::vector<am::am_SoundProperty_s> listSoundProperties;
    mpDBusReceiver->getListSoundProperties(listSoundProperties);
    std::vector<am::am_CustomConnectionFormat_t> listSinkConnectionFormats;
    mpDBusReceiver->getListConnFrmt(listSinkConnectionFormats);
    std::vector<am::am_MainSoundProperty_s> listMainSoundProperties;
    mpDBusReceiver->getListMainSoundProperties(listMainSoundProperties);

    mpHotplugReceive->asyncUpdateSink(sinkID,sinkClassID,listSoundProperties,listSinkConnectionFormats,listMainSoundProperties);
    mpDBusSender->initReply(msg);
    mpDBusSender->sendMessage();

    LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::asyncUpdateSource(DBusConnection *conn, DBusMessage *msg)
{
	(void) ((conn));
	LOG_FN_ENTRY(raContext);
	    assert( mpHotplugReceive != NULL);
	    mpDBusReceiver->initReceive(msg);
	    am::am_sourceID_t sourceID(mpDBusReceiver->getUInt());
	    am::am_sourceClass_t sourceClassID(mpDBusReceiver->getUInt());
	    std::vector<am::am_SoundProperty_s> listSoundProperties;
	    mpDBusReceiver->getListSoundProperties(listSoundProperties);
	    std::vector<am::am_CustomConnectionFormat_t> listSinkConnectionFormats;
	    mpDBusReceiver->getListConnFrmt(listSinkConnectionFormats);
	    std::vector<am::am_MainSoundProperty_s> listMainSoundProperties;
	    mpDBusReceiver->getListMainSoundProperties(listMainSoundProperties);

	mpHotplugReceive->asyncUpdateSource(sourceID,sourceClassID,listSoundProperties,listSinkConnectionFormats,listMainSoundProperties);
	mpDBusSender->initReply(msg);
	mpDBusSender->sendMessage();
	LOG_FN_EXIT(raContext);
}

void IRaHotplugReceiverShadow::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
	LOG_FN_ENTRY(raContext);
	assert(conn != NULL);
	assert(msg != NULL);
	DBusMessage *reply;
	DBusMessageIter args;
	dbus_uint32_t serial = 0;

	reply = dbus_message_new_method_return(msg);
	std::string fullpath(HOTPLUG_RECEIVE_DBUS_INTROSPECTION_FILE);
	std::ifstream in(fullpath.c_str(), std::ifstream::in);
	if (!in)
	{
		LOG_ERROR(raContext, DLT_STRING("IRaHotplugReceiverShadow::sendIntrospection could not load xml file "), DLT_STRING(fullpath.c_str()));
		throw std::runtime_error("IRaHotplugReceiverShadow::sendIntrospection could not load introspection XML");
	}

	std::string introspect((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
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

DBusHandlerResult IRaHotplugReceiverShadow::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
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

void IRaHotplugReceiverShadow::getConnection(DBusConnection *& connection)
{
	LOG_FN_ENTRY(raContext);
	mpDBusWrapper->getDBusConnection(connection);
	LOG_FN_EXIT(raContext);
}

IRaHotplugReceiverShadow::functionMap_t IRaHotplugReceiverShadow::createMap(void)
{
	LOG_FN_ENTRY(raContext);
	functionMap_t m;
	m["confirmHotplugReady"]=&IRaHotplugReceiverShadow::confirmHotplugReady;
	m["asyncRegisterSink"]=&IRaHotplugReceiverShadow::asyncRegisterSink;
	m["asyncDeregisterSink"]=&IRaHotplugReceiverShadow::asyncDeregisterSink;
	m["asyncRegisterSource"]=&IRaHotplugReceiverShadow::asyncRegisterSource;
	m["asyncDeregisterSource"]=&IRaHotplugReceiverShadow::asyncDeregisterSource;
	m["ackSetSinkVolumeChange"]=&IRaHotplugReceiverShadow::ackSetSinkVolumeChange;
	m["ackSetSourceVolumeChang"]=&IRaHotplugReceiverShadow::ackSetSourceVolumeChange;
	m["ackSetSourceState"]=&IRaHotplugReceiverShadow::ackSetSourceState;
	m["ackSetSinkSoundProperties"]=&IRaHotplugReceiverShadow::ackSetSinkSoundProperties;
	m["ackSetSinkSoundProperty"]=&IRaHotplugReceiverShadow::ackSetSinkSoundProperty;
	m["ackSetSourceSoundProperties"]=&IRaHotplugReceiverShadow::ackSetSourceSoundProperties;
	m["ackSetSourceSoundProperty"]=&IRaHotplugReceiverShadow::ackSetSourceSoundProperty;
	m["hookInterruptStatusChange"]=&IRaHotplugReceiverShadow::hookInterruptStatusChange;
	m["hookSinkAvailablityStatusChange"]=&IRaHotplugReceiverShadow::hookSinkAvailablityStatusChange;
	m["hookSourceAvailablityStatusChange"]=&IRaHotplugReceiverShadow::hookSourceAvailablityStatusChange;
	m["hookSinkNotificationDataChange"]=&IRaHotplugReceiverShadow::hookSinkNotificationDataChange;
	m["hookSourceNotificationDataChange"]=&IRaHotplugReceiverShadow::hookSourceNotificationDataChange;
	m["asyncUpdateSink"]=&IRaHotplugReceiverShadow::asyncUpdateSink;
	m["asyncUpdateSource"]=&IRaHotplugReceiverShadow::asyncUpdateSource;

	LOG_FN_EXIT(raContext);
	return m;
}
