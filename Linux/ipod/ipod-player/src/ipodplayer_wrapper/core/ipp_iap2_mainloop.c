#include "iPodPlayerIPCLib.h"
#include "iPodPlayerUtilityLog.h"
#include "ipp_iap2_mainloop.h"
#include "ipp_iap2_init.h"
#include "ipp_iap2_devinit.h"
#include "ipp_iap2_callback.h"
#include "ipp_mainloop_common.h"
#include "ipp_iap2_observer.h"
#include "ipp_iap2_play.h"
#include "ipp_iap2_pause.h"
#include "ipp_iap2_fastforward.h"
#include "ipp_iap2_rewind.h"
#include "ipp_iap2_nexttrack.h"
#include "ipp_iap2_prevtrack.h"
#include "ipp_iap2_nextchapter.h"
#include "ipp_iap2_prevchapter.h"
#include "ipp_iap2_playtrack.h"
#include "ipp_iap2_release.h"

#include "ipp_iap2_setaudiomode.h"
#include "ipp_iap2_seteventnotification.h"
#include "ipp_iap2_setplayspeed.h"
#include "ipp_iap2_changerepeat.h"
#include "ipp_iap2_changeshuffle.h"
#include "ipp_iap2_settrackinfonotification.h"

#include "ipp_iap2_getplaybackstatus.h"
#include "ipp_iap2_gettrackinfo.h"
#include "ipp_iap2_getrepeat.h"
#include "ipp_iap2_getShuffle.h"
#include "ipp_iap2_gettracktotalcount.h"
#include "ipp_iap2_getMediaItemInformation.h"
#include "ipp_iap2_getdeviceproperty.h"
#include "ipp_iap2_getdevicestatus.h"
#include "ipp_iap2_getvolume.h"
#include "ipp_iap2_setvolume.h"

#include "ipp_iap2_getdbentries.h"
#include "ipp_iap2_selectdbentry.h"
#include "ipp_iap2_cleardbselection.h"
#include "ipp_iap2_getdbcount.h"
#include "ipp_iap2_cancel.h"

#include "ipp_iap2_fastforward.h"

#include "ipp_iap2_database.h"
#include "ipp_iap2_setaudiomode.h"
#include "ipp_audiocommon.h"
#include "ipp_iap2_eventnotification.h"
#include "ipp_iap2_requestappstart.h"
#include "ipp_iap2_sendtoapp.h"
#include "ipp_iap2_locationInformation.h"
#include "ipp_iap2_vehiclestatus.h"
#include "ipp_iap2_gototrackposition.h"
#include "ipp_iap2_powersourceupdate.h"
#include "ipp_iap2_callback.h"
#include "ipp_iap2_ctrlcfglist.h"

#include "iPodPlayerCoreFunc.h"

#include <mcheck.h>

/* Set the function tables for iAP2 */

