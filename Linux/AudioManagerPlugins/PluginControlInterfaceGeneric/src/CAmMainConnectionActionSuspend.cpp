/******************************************************************************
 * @file: CAmMainConnectionActionSuspend.cpp
 *
 * This file contains the definition of main connection action suspend class
 * (member functions and data members) used to implement the logic of suspend
 * at main connection level
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

#include "CAmMainConnectionActionSuspend.h"
#include "CAmSourceElement.h"
#include "CAmSourceActionSetState.h"
#include "CAmRouteActionConnect.h"
#include "CAmLogger.h"
#include "CAmTriggerQueue.h"
#include <algorithm>

namespace am {
namespace gc {

CAmMainConnectionActionSuspend::CAmMainConnectionActionSuspend(
                CAmMainConnectionElement* pMainConnection) :
                                CAmActionContainer(std::string("CAmMainConnectionActionSuspend")),
                                mpMainConnection(pMainConnection)
{
    this->_registerParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&mSetSourceStateDirectionParam);
}

CAmMainConnectionActionSuspend::~CAmMainConnectionActionSuspend()
{
}

int CAmMainConnectionActionSuspend::_execute(void)
{
    if (NULL == mpMainConnection)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Parameter not set");
        return E_NOT_POSSIBLE;;
    }
    int state;
    mpMainConnection->getState(state);

    std::vector<CAmRouteElement*> listRouteElements;
    mpMainConnection->getListRouteElements(listRouteElements);
    gc_SetSourceStateDirection_e setSourceStateDirection = SD_MAINSINK_TO_MAINSOURCE;
    mSetSourceStateDirectionParam.getParam(setSourceStateDirection);
    if(setSourceStateDirection == SD_MAINSINK_TO_MAINSOURCE)
    {
        // reverse the vector
        std::reverse(listRouteElements.begin(),listRouteElements.end());
    }

    if (state == CS_DISCONNECTED)
    {
        if (_createListActionsRouteConnect(listRouteElements) != E_OK)
        {
            return E_NOT_POSSIBLE;
        }
        mpMainConnection->setState(CS_CONNECTING);
        _setConnectionStateChangeTrigger();
    }
    if(_createListActionsSetSourceState(listRouteElements,SS_PAUSED) != E_OK )
    {
        return E_NOT_POSSIBLE;
    }
    return E_OK;
}

void CAmMainConnectionActionSuspend::_setConnectionStateChangeTrigger(void)
{
    gc_ConnectionStateChangeTrigger_s* ptrigger = new gc_ConnectionStateChangeTrigger_s;
    ptrigger->connectionName = mpMainConnection->getName();
    mpMainConnection->getState((int&)(ptrigger->connectionState));
    ptrigger->status = (am_Error_e)getError();
    CAmTriggerQueue::getInstance()->pushTop(SYSTEM_CONNECTION_STATE_CHANGE,ptrigger);
}

int CAmMainConnectionActionSuspend::_update(const int result)
{
    if (getStatus() == AS_COMPLETED)
    {
        mpMainConnection->updateState();
        gc_ConnectionStateChangeTrigger_s* ptrigger = new gc_ConnectionStateChangeTrigger_s;
        ptrigger->connectionName = mpMainConnection->getName();
        int connectionState;
        mpMainConnection->getState(connectionState);
        ptrigger->connectionState = static_cast<am_ConnectionState_e>(connectionState);
        ptrigger->status = (am_Error_e)getError();
        CAmTriggerQueue::getInstance()->pushTop(SYSTEM_CONNECTION_STATE_CHANGE,ptrigger);
    }
    return E_OK;
}

bool CAmMainConnectionActionSuspend::_sharedConnectionCheck( const CAmRouteElement& routeElement)
{
    int sourceState;
    std::vector<CAmElement*> listConnections;
    routeElement.getSource()->getState(sourceState);
    int count = routeElement.getObserverCount(ET_CONNECTION, &listConnections);
    if ((sourceState == SS_OFF) || (listConnections.size() <= 1))
    {
        return true;
    }
    for (auto it : listConnections)
    {
        int connectionState;
        (static_cast<CAmMainConnectionElement*>(it))->getState(connectionState);
        if ((connectionState == CS_CONNECTED)
                && (((CAmMainConnectionElement*) it) != mpMainConnection))
        {
            return false;
        }
    }
    return true;
}

am_Error_e CAmMainConnectionActionSuspend::_createListActionsRouteConnect(
        std::vector<CAmRouteElement*>& listRouteElements)
{
    am_Error_e error = E_OK;
    for (auto elem : listRouteElements)
    {
        error = _createActionRouteConnect(elem);
        if (error != E_OK)
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmMainConnectionActionSuspend::_createActionRouteConnect(
        CAmRouteElement* routeElement)
{
    am_Error_e err = E_NOT_POSSIBLE;
    if (routeElement == NULL)
    {
        return E_NOT_POSSIBLE;
    }

    // create router connect action for each element
    IAmActionCommand* pAction = new CAmRouteActionConnect(routeElement);
    if (NULL != pAction)
    {
        CAmActionParam<am_CustomConnectionFormat_t> connectionFormatparam(
                                                        routeElement->getConnectionFormat());
        pAction->setParam(ACTION_PARAM_CONNECTION_FORMAT, &connectionFormatparam);
        pAction->setUndoRequried(getUndoRequired());
        append(pAction);
        err = E_OK;
    }
    return err;
}


am_Error_e CAmMainConnectionActionSuspend::_createListActionsSetSourceState(std::vector<CAmRouteElement*>& listRouteElements,
        const am_SourceState_e requestedSourceState)
{
    am_Error_e error = E_OK;
    for (auto elem : listRouteElements)
    {
        if (elem == NULL)
        {
            error = E_NOT_POSSIBLE;
            break;
        }

        if (false == _sharedConnectionCheck(*elem))
        {
            continue;
        }
        error = _createActionSetSourceState(elem->getSource(),requestedSourceState);
        if(error != E_OK)
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmMainConnectionActionSuspend::_createActionSetSourceState(CAmSourceElement* pSource,
        const am_SourceState_e requestedSourceState)
{
    am_Error_e error = E_NOT_POSSIBLE;
    IAmActionCommand* pAction(NULL);
    if(pSource  != NULL)
    {
        int sourceState;
        pSource->getState(sourceState);
        if (static_cast<am_SourceState_e>(sourceState) == SS_UNKNNOWN)
        {
           return E_OK;
        }
        if ((pSource->getObserverCount(ET_ROUTE) <=1) || ((static_cast<am_SourceState_e>(sourceState) == SS_OFF)))
        {
            pAction = new CAmSourceActionSetState(pSource);
            if (pAction != NULL)
            {
                CAmActionParam<am_SourceState_e> sourceStateParam(requestedSourceState);
                pAction->setParam(ACTION_PARAM_SOURCE_STATE, &sourceStateParam);
                append(pAction);
                error = E_OK;
            }
        }
        else
        {
            error = E_OK;
        }
    }
    return error;
}

} /* namespace gc */
} /* namespace am */
