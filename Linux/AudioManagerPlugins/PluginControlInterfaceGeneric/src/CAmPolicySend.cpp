/******************************************************************************
 * @file: CAmPolicySend.cpp
 *
 * This file contains the definition of policy engine send class (member
 * functions and data members) used to provide the interface to framework to
 * pass the hook/trigger to policy engine to get the actions
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

#include "CAmPolicySend.h"
#include "IAmPolicyReceive.h"
#include "CAmLogger.h"
namespace am {
namespace gc {

extern "C" IAmPolicySend* createPolicySendInterface()
{
    return (new CAmPolicySend());
}

extern "C" void destroyPolicySend(IAmPolicySend* policySendInterface)
{
    delete policySendInterface;
}

CAmPolicySend::CAmPolicySend(void) :
                                mpPolicyEngine(NULL)
{
}

CAmPolicySend::~CAmPolicySend()
{

}
am_Error_e CAmPolicySend::startupInterface(IAmPolicyReceive* pPolicyReceive)
{
    if (NULL != pPolicyReceive)
    {
        // initiate policy engine class
        mpPolicyEngine = new CAmPolicyEngine;
        if (NULL != mpPolicyEngine)
        {
            mpPolicyEngine->startPolicyEngine(pPolicyReceive);
            return E_OK;
        }
    }
    return E_NOT_POSSIBLE;
}

am_Error_e CAmPolicySend::hookRegisterDomain(const std::string& domainName, const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    LOG_FN_ENTRY(__FILENAME__,__func__);
    if (E_OK != status)
    {
        return E_OK;
    }
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_REGISTER_DOMAIN;
    triggerParams.domainName = domainName;

    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookRegisterSource(const std::string& sourceName, const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    if (E_OK != status)
    {
        return E_OK;
    }
    triggerParams.triggerType = SYSTEM_REGISTER_SOURCE;
    triggerParams.sourceName = sourceName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookRegisterSink(const std::string& sinkName, const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    if (E_OK != status)
    {
        return E_OK;
    }
    triggerParams.triggerType = SYSTEM_REGISTER_SINK;
    triggerParams.sinkName = sinkName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookRegisterGateway(const std::string& gatewayName,
                                              const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    if (E_OK != status)
    {
        return E_OK;
    }
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_REGISTER_GATEWAY;
    triggerParams.gatewayName = gatewayName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookDeregisterDomain(const std::string& domainName,
                                               const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    if (status != E_OK)
    {
        return E_OK;
    }
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_DEREGISTER_DOMAIN;
    triggerParams.domainName = domainName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookDeregisterSource(const std::string& sourceName,
                                               const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    if (E_OK != status)
    {
        return E_OK;
    }
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_DEREGISTER_SOURCE;
    triggerParams.sourceName = sourceName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookDeregisterSink(const std::string& sinkName, const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    if (E_OK != status)
    {
        return E_OK;
    }
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_DEREGISTER_SINK;
    triggerParams.sinkName = sinkName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookDeregisterGateway(const std::string& gatewayName,
                                                const am_Error_e status)
{
    gc_triggerParams_s triggerParams;
    //in case error happened in registration on framework side ignore the hook
    if (E_OK != status)
    {
        return E_OK;
    }
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_DEREGISTER_GATEWAY;
    triggerParams.gatewayName = gatewayName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookDomainRegistrationComplete(const std::string& domainName)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_DOMAIN_REGISTRATION_COMPLETE;
    triggerParams.domainName = domainName;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookAllDomainRegistrationComplete(const am_Error_e& error)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,error);
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_ALL_DOMAIN_REGISTRATION_COMPLETE;
    triggerParams.status = error;
    LOG_FN_EXIT(__FILENAME__,__func__);
    return mpPolicyEngine->processTrigger(triggerParams);
}
am_Error_e CAmPolicySend::hookConnectionRequest(const std::string& className,
                                                const std::string& sourceName,
                                                const std::string& sinkName)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = USER_CONNECTION_REQUEST;
    triggerParams.sinkName = sinkName;
    triggerParams.sourceName = sourceName;
    triggerParams.className = className;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookDisconnectionRequest(const std::string& className,
                                                   const std::string& sourceName,
                                                   const std::string& sinkName)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = USER_DISCONNECTION_REQUEST;
    triggerParams.sinkName = sinkName;
    triggerParams.sourceName = sourceName;
    triggerParams.className = className;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSourceAvailabilityChange(
                const std::string& sourceName, const am_Availability_s& availabilityInstance)
{
    gc_triggerParams_s triggerParams;
    gc_Source_s sourceInstance;
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_SOURCE_AVAILABILITY_CHANGED;
    triggerParams.sourceName = sourceName;
    triggerParams.availability.availability = availabilityInstance.availability;
    triggerParams.availability.availabilityReason = availabilityInstance.availabilityReason;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSinkAvailabilityChange(const std::string& sinkName,
                                                     const am_Availability_s& availabilityInstance)
{
    gc_triggerParams_s triggerParams;
    gc_Sink_s sinkInstance;
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_SINK_AVAILABILITY_CHANGED;
    triggerParams.sinkName = sinkName;
    triggerParams.availability.availability = availabilityInstance.availability;
    triggerParams.availability.availabilityReason = availabilityInstance.availabilityReason;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSourceInterruptStateChange(const std::string& sourceName,
                                                         const am_InterruptState_e interruptState)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = SYSTEM_INTERRUPT_STATE_CHANGED;
    triggerParams.sourceName = sourceName;
    triggerParams.interruptState = interruptState;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSetMainSourceSoundProperty(
                const std::string& sourceName, const am_MainSoundProperty_s& soundProperty)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = USER_SET_SOURCE_MAIN_SOUND_PROPERTY;
    triggerParams.sourceName = sourceName;
    triggerParams.mainSoundProperty.type = soundProperty.type;
    triggerParams.mainSoundProperty.value = soundProperty.value;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSetMainSinkSoundProperty(const std::string& sinkName,
                                                       const am_MainSoundProperty_s& soundProperty)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = USER_SET_SINK_MAIN_SOUND_PROPERTY;
    triggerParams.sinkName = sinkName;
    triggerParams.mainSoundProperty.type = soundProperty.type;
    triggerParams.mainSoundProperty.value = soundProperty.value;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSetSystemProperty(const am_SystemProperty_s& systemProperty)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = USER_SET_SYSTEM_PROPERTY;
    triggerParams.systemProperty.type = systemProperty.type;
    triggerParams.systemProperty.value = systemProperty.value;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookVolumeChange(const std::string& sinkName,
                                           const am_mainVolume_t mainVolume)
{
    gc_triggerParams_s triggerParams;
    // store the parameters in member variable
    triggerParams.triggerType = USER_SET_VOLUME;
    triggerParams.sinkName = sinkName;
    triggerParams.mainVolume = mainVolume;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSetSinkMuteState(const std::string& sinkName,
                                               const am_MuteState_e muteState)
{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.sinkName = sinkName;
    triggerParams.muteState = muteState;
    //get the action based on the trigger
    triggerParams.triggerType = USER_SET_SINK_MUTE_STATE;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSetMainSinkNotificationConfiguration(
                const std::string& sinkName,
                const am_NotificationConfiguration_s& notificationConfiguration)
{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.sinkName = sinkName;
    triggerParams.notificatonConfiguration = notificationConfiguration;
    //get the action based on the trigger
    triggerParams.triggerType = USER_SET_SINK_MAIN_NOTIFICATION_CONFIGURATION;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookConnectionStateChange(const std::string& connectionName,
                                               const am_ConnectionState_e& connectionState,
                                               am_Error_e& status)
{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.connectionName = connectionName;
    triggerParams.connectionState = connectionState;
    triggerParams.status = status;
    //get the action based on the trigger
    triggerParams.triggerType = SYSTEM_CONNECTION_STATE_CHANGE;
    return mpPolicyEngine->processTrigger(triggerParams);
}
am_Error_e CAmPolicySend::hookStoredMainConnectionVolume(const std::string& connectionName,
                                                         const am_mainVolume_t mainVolume)

{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.connectionName = connectionName;
    triggerParams.mainVolume = mainVolume;
    triggerParams.triggerType = SYSTEM_STORED_SINK_VOLUME;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSetMainSourceNotificationConfiguration(
                    const std::string& sourceName,
                    const am_NotificationConfiguration_s& notificationConfiguration)
{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.sourceName = sourceName;
    triggerParams.notificatonConfiguration = notificationConfiguration;
    //get the action based on the trigger
    triggerParams.triggerType = USER_SET_SOURCE_MAIN_NOTIFICATION_CONFIGURATION;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSinkNotificationDataChanged(const std::string& sinkName,
                                               const am_NotificationPayload_s& payload)
{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.sinkName = sinkName;
    triggerParams.notificatonPayload = payload;
    //get the action based on the trigger
    triggerParams.triggerType = SYSTEM_SINK_NOTIFICATION_DATA_CHANGED;
    return mpPolicyEngine->processTrigger(triggerParams);
}

am_Error_e CAmPolicySend::hookSourceNotificationDataChanged(const std::string& sourceName,
                                                 const am_NotificationPayload_s& payload)
{
    gc_triggerParams_s triggerParams;
    // store the triggerParams in member variable
    triggerParams.sourceName = sourceName;
    triggerParams.notificatonPayload = payload;
    //get the action based on the trigger
    triggerParams.triggerType = SYSTEM_SOURCE_NOTIFICATION_DATA_CHANGED;
    return mpPolicyEngine->processTrigger(triggerParams);
}
/*
 *  TBD : Check if such a copy is needed. Basically here we are over writing  the configuration
 *  data with the data received from the routing side during the registration, but the issue
 *  is routing side sends the am_xx_s structure whereas configuration has the gc_xx_s, we might
 *  end up in a situation where routing side send a different set of properties of which extended
 *  data is not present in the configuration.
 */
