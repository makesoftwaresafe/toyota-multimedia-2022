/******************************************************************************
 * @file: CAmSystemActionRegister.cpp
 *
 * This file contains the definition of system action register class (member
 * functions and data members) used to implement the logic of registering the
 * static source/sink/gateway as defined in configuration.
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

#include "CAmSystemActionRegister.h"
#include "CAmControlReceive.h"
#include "CAmSourceElement.h"
#include "CAmSinkElement.h"
#include "CAmGatewayElement.h"
#include "CAmClassElement.h"

#include "limits.h"
#include <algorithm>
namespace am {
namespace gc {

CAmSystemActionRegister::CAmSystemActionRegister(CAmControlReceive* pControlReceive) :
                                CAmActionCommand(std::string("CAmSystemActionRegister")),
                                mpControlReceive(pControlReceive)

{
    _registerParam(ACTION_PARAM_SOURCE_INFO, &mListSources);
    _registerParam(ACTION_PARAM_SINK_INFO, &mListSinks);
    _registerParam(ACTION_PARAM_GATEWAY_INFO, &mListGateways);
}
CAmSystemActionRegister::~CAmSystemActionRegister(void)
{
}

int CAmSystemActionRegister::_execute(void)
{
	am_Error_e result = E_NOT_POSSIBLE;
    std::vector<gc_Sink_s > listSinks;
    std::vector<gc_Sink_s >::iterator itListSinks;
    std::vector<gc_Source_s > listSources;
    std::vector<gc_Source_s >::iterator itListSources;
    std::vector<gc_Gateway_s > listGateways;
    std::vector<gc_Gateway_s >::iterator itListGateways;
    am_domainID_t domainID;
    std::map<std::string, am_domainID_t > mapDomainIDs;
    std::map<std::string, am_domainID_t >::iterator itmapDomainIDs;
    mListSinks.getParam(listSinks);
    mListSources.getParam(listSources);
    mListGateways.getParam(listGateways);
    CAmClassElement *klass;

    for (itListSinks = listSinks.begin(); itListSinks != listSinks.end(); ++itListSinks)
    {
        klass = CAmClassFactory::getElement((*itListSinks).className);

        if (klass) {
          (*itListSinks).sinkClassID = klass->getID();
        }

        itmapDomainIDs = mapDomainIDs.find((*itListSinks).domainName);
        if (itmapDomainIDs != mapDomainIDs.end())
        {
            (*itListSinks).domainID = itmapDomainIDs->second;
        }
        else
        {
            if (E_OK == _getDomainID((*itListSinks).domainName, (*itListSinks).domainID))
            {
                mapDomainIDs[(*itListSinks).domainName] = (*itListSinks).domainID;
            }
            else
            {
                return E_NOT_POSSIBLE;
            }
        }
        {
            std::vector <CAmClassElement*>::iterator itListClassElements;
            std::vector <CAmClassElement*> listClassElements;
            CAmClassFactory::getListElements(listClassElements);
            itListClassElements = listClassElements.begin();
            for(;itListClassElements!=listClassElements.end();++itListClassElements)
            {
                am_mainVolume_t mainVolume = (*itListSinks).mainVolume;
                if((true == (*itListClassElements)->isPerSinkClassVolumeEnabled()) &&
                   (E_OK == (*itListClassElements)->getLastVolume((*itListSinks).name, mainVolume)))
                {
                    gc_MainSoundProperty_s mainSoundProperty;
                    mainSoundProperty.type = MSP_SINK_PER_CLASS_VOLUME_TYPE((*itListClassElements)->getID());
                    mainSoundProperty.value = mainVolume;
                    mainSoundProperty.minValue = SHRT_MIN;
                    mainSoundProperty.maxValue = SHRT_MAX;
                    (*itListSinks).listGCMainSoundProperties.push_back(mainSoundProperty);
                    (*itListSinks).listMainSoundProperties.push_back(mainSoundProperty);
                }
            }
        }

        CAmElement* pElement;
        pElement = CAmSinkFactory::createElement((*itListSinks), mpControlReceive);
        if (NULL != pElement)
        {
        	CAmElement* pClassElement = NULL;
        	pClassElement = CAmClassFactory::getElement((*itListSinks).className);
        	if(pClassElement != NULL)
        	{
        		result = pClassElement->attach(pElement);
        		if(result != E_OK)
        		{
        			/*its an error need to decide */
        			LOG_FN_ERROR("element attach failed, result is:",result);

        		}
        	}
        	else
        	{
        		LOG_FN_INFO(" Not able to get class element", (*itListSinks).className);
        		result = E_NOT_POSSIBLE;
        	}
        }
        else
        {
        	LOG_FN_INFO(" Not able to create sink element");
        	result = E_NOT_POSSIBLE;
        }
    }
    for (itListSources = listSources.begin(); itListSources != listSources.end(); ++itListSources)
    {
        klass = CAmClassFactory::getElement((*itListSources).className);

        if (klass) {
          (*itListSources).sourceClassID = klass->getID();
        }

        itmapDomainIDs = mapDomainIDs.find((*itListSources).domainName);
        if (itmapDomainIDs != mapDomainIDs.end())
        {
            (*itListSources).domainID = itmapDomainIDs->second;
        }
        else
        {
            if (E_OK == _getDomainID((*itListSources).domainName, (*itListSources).domainID))
            {
                mapDomainIDs[(*itListSources).domainName] = (*itListSources).domainID;
            }
            else
            {
                return E_NOT_POSSIBLE;
            }
        }
        CAmElement* pElement;
        pElement = CAmSourceFactory::createElement((*itListSources), mpControlReceive);
        if (NULL != pElement)
        {
        	CAmElement* pClassElement = NULL;
        	pClassElement = CAmClassFactory::getElement((*itListSources).className);
        	if(pClassElement != NULL)
        	{
        		result = pClassElement->attach(pElement);
        		if(result != E_OK)
        		{
        			/*its an error need to decide */
        			LOG_FN_ERROR("element attach failed, result is:",result);
        		}
        	}
        	else
        	{
        		LOG_FN_INFO(" Not able to get class element", (*itListSources).className);
        		result = E_NOT_POSSIBLE;
        	}
        }
        else
        {
        	LOG_FN_INFO(" Not able to create sink element");
        	result = E_NOT_POSSIBLE;
        }
    }
    for (itListGateways = listGateways.begin(); itListGateways != listGateways.end();
                    ++itListGateways)
    {
        LOG_FN_INFO(__FILENAME__,__func__,"Registering gateway");
        CAmElement* pElement;
        if (E_OK == _populateGatewayStruct(*itListGateways))
        {
        	pElement = CAmGatewayFactory::createElement(*itListGateways, mpControlReceive);

        	if (NULL != pElement)
        	{
        		CAmElement* pSourceElement = NULL;
        		pSourceElement = CAmSourceFactory::getElement(((gc_Gateway_s)*itListGateways).sourceName);
        		if(pSourceElement!= NULL)
        		{
        			result = pElement->attach(pSourceElement);
        			if(result != E_OK)
        			{
        				/*its an error need to decide */
        				LOG_FN_ERROR("element attach failed, result is:",result);
        			}
        		}
        		else
        		{
        			LOG_FN_INFO(" Not able to get source element",((gc_Gateway_s)*itListGateways).sourceName);
        			result = E_NOT_POSSIBLE;
        		}

        		CAmElement* pSinkElement = NULL;
        		pSinkElement = CAmSinkFactory::getElement(((gc_Gateway_s)*itListGateways).sinkName);
        		if(pSinkElement!= NULL)
        		{
        			result = pElement->attach(pSinkElement);
        			if(result != E_OK)
        			{
        				/*its an error need to decide */
        				LOG_FN_ERROR("element attach failed, result is:",result);
        			}
        		}
        		else
        		{
        			LOG_FN_INFO(" Not able to get sink element", ((gc_Gateway_s)*itListGateways).sinkName);
        			result = E_NOT_POSSIBLE;
        		}
        	}
        	else
        	{
        		LOG_FN_INFO(" Not able to create gateway element");
        		result = E_NOT_POSSIBLE;
        	}
        }
        else
        {
            return E_NOT_POSSIBLE;
        }

    }
    return E_OK;
}

