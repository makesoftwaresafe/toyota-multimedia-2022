/******************************************************************************
 * @file: CAmControlReceive.h
 *
 * This file contains the declaration of control receive class (member functions
 * and data members) used to implement the interface to be used to pass request
 * to AM daemon
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

#ifndef GC_CONTROLRECEIVE_H_
#define GC_CONTROLRECEIVE_H_

#include "IAmControlCommon.h"
#include "CAmEventSubject.h"
#include "CAmTypes.h"

namespace am {
namespace gc {

class CAmControlReceive : public CAmEventSubject
{
public:
    // public functions
    CAmControlReceive(IAmControlReceive* pControlReceive);
    virtual ~CAmControlReceive();

    am_Error_e connect(am_connectionID_t& connectionID, const am_CustomConnectionFormat_t format,
                       const am_sourceID_t sourceID, const am_sinkID_t sinkID);
    am_Error_e disconnect(const am_connectionID_t connectionID);
    am_Error_e crossFade(const am_HotSink_e hotSink, const am_crossfaderID_t crossFaderID,
                         const am_CustomRampType_t rampType, const am_time_t rampTime);
    am_Error_e abortAction();
    am_Error_e setSourceState(const am_sourceID_t sourceID, const am_SourceState_e state);
    am_Error_e setSinkVolume(const am_sinkID_t sinkID, const am_volume_t volume,
                             const am_CustomRampType_t rampType, const am_time_t time);
    am_Error_e setSourceVolume(const am_sourceID_t sourceID, const am_volume_t volume,
                               const am_CustomRampType_t rampType, const am_time_t time);
    am_Error_e setSinkSoundProperties(const am_sinkID_t sinkID,
                                      const std::vector<am_SoundProperty_s >& listSoundProperties);
    am_Error_e setSinkSoundProperty(const am_sinkID_t sinkID,
                                    const am_SoundProperty_s& soundProperty);
    am_Error_e setSourceSoundProperties(
                    const am_sourceID_t sourceID,
                    const std::vector<am_SoundProperty_s >& listSoundProperties);
    am_Error_e setSourceSoundProperty(const am_sourceID_t sourceID,
                                      const am_SoundProperty_s& soundProperty);

    am_Error_e getRoute(const bool onlyfree, const am_sourceID_t sourceID, const am_sinkID_t sinkID,
                        std::vector<am_Route_s >& listReturnRoutes);
    am_Error_e setDomainState(const am_domainID_t domainID, const am_DomainState_e domainState);
    am_Error_e enterDomainDB(const am_Domain_s& domainData, am_domainID_t& domainID);
    am_Error_e enterMainConnectionDB(const am_MainConnection_s& mainConnectionData,
                                     am_mainConnectionID_t& connectionID);
    am_Error_e enterSinkDB(const am_Sink_s& sinkData, am_sinkID_t& sinkID);
    am_Error_e enterCrossfaderDB(const am_Crossfader_s& crossFaderData,
                                 am_crossfaderID_t& crossFaderID);
    am_Error_e enterConverterDB(const am_Converter_s& converterData, am_converterID_t& converterID);

    am_Error_e enterGatewayDB(const am_Gateway_s& gatewayData, am_gatewayID_t& gatewayID);
    am_Error_e enterSourceDB(const am_Source_s& sourceData, am_sourceID_t& sourceID);
    am_Error_e enterSinkClassDB(const am_SinkClass_s& sinkClass, am_sinkClass_t& sinkClassID);
    am_Error_e enterSourceClassDB(am_sourceClass_t& sourceClassID,
                                  const am_SourceClass_s& sourceClass);
    am_Error_e changeSinkClassInfoDB(const am_SinkClass_s& sinkClass);
    am_Error_e changeSourceClassInfoDB(const am_SourceClass_s& sourceClass);
    am_Error_e enterSystemPropertiesListDB(
                    const std::vector<am_SystemProperty_s >& listSystemProperties);
    am_Error_e changeMainConnectionRouteDB(
                    const am_mainConnectionID_t mainconnectionID,
                    const std::vector<am_connectionID_t >& listConnectionIDs);
    am_Error_e changeMainConnectionStateDB(const am_mainConnectionID_t mainconnectionID,
                                           const am_ConnectionState_e connectionState);
    am_Error_e changeSinkMainVolumeDB(const am_mainVolume_t mainVolume, const am_sinkID_t sinkID);
    am_Error_e changeSinkAvailabilityDB(const am_Availability_s& availability,
                                        const am_sinkID_t sinkID);
    am_Error_e changDomainStateDB(const am_DomainState_e domainState, const am_domainID_t domainID);
    am_Error_e changeSinkMuteStateDB(const am_MuteState_e muteState, const am_sinkID_t sinkID);
    am_Error_e changeMainSinkSoundPropertyDB(const am_MainSoundProperty_s& soundProperty,
                                             const am_sinkID_t sinkID);
    am_Error_e changeMainSourceSoundPropertyDB(const am_MainSoundProperty_s& soundProperty,
                                               const am_sourceID_t sourceID);
    am_Error_e changeSourceAvailabilityDB(const am_Availability_s& availability,
                                          const am_sourceID_t sourceID);
    am_Error_e changeSystemPropertyDB(const am_SystemProperty_s& property);
    am_Error_e removeMainConnectionDB(const am_mainConnectionID_t mainConnectionID);
    am_Error_e removeSinkDB(const am_sinkID_t sinkID);
    am_Error_e removeSourceDB(const am_sourceID_t sourceID);
    am_Error_e removeGatewayDB(const am_gatewayID_t gatewayID);
    am_Error_e removeCrossfaderDB(const am_crossfaderID_t crossfaderID);
    am_Error_e removeConverterDB(const am_converterID_t converterID);
    am_Error_e removeDomainDB(const am_domainID_t domainID);
    am_Error_e removeSinkClassDB(const am_sinkClass_t sinkClassID);
    am_Error_e removeSourceClassDB(const am_sourceClass_t sourceClassID);
    am_Error_e getSourceClassInfoDB(const am_sourceID_t sourceID,
                                    am_SourceClass_s& classInfo) const;
    am_Error_e getSinkClassInfoDB(const am_sinkID_t sinkID, am_SinkClass_s& sinkClass) const;
    am_Error_e getSinkInfoDB(const am_sinkID_t sinkID, am_Sink_s& sinkData) const;
    am_Error_e getSourceInfoDB(const am_sourceID_t sourceID, am_Source_s& sourceData) const;
    am_Error_e getGatewayInfoDB(const am_gatewayID_t gatewayID, am_Gateway_s& gatewayData) const;
    am_Error_e getCrossfaderInfoDB(const am_crossfaderID_t crossfaderID,
                                   am_Crossfader_s& crossfaderData) const;
    am_Error_e getConverterInfoDB(const am_converterID_t converterID,
                                  am_Converter_s& converterData) const;
    am_Error_e getMainConnectionInfoDB(const am_mainConnectionID_t mainConnectionID,
                                       am_MainConnection_s& mainConnectionData) const;
    am_Error_e getListSinksOfDomain(const am_domainID_t domainID,
                                    std::vector<am_sinkID_t >& listSinkIDs) const;
    am_Error_e getListSourcesOfDomain(const am_domainID_t domainID,
                                      std::vector<am_sourceID_t >& listSourceIDs) const;
    am_Error_e getListCrossfadersOfDomain(
                    const am_domainID_t domainID,
                    std::vector<am_crossfaderID_t >& listCrossFadersIDs) const;
    am_Error_e getListConvertersOfDomain(const am_domainID_t domainID,
                                         std::vector<am_converterID_t >& listConverterID) const;
    am_Error_e getListGatewaysOfDomain(const am_domainID_t domainID,
                                       std::vector<am_gatewayID_t >& listGatewaysIDs) const;

    am_Error_e getListDomains(std::vector<am_Domain_s >& listDomains) const;
    am_Error_e getListSinks(std::vector<am_Sink_s >& listSinks) const;
    am_Error_e getListSources(std::vector<am_Source_s >& listSources) const;
    am_Error_e getListConverters(std::vector<am_Converter_s >& listConverters) const;
    am_Error_e getListGateways(std::vector<am_Gateway_s >& listGateways) const;
    am_Error_e setVolumes(am_Handle_s& handle, const std::vector<am_Volumes_s >& listVolumes);

    am_Error_e getListMainConnections(std::vector<am_MainConnection_s >& listMainConnections) const;
    am_Error_e getListConnections(std::vector<am_Connection_s >& listConnections) const;
    am_Error_e getListSourceClasses(std::vector<am_SourceClass_s >& listSourceClasses) const;
    am_Error_e getListHandles(std::vector<am_Handle_s >& listHandles) const;
    am_Error_e getListCrossfaders(std::vector<am_Crossfader_s >& listCrossfaders) const;
    am_Error_e getListSinkClasses(std::vector<am_SinkClass_s >& listSinkClasses) const;
    am_Error_e getListSystemProperties(
                    std::vector<am_SystemProperty_s >& listSystemProperties) const;
    am_Error_e getListElements(std::vector<am_Source_s >& listSources) const;
    am_Error_e getListElements(std::vector<am_Sink_s >& listSinks) const;
    am_Error_e getListElements(std::vector<am_Gateway_s >& listGateways) const;
    am_Error_e getListElements(std::vector<am_Domain_s >& listDomains) const;

    am_Error_e changeSourceDB(
                    const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                    const std::vector<am_SoundProperty_s >& listSoundProperties,
                    const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                    const std::vector<am_MainSoundProperty_s >& listMainSoundProperties);
    am_Error_e changeSinkDB(const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                            const std::vector<am_SoundProperty_s >& listSoundProperties,
                            const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                            const std::vector<am_MainSoundProperty_s >& listMainSoundProperties);
    am_Error_e changeConverterDB(
                    const am_converterID_t converterID,
                    const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                    const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                    const std::vector<bool >& convertionMatrix);
    am_Error_e changeGatewayDB(
                    const am_gatewayID_t gatewayID,
                    const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                    const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                    const std::vector<bool >& convertionMatrix);
    am_Error_e setSinkNotificationConfiguration(
                    const am_sinkID_t sinkID,
                    const am_NotificationConfiguration_s& notificationConfiguration);
    am_Error_e setSourceNotificationConfiguration(
                    const am_sourceID_t sourceID,
                    const am_NotificationConfiguration_s& notificationConfiguration);
    am_Error_e changeMainSinkNotificationConfigurationDB(
                    const am_sinkID_t sinkID,
                    const am_NotificationConfiguration_s& mainNotificationConfiguration);
    am_Error_e changeMainSourceNotificationConfigurationDB(
                    const am_sourceID_t sourceID,
                    const am_NotificationConfiguration_s& mainNotificationConfiguration);

    void sendMainSinkNotificationPayload(const am_sinkID_t sinkID,
                                         const am_NotificationPayload_s& notificationPayload);
    void sendMainSourceNotificationPayload(const am_sourceID_t sourceID,
                                           const am_NotificationPayload_s& notificationPayload);

    void setCommandReady();
    void setCommandRundown();
    void setRoutingReady();
    void setRoutingRundown();
    void confirmControllerReady(const am_Error_e error);
    void confirmControllerRundown(const am_Error_e error);
    am_Error_e getSocketHandler(CAmSocketHandler*& socketHandler);
    void getInterfaceVersion(std::string& version) const;
    am_Error_e getDomainInfoByID(const am_domainID_t domainID, am_Domain_s& domainInfo);

#ifdef NSM_IFACE_PRESENT
    am_Error_e getRestartReasonPropertyNSM(NsmRestartReason_e& restartReason);
    am_Error_e getShutdownReasonPropertyNSM(NsmShutdownReason_e& ShutdownReason);
    am_Error_e getRunningReasonPropertyNSM(NsmRunningReason_e& nsmRunningReason);
    NsmErrorStatus_e getNodeStateNSM(NsmNodeState_e& nsmNodeState);
    NsmErrorStatus_e getSessionStateNSM(const std::string& sessionName, const NsmSeat_e seatID,
                                        NsmSessionState_e& sessionState);
    NsmErrorStatus_e getApplicationModeNSM(NsmApplicationMode_e& applicationMode);
    NsmErrorStatus_e registerShutdownClientNSM(const uint32_t shutdownMode,
                                               const uint32_t timeoutMs);
    NsmErrorStatus_e unRegisterShutdownClientNSM(const uint32_t shutdownMode);
    am_Error_e getInterfaceVersionNSM(uint32_t& version);
    NsmErrorStatus_e sendLifecycleRequestCompleteNSM(const uint32_t RequestId,
                                                     const NsmErrorStatus_e status);
#endif
    am_Error_e getListMainSinkSoundProperties(
                    const am_sinkID_t sinkID,
                    std::vector<am_MainSoundProperty_s >& listSoundproperties) const;
    am_Error_e getListMainSourceSoundProperties(
                    const am_sourceID_t sourceID,
                    std::vector<am_MainSoundProperty_s >& listSoundproperties) const;
    am_Error_e getListSinkSoundProperties(
                    const am_sinkID_t sinkID,
                    std::vector<am_SoundProperty_s >& listSoundproperties) const;
    am_Error_e getListSourceSoundProperties(
                    const am_sourceID_t sourceID,
                    std::vector<am_SoundProperty_s >& listSoundproperties) const;
    am_Error_e getMainSinkSoundPropertyValue(const am_sinkID_t sinkID,
                                             const am_CustomMainSoundPropertyType_t propertyType,
                                             int16_t& value) const;
    am_Error_e getSinkSoundPropertyValue(const am_sinkID_t sinkID,
                                         const am_CustomSoundPropertyType_t propertyType,
                                         int16_t& value) const;
    am_Error_e getMainSourceSoundPropertyValue(const am_sourceID_t sourceID,
                                               const am_CustomMainSoundPropertyType_t propertyType,
                                               int16_t& value) const;
    am_Error_e getSourceSoundPropertyValue(const am_sourceID_t sourceID,
                                           const am_CustomSoundPropertyType_t propertyType,
                                           int16_t& value) const;
    void notifyAsyncResult(am_Handle_s Handle, am_Error_e Error);
private:
    void _saveHandle(am_Handle_s Handle);
    // private values
    IAmControlReceive* mpControlReceive;
    am_Handle_s mHandle;

};

} /* namespace gc */
} /* namespace am */
#endif /* GC_CONTROLRECEIVE_H_ */
