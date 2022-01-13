/******************************************************************************
 * @file: CAmMainConnectionActionConnect.cpp
 *
 * This file contains the definition of main connection action connect class
 * (member functions and data members) used to implement the logic of connect
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

#include "CAmMainConnectionActionConnect.h"
#include "CAmMainConnectionElement.h"
#include "CAmSourceElement.h"
#include "CAmSourceActionSetState.h"
#include "CAmRouteActionConnect.h"
#include "CAmLogger.h"
#include "CAmTriggerQueue.h"
#include <algorithm>

namespace am {
namespace gc {

CAmMainConnectionActionConnect::CAmMainConnectionActionConnect(
                CAmMainConnectionElement* pMainConnection) :
                                CAmActionContainer(std::string("CAmMainConnectionActionConnect")),
                                mpMainConnection(pMainConnection)
{
    this->_registerParam(ACTION_PARAM_CONNECTION_FORMAT, &mConnectionFormatParam);
    this->_registerParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&mSetSourceStateDirectionParam);
}

CAmMainConnectionActionConnect::~CAmMainConnectionActionConnect()
{
}

int CAmMainConnectionActionConnect::_execute(void)
{
    if (NULL == mpMainConnection)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Parameters not set.");
        return E_NOT_POSSIBLE;
    }
    int state;
    mpMainConnection->getState(state);
    if(CS_CONNECTED == state)
    {
        return E_OK; // already connected
    }

    std::vector<CAmRouteElement*> listRouteElements;
    std::vector<CAmRouteElement*>::iterator itListRouteElements;
    mpMainConnection->getListRouteElements(listRouteElements);
    gc_SetSourceStateDirection_e setSourceStateDirection = SD_MAINSINK_TO_MAINSOURCE;
    mSetSourceStateDirectionParam.getParam(setSourceStateDirection);
    if(setSourceStateDirection == SD_MAINSINK_TO_MAINSOURCE)
    {
        // reverse the vector
        std::reverse(listRouteElements.begin(),listRouteElements.end());
    }
    if(E_OK != _createListActionsRouteConnect(listRouteElements))
    {
        return E_NOT_POSSIBLE;
    }
    if(E_OK != _createListActionsSetSourceState(listRouteElements,SS_ON))
    {
        return E_NOT_POSSIBLE;
    }

    mpMainConnection->setState(CS_CONNECTING);
    _setConnectionStateChangeTrigger();
    return E_OK;
}

int CAmMainConnectionActionConnect::_undo(void)
{
    // update main connection state in Audio Manager daemon
    mpMainConnection->setState(CS_DISCONNECTING);
    _setConnectionStateChangeTrigger();
    return E_OK;
}

int CAmMainConnectionActionConnect::_update(const int result)
{
    am_ConnectionState_e connectionState(CS_UNKNOWN);
    if ((E_OK == result) && (AS_COMPLETED == getStatus()))
    {
        am_SourceState_e sourceState;
        mpMainConnection->getMainSource()->getState((int&)sourceState);
        if (sourceState == SS_ON)
        {
            connectionState = CS_CONNECTED;
        }
        else
        {
            connectionState = CS_SUSPENDED;
        }
    }
    else if (AS_UNDO_COMPLETE == getStatus())
    {
        connectionState = CS_DISCONNECTED;
    }

    if(connectionState != CS_UNKNOWN)
    {
        // update main connection state in Audio Manager daemon
        mpMainConnection->setState(connectionState);
        mpMainConnection->updateMainVolume();
        _setConnectionStateChangeTrigger();
    }
    return E_OK;
}

am_Error_e CAmMainConnectionActionConnect::_createListActionsRouteConnect(
        std::vector<CAmRouteElement*>& listRouteElements)
{
    am_Error_e error = E_OK;
    std::vector<CAmRouteElement*>::iterator itListRouteElements;
    for (itListRouteElements = listRouteElements.begin();
            itListRouteElements != listRouteElements.end();
            ++itListRouteElements)
    {
        error = _createActionRouteConnect(*itListRouteElements);
        if(error != E_OK)
        {
            break;
        }
    }
    return error;
}

void CAmMainConnectionActionConnect::_setConnectionStateChangeTrigger(void)
{
    gc_ConnectionStateChangeTrigger_s* ptrigger = new gc_ConnectionStateChangeTrigger_s;
    ptrigger->connectionName = mpMainConnection->getName();
    mpMainConnection->getState((int&)(ptrigger->connectionState));
    ptrigger->status = (am_Error_e)getError();
    CAmTriggerQueue::getInstance()->pushTop(SYSTEM_CONNECTION_STATE_CHANGE,ptrigger);
}

am_Error_e CAmMainConnectionActionConnect::_createActionRouteConnect(
        CAmRouteElement* routeElement)
{
    // create router connect action for each element
    IAmActionCommand* pAction = new CAmRouteActionConnect(routeElement);
    if (NULL != pAction)
    {
        am_CustomConnectionFormat_t connectionFormat;
        if (mConnectionFormatParam.getParam(connectionFormat))
        {
            pAction->setParam(ACTION_PARAM_CONNECTION_FORMAT, &mConnectionFormatParam);
        }
        pAction->setUndoRequried(getUndoRequired());
        append(pAction);
    }
    return E_OK;
}

am_Error_e CAmMainConnectionActionConnect::_createListActionsSetSourceState(std::vector<CAmRouteElement*>& listRouteElements,
        const am_SourceState_e requestedSourceState)
{
    am_Error_e error = E_OK;
    std::vector<CAmRouteElement*>::iterator itListRouteElements;
    for (itListRouteElements = listRouteElements.begin();
            itListRouteElements != listRouteElements.end();
            ++itListRouteElements)
    {
        error = _createActionSetSourceState((*itListRouteElements)->getSource(),requestedSourceState);
        if(error != E_OK)
        {
            break;
        }
    }
    return error;
}

am_Error_e CAmMainConnectionActionConnect::_createActionSetSourceState(CAmSourceElement* pSource,
        const am_SourceState_e requestedSourceState)
{
    am_Error_e error = E_NOT_POSSIBLE;
    IAmActionCommand* pAction(NULL);
    if(pSource  != NULL)
    {
        int sourceState;
        pSource->getState(sourceState);
        if(static_cast<am_SourceState_e>(sourceState) != SS_UNKNNOWN)
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
