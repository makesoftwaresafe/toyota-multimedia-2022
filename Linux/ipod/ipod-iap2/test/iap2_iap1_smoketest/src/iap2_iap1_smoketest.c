/*
 * iap2_smoketest.c
 */

/* **********************  includes  ********************** */
#include "iap2_iap1_smoketest.h"
#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include "iap2_dlt_log.h"


#include "iap2_external_accessory_protocol_session.h"


/* **********************  defines   ********************** */


/* **********************  locals    ********************** */

/* test functions */
LOCAL S32 iap2TestStartMediaLibInfo(iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibInfo(iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartMediaLibUpdate(iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibUpdate(iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartNowPlayingUpdate(iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopNowPlayingUpdate(iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartPlayMediaLibraryItem(iAP2Device_t* iap2Device, U16 numSongsToPlay);
LOCAL S32 iap2StartTest(iAP2Device_t* iap2Device);
#if IAP2_GST_AUDIO_STREAM
LOCAL S32 iap2TestStartUSBDeviceModeAudio(iAP2Device_t* iap2Device);
#endif

/* application thread */
LOCAL void iap2AppThread(void* exinf);
/* polling thread */
LOCAL void iap2PollThread(void* exinf);

/* mq handle function */
LOCAL S32 iap2HdlPollMqEvent(S32 mqFD);

S32 g_rc = IAP2_OK;
BOOL g_endPoll = FALSE;
BOOL g_EAPtesting = FALSE;
BOOL g_leaveGstWhile = FALSE;


iap2FileXferBuf g_CoverArtBuf;

BOOL GStreamer_test = FALSE;
U32 CurrentSongTotalDuration = 0;
iap2UserConfig_t g_iap2UserConfig;
iap2TestAppleDevice_t g_iap2TestDevice;
IMPORT BOOL  g_iap1Device ;
IMPORT sem_t g_iap1semlock ;

IMPORT void iap1AppThread(void );

/* **********************  callbacks ********************** */

S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary to avoid compiler warnings */
    context = context;

    switch(dState)
    {
        case iAP2NotConnected :
            g_iap2TestDevice.testDeviceState = iAP2NotConnected;
            printf("\t %u ms %p  Device state : iAP2NotConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2TransportConnected :
            g_iap2TestDevice.testDeviceState = iAP2TransportConnected;
            printf("\t %u ms %p  Device state : iAP2TransportConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkConnected:
            g_iap2TestDevice.testDeviceState = iAP2LinkConnected;
            printf("\t %u ms %p  Device state : iAP2LinkConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2AuthenticationPassed :
            g_iap2TestDevice.testDeviceState = iAP2AuthenticationPassed;
            printf("\t %u ms %p  Device state : iAP2AuthenticationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2IdentificationPassed :
            g_iap2TestDevice.testDeviceState = iAP2IdentificationPassed;
            printf("\t %u ms %p  Device state : iAP2IdentificationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2DeviceReady:
            g_iap2TestDevice.testDeviceState = iAP2DeviceReady;
            printf("\t %u ms %p  Device state : iAP2DeviceReady  \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkiAP1DeviceDetected:
            g_iap2TestDevice.testDeviceState = iAP2LinkiAP1DeviceDetected;
            printf("\t %u ms %p  Device state : iAP2LinkiAP1DeviceDetected\n", iap2CurrTimeMs(), iap2Device);
            break;
        default:
            g_iap2TestDevice.testDeviceState = iAP2ComError;
            printf("\t %u ms %p  Device state : unknown %d \n", iap2CurrTimeMs(), iap2Device, dState);
            rc = IAP2_CTL_ERROR;
            break;
    }

    return rc;
}

S32 iap2AuthenticationSucceeded_CB(iAP2Device_t* iap2Device, iAP2AuthenticationSucceededParameter* authParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context         = context;
    authParameter   = authParameter;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AuthenticationSucceeded_CB called ");
    printf("\t %u ms %p  iap2AuthenticationSucceeded_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2AuthenticationFailed_CB(iAP2Device_t* iap2Device, iAP2AuthenticationFailedParameter* authParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    authParameter = authParameter;
    context = context;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AuthenticationFailed_CB called");
    printf("\t %u ms %p  iap2AuthenticationFailed_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2IdentificationAccepted_CB(iAP2Device_t* iap2Device, iAP2IdentificationAcceptedParameter* idParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    idParameter = idParameter;
    context = context;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2IdentificationAccepted_CB called");
    printf("\t %u ms %p  iap2IdentificationAccepted_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2IdentificationRejected_CB(iAP2Device_t* iap2Device, iAP2IdentificationRejectedParameter* idParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    idParameter = idParameter;
    context = context;
    iap2Device = iap2Device;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2IdentificationRejected_CB called");
    printf("\t %u ms %p  iap2IdentificationRejected_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2PowerUpdate_CB(iAP2Device_t* iap2Device, iAP2PowerUpdateParameter* powerupdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2PowerUpdate_CB called");

    if (powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory_count != 0)
    {
        printf("\t %u ms %p  iap2PowerUpdate_CB CurrentDrawn: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2AccessoryPowerMode_count != 0)
    {
        printf("\t %u ms %p  iap2PowerUpdate_CB AccessoryPowerMode: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2AccessoryPowerMode));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count != 0)
    {
        printf("\t %u ms %p  iap2PowerUpdate_CB BatteryWillCharge: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent));
        rc = IAP2_OK;
    }

    return rc;
}

S32 iap2MediaLibraryInfo_CB(iAP2Device_t* iap2Device, iAP2MediaLibraryInformationParameter* MediaLibraryInfoParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context =context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2MediaLibraryInfo_CB called");
    printf("\t %u ms %p  iap2MediaLibraryInfo_CB called \n", iap2CurrTimeMs(), iap2Device);

    g_iap2TestDevice.testMediaLibInfoID = calloc(1, strlen((const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier))+1);
    if(g_iap2TestDevice.testMediaLibInfoID != NULL)
    {
        memcpy(g_iap2TestDevice.testMediaLibInfoID, *(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier),
                                    strlen((const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier))+1);
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
    }
    iap2SetTestState(MEDIA_LIB_INFO_RECV, TRUE);

    return rc;
}

S32 iap2MediaLibraryUpdates_CB(iAP2Device_t* iap2Device, iAP2MediaLibraryUpdateParameter* MediaLibraryUpdateParameter, void* context)
{
    S32 rc = IAP2_OK;
    U16 i = 0;
    size_t size = 0;
    /* temporary fix for compiler warning */
    context =context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2MediaLibraryUpdates_CB called");
    printf("\t %u ms %p  iap2MediaLibraryUpdates_CB called \n", iap2CurrTimeMs(), iap2Device);

    size = strlen((char*)(g_iap2TestDevice.testMediaLibInfoID));

    /* MediaLibraryUpdate for same MediaLibrary */
    if(strncmp((char*)g_iap2TestDevice.testMediaLibInfoID,
               (char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier), size) == 0)
    {
        /* get percentage completion for current set of MediaLibraryUpdates */
        if(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0)
        {
            g_iap2TestDevice.testMediaLibUpdateProgress = *(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress);
            printf("\n MediaLibUpdateProgress = %d / 100 \n", g_iap2TestDevice.testMediaLibUpdateProgress);
        }

        if(MediaLibraryUpdateParameter->iAP2MediaItem_count > 0)
        {
            printf("\n Number of songs received from Apple device: %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count);

            if(g_iap2TestDevice.testMediaItem == NULL)
            {
                g_iap2TestDevice.testMediaItemCnt = MediaLibraryUpdateParameter->iAP2MediaItem_count;
                g_iap2TestDevice.testMediaItem = calloc(g_iap2TestDevice.testMediaItemCnt, sizeof(iAP2MediaItem));
                if(g_iap2TestDevice.testMediaItem != NULL)
                {
                    for(i=0; ( (i < g_iap2TestDevice.testMediaItemCnt) && (rc == IAP2_OK) ); i++)
                    {
                        if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
                        {
                            g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier_count++;
                            g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier = calloc(1,sizeof(U64));

                            if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier != NULL)
                            {
                                memcpy(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier,
                                   MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier,
                                   sizeof(U64));
                            }
                            else
                            {
                                rc = IAP2_ERR_NO_MEM;
                            }
                        }

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0) && (rc == IAP2_OK) )
                        {
                            g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle_count++;
                            g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle = (U8**)calloc(1,sizeof(U8*));

                            if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle != NULL)
                            {
                                size = strlen((char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle));
                                *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle) = calloc(size,sizeof(U8));
                                if(*(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle) != NULL)
                                {
                                    strncpy((char*)*(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle),
                                            (char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle),
                                            size);
                                }
                                else
                                {
                                    rc = IAP2_ERR_NO_MEM;
                                }
                            }
                            else
                            {
                                rc = IAP2_ERR_NO_MEM;
                            }
                        }

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0) && (rc == IAP2_OK) )
                        {
                            g_iap2TestDevice.testMediaItem[i].iAP2MediaItemGenre_count++;
                            g_iap2TestDevice.testMediaItem[i].iAP2MediaItemGenre = (U8**)calloc(1,sizeof(U8*));
                            if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemGenre != NULL)
                            {
                                size = strlen((char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre));
                                *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemGenre) = calloc(size,sizeof(U8));
                                if(*(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemGenre) != NULL)
                                {
                                    strncpy((char*)*(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemGenre),
                                            (char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre),
                                            size);
                                }
                                else
                                {
                                    rc = IAP2_ERR_NO_MEM;
                                }
                            }
                            else
                            {
                                rc = IAP2_ERR_NO_MEM;
                            }
                        }
                    }
                    if(rc == IAP2_OK)
                    {
                        printf("\t title of first song:      %s  \n", *(g_iap2TestDevice.testMediaItem[0].iAP2MediaItemTitle));
                        i = g_iap2TestDevice.testMediaItemCnt - 1;
                        printf("\t title of last song:       %s  \n", *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle));
                        printf("\n Number of songs in Apple device: %d \n", g_iap2TestDevice.testMediaItemCnt);
                    }
                }
                else
                {
                    printf(" %u ms:  Error g_MediaItem.test_MediaItem is NULL \n", iap2CurrTimeMs());
                }
            }
            else if( (g_iap2TestDevice.testMediaLibInfoID != NULL)
                     &&(strncmp((char*)g_iap2TestDevice.testMediaLibInfoID,
                                (char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier), size) == 0) )
            {
                printf(" %u ms:  iap2MediaLibraryUpdates_CB received for same MediaLibraryID \n", iap2CurrTimeMs());

                size = ((g_iap2TestDevice.testMediaItemCnt + MediaLibraryUpdateParameter->iAP2MediaItem_count) * sizeof(iAP2MediaItem));
                g_iap2TestDevice.testMediaItem = realloc(g_iap2TestDevice.testMediaItem, size);
                memset(g_iap2TestDevice.testMediaItem+g_iap2TestDevice.testMediaItemCnt, 0, (MediaLibraryUpdateParameter->iAP2MediaItem_count) * sizeof(iAP2MediaItem));
                if(g_iap2TestDevice.testMediaItem != NULL)
                {
                    /* add MediaItem information to current existing one */
                    U16 t_index = g_iap2TestDevice.testMediaItemCnt;
                    for(i = 0; ( (i < MediaLibraryUpdateParameter->iAP2MediaItem_count) && (rc == IAP2_OK) ); i++)
                    {
                        if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
                        {
                            g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPersistentIdentifier_count++;
                            g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPersistentIdentifier = calloc(1,sizeof(U64));
                            if(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPersistentIdentifier != NULL)
                            {
                                memcpy(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPersistentIdentifier,
                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier,
                                       sizeof(U64));
                            }
                            else
                            {
                                rc = IAP2_ERR_NO_MEM;
                            }
                        }

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0) && (rc == IAP2_OK) )
                        {
                            g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemTitle_count++;
                            g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemTitle = (U8**)calloc(1,sizeof(U8*));
                            if(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemTitle != NULL)
                            {
                                size = strlen((char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle));
                                *(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemTitle) = calloc(size,sizeof(U8));
                                if(*(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemTitle) != NULL)
                                {
                                    strncpy((char*)*(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemTitle),
                                            (char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle),
                                            size);
                                }
                                else
                                {
                                    rc = IAP2_ERR_NO_MEM;
                                }
                            }
                            else
                            {
                                rc = IAP2_ERR_NO_MEM;
                            }
                        }

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0) && (rc == IAP2_OK) )
                        {
                            g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemGenre_count++;
                            g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemGenre = (U8**)calloc(1,sizeof(U8*));
                            if(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemGenre != NULL)
                            {
                                size = strlen((char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre));
                                *(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemGenre) = calloc(size,sizeof(U8));
                                if(*(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemGenre) != NULL)
                                {
                                    strncpy((char*)*(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemGenre),
                                            (char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre),
                                            size);
                                }
                                else
                                {
                                    rc = IAP2_ERR_NO_MEM;
                                }
                            }
                            else
                            {
                                rc = IAP2_ERR_NO_MEM;
                            }
                        }

                        t_index++;
                    }

                    /* just for printf */
                    t_index = g_iap2TestDevice.testMediaItemCnt;

                    g_iap2TestDevice.testMediaItemCnt += MediaLibraryUpdateParameter->iAP2MediaItem_count;

                    printf("\t title of first song:      %s  \n",
                            *(g_iap2TestDevice.testMediaItem[0].iAP2MediaItemTitle));
                    i = t_index - 1;
                    printf("\t title of prev-last song:  %s  \n",
                            *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle));
                    i = g_iap2TestDevice.testMediaItemCnt - 1;
                    printf("\t title of last song:       %s  \n",
                            *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle));

                    printf("\n Number of songs in Apple device: %d \n", g_iap2TestDevice.testMediaItemCnt);
                }
                else
                {
                    printf(" %u ms:  Error realloc g_MediaItem.test_MediaItem is NULL \n", iap2CurrTimeMs());
                }
            }
            else
            {
                printf(" %u ms:  Error g_MediaItem.test_MediaItem already allocated \n", iap2CurrTimeMs());
            }
        }
    }
    else
    {
        /* received MediaLibraryUpdate for a different MediaLibrary */
        printf("\n received MediaLibraryUpdate for a different MediaLibraryID \n");
        /* get percentage completion for current set of MediaLibraryUpdates */
        if(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0)
        {
            g_iap2TestDevice.testMediaLibUpdateProgress = *(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress);
            printf(" MediaLibUpdateProgress = %d / 100 \n", g_iap2TestDevice.testMediaLibUpdateProgress);
        }

        printf(" Number of songs received from Apple device: %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count);
    }
    iap2SetTestState(MEDIA_LIB_UPDATE_RECV, TRUE);
    return rc;
}

S32 iap2NowPlayingUpdate_CB(iAP2Device_t* iap2Device, iAP2NowPlayingUpdateParameter* NowPlayingUpdateParameter, void* context)
{
    S32 rc = IAP2_OK;
    S16 i, j;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2NowPlayingUpdate_CB called \n");
    printf("\t %u ms %p  iap2NowPlayingUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    for(i = 0;i < NowPlayingUpdateParameter->iAP2MediaItemAttributes_count;i++)
    {
        if( &(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i]) != NULL)
        {
            if(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle_count > 0)
            {
                printf("\t   now playing title:  %s \n", *(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle));
            }

            for(j = 0;j < NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPlaybackDurationInMilliseconds_count;j++)
            {
                if( &(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPlaybackDurationInMilliseconds[j]) != NULL)
                {
                    CurrentSongTotalDuration = NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPlaybackDurationInMilliseconds[j];
                    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "CurrentSongTotalDuration = %d", CurrentSongTotalDuration);
                }
            }
            for(j = 0;j < NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemAlbumTrackNumber_count;j++)
            {
                if( &(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemAlbumTrackNumber[j]) != NULL)
                {
                    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Track Changed Notification from Apple Device");
                }
            }
        }
    }
    for(i = 0;i < NowPlayingUpdateParameter->iAP2PlaybackAttributes_count;i++)
    {
        if( &(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i]) != NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "NowPlayingUpdateParameter->iAP2PlaybackAttributes[%d].iAP2PlaybackElapsedTimeInMilliseconds_count = %d, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds = %d",
                           i, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds_count, *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds) );
            for(j = 0;j < NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds_count;j++)
            {
                if( &(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j]) != NULL)
                {
                    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "CurrentSongTotalDuration = %d, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j] = %d", 
                            CurrentSongTotalDuration, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j]);
                }
            }

            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName_count > 0)
            {
                if( ((NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName) != NULL)
                    && (*(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName) != NULL) )
                {
                    printf("\t   playback app name %s \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName));
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode_count > 0)
            {
                if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode != NULL)
                {
                    printf("\t   shuffle mode:  %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode));
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode_count > 0)
            {
                if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode != NULL)
                {
                    printf("\t   repeat mode:   %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode));
                }
            }

        }
    }

    iap2SetTestState(NOW_PLAYING_UPDATE_RECV, TRUE);

    return rc;
}

