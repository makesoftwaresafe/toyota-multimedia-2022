/******************************************************************************
 * @file: CAmPolicyEngine.cpp
 *
 * This file contains the definition of policy engine class (member functions
 * and data members) used to provide the interface to get the actions related
 * to the trigger/hook from framework side
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

#include <algorithm>

#include "CAmPolicyEngine.h"
#include "IAmPolicyReceive.h"
#include "CAmLogger.h"
#include "CAmMainConnectionElement.h"

namespace am {
namespace gc {

#define isParameterSet(A,B) (1 == B.count(A))
#define AMCO_PRINT_ACTIONS 0
CAmPolicyEngine::CAmPolicyEngine() :
                                mpConfigReader(NULL),
                                mpPolicyReceive(NULL)
{

    // function (as supported in conditions of policy ) mapping to returns string or integer
    mMapFunctionReturnValue[FUNCTION_NAME] = true;
    mMapFunctionReturnValue[FUNCTION_PRIORITY] = false;
    mMapFunctionReturnValue[FUNCTION_CONNECTION_STATE] = false;
    mMapFunctionReturnValue[FUNCTION_VOLUME] = false;
    mMapFunctionReturnValue[FUNCTION_MAIN_VOLUME] = false;
    mMapFunctionReturnValue[FUNCTION_SOUND_PROPERTY] = false;
    mMapFunctionReturnValue[FUNCTION_MAIN_SOUND_PROPERTY_TYPE] = false;
    mMapFunctionReturnValue[FUNCTION_SYSTEM_PROPERTY_TYPE] = false;
    mMapFunctionReturnValue[FUNCTION_MAIN_SOUND_PROPERTY_VALUE] = false;
    mMapFunctionReturnValue[FUNCTION_SYSTEM_PROPERTY_VALUE] = false;
    mMapFunctionReturnValue[FUNCTION_MUTE_STATE] = false;
    mMapFunctionReturnValue[FUNCTION_IS_REGISTRATION_COMPLETE] = false;
    mMapFunctionReturnValue[FUNCTION_AVAILABILITY] = false;
    mMapFunctionReturnValue[FUNCTION_AVAILABILITY_REASON] = false;
    mMapFunctionReturnValue[FUNCTION_CONNECTION_FORMAT] = false;
    mMapFunctionReturnValue[FUNCTION_INTERRUPT_STATE] = false;
    mMapFunctionReturnValue[FUNCTION_IS_REGISTERED] = false;
    mMapFunctionReturnValue[FUNCTION_STATE] = false;
    mMapFunctionReturnValue[FUNCTION_PEEK] = true;
    mMapFunctionReturnValue[FUNCTION_ERROR] = false;
    mMapFunctionReturnValue[FUNCTION_NOTIFICATION_CONFIGURATION_STATUS] = false;
    mMapFunctionReturnValue[FUNCTION_NOTIFICATION_CONFIGURATION_PARAM] = false;
    mMapFunctionReturnValue[FUNCTION_NOTIFICATION_DATA_VALUE] = false;
    mMapFunctionReturnValue[FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_TYPE] = false;
    mMapFunctionReturnValue[FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_STATUS] = false;
    mMapFunctionReturnValue[FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_PARAM] = false;

    mMapPeekFunctions[CATEGORY_SOURCE_OF_CLASS] = &CAmPolicyEngine::_findSourcePeek;
    mMapPeekFunctions[CATEGORY_SINK_OF_CLASS] = &CAmPolicyEngine::_findSinkPeek;

    mMapNotificationStatusFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkNTStatus;
    mMapNotificationStatusFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceNTStatus;

    mMapNotificationParamFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkNTParam;
    mMapNotificationParamFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceNTParam;

    mMapNotificationValueFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserNotificationValue;

    mMapMainNotificationTypeFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserNotificationType;

    mMapMainNotificationStatusFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkMainNTStatus;
    mMapMainNotificationStatusFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceMainNTStatus;
    mMapNotificationStatusFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserNotificationStatus;

    mMapMainNotificationParamFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkMainNTParam;
    mMapMainNotificationParamFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceMainNTParam;
    mMapNotificationParamFunctions[CATEGORY_USER] = &CAmPolicyEngine::__findUserNotificationParam;

    mMapNameFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkName;
    mMapNameFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceName;
    mMapNameFunctions[CATEGORY_CLASS] = &CAmPolicyEngine::_findClassName;
    mMapNameFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionName;
    mMapNameFunctions[CATEGORY_DOMAIN_OF_SOURCE] = &CAmPolicyEngine::_findDomainOfSourceName;
    mMapNameFunctions[CATEGORY_DOMAIN_OF_SINK] = &CAmPolicyEngine::_findDomainOfSinkName;
    mMapNameFunctions[CATEGORY_CLASS_OF_SOURCE] = &CAmPolicyEngine::_findClassOfSourceName;
    mMapNameFunctions[CATEGORY_CLASS_OF_SINK] = &CAmPolicyEngine::_findClassOfSinkName;
    mMapNameFunctions[CATEGORY_SOURCE_OF_CLASS] = &CAmPolicyEngine::_findSourceOfClassName;
    mMapNameFunctions[CATEGORY_SINK_OF_CLASS] = &CAmPolicyEngine::_findSinkOfClassName;
    mMapNameFunctions[CATEGORY_DOMAIN] = &CAmPolicyEngine::_findDomainName;

    mMapPriorityFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkPriority;
    mMapPriorityFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourcePriority;
    mMapPriorityFunctions[CATEGORY_CLASS] = &CAmPolicyEngine::_findClassPriority;
    mMapPriorityFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionPriority;
    mMapPriorityFunctions[CATEGORY_CONNECTION_OF_CLASS] = &CAmPolicyEngine::_findConnectionOfClassPriority;

    mMapConnectionStateFunctions[CATEGORY_CONNECTION_OF_CLASS] = &CAmPolicyEngine::_findConnectionOfClassState;
    mMapConnectionStateFunctions[CATEGORY_CONNECTION_OF_SOURCE] = &CAmPolicyEngine::_findSourceConnectionState;
    mMapConnectionStateFunctions[CATEGORY_CONNECTION_OF_SINK] = &CAmPolicyEngine::_findSinkConnectionState;
    mMapConnectionStateFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionConnectionState;
    mMapConnectionStateFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserConnectionState;

    mMapVolumeFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkDeviceVolume;
    mMapVolumeFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceDeviceVolume;
    mMapVolumeFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionDeviceVolume;
    mMapVolumeFunctions[CATEGORY_CLASS] = &CAmPolicyEngine::_findConnectionOfClassDeviceVolume;

    mMapMainVolumeFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkMainVolume;
    mMapMainVolumeFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceMainVolume;

    mMapErrorFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserErrorValue;

    mMapMainVolumeFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserMainVolume;

    mMapSoundPropertyValueFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkDevicePropertyValue;
    mMapSoundPropertyValueFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceDevicePropertyValue;

    mMapMainSoundPropertyValueFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkUserPropertyValue;
    mMapMainSoundPropertyValueFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceUserPropertyValue;
    mMapMainSoundPropertyValueFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserMainSoundPropertyValue;

    mMapMainSoundPropertyTypeFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserMainSoundPropertyType;

    mMapSystemPropertyValueFunctions[CATEGORY_SYSTEM] = &CAmPolicyEngine::_findSystemPropertyValue;
    mMapSystemPropertyValueFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserSystemPropertyValue;

    mMapSystemPropertyTypeFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserSystemPropertyType;

    mMapMuteStateFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkMuteState;
    mMapMuteStateFunctions[CATEGORY_CLASS] = &CAmPolicyEngine::_findClassMuteState;
    mMapMuteStateFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionMuteState;
    mMapMuteStateFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserMuteState;

    mMapIsRegistrationCompleteFunctions[CATEGORY_DOMAIN] = &CAmPolicyEngine::_findIsDomainRegistrationComplete;

    mMapAvailabilityFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkAvailability;
    mMapAvailabilityFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceAvailability;
    mMapAvailabilityFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserAvailability;

    mMapAvailabilityReasonFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkAvailabilityReason;
    mMapAvailabilityReasonFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceAvailabilityReason;
    mMapAvailabilityReasonFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserAvailabilityReason;

    mMapInterruptStateFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceInterruptState;
    mMapInterruptStateFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionInterruptState;
    mMapInterruptStateFunctions[CATEGORY_USER] = &CAmPolicyEngine::_findUserInterruptState;

    mMapIsRegisteredFunctions[CATEGORY_SINK] = &CAmPolicyEngine::_findSinkIsRegistered;
    mMapIsRegisteredFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceIsRegistered;
    mMapIsRegisteredFunctions[CATEGORY_DOMAIN] = &CAmPolicyEngine::_findDomainIsRegistered;

    mMapStateFunctions[CATEGORY_SOURCE] = &CAmPolicyEngine::_findSourceState;
    mMapStateFunctions[CATEGORY_DOMAIN] = &CAmPolicyEngine::_findDomainState;

    //reserved for future use
    mMapConnectionFormatFunctions[CATEGORY_CONNECTION] = &CAmPolicyEngine::_findConnectionFormat;
    mMapConnectionFormatFunctions[CATEGORY_CLASS] = &CAmPolicyEngine::_findConnectionOfClassFormat;

    //name of the functions as supported in condition tag in configuration
    mMapFunctionNameToFunctionMaps[FUNCTION_NAME] = mMapNameFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_PRIORITY] = mMapPriorityFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_CONNECTION_STATE] = mMapConnectionStateFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_VOLUME] = mMapVolumeFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MAIN_VOLUME] = mMapMainVolumeFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_SOUND_PROPERTY] = mMapSoundPropertyValueFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MAIN_SOUND_PROPERTY_VALUE] = mMapMainSoundPropertyValueFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MAIN_SOUND_PROPERTY_TYPE] = mMapMainSoundPropertyTypeFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_SYSTEM_PROPERTY_TYPE] = mMapSystemPropertyTypeFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_SYSTEM_PROPERTY_VALUE] = mMapSystemPropertyValueFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MUTE_STATE] = mMapMuteStateFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_IS_REGISTRATION_COMPLETE] = mMapIsRegistrationCompleteFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_AVAILABILITY] = mMapAvailabilityFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_AVAILABILITY_REASON] = mMapAvailabilityReasonFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_INTERRUPT_STATE] = mMapInterruptStateFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_IS_REGISTERED] = mMapIsRegisteredFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_STATE] = mMapStateFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_CONNECTION_FORMAT] = mMapConnectionFormatFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_PEEK] = mMapPeekFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_ERROR] = mMapErrorFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_NOTIFICATION_CONFIGURATION_STATUS] = mMapNotificationStatusFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_NOTIFICATION_CONFIGURATION_PARAM] = mMapNotificationParamFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_NOTIFICATION_DATA_VALUE] = mMapNotificationValueFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_TYPE] = mMapMainNotificationTypeFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_STATUS] = mMapMainNotificationStatusFunctions;
    mMapFunctionNameToFunctionMaps[FUNCTION_MAIN_NOTIFICATION_CONFIGURATION_PARAM] = mMapMainNotificationParamFunctions;

    mMapActions[CONFIG_ACTION_NAME_CONNECT] = ACTION_NAME_CONNECT;
    mMapActions[CONFIG_ACTION_NAME_POP] = ACTION_NAME_CONNECT;
    mMapActions[CONFIG_ACTION_NAME_RESUME] = ACTION_NAME_CONNECT;
    mMapActions[CONFIG_ACTION_NAME_DISCONNECT] = ACTION_NAME_DISCONNECT;
    mMapActions[CONFIG_ACTION_NAME_SET_VOLUME] = ACTION_NAME_SET_VOLUME;
    mMapActions[CONFIG_ACTION_NAME_LIMIT] = ACTION_NAME_LIMIT;
    mMapActions[CONFIG_ACTION_NAME_UNLIMIT] = ACTION_NAME_LIMIT;
    mMapActions[CONFIG_ACTION_NAME_PUSH] = ACTION_NAME_INTERRUPT;
    mMapActions[CONFIG_ACTION_NAME_SUSPEND] = ACTION_NAME_SUSPEND;
    mMapActions[CONFIG_ACTION_NAME_MUTE] = ACTION_NAME_MUTE;
    mMapActions[CONFIG_ACTION_NAME_UNMUTE] = ACTION_NAME_MUTE;
    mMapActions[CONFIG_ACTION_NAME_SET_PROPERTY] = ACTION_NAME_SET_PROPERTY;
    mMapActions[CONFIG_ACTION_NAME_REGISTER] = ACTION_NAME_REGISTER;
    mMapActions[CONFIG_ACTION_NAME_DEBUG] = ACTION_DEBUG;
    mMapActions[CONFIG_ACTION_NAME_NOTIFICATION_CONFIGURATION] = ACTION_NAME_SET_NOTIFICATION_CONFIGURATION;
}

am_Error_e CAmPolicyEngine::_findUserNotificationType(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.notificatonConfiguration.type));
    return E_OK;
}

am_Error_e CAmPolicyEngine::_findUserNotificationValue(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.notificatonPayload.value));
    return E_OK;
}

am_Error_e CAmPolicyEngine::_findUserNotificationStatus(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.notificatonConfiguration.status));
    return E_OK;
}

am_Error_e CAmPolicyEngine::__findUserNotificationParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.notificatonConfiguration.parameter));
    return E_OK;
}

am_Error_e CAmPolicyEngine::_findMainNTStatusParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,const bool isLHS,
                                          std::string& mandatoryParameter, gc_Element_e elementType,
                                          am_CustomNotificationType_t ntType,const bool isStatusReq)
{
    std::string optionalParameter;
    am_Error_e result = E_UNKNOWN;
    std::vector<am_NotificationConfiguration_s> listNotificationConfigurations;
    _getValueOfParameter(conditionInstance,isLHS,mandatoryParameter,optionalParameter);
    if(E_OK == mpPolicyReceive->getListMainNotificationConfigurations(elementType,mandatoryParameter,listNotificationConfigurations))
    {
        am_CustomNotificationType_t type;
        if(FUNCTION_MACRO_SUPPORTED_REQUESTING == optionalParameter)
        {
            type = ntType;
        }
        else
        {
            type = (am_CustomNotificationType_t)atoi(optionalParameter.data());
        }
        std::vector<am_NotificationConfiguration_s>::iterator itListNotificationConfigurations;
        for(itListNotificationConfigurations = listNotificationConfigurations.begin();itListNotificationConfigurations!= listNotificationConfigurations.end(); itListNotificationConfigurations++)
        {
            if(type == (*itListNotificationConfigurations).type)
            {
                if( true == isStatusReq)
                {
                    listOutputs.push_back(to_string((*itListNotificationConfigurations).status));
                }
                else
                {
                    listOutputs.push_back(to_string((*itListNotificationConfigurations).parameter));
                }
                result = E_OK;
                break;
            }
        }
    }
    return result;
}

am_Error_e CAmPolicyEngine::_findSinkMainNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findMainNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SINK,parameters.notificatonConfiguration.type,true);
}

am_Error_e CAmPolicyEngine::_findSourceMainNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findMainNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SOURCE,parameters.notificatonConfiguration.type,true);
}

am_Error_e CAmPolicyEngine::_findSinkMainNTParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findMainNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SINK,parameters.notificatonConfiguration.type,false);
}

am_Error_e CAmPolicyEngine::_findSourceMainNTParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findMainNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SOURCE,parameters.notificatonConfiguration.type,false);
}

am_Error_e CAmPolicyEngine::_findNTStatusParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,const bool isLHS,
                                          std::string& mandatoryParameter, gc_Element_e elementType,
                                          am_CustomNotificationType_t ntType,const bool isStatusReq)
{
    std::string optionalParameter;
    am_Error_e result = E_UNKNOWN;
    std::vector<am_NotificationConfiguration_s> listNotificationConfigurations;
    _getValueOfParameter(conditionInstance,isLHS,mandatoryParameter,optionalParameter);
    if(E_OK == mpPolicyReceive->getListNotificationConfigurations(elementType,mandatoryParameter,listNotificationConfigurations))
    {
        am_CustomNotificationType_t type;
        type = (am_CustomNotificationType_t)atoi(optionalParameter.data());
        std::vector<am_NotificationConfiguration_s>::iterator itListNotificationConfigurations;
        for(itListNotificationConfigurations = listNotificationConfigurations.begin();itListNotificationConfigurations!= listNotificationConfigurations.end(); itListNotificationConfigurations++)
        {
            if(type == (*itListNotificationConfigurations).type)
            {
                if( true == isStatusReq)
                {
                    listOutputs.push_back(to_string((*itListNotificationConfigurations).status));
                }
                else
                {
                    listOutputs.push_back(to_string((*itListNotificationConfigurations).parameter));
                }
                result = E_OK;
                break;
            }
        }
    }
    return result;
}
am_Error_e CAmPolicyEngine::_findSinkNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SINK,parameters.notificatonConfiguration.type,true);
}

am_Error_e CAmPolicyEngine::_findSourceNTStatus(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SOURCE,parameters.notificatonConfiguration.type,true);
}

am_Error_e CAmPolicyEngine::_findSinkNTParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SINK,parameters.notificatonConfiguration.type,false);
}

am_Error_e CAmPolicyEngine::_findSourceNTParam(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findNTStatusParam(conditionInstance,listOutputs,isLHS,mandatoryParameter,ET_SOURCE,parameters.notificatonConfiguration.type,false);
}

am_Error_e CAmPolicyEngine::startPolicyEngine(IAmPolicyReceive* pPolicyReceive)
{
    mpPolicyReceive = pPolicyReceive;
    mpConfigReader = new CAmConfigurationReader();
    if (mpConfigReader == NULL)
    {
        return E_UNKNOWN;
    }
    return E_OK;
}

am_Error_e CAmPolicyEngine::_updateActionParameters(std::vector<gc_Action_s >& listActions,
                                                    gc_triggerParams_s& triggerParams)
{
    std::vector<gc_Action_s >::iterator itListActions;
    for (itListActions = listActions.begin(); itListActions != listActions.end(); itListActions++)
    {
        if (((*itListActions).actionName == CONFIG_ACTION_NAME_MUTE) || ((*itListActions).actionName
                        == CONFIG_ACTION_NAME_UNMUTE))
        {
            if ((false == isParameterSet(ACTION_PARAM_SINK_NAME, (*itListActions).mapParameters)) && (false
                            == isParameterSet(ACTION_PARAM_CLASS_NAME,
                                              (*itListActions).mapParameters)))
            {
                (*itListActions).mapParameters[ACTION_PARAM_SINK_NAME] = triggerParams.sinkName;
            }
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_SET_VOLUME)
        {
            if ((false == isParameterSet(ACTION_PARAM_VOLUME, (*itListActions).mapParameters)) && (false
                            == isParameterSet(ACTION_PARAM_VOLUME_STEP,
                                              (*itListActions).mapParameters))
                && (false == isParameterSet(ACTION_PARAM_MAIN_VOLUME,
                                            (*itListActions).mapParameters))
                && (false == isParameterSet(ACTION_PARAM_MAIN_VOLUME_STEP,
                                            (*itListActions).mapParameters)))
            {
                (*itListActions).mapParameters[ACTION_PARAM_MAIN_VOLUME] = to_string(triggerParams.mainVolume);
            }
            if (isParameterSet(ACTION_PARAM_SOURCE_NAME, (*itListActions).mapParameters))
            {
                if(isParameterSet(ACTION_PARAM_VOLUME_STEP,(*itListActions).mapParameters))
                {
                    am_volume_t  stepVolume = atoi((*itListActions).mapParameters[ACTION_PARAM_VOLUME_STEP].data());
                    std::string sourceName = (*itListActions).mapParameters[ACTION_PARAM_SOURCE_NAME];
                    am_volume_t volume;
                    mpPolicyReceive->getVolume(ET_SOURCE,sourceName,volume);
                    (*itListActions).mapParameters[ACTION_PARAM_VOLUME] = to_string(volume + stepVolume);
                }
            }
            if (isParameterSet(ACTION_PARAM_SINK_NAME, (*itListActions).mapParameters))
            {
                std::string sinkName = (*itListActions).mapParameters[ACTION_PARAM_SINK_NAME];
                if(isParameterSet(ACTION_PARAM_VOLUME_STEP,(*itListActions).mapParameters))
                {
                    am_volume_t volume;
                    am_volume_t  stepVolume = atoi((*itListActions).mapParameters[ACTION_PARAM_VOLUME_STEP].data());
                    mpPolicyReceive->getVolume(ET_SINK,sinkName,volume);
                    (*itListActions).mapParameters[ACTION_PARAM_VOLUME] = to_string(volume + stepVolume);
                }
                if(isParameterSet(ACTION_PARAM_MAIN_VOLUME_STEP,(*itListActions).mapParameters))
                {
                    am_mainVolume_t mainVolume;
                    am_mainVolume_t  stepVolume = atoi((*itListActions).mapParameters[ACTION_PARAM_MAIN_VOLUME_STEP].data());
                    mpPolicyReceive->getMainVolume(ET_SINK,sinkName,mainVolume);
                    (*itListActions).mapParameters[ACTION_PARAM_MAIN_VOLUME] = to_string(mainVolume + stepVolume);
                }
            }
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_SET_PROPERTY)
        {
            if (false == isParameterSet(ACTION_PARAM_PROPERTY_TYPE, (*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_PROPERTY_TYPE] = to_string(triggerParams.mainSoundProperty.type);
            }
            if (false == isParameterSet(ACTION_PARAM_PROPERTY_VALUE, (*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_PROPERTY_VALUE] = to_string(triggerParams.mainSoundProperty.value);
            }
        }
        if ((*itListActions).actionName == CONFIG_ACTION_NAME_MUTE)
        {
            (*itListActions).mapParameters[ACTION_PARAM_MUTE_STATE] = to_string(MS_MUTED);
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_UNMUTE)
        {
            (*itListActions).mapParameters[ACTION_PARAM_MUTE_STATE] = to_string(MS_UNMUTED);
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_LIMIT)
        {
            (*itListActions).mapParameters[ACTION_PARAM_LIMIT_STATE] = to_string(LS_LIMITED);
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_UNLIMIT)
        {
            (*itListActions).mapParameters[ACTION_PARAM_LIMIT_STATE] = to_string(LS_UNLIMITED);
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_DEBUG)
        {
            if(false == isParameterSet(ACTION_PARAM_DEBUG_TYPE,(*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_DEBUG_TYPE] = to_string(triggerParams.systemProperty.type);
            }
            if(false == isParameterSet(ACTION_PARAM_DEBUG_VALUE,(*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_DEBUG_VALUE] = to_string(triggerParams.systemProperty.value);
            }
        }
        else if ((*itListActions).actionName == CONFIG_ACTION_NAME_NOTIFICATION_CONFIGURATION)
        {
            if (false == isParameterSet(ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE, (*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE] = to_string(triggerParams.notificatonConfiguration.type);
            }
            if (false == isParameterSet(ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM, (*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM] = to_string(triggerParams.notificatonConfiguration.parameter);
            }
            if (false == isParameterSet(ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS, (*itListActions).mapParameters))
            {
                (*itListActions).mapParameters[ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS] = to_string(triggerParams.notificatonConfiguration.status);
            }
        }

        //convert config actions to framework actions
        (*itListActions).actionName = mMapActions[(*itListActions).actionName];
    }
    return E_OK;
}

am_Error_e CAmPolicyEngine::processTrigger(gc_triggerParams_s& triggerParams)
{
    std::vector<gc_Action_s > listActions;

    am_Error_e result;
    if (triggerParams.triggerType == SYSTEM_SOURCE_AVAILABILITY_CHANGED)
    {
        gc_Source_s source;
        if (E_OK == mpConfigReader->getElementByName(triggerParams.sourceName, source))
        {
            triggerParams.className = source.className;
        }
    }
    else if (triggerParams.triggerType == SYSTEM_SINK_AVAILABILITY_CHANGED)
    {
        gc_Sink_s sink;
        if (E_OK == mpConfigReader->getElementByName(triggerParams.sinkName, sink))
        {
            triggerParams.className = sink.className;
        }
    }
    //get the action based on the trigger
    result = _getActions(triggerParams.triggerType, listActions, triggerParams);
    if (E_OK != result)
    {
        return result;
    }
    _updateActionParameters(listActions, triggerParams);

    return mpPolicyReceive->setListActions(listActions, AL_NORMAL);
}

template <typename Toperand>
bool CAmPolicyEngine::_conditionResult(Toperand lhsData, gc_Operator_e opType, Toperand rhdData)
{
    bool result;
    LOG_FN_DEBUG(__FILENAME__,__func__," LHS =", lhsData, " RHS=", rhdData);
    switch (opType)
    {
    case EQ:
        result = (lhsData == rhdData);
        break;
    case GT:
        result = (lhsData > rhdData);
        break;
    case GEQ:
        result = (lhsData >= rhdData);
        break;
    case LT:
        result = (lhsData < rhdData);
        break;
    case LEQ:
        result = (lhsData <= rhdData);
        break;
    case NE:
        result = (lhsData != rhdData);
        break;
    default:
        result = false;
        break;
    }
    return result;
}

bool CAmPolicyEngine::_isConditionTrue(const gc_ConditionStruct_s& condition,
                                       const gc_triggerParams_s& parameters)
{
    std::string LHSString, RHSString;
    int32_t LHSInt, RHSInt;
    std::vector<std::string > listLHSOutputs;
    std::vector<std::string > listRHSOutputs;
    std::vector<std::string >::iterator itListOutputs;
    bool result;
    gc_Operator_e operation;

    // main evaluation logic comes here!!!!!!!!

    //convert operator from string to enum type
    operation = (gc_Operator_e)atoi(condition.operation.data());
    // invoke the function to get the LHS string from condition
    result = _executeFunction(condition.leftObject.functionName, condition.leftObject.category,
                              condition, listLHSOutputs, parameters, true);
    if (false == result)
    {
        LOG_FN_DEBUG(__FILENAME__,__func__," LHS function return error. function->category 1",
                     condition.leftObject.functionName, condition.leftObject.category);
        return result;
    }
    /*
     * Even if the returned list is empty, the condition should be evaluated
     * the reason is EXC condition would pass for empty list.
     */

    //get the RHS string from condition
    if (true == condition.rightObject.isValue) // direct value is provided by user
    {
        RHSString = condition.rightObject.directValue;
    }
    else
    {
        // invoke the function to get the RHS string from condition
        result = _executeFunction(condition.rightObject.functionObject.functionName,
                                  condition.rightObject.functionObject.category, condition,
                                  listRHSOutputs, parameters, false);
        if (false == result)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__," RHS function return error. function->category 3",
                         condition.rightObject.functionObject.functionName,
                         condition.rightObject.functionObject.category);
            return result;
        }
        if (true == listRHSOutputs.empty())
        {
            LOG_FN_DEBUG(__FILENAME__,__func__," RHS function does not return any value function->category 4",
                         condition.rightObject.functionObject.functionName,
                         condition.rightObject.functionObject.category,
                         condition.rightObject.functionObject.mandatoryParameter,
                         condition.rightObject.functionObject.optionalParameter);
            return false;
        }
        RHSString = listRHSOutputs[0];
    }
    if ((INC == operation) || (EXC == operation)) //operator is include or exclude operator
    {
        result = (INC == operation) ? false : true;
        for (itListOutputs = listLHSOutputs.begin(); itListOutputs != listLHSOutputs.end();
                        itListOutputs++)
        {
            if (*itListOutputs == RHSString)
            {
                result = !result;
                break;
            }
        }
    }
    else // mathematical operator
    {
        result = false;
        if(false == listLHSOutputs.empty())
        {
            if (true == mMapFunctionReturnValue[condition.leftObject.functionName]) // function if of type string
            {
                LHSString = listLHSOutputs[0];
                result = _conditionResult(LHSString, operation, RHSString);
            }
            else //function is of type integer
            {
                RHSInt = atoi(RHSString.data());
                LHSInt = atoi(listLHSOutputs[0].data());
                result = _conditionResult(LHSInt, operation, RHSInt);
            }
        }
    }
    return result;
}

