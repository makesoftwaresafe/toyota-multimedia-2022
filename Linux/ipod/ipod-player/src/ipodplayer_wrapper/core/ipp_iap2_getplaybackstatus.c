/****************************************************
 *  ipp_iap2_getplaybackstatus.c                                         
 *  Created on: 2014/01/17 17:55:29                      
 *  Implementation of the Class ipp_iap2_getplaybackstatus       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_getplaybackstatus.h"
#include "ipp_iap2_database.h"


S32 ippiAP2GetPlaybackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PLAYBACK_STATUS playbackStatus;
    IPOD_PLAYER_REPEAT_STATUS repeatStatus;
    IPOD_PLAYER_SHUFFLE_STATUS shuffleStatus;
    

    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (param == NULL) || (param->waitData == NULL) || (param->contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize parameter */
    memset(&playbackStatus, 0, sizeof(playbackStatus));
    
    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                            iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    /* Init current playback status to result */
    playbackStatus.playbackStatusActiveMask = iPodCtrlCfg->iap2Param.playbackStatusSetMask;
    rc = ippiAP2DBGetPlaybackStatus(iPodCtrlCfg->iap2Param.dbHandle, sizeof(playbackStatus), (U8 *)&playbackStatus);
    if(rc == IPOD_PLAYER_OK)
    {
        /* playback status */
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_STATUS)
        {
            param->waitData->contents.getPlaybackStatusResult.status.status = playbackStatus.status;
        }
        /* track index */
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_TRACK_INDEX)
        {
            param->waitData->contents.getPlaybackStatusResult.status.track.index = playbackStatus.track.index;
        }
        /* chapter index */
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_CHAPTER_INDEX)
        {
            param->waitData->contents.getPlaybackStatusResult.status.chapter.index = playbackStatus.chapter.index;
        }
        /* time */
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_TIME)
        {
            param->waitData->contents.getPlaybackStatusResult.status.track.time = playbackStatus.track.time;
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_TIME)
        {
            param->waitData->contents.getPlaybackStatusResult.status.chapter.time = playbackStatus.chapter.time;
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_APP_NAME)
        {
            strncpy((char*)&param->waitData->contents.getPlaybackStatusResult.status.appName, (const char *)&playbackStatus.appName, sizeof(playbackStatus.appName));
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_APP_BUNDLE_ID)
        {
            strncpy((char*)&param->waitData->contents.getPlaybackStatusResult.status.appBundleID, (const char *)&playbackStatus.appBundleID, sizeof(playbackStatus.appBundleID));
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_MEDIA_LIBRARY_UID)
        {
            strncpy((char*)&param->waitData->contents.getPlaybackStatusResult.status.mediaLibraryUID, (const char *)&playbackStatus.mediaLibraryUID, sizeof(playbackStatus.mediaLibraryUID));
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_AMR_STATION_NAME)
        {
            strncpy((char*)&param->waitData->contents.getPlaybackStatusResult.status.AmrStationName, (const char *)&playbackStatus.AmrStationName, sizeof(playbackStatus.AmrStationName));
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_QUEUE_COUNT)
        {
            param->waitData->contents.getPlaybackStatusResult.status.queueCount = playbackStatus.queueCount;
        }
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_QUEUE_LIST_AVAIL)
        {
            param->waitData->contents.getPlaybackStatusResult.status.queueListAvail = playbackStatus.queueListAvail;
        }

        /* repeat status */
        if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_REPEAT)
        {
            rc = ippiAP2DBGetRepeat(iPodCtrlCfg->iap2Param.dbHandle, &repeatStatus);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Set the current repeat status to result */
                param->waitData->contents.getPlaybackStatusResult.status.repeatStatus = repeatStatus;
            }
            else
            {
                IPOD_DLT_ERROR("Could not get DB in repeat table.");
            }
        }
        if(rc == IPOD_PLAYER_OK)
        {
            /* shuffle status */
            if(playbackStatus.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_SHUFFLE)
            {
                rc = ippiAP2DBGetShuffle(iPodCtrlCfg->iap2Param.dbHandle, &shuffleStatus);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Set the current shuffle status to result */
                    param->waitData->contents.getPlaybackStatusResult.status.shuffleStatus = shuffleStatus;
                }
                else
                {
                    IPOD_DLT_ERROR("Could not get DB in shuffle table.");
                }
            }
        }
        if(rc == IPOD_PLAYER_OK)
        {
            /* set statusSetMask to playbackStatusActiveMask */
            param->waitData->contents.getPlaybackStatusResult.status.playbackStatusActiveMask = playbackStatus.playbackStatusActiveMask;
        }

    }
    else
    {
        IPOD_DLT_ERROR("Could not get DB in playback table.");
    }

    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

