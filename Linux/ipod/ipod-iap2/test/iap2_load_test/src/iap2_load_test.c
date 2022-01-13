/* **********************  includes  ********************** */
#include "iap2_load_test.h"
#include "iap2_test_gstreamer.h"

#include "iap2_usb_role_switch.h"
#include "iap2_load_test_filetransfer_performance.h"
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>
#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include <iap2_external_accessory_protocol_session.h>
#include "iap2_dlt_log.h"



/* **********************  defines   ********************** */

/* configuration values to load the g_ffs */
#define ACC_VENDOR_ID                           "44311"
#define ACC_PRODUCT_ID                          "1111"
#define ACC_BCD_DEVICE                          "1"
#define ACC_QMULT                               "1"

S32 g_rc = IAP2_OK;
U32 CurrentSongTotalDuration = 0;
iap2UserConfig_t g_iap2UserConfig;
iap2TestAppleDevice_t g_iap2TestDevice;

typedef struct
{
    int socket_fd;
    iAP2InitParam_t iAP2InitParameter;
    U8* udevPath;
} iap2AppThreadInitData_t;

LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 application_process_main (int socket_fd);
LOCAL void iap2AppThread(void* exinf);

/* **********************  iap2 test functions ********************** */

LOCAL S32 iap2TestStopPollThread(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd, MQ_CMD_EXIT_IAP2_COM_THREAD, NULL, 0);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Sending iAP2StartMediaLibraryInformation to device */
LOCAL S32 iap2TestStartMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U32 retry_count = 0;
    iAP2StartMediaLibraryInformationParameter theiAP2StartMediaLibraryInformationParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StartMediaLibraryInformationParameter, 0, sizeof(iAP2StartMediaLibraryInformationParameter));

    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_START_MEDIALIB_INFO,
                               &theiAP2StartMediaLibraryInformationParameter, sizeof(theiAP2StartMediaLibraryInformationParameter));
    }

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StartMediaLibraryInformationParameter(&theiAP2StartMediaLibraryInformationParameter); */

    if(rc != IAP2_OK)
    {
        printf(" %u ms  iap2TestStartMediaLibInfo(): failed. iAP2StartMediaLibraryInformation  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        /* wait for callback with requested information */
        while( (rc == IAP2_OK) && (iap2GetTestState(MEDIA_LIB_INFO_RECV) != TRUE)
               &&(iap2GetTestStateError() != TRUE) && (retry_count < 300) )
        {
            if(iap2GetTestDeviceState() == iAP2NotConnected)
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
            printf(" %u ms  iap2TestStartMediaLibInfo(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
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
LOCAL S32 iap2TestStopMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iAP2StopMediaLibraryInformationParameter StopMediaLibraryInformationParameter;

    iap2Device = iap2Device;

    memset(&StopMediaLibraryInformationParameter, 0, sizeof(iAP2StopMediaLibraryInformationParameter));

    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_STOP_MEDIALIB_INFO,
                               &StopMediaLibraryInformationParameter, sizeof(StopMediaLibraryInformationParameter));
    }

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopMediaLibraryInformationParameter(&StopMediaLibraryInformationParameter); */

    return rc;
}

/* sending iAP2StartMediaLibraryUpdates to device */
LOCAL S32 iap2TestStartMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StartMediaLibraryUpdatesParameter theiAP2StartMediaLibraryUpdatesParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StartMediaLibraryUpdatesParameter, 0, sizeof(iAP2StartMediaLibraryUpdatesParameter));

    if(g_iap2TestDevice.testMediaLibInfoID != NULL)
    {
        /* set MediaLibraryIdentifier */
        rc = iap2AllocateandUpdateData(&theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier,
                                       &g_iap2TestDevice.testMediaLibInfoID,
                                       &theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier_count,
                                       1, iAP2_utf8);
        if(rc == IAP2_OK)
        {
            /* set MediaItem properties which you want to receive */
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties = calloc(1, sizeof(iAP2MediaItemProperties));
            if(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if(rc == IAP2_OK)
            {
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUpdateProgress_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count++;
#ifdef IAP2_DATABASE_TRANSFER_PART_PERFORMANCE
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyArtist_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTitle_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;
#endif
#ifdef IAP2_DATABASE_TRANSFER_FULL_PERFORMANCE
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtist_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscCount_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscNumber_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumPersistentIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTitle_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackCount_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackNumber_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyArtist_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyArtistPersistentIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyComposer_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyComposerPersistentIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenrePersistenIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanned_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanSupported_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsLiked_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsLikeSupported_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsPartOfCompilation_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsResidentOndevice_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyMediaType_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyRating_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties = calloc(1, sizeof(iAP2MediaPlaylistProperties));
                if(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties == NULL)
                {
                    rc = IAP2_ERR_NO_MEM;
                }
#endif
            }
#ifdef IAP2_DATABASE_TRANSFER_FULL_PERFORMANCE
            if(rc == IAP2_OK)
            {
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListContainedMediaItems_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsFolder_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsiTunesRadioStation_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyName_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyParentPersistentIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPersistentIdentifier_count++;
                theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count++;
            }
#endif
        }
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_START_MEDIALIB_UPDATE,
                               &theiAP2StartMediaLibraryUpdatesParameter, sizeof(theiAP2StartMediaLibraryUpdatesParameter));
        g_iap2TestDevice.DataBaseTransfer_StartTime_ms = iap2CurrTimeMs();
        g_iap2TestDevice.FirstDataBaseTransfer_ReceivedTime_ms = 0;
        printf("\n\t    DataBaseTransfer StartTime (ms) = %d\n\n", g_iap2TestDevice.DataBaseTransfer_StartTime_ms);
    }

    iAP2FreeiAP2StartMediaLibraryUpdatesParameter(&theiAP2StartMediaLibraryUpdatesParameter);

    if(rc != IAP2_OK)
    {
        printf(" %u ms  iap2TestStartMediaLibUpdates(): failed. iAP2StartMediaLibraryUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        /* Do not wait until the complete MediaLib was received.
         * We did not know when the Apple device sends the first MediaLibUpdate message.
         * We have observed, that it takes more than 10 seconds. */

        /* wait to receive some MediaLib to start playback via StartPlayMediaLibraryItem() */
        rc = iap2MediaItemDbWaitForUpdate(10000, TRUE);
    }

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
/* sending iap2TestStopMediaLibUpdate to device */
LOCAL S32 iap2TestStopMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StopMediaLibraryUpdatesParameter StopMediaLibraryUpdatesParameter;

    iap2Device = iap2Device;

    memset(&StopMediaLibraryUpdatesParameter, 0, sizeof(iAP2StopMediaLibraryUpdatesParameter));
    rc = iap2AllocateandUpdateData(&StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier,
                                   &g_iap2TestDevice.testMediaLibInfoID,
                                   &StopMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier_count,
                                   1, iAP2_utf8);
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_STOP_MEDIALIB_UPDATE,
                               &StopMediaLibraryUpdatesParameter, sizeof(StopMediaLibraryUpdatesParameter));
    }

    iAP2FreeiAP2StopMediaLibraryUpdatesParameter(&StopMediaLibraryUpdatesParameter);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Prepare & Sending iAP2StartNowPlayingUpdates to send */
