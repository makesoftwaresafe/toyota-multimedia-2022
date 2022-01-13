#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iap_types.h>
#include "ipodcommon.h"
#include "iap_common.h"
#include "adit_typedef_linux.h"
#include "pthread_adit.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreCommonFunc.h"
#include "iPodPlayerCoreiPodCtrlFunc.h"
#include "iPodPlayerCoreFunc.h"
#include "iPodPlayerCoreCfg.h"
#include "iPodPlayerUtilityLog.h"
#include "ipp_iap2_init.h"
#include "ipp_iap2_callback.h"
#include "ipp_iap2_mainloop.h"
#include "ipp_mainloop_common.h"
#include "iPodPlayerCoreCmdHandler.h"
#include "ipp_audiocommon.h"
#include "ipp_iap2_ctrlcfglist.h"
#include "ipp_iap2_devinit.h"

#define IPODCORE_PREFIX_LEN 10
EXPORT IPOD_PLAYER_CORE_THREAD_INFO **g_threadInfo;

static S32 iPodCoreiPodCtrlExecuteQueue(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 listMax, IPOD_PLAYER_CORE_WAIT_LIST *waitList, 
                                        IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_FUNC_TABLE *funcTable = NULL;
    U32 i = 0;
    U32 attr = 0;
    IPOD_PLAYER_FUNC_ID funcId = (IPOD_PLAYER_FUNC_ID)0;
    
    /* Parameter check */
    if((iPodCtrlCfg== NULL) || (iPodCtrlCfg->funcTable == NULL) || (waitList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitList, funcTable, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    funcTable = (IPOD_PLAYER_CORE_FUNC_TABLE *)iPodCtrlCfg->funcTable;
    
    /* Loop until maximum queueu number */
    for(i = 0; i < listMax; i++)
    {
        funcId = waitList[i].contents.paramTemp.header.funcId;
        /* Check whether data is queued or not */
        if((waitList[i].status != IPODCORE_QUEUE_STATUS_NONE) && (funcId < IPOD_FUNC_MAX))
        {
            /* Current queue has data. Check whether the function of indicated ID is registered */
            if(funcTable[funcId].func != NULL)
            {
                /* Set the attribute of current queue */
                attr = funcTable[funcId].attr;
                /* Check the current runing mask. If attr bit is set, something function  is running */
                if((iPodCtrlCfg->curRunMask & attr) == 0)
                {
                    /* Set the attr to current runnig mask */
                    if(attr != IPOD_PLAYER_INTERNAL_HMI_STATUS_MASK)
                    {
                        iPodCtrlCfg->curRunMask |= attr;
                    }
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, i);
                    /* Call requested function */
                    rc = funcTable[funcId].func(iPodCtrlCfg, &waitList[i], header, contents, &size);
                }
                else
                {
                    /* Add the attribute also waiting queue otherwise backward queue may be executed */
                    iPodCtrlCfg->curRunMask |= attr;
                    /* Queue is not executed yet. The status is still wait */
                    rc = IPOD_PLAYER_ERR_STILL_WAIT;
                }
            }
            else
            {
                /* function is null. Set to not applicable error */
                rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        rc = iPodCoreCheckResultAndUpdateStatus(rc, &waitList[i]);
        if(waitList[i].status == IPODCORE_QUEUE_STATUS_FINISH)
        {
            /* Requesetd function was finished. attr is removed from current running mask */
            iPodCtrlCfg->curRunMask &= ~attr;
            if(rc != IPOD_PLAYER_ERR_NO_REPLY)
            {
                /* Set result */
                waitList[i].contents.paramResultTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | funcId);
                waitList[i].contents.paramResultTemp.result = rc;
                
                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitList[i].contents, size, 0, IPODCORE_TMOUT_FOREVER);
                if(rc == (S32)size)
                {
                    rc = IPOD_PLAYER_OK;
                }
            }
        }
    }
    
    return rc;
}

S32 iPodCoreiPodCtrlSendCB(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 resSize = size;
    U32 mask = 0;
    U32 devID = 0;
    U32 retDevID = 0;
    U32 i = 0;
    U32 funcId = 0;
    U8 *sendBuf[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    U32 asize = 0;
    
    if((iPodCtrlCfg == NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    retDevID = iPodCtrlCfg->threadInfo->appDevID;
    
    funcId = (U32)header->funcId;
    
    switch(funcId)
    {
        case IPOD_FUNC_NOTIFY_CONNECTION_STATUS:
            if((contents->notifyConnectionStatus.status.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) && 
               (contents->notifyConnectionStatus.status.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
            {
                mask = IPOD_PLAYER_DEVICE_MASK_NAME | IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION | IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER | IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE | IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE | 
                       IPOD_PLAYER_DEVICE_MASK_EVENT | IPOD_PLAYER_DEVICE_MASK_FILE_SPACE;
                       
                /* When iPod notifies the Attach, iPodPlayer get the property of connected iPod */
                iPodCoreiPodCtrlIntGetDeviceProperty(devID, mask, &iPodCtrlCfg->iPodInfo->property);
                if((iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_SONG_TAG) == IPOD_PLAYER_FEATURE_MASK_SONG_TAG)
                {
                    iPodCoreiPodCtrlIntGetCaps(devID, &iPodCtrlCfg->iPodInfo->totalSpace, &iPodCtrlCfg->iPodInfo->maxFileSize, &iPodCtrlCfg->iPodInfo->maxWriteSize);
                }
                
                IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->property.name, iPodCtrlCfg->iPodInfo->property.softVer.majorVer, iPodCtrlCfg->iPodInfo->property.softVer.minorVer);
                IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->property.serial, iPodCtrlCfg->iPodInfo->property.supportedFeatureMask, iPodCtrlCfg->iPodInfo->property.maxPayload, iPodCtrlCfg->iPodInfo->property.curEvent, iPodCtrlCfg->iPodInfo->property.supportEvent, iPodCtrlCfg->iPodInfo->property.fileSpace, iPodCtrlCfg->iPodInfo->property.formatCount);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->property.format[0].formatId, iPodCtrlCfg->iPodInfo->property.format[1].formatId, iPodCtrlCfg->iPodInfo->property.format[2].formatId, iPodCtrlCfg->iPodInfo->property.format[3].formatId, iPodCtrlCfg->iPodInfo->property.format[4].formatId, iPodCtrlCfg->iPodInfo->property.format[5].formatId);
                
                iPodCoreiPodCtrlIntGetNumEQ(devID, &iPodCtrlCfg->iPodInfo->maxEq);
                
                /* WHen iPod notifies the Attach, iPodPlayer get the video settings of connected iPod */
                iPodCoreiPodCtrlIntGetVideoSetting(devID, &iPodCtrlCfg->iPodInfo->videoSetting);
            }
            memcpy(&iPodCtrlCfg->deviceConnection, &contents->notifyConnectionStatus.status, sizeof(iPodCtrlCfg->deviceConnection));
            break;
            
        case IPOD_FUNC_NOTIFY_PLAYBACK_STATUS:
            break;
            
        case IPOD_FUNC_NOTIFY_TRACK_INFO:
            break;
            
        case IPOD_CORE_INT_FUNC_SET_SAMPLERATE:
            break;
        
        case IPOD_FUNC_NOTIFY_DB_ENTRIES:
            break;
        case IPOD_FUNC_NOTIFY_OPEN_APP:
            /* iPod notiefies the open of iOS APplication */
            if(iPodCtrlCfg->iOSAppID[contents->notifyOpenApp.index].isReady == 0)
            {
                /* Set iOS Application information */
                iPodCtrlCfg->iOSAppID[contents->notifyOpenApp.index].appID = contents->notifyOpenApp.index;
                iPodCtrlCfg->iOSAppID[contents->notifyOpenApp.index].sessionID = contents->notifyOpenApp.session;
                iPodCtrlCfg->iOSAppID[contents->notifyOpenApp.index].isReady++;
            }
            
            contents->notifyOpenApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_OPEN_APP;
            contents->notifyOpenApp.header.devID = retDevID;
            contents->notifyOpenApp.appID = iPodCtrlCfg->iOSAppID[contents->notifyOpenApp.index].appID;
            resSize = sizeof(contents->notifyOpenApp);
        
            break;
            
        case IPOD_FUNC_NOTIFY_CLOSE_APP:
            rc = IPOD_PLAYER_ERROR;
            for(i = 0; (i < IPODCORE_MAX_IOSAPPS_INFO_NUM) && (rc != IPOD_PLAYER_OK); i++)
            {
                if(iPodCtrlCfg->iOSAppID[i].sessionID == contents->notifyCloseApp.session)
                {
                    contents->notifyCloseApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CLOSE_APP;
                    contents->notifyCloseApp.header.devID = retDevID;
                    contents->notifyCloseApp.appID = iPodCtrlCfg->iOSAppID[i].appID;
                    iPodCtrlCfg->iOSAppID[i].appID = 0;
                    iPodCtrlCfg->iOSAppID[i].sessionID = 0;
                    iPodCtrlCfg->iOSAppID[i].isReady = 0;
                    rc = IPOD_PLAYER_OK;
                    
                }
            }
            
            break;
            
        case IPOD_FUNC_NOTIFY_RECEIVE_FROM_APP:
            {
                IPOD_PLAYER_PARAM_NOTIFY_RECEIVE_FROM_APP receiveApp;
                
                receiveApp.header = *header;
                memcpy(&((U8 *)&receiveApp)[sizeof(*header)], (const char*)contents, sizeof(receiveApp) - sizeof(receiveApp.header));
                rc = IPOD_PLAYER_ERROR;
                for(i = 0; (i < IPODCORE_MAX_IOSAPPS_INFO_NUM) && (rc != IPOD_PLAYER_OK); i++)
                {
                    if(iPodCtrlCfg->iOSAppID[i].sessionID == receiveApp.session)
                    {
                        sendBuf[0] = (U8 *)header;
                        sendSize[0] = sizeof(*header);
                        sendBuf[1] = (U8 *)contents;
                        sendSize[1] = size - sizeof(*header);
                        memcpy(&contents[0], &iPodCtrlCfg->iOSAppID[i].appID, sizeof(U32));
                        rc = IPOD_PLAYER_OK;
                    }
                }
            }
            break;
            
        case IPOD_FUNC_NOTIFY_DEVICE_EVENT:
            break;
            
        default:
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, funcId);
            break;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        if(header->longData == 0)
        {
            rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)contents, resSize, 0, IPODCORE_TMOUT_FOREVER);
            if(rc >= 0)
            {
                rc = IPOD_PLAYER_OK;
            }
        }
        else
        {
            rc = iPodPlayerIPCLongSend(iPodCtrlCfg->threadInfo->longCmdQueueClient, IPODCORE_LONG_DATA_ARRAY, sendBuf, sendSize, &asize, 0, -1);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlSendiPod(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitList, 
                            IPOD_PLAYER_CORE_FUNC_TABLE *funcTable, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 groupMask = 0;
    
    /* Parameter check */
    if((iPodCtrlCfg== NULL) || (waitList == NULL) || (funcTable == NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitList, funcTable, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    groupMask = (U32)header->funcId;
    /* Function ID is not notification from iPod Ctrl. */
    if((groupMask & IPOD_PLAYER_NOTIFY_FUNC_MASK) != IPOD_PLAYER_NOTIFY_FUNC_MASK)
    {
        /* Set the receving data to tail of queue */
        rc = iPodCoreSetQueue(waitList, header, contents, size);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCoreSortQueue(IPODCORE_WAIT_LIST_MAX, waitList);
        }
    }
    
    if (rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiPodCtrlExecuteQueue(iPodCtrlCfg, IPODCORE_WAIT_LIST_MAX, waitList, header, contents, size);
    }
    else
    {
        /* Send the error to Application */
        contents->paramResultTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | 
                                                                        contents->paramResultTemp.header.funcId);
        contents->paramResultTemp.result = rc;
        size = sizeof(contents->paramResultTemp);
        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)contents, size, 0, IPODCORE_TMOUT_FOREVER);
    }
    
    /* Remove the list that current status is finish */
    rc = iPodCoreDelQueue(waitList);
    /* Clear the current running mask */
    iPodCtrlCfg->curRunMask = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlInternalFunc(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_CORE_INT_FUNC funcId = IPOD_CORE_INT_FUNC_FIRST_GET_IPOD_INFO;
    
    /* For lint */
    header = header;
    size = size;
    
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    funcId = (IPOD_CORE_INT_FUNC)contents->paramTemp.header.funcId;
    switch(funcId)
    {
        case IPOD_CORE_INT_FUNC_SET_SAMPLERATE:
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
            rc = iPodCoreSetSampleRate(iPodCtrlCfg, contents, contents->setSample.sampleRate);
#endif
            iPodCoreFuncAccSample(iPodCtrlCfg->threadInfo->iPodDevID);
            break;
        
        default: break;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

static void iPodCoreiPodCtrlSetFuncTable(IPOD_PLAYER_CORE_FUNC_TABLE *funcTable)
{
    if(funcTable == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, funcTable);
        return;
    }
    
    /* Init function table */
    funcTable[IPOD_FUNC_INIT].func = iPodCoreiPodCtrlFuncInit;
    funcTable[IPOD_FUNC_INIT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    funcTable[IPOD_FUNC_DEINIT].func = iPodCoreiPodCtrlFuncDeInit;
    funcTable[IPOD_FUNC_DEINIT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    funcTable[IPOD_FUNC_SELECT_AUDIO_OUT].func = iPodCoreiPodCtrlFuncSelectAudioOut;
    funcTable[IPOD_FUNC_SELECT_AUDIO_OUT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    funcTable[IPOD_FUNC_START_AUTHENTICATION].func = iPodCoreiPodCtrlFuncStartAuthentication;
    funcTable[IPOD_FUNC_START_AUTHENTICATION].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    
/*
    funcTable[IPOD_FUNC_SELECT_AUDIO_OUT].func = iPodCoreiPodCtrlPlay;
    funcTable[IPOD_FUNC_TEST_READY].func = iPodCoreiPodCtrlPlay;
    funcTable[IPOD_FUNC_SET_IOS_APPS_INFO].func = iPodCoreiPodCtrlPlay;
    funcTable[IPOD_FUNC_START_AUTHENTICATION].func = iPodCoreiPodCtrlPlay;
    
*/
    /* Playback function table */
    funcTable[IPOD_FUNC_TRACK_INFO].func = iPodCoreiPodCtrlIntGetTrackInfo;
    funcTable[IPOD_FUNC_TRACK_INFO].attr = IPOD_PLAYER_NOTIFY_GROUP_MASK;
    funcTable[IPOD_FUNC_PLAY].func = iPodCoreiPodCtrlPlay;
    funcTable[IPOD_FUNC_PLAY].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PAUSE].func = iPodCoreiPodCtrlPause;
    funcTable[IPOD_FUNC_PAUSE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_STOP].func = iPodCoreiPodCtrlStop;
    funcTable[IPOD_FUNC_STOP].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_FASTFORWARD].func = iPodCoreiPodCtrlFastForward;
    funcTable[IPOD_FUNC_FASTFORWARD].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_REWIND].func = iPodCoreiPodCtrlRewind;
    funcTable[IPOD_FUNC_REWIND].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_NEXT_TRACK].func = iPodCoreiPodCtrlNextTrack;
    funcTable[IPOD_FUNC_NEXT_TRACK].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PREV_TRACK].func = iPodCoreiPodCtrlPrevTrack;
    funcTable[IPOD_FUNC_PREV_TRACK].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_NEXTCHAPTER].func = iPodCoreiPodCtrlNextChapter;
    funcTable[IPOD_FUNC_NEXTCHAPTER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PREVCHAPTER].func = iPodCoreiPodCtrlPrevChapter;
    funcTable[IPOD_FUNC_PREVCHAPTER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GOTO_TRACK_POSITION].func = iPodCoreiPodCtrlGotoTrackPosition;
    funcTable[IPOD_FUNC_GOTO_TRACK_POSITION].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PLAYTRACK].func = iPodCoreiPodCtrlPlayTrack;
    funcTable[IPOD_FUNC_PLAYTRACK].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_RELEASE].func = NULL;                               /* only iAP2 */
    funcTable[IPOD_FUNC_RELEASE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;    /* only iAP2 */
  
    /* Set property function table */
    funcTable[IPOD_FUNC_SET_AUDIO_MODE].func = iPodCoreiPodCtrlSetAudioMode;
    funcTable[IPOD_FUNC_SET_AUDIO_MODE].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_MODE].func = iPodCoreiPodCtrlSetMode;
    funcTable[IPOD_FUNC_SET_MODE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_REPEAT].func = iPodCoreiPodCtrlSetRepeat;
    funcTable[IPOD_FUNC_SET_REPEAT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_SHUFFLE].func = iPodCoreiPodCtrlSetShuffle;
    funcTable[IPOD_FUNC_SET_SHUFFLE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_EQUALIZER].func = iPodCoreiPodCtrlSetEqualizer;
    funcTable[IPOD_FUNC_SET_EQUALIZER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VIDEO_DELAY].func = iPodCoreiPodCtrlSetVideoDelay;
    funcTable[IPOD_FUNC_SET_VIDEO_DELAY].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VIDEO_SETTING].func = iPodCoreiPodCtrlSetVideoSetting;
    funcTable[IPOD_FUNC_SET_VIDEO_SETTING].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_DISPLAY_IMAGE].func = iPodCoreiPodCtrlSetDisplayImage;
    funcTable[IPOD_FUNC_SET_DISPLAY_IMAGE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_PLAY_SPEED].func = iPodCoreiPodCtrlSetPlaySpeed;
    funcTable[IPOD_FUNC_SET_PLAY_SPEED].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION].func = iPodCoreiPodCtrlSetTrackInfoNotification;
    funcTable[IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION].func = iPodCoreiPodCtrlSetEventNotification;
    funcTable[IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    
    /* Get Property function table */
    funcTable[IPOD_FUNC_GET_VIDEO_SETTING].func = iPodCoreiPodCtrlGetVideoSetting;
    funcTable[IPOD_FUNC_GET_VIDEO_SETTING].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_COVERART_INFO].func = iPodCoreiPodCtrlGetCoverartInfo;
    funcTable[IPOD_FUNC_GET_COVERART_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_COVERART].func = iPodCoreiPodCtrlGetCoverart;
    funcTable[IPOD_FUNC_GET_COVERART].attr = IPOD_PLAYER_COVERART_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_PLAYBACK_STATUS].func = iPodCoreiPodCtrlGetPlaybackStatus;
    funcTable[IPOD_FUNC_GET_PLAYBACK_STATUS].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_TRACK_INFO].func = iPodCoreiPodCtrlGetTrackInfo;
    funcTable[IPOD_FUNC_GET_TRACK_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_CHAPTER_INFO].func = iPodCoreiPodCtrlGetChapterInfo;
    funcTable[IPOD_FUNC_GET_CHAPTER_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_MODE].func = iPodCoreiPodCtrlGetMode;
    funcTable[IPOD_FUNC_GET_MODE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_REPEAT].func = iPodCoreiPodCtrlGetRepeat;
    funcTable[IPOD_FUNC_GET_REPEAT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_SHUFFLE].func = iPodCoreiPodCtrlGetShuffle;
    funcTable[IPOD_FUNC_GET_SHUFFLE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_PLAY_SPEED].func = iPodCoreiPodCtrlGetPlaySpeed;
    funcTable[IPOD_FUNC_GET_PLAY_SPEED].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_TRACK_TOTAL_COUNT].func = iPodCoreiPodCtrlGetTrackTotalCount;
    funcTable[IPOD_FUNC_GET_TRACK_TOTAL_COUNT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_EQUALIZER].func = iPodCoreiPodCtrlGetEqaulizer;
    funcTable[IPOD_FUNC_GET_EQUALIZER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_EQUALIZER_NAME].func = iPodCoreiPodCtrlGetEqualizerName;
    funcTable[IPOD_FUNC_GET_EQUALIZER_NAME].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_DEVICE_PROPERTY].func = iPodCoreiPodCtrlGetDeviceProperty;
    funcTable[IPOD_FUNC_GET_DEVICE_PROPERTY].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_DEVICE_STATUS].func = iPodCoreiPodCtrlGetDeviceStatus;
    funcTable[IPOD_FUNC_GET_DEVICE_STATUS].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    
    
    /* Database operation function table */
    funcTable[IPOD_FUNC_GET_DB_ENTRIES].func = iPodCoreiPodCtrlGetDBEntries;
    funcTable[IPOD_FUNC_GET_DB_ENTRIES].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_DB_COUNT].func = iPodCoreiPodCtrlGetDBCount;
    funcTable[IPOD_FUNC_GET_DB_COUNT].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_SELECT_DB_ENTRY].func = iPodCoreiPodCtrlSelectDBEntry;
    funcTable[IPOD_FUNC_SELECT_DB_ENTRY].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_CANCEL].func = iPodCoreiPodCtrlCancel;
    funcTable[IPOD_FUNC_CANCEL].attr = IPOD_PLAYER_CANCEL_GROUP_MASK;
    funcTable[IPOD_FUNC_CLEAR_SELECTION].func = iPodCoreiPodCtrlClearSelection;
    funcTable[IPOD_FUNC_CLEAR_SELECTION].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_SELECT_AV].func = iPodCoreiPodCtrlSelectAV;
    funcTable[IPOD_FUNC_SELECT_AV].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    
    /* Non Player function table */
    funcTable[IPOD_FUNC_OPEN_SONG_TAG_FILE].func = iPodCoreiPodCtrlOpenSongTag;
    funcTable[IPOD_FUNC_OPEN_SONG_TAG_FILE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_CLOSE_SONG_TAG_FILE].func = iPodCoreiPodCtrlCloseSongTag;
    funcTable[IPOD_FUNC_CLOSE_SONG_TAG_FILE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SONG_TAG].func = iPodCoreiPodCtrlSongTag;
    funcTable[IPOD_FUNC_SONG_TAG].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SEND_TO_APP].func = iPodCoreiPodCtrlSendToApp;
    funcTable[IPOD_FUNC_SEND_TO_APP].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_REQUEST_APP_START].func = iPodCoreiPodCtrlRequestAppStart;
    funcTable[IPOD_FUNC_REQUEST_APP_START].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VOLUME].func = iPodCoreiPodCtrlSetVolume;
    funcTable[IPOD_FUNC_SET_VOLUME].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_VOLUME].func = iPodCoreiPodCtrlGetVolume;
    funcTable[IPOD_FUNC_GET_VOLUME].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_LOCATION_INFO].func = NULL;
    funcTable[IPOD_FUNC_SET_LOCATION_INFO].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VEHICLE_STATUS].func = NULL;
    funcTable[IPOD_FUNC_SET_VEHICLE_STATUS].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_PAI_RESULT].func = iPodCoreSetSampleRateResult;
    funcTable[IPOD_FUNC_PAI_RESULT].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_GET_STATUS].func = iPodCoreiPodCtrlIntGetStatus;
    funcTable[IPOD_FUNC_INTERNAL_GET_STATUS].attr = IPOD_PLAYER_INTERNAL_GET_STATUS_MASK;

    funcTable[IPOD_FUNC_INTERNAL_NOTIFY_STATUS].func = iPodCoreiPodCtrlNotifyStatus;
    funcTable[IPOD_FUNC_INTERNAL_NOTIFY_STATUS].attr = IPOD_PLAYER_STATUS_GROUP_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_HMI_STATUS_SEND].func = iPodCoreiPodCtrlHMIStatusSend;
    funcTable[IPOD_FUNC_INTERNAL_HMI_STATUS_SEND].attr = IPOD_PLAYER_INTERNAL_HMI_STATUS_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND].func = iPodCoreiPodCtrlHMIButtonStatusSend;
    funcTable[IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND].attr = IPOD_PLAYER_INTERNAL_HMI_STATUS_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_REMOTE_EVENT_NOTIFICATION].func = iPodCoreiPodCtrlRemoteEventNotification;
    funcTable[IPOD_FUNC_INTERNAL_REMOTE_EVENT_NOTIFICATION].attr = IPOD_PLAYER_STATUS_GROUP_MASK;
    
    /* HMI Operation Function table. */
    funcTable[IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE].func = iPodCoreiPodCtrlHMISetSupportedFeature;
    funcTable[IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE].func = iPodCoreiPodCtrlHMIGetSupportedFeature;
    funcTable[IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_BUTTON_INPUT].func = iPodCoreiPodCtrlHMIButtonInput;
    funcTable[IPOD_FUNC_HMI_BUTTON_INPUT].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_ROTATION_INPUT].func = iPodCoreiPodCtrlHMIRotationInput;
    funcTable[IPOD_FUNC_HMI_ROTATION_INPUT].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_PLAYBACK_INPUT].func = NULL;
    funcTable[IPOD_FUNC_HMI_PLAYBACK_INPUT].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_SET_APPLICATION_STATUS].func = iPodCoreiPodCtrlHMISetApplicationStatus;
    funcTable[IPOD_FUNC_HMI_SET_APPLICATION_STATUS].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION].func = NULL;
    funcTable[IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_GET_EVENT_CHANGE].func = NULL;
    funcTable[IPOD_FUNC_HMI_GET_EVENT_CHANGE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_GET_DEVICE_STATUS].func = NULL;
    funcTable[IPOD_FUNC_HMI_GET_DEVICE_STATUS].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    return;
}

