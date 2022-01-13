/******************************************************************************
 * @file: CAmMainConnectionElement.cpp
 *
 * This file contains the definition of main connection class (member functions
 * and data members) used as data container to store the information related to
 * main connection as maintained by controller.
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

#include "CAmMainConnectionElement.h"
#include "CAmControlReceive.h"
#include "CAmSinkElement.h"
#include "CAmSourceElement.h"
#include "CAmLogger.h"
#include "CAmTriggerQueue.h"
#include <algorithm>
namespace am {
namespace gc {

CAmMainConnectionElement::CAmMainConnectionElement(const gc_Route_s& route,
                                                   CAmControlReceive* pControlReceive) :
                                CAmElement(route.name, pControlReceive),
                                mpControlReceive(pControlReceive),
                                mRoute(route)
{
    setType (ET_CONNECTION);
}

CAmMainConnectionElement::~CAmMainConnectionElement()
{
    LOG_FN_ENTRY(__FILENAME__,__func__);
    std::vector<CAmElement* > listOfSubjects;
    std::vector<CAmElement* >::iterator itListSubjects;
    getListElements(ET_ROUTE,listOfSubjects);

    for (itListSubjects = listOfSubjects.begin(); itListSubjects != listOfSubjects.end();
                        ++itListSubjects)
    {
        if (NULL != (*itListSubjects))
        {
            /*if subjects observer count is more than one it means it is shared resource, so don't delete it
              just detach the observer from the subject list*/
            CAmSourceElement* pSource = ((CAmRouteElement*)(*itListSubjects))->getSource();
            CAmSinkElement* pSink = ((CAmRouteElement*)(*itListSubjects))->getSink();
            if(pSource != NULL)
            {
                pSource->setInUse(false);
                detach(pSource);
            }
            if(pSink != NULL)
            {
                pSink->setInUse(false);
                detach(pSink);
            }
            if ((*itListSubjects)->getObserverCount() <= SHARED_COUNT)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"mainconnection deleting route element, its name is :",(*itListSubjects)->getName());
                am_Error_e err = CAmRouteFactory::destroyElement((*itListSubjects)->getName());
            }
            detach(*itListSubjects);
        }
    }
    mListRouteElements.clear();
    LOG_FN_EXIT(__FILENAME__,__func__);
}

std::string CAmMainConnectionElement::getMainSourceName(void) const
{
    std::string mainSourceName;
    CAmSourceElement* pMainSource = getMainSource();
    if (pMainSource != NULL)
    {
        mainSourceName = pMainSource->getName();
    }
    return mainSourceName;
}

std::string CAmMainConnectionElement::getMainSinkName(void) const
{
    std::string mainSinkName;
    CAmSinkElement* pMainSink = getMainSink();
    if (pMainSink != NULL)
    {
        mainSinkName = pMainSink->getName();
    }
    return mainSinkName;

}

CAmSinkElement* CAmMainConnectionElement::getMainSink(void) const
{
    return CAmSinkFactory::getElement(mRoute.sinkID);
}

CAmSourceElement* CAmMainConnectionElement::getMainSource(void) const
{
    return CAmSourceFactory::getElement(mRoute.sourceID);
}

am_Error_e CAmMainConnectionElement::getState(int& state) const
{
    am_MainConnection_s mainConnection;
    am_Error_e result;
    // get the connection info from database
    result = mpControlReceive->getMainConnectionInfoDB(getID(), mainConnection);
    state = mainConnection.connectionState;
    return result;
}

am_Error_e CAmMainConnectionElement::setState(const int state)
{
    int tempState;
    getState(tempState);
    if (tempState != state)
    {
        mpControlReceive->changeMainConnectionStateDB(getID(), (am_ConnectionState_e)state);
        notify((am_ConnectionState_e)state);
    }
    return E_OK;
}

am_Error_e CAmMainConnectionElement::getPriority(int32_t& priority)const
{
	priority = 0;
	int32_t tempPriority;

	std::vector<CAmElement* > listOfSubjects;
	std::vector<CAmElement* >::iterator itListSubjects;
	getListElements(ET_ROUTE,listOfSubjects);

	for (itListSubjects = listOfSubjects.begin(); itListSubjects != listOfSubjects.end();
			++itListSubjects)
	{
		(*itListSubjects)->getPriority(tempPriority);
		priority += tempPriority;
	}
    return E_OK;
}

