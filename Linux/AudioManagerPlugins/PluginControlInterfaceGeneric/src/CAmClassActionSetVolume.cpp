/******************************************************************************
 * @file: CAmClassActionSetVolume.cpp
 *
 * This file contains the definition of user action set volume class
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

#include "CAmClassActionSetVolume.h"
#include "CAmClassElement.h"
#include "CAmSourceElement.h"
#include "CAmSinkElement.h"
#include "CAmMainConnectionElement.h"
#include "CAmMainConnectionActionSetVolume.h"
#include "CAmSinkActionSetVolume.h"
#include "CAmSourceActionSetVolume.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmClassActionSetVolume::CAmClassActionSetVolume(CAmClassElement *pClassElement) :
                                CAmActionContainer(std::string("CAmClassActionSetVolume")),
                                mDirectSinkAction(false),
                                mTargetMainConnectionName(""),
                                mpClassElement(pClassElement),
                                mRampTimeParam(DEFAULT_RAMP_TIME),
                                mRampTypeParam(DEFAULT_RAMP_TYPE)
{
    this->_registerParam(ACTION_PARAM_CLASS_NAME, &mClassNameParam);
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_SOURCE_NAME, &mSourceNameParam);
    this->_registerParam(ACTION_PARAM_CONNECTION_NAME, &mConnectionNameParam);

    this->_registerParam(ACTION_PARAM_MAIN_VOLUME,&mMainVolumeParam);
    this->_registerParam(ACTION_PARAM_MAIN_VOLUME_STEP,&mMainVolumeStepParam);
    this->_registerParam(ACTION_PARAM_VOLUME, &mVolumeParam);
    this->_registerParam(ACTION_PARAM_VOLUME_STEP, &mVolumeStepParam);
    this->_registerParam(ACTION_PARAM_RAMP_TIME, &mRampTimeParam);
    this->_registerParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);

    this->_registerParam(ACTION_PARAM_LIMIT_STATE, &mOpRequestedParam);
    this->_registerParam(ACTION_PARAM_PATTERN, &mPatternParam);
}

CAmClassActionSetVolume::~CAmClassActionSetVolume()
{

}

int CAmClassActionSetVolume::_execute(void)
{
    gc_LimitType_e limitType;
    gc_LimitState_e limitState = LS_UNKNWON;
    uint32_t pattern = 0;
    std::string name;

    if (true == mSourceNameParam.getParam(name))
    {
        return _createSourceSinkActions();
    }

    if(true == _SinkMuteAction())
    {
        return _createSourceSinkActions();
    }

    CAmMainConnectionElement* pTargetMainConnection = _getTargetConnection();
    if(NULL == pTargetMainConnection)
    {
        if(false == mSinkNameParam.getParam(name))
        {
            LOG_FN_ERROR(__FILENAME__,__func__,"Failed to get the target of the set Volume");
        }
        else
        {
            CAmSinkElement* pSink = CAmSinkFactory::getElement(name);
            if(pSink->getInUse() == 0)
            {
                /*
                 * Create the sink and source actions directly
                 */
                return _createSourceSinkActions();
            }
            else
            {
                LOG_FN_INFO(__FILENAME__,__func__,"Sink already in use");
            }
        }
    }

    /*
     * If limit volume is requested add this to the class element
     */
    if(true == mOpRequestedParam.getParam(limitState))
    {
        gc_LimitVolume_s limitVolume;
        limitVolume.limitType = LT_UNKNOWN;
        limitVolume.limitVolume = 0;
        if(limitState == LS_LIMITED)
        {
            if(true == mVolumeParam.getParam((limitVolume.limitVolume)))
            {
                limitVolume.limitType = LT_ABSOLUTE;
            }
            else if(true == mVolumeStepParam.getParam((limitVolume.limitVolume)))
            {
                limitVolume.limitType = LT_RELATIVE;
            }
            else
            {
                LOG_FN_ERROR(__FILENAME__,__func__,"Invalid parameter for limit\n");
            }
        }
        mPatternParam.getParam(pattern);
        mpClassElement->setLimitState(limitState, limitVolume, pattern);
    }

    return _createChildActions(pTargetMainConnection);
}