LOCAL S32 iap2TestStartNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iAP2StartNowPlayingUpdatesParameter theiAP2StartNowPlayingUpdatesParameter;

    iap2Device = iap2Device;

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
#ifdef IAP2_EVALUVATE_FILE_TRANSFER_PERFORMANCE
        /* receive Artwork and use File Transfer Session */
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count++;
#endif
        /* enable information about shuffle mode and repeat mode */
        theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes = (iAP2StartNowPlayingPlaybackAttributes*)calloc(1, sizeof(iAP2StartNowPlayingPlaybackAttributes));
        if(theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes != NULL)
        {
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackAppName_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackStatus_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackAppBundleID_count++;
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

    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_START_NOWPLAYING_UPDATE,
                               &theiAP2StartNowPlayingUpdatesParameter, sizeof(theiAP2StartNowPlayingUpdatesParameter));
    }

    iAP2FreeiAP2StartNowPlayingUpdatesParameter(&theiAP2StartNowPlayingUpdatesParameter);

    /* no wait here.
     * we did not know if the Apple device is in play state. */

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Sending iap2TestStopNowPlayingUpdate to device */
LOCAL S32 iap2TestStopNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iAP2StopNowPlayingUpdatesParameter theiAP2StopNowPlayingUpdatesParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StopNowPlayingUpdatesParameter, 0, sizeof(iAP2StopNowPlayingUpdatesParameter));

    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_STOP_NOWPLAYING_UPDATE,
                               &theiAP2StopNowPlayingUpdatesParameter, sizeof(theiAP2StopNowPlayingUpdatesParameter));
    }

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopNowPlayingUpdatesParameter(&theiAP2StopNowPlayingUpdatesParameter); */

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Sending iAP2PlayMediaLibraryItems to device */
LOCAL S32 iap2TestStartPlayMediaLibraryItem(S32 mq_fd, iAP2Device_t* iap2Device, U32 startIndex, U16 numSongsToPlay)
{
    int rc = IAP2_OK;
    U32 retry_count = 0;
    U16 tmpNumSongs = 0;
    U16 i = 0;
    U16 PersistendIdentifierCount = 0;
    U8* BlobDataLocation = NULL;
    iAP2PlayMediaLibraryItemsParameter theiAP2PlayMediaLibraryItemsParameter;

    iap2Device = iap2Device;

    memset(&theiAP2PlayMediaLibraryItemsParameter, 0, sizeof(iAP2PlayMediaLibraryItemsParameter));

    if((g_iap2TestDevice.testMediaItemCnt > 0)
        && (g_iap2TestDevice.testMediaItem != NULL))
    {
#ifdef IAP2_EVALUVATE_PLAYMEDIA_LIBRARY_ITEMS
        printf("\n\n Enter Total Number of songs to play :");
        rc = scanf("%hi", &tmpNumSongs);
        if(rc > 0)
        {
            rc = IAP2_OK;
        }
        (void)numSongsToPlay;
#else
        /* set number of songs to play */
        if(numSongsToPlay > g_iap2TestDevice.testMediaItemCnt)
        {
            tmpNumSongs = g_iap2TestDevice.testMediaItemCnt;
        }
        else
        {
            tmpNumSongs = numSongsToPlay;
        }
#endif
        /* get number of available persistent identifier */
        for(i = 0; i < tmpNumSongs; i++)
        {
            if( &g_iap2TestDevice.testMediaItem[i] != NULL )
            {
                if( g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0 )
                {
                    PersistendIdentifierCount++;
                }
            }
            else
            {
                printf( " %u ms  iap2TestStartPlayMediaLibraryItem(): g_iap2TestDevice.testMediaItem[%d] is NULL\n",
                        iap2CurrTimeMs(), i );
                rc = IAP2_CTL_ERROR;
                PersistendIdentifierCount = 0;
                break;
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
                rc = iap2AllocateandUpdateData(&theiAP2PlayMediaLibraryItemsParameter.ItemsStartingIndex,
                                               &startIndex,
                                               &theiAP2PlayMediaLibraryItemsParameter.ItemsStartingIndex_count,
                                               1, iAP2_uint32);
            }
            if(rc == IAP2_OK)
            {
                rc = iap2AllocateandUpdateData(&theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier,
                                               &g_iap2TestDevice.testMediaLibInfoID,
                                               &theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier_count,
                                               1, iAP2_utf8);
            }
            if(rc == IAP2_OK)
            {
                rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                       MQ_CMD_START_PLAY_MEDIALIB_ITEM,
                                       &theiAP2PlayMediaLibraryItemsParameter, sizeof(theiAP2PlayMediaLibraryItemsParameter));
#ifdef IAP2_EVALUVATE_PLAYMEDIA_LIBRARY_ITEMS
                g_iap2TestDevice.PlayMediLibraryItems_StartTime_ms = iap2CurrTimeMs();
#endif
            }

            iAP2FreeiAP2PlayMediaLibraryItemsParameter(&theiAP2PlayMediaLibraryItemsParameter);

            if(rc != IAP2_OK)
            {
                printf(" %u ms  iap2TestStartPlayMediaLibraryItem(): failed.  rc = %d \n",
                        iap2CurrTimeMs(), rc);
            }
            else
            {
#ifdef IAP2_EVALUVATE_PLAYMEDIA_LIBRARY_ITEMS
                iap2SetTestState( NOW_PLAYING_UPDATE_RECV, FALSE );
                /* wait for callback with requested information */
                while( (rc == IAP2_OK) && (iap2GetTestState(NOW_PLAYING_UPDATE_RECV) != TRUE)
                        &&(iap2GetTestStateError() != TRUE) && (retry_count < 30000) )
#else
                /* wait for callback with requested information */
                while( (rc == IAP2_OK) && (iap2GetTestState(NOW_PLAYING_UPDATE_RECV) != TRUE)
                        &&(iap2GetTestStateError() != TRUE) && (retry_count < 300) )
#endif
                {
                    iap2SleepMs(10);
                    retry_count++;
                }
                printf("\t iap2GetTestState = %d", iap2GetTestState(NOW_PLAYING_UPDATE_RECV) );
                /* received callback iap2NowPlayingUpdate_CB */
                if( iap2GetTestState( NOW_PLAYING_UPDATE_RECV ) == TRUE )
                {
                    iap2SetTestState( PLAY_MEDIA_LIB_ITEM, TRUE );
                    rc = IAP2_OK;
                }
                else
                {
                    printf( " %u ms  iap2TestStartPlayMediaLibraryItem(): failed. retry: %d \n",
                            iap2CurrTimeMs(), retry_count );
                    rc = IAP2_CTL_ERROR;
                }
            }
        }
        else
        {
            printf( " %u ms  iap2TestStartPlayMediaLibraryItem():  PersistendIdentifierCount is %d \n ",
                    iap2CurrTimeMs(),PersistendIdentifierCount );
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        printf( " %u ms  iap2TestStartPlayMediaLibraryItem(): No MediaItem available. \n", iap2CurrTimeMs() );
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
LOCAL S32 iap2TestStartUSBDeviceModeAudio(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U32 retry_count = 0;
    iAP2StartUSBDeviceModeAudioParameter theiAP2StartUSBDeviceModeAudioParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StartUSBDeviceModeAudioParameter, 0, sizeof(theiAP2StartUSBDeviceModeAudioParameter) );

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_START_USB_DEVICEMODE_AUDIO,
                           &theiAP2StartUSBDeviceModeAudioParameter, sizeof(theiAP2StartUSBDeviceModeAudioParameter));

    if(rc != IAP2_OK)
    {
        printf(" %u ms  iap2TestStartUSBDeviceModeAudio(): failed.  rc = %d \n",
                iap2CurrTimeMs(), rc);
    }
    else
    {
        /* wait for callback with requested information */
        while( (rc == IAP2_OK) && (iap2GetTestState(USB_DEVICE_MODE_AUDIO_INFO_RECV) != TRUE)
                &&(iap2GetTestStateError() != TRUE) && (retry_count < 500) )
        {
            if(iap2GetTestDeviceState() == iAP2NotConnected)
            {
                rc = IAP2_DEV_NOT_CONNECTED;
                break;
            }
            iap2SleepMs(10);
            retry_count++;
        }
        /* received callback iap2NowPlayingUpdate_CB */
        if(iap2GetTestState(USB_DEVICE_MODE_AUDIO_INFO_RECV) == TRUE)
        {
            rc = IAP2_OK;
        }
        else
        {
            printf(" %u ms  iap2TestStartUSBDeviceModeAudio(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
            rc = IAP2_CTL_ERROR;
            iap2SetGstState(IAP2_GSTREAMER_STATE_STOP);
            printf("Sending STOP Notification to gstreamer\n");
        }
    }

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
#endif

/* start HID command usage */
LOCAL S32 iap2TestStartHID(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U16 VendorIdentifier       = 0x01;
    U16 ProductIdentifier      = 0x02;
    iAP2StartHIDParameter theiAP2StartHIDParameter;
    const uint8_t HIDDescriptor[] = {
                                        0x05, 0x0C, // usage page
                                        0x09, 0x01, // usage
                                        0xA1, 0x01, // collection
                                        // Consumer
                                        0x15, 0x00, // logical min
                                        0x25, 0x01, // logical max
                                        0x75, 0x01, // report size
                                        0x95, 0x06, // report count
                                        0x09, 0xB0, // usage: Play
                                        0x09, 0xB1, // usage: Pause
                                        0x09, 0xB5, // usage: Scan Next Track
                                        0x09, 0xB6, // usage: scan previous track
                                        0x09, 0xE9, // usage: volume up
                                        0x09, 0xEA, // usage: volume down
                                        0x81, 0x02, // Input
                                        0x75, 0x02, // report size
                                        0x95, 0x01, // report count
                                        0x81, 0x03, // Input
                                        0xC0 // end collection
                                    };

    iap2Device = iap2Device;

    memset(&theiAP2StartHIDParameter, 0, sizeof(iAP2StartHIDParameter));
    rc = iap2AllocateandUpdateData(&theiAP2StartHIDParameter.iAP2HIDComponentIdentifier,
                                   &g_iap2TestDevice.HIDComponentIdentifier,
                                   &theiAP2StartHIDParameter.iAP2HIDComponentIdentifier_count,
                                   1, iAP2_uint16);
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartHIDParameter.iAP2VendorIdentifier,
                                       &VendorIdentifier,
                                       &theiAP2StartHIDParameter.iAP2VendorIdentifier_count,
                                       1, iAP2_uint16);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartHIDParameter.iAP2ProductIdentifier,
                                       &ProductIdentifier,
                                       &theiAP2StartHIDParameter.iAP2ProductIdentifier_count,
                                       1, iAP2_uint16);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartHIDParameter.iAP2HIDReportDescriptor,
                                       (void *)&HIDDescriptor,
                                       &theiAP2StartHIDParameter.iAP2HIDReportDescriptor_count,
                                       sizeof(HIDDescriptor), iAP2_blob);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_START_HID,
                               &theiAP2StartHIDParameter, sizeof(theiAP2StartHIDParameter));
    }
    iAP2FreeiAP2StartHIDParameter(&theiAP2StartHIDParameter);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