S32 iap2FileTransferSetup_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferSetup_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if(iAP2FileXferSession->iAP2FileXferRxLen > 0)
        {
            printf("\t   artwork available. FileID: %d  FileSize: %llu  \n",
                    iAP2FileXferSession->iAP2FileTransferID, iAP2FileXferSession->iAP2FileXferRxLen);

            g_CoverArtBuf.Buffer = calloc(1,iAP2FileXferSession->iAP2FileXferRxLen);
            g_CoverArtBuf.CurPos = g_CoverArtBuf.Buffer;
            if(g_CoverArtBuf.Buffer != NULL)
            {
                g_CoverArtBuf.CurReceived = 0;
                g_CoverArtBuf.FileID = iAP2FileXferSession->iAP2FileTransferID;
                g_CoverArtBuf.FileSize = iAP2FileXferSession->iAP2FileXferRxLen;
                rc = IAP2_OK;
            }
            else
            {
                printf("\t   memory allocation failed. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
                rc = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            printf("\t   no artwork available. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferDataRcvd_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferDataRcvd_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_CoverArtBuf.CurPos != NULL)
            && (g_CoverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID) )
        {
            memcpy(g_CoverArtBuf.CurPos, iAP2FileXferSession->iAP2FileXferRxBuf, iAP2FileXferSession->iAP2FileXferRxLen);
            g_CoverArtBuf.CurPos += iAP2FileXferSession->iAP2FileXferRxLen;
            g_CoverArtBuf.CurReceived += iAP2FileXferSession->iAP2FileXferRxLen;

            iap2SetTestState(FILE_TRANSFER_DATA_RECV, TRUE);
            printf("\t   received artwork data for FileID: %d \n",iAP2FileXferSession->iAP2FileTransferID);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferSuccess_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;
    FILE *fp;
    char fileName[256];

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferSuccess_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_CoverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID)
            && (g_CoverArtBuf.CurReceived == g_CoverArtBuf.FileSize) )
        {
            memset(&fileName[0], 0, (sizeof(fileName)));
            sprintf(&fileName[0], "%s%d%s", "/opt/CoverArt", iAP2FileXferSession->iAP2FileTransferID, ".jpg");

            fp = fopen(&fileName[0], "w");
            if(fp != NULL)
            {
                fwrite(g_CoverArtBuf.Buffer, 1, g_CoverArtBuf.FileSize, fp);
                fclose(fp);

                printf("\t   File Transfer Success!  Please check %s \n", &fileName[0]);
            }

            if(g_CoverArtBuf.Buffer != NULL)
            {
                iap2TestFreePtr( (void**)&g_CoverArtBuf.Buffer);
                g_CoverArtBuf.CurPos = NULL;
                memset(&g_CoverArtBuf, 0, sizeof(iap2FileXferBuf));
            }

            iap2SetTestState(FILE_TRANSFER_SUCCESS_RECV, TRUE);
        }
        else
        {
            printf("\t   Did not receive all File Transfer data. \n");
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferFailure_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferFailure_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer failed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);

        if(g_CoverArtBuf.Buffer != NULL)
        {
            iap2TestFreePtr( (void**)&g_CoverArtBuf.Buffer);
            g_CoverArtBuf.CurPos = NULL;
            memset(&g_CoverArtBuf, 0, sizeof(iap2FileXferBuf));
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_FAILED_RECV, TRUE);

    return rc;
}

S32 iap2FileTransferCancel_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferCancel_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer canceled.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        if(g_CoverArtBuf.Buffer != NULL)
        {
            iap2TestFreePtr( (void**)&g_CoverArtBuf.Buffer);
            g_CoverArtBuf.CurPos = NULL;
            memset(&g_CoverArtBuf, 0, sizeof(iap2FileXferBuf));
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_CANCELED_RECV, TRUE);
    return rc;
}

