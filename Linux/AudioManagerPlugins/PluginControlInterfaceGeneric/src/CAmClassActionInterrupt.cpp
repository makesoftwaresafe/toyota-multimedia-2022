/******************************************************************************
 * @file: CAmClassActionInterrupt.cpp
 *
 * This file contains the definition of user action interrupt class (member functions
 * and data members) used to implement the logic of pushing (disconnect) the connection
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
#include "CAmClassActionInterrupt.h"
#include "CAmMainConnectionElement.h"
#include "CAmMainConnectionActionDisconnect.h"
#include "CAmMainConnectionActionSetVolume.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmClassActionInterrupt::CAmClassActionInterrupt(CAmClassElement *pClassElement) :
                                CAmActionContainer(std::string("CAmClassActionInterrupt")),
                                mpClassElement(pClassElement)

{
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_SOURCE_NAME, &mSourceNameParam);
}

CAmClassActionInterrupt::~CAmClassActionInterrupt()
{
}

int CAmClassActionInterrupt::_execute(void)
{
    std::string sourceName;
    std::string sinkName;
    mSourceNameParam.getParam(sourceName);
    mSinkNameParam.getParam(sinkName);
    std::vector<CAmMainConnectionElement* > listMainConnections;
    std::vector < am_ConnectionState_e > listConnectionStates {CS_CONNECTED , CS_SUSPENDED};
    CAmConnectionListFilter filterObject;
    filterObject.setSinkName(sinkName);
    filterObject.setSourceName(sourceName);
    filterObject.setListConnectionStates(listConnectionStates);
    mpClassElement->getListMainConnections(listMainConnections,filterObject);
    if ((false == sourceName.empty()) && (false == sinkName.empty())
            && (true == listMainConnections.empty()))
    {
        am_mainConnectionID_t mainConnectionID;
        am_Error_e result;
        result = mpClassElement->createMainConnection(sourceName, sinkName, mainConnectionID);
        return result;
    }

    IAmActionCommand* pAction(NULL);
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    for (itListMainConnections = listMainConnections.begin();
                    itListMainConnections != listMainConnections.end(); ++itListMainConnections)
    {
        mpClassElement->pushMainConnectionInQueue((*itListMainConnections));
        pAction = new CAmMainConnectionActionDisconnect(*itListMainConnections);
        if (NULL != pAction)
        {
           CAmActionParam<gc_SetSourceStateDirection_e> setSourceStateDir;
           if(mpClassElement->getClassType()==C_CAPTURE)
           {
               setSourceStateDir.setParam(SD_MAINSINK_TO_MAINSOURCE);
           }
           else
           {
               setSourceStateDir.setParam(SD_MAINSOURCE_TO_MAINSINK);
           }
           pAction->setParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&setSourceStateDir);
            append(pAction);
        }

        pAction = _createActionSetLimitState(*itListMainConnections);
        if (NULL != pAction)
        {
            append(pAction);
        }
    }
    return E_OK;
}

IAmActionCommand* CAmClassActionInterrupt::_createActionSetLimitState(
                CAmMainConnectionElement* pMainConnection)
{
    IAmActionCommand* pAction = new CAmMainConnectionActionSetVolume(pMainConnection);
    if (NULL != pAction)
    {
        CAmActionParam < std::map<uint32_t, gc_LimitVolume_s > > mapLimitsParam;
        pAction->setParam(ACTION_PARAM_LIMIT_MAP, &mapLimitsParam);
        pAction->setUndoRequried(true);
    }
    return pAction;
}

} /* namespace gc */
} /* namespace am */
