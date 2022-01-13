/************************************************************************
 * @file: CAmDbusCommandSender.cpp
 *
 * @version: 1.1
 *
 * @description: A CAmDbusCommandSender class implementation of command plug-in.
 * CAmDbusCommandSender class will run in the context of AM process.
 * This is DBus wrapper class for sender class in AM side. CAmDbusCommandSender
 * class will call the CCpDbusWrpSender class methods via DBus connection
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

#include "CAmDbusCommandSender.h"
#include "CAmDltWrapper.h"
#include "CAmDbusWrapper.h"
#include "CDBusCommon.h"

DLT_DECLARE_CONTEXT (CP_Dbus)

using namespace am;
using namespace std;

extern "C" IAmCommandSend* amcp_dbus_pluginFactory()
{
    CAmDltWrapper::instance()->registerContext(CP_Dbus, "CP_DBus", "DBus Plugin");
    return (new CAmDbusCommandSender());
}

extern "C" void destroyamcp_dbus_plugin(IAmCommandSend* commandSendInterface)
{
    delete commandSendInterface;
}

CAmDbusCommandSender::CAmDbusCommandSender() :
        mpIAmCommandReceive(NULL), mpCAmDbusWrapper(NULL), mpDBusConnection(NULL)
{
}

CAmDbusCommandSender::~CAmDbusCommandSender()
{
    log(&CP_Dbus, DLT_LOG_INFO, "RoutingSender destructed");
    CAmDltWrapper::instance()->unregisterContext(CP_Dbus);
}

void CAmDbusCommandSender::getInterfaceVersion(string& version) const
{
    version = CommandVersion;
}

am_Error_e CAmDbusCommandSender::startupInterface(IAmCommandReceive* commandreceiveinterface)
{
    logInfo("CAmDbusCommandSender::",__func__," gets called");
    am_Error_e ret_val = E_ABORTED;

    if (commandreceiveinterface != NULL)
    {
        mpIAmCommandReceive = commandreceiveinterface;
        mIAmDbusCommandReceiverShadow.setCommandReceiver(mpIAmCommandReceive);
        mpIAmCommandReceive->getDBusConnectionWrapper(mpCAmDbusWrapper);
        if (mpCAmDbusWrapper != NULL)
        {
            mpCAmDbusWrapper->getDBusConnection(mpDBusConnection);
            mDBusSender.setDBusConnection(mpDBusConnection);
            if (NULL != mpDBusConnection)
            {
                ret_val = E_OK;
            }
        }
    }
    if (E_OK != ret_val)
    {
        logError("CAmDbusCommandSender::startupInterface failed");
    }
    return ret_val;
}

void CAmDbusCommandSender::setCommandReady(const uint16_t handle)
{
    logInfo("CAmDbusCommandSender::",__func__," gets called");
    // Dbus connection need to be make independent start up. Need to be fixed.
    mpIAmCommandReceive->confirmCommandReady(handle, E_OK);
}

void CAmDbusCommandSender::setCommandRundown(const uint16_t handle)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "setCommandRundown");
        mDBusSender.append(handle);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbNewMainConnection(const am_MainConnectionType_s& mainConnectionType)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbNewMainConnection");
        mDBusSender.append(mainConnectionType); // append will fail need to be fixed
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbRemovedMainConnection(const am_mainConnectionID_t mainConnection)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbRemovedMainConnection");
        mDBusSender.append(mainConnection);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbNewSink(const am_SinkType_s& sink)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbNewSink");
        mDBusSender.append(sink);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbRemovedSink(const am_sinkID_t sinkID)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbRemovedSink");
        mDBusSender.append(sinkID);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbNewSource(const am_SourceType_s& source)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbNewSource");
        mDBusSender.append(source);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbRemovedSource(const am_sourceID_t source)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbRemovedSource");
        mDBusSender.append(source);
        mDBusSender.sendMessage();
    }
}
void CAmDbusCommandSender::cbNumberOfSinkClassesChanged()
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbNumberOfSinkClassesChanged");
        mDBusSender.sendMessage();
    }
}
void CAmDbusCommandSender::cbNumberOfSourceClassesChanged()
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbNumberOfSourceClassesChanged");
        mDBusSender.sendMessage();
    }
}
void CAmDbusCommandSender::cbMainConnectionStateChanged(const am_mainConnectionID_t connectionID,
                                                        const am_ConnectionState_e connectionState)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbMainConnectionStateChanged");
        mDBusSender.append(connectionID);
        mDBusSender.append(static_cast<int16_t>(connectionState));
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbMainSinkSoundPropertyChanged(const am_sinkID_t sinkID,
                                                          const am_MainSoundProperty_s& soundProperty)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbMainSinkSoundPropertyChanged");
        mDBusSender.append(sinkID);
        mDBusSender.append(soundProperty);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbMainSourceSoundPropertyChanged(const am_sourceID_t sourceID,
                                                            const am_MainSoundProperty_s& soundProperty)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbMainSourceSoundPropertyChanged");
        mDBusSender.append(sourceID);
        mDBusSender.append(soundProperty);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSinkAvailabilityChanged(const am_sinkID_t sinkID, const am_Availability_s& availability)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSinkAvailabilityChanged");
        mDBusSender.append(sinkID);
        mDBusSender.append(availability);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSourceAvailabilityChanged(const am_sourceID_t sourceID, const am_Availability_s& availability)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSourceAvailabilityChanged");
        mDBusSender.append(sourceID);
        mDBusSender.append(availability);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbVolumeChanged(const am_sinkID_t sinkID, const am_mainVolume_t volume)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbVolumeChanged");
        mDBusSender.append(sinkID);
        mDBusSender.append(volume);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSinkMuteStateChanged(const am_sinkID_t sinkID, const am_MuteState_e muteState)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSinkMuteStateChanged");
        mDBusSender.append(sinkID);
        mDBusSender.append(static_cast<int16_t>(muteState));
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSystemPropertyChanged(const am_SystemProperty_s& systemProperty)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSystemPropertyChanged");
        mDBusSender.append(systemProperty);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbTimingInformationChanged(const am_mainConnectionID_t mainConnectionID, const am_timeSync_t time)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbTimingInformationChanged");
        mDBusSender.append(mainConnectionID);
        mDBusSender.append(time);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSinkUpdated(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                                         const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSinkUpdated");
        mDBusSender.append(sinkID);
        mDBusSender.append(sinkClassID);
        mDBusSender.append(listMainSoundProperties);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSourceUpdated(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                                           const vector<am_MainSoundProperty_s>& listMainSoundProperties)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSourceUpdated");
        mDBusSender.append(sourceID);
        mDBusSender.append(sourceClassID);
        mDBusSender.append(listMainSoundProperties);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSinkNotification(const am_sinkID_t sinkID, const am_NotificationPayload_s& notification)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSinkNotification");
        mDBusSender.append(sinkID);
        mDBusSender.append(notification);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbSourceNotification(const am_sourceID_t sourceID, const am_NotificationPayload_s& notification)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbSourceNotification");
        mDBusSender.append(sourceID);
        mDBusSender.append(notification);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbMainSinkNotificationConfigurationChanged(
        const am_sinkID_t sinkID, const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbMainSinkNotificationConfigurationChanged");
        mDBusSender.append(sinkID);
        mDBusSender.append(mainNotificationConfiguration);
        mDBusSender.sendMessage();
    }
}

void CAmDbusCommandSender::cbMainSourceNotificationConfigurationChanged(
        const am_sourceID_t sourceID, const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    if (NULL != mpDBusConnection)
    {
        mDBusSender.initSignal(COMMAND_DBUS_NAMESAPACE, "cbMainSourceNotificationConfigurationChanged");
        mDBusSender.append(sourceID);
        mDBusSender.append(mainNotificationConfiguration);
        mDBusSender.sendMessage();
    }
}