/* stop HID command usage */
LOCAL S32 iap2TestStopHID(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iAP2StopHIDParameter theiAP2StopHIDParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StopHIDParameter, 0, sizeof(iAP2StopHIDParameter));
    rc = iap2AllocateandUpdateData(&theiAP2StopHIDParameter.iAP2HIDComponentIdentifier,
                                   &g_iap2TestDevice.HIDComponentIdentifier,
                                   &theiAP2StopHIDParameter.iAP2HIDComponentIdentifier_count,
                                   1, iAP2_uint16);
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_STOP_HID,
                               &theiAP2StopHIDParameter, sizeof(theiAP2StopHIDParameter));
    }
    iAP2FreeiAP2StopHIDParameter(&theiAP2StopHIDParameter);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
/* send HID command to device */
LOCAL S32 iap2TestSendHIDReport(S32 mq_fd, iAP2Device_t* iap2Device, U8 report)
{
    S32 rc = IAP2_OK;
    iAP2AccessoryHIDReportParameter theiAP2AccessoryHIDReportParameter;

    iap2Device = iap2Device;

    memset(&theiAP2AccessoryHIDReportParameter, 0, sizeof(iAP2AccessoryHIDReportParameter));
    rc = iap2AllocateandUpdateData(&theiAP2AccessoryHIDReportParameter.iAP2HIDComponentIdentifier,
                                   &g_iap2TestDevice.HIDComponentIdentifier,
                                   &theiAP2AccessoryHIDReportParameter.iAP2HIDComponentIdentifier_count,
                                   1, iAP2_uint16);
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2AccessoryHIDReportParameter.iAP2HIDReport,
                                       &report,
                                       &theiAP2AccessoryHIDReportParameter.iAP2HIDReport_count,
                                       sizeof(report), iAP2_blob);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_SEND_HID_REPORT,
                               &theiAP2AccessoryHIDReportParameter, sizeof(theiAP2AccessoryHIDReportParameter));
    }
    iAP2FreeiAP2AccessoryHIDReportParameter(&(theiAP2AccessoryHIDReportParameter));

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* send Bad DETECT Ack to device to cancel iAP1 support */
LOCAL S32 iap2TestCanceliAP1Support(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
            MQ_CMD_CANCEL_IAP1_SUPPORT, NULL, 0);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* send an EAP message to device */
