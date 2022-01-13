/******************************************************************************
 * @file: CAmControlReceive.cpp
 *
 * This file contains the definition of control receive class (member functions
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

#include "CAmControlReceive.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

//
// public functions
//
CAmControlReceive::CAmControlReceive(IAmControlReceive* pControlReceive) :
                                mpControlReceive(pControlReceive)
{
    mHandle.handle = 0;
}

CAmControlReceive::~CAmControlReceive()
{

    // release instance
    mpControlReceive = NULL;
}

am_Error_e CAmControlReceive::connect(am_connectionID_t& connectionID,
                                      const am_CustomConnectionFormat_t format,
                                      const am_sourceID_t sourceID, const am_sinkID_t sinkID)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->connect(handle, connectionID, format, sourceID, sinkID);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::disconnect(const am_connectionID_t connectionID)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->disconnect(handle, connectionID);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::crossFade(const am_HotSink_e hotSink,
                                        const am_crossfaderID_t crossFaderID,
                                        const am_CustomRampType_t rampType,
                                        const am_time_t rampTime)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->crossfade(handle, hotSink, crossFaderID, rampType, rampTime);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::abortAction()
{
    am_Error_e result;
    result = mpControlReceive->abortAction(mHandle);
    mHandle.handle = 0;
    return (result);
}

am_Error_e CAmControlReceive::setSourceState(const am_sourceID_t sourceID,
                                             const am_SourceState_e state)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSourceState(handle, sourceID, state);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return result;
}

am_Error_e CAmControlReceive::setSinkVolume(const am_sinkID_t sinkID, const am_volume_t volume,
                                            const am_CustomRampType_t rampType,
                                            const am_time_t time)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSinkVolume(handle, sinkID, volume, rampType, time);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::setSourceVolume(const am_sourceID_t sourceID,
                                              const am_volume_t volume,
                                              const am_CustomRampType_t rampType,
                                              const am_time_t rampTime)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSourceVolume(handle, sourceID, volume, rampType, rampTime);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::setSinkSoundProperties(
                const am_sinkID_t sinkID,
                const std::vector<am_SoundProperty_s >& listSoundProperties)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSinkSoundProperties(handle, sinkID, listSoundProperties);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::setSinkSoundProperty(const am_sinkID_t sinkID,
                                                   const am_SoundProperty_s& soundProperty)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSinkSoundProperty(handle, sinkID, soundProperty);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::setSourceSoundProperties(
                const am_sourceID_t sourceID,
                const std::vector<am_SoundProperty_s >& listSoundProperties)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSourceSoundProperties(handle, sourceID, listSoundProperties);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::setSourceSoundProperty(const am_sourceID_t sourceID,
                                                     const am_SoundProperty_s& soundProperty)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSourceSoundProperty(handle, sourceID, soundProperty);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return (result);
}

am_Error_e CAmControlReceive::setDomainState(const am_domainID_t domainID,
                                             const am_DomainState_e domainState)
{
    mpControlReceive->setDomainState(domainID, domainState);
    return (E_OK);
}

am_Error_e CAmControlReceive::enterDomainDB(const am_Domain_s& domainData, am_domainID_t& domainID)
{
    return mpControlReceive->enterDomainDB(domainData, domainID);
}

am_Error_e CAmControlReceive::enterMainConnectionDB(const am_MainConnection_s& mainConnectionData,
                                                    am_mainConnectionID_t& connectionID)
{
    return mpControlReceive->enterMainConnectionDB(mainConnectionData, connectionID);
}

am_Error_e CAmControlReceive::enterSinkDB(const am_Sink_s& sinkData, am_sinkID_t& sinkID)
{
    return mpControlReceive->enterSinkDB(sinkData, sinkID);
}

am_Error_e CAmControlReceive::enterCrossfaderDB(const am_Crossfader_s& crossFaderData,
                                                am_crossfaderID_t& crossFaderID)
{
    return mpControlReceive->enterCrossfaderDB(crossFaderData, crossFaderID);
}

am_Error_e CAmControlReceive::enterConverterDB(const am_Converter_s& converterData,
                                               am_converterID_t& converterID)
{
    return mpControlReceive->enterConverterDB(converterData, converterID);
}
am_Error_e CAmControlReceive::enterGatewayDB(const am_Gateway_s& gatewayData,
                                             am_gatewayID_t& gatewayID)
{
    return mpControlReceive->enterGatewayDB(gatewayData, gatewayID);
}

am_Error_e CAmControlReceive::enterSourceDB(const am_Source_s& sourceData, am_sourceID_t& sourceID)
{
    return mpControlReceive->enterSourceDB(sourceData, sourceID);
}

am_Error_e CAmControlReceive::enterSinkClassDB(const am_SinkClass_s& sinkClass,
                                               am_sinkClass_t& sinkClassID)
{
    return mpControlReceive->enterSinkClassDB(sinkClass, sinkClassID);
}

am_Error_e CAmControlReceive::enterSourceClassDB(am_sourceClass_t& sourceClassID,
                                                 const am_SourceClass_s& sourceClass)
{
    return mpControlReceive->enterSourceClassDB(sourceClassID, sourceClass);
}

am_Error_e CAmControlReceive::changeSinkClassInfoDB(const am_SinkClass_s& sinkClass)
{
    return mpControlReceive->changeSinkClassInfoDB(sinkClass);
}

am_Error_e CAmControlReceive::changeSourceClassInfoDB(const am_SourceClass_s& sourceClass)
{
    return mpControlReceive->changeSourceClassInfoDB(sourceClass);
}

am_Error_e CAmControlReceive::enterSystemPropertiesListDB(
                const std::vector<am_SystemProperty_s >& listSystemProperties)
{
    return mpControlReceive->enterSystemPropertiesListDB(listSystemProperties);
}

am_Error_e CAmControlReceive::changeMainConnectionRouteDB(
                const am_mainConnectionID_t mainconnectionID,
                const std::vector<am_connectionID_t >& listConnectionIDs)
{
    return mpControlReceive->changeMainConnectionRouteDB(mainconnectionID, listConnectionIDs);
}

am_Error_e CAmControlReceive::changeMainConnectionStateDB(
                const am_mainConnectionID_t mainconnectionID,
                const am_ConnectionState_e connectionState)
{
    return mpControlReceive->changeMainConnectionStateDB(mainconnectionID, connectionState);
}

am_Error_e CAmControlReceive::changeSinkMainVolumeDB(const am_mainVolume_t mainVolume,
                                                     const am_sinkID_t sinkID)
{
    return mpControlReceive->changeSinkMainVolumeDB(mainVolume, sinkID);
}

am_Error_e CAmControlReceive::changeSinkAvailabilityDB(const am_Availability_s& availability,
                                                       const am_sinkID_t sinkID)
{
    return mpControlReceive->changeSinkAvailabilityDB(availability, sinkID);
}

am_Error_e CAmControlReceive::changDomainStateDB(const am_DomainState_e domainState,
                                                 const am_domainID_t domainID)
{
    return mpControlReceive->changDomainStateDB(domainState, domainID);
}

am_Error_e CAmControlReceive::changeSinkMuteStateDB(const am_MuteState_e muteState,
                                                    const am_sinkID_t sinkID)
{
    return mpControlReceive->changeSinkMuteStateDB(muteState, sinkID);
}

am_Error_e CAmControlReceive::changeMainSinkSoundPropertyDB(
                const am_MainSoundProperty_s& soundProperty, const am_sinkID_t sinkID)
{
    return mpControlReceive->changeMainSinkSoundPropertyDB(soundProperty, sinkID);
}

am_Error_e CAmControlReceive::changeMainSourceSoundPropertyDB(
                const am_MainSoundProperty_s& soundProperty, const am_sourceID_t sourceID)
{
    return mpControlReceive->changeMainSourceSoundPropertyDB(soundProperty, sourceID);
}

am_Error_e CAmControlReceive::changeSourceAvailabilityDB(const am_Availability_s& availability,
                                                         const am_sourceID_t sourceID)
{
    return mpControlReceive->changeSourceAvailabilityDB(availability, sourceID);
}

am_Error_e CAmControlReceive::changeSystemPropertyDB(const am_SystemProperty_s& property)
{
    return mpControlReceive->changeSystemPropertyDB(property);
}

am_Error_e CAmControlReceive::removeMainConnectionDB(const am_mainConnectionID_t mainConnectionID)
{
    return mpControlReceive->removeMainConnectionDB(mainConnectionID);
}

am_Error_e CAmControlReceive::removeSinkDB(const am_sinkID_t sinkID)
{
    return mpControlReceive->removeSinkDB(sinkID);
}

am_Error_e CAmControlReceive::removeSourceDB(const am_sourceID_t sourceID)
{
    return mpControlReceive->removeSourceDB(sourceID);
}

am_Error_e CAmControlReceive::removeGatewayDB(const am_gatewayID_t gatewayID)
{
    return mpControlReceive->removeGatewayDB(gatewayID);
}

am_Error_e CAmControlReceive::removeCrossfaderDB(const am_crossfaderID_t crossFaderID)
{
    return mpControlReceive->removeCrossfaderDB(crossFaderID);
}

am_Error_e CAmControlReceive::removeConverterDB(const am_converterID_t converterID)
{
    return mpControlReceive->removeConverterDB(converterID);
}

am_Error_e CAmControlReceive::removeDomainDB(const am_domainID_t domainID)
{
    return mpControlReceive->removeDomainDB(domainID);
}

am_Error_e CAmControlReceive::removeSinkClassDB(const am_sinkClass_t sinkClassID)
{
    return mpControlReceive->removeSinkClassDB(sinkClassID);
}

am_Error_e CAmControlReceive::removeSourceClassDB(const am_sourceClass_t sourceClassID)
{
    return mpControlReceive->removeSourceClassDB(sourceClassID);
}

am_Error_e CAmControlReceive::getSourceClassInfoDB(const am_sourceID_t sourceID,
                                                   am_SourceClass_s& classInfo) const
{
    return mpControlReceive->getSourceClassInfoDB(sourceID, classInfo);
}

am_Error_e CAmControlReceive::getSinkClassInfoDB(const am_sinkID_t sinkID,
                                                 am_SinkClass_s& sinkClass) const
{
    return mpControlReceive->getSinkClassInfoDB(sinkID, sinkClass);
}

am_Error_e CAmControlReceive::getSinkInfoDB(const am_sinkID_t sinkID, am_Sink_s& sinkData) const
{
    return mpControlReceive->getSinkInfoDB(sinkID, sinkData);
}

am_Error_e CAmControlReceive::getSourceInfoDB(const am_sourceID_t sourceID,
                                              am_Source_s& sourceData) const
{
    return mpControlReceive->getSourceInfoDB(sourceID, sourceData);
}

am_Error_e CAmControlReceive::getGatewayInfoDB(const am_gatewayID_t gatewayID,
                                               am_Gateway_s& gatewayData) const
{
    return mpControlReceive->getGatewayInfoDB(gatewayID, gatewayData);
}

am_Error_e CAmControlReceive::getCrossfaderInfoDB(const am_crossfaderID_t crossFaderID,
                                                  am_Crossfader_s& crossFaderData) const
{
    return mpControlReceive->getCrossfaderInfoDB(crossFaderID, crossFaderData);
}

am_Error_e CAmControlReceive::getConverterInfoDB(const am_converterID_t converterID,
                                                 am_Converter_s& converterData) const
{
    return mpControlReceive->getConverterInfoDB(converterID, converterData);
}
am_Error_e CAmControlReceive::getMainConnectionInfoDB(const am_mainConnectionID_t mainConnectionID,
                                                      am_MainConnection_s& mainConnectionData) const
{
    return mpControlReceive->getMainConnectionInfoDB(mainConnectionID, mainConnectionData);
}

am_Error_e CAmControlReceive::getListSinksOfDomain(const am_domainID_t domainID,
                                                   std::vector<am_sinkID_t >& listSinkIDs) const
{
    return mpControlReceive->getListSinksOfDomain(domainID, listSinkIDs);
}

am_Error_e CAmControlReceive::getListSourcesOfDomain(
                const am_domainID_t domainID, std::vector<am_sourceID_t >& listSourceIDs) const
{
    return mpControlReceive->getListSourcesOfDomain(domainID, listSourceIDs);
}

am_Error_e CAmControlReceive::getListCrossfadersOfDomain(
                const am_domainID_t domainID,
                std::vector<am_crossfaderID_t >& listCrossfadersIDs) const
{
    return mpControlReceive->getListCrossfadersOfDomain(domainID, listCrossfadersIDs);
}

am_Error_e CAmControlReceive::getListConvertersOfDomain(
                const am_domainID_t domainID, std::vector<am_converterID_t >& listConverterID) const
{
    return mpControlReceive->getListConvertersOfDomain(domainID, listConverterID);
}

am_Error_e CAmControlReceive::getListGatewaysOfDomain(
                const am_domainID_t domainID, std::vector<am_gatewayID_t >& listGatewaysIDs) const
{
    return mpControlReceive->getListGatewaysOfDomain(domainID, listGatewaysIDs);
}

am_Error_e CAmControlReceive::getListDomains(std::vector<am_Domain_s >& listDomains) const
{
    return mpControlReceive->getListDomains(listDomains);
}

am_Error_e CAmControlReceive::getListSinks(std::vector<am_Sink_s >& listSinks) const
{
    return mpControlReceive->getListSinks(listSinks);
}

am_Error_e CAmControlReceive::getListSources(std::vector<am_Source_s >& listSources) const
{
    return mpControlReceive->getListSources(listSources);
}

am_Error_e CAmControlReceive::getListConverters(std::vector<am_Converter_s >& listConverters) const
{
    return mpControlReceive->getListConverters(listConverters);
}

am_Error_e CAmControlReceive::getListGateways(std::vector<am_Gateway_s >& listGateways) const
{
    return mpControlReceive->getListGateways(listGateways);
}

am_Error_e CAmControlReceive::setVolumes(am_Handle_s& handle,
                                         const std::vector<am_Volumes_s >& listVolumes)
{
    return mpControlReceive->setVolumes(handle, listVolumes);
}

am_Error_e CAmControlReceive::getListMainConnections(
                std::vector<am_MainConnection_s >& listMainConnections) const
{
    return mpControlReceive->getListMainConnections(listMainConnections);
}

am_Error_e CAmControlReceive::getListConnections(
                std::vector<am_Connection_s >& listConnections) const
{
    return mpControlReceive->getListConnections(listConnections);
}

am_Error_e CAmControlReceive::getListElements(std::vector<am_Source_s >& listSources) const
{
    return mpControlReceive->getListSources(listSources);
}

am_Error_e CAmControlReceive::getListElements(std::vector<am_Sink_s >& listSinks) const
{
    return mpControlReceive->getListSinks(listSinks);
}

am_Error_e CAmControlReceive::getListElements(std::vector<am_Gateway_s >& listGateways) const
{
    return mpControlReceive->getListGateways(listGateways);
}

am_Error_e CAmControlReceive::getListElements(std::vector<am_Domain_s >& listDomains) const
{
    return mpControlReceive->getListDomains(listDomains);
}

am_Error_e CAmControlReceive::getListSourceClasses(
                std::vector<am_SourceClass_s >& listSourceClasses) const
{
    return mpControlReceive->getListSourceClasses(listSourceClasses);
}

am_Error_e CAmControlReceive::getListHandles(std::vector<am_Handle_s >& listHandles) const
{
    return mpControlReceive->getListHandles(listHandles);
}

am_Error_e CAmControlReceive::getListCrossfaders(
                std::vector<am_Crossfader_s >& listCrossfaders) const
{
    return mpControlReceive->getListCrossfaders(listCrossfaders);
}

am_Error_e CAmControlReceive::getListSinkClasses(
                std::vector<am_SinkClass_s >& listSinkClasses) const
{
    return mpControlReceive->getListSinkClasses(listSinkClasses);
}

am_Error_e CAmControlReceive::getListSystemProperties(
                std::vector<am_SystemProperty_s >& listSystemProperties) const
{
    return mpControlReceive->getListSystemProperties(listSystemProperties);
}

am_Error_e CAmControlReceive::getRoute(const bool onlyfree, const am_sourceID_t sourceID,
                                       const am_sinkID_t sinkID,
                                       std::vector<am_Route_s >& listReturnRoutes)
{
    return mpControlReceive->getRoute(onlyfree, sourceID, sinkID, listReturnRoutes);
}

void CAmControlReceive::setCommandReady()
{
    mpControlReceive->setCommandReady();
}

void CAmControlReceive::setCommandRundown()
{
    mpControlReceive->setCommandRundown();
}

void CAmControlReceive::setRoutingReady()
{
    mpControlReceive->setRoutingReady();
}

void CAmControlReceive::setRoutingRundown()
{
    mpControlReceive->setRoutingRundown();
}

void CAmControlReceive::confirmControllerReady(const am_Error_e error)
{
    mpControlReceive->confirmControllerReady(error);
}

void CAmControlReceive::confirmControllerRundown(const am_Error_e error)
{
    mpControlReceive->confirmControllerRundown(error);
}

am_Error_e CAmControlReceive::getSocketHandler(CAmSocketHandler*& pSocketHandler)
{
    mpControlReceive->getSocketHandler(pSocketHandler);
    return (E_OK);
}

void CAmControlReceive::getInterfaceVersion(std::string& version) const
{
    mpControlReceive->getInterfaceVersion(version);
}

void CAmControlReceive::_saveHandle(am_Handle_s Handle)
{
    mHandle.handle = Handle.handle;
    mHandle.handleType = Handle.handleType;
    LOG_FN_INFO(__FILENAME__,__func__," Handle = ", mHandle.handle, mHandle.handleType);
}

void CAmControlReceive::notifyAsyncResult(am_Handle_s Handle, am_Error_e Error)
{
    /*
     * In case ACK/NACK is received for a unknown Handle means that we have received it
     * after a Timeout and it should be dropped.
     */

    LOG_FN_INFO(__FILENAME__,__func__,"Handle = ", Handle.handle, " mHandle = ", mHandle.handle, " Error = ", Error);
    if (Handle.handle == mHandle.handle)
    {
        mHandle.handle = 0;
        notify(Error);
    }
}