template <typename TinElement, typename ToutElement>
void CAmPolicySend::_copyElementData(const TinElement& inputData, ToutElement& outputData)
{
    outputData.domainID = inputData.domainID;
    outputData.name = inputData.name;
    outputData.volume = inputData.volume;
    outputData.visible = inputData.visible;
    outputData.available = inputData.available;
    outputData.listSoundProperties = inputData.listSoundProperties;
    outputData.listConnectionFormats = inputData.listConnectionFormats;
    outputData.listMainSoundProperties = inputData.listMainSoundProperties;
    outputData.listMainNotificationConfigurations = inputData.listMainNotificationConfigurations;
    outputData.listNotificationConfigurations = inputData.listNotificationConfigurations;
}

am_Error_e CAmPolicySend::getListElements(const std::vector<std::string >& listNames,
                                          std::vector<gc_Source_s >& listSources)
{
    gc_Source_s source;
    std::vector<std::string >::const_iterator itListNames;
    am_Error_e error = E_OK;
    for (itListNames = listNames.begin(); itListNames != listNames.end(); ++itListNames)
    {
        error = mpPolicyEngine->getElement(*itListNames, source);
        if (error == E_OK)
        {
            listSources.push_back(source);
        }
        else
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmPolicySend::getListElements(const std::vector<std::string >& listNames,
                                          std::vector<gc_Sink_s >& listSinks)
{
    gc_Sink_s sink;
    std::vector<std::string >::const_iterator itListNames;
    am_Error_e error = E_OK;
    for (itListNames = listNames.begin(); itListNames != listNames.end(); ++itListNames)
    {
        error = mpPolicyEngine->getElement(*itListNames, sink);
        if (error == E_OK)
        {
            listSinks.push_back(sink);
        }
        else
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmPolicySend::getListElements(const std::vector<std::string >& listNames,
                                          std::vector<gc_Gateway_s >& listGateways)
{
    gc_Gateway_s gateway;
    std::vector<std::string >::const_iterator itListNames;
    am_Error_e error = E_OK;
    for (itListNames = listNames.begin(); itListNames != listNames.end(); ++itListNames)
    {
        error = mpPolicyEngine->getElement(*itListNames, gateway);
        if (error == E_OK)
        {
            listGateways.push_back(gateway);
        }
        else
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmPolicySend::getListElements(const std::vector<std::string >& listNames,
                                          std::vector<gc_Domain_s >& listDomains)
{
    gc_Domain_s domain;
    std::vector<std::string >::const_iterator itListNames;
    am_Error_e error = E_OK;
    for (itListNames = listNames.begin(); itListNames != listNames.end(); ++itListNames)
    {
        error = mpPolicyEngine->getElement(*itListNames, domain);
        if (error == E_OK)
        {
            listDomains.push_back(domain);
        }
        else
        {
            break;
        }
    }
    return error;
}
am_Error_e CAmPolicySend::getListSystemProperties(
                std::vector<am_SystemProperty_s >& listSystemProperties)
{
    return mpPolicyEngine->getListSystemProperties(listSystemProperties);
}

am_Error_e CAmPolicySend::getListClasses(std::vector<gc_Class_s >& listClasses)
{
    return mpPolicyEngine->getListClasses(listClasses);
}

am_Error_e CAmPolicySend::getListDomains(std::vector<gc_Domain_s >& listDomains)
{
    return mpPolicyEngine->getListDomains(listDomains);
}

} /* namespace gc */
} /* namespace am */