static void iPodCoreiAP1Deinit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
     /* Check parameter */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }
    
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    if(iPodCtrlCfg->startAudio > 0)
    {
        iPodCoreiPodCtrlStopAudio(iPodCtrlCfg);
    }
    
    if(iPodCtrlCfg->sckAudioServer >= 0)
    {
        iPodCoreiPodCtrlAudioDeinit(iPodCtrlCfg);
    }
#endif
    
    if(iPodCtrlCfg->threadInfo->iPodDevID > 0)
    {
        iPodCoreFuncDevDeinit(iPodCtrlCfg->threadInfo->iPodDevID);
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->threadInfo->iPodDevID);
        iPodCtrlCfg->threadInfo->iPodDevID = 0;
    }
    
    if(iPodCtrlCfg->threadInfo->iOSAppCount > 0)
    {
        /* Clear the iOSApp information */
        iPodCoreFuncSetiOSAppInfo(iPodCtrlCfg->threadInfo->nameInfo.deviceName, iPodCtrlCfg->threadInfo->iOSAppInfo, 0);
    }
    
    iPodCoreiPodCtrlDeleteCB();
    
    if(iPodCtrlCfg->funcTable != NULL)
    {
        free(iPodCtrlCfg->funcTable);
        iPodCtrlCfg->funcTable = NULL;
    }
    
    return;
}