S32 iap2FileTransferPause_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferPause_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer paused.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2FileTransferResume_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferResume_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer resumed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2StartExternalAccessoryProtocolSession_CB(iAP2Device_t* iap2Device, iAP2StartExternalAccessoryProtocolSessionParameter* theiAP2StartExternalAccessoryProtocolSessionParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    theiAP2StartExternalAccessoryProtocolSessionParameter = theiAP2StartExternalAccessoryProtocolSessionParameter;
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StartExternalAccessoryProtocolSession_CB called \n");
    printf("\t %p  iap2StartExternalAccessoryProtocolSession_CB called \n", iap2Device);

    iap2SetTestState(EAP_SESSION_START_RECV, TRUE);

    return rc;
}

S32 iap2StopExternalAccessoryProtocolSession_CB(iAP2Device_t* iap2Device, iAP2StopExternalAccessoryProtocolSessionParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    thisParameter = thisParameter;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StopExternalAccessoryProtocolSession_CB called \n");
    printf("\t %p  iap2StopExternalAccessoryProtocolSession_CB called \n", iap2Device);

    iap2SetTestState(EAP_SESSION_STOP_RECV, TRUE);

    return rc;
}

S32 iap2iOSAppDataReceived_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8* iAP2iOSAppDataRxd, U16 iAP2iOSAppDataLength, void* context)
{
    S32 rc = IAP2_OK;
    U16 i;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2iOSAppDataReceived_CB called \n");
    printf("\t %p  iap2iOSAppDataReceived_CB called. iAP2iOSAppIdentifier = %d, iAP2iOSAppDataLength = %d \n",
            iap2Device, iAP2iOSAppIdentifier, iAP2iOSAppDataLength);

    printf(" Following EAPSession Datagram received from device: \n");
    for(i = 0; i < iAP2iOSAppDataLength; i++)
    {
        if( (i % 10) == 0)
            printf("\n");
        printf(" 0x%.2X, ", iAP2iOSAppDataRxd[i]);
    }
    printf("\n");

    iap2SetTestState(EAP_SESSION_DATA_RECV, TRUE);

    return rc;
}

#if IAP2_GST_AUDIO_STREAM
S32 iap2USBDeviceModeAudioInformation_CB(iAP2Device_t* iap2Device, iAP2USBDeviceModeAudioInformationParameter* theiAP2USBDeviceModeAudioInformationParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2USBDeviceModeAudioInformation_CB called \n");
    printf("\t %p  iap2USBDeviceModeAudioInformation_CB called \n", iap2Device);

    g_iap2TestDevice.testGstSampleRate = *(theiAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate);
    if(g_iap2TestDevice.testGstState != IAP2_GSTREAMER_STATE_PLAYING)
    {
        iap2SleepMs(300);
        g_iap2TestDevice.testGstState = IAP2_GSTREAMER_STATE_PLAYING;
        sem_post(&g_iap2TestDevice.testGstSemaphore);
        printf("Sending PLAY Notification to gstreamer\n");
    }

    iap2SetTestState(USB_DEVICE_MODE_AUDIO_INFO_RECV, TRUE);

    return rc;
}
#endif

/* **********************  functions ********************** */



void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
}

