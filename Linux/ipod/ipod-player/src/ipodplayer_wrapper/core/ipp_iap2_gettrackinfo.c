/****************************************************
 *  ipp_iap2_gettrackinfo.c                                         
 *  Created on: 2014/01/17 17:55:30                      
 *  Implementation of the Class ipp_iap2_gettrackinfo       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_gettrackinfo.h"
#include "iPodPlayerCoreFunc.h"
#include "ipp_iap2_database.h"



S32 ippiAP2GetTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 count = 0;
    U32 totalCount = 0;
    U32 progress = 0;
    IPOD_PLAYER_IAP2_DB_TRACKLIST trackList;
    IPOD_CB_PARAM_GET_TRACK_INFO_RESULT getTrackInfoResult;
    
    
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
    memset(&trackList, 0, sizeof(trackList));
    memset(&getTrackInfoResult, 0, sizeof(getTrackInfoResult));
    
    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                                iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, param->waitData->status);
    
    switch(param->waitData->stage)
    {
        /* Request to get the track information to iPod Ctrl at first */
        case IPP_IAP2_GET_TRACKINFO_STAGE_CHECK_PROGRESS:
        
            /* Get the progress from database */
            rc = ippiAP2DBGetProgress(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, &progress);
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* Progress is max. It means that all database has been already retrieved from Apple devoce */
                if(progress == IPOD_PLAYER_IAP2_MAX_PROGRESS)
                {
                    /* Get track total count */
                    if(param->waitData->contents.getTrackInfo.type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
                    {
                        /* Playback type */
                        rc = ippiAP2DBGetNowPlayingCount(iPodCtrlCfg->iap2Param.dbHandle, &totalCount);
                        //IPOD_DLT_INFO("[DBG]rc=%d, count=%u", rc, totalCount);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            /* no queue list */
                            if(totalCount == 0)
                            {
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_STATUS, totalCount);
                                rc = IPOD_PLAYER_ERR_INVALID_STATUS;
                            }
                        }
                    }
                    else
                    {
                        rc = ippiAP2DBGetTrackTotalCount(iPodCtrlCfg->iap2Param.dbHandle, &iPodCtrlCfg->iap2Param.dbHandle->localDevice, param->waitData->contents.getTrackInfo.type, &totalCount, &(param->waitData->contents.getTrackInfo.mediaType));
                    }

                    if((rc == IPOD_PLAYER_OK) && (param->waitData->contents.getTrackInfo.count > 0) && (param->waitData->contents.getTrackInfo.type <= IPOD_PLAYER_TRACK_TYPE_UID) &&
                       (param->waitData->contents.getTrackInfo.startID < totalCount) && (IPP_IAP2_SUPPORTED_TRACK_INFO_MASK & param->waitData->contents.getTrackInfo.trackInfoMask))
                    {
                        /* Set the timer to be run the next status */
                        param->waitData->stage = IPP_IAP2_GET_TRACKINFO_STAGE_GET_TRACKINFO;
                        param->waitData->contents.getTrackInfo.startIndex = param->waitData->contents.getTrackInfo.startID;
                        
                        if((param->waitData->contents.getTrackInfo.startID + param->waitData->contents.getTrackInfo.count) > totalCount)
                        {
                            count = totalCount - param->waitData->contents.getTrackInfo.startID;
                            if(count > 0)
                            {
                                param->waitData->contents.getTrackInfo.count = count;
                            }
                            else
                            {
                                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                            }
                        }
                        if(rc == IPOD_PLAYER_OK)
                        {
                            iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
                            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        }
                    }
                    else if(IPP_IAP2_IS_MASK_SET((IPP_IAP2_TRACK_INFO_MASK & (~IPP_IAP2_SUPPORTED_TRACK_INFO_MASK)), param->waitData->contents.getTrackInfo.trackInfoMask))
                    {
                        rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, param->waitData->contents.getTrackInfo.trackInfoMask,
                                                                                    param->waitData->contents.getTrackInfo.startID, param->waitData->contents.getTrackInfo.type, 
                                                                                                                         param->waitData->contents.getTrackInfo.count, totalCount);
                        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    }
                }
                else
                {
                    iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_IAP2_TIMER_FOR_PROGRESS, 0);
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
            }
            break;
            
        /* Wait to receive the data from callback function */
        case IPP_IAP2_GET_TRACKINFO_STAGE_GET_TRACKINFO:
            if(param->waitData->contents.getTrackInfo.startIndex < (param->waitData->contents.getTrackInfo.startID + param->waitData->contents.getTrackInfo.count))
            {
                /* Get remained track number */
                count = param->waitData->contents.getTrackInfo.startID + param->waitData->contents.getTrackInfo.count - param->waitData->contents.getTrackInfo.startIndex;
                if(count > IPP_IAP2_GET_TRACKINFO_MAX)
                {
                    count = IPP_IAP2_GET_TRACKINFO_MAX;
                }
                trackList.trackInfo = calloc(count, sizeof(IPOD_PLAYER_TRACK_INFO));
                if(trackList.trackInfo != NULL)
                {
                    rc = ippiAP2DBGetTrackInfoList(iPodCtrlCfg->iap2Param.dbHandle, &iPodCtrlCfg->iap2Param.dbHandle->localDevice,
                                                   param->waitData->contents.getTrackInfo.type, param->waitData->contents.getTrackInfo.trackInfoMask,
                                                   param->waitData->contents.getTrackInfo.startIndex, count, &trackList, param->waitData->contents.getTrackInfo.mediaType);
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_NOMEM;
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Set nofify resault */
                    getTrackInfoResult.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | IPOD_FUNC_GET_TRACK_INFO);
                    getTrackInfoResult.header.appID = param->waitData->contents.getTrackInfo.header.appID;
                    getTrackInfoResult.header.devID = param->waitData->contents.getTrackInfo.header.devID;
                    getTrackInfoResult.startID = param->waitData->contents.getTrackInfo.startID;
                    getTrackInfoResult.count = param->waitData->contents.getTrackInfo.count;
                    getTrackInfoResult.type = param->waitData->contents.getTrackInfo.type;
                    getTrackInfoResult.actualMask = param->waitData->contents.getTrackInfo.trackInfoMask;
                    /* Notify all of track info */
                    for(i = 0; i < trackList.count; i++)
                    {
                        /* Clear the notification track info */
                        memset(&getTrackInfoResult.info, 0, sizeof(getTrackInfoResult.info));
                        /* Set the notification track info */
                        getTrackInfoResult.mask = trackList.trackInfo[i].trackInfoMask;
                        getTrackInfoResult.sendMask = trackList.trackInfo[i].trackInfoMask;
                        getTrackInfoResult.trackID = param->waitData->contents.getTrackInfo.startIndex;
                        memcpy(&getTrackInfoResult.info, &trackList.trackInfo[i], sizeof(getTrackInfoResult.info));
                        getTrackInfoResult.result = IPOD_PLAYER_OK;
                        
                        /* Set the next track ID in queued data */
                        param->waitData->contents.getTrackInfo.startIndex++;
                        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&getTrackInfoResult, sizeof(getTrackInfoResult), 0, IPODCORE_TMOUT_FOREVER);
                        if(rc != sizeof(getTrackInfoResult))
                        {
                            rc = IPOD_PLAYER_ERROR;
                            break;
                        }
                    }
                    
                    if(trackList.trackInfo != NULL)
                    {
                        free(trackList.trackInfo);
                        trackList.trackInfo = NULL;
                    }
                    
                    if(rc != IPOD_PLAYER_ERROR)
                    {
                        /* Get next set of track info */
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
                    }
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERR_NO_REPLY;
            }
            break;
            
        default:
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, param->waitData->status);
            break;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}