CAmMainConnectionElement* CAmClassActionSetVolume::_getTargetConnection(void)
{
    std::string mainConnectionName;
    CAmMainConnectionElement* pMainConnection=NULL;
    if(true == mConnectionNameParam.getParam(mainConnectionName))
    {
        LOG_FN_DEBUG(__FILENAME__,__func__,"FINDING connection from Name");
        pMainConnection = CAmMainConnectionFactory::getElement(mainConnectionName);
        return pMainConnection;
    }
    else
    {
        std::string sinkName="";
        mSinkNameParam.getParam(sinkName);
        CAmSinkElement* pSink = CAmSinkFactory::getElement(sinkName);
        LOG_FN_DEBUG(__FILENAME__,__func__,"FINDING active connection SinkName=",sinkName);
        std::vector < am_ConnectionState_e > listConnectionStates {CS_CONNECTED};
        pMainConnection = mpClassElement->getMainConnection(std::string(""), sinkName, listConnectionStates);
        /*
         * If connected main connection is not found then search using a sink
         * which is is CS_DISCONNECTED or CS_SUSPENDED but only part of single
         * connection
         */
        if((pMainConnection == NULL) &&
           (pSink!=NULL) && (pSink->getInUse() < 2))
        {
            listConnectionStates.push_back(CS_DISCONNECTED);
            listConnectionStates.push_back(CS_SUSPENDED);
            pMainConnection = mpClassElement->getMainConnection(std::string(""), sinkName, listConnectionStates);
        }
    }
    LOG_FN_EXIT(__FILENAME__,__func__,"No Main Connection found",pMainConnection);
    return pMainConnection;
}

am_Error_e CAmClassActionSetVolume::_createSourceSinkActions(void)
{
    std::string name;
    IAmActionCommand* pAction = NULL;
    gc_LimitState_e limitState;
    if(true == mSinkNameParam.getParam(name))
    {
        CAmSinkElement* pSink = CAmSinkFactory::getElement(name);
        pAction = new CAmSinkActionSetVolume(pSink);
        pAction->setParam(ACTION_PARAM_MAIN_VOLUME, &mMainVolumeParam);
        pAction->setParam(ACTION_PARAM_MAIN_VOLUME_STEP, &mMainVolumeStepParam);
        pAction->setParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    }

    if(true == mSourceNameParam.getParam(name))
    {
        CAmSourceElement* pSource = CAmSourceFactory::getElement(name);
        pAction = new CAmSourceActionSetVolume(pSource);
    }

    if(pAction != NULL)
    {
        if(true == mOpRequestedParam.getParam(limitState))
        {
            pAction->setParam(ACTION_PARAM_LIMIT_VOLUME, &mVolumeParam);
        }
        else
        {
            pAction->setParam(ACTION_PARAM_VOLUME, &mVolumeParam);
        }
        pAction->setParam(ACTION_PARAM_VOLUME_STEP, &mVolumeStepParam);
        pAction->setParam(ACTION_PARAM_RAMP_TIME, &mRampTimeParam);
        pAction->setParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);
        pAction->setUndoRequried(true);
        append(pAction);
    }
    mDirectSinkAction = true;
    return E_OK;
}

