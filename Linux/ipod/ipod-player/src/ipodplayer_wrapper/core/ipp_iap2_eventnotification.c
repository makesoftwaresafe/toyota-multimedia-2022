#include "ipp_iap2_eventnotification.h"
#include "iPodPlayerUtilityLog.h"
#include "ipp_iap2_database.h"
#include "ipp_iap2_observer.h"


static S32 ippiAP2EventNotificationSendLongNotifyMessage(S32 handle, U8 **message, U32 *size);

S32 ippiAP2EventNotificationCoverArtData(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataSize, U8 *artWorkData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_TEMP_LONG_DATA longData;
    U8 *sendBuf[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&longData, 0, sizeof(longData));
    
    longData.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_COVERART_DATA;
    longData.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    longData.header.longData = 1;
    
    longData.coverart.coverartHeader.formatId = 1;
    longData.coverart.bufSize = dataSize;
    
    /* Set long data */
    sendBuf[IPODCORE_POS0] = (U8 *)&longData;
    sendSize[IPODCORE_POS0] = sizeof(longData);
    sendBuf[IPODCORE_POS1] = artWorkData;
    sendSize[IPODCORE_POS1] = dataSize;
    
    /* Notify to iPodPlayerCore */
    rc = ippiAP2EventNotificationSendLongNotifyMessage(iPodCtrlCfg->threadInfo->longCmdQueueClient, sendBuf, sendSize);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 ippiAP2EventNotificationPlayBackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_PLAYBACK_STATUS playbackStatus;
    IPOD_PLAYER_REPEAT_STATUS repeatStatus;
    IPOD_PLAYER_SHUFFLE_STATUS shuffleStatus;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    /* Initialize the structure */
    memset(&playbackStatus, 0, sizeof(playbackStatus));
    /* Set the playback status active status */
    playbackStatus.status.playbackStatusActiveMask = iPodCtrlCfg->iap2Param.playbackStatusActiveMask;
    
    rc = ippiAP2DBGetPlaybackStatus(iPodCtrlCfg->iap2Param.dbHandle, sizeof(playbackStatus.status), (U8 *)&playbackStatus.status);
    if(rc == IPOD_PLAYER_OK)
    {
        if(iPodCtrlCfg->iap2Param.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_REPEAT)
        {
            /* get repeat status */
            rc = ippiAP2DBGetRepeat(iPodCtrlCfg->iap2Param.dbHandle, &repeatStatus);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Set the current repeat status to result */
                playbackStatus.status.repeatStatus = repeatStatus;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        if(iPodCtrlCfg->iap2Param.playbackStatusActiveMask & IPOD_PLAYER_PLAY_ACT_SHUFFLE)
        {
            /* get shuffle status */
            rc = ippiAP2DBGetShuffle(iPodCtrlCfg->iap2Param.dbHandle, &shuffleStatus);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Set the current shuffle status to result */
                playbackStatus.status.shuffleStatus = shuffleStatus;
            }
        }
    }

    /* The playbackStatusUpdateMask must be checked because device might be not sending NowPlayingUpdate message. */
    if(rc == IPOD_PLAYER_OK)
    {
        if(iPodCtrlCfg->iap2Param.playbackStatusActiveMask != 0)
        {
            /* Set the notify playback status command */
            playbackStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_PLAYBACK_STATUS;
            playbackStatus.header.devID = iPodCtrlCfg->threadInfo->appDevID;
            /* Notify to iPodPlayerCore */
            rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&playbackStatus, sizeof(playbackStatus));
        }
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCoreObserverClearUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_PLAYBACK_STATUS, 0);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 ippiAP2EventNotificationTrackInformation(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 infoMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_TRACK_INFO trackInfo;
    IPOD_PLAYER_PLAYBACK_STATUS status;
    U32 progress = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, infoMask);
    //IPOD_DLT_INFO("[DBG]infoMask=%x(%d)", infoMask, infoMask);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (infoMask == 0))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, infoMask);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&trackInfo, 0, sizeof(trackInfo));

    /* get playback queue index from iPodInfo DB */
    status.playbackStatusActiveMask = IPOD_PLAYER_PLAY_ACT_TRACK_INDEX; /* track index is active */
    rc = ippiAP2DBGetPlaybackStatus(iPodCtrlCfg->iap2Param.dbHandle, sizeof(status), (U8 *)&status);
    if(rc == IPOD_PLAYER_OK)
    {
        rc = ippiAP2DBGetNowPlayingUpdateTrackInfo(iPodCtrlCfg->iap2Param.dbHandle, infoMask, &trackInfo.info);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Set media type */
            if((infoMask & IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) == IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY)
            {
                /* Get the progress from database */
                rc = ippiAP2DBGetProgress(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, &progress);
                
                if(rc == IPOD_PLAYER_OK)
                {
                    trackInfo.info.mediaType = -1;
                    trackInfo.info.capa = 0;
                    /* Progress is max. It means that all database has been already retrieved from Apple devoce */
                    if(progress == IPOD_PLAYER_IAP2_MAX_PROGRESS)
                    {
                        rc = ippiAP2DBGetMediaType(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, trackInfo.info.trackID, &trackInfo.info.mediaType);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            ippiAP2DBSetCapability(trackInfo.info.mediaType, &(trackInfo.info.capa));
                        }
                        else
                        {
                            IPOD_DLT_WARN("access error to internal DB (Media Item) handle = %p", iPodCtrlCfg->iap2Param.dbHandle); 
                        }
                    }
                    else
                    {
                        IPOD_DLT_WARN("MediaLibraryUpdate isn't finishing(Media Item) progress = %u", progress); 
                    }
                }
                else
                {
                    IPOD_DLT_ERROR("Could not get progress of media library up date. handle = %p", iPodCtrlCfg->iap2Param.dbHandle); 
                }
            }
            
            /* Set the notify track information command */
            trackInfo.resultFlag = 1;
            trackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_TRACK_INFO;
            trackInfo.header.devID = iPodCtrlCfg->threadInfo->appDevID;
            trackInfo.trackIndex = status.track.index; /* set Playback queue index in NowPlayingUpdate. */
            
            /* Notify to iPodPlayerCore */
            rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&trackInfo, sizeof(trackInfo));
            if(rc == IPOD_PLAYER_OK)
            {
                iPodCoreObserverClearUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_TRACK_INFO, infoMask);
            }
        }
        else
        {
            IPOD_DLT_ERROR("access error to internal DB (PlayingItem) handle = %p", iPodCtrlCfg->iap2Param.dbHandle); 
        }
    }
    else
    {
        IPOD_DLT_ERROR("access error to internal DB (iPodInfo) handle = %p", iPodCtrlCfg->iap2Param.dbHandle); 
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 ippiAP2EventNotificationDeviceEvent(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 eventMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 num = 0;
    U32 profile = 0;
    U32 tbIDMask = 0;
    IPOD_PLAYER_PARAM_NOTIFY_DEVICE_EVENT notifyEvent;
    U8 temp[IPODCORE_BT_MAC_MAX_LEN + 1];
    U64 tempVal = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, eventMask);
    
    if((iPodCtrlCfg == NULL) || (eventMask == 0))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, eventMask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&notifyEvent, 0, sizeof(notifyEvent));
    
    
    /* Set the notify device event command with internal */
    notifyEvent.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_DEVICE_EVENT;
    notifyEvent.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    /* Assitive event */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_ASSISTIVE))
    {
        rc = ippiAP2DBGetAssistiveStatus(iPodCtrlCfg->iap2Param.dbHandle, IPP_IAP2_ASSISTIVE_ID, &notifyEvent.event.assitiveEvent.status);
        if(rc == IPOD_PLAYER_OK)
        {
            notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_ASSISTIVE;
            rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        }
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }
    
    /* power update event */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_POWER))
    {
        /* create power update information */
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_POWER;
        notifyEvent.event.powerNotify = iPodCtrlCfg->iap2Param.powerNotify;
        notifyEvent.event.powerNotify.powerUpdateEventMask = iPodCtrlCfg->iap2Param.powerUpdateEventStatus;

        /* clear power update event */
        iPodCtrlCfg->iap2Param.powerUpdateEventStatus = 0;

        /* notification Power Update */
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));

         /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* Database event of Local Device media library */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_DATABASE))
    {
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE;
        notifyEvent.event.info = iPodCtrlCfg->iap2Param.iap2MediaParam->infoOfLocalDevice;
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* Database event of Apple Music Radio media library */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_DATABASE_AMR))
    {
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE_AMR;
        notifyEvent.event.info = iPodCtrlCfg->iap2Param.iap2MediaParam->infoOfAppleMusicRadio;
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* sample rate event */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_SAMPLE_RATE))
    {
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_SAMPLE_RATE;
        notifyEvent.event.sampleRate = iPodCtrlCfg->iPodInfo->sampleRate;
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* Store database event of Local Device media library */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_STORE_DB))
    {
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_STORE_DB;
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* Store database event of Apple Music Radio media library */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_STORE_DB_AMR))
    {
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_STORE_DB_AMR;
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* update playback list event */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_UPDATE_PBLIST))
    {
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_UPDATE_PBLIST;
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }

    /* Bluetooth event */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_BT))
    {
        /* Set notify type */
        notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_BT;
        /* Get bluetooth id mask */
        tbIDMask = iPodCtrlCfg->iap2Param.updateDataMask.blueToothIDMask;

        /* Get bluetooth number */
        if(iPodCtrlCfg->threadInfo->btInfo.macCount == 0)
        {
            num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC_COUNT);
        }
        else
        {
            num = iPodCtrlCfg->threadInfo->btInfo.macCount;
        }

        if(num == 0)
        {
            IPOD_DLT_ERROR("Could not get bluetooth Mac count.(mac count = %u).", num);
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
        /* Check all bluetooth */
        for(i = 0; i < num; i++)
        {
            /* if is updated bluetooth */
            if(IPP_IAP2_IS_MASK_SET(tbIDMask, (U32)(1 << i)))
            {
                /* Get bluetooth profile */
                rc = ippiAP2DBGetBluetoothStatus(iPodCtrlCfg->iap2Param.dbHandle, i, &profile);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Set bluetooth info */
                    notifyEvent.event.btEvent.profileList = profile;
                    if(iPodCtrlCfg->threadInfo->btInfo.macCount == 0)
                    {
                        memset(temp, 0, sizeof(temp));
                        if(iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC, i, IPODCORE_BT_MAC_MAX_LEN, temp) == IPOD_PLAYER_OK)
                        {
                            tempVal = be64toh(strtoull((const char*)temp, NULL, 16));
                            tempVal >>= 16;
                            memcpy(notifyEvent.event.btEvent.macAddr, &tempVal, sizeof(notifyEvent.event.btEvent.macAddr));    
                        }
                        else
                        {
                            IPOD_DLT_ERROR("Could not get configuration for bluetooth Mac address. Illegal configuration of Mac address.");
                            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                        }
                    }
                    else
                    {
                        memcpy(notifyEvent.event.btEvent.macAddr, iPodCtrlCfg->threadInfo->btInfo.macAddr[i].addr, 
                                                                sizeof(iPodCtrlCfg->threadInfo->btInfo.macAddr[i].addr));
                    }
                }
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Notify bluetooth event */
                    rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
                }
            }
            /* Clear device event data */
            memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
        }
    }
    
    /* iOS application event */
    if(IPP_IAP2_IS_MASK_SET(eventMask, IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL))
    {
        /* Get the iOS application name from db */
        rc = ippiAP2DBGetiOSAppName(iPodCtrlCfg->iap2Param.dbHandle, sizeof(notifyEvent.event.playingiOSAppEvent.appName), (U8 *)notifyEvent.event.playingiOSAppEvent.appName);
        if(rc == IPOD_PLAYER_OK)
        {
            notifyEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPPFULL;
            rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyEvent, sizeof(notifyEvent));
        }
        /* Clear device event data */
        memset(&notifyEvent.event, 0, sizeof(notifyEvent.event));
    }
    
    iPodCoreObserverClearUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, eventMask);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void ippiAP2DelayedEventSetIosApp(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, int ix)
{
    iPodCtrlCfg->delayedEvent.iOSAppQueue[iPodCtrlCfg->delayedEvent.iosIxw].appInfo = iPodCtrlCfg->iOSAppID[ix];
    iPodCtrlCfg->delayedEvent.iOSAppQueue[iPodCtrlCfg->delayedEvent.iosIxw].devIx = ix;
    iPodCtrlCfg->delayedEvent.iosIxw++;
    if(iPodCtrlCfg->delayedEvent.iosIxw > IPP_DELAYED_QUEUE_MAX)
    {
        iPodCtrlCfg->delayedEvent.iosIxw = IPP_DELAYED_QUEUE_MAX;
    }
}