am_Error_e CAmControlReceive::getDomainInfoByID(const am_domainID_t domainID,
                                                am_Domain_s& domainInfo)
{
    std::vector<am_Domain_s > listDomains;
    std::vector<am_Domain_s >::iterator itListDomains;
    am_Error_e result;
    result = getListElements(listDomains);
    if (E_OK == result)
    {
        result = E_UNKNOWN;
        for (itListDomains = listDomains.begin(); itListDomains != listDomains.end();
                        itListDomains++)
        {
            if ((*itListDomains).domainID == domainID)
            {
                result = E_OK;
                domainInfo = (*itListDomains);
                break;
            }
        }
    }
    return result;
}
am_Error_e CAmControlReceive::changeSourceDB(
                const am_sourceID_t sourceID, const am_sourceClass_t sourceClassID,
                const std::vector<am_SoundProperty_s >& listSoundProperties,
                const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                const std::vector<am_MainSoundProperty_s >& listMainSoundProperties)
{
    return mpControlReceive->changeSourceDB(sourceID, sourceClassID, listSoundProperties,
                                            listConnectionFormats, listMainSoundProperties);
}
am_Error_e CAmControlReceive::changeSinkDB(
                const am_sinkID_t sinkID, const am_sinkClass_t sinkClassID,
                const std::vector<am_SoundProperty_s >& listSoundProperties,
                const std::vector<am_CustomConnectionFormat_t >& listConnectionFormats,
                const std::vector<am_MainSoundProperty_s >& listMainSoundProperties)
{
    return mpControlReceive->changeSinkDB(sinkID, sinkClassID, listSoundProperties,
                                          listConnectionFormats, listMainSoundProperties);
}

