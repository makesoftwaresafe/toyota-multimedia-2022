#include <adit_typedef.h>
#include "iPodPlayerDef.h"
#include "iPodPlayerCB.h"
#include "iPodPlayerAPI.h"
#include "iPodPlayerDebug.h"
#include "iPodPlayerUtilityLog.h"
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <pthread_adit.h>

#define IPOD_UNIT_WAIT_RETRY_MAX 200
EXPORT U32 g_devID = 0;
EXPORT IPOD_PLAYER_CONNECTION_STATUS g_connectStatus;
EXPORT IPOD_PLAYER_PLAYBACK_STATUS g_playbackStatus;
EXPORT IPOD_PLAYER_DEVICE_PROPERTY g_property;
EXPORT IPOD_PLAYER_REPEAT_STATUS g_repeat;
EXPORT IPOD_PLAYER_SHUFFLE_STATUS g_shuffle;
EXPORT IPOD_PLAYER_PLAYING_SPEED g_speed;
EXPORT IPOD_PLAYER_TRACK_INFO g_info;
EXPORT U32 g_coverartCount = 0;
EXPORT U32 g_coverartTime[256] = {0};
EXPORT U32 g_result = 0;
EXPORT S32 g_error = IPOD_PLAYER_OK;
EXPORT U32 g_appID = 0;
EXPORT pthread_cond_t g_cond;
EXPORT pthread_mutex_t g_mutex;
EXPORT U32 g_waitMask;

S32 iPodPlayerTestWaitInit(pthread_cond_t *cond, pthread_mutex_t *mutex);
void iPodPlayerTestWaitDeinit(pthread_cond_t *cond, pthread_mutex_t *mutex);
S32 iPodPlayerTestWait(pthread_cond_t *cond, pthread_mutex_t *mutex, U32 sec);

void app_cb_play(U32 devID, S32 result);
void app_cb_playCurrentSelection(U32 devID, S32 result);
void app_cb_pause(U32 devID, S32 result);
void app_cb_trackup(U32 devID, S32 result);
void app_cb_modechange(U32 devID, S32 result);
void app_cb_playtrack(U32 devID, S32 result);
void app_cb_release(U32 devID, S32 result);

void app_cb_notifyConnectionStatus(U32 devID, IPOD_PLAYER_CONNECTION_STATUS *status);
void app_cb_notifyPlaybackStatus(U32 devID, IPOD_PLAYER_PLAYBACK_STATUS *status);
void app_cb_notifyGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 count, IPOD_PLAYER_ENTRY_LIST *entryList);
void app_cb_notifyOpenApp(U32 devID, U32 appID);
void app_cb_notifyCloseApp(U32 devID, U32 appID);
void app_cb_notifyReceiveFromApp(U32 devID, U32 appID, U32 dataSize, U8 *data);
void app_cb_notifyGetCoverart(U32 devID, U32 trackIndex, U32 time, IPOD_PLAYER_COVERART_HEADER *header, U32 size, U8 *data);
void app_cb_getCoverart(U32 devID, S32 result);
void app_cb_openTagFile(U32 devID, S32 result, U32 handle);
void app_cb_closeTagFile(U32 devID, S32 result);
void app_cb_songTag(U32 devID, S32 result);
void app_cb_setAudioMode(U32 devID, S32 result);

void app_cb_setTrackInfoNotification(U32 devID, S32 result);

void app_cb_getCoverartInfo(U32 devID, S32 result, U32 timeCount, U32 *time);
void app_cb_getRepeatStatus(U32 devID, S32 result, IPOD_PLAYER_REPEAT_STATUS status);
void app_cb_getShuffleStatus(U32 devID, S32 result, IPOD_PLAYER_SHUFFLE_STATUS status);
void app_cb_getPlaySpeed(U32 devID, S32 result, IPOD_PLAYER_PLAYING_SPEED speed);
void app_cb_getTrackTotalCount(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U32 count);
void app_cb_getDeviceProperty(U32 devID, S32 result, IPOD_PLAYER_DEVICE_PROPERTY *property);
void app_cb_getTrackInfo(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, IPOD_PLAYER_TRACK_INFO *info);