S32 ippiAP2SetAppInformation(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 appID, U16 sessionID)
{
    S32 rc = IPOD_PLAYER_ERR_NOMEM;
    int i= 0;
    
    /* Find iOS information to match */
    for(i = 0; i < IPODCORE_MAX_IOSAPPS_INFO_NUM; i++)
    {
        if(iPodCtrlCfg->iOSAppID[i].appID == appID)
        {
            iPodCtrlCfg->iOSAppID[i].sessionID = sessionID;
            iPodCtrlCfg->iOSAppID[i].isReady = 1;
            iPodCtrlCfg->iOSAppID[i].status = IPODCORE_IOSAPP_OPEN;
            ippiAP2DelayedEventSetIosApp(iPodCtrlCfg, i);
            rc = IPOD_PLAYER_OK;
            break;
        }
    }

    if(rc != IPOD_PLAYER_OK)
    {
        /* Find space in IOS information when session ID cannot be found. */
        for(i = 0; i < IPODCORE_MAX_IOSAPPS_INFO_NUM; i++)
        {
            if(iPodCtrlCfg->iOSAppID[i].appID == 0)
            {
                iPodCtrlCfg->iOSAppID[i].appID = appID;
                iPodCtrlCfg->iOSAppID[i].sessionID = sessionID;
                iPodCtrlCfg->iOSAppID[i].isReady = 1;
                iPodCtrlCfg->iOSAppID[i].status = IPODCORE_IOSAPP_OPEN;
                ippiAP2DelayedEventSetIosApp(iPodCtrlCfg, i);
                rc = IPOD_PLAYER_OK;
                break;
            }
        }
    }

    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }

    return rc;
}

