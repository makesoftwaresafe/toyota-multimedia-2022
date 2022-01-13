/******************************************************************************
 * @file: CAmGatewayElement.cpp
 *
 * This file contains the definition of gateway element class (member functions
 * and data members) used as data container to store the information related to
 * gateway as maintained by controller.
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

#include "CAmGatewayElement.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmGatewayElement::CAmGatewayElement(const gc_Gateway_s& gateway,
                                     CAmControlReceive* pControlReceive) :
                                CAmElement(gateway.name, pControlReceive),
                                mGateway(gateway),
                                mpControlReceive(pControlReceive)
{
    setType (ET_GATEWAY);
}

CAmGatewayElement::~CAmGatewayElement()
{
    std::vector<CAmElement* > listOfSubjects;
    std::vector<CAmElement* >::iterator itListSubjects;
    getListElements(ET_SOURCE,listOfSubjects);

    for (itListSubjects = listOfSubjects.begin(); itListSubjects != listOfSubjects.end();
                        ++itListSubjects)
    {
        if (NULL != (*itListSubjects))
        {
            detach(*itListSubjects);
        }
    }

    getListElements(ET_SINK,listOfSubjects);

    for (itListSubjects = listOfSubjects.begin(); itListSubjects != listOfSubjects.end();
                        ++itListSubjects)
    {
        if (NULL != (*itListSubjects))
        {
            detach(*itListSubjects);
        }
    }

}

am_Error_e CAmGatewayElement::_register(void)
{
    am_gatewayID_t gatewayID(0);
    am_Error_e result = E_DATABASE_ERROR;
    if (E_OK == mpControlReceive->enterGatewayDB(mGateway, gatewayID))
    {
        setID(gatewayID);
        result = E_OK;
    }
    return result;
}

am_Error_e CAmGatewayElement::_unregister(void)
{
    am_Error_e result = E_DATABASE_ERROR;
    if (E_OK == mpControlReceive->removeGatewayDB(getID()))
    {
        setID(0);
        result = E_OK;
    }
    return result;

}
am_sinkID_t CAmGatewayElement::getSinkID(void)
{
    return mGateway.sinkID;
}

am_sourceID_t CAmGatewayElement::getSourceID(void)
{
    return mGateway.sourceID;
}
am_domainID_t CAmGatewayElement::getSourceDomainID(void)
{
    return mGateway.domainSourceID;
}
am_domainID_t CAmGatewayElement::getSinkDomainID(void)
{
    return mGateway.domainSinkID;
}

} /* namespace gc */
} /* namespace am */