void CAmMainConnectionElement::getListRouteElements(
                std::vector<CAmRouteElement* >& listRouteElements)const
{
    std::vector<CAmElement* > listOfSubjects;
    std::vector<CAmElement*>::iterator itListSubjects;
    getListElements(ET_ROUTE, listOfSubjects);
    for (itListSubjects = listOfSubjects.begin();
         itListSubjects != listOfSubjects.end(); ++itListSubjects)
    {
        listRouteElements.push_back((CAmRouteElement*) *itListSubjects);
    }
}

void CAmMainConnectionElement::updateState()
{
    int connectionState = CS_DISCONNECTED;
    std::vector<CAmRouteElement* >::iterator itlistRouteElements;
    for (itlistRouteElements = mListRouteElements.begin();
                    itlistRouteElements != mListRouteElements.end(); ++itlistRouteElements)
    {
        (*itlistRouteElements)->getState(connectionState);
        if (connectionState != CS_CONNECTED)
        {
            break;
        }
    }
    CAmSourceElement* pMainSource = CAmSourceFactory::getElement(mRoute.sourceID);
    if (pMainSource != NULL)
    {
        am_SourceState_e sourceState;
        pMainSource->getState((int&)sourceState);
        if ((connectionState == CS_CONNECTED) && (sourceState != SS_ON))
        {
            connectionState = CS_SUSPENDED;
        }
    }
    this->setState(connectionState);
}

am_Error_e CAmMainConnectionElement::_register(void)
{
    am_Error_e result = E_DATABASE_ERROR;
    std::vector<am_RoutingElement_s >::iterator itListRoutingElements;
    for (itListRoutingElements = mRoute.route.begin(); itListRoutingElements != mRoute.route.end();
                    itListRoutingElements++)
    {
        //TODO Don't touch to source/sink directly
        CAmSourceElement* pSource = CAmSourceFactory::getElement((*itListRoutingElements).sourceID);
        CAmSinkElement* pSink = CAmSinkFactory::getElement((*itListRoutingElements).sinkID);
        if ((pSource == NULL) || (pSink == NULL))
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"Source or sink not found");
            return result;
        }

        gc_RoutingElement_s gcRoutingElement;
        gcRoutingElement.name = pSource->getName() + ":" + pSink->getName();
        gcRoutingElement.sourceID = (*itListRoutingElements).sourceID;
        gcRoutingElement.sinkID = (*itListRoutingElements).sinkID;
        gcRoutingElement.domainID = (*itListRoutingElements).domainID;
        gcRoutingElement.connectionFormat = (*itListRoutingElements).connectionFormat;
        CAmRouteElement* pRoutingElement = CAmRouteFactory::createElement(gcRoutingElement,
                                                                          mpControlReceive);
        if (pRoutingElement != NULL)
        {
        	LOG_FN_DEBUG(__FILENAME__,__func__,"sourceID SinkID", pRoutingElement->getSourceID(),
        			pRoutingElement->getSinkID());
        	pSource->setInUse(true);
        	pSink->setInUse(true);
        	mListRouteElements.push_back(pRoutingElement);
        	result = pRoutingElement->attach(pSource);
        	if(result != E_OK)
        	{
        		/*its an error need to decide */
        		LOG_FN_ERROR(__FILENAME__,__func__,"element attach failed, result is:",result);
        	}
        	result = pRoutingElement->attach(pSink);
        	if(result != E_OK)
        	{
        		/*its an error need to decide */
        		LOG_FN_ERROR(__FILENAME__,__func__,"element attach failed, result is:",result);
        	}
        	result = attach(pRoutingElement);
        	if(result != E_OK)
        	{
        		/*its an error need to decide */
        		LOG_FN_ERROR(__FILENAME__,__func__,"element attach failed, result is:",result);
        	}
        }
        else
        {
        	return result;
        }
    }
    std::vector <CAmRouteElement* >::iterator itListChild;
    for(  itListChild = mListRouteElements.begin();
          itListChild != mListRouteElements.end();
          ++itListChild )
    {
        if((*itListChild)->getVolumeSupport() == true)
        {
            addVolumeElement(*itListChild);
        }
    }
    setMinVolume(getMainSink()->getMinVolume());
    setMaxVolume(getMainSink()->getMaxVolume());
    am_MainConnection_s mainConnection;
    am_mainConnectionID_t mainConnectionID;
    mainConnection.sourceID = mRoute.sourceID;
    mainConnection.sinkID = mRoute.sinkID;
    mainConnection.connectionState = CS_DISCONNECTED;
    mainConnection.mainConnectionID = 0;
    //register the connection with database
    if (E_OK == mpControlReceive->enterMainConnectionDB(mainConnection, mainConnectionID))
    {
        setID(mainConnectionID);
        result = E_OK;
    }
    return result;
}

