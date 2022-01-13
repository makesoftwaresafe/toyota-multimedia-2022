/******************************************************************************
 * @file: CAmClassActionSuspend.cpp
 *
 * This file contains the definition of user action suspend class
 * (member functions and data members) used to implement the logic of suspending
 * the connected connection at user level
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

#include "CAmLogger.h"
#include "CAmMainConnectionActionSuspend.h"
#include "CAmMainConnectionActionSetVolume.h"
#include "CAmClassElement.h"
#include "CAmClassActionSuspend.h"

namespace am {
namespace gc {

CAmClassActionSuspend::CAmClassActionSuspend(CAmClassElement *pClassElement) :
                                CAmActionContainer(std::string("CAmClassActionSuspend")),
                                mpClassElement(pClassElement),
                                mOrderParam(DEFAULT_CONNECT_ORDER)
{
    this->_registerParam(ACTION_PARAM_SOURCE_NAME, &mSourceNameParam);
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_ORDER, &mOrderParam);
}

CAmClassActionSuspend::~CAmClassActionSuspend(void)
{
}

int CAmClassActionSuspend::_execute(void)
{
    std::string sourceName;
    std::string sinkName;
    CAmMainConnectionElement* pMainConnectionElement;
    CAmMainConnectionElement* pConnectedConnection;
    /*
     * First check if suspended connection already exists in the queue
     */
    std::vector < am_ConnectionState_e > listConnectionStates = { CS_SUSPENDED };
    pMainConnectionElement = mpClassElement->getMainConnection(sourceName, sinkName, listConnectionStates);
    if (pMainConnectionElement != NULL)
    {
        LOG_FN_ERROR(__FILENAME__, __func__, "Suspended connection already present in the queue");
        return E_OK;
    }

    /*
     * Next get the connection in connected state
     */
    listConnectionStates = { CS_CONNECTED };

    pConnectedConnection = mpClassElement->getMainConnection(sourceName, sinkName, listConnectionStates);
    gc_Order_e order;
    mOrderParam.getParam(order);
    mSourceNameParam.getParam(sourceName);
    mSinkNameParam.getParam(sinkName);
    listConnectionStates = {CS_CONNECTED, CS_DISCONNECTED};
    pMainConnectionElement = mpClassElement->getMainConnection(sourceName, sinkName, listConnectionStates, order);
    if ((pConnectedConnection != NULL) &&
       (pMainConnectionElement!= NULL) &&
       (pMainConnectionElement != pConnectedConnection))
    {
        LOG_FN_ERROR(__FILENAME__, __func__, "Not allowed to have CS_CONNECTED as well as CS_SUSPENDED");
        return E_OK;
    }
    if (pMainConnectionElement != NULL)
    {
        IAmActionCommand* pAction = new CAmMainConnectionActionSuspend(pMainConnectionElement);
        if (NULL != pAction)
        {
            pAction->setUndoRequried(true);
            CAmActionParam<gc_SetSourceStateDirection_e> setSourceStateDir;
            setSourceStateDir.setParam(classTypeToDisconnectDirectionLUT[mpClassElement->getClassType()]);
            pAction->setParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&setSourceStateDir);
            append(pAction);
        }
        pAction = _createActionSetLimitState(pMainConnectionElement);
        if (NULL != pAction)
        {
            append(pAction);
        }
    }
    else
    {
        LOG_FN_ERROR(__FILENAME__, __func__, "Main Connection not found with matching criteria");
    }
    return E_OK;
}

int CAmClassActionSuspend::_update(const int result)
{
    mpClassElement->updateMainConnectionQueue();
}

IAmActionCommand* CAmClassActionSuspend::_createActionSetLimitState(
                CAmMainConnectionElement* pMainConnection)
{
    IAmActionCommand* pAction = new CAmMainConnectionActionSetVolume(pMainConnection);
    if (NULL != pAction)
    {
        CAmActionParam < gc_LimitType_e > limitTypeParam(LT_UNKNOWN);
        pAction->setParam(ACTION_PARAM_LIMIT_STATE, &limitTypeParam);
        pAction->setUndoRequried(true);
    }
    return pAction;
}

} /* namespace gc */
} /* namespace am */
