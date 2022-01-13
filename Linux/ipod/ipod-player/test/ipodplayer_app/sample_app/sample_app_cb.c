#include <adit_typedef.h>
#include "iPodPlayerDef.h"
#include "iPodPlayerCB.h"
#include "iPodPlayerDebug.h"
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <string.h>
#include <pthread_adit.h>
#include <time.h>

IMPORT U32 g_devID;
IMPORT IPOD_PLAYER_CONNECTION_STATUS g_connectStatus;
IMPORT IPOD_PLAYER_PLAYBACK_STATUS g_playbackStatus;
IMPORT IPOD_PLAYER_DEVICE_PROPERTY g_property;
IMPORT IPOD_PLAYER_REPEAT_STATUS g_repeat;
IMPORT IPOD_PLAYER_SHUFFLE_STATUS g_shuffle;
IMPORT IPOD_PLAYER_PLAYING_SPEED g_speed;
IMPORT IPOD_PLAYER_TRACK_INFO g_info;
IMPORT U32 g_coverartCount;
IMPORT U32 g_coverartTime[256];
IMPORT U32 g_result;
IMPORT S32 g_error;
IMPORT U32 g_appID;
IMPORT pthread_cond_t g_cond;
IMPORT pthread_mutex_t g_mutex;
IMPORT U32 g_waitMask;

S32 iPodPlayerTestWaitInit(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if((cond == NULL) || (mutex == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = pthread_cond_init(cond, NULL);
    if(rc == 0)
    {
        rc = pthread_mutex_init(mutex, NULL);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            pthread_cond_destroy(cond);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

void iPodPlayerTestWaitDeinit(pthread_cond_t *cond, pthread_mutex_t *mutex)
{
    if(mutex != NULL)
    {
        pthread_mutex_destroy(mutex);
    }
    
    if(cond != NULL)
    {
        pthread_cond_destroy(cond);
    }
    
    return;
}

S32 iPodPlayerTestWait(pthread_cond_t *cond, pthread_mutex_t *mutex, U32 sec)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct timespec waitTime;
    
    if((cond == NULL) || (mutex == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&waitTime, 0, sizeof(waitTime));
    
    waitTime.tv_sec = time(NULL) + sec;
    waitTime.tv_nsec = 0;
    pthread_mutex_lock(mutex);
    
    if(g_waitMask != 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    
    while(g_waitMask == 0)
    {
        rc = pthread_cond_timedwait(cond, mutex, &waitTime);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        else if(rc == ETIMEDOUT)
        {
            rc = IPOD_PLAYER_ERR_TMOUT;
            break;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
            break;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = g_error;
    }
    
    g_result++;
    if(g_waitMask > 0)
    {
        g_waitMask--;
    }
    
    pthread_mutex_unlock(mutex);
    
    return rc;
}

S32 iPodPlayerTestSig(pthread_cond_t *cond, pthread_mutex_t *mutex, S32 result)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if((cond == NULL) || (mutex == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    pthread_mutex_lock(mutex);
    g_waitMask++;
    g_error = result;
    rc = pthread_cond_signal(cond);
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    pthread_mutex_unlock(mutex);
    
    return rc;
}

void app_cb_notifyConnectionStatus(U32 devID, IPOD_PLAYER_CONNECTION_STATUS *status)
{
    if(status == NULL)
    {
        return;
    }
    printf("devID=%d Status Change! Connection=%d, Authentication = %d, Power = %d, iapType = %d\n", devID, status->deviceStatus, status->authStatus, status->powerStatus, status->iapType);
    g_connectStatus = *status;
    g_devID = devID;
}

void app_cb_notifyPlaybackStatus(U32 devID, IPOD_PLAYER_PLAYBACK_STATUS *status)
{
    devID = devID;
    
    if(status == NULL)
    {
        return;
    }
    
    g_playbackStatus = *status;
    memcpy(&g_playbackStatus, status, sizeof(g_playbackStatus));
}



void app_cb_notifyGetCoverart(U32 devID, U32 trackIndex, U32 coverTime, IPOD_PLAYER_COVERART_HEADER *header, U32 size, U8 *data)
{
    devID = devID;
    trackIndex = trackIndex;
    coverTime = coverTime;
    header = header;
    size = size;
    data = data;
    /* Nothing do */
    return;
}

void app_cb_notifyOpenApp(U32 devID, U32 appID)
{
    printf("app_cb_notifyOpenApp START !! \n");
    printf("Device ID = %d\n", devID);
    printf("Application ID = %d \n", appID);
    
    g_appID = appID;
    
    return;

}

void app_cb_notifyCloseApp(U32 devID, U32 appID)
{
    printf("app_cb_notifyCloseApp START !! \n");
    printf("Device ID = %d\n", devID);
    printf("Application ID = %d \n", appID);
    
    if(g_appID == appID)
    {
        g_appID = 0;
    }
    
    return;
    
}

void app_cb_notifyReceiveFromApp(U32 devID, U32 appID, U32 dataSize, U8 *data)
{
    printf("app_cb_notifyReceiveFromApp START !! \n");
    printf("Device ID = %d\n", devID);
    printf("Application ID = %d dataSize = %d\n", appID, dataSize);
    printf("Data = %s\n", data);
    return;
    
}


void app_cb_notifyGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 count, IPOD_PLAYER_ENTRY_LIST *entryList)
{
    U32 i = 0;
    devID = devID;
    if(entryList != NULL)
    {
        printf("devID=%d, Type = %d count = %d\n", devID, type, count);
        
        for(i = 0; i < count; i++)
        {
            printf("trackIndex=%d: Name = %s\n", entryList[i].trackIndex, entryList[i].name);
        }
    }
    
    return;
    
}

void app_cb_trackup(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
}

void app_cb_modechange(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
}

void app_cb_getDeviceProperty(U32 devID, S32 result, IPOD_PLAYER_DEVICE_PROPERTY *property)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    g_property = *property;
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
}

void app_cb_getRepeatStatus(U32 devID, S32 result, IPOD_PLAYER_REPEAT_STATUS status)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    g_repeat = status;
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

}

void app_cb_getShuffleStatus(U32 devID, S32 result, IPOD_PLAYER_SHUFFLE_STATUS status)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    g_shuffle = status;
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
}

void app_cb_getPlaySpeed(U32 devID, S32 result, IPOD_PLAYER_PLAYING_SPEED speed)
{
    devID = devID;
    g_speed = speed;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
}

void app_cb_getCoverartInfo(U32 devID, S32 result, U32 timeCount, U32 *coverTime)
{
    devID = devID;
    
    printf("%s result = %d\n", __func__, result);
    g_coverartCount = timeCount;
    memcpy(g_coverartTime, coverTime, (sizeof(g_coverartTime) * g_coverartCount));
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;
    
}

void app_cb_getCoverart(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    if(g_error == IPOD_PLAYER_OK)
    {
        g_result++;
        g_error = result;
    }
    
    return;
    
}

void app_cb_getTrackInfo(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U64 trackID, IPOD_PLAYER_TRACK_INFO *info)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    if(info != NULL)
    {
        g_info = *info;
        printf("Result = %d\n", result);
        printf("Type = %d\n", type);
        printf("Index = %lld\n", trackID);
        printf("TrackMask = %x\n", info->trackInfoMask);
        
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME)
        {
            printf("TrackName = %s\n", info->trackName);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME)
        {
            printf("AlbumName = %s\n", info->albumName);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME)
        {
            printf("ArtistName = %s\n", info->artistName);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME)
        {
            printf("Podcast = %s\n", info->podcastName);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION) == IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION)
        {
            printf("Description = %s\n", info->description);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_LYRIC) == IPOD_PLAYER_TRACK_INFO_MASK_LYRIC)
        {
            printf("Lyric = %s\n", info->lyric);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_GENRE) == IPOD_PLAYER_TRACK_INFO_MASK_GENRE)
        {
            printf("Genre = %s\n", info->genre);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER) == IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER)
        {
            printf("Composer = %s\n", info->composer);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE) == IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE)
        {
            printf("ReleaseDate = \n");
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) == IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY)
        {
            printf("Capability = %d\n", info->capa);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_UID) == IPOD_PLAYER_TRACK_INFO_MASK_UID)
        {
            printf("UID = %lld\n", info->uID);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH) == IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH)
        {
            printf("Track Length = %d\n", info->length);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT) == IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT)
        {
            printf("Chapter Count = %d\n", info->chapterCount);
        }
        if((info->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND) == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND)
        {
            printf("TrackKind = %d\n", info->trackKind);
        }
        
    }
    else
    {
        printf("info is NULL!!\n");
    }
    
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    return;
    
}

