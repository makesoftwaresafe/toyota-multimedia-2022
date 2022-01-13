/************************************************************************
 * @file: CAmControlSenderBase.h
 *
 * @version: 1.1
 *
 * Modified AM controller for testing Routing Adapter behaviour
 * <please add the description>
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

#ifndef CONTROLSENDER_H_
#define CONTROLSENDER_H_

#include <list>
#include "IAmControl.h"

class CAmControlSenderBase:public am::IAmControlSend
{
public:
    CAmControlSenderBase();
    virtual ~CAmControlSenderBase();
    /**
     *
     * @param controlreceiveinterface
     * @return
     */
    am::am_Error_e startupController(am::IAmControlReceive* controlreceiveinterface);
    void setControllerReady();
    /**
     *
     * @param signal
     */
    void setControllerRundown(const int16_t signal);
    am::am_Error_e hookUserConnectionRequest(const am::am_sourceID_t sourceID, const am::am_sinkID_t sinkID,
            am::am_mainConnectionID_t& mainConnectionID);
    am::am_Error_e hookUserDisconnectionRequest(const am::am_mainConnectionID_t connectionID);
    am::am_Error_e hookUserSetMainSinkSoundProperty(const am::am_sinkID_t sinkID,
            const am::am_MainSoundProperty_s& soundProperty);
    am::am_Error_e hookUserSetMainSourceSoundProperty(const am::am_sourceID_t sourceID,
                                                  const am::am_MainSoundProperty_s& soundProperty);
    am::am_Error_e hookUserSetSystemProperty(const am::am_SystemProperty_s& property);
    am::am_Error_e hookUserVolumeChange(const am::am_sinkID_t SinkID, const am::am_mainVolume_t newVolume);
    am::am_Error_e hookUserVolumeStep(const am::am_sinkID_t SinkID, const int16_t increment);
    am::am_Error_e hookUserSetSinkMuteState(const am::am_sinkID_t sinkID, const am::am_MuteState_e muteState);
    am::am_Error_e hookSystemRegisterDomain(const am::am_Domain_s& domainData, am::am_domainID_t& domainID);
    am::am_Error_e hookSystemDeregisterDomain(const am::am_domainID_t domainID);
    void hookSystemDomainRegistrationComplete(const am::am_domainID_t domainID);
    am::am_Error_e hookSystemRegisterSink(const am::am_Sink_s& sinkData, am::am_sinkID_t& sinkID);
    am::am_Error_e hookSystemDeregisterSink(const am::am_sinkID_t sinkID);
    am::am_Error_e hookSystemRegisterSource(const am::am_Source_s& sourceData, am::am_sourceID_t& sourceID);
    am::am_Error_e hookSystemDeregisterSource(const am::am_sourceID_t sourceID);
    am::am_Error_e hookSystemRegisterConverter(const am::am_Converter_s& converterData, am::am_converterID_t& converterID);
    am::am_Error_e hookSystemDeregisterConverter(const am::am_converterID_t converterID);
    am::am_Error_e hookSystemRegisterGateway(const am::am_Gateway_s& gatewayData, am::am_gatewayID_t& gatewayID);
    am::am_Error_e hookSystemDeregisterGateway(const am::am_gatewayID_t gatewayID);
    am::am_Error_e hookSystemRegisterCrossfader(const am::am_Crossfader_s& crossfaderData, am::am_crossfaderID_t& crossfaderID);
    am::am_Error_e hookSystemDeregisterCrossfader(const am::am_crossfaderID_t crossfaderID);
    void hookSystemSinkVolumeTick(const am::am_Handle_s handle, const am::am_sinkID_t sinkID, const am::am_volume_t volume);
    void hookSystemSourceVolumeTick(const am::am_Handle_s handle, const am::am_sourceID_t sourceID, const am::am_volume_t volume);
    void hookSystemInterruptStateChange(const am::am_sourceID_t sourceID, const am::am_InterruptState_e interruptState);
    void hookSystemSinkAvailablityStateChange(const am::am_sinkID_t sinkID, const am::am_Availability_s& availability);
    void hookSystemSourceAvailablityStateChange(const am::am_sourceID_t sourceID, const am::am_Availability_s& availability);
    void hookSystemDomainStateChange(const am::am_domainID_t domainID, const am::am_DomainState_e state);
    void hookSystemReceiveEarlyData(const std::vector<am::am_EarlyData_s>& data);
    void hookSystemSpeedChange(const am::am_speed_t speed);
    void hookSystemTimingInformationChanged(const am::am_mainConnectionID_t mainConnectionID, const am::am_timeSync_t time);
    void cbAckConnect(const am::am_Handle_s handle, const am::am_Error_e errorID);
    void cbAckDisconnect(const am::am_Handle_s handle, const am::am_Error_e errorID);
    void cbAckCrossFade(const am::am_Handle_s handle, const am::am_HotSink_e hostsink, const am::am_Error_e error);
    void cbAckSetSinkVolumeChange(const am::am_Handle_s handle, const am::am_volume_t volume, const am::am_Error_e error);
    void cbAckSetSourceVolumeChange(const am::am_Handle_s handle, const am::am_volume_t voulme, const am::am_Error_e error);
    void cbAckSetSourceState(const am::am_Handle_s handle, const am::am_Error_e error);
    void cbAckSetSourceSoundProperties(const am::am_Handle_s handle, const am::am_Error_e error);
    void cbAckSetSourceSoundProperty(const am::am_Handle_s handle, const am::am_Error_e error);
    void cbAckSetSinkSoundProperties(const am::am_Handle_s handle, const am::am_Error_e error);
    void cbAckSetSinkSoundProperty(const am::am_Handle_s handle, const am::am_Error_e error);
    am::am_Error_e getConnectionFormatChoice(const am::am_sourceID_t sourceID, const am::am_sinkID_t sinkID,
        const am::am_Route_s listRoute,
        const std::vector<am::am_CustomConnectionFormat_t> listPossibleConnectionFormats,
        std::vector<am::am_CustomConnectionFormat_t>& listPrioConnectionFormats);
    void getInterfaceVersion(std::string& version) const;
    void confirmCommandReady(const am::am_Error_e error);
    void confirmRoutingReady(const am::am_Error_e error);
    void confirmCommandRundown(const am::am_Error_e error);
    void confirmRoutingRundown(const am::am_Error_e error);
    am::am_Error_e hookSystemUpdateSink(const am::am_sinkID_t sinkID, const am::am_sinkClass_t sinkClassID,
        const std::vector<am::am_SoundProperty_s>& listSoundProperties,
        const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats,
        const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties);
    am::am_Error_e hookSystemUpdateSource(const am::am_sourceID_t sourceID, const am::am_sourceClass_t sourceClassID,
        const std::vector<am::am_SoundProperty_s>& listSoundProperties,
        const std::vector<am::am_CustomConnectionFormat_t>& listConnectionFormats,
        const std::vector<am::am_MainSoundProperty_s>& listMainSoundProperties);
    am::am_Error_e hookSystemUpdateGateway(const am::am_gatewayID_t gatewayID,
        const std::vector<am::am_CustomConnectionFormat_t>& listSourceConnectionFormats,
        const std::vector<am::am_CustomConnectionFormat_t>& listSinkConnectionFormats,
        const std::vector<bool>& convertionMatrix);
    am::am_Error_e hookSystemUpdateConverter(const am::am_converterID_t converterID,
        const std::vector<am::am_CustomConnectionFormat_t>& listSourceConnectionFormats,
        const std::vector<am::am_CustomConnectionFormat_t>& listSinkConnectionFormats,
        const std::vector<bool>& convertionMatrix);
    void cbAckSetVolumes(const am::am_Handle_s handle, const std::vector<am::am_Volumes_s>& listVolumes,
                         const am::am_Error_e error);
    void cbAckSetSinkNotificationConfiguration(const am::am_Handle_s handle, const am::am_Error_e error);
    void cbAckSetSourceNotificationConfiguration(const am::am_Handle_s handle, const am::am_Error_e error);
    void hookSinkNotificationDataChanged(const am::am_sinkID_t sinkID, const am::am_NotificationPayload_s& payload);
    void hookSourceNotificationDataChanged(const am::am_sourceID_t sourceID, const am::am_NotificationPayload_s& payload);
    am::am_Error_e hookUserSetMainSinkNotificationConfiguration(
            const am::am_sinkID_t sinkID, const am::am_NotificationConfiguration_s& notificationConfiguration);
    am::am_Error_e hookUserSetMainSourceNotificationConfiguration(
            const am::am_sourceID_t sourceID, const am::am_NotificationConfiguration_s& notificationConfiguration);
    void hookSystemNodeStateChanged(const NsmNodeState_e NodeStateId);
    void hookSystemNodeApplicationModeChanged(const NsmApplicationMode_e ApplicationModeId);
    void hookSystemSessionStateChanged(const std::string& sessionName, const NsmSeat_e seatID,
                                       const NsmSessionState_e sessionStateID);
    NsmErrorStatus_e hookSystemLifecycleRequest(const uint32_t Request, const uint32_t RequestId);
    void hookSystemSingleTimingInformationChanged(const am::am_connectionID_t connectionID, const am::am_timeSync_t time);

