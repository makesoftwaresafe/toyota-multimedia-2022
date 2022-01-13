/******************************************************************************
 * @file: CAmPolicyReceive.cpp
 *
 * This file contains the definition of policy engine receive class (member
 * functions and data members) used to provide the interface to policy engine to
 * get the context of controller as per current scenario
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
#include "CAmPolicyReceive.h"
#include "CAmPolicyAction.h"
#include "CAmLogger.h"
#include "CAmSinkElement.h"
#include "CAmSourceElement.h"
#include "CAmGatewayElement.h"
#include "IAmPolicySend.h"
#include "CAmRootAction.h"
#include "CAmMainConnectionElement.h"
#include "CAmClassElement.h"
#include "CAmDomainElement.h"

namespace am {
namespace gc {

CAmPolicyReceive::CAmPolicyReceive(CAmControlReceive* pControlReceive, IAmPolicySend* pPolicySend) :
                                mpControlReceive(pControlReceive),
                                mpPolicySend(pPolicySend)
{
}

bool CAmPolicyReceive::isDomainRegistrationComplete(const std::string& domainName)
{
    std::vector < am_Domain_s > listDomains;
    std::vector<am_Domain_s >::iterator itListDomains;
    if (E_OK == mpControlReceive->getListElements(listDomains))
    {
        for (itListDomains = listDomains.begin(); itListDomains != listDomains.end();
                        itListDomains++)
        {
            if ((*itListDomains).name == domainName)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"  domain found ", (*itListDomains).complete);
                return (*itListDomains).complete;
            }
        }
    }
    return false;
}

bool CAmPolicyReceive::isRegistered(const gc_Element_e type, const std::string& name)
{
    return (_getElement(type, name) != NULL) ? true : false;
}

am_Error_e CAmPolicyReceive::getAvailability(const gc_Element_e type, const std::string& name,
                                             am_Availability_s& availability)
{
    am_Error_e result(E_NOT_POSSIBLE);
    CAmElement* pElement = _getElement(type, name);
    if (pElement != NULL)
    {
        result = pElement->getAvailability(availability);
    }
    return result;
}

am_Error_e CAmPolicyReceive::getState(const gc_Element_e type, const std::string& name, int& state)
{
    am_Error_e result(E_NOT_POSSIBLE);
    CAmElement* pElement = _getElement(type, name);
    if (pElement != NULL)
    {
        result = pElement->getState(state);
    }
    return result;
}

am_Error_e CAmPolicyReceive::getInterruptState(const gc_Element_e type, const std::string& name,
                                               am_InterruptState_e& interruptState)
{
    am_Error_e result(E_NOT_POSSIBLE);
    CAmElement* pElement = _getElement(type, name);
    if (pElement != NULL)
    {
        result = pElement->getInterruptState(interruptState);
    }
    return result;
}

am_Error_e CAmPolicyReceive::getMuteState(const gc_Element_e type, const std::string& name,
                                          am_MuteState_e& muteState)
{
    am_Error_e result(E_NOT_POSSIBLE);
    CAmElement* pElement = _getElement(type, name);
    if (pElement != NULL)
    {
        muteState = pElement->getMuteState();
        result = E_OK;
    }
    return result;
}

am_Error_e CAmPolicyReceive::getSoundProperty(const gc_Element_e elementType,
                                              const std::string& elementName,
                                              const am_CustomSoundPropertyType_t propertyType,
                                              int16_t &propertyValue)
{
    am_Error_e result(E_NOT_POSSIBLE);
    if (elementType == ET_SOURCE)
    {
        CAmSourceElement* pElement = CAmSourceFactory::getElement(elementName);
        if (pElement != NULL)
        {
            result = pElement->getSoundPropertyValue(propertyType, propertyValue);
        }
    }
    else if (elementType == ET_SINK)
    {
        CAmSinkElement* pElement = CAmSinkFactory::getElement(elementName);
        if (pElement != NULL)
        {
            result = pElement->getSoundPropertyValue(propertyType, propertyValue);
        }
    }
    return result;
}

am_Error_e CAmPolicyReceive::getMainSoundProperty(const gc_Element_e elementType,
                                                  const std::string& elementName,
                                                  const am_CustomMainSoundPropertyType_t propertyType,
                                                  int16_t& propertyValue)
{
    am_Error_e result(E_NOT_POSSIBLE);
    if (elementType == ET_SOURCE)
    {
        CAmSourceElement* pElement = CAmSourceFactory::getElement(elementName);
        if (pElement != NULL)
        {
            result = pElement->getMainSoundPropertyValue(propertyType, propertyValue);
        }
    }
    else if (elementType == ET_SINK)
    {
        CAmSinkElement* pElement = CAmSinkFactory::getElement(elementName);
        if (pElement != NULL)
        {
            result = pElement->getMainSoundPropertyValue(propertyType, propertyValue);
        }
    }
    return result;
}

am_Error_e CAmPolicyReceive::getSystemProperty(const am_CustomSystemPropertyType_t systemPropertyType,
                                               int16_t& value)
{
    std::vector<am_SystemProperty_s> listSystemProperties;
    std::vector<am_SystemProperty_s>::iterator itListSystemProperties;
    //get the list of system properties from database
    mpControlReceive->getListSystemProperties(listSystemProperties);
    for (itListSystemProperties = listSystemProperties.begin();
                    itListSystemProperties != listSystemProperties.end(); itListSystemProperties++)
    {
        if ((*itListSystemProperties).type == systemPropertyType)
        {
            value = (*itListSystemProperties).value;
            return E_OK;
        }
    }
    return E_NOT_POSSIBLE;
}

am_Error_e CAmPolicyReceive::getVolume(const gc_Element_e elementType,
                                       const std::string& elementName,
                                       am_volume_t& volume)
{
    am_Error_e result(E_NOT_POSSIBLE);
    CAmElement* pElement = _getElement(elementType, elementName);
    if (pElement != NULL)
    {
        volume = pElement->getVolume();
        result = E_OK;
    }
    return result;
}

am_Error_e CAmPolicyReceive::getMainVolume(const gc_Element_e elementType,
                                           const std::string& elementName,
                                           am_mainVolume_t& mainVolume)
{
    am_Error_e result(E_NOT_POSSIBLE);
    if(ET_SOURCE != elementType)
    {
        CAmElement* pElement = _getElement(elementType, elementName);
        if (pElement != NULL)
        {
            mainVolume = pElement->getMainVolume();
            result = E_OK;
        }
    }
    return result;
}

bool CAmPolicyReceive::_sortingLowest(gc_ConnectionInfo_s i, gc_ConnectionInfo_s j)
{
    return (i.priority > j.priority);
}

bool CAmPolicyReceive::_sortingHighest(gc_ConnectionInfo_s i, gc_ConnectionInfo_s j)
{
    return (i.priority < j.priority);
}

am_Error_e CAmPolicyReceive::getListMainConnections(const std::string& name,
                                                    std::vector<gc_ConnectionInfo_s >& listConnectionInfos,gc_Order_e order)
{
    am_Error_e result = E_NOT_POSSIBLE;
    CAmClassElement* pClassElement;
    pClassElement = CAmClassFactory::getElement(name);
    if (NULL != pClassElement)
    {
        _getConnectionInfo(pClassElement, listConnectionInfos);
        switch(order)
        {
        case O_HIGH_PRIORITY:
            std::stable_sort(listConnectionInfos.begin(), listConnectionInfos.end(), _sortingHighest);
            result = E_OK;
            break;
        case O_LOW_PRIORITY:
            std::stable_sort(listConnectionInfos.begin(), listConnectionInfos.end(), _sortingLowest);
            result = E_OK;
            break;
        case O_NEWEST:
        {
            std::vector<gc_ConnectionInfo_s > listTempConnectionInfos;
            std::vector<gc_ConnectionInfo_s >::reverse_iterator itListRevTempConnectionInfos;
            listTempConnectionInfos = listConnectionInfos;
            listConnectionInfos.clear();
            for(itListRevTempConnectionInfos = listTempConnectionInfos.rbegin();itListRevTempConnectionInfos!= listTempConnectionInfos.rend();itListRevTempConnectionInfos++)
            {
                listConnectionInfos.push_back(*itListRevTempConnectionInfos);
            }
            result = E_OK;
        }
        break;
        case O_OLDEST:
            result = E_OK;
            break;
        default:
            break;
        }
    }
    return result;
}

am_Error_e CAmPolicyReceive::getListNotificationConfigurations(
                const gc_Element_e elementType, const std::string& name,
                std::vector<am_NotificationConfiguration_s >& listNotificationConfigurations)
{
    am_Error_e result(E_NOT_POSSIBLE);
    switch(elementType)
    {
    case ET_SOURCE:
    {
        CAmSourceElement* pElement = (CAmSourceElement*)_getElement(elementType, name);
        if (pElement != NULL)
        {
            result = pElement->getListNotificationConfigurations(listNotificationConfigurations);
        }
    }
    break;
    case ET_SINK:
    {
        CAmSinkElement* pElement = (CAmSinkElement*)_getElement(elementType, name);
        if (pElement != NULL)
        {
            result = pElement->getListNotificationConfigurations(listNotificationConfigurations);
        }
    }
    break;
    default:
        break;
    }
    return result;
}

am_Error_e CAmPolicyReceive::getListMainNotificationConfigurations(
                const gc_Element_e elementType, const std::string& name,
                std::vector<am_NotificationConfiguration_s >& listMainNotificationConfigurations)
{
    am_Error_e result(E_NOT_POSSIBLE);
    switch(elementType)
    {
    case ET_SOURCE:
    {
        CAmSourceElement* pElement = (CAmSourceElement*)_getElement(elementType, name);
        if (pElement != NULL)
        {
            result = pElement->getListMainNotificationConfigurations(listMainNotificationConfigurations);
        }
    }
    break;
    case ET_SINK:
    {
        CAmSinkElement* pElement = (CAmSinkElement*)_getElement(elementType, name);
        if (pElement != NULL)
        {
            result = pElement->getListMainNotificationConfigurations(listMainNotificationConfigurations);
        }
    }
    break;
    default:
        break;
    }
    return result;
}

am_Error_e CAmPolicyReceive::getListMainConnections(
                const gc_Element_e elementType, const std::string& elementName,
                std::vector<gc_ConnectionInfo_s >& listConnectionInfos)
{
    CAmClassElement* pClassElement;
    std::vector<CAmClassElement* > listClasses;
    std::vector<CAmClassElement* >::iterator itListClasses;
    am_Error_e result = E_NOT_POSSIBLE;
    CAmElement* pElement;

    LOG_FN_ENTRY(__FILENAME__,__func__,elementType);
    switch (elementType)
    {
    case ET_CLASS:
        pClassElement = CAmClassFactory::getElement(elementName);
        if (NULL != pClassElement)
        {
            _getConnectionInfo(pClassElement, listConnectionInfos);
            result = E_OK;
        }
        break;
    case ET_SOURCE:
    case ET_SINK:
        pElement = _getElement(elementType, elementName);
        if (NULL != pElement)
        {
            if (ET_SOURCE == elementType)
            {
                CAmClassFactory::getElementsBySource(pElement->getName(), listClasses);
            }
            else
            {
                CAmClassFactory::getElementsBySink(pElement->getName(), listClasses);
            }
            for (itListClasses = listClasses.begin(); itListClasses != listClasses.end();
                            itListClasses++)
            {
                _getConnectionInfo(*itListClasses, listConnectionInfos);
            }
            result = E_OK;
        }
        break;
    case ET_CONNECTION:
    {
        std::vector<CAmMainConnectionElement* > listConnections;
        std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
        pClassElement = CAmClassFactory::getElementByConnection(elementName);
        if (NULL != pClassElement)
        {
            pClassElement->getListMainConnections(listConnections);
            for (itListMainConnections = listConnections.begin();
                            itListMainConnections != listConnections.end(); itListMainConnections++)
            {
                //compare all the connection with given connection name
                if ((*itListMainConnections)->getName() == elementName)
                {
                    gc_ConnectionInfo_s connection((*itListMainConnections)->getMainSourceName(), (*itListMainConnections)->getMainSinkName());
                    (*itListMainConnections)->getPriority(connection.priority);
                    (*itListMainConnections)->getState((int &)(connection.connectionState));
                    connection.volume = (*itListMainConnections)->getVolume();
                    listConnectionInfos.push_back(connection);
                    result = E_OK;
                    break;
                }
            }
        }
        break;
    }
    default:
        LOG_FN_DEBUG(__FILENAME__,__func__,"  class not found with given element type: ", elementType);
        break;
    }
    LOG_FN_EXIT(__FILENAME__,__func__,result);
    return result;
}

void CAmPolicyReceive::_getConnectionInfo(CAmClassElement* pClass,
                                          std::vector<gc_ConnectionInfo_s >& listConnectionInfo)
{
    std::vector<CAmMainConnectionElement* > listConnections;
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    gc_ConnectionInfo_s connection;
    int32_t priority;
    int state;
    pClass->getListMainConnections(listConnections);
    for (itListMainConnections = listConnections.begin();
                    itListMainConnections != listConnections.end(); itListMainConnections++)
    {

        connection.sinkName = (*itListMainConnections)->getMainSinkName();
        connection.sourceName = (*itListMainConnections)->getMainSourceName();
        (*itListMainConnections)->getPriority(priority);
        connection.priority = priority;
        (*itListMainConnections)->getState(state);
        connection.connectionState = (am_ConnectionState_e)state;
        connection.volume = (*itListMainConnections)->getVolume();
        listConnectionInfo.push_back(connection);
    }
}

am_Error_e CAmPolicyReceive::setListActions(std::vector<gc_Action_s >& listActions,
                                            gc_ActionList_e actionListType)
{
    if (listActions.empty())
    {
        return E_NO_CHANGE;
    }
    CAmRootAction* pRootAction = CAmRootAction::getInstance();
    CAmPolicyAction* pPolicyAction = new CAmPolicyAction(listActions, mpPolicySend,
                                                         mpControlReceive);
    if (NULL == pPolicyAction)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  bad memory state");
        return E_NOT_POSSIBLE;
    }
//attach the dynamic action pointer to root
    pRootAction->append(pPolicyAction);
    return E_OK;
}

CAmElement* CAmPolicyReceive::_getElement(const gc_Element_e type, const std::string& name)
{
    CAmElement* pElement(NULL);
    if (type == ET_SINK)
    {
        pElement = CAmSinkFactory::getElement(name);
    }
    else if (type == ET_SOURCE)
    {
        pElement = CAmSourceFactory::getElement(name);
    }
    else if (type == ET_CLASS)
    {
        pElement = CAmClassFactory::getElement(name);
    }
    else if (type == ET_GATEWAY)
    {
        pElement = CAmGatewayFactory::getElement(name);
    }
    else if (type == ET_CONNECTION)
    {
        pElement = CAmMainConnectionFactory::getElement(name);
    }
    else if (type == ET_DOMAIN)
    {
        pElement = CAmDomainFactory::getElement(name);
    }
    return pElement;
}

}
/* namespace gc */
} /* namespace am */