static S32 iPodCoreiAP2FuncDeinit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                                  IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (param == NULL) || (param->waitData == NULL) || (param->contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, 
                                                                        iPodCtrlCfg, param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    switch(param->waitData->stage)
    {
    case 0:
        /* Connection with Audio server was established */
        if(iPodCtrlCfg->sckAudioServer >= 0)
        {
            /* Audio streaming has been started */
            if(iPodCtrlCfg->startAudio > 0)
            {
                /* Stop audio streaming */
                rc = iPodCoreiAP2StopAudio(iPodCtrlCfg);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Change the stage to 1 */
                    param->waitData->stage = 1;
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
            }
            else
            {
                /* Todo */
                rc = IPOD_PLAYER_OK;
            }
        }
        else
        {
            /* Nothing do */
            rc = IPOD_PLAYER_OK;
        }
        break;
        
    case 1:
        /* Check the input parameter */
        if(param->contents->paiResult.header.funcId == IPOD_FUNC_PAI_RESULT)
        {
            if(param->contents->paiResult.cmdId == IPOD_FUNC_PAI_STOP)
            {
                rc = param->contents->paiResult.result;
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* Deinit configuration */
                rc = iPodCoreiAP2AudioDeinitCfg(iPodCtrlCfg);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Change the stage to next */
                    param->waitData->stage = 2;
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        break;
        
    case 2:
        /* Check input parameter */
        if(param->contents->paiResult.header.funcId == IPOD_FUNC_PAI_RESULT)
        {
            if(param->contents->paiResult.cmdId == IPOD_FUNC_PAI_CLRCFG)
            {
                rc = param->contents->paiResult.result;
                if(rc == IPOD_PLAYER_OK)
                {
                    iPodCtrlCfg->audioSetting.mode = param->waitData->contents.setAudioMode.setting.mode;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        break;
        
    default:
        rc = IPOD_PLAYER_ERROR;
        break;
    }
    
    if(rc != IPOD_PLAYER_ERR_REQUEST_CONTINUE)
    {
        /* Result is DISCONNECTED to finalyze the thread */
        rc = IPOD_PLAYER_ERR_DISCONNECTED;
    }
    
    return rc;
}

static void iPodCoreSetiAP2FuncTable(IPOD_PLAYER_CORE_IAP2_FUNC_TABLE *funcTable)
{
    if(funcTable == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, funcTable);
        return;
    }
    
    /* Init function table */
    funcTable[IPOD_FUNC_INIT].func = (IPODCORE_IAP2_FUNC)ippiAP2DevInit;
    funcTable[IPOD_FUNC_INIT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    funcTable[IPOD_FUNC_DEINIT].func = NULL;
    funcTable[IPOD_FUNC_DEINIT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    funcTable[IPOD_FUNC_SELECT_AUDIO_OUT].func = NULL;
    funcTable[IPOD_FUNC_SELECT_AUDIO_OUT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    funcTable[IPOD_FUNC_START_AUTHENTICATION].func = NULL;
    funcTable[IPOD_FUNC_START_AUTHENTICATION].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    
    /* Playback function table */
    funcTable[IPOD_FUNC_TRACK_INFO].func = NULL;
    funcTable[IPOD_FUNC_TRACK_INFO].attr = IPOD_PLAYER_NOTIFY_GROUP_MASK;
    funcTable[IPOD_FUNC_PLAY].func = (IPODCORE_IAP2_FUNC)ippiAP2Play;
    funcTable[IPOD_FUNC_PLAY].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PAUSE].func = (IPODCORE_IAP2_FUNC)ippiAP2Pause;
    funcTable[IPOD_FUNC_PAUSE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_STOP].func =  NULL;
    funcTable[IPOD_FUNC_STOP].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_FASTFORWARD].func = (IPODCORE_IAP2_FUNC)ippiAP2FastForward;
    funcTable[IPOD_FUNC_FASTFORWARD].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_REWIND].func =  (IPODCORE_IAP2_FUNC)ippiAP2Rewind;
    funcTable[IPOD_FUNC_REWIND].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_NEXT_TRACK].func =  (IPODCORE_IAP2_FUNC)ippiAP2NextTrack;
    funcTable[IPOD_FUNC_NEXT_TRACK].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PREV_TRACK].func =  (IPODCORE_IAP2_FUNC)ippiAP2PrevTrack;
    funcTable[IPOD_FUNC_PREV_TRACK].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_NEXTCHAPTER].func =  (IPODCORE_IAP2_FUNC)ippiAP2NextChapter;
    funcTable[IPOD_FUNC_NEXTCHAPTER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PREVCHAPTER].func =  (IPODCORE_IAP2_FUNC)ippiAP2PrevChapter;
    funcTable[IPOD_FUNC_PREVCHAPTER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GOTO_TRACK_POSITION].func =  (IPODCORE_IAP2_FUNC)ippiAP2GotoTrackPosition;
    funcTable[IPOD_FUNC_GOTO_TRACK_POSITION].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_PLAYTRACK].func =  (IPODCORE_IAP2_FUNC)ippiAP2PlayTrack;
    funcTable[IPOD_FUNC_PLAYTRACK].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_RELEASE].func =  (IPODCORE_IAP2_FUNC)ippiAP2Release;
    funcTable[IPOD_FUNC_RELEASE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
  
    /* Set property function table */
    funcTable[IPOD_FUNC_SET_AUDIO_MODE].func = (IPODCORE_IAP2_FUNC)ippiAP2SetAudioMode;
    funcTable[IPOD_FUNC_SET_AUDIO_MODE].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_MODE].func = NULL;
    funcTable[IPOD_FUNC_SET_MODE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_REPEAT].func = NULL;
    funcTable[IPOD_FUNC_SET_REPEAT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_CHANGE_REPEAT].func = (IPODCORE_IAP2_FUNC)ippiAP2ChangeRepeat;          /* support iap2 */
    funcTable[IPOD_FUNC_CHANGE_REPEAT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;                  /* support iap2 */
    funcTable[IPOD_FUNC_SET_SHUFFLE].func = NULL;
    funcTable[IPOD_FUNC_SET_SHUFFLE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_CHANGE_SHUFFLE].func = (IPODCORE_IAP2_FUNC)ippiAP2ChangeShuffle;        /* support iap2 */
    funcTable[IPOD_FUNC_CHANGE_SHUFFLE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;                 /* support iap2 */
    funcTable[IPOD_FUNC_SET_EQUALIZER].func = NULL;
    funcTable[IPOD_FUNC_SET_EQUALIZER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VIDEO_DELAY].func = NULL;
    funcTable[IPOD_FUNC_SET_VIDEO_DELAY].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VIDEO_SETTING].func = NULL;
    funcTable[IPOD_FUNC_SET_VIDEO_SETTING].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_DISPLAY_IMAGE].func = NULL;
    funcTable[IPOD_FUNC_SET_DISPLAY_IMAGE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_PLAY_SPEED].func = (IPODCORE_IAP2_FUNC)ippiAP2SetPlaySpeed;
    funcTable[IPOD_FUNC_SET_PLAY_SPEED].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION].func = (IPODCORE_IAP2_FUNC)ippiAP2SetTrackInfoNotification;
    funcTable[IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION].func = (IPODCORE_IAP2_FUNC)ippiAP2SetEventNotification;
    funcTable[IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    
    /* Get Property function table */
    funcTable[IPOD_FUNC_GET_VIDEO_SETTING].func = NULL;
    funcTable[IPOD_FUNC_GET_VIDEO_SETTING].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_COVERART_INFO].func = NULL;
    funcTable[IPOD_FUNC_GET_COVERART_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_COVERART].func = NULL;
    funcTable[IPOD_FUNC_GET_COVERART].attr = IPOD_PLAYER_COVERART_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_PLAYBACK_STATUS].func = (IPODCORE_IAP2_FUNC)ippiAP2GetPlaybackStatus;
    funcTable[IPOD_FUNC_GET_PLAYBACK_STATUS].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_TRACK_INFO].func = (IPODCORE_IAP2_FUNC)ippiAP2GetTrackInfo;
    funcTable[IPOD_FUNC_GET_TRACK_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_CHAPTER_INFO].func = NULL;
    funcTable[IPOD_FUNC_GET_CHAPTER_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_MODE].func = NULL;
    funcTable[IPOD_FUNC_GET_MODE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_REPEAT].func = (IPODCORE_IAP2_FUNC)ippiAP2GetRepeat;
    funcTable[IPOD_FUNC_GET_REPEAT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_SHUFFLE].func = (IPODCORE_IAP2_FUNC)ippiAP2GetShuffle;
    funcTable[IPOD_FUNC_GET_SHUFFLE].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_PLAY_SPEED].func = NULL;
    funcTable[IPOD_FUNC_GET_PLAY_SPEED].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_TRACK_TOTAL_COUNT].func = (IPODCORE_IAP2_FUNC)ippiAP2GetTrackTotalCount;
    funcTable[IPOD_FUNC_GET_TRACK_TOTAL_COUNT].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_MEDIA_ITEM_INFO].func = (IPODCORE_IAP2_FUNC)ippiAP2GetMediaItemInfo;
    funcTable[IPOD_FUNC_GET_MEDIA_ITEM_INFO].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_EQUALIZER].func = NULL;
    funcTable[IPOD_FUNC_GET_EQUALIZER].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_EQUALIZER_NAME].func = NULL;
    funcTable[IPOD_FUNC_GET_EQUALIZER_NAME].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_DEVICE_PROPERTY].func = (IPODCORE_IAP2_FUNC)ippiAP2GetDeviceProperty;
    funcTable[IPOD_FUNC_GET_DEVICE_PROPERTY].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_DEVICE_STATUS].func = (IPODCORE_IAP2_FUNC)ippiAP2GetDeviceStatus;
    funcTable[IPOD_FUNC_GET_DEVICE_STATUS].attr = IPOD_PLAYER_PLAYBACK_GROUP_MASK;
    
    /* Database operation function table */
    funcTable[IPOD_FUNC_GET_DB_ENTRIES].func = (IPODCORE_IAP2_FUNC)ippiAP2GetDBEntries;
    funcTable[IPOD_FUNC_GET_DB_ENTRIES].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_DB_COUNT].func = (IPODCORE_IAP2_FUNC)ippiAP2GetDBCount;
    funcTable[IPOD_FUNC_GET_DB_COUNT].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_SELECT_DB_ENTRY].func = (IPODCORE_IAP2_FUNC)ippiAP2SelectDBEntry;
    funcTable[IPOD_FUNC_SELECT_DB_ENTRY].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_CANCEL].func = (IPODCORE_IAP2_FUNC)ippiAP2Cancel;
    funcTable[IPOD_FUNC_CANCEL].attr = IPOD_PLAYER_CANCEL_GROUP_MASK;
    funcTable[IPOD_FUNC_CLEAR_SELECTION].func = (IPODCORE_IAP2_FUNC)ippiAP2ClearDBSelection;
    funcTable[IPOD_FUNC_CLEAR_SELECTION].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    funcTable[IPOD_FUNC_SELECT_AV].func = NULL;
    funcTable[IPOD_FUNC_SELECT_AV].attr = IPOD_PLAYER_DATABASE_GROUP_MASK;
    
    /* Non Player function table */
    funcTable[IPOD_FUNC_OPEN_SONG_TAG_FILE].func = NULL;
    funcTable[IPOD_FUNC_OPEN_SONG_TAG_FILE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_CLOSE_SONG_TAG_FILE].func = NULL;
    funcTable[IPOD_FUNC_CLOSE_SONG_TAG_FILE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SONG_TAG].func = NULL;
    funcTable[IPOD_FUNC_SONG_TAG].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SEND_TO_APP].func = (IPODCORE_IAP2_FUNC)ippiAP2SendToApp;
    funcTable[IPOD_FUNC_SEND_TO_APP].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_REQUEST_APP_START].func = (IPODCORE_IAP2_FUNC)ippiAP2RequestAppStart;
    funcTable[IPOD_FUNC_REQUEST_APP_START].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_VOLUME].func = (IPODCORE_IAP2_FUNC)ippiAP2SetVolume;
    funcTable[IPOD_FUNC_SET_VOLUME].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    funcTable[IPOD_FUNC_GET_VOLUME].func = (IPODCORE_IAP2_FUNC)ippiAP2GetVolume;
    funcTable[IPOD_FUNC_GET_VOLUME].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;

    funcTable[IPOD_FUNC_SET_LOCATION_INFO].func = (IPODCORE_IAP2_FUNC)ippiAP2SetLocationInformation;
    funcTable[IPOD_FUNC_SET_LOCATION_INFO].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;

    funcTable[IPOD_FUNC_SET_VEHICLE_STATUS].func = (IPODCORE_IAP2_FUNC)ippiAP2SetVehicleStatus;
    funcTable[IPOD_FUNC_SET_VEHICLE_STATUS].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    funcTable[IPOD_FUNC_SET_POWER_SUPPLY].func = (IPODCORE_IAP2_FUNC)ippiAP2PowerSourceUpdate;
    funcTable[IPOD_FUNC_SET_POWER_SUPPLY].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_PAI_RESULT].func = NULL;
    funcTable[IPOD_FUNC_PAI_RESULT].attr = IPOD_PLAYER_AUDIO_GROUP_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_GET_STATUS].func = NULL;
    funcTable[IPOD_FUNC_INTERNAL_GET_STATUS].attr = IPOD_PLAYER_INTERNAL_GET_STATUS_MASK;

    funcTable[IPOD_FUNC_INTERNAL_NOTIFY_STATUS].func = NULL;
    funcTable[IPOD_FUNC_INTERNAL_NOTIFY_STATUS].attr = IPOD_PLAYER_STATUS_GROUP_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_HMI_STATUS_SEND].func = NULL;
    funcTable[IPOD_FUNC_INTERNAL_HMI_STATUS_SEND].attr = IPOD_PLAYER_INTERNAL_HMI_STATUS_MASK;
    
    funcTable[IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND].func = NULL;
    funcTable[IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND].attr = IPOD_PLAYER_INTERNAL_HMI_STATUS_MASK;
    
    /* HMI Operation Function table. */
    funcTable[IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE].func = NULL;
    funcTable[IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE].func = NULL;
    funcTable[IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_BUTTON_INPUT].func = NULL;
    funcTable[IPOD_FUNC_HMI_BUTTON_INPUT].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_ROTATION_INPUT].func = NULL;
    funcTable[IPOD_FUNC_HMI_ROTATION_INPUT].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_PLAYBACK_INPUT].func = NULL;
    funcTable[IPOD_FUNC_HMI_PLAYBACK_INPUT].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_SET_APPLICATION_STATUS].func = NULL;
    funcTable[IPOD_FUNC_HMI_SET_APPLICATION_STATUS].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION].func = NULL;
    funcTable[IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_GET_EVENT_CHANGE].func = NULL;
    funcTable[IPOD_FUNC_HMI_GET_EVENT_CHANGE].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_HMI_GET_DEVICE_STATUS].func = NULL;
    funcTable[IPOD_FUNC_HMI_GET_DEVICE_STATUS].attr = IPOD_PLAYER_NONPLAYER_GROUP_MASK;
    
    funcTable[IPOD_FUNC_DEVDEINIT].func = (IPODCORE_IAP2_FUNC)iPodCoreiAP2FuncDeinit;
    funcTable[IPOD_FUNC_DEVDEINIT].attr = IPOD_PLAYER_INIT_GROUP_MASK;
    
    return;
}