am_Error_e CAmClassActionSetVolume::_createChildActions(
                                    CAmMainConnectionElement* pMainConnection)
{
    std::string name;
    am_volume_t volume;
    am_mainVolume_t mainVolume;
    uint32_t pattern;
    gc_LimitType_e limitType;
    gc_LimitState_e limitState;

    if(pMainConnection == NULL)
    {
        return E_OK;
    }
    mTargetMainConnectionName = pMainConnection->getName();
    CAmMainConnectionActionSetVolume* pMainConnectionActionSetVolume =
            new CAmMainConnectionActionSetVolume(pMainConnection);
    //Set the parameters
    pMainConnectionActionSetVolume->setParam(ACTION_PARAM_SINK_NAME,&mSinkNameParam);
    pMainConnectionActionSetVolume->setParam(ACTION_PARAM_SOURCE_NAME,&mSourceNameParam);
    pMainConnectionActionSetVolume->setParam(ACTION_PARAM_MAIN_VOLUME,&mMainVolumeParam);
    pMainConnectionActionSetVolume->setParam(ACTION_PARAM_MAIN_VOLUME_STEP,&mMainVolumeStepParam);
    if(false == mOpRequestedParam.getParam(limitState))
    {
        pMainConnectionActionSetVolume->setParam(ACTION_PARAM_VOLUME, &mVolumeParam);
        pMainConnectionActionSetVolume->setParam(ACTION_PARAM_VOLUME_STEP, &mVolumeStepParam);
    }
    pMainConnectionActionSetVolume->setParam(ACTION_PARAM_RAMP_TIME, &mRampTimeParam);
    pMainConnectionActionSetVolume->setParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);
    /*
     * If none of the volume is set we assume it is last voluem request
     *
     */
    if( (false == mMainVolumeParam.getParam(mainVolume)) &&
        (false == mMainVolumeStepParam.getParam(mainVolume)) &&
        (false == mVolumeParam.getParam(volume)) &&
        (false == mVolumeStepParam.getParam(volume)) &&
        (false == mOpRequestedParam.getParam(limitState)))
    {
        // TODO : To decide if volume or mainvolume
        LOG_FN_DEBUG(__FILENAME__,__func__,"No Volume parameter set.");
        volume = mpClassElement->getVolume();
        CAmActionParam<am_volume_t> volumeParam(volume);
        pMainConnectionActionSetVolume->setParam(ACTION_PARAM_VOLUME,&volumeParam);
    }

    /*
     * If Limit parameter is set the save the limit in class
     */
    if(true == mOpRequestedParam.getParam(limitState))
    {
        std::map<uint32_t, gc_LimitVolume_s > limitMap;
        mpClassElement->getListClassLimits(limitMap);

            CAmActionParam< std::map<uint32_t, gc_LimitVolume_s > > mapLimitsParam(limitMap);
            pMainConnectionActionSetVolume->setParam(ACTION_PARAM_LIMIT_MAP,&mapLimitsParam);
    }
    pMainConnectionActionSetVolume->setUndoRequried(true);
    append(pMainConnectionActionSetVolume);

    return E_OK;
}

int CAmClassActionSetVolume::_update(int result)
{
    if (result != 0)
    {
        return 0;
    }

    std::string sinkName("");
    CAmSinkElement* pSink = NULL;
    am_mainVolume_t mainVolume = 0;
    if (true == mDirectSinkAction)
    {
        mSinkNameParam.getParam(sinkName);
        CAmSinkElement* pSink = CAmSinkFactory::getElement(sinkName);
        if (pSink != NULL)
        {
            mainVolume = pSink->convertVolumeToMainVolume(pSink->getVolume());
            pSink->setMainVolume(mainVolume);
            mpClassElement->setLastVolume(NULL,sinkName, mainVolume);
        }
    }
    else if (mTargetMainConnectionName != "")
    {
        CAmMainConnectionElement* pMainConnection = CAmMainConnectionFactory::getElement(mTargetMainConnectionName);
        if(pMainConnection != NULL)
        {
            mpClassElement->setLastVolume(pMainConnection);
        }
        else
        {
            LOG_FN_WARN(__FILENAME__,__func__,"Main Connection was not found");
        }
    }
    else
    {
        return 0;
    }

    return 0;
}

bool CAmClassActionSetVolume::_SinkMuteAction()
{
    bool sinkMuteAction = false;
    std::string name;
    gc_LimitState_e limitState;

    if (true == mSinkNameParam.getParam(name) &&
        true == mOpRequestedParam.getParam(limitState))
    {
        sinkMuteAction = true;
    }

    return sinkMuteAction;
}

} /* namespace gc */
} /* namespace am */