am_Error_e CAmMainConnectionElement::_unregister(void)
{
    return mpControlReceive->removeMainConnectionDB(getID());
}

am_MuteState_e CAmMainConnectionElement::getMuteState() const
{
    am_MuteState_e muteState = MS_UNMUTED;
    if(true == isMainConnectionMuted())
    {
        muteState = MS_MUTED;
    }
    return muteState;

}

am_volume_t CAmMainConnectionElement::_getNewLimitVolume(am_volume_t newVolume)
{
    am_volume_t offsetVolume=0;
    am_volume_t volumeAfterLimit = 0;
    std::map<uint32_t, gc_LimitVolume_s >::iterator itMapLimits;
    for(itMapLimits = mMapLimits.begin();itMapLimits!= mMapLimits.end();itMapLimits++)
    {
        if(itMapLimits->second.limitType == LT_ABSOLUTE)
        {
            if(itMapLimits->second.limitVolume == AM_MUTE)
            {
                LOG_FN_DEBUG(__FILENAME__,__func__,"offset volume = AM_MUTE");
                return AM_MUTE;
            }
            volumeAfterLimit = itMapLimits->second.limitVolume;
        }
        if(itMapLimits->second.limitType == LT_RELATIVE)
        {
            volumeAfterLimit = newVolume - itMapLimits->second.limitVolume;
        }
        offsetVolume = std::min(newVolume,volumeAfterLimit);
    }
    LOG_FN_DEBUG(__FILENAME__,__func__,"offset volume = ",offsetVolume-newVolume);
    return offsetVolume-newVolume;
}

am_Error_e CAmMainConnectionElement::setLimits(std::map<uint32_t, gc_LimitVolume_s >& mapLimits)
{
    LOG_FN_DEBUG(__FILENAME__,__func__,"Num Limits=",mapLimits.size());
    mMapLimits = mapLimits;
    return E_OK;
}

am_Error_e CAmMainConnectionElement::getLimits(std::map<uint32_t, gc_LimitVolume_s >& mapLimits) const
{
    mapLimits = mMapLimits;
    return E_OK;
}

void CAmMainConnectionElement::getVolumeChangeElements(am_volume_t requestedVolume,
                  std::map<CAmElement*,gc_volume_s >& listVolumeElements)
{
    am_volume_t limitVolume=0;
    CAmElement::getVolumeChangeElements(requestedVolume,listVolumeElements);
    LOG_FN_DEBUG(__FILENAME__,__func__,"Number of Volume Elements=",
            listVolumeElements.size());
    clearLimitVolume(listVolumeElements);
    if(false == mMapLimits.empty())
    {
        am_volume_t offsetVolume = _getNewLimitVolume(requestedVolume);
        if(offsetVolume ==  AM_MUTE)
        {
            LOG_FN_DEBUG("Sink Mute Found");
            std::map<CAmElement*,gc_volume_s >::iterator itlistVolumeElements;
            itlistVolumeElements = listVolumeElements.find(getMainSink());
            if(itlistVolumeElements != listVolumeElements.end())
            {
                itlistVolumeElements->second.isOffsetSet = true;
                itlistVolumeElements->second.offsetVolume = AM_MUTE;
            }
            else
            {
                gc_volume_s volume;
                volume.isvolumeSet = false;
                volume.isOffsetSet= true;
                volume.offsetVolume = AM_MUTE;
                listVolumeElements[getMainSink()] = volume;
            }
        }
        else
        {
            getLimitVolumeElements(offsetVolume,listVolumeElements);
        }
    }
}

bool CAmMainConnectionElement::isSinkBelongingToMainConnection(CAmSinkElement* pSink)
{
    bool found = false;
    std::vector<CAmRouteElement*>::iterator itListRouteElement;
    for(itListRouteElement = mListRouteElements.begin();
            itListRouteElement != mListRouteElements.end();++itListRouteElement)
    {
        if((*itListRouteElement)->getSink()== pSink)
        {
            found = true;
            break;
        }
    }
    return found;
}