static U32 iPodCoreiAP2GetResultSize(IPOD_PLAYER_FUNC_RESULT_ID resultId)
{
    U32 size = 0;
    
    switch(resultId)
    {
    case IPOD_FUNC_INIT_RESULT:
    case IPOD_FUNC_DEINIT_RESULT:
    case IPOD_FUNC_SELECT_AUDIO_OUT_RESULT:
    case IPOD_FUNC_TEST_READY_RESULT:
    case IPOD_FUNC_SET_IOS_APPS_INFO_RESULT:
    case IPOD_FUNC_START_AUTHENTICATION_RESULT:
    case IPOD_FUNC_PAUSE_RESULT:
    case IPOD_FUNC_STOP_RESULT:
    case IPOD_FUNC_NEXT_TRACK_RESULT:
    case IPOD_FUNC_PREV_TRACK_RESULT:
    case IPOD_FUNC_NEXTCHAPTER_RESULT:
    case IPOD_FUNC_PREVCHAPTER_RESULT:
    case IPOD_FUNC_FASTFORWARD_RESULT:
    case IPOD_FUNC_REWIND_RESULT:
    case IPOD_FUNC_GOTO_TRACK_POSITION_RESULT:
    case IPOD_FUNC_PLAYTRACK_RESULT:
    case IPOD_FUNC_RELEASE_RESULT:
    case IPOD_FUNC_SET_AUDIO_MODE_RESULT:
    case IPOD_FUNC_SET_MODE_RESULT:
    case IPOD_FUNC_SET_REPEAT_RESULT:
    case IPOD_FUNC_SET_SHUFFLE_RESULT:
    case IPOD_FUNC_SET_EQUALIZER_RESULT:
    case IPOD_FUNC_SET_VIDEO_DELAY_RESULT:
    case IPOD_FUNC_SET_VIDEO_SETTING_RESULT:
    case IPOD_FUNC_SET_DISPLAY_IMAGE_RESULT:
    case IPOD_FUNC_SET_PLAY_SPEED_RESULT:
    case IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION_RESULT:
    case IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION_RESULT:
    case IPOD_FUNC_SELECT_DB_ENTRY_RESULT:
    case IPOD_FUNC_CLEAR_SELECTION_RESULT:
    case IPOD_FUNC_SELECT_AV_RESULT:
    case IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE_RESULT:
    case IPOD_FUNC_CANCEL_RESULT:
    case IPOD_FUNC_HMI_BUTTON_INPUT_RESULT:
    case IPOD_FUNC_HMI_ROTATION_INPUT_RESULT:
    case IPOD_FUNC_HMI_PLAYBACK_INPUT_RESULT:
    case IPOD_FUNC_HMI_SET_APPLICATION_STATUS_RESULT:
    case IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION_RESULT:
        size = sizeof(IPOD_PLAYER_PARAM_RESULT_TEMP);
        break;
        
    case IPOD_FUNC_PLAY_RESULT:
        size = sizeof(IPOD_CB_PARAM_PLAY_RESULT);
        break;

    case IPOD_FUNC_GET_VIDEO_SETTING_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_VIDEO_SETTING_RESULT);
        break;
        
    case IPOD_FUNC_GET_COVERART_INFO_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_COVERART_INFO_RESULT);
        break;
        
    case IPOD_FUNC_GET_COVERART_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_COVERART_RESULT);
        break;
        
    case IPOD_FUNC_GET_PLAYBACK_STATUS_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_PLAYBACK_STATUS_RESULT);
        break;
        
    case IPOD_FUNC_GET_TRACK_INFO_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
        break;
        
    case IPOD_FUNC_GET_CHAPTER_INFO_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_CHAPTER_INFO_RESULT);
        break;
        
    case IPOD_FUNC_GET_MODE_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_MODE_RESULT);
        break;
        
    case IPOD_FUNC_GET_REPEAT_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_REPEAT_STATUS_RESULT);
        break;
        
    case IPOD_FUNC_GET_SHUFFLE_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_SHUFFLE_STATUS_RESULT);
        break;
        
    case IPOD_FUNC_GET_PLAY_SPEED_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_PLAY_SPEED_RESULT);
        break;
        
    case IPOD_FUNC_GET_TRACK_TOTAL_COUNT_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_TRACK_TOTAL_COUNT_RESULT);
        break;
        
    case IPOD_FUNC_GET_MEDIA_ITEM_INFO_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_MEDIA_ITEM_INFO_RESULT);
        break;
        
    case IPOD_FUNC_GET_EQUALIZER_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_EQUALIZER_RESULT);
        break;
        
    case IPOD_FUNC_GET_EQUALIZER_NAME_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_EQUALIZER_NAME_RESULT);
        break;
        
    case IPOD_FUNC_GET_DEVICE_PROPERTY_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_DEVICE_PROPERTY_RESULT);
        break;
        
    case IPOD_FUNC_GET_DEVICE_STATUS_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_DEVICE_STATUS_RESULT);
        break;
        
    case IPOD_FUNC_GET_DB_ENTRIES_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_DB_ENTRIES_RESULT);
        break;
        
    case IPOD_FUNC_GET_DB_COUNT_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_DB_COUNT_RESULT);
        break;
        
    case IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE_RESULT:
        size = sizeof(IPOD_CB_PARAM_HMI_GET_SUPPORTED_FEATURE_RESULT);
        break;

    case IPOD_FUNC_HMI_GET_EVENT_CHANGE_RESULT:
        size = sizeof(IPOD_CB_PARAM_HMI_GET_EVENT_CHANGE_RESULT);
        break;
    
    case IPOD_FUNC_HMI_GET_DEVICE_STATUS_RESULT:
        size = sizeof(IPOD_CB_PARAM_HMI_GET_DEVICE_STATUS_RESULT);
        break;
    
    /* Non Player Function Result ID. */
    
    case IPOD_FUNC_SEND_TO_APP_RESULT:
        size = sizeof(IPOD_CB_PARAM_SEND_TO_APP_RESULT);
        break;
    
    case IPOD_FUNC_OPEN_SONG_TAG_FILE_RESULT:
        size = sizeof(IPOD_CB_PARAM_OPEN_SONG_TAG_FILE_RESULT);
        break;
        
    case IPOD_FUNC_GET_PL_PROPERTIES_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_PL_PROPERTIES_RESULT);
        break;
        
    case IPOD_FUNC_GET_VOLUME_RESULT:
        size = sizeof(IPOD_CB_PARAM_GET_VOLUME_RESULT);
        break;
    
    case IPOD_FUNC_CLOSE_SONG_TAG_FILE_RESULT:
    
    case IPOD_FUNC_SONG_TAG_RESULT:
    
    case IPOD_FUNC_REQUEST_APP_START_RESULT:
    
    case IPOD_FUNC_SET_POWER_SUPPLY_RESULT:
    
    case IPOD_FUNC_SET_VOLUME_RESULT:
        
    case IPOD_FUNC_CREATE_INTELLIGENT_PL_RESULT:
    
    case IPOD_FUNC_TRACK_SUPPORTS_INTELLIGENT_PL_RESULT:
    
    case IPOD_FUNC_REFRESH_INTELLIGENT_PL_RESULT:
    
    case IPOD_FUNC_SET_GPS_DATA_RESULT:
    
    case IPOD_FUNC_SET_GPS_CURRENT_SYSTEM_TIME_RESULT:
    
    case IPOD_FUNC_SET_DEVICE_DETECTION_RESULT:


    /* Internal Function Result ID. */
    case IPOD_FUNC_DEVINIT_RESULT:
    
    case IPOD_FUNC_DEVDETECT_RESULT:

    case IPOD_FUNC_DEVDEINIT_RESULT:
    
    case IPOD_FUNC_EF_RESULT:
    
    /* Audio Streaming Function ID */
    case IPOD_FUNC_PAI_START_RESULT:
    
    case IPOD_FUNC_PAI_STOP_RESULT:
    
    case IPOD_FUNC_PAI_SETCFG_RESULT:
    
    case IPOD_FUNC_PAI_SETSR_RESULT:
    
    case IPOD_FUNC_PAI_SETVOL_RESULT:
    
    case IPOD_FUNC_PAI_GETVOL_RESULT:
    
    case IPOD_FUNC_PAI_CLRCFG_RESULT:
    
    case IPOD_FUNC_PAI_RESULT_RESULT:
    
    /* Internal function */
    case IPOD_FUNC_INTERNAL_GET_STATUS_RESULT:
    
    case IPOD_FUNC_INTERNAL_NOTIFY_STATUS_RESULT:
    
    case IPOD_FUNC_INTERNAL_HMI_STATUS_SEND_RESULT:
    
    case IPOD_FUNC_INTERNAL_REMOTE_EVENT_NOTIFICATION_RESULT:
    
    case IPOD_FUNC_SHUTDOWN_RESULT:

    default:
        size = sizeof(IPOD_PLAYER_PARAM_RESULT_TEMP);
        break;
    }
    
    return size;
}