int CAmSystemActionRegister::_populateGatewayStruct(gc_Gateway_s& gatewayData)
{
    CAmSourceElement* pSource;
    CAmSinkElement* pSink;
    am_Error_e result = E_OK;
    bool DomainRegistered = false;

    do
    {
        if (E_OK == _getDomainID(gatewayData.controlDomainName, gatewayData.controlDomainID))
        {
            DomainRegistered = true;
        }
        pSource = CAmSourceFactory::getElement(gatewayData.sourceName);
        pSink = CAmSinkFactory::getElement(gatewayData.sinkName);
        if (!(DomainRegistered && (pSource != NULL) && (pSink != NULL)))
        {
            result = E_NOT_POSSIBLE;
            break;
        }
        gatewayData.sinkID = pSink->getID();
        gatewayData.domainSinkID = pSink->getDomainID();
        pSink->getListConnectionFormats(gatewayData.listSinkFormats);
        gatewayData.sourceID = pSource->getID();
        gatewayData.domainSourceID = pSource->getDomainID();
        pSource->getListConnectionFormats(gatewayData.listSourceFormats);
        gatewayData.convertionMatrix.clear();
        unsigned int SinkCounter, SourceCounter;
        std::pair<am_CustomConnectionFormat_t, am_CustomConnectionFormat_t > key;
        std::vector<std::pair<am_CustomConnectionFormat_t, am_CustomConnectionFormat_t > >::iterator itListConversionMatrix;
        for (SinkCounter = 0; SinkCounter < gatewayData.listSinkFormats.size(); ++SinkCounter)
        {
            for (SourceCounter = 0; SourceCounter < gatewayData.listSourceFormats.size();
                            ++SourceCounter)
            {
                key.first = gatewayData.listSinkFormats[SinkCounter];
                key.second = gatewayData.listSourceFormats[SourceCounter];
                itListConversionMatrix = std::find(gatewayData.listConvertionmatrix.begin(),
                                                   gatewayData.listConvertionmatrix.end(), key);
                if (itListConversionMatrix != gatewayData.listConvertionmatrix.end())
                {
                    gatewayData.convertionMatrix.push_back(true);
                }
                else
                {
                    gatewayData.convertionMatrix.push_back(false);
                }
            }
        }
    } while (0);
    return result;
}

am_Error_e CAmSystemActionRegister::_getDomainID(const std::string& domainName,
                                                 am_domainID_t& domainID)
{
    std::vector<am_Domain_s > listDomains;
    std::vector<am_Domain_s >::iterator itListDomains;
    if (E_OK == mpControlReceive->getListElements(listDomains))
    {
        for (itListDomains = listDomains.begin(); itListDomains != listDomains.end();
                        ++itListDomains)
        {
            if (itListDomains->name == domainName)
            {
                domainID = itListDomains->domainID;
                return E_OK;
            }
        }
    }
    return E_UNKNOWN;
}

} /* namespace gc */
} /* namespace am */