void app_cb_selectDBEntry(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

}

void app_cb_GetDBEntries(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
}

void app_cb_cancel(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
}
    
void app_cb_clearSelection(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
}


void app_cb_openTagFile(U32 devID, S32 result, U32 handle)
{
    devID = devID;
    handle = handle;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;
    
}

void app_cb_closeTagFile(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

    return;
    
}

void app_cb_songTag(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;

}

void app_cb_setAudioMode(U32 devID, S32 result)
{
    devID = devID;
    if(result == IPOD_PLAYER_OK)
    {
        printf("%s result = %d\n", __func__, result);
        iPodPlayerTestSig(&g_cond, &g_mutex, result);
    }
    else
    {
        printf("%s :warning iPodPlayerSetAudioMode fail(result = %d)\n", __func__, result);
        iPodPlayerTestSig(&g_cond, &g_mutex, IPOD_PLAYER_OK);
    }
    
    return;
    
}

void app_cb_getDBCount(U32 devID, S32 result, U32 num)
{
    devID = devID;
    num = num;
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    printf("%s result = %d\n", __func__, result);

    
    return;
    
}
void app_cb_setTrackInfoNotification(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

    return;

}
void app_cb_selectAV(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;
    
}
void app_cb_getTrackTotalCount(U32 devID, S32 result, IPOD_PLAYER_TRACK_TYPE type, U32 count)
{
    devID = devID;
    type = type;
    count = count;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

    return;
    
}

void app_cb_play(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;
    
}

void app_cb_playCurrentSelection(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;
    
}

void app_cb_pause(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);
    
    return;
    
}

void app_cb_playtrack(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

    return;
    
}

void app_cb_release(U32 devID, S32 result)
{
    devID = devID;
    printf("%s result = %d\n", __func__, result);
    iPodPlayerTestSig(&g_cond, &g_mutex, result);

    return;
    
}
