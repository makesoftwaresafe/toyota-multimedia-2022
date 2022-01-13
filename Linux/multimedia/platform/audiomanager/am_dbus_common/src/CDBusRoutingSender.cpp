/************************************************************************
 * @file: CDBusRoutingSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CDBusRoutingSender class implementation of Routing Adapter.
 * A wrapper class for sender class. CDBusRoutingSender class will call the
 * sender class API which has the actual sender API definition.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2016
 *          Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 * @copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/

#include <fstream> //for file operations
#include <stdexcept> // for runtime_error
#include <vector>
#include "audiomanagertypes.h"
#include "CAmDltWrapper.h"
#include "IDBusRoutingClient.h"
#include "CDBusRoutingSender.h"
#include <assert.h>
#include "CAmDbusWrapper.h"

using namespace std;
using namespace am;

static DBusObjectPathVTable gObjectPathVTable;

DLT_DECLARE_CONTEXT (RP_CorD)

CDBusRoutingSender::CDBusRoutingSender(IDBusRoutingClient* const client, CAmDbusWrapper*& wrapper) :
	mpCAmDbusWrapper(wrapper), mpIDBusRoutingClient(client), mFunctionMap(createMap())
{
    try
    {
        CAmDltWrapper::instance()->registerContext(RP_CorD, "RP_CorD", "RP_CContext");

        DBusConnection *connection;
        mpCAmDbusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mCDBusSender.setDBusConnection(connection);
            mCDBusReceiver.setDBusConnection(connection);
            mpCAmDbusWrapper->registerSignalWatch(CDBusRoutingSender::receiveCallback, DBUS_SET_INTERFACE_RULE(ROUTING_DBUS_NAMESPACE), this);
            gObjectPathVTable.message_function = CDBusRoutingSender::receiveCallback;
        }
        else
        {
            log(&RP_CorD, DLT_LOG_ERROR, "CDBusRoutingSender::CDBusRoutingSender DBus connection not created");
            std::runtime_error("CDBusRoutingSender::CDBusRoutingSender DBus connection not created");
        }
    }
    catch (...)
    {
        log(&RP_CorD, DLT_LOG_ERROR, "CDBusRoutingSender::CDBusRoutingSender Failed to create object");
        this->~CDBusRoutingSender();
    }
}

CDBusRoutingSender::~CDBusRoutingSender()
{
    CAmDltWrapper::instance()->unregisterContext(RP_CorD);
}

void CDBusRoutingSender::setRoutingReady(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    uint16_t handle(mCDBusReceiver.getUInt());

    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();

    mpIDBusRoutingClient->setRoutingReady(handle);
}

void CDBusRoutingSender::setRoutingRundown(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    uint16_t handle(mCDBusReceiver.getUInt());
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();

    mpIDBusRoutingClient->setRoutingRundown(handle);
}

void CDBusRoutingSender::asyncAbort(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_Error_e returnCode = mpIDBusRoutingClient->asyncAbort(handle.getAmHandle());
    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();

}

void CDBusRoutingSender::asyncConnect(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_connectionID_t connectionID(mCDBusReceiver.getUInt());
    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    am_sinkID_t sinkID(mCDBusReceiver.getUInt());
    am_CustomConnectionFormat_t connectionFormat = static_cast<am_CustomConnectionFormat_t>(mCDBusReceiver.getInt());
    am_Error_e returnCode = mpIDBusRoutingClient->asyncConnect(handle.getAmHandle(), connectionID, sourceID, sinkID, connectionFormat);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncDisconnect(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_connectionID_t connectionID(mCDBusReceiver.getUInt());
    am_Error_e returnCode = mpIDBusRoutingClient->asyncDisconnect(handle.getAmHandle(), connectionID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSinkVolume(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sinkID_t sinkID(mCDBusReceiver.getUInt());
    am_volume_t volume(mCDBusReceiver.getInt());
    am_CustomRampType_t ramp = static_cast<am_CustomRampType_t>(mCDBusReceiver.getInt());
    am_time_t amTime(mCDBusReceiver.getUInt());

    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSinkVolume(handle.getAmHandle(), sinkID, volume, ramp, amTime);
    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSourceVolume(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    am_volume_t volume(mCDBusReceiver.getInt());
    am_CustomRampType_t ramp = static_cast<am_CustomRampType_t>(mCDBusReceiver.getInt());
    am_time_t amTime(mCDBusReceiver.getUInt());
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSourceVolume(handle.getAmHandle(), sourceID, volume, ramp, amTime);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSourceState(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());

    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    am_SourceState_e sourceState = static_cast<am_SourceState_e>(mCDBusReceiver.getInt());
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSourceState(handle.getAmHandle(), sourceID, sourceState);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSinkSoundProperties(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    vector<am_SoundProperty_s> listSoundProperties;
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sinkID_t sinkID(mCDBusReceiver.getUInt());
    mCDBusReceiver.getListSoundProperties(listSoundProperties);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSinkSoundProperties(handle.getAmHandle(), sinkID, listSoundProperties);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}
void CDBusRoutingSender::asyncSetSinkSoundProperty(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    am_SoundProperty_s soundProperty;
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sinkID_t sinkID(mCDBusReceiver.getUInt());
    mCDBusReceiver.getSoundProperty(soundProperty);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSinkSoundProperty(handle.getAmHandle(), sinkID, soundProperty);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();

}

void CDBusRoutingSender::asyncSetSourceSoundProperties(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    vector<am_SoundProperty_s> listSoundProperties;
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    mCDBusReceiver.getListSoundProperties(listSoundProperties);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSourceSoundProperties(handle.getAmHandle(), sourceID, listSoundProperties);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSourceSoundProperty(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    am_SoundProperty_s soundProperty;
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    mCDBusReceiver.getSoundProperty(soundProperty);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSourceSoundProperty(handle.getAmHandle(), sourceID, soundProperty);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetVolumes(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    vector<am_Volumes_s> listVolumes;
    mCDBusReceiver.getListVolumes(listVolumes);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetVolumes(handle.getAmHandle(), listVolumes);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSinkNotificationConfiguration(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sinkID_t sinkID(mCDBusReceiver.getUInt());
    am_NotificationConfiguration_s NotificationConfiguration;
    mCDBusReceiver.getNotificationConfiguration(NotificationConfiguration);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSinkNotificationConfiguration(handle.getAmHandle(), sinkID,
                                                                                          NotificationConfiguration);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncSetSourceNotificationConfiguration(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_DEBUG, "CDBusRoutingSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_sourceID_t sourceID(mCDBusReceiver.getUInt());
    am_NotificationConfiguration_s NotificationConfiguration;
    mCDBusReceiver.getNotificationConfiguration(NotificationConfiguration);
    am_Error_e returnCode = mpIDBusRoutingClient->asyncSetSourceNotificationConfiguration(handle.getAmHandle(), sourceID,
                                                                                            NotificationConfiguration);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::asyncCrossFade(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_INFO,"CRpDbusWrpSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    handle_uint16_s handle(mCDBusReceiver.getUInt());
    am_crossfaderID_t crossfaderID(mCDBusReceiver.getUInt());
    am_HotSink_e hotSink = static_cast<am_HotSink_e>(mCDBusReceiver.getInt());
    am_CustomRampType_t rampType = static_cast<am_CustomRampType_t>(mCDBusReceiver.getInt());
    am_time_t amTime(mCDBusReceiver.getUInt());
    am_Error_e returnCode = mpIDBusRoutingClient->asyncCrossFade(handle.getAmHandle(), crossfaderID, hotSink, rampType, amTime);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void CDBusRoutingSender::setDomainState(DBusMessage *msg)
{
    log(&RP_CorD, DLT_LOG_INFO, "CRpDbusWrpSender::",__func__, "gets called");
    mCDBusReceiver.initReceive(msg);
    am_domainID_t domainID(mCDBusReceiver.getUInt());
    am_DomainState_e domainState = static_cast<am_DomainState_e>(mCDBusReceiver.getInt());

    am_Error_e returnCode = mpIDBusRoutingClient->setDomainState(domainID, domainState);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

DBusHandlerResult CDBusRoutingSender::receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    DBusHandlerResult ret_val = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    if ((conn != NULL) && (msg != NULL) && (user_data != NULL))
    {
        CDBusRoutingSender* ref = (CDBusRoutingSender*) ((user_data));
        ret_val = ref->receiveCallbackDelegate(conn, msg);
    }
    else
    {
        logError("#",__func__,"DBus pointer not initialised");
    }
    return ret_val;
}

void CDBusRoutingSender::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
    if ((conn != NULL) && (msg != NULL))
    {
        DBusMessage* reply;
        DBusMessageIter args;
        dbus_uint32_t serial = 0;

        // create a reply from the message
        reply = dbus_message_new_method_return(msg);
        string fullpath(ROUTING_SEND_DBUS_INTROSPECTION_FILE);
        ifstream in(fullpath.c_str(), ifstream::in);
        if (!in)
        {
            throw runtime_error("IAmCommandReceiverShadow::sendIntrospection Could not load introspecton XML");
        }
        string introspect((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        const char* str = introspect.c_str();
        // add the arguments to the reply
        dbus_message_iter_init_append(reply, &args);
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &str))
        {
            logError("#",__func__,"DBUS handler Out Of Memory!");
        }
        // send the reply && flush the connection
        if (!dbus_connection_send(conn, reply, &serial))
        {
            logError("#",__func__, "DBUS handler Out Of Memory!");
        }
        dbus_connection_flush(conn);
        // free the reply
        dbus_message_unref(reply);
    }
    else
    {
        logError("#",__func__,"DBus pointer not initialised");
    }
}

DBusHandlerResult CDBusRoutingSender::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
{
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "CallBack"))
    {
        sendIntrospection(conn, msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    char* messagename = (char*)dbus_message_get_member(msg);
    if(messagename == NULL)
    {
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
    }
    functionMap_t::iterator iter = mFunctionMap.begin();
    string k(messagename);
    iter = mFunctionMap.find(k);
    if (iter != mFunctionMap.end())
    {
        string p(iter->first);
        CallBackMethod cb = iter->second;
        (this->*cb)(msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    return (DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

CDBusRoutingSender::functionMap_t CDBusRoutingSender::createMap()
{
    functionMap_t m;

    m["setRoutingReady"] = &CDBusRoutingSender::setRoutingReady;
    m["setRoutingRundown"] = &CDBusRoutingSender::setRoutingRundown;
    m["asyncAbort"] = &CDBusRoutingSender::asyncAbort;
    m["asyncConnect"] = &CDBusRoutingSender::asyncConnect;
    m["asyncDisconnect"] = &CDBusRoutingSender::asyncDisconnect;
    m["asyncSetSinkVolume"] = &CDBusRoutingSender::asyncSetSinkVolume;
    m["asyncSetSourceVolume"] = &CDBusRoutingSender::asyncSetSourceVolume;
    m["asyncSetSourceState"] = &CDBusRoutingSender::asyncSetSourceState;
    m["asyncSetSinkSoundProperties"] = &CDBusRoutingSender::asyncSetSinkSoundProperties;
    m["asyncSetSinkSoundProperty"] = &CDBusRoutingSender::asyncSetSinkSoundProperty;
    m["asyncSetSourceSoundProperties"] = &CDBusRoutingSender::asyncSetSourceSoundProperties;
    m["asyncSetSourceSoundProperty"] = &CDBusRoutingSender::asyncSetSourceSoundProperty;
    m["asyncSetVolumes"] = &CDBusRoutingSender::asyncSetVolumes;
    m["asyncSetSinkNotificationConfiguration"] = &CDBusRoutingSender::asyncSetSinkNotificationConfiguration;
    m["asyncSetSourceNotificationConfiguration"] = &CDBusRoutingSender::asyncSetSourceNotificationConfiguration;

    return (m);
}