am_Error_e CAmControlReceive::changeConverterDB(
                const am_converterID_t converterID,
                const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                const std::vector<bool >& convertionMatrix)
{
    return mpControlReceive->changeConverterDB(converterID, listSourceConnectionFormats,
                                               listSinkConnectionFormats, convertionMatrix);
}

am_Error_e CAmControlReceive::changeGatewayDB(
                const am_gatewayID_t gatewayID,
                const std::vector<am_CustomConnectionFormat_t >& listSourceConnectionFormats,
                const std::vector<am_CustomConnectionFormat_t >& listSinkConnectionFormats,
                const std::vector<bool >& convertionMatrix)
{
    return mpControlReceive->changeGatewayDB(gatewayID, listSourceConnectionFormats,
                                             listSinkConnectionFormats, convertionMatrix);
}

am_Error_e CAmControlReceive::setSinkNotificationConfiguration(
                const am_sinkID_t sinkID,
                const am_NotificationConfiguration_s& notificationConfiguration)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSinkNotificationConfiguration(handle, sinkID,
                                                                notificationConfiguration);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return result;
}

am_Error_e CAmControlReceive::setSourceNotificationConfiguration(
                const am_sourceID_t sourceID,
                const am_NotificationConfiguration_s& notificationConfiguration)
{
    am_Error_e result;
    am_Handle_s handle;
    result = mpControlReceive->setSourceNotificationConfiguration(handle, sourceID,
                                                                  notificationConfiguration);
    if (result == E_OK)
    {
        _saveHandle(handle);
    }
    return result;
}

