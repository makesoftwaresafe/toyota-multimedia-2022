/************************************************************************
 * @file: CAmDbusCommandSender.h
 *
 * @version: 1.1
 *
 * @description: A CAmDbusCommandSender class definition of command plug-in.
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

#ifndef CAMDBUSCOMMANDSENDER_H_
#define CAMDBUSCOMMANDSENDER_H_

#include "IAmCommand.h"
#include "IAmDbusCommandReceiverShadow.h"
#include "CDBusSender.h"
#include "CDBusReceiver.h"

namespace am
{

class CAmDbusCommandSender: public IAmCommandSend
{
public:
    CAmDbusCommandSender();
    virtual ~CAmDbusCommandSender();
    void getInterfaceVersion(std::string& version) const;
    am_Error_e startupInterface(IAmCommandReceive* commandreceiveinterface);
    void setCommandReady(const uint16_t handle);
    void setCommandRundown(const uint16_t handle);
    void cbNewMainConnection(const am_MainConnectionType_s& mainConnectionType);
    void cbRemovedMainConnection(const am_mainConnectionID_t mainConnection);
    void cbNewSink(const am_SinkType_s& sink);
    void cbRemovedSink(const am_sinkID_t sinkID);
    void cbNewSource(const am_SourceType_s& source);
    void cbRemovedSource(const am_sourceID_t source);
    void cbNumberOfSinkClassesChanged();
    void cbNumberOfSourceClassesChanged();
    void cbMainConnectionStateChanged(const am_mainConnectionID_t connectionID, const am_ConnectionState_e connectionState);
    void cbMainSinkSoundPropertyChanged(const am_sinkID_t sinkID, const am_MainSoundProperty_s& soundProperty);
    void cbMainSourceSoundPropertyChanged(const am_sourceID_t sourceID, const am_MainSoundProperty_s& soundProperty);
    void cbSinkAvailabilityChanged(const am_sinkID_t sinkID, const am_Availability_s& availability);
    void cbSourceAvailabilityChanged(const am_sourceID_t sourceID, const am_Availability_s& availability);
    void cbVolumeChanged(const am_sinkID_t sinkID, const am_mainVolume_t volume);
    void cbSinkMuteStateChanged(const am_sinkID_t sinkID, const am_MuteState_e muteState);
    void cbSystemPropertyChanged(const am_SystemProperty_s& systemProperty);
    void cbTimingInformationChanged(const am_mainConnectionID_t mainConnectionID, const am_timeSync_t time);
    void cbSinkUpdated(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                       const std::vector<am_MainSoundProperty_s>& listMainSoundProperties);
    void cbSourceUpdated(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                         const std::vector<am_MainSoundProperty_s>& listMainSoundProperties);
    void cbSinkNotification(const am_sinkID_t sinkID, const am_NotificationPayload_s& notification);
    void cbSourceNotification(const am_sourceID_t sourceID, const am_NotificationPayload_s& notification);
    void cbMainSinkNotificationConfigurationChanged(const am_sinkID_t sinkID,
                                                    const am_NotificationConfiguration_s& mainNotificationConfiguration);
    void cbMainSourceNotificationConfigurationChanged(const am_sourceID_t sourceID,
                                                      const am_NotificationConfiguration_s& mainNotificationConfiguration);

private:
    IAmCommandReceive* mpIAmCommandReceive;
    CAmDbusWrapper* mpCAmDbusWrapper;
    DBusConnection* mpDBusConnection;
    IAmDbusCommandReceiverShadow mIAmDbusCommandReceiverShadow;
    CDBusSender mDBusSender;

};

} // namespace am

#endif /* CAMDBUSCOMMANDSENDER_H_ */
