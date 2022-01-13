
#include "iPodPlayerLocal.h"
#include "iPodPlayerIPCLib.h"

#include "iPodPlayerData.h"

#include "iPodPlayerAPI.h"
#include "iPodPlayerCB.h"

S32 iPodPlayerPlayCommon(U32 devID, BOOL playCurSel)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_PLAY param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_PLAY;
        param.header.appID = pid;

        param.header.devID = devID;
        param.playCurSel = playCurSel;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerCallback(IPOD_PLAYER_MESSAGE_DATA_CONTENTS *data)
 * \par INPUT PARAMETERS
 * IPOD_PLAYER_MESSAGE_DATA_INFO size - recv size.<br>
 * U8* data - data pointer.<br>
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_OK: Callback success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : Callback failed.</b>
 * \par DESCRIPTION
 * This Function callback to Application.
 */

S32 iPodPlayerCallback(IPOD_PLAYER_FUNC_HEADER *header, U8 dataNum, U8 **data, U32 *size, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_FUNC_RESULT_ID resultID = IPOD_FUNC_INIT_RESULT;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *rdata = NULL;
    S32 sendQueue = -1;
    
    /* NULL check */
    if((header == NULL) || (cbTable == NULL))
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* For lint */
    size = size;
    dataNum = dataNum;
    
    resultID = (IPOD_PLAYER_FUNC_RESULT_ID)header->funcId;
    rdata = (IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)(void *)data[0];

    /* data analyze */
    switch(resultID)
    {
    case IPOD_FUNC_DEINIT_RESULT:
        rc = IPOD_PLAYER_ERROR;
        break;
        
    case IPOD_FUNC_SELECT_AUDIO_OUT_RESULT:
        rc = iPodPlayerGetSocketInfo(&sendQueue, IPOD_PLAYER_OPEN_QUEUE_CLIENT);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodPlayerIPCSend(sendQueue, (U8*)rdata, sizeof(rdata->selectAudioOutResult), 0, -1);
            if(rc == sizeof(rdata->selectAudioOutResult))
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        break;
        
    case IPOD_FUNC_TEST_READY_RESULT:
        rc = iPodPlayerGetSocketInfo(&sendQueue, IPOD_PLAYER_OPEN_QUEUE_CLIENT);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodPlayerIPCSend(sendQueue, (U8*)rdata, sizeof(rdata->testReadyResult), 0, -1);
            if(rc == sizeof(rdata->selectAudioOutResult))
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        break;
    
    case IPOD_FUNC_START_AUTHENTICATION_RESULT:
        rc = iPodPlayerGetSocketInfo(&sendQueue, IPOD_PLAYER_OPEN_QUEUE_CLIENT);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodPlayerIPCSend(sendQueue, (U8*)rdata, sizeof(rdata->startAuthenticationResult), 0, -1);
            if(rc == sizeof(rdata->selectAudioOutResult))
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        break;
    case IPOD_FUNC_NOTIFY_PLAYBACK_STATUS:
        if(cbTable->cbNotifyPlaybackStatus != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS)(cbTable->cbNotifyPlaybackStatus))(rdata->notifyPlaybackStatus.header.devID, &rdata->notifyPlaybackStatus.status);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_CONNECTION_STATUS:
        if(cbTable->cbNotifyConnectionStatus != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS)(cbTable->cbNotifyConnectionStatus))(rdata->notifyConnectionStatus.header.devID, &rdata->notifyConnectionStatus.status);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_TRACK_INFO:
        if(cbTable->cbNotifyTrackInfo != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_TRACK_INFO)(cbTable->cbNotifyTrackInfo))(rdata->notifyTrackInfo.header.devID, rdata->notifyTrackInfo.trackIndex, &rdata->notifyTrackInfo.info);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_PLAYBACK_CHANGE:
        if(cbTable->cbNotifyPlaybackChange != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_PLAYBACK_CHANGE)(cbTable->cbNotifyPlaybackChange))(rdata->notifyPlaybackChange.header.devID);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_DB_ENTRIES:
        if(cbTable->cbNotifyDBEntries != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES)(cbTable->cbNotifyDBEntries))(rdata->notifyDBEntries.header.devID, rdata->notifyDBEntries.type, rdata->notifyDBEntries.count, rdata->notifyDBEntries.list);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_COVERART_DATA:
        if(cbTable->cbNotifyCoverartData != NULL)
        {
            U32 trackIndex = 0;
            U32 iPodTime = 0;
            U32 bufSize = 0;
            U8 *buf = NULL;
            IPOD_PLAYER_COVERART_HEADER coverartHeader;
            IPOD_PLAYER_PARAM_INTERNAL_COVERART *intCover = NULL;
            
            if(data[0] != NULL)
            {
                intCover = (IPOD_PLAYER_PARAM_INTERNAL_COVERART *)(void *)data[0];
                trackIndex = intCover->trackIndex;
                iPodTime = intCover->time;
                bufSize = intCover->bufSize;
                
                //memcpy(&trackIndex, &data[0], sizeof(trackIndex));
                //memcpy(&iPodTime, &data[0][4], sizeof(iPodTime));
                //memcpy(&bufSize, &data[0][sizeof(IPOD_PLAYER_COVERART_HEADER) + 8], sizeof(bufSize));
                memcpy(&coverartHeader, &intCover->coverartHeader, sizeof(coverartHeader));
                buf = bufSize > 0 ? &data[0][sizeof(*intCover)] : NULL;
                (*(IPOD_PLAYER_CB_NOTIFY_COVERART_DATA)(cbTable->cbNotifyCoverartData))(header->devID, trackIndex, iPodTime, 
                  &coverartHeader, bufSize, buf);
                  
                (*(IPOD_PLAYER_CB_GET_COVERART_RESULT)(cbTable->cbGetCoverartResult))(header->devID, IPOD_PLAYER_IPC_OK);
            }
        }
        break;
        
    case IPOD_FUNC_NOTIFY_OPEN_APP:
        if(cbTable->cbNotifyOpenApp != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_OPEN_APP)(cbTable->cbNotifyOpenApp))(rdata->notifyOpenApp.header.devID, rdata->notifyOpenApp.appID);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_CLOSE_APP:
        if(cbTable->cbNotifyCloseApp != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_CLOSE_APP)(cbTable->cbNotifyCloseApp))(rdata->notifyCloseApp.header.devID, rdata->notifyCloseApp.appID);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_RECEIVE_FROM_APP:
        if(cbTable->cbNotifyReceiveFromApp != NULL)
        {
            U32 appID = 0;
            U32 dataSize = 0;
            U8 *receiveData = NULL;
            
            memcpy(&appID, &data[0][0], sizeof(appID));
            memcpy(&dataSize, &data[0][sizeof(appID)], sizeof(dataSize));
            receiveData = &data[0][sizeof(IPOD_PLAYER_PARAM_NOTIFY_RECEIVE_FROM_APP) - sizeof(*header)];
            (*(IPOD_PLAYER_CB_NOTIFY_RECEIVE_FROM_APP)(cbTable->cbNotifyReceiveFromApp))(header->devID, appID, dataSize, receiveData);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_GPS_STATUS:
        if(cbTable->cbNotifyGPSStatus != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_GPS_STATUS)(cbTable->cbNotifyGPSStatus))(rdata->notifyGPSStatus.header.devID, rdata->notifyGPSStatus.bitmask);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_GPS_DATA:
        if(cbTable->cbNotifyGPSData != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_GPS_DATA)(cbTable->cbNotifyGPSData))(rdata->notifyGPSData.header.devID, rdata->notifyGPSData.type, 
            rdata->notifyGPSData.dataSize, rdata->notifyGPSData.data);
        }
        break;
    case IPOD_FUNC_NOTIFY_GPS_CURRENT_POSITION:
        if(cbTable->cbNotifyGPSCurrentPosition != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_POSITION)(cbTable->cbNotifyGPSCurrentPosition))(rdata->notifyGPSCurrentPosition.header.devID, 
            &rdata->notifyGPSCurrentPosition.time, &rdata->notifyGPSCurrentPosition.angle);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_GPS_CURRENT_TIME:
        if(cbTable->cbNotifyGPSCurrentTime != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_GPS_CURRENT_TIME)(cbTable->cbNotifyGPSCurrentTime))(rdata->notifyGPSCurrentTime.header.devID, &rdata->notifyGPSCurrentTime.time);
        }
        break;
        
    case IPOD_FUNC_NOTIFY_HMI_EVENT:
        if(cbTable->cbNotifyHMIEvent != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_HMI_EVENT)(cbTable->cbNotifyHMIEvent))(rdata->notifyHMIEvent.header.devID, rdata->notifyHMIEvent.type, rdata->notifyHMIEvent.status, &rdata->notifyHMIEvent.date);
        }
        break;
    
    case IPOD_FUNC_NOTIFY_DEVICE_EVENT:
        if(cbTable->cbNotifyDeviceEvent != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT)(cbTable->cbNotifyDeviceEvent))(rdata->notifyDeviceEvent.header.devID, rdata->notifyDeviceEvent.type, &rdata->notifyDeviceEvent.event);
        }
        break;
    
    case IPOD_FUNC_NOTIFY_LOCATION_INFO_STATUS:
        if(cbTable->cbNotifyLocationInfoStatus != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_LOCATION_INFO_STATUS)(cbTable->cbNotifyLocationInfoStatus))(rdata->notifyLocationInfoStatus.header.devID, rdata->notifyLocationInfoStatus.status, rdata->notifyLocationInfoStatus.locationMask);
        }
        break;

    case IPOD_FUNC_NOTIFY_VEHICLE_STATUS:
        if(cbTable->cbNotifyVehicleStatus != NULL)
        {
            (*(IPOD_PLAYER_CB_NOTIFY_VEHICLE_STATUS)(cbTable->cbNotifyVehicleStatus))(rdata->notifyVehicleStatus.header.devID, rdata->notifyVehicleStatus.status);
        }
        break;
    
    /* -------------- Playback Function Result -------------*/
    case IPOD_FUNC_PLAY_RESULT:
        if(rdata->playResult.playCurSel == FALSE)
        {
            /* iPodPlayerPlay Callback */
            if(cbTable->cbPlayResult != NULL)
            {
                (*(IPOD_PLAYER_CB_PLAY_RESULT)(cbTable->cbPlayResult))(rdata->playResult.header.devID, rdata->playResult.result);
            }
        }
        else
        {
            /* iPodPlayerCurrentSelection Callback */
            if(cbTable->cbPlayCurrentSelectionResult != NULL)
            {
                (*(IPOD_PLAYER_CB_PLAY_CURRENT_SELECTION_RESULT)(cbTable->cbPlayCurrentSelectionResult))(rdata->playResult.header.devID, rdata->playResult.result);
            }
        }
        break;
        
    case IPOD_FUNC_PAUSE_RESULT:
        if(cbTable->cbPauseResult != NULL)
        {
            (*(IPOD_PLAYER_CB_PAUSE_RESULT)(cbTable->cbPauseResult))(rdata->pauseResult.header.devID, rdata->pauseResult.result);
        }
        break;
        
    case IPOD_FUNC_STOP_RESULT:
        if(cbTable->cbStopResult != NULL)
        {
            (*(IPOD_PLAYER_CB_STOP_RESULT)(cbTable->cbStopResult))(rdata->stopResult.header.devID, rdata->stopResult.result);
        }
        break;
        
    case IPOD_FUNC_NEXT_TRACK_RESULT:
        if(cbTable->cbNextTrackResult != NULL)
        {
            (*(IPOD_PLAYER_CB_NEXTTRACK_RESULT)(cbTable->cbNextTrackResult))(rdata->nextTrackResult.header.devID, rdata->nextTrackResult.result);
        }
        break;
        
    case IPOD_FUNC_PREV_TRACK_RESULT:
        if(cbTable->cbPrevTrackResult != NULL)
        {
            (*(IPOD_PLAYER_CB_PREVTRACK_RESULT)(cbTable->cbPrevTrackResult))(rdata->prevTrackResult.header.devID, rdata->prevTrackResult.result);
        }
        break;
        
    case IPOD_FUNC_NEXTCHAPTER_RESULT:
        if(cbTable->cbNextChapterResult != NULL)
        {
            (*(IPOD_PLAYER_CB_NEXT_CHAPTER_RESULT)(cbTable->cbNextChapterResult))(rdata->nextChapterResult.header.devID, rdata->nextChapterResult.result);
        }
        break;
        
    case IPOD_FUNC_PREVCHAPTER_RESULT:
        if(cbTable->cbPrevChapterResult != NULL)
        {
            (*(IPOD_PLAYER_CB_PREV_CHAPTER_RESULT)(cbTable->cbPrevChapterResult))(rdata->prevChapterResult.header.devID, rdata->prevChapterResult.result);
        }
        break;
        
    case IPOD_FUNC_FASTFORWARD_RESULT:
        if(cbTable->cbFastforwardResult != NULL)
        {
            (*(IPOD_PLAYER_CB_FASTFORWARD_RESULT)(cbTable->cbFastforwardResult))(rdata->fastforwardResult.header.devID, rdata->fastforwardResult.result);
        }
        break;
    case IPOD_FUNC_REWIND_RESULT:
        if(cbTable->cbRewindResult != NULL)
        {
            (*(IPOD_PLAYER_CB_REWIND_RESULT)(cbTable->cbRewindResult))(rdata->rewindResult.header.devID, rdata->rewindResult.result);
        }
        break;
        
    case IPOD_FUNC_GOTO_TRACK_POSITION_RESULT:
        if(cbTable->cbGotoTrackPositionResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GOTO_TRACKPOSITION_RESULT)(cbTable->cbGotoTrackPositionResult))(rdata->gotoTrackPositionResult.header.devID, rdata->gotoTrackPositionResult.result);
        }
        break;
        
    case IPOD_FUNC_PLAYTRACK_RESULT:
        if(cbTable->cbPlayTrackResult != NULL)
        {
            (*(IPOD_PLAYER_CB_PLAY_TRACK_RESULT)(cbTable->cbPlayTrackResult))(rdata->playTrackResult.header.devID, rdata->playTrackResult.result);
        }
        break;
        
    case IPOD_FUNC_RELEASE_RESULT:
        if(cbTable->cbReleaseResult != NULL)
        {
            (*(IPOD_PLAYER_CB_RELEASE_RESULT)(cbTable->cbReleaseResult))(rdata->releaseResult.header.devID, rdata->releaseResult.result);
        }
        break;
        
        
    /* -------------- Playback Result fucntion end ------------------- */
    
    case IPOD_FUNC_SET_AUDIO_MODE_RESULT:
        if(cbTable->cbSetAudioModeResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT)(cbTable->cbSetAudioModeResult))(rdata->setAudioModeResult.header.devID, rdata->setAudioModeResult.result);
        }
        break;
        
    case IPOD_FUNC_SET_MODE_RESULT:
        if(cbTable->cbSetModeResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_MODE_RESULT)(cbTable->cbSetModeResult))(rdata->setModeResult.header.devID, rdata->setModeResult.result);
        }
        break;
        

    case IPOD_FUNC_SET_REPEAT_RESULT:
        if(cbTable->cbSetRepeatStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_REPEAT_STATUS_RESULT)(cbTable->cbSetRepeatStatusResult))(rdata->setRepeatResult.header.devID, rdata->setRepeatResult.result);
        }
        break;
    case IPOD_FUNC_SET_SHUFFLE_RESULT:
        if(cbTable->cbSetShuffleStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_SHUFFLE_STATUS_RESULT)(cbTable->cbSetShuffleStatusResult))(rdata->setShuffleResult.header.devID, rdata->setShuffleResult.result);
        }
        break;
    
    case IPOD_FUNC_CHANGE_REPEAT_RESULT:                            /* support iAP2 */
        if(cbTable->cbChangeRepeatStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_CHANGE_REPEAT_STATUS_RESULT)(cbTable->cbChangeRepeatStatusResult))(rdata->changeRepeatResult.header.devID, rdata->changeRepeatResult.result);
        }
        break;
    case IPOD_FUNC_CHANGE_SHUFFLE_RESULT:                           /* support iAP2 */
        if(cbTable->cbChangeShuffleStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_CHANGE_SHUFFLE_STATUS_RESULT)(cbTable->cbChangeShuffleStatusResult))(rdata->changeShuffleResult.header.devID, rdata->changeShuffleResult.result);
        }
        break;
    
    case IPOD_FUNC_SET_EQUALIZER_RESULT:
        if(cbTable->cbSetEqualizerResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_EQUALIZER_RESULT)(cbTable->cbSetEqualizerResult))(rdata->setEqualizerResult.header.devID, rdata->setEqualizerResult.result);
        }
        break;
         
    case IPOD_FUNC_SET_VIDEO_DELAY_RESULT:
        if(cbTable->cbSetVideoDelayResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_VIDEO_DELAY_RESULT)(cbTable->cbSetVideoDelayResult))(rdata->setVideoDelay.header.devID, rdata->setVideoDelayResult.result);
        }
        break;

    case IPOD_FUNC_SET_VIDEO_SETTING_RESULT:
        if(cbTable->cbSetVideoSettingResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_VIDEO_SETTING_RESULT)(cbTable->cbSetVideoSettingResult))(rdata->setVideoSettingResult.header.devID, rdata->setVideoSettingResult.result);
        }
        break;

    case IPOD_FUNC_SET_DISPLAY_IMAGE_RESULT:
        if(cbTable->cbSetDisplayImageResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_DISPLAY_IMAGE_RESULT)(cbTable->cbSetDisplayImageResult))(rdata->setDisplayImageResult.header.devID, rdata->setDisplayImageResult.result);
        }
        break;

    case IPOD_FUNC_SET_PLAY_SPEED_RESULT:
        if(cbTable->cbSetPlaySpeedResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_PLAY_SPEED_RESULT)(cbTable->cbSetPlaySpeedResult))(rdata->setPlaySpeedResult.header.devID, rdata->setPlaySpeedResult.result);
        }
        break;

    case IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION_RESULT:
        if(cbTable->cbSetTrackInfoNotificationResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_TRACK_INFO_NOTIFICATION_RESULT)(cbTable->cbSetTrackInfoNotificationResult))(rdata->setTrackInfoNotificationResult.header.devID, rdata->setTrackInfoNotificationResult.result);
        }
        break;

    case IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION_RESULT:
        if(cbTable->cbSetDeviceEventNotificationResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT)(cbTable->cbSetDeviceEventNotificationResult))(rdata->setDeviceEventNotificationResult.header.devID, rdata->setDeviceEventNotificationResult.result);
        }
        break;

    case IPOD_FUNC_GET_VIDEO_SETTING_RESULT:
        if(cbTable->cbGetVideoSettingResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_VIDEO_SETTING_RESULT)(cbTable->cbGetVideoSettingResult))(rdata->getVideoSettingResult.header.devID, rdata->getVideoSettingResult.result, &rdata->getVideoSettingResult.setting);
        }
        break;

    case IPOD_FUNC_GET_COVERART_INFO_RESULT:
        if(cbTable->cbGetCoverartInfoResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_COVERART_INFO_RESULT)(cbTable->cbGetCoverartInfoResult))(rdata->getCoverartInfoResult.header.devID, rdata->getCoverartInfoResult.result, rdata->getCoverartInfoResult.timeCount, rdata->getCoverartInfoResult.time);
        }
        break;

    case IPOD_FUNC_GET_COVERART_RESULT:
        if(cbTable->cbGetCoverartResult != NULL)
        {
            if(rdata->getCoverartResult.result != IPOD_PLAYER_OK)
            {
                (*(IPOD_PLAYER_CB_GET_COVERART_RESULT)(cbTable->cbGetCoverartResult))(rdata->getCoverartResult.header.devID, rdata->getCoverartResult.result);
            }
        }
        break;

    case IPOD_FUNC_GET_PLAYBACK_STATUS_RESULT:
        if(cbTable->cbGetPlaybackStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_PLAYBACK_STATUS_RESULT)(cbTable->cbGetPlaybackStatusResult))(rdata->getPlaybackStatusResult.header.devID, rdata->getPlaybackStatusResult.result, &rdata->getPlaybackStatusResult.status);
        }
        break;

    case IPOD_FUNC_GET_TRACK_INFO_RESULT:
        if(cbTable->cbGetTrackInfoResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT)(cbTable->cbGetTrackInfoResult))(rdata->getTrackInfoResult.header.devID, rdata->getTrackInfoResult.result, rdata->getTrackInfoResult.type, rdata->getTrackInfoResult.trackID, &rdata->getTrackInfoResult.info);
        }
        break;

    case IPOD_FUNC_GET_CHAPTER_INFO_RESULT:
        if(cbTable->cbGetChapterInfoResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT)(cbTable->cbGetChapterInfoResult))(rdata->getChapterInfoResult.header.devID, rdata->getChapterInfoResult.result, rdata->getChapterInfoResult.trackIndex, rdata->getChapterInfoResult.chapterIndex, &rdata->getChapterInfoResult.info);
        }
        break;

    case IPOD_FUNC_GET_MODE_RESULT:
        if(cbTable->cbGetModeResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_MODE_RESULT)(cbTable->cbGetModeResult))(rdata->getModeResult.header.devID, rdata->getModeResult.result, rdata->getModeResult.mode);
        }
        break;

    case IPOD_FUNC_GET_REPEAT_RESULT:
        if(cbTable->cbGetRepeatStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_REPEAT_STATUS_RESULT)(cbTable->cbGetRepeatStatusResult))(rdata->getRepeatResult.header.devID, rdata->getRepeatResult.result, rdata->getRepeatResult.status);
        }
        break;

    case IPOD_FUNC_GET_SHUFFLE_RESULT:
        if(cbTable->cbGetShuffleStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_SHUFFLE_STATUS_RESULT)(cbTable->cbGetShuffleStatusResult))(rdata->getShuffleResult.header.devID, rdata->getShuffleResult.result, rdata->getShuffleResult.status);
        }
        break;
    
    case IPOD_FUNC_GET_PLAY_SPEED_RESULT:
        if(cbTable->cbGetPlaySpeedResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_PLAY_SPEED_RESULT)(cbTable->cbGetPlaySpeedResult))(rdata->getPlaySpeedResult.header.devID, rdata->getEqualizerResult.result, rdata->getPlaySpeedResult.speed);
        }
        break;

    case IPOD_FUNC_GET_TRACK_TOTAL_COUNT_RESULT:
        if(cbTable->cbGetTrackTotalCountResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_TRACK_TOTAL_COUNT_RESULT)(cbTable->cbGetTrackTotalCountResult))(rdata->getTrackTotalCountResult.header.devID, rdata->getTrackTotalCountResult.result, rdata->getTrackTotalCountResult.type, rdata->getTrackTotalCountResult.count);
        }
        break;

    case IPOD_FUNC_GET_MEDIA_ITEM_INFO_RESULT:
        if(cbTable->cbGetMediaItemInfoResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT)(cbTable->cbGetMediaItemInfoResult))(rdata->getMediaItemMediaItemInfoResult.header.devID, rdata->getMediaItemMediaItemInfoResult.result, rdata->getMediaItemMediaItemInfoResult.trackID, rdata->getMediaItemMediaItemInfoResult.mediaType);
        }
        break;

    case IPOD_FUNC_GET_EQUALIZER_RESULT:
        if(cbTable->cbGetEqualizerResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_EQUALIZER_RESULT)(cbTable->cbGetEqualizerResult))(rdata->getEqualizerResult.header.devID, rdata->getEqualizerResult.result, rdata->getEqualizerResult.value);
        }
        break;

    case IPOD_FUNC_GET_EQUALIZER_NAME_RESULT:
        if(cbTable->cbGetEqualizerNameResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_EQUALIZER_NAME_RESULT)(cbTable->cbGetEqualizerNameResult))(rdata->getEqualizerNameResult.header.devID, rdata->getEqualizerNameResult.result, rdata->getEqualizerNameResult.name);
        }
        break;

    case IPOD_FUNC_GET_DEVICE_PROPERTY_RESULT:
        if(cbTable->cbGetDevicePropertyResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_DEVICE_PROPERTY_RESULT)(cbTable->cbGetDevicePropertyResult))(rdata->getDevicePropertyResult.header.devID, rdata->getDevicePropertyResult.result, &rdata->getDevicePropertyResult.property);
        }
        break;

    case IPOD_FUNC_GET_DEVICE_STATUS_RESULT:
        if(cbTable->cbGetDeviceStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_DEVICE_STATUS_RESULT)(cbTable->cbGetDeviceStatusResult))(rdata->getDeviceStatusResult.header.devID, rdata->getDeviceStatusResult.result, &rdata->getDeviceStatusResult.info);
        }
        break;

    case IPOD_FUNC_GET_DB_ENTRIES_RESULT:
        if(cbTable->cbGetDBEntriesResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_DB_ENTRIES)(cbTable->cbGetDBEntriesResult))(rdata->getDBEntriesResult.header.devID, rdata->getDBEntriesResult.result);
        }
        break;

    case IPOD_FUNC_GET_DB_COUNT_RESULT:
        if(cbTable->cbGetDBCountResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_DB_COUNT_RESULT)(cbTable->cbGetDBCountResult))(rdata->getDBCountResult.header.devID, rdata->getDBCountResult.result, rdata->getDBCountResult.num);
        }
        break;

    case IPOD_FUNC_SELECT_DB_ENTRY_RESULT:
        if(cbTable->cbSelectDBEntryResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT)(cbTable->cbSelectDBEntryResult))(rdata->selectDBEntryResult.header.devID, rdata->selectDBEntryResult.result);
        }
        break;

    case IPOD_FUNC_CANCEL_RESULT:
        if(cbTable->cbCancelResult != NULL)
        {
            (*(IPOD_PLAYER_CB_CANCEL_RESULT)(cbTable->cbCancelResult))(rdata->cancelResult.header.devID, rdata->cancelResult.result);
        }
        break;

    case IPOD_FUNC_CLEAR_SELECTION_RESULT:
        if(cbTable->cbClearSelectionResult != NULL)
        {
            (*(IPOD_PLAYER_CB_CLEAR_SELECTION)(cbTable->cbClearSelectionResult))(rdata->clearSelectionResult.header.devID, rdata->clearSelectionResult.result);
        }
        break;

    case IPOD_FUNC_SELECT_AV_RESULT:
        if(cbTable->cbSelectAVResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SELECT_AV_RESULT)(cbTable->cbSelectAVResult))(rdata->selectAVResult.header.devID, rdata->selectAVResult.result);
        }
        break;
        
    case IPOD_FUNC_SEND_TO_APP_RESULT:
        if(cbTable->cbSendToAppResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SEND_TO_APP_RESULT)(cbTable->cbSendToAppResult))(rdata->sendToAppResult.header.devID, rdata->sendToAppResult.result, rdata->sendToAppResult.handle);
        }
        break;

    case IPOD_FUNC_SET_LOCATION_INFO_RESULT:
        if(cbTable->cbLocationInfoResult != NULL)
        {
            (*(IPOD_PLAYER_CB_LOCATION_INFO_RESULT)(cbTable->cbLocationInfoResult))(rdata->locationInfoResult.header.devID, rdata->locationInfoResult.result);
        }
        break;
        
    case IPOD_FUNC_SET_VEHICLE_STATUS_RESULT:
        if(cbTable->cbVehicleStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_VEHICLE_STATUS_RESULT)(cbTable->cbVehicleStatusResult))(rdata->locationInfoResult.header.devID, rdata->locationInfoResult.result);
        }
        break;
        
    case IPOD_FUNC_OPEN_SONG_TAG_FILE_RESULT:
        if(cbTable->cbOpenSongTagResult != NULL)
        {
            (*(IPOD_PLAYER_CB_OPEN_SONG_TAG_RESULT)(cbTable->cbOpenSongTagResult))(rdata->openSongTagFileResult.header.devID, rdata->openSongTagFileResult.result, rdata->openSongTagFileResult.handle);
        }
        break;
    
    case IPOD_FUNC_CLOSE_SONG_TAG_FILE_RESULT:
        if(cbTable->cbCloseSongTagResult != NULL)
        {
            (*(IPOD_PLAYER_CB_CLOSE_SONG_TAG_RESULT)(cbTable->cbCloseSongTagResult))(rdata->closeSongTagFileResult.header.devID, rdata->closeSongTagFileResult.result);
        }
        break;

    case IPOD_FUNC_SONG_TAG_RESULT:
        if(cbTable->cbSongTagResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SONG_TAG_RESULT)(cbTable->cbSongTagResult))(rdata->songTagResult.header.devID, rdata->songTagResult.result);
        }
        break;
        
    case IPOD_FUNC_REQUEST_APP_START_RESULT:
        if(cbTable->cbRequestAppStartResult != NULL)
        {
            (*(IPOD_PLAYER_CB_REQUEST_APP_START)(cbTable->cbRequestAppStartResult))(rdata->requestAppStartResult.header.devID, rdata->requestAppStartResult.result);
        }
        break;
        
    case IPOD_FUNC_SET_POWER_SUPPLY_RESULT:
        if(cbTable->cbSetPowerSupplyResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_POWER_SUPPLY_RESULT)(cbTable->cbSetPowerSupplyResult))(rdata->requestAppStartResult.header.devID, rdata->requestAppStartResult.result);
        }
        break;
        
    case IPOD_FUNC_SET_VOLUME_RESULT:
        if(cbTable->cbSetVolumeResult != NULL)
        {
            (*(IPOD_PLAYER_CB_SET_VOLUME_RESULT)(cbTable->cbSetVolumeResult))(rdata->setVolumeResult.header.devID, rdata->setVolumeResult.result);
        }
        break;
        
    case IPOD_FUNC_GET_VOLUME_RESULT:
        if(cbTable->cbGetVolumeResult != NULL)
        {
            (*(IPOD_PLAYER_CB_GET_VOLUME_RESULT)(cbTable->cbGetVolumeResult))(rdata->getVolumeResult.header.devID, rdata->getVolumeResult.result, &rdata->getVolumeResult.volume);
        }
        break;
        
    case IPOD_FUNC_SET_DEVICE_DETECTION_RESULT:
        if(cbTable->cbDeviceDetectionResult != NULL)
        {
            (*(IPOD_PLAYER_CB_DEVICE_DETECTION_RESULT)(cbTable->cbDeviceDetectionResult))(rdata->setDeviceDetectionResult.header.devID, rdata->setDeviceDetectionResult.result);
        }
        break;

        case IPOD_FUNC_HMI_ROTATION_INPUT_RESULT:
        if(cbTable->cbHMIRotationInputResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_ROTATION_INPUT_RESULT)(cbTable->cbHMIRotationInputResult))(rdata->hmiRotationInputResult.header.devID, rdata->hmiRotationInputResult.result);
        }
        break;
        
    case IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE_RESULT:
        if(cbTable->cbHMISetSupportedFeatureResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_SET_SUPPORTED_FEATURE_RESULT)(cbTable->cbHMISetSupportedFeatureResult))(rdata->hmiSetSupportedFeatureResult.header.devID, rdata->hmiSetSupportedFeatureResult.result);
        }
        break;
        
    case IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE_RESULT:
        if(cbTable->cbHMIGetSupportedFeatureResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT)(cbTable->cbHMIGetSupportedFeatureResult))(rdata->hmiGetSupportedFeatureResult.header.devID, rdata->hmiGetSupportedFeatureResult.result,rdata->hmiGetSupportedFeatureResult.type, rdata->hmiGetSupportedFeatureResult.optionsBits);
        }
        break;
        
    case IPOD_FUNC_HMI_BUTTON_INPUT_RESULT:
        if(cbTable->cbHMIButtonInputResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_BUTTON_INPUT_RESULT)(cbTable->cbHMIButtonInputResult))(rdata->hmiButtonInputResult.header.devID, rdata->hmiButtonInputResult.result);
        }
        break;
        
    case IPOD_FUNC_HMI_PLAYBACK_INPUT_RESULT:
        if(cbTable->cbHMIPlaybackInputResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_PLAYBACK_INPUT_RESULT)(cbTable->cbHMIPlaybackInputResult))(rdata->hmiPlaybackInputResult.header.devID, rdata->hmiPlaybackInputResult.result);
        }
        break;
        
    case IPOD_FUNC_HMI_SET_APPLICATION_STATUS_RESULT:
        if(cbTable->cbHMISetApplicationStatusResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_SET_APPLICATION_STATUS_RESULT)(cbTable->cbHMISetApplicationStatusResult))(rdata->hmiSetApplicationStatusResult.header.devID, rdata->hmiSetApplicationStatusResult.result);
        }
        break;
        
    case IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION_RESULT:
        if(cbTable->cbHMISetEventNotificationResult != NULL)
        {
            (*(IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT)(cbTable->cbHMISetEventNotificationResult))(rdata->hmiSetEventNotificationResult.header.devID, rdata->hmiSetEventNotificationResult.result);
        }
        break;
        
    case IPOD_FUNC_HMI_GET_EVENT_CHANGE_RESULT:
        break;
        
    case IPOD_FUNC_HMI_GET_DEVICE_STATUS_RESULT:
        break;

    case IPOD_FUNC_DEVINIT_RESULT:break;
    case IPOD_FUNC_DEVDETECT_RESULT:break;
    case IPOD_FUNC_DEVDEINIT_RESULT:break;
    default:
        //(*(IPOD_CB_PLAY_RESULT*)rdata.info.callback)((S32)IPOD_PLAYER_ERROR);
        break;
    }

    return rc;
}