am_Error_e CAmPolicyEngine::_evaluateConditionSet(
                const std::vector<gc_ConditionStruct_s >& listConditionSets,
                const gc_triggerParams_s& parameters)
{
    std::vector<gc_ConditionStruct_s >::const_iterator itListConditions;
    for (itListConditions = listConditionSets.begin(); itListConditions != listConditionSets.end();
                    ++itListConditions)
    {
        if (false == _isConditionTrue(*itListConditions, parameters))
        {
            return E_NOT_POSSIBLE;
        }
    }
    return E_OK;
}

bool CAmPolicyEngine::_getActionsfromPolicy(const gc_Process_s& process,
                                            std::vector<gc_Action_s >& listActions,
                                            const gc_triggerParams_s& parameters)
{
    bool returnValue = false;
    listActions.clear();
    if (E_OK == _evaluateConditionSet(process.listConditions, parameters))
    {
        LOG_FN_INFO(__FILENAME__,__func__,"Policy evaluated to true:", process.comment);
        listActions.insert(listActions.end(), process.listActions.begin(),
                           process.listActions.end());
        returnValue = process.stopEvaluation;
    }
    return returnValue;
}

void CAmPolicyEngine::_removeDoubleQuotes(std::string& inputString,
                                          const std::string& replaceString)
{
    std::size_t found = inputString.find(FUNCTION_MACRO_SUPPORTED_REQUESTING);
    while (std::string::npos != found)
    {
        char * temp = (char *)inputString.c_str();
        if (temp[found - 1] != '\"')
        {
            inputString.replace(found, SIZE_OF_REQUESTING_MACRO, replaceString);
        }
        found = inputString.find(FUNCTION_MACRO_SUPPORTED_REQUESTING, found + 1);
    }
    found = inputString.find("\"");
    while (std::string::npos != found)
    {
        inputString.erase(found, 1);
        found = inputString.find("\"");
    }
}

