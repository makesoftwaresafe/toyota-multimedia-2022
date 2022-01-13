/************************************************************************
 * @file: IAmCommandClient.h
 *
 * @version: 1.1
 *
 * @description: IAmCommandClient is a common interface class for
 * both receiver and sender command plug-in interface of AM.
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

#ifndef IAMCOMMANDCLIENT_H_
#define IAMCOMMANDCLIENT_H_

#include "IDBusCommandClient.h"

namespace am
{

class IAmCommandClient : public IDBusCommandClient
{
public:
    IAmCommandClient(DBusBusType dbusWrapperType = DBUS_BUS_SYSTEM, CAmSocketHandler *socketHandler = NULL);
    virtual ~IAmCommandClient();
    am_Error_e startSocketHandler(void);
    am_Error_e stopSocketHandler(void);
    /*
     * Explicit Receiver methods
     */
    am_Error_e connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID);
    am_Error_e disconnect(const am_mainConnectionID_t mainConnectionID);
    am_Error_e setVolume(const am_sinkID_t sinkID, const am_mainVolume_t volume);
    am_Error_e volumeStep(const am_sinkID_t sinkID, const int16_t volumeStep);
    am_Error_e setSinkMuteState(const am_sinkID_t sinkID, const am_MuteState_e muteState);
    am_Error_e setMainSinkSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sinkID_t sinkID);
    am_Error_e setMainSourceSoundProperty(const am_MainSoundProperty_s& soundProperty, const am_sourceID_t sourceID);
    am_Error_e setSystemProperty(const am_SystemProperty_s& property);
    am_Error_e getListMainConnections(std::vector<am_MainConnectionType_s>& listConnections);
    am_Error_e getListMainSinks(std::vector<am_SinkType_s>& listMainSinks);
    am_Error_e getListMainSources(std::vector<am_SourceType_s>& listMainSources);
    am_Error_e getListMainSinkSoundProperties(const am_sinkID_t sinkID,
                                              std::vector<am_MainSoundProperty_s>& listSoundProperties);
    am_Error_e getListMainSourceSoundProperties(const am_sourceID_t sourceID,
                                                std::vector<am_MainSoundProperty_s>& listSourceProperties);
    am_Error_e getListSourceClasses(std::vector<am_SourceClass_s>& listSourceClasses);
    am_Error_e getListSinkClasses(std::vector<am_SinkClass_s>& listSinkClasses);
    am_Error_e getListSystemProperties(std::vector<am_SystemProperty_s>& listSystemProperties);
    am_Error_e getTimingInformation(const am_mainConnectionID_t mainConnectionID, am_timeSync_t& delay);
    void confirmCommandReady(const uint16_t handle, const am_Error_e error);
    void confirmCommandRundown(const uint16_t handle, const am_Error_e error);
    am_Error_e getListMainSinkNotificationConfigurations(
            const am_sinkID_t sinkID, std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations);
    am_Error_e getListMainSourceNotificationConfigurations(
            const am_sourceID_t sourceID,
            std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations);
    am_Error_e setMainSinkNotificationConfiguration(const am_sinkID_t sinkID,
                                                    const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e setMainSourceNotificationConfiguration(const am_sourceID_t sourceID,
                                                      const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e getVolume(const am_sinkID_t sinkID, am_mainVolume_t& mainVolume);
public:
    /*
     * C++11 pure virtual overrider
     */
public:
    void setCommandReady(const uint16_t handle) override = 0;
    void setCommandRundown(const uint16_t handle) override = 0;
    void cbNewMainConnection(const am_MainConnectionType_s& mainConnectionType) override = 0;
    void cbRemovedMainConnection(const am_mainConnectionID_t mainConnection) override = 0;
    void cbNewSink(const am_SinkType_s& sink) override = 0;
    void cbRemovedSink(const am_sinkID_t sinkID) override = 0;
    void cbNewSource(const am_SourceType_s& source) override = 0;
    void cbRemovedSource(const am_sourceID_t source) override = 0;
    void cbNumberOfSinkClassesChanged() override = 0;
    void cbNumberOfSourceClassesChanged() override = 0;
    void cbMainConnectionStateChanged(const am_mainConnectionID_t connectionID,
                                              const am_ConnectionState_e connectionState) override = 0;
    void cbMainSinkSoundPropertyChanged(const am_sinkID_t sinkID, const am_MainSoundProperty_s& soundProperty) override = 0;
    void cbMainSourceSoundPropertyChanged(const am_sourceID_t sourceID,
                                                  const am_MainSoundProperty_s& soundProperty) override = 0;
    void cbSinkAvailabilityChanged(const am_sinkID_t sinkID, const am_Availability_s& availability) override = 0;
    void cbSourceAvailabilityChanged(const am_sourceID_t sourceID, const am_Availability_s& availability) override = 0;
    void cbVolumeChanged(const am_sinkID_t sinkID, const am_mainVolume_t volume) override = 0;
    void cbSinkMuteStateChanged(const am_sinkID_t sinkID, const am_MuteState_e muteState) override = 0;
    void cbSystemPropertyChanged(const am_SystemProperty_s& systemProperty) override = 0;
    void cbTimingInformationChanged(const am_mainConnectionID_t mainConnectionID, const am_timeSync_t time) override = 0;
    void cbSinkUpdated(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                               const std::vector<am_MainSoundProperty_s>& listMainSoundProperties) override = 0;
    void cbSourceUpdated(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                                 const std::vector<am_MainSoundProperty_s>& listMainSoundProperties) override = 0;
    void cbSinkNotification(const am_sinkID_t sinkID, const am_NotificationPayload_s& notification) override = 0;
    void cbSourceNotification(const am_sourceID_t sourceID, const am_NotificationPayload_s& notification) override = 0;
    void cbMainSinkNotificationConfigurationChanged(
            const am_sinkID_t sinkID, const am_NotificationConfiguration_s& mainNotificationConfiguration) override = 0;
    void cbMainSourceNotificationConfigurationChanged(
            const am_sourceID_t sourceID, const am_NotificationConfiguration_s& mainNotificationConfiguration) override = 0;
protected:
    static void* _WorkerThread(void* This) {
        return static_cast<IAmCommandClient*>(This)->WorkerThread();
    };
    void* WorkerThread(void);
private:
    pthread_t              mThread;
    CAmSocketHandler       *mpSocketHandler;
    bool                   mIsSocketHandlerInternal;
    CAmDbusWrapper         *mpCAmDbusWrapper;
    CDBusCommandSender     *mpCpDbusSenderCore;
    V2::CAmSerializer      *mpSerializer;
    IAmCommandReceive      *mpIAmCommandReceive;
};

}
#endif /* IAMCOMMANDCLIENT_H_ */
