/******************************************************************************
 * @file: CAmConfigurationReader.cpp
 *
 * This file contains the definition of configuration reader class (member
 * functions  and data members) used as data container to store the information
 * related to configuration.
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

#include "CAmConfigurationReader.h"
#include "CAmLogger.h"
#include <algorithm>
namespace am {
namespace gc {
CAmConfigurationReader::CAmConfigurationReader()
{
    gc_Configuration_s configuration;
    CAmXmlConfigParser xmlConfigParser;
    am_Error_e err = xmlConfigParser.parse(&configuration);
    if (err == E_OK)
    {
        _prepareDomainMap(&configuration);
        _prepareSinkMap(&configuration);
        _prepareSourceMap(&configuration);
        _prepareGatewayMap(&configuration);
        _prepareClassMap(&configuration);
        mListPolicies = configuration.listPolicies;
        mListSystemProperties = configuration.listSystemProperties;
    }
}
void CAmConfigurationReader::getListSystemProperties(
                std::vector<am_SystemProperty_s >& listSystemProperties)
{
    listSystemProperties = mListSystemProperties;
}

am_Error_e CAmConfigurationReader::getListElements(std::vector<gc_Source_s >& listSources)
{
    _getListElements(listSources, mMapSources);
    return E_OK;
}

am_Error_e CAmConfigurationReader::getListElements(std::vector<gc_Sink_s >& listSinks)
{
    _getListElements(listSinks, mMapSinks);
    return E_OK;
}

am_Error_e CAmConfigurationReader::getListElements(std::vector<gc_Gateway_s >& listGateways)
{
    _getListElements(listGateways, mMapGateways);
    return E_OK;
}

am_Error_e CAmConfigurationReader::getListElements(std::vector<gc_Domain_s >& listDomains)
{
    _getListElements(listDomains, mMapDomains);
    return E_OK;
}

am_Error_e CAmConfigurationReader::getListElements(std::vector<gc_Class_s >& listClasses)
{
    _getListElements(listClasses, mMapClasses);
    return E_OK;
}

am_Error_e CAmConfigurationReader::getElementByName(const std::string& elementName,
                                                    gc_Gateway_s& gatewayInstance)
{
    return _getElementByName(elementName, gatewayInstance, mMapGateways);
}

am_Error_e CAmConfigurationReader::getElementByName(const std::string& elementName,
                                                    gc_Sink_s& sinkInstance)
{
    return _getElementByName(elementName, sinkInstance, mMapSinks);
}

am_Error_e CAmConfigurationReader::getElementByName(const std::string& elementName,
                                                    gc_Source_s& sourceInstance)
{
    return _getElementByName(elementName, sourceInstance, mMapSources);
}

am_Error_e CAmConfigurationReader::getElementByName(const std::string& elementName,
                                                    gc_Domain_s& domainInstance)
{
    return _getElementByName(elementName, domainInstance, mMapDomains);
}

am_Error_e CAmConfigurationReader::getElementByName(const std::string& elementName,
                                                    gc_Class_s& classInstance)
{
    return _getElementByName(elementName, classInstance, mMapClasses);
}

bool CAmConfigurationReader::_policysorting(gc_Process_s i, gc_Process_s j)
{
    return (i.priority < j.priority);
}

am_Error_e CAmConfigurationReader::getListProcess(const gc_Trigger_e trigger,
                                                  std::vector<gc_Process_s >& listProcesses)
{
    std::vector<gc_Policy_s >::iterator itListPolicies;
    std::vector<int >::iterator itListTriggers;
    listProcesses.clear();
    for (itListPolicies = mListPolicies.begin(); itListPolicies != mListPolicies.end();
                    ++itListPolicies)
    {
        for (itListTriggers = itListPolicies->listEvents.begin();
                        itListTriggers != itListPolicies->listEvents.end(); ++itListTriggers)
        {
            if ((*itListTriggers == trigger) || (*itListTriggers == USER_ALL_TRIGGER))
            {
                listProcesses.insert(listProcesses.end(), itListPolicies->listProcesses.begin(),
                                     itListPolicies->listProcesses.end());
            }
        }
    }
    // Now sort the policies in ascending order of priority
    std::stable_sort(listProcesses.begin(), listProcesses.end(), _policysorting);
    return E_OK;
}

template <typename Telement>
void CAmConfigurationReader::_prepareElementSet(gc_Configuration_s* pConfiguration,
                                                Telement& itListElement)
{
    gc_Domain_s domainInstance;
    std::vector<gc_SoundProperty_s >::iterator itListSoundProperties;
    std::vector<gc_MainSoundProperty_s >::iterator itListMainSoundProperties;
    am_SoundProperty_s soundProperty;
    am_MainSoundProperty_s mainSoundProperty;
    if (E_OK != _getElementByName(itListElement->domainName, domainInstance, mMapDomains))
    {
        return;
    }
    itListElement->domainID = domainInstance.domainID;
    for (itListSoundProperties = itListElement->listGCSoundProperties.begin();
                    itListSoundProperties != itListElement->listGCSoundProperties.end();
                    ++itListSoundProperties)
    {
        soundProperty.type = itListSoundProperties->type;
        soundProperty.value = itListSoundProperties->value;
        itListElement->listSoundProperties.push_back(soundProperty);
    }
    for (itListMainSoundProperties = itListElement->listGCMainSoundProperties.begin();
                    itListMainSoundProperties != itListElement->listGCMainSoundProperties.end();
                    ++itListMainSoundProperties)
    {
        mainSoundProperty.type = (am::am_CustomMainSoundPropertyType_t)itListMainSoundProperties->type;
        mainSoundProperty.value = itListMainSoundProperties->value;
        itListElement->listMainSoundProperties.push_back(mainSoundProperty);
    }
}

void CAmConfigurationReader::_prepareSinkMap(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Sink_s >::iterator itListSinks;
    for (itListSinks = pConfiguration->listSinks.begin();
                    itListSinks != pConfiguration->listSinks.end(); ++itListSinks)
    {
        if ((itListSinks->registrationType != REG_TEMPLATE) && (itListSinks->registrationType
                        != REG_NONE))
        {
            _prepareElementSet(pConfiguration, itListSinks);
            mMapSinks[itListSinks->name] = *itListSinks;
        }
    }
}

void CAmConfigurationReader::_prepareSourceMap(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Source_s >::iterator itListSources;
    for (itListSources = pConfiguration->listSources.begin();
                    itListSources != pConfiguration->listSources.end(); ++itListSources)
    {
        if ((itListSources->registrationType != REG_TEMPLATE) && (itListSources->registrationType
                        != REG_NONE))
        {
            _prepareElementSet(pConfiguration, itListSources);
            mMapSources[itListSources->name] = *itListSources;
        }
    }
}

void CAmConfigurationReader::_prepareGatewayMap(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Gateway_s >::iterator itListGateways;
    gc_Sink_s sinkInstance;
    gc_Source_s sourceInstance;
    gc_Domain_s domainInstance;
    for (itListGateways = pConfiguration->listGateways.begin();
                    itListGateways != pConfiguration->listGateways.end(); ++itListGateways)
    {
        if ((itListGateways->registrationType != REG_TEMPLATE) && (itListGateways->registrationType
                        != REG_NONE))
        {
            if (E_OK != _getElementByName(itListGateways->sourceName, sourceInstance, mMapSources))
            {
                continue;
            }
            itListGateways->sourceID = sourceInstance.sourceID;
            if (E_OK != _getElementByName(itListGateways->sinkName, sinkInstance, mMapSinks))
            {
                continue;
            }
            itListGateways->sinkID = sinkInstance.sinkID;
            if (E_OK != _getElementByName(itListGateways->controlDomainName, domainInstance,
                                          mMapDomains))
            {
                continue;
            }
            itListGateways->controlDomainID = domainInstance.domainID;
            mMapGateways[itListGateways->name] = (*itListGateways);
        }
    }
}

void CAmConfigurationReader::_prepareDomainMap(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Domain_s >::iterator itListDomains;
    for (itListDomains = pConfiguration->listDomains.begin();
                    itListDomains != pConfiguration->listDomains.end(); ++itListDomains)
    {
        if ((itListDomains->registrationType != REG_TEMPLATE) && (itListDomains->registrationType
                        != REG_NONE))
        {
            mMapDomains[itListDomains->name] = *itListDomains;
        }
    }

}
void CAmConfigurationReader::_prepareClassMap(gc_Configuration_s* pConfiguration)
{
    std::vector<gc_Class_s >::iterator itListClasses;
    for (itListClasses = pConfiguration->listClasses.begin();
                    itListClasses != pConfiguration->listClasses.end(); ++itListClasses)
    {
        if ((itListClasses->registrationType != REG_TEMPLATE) && (itListClasses->registrationType
                        != REG_NONE))
        {
            mMapClasses[itListClasses->name] = *itListClasses;
        }
    }
}

template <typename Telement>
am_Error_e CAmConfigurationReader::_getListElements(std::vector<Telement >& listElements,
                                                    std::map<std::string, Telement >& mapElements)
{
    typename std::map<std::string, Telement >::iterator itElement;
    listElements.clear();
    for (itElement = mapElements.begin(); itElement != mapElements.end(); ++itElement)
    {
        listElements.push_back(itElement->second);
    }
    return E_OK;
}

template <typename Telement>
am_Error_e CAmConfigurationReader::_getElementByName(const std::string& elementName,
                                                     Telement& elementInstance,
                                                     std::map<std::string, Telement >& mapElements)
{
    am_Error_e result = E_UNKNOWN;
    typename std::map<std::string, Telement >::iterator itMapElements;
    itMapElements = mapElements.find(elementName);
    if (itMapElements != mapElements.end())
    {
        elementInstance = itMapElements->second;
        result = E_OK;
    }
    return result;
}

}
}
