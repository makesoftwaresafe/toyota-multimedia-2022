/************************************************************************
 * @file: ICpDbusWrpReceiverShadow.cpp
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of command plug-in.
 * IAmDbusCommandReceiverShadow class will be running with the AM process.
 * IAmDbusCommandReceiverShadow class methods will be called via DBus to
 * this methods will intern call the Actual call in AM.
 * @component: platform/audiomanager
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2013,2014
 *          Jayanth MC, Jayanth.mc@in.bosch.com 2013,2014
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

#include <fstream> // for ifstream
#include <stdexcept> // for runtime_error
#include <string.h> // for memset
#include "IAmDbusCommandReceiverShadow.h"
#include "CAmDltWrapper.h"
#include "CDBusCommon.h"

DLT_IMPORT_CONTEXT (CP_Dbus)

using namespace am;
using namespace std;

/**
 * static ObjectPathTable is needed for DBus call back handling
 */

IAmDbusCommandReceiverShadow::IAmDbusCommandReceiverShadow() :
        mpIAmCommandReceive(NULL), mpCAmDBusWrapper(NULL), mFunctionMap(createMap())
{
    logInfo("IAmDbusCommandReceiverShadow object created");
}

IAmDbusCommandReceiverShadow::~IAmDbusCommandReceiverShadow()
{
    logInfo("~IAmDbusCommandReceiverShadow object destroyed");
}