void CAmPolicyEngine::_convertActionParamToValue(std::vector<gc_Action_s >& listActions,
                                                 const gc_triggerParams_s& parameters)
{
    std::vector<gc_Action_s >::iterator itListActions;
    std::map<std::string, std::string >::iterator itMapParameters;
    for (itListActions = listActions.begin(); itListActions != listActions.end(); itListActions++)
    {
        //for param_map of action parameter
        for (itMapParameters = (*itListActions).mapParameters.begin();
                        itMapParameters != (*itListActions).mapParameters.end(); itMapParameters++)
        {
            if (itMapParameters->first == ACTION_PARAM_EXCEPT_SOURCE_NAME)
            {
                _removeDoubleQuotes(itMapParameters->second, parameters.sourceName);
            }
            else if (itMapParameters->first == ACTION_PARAM_EXCEPT_SINK_NAME)
            {
                _removeDoubleQuotes(itMapParameters->second, parameters.sinkName);
            }
            else if (itMapParameters->first == ACTION_PARAM_EXCEPT_CLASS_NAME)
            {
                _removeDoubleQuotes(itMapParameters->second, parameters.className);
            }
            // removing the double quote from action parameter.
            else if (itMapParameters->second.data()[0] == '"')
            {
                _removeDoubleQuotes(itMapParameters->second, "");
            }
            else
            {
                //converting Macro "REQUESTING" to actual value at run time
                if (FUNCTION_MACRO_SUPPORTED_REQUESTING == itMapParameters->second)
                {
                    if (ACTION_PARAM_SOURCE_NAME == itMapParameters->first) //macro is for source name
                    {
                        itMapParameters->second = parameters.sourceName;
                    }
                    else if (ACTION_PARAM_SINK_NAME == itMapParameters->first) //macro is for sink name
                    {
                        itMapParameters->second = parameters.sinkName;
                    }
                    else if (ACTION_PARAM_CLASS_NAME == itMapParameters->first) // macro is for class name
                    {
                        itMapParameters->second = parameters.className;
                    }
                }
            }
        }
    }
}

