/******************************************************************************
 * @file: CAmClassActionConnect.cpp
 *
 * This file contains the definition of user connection action connect class
 * (member functions and data members) used to implement the logic of disconnect
 * at user level
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

#include "CAmClassElement.h"
#include "CAmClassActionConnect.h"
#include "CAmSinkElement.h"
#include "CAmMainConnectionElement.h"
#include "CAmMainConnectionActionConnect.h"
#include "CAmMainConnectionActionSetVolume.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmClassActionConnect::CAmClassActionConnect(CAmClassElement *pClassElement) :
                                CAmActionContainer(std::string("CAmClassActionConnect")),
                                mpClassElement(pClassElement),
                                mpMainConnectionElement(NULL),
                                mOrderParam(DEFAULT_CONNECT_ORDER)

{
    this->_registerParam(ACTION_PARAM_SOURCE_NAME, &mSourceNameParam);
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_ORDER, &mOrderParam);
    this->_registerParam(ACTION_PARAM_CONNECTION_FORMAT, &mConnectionFormatParam);
}

CAmClassActionConnect::~CAmClassActionConnect()
{
}

int CAmClassActionConnect::_execute(void)
{
    am_Error_e result;
    result = _getMainConnectionElement();
    if ((E_OK != result) && (E_ALREADY_EXISTS != result))
    {
        return result;
    }
    if (NULL != mpMainConnectionElement)
    {
        IAmActionCommand* pAction(NULL);
        pAction = _createActionMainConnectionSetVolume(mpMainConnectionElement);
        if (NULL != pAction)
        {
            append(pAction);
        }
        pAction = new CAmMainConnectionActionConnect(mpMainConnectionElement);
        if (NULL != pAction)
        {
            pAction->setParam(ACTION_PARAM_CONNECTION_FORMAT, &mConnectionFormatParam);
            pAction->setUndoRequried(true);
            CAmActionParam<gc_SetSourceStateDirection_e> setSourceStateDir;
            setSourceStateDir.setParam(classTypeToConnectDirectionLUT[mpClassElement->getClassType()]);
            pAction->setParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&setSourceStateDir);
            append(pAction);
        }
    }
    return E_OK;
}

int CAmClassActionConnect::_update(const int result)
{
    if (this->getStatus() == AS_UNDO_COMPLETE)
    {
        if (NULL != mpMainConnectionElement)
        {
            mpClassElement->disposeConnection(mpMainConnectionElement);
        }
    }
    if ((mpMainConnectionElement !=NULL) && (this->getStatus() == AS_COMPLETED ))
    {
        int state = CS_UNKNOWN;
        mpMainConnectionElement->getState(state);
        if (CS_CONNECTED == state)
        {
            mpClassElement->setLastVolume(mpMainConnectionElement);
        }
    }
    mpClassElement->updateMainConnectionQueue();
    return E_OK;
}

am_Error_e CAmClassActionConnect::_getMainConnectionElement()
{
    std::string sourceName;
    std::string sinkName;
    CAmMainConnectionElement* pSuspendedConnection;
    /*
     * First check if Connected connection already exists in the queue
     */
    std::vector < am_ConnectionState_e > listConnectionStates = { CS_CONNECTED };
    /*
     * The name of this variable is confusing since here it holds the pointer to
     * connected connection, since this variable is reused for suspended connection
     * its named as pSuspended.
     */
    pSuspendedConnection = mpClassElement->getMainConnection(sourceName, sinkName, listConnectionStates);
    if (pSuspendedConnection != NULL)
    {
        LOG_FN_ERROR(__FILENAME__, __func__, "Connected connection already present in the queue");
        return E_OK;
    }

    /*
     * Next get the suspended connection
     */
    listConnectionStates = { CS_SUSPENDED };
    /*
     * Here the variable holds the suspended connection pointer
     */
    pSuspendedConnection = mpClassElement->getMainConnection(sourceName, sinkName, listConnectionStates);

    mSourceNameParam.getParam(sourceName);
    mSinkNameParam.getParam(sinkName);
    if (!sourceName.empty() && !sinkName.empty())
    {
        am_mainConnectionID_t mainConnectionID;
        am_Error_e result;
        result = mpClassElement->createMainConnection(sourceName, sinkName, mainConnectionID);
        if ((E_OK != result) && (E_ALREADY_EXISTS != result))
        {
            return result;
        }
    }
    gc_Order_e order;
    mOrderParam.getParam(order);
    listConnectionStates = {CS_DISCONNECTED, CS_SUSPENDED};
    mpMainConnectionElement = mpClassElement->getMainConnection(sourceName, sinkName, listConnectionStates, order);
    /*
     * If the suspended connection found earlier and the connection which we are planning to
     * connect are  not same then we will end up with the situation that we have suspended
     * as well as connected connection in the same queue.
     */
    if ((pSuspendedConnection !=NULL ) &&(mpMainConnectionElement!=NULL)&&(pSuspendedConnection!=mpMainConnectionElement))
    {
        mpMainConnectionElement = NULL;
        LOG_FN_ERROR(__FILENAME__, __func__, "either suspended or conncted only one allowed");
    }
    return E_OK;
}

IAmActionCommand* CAmClassActionConnect::_createActionMainConnectionSetVolume(
                CAmMainConnectionElement* pMainConnection)
{
    IAmActionCommand* pAction = new CAmMainConnectionActionSetVolume(pMainConnection);
    if (NULL != pAction)
    {
        std::map<uint32_t, gc_LimitVolume_s > mapLimits;
        CAmActionParam < std::map<uint32_t, gc_LimitVolume_s > > mapLimitsParam;
        am_mainVolume_t mainVolume;
        mpClassElement->getListClassLimits(mapLimits);
        if( false == mapLimits.empty())
        {
            mapLimitsParam.setParam(mapLimits);

            pAction->setParam(ACTION_PARAM_LIMIT_MAP, &mapLimitsParam);
        }
        std::string sinkName = pMainConnection->getMainSink()->getName();

        mainVolume = pMainConnection->getMainSink()->convertVolumeToMainVolume(pMainConnection->getVolume());
        mpClassElement->getLastVolume(pMainConnection,mainVolume);
        LOG_FN_DEBUG(getName(),"last volume=",mainVolume);
        {
            CAmActionParam < am_mainVolume_t > mainVolumeParam;
            mainVolumeParam.setParam(mainVolume);
            pAction->setParam(ACTION_PARAM_MAIN_VOLUME,&mainVolumeParam);
        }
        if(false == mpClassElement->isPerSinkClassVolumeEnabled())
        {
            CAmActionParam < bool > saveLastVolume(true);
            pAction->setParam(ACTION_PARAM_SAVE_LAST_VOLUME,&saveLastVolume);
        }
        pAction->setUndoRequried(true);
    }
    return pAction;
}

}
}
