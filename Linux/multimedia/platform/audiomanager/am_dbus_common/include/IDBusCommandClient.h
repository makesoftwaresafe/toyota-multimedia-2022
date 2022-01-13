/************************************************************************
 * @file: IDBusCommandClient.h
 *
 * @version: 1.1
 *
 * @description: IDBusCommandClient is a common interface class for
 * both receiver and sender command plug-in interface of AM.
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

#ifndef _I_DBUS_COMMAND_CLIENT_H_
#define _I_DBUS_COMMAND_CLIENT_H_

#include <pthread.h>
#include "CAmSerializer.h"
#include "CAmDbusWrapper.h"

#include "CDBusReceiver.h"
#include "IAmCommand.h"
namespace am
{

class CDBusCommandSender;

class IDBusCommandClient
{
public:
    IDBusCommandClient();
    virtual ~IDBusCommandClient();

    /* Receiver methods. Protected ones will be shadowed to ApplicationClient */
protected:
    am_Error_e connect(const am_sourceID_t sourceID, const am_sinkID_t sinkID, am_mainConnectionID_t& mainConnectionID);
    am_Error_e disconnect(const am_mainConnectionID_t mainConnectionID);
    /*
     * Will not be used in the ApplicationClient
     */
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
            const am_sourceID_t sourceID, std::vector<am_NotificationConfiguration_s>& listMainNotificationConfigurations);
    am_Error_e setMainSinkNotificationConfiguration(const am_sinkID_t sinkID,
                                                    const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e setMainSourceNotificationConfiguration(const am_sourceID_t sourceID,
                                                      const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e getVolume(const am_sinkID_t sinkID, am_mainVolume_t& mainVolume);

    /* Sender callbacks */

public:
    virtual void setCommandReady(const uint16_t handle) = 0;
    virtual void setCommandRundown(const uint16_t handle) = 0;
    virtual void cbNewMainConnection(const am_MainConnectionType_s& mainConnectionType) = 0;
    virtual void cbRemovedMainConnection(const am_mainConnectionID_t mainConnection) = 0;
    virtual void cbNewSink(const am_SinkType_s& sink) = 0;
    virtual void cbRemovedSink(const am_sinkID_t sinkID) = 0;
    virtual void cbNewSource(const am_SourceType_s& source) = 0;
    virtual void cbRemovedSource(const am_sourceID_t source) = 0;
    virtual void cbNumberOfSinkClassesChanged() = 0;
    virtual void cbNumberOfSourceClassesChanged() = 0;
    virtual void cbMainConnectionStateChanged(const am_mainConnectionID_t connectionID,
                                              const am_ConnectionState_e connectionState) = 0;
    virtual void cbMainSinkSoundPropertyChanged(const am_sinkID_t sinkID, const am_MainSoundProperty_s& soundProperty) = 0;
    virtual void cbMainSourceSoundPropertyChanged(const am_sourceID_t sourceID,
                                                  const am_MainSoundProperty_s& soundProperty) = 0;
    virtual void cbSinkAvailabilityChanged(const am_sinkID_t sinkID, const am_Availability_s& availability) = 0;
    virtual void cbSourceAvailabilityChanged(const am_sourceID_t sourceID, const am_Availability_s& availability) = 0;
    virtual void cbVolumeChanged(const am_sinkID_t sinkID, const am_mainVolume_t volume) = 0;
    virtual void cbSinkMuteStateChanged(const am_sinkID_t sinkID, const am_MuteState_e muteState) = 0;
    virtual void cbSystemPropertyChanged(const am_SystemProperty_s& systemProperty) = 0;
    virtual void cbTimingInformationChanged(const am_mainConnectionID_t mainConnectionID, const am_timeSync_t time) = 0;
    virtual void cbSinkUpdated(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                               const std::vector<am_MainSoundProperty_s>& listMainSoundProperties) = 0;
    virtual void cbSourceUpdated(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                                 const std::vector<am_MainSoundProperty_s>& listMainSoundProperties) = 0;
    virtual void cbSinkNotification(const am_sinkID_t sinkID, const am_NotificationPayload_s& notification) = 0;
    virtual void cbSourceNotification(const am_sourceID_t sourceID, const am_NotificationPayload_s& notification) = 0;
    virtual void cbMainSinkNotificationConfigurationChanged(
            const am_sinkID_t sinkID, const am_NotificationConfiguration_s& mainNotificationConfiguration) = 0;
    virtual void cbMainSourceNotificationConfigurationChanged(
            const am_sourceID_t sourceID, const am_NotificationConfiguration_s& mainNotificationConfiguration) = 0;

public:
    void setIAmCommandReceive(IAmCommandReceive *commandReceive)
    {
        mpIAmCommandReceive = commandReceive;
    }
    IAmCommandReceive *getIAmCommandReceive()
    {
        return mpIAmCommandReceive;
    }

    void setCAmSerializer(V2::CAmSerializer *serializer)
    {
        mpSerializer = serializer;
    }
    V2::CAmSerializer *getCAmSerializer()
    {
        return mpSerializer;
    }

private:
    pthread_mutex_t    mMutex;
    IAmCommandReceive  *mpIAmCommandReceive;
    V2::CAmSerializer      *mpSerializer;

};

}
#endif /* _I_DBUS_COMMAND_CLIENT_H_ */