void app_cb_getDBCount(U32 devID, S32 result, U32 num);
void app_cb_selectDBEntry(U32 devID, S32 result);
void app_cb_GetDBEntries(U32 devID, S32 result);
void app_cb_cancel(U32 devID, S32 result);
void app_cb_clearSelection(U32 devID, S32 result);
void app_cb_selectAV(U32 devID, S32 result);

U8 data[]="aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";

/* Wait the result callback */
S32 iPodPlayerTestWaitCB(U32 callCount, U32 *callResult)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Loop until all resullt is received */
    while(g_result < callCount)
    {
        rc = iPodPlayerTestWait(&g_cond, &g_mutex, IPOD_UNIT_WAIT_RETRY_MAX);
        if((rc != IPOD_PLAYER_OK) && (rc != IPOD_PLAYER_ERR_NOT_APPLICABLE))
        {
            break;
        }
        else if(rc == IPOD_PLAYER_ERR_NOT_APPLICABLE)
        {
            if(g_result >= callCount)
            {
                printf("WARN: some functions are not applicable in this Apple device.\n");
            }
            rc = IPOD_PLAYER_OK;
        }
        
        if(i < IPOD_UNIT_WAIT_RETRY_MAX)
        {
            i++;
        }
        else
        {
            /* Result was not able to receive */
            rc = IPOD_PLAYER_ERROR;
            break;
        }
    }
    
    /* Set the result */
    *callResult = g_result;
    
    /* Clear the result count */
    g_result = 0;
    g_error = IPOD_PLAYER_OK;
    
    return rc;
}

void iPodPlayerTestSetTable(IPOD_PLAYER_REGISTER_CB_TABLE *table)
{
    if(table == NULL)
    {
        return;
    }
    
    table->cbNotifyConnectionStatus = app_cb_notifyConnectionStatus;
    table->cbNotifyPlaybackStatus = app_cb_notifyPlaybackStatus;
    table->cbNotifyDBEntries = app_cb_notifyGetDBEntries;
    table->cbNotifyOpenApp = app_cb_notifyOpenApp;
    table->cbNotifyCloseApp = app_cb_notifyCloseApp;
    table->cbNotifyReceiveFromApp = app_cb_notifyReceiveFromApp;
    table->cbNotifyCoverartData = app_cb_notifyGetCoverart;
    table->cbPlayResult = app_cb_play;
    table->cbPlayCurrentSelectionResult = app_cb_playCurrentSelection;
    table->cbPauseResult = app_cb_pause;
    table->cbNextTrackResult = app_cb_trackup;
    table->cbSetModeResult = app_cb_modechange;
    table->cbPlayTrackResult = app_cb_playtrack;
    table->cbReleaseResult = app_cb_release;
    table->cbSetTrackInfoNotificationResult = app_cb_setTrackInfoNotification;
    table->cbGetCoverartInfoResult = app_cb_getCoverartInfo;
    table->cbGetCoverartResult           = app_cb_getCoverart;
    table->cbGetRepeatStatusResult = app_cb_getRepeatStatus;
    table->cbGetShuffleStatusResult = app_cb_getShuffleStatus;
    table->cbGetPlaySpeedResult =  app_cb_getPlaySpeed;
    table->cbGetTrackTotalCountResult = app_cb_getTrackTotalCount;
    table->cbGetDevicePropertyResult = app_cb_getDeviceProperty;
    table->cbGetTrackInfoResult = app_cb_getTrackInfo;
    table->cbGetDBCountResult = app_cb_getDBCount;
    table->cbSelectDBEntryResult = app_cb_selectDBEntry;
    table->cbGetDBEntriesResult = app_cb_GetDBEntries;
    table->cbCancelResult = app_cb_cancel;
    table->cbClearSelectionResult = app_cb_clearSelection;
    table->cbSelectAVResult = app_cb_selectAV;
    table->cbOpenSongTagResult = app_cb_openTagFile;
    table->cbCloseSongTagResult = app_cb_closeTagFile;
    table->cbSongTagResult = app_cb_songTag;
    table->cbSetAudioModeResult = app_cb_setAudioMode;
    
    return;
}