void IAmDbusCommandReceiverShadow::getInterfaceVersion(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getInterfaceVersion gets called");
    string version;

    mCDBusReceiver.initReceive(msg);
    mpIAmCommandReceive->getInterfaceVersion(version);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(version);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::connect(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::connect gets called");
    am_Error_e returnCode;
    am_sourceID_t sourceID;
    am_sinkID_t sinkID;
    am_mainConnectionID_t mainConnectionID;

    mCDBusReceiver.initReceive(msg);
    sourceID = mCDBusReceiver.getUInt();
    sinkID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->connect(sourceID, sinkID, mainConnectionID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(mainConnectionID);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::disconnect(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::disconnect gets called");
    am_Error_e returnCode;
    am_mainConnectionID_t mainConnectionID;

    mCDBusReceiver.initReceive(msg);
    mainConnectionID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->disconnect(mainConnectionID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::setVolume(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setVolume gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    am_mainVolume_t volume;

    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    volume = mCDBusReceiver.getInt();
    returnCode = mpIAmCommandReceive->setVolume(sinkID, volume);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();

}

void IAmDbusCommandReceiverShadow::volumeStep(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::volumeStep gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    int16_t volStep;

    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    volStep = mCDBusReceiver.getInt();
    returnCode = mpIAmCommandReceive->volumeStep(sinkID, volStep);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::setSinkMuteState(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setSinkMuteState gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    am_MuteState_e muteState;

    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    muteState = static_cast<am_MuteState_e>(mCDBusReceiver.getInt());
    returnCode = mpIAmCommandReceive->setSinkMuteState(sinkID, muteState);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();

}

void IAmDbusCommandReceiverShadow::setMainSinkSoundProperty(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setMainSinkSoundProperty gets called");
    am_Error_e returnCode;
    am_MainSoundProperty_s soundProperty;
    am_sinkID_t sinkID;

    mCDBusReceiver.initReceive(msg);
    mCDBusReceiver.getMainSoundProperty(soundProperty);
    sinkID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->setMainSinkSoundProperty(soundProperty, sinkID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::setMainSourceSoundProperty(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setMainSourceSoundProperty gets called");
    am_Error_e returnCode;
    am_MainSoundProperty_s soundProperty;
    am_sourceID_t sourceID;

    mCDBusReceiver.initReceive(msg);
    mCDBusReceiver.getMainSoundProperty(soundProperty);
    sourceID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->setMainSourceSoundProperty(soundProperty, sourceID);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::setSystemProperty(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setSystemProperty gets called");
    am_Error_e returnCode;
    am_SystemProperty_s property;

    mCDBusReceiver.initReceive(msg);
    mCDBusReceiver.getSystemProperty(property);
    returnCode = mpIAmCommandReceive->setSystemProperty(property);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainConnections(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainConnections gets called");
    am_Error_e returnCode;
    vector<am_MainConnectionType_s> listConnections;

    mCDBusReceiver.initReceive(msg);
    returnCode = mpIAmCommandReceive->getListMainConnections(listConnections);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listConnections);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainSinks(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainSinks gets called");
    am_Error_e returnCode;
    vector<am_SinkType_s> listMainSinks;

    mCDBusReceiver.initReceive(msg);
    returnCode = mpIAmCommandReceive->getListMainSinks(listMainSinks);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listMainSinks);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainSources(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainSources gets called");
    am_Error_e returnCode;
    vector<am_SourceType_s> listMainSources;

    mCDBusReceiver.initReceive(msg);
    returnCode = mpIAmCommandReceive->getListMainSources(listMainSources);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listMainSources);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainSinkSoundProperties(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainSinkSoundProperties gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    vector<am_MainSoundProperty_s> listSoundProperties;

    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->getListMainSinkSoundProperties(sinkID, listSoundProperties);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listSoundProperties);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainSourceSoundProperties(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainSourceSoundProperties gets called");
    am_Error_e returnCode;
    am_sourceID_t sourceID;
    vector<am_MainSoundProperty_s> listSoundProperties;

    mCDBusReceiver.initReceive(msg);
    sourceID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->getListMainSourceSoundProperties(sourceID, listSoundProperties);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listSoundProperties);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListSourceClasses(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListSourceClasses gets called");
    am_Error_e returnCode;
    vector<am_SourceClass_s> listSourceClasses;

    mCDBusReceiver.initReceive(msg);
    returnCode = mpIAmCommandReceive->getListSourceClasses(listSourceClasses);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listSourceClasses);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListSinkClasses(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListSinkClasses gets called");
    am_Error_e returnCode;
    vector<am_SinkClass_s> listSinkClasses;

    mCDBusReceiver.initReceive(msg);
    returnCode = mpIAmCommandReceive->getListSinkClasses(listSinkClasses);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listSinkClasses);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListSystemProperties(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListSystemProperties gets called");
    am_Error_e returnCode;
    vector<am_SystemProperty_s> listSystemProperties;

    mCDBusReceiver.initReceive(msg);
    returnCode = mpIAmCommandReceive->getListSystemProperties(listSystemProperties);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listSystemProperties);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getTimingInformation(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getTimingInformation gets called");
    am_Error_e returnCode;
    am_mainConnectionID_t mainConnectionID;
    am_timeSync_t delay;

    mCDBusReceiver.initReceive(msg);
    mainConnectionID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->getTimingInformation(mainConnectionID, delay);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(delay);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getDBusConnectionWrapper(DBusMessage *msg)
{
    /* Need to check with Jens how to get the CAmDbusWrapper pointer*/
    (void) (msg);
}

void IAmDbusCommandReceiverShadow::getSocketHandler(DBusMessage *msg)
{
    /* Need to check with Jens how to get the CAmSocketHandler pointer */
    (void) (msg);
}

void IAmDbusCommandReceiverShadow::confirmCommandReady(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::confirmCommandReady gets called");
    uint16_t handle;
    am_Error_e error;

    mCDBusReceiver.initReceive(msg);
    handle = mCDBusReceiver.getUInt();
    error = static_cast<am_Error_e>(mCDBusReceiver.getUInt());
    mpIAmCommandReceive->confirmCommandReady(handle, error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::confirmCommandRundown(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::confirmCommandRundown gets called");
    uint16_t handle;
    am_Error_e error;

    mCDBusReceiver.initReceive(msg);
    handle = mCDBusReceiver.getUInt();
    error = static_cast<am_Error_e>(mCDBusReceiver.getUInt());
    mpIAmCommandReceive->confirmCommandRundown(handle, error);
    mCDBusSender.initReply(msg);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainSinkNotificationConfigurations(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainSinkNotificationConfigurations gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    vector<am_NotificationConfiguration_s> listMainNotificationConfigurations;

    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->getListMainSinkNotificationConfigurations(sinkID, listMainNotificationConfigurations);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listMainNotificationConfigurations);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getListMainSourceNotificationConfigurations(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getListMainSourceNotificationConfigurations gets called");
    am_Error_e returnCode;
    am_sourceID_t sourceID;
    vector<am_NotificationConfiguration_s> listMainNotificationConfigurations;

    mCDBusReceiver.initReceive(msg);
    sourceID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->getListMainSourceNotificationConfigurations(sourceID,
                                                                                  listMainNotificationConfigurations);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(listMainNotificationConfigurations);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::setMainSinkNotificationConfiguration(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setMainSinkNotificationConfiguration gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    am_NotificationConfiguration_s mainNotificationConfiguration;

    memset(&mainNotificationConfiguration, 0, sizeof(am_NotificationConfiguration_s));
    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    mCDBusReceiver.getNotificationConfiguration(mainNotificationConfiguration);
    returnCode = mpIAmCommandReceive->setMainSinkNotificationConfiguration(sinkID, mainNotificationConfiguration);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::setMainSourceNotificationConfiguration(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::setMainSourceNotificationConfiguration gets called");
    am_Error_e returnCode;
    am_sourceID_t sourceID;
    am_NotificationConfiguration_s mainNotificationConfiguration;

    memset(&mainNotificationConfiguration, 0, sizeof(am_NotificationConfiguration_s));
    mCDBusReceiver.initReceive(msg);
    sourceID = mCDBusReceiver.getUInt();
    mCDBusReceiver.getNotificationConfiguration(mainNotificationConfiguration);
    returnCode = mpIAmCommandReceive->setMainSourceNotificationConfiguration(sourceID, mainNotificationConfiguration);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

void IAmDbusCommandReceiverShadow::getVolume(DBusMessage *msg)
{
    log(&CP_Dbus, DLT_LOG_INFO, "IAmDbusCommandReceiverShadow::getVolume gets called");
    am_Error_e returnCode;
    am_sinkID_t sinkID;
    am_mainVolume_t mainVolume;

    mCDBusReceiver.initReceive(msg);
    sinkID = mCDBusReceiver.getUInt();
    returnCode = mpIAmCommandReceive->getVolume(sinkID, mainVolume);

    mCDBusSender.initReply(msg);
    mCDBusSender.append(mainVolume);
    mCDBusSender.append(returnCode);
    mCDBusSender.sendMessage();
}

DBusHandlerResult IAmDbusCommandReceiverShadow::receiveCallback(DBusConnection* conn, DBusMessage* msg, void* user_data)
{
    DBusHandlerResult ret_val = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    if ((conn != NULL) && (msg != NULL) && (user_data != NULL))
    {
        IAmDbusCommandReceiverShadow* reference = (IAmDbusCommandReceiverShadow*) ((user_data));
        ret_val = reference->receiveCallbackDelegate(conn, msg);
    }
    else
    {
        logError("IAmDbusCommandReceiverShadow::receiveCallback DBus pointer or DBus message or user data not initialised");
    }
    return ret_val;
}

void IAmDbusCommandReceiverShadow::sendIntrospection(DBusConnection* conn, DBusMessage* msg)
{
    if ((conn != NULL) && (msg != NULL))
    {
        DBusMessage* reply;
        DBusMessageIter args;
        dbus_uint32_t serial = 0;

        // create a reply from the message
        reply = dbus_message_new_method_return(msg);
        string fullpath(COMMAND_RECV_DBUS_INTROSPECTION_FILE);
        ifstream in(fullpath.c_str(), ifstream::in);
        if (!in)
        {
            logError("IAmCommandReceiverShadow::sendIntrospection could not load xml file ", fullpath);
            throw runtime_error("IAmCommandReceiverShadow::sendIntrospection Could not load introspecton XML");
        }
        string introspect((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
        const char* string = introspect.c_str();

        // add the arguments to the reply
        dbus_message_iter_init_append(reply, &args);
        if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_STRING, &string))
        {
            log(&CP_Dbus, DLT_LOG_INFO, "DBUS handler Out Of Memory!");
        }

        // send the reply && flush the connection
        if (!dbus_connection_send(conn, reply, &serial))
        {
            log(&CP_Dbus, DLT_LOG_INFO, "DBUS handler Out Of Memory!");
        }
        dbus_connection_flush(conn);

        // free the reply
        dbus_message_unref(reply);
    }
    else
    {
        logError("IAmDbusCommandReceiverShadow::sendIntrospection DBus and/or Message pointer not initialised");
    }
}

DBusHandlerResult IAmDbusCommandReceiverShadow::receiveCallbackDelegate(DBusConnection* conn, DBusMessage* msg)
{
    if (dbus_message_is_method_call(msg, DBUS_INTERFACE_INTROSPECTABLE, "CallBack"))
    {
        sendIntrospection(conn, msg);
        return (DBUS_HANDLER_RESULT_HANDLED);
    }
    functionMap_t::iterator iter = mFunctionMap.begin();
    const char *methodName = dbus_message_get_member(msg);
    if (NULL != methodName) {
        string k(methodName);
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

void IAmDbusCommandReceiverShadow::setCommandReceiver(IAmCommandReceive*& receiver)
{
    mpIAmCommandReceive = receiver;
    DBusConnection* connection;
    mpIAmCommandReceive->getDBusConnectionWrapper(mpCAmDBusWrapper);
    if (mpCAmDBusWrapper != NULL)
    {
        mpCAmDBusWrapper->getDBusConnection(connection);
        if (connection != NULL)
        {
            mCDBusReceiver.setDBusConnection(connection);
            mCDBusSender.setDBusConnection(connection);
            mpCAmDBusWrapper->registerSignalWatch(IAmDbusCommandReceiverShadow::receiveCallback,
                    DBUS_SET_INTERFACE_RULE(COMMAND_DBUS_NAMESAPACE), this);
        }
        else
        {
            logError("IAmDbusCommandReceiverShadow::setRoutingReceiver Failed to get DBus connection pointer");
        }
    }
    else
    {
        logError("IAmDbusCommandReceiverShadow::setRoutingReceiver Failed to get DBus wrapper pointer");
    }
}

IAmDbusCommandReceiverShadow::functionMap_t IAmDbusCommandReceiverShadow::createMap()
{
    functionMap_t m;
    m["getInterfaceVersion"] = &IAmDbusCommandReceiverShadow::getInterfaceVersion;
    m["connect"] = &IAmDbusCommandReceiverShadow::connect;
    m["disconnect"] = &IAmDbusCommandReceiverShadow::disconnect;
    m["setVolume"] = &IAmDbusCommandReceiverShadow::setVolume;
    m["volumeStep"] = &IAmDbusCommandReceiverShadow::volumeStep;
    m["setSinkMuteState"] = &IAmDbusCommandReceiverShadow::setSinkMuteState;
    m["setMainSinkSoundProperty"] = &IAmDbusCommandReceiverShadow::setMainSinkSoundProperty;
    m["setMainSourceSoundProperty"] = &IAmDbusCommandReceiverShadow::setMainSourceSoundProperty;
    m["setSystemProperty"] = &IAmDbusCommandReceiverShadow::setSystemProperty;
    m["getListMainConnections"] = &IAmDbusCommandReceiverShadow::getListMainConnections;
    m["getListMainSinks"] = &IAmDbusCommandReceiverShadow::getListMainSinks;
    m["getListMainSources"] = &IAmDbusCommandReceiverShadow::getListMainSources;
    m["getListMainSinkSoundProperties"] = &IAmDbusCommandReceiverShadow::getListMainSinkSoundProperties;
    m["getListMainSourceSoundProperties"] = &IAmDbusCommandReceiverShadow::getListMainSourceSoundProperties;
    m["getListSourceClasses"] = &IAmDbusCommandReceiverShadow::getListSourceClasses;
    m["getListSinkClasses"] = &IAmDbusCommandReceiverShadow::getListSinkClasses;
    m["getListSystemProperties"] = &IAmDbusCommandReceiverShadow::getListSystemProperties;
    m["getTimingInformation"] = &IAmDbusCommandReceiverShadow::getTimingInformation;
    m["getDBusConnectionWrapper"] = &IAmDbusCommandReceiverShadow::getDBusConnectionWrapper;
    m["getSocketHandler"] = &IAmDbusCommandReceiverShadow::getSocketHandler;
    m["confirmCommandReady"] = &IAmDbusCommandReceiverShadow::confirmCommandReady;
    m["confirmCommandRundown"] = &IAmDbusCommandReceiverShadow::confirmCommandRundown;
    m["getListMainSinkNotificationConfigurations"] =
            &IAmDbusCommandReceiverShadow::getListMainSinkNotificationConfigurations;
    m["getListMainSourceNotificationConfigurations"] =
            &IAmDbusCommandReceiverShadow::getListMainSourceNotificationConfigurations;
    m["setMainSinkNotificationConfiguration"] = &IAmDbusCommandReceiverShadow::setMainSinkNotificationConfiguration;
    m["setMainSourceNotificationConfiguration"] = &IAmDbusCommandReceiverShadow::setMainSourceNotificationConfiguration;
    m["getVolume"] = &IAmDbusCommandReceiverShadow::getVolume;

    return (m);
}
