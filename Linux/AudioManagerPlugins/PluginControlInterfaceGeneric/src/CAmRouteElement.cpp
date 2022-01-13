/******************************************************************************
 * @file: CAmRouteElement.cpp
 *
 * This file contains the definition of route element class (member functions
 * and data members) used as data container to store the information related to
 * route of connection as maintained by controller.
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

#include "CAmRouteElement.h"
#include "CAmSourceElement.h"
#include "CAmSinkElement.h"
#include "CAmControlReceive.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmRouteElement::CAmRouteElement(const gc_RoutingElement_s& routingElement,
                                 CAmControlReceive* pControlReceive) :
                CAmElement(routingElement.name, pControlReceive),
                mRoutingElement(routingElement),
                mpSink(NULL),
                mpSource(NULL)
{
    setType(ET_ROUTE);
    setState(CS_DISCONNECTED);
    mpSink = CAmSinkFactory::getElement(mRoutingElement.sinkID);
    mpSource = CAmSourceFactory::getElement(mRoutingElement.sourceID);
    if(true == mpSource->getVolumeSupport())
    {
        addVolumeElement(mpSource);
        if(false == mpSink->getVolumeSupport())
        {
            setMaxVolume(mpSource->getMaxVolume());
            setMinVolume(mpSource->getMinVolume());
        }
    }
    if(true  == mpSink->getVolumeSupport())
    {
        addVolumeElement(mpSink);
        if(false == mpSource->getVolumeSupport())
        {
            setMaxVolume(mpSink->getMaxVolume());
            setMinVolume(mpSink->getMinVolume());
        }
    }
    if((true == mpSink->getVolumeSupport()) &&
       (true == mpSource->getVolumeSupport()) )
    {
        setMaxVolume(std::max(mpSink->getMaxVolume(),mpSource->getMaxVolume()));
        setMinVolume(std::min(mpSink->getMinVolume(),mpSource->getMinVolume()));
    }
}

CAmRouteElement::~CAmRouteElement()
{
	_detachAll();
}

am_Error_e CAmRouteElement::getPriority(int32_t& priority) const
{
    priority = 0;
    int32_t sinkPriority(0);
    int32_t sourcePriority(0);
    CAmElement* pSourceElement = getSource();
    CAmElement* pSinkElement = getSink();
    if ((pSourceElement != NULL) && (pSinkElement != NULL))
    {
        pSourceElement->getPriority(sourcePriority);
        pSinkElement->getPriority(sinkPriority);
        priority = sourcePriority + sinkPriority;
    }
    return E_OK;
}


am_sourceID_t CAmRouteElement::getSourceID(void) const
{
    return mRoutingElement.sourceID;
}

am_sinkID_t CAmRouteElement::getSinkID(void) const
{
    return mRoutingElement.sinkID;
}

CAmSourceElement* CAmRouteElement::getSource(void) const
{
    return mpSource;
}

CAmSinkElement* CAmRouteElement::getSink(void) const
{
    return mpSink;
}

am_CustomConnectionFormat_t CAmRouteElement::getConnectionFormat(void) const
{
    return mRoutingElement.connectionFormat;
}

int CAmRouteElement::releaseResources(CAmElement *pNotifierElement)
{
	if(pNotifierElement != NULL)
	{
		LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is :",pNotifierElement->getName());
		detach(pNotifierElement);
		notify(ROUTE_DISCONNECT); /*notify main connection element for disconnection of route*/
	}
	else
	{
		LOG_FN_DEBUG(__FILENAME__,__func__,"notifing element is invalid");
	}
}

}/* namespace gc */
}/* namespace am */
