/************************************************************************
 * @file: CDBusCommandSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CCpDbusWrpSender class implementation of command plug-in.
 * A wrapper class for sender class. CCpDbusWrpSender class will forward the
 * call to Application.
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

#include <fstream> // for ifstream
#include <stdexcept> // for runtime_error
#include <vector>
#include "CAmDltWrapper.h"
#include "CDBusCommandSender.h"
#include "CDBusCommon.h"
#include "IDBusCommandClient.h"

using namespace std;
using namespace am;

DLT_DECLARE_CONTEXT (CP_CorD)

CDBusCommandSender::CDBusCommandSender(IDBusCommandClient* const client, CAmDbusWrapper*& wrapper) :
        mpCAmDbusWrapper(wrapper), mpIDBusCommandClient(client), mFunctionMap(createMap())
{
    try
    {
        CAmDltWrapper::instance()->registerContext(CP_CorD, "CP_CorD", "CP_Context");

        DBusConnection *connection;
        mpCAmDbusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mCDBusReceiver.setDBusConnection(connection);
            mpCAmDbusWrapper->registerSignalWatch(CDBusCommandSender::receiveCallback,
                    DBUS_SET_INTERFACE_RULE(COMMAND_DBUS_NAMESAPACE), this);
        }
        else
        {
            log(&CP_CorD, DLT_LOG_ERROR, "CDBusCommandSender::CDBusCommandSender DBus connection not created");
            std::runtime_error("CCpDbusWrpSender::CCpDbusWrpSender DBus connection not created");
        }
    }
    catch (...)
    {
        log(&CP_CorD, DLT_LOG_ERROR, "CDBusCommandSender::CDBusCommandSender Failed to create object");
        this->~CDBusCommandSender();
    }
}

CDBusCommandSender::~CDBusCommandSender()
{
    CAmDltWrapper::instance()->unregisterContext(CP_CorD);
}

void CDBusCommandSender::setCommandReady(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CDBusCommandSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    uint16_t handle = static_cast<uint16_t>(mCDBusReceiver.getUInt());
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->setCommandReady(handle);
    }
    else
    {
        log(&CP_CorD, DLT_LOG_ERROR, "CCpDbusWrpSender::setCommandReady mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::setCommandRundown(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CDBusCommandSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    uint16_t handle = static_cast<uint16_t>(mCDBusReceiver.getUInt());
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->setCommandRundown(handle);
    }
    else
    {
        log(&CP_CorD, DLT_LOG_ERROR, "CCpDbusWrpSender::setCommandRundown mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbNewMainConnection(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_MainConnectionType_s mainConnectionType;
    mCDBusReceiver.getMainConnectionType(mainConnectionType);
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbNewMainConnection(mainConnectionType);
    }
    else
    {
        logError("CCpDbusWrpSender::cbNewMainConnection mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbRemovedMainConnection(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");
    mCDBusReceiver.initReceive(msg);
    am_mainConnectionID_t mainConnection = static_cast<am_mainConnectionID_t>(mCDBusReceiver.getUInt());
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbRemovedMainConnection(mainConnection);
    }
    else
    {
        logError("CCpDbusWrpSender::cbRemovedMainConnection mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbNewSink(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_SinkType_s sink;
    mCDBusReceiver.getSinkType(sink);
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbNewSink(sink);
    }
    else
    {
        logError("CCpDbusWrpSender::cbNewSink mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbRemovedSink(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbRemovedSink(sinkID);
    }
    else
    {
        logError("CCpDbusWrpSender::cbRemovedSink mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbNewSource(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_SourceType_s source;
    mCDBusReceiver.getSourceType(source);
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbNewSource(source);
    }
    else
    {
        logError("CCpDbusWrpSender::cbNewSource mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbRemovedSource(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbRemovedSource(sourceID);
    }
    else
    {
        logError("CCpDbusWrpSender::cbRemovedSource mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbNumberOfSinkClassesChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbNumberOfSinkClassesChanged();
    }
    else
    {
        logError("CCpDbusWrpSender::cbNumberOfSinkClassesChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbNumberOfSourceClassesChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbNumberOfSourceClassesChanged();
    }
    else
    {
        logError("CCpDbusWrpSender::cbNumberOfSourceClassesChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbMainConnectionStateChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_mainConnectionID_t connectionID = static_cast<am_mainConnectionID_t>(mCDBusReceiver.getUInt());
    am_ConnectionState_e connectionState = static_cast<am_ConnectionState_e>(mCDBusReceiver.getInt());
    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbMainConnectionStateChanged(connectionID, connectionState);
    }
    else
    {
        logError("CCpDbusWrpSender::cbMainConnectionStateChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbMainSinkSoundPropertyChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_MainSoundProperty_s soundProperty;
    mCDBusReceiver.getMainSoundProperty(soundProperty);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbMainSinkSoundPropertyChanged(sinkID, soundProperty);
    }
    else
    {
        logError("CCpDbusWrpSender::cbMainSinkSoundPropertyChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbMainSourceSoundPropertyChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    am_MainSoundProperty_s soundProperty;
    mCDBusReceiver.getMainSoundProperty(soundProperty);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbMainSourceSoundPropertyChanged(sourceID, soundProperty);
    }
    else
    {
        logError("CCpDbusWrpSender::cbMainSourceSoundPropertyChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSinkAvailabilityChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_Availability_s availability;
    mCDBusReceiver.getAvailability(availability);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSinkAvailabilityChanged(sinkID, availability);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSinkAvailabilityChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSourceAvailabilityChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    am_Availability_s availability;
    mCDBusReceiver.getAvailability(availability);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSourceAvailabilityChanged(sourceID, availability);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSourceAvailabilityChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbVolumeChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_mainVolume_t volume = static_cast<am_mainVolume_t>(mCDBusReceiver.getInt());

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbVolumeChanged(sinkID, volume);
    }
    else
    {
        logError("CCpDbusWrpSender::cbVolumeChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSinkMuteStateChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_MuteState_e muteState = static_cast<am_MuteState_e>(mCDBusReceiver.getInt());

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSinkMuteStateChanged(sinkID, muteState);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSinkMuteStateChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSystemPropertyChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_SystemProperty_s systemProperty;
    mCDBusReceiver.getSystemProperty(systemProperty);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSystemPropertyChanged(systemProperty);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSystemPropertyChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbTimingInformationChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_mainConnectionID_t mainConnectionID = static_cast<am_mainConnectionID_t>(mCDBusReceiver.getUInt());
    am_timeSync_t time = static_cast<am_timeSync_t>(mCDBusReceiver.getInt());

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbTimingInformationChanged(mainConnectionID, time);
    }
    else
    {
        logError("CCpDbusWrpSender::cbTimingInformationChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSinkUpdated(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_sinkClass_t sinkClassID = static_cast<am_sinkClass_t>(mCDBusReceiver.getUInt());
    vector<am_MainSoundProperty_s> listMainSoundProperties;
    mCDBusReceiver.getListMainSoundProperties(listMainSoundProperties);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSinkUpdated(sinkID, sinkClassID, listMainSoundProperties);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSinkUpdated mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSourceUpdated(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    am_sourceClass_t sourceClassID = static_cast<am_sourceClass_t>(mCDBusReceiver.getUInt());
    vector<am_MainSoundProperty_s> listMainSoundProperties;
    mCDBusReceiver.getListMainSoundProperties(listMainSoundProperties);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSourceUpdated(sourceID, sourceClassID, listMainSoundProperties);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSourceUpdated mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSinkNotification(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_NotificationPayload_s notification;
    mCDBusReceiver.getNotificationPayload(notification);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSinkNotification(sinkID, notification);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSinkNotification mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbSourceNotification(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    am_NotificationPayload_s notification;
    mCDBusReceiver.getNotificationPayload(notification);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbSourceNotification(sourceID, notification);
    }
    else
    {
        logError("CCpDbusWrpSender::cbSourceNotification mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbMainSinkNotificationConfigurationChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sinkID_t sinkID = static_cast<am_sinkID_t>(mCDBusReceiver.getUInt());
    am_NotificationConfiguration_s mainNotificationConfiguration;
    mCDBusReceiver.getNotificationConfiguration(mainNotificationConfiguration);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbMainSinkNotificationConfigurationChanged(sinkID, mainNotificationConfiguration);
    }
    else
    {
        logError("CCpDbusWrpSender::cbMainSinkNotificationConfigurationChanged mpIAmCommandClient should not be NULL");
    }
}

void CDBusCommandSender::cbMainSourceNotificationConfigurationChanged(DBusMessage *msg)
{
    log(&CP_CorD, DLT_LOG_DEBUG, "CCpDbusWrpSender::",__func__," gets called");

    mCDBusReceiver.initReceive(msg);
    am_sourceID_t sourceID = static_cast<am_sourceID_t>(mCDBusReceiver.getUInt());
    am_NotificationConfiguration_s mainNotificationConfiguration;
    mCDBusReceiver.getNotificationConfiguration(mainNotificationConfiguration);

    if (NULL != mpIDBusCommandClient)
    {
        mpIDBusCommandClient->cbMainSourceNotificationConfigurationChanged(sourceID, mainNotificationConfiguration);
    }
    else
    {
        logError("CCpDbusWrpSender::cbMainSourceNotificationConfigurationChanged mpIAmCommandClient should not be NULL");
    }
}

DBusHandlerResult CDBusCommandSender::receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    DBusHandlerResult ret_val = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if ((conn != NULL) && (msg != NULL) && (user_data != NULL))
    {
        CDBusCommandSender* reference = (CDBusCommandSender*) ((user_data));
        ret_val = reference->receiveCallbackDelegate(conn, msg);
    }
    else
    {
        log(&CP_CorD, DLT_LOG_ERROR, "CCpDbusWrpSender::receiveCallback DBus pointer not initialised");
    }
    return ret_val;
}

void CDBusCommandSender::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
    if ((conn != NULL) && (msg != NULL))
    {
        DBusMessage* reply;
        DBusMessageIter args;
        dbus_uint32_t serial = 0;

        // create a reply from the message
        reply = dbus_message_new_method_return(msg);
        string fullpath(COMMAND_SEND_DBUS_INTROSPECTION_FILE);
        ifstream in(fullpath.c_str(), ifstream::in);
        if (!in)
        {
            throw runtime_error("IAmCommandReceiverShadow::sendIntrospection Could not load introspecton XML");
        }
        string introspect((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        const char* string = introspect.c_str();
        // add the arguments to the reply
        dbus_message_iter_init_append(reply, &args);
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &string))
        {
            log(&CP_CorD, DLT_LOG_INFO, "DBUS handler Out Of Memory!");
        }
        // send the reply && flush the connection
        if (!dbus_connection_send(conn, reply, &serial))
        {
            log(&CP_CorD, DLT_LOG_INFO, "DBUS handler Out Of Memory!");
        }
        dbus_connection_flush(conn);
        // free the reply
        dbus_message_unref(reply);
    }
    else
    {
        log(&CP_CorD, DLT_LOG_ERROR, "CCpDbusWrpSender::sendIntrospection DBus pointer not initialised");
    }
}

DBusHandlerResult CDBusCommandSender::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
{
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "CallBack"))
    {
        sendIntrospection(conn, msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    functionMap_t::iterator iter = mFunctionMap.begin();
    /*
     * The following check prevents DBus issues in case of receiving messages of
     * type DBUS_MESSAGE_TYPE_METHOD_RETURN, seen on ApplicationClient implementation.
     */
    if( msg != NULL && dbus_message_get_type(msg) == DBUS_MESSAGE_TYPE_SIGNAL )
    {
        string k(dbus_message_get_member(msg));
        iter = mFunctionMap.find(k);

        if (iter != mFunctionMap.end())
        {
            string p(iter->first);
            CallBackMethod cb = iter->second;
            (this->*cb)(msg);
            return (DBUS_HANDLER_RESULT_HANDLED);
        }
    }
    return (DBUS_HANDLER_RESULT_NOT_YET_HANDLED);
}

CDBusCommandSender::functionMap_t CDBusCommandSender::createMap()
{
    functionMap_t m;
    m["setCommandReady"] = &CDBusCommandSender::setCommandReady;
    m["setCommandRundown"] = &CDBusCommandSender::setCommandRundown;
    m["cbRemovedMainConnection"] = &CDBusCommandSender::cbRemovedMainConnection;

    return (m);
}