void ippiAP2GetAppInformationWithClear(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U16 sessionID)
{
    int i = 0;
    
    for(i = 0; i < IPODCORE_MAX_IOSAPPS_INFO_NUM; i++)
    {
        if(iPodCtrlCfg->iOSAppID[i].sessionID == sessionID)
        {
            iPodCtrlCfg->iOSAppID[i].isReady = 0;
            iPodCtrlCfg->iOSAppID[i].status = IPODCORE_IOSAPP_CLOSE;
            ippiAP2DelayedEventSetIosApp(iPodCtrlCfg, i);
            break;
        }
    }
}

S32 ippiAP2EventNotificationOpeniOSApplication(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IAP2_IOSAPP_INFO *iOSAppInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;

    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, iOSAppInfo);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (iOSAppInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, iOSAppInfo);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = ippiAP2SetAppInformation(iPodCtrlCfg, iOSAppInfo->index, iOSAppInfo->session);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 ippiAP2EventNotificationCloseiOSApplication(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U16 appSession)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    ippiAP2GetAppInformationWithClear(iPodCtrlCfg, appSession);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 ippiAP2EventNotificationReceiveiOSApplication(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 appSession, U16 dataSize, U8 *appData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_RECEIVE_FROM_APP notifyAppData;                /* for notify app data       */
    U8 *sendBuf[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (appData == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&notifyAppData, 0, sizeof(notifyAppData));
    
    notifyAppData.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_RECEIVE_FROM_APP;
    notifyAppData.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    
    /* Set the opend paramtetr */
    notifyAppData.header.longData = 1;
    notifyAppData.appID = appSession;
    notifyAppData.dataSize = dataSize;
    sendBuf[0] = (U8 *)&notifyAppData;
    sendSize[0] = sizeof(notifyAppData);
    sendBuf[1] = appData;
    sendSize[1] = dataSize;
    
    /* Notify to iPodPlayerCore */
    rc = ippiAP2EventNotificationSendLongNotifyMessage(iPodCtrlCfg->threadInfo->longCmdQueueClient, sendBuf, sendSize);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 ippiAP2EventNotificationLocationInformationStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_LOCATION_INFO_STATUS status, U32 locationMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_LOCATION_INFO_STATUS notifyLocation;       /* for notify app data       */
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&notifyLocation, 0, sizeof(notifyLocation));
    
    notifyLocation.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_LOCATION_INFO_STATUS;
    notifyLocation.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    notifyLocation.status = status;
    notifyLocation.locationMask = locationMask;
    
    /* Notify to iPodPlayerCore */
    rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyLocation, sizeof(notifyLocation));
 
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 ippiAP2EventNotificationVehicleStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_VEHICLE_STATUS status)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_VEHICLE_STATUS notifyVehicle;       /* for notify app data       */
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        /* Return iAP2 invalid parameter error */
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&notifyVehicle, 0, sizeof(notifyVehicle));
    
    notifyVehicle.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_VEHICLE_STATUS;
    notifyVehicle.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    notifyVehicle.status = status;
    
    /* Notify to iPodPlayerCore */
    rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyVehicle, sizeof(notifyVehicle));
 
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 ippiAP2EventNotificationSendNotifyMessage(S32 handle, U8 *message, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;                     /* for return code */
    
    /* Send IPC message */
   rc = iPodPlayerIPCSend(handle, message, size, 0, IPODCORE_SEND_TMOUT);
    if(rc == (S32)size)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}