static S32 iPodCoreiAP2SetQueue(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_RESULT_TEMP resultTemp;
    
    /* Parameter check */
    if((iPodCtrlCfg== NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Initialize the structure */
    memset(&resultTemp, 0, sizeof(resultTemp));
    
    /* Set the receving data to tail of queue */
    rc = iPodCoreSetQueue(iPodCtrlCfg->waitList, header, contents, size);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Sort the queue */
        iPodCoreSortQueue(IPODCORE_WAIT_LIST_MAX, iPodCtrlCfg->waitList);
    }
    /* Request was not set to queue because queue is full */
    else
    {
        /* Send the error to Application */
        resultTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | 
                                                         contents->paramTemp.header.funcId);
        resultTemp.result = rc;
        size = sizeof(resultTemp);
        iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&resultTemp, size, 0, IPODCORE_TMOUT_FOREVER);
    }
    
    return rc;
}

S32 iPodCoreiAP2ExecuteQueue(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 listMax, IPOD_PLAYER_CORE_WAIT_LIST *waitList,
                             IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_IAP2_FUNC_TABLE *funcTable = NULL;
    U32 i = 0;
    U32 attr = 0;
    IPOD_PLAYER_FUNC_ID funcId = (IPOD_PLAYER_FUNC_ID)0;
    IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM param;
    U32 resSize = 0;
    /* Parameter check */
    if((iPodCtrlCfg== NULL) || (waitList == NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitList, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&param, 0, sizeof(param));
    
    funcTable = (IPOD_PLAYER_CORE_IAP2_FUNC_TABLE *)iPodCtrlCfg->funcTable;
    /* Loop until maximum queueu number */
    for(i = 0; (i < listMax) && (rc != IPOD_PLAYER_ERR_DISCONNECTED); i++)
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
                    
                    param.waitData = &waitList[i];
                    param.header = header;
                    param.size = size;
                    param.contents = contents;
                    /* Call requested function */
                    rc = funcTable[funcId].func(iPodCtrlCfg, &param);
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
                /* function is null. Set to not support error */
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
            if((rc != IPOD_PLAYER_ERR_NO_REPLY) &&
               (rc != IPOD_PLAYER_ERR_DISCONNECTED))
            {
                /* Set result */
                waitList[i].contents.paramResultTemp.header.longData = 0;
                waitList[i].contents.paramResultTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | funcId);
                waitList[i].contents.paramResultTemp.result = rc;
                resSize = iPodCoreiAP2GetResultSize((IPOD_PLAYER_FUNC_RESULT_ID)waitList[i].contents.paramResultTemp.header.funcId);
                
                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitList[i].contents, resSize, 0, IPODCORE_TMOUT_FOREVER);
                if(rc == (S32)resSize)
                {
                    rc = IPOD_PLAYER_OK;
                }
            }
        }
    }
    
    return rc;
}