static S32 iPodCoreiAP1Init(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
     /* Check parameter */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    iPodCtrlCfg->funcTable = calloc(IPOD_FUNC_MAX, sizeof(IPOD_PLAYER_CORE_FUNC_TABLE));
    if(iPodCtrlCfg->funcTable != NULL)
    {
        iPodCoreiPodCtrlSetFuncTable((IPOD_PLAYER_CORE_FUNC_TABLE *)iPodCtrlCfg->funcTable);
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, 0);
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiPodCtrlAudioInit(iPodCtrlCfg);
    }
#endif
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiPodCtrlRegisterCB();
        if(rc == IPOD_PLAYER_OK)
        {
            if(iPodCtrlCfg->threadInfo->iOSAppCount > 0)
            {
                rc = iPodCoreFuncSetiOSAppInfo(iPodCtrlCfg->threadInfo->nameInfo.deviceName, iPodCtrlCfg->threadInfo->iOSAppInfo, iPodCtrlCfg->threadInfo->iOSAppCount);
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                IPOD_ACC_INFO_CONFIGURATION cfg = {0};

                if(iPodCtrlCfg->threadInfo->accInfo.Name[0] != 0)
                {
                    if(strnlen((char *)(iPodCtrlCfg->threadInfo->accInfo.Name), IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX) >= IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX)
                    {
                        iPodCtrlCfg->threadInfo->accInfo.Name[IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX - 1] = '\0';
                    }
                    cfg.Name = iPodCtrlCfg->threadInfo->accInfo.Name;
                }
                if(iPodCtrlCfg->threadInfo->accInfo.Manufacturer [0] != 0)
                {
                    if(strnlen((char *)(iPodCtrlCfg->threadInfo->accInfo.Manufacturer), IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX) >= IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX)
                    {
                        iPodCtrlCfg->threadInfo->accInfo.Manufacturer[IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX - 1] = '\0';
                    }
                    cfg.Manufacturer = iPodCtrlCfg->threadInfo->accInfo.Manufacturer;
                }
                if(iPodCtrlCfg->threadInfo->accInfo.ModelNumber[0] != 0)
                {
                    if(strnlen((char *)(iPodCtrlCfg->threadInfo->accInfo.ModelNumber), IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX) >= IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX)
                    {
                        iPodCtrlCfg->threadInfo->accInfo.ModelNumber[IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX - 1] = '\0';
                    }
                    cfg.ModelNumber = iPodCtrlCfg->threadInfo->accInfo.ModelNumber;
                }
                if(iPodCtrlCfg->threadInfo->accInfo.SerialNumber[0] != 0)
                {
                    if(strnlen((char *)(iPodCtrlCfg->threadInfo->accInfo.SerialNumber), IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX) >= IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX)
                    {
                        iPodCtrlCfg->threadInfo->accInfo.SerialNumber[IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX - 1] = '\0';
                    }
                    cfg.SerialNumber = iPodCtrlCfg->threadInfo->accInfo.SerialNumber;
                }
                cfg.Hardware_version.Major_Number = iPodCtrlCfg->threadInfo->accInfo.Hardware_version.Major_Number;
                cfg.Hardware_version.Minor_Number = iPodCtrlCfg->threadInfo->accInfo.Hardware_version.Minor_Number;
                cfg.Hardware_version.Revision_Number = iPodCtrlCfg->threadInfo->accInfo.Hardware_version.Revision_Number;
                cfg.Software_version.Major_Number = iPodCtrlCfg->threadInfo->accInfo.Software_version.Major_Number;
                cfg.Software_version.Minor_Number = iPodCtrlCfg->threadInfo->accInfo.Software_version.Minor_Number;
                cfg.Software_version.Revision_Number = iPodCtrlCfg->threadInfo->accInfo.Software_version.Revision_Number;

                if(iPodInitAccessoryConnection(cfg) != IPOD_OK)
                {
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_DLT_ERROR("Accessory information data of iPodPlayerSetDeviceDetection is invalid parameter.");
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                }
                else
                {
                    rc = iPodCoreFuncDevInit(iPodCtrlCfg->threadInfo->nameInfo.deviceName, iPodCtrlCfg->threadInfo->nameInfo.devType);
                    if(rc > 0)
                    {
                        iPodCtrlCfg->threadInfo->iPodDevID = rc;
                        rc = IPOD_PLAYER_OK;
                    }
                }
            }
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        iPodCoreiAP1Deinit(iPodCtrlCfg);
    }
    
    return rc;
}

