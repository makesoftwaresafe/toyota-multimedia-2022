/******************************************************************************
 * @file: CAmSourceActionSetState.cpp
 *
 * This file contains the definition of router action set source state class
 * (member functions and data members) used to implement the logic of setting
 * the source state at router level
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

#include "CAmSourceActionSetState.h"
#include "CAmControlReceive.h"
#include "CAmSourceElement.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmSourceActionSetState::CAmSourceActionSetState(CAmSourceElement* pSourceElement) :
                                CAmActionCommand(std::string("CAmSourceActionSetState")),
                                mpSourceElement(pSourceElement),
                                mState(SS_UNKNNOWN),
                                mOldState(SS_UNKNNOWN)
{
    this->_registerParam(ACTION_PARAM_SOURCE_STATE, &mSourceStateParam);
}

CAmSourceActionSetState::~CAmSourceActionSetState()
{
}

int CAmSourceActionSetState::_execute(void)
{
    am_Error_e result;
    if ((NULL == mpSourceElement) || (false == mSourceStateParam.getParam(mState)))
    {
        LOG_FN_ERROR(__FILENAME__,__func__," Parameters not set");
        //set the error to invoke undo process
        return E_NOT_POSSIBLE;
    }

    mpSourceElement->getState((int&)mOldState);
    if (mOldState == mState)
    {
        return E_OK;
    }

    CAmControlReceive* pControlReceive = mpSourceElement->getControlReceive();
    result = pControlReceive->setSourceState(mpSourceElement->getID(), mState);
    if (E_OK != result)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Failed to set source state:", result);
        //set the error to invoke undo process
        return result;
    }
    // register the observer
    pControlReceive->registerObserver(this);
    //store the old state in case undo is required

    return E_WAIT_FOR_CHILD_COMPLETION;
}

int CAmSourceActionSetState::_undo(void)
{
    am_Error_e result;
    CAmControlReceive* pControlReceive = mpSourceElement->getControlReceive();

    // register the observer
    pControlReceive->registerObserver(this);
    result = pControlReceive->setSourceState(mpSourceElement->getID(), mOldState);
    if (E_OK != result)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  Failed to set source state:", result);
        //unregister the observer as its update will not be invoked.
        pControlReceive->unregisterObserver(this);
        return result;
    }
    return E_WAIT_FOR_CHILD_COMPLETION;
}

int CAmSourceActionSetState::_update(const int result)
{
    (void)result;
    //unregister the observer
    CAmControlReceive* pControlReceive = mpSourceElement->getControlReceive();

    pControlReceive->unregisterObserver(this);
    return E_OK;
}

void CAmSourceActionSetState::_timeout(void)
{
    CAmControlReceive* pControlReceive = mpSourceElement->getControlReceive();
    pControlReceive->abortAction();
}

} /* namespace gc */
} /* namespace am */
