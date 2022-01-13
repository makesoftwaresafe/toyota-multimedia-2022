/******************************************************************************
 * @file: CAmSourceActionSetVolume.cpp
 *
 * This file contains the definition of router action set volume source
 * (member functions and data members) used to implement the logic of setting
 * the volume of element at router level
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

#include "CAmSourceActionSetVolume.h"
#include "CAmControlReceive.h"
#include "CAmSourceElement.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmSourceActionSetVolume::CAmSourceActionSetVolume(CAmSourceElement* pSource) :
                                CAmActionCommand(std::string("CAmSourceActionSetVolume")),
                                mpSource(pSource),
                                mOldVolume(0), mOldLimitVolume(0),
                                mRequestedVolume(0), mRequestedLimitVolume(0),
                                mRampTypeParam(DEFAULT_RAMP_TYPE),
                                mRampTimeParam(DEFAULT_RAMP_TIME)
{
    this->_registerParam(ACTION_PARAM_RAMP_TIME, &mRampTimeParam);
    this->_registerParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);
    this->_registerParam(ACTION_PARAM_VOLUME, &mVolumeParam);
    this->_registerParam(ACTION_PARAM_VOLUME_STEP, &mVolumeStepParam);
    this->_registerParam(ACTION_PARAM_LIMIT_VOLUME, &mLimitVolumeParam);
}

CAmSourceActionSetVolume::~CAmSourceActionSetVolume()
{
}

int CAmSourceActionSetVolume::_execute(void)
{
    am_volume_t volumeStep = 0;
    if ((false == mLimitVolumeParam.getParam(mRequestedLimitVolume))
        && (false == mVolumeParam.getParam(mRequestedVolume)) &&
           (false == mVolumeStepParam.getParam(mRequestedVolume)))
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"parameters not set properly");
        return E_NOT_POSSIBLE;
    }
    // remember old volume
    mOldVolume = mpSource->getVolume();
    mOldLimitVolume = mpSource->getOffsetVolume();
    mRequestedVolume = mOldVolume;
    mRequestedLimitVolume = mOldLimitVolume;

    mVolumeParam.getParam(mRequestedVolume);
    if(true == mVolumeStepParam.getParam(volumeStep))
    {
        mRequestedVolume += volumeStep;
    }
    mRequestedVolume = std::min(std::max(mRequestedVolume,mpSource->getMinVolume()),
                mpSource->getMaxVolume());
    mLimitVolumeParam.getParam(mRequestedLimitVolume);

    if((mOldVolume + mOldLimitVolume)== (mRequestedVolume+mRequestedLimitVolume))
    {
        mpSource->setVolume(mRequestedVolume);
        mpSource->setOffsetVolume(mRequestedLimitVolume);
        return E_OK;
    }
    return _setRoutingSideVolume(mRequestedVolume,mRequestedLimitVolume);
}

int CAmSourceActionSetVolume::_setRoutingSideVolume(am_volume_t volume, am_volume_t limitVolume)
{
    am_CustomRampType_t rampType;
    am_time_t rampTime;
    int result;
    CAmControlReceive* pControlReceive = mpSource->getControlReceive();

    mRampTypeParam.getParam(rampType);
    mRampTimeParam.getParam(rampTime);
    /*
     * based on the volume and limit volume to be set
     */
    result = pControlReceive->setSourceVolume(mpSource->getID(),
                          mpSource->getRoutingSideVolume(volume + limitVolume),
                          rampType, rampTime);
    if (result == E_OK)
    {
        pControlReceive->registerObserver(this);
        result = E_WAIT_FOR_CHILD_COMPLETION;
    }
    return result;
}

int CAmSourceActionSetVolume::_update(const int result)
{
    if(result == E_OK)
    {
        mpSource->setVolume(mRequestedVolume);
        mpSource->setOffsetVolume(mRequestedLimitVolume);
    }
    mpSource->getControlReceive()->unregisterObserver(this);
    return E_OK;
}

int CAmSourceActionSetVolume::_undo(void)
{
    return _setRoutingSideVolume(mOldVolume,mOldLimitVolume);
}

} /* namespace gc */
} /* namespace am */