/**
 * iPodCoreiPodCtrlInit
 *
 * This function initializes the resource of iPod Control thread
 * 
 **/
static S32 iPodCoreiPodCtrlInit(IPOD_PLAYER_CORE_THREAD_INFO *iPodCfg, IPOD_PLAYER_CORE_IPODCTRL_CFG **iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 dev_num = 0;
    IPOD_PLAYER_IPC_OPEN_INFO info;
    U8 tempName[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    U8 semName[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    S32 handle = -1;
    
    /* Check parameter */
    if((iPodCfg == NULL) || (iPodCtrlCfg == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    *iPodCtrlCfg = calloc(1, sizeof(**iPodCtrlCfg));
    if(*iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_NOMEM;
    }
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    
    /* Initialize pass flag for authentication and identification */
    (*iPodCtrlCfg)->authIdenPass.authPass = FALSE;
    (*iPodCtrlCfg)->authIdenPass.idenPass = FALSE;

    memset((*iPodCtrlCfg)->handle, 0, sizeof((*iPodCtrlCfg)->handle));
    (*iPodCtrlCfg)->handleNum = 0;
    (*iPodCtrlCfg)->sckAudio = -1;
    (*iPodCtrlCfg)->sckAudioServer = -1;
    (*iPodCtrlCfg)->startAudio = 0;
    (*iPodCtrlCfg)->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
    (*iPodCtrlCfg)->curRunMask = 0;
    (*iPodCtrlCfg)->audioSetting.mode = IPOD_PLAYER_SOUND_MODE_OFF;
    (*iPodCtrlCfg)->audioSetting.adjust = IPOD_PLAYER_STATE_ADJUST_DISABLE;
    (*iPodCtrlCfg)->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_UNKNOWN;
    (*iPodCtrlCfg)->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_UNKNOWN;
    (*iPodCtrlCfg)->deviceConnection.powerStatus = IPOD_PLAYER_POWER_UNKNOWN;
    (*iPodCtrlCfg)->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
    (*iPodCtrlCfg)->defaultProtocol = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEFAULT_PROTOCOL);
    
    /* iOS APP INFORMATION Initialize */
    for(i = 0; i < IPODCORE_MAX_IOSAPPS_INFO_NUM; i++)
    {
        (*iPodCtrlCfg)->iOSAppID[i].appID = 0;
        (*iPodCtrlCfg)->iOSAppID[i].isReady = 0;
        (*iPodCtrlCfg)->iOSAppID[i].sessionID = 0;
        (*iPodCtrlCfg)->iOSAppID[i].status = IPODCORE_IOSAPP_NONE;
    }

    /* location information status initilize */
    (*iPodCtrlCfg)->delayedEvent.locationNow.status = IPOD_PLAYER_LOCATION_INFO_UNKNOWN;
    (*iPodCtrlCfg)->delayedEvent.locationNow.locationMask = 0;
    /* vehicale status massage initilize */
    (*iPodCtrlCfg)->delayedEvent.vehicleNow.status = IPOD_PLAYER_VEHICLE_STATUS_UNKNOWN;
    
    /* last event flag for init api initilize */
    (*iPodCtrlCfg)->delayedEvent.lastEventNotify = FALSE;

    /* Mutex for iPodCtrlCfg initilize */
    rc = ippiAP2InitServiceCallbacksMutex(*iPodCtrlCfg);
    
    /* Update Progress flag for DataBase update initilize */
    (*iPodCtrlCfg)->dbUpdateProgress = FALSE;
    /* Update Playlist flag for DataBase update initilize */
    (*iPodCtrlCfg)->dbUpdatePlaylist = FALSE;
    /* start mediaLibraryUpdate */
    (*iPodCtrlCfg)->startMediaLib = FALSE;

    /* Allocate the memory */
    if(rc == IPOD_PLAYER_OK)
    {
        (*iPodCtrlCfg)->contents = calloc(1, sizeof(*(*iPodCtrlCfg)->contents));
        if((*iPodCtrlCfg)->contents != NULL)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        (*iPodCtrlCfg)->iPodInfo = calloc(1, sizeof(*(*iPodCtrlCfg)->iPodInfo));
        if((*iPodCtrlCfg)->iPodInfo != NULL)
        {
            (*iPodCtrlCfg)->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->speed = (U8)IPOD_PLAYER_PLAYING_SPEED_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->shuffleStatus = (U8)IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->repeatStatus = (U8)IPOD_PLAYER_REPEAT_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->equalizer = 0xFF;
            (*iPodCtrlCfg)->iPodInfo->maxEq = 0;
            (*iPodCtrlCfg)->iPodInfo->sampleRate = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_AUDIO_DEFAULT_SAMPLE);
            (*iPodCtrlCfg)->iPodInfo->trackInfoMask = 0;
            (*iPodCtrlCfg)->iPodInfo->deviceEventMask = 0;
            (*iPodCtrlCfg)->iPodInfo->formatId = 0;
            (*iPodCtrlCfg)->iPodInfo->mode = (U8)IPOD_PLAYER_MODE_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->topType = IPOD_PLAYER_DB_TYPE_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->getDBprevTime = 0;
            (*iPodCtrlCfg)->iPodInfo->curTotalType = IPOD_PLAYER_DB_TYPE_UNKNOWN;
            (*iPodCtrlCfg)->iPodInfo->curTotalCount = 0;
            (*iPodCtrlCfg)->iPodInfo->getTrackInfoFd = -1;
            (*iPodCtrlCfg)->iPodInfo->getintTrackInfoFd = -1;
            (*iPodCtrlCfg)->iPodInfo->getDBprevTimeFd = -1;
            (*iPodCtrlCfg)->iPodInfo->nextRequestFd = -1;
            
            pthread_mutex_init(&((*iPodCtrlCfg)->iPodInfo->waitMutex), NULL);
            pthread_cond_init(&((*iPodCtrlCfg)->iPodInfo->waitCond), NULL);
            
            memset((*iPodCtrlCfg)->iPodInfo->catCountList, 0, sizeof((*iPodCtrlCfg)->iPodInfo->catCountList));
            rc = iPodCoreiPodCtrlWaitInit((*iPodCtrlCfg)->iPodInfo);
            if(rc == IPOD_PLAYER_OK)
            {
                (*iPodCtrlCfg)->iPodInfo->timeWaitFlag = 1;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        (*iPodCtrlCfg)->waitList = calloc(IPODCORE_WAIT_LIST_MAX, sizeof(*(*iPodCtrlCfg)->waitList));
        if((*iPodCtrlCfg)->waitList == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Create epoll */
        rc = epoll_create(IPOD_PLAYER_IPC_MAX_EPOLL_NUM);
        if(rc > 0)
        {
            /* Set handle */
            (*iPodCtrlCfg)->waitHandle = rc;
            /* Register the handle to ipc library */
            rc = iPodPlayerIPCCreateHandle(&handle,  (*iPodCtrlCfg)->waitHandle);
            if(rc == IPOD_PLAYER_OK)
            {
                rc = iPodCoreSetHandle((*iPodCtrlCfg)->handle, &((*iPodCtrlCfg)->handleNum), (*iPodCtrlCfg)->waitHandle);
            }
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    
    if(rc == IPOD_PLAYER_OK)
    {
        if((*iPodCtrlCfg)->threadInfo == NULL)
        {
            (*iPodCtrlCfg)->threadInfo = iPodCfg;
            snprintf((char *)tempName, sizeof(tempName), "%s%d_long", IPODCORE_IPOD_HANDLE_IDENTIFY, (*iPodCtrlCfg)->threadInfo->appDevID);
            
            info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG;
            info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
            info.identify = (U8 *)tempName;
            info.connectionNum = IPODCORE_QUEUE_MAX_NUM;
            info.semName = NULL;
            info.maxBufSize = (*iPodCtrlCfg)->threadInfo->maxMsgSize;
            info.maxPacketSize = (*iPodCtrlCfg)->threadInfo->maxMsgSize;
            rc = iPodPlayerIPCOpen(&(*iPodCtrlCfg)->threadInfo->longQueueServer, &info);
            if(rc == IPOD_PLAYER_OK)
            {
                snprintf((char *)semName, sizeof(semName), "/tmp_sem%s", tempName);
                info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG;
                info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
                info.identify = (U8 *)tempName;
                info.connectionNum = IPODCORE_QUEUE_MAX_NUM;
                info.semName = semName;
                info.maxBufSize = (*iPodCtrlCfg)->threadInfo->maxMsgSize;
                info.maxPacketSize = (*iPodCtrlCfg)->threadInfo->maxMsgSize;
                rc = iPodPlayerIPCOpen(&(*iPodCtrlCfg)->threadInfo->longQueueClient, &info);
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                snprintf((char *)semName, sizeof(semName), "/tmp_sem%s%d", IPODCORE_COMMAND_QUEUE_LONG_IDENTIFY, (*iPodCtrlCfg)->threadInfo->appDevID);
                info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG;
                info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
                info.identify = (U8 *)IPODCORE_COMMAND_QUEUE_LONG_IDENTIFY;
                info.connectionNum = IPODCORE_QUEUE_MAX_NUM;
                info.semName = semName;
                info.maxBufSize = (*iPodCtrlCfg)->threadInfo->maxMsgSize;
                info.maxPacketSize = (*iPodCtrlCfg)->threadInfo->maxMsgSize;
                rc = iPodPlayerIPCOpen(&(*iPodCtrlCfg)->threadInfo->longCmdQueueClient, &info);
            }
            
            if((rc == IPOD_PLAYER_OK) && (g_threadInfo != NULL))
            {
                /* get cfg max device number */
                dev_num = iPodCoreGetIndexCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM, 0);
                /* check all thread info */
                for(i = 0; i < dev_num; i++)
                {
                    /* found null thread info */
                    if(g_threadInfo[i] == NULL)
                    {
                        /* Register the queue for callback function */
                        g_threadInfo[i] = (*iPodCtrlCfg)->threadInfo;
                        break;
                    }
                }
            }
            
            (*iPodCtrlCfg)->threadInfo->aTime = (*iPodCtrlCfg)->threadInfo->timeout;
            rc = iPodCoreSetHandle((*iPodCtrlCfg)->handle, &(*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->threadInfo->queueInfo);
            rc = iPodCoreSetHandle((*iPodCtrlCfg)->handle, &(*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->threadInfo->longQueueServer);
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* Initialize the long receive info structure */
                iPodCoreInitLongRecvInfo((*iPodCtrlCfg)->longRecvInfo);
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiPodCtrlTimerInit(&(*iPodCtrlCfg)->iPodInfo->getTrackInfoFd, &(*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->handle);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreiPodCtrlTimerInit(&(*iPodCtrlCfg)->iPodInfo->getintTrackInfoFd, &(*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->handle);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreiPodCtrlTimerInit(&(*iPodCtrlCfg)->iPodInfo->getDBprevTimeFd, &(*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->handle);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreiPodCtrlTimerInit(&(*iPodCtrlCfg)->iPodInfo->nextRequestFd, &(*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->handle);
        }
    }

    if(rc == IPOD_PLAYER_OK)
    {
        /* Init eventFd for iAPP2ServiceCallbackRequest */
        rc = ippiAP2InitServiceCallbackRequest(*iPodCtrlCfg);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Enqueue to CtrlCfgList */
            rc = ippiAP2EnqueueToCtrlCfgList(*iPodCtrlCfg);
        }
    }
    
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiPodCtrlAudioInitIPC(*iPodCtrlCfg);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCoreSetHandle((*iPodCtrlCfg)->handle, &((*iPodCtrlCfg)->handleNum), (*iPodCtrlCfg)->sckAudioServer);
        }
    }
#endif
    
    /* Check the result */
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCoreAddFDs((*iPodCtrlCfg)->waitHandle, (*iPodCtrlCfg)->handleNum, (*iPodCtrlCfg)->handle);
    }
    
    /* Check the result */
    if(rc == IPOD_PLAYER_OK)
    {
        /* Device status is updated to connect */
        (*iPodCtrlCfg)->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        /* iAP2 session start flag is initialized */
        (*iPodCtrlCfg)->StartSession = FALSE;
    }
    else
    {
        /* Device status is updated to disconnect */
        (*iPodCtrlCfg)->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

static void iPodCoreiPodCtrlDeinit(void *arg)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)arg;
    U32 i = 0;
    U32 dev_num = 0;            /* for max number of device */
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, arg);
    
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->threadInfo);
    
    if(iPodCtrlCfg->threadInfo != NULL)
    {
        if(iPodCtrlCfg->xmlText != NULL)
        {
            /* Free text writer. After it, xml buffer can be used */
            xmlFreeTextWriter(iPodCtrlCfg->xmlText);
            iPodCtrlCfg->xmlText = NULL;
        }
        
        if(iPodCtrlCfg->xmlBuf != NULL)
        {
            xmlBufferFree(iPodCtrlCfg->xmlBuf);
            xmlCleanupParser();
            iPodCtrlCfg->xmlBuf = NULL;
        }
        
        if(iPodCtrlCfg->curTagPos != 0)
        {
            iPodCtrlCfg->curTagPos = 0;
        }
    }
    
    if(iPodCtrlCfg->waitList != NULL)
    {
        for(i =0; i < IPODCORE_WAIT_LIST_MAX; i++)
        {
            if(iPodCtrlCfg->waitList[i].storeBuf != NULL)
            {
                free(iPodCtrlCfg->waitList[i].storeBuf);
                iPodCtrlCfg->waitList[i].storeBuf = NULL;
            }
        }
        
        free(iPodCtrlCfg->waitList);
        iPodCtrlCfg->waitList = NULL;
    }
    
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    
    if(iPodCtrlCfg->sckAudioServer >= 0)
    {
        iPodCoreiPodCtrlAudioDeinitIPC(iPodCtrlCfg);
    }
#endif
    
    if(iPodCtrlCfg->iPodInfo != NULL)
    {
        if(iPodCtrlCfg->iPodInfo->timeWaitFlag != 0)
        {
            iPodCoreiPodCtrlWaitDeinit(iPodCtrlCfg->iPodInfo);
            iPodCtrlCfg->iPodInfo->timeWaitFlag = 0;
        }
        
        if(iPodCtrlCfg->iPodInfo->getTrackInfoFd >= 0)
        {
            iPodCoreiPodCtrlTimerDeinit(iPodCtrlCfg->iPodInfo->getTrackInfoFd, &iPodCtrlCfg->handleNum, iPodCtrlCfg->handle);
            iPodCtrlCfg->iPodInfo->getTrackInfoFd = -1;
        }
        
        if(iPodCtrlCfg->iPodInfo->getintTrackInfoFd >= 0)
        {
            iPodCoreiPodCtrlTimerDeinit(iPodCtrlCfg->iPodInfo->getintTrackInfoFd, &iPodCtrlCfg->handleNum, iPodCtrlCfg->handle);
            iPodCtrlCfg->iPodInfo->getintTrackInfoFd = -1;
        }
        
        if(iPodCtrlCfg->iPodInfo->getDBprevTimeFd >= 0)
        {
            iPodCoreiPodCtrlTimerDeinit(iPodCtrlCfg->iPodInfo->getDBprevTimeFd, &iPodCtrlCfg->handleNum, iPodCtrlCfg->handle);
            iPodCtrlCfg->iPodInfo->getDBprevTimeFd = -1;
        }
        
        if(iPodCtrlCfg->iPodInfo->nextRequestFd >= 0)
        {
            iPodCoreiPodCtrlTimerDeinit(iPodCtrlCfg->iPodInfo->nextRequestFd, &iPodCtrlCfg->handleNum, iPodCtrlCfg->handle);
            iPodCtrlCfg->iPodInfo->nextRequestFd = -1;
        }
        
        free(iPodCtrlCfg->iPodInfo);
        iPodCtrlCfg->iPodInfo = NULL;
    }
    
    if(iPodCtrlCfg->threadInfo != NULL)
    {
        if(iPodCtrlCfg->threadInfo->longQueueClient >= 0)
        {
            iPodPlayerIPCClose(iPodCtrlCfg->threadInfo->longQueueClient);
            iPodCtrlCfg->threadInfo->longQueueClient = -1;
        }
        
        if(iPodCtrlCfg->threadInfo->longQueueServer >= 0)
        {
            iPodCoreClearHandle(iPodCtrlCfg->handle, &iPodCtrlCfg->handleNum, iPodCtrlCfg->threadInfo->longQueueServer);
            iPodPlayerIPCClose(iPodCtrlCfg->threadInfo->longQueueServer);
            iPodCtrlCfg->threadInfo->longQueueServer = -1;
        }
        
        if(iPodCtrlCfg->threadInfo->longCmdQueueClient >= 0)
        {
            iPodPlayerIPCClose(iPodCtrlCfg->threadInfo->longCmdQueueClient);
            iPodCtrlCfg->threadInfo->longCmdQueueClient = -1;
        }
        
        if(g_threadInfo != NULL)
        {
            /* get cfg max device number */
            dev_num = iPodCoreGetIndexCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM, 0);
            /* check all thread info */
            for(i = 0; i < dev_num; i++)
            {
                /* found current thread info */
                if(g_threadInfo[i] == iPodCtrlCfg->threadInfo)
                {
                    /* clear */
                    g_threadInfo[i] = NULL;
                    break;
                }
            }
        }
        
        if(iPodCtrlCfg->threadInfo != NULL)
        {
            iPodCtrlCfg->threadInfo = NULL;
        }
    }
    
    if(iPodCtrlCfg->waitHandle > 0)
    {
        iPodPlayerIPCDeleteHandle(iPodCtrlCfg->waitHandle);
        close(iPodCtrlCfg->waitHandle);
        iPodCtrlCfg->waitHandle = -1;
    }
    
    /* Contents data is already allocated */
    if(iPodCtrlCfg->contents != NULL)
    {
        free(iPodCtrlCfg->contents);
        iPodCtrlCfg->contents = NULL;
    }
    
    free(iPodCtrlCfg);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
    
}

/* Continue the asynchronous event without acknowledge */
static S32 iPodCoreiPodCtrlRequestContinue(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, S32 fd, IPOD_PLAYER_CORE_WAIT_LIST *waitList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U64 times = 0;
    U32 i = 0;
    U32 size = 0;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents = NULL;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (waitList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitList);
        return IPOD_PLAYER_ERROR;
    }
    
    /* Read current timer count */
    rc = read(fd, &times, sizeof(times));
    if(rc >= 0)
    {
        /* Loop until queueu count */
        for(i = 0; i < IPODCORE_WAIT_LIST_MAX; i++)
        {
            /* Current queue status is not finish and none */
            if((waitList[i].status != IPODCORE_QUEUE_STATUS_FINISH) && (waitList[i].status != IPODCORE_QUEUE_STATUS_NONE))
            {
                /* Current queue allow to conitnue the request */
                if(waitList[i].nextRequest != 0)
                {
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitList[i].contents.paramTemp.header.funcId);
                    /* Copy the data of queue as request */
                    iPodCtrlCfg->contents->paramTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)
                        (IPOD_PLAYER_NOTIFY_FUNC_MASK | waitList[i].contents.paramTemp.header.funcId);
                    iPodCtrlCfg->contents->paramTemp.header.appID = waitList[i].contents.paramTemp.header.appID;
                    iPodCtrlCfg->contents->paramTemp.header.devID = waitList[i].contents.paramTemp.header.devID;
                    iPodCtrlCfg->contents->paramTemp.header.longData = 0;
                    contents = iPodCtrlCfg->contents;
                    /* Execute the request */
                    rc = iPodCoreiPodCtrlSendiPod(iPodCtrlCfg, waitList, (IPOD_PLAYER_CORE_FUNC_TABLE *)iPodCtrlCfg->funcTable, &contents->paramTemp.header, contents, size);
                }
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    return rc;
}

/* Get the request from application, timer event and iPodCtrl */
static S32 iPodCoreiPodCtrlGetRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 type, 
                                        S32 fd, IPOD_PLAYER_MESSAGE_DATA_CONTENTS **contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfo = NULL;
    IPOD_PLAYER_PARAM_NOTIFY_PLAYBACK_STATUS playbackStatus;
    U64 timeCount = 0;
    
    memset(&playbackStatus, 0, sizeof(playbackStatus));
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the request from application by long receive */
    if(type == IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG)
    {
        /* Get the long receive information from file descriptor */
        recvInfo = iPodCoreGetLongRecvInfo(iPodCtrlCfg->longRecvInfo, fd);
        if(recvInfo != NULL)
        {
            /* Receive the data */
            rc = iPodPlayerIPCLongReceive(recvInfo->fd, recvInfo->num, &recvInfo->buf[IPODCORE_LONG_HEADER_POS], 
                                            &recvInfo->size[IPODCORE_LONG_HEADER_POS], size, 0, iPodCtrlCfg->waitHandle, -1);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                /* Copy the header information*/
                memcpy(&iPodCtrlCfg->contents->paramTemp.header, &recvInfo->header, sizeof(iPodCtrlCfg->contents->paramTemp.header));
                *contents = (IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)(void *)recvInfo->buf[IPODCORE_LONG_DATA_POS];
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->contents->paramTemp.header.funcId);
                rc = IPOD_PLAYER_OK;
            }
            else if(rc == IPOD_PLAYER_IPC_ERR_NOMEM)
            {
                /* Previous buffer still remain */
                if(recvInfo->buf[IPODCORE_LONG_DATA_POS] != NULL)
                {
                    /* Free previous buffer */
                    free(recvInfo->buf[IPODCORE_LONG_DATA_POS]);
                    recvInfo->buf[IPODCORE_LONG_DATA_POS] = NULL;
                }
                
                /* Allocate buffer until size - header size */
                recvInfo->buf[IPODCORE_LONG_DATA_POS] = calloc(*size - recvInfo->size[IPODCORE_LONG_HEADER_POS], sizeof(U8));
                if(recvInfo->buf[IPODCORE_LONG_DATA_POS] != NULL)
                {
                    recvInfo->size[IPODCORE_LONG_DATA_POS] = *size - recvInfo->size[IPODCORE_LONG_HEADER_POS];
                    recvInfo->num = IPODCORE_LONG_DATA_ARRAY;
                    /* Register receive informaiton again */
                    rc = iPodCoreSetLongRecvInfo(iPodCtrlCfg->longRecvInfo, fd, recvInfo);
                }
                
                /* Anyway error because request is still not received */
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
            else if(rc == IPOD_PLAYER_IPC_ERR_MORE_PACKET)
            {
                rc = IPOD_PLAYER_ERR_MORE_PACKET;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        /* GetTrackInfo timer event is occurred */
        if(fd == iPodCtrlCfg->iPodInfo->getTrackInfoFd)
        {
            /* Read the time count */
            rc = read(fd, &timeCount, sizeof(timeCount));
            if(rc >= 0)
            {
                /* Set the request to execute the request. */
                iPodCtrlCfg->contents->getTrackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | 
                                                                                          IPOD_FUNC_GET_TRACK_INFO);
                iPodCtrlCfg->contents->getTrackInfo.header.devID = 0;
                iPodCtrlCfg->contents->getTrackInfo.header.appID = 0;
                iPodCtrlCfg->contents->getTrackInfo.trackInfoMask = 0;
                iPodCtrlCfg->contents->getTrackInfo.resultFlag = 0;
                *contents = iPodCtrlCfg->contents;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
                rc = IPOD_PLAYER_ERR_TIMER;
            }
        }
        /* Internally GetTrackInfo timer event is occurred */
        else if(fd == iPodCtrlCfg->iPodInfo->getintTrackInfoFd)
        {
            /* Read the time count */
            rc = read(fd, &timeCount, sizeof(timeCount));
            if(rc >= 0)
            {
                /* Set the request to execute the request */
                iPodCtrlCfg->contents->notifyTrackInfo.trackIndex = 0;
                iPodCtrlCfg->contents->notifyTrackInfo.info.trackInfoMask = 0;
                iPodCtrlCfg->contents->notifyTrackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | 
                                                                                             IPOD_FUNC_TRACK_INFO);
                iPodCtrlCfg->contents->notifyTrackInfo.header.devID = 0;
                iPodCtrlCfg->contents->notifyTrackInfo.header.appID = 0;
                *contents = iPodCtrlCfg->contents;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
                rc = IPOD_PLAYER_ERR_TIMER;
            }
        }
        /* Asynchronous timer event is occurred */
        else if(fd == iPodCtrlCfg->iPodInfo->nextRequestFd)
        {
            /* Execute the asynchronous request without acknowledge */
            rc = iPodCoreiPodCtrlRequestContinue(iPodCtrlCfg, fd, iPodCtrlCfg->waitList);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Anyway error because request has already been executed */
                rc = IPOD_PLAYER_ERROR;
            }
        }
        /* Get the request from application by short data */
        else
        {
            /* Receive message from queue */
            rc = iPodPlayerIPCReceive(fd, (U8 *)iPodCtrlCfg->contents, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS), 
                                      0, iPodCtrlCfg->waitHandle, IPODCORE_TMOUT_FOREVER);
            if(rc >= 0)
            {
                *size = rc;
                *contents = iPodCtrlCfg->contents;
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->contents->paramTemp.header.funcId);
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


static S32 iPodCoreiPodCtrlExecuteRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_FUNC_HEADER *header,
                                          IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 checkId = 0;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    checkId = (U32)header->funcId;
    /* Check the request wheter finalize event*/
    if(checkId != IPOD_FUNC_DEVDEINIT)
    {
        /* Function ID is added the result mask */
        if((checkId & IPOD_PLAYER_RESULT_ID_MASK) == (IPOD_PLAYER_FUNC_ID)IPOD_PLAYER_RESULT_ID_MASK)
        {
            /* Prepare to send the data to Application */
            rc = iPodCoreiPodCtrlSendCB(iPodCtrlCfg, header, contents, size);
        }
        /* Function ID is added the internal function mask */
        else if((checkId & IPOD_PLAYER_INTERNAL_FUNC_MASK) == (IPOD_PLAYER_FUNC_ID)IPOD_PLAYER_INTERNAL_FUNC_MASK)
        {
            /* Execute interna function */
            rc = iPodCoreiPodCtrlInternalFunc(iPodCtrlCfg, header, contents, size);
        }
        /* Normal request */
        else
        {
            /* funcTable set as pointer */
            rc = iPodCoreiPodCtrlSendiPod(iPodCtrlCfg, iPodCtrlCfg->waitList, (IPOD_PLAYER_CORE_FUNC_TABLE *)iPodCtrlCfg->funcTable, header, contents, size);
        }
    }
    else
    {
        /* Returns disconnected error. It means that thread will be deleted */
        rc = IPOD_PLAYER_ERR_DISCONNECTED;
    }
    
    return rc;
}

void iPodCoreiPodCtrlCheckAndExecuteInternalFunc(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 funcId,
                                                 IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 queueCheck = 0;
    U32 i = 0;
    IPOD_PLAYER_CORE_FUNC_TABLE *funcTable = NULL;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (header == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, header, contents, size);
        return;
    }
    
    funcTable = (IPOD_PLAYER_CORE_FUNC_TABLE *)iPodCtrlCfg->funcTable;
    
    /* Check whether there is funcId in queue */
    queueCheck = iPodCoreCheckQueue(iPodCtrlCfg->waitList, (IPOD_PLAYER_FUNC_ID)funcId);
    if(queueCheck != IPOD_PLAYER_ERROR)
    {
        i = queueCheck;
        /* Check  whether there is coverart request in queue because coverare request prevent the execution of any other requests */
        queueCheck = iPodCoreCheckQueue(iPodCtrlCfg->waitList, IPOD_FUNC_GET_COVERART);
        if(queueCheck == IPOD_PLAYER_ERROR)
        {
            if(funcTable[funcId].func != NULL)
            {
                /* Execute internal function */
                funcTable[funcId].func(iPodCtrlCfg, &iPodCtrlCfg->waitList[i], header, contents, size);
            }
        }
    }
    
    return;
}


S32 iPodCoreiPodCtrliAP1MainLoop(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 endFlag = 0;
    U32 size = 0;
    U32 checkNum = 0;
    U32 i = 0;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents = NULL;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPODCORE_EPOLL_RETRY_WAIT};
    IPOD_PLAYER_IPC_HANDLE_INFO outputInfo[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(outputInfo, 0, sizeof(outputInfo));
    
    rc = iPodCoreiAP1Init(iPodCtrlCfg);
    if(rc != IPOD_PLAYER_OK)
    {
        endFlag = 1;
    }
    
    while(endFlag == 0)
    {
        rc = iPodCorePollWait(iPodCtrlCfg->waitHandle, iPodCtrlCfg->threadInfo->aTime, sizeof(outputInfo), outputInfo);
        if(rc > 0)
        {
            checkNum = rc;
            /* Loop until receivable event number */
            for(i = 0; (i < checkNum) && (endFlag == 0); i++)
            {
                /* Get the request */
                rc = iPodCoreiPodCtrlGetRequest(iPodCtrlCfg, outputInfo[i].type, outputInfo[i].handle, &contents, &size);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Execute the request */
                    rc = iPodCoreiPodCtrlExecuteRequest(iPodCtrlCfg, &iPodCtrlCfg->contents->paramTemp.header, contents, size);
                    if(rc == IPOD_PLAYER_ERR_DISCONNECTED)
                    {
                        /* Device was disconnected. Thread will be deleted */
                        endFlag = 1;
                    }
                }
                
                /* Remove the information for long receive */
                iPodCoreCheckAndRemoveLongRecvInfo(iPodCtrlCfg->longRecvInfo, rc, &outputInfo[i]);
            }
            
            if((rc != IPOD_PLAYER_OK) && (rc != IPOD_PLAYER_ERR_MORE_PACKET))
            {
                /* Clear the previous data */
                iPodCoreClearData(iPodCtrlCfg->contents);
            }
        }
        else if(rc == IPOD_PLAYER_ERR_TMOUT)
        {
            iPodCoreiPodCtrlCheckAndExecuteInternalFunc(iPodCtrlCfg, IPOD_FUNC_INTERNAL_GET_STATUS, &iPodCtrlCfg->contents->paramTemp.header, contents, &size);
        }
        else if(rc == 0)
        {
            /* Nothing do */
        }
        else
        {
            /* System error may be occurred. Wait 100ms and retry wait */
            nanosleep(&waitTime, NULL);
        }
    }
    
    iPodCoreiAP1Deinit(iPodCtrlCfg);
    
    return rc;
}


/**
 * void iPodCoreiPodCtrlHandler
 *
 * This function is thread of iPod Control handling
 * This function works that receive the message from each queue and send to iPod Control.
 **/
void iPodCoreiPodCtrlHandler(void *exinf)
{
    volatile S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_THREAD_INFO *iPodCfg = (IPOD_PLAYER_CORE_THREAD_INFO *)exinf;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, exinf);
    
    
    /* Check parameter */
    if(iPodCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return;
    }
    
    /* Initialize for iPodCtrlHandler thread */
    rc = iPodCoreiPodCtrlInit(iPodCfg, &iPodCtrlCfg);
    
    /* Thread Initialize */
    pthread_cleanup_push(iPodCoreiPodCtrlDeinit, (void *)iPodCtrlCfg);
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* If default protocol is set to 2, iAP2 mainloop will be started */
        if(iPodCtrlCfg->defaultProtocol == 2)
        {
            /* Main loop for iAP2 */
            rc = iPodCoreiAP2MainLoop(iPodCtrlCfg);
        }
        
        /* If default protocol is set to 1 or default protocol was set to 2 but error occurred with iAP1 device detected ,*/
        /* iAP1 mainloop will be started */
        if((iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_SUPPORT_IAP1) != 0) &&
           ((iPodCtrlCfg->defaultProtocol == 1) ||
           ((iPodCtrlCfg->defaultProtocol == 2) && (rc == IPOD_PLAYER_ERR_IAP1_DETECTED))))
        {
            /* Main loop for iAP1 */
            rc = iPodCoreiPodCtrliAP1MainLoop(iPodCtrlCfg);
        }
    }
    
    pthread_cleanup_pop(1);
    
    pthread_exit(0);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
}