am_Error_e CAmPolicyEngine::_getActions(const gc_Trigger_e trigger,
                                        std::vector<gc_Action_s >& listActions,
                                        const gc_triggerParams_s& parameters)
{
    std::vector<gc_Process_s > listProcesses;
    std::vector<gc_Process_s >::iterator itListProcesses;
    std::vector<gc_Action_s > listActionSets;
    std::vector<gc_Action_s >::iterator itListActions;
    listActions.clear();

    if (E_OK != mpConfigReader->getListProcess(trigger, listProcesses))
    {
        return E_NOT_POSSIBLE;
    }
    for (itListProcesses = listProcesses.begin(); itListProcesses != listProcesses.end();
                    ++itListProcesses)
    {
        bool stopEvaluation = _getActionsfromPolicy(*itListProcesses, listActionSets, parameters);
        listActions.insert(listActions.end(), listActionSets.begin(), listActionSets.end());
        if (true == stopEvaluation)
        {
            break;
        }
    }
    _convertActionParamToValue(listActions, parameters);
    _getImplicitActions(trigger, listActions, parameters);
    LOG_FN_INFO(__FILENAME__,__func__," ----------------------------------------");
    for (itListActions = listActions.begin(); itListActions != listActions.end(); itListActions++)
    {
        LOG_FN_INFO(__FILENAME__,__func__,itListActions->actionName);
    }
    LOG_FN_INFO(__FILENAME__,__func__," ----------------------------------------");
    return E_OK;
}

// return the value of mandatory and optional parameter
void CAmPolicyEngine::_getValueOfParameter(const gc_ConditionStruct_s& conditionInstance,
                                           const bool isLHS, std::string& mandatoryParameter,
                                           std::string &optionalParameter) const
{
    bool isValueMacro;
    std::string tempMandatoryParameter;
    std::string tempOptionalParameter;
    isValueMacro = (isLHS == true) ? conditionInstance.leftObject.isValueMacro : conditionInstance.rightObject.functionObject.isValueMacro;
    tempMandatoryParameter =
                    (isLHS == true) ? conditionInstance.leftObject.mandatoryParameter : conditionInstance.rightObject.functionObject.mandatoryParameter;
    tempOptionalParameter =
                    (isLHS == true) ? conditionInstance.leftObject.optionalParameter : conditionInstance.rightObject.functionObject.optionalParameter;
    if ((FUNCTION_MACRO_SUPPORTED_REQUESTING != tempMandatoryParameter) || (false == isValueMacro))
    {
        mandatoryParameter = tempMandatoryParameter;
    }
    optionalParameter = tempOptionalParameter;
}

// function overloaded to support 3 parameter functionality
void CAmPolicyEngine::_getValueOfParameter(const gc_ConditionStruct_s &conditionInstance,
                                           const bool isLHS, std::string& mandatoryParameter) const
{
    std::string optionalParameter;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);
}

// return weather all domain are requested or single domain is requested
bool CAmPolicyEngine::_isSingleDomainRequest(const gc_ConditionStruct_s &conditionInstance,
                                             const bool isLHS,
                                             std::string& mandatoryParameter) const
{
    bool result = false;
    bool isValueMacro;
    std::string tempMandatoryParameter;
    isValueMacro = (isLHS == true) ? conditionInstance.leftObject.isValueMacro : conditionInstance.rightObject.functionObject.isValueMacro;
    tempMandatoryParameter =
                    (isLHS == true) ? conditionInstance.leftObject.mandatoryParameter : conditionInstance.rightObject.functionObject.mandatoryParameter;
    if (FUNCTION_MACRO_SUPPORTED_REQUESTING == tempMandatoryParameter)
    {
        result = true;
    }
    else if ((FUNCTION_MACRO_SUPPORTED_ALL != tempMandatoryParameter) || (false == isValueMacro))
    {
        mandatoryParameter = tempMandatoryParameter;
        result = true;
    }
    return result;
}

// get the connection list based on element type,name and optional parameter passed in condition in configuration
am_Error_e CAmPolicyEngine::_getConnectionList(
                const gc_Element_e elementType, const std::string& elementName,
                const std::string& optionalParameter,
                std::vector<gc_ConnectionInfo_s > &listConnectionInfo,
                const gc_triggerParams_s &parameters) const
{
    std::vector<gc_ConnectionInfo_s > listLocalConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itListConnectionInfo;
    am_Error_e result = E_UNKNOWN;
    if (E_OK == mpPolicyReceive->getListMainConnections(elementType, elementName,
                                                        listLocalConnectionInfo))
    {
        // if parameter is ALL or empty means all the connection of this class is needed
        if ((FUNCTION_MACRO_SUPPORTED_ALL == optionalParameter) || (true
                        == optionalParameter.empty()))
        {
            listConnectionInfo = listLocalConnectionInfo;
        }
        else if (FUNCTION_MACRO_SUPPORTED_OTHERS == optionalParameter) // except requesting all connection needed
        {
            for (itListConnectionInfo = listLocalConnectionInfo.begin();
                            itListConnectionInfo != listLocalConnectionInfo.end();
                            itListConnectionInfo++)
            {
                if (((*itListConnectionInfo).sinkName == parameters.sinkName) && ((*itListConnectionInfo).sourceName
                                == parameters.sourceName))
                {
                    continue;
                }
                listConnectionInfo.push_back((*itListConnectionInfo));
            }
        }
        else // for macro like CS_CONNECTED/CS_DISCONNECTED etc as specified in configuration
        {
            for (itListConnectionInfo = listLocalConnectionInfo.begin();
                            itListConnectionInfo != listLocalConnectionInfo.end();
                            itListConnectionInfo++)
            {
                // convert the macro in value based on schema and store in std::string format
                if (to_string((*itListConnectionInfo).connectionState) == optionalParameter)
                {
                    listConnectionInfo.push_back((*itListConnectionInfo));
                }
            }
        }
        result = E_OK;
    }
    return result;
}

//find the device i.e. router side volume based on element type and name
am_Error_e CAmPolicyEngine::_findDeviceVolume(const gc_Element_e elementType,
                                              const gc_ConditionStruct_s &conditionInstance,
                                              std::string& elementName,
                                              std::vector<std::string > &listOutputs,
                                              const bool isLHS)
{
    am_volume_t deviceVolume;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, elementName);
    // get device volume based on element name
    if (E_OK == mpPolicyReceive->getVolume(elementType, elementName, deviceVolume))
    {
        // store the volume in std::string format
        listOutputs.push_back(to_string(deviceVolume));
        result = E_OK;
    }
    return result;
}

//find the user i.e main volume based on element type and name
am_Error_e CAmPolicyEngine::_findMainVolume(const gc_Element_e elementType,
                                            const gc_ConditionStruct_s &conditionInstance,
                                            std::string& elementName,
                                            std::vector<std::string > &listOutputs,
                                            const bool isLHS)
{
    am_mainVolume_t volume;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, elementName);
    // get main volume based on element name
    if (E_OK == mpPolicyReceive->getMainVolume(elementType, elementName, volume))
    {
        // store the volume in std::string format
        listOutputs.push_back(to_string(volume));
        result = E_OK;
    }
    return result;
}

//find the device i.e. router side property value based on element type and name
am_Error_e CAmPolicyEngine::_findDevicePropertyValue(const gc_Element_e elementType,
                                                     const gc_ConditionStruct_s &conditionInstance,
                                                     std::string& elementName,
                                                     std::vector<std::string > &listOutputs,
                                                     const bool isLHS)
{
    int16_t value;
    std::string propertyType;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, elementName, propertyType);
    am_CustomSoundPropertyType_t property = (am_CustomSoundPropertyType_t)atoi(propertyType.data());
    // get device property based on element name
    if (E_OK == mpPolicyReceive->getSoundProperty(elementType, elementName, property, value))
    {
        // store the value in std::string format
        listOutputs.push_back(to_string(value));
        result = E_OK;
    }
    return result;
}