am_Error_e CAmControlReceive::changeMainSinkNotificationConfigurationDB(
                const am_sinkID_t sinkID,
                const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    return mpControlReceive->changeMainSinkNotificationConfigurationDB(
                    sinkID, mainNotificationConfiguration);
}

am_Error_e CAmControlReceive::changeMainSourceNotificationConfigurationDB(
                const am_sourceID_t sourceID,
                const am_NotificationConfiguration_s& mainNotificationConfiguration)
{
    return mpControlReceive->changeMainSourceNotificationConfigurationDB(
                    sourceID, mainNotificationConfiguration);
}

void CAmControlReceive::sendMainSinkNotificationPayload(
                const am_sinkID_t sinkID, const am_NotificationPayload_s& notificationPayload)
{
    mpControlReceive->sendMainSinkNotificationPayload(sinkID, notificationPayload);
}

void CAmControlReceive::sendMainSourceNotificationPayload(
                const am_sourceID_t sourceID, const am_NotificationPayload_s& notificationPayload)
{
    mpControlReceive->sendMainSourceNotificationPayload(sourceID, notificationPayload);
}
#ifdef NSM_IFACE_PRESENT
am_Error_e CAmControlReceive::getRestartReasonPropertyNSM(NsmRestartReason_e& restartReason)
{
    return mpControlReceive->getRestartReasonPropertyNSM(restartReason);
}

