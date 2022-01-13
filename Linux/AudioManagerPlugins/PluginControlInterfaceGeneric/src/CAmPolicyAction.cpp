/******************************************************************************
 * @file: CAmPolicyAction.cpp
 *
 * This file contains the definition of policy action class
 * (member functions and data members)
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

#include <string.h>
#include "IAmAction.h"
#include  "CAmControlReceive.h"
#include "IAmPolicySend.h"
#include "CAmPolicyAction.h"

#include "CAmClassActionSetVolume.h"
#include "CAmClassActionConnect.h"
#include "CAmClassActionDisconnect.h"
#include "CAmClassActionSetVolume.h"
#include "CAmClassActionInterrupt.h"
#include "CAmClassActionSuspend.h"
#include "CAmSourceSinkActionSetSoundProperty.h"
#include "CAmSourceSinkActionSetNotificationConfiguration.h"
#include "CAmSystemActionRegister.h"
#include "CAmSystemActionSetProperty.h"
#include "CAmClassElement.h"
#include "CAmLogger.h"
#include "CAmSourceElement.h"
#include "CAmSinkElement.h"

namespace am {
namespace gc {

#define isParameterSet(mapParams,paramName) (1 == mapParams.count(paramName))

#define checkAndSetStringParam(mapParams , paramName, pAction) {\
    if (isParameterSet(mapParams ,paramName ))\
    {\
        CAmActionParam<std::string> actionParam(mapParams[paramName]);\
        pAction->setParam(paramName, &actionParam);\
    }\
}

#define checkAndSetNumericParam(mapParams , paramName, paramType,pAction) {\
    if (isParameterSet(mapParams ,paramName))\
    {\
        CAmActionParam < paramType > actionParam((paramType)atoi(mapParams[paramName].data()));\
        pAction->setParam(paramName, &actionParam);\
    }\
}

CAmPolicyAction::CAmPolicyAction(const std::vector<gc_Action_s > &listPolicyActions,
                                 IAmPolicySend* pPolicySend, CAmControlReceive* pControlReceive) :
                                CAmActionContainer(std::string("CAmPolicyAction")),
                                mListActions(listPolicyActions),
                                mpPolicySend(pPolicySend),
                                mpControlReceive(pControlReceive)
{
    mMapConnectionStates["ALL"] =
    {   CS_UNKNOWN, CS_CONNECTING, CS_CONNECTED, CS_DISCONNECTING, CS_DISCONNECTED, CS_SUSPENDED};
    mMapConnectionStates["CS_CONNECTED"] =
    {   CS_CONNECTED};
    mMapConnectionStates[""] =
    {   CS_CONNECTED};
    mMapConnectionStates["CS_DISCONNECTED"] =
    {   CS_DISCONNECTED};
    mMapConnectionStates["CS_SUSPENDED"] =
    {   CS_SUSPENDED};
    mMapConnectionStates["OTHERS"] =
    {   CS_UNKNOWN, CS_CONNECTING, CS_DISCONNECTING,
        CS_DISCONNECTED, CS_SUSPENDED};
}

CAmPolicyAction::~CAmPolicyAction()
{
}

int CAmPolicyAction::_execute(void)
{
    std::vector<gc_Action_s >::iterator itListPolicyActions;
    std::vector<IAmActionCommand * > listFrameworkActions;
    std::vector<IAmActionCommand * >::iterator itListFrameworkActions;
    // for each policy action get the list of framework actions and appens
    // gc_Action_s = policy action
    // IAmActionCommand = framework action
    for (itListPolicyActions = mListActions.begin(); itListPolicyActions != mListActions.end();
                    ++itListPolicyActions)
    {
        listFrameworkActions.clear();
        _createActions(*itListPolicyActions, listFrameworkActions);
        for (itListFrameworkActions = listFrameworkActions.begin();
                        itListFrameworkActions != listFrameworkActions.end();
                        itListFrameworkActions++)
        {
            append(*itListFrameworkActions);
        }
    }
    return E_OK;
}

int CAmPolicyAction::_update(const int Result)
{
    // TODO : unclear what to do here ??
    return E_OK;
}

// helper functions to create action and populate action parameters
std::string CAmPolicyAction::_getParam(std::map<std::string, std::string > &map,
                                       const std::string &paramName)
{
    std::string parameter;
    if (map.find(paramName) != map.end())
    {
        parameter = map[paramName];
    }
    return parameter;
}

am_Error_e CAmPolicyAction::_getListNames(std::string name, std::vector<std::string >& listNames)
{
    listNames.clear();
    std::string temp;
    std::stringstream ss(name);
    while (ss >> temp)
    {
        listNames.push_back(temp);
    }
    return E_OK;
}

void CAmPolicyAction::_setActionParameters(std::map<std::string, std::string >& mapParams,
                                           IAmActionCommand *pAction)
{
    if (isParameterSet(mapParams, ACTION_PARAM_SOURCE_NAME))
    {
        std::string sourceName = mapParams[ACTION_PARAM_SOURCE_NAME];
        /*
         * If no whitespace is found then its a single name
         */
        if (sourceName.find(" ") == std::string::npos)
        {
            CAmActionParam < std::string > sourceNameParam(sourceName);
            pAction->setParam(ACTION_PARAM_SOURCE_NAME, &sourceNameParam);
        }
        else
        {
            std::vector < gc_Source_s > listSources;
            std::vector < std::string > listNames;
            /*
             * Find the list of sources based on the whitespace separated names
             */
            _getListNames(sourceName, listNames);
            mpPolicySend->getListElements(listNames, listSources);
            CAmActionParam < std::vector<gc_Source_s > > listSourcesParameter(listSources);
            pAction->setParam(ACTION_PARAM_SOURCE_INFO, &listSourcesParameter);
        }
    }
    if (isParameterSet(mapParams, ACTION_PARAM_SINK_NAME))
    {
        std::string sinkName = mapParams[ACTION_PARAM_SINK_NAME];
        /*
         * If no whitespace is found then its a single name
         */
        if (sinkName.find(" ") == std::string::npos)
        {
            CAmActionParam < std::string > sinkNameParam(sinkName);
            pAction->setParam(ACTION_PARAM_SINK_NAME, &sinkNameParam);
        }
        else
        {
            std::vector < gc_Sink_s > listSinks;
            std::vector < std::string > listNames;
            /*
             * Find the list of sources based on the whitespace separated names
             */
            _getListNames(sinkName, listNames);
            mpPolicySend->getListElements(listNames, listSinks);
            CAmActionParam < std::vector<gc_Sink_s > > listSinksParameter(listSinks);
            pAction->setParam(ACTION_PARAM_SINK_INFO, &listSinksParameter);
        }
    }
    if (isParameterSet(mapParams, ACTION_PARAM_GATEWAY_NAME))
    {
        std::string gatewayName = mapParams[ACTION_PARAM_GATEWAY_NAME];
        /*
         * If no whitespace is found then its a single name
         */
        if (gatewayName.find(" ") == std::string::npos)
        {
            CAmActionParam < std::string > sinkNameParam(gatewayName);
            pAction->setParam(ACTION_PARAM_GATEWAY_NAME, &sinkNameParam);
        }
        else
        {
            std::vector < gc_Gateway_s > listGateways;
            std::vector < std::string > listNames;
            /*
             * Find the list of sources based on the whitespace separated names
             */
            _getListNames(gatewayName, listNames);
            mpPolicySend->getListElements(listNames, listGateways);
            CAmActionParam < std::vector<gc_Gateway_s > > listGatewaysParameter(listGateways);
            pAction->setParam(ACTION_PARAM_GATEWAY_INFO, &listGatewaysParameter);
        }
    }
    if (isParameterSet(mapParams, ACTION_PARAM_EXCEPT_SOURCE_NAME))
    {
        std::vector < std::string > listNames;
        _getListNames(mapParams[ACTION_PARAM_EXCEPT_SOURCE_NAME], listNames);
        CAmActionParam < std::vector<std::string > > listExceptSourcesParameter(listNames);
        pAction->setParam(ACTION_PARAM_EXCEPT_SOURCE_NAME, &listExceptSourcesParameter);
    }
    if (isParameterSet(mapParams, ACTION_PARAM_EXCEPT_SINK_NAME))
    {
        std::vector < std::string > listNames;
        _getListNames(mapParams[ACTION_PARAM_EXCEPT_SINK_NAME], listNames);
        CAmActionParam < std::vector<std::string > > listExceptSinkParameter(listNames);
        pAction->setParam(ACTION_PARAM_EXCEPT_SINK_NAME, &listExceptSinkParameter);
    }
    if (isParameterSet(mapParams,ACTION_PARAM_CONNECTION_STATE))
    {
        /*
         * convert the connection state filter from string to list connection states
         * here
         */
        std::string filter(mapParams[ACTION_PARAM_CONNECTION_STATE]);

        if ((true == isParameterSet(mapParams,ACTION_PARAM_SOURCE_NAME))&&
            (true == isParameterSet(mapParams,ACTION_PARAM_SINK_NAME)))
        {
            filter = "ALL";
        }
        CAmActionParam < std::vector<am_ConnectionState_e > > listConnectionStatesParam(
                        mMapConnectionStates[filter]);
        pAction->setParam(ACTION_PARAM_CONNECTION_STATE, &listConnectionStatesParam);
    }
    else
    {
        if ((true == isParameterSet(mapParams,ACTION_PARAM_SINK_NAME))&&
            (true == isParameterSet(mapParams,ACTION_PARAM_SOURCE_NAME)))
        {
            CAmActionParam < std::vector<am_ConnectionState_e > > listConnectionStatesParam(
                            mMapConnectionStates["ALL"]);
            pAction->setParam(ACTION_PARAM_CONNECTION_STATE, &listConnectionStatesParam);
        }
    }
    if (isParameterSet(mapParams, ACTION_PARAM_TIMEOUT))
    {
        pAction->setTimeout(atoi(mapParams[ACTION_PARAM_TIMEOUT].data()));
    }
    if (isParameterSet(mapParams, ACTION_PARAM_MUTE_STATE))
    {
        am_MuteState_e muteState;
        muteState = (am_MuteState_e)atoi(mapParams[ACTION_PARAM_MUTE_STATE].c_str());
        CAmActionParam < gc_LimitState_e > limitStateParam;
        CAmActionParam < am_volume_t > targetVolumeParam;
        CAmActionParam < uint32_t > patternParam;
        LOG_FN_INFO(__FILENAME__,__func__,"MUTESTATE=", muteState);
        if (muteState == MS_MUTED)
        {
            limitStateParam.setParam(LS_LIMITED);
            targetVolumeParam.setParam(AM_MUTE);
        }
        else
        {
            limitStateParam.setParam(LS_UNLIMITED);
            targetVolumeParam.setParam(0);
        }
        patternParam.setParam(0xFFFF0000);
        pAction->setParam(ACTION_PARAM_LIMIT_STATE, &limitStateParam);
        pAction->setParam(ACTION_PARAM_VOLUME, &targetVolumeParam);
        pAction->setParam(ACTION_PARAM_PATTERN, &patternParam);
    }
    if (isParameterSet(mapParams, ACTION_PARAM_LIMIT_STATE))
    {
        CAmActionParam < uint32_t > patternParam;
        CAmActionParam < gc_LimitState_e > volumeOperationParam(
                        (gc_LimitState_e)atoi(mapParams[ACTION_PARAM_LIMIT_STATE].c_str()));
        patternParam.setParam(0xFFFF);
        pAction->setParam(ACTION_PARAM_LIMIT_STATE, &volumeOperationParam);
        pAction->setParam(ACTION_PARAM_PATTERN, &patternParam);
    }
    if (isParameterSet(mapParams, ACTION_PARAM_PATTERN))
    {
        CAmActionParam < uint32_t > pattern(
                        strtol(mapParams[ACTION_PARAM_PATTERN].data(), NULL, 16));
        pAction->setParam(ACTION_PARAM_PATTERN, &pattern);
    }
    checkAndSetStringParam(mapParams, ACTION_PARAM_CLASS_NAME, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_RAMP_TYPE, am_CustomRampType_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_RAMP_TIME, am_time_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_MAIN_VOLUME, am_mainVolume_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_MAIN_VOLUME_STEP, am_mainVolume_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_VOLUME, am_volume_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_VOLUME_STEP, am_volume_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_ORDER, gc_Order_e, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_PROPERTY_TYPE, am_CustomSoundPropertyType_t,
                            pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_PROPERTY_VALUE, int16_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_DEBUG_TYPE, uint16_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_DEBUG_VALUE, int16_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_CONNECTION_FORMAT, am_CustomConnectionFormat_t,
                            pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_NOTIFICATION_CONFIGURATION_TYPE,
                            am_CustomNotificationType_t, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_NOTIFICATION_CONFIGURATION_STATUS,
                            am_NotificationStatus_e, pAction);
    checkAndSetNumericParam(mapParams, ACTION_PARAM_NOTIFICATION_CONFIGURATION_PARAM, int16_t,
                            pAction);

}

