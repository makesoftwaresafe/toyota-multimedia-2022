/************************************************************************
 * @file: IDBusCommandReceiver.h
 *
 * @version: 1.1
 *
 * @description: A Receiver class shadow implementation of command plug-in.
 * Receiver class will make call to AM via DBus connection.This calls will
 * be invoked by application to make the call.
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
#include <string.h>
#include <fstream>
#include <stdexcept>
#include "IDBusCommandReceiver.h"
#include "CDBusCommon.h"

DLT_IMPORT_CONTEXT (CP_Dbus)

using namespace std;
using namespace am;

IDBusCommandReceiver::IDBusCommandReceiver(DBusConnection* DbusConnection) :
        mpDBusConnection(DbusConnection), mDBusData(COMMAND_DBUS_NAMESAPACE)
{
}

IDBusCommandReceiver::~IDBusCommandReceiver()
{
}

void IDBusCommandReceiver::getInterfaceVersion(string& version) const
{
    CDBusSender send(mpDBusConnection, "getInterfaceVersion", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        version = receive.getString();
    }
}

am_Error_e IDBusCommandReceiver::connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                             am_mainConnectionID_t& mainConnectionID)
{
    CDBusSender send(mpDBusConnection, "connect", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(sourceID);
    send.append(sinkID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        mainConnectionID = receive.getUInt();
        ret =  (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::disconnect(const am_mainConnectionID_t mainConnectionID)
{
    CDBusSender send(mpDBusConnection, "disconnect", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(mainConnectionID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::setVolume(const am_sinkID_t sinkID, const am_mainVolume_t volume)
{
    CDBusSender send(mpDBusConnection, "setVolume", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(sinkID);
    send.append(volume);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::volumeStep(const am_sinkID_t sinkID, const int16_t volumeStep)
{
    CDBusSender send(mpDBusConnection, "volumeStep", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(sinkID);
    send.append(volumeStep);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;

}

am_Error_e IDBusCommandReceiver::setSinkMuteState(const am_sinkID_t sinkID, const am_MuteState_e muteState)
{
    CDBusSender send(mpDBusConnection, "setSinkMuteState", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(sinkID);
    send.append(static_cast<int16_t>(muteState));
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::setMainSinkSoundProperty(const am_MainSoundProperty_s& soundProperty,
                                                              const am_sinkID_t sinkID)
{
    CDBusSender send(mpDBusConnection, "setMainSinkSoundProperty", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(soundProperty);
    send.append(sinkID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::setMainSourceSoundProperty(const am_MainSoundProperty_s& soundProperty,
                                                                const am_sourceID_t sourceID)
{
    CDBusSender send(mpDBusConnection, "setMainSourceSoundProperty", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(soundProperty);
    send.append(sourceID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::setSystemProperty(const am_SystemProperty_s& property)
{
    CDBusSender send(mpDBusConnection, "setSystemProperty", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(property);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListMainConnections(vector<am_MainConnectionType_s>& listConnections) const
{
    CDBusSender send(mpDBusConnection, "getListMainConnections", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListMainConnectionType(listConnections);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListMainSinks(vector<am_SinkType_s>& listMainSinks) const
{
    CDBusSender send(mpDBusConnection, "getListMainSinks", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListSinkType(listMainSinks);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListMainSources(vector<am_SourceType_s>& listMainSources) const
{
    CDBusSender send(mpDBusConnection, "getListMainSources", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListSourceType(listMainSources);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListMainSinkSoundProperties(
        const am_sinkID_t sinkID, vector<am_MainSoundProperty_s>& listSoundProperties) const
{
    CDBusSender send(mpDBusConnection, "getListMainSinkSoundProperties", mDBusData.dest, mDBusData.path,
                     mDBusData.iface);
    send.append(sinkID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListMainSoundProperties(listSoundProperties);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListMainSourceSoundProperties(
        const am_sourceID_t sourceID, vector<am_MainSoundProperty_s>& listSourceProperties) const
{
    CDBusSender send(mpDBusConnection, "getListMainSourceSoundProperties", mDBusData.dest, mDBusData.path,
                     mDBusData.iface);
    send.append(sourceID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListMainSoundProperties(listSourceProperties);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListSourceClasses(vector<am_SourceClass_s>& listSourceClasses) const
{
    CDBusSender send(mpDBusConnection, "getListSourceClasses", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListSourceClass(listSourceClasses);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListSinkClasses(vector<am_SinkClass_s>& listSinkClasses) const
{
    CDBusSender send(mpDBusConnection, "getListSinkClasses", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListSinkClass(listSinkClasses);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListSystemProperties(vector<am_SystemProperty_s>& listSystemProperties) const
{
    CDBusSender send(mpDBusConnection, "getListSystemProperties", mDBusData.dest, mDBusData.path, mDBusData.iface);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListSystemProperty(listSystemProperties);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getTimingInformation(const am_mainConnectionID_t mainConnectionID,
                                                          am_timeSync_t& delay) const
{
    CDBusSender send(mpDBusConnection, "getTimingInformation", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(mainConnectionID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        delay = receive.getInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getDBusConnectionWrapper(CAmDbusWrapper*& dbusConnectionWrapper) const
{
    /* As discussed with Jens no definition is needed for this call */
    (void) (dbusConnectionWrapper);
    return E_OK;
}