S32 iPodPlayerTestPrepare(void)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_REGISTER_CB_TABLE table;
    IPOD_PLAYER_AUDIO_SETTING audios = {IPOD_PLAYER_SOUND_MODE_ON, IPOD_PLAYER_STATE_ADJUST_ENABLE};
    U32 callCount = 0;
    U32 callResult = 0;
    U32 mask =  IPOD_PLAYER_DEVICE_MASK_NAME | IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION | IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER | 
        IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE | IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE | IPOD_PLAYER_DEVICE_MASK_EVENT |
            IPOD_PLAYER_DEVICE_MASK_FILE_SPACE | IPOD_PLAYER_DEVICE_MASK_FORMAT | IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT | 
                IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT;
    U32 authCount = 0;
    memset(&table, 0, sizeof(table));
    iPodPlayerTestSetTable(&table);
    
    rc = iPodPlayerInit(IPOD_PLAYER_USE_DEVICE_USB , &table);
    if(rc == IPOD_PLAYER_OK)
    {
        printf("Wait for connection and authentication iPod\n");
        for(authCount = 0; authCount < 20; authCount++)
        {
            if((g_connectStatus.deviceStatus == 1) && (g_connectStatus.authStatus == 2))
            {
                rc = IPOD_PLAYER_OK;
                break;
            }
            else
            {
                sleep(1);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodPlayerSetAudioMode(g_devID, audios);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerSetMode(g_devID, IPOD_PLAYER_MODE_REMOTE_CONTROL);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerGetDeviceProperty(g_devID, mask);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerGetRepeatStatus (g_devID);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerGetShuffleStatus (g_devID);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerGetPlaySpeed (g_devID);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerTestWaitCB(callCount, &callResult);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            U32 i = 0;
            printf("###################### Connected iPod ############################\n");
            printf("##  Device Name: %s\t\t\t\t\t##\n", g_property.name);
            printf("##  Software Version: %d,%d%d\t\t\t\t\t##\n", g_property.softVer.majorVer, g_property.softVer.minorVer, g_property.softVer.revision);
            printf("##  Serial Name: %s \t\t\t\t\t##\n", g_property.serial);
            printf("##  Supported Mask: 0x%x\t\t\t\t\t##\n", g_property.supportedFeatureMask);
            printf("##  Coverart Format:\t\t\t\t\t\t##\n");
            for(i = 0; i < g_property.formatCount; i++)
            {
                printf("##   ID: %d, Width: %d, Height: %d\t\t\t\t##\n", g_property.format[i].formatId, g_property.format[i].imageWidth, g_property.format[i].imageHeight);
            }
            printf("##  Repeat Status: %d\t\t\t\t\t\t##\n", g_repeat);
            printf("##  Shuffle Status: %d \t\t\t\t\t\t##\n", g_shuffle);
            printf("##  Speed Status: %d\t\t\t\t\t\t##\n", g_speed);
            printf("##################################################################\n");
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        printf("TestPrepare Error!! CallCount: %d, CallResult: %d, rc: %d\n", callCount, callResult, rc);
    }
    
    return rc;
}

S32 iPodPlayerTestPlay(U32 id)
{
    S32 rc = 0;
    U32 callCount = 0;
    U32 callResult = 0;
    
    rc = iPodPlayerPlay(id);
    if(rc == IPOD_PLAYER_OK)
    {
        callCount++;
        rc = iPodPlayerTestWaitCB(callCount, &callResult);
        if(rc == IPOD_PLAYER_OK)
        {
            sleep(3);
        }
        else
        {
            printf("iPodPlayer failed. Result of play is %d !!\n", rc);
        }
    }
    else
    {
        printf("iPodPlayer failed. Play request is %d\n", rc);
    }
    
    return rc;
}

S32 iPodPlayerTestListed(U32 id, IPOD_PLAYER_DB_TYPE type)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 callCount = 0;
    U32 callResult = 0;
    
    rc = iPodPlayerClearSelection(id, IPOD_PLAYER_DB_TYPE_ALL);
    if(rc == IPOD_PLAYER_OK)
    {
        callCount++;
        rc = iPodPlayerTestWaitCB(callCount, &callResult);
        if(rc == IPOD_PLAYER_OK)
        {
            callCount = 0;
            rc = iPodPlayerGetDBEntries(id, type, 0, -1);
            if(rc == IPOD_PLAYER_OK)
            {
                callCount++;
                rc = iPodPlayerTestWaitCB(callCount, &callResult);
                if(rc != IPOD_PLAYER_OK)
                {
                    printf("iPodPlayer failed. Result of get db entries is %d !! \n", rc);
                }
            }
            else
            {
                printf("iPodPlayer failed. Get db entries is %d !! \n", rc);
            }
        }
        else
        {
            printf("iPodPlayer failed. Result of clear selection is %d !! \n", rc);
        }
    }
    else
    {
        printf("iPodPlayer failed. Clear selection is %d !! \n", rc);
    }
    
    return rc;
}