static S32 iPodCoreiAP2GetInternalRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 type, 
                                        S32 fd, IPOD_PLAYER_MESSAGE_DATA_CONTENTS **contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERR_NO_INTERNAL_REQUEST;
    U64 timeCount = 0;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* For Lint */
    type = type;
    
    if(fd == iPodCtrlCfg->iPodInfo->nextRequestFd)
    {
        /* Read the time count */
        rc = read(fd, &timeCount, sizeof(timeCount));
        if(rc >= 0)
        {
            /* Set the request to execute the request. */
            memset(iPodCtrlCfg->contents, 0, sizeof(*iPodCtrlCfg->contents));
            *contents = iPodCtrlCfg->contents;
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
            rc = IPOD_PLAYER_ERR_TIMER;
        }
    }
    
    return rc;
}


static S32 iPodCoreiAP2GetApplicationRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 type, 
                                        S32 fd, IPOD_PLAYER_MESSAGE_DATA_CONTENTS **contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfo = NULL;
    
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
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiAP2SetQueue(iPodCtrlCfg, &iPodCtrlCfg->contents->paramTemp.header, *contents, *size);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Request from iAP2Service callback */
static S32 iPodCoreiAP2GetiAP2ServiceCallbackRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, S32 fd)
{
    S32 rc = IPOD_PLAYER_ERR_NO_IAP2SARVICECALLBACK_REQUEST;
    S32 rcmq = 0;
    U8 eventValue[] = IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE;
    static U32 debugCount = 0;

    if(fd == iPodCtrlCfg->serviceCallbacksEventFD)
    {
        rcmq = mq_receive(iPodCtrlCfg->serviceCallbacksEventFD, (char *)eventValue, sizeof(eventValue), NULL);
        if(rcmq != -1)
        {
            rc = IPOD_PLAYER_OK;
            debugCount++;
            //IPOD_DLT_INFO("[DBG]receive event value=%s, count=%u", eventValue, debugCount);
            if(strncmp((const char *)eventValue, IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICECONNECTEDDEVICE1, sizeof(eventValue)) == 0)
            {
                rc = ippiAP2ServiceInitDeviceConnection(&iPodCtrlCfg->iap2Param);
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcmq, errno);
            rc = IPOD_PLAYER_ERROR;
        }
    }

    return rc;
}

