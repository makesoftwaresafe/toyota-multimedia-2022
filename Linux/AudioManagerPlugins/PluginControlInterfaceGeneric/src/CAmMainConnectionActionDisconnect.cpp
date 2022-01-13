/******************************************************************************
 * @file: CAmMainConenctionActionDisconnect.cpp
 *
 * This file contains the definition of main connection action disconnect class
 * (member functions and data members) used to implement the logic of disconnect
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

#include "CAmMainConnectionActionDisconnect.h"
#include "CAmMainConnectionElement.h"
#include "CAmSourceElement.h"
#include "CAmSourceActionSetState.h"
#include "CAmRouteActionDisconnect.h"
#include "CAmLogger.h"
#include "CAmTriggerQueue.h"
#include <algorithm>

namespace am {
namespace gc {

CAmMainConnectionActionDisconnect::CAmMainConnectionActionDisconnect(CAmMainConnectionElement* pMainConnection) :
                CAmActionContainer(std::string("CAmMainConnectionActionDisconnect")),
                mpMainConnection(pMainConnection),
                mActionCompleted(false)
{
    this->_registerParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&mSetSourceStateDirectionParam);
}

CAmMainConnectionActionDisconnect::~CAmMainConnectionActionDisconnect()
{
}

int CAmMainConnectionActionDisconnect::_execute(void)
{
    if (NULL == mpMainConnection)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Parameters not set.");
        return E_NOT_POSSIBLE;
    }

     //get route elements from main connection class
     std::vector<CAmRouteElement*> listRouteElements;
     std::vector<CAmRouteElement*>::iterator itListRouteElements;
     mpMainConnection->getListRouteElements(listRouteElements);
     gc_SetSourceStateDirection_e setSourceStateDirection = SD_MAINSINK_TO_MAINSOURCE;
     mSetSourceStateDirectionParam.getParam(setSourceStateDirection);
    if(setSourceStateDirection == SD_MAINSINK_TO_MAINSOURCE)
    {
        std::reverse(listRouteElements.begin(),listRouteElements.end());
    }
    if(E_OK != _createListActionsSetSourceState(listRouteElements,SS_OFF))
    {
        return E_NOT_POSSIBLE;
    }
    if(E_OK != _createListActionsRouteDisconnect(listRouteElements))
    {
        return E_NOT_POSSIBLE;
    }
    // update main connection state in Audio Manager daemon
    mpMainConnection->setState(CS_DISCONNECTING);
    _setConnectionStateChangeTrigger();
    return E_OK;
}

int CAmMainConnectionActionDisconnect::_undo(void)
{
    // update main connection state in Audio Manager daemon
    mpMainConnection->setState(CS_CONNECTING);
    _setConnectionStateChangeTrigger();
    return E_OK;
}

int CAmMainConnectionActionDisconnect::_update(const int result)
{
    if (AS_COMPLETED == getStatus())
    {
        if (E_OK == result)
        {
            mActionCompleted = true;
        }
        mpMainConnection->updateState();
        _setConnectionStateChangeTrigger();
    }
    else if (AS_UNDO_COMPLETE == getStatus())
    {
        if (mActionCompleted == false)
        {
            mpMainConnection->updateState();
            _setConnectionStateChangeTrigger();
        }
    }
    return E_OK;
}

bool CAmMainConnectionActionDisconnect::_checkSharedRouteDisconnected(const CAmRouteElement& routeElement)
{
    std::vector<CAmElement*> listElements;
    if (routeElement.getObserverCount(ET_CONNECTION, &listElements) <= 1)
    {
        return true;
    }
    for (auto it : listElements)
    {
        int connectionState;
        (static_cast<CAmMainConnectionElement*>(it))->getState(connectionState);
        if ((connectionState != CS_DISCONNECTED) && (it != mpMainConnection))
        {
            return false;
        }
    }
    return true;
}

bool CAmMainConnectionActionDisconnect::_checkSharedSourceDisconnected(
        const CAmSourceElement& source, const CAmRouteElement& routeElement)
{
    std::vector<CAmElement*> listElements;
    if (source.getObserverCount(ET_ROUTE, &listElements) <= 1)
    {
        return true;
    }
    for (auto it : listElements)
    {
        int connectionState;
        ((CAmRouteElement*) it)->getState(connectionState);
        if ((connectionState != CS_DISCONNECTED) && (it->getName() != routeElement.getName()))
        {
            return false;
        }
    }
    return true;
}

void CAmMainConnectionActionDisconnect::_setConnectionStateChangeTrigger(void)
{
    gc_ConnectionStateChangeTrigger_s* ptrigger = new gc_ConnectionStateChangeTrigger_s;
    ptrigger->connectionName = mpMainConnection->getName();
    int connectionState;
    mpMainConnection->getState(connectionState);
    ptrigger->connectionState = static_cast<am_ConnectionState_e>(connectionState);
    ptrigger->status = (am_Error_e)getError();
    CAmTriggerQueue::getInstance()->pushTop(SYSTEM_CONNECTION_STATE_CHANGE,ptrigger);
}

am_Error_e CAmMainConnectionActionDisconnect::_createListActionsRouteDisconnect
                            ( std::vector<CAmRouteElement*>& listRouteElements)
{
    am_Error_e error = E_OK;
    for (auto itListRouteElements : listRouteElements)
    {
        error = _createActionRouteDisconnect(itListRouteElements);
        if(error != E_OK)
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmMainConnectionActionDisconnect::_createActionRouteDisconnect( CAmRouteElement* routeElement)
{
    if (routeElement == NULL)
    {
        return E_NOT_POSSIBLE;
    }
    if (_checkSharedRouteDisconnected(*routeElement))
    {
        IAmActionCommand* pAction = new CAmRouteActionDisconnect(routeElement);
        if (NULL != pAction)
        {
            // add the newly created route disconnect action to dynamic action
            pAction->setUndoRequried(getUndoRequired());
            append(pAction);
        }
    }
    return E_OK;
}

am_Error_e CAmMainConnectionActionDisconnect::_createListActionsSetSourceState(std::vector<CAmRouteElement*>& listRouteElements, const am_SourceState_e requestedSourceState)
{
    am_Error_e error = E_OK;
    for (auto itListRouteElements : listRouteElements)
    {
        if (itListRouteElements == NULL)
        {
            return E_NOT_POSSIBLE;
        }
        /*
         * If route is part of two connection then skip sending setSourceState
         */
        if (false == _checkSharedRouteDisconnected(*itListRouteElements))
        {
            continue;
        }
        error = _createActionSetSourceState(itListRouteElements->getSource(),requestedSourceState,itListRouteElements);
        if(error != E_OK)
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmMainConnectionActionDisconnect::_createActionSetSourceState(CAmSourceElement* pSource,
        const am_SourceState_e requestedSourceState, CAmRouteElement* pRouteElement)
{
    am_Error_e error = E_NOT_POSSIBLE;
    IAmActionCommand* pAction(NULL);
    if ((pSource  != NULL) && (pRouteElement != NULL))
    {
        int sourceState;
        pSource->getState(sourceState);
        /*
         * Set the source state to SS_OFF only if source is not shared and current state is not SS_UNKNOWN
         */
        if ((static_cast<am_SourceState_e>(sourceState) != SS_UNKNNOWN) &&
            (true == _checkSharedSourceDisconnected(*pSource,*pRouteElement)))
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