S32 iPodPlayerTestToggleRepeat(U32 id)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 callCount = 0;
    U32 callResult = 0;
    U32 i = 0;
    
    for(i = 0; i < 2; i++)
    {
        callCount = 0;
        callResult = 0;
        rc = iPodPlayerPause(id);
        if(rc == IPOD_PLAYER_OK)
        {
            callCount++;
            rc = iPodPlayerTestWaitCB(callCount, &callResult);
            if(rc == IPOD_PLAYER_OK)
            {
                sleep(3);
                rc = iPodPlayerPlay(id);
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = iPodPlayerTestWaitCB(callCount, &callResult);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        sleep(3);
                    }
                    else
                    {
                        printf("iPodPlayer failed. Result of toggle play is %d !! \n", rc);
                        break;
                    }
                }
                else
                {
                    printf("iPodPlayer failed. toggle play is %d !! \n", rc);
                    break;
                }
            }
            else
            {
                printf("iPodPlayer failed. Result of toggle pause is %d !! \n", rc);
                break;
            }
        }
        else
        {
            printf("iPodPlayer failed. toggle pause is %d !! \n", rc);
            break;
        }
    }
    
    rc = iPodPlayerPause(id);
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerTestWaitCB(callCount, &callResult);
        if(rc != IPOD_PLAYER_OK)
        {
            printf("iPodPlayer failed. Result of toggle pause is %d !! \n", rc);
        }
    }

    
    return rc;
}

int main()
{
    S32 rc = 0;
    U8 count = 0;
    
    g_waitMask = 0;
    iPodPlayerTestWaitInit(&g_cond, &g_mutex);
    
    /* Call iPod Player Initialize API */
    do
    {
        switch(count)
        {
        case 0:
            rc = iPodPlayerTestPrepare();
            break;
        case 1:
            rc = iPodPlayerTestPlay(g_devID);
            break;
        case 2:
            rc = iPodPlayerTestListed(g_devID, IPOD_PLAYER_DB_TYPE_TRACK);
            break;
        case 3:
            rc = iPodPlayerTestToggleRepeat(g_devID);
            break;
        case 4:
            sleep(5);
            break;
            
        default:
            break;
        }
        count++;
    } while((count < 7) && (rc == IPOD_PLAYER_OK));
    
    iPodPlayerTestWaitDeinit(&g_cond, &g_mutex);
    iPodPlayerDeinit();
    return rc;
}