static S32 ippiAP2EventNotificationSendLongNotifyMessage(S32 handle, U8 **message, U32 *size)
{
    
    S32 rc = IPOD_PLAYER_ERROR;                     /* for return code */
    U32 asize = 0;                                  /* for send size   */
    
    /* Send long IPC message */
    rc = iPodPlayerIPCLongSend(handle, IPODCORE_LONG_DATA_ARRAY, message, size, &asize, 0, IPODCORE_SEND_TMOUT);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}


IAP2_EVENTNOTIFICATION_FUNC_TABLE *ippiAP2EventNotificationSetNotifyFunctions(void)
{
    IAP2_EVENTNOTIFICATION_FUNC_TABLE *notifyFuncs = NULL;
    
    notifyFuncs = calloc(1, sizeof(IAP2_EVENTNOTIFICATION_FUNC_TABLE));
    if(notifyFuncs != NULL)
    {
        notifyFuncs->notifyConntectionStatus           = NULL;
        notifyFuncs->notifyPlayBackStatus              = ippiAP2EventNotificationPlayBackStatus;
        notifyFuncs->notifyCoverArtData                = ippiAP2EventNotificationCoverArtData;
        notifyFuncs->notifyDBEntries                   = NULL;
        notifyFuncs->notifyTrackInformation            = ippiAP2EventNotificationTrackInformation;
        notifyFuncs->notifyOpeniOSApplication          = ippiAP2EventNotificationOpeniOSApplication;
        notifyFuncs->notifyCloseiOSApplication         = ippiAP2EventNotificationCloseiOSApplication;
        notifyFuncs->notifyReceiveFromiOSApplication   = ippiAP2EventNotificationReceiveiOSApplication;
        notifyFuncs->notifyPlayBackChange              = NULL;
        notifyFuncs->notifyDeviceEvent                 = ippiAP2EventNotificationDeviceEvent;
        notifyFuncs->notifyLocationInfoStatus          = ippiAP2EventNotificationLocationInformationStatus; 
        notifyFuncs->notifyVehicleStatus               = ippiAP2EventNotificationVehicleStatus; 
    }
    
    return notifyFuncs;
}


void ippiAP2EventNotificationClearNotifyFunctions(IAP2_EVENTNOTIFICATION_FUNC_TABLE *notifyFuncs)
{
    if(notifyFuncs != NULL)
    {
        free(notifyFuncs);
    }
    
    return;
}