LOCAL S32 iap2TestSendEAPSessionMessage(S32 mq_fd, iAP2Device_t* iap2Device, U8* msg)
{
    S32 rc = IAP2_OK;
    iap2TestEAP_t eapMsg;

    iap2Device = iap2Device;

    eapMsg.EAPid = g_iap2TestDevice.EaProtocolID;
    eapMsg.EAPmsglen = (U32)strlen((const char*)msg);
    eapMsg.EAPmsg = msg;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_SEND_EAP_SESSION_MSG, &(eapMsg), sizeof(eapMsg));

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

#if IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST
/* send RequestAppLaunch to device */
LOCAL S32 iap2TestRequestAppLaunch(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iAP2RequestAppLaunchParameter theiAP2RequestAppLaunchParameter;

    iap2Device = iap2Device;

    memset(&theiAP2RequestAppLaunchParameter, 0, sizeof(iAP2RequestAppLaunchParameter));

    rc = iap2AllocateandUpdateData(&theiAP2RequestAppLaunchParameter.iAP2AppBundleID,
                                       &g_iap2TestDevice.AppBundleID,
                                       &theiAP2RequestAppLaunchParameter.iAP2AppBundleID_count,
                                       1, iAP2_utf8);
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_APP_LAUNCH,
                               &theiAP2RequestAppLaunchParameter, sizeof(theiAP2RequestAppLaunchParameter));
    }
    iAP2FreeiAP2RequestAppLaunchParameter(&theiAP2RequestAppLaunchParameter);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