am_Error_e CAmControlReceive::getShutdownReasonPropertyNSM(NsmShutdownReason_e& ShutdownReason)
{
    return mpControlReceive->getShutdownReasonPropertyNSM(ShutdownReason);
}

am_Error_e CAmControlReceive::getRunningReasonPropertyNSM(NsmRunningReason_e& nsmRunningReason)
{
    return mpControlReceive->getRunningReasonPropertyNSM(nsmRunningReason);
}

NsmErrorStatus_e CAmControlReceive::getNodeStateNSM(NsmNodeState_e& nsmNodeState)
{
    return mpControlReceive->getNodeStateNSM(nsmNodeState);
}

NsmErrorStatus_e CAmControlReceive::getSessionStateNSM(const std::string& sessionName,
                                                       const NsmSeat_e seatID,
                                                       NsmSessionState_e& sessionState)
{
    return mpControlReceive->getSessionStateNSM(sessionName, seatID, sessionState);
}

NsmErrorStatus_e CAmControlReceive::getApplicationModeNSM(NsmApplicationMode_e& applicationMode)
{
    return mpControlReceive->getApplicationModeNSM(applicationMode);
}

NsmErrorStatus_e CAmControlReceive::registerShutdownClientNSM(const uint32_t shutdownMode,
                                                              const uint32_t timeoutMs)
{
    return mpControlReceive->registerShutdownClientNSM(shutdownMode, timeoutMs);
}

