/******************************************************************************
 * @file: CAmMainConnectionActionSetVolume.cpp
 *
 * This file contains the definition of main connection set volume class
 * (member functions and data members) used to implement the main connection
 * level set volume.
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

#include "CAmMainConnectionActionSetVolume.h"
#include "CAmSourceElement.h"
#include "CAmSinkElement.h"
#include "CAmMainConnectionElement.h"
#include "CAmSinkActionSetVolume.h"
#include "CAmSourceActionSetVolume.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmMainConnectionActionSetVolume::CAmMainConnectionActionSetVolume(CAmMainConnectionElement* pMainConnection) :
                                CAmActionContainer(std::string("CAmMainConnectionActionSetVolume")),
                                mpMainConnection(pMainConnection),
                                mMainConnectionName(pMainConnection->getName()),
                                mRampTimeParam(DEFAULT_RAMP_TIME),
                                mRampTypeParam(DEFAULT_RAMP_TYPE)
{
    this->_registerParam(ACTION_PARAM_SINK_NAME, &mSinkNameParam);
    this->_registerParam(ACTION_PARAM_SOURCE_NAME, &mSourceNameParam);

    this->_registerParam(ACTION_PARAM_VOLUME, &mVolumeParam);
    this->_registerParam(ACTION_PARAM_VOLUME_STEP, &mVolumeStepParam);
    this->_registerParam(ACTION_PARAM_MAIN_VOLUME,&mMainVolumeParam);
    this->_registerParam(ACTION_PARAM_MAIN_VOLUME_STEP,&mMainVolumeStepParam);
    this->_registerParam(ACTION_PARAM_LIMIT_MAP, &mMapLimitsParam);
    this->_registerParam(ACTION_PARAM_RAMP_TIME, &mRampTimeParam);
    this->_registerParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);
    this->_registerParam(ACTION_PARAM_SAVE_LAST_VOLUME,&mSaveLastVolumeParam);
}

CAmMainConnectionActionSetVolume::~CAmMainConnectionActionSetVolume()
{
}

int CAmMainConnectionActionSetVolume::_execute(void)
{
    am_Error_e result = E_NOT_POSSIBLE;
    CAmSourceElement* pSourceElement = NULL;
    CAmSinkElement* pSinkElement = NULL;
    std::map<CAmElement*, gc_volume_s > mapElementForSetVolume;
    std::string name;
    /*
     * First find the target of the volume
     * 1. Source/Sink :- Set the volume directly
     * 2. main connection: Get the target by using a method of main connection
     * element
     *
     */
    std::map<uint32_t, gc_LimitVolume_s > mapLimits;
    if(true == mMapLimitsParam.getParam(mapLimits))
    {
        mpMainConnection->setLimits(mapLimits);
    }
    if(true == mSinkNameParam.getParam(name))
    {
        pSinkElement = CAmSinkFactory::getElement(name);
        /*
         * If main Connection is limited no need to apply the sink volume
         */
        if( (pSinkElement->getInUse() > 1) &&
           (true ==mpMainConnection->isMainConnectionLimited()) &&
           (false == mpMainConnection->isMainConnectionMuted()))
        {
            return E_OK;
        }
    }
    if(true == mSourceNameParam.getParam(name))
    {
        pSourceElement = CAmSourceFactory::getElement(name);
    }

    /*
     * Get the volume
     * 1. In case main Volume/ step then use the main sink mapping table to convert to
     * volume.
     * 2. In case no volume parameter set the restore the last connection volume
     */
    am_volume_t requestedVolume;
    if(E_OK == _getRequestedVolume(requestedVolume))
    {
        mSaveLastVolumeParam.setParam(true);
    }
    else
    {
        requestedVolume = mpMainConnection->getVolume();
    }
    LOG_FN_DEBUG(__FILENAME__,__func__,"requested Volume",requestedVolume);
//    if((NULL == pSinkElement) && (NULL == pSourceElement) )
//    {
        /*
         * Its is a main Connection level volume request, call an API to
         * get the main connection element for setting volume
         */
        mpMainConnection->getVolumeChangeElements(requestedVolume,mapElementForSetVolume);
