/******************************************************************************
 * @file: CAmRouteActionDisconnect.cpp
 *
 * This file contains the definition of router action disconnect class (member
 * functions and data members) used to implement the logic of disconnection at
 * router level
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

#include "CAmRouteActionDisconnect.h"
#include "CAmControlReceive.h"
#include "CAmRouteElement.h"
#include "CAmLogger.h"

namespace am {

namespace gc {

CAmRouteActionDisconnect::CAmRouteActionDisconnect(CAmRouteElement* pRouteElement) :
                                CAmActionCommand(std::string("CAmRouterActionDisconnect")),
                                mpRouteElement(pRouteElement)
{
}

CAmRouteActionDisconnect::~CAmRouteActionDisconnect()
{
}

int CAmRouteActionDisconnect::_execute(void)
{
    if (NULL == mpRouteElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Parameters not set");
        return E_NOT_POSSIBLE;
    }
am_ConnectionState_e connectionState;
    CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
    mpRouteElement->getState((int&)connectionState);
    if ((CS_DISCONNECTED ==  connectionState)|| (CS_UNKNOWN == connectionState))
    {
        return E_OK;
    }

    int result = pControlReceive->disconnect(mpRouteElement->getID());
    if (E_OK == result)
    {
        pControlReceive->registerObserver(this);
        mpRouteElement->setState(CS_DISCONNECTING);
        result = E_WAIT_FOR_CHILD_COMPLETION;
    }
    return result;
}

int CAmRouteActionDisconnect::_undo(void)
{
    am_connectionID_t connectionID(0);
    CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
    int result = pControlReceive->connect(connectionID,
                                          mpRouteElement->getConnectionFormat(),
                                          mpRouteElement->getSourceID(),
                                          mpRouteElement->getSinkID());
    if (E_OK == result)
    {
        pControlReceive->registerObserver(this);
        mpRouteElement->setID(connectionID);
        mpRouteElement->setState(CS_CONNECTING);
        result = E_WAIT_FOR_CHILD_COMPLETION;
    }
    return result;
}

int CAmRouteActionDisconnect::_update(const int result)
{
    if (AS_UNDO_COMPLETE == getStatus())
    {
        // Result need not be checked as, even if undo is failed nothing can be done.
        mpRouteElement->setState(CS_CONNECTED);
    }
    else
    {
        if ((E_OK == result) && (getStatus() == AS_COMPLETED))
        {
            mpRouteElement->setState(CS_DISCONNECTED);
        }
        else
        {
            mpRouteElement->setState(CS_CONNECTED);
        }
    }
    //unregister the observer
    CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
    pControlReceive->unregisterObserver(this);
    return E_OK;
}

void CAmRouteActionDisconnect::_timeout(void)
{
    CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
    pControlReceive->abortAction();
}
}/* namespace gc */
}/* namespace am */
