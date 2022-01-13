#include "iap2_dlt_log.h"
#include "iap2_load_test.h"
#include "iap2_test_gstreamer.h"


IMPORT iap2TestAppleDevice_t g_iap2TestDevice;
IMPORT U32 CurrentSongTotalDuration;

/* helper API for callback iap2MediaLibraryUpdates_CB */
LOCAL S32 iap2MediaItemDbUpdate(iAP2MediaLibraryUpdateParameter* MediaLibraryUpdateParameter)
{
    S32 rc = IAP2_OK;
    U16 i = 0;
    size_t size = 0;
    U16 mediaItemCnt = 0;
    U16 t_index = 0;

    pthread_mutex_lock(&g_iap2TestDevice.testMediaLibMutex);

    if( g_iap2TestDevice.tmpMediaItem == NULL )
    {
        /* first MediaLibraryUpdates_CB */
        mediaItemCnt = MediaLibraryUpdateParameter->iAP2MediaItem_count;
        g_iap2TestDevice.tmpMediaItem = calloc( MediaLibraryUpdateParameter->iAP2MediaItem_count, sizeof( iAP2MediaItem ) );
        if( g_iap2TestDevice.tmpMediaItem != NULL )
        {
            for( i=0; ( ( i < MediaLibraryUpdateParameter->iAP2MediaItem_count ) && ( rc == IAP2_OK ) ); i++ )
            {
                if( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0 )
                {
                    rc = iap2AllocateandUpdateData( &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                    MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                    &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                    MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                    iAP2_uint64 );
                    if( ( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0 ) && ( rc == IAP2_OK ) )
                    {
                        rc = iap2AllocateandUpdateData( &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle,
                                                        &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle_count,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count,
                                                        iAP2_utf8 );
                    }
                    if( ( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0 ) && ( rc == IAP2_OK ) )
                    {
                        rc = iap2AllocateandUpdateData( &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre,
                                                        &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre_count,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count,
                                                        iAP2_utf8 );
                    }
                }
            }
            if( rc == IAP2_OK )
            {
                rc = mediaItemCnt;
            }
        }
        else
        {
            printf( "\t %u ms:  iap2MediaItemDbUpdate(): g_MediaItem.test_MediaItem is NULL \n", iap2CurrTimeMs() );
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else if( ( g_iap2TestDevice.testMediaLibInfoID != NULL )
             &&( strncmp( ( char* )g_iap2TestDevice.testMediaLibInfoID,
                          ( char* )( *MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier ), size ) == 0 ) )
    {
        /* MediaLibrary database still exists, update the DB */
        mediaItemCnt = MediaLibraryUpdateParameter->iAP2MediaItem_count;
        size = ( ( g_iap2TestDevice.tmpMediaItemCnt + MediaLibraryUpdateParameter->iAP2MediaItem_count ) * sizeof( iAP2MediaItem ) );
        g_iap2TestDevice.tmpMediaItem = realloc( g_iap2TestDevice.tmpMediaItem, size );
        memset( g_iap2TestDevice.tmpMediaItem+g_iap2TestDevice.tmpMediaItemCnt, 0, ( MediaLibraryUpdateParameter->iAP2MediaItem_count ) * sizeof( iAP2MediaItem ) );
        if( g_iap2TestDevice.tmpMediaItem != NULL )
        {
            /* add MediaItem information to current existing one */
            t_index = g_iap2TestDevice.tmpMediaItemCnt;
            for( i = 0; ( ( i < MediaLibraryUpdateParameter->iAP2MediaItem_count ) && ( rc == IAP2_OK ) ); i++ )
            {
                if( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0 )
                {
                    rc = iap2AllocateandUpdateData( &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemPersistentIdentifier,
                                                    MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                    &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemPersistentIdentifier_count,
                                                    MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                    iAP2_uint64 );
                    if( ( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0 ) && ( rc == IAP2_OK ) )
                    {
                        rc = iap2AllocateandUpdateData( &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemTitle,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle,
                                                        &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemTitle_count,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count,
                                                        iAP2_utf8 );
                    }
                    if( ( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0 ) && ( rc == IAP2_OK ) )
                    {
                        rc = iap2AllocateandUpdateData( &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemGenre,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre,
                                                        &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemGenre_count,
                                                        MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count,
                                                        iAP2_utf8 );
                    }
                    t_index++;
                }
            }
            if( rc == IAP2_OK )
            {
                rc = mediaItemCnt;
            }
        }
        else
        {
            printf( "\t %u ms  iap2MediaItemDbUpdate(): g_MediaItem.test_MediaItem is NULL. Realloc failed. \n", iap2CurrTimeMs() );
            rc = IAP2_ERR_NO_MEM;
        }
    }
    else
    {
        printf( "\t %u ms  iap2MediaItemDbUpdate(): g_MediaItem.test_MediaItem already allocated \n", iap2CurrTimeMs() );
        rc = IAP2_CTL_ERROR;
    }

    pthread_mutex_unlock(&g_iap2TestDevice.testMediaLibMutex);

    return rc;
}

/* **********************  callbacks ********************** */
S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    switch(dState)
    {
        case iAP2NotConnected :
            printf("\t %u ms %p  Device state : iAP2NotConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2TransportConnected :
            printf("\t %u ms %p  Device state : iAP2TransportConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkConnected:
            printf("\t %u ms %p  Device state : iAP2LinkConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2AuthenticationPassed :
            printf("\t %u ms %p  Device state : iAP2AuthenticationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2IdentificationPassed :
            printf("\t %u ms %p  Device state : iAP2IdentificationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2DeviceReady:
            printf("\t %u ms %p  Device state : iAP2DeviceReady  \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkiAP1DeviceDetected:
            printf("\t %u ms %p  Device state : iAP2LinkiAP1DeviceDetected\n", iap2CurrTimeMs(), iap2Device);
            break;
        default:
            printf("\t %u ms %p  Device state : unknown %d \n", iap2CurrTimeMs(), iap2Device, dState);
            switch(iAP2GetDeviceErrorState(iap2Device, NULL))
            {
                case iAP2NoError:
                    printf("\t %u ms %p  Device ErrorState:  iAP2NoError \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2TransportConnectionFailed:
                    printf("\t %u ms %p  DeviceErrorState:  iAP2TransportConnectionFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2LinkConnectionFailed:
                    printf("\t %u ms %p  DeviceErrorState:  iAP2LinkConnectionFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2AuthenticationFailed:
                    printf("\t %u ms %p  DeviceErrorState:  iAP2AuthenticationFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2IdentificationFailed:
                    printf("\t %u ms %p  DeviceErrorState:  iAP2IdentificationFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                default:
                    printf("\t %u ms %p  DeviceErrorState: unknown \n", iap2CurrTimeMs(), iap2Device);
                    break;
            }
            rc = IAP2_CTL_ERROR;
            break;
    }
    iap2SetTestDeviceState(dState);

    return rc;
}

S32 iap2AuthenticationSucceeded_CB(iAP2Device_t* iap2Device, iAP2AuthenticationSucceededParameter* authParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context         = context;
    authParameter   = authParameter;

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

    g_iap2TestDevice.testMediaLibInfoID = (U8*)strndup( (const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier),
                            strnlen((const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier), STRING_MAX) );
    if(g_iap2TestDevice.testMediaLibInfoID == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2TESTDLTLOG(DLT_LOG_ERROR,"testMediaLibInfoID is NULL");
    }

    iap2SetTestState(MEDIA_LIB_INFO_RECV, TRUE);

    return rc;
}

S32 iap2MediaLibraryUpdates_CB(iAP2Device_t* iap2Device, iAP2MediaLibraryUpdateParameter* MediaLibraryUpdateParameter, void* context)
{
    S32 rc = IAP2_OK;
    U16 i = 0;
    size_t size = 0;
    U32 callTime = 0;
    /* temporary fix for compiler warning */
    context =context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2MediaLibraryUpdates_CB called");
    printf("\t %u ms %p  iap2MediaLibraryUpdates_CB called \n", iap2CurrTimeMs(), iap2Device);

    if((iap2GetTestState(RUNNING) == FALSE)
       ||(iap2GetTestState(STOPPED) == TRUE))
    {
        /* iAP2 Smoketest already stopped */
        return IAP2_OK;
    }

    size = strlen((char*)(g_iap2TestDevice.testMediaLibInfoID));

    /* MediaLibraryUpdate for same MediaLibrary */
    if(strncmp((char*)g_iap2TestDevice.testMediaLibInfoID,
               (char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier), size) == 0)
    {
        /* get percentage completion for current set of MediaLibraryUpdates */
        if( ( MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0 )
                &&( g_iap2TestDevice.testMediaLibUpdateProgress < 100 ) )
        {
            if(g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms == 0)
            {
                g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms = iap2CurrTimeMs();
                printf("\n\t    Time Taken For First DataBase Transfer(msec) = %d\n\n", (g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms - g_iap2TestDevice.DataBaseTransfer_StartTime_ms));
            }
            if(*(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress) == 100)
            {
                printf("\n\t    Time Taken For DataBase Transfer(msec) = %d\n\n", (iap2CurrTimeMs() - g_iap2TestDevice.DataBaseTransfer_StartTime_ms));
            }
            printf( "\t    MediaLibUpdateProgress = %d / 100 \n", *( MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress ) );
            if( MediaLibraryUpdateParameter->iAP2MediaItem_count > 0 )
            {
                printf( "\t    No. of received MediaItems:  %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count );
                callTime = iap2CurrTimeMs();
                rc = iap2MediaItemDbUpdate( MediaLibraryUpdateParameter );
                printf( "\t    iap2MediaItemDbUpdate() takes %u ms \n", ( iap2CurrTimeMs() - callTime ) );
                if( rc > IAP2_OK )
                {
                    g_iap2TestDevice.tmpMediaItemCnt += rc;
                    rc = IAP2_OK;
                }
            }
            if( MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier_count > 0 )
            {
                printf( "\t    iAP2MediaItemDeletePersistentIdentifier_count  %d \n",
                        MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier_count );
            }
            if( *( MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress ) == 100 )
            {
                printf( "\n Current number of songs: %d \n", g_iap2TestDevice.tmpMediaItemCnt );
            }
            g_iap2TestDevice.testMediaLibUpdateProgress = *( MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress );
        }
        else
        {
            /* MediaLibraryUpdates because of playback change */
            for( i=0; ( ( i < MediaLibraryUpdateParameter->iAP2MediaItem_count ) && ( rc == IAP2_OK ) ); i++ )
            {
                if( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0 )
                {
                    printf( "\t    iAP2MediaItemPersistentIdentifier:  %llu \n",
                            *( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier ) );
                }
                if( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0 )
                {
                    printf( "\t    iAP2MediaItemTitle:  %s \n",
                            *( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle ) );
                }
                if( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0 )
                {
                    printf( "\t    iAP2MediaItemGenre:  %s \n",
                            *( MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre ) );
                }
            }
        }
    }
    else
    {
        /* received MediaLibraryUpdate for a different MediaLibrary */
        printf("\n\t    received MediaLibraryUpdate for a different MediaLibraryID \n");
        if(g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms == 0)
        {
            g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms = iap2CurrTimeMs();
            printf("\n\t    Time Taken For First DataBase Transfer(msec) = %d\n\n", (g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms - g_iap2TestDevice.DataBaseTransfer_StartTime_ms));
        }
        /* get percentage completion for current set of MediaLibraryUpdates */
        if(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0)
        {
            g_iap2TestDevice.testMediaLibUpdateProgress = *(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress);
            printf("\t    MediaLibUpdateProgress = %d / 100 \n", g_iap2TestDevice.testMediaLibUpdateProgress);
            if(*(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress) == 100)
            {
                printf("\n\t    Time Taken For DataBase Transfer(msec) = %d\n\n", (iap2CurrTimeMs() - g_iap2TestDevice.DataBaseTransfer_StartTime_ms));
            }
        }

        printf("\t    Number of songs received from Apple device: %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count);
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
                printf("\t    now playing title:  %s \n", *(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle));
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
            for(j = 0;j < NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds_count;j++)
            {
                if(NULL != NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds)
                {
                    printf("\t    ElapsedTime:  %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds));
                }
                if( &(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j]) != NULL)
                {
                    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "CurrentSongTotalDuration = %d, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j] = %d",
                            CurrentSongTotalDuration, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j]);
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackStatus_count > 0)
            {
                if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackStatus != NULL)
                {
                    printf("\t    PlaybackStatus:  %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackStatus));
#ifdef IAP2_EVALUVATE_PLAYMEDIA_LIBRARY_ITEMS
                    if(*(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackStatus) == IAP2_PLAYBACK_STATUS_PLAYING)
                    {
                        printf("\t\n    Timetaken for playback(msec):  %d \n", (iap2CurrTimeMs() - g_iap2TestDevice.PlayMediLibraryItems_StartTime_ms) );
                    }
#endif
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName_count > 0)
            {
                if( ((NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName) != NULL)
                    && (*(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName) != NULL) )
                {
                    printf("\t    PlaybackAppName:  %s \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName));
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode_count > 0)
            {
                if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode != NULL)
                {
                    printf("\t    ShuffleMode:  %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode));
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode_count > 0)
            {
                if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode != NULL)
                {
                    printf("\t    RepeatMode:   %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode));
                }
            }

        }
    }

    iap2SetTestState(NOW_PLAYING_UPDATE_RECV, TRUE);
    return rc;
}

S32 iap2StartExternalAccessoryProtocolSession_CB(iAP2Device_t* iap2Device, iAP2StartExternalAccessoryProtocolSessionParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    thisParameter = thisParameter;
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StartExternalAccessoryProtocolSession_CB called \n");
    printf("\t %u ms %p  iap2StartExternalAccessoryProtocolSession_CB called \n", iap2CurrTimeMs(), iap2Device);
    printf("\t       ProtocolIdentifier:  %d | SessionIdentifier:  %d \n", *(thisParameter->iAP2ExternalAccesoryProtocolIdentifier), *(thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier));

    g_iap2TestDevice.EaProtocolID = *(thisParameter->iAP2ExternalAccesoryProtocolIdentifier);

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
    printf("\t %u ms %p  iap2StopExternalAccessoryProtocolSession_CB called \n", iap2CurrTimeMs(), iap2Device);
    printf("\t       iOS Identifier:  %d \n", *(thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier));

    g_iap2TestDevice.EaProtocolID = 0;

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
    printf("\t %u ms %p  iap2iOSAppDataReceived_CB called. iAP2iOSAppIdentifier = %d, iAP2iOSAppDataLength = %d \n",
            iap2CurrTimeMs(), iap2Device, iAP2iOSAppIdentifier, iAP2iOSAppDataLength);

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

S32 iap2StartEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    printf("\t %u ms %p  iap2StartEANativeTransport_CB called \n", iap2CurrTimeMs(), iap2Device);
    printf("\t       Identifier:  %d | writeEp:  %d | readEp:  %d\n", iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint);

    g_iap2TestDevice.EANativeTransportAppId = iAP2iOSAppIdentifier;
    g_iap2TestDevice.SinkEndpoint = sinkEndpoint;
    g_iap2TestDevice.SourceEndpoint = sourceEndpoint;

    iap2SetTestState(EA_NATIVE_TRANSPORT_STOP_RECV, FALSE);
    iap2SetTestState(EA_NATIVE_TRANSPORT_START_RECV, TRUE);

    return rc;
}

S32 iap2StopEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    printf("\t %u ms %p  iap2StopEANativeTransport_CB called \n", iap2CurrTimeMs(),  iap2Device);
    printf("\t       Identifier:  %d | writeEp:  %d | readEp:  %d\n", iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint);

    g_iap2TestDevice.EANativeTransportAppId = iAP2iOSAppIdentifier;
    g_iap2TestDevice.SinkEndpoint = sinkEndpoint;
    g_iap2TestDevice.SourceEndpoint = sourceEndpoint;

    iap2SetTestState(EA_NATIVE_TRANSPORT_START_RECV, FALSE);
    iap2SetTestState(EA_NATIVE_TRANSPORT_STOP_RECV, TRUE);

    return rc;
}

#if IAP2_GST_AUDIO_STREAM
S32 iap2USBDeviceModeAudioInformation_CB(iAP2Device_t* iap2Device, iAP2USBDeviceModeAudioInformationParameter* theiAP2USBDeviceModeAudioInformationParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2USBDeviceModeAudioInformation_CB called \n");
    printf("\t %u ms %p  iap2USBDeviceModeAudioInformation_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(theiAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate_count > 0)
    {
        if(iap2GetGstSampleRate() != *(theiAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate))
        {
            iap2SetGstSampleRate(*(theiAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate));

            iap2SetGstState(IAP2_GSTREAMER_SAMPLE_RATE_CHANGE);
        }
    }
    else if(iap2GetGstSampleRate() != IAP2_GSTREAMER_STATE_PLAYING)
    {
        iap2SetGstState(IAP2_GSTREAMER_STATE_PLAYING);
        printf("Sending PLAY Notification to gstreamer\n");
    }

    iap2SetTestState(USB_DEVICE_MODE_AUDIO_INFO_RECV, TRUE);
    return rc;
}
#endif

S32 iap2DeviceInformationUpdate_CB(iAP2Device_t* iap2Device, iAP2DeviceInformationUpdateParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    printf("\t %u ms %p  iap2DeviceInformationUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);
    if(NULL != thisParameter)
    {
        if(thisParameter->iAP2DeviceName_count > 0)
        {
            printf("\t       iAP2DeviceName:  %s\n", *(thisParameter->iAP2DeviceName));
        }
        else
        {
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}