/**
 * S32 iPodCoreCreateiPodCtrlHandler
 *
 * This function creates the thread for handling of iPod control and creates resources.
 *
 */
S32 iPodCoreCreateiPodCtrlHandler(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION *detectInfo, U32 devID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 devNum = 0;
    U32 iosAppNum = 0;
    pthread_attr_t attr = {{0}};
    U8 temp[IPODCORE_PREFIX_LEN] = {0};
    IPOD_PLAYER_IPC_OPEN_INFO info;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg, detectInfo, devID);
    
    /* Check parameter */
    if((iPodCfg == NULL) || (detectInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, detectInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    
    /* get cfg number */
    devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
    /* Loop until maximum number of iPod. It checks whehter thread is created or not */
    for(i = 0; i < devNum; i++)
    {
        /* This thread is not created yet */
        if(iPodCfg->threadInfo[i].isReady == 0)
        {
            rc = IPOD_PLAYER_OK;
            break;
        }
        /* Thread is created */
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Initialize the attribution of iPod Control thread */
        rc = pthread_attr_init(&attr);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Add the prefix of each thread */
        snprintf((char *)temp, sizeof(temp), "%s%d", IPODCORE_IPOD_HANDLE_IDENTIFY, devID);
        
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = temp;
        info.connectionNum = 10;
        info.semName = NULL;
        info.maxBufSize = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_QUEUE_SIZE);
        info.maxPacketSize = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_MSG_SIZE);
        /* Create the queue */
        rc = iPodPlayerIPCOpen(&iPodCfg->threadInfo[i].queueInfo, &info);
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            /* id sets to -1 */
            iPodCfg->threadInfo[i].queueInfo = -1;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
            info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
            info.identify = (U8 *)IPODCORE_COMMAND_QUEUE_IDENTIFY;
            info.connectionNum = IPODCORE_QUEUE_MAX_NUM;
            info.semName = NULL;
            rc = iPodPlayerIPCOpen(&iPodCfg->threadInfo[i].cmdQueueInfoClient, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
            info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
            info.identify = (U8 *)temp;
            info.connectionNum = 10;
            info.semName = NULL;
            rc = iPodPlayerIPCOpen(&iPodCfg->threadInfo[i].queueInfoClient, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Copy the connected device information to thread */
        strncpy((char *)iPodCfg->threadInfo[i].nameInfo.deviceName, (const char *)detectInfo->info.devPath, IPOD_PLAYER_STRING_LEN_MAX);
        iPodCfg->threadInfo[i].nameInfo.deviceName[IPOD_PLAYER_STRING_LEN_MAX - 1] = '\0';
        strncpy((char *)iPodCfg->threadInfo[i].nameInfo.audioInName, (const char *)detectInfo->info.audioPath, IPOD_PLAYER_STRING_LEN_MAX);
        iPodCfg->threadInfo[i].nameInfo.audioInName[IPOD_PLAYER_STRING_LEN_MAX - 1] = '\0';
        iPodCfg->threadInfo[i].nameInfo.devType = detectInfo->info.devType;

        /* Copy vehicle infomation */
        iPodCfg->threadInfo[i].vehicleInfo.displayName_valid = detectInfo->vehicleInfo.displayName_valid;
        strncpy((char *)iPodCfg->threadInfo[i].vehicleInfo.displayName, (const char *)detectInfo->vehicleInfo.displayName, IPOD_PLAYER_STRING_LEN_MAX);
        iPodCfg->threadInfo[i].vehicleInfo.displayName[IPOD_PLAYER_STRING_LEN_MAX - 1] = '\0';
        
        /* Copy the BT information to thread */
        iPodCfg->threadInfo[i].btInfo.macCount = detectInfo->devInfo.macCount;
        memcpy(iPodCfg->threadInfo[i].btInfo.macAddr, detectInfo->macAddr, sizeof(IPOD_PLAYER_BT_MAC_ADDR) * detectInfo->devInfo.macCount);

        /* Copy accessary information to thread */
        iPodCfg->threadInfo[i].accInfo = detectInfo->accInfo;
        /* Copy iOS information to thread */
        memcpy(iPodCfg->threadInfo[i].iosInfo, detectInfo->iosInfo, sizeof(IPOD_PLAYER_ACC_IOS_INFO) * detectInfo->accInfo.SupportediOSAppCount);
        /* Copy endPoint path */
        strncpy((char *)iPodCfg->threadInfo[i].endpointPath, (const char *)detectInfo->info.endPointPath, IPOD_PLAYER_STRING_LEN_MAX);
        iPodCfg->threadInfo[i].endpointPath[IPOD_PLAYER_STRING_LEN_MAX - 1] = '\0';
        
        /* get cfg number */
        iosAppNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_IOSAPP_COUNT);
        memcpy(iPodCfg->threadInfo[i].iOSAppInfo, iPodCfg->iOSAppInfo, sizeof(IPOD_PLAYER_IOSAPP_INFO) * iosAppNum);
        iPodCfg->threadInfo[i].iOSAppCount = iosAppNum;

        /* Set thread information */
        iPodCfg->threadInfo[i].isReady++;
        iPodCfg->threadInfo[i].appDevID = devID;
        iPodCfg->threadInfo[i].maxEntry = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_ENTRY_NUM);
        iPodCfg->threadInfo[i].timeout = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_PLAYER_TMOUT);
        iPodCfg->threadInfo[i].maxQueueSize = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_QUEUE_SIZE);
        iPodCfg->threadInfo[i].maxMsgSize = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_MSG_SIZE);
        
        memset(&iPodCfg->threadInfo[i].playbackStatus, 0, sizeof(iPodCfg->threadInfo[i].playbackStatus));
        iPodCfg->threadInfo[i].prevPlayTime = 0;
        
        /* Create the thread */
        rc = pthread_create(&iPodCfg->threadInfo[i].id, &attr, 
                            (void *(*)(void *))iPodCoreiPodCtrlHandler, (void *)&iPodCfg->threadInfo[i]);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            /* Clear connected device information */
            memset(iPodCfg->threadInfo[i].nameInfo.deviceName, 0, IPOD_PLAYER_STRING_LEN_MAX);
            memset(iPodCfg->threadInfo[i].nameInfo.audioInName, 0, IPOD_PLAYER_STRING_LEN_MAX);
            memset(iPodCfg->threadInfo[i].nameInfo.audioOutName, 0, IPOD_PLAYER_STRING_LEN_MAX);
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Delete iPodCtrl Handler thread */
S32 iPodCoreDeleteiPodCtrlHandler(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION *detectInfo, U32 devID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 j = 0;
    U32 devNum = 0;
    IPOD_PLAYER_PARAM_DEVDEINIT deinit;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPODCORE_EPOLL_RETRY_WAIT};
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg, detectInfo, devID);
    
    /* Check the parameter */
    if((iPodCfg == NULL) || (detectInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, detectInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCfg->threadInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg->threadInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the signal structure */
    memset(&deinit, 0, sizeof(deinit));
    
    devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
    /* Loop until maximum number of iPod */
    for(i = 0; i < devNum; i++)
    {
        /* iPodCtrl Handler thread has been created */
        if(iPodCfg->threadInfo[i].isReady != 0)
        {
            /* Compare the device name refered to thread and connected device name */
            if(strncmp((const char *)iPodCfg->threadInfo[i].nameInfo.deviceName, (const char *)detectInfo->info.devPath, IPOD_PLAYER_STRING_LEN_MAX) == 0)
            {
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCfg->threadInfo[i].id);
                
                deinit.header.funcId = IPOD_FUNC_DEVDEINIT;
                deinit.header.devID = devID;
                
                /* Delete is sent to iPodControl Handler */
                for(j = 0; (j < IPODCORE_DELETE_REQ_RETRY) && (rc != IPOD_PLAYER_OK); j++)
                {
                    rc = iPodPlayerIPCSend(iPodCfg->threadInfo[i].queueInfoClient, (U8 *)&deinit, sizeof(deinit), 0, IPODCORE_SEND_TMOUT);
                    if(rc == sizeof(deinit))
                    {
                        /* Delete request could send to iPodControl Handler */
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        /* Delete request could not send to iPodControl Handler. This case should be retry */
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                        /* Wait 100ms */
                        nanosleep(&waitTime, NULL);
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                
                break;
            }
            else
            {
                /* Name is difference */
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            /* thread is not created yet */
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCfg->threadInfo[i].id);
        
        /* Wait thread ends */
        pthread_join(iPodCfg->threadInfo[i].id, NULL);
        /* Thread information is removed */
        iPodCfg->threadInfo[i].isReady = 0;
        /* Set removed device ID */
        iPodCfg->threadInfo[i].appDevID = 0;
        iPodCfg->threadInfo[i].maxEntry = 0;
        
        /* Queue is created */
        if(iPodCfg->threadInfo[i].queueInfoClient >= 0)
        {
            /* Remove the queue information */
            iPodPlayerIPCClose(iPodCfg->threadInfo[i].queueInfoClient);
            iPodCfg->threadInfo[i].queueInfoClient = -1;
        }
        
        /* Queue is created */
        if(iPodCfg->threadInfo[i].queueInfo >= 0)
        {
            iPodPlayerIPCClose(iPodCfg->threadInfo[i].queueInfo);
            iPodCfg->threadInfo[i].queueInfo = -1;
        }
        
        /* Queue is created */
        if(iPodCfg->threadInfo[i].cmdQueueInfoClient >= 0)
        {
            iPodPlayerIPCClose(iPodCfg->threadInfo[i].cmdQueueInfoClient);
            iPodCfg->threadInfo[i].cmdQueueInfoClient = -1;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* initialize thread information stack */
S32 iPodCoreiPodCtrlInitThreadInfoStack()
{
    S32 rc = IPOD_PLAYER_ERROR;                            /* for return code   */
    U32 dev_num = 0;                                       /* for device number */
    IPOD_PLAYER_CORE_THREAD_INFO **thread_info = NULL;     /* for thread info   */
    
    
    /* get cfg number */
    dev_num = iPodCoreGetIndexCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM, 0);
    if(dev_num > 0)
    {
        /* getting for thread info memory */
        thread_info = (IPOD_PLAYER_CORE_THREAD_INFO **)malloc(dev_num * sizeof(IPOD_PLAYER_CORE_THREAD_INFO*));
        if(thread_info != NULL)
        {
            /* initialize parameter*/
            memset(thread_info, 0, (dev_num * sizeof(IPOD_PLAYER_CORE_THREAD_INFO*)));
            /* set global parameter */
            g_threadInfo = thread_info;
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    return rc;
}

/* deinitialize thread information stack */
void iPodCoreiPodCtrlDeInitThreadInfoStack()
{
    /* free memory */
    if(g_threadInfo != NULL)
    {
        free(g_threadInfo);
        g_threadInfo = NULL;
    }
}