//find the user i.e main property value based on element type and name
am_Error_e CAmPolicyEngine::_findUserPropertyValue(const gc_Element_e elementType,
                                                   const gc_ConditionStruct_s &conditionInstance,
                                                   std::string& elementName,
                                                   std::vector<std::string > &listOutputs,
                                                   const bool isLHS)
{
    int16_t value;
    std::string propertyType;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, elementName, propertyType);
    am_CustomMainSoundPropertyType_t property = (am_CustomMainSoundPropertyType_t)atoi(
                    propertyType.data());
    // get user property based on element name
    if (E_OK == mpPolicyReceive->getMainSoundProperty(elementType, elementName, property, value))
    {
        // store the value in std::string format
        listOutputs.push_back(to_string(value));
        result = E_OK;
    }
    return result;
}

am_Error_e CAmPolicyEngine::_findElementName(const gc_ConditionStruct_s &conditionInstance,
                                             std::vector<std::string > &listOutputs,
                                             const std::string& name, const bool isLHS)
{
    am_Error_e result = E_UNKNOWN;
    bool isValueMacro;
    std::string mandatoryParameter;
    isValueMacro = (isLHS == true) ? conditionInstance.leftObject.isValueMacro : conditionInstance.rightObject.functionObject.isValueMacro;
    mandatoryParameter =
                    (isLHS == true) ? conditionInstance.leftObject.mandatoryParameter : conditionInstance.rightObject.functionObject.mandatoryParameter;
    if ((FUNCTION_MACRO_SUPPORTED_REQUESTING == mandatoryParameter) && (true == isValueMacro))
    {
        result = E_OK;
        listOutputs.push_back(name);
    }
    return result;
}

// find the name of sink
am_Error_e CAmPolicyEngine::_findSinkName(const gc_ConditionStruct_s &conditionInstance,
                                          std::vector<std::string > &listOutputs,
                                          const gc_triggerParams_s &parameters, const bool isLHS)
{
    return _findElementName(conditionInstance, listOutputs, parameters.sinkName, isLHS);
}

// find the name of source
am_Error_e CAmPolicyEngine::_findSourceName(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters, const bool isLHS)
{
    return _findElementName(conditionInstance, listOutputs, parameters.sourceName, isLHS);
}

am_Error_e CAmPolicyEngine::_findDomainName(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters, const bool isLHS)
{
    return _findElementName(conditionInstance, listOutputs, parameters.domainName, isLHS);
}

// find the name of class
am_Error_e CAmPolicyEngine::_findClassName(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           const gc_triggerParams_s &parameters, const bool isLHS)
{
    return _findElementName(conditionInstance, listOutputs, parameters.className, isLHS);
}

// find the name of connection
am_Error_e CAmPolicyEngine::_findConnectionName(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    if(parameters.connectionName.empty() == true)
    {
        return _findElementName(conditionInstance, listOutputs,
                                    parameters.sourceName + ":" + parameters.sinkName, isLHS);
    }
    else
    {
        return _findElementName(conditionInstance, listOutputs,
                                            parameters.connectionName, isLHS);
    }

}

// find the name of domain from source name
template <typename Telement>
am_Error_e CAmPolicyEngine::_findDomainOrClassOfElementName(
                Telement& elementInstance, const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, std::string& mandatoryParameter,
                const bool isLHS, const bool isClassRequired)
{
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    if (E_OK == mpConfigReader->getElementByName(mandatoryParameter, elementInstance))
    {
        if (true == isClassRequired)
        {
            listOutputs.push_back(elementInstance.className);
        }
        else
        {
            listOutputs.push_back(elementInstance.domainName);
        }
        result = E_OK;
    }
    return result;
}

// find the name of domain from source name
am_Error_e CAmPolicyEngine::_findDomainOfSourceName(const gc_ConditionStruct_s &conditionInstance,
                                                    std::vector<std::string > &listOutputs,
                                                    const gc_triggerParams_s &parameters,
                                                    const bool isLHS)
{
    gc_Source_s sourceInstance;
    std::string mandatoryParameter = parameters.sourceName;
    return _findDomainOrClassOfElementName(sourceInstance, conditionInstance, listOutputs,
                                           mandatoryParameter, isLHS, false);
}

// find the name of domain from sink name
am_Error_e CAmPolicyEngine::_findDomainOfSinkName(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS)
{
    gc_Sink_s sinkInstance;
    std::string mandatoryParameter = parameters.sinkName;
    return _findDomainOrClassOfElementName(sinkInstance, conditionInstance, listOutputs,
                                           mandatoryParameter, isLHS, false);
}

// find the name of class from sink name
am_Error_e CAmPolicyEngine::_findClassOfSinkName(const gc_ConditionStruct_s &conditionInstance,
                                                 std::vector<std::string > &listOutputs,
                                                 const gc_triggerParams_s &parameters,
                                                 const bool isLHS)
{
    gc_Sink_s sinkInstance;
    std::string mandatoryParameter = parameters.sinkName;
    return _findDomainOrClassOfElementName(sinkInstance, conditionInstance, listOutputs,
                                           mandatoryParameter, isLHS, true);
}

// find the name of class from source name
am_Error_e CAmPolicyEngine::_findClassOfSourceName(const gc_ConditionStruct_s &conditionInstance,
                                                   std::vector<std::string > &listOutputs,
                                                   const gc_triggerParams_s &parameters,
                                                   const bool isLHS)
{
    gc_Source_s sourceInstance;
    std::string mandatoryParameter = parameters.sourceName;
    return _findDomainOrClassOfElementName(sourceInstance, conditionInstance, listOutputs,
                                           mandatoryParameter, isLHS, true);
}

am_Error_e CAmPolicyEngine::_getElementNameList(const std::string& mandatoryParameter,
                                                const std::string& optionalParameter,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isSinkRequired)
{
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itListConnectionInfo;
    am_Error_e result = E_UNKNOWN;
    if (E_OK == _getConnectionList(ET_CLASS, mandatoryParameter, optionalParameter,
                                   listConnectionInfo, parameters))
    {
        for (itListConnectionInfo = listConnectionInfo.begin();
                        itListConnectionInfo != listConnectionInfo.end(); itListConnectionInfo++)
        {
            if (true == isSinkRequired)
            {
                listOutputs.push_back((*itListConnectionInfo).sinkName);
            }
            else
            {
                listOutputs.push_back((*itListConnectionInfo).sourceName);
            }
        }
        result = E_OK;
    }
    return result;
}

//get the list of sink belonging to class using class name
am_Error_e CAmPolicyEngine::_findSinkOfClassName(const gc_ConditionStruct_s &conditionInstance,
                                                 std::vector<std::string > &listOutputs,
                                                 const gc_triggerParams_s &parameters,
                                                 const bool isLHS)
{
    std::string optionalParameter;
    std::string mandatoryParameter = parameters.className;
    am_Error_e result = E_UNKNOWN;

    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);
    // if parameter is ALL or empty means all the sink of this class is needed
    if ((FUNCTION_MACRO_SUPPORTED_ALL == optionalParameter) || (true == optionalParameter.empty()))
    {
        std::vector<gc_Sink_s > listSinks;
        std::vector<gc_Sink_s >::iterator itListSinks;
        if (E_OK == mpConfigReader->getListElements(listSinks))
        {
            for (itListSinks = listSinks.begin(); itListSinks != listSinks.end(); itListSinks++)
            {
                if ((*itListSinks).className == mandatoryParameter)
                {
                    listOutputs.push_back((*itListSinks).name);
                }
            }
            result = E_OK;
        }
    }
    else //get the connection list and find the sink name involved in connection
    {
        result = _getElementNameList(mandatoryParameter, optionalParameter, listOutputs, parameters,
                                     true);
    }
    return result;
}

//get the list of source belonging to class using class name
am_Error_e CAmPolicyEngine::_findSourceOfClassName(const gc_ConditionStruct_s &conditionInstance,
                                                   std::vector<std::string > &listOutputs,
                                                   const gc_triggerParams_s &parameters,
                                                   const bool isLHS)
{
    std::string optionalParameter;
    std::string mandatoryParameter = parameters.className;
    am_Error_e result = E_UNKNOWN;

    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);
    // if parameter is ALL or empty means all the source of this class is needed
    if ((FUNCTION_MACRO_SUPPORTED_ALL == optionalParameter) || (true == optionalParameter.empty()))
    {
        std::vector<gc_Source_s > listSources;
        std::vector<gc_Source_s >::iterator itListSources;
        if (E_OK == mpConfigReader->getListElements(listSources))
        {
            for (itListSources = listSources.begin(); itListSources != listSources.end();
                            itListSources++)
            {
                if ((*itListSources).className == mandatoryParameter)
                {
                    listOutputs.push_back((*itListSources).name);
                }
            }
            result = E_OK;
        }
    }
    else //get the connection list and find the source name involved in connection
    {
        result = _getElementNameList(mandatoryParameter, optionalParameter, listOutputs, parameters,
                                     false);
    }
    return result;
}

am_Error_e CAmPolicyEngine::_findElementPeek(const gc_ConditionStruct_s &conditionInstance,
                                             std::vector<std::string > &listOutputs,
                                             const std::string clasName,
                                             const bool isLHS,const bool isSinkRequired)
{
    std::string optionalParameter;
    std::string mandatoryParameter = clasName;
    am_Error_e result = E_UNKNOWN;
    gc_Order_e order = O_HIGH_PRIORITY;
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;

    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);
    std::string tempOptionalParameter2 =
                    (isLHS == true) ? conditionInstance.leftObject.optionalParameter2 : conditionInstance.rightObject.functionObject.optionalParameter2;

    if(false == optionalParameter.empty())
    {
        order = (gc_Order_e)atoi(optionalParameter.data());
    }
    if (E_OK == mpPolicyReceive->getListMainConnections(mandatoryParameter,
                                                        listConnectionInfo,order))
    {
        if(false == listConnectionInfo.empty())
        {
            if(true == tempOptionalParameter2.empty())
            {
                if(true == isSinkRequired)
                {
                    listOutputs.push_back(listConnectionInfo[0].sinkName);
                }
                else
                {
                    listOutputs.push_back(listConnectionInfo[0].sourceName);
                }
                result = E_OK;
            }
            else
            {
                int index = atoi(tempOptionalParameter2.data());
                if(listConnectionInfo.size() >= index)
                {
                    if(true == isSinkRequired)
                    {
                        listOutputs.push_back(listConnectionInfo[index].sinkName);
                    }
                    else
                    {
                        listOutputs.push_back(listConnectionInfo[index].sourceName);
                    }
                    result = E_OK;
                }
            }
        }
    }
    return result;
}