NsmErrorStatus_e CAmControlReceive::unRegisterShutdownClientNSM(const uint32_t shutdownMode)
{
    return mpControlReceive->unRegisterShutdownClientNSM(shutdownMode);
}

am_Error_e CAmControlReceive::getInterfaceVersionNSM(uint32_t& version)
{
    return getInterfaceVersionNSM(version);
}

NsmErrorStatus_e CAmControlReceive::sendLifecycleRequestCompleteNSM(const uint32_t RequestId,
                                                                    const NsmErrorStatus_e status)
{
    return mpControlReceive->sendLifecycleRequestCompleteNSM(RequestId, status);
}
#endif

am_Error_e CAmControlReceive::getListMainSinkSoundProperties(
                const am_sinkID_t sinkID,
                std::vector<am_MainSoundProperty_s >& listSoundproperties) const
{
    return mpControlReceive->getListMainSinkSoundProperties(sinkID, listSoundproperties);
}

am_Error_e CAmControlReceive::getListMainSourceSoundProperties(
                const am_sourceID_t sourceID,
                std::vector<am_MainSoundProperty_s >& listSoundproperties) const
{
    return mpControlReceive->getListMainSourceSoundProperties(sourceID, listSoundproperties);
}

am_Error_e CAmControlReceive::getListSinkSoundProperties(
                const am_sinkID_t sinkID,
                std::vector<am_SoundProperty_s >& listSoundproperties) const
{
    return mpControlReceive->getListSinkSoundProperties(sinkID, listSoundproperties);
}