void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks, BOOL iAP2EAPSupported)
{
    iap2CSCallbacks->iAP2AuthenticationFailed_cb                     = &iap2AuthenticationFailed_CB;
    iap2CSCallbacks->iAP2AuthenticationSucceeded_cb                 = &iap2AuthenticationSucceeded_CB;
    iap2CSCallbacks->iAP2RequestAuthenticationCertificate_cb        = NULL;
    iap2CSCallbacks->iAP2RequestAuthenticationChallengeResponse_cb  = NULL;
    iap2CSCallbacks->iAP2StartIdentification_cb                     = NULL;
    iap2CSCallbacks->iAP2IdentificationAccepted_cb                  = &iap2IdentificationAccepted_CB;
    iap2CSCallbacks->iAP2IdentificationRejected_cb                  = &iap2IdentificationRejected_CB;
    iap2CSCallbacks->iAP2BluetoothConnectionUpdate_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationCertificate_cb         = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationResponse_cb            = NULL;
    iap2CSCallbacks->iAP2DeviceInformationUpdate_cb                 = NULL;
    iap2CSCallbacks->iAP2DeviceLanguageUpdate_cb                    = NULL;
    iap2CSCallbacks->iAP2TelephonyCallStateInformation_cb           = NULL;
    iap2CSCallbacks->iAP2TelephonyUpdate_cb                         = NULL;
    iap2CSCallbacks->iAP2StartVehicleStatusUpdates_cb               = NULL;
    iap2CSCallbacks->iAP2StopVehicleStatusUpdates_cb                = NULL;
    iap2CSCallbacks->iAP2AssistiveTouchInformation_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceHIDReport_cb                         = NULL;
    iap2CSCallbacks->iAP2StartLocationInformation_cb                = NULL;
    iap2CSCallbacks->iAP2StopLocationInformation_cb                 = NULL;
    iap2CSCallbacks->iAP2MediaLibraryInformation_cb                 = &iap2MediaLibraryInfo_CB;
    iap2CSCallbacks->iAP2MediaLibraryUpdate_cb                      = &iap2MediaLibraryUpdates_CB;
    iap2CSCallbacks->iAP2NowPlayingUpdateParameter_cb               = &iap2NowPlayingUpdate_CB;
    iap2CSCallbacks->iAP2PowerUpdate_cb                             = &iap2PowerUpdate_CB;
#if IAP2_GST_AUDIO_STREAM
    iap2CSCallbacks->iAP2USBDeviceModeAudioInformation_cb           = &iap2USBDeviceModeAudioInformation_CB;
#endif
    iap2CSCallbacks->iAP2VoiceOverUpdate_cb                         = NULL;
    iap2CSCallbacks->iAP2WiFiInformation_cb                         = NULL;
    if(iAP2EAPSupported == TRUE)
    {
        iap2CSCallbacks->iAP2StartExternalAccessoryProtocolSession_cb  = &iap2StartExternalAccessoryProtocolSession_CB;
        iap2CSCallbacks->iAP2StopExternalAccessoryProtocolSession_cb   = &iap2StopExternalAccessoryProtocolSession_CB;
    }
}

void iap2InitEAPSessionCallbacks(iAP2EAPSessionCallbacks_t* iAP2EAPSessionCallbacks)
{
    iAP2EAPSessionCallbacks->iAP2iOSAppDataReceived_cb = &iap2iOSAppDataReceived_CB;
}

void iap2InitFileTransferCallbacks(iAP2FileTransferCallbacks_t* iAP2FileTransferCallbacks)
{
    iAP2FileTransferCallbacks->iAP2FileTransferSuccess_cb           = iap2FileTransferSuccess_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferFailure_cb           = iap2FileTransferFailure_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferCancel_cb            = iap2FileTransferCancel_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferPause_cb             = iap2FileTransferPause_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferResume_cb            = iap2FileTransferResume_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferDataRcvd_cb          = iap2FileTransferDataRcvd_CB;
    iAP2FileTransferCallbacks->iAP2FileTransferSetup_cb             = iap2FileTransferSetup_CB;
}