LOCAL S32 iap2RequestAppLaunch(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    g_iap2TestDevice.AppBundleID = calloc(1, strlen(IAP2_IOS_BUNDLE_ID) + 1);
    if( g_iap2TestDevice.AppBundleID == NULL )
    {
        rc = IAP2_ERR_NO_MEM;
    }
    else
    {
        memset( g_iap2TestDevice.AppBundleID, 0, strlen( IAP2_IOS_BUNDLE_ID ) + 1 );
        strncpy( ( char* )g_iap2TestDevice.AppBundleID, IAP2_IOS_BUNDLE_ID, strlen( IAP2_IOS_BUNDLE_ID ) );
        printf( " %u ms  iap2TestRequestAppLaunch(%s) \n", iap2CurrTimeMs(), g_iap2TestDevice.AppBundleID );
        rc = iap2TestRequestAppLaunch( mq_fd, iap2Device );
        if( rc == IAP2_OK )
        {
            printf( "\n **** Please allow the communication with the iOS App. **** \n" );
            printf( " **** Select '%s' and open the iOS App **** \n", IAP2_ACC_INFO_NAME );
        }
        iap2TestFreePtr( (void**)&g_iap2TestDevice.AppBundleID);
    }

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
#endif

S32 iap2TestEAPSession(S32 mq_fd, iAP2Device_t* iap2Device);
S32 iap2TestEAPSession(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U8 testbuf[] = {"Hello, We are testing ExternalAccessoryProtocol Session!!! \n"};

    while((iap2GetTestState(EAP_SESSION_STOP_RECV) == FALSE)
          && (iap2GetGlobalQuit() == FALSE)
          && (iap2GetTestDeviceState() != iAP2NotConnected))
    {
        iap2SleepMs(50);
        if(iap2GetTestState(EAP_SESSION_START_RECV) == TRUE)
        {
            /* EAP session test */
            rc = iap2TestSendEAPSessionMessage(mq_fd, iap2Device, &testbuf[0]);
            printf(" iap2TestSendEAPSessionMessage(len: %zu) rc = %d \n",
                    strlen((const char*)testbuf), rc);

            iap2SetTestState(EAP_SESSION_START_RECV, FALSE);
        }
    }
    return rc;
}

S32 iap2TestEANativeTransport(iAP2Device_t* iap2Device);
S32 iap2TestEANativeTransport(iAP2Device_t* iap2Device)
{
    S32 rc              = IAP2_OK;
    U16 i               = 0;

    S32 write_fd        = -1;
    S32 read_fd         = -1;
    U8 writename[256]   = {"/dev/ffs/ep"};
    U8 readname[256]    = {"/dev/ffs/ep"};
    U32 len             = strlen("/dev/ffs/ep");
    size_t readBytes    = 11264;
    U8 readBuf[11264]   = {0};
    U8 testbuf[]        = {"Hello, We are testing ExternalAccessory via Native Transport!!! \n"};

    /* temporary to avoid compiler warnings */
    iap2Device = iap2Device;

    /* sleep, because gadget_fs send enable event if g_ffs module loaded */
//    iap2SleepMs(1000);
    /* ignore the first iap2StopEANativeTransport_CB */
    iap2SetTestState(EA_NATIVE_TRANSPORT_STOP_RECV, FALSE);

    while((iap2GetTestState(EA_NATIVE_TRANSPORT_STOP_RECV) == FALSE)
           && (iap2GetGlobalQuit() == FALSE) && (rc == IAP2_OK)
           && (iap2GetTestDeviceState() != iAP2NotConnected))
    {
        iap2SleepMs(50);
        if(iap2GetTestState(EA_NATIVE_TRANSPORT_START_RECV) == TRUE)
        {
            snprintf((char*)&writename[len], len +1,
                     "%d", g_iap2TestDevice.SinkEndpoint);

            snprintf((char*)&readname[len], len +1,
                     "%d", g_iap2TestDevice.SourceEndpoint);

            write_fd = open((const char*)&writename[0], O_NONBLOCK | O_WRONLY);
            read_fd  = open((const char*)&readname[0], O_NONBLOCK | O_RDONLY);
            if( ( write_fd >= 0 ) && ( read_fd >= 0 ) )
            {
                rc = write( write_fd, ( void* )&testbuf[0], strlen( ( const char* )&testbuf[0] ) );
                printf( " %u ms  Send message (len: %zu) to iOS App  rc = %d \n",
                        iap2CurrTimeMs(), strlen( ( const char* )&testbuf[0] ), rc );
                if( rc == ( S32 )strlen( ( const char* )&testbuf[0] ) )
                {
                    while( ( rc > 0 ) && ( iap2GetGlobalQuit() == FALSE )
                            && ( iap2GetTestState( EA_NATIVE_TRANSPORT_STOP_RECV ) == FALSE )
                            && ( iap2GetTestDeviceState() != iAP2NotConnected ) )
                    {
                        rc = read( read_fd, &readBuf[0], readBytes );
                        printf( " read = %d \n", rc );
                        if( rc > 0 )
                        {
                            for( i=0; i<rc; i++ )
                            {
                                if( ( i % 10 ) == 0 )
                                {
                                    printf( "\n" );
                                }
                                printf( "    0x%.2X, ", readBuf[i] );
                            }
                            printf( "\n" );
                            memset( &readBuf[0], 0, readBytes );
                        }
                        else
                        {
                            printf( " %u ms  read = %d | errno: %s\n",
                                    iap2CurrTimeMs(), rc, strerror( errno ) );
                        }
                    }
                    rc = IAP2_OK;
                }
                else
                {
                    printf( " %u ms  Send message = rc: %d | errno: %s\n",
                            iap2CurrTimeMs(), rc, strerror( errno ) );
                    rc = IAP2_CTL_ERROR;
                    iap2SetGlobalQuit( TRUE );
                }
            }
            else
            {
                rc = IAP2_CTL_ERROR;
                iap2SetGlobalQuit( TRUE );
            }
        }
    }
    if( iap2GetTestState( EA_NATIVE_TRANSPORT_STOP_RECV ) == TRUE )
    {
        printf( " %u ms  iOS App %d closed \n", iap2CurrTimeMs(), g_iap2TestDevice.EANativeTransportAppId );
    }
    if( write_fd >= 0 )
    {
        rc = close( write_fd );
    }
    if( read_fd >= 0 )
    {
        rc = close( read_fd );
    }

    return rc;
}

S32 iap2HdlComThreadPollMqEvent_CB(iAP2Device_t* iap2Device, S32 mqFD, S32 mqFdSendAck, BOOL* b_endComThread)
{
    S32 rc = IAP2_OK;
    char recvBuf[TEST_MQ_MAX_SIZE];

    char ack_mq_cmd = MQ_CMD_ACK;

    iap2ComThreadMq_t *mq_st = NULL;

    /* receive message from AppThread */
    rc = iap2RecvMq(mqFD, &recvBuf[0], TEST_MQ_MAX_SIZE);
    if(rc > 0)
    {
        mq_st = (iap2ComThreadMq_t*)recvBuf;
        if(mq_st == NULL){
            ack_mq_cmd = MQ_CMD_ERROR;
            rc = IAP2_CTL_ERROR;
            printf("\t %u ms  iap2HdlComThreadPollMqEvent():  mq_st is NULL \n", iap2CurrTimeMs());
        } else{
            switch(mq_st->mq_cmd)
            {
                case MQ_CMD_EXIT_IAP2_COM_THREAD:
                {
                    printf(" %u ms  iap2HdlComThreadPollMqEvent():  leave poll thread\n", iap2CurrTimeMs());
                    *b_endComThread = TRUE;
                    break;
                }
                case MQ_CMD_START_NOWPLAYING_UPDATE:
                {
                    /* Sending iAP2StartNowPlayingUpdates to device */
                    rc = iAP2StartNowPlayingUpdates(iap2Device, (iAP2StartNowPlayingUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StartNowPlayingUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_STOP_NOWPLAYING_UPDATE:
                {
                    /* Sending iap2TestStopNowPlayingUpdate to device */
                    rc = iAP2StopNowPlayingUpdates(iap2Device, (iAP2StopNowPlayingUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StopNowPlayingUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_START_MEDIALIB_INFO:
                {
                    /* Sending iAP2StartMediaLibraryInformation to device */
                    rc = iAP2StartMediaLibraryInformation(iap2Device, (iAP2StartMediaLibraryInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StartMediaLibraryInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_STOP_MEDIALIB_INFO:
                {
                    /* Sending iAP2StopMediaLibraryInformation to device */
                    rc = iAP2StopMediaLibraryInformation(iap2Device, (iAP2StopMediaLibraryInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StopMediaLibraryInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_START_MEDIALIB_UPDATE:
                {
                    /* start MediaLibraryUpdates */
                    rc = iAP2StartMediaLibraryUpdates(iap2Device, (iAP2StartMediaLibraryUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StartMediaLibraryUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_STOP_MEDIALIB_UPDATE:
                {
                    /* stop MediaLibraryUpdates */
                    rc = iAP2StopMediaLibraryUpdates(iap2Device, (iAP2StopMediaLibraryUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StopMediaLibraryUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_START_HID:
                {
                    /* start HID */
                    rc = iAP2StartHID(iap2Device, (iAP2StartHIDParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StartHID failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_STOP_HID:
                {
                    /* stop HID */
                    rc = iAP2StopHID(iap2Device, (iAP2StopHIDParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StopHID failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_SEND_HID_REPORT:
                {
                    /* send HID report to device */
                    rc = iAP2AccessoryHIDReport(iap2Device, (iAP2AccessoryHIDReportParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2AccessoryHIDReport failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_START_USB_DEVICEMODE_AUDIO:
                {
                    /* send start USB device mode audio to device */
                    rc = iAP2StartUSBDeviceModeAudio(iap2Device, (iAP2StartUSBDeviceModeAudioParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StartUSBDeviceModeAudio failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_START_PLAY_MEDIALIB_ITEM:
                {
                    /* start playback */
                    rc = iAP2PlayMediaLibraryItems(iap2Device, (iAP2PlayMediaLibraryItemsParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2PlayMediaLibraryItems failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_SEND_EAP_SESSION_MSG:
                {
                    iap2TestEAP_t* param = (iap2TestEAP_t*)mq_st->param;

                    /* send EAP message */
                    rc = iAP2SendEAPSessionMessage(iap2Device, (U8*)param->EAPmsg, param->EAPmsglen, param->EAPid);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2SendEAPSessionMessage failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_CANCEL_IAP1_SUPPORT:
                {
                    /* send CanceliAP1Support */
                    rc = iAP2CanceliAP1Support(iap2Device);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2CanceliAP1Support failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_APP_LAUNCH:
                {
                    /* send RequestAppLaunch */
                    rc = iAP2RequestAppLaunch(iap2Device, (iAP2RequestAppLaunchParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2RequestAppLaunch failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                default:
                {
                    ack_mq_cmd = MQ_CMD_ERROR;
                    rc = IAP2_CTL_ERROR;
                    break;
                }
            }/* switch */

            if(rc != IAP2_OK){
                /* set testStateError to TRUE */
                iap2SetTestStateError(TRUE);
            }
        }
    }
    else
    {
        ack_mq_cmd = MQ_CMD_ERROR;
        rc = IAP2_CTL_ERROR;
    }

    if(mqFdSendAck > 0){
        (void)iap2SendMq(mqFdSendAck, &ack_mq_cmd, sizeof(ack_mq_cmd));
    }

    return rc;
}

LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2device)
{
	S32 rc = IAP2_OK;
	U32 retry_count = 0;

    if((rc == IAP2_OK) && (iap2GetTestStateError() != TRUE))
    {
        if(iap2GetEAtesting() == TRUE)
        {
#if IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST
            rc = iap2RequestAppLaunch(mq_fd, iap2device);
            printf("\n %u ms  iap2RequestAppLaunch()  rc = %d\n", iap2CurrTimeMs(), rc);
#endif
            if(rc == IAP2_OK)
            {
                if(g_iap2UserConfig.iap2EAPSupported == TRUE)
                {
                    rc = iap2TestEAPSession(mq_fd, iap2device);
                    printf("\n %u ms  iap2TestEAPSession()  rc = %d\n", iap2CurrTimeMs(), rc);
                }
                else if(g_iap2UserConfig.iap2EANativeTransport == TRUE)
                {
                    rc = iap2TestEANativeTransport(iap2device);
                    printf("\n %u ms  iap2TestEANativeTransport()  rc = %d\n", iap2CurrTimeMs(), rc);
                }
                else
                {
                    printf(" %u ms  No EA transport type (EAP | Native Transport) selected. \n",
                            iap2CurrTimeMs());
                    rc = IAP2_CTL_ERROR;
                }
            }
        }
        else
        {
            if(iap2GetTestStateError() != TRUE)
            {
                /* start NowPlayingUpdates */
                printf("\n %u ms  iap2TestStartNowPlayingUpdate \n", iap2CurrTimeMs());
                rc = iap2TestStartNowPlayingUpdate(mq_fd, iap2device);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* start HID report */
                printf("\n %u ms  iap2TestStartHID() \n", iap2CurrTimeMs());
                rc = iap2TestStartHID(mq_fd, iap2device);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* start MediaLibraryInfo */
                printf("\n %u ms  iap2TestStartMediaLibInfo() \n", iap2CurrTimeMs());
                rc = iap2TestStartMediaLibInfo(mq_fd, iap2device);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* start MediaLibrarayUpdate */
                printf("\n %u ms  iap2TestStartMediaLibUpdate() \n", iap2CurrTimeMs());
                rc = iap2TestStartMediaLibUpdate(mq_fd, iap2device);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                if(g_iap2TestDevice.testMediaLibUpdateProgress == 100)
                {
                    /* Use PlayMediaLibraryItem if MediaLibUpdate return MediaLibraryItem */
                    printf("\n %u ms  iap2TestStartPlayMediaLibraryItem() \n", iap2CurrTimeMs());
                    rc = iap2TestStartPlayMediaLibraryItem(mq_fd, iap2device, 1, 10);
                } else{ // TBDO: only in device mode ??
                    /* Use SendHIDReport(Play) if MediaLibUpdate does not return MediaLibraryItem */
                    printf("\n %u ms  iap2TestSendHIDReport(Play) \n", iap2CurrTimeMs());
                    rc = iap2TestSendHIDReport(mq_fd, iap2device, 1); /*Press*/
                    (void)iap2TestSendHIDReport(mq_fd, iap2device, 0); /*Release*/
                }
                if(rc == IAP2_OK)
                    iap2SleepMs(1000); /*Play for a while*/
            }

#if IAP2_GST_AUDIO_STREAM
            if( (iap2GetTestStateError() != TRUE)
                && (g_iap2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE) )
            {
                /* start USBDeviceModeAudio */
                printf("\n %u ms  iap2TestStartUSBDeviceModeAudio() \n", iap2CurrTimeMs());
                rc = iap2TestStartUSBDeviceModeAudio(mq_fd, iap2device);
            }
            if(rc == IAP2_OK)
                iap2SleepMs(4000); /*Play for a while*/
#endif

            if(iap2GetTestStateError() != TRUE)
            {
                /* send some HID reports to Apple device */
                printf("\n %u ms  iap2TestSendHIDReport(Pause) \n", iap2CurrTimeMs());
                rc = iap2TestSendHIDReport(mq_fd, iap2device, 2); /*Pause*/
                (void)iap2TestSendHIDReport(mq_fd, iap2device, 0);
                if(rc == IAP2_OK)
                    iap2SleepMs(5000);
            }

            if(iap2GetTestStateError() != TRUE)
            {
                /* send some HID reports to Apple device */
                printf("\n %u ms  iap2TestSendHIDReport(Play) \n", iap2CurrTimeMs());
                rc = iap2TestSendHIDReport(mq_fd, iap2device, 1); /*Play*/
                (void)iap2TestSendHIDReport(mq_fd, iap2device, 0);
            }

#if IAP2_GST_AUDIO_STREAM
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

            /* Check the progress of MediaLibUpdate */
            if(iap2GetTestStateError() != TRUE)
            {
                rc = iap2MediaItemDbWaitForUpdate(5000, TRUE);
            }

            /* stop playback at end of load_test */
            printf("\n %u ms  iap2TestSendHIDReport(Pause) \n", iap2CurrTimeMs());
            rc = iap2TestSendHIDReport(mq_fd, iap2device, 2); /*Pause*/
            (void)iap2TestSendHIDReport(mq_fd, iap2device, 0);

            if(g_iap2TestDevice.testMediaLibInfoID != NULL)
            {
                /* check if Apple device was disconnected */
                if( iap2GetTestDeviceState() == iAP2NotConnected )
                {
                    printf( "\n %u ms  Apple device was disconnected! \n", iap2CurrTimeMs() );
                }
                else
                {
                    /* stop NowPlayingUpdates */
                    printf( "\n %u ms  iap2TestStopNowPlayingUpdate() \n", iap2CurrTimeMs() );
                    rc = iap2TestStopNowPlayingUpdate( mq_fd, iap2device );
                    /* stop MediaLibraryUpdate */
                    printf( "\n %u ms  iap2TestStopMediaLibUpdate() \n", iap2CurrTimeMs() );
                    rc = iap2TestStopMediaLibUpdate( mq_fd, iap2device );
                    /* stop MediaLibraryInfo */
                    printf( "\n %u ms  iap2TestStopMediaLibInfo() \n", iap2CurrTimeMs() );
                    rc = iap2TestStopMediaLibInfo( mq_fd, iap2device );
                    /* stop HID report */
                    printf( "\n %u ms  iap2TestStopHID() \n", iap2CurrTimeMs() );
                    rc = iap2TestStopHID( mq_fd, iap2device );
                    iap2SleepMs( 1000 );
                }
            }
        }
    }
    return rc;
}

/* application thread */
LOCAL void iap2AppThread(void* exinf)
{
    S32 rc = IAP2_OK;
    S32 rc_tmp;

    TEST_THREAD_ID threadID = 0;
    char threadName[8];
    void* status;
    mqd_t mq_fd = -1;
    U16 i = 0;

    U32 retry_count = 0;

    iap2AppThreadInitData_t* iAP2AppThreadInitData = (iap2AppThreadInitData_t*)exinf;
    iAP2InitParam_t* iAP2InitParameter = &(iAP2AppThreadInitData->iAP2InitParameter);
    iAP2Device_t* iap2device = NULL;
    int socket_fd = iAP2AppThreadInitData->socket_fd;

    /* create mutex to protect MediaLib access */
    pthread_mutex_init(&g_iap2TestDevice.testMediaLibMutex, NULL);

    g_iap2TestDevice.testMediaLibUpdateProgress = 0;

    rc = iap2CreateMq(&mq_fd, TEST_MQ_NAME, O_CREAT | O_RDWR);
    if(rc != IAP2_OK)
    {
        printf("  create mq %s failed %d \n", TEST_MQ_NAME, rc);
    }

    if(rc == IAP2_OK)
    {
        rc = iap2CreateMq(&g_iap2TestDevice.mqAppTskFd, TEST_MQ_NAME_APP_TSK, O_CREAT | O_RDWR);
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
        if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
           &&(rc == IAP2_OK))
        {
            rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_HOST, socket_fd,
            iAP2AppThreadInitData->udevPath, iAP2InitParameter);
        }

        if(rc  == IAP2_OK)
        {
            rc = iAP2InitDeviceConnection(iap2device);
            printf(" %u ms  iAP2InitDeviceConnection:   rc  = %d \n", iap2CurrTimeMs(), rc);
            IAP2TESTDLTLOG(DLT_LOG_DEBUG,"iAP2InitDeviceConnection rc = %d ",rc);
        }

        if(rc == IAP2_OK)
        {
            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iCom", 1);

            /* -----------  start polling thread  ----------- */
            threadID = iap2CreateThread(iap2ComThread, &threadName[0], iap2device);
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
            while( (g_iap2TestDevice.testDeviceState    != iAP2DeviceReady)
                   && (g_iap2TestDevice.testDeviceState != iAP2LinkiAP1DeviceDetected)
                   && (g_iap2TestDevice.testDeviceState != iAP2ComError)
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

                printf(" iAP1 not supported. send Bad DETECT Ack to device \n");
                rc = iap2TestCanceliAP1Support(mq_fd, iap2device);

                iap2SetTestStateError(TRUE);
            }
            else if(g_iap2TestDevice.testDeviceState == iAP2ComError)
            {
                printf(" %u ms  Error in Device Authentication or Identification  retry: %d \n", iap2CurrTimeMs(), retry_count);
                iap2SetTestStateError(TRUE);
                rc = IAP2_CTL_ERROR;
            }
            else
            {
                printf(" %u ms  device not attached [state: %d | retry: %d] \n", iap2CurrTimeMs(), g_iap2TestDevice.testDeviceState, retry_count);
                iap2SetTestStateError(TRUE);
                rc = IAP2_CTL_ERROR;
            }
        }

        if(rc == IAP2_OK)
        {
            rc= iap2StartTest(mq_fd,iap2device);
        }

        if( (g_rc == IAP2_OK) && (rc != IAP2_OK) )
        {
            g_rc = rc;
        }

        /* Indicate that no iAP2 library callbacks shall be handled.  */
        iap2SetTestState(STOPPED, TRUE);
        iap2SetTestState(RUNNING, FALSE);

#if IAP2_GST_AUDIO_STREAM
        /* stop gstreamer main loop */
        iap2SetGstState(IAP2_GSTREAMER_STATE_STOP);
#endif
        /* exit pollThread */
        if(threadID > 0)
        {
            (void)iap2TestStopPollThread(mq_fd, iap2device);
        }

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

        /* lock mutex to avoid MediaLibraryUpdates_CB while free .testMediaItem */
        pthread_mutex_lock(&g_iap2TestDevice.testMediaLibMutex);
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
            g_iap2TestDevice.testMediaItemCnt = 0;
        }
        pthread_mutex_unlock(&g_iap2TestDevice.testMediaLibMutex);

        if(g_iap2TestDevice.playbackQueueList.Buffer != NULL)
        {
#if IAP2_ENABLE_PLAYBACK_QUEUE_LIST_PRINTING
            if(g_iap2TestDevice.playbackQueueList.transferred == TRUE)
            {
                U64 i;
                U64* sourcebuf;

                for(i = 0; i < g_iap2TestDevice.playbackQueueList.Size; i += sizeof(U64))
                {
                    sourcebuf = (U64*)&(g_iap2TestDevice.playbackQueueList.Buffer[i]);
                    printf("PlaybackQueueList - PersistentIdentifier %lld\n", be64toh(*sourcebuf));
                }
            }
#endif
            iap2TestFreePtr( (void**)&g_iap2TestDevice.playbackQueueList.Buffer);
            memset(&g_iap2TestDevice.playbackQueueList, 0 , sizeof(iap2PlaybackQueueList_t) );

            g_iap2TestDevice.playbackQueueList.transferred = FALSE;
        }

        rc = iAP2DisconnectDevice(iap2device);
        printf(" %u ms  iAP2DisconnectDevice:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DisconnectDevice = %d", rc);

        if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
           &&(rc == IAP2_OK))
        {
            rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_DEVICE, socket_fd,
            iAP2AppThreadInitData->udevPath, iAP2InitParameter);
        }

        /* de-initialize the device structure */
        rc_tmp = iAP2DeInitDeviceStructure(iap2device);
        printf(" %u ms  iAP2DeInitDeviceStructure:   rc  = %d \n", iap2CurrTimeMs(), rc_tmp);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DeInitDeviceStructure = %d ", rc);
        if (rc == IAP2_OK) {
            rc = rc_tmp;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    if( (g_rc == IAP2_OK) && (rc != IAP2_OK) )
    {
        g_rc = rc;
    }

    /* destroy mutex */
    pthread_mutex_destroy(&g_iap2TestDevice.testMediaLibMutex);

    printf(" %u ms  exit iap2AppThread \n", iap2CurrTimeMs());
    pthread_exit((void*)exinf);
}

LOCAL S32 application_process_main (int socket_fd)
{
    S32 rc = IAP2_CTL_ERROR;

    TEST_THREAD_ID AppTskID = 0;
    char threadName[8];
    void* status;

#if IAP2_GST_AUDIO_STREAM
    TEST_THREAD_ID GstTskID = 0;
    U16 retry = 0;
#endif

    iap2AppThreadInitData_t iAP2AppThreadInitData;
    iAP2InitParam_t *iAP2InitParameter = &iAP2AppThreadInitData.iAP2InitParameter;

    iAP2AppThreadInitData.socket_fd = socket_fd;

    signal(SIGINT, signalHandlerAppProcess);
    signal(SIGQUIT, signalHandlerAppProcess);

    IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
    IAP2REGISTERCTXTWITHDLT();

    memset(iAP2InitParameter, 0, sizeof(iAP2InitParam_t) );

    rc = iap2InitDev(iAP2InitParameter);


    if(rc == IAP2_OK)
    {
        /* Application provided Data */
        rc = iap2SetInitialParameter(iAP2InitParameter, g_iap2UserConfig);
        printf(" iap2SetInitialParameter:  rc  = %d \n",rc);
    }
    if(rc == IAP2_OK)
    {
        /* copy pointer to udev path of testing device */
        iAP2AppThreadInitData.udevPath = g_iap2TestDevice.udevDevice.udevPath;

        /* set thread name */
        memset(&threadName[0], 0, (sizeof(threadName)));
        sprintf(&threadName[0], "%s%d", "iApp", 1);

        /* -----------  start application thread  ----------- */
        AppTskID = iap2CreateThread(iap2AppThread, &threadName[0], &iAP2AppThreadInitData);
        if(iap2VerifyThreadId(AppTskID) != IAP2_OK)
        {
            rc = IAP2_CTL_ERROR;
            printf("  create thread %s failed %d \n", threadName, rc);
        }

#if IAP2_GST_AUDIO_STREAM
        if((rc == IAP2_OK) && (iap2GetEAtesting() == FALSE))
        {
            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iGst", 1);

            iap2SetExitGstThread(FALSE);

            /* -----------  start gstreamer thread  ----------- */
            GstTskID = iap2CreateThread(iap2GstThread, &threadName[0], g_iap2TestDevice.udevDevice.productName);
            if(iap2VerifyThreadId(GstTskID) != IAP2_OK)
            {
                rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
            else
            {
                /* iap2GstThread need some time for initialization */
                iap2SleepMs(1000);
                iap2SetGstState(IAP2_GSTREAMER_STATE_INITIALIZE);
            }
            /* wait until GStreamer is initialized */
            while( (iap2GetGstState() != IAP2_GSTREAMER_INITIALIZED)
                   && (retry < 60) && (iap2GetGlobalQuit() == FALSE) )
            {
                iap2SleepMs(1000);
                retry++;
            }
            if(retry >= 60){
                printf(" %u ms Timeout. GStreamer is not initialized within 60 seconds\n", iap2CurrTimeMs());
                rc = IAP2_CTL_ERROR;
            }
        }
#endif

        /* -----------  clean thread  ----------- */
        if(AppTskID > 0)
        {
            (void)pthread_join(AppTskID, &status);
        }
#if IAP2_GST_AUDIO_STREAM
        if((GstTskID > 0) && (iap2GetEAtesting() == FALSE))
        {
            /* leave gstreamer while-loop */
            iap2SetExitGstThread(TRUE);
            iap2SetGstState(IAP2_GSTREAMER_STATE_DEINITIALIZE);

            (void)pthread_join(GstTskID, &status);
        }

        if((rc != IAP2_OK) && (g_rc == IAP2_OK))
        {
            g_rc = rc;
        }
#endif
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2Udev failed with %d \n", rc);
        g_rc = rc;
    }
    iap2DeinitDev(iAP2InitParameter);

    printf("\n");
    if(g_rc == IAP2_OK)
    {
        printf("iap2_load_test                 PASS\n");
        IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 LOAD_TEST PASS *****");
    }
    else
    {
        printf("iap2_load_test                 FAIL       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 LOAD_TEST FAIL *****");
    }

    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return g_rc;
}

S32 main(int argc, const char** argv)
{
    S32 rc;
    int socket_fds[2];
    static const int host_device_switch_process = 0;
    static const int application_process = 1;
    pid_t host_device_switch_process_pid;

    memset(&g_iap2TestDevice, 0, sizeof(g_iap2TestDevice));
    memset(&g_iap2UserConfig, 0, sizeof(g_iap2UserConfig));

    SetGlobPtr(&g_iap2TestDevice);

    /**
     * get user configuration. iap2GetArguments() gets runtime user inputs and
     * stores in global g_iap2UserConfig Structure.
     */
    rc = iap2GetArguments(argc, argv, &g_iap2UserConfig);

    if (rc == IAP2_OK) {
        socketpair(PF_LOCAL, SOCK_STREAM, 0, socket_fds);

        host_device_switch_process_pid = fork();
        if (host_device_switch_process_pid == 0) {
            close(socket_fds[application_process]);

            host_device_switch_process_main(socket_fds[host_device_switch_process],&g_iap2UserConfig);
            close(socket_fds[host_device_switch_process]);
            rc = 0;
        } else {
            close(socket_fds[host_device_switch_process]);

            if (g_iap2UserConfig.app_thread_user != 0) {
                printf ("Setting app thread user %d, group %d and %d supplementary groups\n", g_iap2UserConfig.app_thread_user, g_iap2UserConfig.app_thread_prim_group, g_iap2UserConfig.app_thread_groups_cnt);

                setgroups( g_iap2UserConfig.app_thread_groups_cnt, g_iap2UserConfig.app_thread_groups );
                if(setgid( g_iap2UserConfig.app_thread_prim_group ) < 0)
                {
                    printf("setgid() failed %s\n", strerror(errno));
                }
                if(setuid( g_iap2UserConfig.app_thread_user ) < 0)
                {
                    printf("setuid() failed %s\n", strerror(errno));
                }

                /* don't ask why */
                prctl (PR_SET_DUMPABLE, 1);
            }

            rc = application_process_main(socket_fds[application_process]);
            close(socket_fds[application_process]);
        }
    }

    iap2TestFreePtr( (void**)&g_iap2UserConfig.app_thread_groups);

    return rc;
}