am_Error_e CAmControlReceive::getListSourceSoundProperties(
                const am_sourceID_t sourceID,
                std::vector<am_SoundProperty_s >& listSoundproperties) const
{
    return mpControlReceive->getListSourceSoundProperties(sourceID, listSoundproperties);
}

am_Error_e CAmControlReceive::getMainSinkSoundPropertyValue(
                const am_sinkID_t sinkID, const am_CustomMainSoundPropertyType_t propertyType,
                int16_t& value) const
{
    return mpControlReceive->getMainSinkSoundPropertyValue(sinkID, propertyType, value);
}

am_Error_e CAmControlReceive::getSinkSoundPropertyValue(
                const am_sinkID_t sinkID, const am_CustomSoundPropertyType_t propertyType,
                int16_t& value) const
{
    return mpControlReceive->getMainSinkSoundPropertyValue(sinkID, propertyType, value);
}

am_Error_e CAmControlReceive::getMainSourceSoundPropertyValue(
                const am_sourceID_t sourceID, const am_CustomMainSoundPropertyType_t propertyType,
                int16_t& value) const
{
    return mpControlReceive->getMainSourceSoundPropertyValue(sourceID, propertyType, value);
}

am_Error_e CAmControlReceive::getSourceSoundPropertyValue(
                const am_sourceID_t sourceID, const am_CustomSoundPropertyType_t propertyType,
                int16_t& value) const
{
    return mpControlReceive->getMainSinkSoundPropertyValue(sourceID, propertyType, value);
}

} /* namespace gc */
} /* namespace am */