LOCAL S32 iap2HdlPollMqEvent(S32 mqFD)
{
    S32 rc = IAP2_CTL_ERROR;
    char recvBuf[TEST_MQ_MAX_SIZE + 1];

    /* receive message from AppThread */
    rc = iap2RecvMq(mqFD, &recvBuf[0], TEST_MQ_MAX_SIZE + 1);
    if(rc > 0)
    {
        if (strncmp(&recvBuf[0], TEST_MQ_CMD_STOP_POLL, strlen(TEST_MQ_CMD_STOP_POLL)) == 0)
        {
            printf("\t %u ms  iap2HdlPollMqEvent():  fd: %d leave poll thread \n", iap2CurrTimeMs(), mqFD);
            g_endPoll = TRUE;
            rc = IAP2_OK;
        }
        else
        {
            printf("\t iap2HdlPollMqEvent(): error: iap2HdlPollMqEvent[fd %d] buf: %s\n", mqFD, recvBuf);
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

/* Sending iAP2StartMediaLibraryInformation to device */
LOCAL S32 iap2TestStartMediaLibInfo(iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 retry_count = 0;
    iAP2StartMediaLibraryInformationParameter theiAP2StartMediaLibraryInformationParameter;

    memset(&theiAP2StartMediaLibraryInformationParameter, 0, sizeof(iAP2StartMediaLibraryInformationParameter));
    rc = iAP2StartMediaLibraryInformation(iap2Device, &theiAP2StartMediaLibraryInformationParameter);

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StartMediaLibraryInformationParameter(&theiAP2StartMediaLibraryInformationParameter); */

    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2TestStartMediaLibInfo(): failed. iAP2StartMediaLibraryInformation  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        /* wait for callback with requested information */
        while( (rc == IAP2_OK) && (iap2GetTestState(MEDIA_LIB_INFO_RECV) != TRUE)
               &&(iap2GetTestStateError() != TRUE) && (retry_count < 300) )
        {
            if(g_iap2TestDevice.testDeviceState == iAP2NotConnected)
            {
                rc = IAP2_DEV_NOT_CONNECTED;
                break;
            }
            iap2SleepMs(10);
            retry_count++;
        }
        /* received callback iap2MediaLibraryInfo_CB */
        if(iap2GetTestState(MEDIA_LIB_INFO_RECV) == TRUE)
        {
            rc = IAP2_OK;
        }
        else
        {
            printf(" %u ms:  iap2TestStartMediaLibInfo(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
            rc = IAP2_CTL_ERROR;
        }
    }

    if(rc != IAP2_OK)
    {
    	iap2SetTestStateError(TRUE);
    }

    return rc;
}
/* sending StopMediaLibraryInformation to device */
LOCAL S32 iap2TestStopMediaLibInfo(iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StopMediaLibraryInformationParameter StopMediaLibraryInformationParameter;

    memset(&StopMediaLibraryInformationParameter, 0, sizeof(iAP2StopMediaLibraryInformationParameter));
    rc = iAP2StopMediaLibraryInformation(iap2Device, &StopMediaLibraryInformationParameter);

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopMediaLibraryInformationParameter(&StopMediaLibraryInformationParameter); */
    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2TestStopMediaLibInfo(): failed. iAP2StopMediaLibraryInformation  rc = %d \n", iap2CurrTimeMs(), rc);
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* sending iAP2StartMediaLibraryUpdates to device */
LOCAL S32 iap2TestStartMediaLibUpdate(iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    U32 retry_count = 0;
    iAP2StartMediaLibraryUpdatesParameter theiAP2StartMediaLibraryUpdatesParameter;

    memset(&theiAP2StartMediaLibraryUpdatesParameter, 0, sizeof(iAP2StartMediaLibraryUpdatesParameter));

    if(g_iap2TestDevice.testMediaLibInfoID != NULL)
    {
        /* set MediaLibraryIdentifier */
        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier = calloc(1, sizeof(U8**));
        if(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier != NULL)
        {
            *(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier) = calloc(1, strlen((const char*)g_iap2TestDevice.testMediaLibInfoID));
            if(*(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier) != NULL)
            {
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier_count++;
                memcpy(*(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier), g_iap2TestDevice.testMediaLibInfoID, strlen((const char*)g_iap2TestDevice.testMediaLibInfoID));
                /* set MediaItem properties which you want to receive */
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties = calloc(1, sizeof(iAP2MediaItemProperties));
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties_count++;
                /* MediaItem property PersistentIdentifier must be received */
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count++;
                /* set which MediaItem property (title, artist, etc.) should receive */
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count++;

                /* get percentage completion for current set of MediaLibraryUpdates */
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUpdateProgress = calloc(1, sizeof(U8));
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUpdateProgress_count++;

                /* start MediaLibraryUpdates */
                rc = iAP2StartMediaLibraryUpdates(iap2Device, &theiAP2StartMediaLibraryUpdatesParameter);
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
        iAP2FreeiAP2StartMediaLibraryUpdatesParameter(&theiAP2StartMediaLibraryUpdatesParameter);
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2TestStartMediaLibUpdates(): failed. iAP2StartMediaLibraryUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        while( (rc == IAP2_OK) && (g_iap2TestDevice.testMediaLibUpdateProgress < 100)
                &&(iap2GetTestStateError() != TRUE) && (retry_count < 500) )
        {
            if(g_iap2TestDevice.testDeviceState == iAP2NotConnected)
            {
                rc = IAP2_DEV_NOT_CONNECTED;
                break;
            }
            iap2SleepMs(10);
            retry_count++;
            /* check if MediaLibraryUpdates callback was called and reset retry_count in case not all data were received */
            if( (g_iap2TestDevice.testMediaLibUpdateProgress < 100)
                 &&(iap2GetTestState(MEDIA_LIB_UPDATE_RECV) == TRUE) )
            {
                retry_count = 0;
                iap2SetTestState(MEDIA_LIB_UPDATE_RECV,FALSE);
            }
        }
        if(g_iap2TestDevice.testMediaLibUpdateProgress == 100)
        {
            rc = IAP2_OK;
            /* how to react in case of multiple callbacks ?
             *  - use testMediaLibUpdateProgress which indicates the progress of MediaLibraryUpdates */
        }
        else
        {
            printf(" %u ms:  iap2TestStartMediaLibUpdate(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
            rc = IAP2_CTL_ERROR;
        }
    }

    if(rc != IAP2_OK)
    {
    	iap2SetTestStateError(TRUE);
    }

    return rc;
}
/* sending iap2TestStopMediaLibUpdate to device */
LOCAL S32 iap2TestStopMediaLibUpdate(iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StopMediaLibraryUpdatesParameter StopMediaLibraryUpdatesParameter;

    memset(&StopMediaLibraryUpdatesParameter, 0, sizeof(iAP2StopMediaLibraryUpdatesParameter));

    StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier= calloc(1, sizeof(U8**));
    if(StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier != NULL)
    {
        *(StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier) = calloc(1, strlen((const char*)g_iap2TestDevice.testMediaLibInfoID) + 1);
        if(*(StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier) != NULL)
        {
            memcpy(*(StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier), g_iap2TestDevice.testMediaLibInfoID,
                    strlen((const char*)g_iap2TestDevice.testMediaLibInfoID+1));
            StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier_count++;
            rc = iAP2StopMediaLibraryUpdates(iap2Device, &StopMediaLibraryUpdatesParameter);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
    }
    iAP2FreeiAP2StopMediaLibraryUpdatesParameter(&StopMediaLibraryUpdatesParameter);

    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2TestStopMediaLibUpdate(): failed. iAP2StopMediaLibraryUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* Sending iAP2StartNowPlayingUpdates to device */
LOCAL S32 iap2TestStartNowPlayingUpdate(iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StartNowPlayingUpdatesParameter theiAP2StartNowPlayingUpdatesParameter;

    memset(&theiAP2StartNowPlayingUpdatesParameter, 0, sizeof(iAP2StartNowPlayingUpdatesParameter));

    /* set MediaItemAttributes properties which you want to receive */
    theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes = (iAP2MediaItemAttributes*)calloc(1, sizeof(iAP2MediaItemAttributes));
    if(theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes != NULL)
    {
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemTitle_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemPlaybackDurationInMilliseconds_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemAlbumTitle_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemArtist_count++;
        /* receive Artwork and use File Transfer Session */
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count++;

        /* enable information about shuffle mode and repeat mode */
        theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes = (iAP2StartNowPlayingPlaybackAttributes*)calloc(1, sizeof(iAP2StartNowPlayingPlaybackAttributes));
        if(theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes != NULL)
        {
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackAppName_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackAppBundleID_count++;
        }

        rc = iAP2StartNowPlayingUpdates(iap2Device, &theiAP2StartNowPlayingUpdatesParameter);
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
    }
    iAP2FreeiAP2StartNowPlayingUpdatesParameter(&theiAP2StartNowPlayingUpdatesParameter);

    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2StartNowPlayingUpdate(): failed. iAP2StartNowPlayingUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        /* no wait here.
         * we did not know if the Apple device is in play state. */
    }

    if(rc != IAP2_OK)
    {
    	iap2SetTestStateError(TRUE);
    }

    return rc;
}
/* Sending iap2TestStopNowPlayingUpdate to device */
LOCAL S32 iap2TestStopNowPlayingUpdate(iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StopNowPlayingUpdatesParameter theiAP2StopNowPlayingUpdatesParameter;

    memset(&theiAP2StopNowPlayingUpdatesParameter, 0, sizeof(iAP2StopNowPlayingUpdatesParameter));
    rc = iAP2StopNowPlayingUpdates(iap2Device, &theiAP2StopNowPlayingUpdatesParameter);

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopNowPlayingUpdatesParameter(&theiAP2StopNowPlayingUpdatesParameter); */

    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2TestStopNowPlayingUpdate(): failed. iAP2StopNowPlayingUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* Sending iAP2PlayMediaLibraryItems to device */
LOCAL S32 iap2TestStartPlayMediaLibraryItem(iAP2Device_t* iap2Device, U16 numSongsToPlay)
{
    int rc = IAP2_OK;
    U32 retry_count = 0;
    U16 tmpNumSongs = 0;
    U16 i = 0;
    U16 PersistendIdentifierCount = 0;
    U8* BlobDataLocation = NULL;
    iAP2PlayMediaLibraryItemsParameter theiAP2PlayMediaLibraryItemsParameter;

    memset(&theiAP2PlayMediaLibraryItemsParameter, 0, sizeof(iAP2PlayMediaLibraryItemsParameter));

    /* set number of songs to play */
    if(numSongsToPlay > g_iap2TestDevice.testMediaItemCnt)
    {
        tmpNumSongs = g_iap2TestDevice.testMediaItemCnt;
    }
    else
    {
        tmpNumSongs = numSongsToPlay;
    }
    /* get number of available persistent identifier */
    for(i = 0; i < tmpNumSongs; i++)
    {
        if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
        {
            PersistendIdentifierCount++;
        }
    }
    if(PersistendIdentifierCount > 0)
    {
        /* create array of ordered uint64 MediaItemPersistentIdentifiers */
        theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers = calloc(1, sizeof(iAP2Blob));
        if(theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if(rc == IAP2_OK)
        {
            theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers_count = 1;
            theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobData = calloc(PersistendIdentifierCount, sizeof(U64));
            if(theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobData == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if(rc == IAP2_OK)
        {
            theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobLength = PersistendIdentifierCount * sizeof(U64);
            BlobDataLocation = theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobData;

            /* add MediaItemPersistentIdentifier of the songs which should be played */
            for(i = 0; i < tmpNumSongs; i++)
            {
                if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier_count != 0)
                {
                    memcpy(BlobDataLocation, g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier, sizeof(U64) );
                    BlobDataLocation += sizeof(U64);
                }
            }
            /* set index of first item to start playback */
            theiAP2PlayMediaLibraryItemsParameter.ItemsStartingIndex_count = 1;
            theiAP2PlayMediaLibraryItemsParameter.ItemsStartingIndex = calloc(1, sizeof(U32));
            if(theiAP2PlayMediaLibraryItemsParameter.ItemsStartingIndex == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if(rc == IAP2_OK)
        {
            *theiAP2PlayMediaLibraryItemsParameter.ItemsStartingIndex = (U32)1;

            /* add MediaLibraryIdentifier */
            theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier = calloc(1, sizeof(U8**));
            if(theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if(rc == IAP2_OK)
        {
            *(theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier) = calloc(1, strlen((const char*)g_iap2TestDevice.testMediaLibInfoID)+1);
            if( *(theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier) == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if(rc == IAP2_OK)
        {
            memcpy(*(theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier),
                    g_iap2TestDevice.testMediaLibInfoID,
                    strlen((const char*)g_iap2TestDevice.testMediaLibInfoID));
            theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier_count++;

            /* start playback */
            rc = iAP2PlayMediaLibraryItems(iap2Device, &theiAP2PlayMediaLibraryItemsParameter);
        }

        iAP2FreeiAP2PlayMediaLibraryItemsParameter(&theiAP2PlayMediaLibraryItemsParameter);

        if(rc != IAP2_OK)
        {
            printf(" %u ms:  iap2TestStartPlayMediaLibraryItem(): failed. iAP2PlayMediaLibraryItems  rc = %d \n", iap2CurrTimeMs(), rc);
        }
        else
        {
            /* wait for callback with requested information */
            while( (rc == IAP2_OK) && (iap2GetTestState(NOW_PLAYING_UPDATE_RECV) != TRUE)
                    &&(iap2GetTestStateError() != TRUE) && (retry_count < 300) )
            {
                iap2SleepMs(10);
                retry_count++;
            }
            /* received callback iap2NowPlayingUpdate_CB */
            if(iap2GetTestState(NOW_PLAYING_UPDATE_RECV) == TRUE)
            {
            	iap2SetTestState(PLAY_MEDIA_LIB_ITEM,TRUE);
                rc = IAP2_OK;
            }
            else
            {
                printf(" %u ms:  iap2TestStartPlayMediaLibraryItem(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
                rc = IAP2_CTL_ERROR;
            }
        }
    }
    else
    {
        printf(" %u ms:  iap2TestStartPlayMediaLibraryItem():  PersistendIdentifierCount is %d \n ",
                iap2CurrTimeMs(),PersistendIdentifierCount);
        rc = IAP2_CTL_ERROR;
    }

    if(rc != IAP2_OK)
    {
    	iap2SetTestStateError(TRUE);
    }

    return rc;
}

#if IAP2_GST_AUDIO_STREAM
/* Sending iAP2StartUSBDeviceModeAudio to device */
LOCAL S32 iap2TestStartUSBDeviceModeAudio(iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U32 retry_count = 0;
    iAP2StartUSBDeviceModeAudioParameter theiAP2StartUSBDeviceModeAudioParameter;

    memset(&theiAP2StartUSBDeviceModeAudioParameter, 0, sizeof(theiAP2StartUSBDeviceModeAudioParameter) );
    rc = iAP2StartUSBDeviceModeAudio(iap2Device, &theiAP2StartUSBDeviceModeAudioParameter);

    if(rc != IAP2_OK)
    {
        printf(" %u ms:  iap2TestStartUSBDeviceModeAudio(): failed. iAP2StartNowPlayingUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        /* wait for callback with requested information */
        while( (rc == IAP2_OK) && (g_iap2TestDevice.testState.testStateUSBDeviceModeAudioInformationReceived != TRUE)
                && (retry_count < 500) )
        {
            if(g_iap2TestDevice.testDeviceState == iAP2NotConnected)
            {
                rc = IAP2_DEV_NOT_CONNECTED;
                break;
            }
            iap2SleepMs(10);
            retry_count++;
        }
        /* received callback iap2NowPlayingUpdate_CB */
        if(g_iap2TestDevice.testState.testStateUSBDeviceModeAudioInformationReceived == TRUE)
        {
            rc = IAP2_OK;
        }
        else
        {
            printf(" %u ms:  iap2TestStartUSBDeviceModeAudio(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
            rc = IAP2_CTL_ERROR;
            g_iap2TestDevice.testGstState = IAP2_GSTREAMER_STATE_STOPPED;
            sem_post(&g_iap2TestDevice.testGstSemaphore);
            printf("Sending STOP Notification to gstreamer\n");
        }
    }

    if(rc != IAP2_OK)
    {
        g_iap2TestDevice.testState.testStateError = TRUE;
    }

    return rc;
}

void signalHandler(int signo)
{
    printf("Signal %d received\n", signo);
    gQuit = 1;
    g_main_loop_quit(GStreamer_play.loop);
}
#endif

/* polling thread */
LOCAL void iap2PollThread(void* exinf)
{
    S32 rc = IAP2_CTL_ERROR;
    mqd_t mq_fd = -1;

    iAP2GetPollFDs_t getPollFDs;
    iAP2PollFDs_t pollFDs[10];
    S32 desc_ready = 0;
    S32 j = 0;
    S32 cntfds = 0;
    S32 nfds = 0;       /* highest-numbered file descriptor in any of the three sets, plus 1 */
    fd_set read_fds;    /* will be watched to see if characters become available for reading */
    fd_set write_fds;

    iAP2Device_t* iap2device = (iAP2Device_t*)exinf;

    rc = iap2CreateMq(&mq_fd, TEST_MQ_NAME, O_RDONLY);
    if(rc == IAP2_OK)
    {
//        printf("  create mq %s with ID %d \n", TEST_MQ_NAME, (S32)mq_fd);
    }
    else
    {
        printf("  create mq %s failed %d \n", TEST_MQ_NAME, rc);
    }

    if(rc == IAP2_OK)
    {
        /* get file descriptors */
        rc = iAP2GetPollFDs(iap2device, &getPollFDs);
        printf(" %u ms  iAP2GetPollFDs = %d \n", iap2CurrTimeMs(), rc);

        for(j=0; j<getPollFDs.numberFDs; j++)
        {
            rc = iap2AddFDToPollFDs(&pollFDs[0], j, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);
            if(rc >= IAP2_OK)
            {
                cntfds = rc;
                rc = IAP2_OK;
            }
            else
            {
                printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
            }
        }
        if(cntfds == getPollFDs.numberFDs)
        {
            rc = iap2AddFDToPollFDs(&pollFDs[0], cntfds, mq_fd, POLLIN);
            if(rc >= IAP2_OK)
            {
                cntfds = rc;
                rc = IAP2_OK;
            }
            else
            {
                printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
            }
        }
        else
        {
            printf(" %u ms  cntfds: %d != %d numberFDs\n", iap2CurrTimeMs(), cntfds, getPollFDs.numberFDs);
            rc = IAP2_CTL_ERROR;
        }
    }

    if(rc == IAP2_OK)
    {
        /* main loop */
        while (FALSE == g_endPoll)
        {
            /* FD_ZERO() clears out the fd_set, so it doesn't contain any file descriptors */
            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);

            rc = iap2AddFDsToFDset(&pollFDs[0], cntfds, &nfds, &read_fds, &write_fds);
            if(rc != IAP2_OK)
            {
                printf(" iap2AddFDsToFDset = %d \n", rc);
            }

            desc_ready = select(nfds+1, &read_fds, &write_fds, NULL, NULL);
            if(desc_ready > 0)
            {
                for(j = 0; (j < cntfds) && (desc_ready > 0) && (rc >= IAP2_OK); j++)
                {
                    if( (j < cntfds) && (FD_ISSET(pollFDs[j].fd, &read_fds)) )
                    {
//                        printf("    fd[%d] %d (event: %d) is set \n",
//                                j, pollFDs[j].fd, pollFDs[j].event);

                        if(mq_fd == pollFDs[j].fd)
                        {
                            rc = iap2HdlPollMqEvent(pollFDs[j].fd);
                        }
                        else
                        {
                            rc = iAP2HandleEvent(iap2device, pollFDs[j].fd, pollFDs[j].event);
                        }
                        desc_ready--;
                    }
                    if( (j < cntfds) && (FD_ISSET(pollFDs[j].fd, &write_fds)) )
                    {
//                        printf("    fd[%d] %d (event: %d) is set \n",
//                                j, pollFDs[j].fd, pollFDs[j].event);

                        rc = iAP2HandleEvent(iap2device, pollFDs[j].fd, pollFDs[j].event);
                        desc_ready--;
                    }
                }

                if(IAP2_DEV_NOT_CONNECTED == rc){
                	iap2SetTestStateError(TRUE);
                    /* remove libusb file descriptors, but wait to handle mq messages */
                    rc = iAP2GetPollFDs(iap2device, &getPollFDs);

                    memset(&pollFDs[0], 0, sizeof(pollFDs));
                    cntfds = 0;
                    for(j=0; j<getPollFDs.numberFDs; j++)
                    {
                        rc = iap2AddFDToPollFDs(&pollFDs[0], j, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);
                        if(rc >= IAP2_OK){
                            cntfds = rc;
                            rc = IAP2_OK;
                        } else{
                            printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
                        }
                    }
                    if(cntfds == getPollFDs.numberFDs){
                        rc = iap2AddFDToPollFDs(&pollFDs[0], cntfds, mq_fd, POLLIN);
                        if(rc >= IAP2_OK){
                            cntfds = rc;
                            rc = IAP2_OK;
                        } else{
                            printf(" %u ms  iap2AddFDToPollFDs = %d \n", iap2CurrTimeMs(), rc);
                        }
                    }
                }
            }
            else
            {
                /* desc_ready = 0 : select() timed out
                 * desc_ready < 0 : indicates an error (check errno)
                 */
                printf("  myPollFDs failed = %d \n", desc_ready);

                if(desc_ready < 0)
                {
                    g_endPoll = TRUE;
                }
            }
        }/* while-select */
    }

    if(mq_fd > 0)
    {
        rc = mq_close(mq_fd);
    }

    if(rc != IAP2_OK)
    {
        g_rc = rc;
    }

    printf(" %u ms:  exit iap2PollThread \n", iap2CurrTimeMs());
    pthread_exit((void*)exinf);
}

LOCAL S32 iap2StartTest(iAP2Device_t* iap2device)
{
	S32 rc = IAP2_OK;

    if((rc == IAP2_OK) && (iap2GetTestStateError() != TRUE))
    {
        if(g_EAPtesting == TRUE)
        {
            while((iap2GetTestState(EAP_SESSION_STOP_RECV) == FALSE)
                  && (iap2GetGlobalQuit() == FALSE)
                  && (iap2GetTestDeviceState() != iAP2NotConnected))
            {
                iap2SleepMs(50);
                if(iap2GetTestState(EAP_SESSION_START_RECV) == TRUE)
                {
                    /* EA session test */
                    U8 testbuf[] = {"Hello, We are testing ExternalAccessoryProtocol Session!!! \n"};

                    rc = iAP2SendEAPSessionMessage(iap2device, testbuf, strlen((const char*)testbuf), 1);
                    printf(" iAP2SendEAPSessionMessage() rc = %d \n", rc);
                    iap2SetTestState(EAP_SESSION_START_RECV,FALSE);
                }
            }
        }
        else
        {
            if(iap2GetTestStateError() != TRUE)
            {
                /* start MediaLibraryInfo */
                printf("\n %u ms  iap2TestStartMediaLibInfo() \n", iap2CurrTimeMs());
                rc = iap2TestStartMediaLibInfo(iap2device);
                /* stop MediaLibraryInfo */
                printf("\n %u ms  iap2TestStopMediaLibInfo() \n", iap2CurrTimeMs());
                rc = iap2TestStopMediaLibInfo(iap2device);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* start MediaLibrarayUpdate */
                printf("\n %u ms  iap2TestStartMediaLibUpdate() \n", iap2CurrTimeMs());
                rc = iap2TestStartMediaLibUpdate(iap2device);
//                iap2SleepMs(2000);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* start NowPlayingUpdates */
                printf("\n %u ms  iap2TestStartNowPlayingUpdate() \n", iap2CurrTimeMs());
                rc = iap2TestStartNowPlayingUpdate(iap2device);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* PlayMediaLibraryItem */
                printf("\n %u ms  iap2TestStartPlayMediaLibraryItem() \n", iap2CurrTimeMs());
                rc = iap2TestStartPlayMediaLibraryItem(iap2device, 10);
                iap2SleepMs(2000);
            }

#if IAP2_GST_AUDIO_STREAM
            if( (iap2GetTestStateError() != TRUE)
                && (g_iap2TestDevice.iap2USBHostMode == FALSE) )
            {
                /* start USBDeviceModeAudio */
                printf("\n %u ms  iap2TestStartUSBDeviceModeAudio() \n", iap2CurrTimeMs());
                rc = iap2TestStartUSBDeviceModeAudio(iap2device);
                iap2SleepMs(1000);
            }


            if( (rc == IAP2_OK)
                &&(iap2GetTestStateError() != TRUE)
                && (iap2GetGstState() == IAP2_GSTREAMER_PLAYING) )
            {
                /* if it works - let Gstreamer Play for 30 seconds */
                retry_count = 0;
                while( (iap2GetTestStateError() != TRUE)
                        && (iap2GetGlobalQuit() == FALSE)
                        && (iap2GetTestDeviceState() != iAP2NotConnected)
                        && (retry_count < TEST_PLAYBACK_TIME_MS) )
                {
                    iap2SleepMs(100);
                    retry_count += 100;
                }
            }
#endif

            if(g_iap2TestDevice.testMediaLibInfoID != NULL)
            {
                /* check if Apple device was disconnected */
                if(g_iap2TestDevice.testDeviceState == iAP2NotConnected){
                    printf("\n %u ms  Apple device was disconnected! \n", iap2CurrTimeMs());
                } else{
                    /* stop NowPlayingUpdates */
                    printf("\n %u ms  iap2TestStopNowPlayingUpdate() \n", iap2CurrTimeMs());
                    rc = iap2TestStopNowPlayingUpdate(iap2device);

                    /* stop MediaLibraryUpdate */
                    printf("\n %u ms  iap2TestStopMediaLibUpdate() \n", iap2CurrTimeMs());
                    rc = iap2TestStopMediaLibUpdate(iap2device);
                }
                iap2SleepMs(1000);
            }
        }
    }
    return rc;
}

/* application thread */
void iap2AppThread(void* exinf)
{
    S32 rc = IAP2_OK;

    TEST_THREAD_ID threadID = 0;
    char threadName[8];
    void* status;
    mqd_t mq_fd = -1;
    U16 i = 0;

    U32 retry_count = 0;

    iAP2InitParam_t* iAP2InitParameter = (iAP2InitParam_t*)exinf;
    iAP2Device_t* iap2device = NULL;

    rc = iap2CreateMq(&mq_fd, TEST_MQ_NAME, O_CREAT | O_RDWR);
    if(rc != IAP2_OK)
    {
        printf("  create mq %s failed %d \n", TEST_MQ_NAME, rc);
    }

    if(rc == IAP2_OK)
    {
        rc = iap2CreateMq(&g_iap2TestDevice.mqAppTskFd, TEST_MQ_NAME_APP_TSK, O_CREAT | O_NONBLOCK | O_RDWR);
        if(rc != IAP2_OK)
        {
            printf("  create mq %s failed %d \n", TEST_MQ_NAME_APP_TSK, rc);
        }
    }

    /**
     * create internal device structure and initialize
     * with iAP2InitParameter provided by application.
     */
    iap2device = iAP2InitDeviceStructure(iAP2InitParameter);
    printf(" %u ms  iAP2InitDeviceStructure = %p \n", iap2CurrTimeMs(), iap2device);
    IAP2TESTDLTLOG(DLT_LOG_ERROR, "iAP2InitDeviceStructure iap2device = %p ", iap2device);

    if(NULL != iap2device)
    {
        rc = iAP2InitDeviceConnection(iap2device);
        printf(" %u ms  iAP2InitDeviceConnection:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG,"iAP2InitDeviceConnection rc = %d ",rc);

        if(rc == IAP2_OK)
        {
            g_endPoll = FALSE;

            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iPoll", 1);

            /* -----------  start polling thread  ----------- */
            threadID = iap2CreateThread(iap2PollThread, &threadName[0], iap2device);
            if(iap2VerifyThreadId(threadID) != IAP2_OK)
            {
            	rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
        }

        if(rc == IAP2_OK)
        {
        	iap2SetTestState(RUNNING, TRUE);
            /* wait until device is attached */
            while( (g_iap2TestDevice.testDeviceState != iAP2DeviceReady) && (g_iap2TestDevice.testDeviceState != iAP2LinkiAP1DeviceDetected)
                    && (retry_count < 300) )
            {
                if(g_iap2TestDevice.testDeviceState == iAP2NotConnected)
                {
                    rc = IAP2_DEV_NOT_CONNECTED;
                    break;
                }
                iap2SleepMs(50);
                retry_count++;
            }
            if(g_iap2TestDevice.testDeviceState == iAP2DeviceReady)
            {
                printf(" %u ms  device attached  retry: %d \n", iap2CurrTimeMs(), retry_count);
                rc = IAP2_OK;
            }
            else if(g_iap2TestDevice.testDeviceState == iAP2LinkiAP1DeviceDetected)
            {
                printf(" %u ms  iAP1 device detected  retry: %d \n", iap2CurrTimeMs(), retry_count);

//                printf(" iAP1 not supported. send Bad DETECT Ack to device \n");
//                rc = iAP2CanceliAP1Support(iap2device);
                printf(" Set g_iap1Device to true.  \n");
                g_iap1Device = TRUE;

                iap2SetTestStateError(TRUE);
            }
            else
            {
                printf(" %u ms  device not attached [state: %d | retry: %d] \n", iap2CurrTimeMs(), g_iap2TestDevice.testDeviceState, retry_count);
                iap2SetTestStateError(TRUE);
            }
        }

        if(rc == IAP2_OK)
        {
            rc= iap2StartTest(iap2device);
        }

        /* exit pollThread */
        if(threadID > 0)
            rc = iap2SendMq(mq_fd, TEST_MQ_CMD_STOP_POLL, sizeof(TEST_MQ_CMD_STOP_POLL));
        iap2SetTestState(STOPPED, TRUE);
        iap2SetTestState(RUNNING, FALSE);

        /* -----------  clean thread and mq  ----------- */
        if(threadID > 0)
        {
           rc = pthread_join(threadID, &status);
        }
        if(mq_fd > 0)
        {
            rc = mq_close(mq_fd);
            rc = mq_unlink(TEST_MQ_NAME);
        }
        if(g_iap2TestDevice.mqAppTskFd > 0)
        {
            rc = mq_close(g_iap2TestDevice.mqAppTskFd);
            rc = mq_unlink(TEST_MQ_NAME_APP_TSK);
        }

        /* check if it was not freed during test run */
        iap2TestFreePtr( (void**)&g_iap2TestDevice.testMediaLibInfoID);
        if(g_iap2TestDevice.testMediaItem != NULL)
        {
            for(i=0; i<g_iap2TestDevice.testMediaItemCnt;i++)
            {
                if(&(g_iap2TestDevice.testMediaItem[i]) != NULL)
                {
                    iAP2FreeiAP2MediaItem(&(g_iap2TestDevice.testMediaItem[i]));
                }
            }
            iap2TestFreePtr( (void**)&g_iap2TestDevice.testMediaItem);
        }

        rc = iAP2DisconnectDevice(iap2device);
        printf(" %u ms  iAP2DisconnectDevice:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DisconnectDevice = %d", rc);

        /* de-initialize the device structure */
        rc = iAP2DeInitDeviceStructure(iap2device);
        printf(" %u ms  iAP2DeInitDeviceStructure:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DeInitDeviceStructure = %d ", rc);

    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    if(rc != IAP2_OK)
    {
        g_rc = rc;
    }

    printf(" %u ms:  exit iap2AppThread \n", iap2CurrTimeMs());
    pthread_exit((void*)exinf);
}

S32 main(int argc, const char** argv)
{
    S32 rc = IAP2_CTL_ERROR;
    TEST_THREAD_ID AppTskID = 0;
#if IAP2_GST_AUDIO_STREAM
    TEST_THREAD_ID GstTskID = 0;
#endif
    TEST_THREAD_ID iAP1AppTskId = 0;
    char threadName[8];
    void* status;

    iAP2InitParam_t iAP2InitParameter;

//    signal(SIGINT, signalHandler);
//    signal(SIGQUIT, signalHandler);

    IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
    IAP2REGISTERCTXTWITHDLT();

    memset(&g_iap2UserConfig, 0, sizeof(g_iap2UserConfig));
    memset(&g_iap2TestDevice, 0, sizeof(g_iap2TestDevice));
    memset(&iAP2InitParameter, 0, sizeof(iAP2InitParam_t) );

    SetGlobPtr(&g_iap2TestDevice);

    /**
     * get user configuration. iap2GetArguments() gets runtime user inputs and
     * stores in global g_iAP2UserConfig Structure.
     */
    rc = iap2GetArguments(argc, argv, &g_iap2UserConfig);
    if(rc == IAP2_OK)
    {
        rc = iap2InitDev(&iAP2InitParameter);
    }

    if(rc == IAP2_OK)
    {
        /* Application provided Data */
        rc = iap2SetInitialParameter(&iAP2InitParameter, g_iap2UserConfig);
        printf(" iap2SetInitialParameter:  rc  = %d \n",rc);
    }
    if(rc == IAP2_OK)
    {
#if IAP2_GST_AUDIO_STREAM
        /* set thread name */
        memset(&threadName[0], 0, (sizeof(threadName)));
        sprintf(&threadName[0], "%s%d", "iGst", 1);

        g_leaveGstWhile = FALSE;

        rc = sem_init(&g_iap2TestDevice.testGstSemaphore, 0, 0);
        if(rc != IAP2_OK)
        {
            printf("  sem_init %s failed %d \n", threadName, rc);
        }
        /* -----------  start gstreamer thread  ----------- */
        GstTskID = iap2CreateThread(iap2GstThread, &threadName[0], NULL);
        if(iap2VerifyThreadId(GstTskID) != IAP2_OK)
        {
        	rc = IAP2_CTL_ERROR;
        	printf("  create thread %s failed %d \n", threadName, rc);
        }
        else
        {
        	g_iap2TestDevice.testGstState = IAP2_GSTREAMER_STATE_INITIALIZE;
        	sem_post(&g_iap2TestDevice.testGstSemaphore);
        }
#endif

        if(rc == IAP2_OK)
        {
            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iApp", 1);

            /* -----------  start application thread  ----------- */
            AppTskID = iap2CreateThread(iap2AppThread, &threadName[0], &iAP2InitParameter);
            if(iap2VerifyThreadId(AppTskID) != IAP2_OK)
            {
                rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
        }

        /*Create iAP1 application thread  */

        rc = sem_init(&g_iap1semlock, 0, 0);
        printf("###########   rc = %d \n", rc);

        if(rc == IAP2_OK)
        {

           /* set thread name */
           memset(&threadName[0], 0, (sizeof(threadName)));
           sprintf(&threadName[0], "%s%d", "iAP1App", 1);

           g_iap1Device = FALSE;

            iAP1AppTskId = iap2CreateThread(iap1AppThread, &threadName[0], NULL);
            if(iap2VerifyThreadId(iAP1AppTskId) != IAP2_OK)
            {
                rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
        }

        /* -----------  clean thread  ----------- */
        if(AppTskID > 0)
        {
            (void)pthread_join(AppTskID, &status);
        }
#if IAP2_GST_AUDIO_STREAM
        /* leave gstreamer while-loop */
        g_leaveGstWhile = TRUE;
        sem_post(&g_iap2TestDevice.testGstSemaphore);
        if(GstTskID > 0)
        {
            (void)pthread_join(GstTskID, &status);
        }
#endif

        /* Irespective of the device type connected , release the semaphore so that the
         * iap1AppThread can be terminated.
         */
        sem_post(&g_iap1semlock);
        if(iAP1AppTskId > 0)
        {
            (void)pthread_join(iAP1AppTskId, &status);
         }
        sem_destroy(&g_iap1semlock);


    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2Udev failed with %d \n", rc);
    }

    /* de-initialize the iAP2InitParam_t structure */
    iap2ResetInitialParameter(&iAP2InitParameter);

    if(g_rc == IAP2_OK)
    {
        printf(" \n\n ***** iAP2-1AP1 SMOKETEST PASS ***** \n\n");
        IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 SMOKETEST PASS *****");
    }
    else
    {
        printf("\n\n ***** iAP2-iAP1 SMOKETEST FAIL ***** \n\n");
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 SMOKETEST FAIL *****");
    }

    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return rc;
}
