/******************************************************************************
 * @file: CAmControlSend.h
 *
 * This file contains the declaration of control send class (member functions
 * and data members) used to implement the interface of AM daemon to allow the
 * hook from application and routing adaptor to reach the controller
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Toshiaki Isogai <tisogai@jp.adit-jv.com>
 *          Kapildev Patel  <kpatel@jp.adit-jv.com>
 *          Prashant Jain   <pjain@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/

#ifndef GC_CONTROLSEND_H_
#define GC_CONTROLSEND_H_

#include "IAmControlCommon.h"
#include "CAmTypes.h"
#include "CAmTriggerQueue.h"
#include "CAmTimerEvent.h"
#include "CAmCommandLineSingleton.h"

namespace am {
namespace gc {

class IAmPolicyReceive;
class IAmPolicySend;
class CAmControlReceive;

class CAmControlSend : public IAmControlSend
{
public:
    // public functions
    CAmControlSend();
    virtual ~CAmControlSend();
    void getInterfaceVersion(std::string& version) const;
    am_Error_e startupController(IAmControlReceive* controlreceiveinterface);
    void setControllerReady();
    void setControllerRundown(const int16_t signal);
    am_Error_e hookUserConnectionRequest(const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                                         am_mainConnectionID_t& mainConnectionID);
    am_Error_e hookUserDisconnectionRequest(const am_mainConnectionID_t connectionID);
    am_Error_e hookUserSetMainSinkSoundProperty(const am_sinkID_t sinkID,
                                                const am_MainSoundProperty_s& soundProperty);
    am_Error_e hookUserSetMainSourceSoundProperty(const am_sourceID_t sourceID,
                                                  const am_MainSoundProperty_s& soundProperty);
    am_Error_e hookUserSetSystemProperty(const am_SystemProperty_s& property);
    am_Error_e hookUserVolumeChange(const am_sinkID_t sinkID, const am_mainVolume_t newVolume);
    am_Error_e hookUserVolumeStep(const am_sinkID_t sinkID, const int16_t increment);
    am_Error_e hookUserSetSinkMuteState(const am_sinkID_t sinkID, const am_MuteState_e muteState);
    am_Error_e hookSystemRegisterDomain(const am_Domain_s& domainData, am_domainID_t& domainID);
    am_Error_e hookSystemDeregisterDomain(const am_domainID_t domainID);
    void hookSystemDomainRegistrationComplete(const am_domainID_t domainID);
    am_Error_e hookSystemRegisterSink(const am_Sink_s& sinkData, am_sinkID_t& sinkID);
    am_Error_e hookSystemDeregisterSink(const am_sinkID_t sinkID);
    am_Error_e hookSystemRegisterSource(const am_Source_s& sourceData, am_sourceID_t& sourceID);
    am_Error_e hookSystemDeregisterSource(const am_sourceID_t sourceID);
    am_Error_e hookSystemRegisterGateway(const am_Gateway_s& gatewayData,
                                         am_gatewayID_t& gatewayID);
    am_Error_e hookSystemDeregisterGateway(const am_gatewayID_t gatewayID);
    am_Error_e hookSystemRegisterCrossfader(const am_Crossfader_s& crossFaderData,
                                            am_crossfaderID_t& crossFaderID);
    am_Error_e hookSystemDeregisterCrossfader(const am_crossfaderID_t crossFaderID);
    am_Error_e hookSystemRegisterConverter(const am_Converter_s& converterData,
                                           am_converterID_t& converterID);
    am_Error_e hookSystemDeregisterConverter(const am_converterID_t converterID);
    am_Error_e hookSystemUpdateConverter(
                    const am_converterID_t converterID,
                    const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                    const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                    const std::vector<bool >& listConvertionMatrix);
    void hookSystemSinkVolumeTick(const am_Handle_s handle, const am_sinkID_t sinkID,
                                  const am_volume_t volume);
    void hookSystemSourceVolumeTick(const am_Handle_s handle, const am_sourceID_t sourceID,
                                    const am_volume_t volume);
    void hookSystemInterruptStateChange(const am_sourceID_t sourceID,
                                        const am_InterruptState_e interruptState);
    void hookSystemSinkAvailablityStateChange(const am_sinkID_t sinkID,
                                              const am_Availability_s& availabilityInstance);
    void hookSystemSourceAvailablityStateChange(const am_sourceID_t sourceID,
                                                const am_Availability_s& availabilityInstance);
    void hookSystemDomainStateChange(const am_domainID_t domainID, const am_DomainState_e state);
    void hookSystemReceiveEarlyData(const std::vector<am_EarlyData_s >& listData);
    void hookSystemSpeedChange(const am_speed_t speed);
    void hookSystemTimingInformationChanged(const am_mainConnectionID_t mainConnectionID,
                                            const am_timeSync_t time);
    void cbAckConnect(const am_Handle_s handle, const am_Error_e errorID);
    void cbAckDisconnect(const am_Handle_s handle, const am_Error_e errorID);
    void cbAckCrossFade(const am_Handle_s handle, const am_HotSink_e hotSink,
                        const am_Error_e error);
    void cbAckSetSinkVolumeChange(const am_Handle_s handle, const am_volume_t volume,
                                  const am_Error_e error);
    void cbAckSetSourceVolumeChange(const am_Handle_s handle, const am_volume_t voulme,
                                    const am_Error_e error);
    void cbAckSetSourceState(const am_Handle_s handle, const am_Error_e error);
    void cbAckSetSourceSoundProperties(const am_Handle_s handle, const am_Error_e error);
    void cbAckSetSourceSoundProperty(const am_Handle_s handle, const am_Error_e error);
    void cbAckSetSinkSoundProperties(const am_Handle_s handle, const am_Error_e error);
    void cbAckSetSinkSoundProperty(const am_Handle_s handle, const am_Error_e error);
    am_Error_e getConnectionFormatChoice(
                    const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                    const am_Route_s routeInstance,
                    const std::vector<am_CustomConnectionFormat_t > listPossibleConnectionFormats,
                    std::vector<am_CustomConnectionFormat_t >& listPrioConnectionFormats);
    void confirmCommandReady(const am_Error_e error);
    void confirmRoutingReady(const am_Error_e error);
    void confirmCommandRundown(const am_Error_e error);
    void confirmRoutingRundown(const am_Error_e error);
    am_Error_e hookSystemUpdateSink(
                    const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                    const std::vector<am_SoundProperty_s >& listSoundProperties,
                    const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                    const std::vector<am_MainSoundProperty_s >& listMainSoundProperties);
    am_Error_e hookSystemUpdateSource(
                    const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                    const std::vector<am_SoundProperty_s >& listSoundProperties,
                    const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                    const std::vector<am_MainSoundProperty_s >& listMainSoundProperties);
    am_Error_e hookSystemUpdateGateway(
                    const am_gatewayID_t gatewayID,
                    const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                    const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                    const std::vector<bool >& listConvertionMatrix);
    void cbAckSetVolumes(const am_Handle_s handle, const std::vector<am_Volumes_s >& listVolumes,
                         const am_Error_e error);
    void cbAckSetSinkNotificationConfiguration(const am_Handle_s handle, const am_Error_e error);
    void cbAckSetSourceNotificationConfiguration(const am_Handle_s handle, const am_Error_e error);
    void hookSinkNotificationDataChanged(const am_sinkID_t sinkID,
                                         const am_NotificationPayload_s& payload);
    void hookSourceNotificationDataChanged(const am_sourceID_t sourceID,
                                           const am_NotificationPayload_s& payload);
    am_Error_e hookUserSetMainSinkNotificationConfiguration(
                    const am_sinkID_t sinkID,
                    const am_NotificationConfiguration_s& notificationConfiguration);
    am_Error_e hookUserSetMainSourceNotificationConfiguration(
                    const am_sourceID_t sourceID,
                    const am_NotificationConfiguration_s& notificationConfiguration);
#ifdef NSM_IFACE_PRESENT
    void hookSystemNodeStateChanged(const NsmNodeState_e nodeStateId);
    void hookSystemNodeApplicationModeChanged(const NsmApplicationMode_e applicationModeId);
    void hookSystemSessionStateChanged(const std::string& sessionName, const NsmSeat_e seatID,
                                       const NsmSessionState_e sessionStateID);
    NsmErrorStatus_e hookSystemLifecycleRequest(const uint32_t request, const uint32_t requestId);
#endif
    void hookSystemSingleTimingInformationChanged(const am_connectionID_t connectionID,
                                                  const am_timeSync_t time);
    void iterateActions(void);
private:
    void _freeMemory(void);
    am_Error_e _forwardTriggertoPolicyEngine(gc_Trigger_e triggerType,
                                             gc_TriggerElement_s* triggerData);
    am_Error_e _createLastMainConnection(std::string);
    bool _checkAllDomainRegistered(void);
    bool _isDomainRegistrationComplete(const std::string& domainName);
    am_Error_e _restoreConnectionsFromPersistency(void);
    am_Error_e _parseString(const std::string& delimiter, std::string& inputString,std::vector<std::string>& listOutput);
    void _domainRegistrationTimeout(void* data);
    am_Error_e _storeVolumetoPersistency();
    am_Error_e _storeMainConnectiontoPersistency();
    am_Error_e _restoreVolumefromPersistency();
    am_Error_e _restoreMainConnectionVolumefromPersistency();
    am_Error_e _storeMainConnectionVolumetoPersistency();
    // private values
    CAmControlReceive* mpControlReceive;
    IAmPolicySend* mpPolicySend;
    Tcallback<CAmControlSend > mpdomainRegiTimerCallback;
    sh_timerHandle_t mDomainRegistrationTimerHandle;
    /*
     * For parsing the command line arguments
     */
    TCLAP::SwitchArg mdebugEnable;

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CONTROLSEND_H_ */