/* Iterate iAP2 request for each fd */
static S32 iPodCoreiAP2GetiAP2Request(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param, S32 fd)
{
    S32 rc = IPOD_PLAYER_ERR_NO_IAP2_REQUEST;
    U32 i = 0;
    
    for(i = 0; i < (U32)iap2Param->pollFDs.numberFDs; i++)
    {
        /* File descriptor matches with one of iAP2 file descriptor */
        if(fd == iap2Param->pollFDs.fds[i].fd)
        {
            /* This path is a past implementation. */
            /* ippiAP2HandleEvent is not used in iAP2Service. */
            /* Instead iAP2ServiceHandleEvents is used.       */
            /* This path should not be reached.               */
            IPOD_DLT_INFO("This path should not be reached.");
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR);
            /* Iterate iAP2 library */
            rc = ippiAP2HandleEvent(iap2Param, fd, iap2Param->pollFDs.fds[i].event);
            break;
        }
    }
    
    return rc;
}

/* Execute request */
static S32 iPodCoreiAP2ExecuteRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_FUNC_HEADER *header,
                                      IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Execute the queue */
    rc = iPodCoreiAP2ExecuteQueue(iPodCtrlCfg, IPODCORE_WAIT_LIST_MAX, iPodCtrlCfg->waitList, header, contents, size);
    /* Remove the list that current status is finish */
    iPodCoreDelQueue(iPodCtrlCfg->waitList);
    
    /* Clear the current running mask */
    iPodCtrlCfg->curRunMask = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