am_Error_e CAmPolicyEngine::_findSinkPeek(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters,
                                            const bool isLHS)
{
    return _findElementPeek(conditionInstance,listOutputs,parameters.className,isLHS,true);
}

//peek the source belonging to class using class name
am_Error_e CAmPolicyEngine::_findSourcePeek(const gc_ConditionStruct_s &conditionInstance,
                                            std::vector<std::string > &listOutputs,
                                            const gc_triggerParams_s &parameters,
                                            const bool isLHS)
{
    return _findElementPeek(conditionInstance,listOutputs,parameters.className,isLHS,false);
}

template <typename Telement>
am_Error_e CAmPolicyEngine::_findElementPriority(Telement& elementInstance,
                                                 const gc_ConditionStruct_s &conditionInstance,
                                                 std::string& mandatoryParameter, const bool isLHS,
                                                 std::vector<std::string > &listOutputs)
{
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    if (E_OK == mpConfigReader->getElementByName(mandatoryParameter, elementInstance))
    {
        listOutputs.push_back(to_string(elementInstance.priority));
        result = E_OK;
    }
    return result;
}

// get the priority of sink by sink name
am_Error_e CAmPolicyEngine::_findSinkPriority(const gc_ConditionStruct_s &conditionInstance,
                                              std::vector<std::string > &listOutputs,
                                              const gc_triggerParams_s &parameters,
                                              const bool isLHS)
{
    gc_Sink_s sinkInstance;
    std::string mandatoryParameter = parameters.sinkName;
    return _findElementPriority(sinkInstance, conditionInstance, mandatoryParameter, isLHS,
                                listOutputs);
}

// get the priority of source by source name
am_Error_e CAmPolicyEngine::_findSourcePriority(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    gc_Source_s sourceInstance;
    std::string mandatoryParameter = parameters.sourceName;
    return _findElementPriority(sourceInstance, conditionInstance, mandatoryParameter, isLHS,
                                listOutputs);
}

// get the priority of class by class name
am_Error_e CAmPolicyEngine::_findClassPriority(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS)
{
    gc_Class_s classInstance;
    std::string mandatoryParameter = parameters.className;
    return _findElementPriority(classInstance, conditionInstance, mandatoryParameter, isLHS,
                                listOutputs);
}

// get the priority of connection by connection name
am_Error_e CAmPolicyEngine::_findConnectionPriority(const gc_ConditionStruct_s &conditionInstance,
                                                    std::vector<std::string > &listOutputs,
                                                    const gc_triggerParams_s &parameters,
                                                    const bool isLHS)
{
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itListConnectionInfo;
    am_Error_e result = E_UNKNOWN;
    std::string mandatoryParameter = parameters.sourceName + ":" + parameters.sinkName;

    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    // get list of connection based on connection name
    if (E_OK == mpPolicyReceive->getListMainConnections(ET_CONNECTION, mandatoryParameter,
                                                        listConnectionInfo))
    {
        for (itListConnectionInfo = listConnectionInfo.begin();
                        itListConnectionInfo != listConnectionInfo.end(); itListConnectionInfo++)
        {
            listOutputs.push_back(to_string((*itListConnectionInfo).priority));
        }
        result = E_OK;
    }
    return result;
}

// get the list of connection priority of connections using class name
am_Error_e CAmPolicyEngine::_findConnectionOfClassPriority(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itListConnectionInfo;
    std::string optionalParameter;
    std::string mandatoryParameter = parameters.className;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);

    if (E_OK == _getConnectionList(ET_CLASS, mandatoryParameter, optionalParameter,
                                   listConnectionInfo, parameters))
    {
        for (itListConnectionInfo = listConnectionInfo.begin();
                        itListConnectionInfo != listConnectionInfo.end(); itListConnectionInfo++)
        {
            listOutputs.push_back(to_string((*itListConnectionInfo).priority));
        }
        result = E_OK;
    }
    return result;
}

// get the list of connection state of connections using class name
am_Error_e CAmPolicyEngine::_findConnectionOfClassState(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itlistConnectionInfo;
    std::string optionalParameter;
    std::string mandatoryParameter = parameters.className;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);
    // get list of connection based on class name
    if (E_OK == _getConnectionList(ET_CLASS, mandatoryParameter, optionalParameter,
                                   listConnectionInfo, parameters))
    {
        for (itlistConnectionInfo = listConnectionInfo.begin();
                        itlistConnectionInfo != listConnectionInfo.end(); itlistConnectionInfo++)
        {
            listOutputs.push_back(to_string((*itlistConnectionInfo).connectionState));
        }
        result = E_OK;
    }
    return result;
}

am_Error_e CAmPolicyEngine::_findElementConnectionState(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, std::string& mandatoryParameter,
                const bool isLHS, const gc_Element_e elementType)
{
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itlistConnectionInfo;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);

    // get list of connection based on source name
    if (E_OK == mpPolicyReceive->getListMainConnections(elementType, mandatoryParameter,
                                                        listConnectionInfo))
    {
        gc_ConnectionInfo_s elementToFind(elementType, mandatoryParameter);
        std::vector<gc_ConnectionInfo_s > listOutputConnectionInfo;
        std::copy_if(listConnectionInfo.begin(),
                     listConnectionInfo.end(),
                     std::back_inserter(listOutputConnectionInfo),
                     elementToFind);
        std::vector<gc_ConnectionInfo_s >::iterator itlistOutputs;
        for(itlistOutputs = listOutputConnectionInfo.begin();
            itlistOutputs != listOutputConnectionInfo.end();++itlistOutputs)
        {
            listOutputs.push_back(to_string((*itlistOutputs).connectionState));
        }
        return E_OK;
    }
    return E_DATABASE_ERROR;
}
// get the list of connection state of connections using source name
am_Error_e CAmPolicyEngine::_findSourceConnectionState(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findElementConnectionState(conditionInstance, listOutputs, mandatoryParameter, isLHS,
                                       ET_SOURCE);
}

// get the list of connection state of connections using sink name
am_Error_e CAmPolicyEngine::_findSinkConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                                     std::vector<std::string > &listOutputs,
                                                     const gc_triggerParams_s &parameters,
                                                     const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findElementConnectionState(conditionInstance, listOutputs, mandatoryParameter, isLHS,
                                       ET_SINK);
}

am_Error_e CAmPolicyEngine::_findConnectionConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                                     std::vector<std::string > &listOutputs,
                                                     const gc_triggerParams_s &parameters,
                                                     const bool isLHS)
{
    std::string mandatoryParameter;
    return _findElementConnectionState(conditionInstance, listOutputs, mandatoryParameter, isLHS,
                                       ET_CONNECTION);
}

//find the sink device volume
am_Error_e CAmPolicyEngine::_findSinkDeviceVolume(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findDeviceVolume(ET_SINK, conditionInstance, mandatoryParameter, listOutputs, isLHS);
}

//find the source device volume
am_Error_e CAmPolicyEngine::_findSourceDeviceVolume(const gc_ConditionStruct_s &conditionInstance,
                                                    std::vector<std::string > &listOutputs,
                                                    const gc_triggerParams_s &parameters,
                                                    const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findDeviceVolume(ET_SOURCE, conditionInstance, mandatoryParameter, listOutputs, isLHS);
}

//find the connection device volume
am_Error_e CAmPolicyEngine::_findConnectionDeviceVolume(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName + ":" + parameters.sinkName;
    return _findDeviceVolume(ET_CONNECTION, conditionInstance, mandatoryParameter, listOutputs,
                             isLHS);
}

//find the list of volume of connection belonging to given class
am_Error_e CAmPolicyEngine::_findConnectionOfClassDeviceVolume(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::vector<gc_ConnectionInfo_s > listConnectionInfo;
    std::vector<gc_ConnectionInfo_s >::iterator itlistConnectionInfo;
    std::string optionalParameter;
    std::string mandatoryParameter = parameters.className;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter, optionalParameter);

    //get the list of connections belonging to class based on optional parameter
    if (E_OK == _getConnectionList(ET_CLASS, mandatoryParameter, optionalParameter,
                                   listConnectionInfo, parameters))
    {
        for (itlistConnectionInfo = listConnectionInfo.begin();
                        itlistConnectionInfo != listConnectionInfo.end(); itlistConnectionInfo++)
        {
            listOutputs.push_back(to_string((*itlistConnectionInfo).volume));
        }
        result = E_OK;
    }
    return result;
}

//find the sink user volume
am_Error_e CAmPolicyEngine::_findSinkMainVolume(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findMainVolume(ET_SINK, conditionInstance, mandatoryParameter, listOutputs, isLHS);
}

//find the trigger user volume
am_Error_e CAmPolicyEngine::_findUserMainVolume(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.mainVolume));
    return E_OK;
}

//find the trigger connection state value
am_Error_e CAmPolicyEngine::_findUserConnectionState(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.connectionState));
    return E_OK;
}

//find the trigger error value
am_Error_e CAmPolicyEngine::_findUserErrorValue(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.status));
    return E_OK;
}

//find the source user volume
am_Error_e CAmPolicyEngine::_findSourceMainVolume(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findMainVolume(ET_SOURCE, conditionInstance, mandatoryParameter, listOutputs, isLHS);
}

