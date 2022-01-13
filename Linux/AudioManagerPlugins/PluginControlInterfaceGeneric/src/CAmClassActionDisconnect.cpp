/******************************************************************************
 * @file: CAmClassActionDisconnect.cpp
 *
 * This file contains the definition of user connection action disconnect class
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

#include "CAmLogger.h"
#include "CAmMainConnectionElement.h"
#include "CAmMainConnectionActionDisconnect.h"
#include "CAmClassElement.h"
#include "CAmClassActionDisconnect.h"

namespace am {
namespace gc {

CAmClassActionDisconnect::CAmClassActionDisconnect(CAmClassElement *pClassElement) :
                                CAmActionContainer(std::string("CAmClassActionDisconnect")),
                                mpClassElement(pClassElement)
{
    std::vector < am_ConnectionState_e > listConnectionStates {CS_CONNECTED};
    mListConnectionStatesParam.setParam(listConnectionStates);
    this->_registerParam(ACTION_PARAM_SOURCE_NAME, &mSourceNameParam);
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_EXCEPT_SOURCE_NAME, &mlistSourceExceptionsParam);
    this->_registerParam(ACTION_PARAM_EXCEPT_SINK_NAME, &mListSinkExceptionsParam);
    this->_registerParam(ACTION_PARAM_CONNECTION_STATE, &mListConnectionStatesParam);
}

CAmClassActionDisconnect::~CAmClassActionDisconnect()
{
}

int CAmClassActionDisconnect::_execute(void)
{
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    std::string sinkName;
    std::string sourceName;
    std::vector < am_ConnectionState_e > listConnectionStates;
    std::vector < std::string > listExceptSources;
    std::vector < std::string > listExceptSinks;
    LOG_FN_ENTRY(__FILENAME__,__func__);
    // Based on the parameter get the list of the connections on which action to be taken
    mSinkNameParam.getParam(sinkName);
    mSourceNameParam.getParam(sourceName);
    mListConnectionStatesParam.getParam(listConnectionStates);
    mlistSourceExceptionsParam.getParam(listExceptSources);
    mListSinkExceptionsParam.getParam(listExceptSinks);
    CAmConnectionListFilter filterObject;
    filterObject.setSourceName(sourceName);
    filterObject.setSinkName(sinkName);
    filterObject.setListConnectionStates(listConnectionStates);
    filterObject.setListExceptSinkNames(listExceptSinks);
    filterObject.setListExceptSourceNames(listExceptSources);
    mpClassElement->getListMainConnections(mpListMainConnections,filterObject);
    LOG_FN_INFO(__FILENAME__,__func__,"Number of connection to disconnect=", mpListMainConnections.size());

    // Finally from the list of the connections create the child actions

    IAmActionCommand* pAction(NULL);
    for (itListMainConnections = mpListMainConnections.begin();
                    itListMainConnections != mpListMainConnections.end(); ++itListMainConnections)
    {
        pAction = new CAmMainConnectionActionDisconnect(*itListMainConnections);
        if (NULL != pAction)
        {
            CAmActionParam<gc_SetSourceStateDirection_e> setSourceStateDir;
            setSourceStateDir.setParam(classTypeToDisconnectDirectionLUT[mpClassElement->getClassType()]);
            pAction->setParam(ACTION_PARAM_SET_SOURCE_STATE_DIRECTION,&setSourceStateDir);
            append(pAction);
        }

    }
    return E_OK;
}

int CAmClassActionDisconnect::_update(const int result)
{
    std::vector<CAmMainConnectionElement* >::iterator itListMainConnections;
    int state;
    for (itListMainConnections = mpListMainConnections.begin();
                    itListMainConnections != mpListMainConnections.end(); ++itListMainConnections)
    {
        if ((*itListMainConnections) != NULL)
        {
            (*itListMainConnections)->getState(state);
            if (state == CS_DISCONNECTED)
            {
                mpClassElement->disposeConnection((*itListMainConnections));
                *itListMainConnections = NULL;
            }
        }
    }
    return E_OK;
}

} /* namespace gc */
} /* namespace am */