am_Error_e IDBusCommandReceiver::getSocketHandler(CAmSocketHandler*& socketHandler) const
{
	/* As discussed with Jens no definition is needed for this call */
    (void) (socketHandler);
    return E_OK;
}

void IDBusCommandReceiver::confirmCommandReady(const uint16_t handle, const am_Error_e error)
{
    CDBusSender send(mpDBusConnection, "confirmCommandReady", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(handle);
    send.append(error);
    send.send_async();
}

void IDBusCommandReceiver::confirmCommandRundown(const uint16_t handle, const am_Error_e error)
{
    CDBusSender send(mpDBusConnection, "confirmCommandRundown", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(handle);
    send.append(error);
    send.send_async();
}

am_Error_e IDBusCommandReceiver::getListMainSinkNotificationConfigurations(
        const am_sinkID_t sinkID, vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations) const
{
    CDBusSender send(mpDBusConnection, "getListMainSinkNotificationConfigurations", mDBusData.dest, mDBusData.path,
                     mDBusData.iface);
    send.append(sinkID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListNotificationConfiguration(listMainNotificationConfigurations);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getListMainSourceNotificationConfigurations(
        const am_sourceID_t sourceID, vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations) const
{
    CDBusSender send(mpDBusConnection, "getListMainSourceNotificationConfigurations", mDBusData.dest, mDBusData.path,
                     mDBusData.iface);
    send.append(sourceID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {

        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        receive.getListNotificationConfiguration(listMainNotificationConfigurations);
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::setMainSinkNotificationConfiguration(
        const am_sinkID_t sinkID, const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    CDBusSender send(mpDBusConnection, "setMainSinkNotificationConfiguration", mDBusData.dest, mDBusData.path,
                     mDBusData.iface);
    send.append(sinkID);
    send.append(mainNotificationConfiguration);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::setMainSourceNotificationConfiguration(
        const am_sourceID_t sourceID, const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    CDBusSender send(mpDBusConnection, "setMainSourceNotificationConfiguration", mDBusData.dest, mDBusData.path,
                     mDBusData.iface);
    send.append(sourceID);
    send.append(mainNotificationConfiguration);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}

am_Error_e IDBusCommandReceiver::getVolume(const am_sinkID_t sinkID, am_mainVolume_t& mainVolume) const
{
    CDBusSender send(mpDBusConnection, "getVolume", mDBusData.dest, mDBusData.path, mDBusData.iface);
    send.append(sinkID);
    am_Error_e ret = send.send_sync();
    if(ret == E_OK)
    {
        CDBusReceiver receive(mpDBusConnection);
        receive.initReceive(send.getDbusMessage());
        mainVolume = receive.getInt();
        ret = (static_cast<am_Error_e>(receive.getUInt()));
    }
    return ret;
}
