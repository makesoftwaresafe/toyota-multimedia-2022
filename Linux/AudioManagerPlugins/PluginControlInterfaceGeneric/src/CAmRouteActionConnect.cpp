/******************************************************************************
 * @file: CAmRouteActionConnect.cpp
 *
 * This file contains the definition of router action connect class (member
 * functions and data members) used to implement the logic of connection at
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

#include "CAmRouteActionConnect.h"
#include "CAmControlReceive.h"
#include "CAmRouteElement.h"
#include "CAmLogger.h"
namespace am {
namespace gc {

CAmRouteActionConnect::CAmRouteActionConnect(CAmRouteElement* pRouteElement) :
                                CAmActionCommand(std::string("CAmRouterActionConnect")),
                                mpRouteElement(pRouteElement)
{
}

CAmRouteActionConnect::~CAmRouteActionConnect()
{
}

int CAmRouteActionConnect::_execute(void)
{
    if (NULL == mpRouteElement)
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Parameters not set");
        return E_NOT_POSSIBLE;
    }

	int result = E_OK;
    am_connectionID_t connectionID(0);
	am_ConnectionState_e connectionState;
	mpRouteElement->getState((int&)connectionState);
	if (connectionState != CS_CONNECTED)
	{
		CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
		result = pControlReceive->connect(connectionID,
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
	}
    return result;
}

int CAmRouteActionConnect::_undo(void)
{
    int result = E_OK;
    am_ConnectionState_e connectionState;
    mpRouteElement->getState((int&)connectionState);
    if (CS_CONNECTED == connectionState)
    {
        CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
        result = pControlReceive->disconnect(mpRouteElement->getID());
        if (E_OK == result)
        {
            pControlReceive->registerObserver(this);
            mpRouteElement->setState(CS_DISCONNECTING);
            result = E_WAIT_FOR_CHILD_COMPLETION;
        }
    }
    return result;
}

int CAmRouteActionConnect::_update(const int result)
{
    if (AS_UNDO_COMPLETE == getStatus())
    {
        // Result need not be checked as, even if undo is failed nothing can be done.
        mpRouteElement->setState(CS_DISCONNECTED);
    }
    else
    {
        if ((E_OK == result) && (getStatus() == AS_COMPLETED))
        {
            mpRouteElement->setState(CS_CONNECTED);
        }
        else
        {
            mpRouteElement->setState(CS_DISCONNECTED);
        }
    }
    //unregister the observer
    CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
    pControlReceive->unregisterObserver(this);
    return E_OK;
}

void CAmRouteActionConnect::_timeout()
{
    CAmControlReceive* pControlReceive = mpRouteElement->getControlReceive();
    pControlReceive->abortAction();
}

}/* namespace gc */
}/* namespace am */