//    }
#if 0
    else
    {
        if(NULL != pSinkElement)
        {
            gc_volume_s volume;
            volume.isvolumeSet = true;
            volume.isOffsetSet = false;
            volume.volume = requestedVolume;
            mapElementForSetVolume.insert(std::make_pair(pSinkElement,volume));
            am_volume_t sinkVolume;
            sinkVolume = pSinkElement->getVolume();
            requestedMainConnectionVolume += (requestedVolume - sinkVolume);
        }
        if(NULL != pSourceElement)
        {
            gc_volume_s volume;
            volume.isvolumeSet = true;
            volume.isOffsetSet = false;
            volume.volume = requestedVolume;
            mapElementForSetVolume.insert(std::make_pair(pSourceElement,volume));

            am_volume_t sourceVolume;
            sourceVolume = pSourceElement->getVolume();
            requestedMainConnectionVolume += (requestedVolume - sourceVolume);
        }
    }
#endif
    /**
     * Finally create the actions
     */
    result = _createChildActions(mapElementForSetVolume);
    return result;
}

am_Error_e CAmMainConnectionActionSetVolume::_getRequestedVolume(am_volume_t& volume)
{
    CAmSinkElement* pSinkElement = mpMainConnection->getMainSink();
    am_mainVolume_t requestedMainVolume=0;
    if( (true == mMainVolumeParam.getParam(requestedMainVolume)) || \
        (true == mMainVolumeStepParam.getParam(requestedMainVolume)))
    {
        if((false == mMainVolumeParam.getParam(requestedMainVolume)) && \
           (true == mMainVolumeStepParam.getParam(requestedMainVolume)))
        {
            am_mainVolume_t sinkMainVolume = pSinkElement->getMainVolume();
            requestedMainVolume += sinkMainVolume;
        }
        volume = pSinkElement->convertMainVolumeToVolume(requestedMainVolume);
        return E_OK;
    }

    if((true == mVolumeParam.getParam(volume)) || \
       (true == mVolumeStepParam.getParam(volume)))
    {
        if((false == mVolumeParam.getParam(volume)) && \
           (true == mVolumeStepParam.getParam(volume)))
        {
            am_volume_t sinkVolume;
            volume += pSinkElement->getVolume();
        }
        return E_OK;
    }
    return E_NOT_POSSIBLE;
}

am_Error_e CAmMainConnectionActionSetVolume::_createChildActions(
        std::map< CAmElement*, gc_volume_s >& mapVolumeElements)
{
    std::map< CAmElement*, gc_volume_s >::iterator itVolumeElements;
    itVolumeElements = mapVolumeElements.begin();
    for(;itVolumeElements != mapVolumeElements.end();++itVolumeElements)
    {
        IAmActionCommand* pAction(NULL);
        /*
         * if the limit volume element is already present in the list of
         * volume elements just update the limit parameter.
         */
        if(ET_SINK == itVolumeElements->first->getType())
        {
            pAction = new CAmSinkActionSetVolume((CAmSinkElement*)itVolumeElements->first);
        }
        else
        {
            pAction = new CAmSourceActionSetVolume((CAmSourceElement*)itVolumeElements->first);
        }
        if(true  == itVolumeElements->second.isvolumeSet)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"ACTION_PARAM_VOLUME=",itVolumeElements->second.volume);
            CAmActionParam <am_volume_t > volumeParam(itVolumeElements->second.volume);
            pAction->setParam(ACTION_PARAM_VOLUME,&volumeParam);
        }
        if(true  == itVolumeElements->second.isOffsetSet)
        {
            LOG_FN_DEBUG(__FILENAME__,__func__,"ACTION_PARAM_LIMIT_VOLUME=",itVolumeElements->second.offsetVolume);
            CAmActionParam <am_volume_t > volumeParam(itVolumeElements->second.offsetVolume);
            pAction->setParam(ACTION_PARAM_LIMIT_VOLUME,&volumeParam);
        }

        pAction->setParam(ACTION_PARAM_RAMP_TIME,&mRampTimeParam);
        pAction->setParam(ACTION_PARAM_RAMP_TYPE, &mRampTypeParam);
        pAction->setUndoRequried(true);
        append(pAction);
    }
    return E_OK;
}

int CAmMainConnectionActionSetVolume::_update(int result)
{

    if(E_OK == result)
    {
        if (mMainConnectionName != "")
        {
            mpMainConnection = CAmMainConnectionFactory::getElement(
                                                           mMainConnectionName);
            if (mpMainConnection != NULL)
            {
                mpMainConnection->updateMainVolume();
            }
            else
            {
                LOG_FN_WARN(__FILENAME__,__func__,"Main Connection not found");
            }
        }
    }
    return 0;
}
}/* namespace gc */
} /* namespace am */