void CAmPolicyAction::_getClassList(gc_Action_s& policyAction,
                                    std::vector<CAmClassElement * >& listClasses)
{
    std::string sourceName = _getParam(policyAction.mapParameters, ACTION_PARAM_SOURCE_NAME);
    std::string sinkName = _getParam(policyAction.mapParameters, ACTION_PARAM_SINK_NAME);
    std::string className = _getParam(policyAction.mapParameters, ACTION_PARAM_CLASS_NAME);
    std::vector < std::string > listClassNames;
    std::vector<std::string >::iterator itListClassNames;
    std::vector<CAmClassElement * > listTempClasses;
    std::vector<CAmClassElement * >::iterator itlistTempClasses;
    if (isParameterSet(policyAction.mapParameters, ACTION_PARAM_EXCEPT_CLASS_NAME))
    {
        _getListNames(policyAction.mapParameters[ACTION_PARAM_EXCEPT_CLASS_NAME], listClassNames);
    }
    if (((true == sourceName.empty()) && (true == sinkName.empty()) && (true == className.empty())) || ("ALL"
                    == className))
    {
        CAmClassFactory::getListElements(listClasses);
    }
    else if ((false == sourceName.empty()) && (false == sinkName.empty())) // the source and sink is specified explicitly
    {
        CAmClassElement *pClassElement = (CAmClassElement*)CAmClassFactory::getElement(sourceName,
                                                                                       sinkName);
        if (NULL != pClassElement)
        {
            listClasses.push_back(pClassElement);
        }
    }
    else if (false == className.empty()) // both source and sink information are not available but class name is specified explicitly
    {
        CAmClassElement *pClassElement = (CAmClassElement*)CAmClassFactory::getElement(className);
        if (NULL != pClassElement)
        {
            listClasses.push_back(pClassElement);
        }
    }
    else // find the class which contains given source or sink
    {
        if (true == sinkName.empty())
        {
            CAmClassFactory::getElementsBySource(sourceName, listClasses);
        }
        else if (true == sourceName.empty())
        {
            CAmClassFactory::getElementsBySink(sinkName, listClasses);
        }
    }
    listTempClasses = listClasses;
    listClasses.clear();
    for (itlistTempClasses = listTempClasses.begin(); itlistTempClasses != listTempClasses.end();
                    itlistTempClasses++)
    {
        for (itListClassNames = listClassNames.begin(); itListClassNames != listClassNames.end();
                        ++itListClassNames)
        {
            if (*itListClassNames == (*itlistTempClasses)->getName())
            {
                break;
            }
        }
        if (itListClassNames == listClassNames.end())
        {
            listClasses.push_back(*itlistTempClasses);
        }
    }

}