static void iPodCoreiAP2Deinit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 i = 0;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPODCORE_DEINIT_DELAY_WAIT};
    S32 rcm = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }

    ippiAP2Deinit(&iPodCtrlCfg->iap2Param, iPodCtrlCfg->threadInfo->nameInfo.deviceName);
    nanosleep(&waitTime, NULL);
    rcm = ippiAP2DeinitServiceCallbacksMutex(iPodCtrlCfg);
    if(rcm ==  IPOD_PLAYER_ERROR)
    {
        nanosleep(&waitTime, NULL);
        rcm = ippiAP2DeinitServiceCallbacksMutex(iPodCtrlCfg);
        if(rcm ==  IPOD_PLAYER_ERROR)
        {
            IPOD_DLT_ERROR("Mutex deinit failed.");
        }
    }
    IPOD_DLT_INFO("Start Deinit");

    if(iPodCtrlCfg->iap2Param.xferList != NULL)
    {
        ippiAP2FileXferDeinitAll(&iPodCtrlCfg->iap2Param.xferList);
    }
    else
    {
        IPOD_DLT_INFO("The FileXferDeinit was canceled.");
    }
    
    for(i = 0; i < (U32)iPodCtrlCfg->iap2Param.pollFDs.numberFDs; i++)
    {
        iPodPlayerIPCDeleteHandle(iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd);
        iPodCoreEpollCtl(iPodCtrlCfg->waitHandle, iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd, EPOLL_CTL_DEL, iPodCtrlCfg->iap2Param.pollFDs.fds[i].event);
    }

    ippiAP2ClearCallbacks(&iPodCtrlCfg->iap2Param);
    ippiAP2CloseDB(iPodCtrlCfg);
    ippiAP2EventNotificationClearNotifyFunctions(iPodCtrlCfg->iap2Param.notifyFuncTable);
    ippiAP2DeinitParam(&iPodCtrlCfg->iap2Param);
    
    if(iPodCtrlCfg->iap2Param.iap2MediaParam != NULL)
    {
        free(iPodCtrlCfg->iap2Param.iap2MediaParam);
        iPodCtrlCfg->iap2Param.iap2MediaParam = NULL;
    }

    /* Dequeue from CtrlCfgList */
    rc = ippiAP2DequeueFromCtrlCfgList(iPodCtrlCfg);
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    ippiAP2DeinitServiceCallbackRequest(iPodCtrlCfg);
    
    /* Clear structure */
    memset(&iPodCtrlCfg->iap2Param, 0, sizeof(iPodCtrlCfg->iap2Param));
    
    if(iPodCtrlCfg->funcTable != NULL)
    {
        free(iPodCtrlCfg->funcTable);
        iPodCtrlCfg->funcTable = NULL;
    }
    
    return;
}

static S32 iPodCoreiAP2Init(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 prefix[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    U8 memoryRevision[IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN] = {0};
    BOOL bluetoothConnection = FALSE;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Allocate memory for function table */
    iPodCtrlCfg->funcTable = calloc(IPOD_FUNC_MAX, sizeof(IPOD_PLAYER_CORE_IAP2_FUNC_TABLE));
    if(iPodCtrlCfg->funcTable != NULL)
    {
        /* Set function table for iAP2 */
        iPodCoreSetiAP2FuncTable((IPOD_PLAYER_CORE_IAP2_FUNC_TABLE *)iPodCtrlCfg->funcTable);
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, 0);
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    memset(&iPodCtrlCfg->iap2Param, 0, sizeof(iPodCtrlCfg->iap2Param));
    iPodCtrlCfg->iap2Param.sampleRateStatus = FALSE;  /* initialize sample rate status */
    iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaLibraryType = IPOD_PLAYER_MEDIA_LIBRARY_TYPE_LOCAL;
    iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaLibraryType = IPOD_PLAYER_MEDIA_LIBRARY_TYPE_AMR;
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCtrlCfg->iap2Param.iap2MediaParam = calloc(1, sizeof(*iPodCtrlCfg->iap2Param.iap2MediaParam));
        if(iPodCtrlCfg->iap2Param.iap2MediaParam != NULL)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCtrlCfg->iap2Param.notifyFuncTable = ippiAP2EventNotificationSetNotifyFunctions();
        if(iPodCtrlCfg->iap2Param.notifyFuncTable != NULL)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_DATABASE_LOCATION_PREFIX, sizeof(prefix), prefix);
        /* Open database */
        IPOD_DLT_INFO("devType=%d", iPodCtrlCfg->threadInfo->nameInfo.devType);
        if(iPodCtrlCfg->threadInfo->nameInfo.devType == IPOD_PLAYER_DEVICE_TYPE_BT)
        {
            bluetoothConnection = TRUE;
        }
        iPodCtrlCfg->iap2Param.dbHandle = ippiAP2CreateDB((const U8 *)prefix, iPodCtrlCfg->threadInfo->nameInfo.deviceName, bluetoothConnection);
        if(iPodCtrlCfg->iap2Param.dbHandle != NULL)
        {
            memoryRevision[0] = '\0';
            ippiAP2DBGetRevision(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, memoryRevision);
            strncpy((char *)iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbRevision, (const char *)memoryRevision, sizeof(iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbRevision));

            memoryRevision[0] = '\0';
            ippiAP2DBGetRevision(&iPodCtrlCfg->iap2Param.dbHandle->appleMusicRadio, memoryRevision);
            strncpy((char *)iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbRevision, (const char *)memoryRevision, sizeof(iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbRevision));

            /* Database was able to be opened */
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            /* Database was not able to be opened */
            rc = IPOD_PLAYER_ERR_DB_OPEN;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Initialize the parameter for iAP2 */
        rc = ippiAP2InitParam(&iPodCtrlCfg->iap2Param, iPodCtrlCfg->threadInfo->nameInfo.devType, iPodCtrlCfg->threadInfo->nameInfo.deviceName, iPodCtrlCfg);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Set callback functions to be called from iAP2 */
            rc = ippiAP2SetCallbacks(&iPodCtrlCfg->iap2Param);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Initialize the iAP2. Authentication and Itentification will be started  */
                rc = ippiAP2Init(&iPodCtrlCfg->iap2Param);
            }
        }
    }
     
    if(rc != IPOD_PLAYER_OK)
    {
        iPodCoreiAP2Deinit(iPodCtrlCfg);
    }
    
    return rc;
}