//find the sink device property value
am_Error_e CAmPolicyEngine::_findSinkDevicePropertyValue(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findDevicePropertyValue(ET_SINK, conditionInstance, mandatoryParameter, listOutputs,
                                    isLHS);
}

//find the source device property value
am_Error_e CAmPolicyEngine::_findSourceDevicePropertyValue(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findDevicePropertyValue(ET_SOURCE, conditionInstance, mandatoryParameter, listOutputs,
                                    isLHS);
}

//find the sink user property value
am_Error_e CAmPolicyEngine::_findSinkUserPropertyValue(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findUserPropertyValue(ET_SINK, conditionInstance, mandatoryParameter, listOutputs,
                                  isLHS);
}

//find the trigger user property value
am_Error_e CAmPolicyEngine::_findUserMainSoundPropertyValue(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.mainSoundProperty.value));
    return E_OK;
}

//find the trigger user property type
am_Error_e CAmPolicyEngine::_findUserMainSoundPropertyType(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.mainSoundProperty.type));
    return E_OK;
}

//find the source user property value
am_Error_e CAmPolicyEngine::_findSourceUserPropertyValue(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findUserPropertyValue(ET_SOURCE, conditionInstance, mandatoryParameter, listOutputs,
                                  isLHS);
}

//find the trigger user property value
am_Error_e CAmPolicyEngine::_findUserSystemPropertyValue(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.systemProperty.value));
    return E_OK;
}

//find the trigger user property type
am_Error_e CAmPolicyEngine::_findUserSystemPropertyType(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.systemProperty.type));
    return E_OK;
}

//find the system property value
am_Error_e CAmPolicyEngine::_findSystemPropertyValue(const gc_ConditionStruct_s &conditionInstance,
                                                     std::vector<std::string > &listOutputs,
                                                     const gc_triggerParams_s &parameters,
                                                     const bool isLHS)
{
    int16_t value;
    std::string propertyType;
    am_Error_e result = E_UNKNOWN;
    // convert the macro in value based on schema and store in std::string format
    propertyType = to_string(parameters.systemProperty.type);
    _getValueOfParameter(conditionInstance, isLHS, propertyType);
    am_CustomSystemPropertyType_t property = (am_CustomSystemPropertyType_t)atoi(
                    propertyType.data());
    // get system property
    if (E_OK == mpPolicyReceive->getSystemProperty(property, value))
    {
        listOutputs.push_back(to_string(value));
        result = E_OK;
    }
    return result;
}

am_Error_e CAmPolicyEngine::_findUserMuteState(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.muteState));
    return E_OK;
}

am_Error_e CAmPolicyEngine::_findMuteState(const gc_ConditionStruct_s &conditionInstance,
                                           std::vector<std::string > &listOutputs,
                                           std::string& mandatoryParameter, const bool isLHS,
                                           const gc_Element_e elementType)
{
    am_MuteState_e muteState;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);

    if (E_OK == mpPolicyReceive->getMuteState(elementType, mandatoryParameter, muteState))
    {
        listOutputs.push_back(to_string(muteState));
        result = E_OK;
    }
    return result;
}

//find the mute state of sink
am_Error_e CAmPolicyEngine::_findSinkMuteState(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findMuteState(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_SINK);
}

//find the mute state of active connection of class
am_Error_e CAmPolicyEngine::_findClassMuteState(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                const gc_triggerParams_s &parameters,
                                                const bool isLHS)
{
    std::string mandatoryParameter = parameters.className;
    return _findMuteState(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_CLASS);
}

//find the mute state of connection
am_Error_e CAmPolicyEngine::_findConnectionMuteState(const gc_ConditionStruct_s &conditionInstance,
                                                     std::vector<std::string > &listOutputs,
                                                     const gc_triggerParams_s &parameters,
                                                     const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName + ":" + parameters.sinkName;
    return _findMuteState(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_CONNECTION);
}

am_Error_e CAmPolicyEngine::_findAvailability(const gc_ConditionStruct_s &conditionInstance,
                                              std::vector<std::string > &listOutputs,
                                              std::string& mandatoryParameter, const bool isLHS,
                                              const gc_Element_e elementType,
                                              const bool isReasonRequired)
{
    am_Availability_s availability;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    if (E_OK == mpPolicyReceive->getAvailability(elementType, mandatoryParameter, availability))
    {
        if (true == isReasonRequired)
        {
            listOutputs.push_back(to_string(availability.availabilityReason));
        }
        else
        {
            listOutputs.push_back(to_string(availability.availability));
        }
        result = E_OK;
    }
    return result;
}
//find the sink availability
am_Error_e CAmPolicyEngine::_findSinkAvailability(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findAvailability(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_SINK,
                             false);
}

//find the source availability
am_Error_e CAmPolicyEngine::_findSourceAvailability(const gc_ConditionStruct_s &conditionInstance,
                                                    std::vector<std::string > &listOutputs,
                                                    const gc_triggerParams_s &parameters,
                                                    const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findAvailability(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_SOURCE,
                             false);
}

am_Error_e CAmPolicyEngine::_findUserAvailability(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.availability.availability));
    return E_OK;
}

//find the sink availability reason
am_Error_e CAmPolicyEngine::_findSinkAvailabilityReason(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sinkName;
    return _findAvailability(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_SINK,
                             true);
}

//find the source availability reason
am_Error_e CAmPolicyEngine::_findSourceAvailabilityReason(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findAvailability(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_SOURCE,
                             true);
}

am_Error_e CAmPolicyEngine::_findUserAvailabilityReason(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.availability.availabilityReason));
    return E_OK;
}

//Reserved for future use
am_Error_e CAmPolicyEngine::_findConnectionFormat(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS)
{
    (void)conditionInstance;
    (void)listOutputs;
    (void)parameters;
    (void)isLHS;
    listOutputs.push_back(to_string(CF_GENIVI_STEREO));
    return E_OK;
}

//Reserved for future use
am_Error_e CAmPolicyEngine::_findConnectionOfClassFormat(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    (void)conditionInstance;
    (void)listOutputs;
    (void)parameters;
    (void)isLHS;
    listOutputs.push_back(to_string(CF_GENIVI_STEREO));
    return E_OK;
}
am_Error_e CAmPolicyEngine::_findInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                                std::vector<std::string > &listOutputs,
                                                std::string& mandatoryParameter, const bool isLHS,
                                                gc_Element_e elementType)
{
    am_InterruptState_e interruptState;
    am_Error_e result = E_UNKNOWN;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    if (E_OK == mpPolicyReceive->getInterruptState(elementType, mandatoryParameter, interruptState))
    {
        listOutputs.push_back(to_string(interruptState));
        result = E_OK;
    }
    return result;
}
//find the source interrupt state
am_Error_e CAmPolicyEngine::_findSourceInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                                      std::vector<std::string > &listOutputs,
                                                      const gc_triggerParams_s &parameters,
                                                      const bool isLHS)
{
    std::string mandatoryParameter = parameters.sourceName;
    return _findInterruptState(conditionInstance, listOutputs, mandatoryParameter, isLHS, ET_SOURCE);
}

//find the connection interrupt state
am_Error_e CAmPolicyEngine::_findConnectionInterruptState(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    std::string mandatoryParameter;
    return _findInterruptState(conditionInstance, listOutputs, mandatoryParameter, isLHS,
                               ET_CONNECTION);
}

am_Error_e CAmPolicyEngine::_findUserInterruptState(const gc_ConditionStruct_s &conditionInstance,
                                               std::vector<std::string > &listOutputs,
                                               const gc_triggerParams_s &parameters,
                                               const bool isLHS)
{
    listOutputs.push_back(to_string(parameters.interruptState));
    return E_OK;
}

//check sink is registered or not
am_Error_e CAmPolicyEngine::_findSinkIsRegistered(const gc_ConditionStruct_s &conditionInstance,
                                                  std::vector<std::string > &listOutputs,
                                                  const gc_triggerParams_s &parameters,
                                                  const bool isLHS)
{
    bool result;
    std::string mandatoryParameter = parameters.sinkName;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    // get registration status
    result = mpPolicyReceive->isRegistered(ET_SINK, mandatoryParameter);
    listOutputs.push_back(to_string(result));
    return E_OK;
}

//check source is registered or not
am_Error_e CAmPolicyEngine::_findSourceIsRegistered(const gc_ConditionStruct_s &conditionInstance,
                                                    std::vector<std::string > &listOutputs,
                                                    const gc_triggerParams_s &parameters,
                                                    const bool isLHS)
{
    bool result;
    std::string mandatoryParameter = parameters.sourceName;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    // get registration status
    result = mpPolicyReceive->isRegistered(ET_SOURCE, mandatoryParameter);
    listOutputs.push_back(to_string(result));
    return E_OK;
}

//check domain is registered or not
am_Error_e CAmPolicyEngine::_findDomainIsRegistered(const gc_ConditionStruct_s &conditionInstance,
                                                    std::vector<std::string > &listOutputs,
                                                    const gc_triggerParams_s &parameters,
                                                    const bool isLHS)
{
    bool result = false;
    am_Error_e returnValue = E_UNKNOWN;
    gc_Domain_s domain;
    std::vector<gc_Domain_s > listDomains;
    std::vector<gc_Domain_s >::iterator itListDomains;

    std::string mandatoryParameter = parameters.domainName;
    if (true == _isSingleDomainRequest(conditionInstance, isLHS, mandatoryParameter))
    {
        if (E_OK == mpConfigReader->getElementByName(mandatoryParameter, domain))
        {
            listDomains.push_back(domain);
        }
    }
    else //check for all the domains
    {
        mpConfigReader->getListElements(listDomains);
    }
    for (itListDomains = listDomains.begin(); itListDomains != listDomains.end(); itListDomains++)
    {
        returnValue = E_OK;
        // get registration complete status
        result = mpPolicyReceive->isRegistered(ET_DOMAIN, (*itListDomains).name);
        if (false == result)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"domain not registered:", (*itListDomains).name);
            break;
        }
    }
    listOutputs.push_back(to_string(result));
    return returnValue;
}