private:
    am::IAmControlReceive * mControlReceiveInterface;

    void disconnect(am::am_mainConnectionID_t connectionID);
    void connect(am::am_sourceID_t sourceID, am::am_sinkID_t sinkID, am::am_mainConnectionID_t mainConnectionID);

    struct handleStatus
    {
        bool status;
        am::am_Handle_s handle;
        am::am_sinkID_t sinkID;
        am::am_sourceID_t sourceID;
    };

    struct mainConnectionSet
    {
        am::am_mainConnectionID_t connectionID;
        std::vector<handleStatus> listHandleStaus;
    };

    struct mainVolumeSet
    {
        am::am_sinkID_t sinkID;
        am::am_Handle_s handle;
        am::am_mainVolume_t mainVolume;
    };

    struct mainSinkSoundPropertySet
    {
        am::am_sinkID_t sinkID;
        am::am_Handle_s handle;
        am::am_MainSoundProperty_s mainSoundProperty;
    };

    class findHandle
    {
        handleStatus mHandle;
    public:
        explicit findHandle(handleStatus handle)
                : mHandle(handle)
        {
        }
        bool operator()(const handleStatus& handle) const
        {
            return (handle.handle.handle == mHandle.handle.handle);
        }
    };

    struct checkHandle
    {

        handleStatus mHandleStatus;
        explicit checkHandle(const handleStatus& value)
                : mHandleStatus(value)
        {
        }

        bool operator()(const handleStatus &value)
        {
            return !value.status;
        }
    };

    struct checkMainConnectionID
    {
        am::am_MainConnection_s mMainConnection;
        explicit checkMainConnectionID(const am::am_MainConnection_s& mainConnection)
                : mMainConnection(mainConnection)
        {
        }
        bool operator()(const am::am_MainConnection_s& mainConnection)
        {
            if (mMainConnection.mainConnectionID == mainConnection.mainConnectionID)
                return true;
            return false;
        }
    };

    enum cs_stateflow_e
    {
        SF_NONE, SF_CONNECT, SF_NAVI, SF_TA
    };

    enum cs_connectSf_e
    {
        SFC_RAMP_DOWN,
        SFC_SOURCE_STATE_OFF,
        SFC_DISCONNECT,
        SFC_WAIT_STATE,
        SFC_CONNECT,
        SFC_SOURCE_STATE_ON,
        SFC_RAMP_UP,
        SFC_FINISHED
    };

    enum cs_naviSf_e
    {
        NAVC_RAMP_DOWN,
        NAVC_CONNECT,
        NAVC_SOURCE_STATE_ON,
        NAVC_RAMP_UP,
        NAVC_WAIT_STATE,
        NAVC_RAMP_DOWN_AGAIN,
        NAVC_SOURCE_VOLUME_BACK,
        NAVC_SOURCE_ACTIVITY_BACK,
        NAVC_DISCONNECT,
        NAVC_FINISHED
    };

    enum cs_trafficSf_e
    {
        TA_RAMP_DOWN,
        TA_CONNECT,
        TA_SOURCE_STATE_ON,
        TA_RAMP_UP,
        TA_WAIT_STATE,
        TA_RAMP_DOWN_AGAIN,
        TA_SOURCE_STATE_OFF,
        TA_SOURCE_STATE_OLD_OFF,
        TA_DISCONNECT,
        TA_FINISHED
    };

    struct cs_connectData_s
    {
        am::am_mainConnectionID_t currentMainConnection;
        am::am_mainConnectionID_t newMainConnection;
        am::am_sourceID_t oldSourceID;
        am::am_sinkID_t sinkID;
        am::am_sourceID_t sourceID;
    };

    void callStateFlowHandler();
    void callConnectHandler();
    void callNaviHandler();
    void callTAHandler();

    std::vector<mainConnectionSet> mListOpenConnections;
    std::vector<mainConnectionSet> mListOpenDisconnections;
    std::vector<mainVolumeSet> mListOpenVolumeChanges;
    std::vector<mainSinkSoundPropertySet> mListMainSoundPropertyChanges;
    std::vector<am::am_sourceID_t> mListUsedSources;
    std::vector<am::am_sinkID_t> mListUsedSinks;

    cs_connectSf_e mConnectSf;
    cs_naviSf_e mNaviSf;
    cs_trafficSf_e mTrafficSf;
    cs_connectData_s mConnectData;
    cs_stateflow_e mStateflow;

};

#endif /* CONTROLSENDER_H_ */