bool CAmMainConnectionElement::isSourceBelongingToMainConnection(CAmSourceElement* pSource)
{
    bool found = false;
    std::vector<CAmRouteElement*>::iterator itListRouteElement;
    for(itListRouteElement = mListRouteElements.begin();
            itListRouteElement != mListRouteElements.end();++itListRouteElement)
    {
        if((*itListRouteElement)->getSource() == pSource)
        {
            found = true;
            break;
        }
    }
    return found;
}

bool CAmMainConnectionElement::isMainConnectionLimited(void) const
{
    return !(mMapLimits.empty());
}

bool CAmMainConnectionElement::isMainConnectionMuted(void) const
{
    bool isMuted = false;
    std::map<uint32_t, gc_LimitVolume_s >::const_iterator itMapLimits;
    for(itMapLimits=mMapLimits.begin();
        itMapLimits!=mMapLimits.end();
        itMapLimits++)
    {
        if(itMapLimits->second.limitVolume == AM_MUTE)
        {
            isMuted = true;
            break;
        }
    }
    return isMuted;
}

int CAmMainConnectionElement::update(CAmElement *pNotifierElement,const am_mainVolume_t& mainVolume)
{
    setMainVolume(mainVolume);
    return E_OK;
}
int CAmMainConnectionElement::update(CAmElement *pNotifierElement,gc_Element_Status_e& elementStatus)
{
	LOG_FN_ENTRY(__FILENAME__,__func__);
	if(pNotifierElement != NULL)
	{
	    if(elementStatus == ROUTE_DISCONNECT)
	    {
	        LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is :",pNotifierElement->getName());
	        notify(MAINCONNECTION_DISCONNECT);
	    }
	}
	else
	{
		LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is invalid");
	}
	LOG_FN_EXIT(__FILENAME__,__func__);
}

int CAmMainConnectionElement::releaseResources(CAmElement *pNotifierElement)
{
	LOG_FN_ENTRY(__FILENAME__,__func__);
	if(pNotifierElement != NULL)
	{
	    LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is :",pNotifierElement->getName());
	    detach(pNotifierElement);
	}
	else
	{
		LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is invalid");
	}
	LOG_FN_EXIT(__FILENAME__,__func__);
}

am_Error_e CAmMainConnectionElement::updateMainVolume()
{
    am_volume_t volume = 0;
    am_mainVolume_t mainVolume = 0;
    CAmSinkElement* pSink = getMainSink();
    int state = CS_UNKNOWN;
    getState(state);
    if (pSink != NULL)
    {
        if (state == CS_CONNECTED)
        {
            /*
             * The shared sink's main volume is the sum of the volume of all
             * the connection in which it is involved.
             *             mc1                 mc2
             *  {source1} <---> {shared-sink} <---> {source2}
             *  volume = Vsource1 + Vshared-sink + Vsource2
             */
            std::vector<CAmMainConnectionElement*> listMainConnections;
            std::vector<CAmMainConnectionElement*>::iterator itListMainConnections;
            std::map<CAmElement*, am_volume_t> mapVolume;
            CAmMainConnectionFactory::getListElements(listMainConnections);
            itListMainConnections = listMainConnections.begin();
            for (;itListMainConnections != listMainConnections.end();++itListMainConnections)
            {
                int state;
                (*itListMainConnections)->getState(state);
                if ((state == CS_CONNECTED ) &&
                    ((*itListMainConnections)->isSinkBelongingToMainConnection(pSink) == true))
                {
                    (*itListMainConnections)->getRootVolumeElements(mapVolume);
                }
            }

            std::map<CAmElement*, am_volume_t>::iterator itMapVolume;
            itMapVolume = mapVolume.begin();
            for (; itMapVolume != mapVolume.end(); ++itMapVolume)
            {
                LOG_FN_INFO(__FILENAME__, __func__, "The volume of sink=", itMapVolume->second);
                volume += itMapVolume->second;
            }
            LOG_FN_INFO(__FILENAME__, __func__, "The volume of sink=", volume);
        }
        else
        {
            volume = getVolume();
        }
        am_mainVolume_t mainVolume = pSink->convertVolumeToMainVolume(volume);
        pSink->setMainVolume(mainVolume);
        volume = getVolume();
        setMainVolume(pSink->convertVolumeToMainVolume(volume));

    }
    return E_OK;
}
} /* namespace gc */
} /* namespace am */