void CAmPolicyAction::_getActions(gc_Action_s& policyAction,
                                  std::vector<IAmActionCommand * >& listFrameworkActions)
{
    std::vector<CAmClassElement* > listClasses;
    _getClassList(policyAction, listClasses);
    if (false == listClasses.empty())
    {
        IAmActionCommand *pAction = NULL;
        for (std::vector<CAmClassElement* >::iterator itListClasses = listClasses.begin();
                        itListClasses != listClasses.end(); itListClasses++)
        {
            if (policyAction.actionName == ACTION_NAME_CONNECT)
            {
                pAction = new CAmClassActionConnect(*itListClasses);
            }
            else if (policyAction.actionName == ACTION_NAME_SET_VOLUME)
            {
                pAction = new CAmClassActionSetVolume(*itListClasses);
            }
            else if (policyAction.actionName == ACTION_NAME_INTERRUPT)
            {
                pAction = new CAmClassActionInterrupt(*itListClasses);
            }
            else if (policyAction.actionName == ACTION_NAME_SUSPEND)
            {
                pAction = new CAmClassActionSuspend(*itListClasses);
            }
            else if (policyAction.actionName == ACTION_NAME_DISCONNECT)
            {
                pAction = new CAmClassActionDisconnect(*itListClasses);
            }
            else if ((policyAction.actionName == ACTION_NAME_LIMIT) || (policyAction.actionName
                            == ACTION_NAME_MUTE))
            {
                pAction = new CAmClassActionSetVolume(*itListClasses);
            }
            else
            {
                pAction = NULL;
            }
            if (NULL != pAction)
            {
                _setActionParameters(policyAction.mapParameters, pAction);
                listFrameworkActions.push_back(pAction);
            }
        }
    }
    else
    {
        LOG_FN_WARN(__FILENAME__,__func__,"Failed to get class for action",policyAction.actionName);
    }
}

