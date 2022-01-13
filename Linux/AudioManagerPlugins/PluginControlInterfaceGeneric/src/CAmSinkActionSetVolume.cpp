/******************************************************************************
 * @file: CAmSinkActionSetVolume.cpp
 *
 * This file contains the definition of user action set volume sink
 * (member functions and data members) used to implement the logic of setting
 * the volume of element at user level
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

#include "CAmSinkActionSetVolume.h"
#include "CAmControlReceive.h"
#include "CAmSinkElement.h"
#include "CAmSourceActionSetVolume.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmSinkActionSetVolume::CAmSinkActionSetVolume(CAmSinkElement *pSinkElement) :
                                CAmActionCommand(std::string("CAmSinkActionSetVolume")),
                                mpSink(pSinkElement),
                                mOldVolume(0), mOldOffsetVolume(0),mOldSinkMute(0),
                                mRequestedVolume(0), mRequestedLimitVolume(0),mRequestedSinkLimitVolume(0),
                                mRampTimeParam(DEFAULT_RAMP_TIME),
                                mRampTypeParam(DEFAULT_RAMP_TYPE)
{
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_MAIN_VOLUME, &mMainVolumeParam);
    this->_registerParam(ACTION_PARAM_MAIN_VOLUME_STEP, &mMainVolumeStepParam);
    this->_registerParam(ACTION_PARAM_VOLUME, &mVolumeParam);
    this->_registerParam(ACTION_PARAM_VOLUME_STEP, &mVolumeStepParam);
    this->_registerParam(ACTION_PARAM_LIMIT_VOLUME, &mLimitVolumeParam);
    this->_registerParam(ACTION_PARAM_RAMP_TIME, &mRampTimeParam);
    this->_registerParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);
}

CAmSinkActionSetVolume::~CAmSinkActionSetVolume()
{
}

int CAmSinkActionSetVolume::_execute(void)
{
    am_mainVolume_t mainVolume = 0;
    am_mainVolume_t mainStepVolume = 0;
    std::string name;
    bool mainVolumeRequested = false;
    am_volume_t volume = 0;
    am_volume_t volumeStep = 0;
    am_volume_t offsetVolume = 0;
    am_volume_t sinkLimitVolume = 0;
    mOldVolume = mpSink->getVolume();
    mOldOffsetVolume = mpSink->getOffsetVolume();
    mOldSinkMute = mpSink->getSinkLimit().limitVolume;
    LOG_FN_DEBUG("mOldVolume,mOldOffsetVolume,mOldSinkMute",mOldVolume,mOldOffsetVolume,mOldSinkMute);
    if(mSinkNameParam.getParam(name) == true)
    {
        mLimitVolumeParam.getParam(sinkLimitVolume);
        offsetVolume = mOldOffsetVolume;
    }
    else
    {
        mLimitVolumeParam.getParam(offsetVolume);
        sinkLimitVolume = mOldSinkMute;
    }

    mainVolume = mpSink->getMainVolume();
    volume = mOldVolume;
    if((true == mMainVolumeParam.getParam(mainVolume)) ||
       (true == mMainVolumeStepParam.getParam(mainStepVolume)))
    {
        mainVolume += mainStepVolume;
        mainVolumeRequested = true;
    }

    if (true == mainVolumeRequested)
    {
        /*
         * This is a request to set the mainvolume from user side or policy side
         */
        volume = mpSink->convertMainVolumeToVolume(mainVolume);
    }
    else
    {
        if((true == mVolumeParam.getParam(volume)) ||
           (true == mMainVolumeStepParam.getParam(volumeStep)))
        {
            volume += volumeStep;
        }
    }
    LOG_FN_DEBUG("Requested Volume,OffsetVolume,SinkMute",volume,offsetVolume,sinkLimitVolume);
    volume = std::min(std::max(volume,mpSink->getMinVolume()),mpSink->getMaxVolume());
    if((mOldSinkMute == sinkLimitVolume) &&
       ((mOldVolume + mOldOffsetVolume) == (volume + offsetVolume))
      )
    {
        mpSink->setVolume(volume);
        mpSink->setOffsetVolume(offsetVolume);
        return E_OK;
    }
    return _setRoutingSideVolume(volume, offsetVolume, sinkLimitVolume);
}

int CAmSinkActionSetVolume::_setRoutingSideVolume(am_volume_t volume,
                                                  am_volume_t offsetVolume,
                                                  am_volume_t sinkMute)
{
    am_CustomRampType_t rampType;
    am_time_t rampTime;
    int result;
    CAmControlReceive* pControlReceive = mpSink->getControlReceive();
    mRampTypeParam.getParam(rampType);
    mRampTimeParam.getParam(rampTime);
    mRequestedVolume = volume;
    mRequestedLimitVolume = offsetVolume;
    mRequestedSinkLimitVolume = sinkMute;

    if((offsetVolume == AM_MUTE) || (sinkMute == AM_MUTE))
    {
        volume = AM_MUTE;
    }
    else
    {
        volume += offsetVolume;
        volume = std::min(std::max(volume,
                                mpSink->getMinVolume()), mpSink->getMaxVolume());
    }

    result = pControlReceive->setSinkVolume(mpSink->getID(), volume, rampType, rampTime);
    if (result == E_OK)
    {
        pControlReceive->registerObserver(this);
        result = E_WAIT_FOR_CHILD_COMPLETION;
    }
    return result;
}
int CAmSinkActionSetVolume::_update(const int result)
{
    std::string name;
    gc_LimitVolume_s limit;
    mpSink->getControlReceive()->unregisterObserver(this);
    if (((E_OK == result) && (AS_COMPLETED == getStatus()))
         || (AS_UNDO_COMPLETE == getStatus()))
    {
        if ((mRequestedLimitVolume == AM_MUTE)||(mRequestedSinkLimitVolume == AM_MUTE))
        {
            mpSink->setMuteState(MS_MUTED);
        }
        else
        {
            mpSink->setMuteState(MS_UNMUTED);
        }
        if(mSinkNameParam.getParam(name) == true)
        {
            limit.limitType = (mRequestedSinkLimitVolume == AM_MUTE)? LT_ABSOLUTE : LT_UNKNOWN;
            limit.limitVolume = (mRequestedSinkLimitVolume == AM_MUTE)? AM_MUTE : AM_VOLUME_NO_LIMIT;
            mpSink->setSinkLimit(limit);
        }

        LOG_FN_INFO(__FILENAME__,__func__,"Volume,Limit",mRequestedVolume,mRequestedLimitVolume);
        mpSink->setVolume(mRequestedVolume);
        mpSink->setOffsetVolume(mRequestedLimitVolume);
    }
    mpSink->getControlReceive()->unregisterObserver(this);
    return E_OK;
}

int CAmSinkActionSetVolume::_undo(void)
{
    return _setRoutingSideVolume(mOldVolume, mOldOffsetVolume, mOldSinkMute);
}

} /* namespace gc */
} /* namespace am */