static S32 iPodCoreiAP2GetAndSetiAP2FDs(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    S32 handle = -1;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get fds from iAP2 */
    rc = ippiAP2GetPollFDs(&iPodCtrlCfg->iap2Param);
    if((rc == IPOD_PLAYER_OK) && (iPodCtrlCfg->iap2Param.pollFDs.fds != NULL))
    {
        /* Set fds to epoll */
        for(i = 0; i < (U32)iPodCtrlCfg->iap2Param.pollFDs.numberFDs; i++)
        {
            rc = iPodCoreEpollCtl(iPodCtrlCfg->waitHandle, iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd, EPOLL_CTL_ADD, iPodCtrlCfg->iap2Param.pollFDs.fds[i].event);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Register the handle to iPodCtrlCfg handles */
                rc = iPodPlayerIPCCreateHandle(&handle, iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd);
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = iPodCoreSetHandle(iPodCtrlCfg->handle, &iPodCtrlCfg->handleNum, iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd);
                }
                rc = IPOD_PLAYER_OK;
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

static void iPodCoreiAP2GetAndDeliAP2FDs(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }
    
    /* Get fds from iAP2 */
    rc = ippiAP2GetPollFDs(&iPodCtrlCfg->iap2Param);
    if((rc == IPOD_PLAYER_OK) && (iPodCtrlCfg->iap2Param.pollFDs.fds != NULL))
    {
        /* Set fds to epoll */
        for(i = 0; i < (U32)iPodCtrlCfg->iap2Param.pollFDs.numberFDs; i++)
        {
            iPodCoreEpollCtl(iPodCtrlCfg->waitHandle, iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd, EPOLL_CTL_DEL, iPodCtrlCfg->iap2Param.pollFDs.fds[i].event);
            iPodCoreClearHandle(iPodCtrlCfg->handle, &iPodCtrlCfg->handleNum, iPodCtrlCfg->iap2Param.pollFDs.fds[i].fd);
        }
    }
    
    return;
}

S32 iPodCoreiAP2MainLoop(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 endFlag = 0;
    U32 checkNum = 0;
    U32 size = 0;
    U32 i = 0;
    S32 rcm = IPOD_PLAYER_ERROR;
    
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPODCORE_EPOLL_RETRY_WAIT};
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents = NULL;
    IPOD_PLAYER_IPC_HANDLE_INFO outputInfo[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(outputInfo, 0, sizeof(outputInfo));
    
    pthread_cleanup_push((void *)iPodCoreiAP2Deinit, (void *)iPodCtrlCfg);
    contents = NULL;
    endFlag = 0;
    
    /* Initialize for iAP2 */
    rc = iPodCoreiAP2Init(iPodCtrlCfg);
    if(rc != IPOD_PLAYER_OK)
    {
        endFlag = 1;
    }
    
    while(endFlag == 0)
    {
        /* Set the fds to poll */
        iPodCoreiAP2GetAndSetiAP2FDs(iPodCtrlCfg);
        /* Wait for fds */
        rc = iPodCorePollWait(iPodCtrlCfg->waitHandle, iPodCtrlCfg->threadInfo->aTime, sizeof(outputInfo), outputInfo);
        rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
        if(rcm != IPOD_PLAYER_OK)
        {
            endFlag = 1;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        }
        if(rc > 0)
        {
            checkNum = rc;
            /* Loop until receivable event number */
            for(i = 0; (i < checkNum) && (endFlag == 0); i++)
            {
                /* Get the request from iAP2Service callback */
                rc = iPodCoreiAP2GetiAP2ServiceCallbackRequest(iPodCtrlCfg, outputInfo[i].handle);
                if(rc == IPOD_PLAYER_ERR_NO_IAP2SARVICECALLBACK_REQUEST)
                {
                    /* Get the request from iAP2 library */
                    rc = iPodCoreiAP2GetiAP2Request(&iPodCtrlCfg->iap2Param, outputInfo[i].handle);
                    if(rc == IPOD_PLAYER_ERR_NO_IAP2_REQUEST)
                    {
                        /* Get the internal request */
                        rc = iPodCoreiAP2GetInternalRequest(iPodCtrlCfg, outputInfo[i].type, outputInfo[i].handle, &contents, &size);
                        if(rc == IPOD_PLAYER_ERR_NO_INTERNAL_REQUEST)
                        {
                            /* Get the request from application */
                            rc = iPodCoreiAP2GetApplicationRequest(iPodCtrlCfg, outputInfo[i].type, outputInfo[i].handle, &contents, &size);
                        }
                        if(rc == IPOD_PLAYER_OK)
                        {
                            /* Execute the request */
                            rc = iPodCoreiAP2ExecuteRequest(iPodCtrlCfg, &iPodCtrlCfg->contents->paramTemp.header, contents, size);
                            if(rc == IPOD_PLAYER_ERR_DISCONNECTED)
                            {
                                /* Device was disconnected. Thread will be deleted */
                                endFlag = 1;
                            }
                        }
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
            /* Nothing do */
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
        
        /* Check status changes */
        rc = iPodCoreObserver(iPodCtrlCfg);
        if((iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_SUPPORT_IAP1) != 0) &&
           (rc == IPOD_PLAYER_ERR_IAP1_DETECTED))
        {
            endFlag = 1;
        }
        rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
        if(rcm != IPOD_PLAYER_OK)
        {
            endFlag = 1;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        }
    }

    iPodCoreiAP2GetAndDeliAP2FDs(iPodCtrlCfg);
    pthread_cleanup_pop(1);
    
    return rc;
}