void CAmPolicyAction::_createActions(gc_Action_s& policyAction,
                                     std::vector<IAmActionCommand * >& listFrameworkActions)
{
    IAmActionCommand *pFrameworkAction = NULL;

    // instantiate the action
    if ((policyAction.actionName == ACTION_NAME_CONNECT) || (policyAction.actionName
                    == ACTION_NAME_DISCONNECT)
        || (policyAction.actionName == ACTION_NAME_SET_VOLUME)
        || (policyAction.actionName == ACTION_NAME_LIMIT)
        || (policyAction.actionName == ACTION_NAME_INTERRUPT)
        || (policyAction.actionName == ACTION_NAME_SUSPEND)
        || (policyAction.actionName == ACTION_NAME_MUTE))
    {
        _getActions(policyAction, listFrameworkActions);
    }
    else if (policyAction.actionName == ACTION_NAME_REGISTER)
    {
        pFrameworkAction = new CAmSystemActionRegister(mpControlReceive);
    }
    else if (policyAction.actionName == ACTION_DEBUG)
    {
        CAmSystemElement* pSystem = CAmSystemFactory::getElement(SYSTEM_ELEMENT_NAME);
        pFrameworkAction = new CAmSystemActionSetProperty(pSystem,mpControlReceive);
    }
    else
    {
        if ((policyAction.actionName == ACTION_NAME_SET_PROPERTY) || (policyAction.actionName
                        == ACTION_NAME_SET_NOTIFICATION_CONFIGURATION))
        {
            std::string sourceName = _getParam(policyAction.mapParameters,
                                               ACTION_PARAM_SOURCE_NAME);
            std::string sinkName = _getParam(policyAction.mapParameters, ACTION_PARAM_SINK_NAME);
            if (false == sourceName.empty())
            {
                CAmSourceElement* pSourceElement = CAmSourceFactory::getElement(sourceName);
                if (pSourceElement != NULL)
                {
                    if (policyAction.actionName == ACTION_NAME_SET_PROPERTY)
                    {
                        pFrameworkAction = new CAmSourceSinkActionSetSoundProperty<CAmSourceElement >(
                                        pSourceElement);
                    }
                    else
                    {
                        pFrameworkAction = new CAmSourceSinkActionSetNotificationConfiguration<
                                        CAmSourceElement >(pSourceElement);
                    }
                }
            }
            else if (false == sinkName.empty())
            {
                CAmSinkElement* pSinkElement = CAmSinkFactory::getElement(sinkName);
                if (pSinkElement != NULL)
                {
                    if (policyAction.actionName == ACTION_NAME_SET_PROPERTY)
                    {
                        pFrameworkAction = new CAmSourceSinkActionSetSoundProperty<CAmSinkElement >(
                                        pSinkElement);
                    }
                    else
                    {
                        pFrameworkAction = new CAmSourceSinkActionSetNotificationConfiguration<
                                        CAmSinkElement >(pSinkElement);
                    }
                }
            }
        }
    }
    if (NULL != pFrameworkAction)
    {
        _setActionParameters(policyAction.mapParameters, pFrameworkAction);
        listFrameworkActions.push_back(pFrameworkAction);
    }
}

}
}