//check domain registration is completed or not
am_Error_e CAmPolicyEngine::_findIsDomainRegistrationComplete(
                const gc_ConditionStruct_s &conditionInstance,
                std::vector<std::string > &listOutputs, const gc_triggerParams_s &parameters,
                const bool isLHS)
{
    bool result = false;
    std::string mandatoryParameter = parameters.domainName;
    am_Error_e returnValue = E_UNKNOWN;
    gc_Domain_s domain;
    std::vector<gc_Domain_s > listDomains;
    std::vector<gc_Domain_s >::iterator itListDomains;

    if (true == _isSingleDomainRequest(conditionInstance, isLHS, mandatoryParameter))
    {
        if (E_OK == mpConfigReader->getElementByName(mandatoryParameter, domain))
        {
            listDomains.push_back(domain);
        }
    }
    else //check for all the domains
    {
        mpConfigReader->getListElements(listDomains);
    }
    for (itListDomains = listDomains.begin(); itListDomains != listDomains.end(); itListDomains++)
    {
        returnValue = E_OK;
        // get registration complete status
        result = mpPolicyReceive->isDomainRegistrationComplete((*itListDomains).name);
        if (false == result)
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"domain registration not complete:", (*itListDomains).name);
            break;
        }
    }
    listOutputs.push_back(to_string(result));
    return returnValue;
}

// get the source state
am_Error_e CAmPolicyEngine::_findSourceState(const gc_ConditionStruct_s &conditionInstance,
                                             std::vector<std::string > &listOutputs,
                                             const gc_triggerParams_s &parameters, const bool isLHS)
{
    int sourceState;
    am_Error_e result = E_UNKNOWN;
    std::string mandatoryParameter = parameters.sourceName;
    _getValueOfParameter(conditionInstance, isLHS, mandatoryParameter);
    if (E_OK == mpPolicyReceive->getState(ET_SOURCE, mandatoryParameter, sourceState))
    {
        listOutputs.push_back(to_string(static_cast<am_SourceState_e>(sourceState)));
        result = E_OK;
    }
    return result;
}

//get the domain state
am_Error_e CAmPolicyEngine::_findDomainState(const gc_ConditionStruct_s &conditionInstance,
                                             std::vector<std::string > &listOutputs,
                                             const gc_triggerParams_s &parameters, const bool isLHS)
{
    int domainState;
    am_Error_e result = E_OK;
    gc_Domain_s domain;
    std::vector<gc_Domain_s > listDomains;
    std::vector<gc_Domain_s >::iterator itListDomains;

    std::string mandatoryParameter = parameters.domainName;
    if (true == _isSingleDomainRequest(conditionInstance, isLHS, mandatoryParameter))
    {
        if (E_OK == mpConfigReader->getElementByName(mandatoryParameter, domain))
        {
            listDomains.push_back(domain);
        }
    }
    else // get state for all the domains
    {
        mpConfigReader->getListElements(listDomains);
    }
    for (itListDomains = listDomains.begin(); itListDomains != listDomains.end(); itListDomains++)
    {
        // get domain state
        if (E_OK != mpPolicyReceive->getState(ET_DOMAIN, (*itListDomains).name, domainState))
        {
            result = E_UNKNOWN;
            break;
        }
        listOutputs.push_back(to_string(static_cast<am_DomainState_e>(domainState)));
    }
    return result;
}
bool CAmPolicyEngine::_executeFunction(const std::string& functionName, const std::string& category,
                                       const gc_ConditionStruct_s &conditionInstance,
                                       std::vector<std::string > &listOutputs,
                                       const gc_triggerParams_s &parameters, const bool isLHS)
{
    am_Error_e error = (this->*mMapFunctionNameToFunctionMaps[functionName][category])(
                    conditionInstance, listOutputs, parameters, isLHS);
    return (error == E_OK) ? true : false;
}

void CAmPolicyEngine::_getListStaticSinks(const std::string& domainName,
                                          std::vector<gc_Sink_s >& listStaticSinks)
{
    std::vector<gc_Sink_s > listSinks;
    std::vector<gc_Sink_s >::iterator itListSinks;
    mpConfigReader->getListElements(listSinks);
    for (itListSinks = listSinks.begin(); itListSinks != listSinks.end(); itListSinks++)
    {
        if ((itListSinks->registrationType == REG_CONTROLLER) && (itListSinks->domainName
                        == domainName))
        {
            listStaticSinks.push_back(*itListSinks);
        }
    }
}
void CAmPolicyEngine::_getListStaticSources(const std::string& domainName,
                                            std::vector<gc_Source_s >& listStaticSources)
{
    std::vector<gc_Source_s > listSources;
    std::vector<gc_Source_s >::iterator itListSources;
    mpConfigReader->getListElements(listSources);
    for (itListSources = listSources.begin(); itListSources != listSources.end(); itListSources++)
    {
        if ((itListSources->registrationType == REG_CONTROLLER) && (itListSources->domainName
                        == domainName))
        {
            listStaticSources.push_back(*itListSources);
        }
    }
}

void CAmPolicyEngine::_getListStaticGateways(std::vector<std::string >& listGateways,
                                             std::string& listSources,
                                             std::string& listSinks)
{
    std::vector<gc_Gateway_s >::iterator itListGateways;
    std::vector<gc_Gateway_s > listConfiguredGateways;
    mpConfigReader->getListElements(listConfiguredGateways);
    for (itListGateways = listConfiguredGateways.begin();
                    itListGateways != listConfiguredGateways.end(); itListGateways++)
    {
        if ((false == mpPolicyReceive->isRegistered(ET_GATEWAY, itListGateways->name)) &&
            (itListGateways->registrationType == REG_CONTROLLER))
        {
            if ((true == mpPolicyReceive->isRegistered(ET_SOURCE, itListGateways->sourceName)) ||
                (std::string::npos != listSources.find(itListGateways->sourceName)))
            {
                if ((true == mpPolicyReceive->isRegistered(ET_SINK, itListGateways->sinkName)) ||
                    (std::string::npos != listSinks.find(itListGateways->sinkName)))
                {
                    listGateways.push_back(itListGateways->name);
                }
            }
        }
    }
}

void CAmPolicyEngine::_getImplicitActions(gc_Trigger_e trigger,
                                          std::vector<gc_Action_s >& listActions,
                                          const gc_triggerParams_s& parameters)
{
    uint8_t flags = 0;
    std::vector<gc_Sink_s > listStaticSinks;
    std::vector<gc_Sink_s >::iterator itListStaticSinks;
    std::vector<gc_Source_s > listStaticSources;
    std::vector<gc_Source_s >::iterator itListStaticSources;
    std::vector<std::string > listStaticGateways;
    std::vector<std::string >::iterator itListStaticGateways;
    std::map<std::string, std::string >::iterator itMapParam;
    std::string sinkList("");
    std::string sourceList("");
    gc_Action_s actionRegister;
    if ((trigger != SYSTEM_REGISTER_DOMAIN) && (trigger != SYSTEM_REGISTER_SINK)
        && (trigger != SYSTEM_REGISTER_SOURCE) && (trigger != SYSTEM_DOMAIN_REGISTRATION_COMPLETE))
    {
        return;
    }
    else if (trigger == SYSTEM_REGISTER_DOMAIN)
    {
        flags = (SEARCH_STATIC_SOURCE | SEARCH_STATIC_SINK | SEARCH_STATIC_GATEWAY);
    }
    else if (trigger == SYSTEM_REGISTER_SINK || trigger == SYSTEM_REGISTER_SOURCE)
    {
        flags = SEARCH_STATIC_GATEWAY;
    }
    if (flags & SEARCH_STATIC_SINK)
    {
        _getListStaticSinks(parameters.domainName, listStaticSinks);
    }
    if (flags & SEARCH_STATIC_SOURCE)
    {
        _getListStaticSources(parameters.domainName, listStaticSources);
    }
    actionRegister.actionName = CONFIG_ACTION_NAME_REGISTER;
    for (itListStaticSources = listStaticSources.begin();
                    itListStaticSources != listStaticSources.end(); ++itListStaticSources)
    {
        actionRegister.mapParameters[ACTION_PARAM_SOURCE_NAME] += (*itListStaticSources).name + " ";
    }
    for (itListStaticSinks = listStaticSinks.begin(); itListStaticSinks != listStaticSinks.end();
                    ++itListStaticSinks)
    {
        actionRegister.mapParameters[ACTION_PARAM_SINK_NAME] += (*itListStaticSinks).name + " ";
    }
    if (flags & SEARCH_STATIC_GATEWAY)
    {
        itMapParam = actionRegister.mapParameters.find(ACTION_PARAM_SINK_NAME);
        if(itMapParam!= actionRegister.mapParameters.end())
        {
            sinkList = itMapParam->second;
        }
        itMapParam = actionRegister.mapParameters.find(ACTION_PARAM_SOURCE_NAME);
        if(itMapParam!= actionRegister.mapParameters.end())
        {
            sourceList = itMapParam->second;
        }
        _getListStaticGateways(listStaticGateways,sourceList, sinkList);
    }
    for (itListStaticGateways = listStaticGateways.begin();
                    itListStaticGateways != listStaticGateways.end(); ++itListStaticGateways)
    {
        actionRegister.mapParameters[ACTION_PARAM_GATEWAY_NAME] += *itListStaticGateways + " ";
    }
    // append the action only if some source sink or gateway is found
    if (actionRegister.mapParameters.size() > 0)
    {
        listActions.push_back(actionRegister);
    }
}
am_Error_e CAmPolicyEngine::getListSystemProperties(
                std::vector<am_SystemProperty_s >& listSystemProperties)
{
    mpConfigReader->getListSystemProperties(listSystemProperties);
    return E_OK;
}
am_Error_e CAmPolicyEngine::getListClasses(std::vector<gc_Class_s >& listClasses)
{
    return mpConfigReader->getListElements(listClasses);
}
am_Error_e CAmPolicyEngine::getListDomains(std::vector<gc_Domain_s >& listDomains)
{
    return mpConfigReader->getListElements(listDomains);
}

} /* namespace gc */
} /* namespace am */
