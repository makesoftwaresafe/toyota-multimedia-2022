/************************************************************************
 * @file: CAmInterface.h
 *
 * @version: 1.0
 *
 * @description: Implements the callback handler triggered by AudioManager
 *               and instantiates the client interface.
 *
 * @component: platform/audiomanager/amcp_test
 *
 * @author: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#ifndef CAMINTERFACE_H_
#define CAMINTERFACE_H_

#include "IAmCommandClient.h"

namespace am
{

class CAmInterface: public IAmCommandClient
{
public:
    CAmInterface();
    virtual ~CAmInterface();

    am_Error_e waitForStateChange(const am_mainConnectionID_t connectionID,
                        const am_ConnectionState_e connectionState);

    virtual void setCommandReady(const uint16_t handle) override;
    virtual void setCommandRundown(const uint16_t handle) override;
    virtual void cbNewMainConnection(const am_MainConnectionType_s& mainConnectionType) override;
    virtual void cbRemovedMainConnection(const am_mainConnectionID_t mainConnection) override;
    virtual void cbNewSink(const am_SinkType_s& sink) override;
    virtual void cbRemovedSink(const am_sinkID_t sinkID) override;
    virtual void cbNewSource(const am_SourceType_s& source) override;
    virtual void cbRemovedSource(const am_sourceID_t sourceID) override;
    virtual void cbNumberOfSinkClassesChanged() override;
    virtual void cbNumberOfSourceClassesChanged() override;
    virtual void cbMainConnectionStateChanged(const am_mainConnectionID_t connectionID, const am_ConnectionState_e connectionState) override;
    virtual void cbMainSinkSoundPropertyChanged(const am_sinkID_t sinkID, const am_MainSoundProperty_s& soundProperty) override;
    virtual void cbMainSourceSoundPropertyChanged(const am_sourceID_t sourceID, const am_MainSoundProperty_s& soundProperty) override;
    virtual void cbSinkAvailabilityChanged(const am_sinkID_t sinkID, const am_Availability_s& availability) override;
    virtual void cbSourceAvailabilityChanged(const am_sourceID_t sourceID, const am_Availability_s& availability) override;
    virtual void cbVolumeChanged(const am_sinkID_t sinkID, const am_mainVolume_t volume) override;
    virtual void cbSinkMuteStateChanged(const am_sinkID_t sinkID, const am_MuteState_e muteState) override;
    virtual void cbSystemPropertyChanged(const am_SystemProperty_s& systemProperty) override;
    virtual void cbTimingInformationChanged(const am_mainConnectionID_t mainConnectionID, const am_timeSync_t time) override;
    virtual void cbSinkUpdated(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                       const std::vector<am_MainSoundProperty_s>& listMainSoundProperties) override;
    virtual void cbSourceUpdated(const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                         const std::vector<am_MainSoundProperty_s>& listMainSoundProperties) override;
    virtual void cbSinkNotification(const am_sinkID_t sinkID, const am_NotificationPayload_s& notification) override;
    virtual void cbSourceNotification(const am_sourceID_t sourceID, const am_NotificationPayload_s& notification) override;
    virtual void cbMainSinkNotificationConfigurationChanged(const am_sinkID_t sinkID,
                                                    const am_NotificationConfiguration_s& mainNotificationConfiguration) override;
    virtual void cbMainSourceNotificationConfigurationChanged(const am_sourceID_t sourceID,
                                                      const am_NotificationConfiguration_s& mainNotificationConfiguration) override;


private:
    pthread_mutex_t mMtx;
    pthread_cond_t mCond;

    am_mainConnectionID_t mConnectionID;
    am_ConnectionState_e mConnectionState;
};

}

#endif /* CAMINTERFACE_H_ */
