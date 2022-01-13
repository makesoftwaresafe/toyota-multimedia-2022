/*
 * iap2_endurance_test.c
 */

/* **********************  includes  ********************** */
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>

#include "iap2_endurance_test.h"
#include "iap2_test_gstreamer.h"
#include "iap2_usb_role_switch.h"

#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include <iap2_external_accessory_protocol_session.h>
#include <endian.h>
#include "iap2_dlt_log.h"

typedef struct
{
    int socket_fd;
    iAP2InitParam_t iAP2InitParameter;
    U8* udevPath;
} iap2AppThreadInitData_t;


LOCAL S32 application_process_main (int socket_fd);
LOCAL void iap2AppThread(void* exinf);
LOCAL int iap2GetArgument(int argc, const char** argv);
/* test functions */
LOCAL S32 iap2TestStopPollThread(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartPlayMediaLibraryItem(S32 mq_fd, iAP2Device_t* iap2Device, U32 startIndex, U16 numSongsToPlay);
LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2Device);
#if IAP2_GST_AUDIO_STREAM
LOCAL S32 iap2TestStartUSBDeviceModeAudio(S32 mq_fd, iAP2Device_t* iap2Device);
#endif
LOCAL S32 iap2TestStartHID(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopHID(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestSendHIDReport(S32 mq_fd, iAP2Device_t* iap2Device, U8 report);
#if IAP2_ENABLE_FILE_TRANSFER_CANCEL_PAUSE_CMD
LOCAL S32 iap2TestCancelFileTransfer(S32 mq_fd, iAP2Device_t* iap2Device, U8 fileId);
LOCAL S32 iap2TestPauseFileTransfer(S32 mq_fd, iAP2Device_t* iap2Device, U8 fileId);
LOCAL S32 iap2TestResumeFileTransfer(S32 mq_fd, iAP2Device_t* iap2Device, U8 fileId);
#endif
LOCAL S32 iap2TestCanceliAP1Support(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestSendEAPSessionMessage(S32 mq_fd, iAP2Device_t* iap2Device, U8* msg);
#if IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST
LOCAL S32 iap2TestRequestAppLaunch(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2RequestAppLaunch(S32 mq_fd, iAP2Device_t* iap2Device);
#endif
LOCAL S32 iap2TestPowerSourceUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartPowerUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopPowerUpdates(S32 mq_fd, iAP2Device_t* iap2Device);

LOCAL S32 iap2MediaItemDbWaitForUpdate(U32 waitTime, BOOL waitToFinish);
LOCAL S32 iap2MediaItemDbCleanUp(U8 updateProgress);
LOCAL S32 iap2MediaItemDbUpdate(iAP2MediaLibraryUpdateParameter* MediaLibraryUpdateParameter);


S32 g_rc = IAP2_OK;
U32 g_loopcount = 0;
iap2UserConfig_t g_iap2UserConfig;
iap2TestAppleDevice_t g_iap2TestDevice;
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
            if(iAP2GetDeviceErrorState(iap2Device, NULL) != iAP2IdentificationFailed)
            {
                rc = IAP2_CTL_ERROR;
            }
            break;
    }
    if(iAP2GetDeviceErrorState(iap2Device, NULL) != iAP2IdentificationFailed)
    {
        iap2SetTestDeviceState(dState);
    }

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
    S32 rc = IAP2_OK;

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
    if (powerupdateParameter->iAP2IsExternalChargerConnected_count != 0)
    {
        printf("\t %u ms %p  iap2PowerUpdate_CB ExternalChargerConnected\n", iap2CurrTimeMs(), iap2Device);
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2BatteryChargingState_count != 0)
    {
        printf("\t %u ms %p  iap2PowerUpdate_CB BatteryChargingState: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2BatteryChargingState));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2BatteryChargeLevel_count != 0)
    {
        printf("\t %u ms %p  iap2PowerUpdate_CB BatteryChargeLevel: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2BatteryChargeLevel));
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
    if (g_iap2TestDevice.testMediaLibInfoID == NULL)
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

    if(MediaLibraryUpdateParameter->iAP2PlayAllSongsCapable_count == 1)
    {
        g_iap2TestDevice.iap2PlayAllSongsCapable = TRUE;
        printf("\t %u ms %p  Device is PlayAllSongsCapable \n", iap2CurrTimeMs(), iap2Device);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Device is PlayAllSongsCapable");
    }
    /* MediaLibraryUpdate for same MediaLibrary */
    if(strncmp((char*)g_iap2TestDevice.testMediaLibInfoID,
               (char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier), size) == 0)
    {
        /* get percentage completion for current set of MediaLibraryUpdates */
        if((MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0)
           &&(g_iap2TestDevice.testMediaLibUpdateProgress < 100))
        {
            printf("\t    MediaLibUpdateProgress = %d / 100 \n", *(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress));

            if(MediaLibraryUpdateParameter->iAP2MediaItem_count > 0){
                printf("\t    No. of received MediaItems:  %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count);

                callTime = iap2CurrTimeMs();
                rc = iap2MediaItemDbUpdate(MediaLibraryUpdateParameter);
                printf("\t    iap2MediaItemDbUpdate() takes %u ms \n", (iap2CurrTimeMs() - callTime));

                if(rc > IAP2_OK){
                    g_iap2TestDevice.tmpMediaItemCnt += rc;
                    rc = IAP2_OK;
                }
            }
            if(MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier_count > 0){
                printf("\t    iAP2MediaItemDeletePersistentIdentifier_count  %d \n",
                        MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier_count);
            }

            if(*(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress) == 100){
                printf("\n Current number of songs: %d \n", g_iap2TestDevice.tmpMediaItemCnt);
            }
            g_iap2TestDevice.testMediaLibUpdateProgress = *(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress);
        } else{
            /* MediaLibraryUpdates because of playback change */
            for(i=0; ( (i < MediaLibraryUpdateParameter->iAP2MediaItem_count) && (rc == IAP2_OK) ); i++)
            {
                if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
                {
                    printf("\t    iAP2MediaItemPersistentIdentifier:  %llu \n",
                            *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier));
                }
                if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0)
                {
                    printf("\t    iAP2MediaItemTitle:  %s \n",
                            *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle));
                }
                if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0)
                {
                    printf("\t    iAP2MediaItemGenre:  %s \n",
                        *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre));
                }
            }
        }
    }
    else
    {
        /* received MediaLibraryUpdate for a different MediaLibrary */
        printf("\n\t    received MediaLibraryUpdate for a different MediaLibraryID \n");
        /* get percentage completion for current set of MediaLibraryUpdates */
        if(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0)
        {
            g_iap2TestDevice.testMediaLibUpdateProgress = *(MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress);
            printf("\t    MediaLibUpdateProgress = %d / 100 \n", g_iap2TestDevice.testMediaLibUpdateProgress);
        }

        printf("\t    Number of songs received from Apple device: %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count);
    }

    iap2SetTestState(MEDIA_LIB_UPDATE_RECV, TRUE);
    return rc;
}

S32 iap2NowPlayingUpdate_CB(iAP2Device_t* iap2Device, iAP2NowPlayingUpdateParameter* NowPlayingUpdateParameter, void* context)
{
    S32 rc = IAP2_OK;
    S16 i = 0;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2NowPlayingUpdate_CB called \n");
    printf("\t %u ms %p  iap2NowPlayingUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    /* MFi R15: NowPlayingUpdate::MediaItemAttributes can occur 0/1. */
    if(NowPlayingUpdateParameter->iAP2MediaItemAttributes_count > 0)
    {
        if(NULL != NowPlayingUpdateParameter->iAP2MediaItemAttributes)
        {
            /* MFi R15: Each parameter of iAP2MediaItemAttributes can occur 0/1.
             * Expect parameter iAP2MediaItemMediaType (0+). */
            if(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle_count > 0)
            {
                printf("\t    now playing title:  %s \n",
                        *(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle));
            }
            if(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPlaybackDurationInMilliseconds_count > 0)
            {
                printf("\t    CurrentSongTotalDuration:  %d \n",
                        *(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPlaybackDurationInMilliseconds));
            }
            if(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemAlbumTrackNumber_count > 0)
            {
                printf("\t    Track Changed Notification:  %d \n",
                        *(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemAlbumTrackNumber));
            }
            if(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPersistentIdentifier_count > 0)
            {
                printf("\t    MediaItemPersistentIdentifier:  %lld \n",
                        *(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i].iAP2MediaItemPersistentIdentifier));
            }
        }
        else
        {
            printf("\t %u ms %p  iap2NowPlayingUpdate_CB():  iAP2MediaItemAttributes is NULL.\n",
                    iap2CurrTimeMs(), iap2Device);
            rc = IAP2_CTL_ERROR;
        }
    }
    /* MFi R15: NowPlayingUpdate::PlaybackAttributes can occur 0/1. */
    if( (IAP2_OK == rc) && (NowPlayingUpdateParameter->iAP2PlaybackAttributes_count > 0) )
    {
        if(NULL != NowPlayingUpdateParameter->iAP2PlaybackAttributes)
        {
            /* MFi R15: Each parameter of iAP2PlaybackAttributes can occur 0/1. */
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds_count > 0)
            {
                printf("\t    ElapsedTime:  %d \n",
                        *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds));

                IAP2TESTDLTLOG(DLT_LOG_DEBUG, "NowPlayingUpdateParameter->iAP2PlaybackAttributes[%d].iAP2PlaybackElapsedTimeInMilliseconds_count = %d, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds = %d",
                           i, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds_count, *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds));
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackStatus_count > 0)
            {
                printf("\t    PlaybackStatus:  %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackStatus));
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode_count > 0)
            {
                printf("\t    ShuffleMode:  %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackShuffleMode));
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode_count > 0)
            {
                printf("\t    RepeatMode:   %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackRepeatMode));
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName_count > 0)
            {
                if( ((NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName) != NULL)
                    && (*(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName) != NULL) )
                {
                    printf("\t    PlaybackAppName:  %s \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppName));
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackSpeed_count > 0)
            {
                printf("\t    PlaybackSpeed:   %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackSpeed));
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2SetElapsedTimeAvailable_count > 0)
            {
                printf("\t    SetElapsedTimeAvailable\n");
                g_iap2TestDevice.iap2SetElapsedTimeAvailable = TRUE;
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueListAvail_count > 0)
            {
                printf("\t    PlaybackQueueList Available\n");
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueListTransferID_count > 0)
            {
                printf("\t    PlaybackQueueListTransferID:   %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueListTransferID));
                g_iap2TestDevice.playbackQueueList.TransferID = *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueListTransferID);
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppBundleID_count > 0)
            {
                if( ((NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppBundleID) != NULL)
                    && (*(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppBundleID) != NULL) )
                {
                    printf("\t    PlaybackAppBundleID:  %s \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackAppBundleID));
                }
            }
            if(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueIndex_count > 0)
            {
                printf("\t    PlaybackQueueIndex:   %d \n", *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueIndex));
                g_iap2TestDevice.iap2PlaybackQueueIndex = *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackQueueIndex);
            }
        }
        else
        {
            printf("\t %u ms %p  iap2NowPlayingUpdate_CB():  iAP2PlaybackAttributes is NULL.\n",
                    iap2CurrTimeMs(), iap2Device);
            rc = IAP2_CTL_ERROR;
        }
    }

    iap2SetTestState(NOW_PLAYING_UPDATE_RECV, TRUE);
    return rc;
}

S32 iap2FileTransferSetup_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;
    U8 temp;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferSetup_CB called \n", iap2CurrTimeMs(), iap2Device);

    if((iAP2FileXferSession != NULL) && (iAP2FileXferSession->iAP2FileXferRxLen > 0))
    {
        if(g_iap2TestDevice.playbackQueueList.TransferID == iAP2FileXferSession->iAP2FileTransferID)
        {
            printf("\t   PlaybackQueueList available. FileID: %d  FileSize: %llu  \n",
                    iAP2FileXferSession->iAP2FileTransferID, iAP2FileXferSession->iAP2FileXferRxLen);

            if((g_iap2TestDevice.playbackQueueList.Buffer != NULL)
               && (g_iap2TestDevice.playbackQueueList.transferred == TRUE))
            {
                printf("\t   iap2PlaybackQueueList_Buffer is not NULL, Free earlier received Queuelist. \n");

                /* free earlier received QueueList */
                iap2TestFreePtr( (void**)&g_iap2TestDevice.playbackQueueList.Buffer);
                temp = g_iap2TestDevice.playbackQueueList.TransferID;
                memset(&g_iap2TestDevice.playbackQueueList, 0, sizeof(iap2PlaybackQueueList_t));
                g_iap2TestDevice.playbackQueueList.TransferID = temp;
            }

            g_iap2TestDevice.playbackQueueList.Buffer = calloc(iAP2FileXferSession->iAP2FileXferRxLen, sizeof(U8));
            if(g_iap2TestDevice.playbackQueueList.Buffer != NULL)
            {
                rc = IAP2_OK;
                g_iap2TestDevice.playbackQueueList.CurPos = g_iap2TestDevice.playbackQueueList.Buffer;
                g_iap2TestDevice.playbackQueueList.Size = iAP2FileXferSession->iAP2FileXferRxLen;
            }
            else
            {
                printf("\t   memory allocation failed. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
                rc = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            printf("\t   artwork available. FileID: %d  FileSize: %llu  \n",
                    iAP2FileXferSession->iAP2FileTransferID, iAP2FileXferSession->iAP2FileXferRxLen);

            g_iap2TestDevice.coverArtBuf.Buffer = calloc(1,iAP2FileXferSession->iAP2FileXferRxLen);
            g_iap2TestDevice.coverArtBuf.CurPos = g_iap2TestDevice.coverArtBuf.Buffer;
            if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
            {
                g_iap2TestDevice.coverArtBuf.CurReceived = 0;
                g_iap2TestDevice.coverArtBuf.FileID = iAP2FileXferSession->iAP2FileTransferID;
                g_iap2TestDevice.coverArtBuf.FileSize = iAP2FileXferSession->iAP2FileXferRxLen;
                rc = IAP2_OK;
            }
            else
            {
                printf("\t   memory allocation failed. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
                rc = IAP2_ERR_NO_MEM;
            }
        }
    }
    else
    {
        if(iAP2FileXferSession != NULL){
            printf("\t   no artwork available. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        } else{
            printf("\t   iAP2FileXferSession is NULL \n");
        }
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
        if( (g_iap2TestDevice.playbackQueueList.CurPos != NULL) &&
            (g_iap2TestDevice.playbackQueueList.TransferID == iAP2FileXferSession->iAP2FileTransferID) )
        {
            /* only copy if earlier received Queuelist was cleared */
            if(g_iap2TestDevice.playbackQueueList.transferred == FALSE)
            {
                memcpy(g_iap2TestDevice.playbackQueueList.CurPos,
                       iAP2FileXferSession->iAP2FileXferRxBuf,
                       iAP2FileXferSession->iAP2FileXferRxLen);
                g_iap2TestDevice.playbackQueueList.CurPos += iAP2FileXferSession->iAP2FileXferRxLen;
                g_iap2TestDevice.playbackQueueList.CurReceived += iAP2FileXferSession->iAP2FileXferRxLen;
            }
            printf("\t   received PlaybackQueueList for FileID: %d \n",iAP2FileXferSession->iAP2FileTransferID);
        }
        else if( (g_iap2TestDevice.coverArtBuf.CurPos != NULL)
            && (g_iap2TestDevice.coverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID) )
        {
            memcpy(g_iap2TestDevice.coverArtBuf.CurPos, iAP2FileXferSession->iAP2FileXferRxBuf, iAP2FileXferSession->iAP2FileXferRxLen);
            g_iap2TestDevice.coverArtBuf.CurPos += iAP2FileXferSession->iAP2FileXferRxLen;
            g_iap2TestDevice.coverArtBuf.CurReceived += iAP2FileXferSession->iAP2FileXferRxLen;

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
    char fileName[MAX_STRING_LEN];

    /* To avoid compiler warnings */
    context = context;

    printf("\t %u ms %p  iap2FileTransferSuccess_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if( (g_iap2TestDevice.playbackQueueList.CurReceived == g_iap2TestDevice.playbackQueueList.Size) &&
            (g_iap2TestDevice.playbackQueueList.TransferID == iAP2FileXferSession->iAP2FileTransferID) )
        {
            g_iap2TestDevice.playbackQueueList.transferred = TRUE;
        }
        else if( (g_iap2TestDevice.coverArtBuf.FileID == iAP2FileXferSession->iAP2FileTransferID)
            && (g_iap2TestDevice.coverArtBuf.CurReceived == g_iap2TestDevice.coverArtBuf.FileSize) )
        {
            memset(&fileName[0], 0, (sizeof(fileName)));
            sprintf(&fileName[0], "%s%d%s", "/tmp/CoverArt", iAP2FileXferSession->iAP2FileTransferID, ".jpg");

            fp = fopen(&fileName[0], "w");
            if(fp != NULL)
            {
                fwrite(g_iap2TestDevice.coverArtBuf.Buffer, 1, g_iap2TestDevice.coverArtBuf.FileSize, fp);
                fclose(fp);

                printf("\t   File Transfer Success!  Please check %s \n", &fileName[0]);
            }

            if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
            {
                iap2TestFreePtr( (void**)&g_iap2TestDevice.coverArtBuf.Buffer);
                g_iap2TestDevice.coverArtBuf.CurPos = NULL;
                memset(&g_iap2TestDevice.coverArtBuf, 0, sizeof(iap2FileXferBuf));
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

        if( (g_iap2TestDevice.playbackQueueList.TransferID == iAP2FileXferSession->iAP2FileTransferID) &&
            (g_iap2TestDevice.playbackQueueList.Buffer != NULL) )
        {
            iap2TestFreePtr( (void**)&g_iap2TestDevice.playbackQueueList.Buffer);
            g_iap2TestDevice.playbackQueueList.CurPos = NULL;
            memset(&g_iap2TestDevice.playbackQueueList, 0 , sizeof(iap2PlaybackQueueList_t) );
        }
        if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
        {
            iap2TestFreePtr( (void**)&g_iap2TestDevice.coverArtBuf.Buffer);
            g_iap2TestDevice.coverArtBuf.CurPos = NULL;
            memset(&g_iap2TestDevice.coverArtBuf, 0, sizeof(iap2FileXferBuf));
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
        if( (g_iap2TestDevice.playbackQueueList.TransferID == iAP2FileXferSession->iAP2FileTransferID) &&
            (g_iap2TestDevice.playbackQueueList.Buffer != NULL) )
        {
            iap2TestFreePtr( (void**)&g_iap2TestDevice.playbackQueueList.Buffer);
            g_iap2TestDevice.playbackQueueList.CurPos = NULL;
            memset(&g_iap2TestDevice.playbackQueueList, 0 , sizeof(iap2PlaybackQueueList_t) );
        }
        if(g_iap2TestDevice.coverArtBuf.Buffer != NULL)
        {
            iap2TestFreePtr( (void**)&g_iap2TestDevice.coverArtBuf.Buffer);
            g_iap2TestDevice.coverArtBuf.CurPos = NULL;
            memset(&g_iap2TestDevice.coverArtBuf, 0, sizeof(iap2FileXferBuf));
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

    iap2SetTestState(FILE_TRANSFER_PAUSED_RECV, TRUE);
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

    iap2SetTestState(FILE_TRANSFER_RESUMED_RECV, TRUE);
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
    g_iap2TestDevice.EAPSessionIdentifier = *(thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier);

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
    printf("\t       Identifier:  %d | SinkEndpoint:  %d | SourceEndpoint:  %d\n",
            iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint);

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
    printf("\t       Identifier:  %d | SinkEndpoint:  %d | SourceEndpoint:  %d\n",
            iAP2iOSAppIdentifier, sinkEndpoint, sourceEndpoint);

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

S32 iap2DeviceTimeUpdate_CB(iAP2Device_t* iap2Device, iAP2DeviceTimeUpdateParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    (void)context;
    printf("\t %u ms %p  iap2DeviceTimeUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);
    if(NULL != thisParameter)
    {
        if( (thisParameter->iAP2SecondsSinceReferenceDate_count == 1) &&
            (thisParameter->iAP2SecondsSinceReferenceDate != NULL) )
        {
            printf("\t       SecondsSinceReferenceDate:  %lld\n", *(thisParameter->iAP2SecondsSinceReferenceDate));
        }
        else
        {
            rc = IAP2_CTL_ERROR;
        }
        if( (thisParameter->iAP2TimeZoneOffsetMinutes_count == 1) &&
            (thisParameter->iAP2TimeZoneOffsetMinutes != NULL) )
        {
            printf("\t       iAP2TimeZoneOffsetMinutes:  %d\n", *(thisParameter->iAP2TimeZoneOffsetMinutes));
        }
        else
        {
            rc = IAP2_CTL_ERROR;
        }
        if( (thisParameter->iAP2DaylightSavingsOffsetMinutes_count == 1) &&
            (thisParameter->iAP2DaylightSavingsOffsetMinutes != NULL) )
        {
            printf("\t       iAP2DaylightSavingsOffsetMinutes:  %d\n", *(thisParameter->iAP2DaylightSavingsOffsetMinutes));
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

/* **********************  iap2 smoketest functions ********************** */


/* **********************  register callback functions ********************** */

void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
}

void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks, BOOL iap2EAPSupported)
{
    iap2CSCallbacks->iAP2AuthenticationFailed_cb                    = &iap2AuthenticationFailed_CB;
    iap2CSCallbacks->iAP2AuthenticationSucceeded_cb                 = &iap2AuthenticationSucceeded_CB;
    iap2CSCallbacks->iAP2RequestAuthenticationCertificate_cb        = NULL;
    iap2CSCallbacks->iAP2RequestAuthenticationChallengeResponse_cb  = NULL;
    iap2CSCallbacks->iAP2StartIdentification_cb                     = NULL;
    iap2CSCallbacks->iAP2IdentificationAccepted_cb                  = &iap2IdentificationAccepted_CB;
    iap2CSCallbacks->iAP2IdentificationRejected_cb                  = &iap2IdentificationRejected_CB;
    iap2CSCallbacks->iAP2BluetoothConnectionUpdate_cb               = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationCertificate_cb         = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationResponse_cb            = NULL;
    iap2CSCallbacks->iAP2DeviceInformationUpdate_cb                 = &iap2DeviceInformationUpdate_CB;
    iap2CSCallbacks->iAP2DeviceLanguageUpdate_cb                    = NULL;
    if(iap2GetiOS8testing() == TRUE)
    {
        iap2CSCallbacks->iAP2DeviceTimeUpdate_cb                    = &iap2DeviceTimeUpdate_CB;
    }
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
    if(iap2EAPSupported == TRUE)
    {
        iap2CSCallbacks->iAP2StartExternalAccessoryProtocolSession_cb  = &iap2StartExternalAccessoryProtocolSession_CB;
        iap2CSCallbacks->iAP2StopExternalAccessoryProtocolSession_cb   = &iap2StopExternalAccessoryProtocolSession_CB;
    }
}

void iap2InitEAPSessionCallbacks(iAP2EAPSessionCallbacks_t* iAP2EAPSessionCallbacks)
{
    iAP2EAPSessionCallbacks->iAP2iOSAppDataReceived_cb = &iap2iOSAppDataReceived_CB;
}

void iap2InitEANativeTransportCallbacks(iAP2EANativeTransportCallbacks_t* iAP2EANativeTransportCallbacks)
{
    iAP2EANativeTransportCallbacks->p_iAP2StartEANativeTransport_cb = &iap2StartEANativeTransport_CB;
    iAP2EANativeTransportCallbacks->p_iAP2StopEANativeTransport_cb = &iap2StopEANativeTransport_CB;
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

/* **********************  iap2 test functions ********************** */

LOCAL S32 iap2TestStopPollThread(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd, MQ_CMD_EXIT_IAP2_COM_THREAD, NULL, 0);

    if(rc != IAP2_OK){
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

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_START_MEDIALIB_INFO,
                           &theiAP2StartMediaLibraryInformationParameter, sizeof(theiAP2StartMediaLibraryInformationParameter));

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
        if(iap2GetTestState(MEDIA_LIB_INFO_RECV) == TRUE){
            rc = IAP2_OK;
        }
        else
        {
            printf(" %u ms  iap2TestStartMediaLibInfo(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
            rc = IAP2_CTL_ERROR;
        }
    }

    if(rc != IAP2_OK){
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

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_MEDIALIB_INFO,
                           &StopMediaLibraryInformationParameter, sizeof(StopMediaLibraryInformationParameter));

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
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties_count++;
            /* MediaItem property PersistentIdentifier must be received */
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count++;
            /* set which MediaItem property (title, artist, etc.) should receive */
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count++;
            if(iap2GetiOS8testing() == TRUE)
            {
                theiAP2StartMediaLibraryUpdatesParameter.iAP2PlayAllSongsCapable_count++;
            }

            /* get percentage completion for current set of MediaLibraryUpdates */
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUpdateProgress_count++;
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
        rc = iap2MediaItemDbWaitForUpdate(10000, FALSE);
    }

    if(rc != IAP2_OK){
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

    if(rc != IAP2_OK){
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
        /* receive Artwork and use File Transfer Session */
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count++;
        if(iap2GetiOS8testing() == TRUE)
        {
            theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemPersistentIdentifier_count++;
        }

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

            if(iap2GetiOS8testing() == TRUE)
            {
                theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackQueueIndex_count++;
                theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackSpeed_count++;
                theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2SetElapsedTimeAvailable_count++;
                theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackQueueListAvail_count++;
                theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackQueueListTransferID_count++;
            }
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

    if(rc != IAP2_OK){
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

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_NOWPLAYING_UPDATE,
                           &theiAP2StopNowPlayingUpdatesParameter, sizeof(theiAP2StopNowPlayingUpdatesParameter));

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopNowPlayingUpdatesParameter(&theiAP2StopNowPlayingUpdatesParameter); */

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Sending iAP2PlayMediaLibrarySpecial to device */
LOCAL S32 iap2TestPlayMediaLibrarySpecial(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc;
    iAP2PlayMediaLibrarySpecialParameter theiAP2PlayMediaLibrarySpecialParameter;

    (void)iap2Device;

    memset(&theiAP2PlayMediaLibrarySpecialParameter, 0, sizeof(iAP2PlayMediaLibrarySpecialParameter) );
    rc = iap2AllocateandUpdateData(&theiAP2PlayMediaLibrarySpecialParameter.iAP2MediaLibraryUniqueIdentifier,
                                   &g_iap2TestDevice.testMediaLibInfoID,
                                   &theiAP2PlayMediaLibrarySpecialParameter.iAP2MediaLibraryUniqueIdentifier_count,
                                   1,
                                   iAP2_utf8);

    if(rc == IAP2_OK)
    {
        theiAP2PlayMediaLibrarySpecialParameter.iAP2AllSongs_count++;
        rc = iap2SendMqRecvAck(mq_fd,
                               g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_PLAY_MEDIA_LIBRARY_SPECIAL,
                               &theiAP2PlayMediaLibrarySpecialParameter,
                               sizeof(theiAP2PlayMediaLibrarySpecialParameter));
    }
    iAP2FreeiAP2PlayMediaLibrarySpecialParameter(&theiAP2PlayMediaLibrarySpecialParameter);

    return rc;
}

/* Sending iAP2SetNowPlayingInformation to device */
LOCAL S32 iap2TestSetNowPlayingInformation(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc;
    iAP2SetNowPlayingInformationParameter theiAP2SetNowPlayingInformationParameter;
    U32 ElapsedTime = 10000;

    (void)iap2Device;

    memset(&theiAP2SetNowPlayingInformationParameter, 0, sizeof(iAP2SetNowPlayingInformationParameter) );
    rc = iap2AllocateandUpdateData(&theiAP2SetNowPlayingInformationParameter.iAP2ElapsedTime,
                                   &ElapsedTime,
                                   &theiAP2SetNowPlayingInformationParameter.iAP2ElapsedTime_count,
                                   1,
                                   iAP2_uint32);
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2SetNowPlayingInformationParameter.iAP2PlaybackQueueIndex,
                                       &g_iap2TestDevice.iap2PlaybackQueueIndex,
                                       &theiAP2SetNowPlayingInformationParameter.iAP2PlaybackQueueIndex_count,
                                       1,
                                       iAP2_uint32);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd,
                               g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_SET_NOW_PLAYING_INFORMATION,
                               &theiAP2SetNowPlayingInformationParameter,
                               sizeof(theiAP2SetNowPlayingInformationParameter));
    }
    iAP2FreeiAP2SetNowPlayingInformationParameter(&theiAP2SetNowPlayingInformationParameter);

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
            if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0){
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
            }

            iAP2FreeiAP2PlayMediaLibraryItemsParameter(&theiAP2PlayMediaLibraryItemsParameter);

            if(rc != IAP2_OK)
            {
                printf(" %u ms  iap2TestStartPlayMediaLibraryItem(): failed.  rc = %d \n",
                        iap2CurrTimeMs(), rc);
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
                if(iap2GetTestState(NOW_PLAYING_UPDATE_RECV) == TRUE){
                    iap2SetTestState(PLAY_MEDIA_LIB_ITEM, TRUE);
                    rc = IAP2_OK;
                } else{
                    printf(" %u ms  iap2TestStartPlayMediaLibraryItem(): failed. retry: %d \n",
                            iap2CurrTimeMs(), retry_count);
                    rc = IAP2_CTL_ERROR;
                }
            }
        } else{
            printf(" %u ms  iap2TestStartPlayMediaLibraryItem():  PersistendIdentifierCount is %d \n ",
                    iap2CurrTimeMs(),PersistendIdentifierCount);
            rc = IAP2_CTL_ERROR;
        }
    } else{
        printf(" %u ms  iap2TestStartPlayMediaLibraryItem(): No MediaItem available. \n", iap2CurrTimeMs());
        rc = IAP2_CTL_ERROR;
    }

    if(rc != IAP2_OK){
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

    if(rc != IAP2_OK){
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
                                       (void*)&HIDDescriptor,
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

    if(rc != IAP2_OK){
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

    if(rc != IAP2_OK){
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

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

#if IAP2_ENABLE_FILE_TRANSFER_CANCEL_PAUSE_CMD
LOCAL S32 iap2TestCancelFileTransfer(S32 mq_fd, iAP2Device_t* iap2Device, U8 fileId)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
            MQ_CMD_CANCEL_FILE_TRANSFER, &fileId, 0);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

LOCAL S32 iap2TestPauseFileTransfer(S32 mq_fd, iAP2Device_t* iap2Device, U8 fileId)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
            MQ_CMD_PAUSE_FILE_TRANSFER, &fileId, 0);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

LOCAL S32 iap2TestResumeFileTransfer(S32 mq_fd, iAP2Device_t* iap2Device, U8 fileId)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
            MQ_CMD_RESUME_FILE_TRANSFER, &fileId, 0);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
#endif

/* send Bad DETECT Ack to device to cancel iAP1 support */
LOCAL S32 iap2TestCanceliAP1Support(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    iap2Device = iap2Device;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
            MQ_CMD_CANCEL_IAP1_SUPPORT, NULL, 0);

    if(rc != IAP2_OK){
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

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* send an StatusExternalAccessoryProtocolSession message to device */
LOCAL S32 iap2TestSendStatusEAPSessionMessage(S32 mq_fd, iAP2Device_t* iap2Device, iAP2ExternalAccessoryProtocolSessionStatus theiAP2ExternalAccessoryProtocolSessionStatus)
{
    S32 rc = IAP2_OK;
    iAP2StatusExternalAccessoryProtocolSessionParameter theiAP2StatusExternalAccessoryProtocolSessionParameter;

    (void)iap2Device;
    memset(&theiAP2StatusExternalAccessoryProtocolSessionParameter, 0, sizeof(iAP2StatusExternalAccessoryProtocolSessionParameter));

    rc = iap2AllocateandUpdateData(&theiAP2StatusExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionIdentifier,
                                   &g_iap2TestDevice.EAPSessionIdentifier,
                                   &theiAP2StatusExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionIdentifier_count,
                                   1,
                                   iAP2_uint16);

    if(rc == IAP2_OK)
    {
        theiAP2StatusExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionStatus = calloc(1, sizeof(iAP2ExternalAccessoryProtocolSessionStatus));
        if(theiAP2StatusExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionStatus == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            *(theiAP2StatusExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionStatus) = theiAP2ExternalAccessoryProtocolSessionStatus;
            theiAP2StatusExternalAccessoryProtocolSessionParameter.iAP2ExternalAccessoryProtocolSessionStatus_count++;
        }
    }

    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd,
                               g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_STATUS_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
                               &theiAP2StatusExternalAccessoryProtocolSessionParameter,
                               sizeof(theiAP2StatusExternalAccessoryProtocolSessionParameter));
    }
    iAP2FreeiAP2StatusExternalAccessoryProtocolSessionParameter(&theiAP2StatusExternalAccessoryProtocolSessionParameter);

    if (rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

#if IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST
LOCAL S32 iap2RequestAppLaunch(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;

    g_iap2TestDevice.AppBundleID = calloc(1, strlen(IAP2_IOS_BUNDLE_ID) + 1);
    if(g_iap2TestDevice.AppBundleID == NULL){
        rc = IAP2_ERR_NO_MEM;
    } else{
        memset(g_iap2TestDevice.AppBundleID, 0, strlen(IAP2_IOS_BUNDLE_ID) + 1);
        strncpy((char*)g_iap2TestDevice.AppBundleID, IAP2_IOS_BUNDLE_ID, strlen(IAP2_IOS_BUNDLE_ID));

        printf(" %u ms  iap2TestRequestAppLaunch(%s) \n", iap2CurrTimeMs(), g_iap2TestDevice.AppBundleID);
        rc = iap2TestRequestAppLaunch(mq_fd, iap2Device);
        if(rc == IAP2_OK)
        {
            printf("\n **** Please allow the communication with the iOS App. **** \n");
            printf(" **** Select '%s' and open the iOS App **** \n", IAP2_ACC_INFO_NAME);
        }

        iap2TestFreePtr( (void**)&g_iap2TestDevice.AppBundleID);
    }

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* send RequestAppLaunch to device */
LOCAL S32 iap2TestRequestAppLaunch(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iAP2RequestAppLaunchParameter theiAP2RequestAppLaunchParameter;

    iap2Device = iap2Device;

    memset(&theiAP2RequestAppLaunchParameter, 0, sizeof(iAP2RequestAppLaunchParameter));
    if(iap2GetiOS8testing() == TRUE)
    {
        theiAP2RequestAppLaunchParameter.iAP2LaunchAlert = calloc(1, sizeof(iAP2AppLaunchMethod));
        if(theiAP2RequestAppLaunchParameter.iAP2LaunchAlert == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            /* All devices that establish an iAP connection via the Lightning connector may set LaunchAlert to 'no alert' */
            *(theiAP2RequestAppLaunchParameter.iAP2LaunchAlert) = IAP2_LAUNCH_WITHOUT_USER_ALERT;
        }
    }
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2RequestAppLaunchParameter.iAP2AppBundleID,
                                           &g_iap2TestDevice.AppBundleID,
                                           &theiAP2RequestAppLaunchParameter.iAP2AppBundleID_count,
                                           1, iAP2_utf8);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_APP_LAUNCH,
                               &theiAP2RequestAppLaunchParameter, sizeof(theiAP2RequestAppLaunchParameter));
    }
    iAP2FreeiAP2RequestAppLaunchParameter(&theiAP2RequestAppLaunchParameter);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}
#endif

/* Sending iAP2PowerSourceUpdate to device */
LOCAL S32 iap2TestPowerSourceUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    U16 AvailableCurrentForDevice = IAP2_CURRENT_DRAW_FOR_DEVICE;
    U8 DeviceBatteryShouldChargeIfPowerIsPresent = 1;
    iAP2PowerSourceUpdateParameter theiAP2PowerSourceUpdateParameter;

    iap2Device = iap2Device;

    memset(&theiAP2PowerSourceUpdateParameter, 0, sizeof(iAP2PowerSourceUpdateParameter));

    rc = iap2AllocateandUpdateData(&theiAP2PowerSourceUpdateParameter.iAP2AvailableCurrentForDevice,
                                   &AvailableCurrentForDevice,
                                   &theiAP2PowerSourceUpdateParameter.iAP2AvailableCurrentForDevice_count,
                                   1, iAP2_uint16);
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2PowerSourceUpdateParameter.iAP2DeviceBatteryShouldChargeIfPowerIsPresent,
                                       &DeviceBatteryShouldChargeIfPowerIsPresent,
                                       &theiAP2PowerSourceUpdateParameter.iAP2DeviceBatteryShouldChargeIfPowerIsPresent_count,
                                       1, iAP2_uint8);
    }

    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_POWER_SOURCE_UPDATE,
                               &theiAP2PowerSourceUpdateParameter, sizeof(theiAP2PowerSourceUpdateParameter));
    }
    iAP2FreeiAP2PowerSourceUpdateParameter(&theiAP2PowerSourceUpdateParameter);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Sending iAP2StartPowerUpdates to device */
LOCAL S32 iap2TestStartPowerUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;

    U8 DeviceBatteryWillChargeIfPowerIsPresent = 1;
    U8 MaximumcurrentDrawnFromAccessory = 1;
    iAP2StartPowerUpdatesParameter theiAP2StartPowerUpdatesParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StartPowerUpdatesParameter, 0, sizeof(iAP2StartPowerUpdatesParameter));

    /* iAP2AccessoryPowerMode must be present if and only if the accessory draws power from apple device. */

    rc = iap2AllocateandUpdateData(&theiAP2StartPowerUpdatesParameter.iAP2DeviceBatteryWillChargeIfPowerIsPresent,
                                   &DeviceBatteryWillChargeIfPowerIsPresent,
                                   &theiAP2StartPowerUpdatesParameter.iAP2DeviceBatteryWillChargeIfPowerIsPresent_count,
                                   1, iAP2_uint8);

    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartPowerUpdatesParameter.iAP2MaximumcurrentDrawnFromAccessory,
                                       &MaximumcurrentDrawnFromAccessory,
                                       &theiAP2StartPowerUpdatesParameter.iAP2MaximumcurrentDrawnFromAccessory_count,
                                       1, iAP2_uint8);
    }

    if(rc == IAP2_OK)
    {
        if(iap2GetiOS8testing() == TRUE)
        {
            theiAP2StartPowerUpdatesParameter.iAP2IsExternalChargerConnected_count++;
            theiAP2StartPowerUpdatesParameter.iAP2BatteryChargingState_count++;
            theiAP2StartPowerUpdatesParameter.iAP2BatteryChargeLevel_count++;
        }
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_START_POWER_UPDATE,
                               &theiAP2StartPowerUpdatesParameter, sizeof(theiAP2StartPowerUpdatesParameter));
    }
    iAP2FreeiAP2StartPowerUpdatesParameter(&theiAP2StartPowerUpdatesParameter);

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

/* Sending iAP2StopPowerUpdates to device */
LOCAL S32 iap2TestStopPowerUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iAP2StopPowerUpdatesParameter theiAP2StopPowerUpdatesParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StopPowerUpdatesParameter, 0, sizeof(iAP2StopPowerUpdatesParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_POWER_UPDATE,
                           &theiAP2StopPowerUpdatesParameter, sizeof(theiAP2StopPowerUpdatesParameter));

    /* No need to free as there are no associated parameters */

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }
    return rc;
}


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
            if(iap2GetiOS8testing() == TRUE)
            {
                rc = iap2TestSendStatusEAPSessionMessage(mq_fd, iap2Device, IAP2_SESSION_STATUS_OK);
                printf(" iap2TestSendStatusEAPSessionMessage rc = %d \n", rc);
            }
            /* EAP session test */
            rc = iap2TestSendEAPSessionMessage(mq_fd, iap2Device, &testbuf[0]);
            printf(" iap2TestSendEAPSessionMessage(len: %zu) rc = %d \n",
                    strlen((const char*)testbuf), rc);

            iap2SetTestState(EAP_SESSION_START_RECV, FALSE);
        }
        if( (iap2GetTestState(EAP_SESSION_STOP_RECV) == TRUE) &&
            (iap2GetiOS8testing() == TRUE) )
        {
            rc = iap2TestSendStatusEAPSessionMessage(mq_fd, iap2Device, IAP2_SESSION_CLOSE);
            printf(" iap2TestSendStatusEAPSessionMessage rc = %d \n", rc);
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
            iap2SetTestState(EA_NATIVE_TRANSPORT_START_RECV, FALSE);

            snprintf((char*)&writename[len], len +1,
                     "%d", g_iap2TestDevice.SinkEndpoint);

            snprintf((char*)&readname[len], len +1,
                     "%d", g_iap2TestDevice.SourceEndpoint);

            write_fd = open((const char*)&writename[0], O_NONBLOCK | O_WRONLY);
            read_fd  = open((const char*)&readname[0], O_NONBLOCK | O_RDONLY);
            if((write_fd >= 0) && (read_fd >= 0))
            {
                rc = write(write_fd, (void*)&testbuf[0], strlen((const char*)&testbuf[0]));
                printf(" %u ms  Send message (len: %zu) to iOS App  rc = %d \n",
                       iap2CurrTimeMs(), strlen((const char*)&testbuf[0]), rc);

                if( rc == (S32)strlen((const char*)&testbuf[0]) )
                {
                    while((rc > 0) && (iap2GetGlobalQuit() == FALSE)
                          && (iap2GetTestState(EA_NATIVE_TRANSPORT_STOP_RECV) == FALSE)
                          && (iap2GetTestDeviceState() != iAP2NotConnected))
                    {
                        rc = read(read_fd, &readBuf[0], readBytes);
                        printf(" read = %d \n", rc);

                        if(rc > 0){
                            for(i=0;i<rc;i++)
                            {
                                if( (i % 10) == 0){
                                    printf("\n");
                                }
                                printf("    0x%.2X, ", readBuf[i]);
                            }
                            printf("\n");
                            memset(&readBuf[0], 0, readBytes);
                        } else{
                            printf(" %u ms  read = %d | errno: %s\n",
                                    iap2CurrTimeMs(), rc, strerror(errno));
                        }
                    }
                } else{
                    printf(" %u ms  Send message = rc: %d | errno: %s\n",
                            iap2CurrTimeMs(), rc, strerror(errno));
                    rc = IAP2_CTL_ERROR;
                    iap2SetGlobalQuit(TRUE);
                }

                if(write_fd >= 0){
                    close(write_fd);
                }
                if(read_fd >= 0){
                    close(read_fd);
                }
            } else{
                rc = IAP2_CTL_ERROR;
                iap2SetGlobalQuit(TRUE);
            }
        }
    }

    iap2SleepMs(10);
    if(iap2GetTestState(EA_NATIVE_TRANSPORT_STOP_RECV) == TRUE){
        iap2SetTestState(EA_NATIVE_TRANSPORT_STOP_RECV, FALSE);
        printf(" %u ms  iOS App %d closed \n", iap2CurrTimeMs(), g_iap2TestDevice.EANativeTransportAppId);
        rc = IAP2_OK;
    }

    return rc;
}

/* helper API for MediaLibraryUpdate and database handling */
LOCAL S32 iap2MediaItemDbWaitForUpdate(U32 waitTime, BOOL waitToFinish)
{
    int rc = IAP2_OK;
    U32 retry_count = 0;
    U32 tmpWaitTime = 0;
    U32 callTime = 0;

    /* iap2SleepMs() set to 10ms
     * If waitTime is 5000, tmpWaitTime must be 500 to wait 5000ms */
    tmpWaitTime = (waitTime / 10);

    while( (rc == IAP2_OK) && (g_iap2TestDevice.testMediaLibUpdateProgress < 100)
            &&(iap2GetTestStateError() != TRUE) && (retry_count < tmpWaitTime) )
    {
        if(iap2GetTestDeviceState() == iAP2NotConnected){
            rc = IAP2_DEV_NOT_CONNECTED;
            break;
        }
        iap2SleepMs(10);
        retry_count++;

        /* check if MediaLibraryUpdates callback was called */
        if(iap2GetTestState(MEDIA_LIB_UPDATE_RECV) == TRUE){
            if(g_iap2TestDevice.testMediaLibUpdateProgress == 100) {
                /* MediaLibUpdate complete */
                iap2SleepMs(10);
            } else{
                /* MediaLibUpdate not completed */
                iap2SetTestState(MEDIA_LIB_UPDATE_RECV, FALSE);
                if(waitToFinish == TRUE){
                    /* reset retry_count in case not all data were received */
                    retry_count = 0;
                } else{
                    if(g_iap2TestDevice.testMediaItemCnt > 10){
                        /* Should not wait until MediaLibUpdate is complete.
                         * Timeout not reached.
                         * Some MediaItems received - done. */
                        rc = IAP2_OK;
                        break;
                    }
                }
            }
        }
    }
    if(g_iap2TestDevice.testMediaLibUpdateProgress < 100){
        printf(" %u ms  iap2MediaItemDbWaitForUpdate(): retry: %d | progress: (%d / 100)\n",
            iap2CurrTimeMs(), retry_count, g_iap2TestDevice.testMediaLibUpdateProgress);
    } else{
        /* don't do this in the callback. */
        callTime = iap2CurrTimeMs();
        rc = iap2MediaItemDbCleanUp(g_iap2TestDevice.testMediaLibUpdateProgress);
        printf(" %u ms  iap2MediaItemDbWaitForUpdate(): iap2MediaItemDbCleanUp() takes %u ms \n",
                iap2CurrTimeMs(), (iap2CurrTimeMs() - callTime));
        printf("\n Number of songs: %d \n", g_iap2TestDevice.testMediaItemCnt);
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
                case MQ_CMD_CANCEL_FILE_TRANSFER:
                {
                    U8* fileId = (U8*)(mq_st->param);
                    printf("call iAP2CancelFileTransfer(%d) \n", *fileId);
                    rc = iAP2CancelFileTransfer(iap2Device, *fileId);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2CancelFileTransfer failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_PAUSE_FILE_TRANSFER:
                {
                    U8* fileId = (U8*)(mq_st->param);
                    printf("call iAP2PauseFileTransfer(%d) \n", *fileId);
                    rc = iAP2PauseFileTransfer(iap2Device, *fileId);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2PauseFileTransfer failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_RESUME_FILE_TRANSFER:
                {
                    U8* fileId = (U8*)(mq_st->param);
                    printf("call iAP2ResumeFileTransfer(%d) \n", *fileId);
                    rc = iAP2ResumeFileTransfer(iap2Device, *fileId);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2ResumeFileTransfer failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_POWER_SOURCE_UPDATE:
                {
                    rc = iAP2PowerSourceUpdate(iap2Device, (iAP2PowerSourceUpdateParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2PowerSourceUpdate failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_START_POWER_UPDATE:
                {
                    rc = iAP2StartPowerUpdates(iap2Device, (iAP2StartPowerUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StartPowerUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_STOP_POWER_UPDATE:
                {
                    rc = iAP2StopPowerUpdates(iap2Device, (iAP2StopPowerUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StopPowerUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_PLAY_MEDIA_LIBRARY_SPECIAL:
                {
                    rc = iAP2PlayMediaLibrarySpecial(iap2Device, (iAP2PlayMediaLibrarySpecialParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2PlayMediaLibrarySpecial failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_SET_NOW_PLAYING_INFORMATION:
                {
                    rc = iAP2SetNowPlayingInformation(iap2Device, (iAP2SetNowPlayingInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2SetNowPlayingInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                    }
                    break;
                }
                case MQ_CMD_STATUS_EXTERNAL_ACCESSORY_PROTOCOL_SESSION:
                {
                    rc = iAP2StatusExternalAccessoryProtocolSession(iap2Device, (iAP2StatusExternalAccessoryProtocolSessionParameter*)mq_st->param);
                    if(rc != IAP2_OK){
                        printf(" %u ms  iAP2StatusExternalAccessoryProtocolSession failed!  rc = %d \n", iap2CurrTimeMs(), rc);
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


/* helper API to remove multiple MediaItem entries */
LOCAL S32 iap2MediaItemDbCleanUp(U8 updateProgress)
{
    S32 rc = IAP2_OK;
    S32 ret = IAP2_OK;
    U16 i = 0;
    U16 j = 0;
    U16 mediaItemCnt = 0;

    if(updateProgress < 100){
        printf(" %u ms  iap2MediaItemDbCleanUp():  MediaLibUpdateProgress is only at %d /100 \n",
                iap2CurrTimeMs(), updateProgress);
        return IAP2_CTL_ERROR;
    } else if ( (g_iap2TestDevice.tmpMediaItem == NULL) && (g_iap2TestDevice.testMediaItem == NULL) ){
        /* updateProgress is 100, but no MediaItem database was created */
        printf(" %u ms  iap2MediaItemDbCleanUp():  No MediaItem database available. tmpMediaItem is NULL \n",
            iap2CurrTimeMs());
        return IAP2_CTL_ERROR;
    } else if ( (g_iap2TestDevice.tmpMediaItem != NULL) && (g_iap2TestDevice.testMediaItem == NULL) ){

        pthread_mutex_lock(&g_iap2TestDevice.testMediaLibMutex);
        printf(" %u ms  iap2MediaItemDbCleanUp():  Get pure DB takes a while... \n", iap2CurrTimeMs());

        g_iap2TestDevice.testMediaItem = calloc(g_iap2TestDevice.tmpMediaItemCnt, sizeof(iAP2MediaItem));
        if(g_iap2TestDevice.testMediaItem != NULL)
        {
            mediaItemCnt = 0;
            for(i=0; ( (i < g_iap2TestDevice.tmpMediaItemCnt) && (rc == IAP2_OK) ); i++)
            {
                if(g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
                {
                    /* check if PersistentId is still available */
                    for(j=0; ((j<mediaItemCnt)&&(ret == IAP2_OK)); j++)
                    {
                        if(g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier_count > 0)
                        {
                            if(*(g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier) == *(g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier))
                            {
                                ret = IAP2_CTL_ERROR;
                            }
                        }
                    }
                    if(ret == IAP2_OK){
                        mediaItemCnt++;
                        rc = iap2AllocateandUpdateData(&g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier,
                                                       g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                       &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemPersistentIdentifier_count,
                                                       g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                       iAP2_uint64);

                        if( (g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle_count > 0) && (rc == IAP2_OK) )
                        {
                            rc = iap2AllocateandUpdateData(&g_iap2TestDevice.testMediaItem[j].iAP2MediaItemTitle,
                                                           g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle,
                                                           &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemTitle_count,
                                                           g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle_count,
                                                           iAP2_utf8);
                        }

                        if( (g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre_count > 0) && (rc == IAP2_OK) )
                        {
                            rc = iap2AllocateandUpdateData(&g_iap2TestDevice.testMediaItem[j].iAP2MediaItemGenre,
                                                           g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre,
                                                           &g_iap2TestDevice.testMediaItem[j].iAP2MediaItemGenre_count,
                                                           g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre_count,
                                                           iAP2_utf8);
                        }
                    } else{
                        ret = IAP2_OK;
                    }
                }
            }
            if(rc == IAP2_OK){
                g_iap2TestDevice.testMediaItemCnt = mediaItemCnt;
                rc = IAP2_OK;
            }
        } else{
            printf(" %u ms  iap2MediaItemDbCleanUp(): .testMediaItem is NULL \n", iap2CurrTimeMs());
            rc = IAP2_ERR_NO_MEM;
        }

        /* Free temporary MediaItem database which was created with iAP2MediaLibraryUpdates */
        if(g_iap2TestDevice.tmpMediaItem != NULL)
        {
            for(i=0; i<g_iap2TestDevice.tmpMediaItemCnt;i++)
            {
                iAP2FreeiAP2MediaItem(&(g_iap2TestDevice.tmpMediaItem[i]));
            }
            iap2TestFreePtr( (void**)&g_iap2TestDevice.tmpMediaItem);
            g_iap2TestDevice.tmpMediaItemCnt = 0;
        }

        pthread_mutex_unlock(&g_iap2TestDevice.testMediaLibMutex);
    } else{
        /* updateProgress is 100 and MediaItem database (testMediaItem) was created */
        /* nothing to do */
        rc = IAP2_OK;
    }

    return rc;
}

/* helper API for callback iap2MediaLibraryUpdates_CB */
LOCAL S32 iap2MediaItemDbUpdate(iAP2MediaLibraryUpdateParameter* MediaLibraryUpdateParameter)
{
    S32 rc = IAP2_OK;
    U16 i = 0;
    size_t size = 0;
    U16 mediaItemCnt = 0;
    U16 t_index = 0;

    pthread_mutex_lock(&g_iap2TestDevice.testMediaLibMutex);

    if(g_iap2TestDevice.tmpMediaItem == NULL){
        /* first MediaLibraryUpdates_CB */
        mediaItemCnt = MediaLibraryUpdateParameter->iAP2MediaItem_count;

        g_iap2TestDevice.tmpMediaItem = calloc(MediaLibraryUpdateParameter->iAP2MediaItem_count, sizeof(iAP2MediaItem));
        if(g_iap2TestDevice.tmpMediaItem != NULL)
        {
            for(i=0; ( (i < MediaLibraryUpdateParameter->iAP2MediaItem_count) && (rc == IAP2_OK) ); i++)
            {
                if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
                {
                    rc = iap2AllocateandUpdateData(&g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                   MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                   &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                   MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                   iAP2_uint64);

                    if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0) && (rc == IAP2_OK) )
                    {
                        rc = iap2AllocateandUpdateData(&g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle,
                                                       &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemTitle_count,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count,
                                                       iAP2_utf8);
                    }

                    if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0) && (rc == IAP2_OK) )
                    {
                        rc = iap2AllocateandUpdateData(&g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre,
                                                       &g_iap2TestDevice.tmpMediaItem[i].iAP2MediaItemGenre_count,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count,
                                                       iAP2_utf8);
                    }
                }
            }
            if(rc == IAP2_OK){
                rc = mediaItemCnt;
            }
        } else{
            printf("\t %u ms:  iap2MediaItemDbUpdate(): g_MediaItem.test_MediaItem is NULL \n", iap2CurrTimeMs());
            rc = IAP2_ERR_NO_MEM;
        }
    } else if( (g_iap2TestDevice.testMediaLibInfoID != NULL)
               &&(strncmp((char*)g_iap2TestDevice.testMediaLibInfoID,
                  (char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier), size) == 0) ){
        /* MediaLibrary database still exists, update the DB */
        mediaItemCnt = MediaLibraryUpdateParameter->iAP2MediaItem_count;

        size = ((g_iap2TestDevice.tmpMediaItemCnt + MediaLibraryUpdateParameter->iAP2MediaItem_count) * sizeof(iAP2MediaItem));
        g_iap2TestDevice.tmpMediaItem = realloc(g_iap2TestDevice.tmpMediaItem, size);
        memset(g_iap2TestDevice.tmpMediaItem+g_iap2TestDevice.tmpMediaItemCnt, 0, (MediaLibraryUpdateParameter->iAP2MediaItem_count) * sizeof(iAP2MediaItem));
        if(g_iap2TestDevice.tmpMediaItem != NULL)
        {
            /* add MediaItem information to current existing one */
            t_index = g_iap2TestDevice.tmpMediaItemCnt;
            for(i = 0; ( (i < MediaLibraryUpdateParameter->iAP2MediaItem_count) && (rc == IAP2_OK) ); i++)
            {
                if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count > 0)
                {
                    rc = iap2AllocateandUpdateData(&g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemPersistentIdentifier,
                                                   MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier,
                                                   &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemPersistentIdentifier_count,
                                                   MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count,
                                                   iAP2_uint64);

                    if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count > 0) && (rc == IAP2_OK) )
                    {
                        rc = iap2AllocateandUpdateData(&g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemTitle,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle,
                                                       &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemTitle_count,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemTitle_count,
                                                       iAP2_utf8);
                    }

                    if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count > 0) && (rc == IAP2_OK) )
                    {
                        rc = iap2AllocateandUpdateData(&g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemGenre,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre,
                                                       &g_iap2TestDevice.tmpMediaItem[t_index].iAP2MediaItemGenre_count,
                                                       MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemGenre_count,
                                                       iAP2_utf8);
                    }
                    t_index++;
                }
            }
            if(rc == IAP2_OK){
                rc = mediaItemCnt;
            }
        } else{
            printf("\t %u ms  iap2MediaItemDbUpdate(): g_MediaItem.test_MediaItem is NULL. Realloc failed. \n", iap2CurrTimeMs());
            rc = IAP2_ERR_NO_MEM;
        }
    } else{
        printf("\t %u ms  iap2MediaItemDbUpdate(): g_MediaItem.test_MediaItem already allocated \n", iap2CurrTimeMs());
        rc = IAP2_CTL_ERROR;
    }

    pthread_mutex_unlock(&g_iap2TestDevice.testMediaLibMutex);

    return rc;
}

LOCAL int iap2GetArgument(int argc, const char** argv)
{
    int rc = 0;
    int app_user_idx;
    int ffs_group_idx;

    /* set default settings */
    g_iap2UserConfig.iAP2AuthenticationType = iAP2AUTHI2C;
    g_iap2UserConfig.iAP2TransportType      = iAP2USBDEVICEMODE;
    g_iap2UserConfig.iap2iOSintheCar        = FALSE;
    g_iap2UserConfig.iap2EANativeTransport  = FALSE;

    g_iap2UserConfig.iap2EAPSupported       = FALSE;
    g_iap2UserConfig.iap2UsbOtgGPIOPower    = NULL;
    g_iap2UserConfig.SupportediOSAppCount   = 0;

    /* initalize (don't set user or group) */
    g_iap2UserConfig.app_thread_user = 0;
    g_iap2UserConfig.app_thread_prim_group = 0;
    g_iap2UserConfig.ffs_group = 0;

    /* start with empty list of groups */
    g_iap2UserConfig.app_thread_groups_cnt = 0;
    g_iap2UserConfig.app_thread_groups = NULL;

    g_loopcount = 1;
    app_user_idx = 5;
    ffs_group_idx = 6;


    /* check input parameters */
    if(argc >= 5)
    {
        if(atoi(argv[1]) != 0)
        {
            g_loopcount = atoi(argv[1]);
        }
        else
        {
            rc = -1;
            printf("Loop count parameter is not integer.\n");
        }
        /* - run the selected test in USB Device mode or USB Host mode */
        if(0 == strncmp(argv[2], "host", 4))
        {
            g_iap2UserConfig.iAP2TransportType = iAP2USBHOSTMODE;
            printf("Host Mode selected. '%s' '%s'\n", argv[2], argv[3]);
        }
        else if(0 == strncmp(argv[2], "gadget", 6))
        {
            g_iap2UserConfig.iAP2TransportType = iAP2USBDEVICEMODE;
            printf("Device Mode selected. '%s' '%s' '%s'\n", argv[2], argv[3], argv[4]);
        }
        else
        {
            rc = -1;
            printf("Wrong Mode selected. '%s' \n", argv[1]);
        }

        if(rc == 0)
        {
            /* set GPIO for USB power */
            if (0 == strncmp(argv[3], "sd", 2))
            {
                g_iap2UserConfig.iap2UsbOtgGPIOPower = (VP)IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_SD;
            }
            else if (0 == strncmp(argv[3], "ai", 2))
            {
                g_iap2UserConfig.iap2UsbOtgGPIOPower = (VP)IAP2_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI;
            }
            else
            {
                rc = -1;
                printf("Wrong board type selected. '%s' \n", argv[3]);
            }
        }

        if(rc == 0)
        {
            if (0 == strncmp(argv[4], "dipo", 4))
            {
                /* --- host mode with DiPO support--- */
                g_iap2UserConfig.iap2iOSintheCar = TRUE;

                printf("dipo selected. \n");
            }
            else if (0 == strncmp(argv[4], "nopo", 4))
            {
                /* --- without DiPO --- */
                g_iap2UserConfig.iap2iOSintheCar = FALSE;
                if(argc == 6)
                {
                    if(0 == strncmp(argv[5], "ios8", 4))
                    {
                        app_user_idx++;
                        ffs_group_idx++;
                        iap2SetiOS8testing(TRUE);
                        printf("iOS8 Testing selected. \n");
                    }
                }
            }
            else if (0 == strncmp(argv[4], "ea", 3))
            {
                app_user_idx++;
                ffs_group_idx++;

                /* --- EA test --- */
                iap2SetEAtesting(TRUE);

                if ((0 == strncmp(argv[5], "np", 2))
                    &&(g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EAP Session --- */
                    g_iap2UserConfig.iap2EAPSupported = TRUE;
                    /* --- EA Native Transport --- */
                    g_iap2UserConfig.iap2EANativeTransport = TRUE;

                    g_iap2UserConfig.SupportediOSAppCount = 2;
                }
                else if ((0 == strncmp(argv[5], "p", 1))
                      &&(g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EAP Session --- */
                    g_iap2UserConfig.iap2EAPSupported = TRUE;

                    g_iap2UserConfig.SupportediOSAppCount = 1;
                }
                else if ((0 == strncmp(argv[5], "p", 1))
                        &&(g_iap2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE))
                {
                    /* --- EAP Session --- */
                    g_iap2UserConfig.iap2EAPSupported = TRUE;

                    g_iap2UserConfig.SupportediOSAppCount = 1;
                }
                else if ((0 == strncmp(argv[5], "nn", 2))
                         &&(g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EA Native Transport --- */
                    g_iap2UserConfig.iap2EANativeTransport = TRUE;

                    g_iap2UserConfig.SupportediOSAppCount = 2;
                }
                else if ((0 == strncmp(argv[5], "n", 1))
                         &&(g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE))
                {
                    /* --- EA Native Transport --- */
                    g_iap2UserConfig.iap2EANativeTransport = TRUE;

                    g_iap2UserConfig.SupportediOSAppCount = 1;
                }
                else
                {
                    rc = -1;
                    printf("Wrong EA test type or TransportType selected. '%s' '%s' \n", argv[4], argv[1]);
                }
                if(argc == 7)
                {
                    if(0 == strncmp(argv[6], "ios8", 4))
                    {
                        app_user_idx++;
                        ffs_group_idx++;
                        iap2SetiOS8testing(TRUE);
                        printf("iOS8 Testing selected. \n");
                    }
                }
            }
            else
            {
                rc = -1;
                printf("Wrong test type selected. '%s' \n", argv[3]);
            }
        }

        if(argc > app_user_idx)
        {
            /* parse user for app and group for ffs */
            struct passwd *app_user;

            app_user = getpwnam(argv[app_user_idx]);
            if (!app_user) {
                printf("Unknown user %s\n", argv[app_user_idx]);
                rc = -1;
            } else {
                g_iap2UserConfig.app_thread_user = app_user->pw_uid;
                g_iap2UserConfig.app_thread_prim_group = app_user->pw_gid;

                getgrouplist(app_user->pw_name, app_user->pw_gid, NULL, &g_iap2UserConfig.app_thread_groups_cnt);

                g_iap2UserConfig.app_thread_groups = malloc(g_iap2UserConfig.app_thread_groups_cnt * sizeof (gid_t));
                if (!g_iap2UserConfig.app_thread_groups) {
                    printf ("No memory for supplementary group list\n");
                    rc = -1;
                } else {
                    getgrouplist(app_user->pw_name, app_user->pw_gid, g_iap2UserConfig.app_thread_groups, &g_iap2UserConfig.app_thread_groups_cnt);
                }
            }
        }

        if(argc > ffs_group_idx)
        {
            struct group *ffs_group;
            ffs_group = getgrnam(argv[ffs_group_idx]);
            if (!ffs_group) {
                printf("Unknown group %s\n", argv[ffs_group_idx]);
                rc = -1;
            } else {
                g_iap2UserConfig.ffs_group = ffs_group->gr_gid;
            }
        }
    }
    else if(argc == 2)
    {
        if(atoi(argv[1]) != 0)
        {
            g_loopcount = atoi(argv[1]);
            g_iap2UserConfig.iAP2TransportType = iAP2USBDEVICEMODE;
        }
        else
        {
            rc = -1;
            printf("minimum 1 argument is required.\n");
        }
    }
    else if(argc == 1)
    {
        /* if now arguments, then:
         * - run the iAP2 Smoketest in USB Device mode
         * - run standard media library tests */

        /* --- device mode --- */
        g_iap2UserConfig.iAP2TransportType = iAP2USBDEVICEMODE;
        g_iap2UserConfig.iap2iOSintheCar   = FALSE;
        g_iap2UserConfig.iap2EANativeTransport = FALSE;

        g_iap2UserConfig.iap2EAPSupported = FALSE;

        g_loopcount = 1;
        g_iap2UserConfig.iap2UsbOtgGPIOPower = NULL;
        g_iap2UserConfig.SupportediOSAppCount = 0;
        iap2SetEAtesting(FALSE);
        printf("Device Mode selected.\n");
    }
    else
    {
        printf("Wrong number of arguments %d \n", argc);
        rc = -1;
    }
    if (g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE){
    g_iap2TestDevice.iap2USBHostMode = TRUE;
    }else{
    g_iap2TestDevice.iap2USBHostMode = FALSE;
    }
    return rc;
}

LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2device)
{
    S32 rc = IAP2_OK;
    U32 retry_count = 0;

    /* Power cannot be provided in case of Bluetooth. */
    /* Send PowerSourceUpdate after Authentication and Identification succeeded. */
    rc = iap2TestPowerSourceUpdate(mq_fd, iap2device);
    printf("\n %u ms  iap2TestPowerSourceUpdate()  rc = %d\n", iap2CurrTimeMs(), rc);

    if(iap2GetTestStateError() != TRUE)
    {
        /* start PowerUpdates */
        printf("\n %u ms  iap2TestStartPowerUpdates() \n", iap2CurrTimeMs());
        rc = iap2TestStartPowerUpdates(mq_fd, iap2device);
    }

    if(iap2GetEAtesting() == TRUE)
    {
#if IAP2_ENABLE_REQUEST_APP_LAUNCH_TEST
        if(iap2GetTestStateError() != TRUE)
        {
            rc = iap2RequestAppLaunch(mq_fd, iap2device);
            printf("\n %u ms  iap2RequestAppLaunch()  rc = %d\n", iap2CurrTimeMs(), rc);
        }
#endif
        if(iap2GetTestStateError() != TRUE)
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

        /* check if Apple device was disconnected */
        if(iap2GetTestDeviceState() != iAP2NotConnected)
        {
            /* stop PowerUpdates */
            printf("\n %u ms  iap2TestStopPowerUpdates() \n", iap2CurrTimeMs());
            rc = iap2TestStopPowerUpdates(mq_fd, iap2device);
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
                if( (g_iap2TestDevice.iap2PlayAllSongsCapable == TRUE) &&
                    (iap2GetiOS8testing() == TRUE) )
                {
                    /* Use PlayMediaLibrarySpecial if MediaLibUpdate return PlayAllSongsCapable */
                    printf("\n %u ms  iap2TestPlayMediaLibrarySpecial() \n", iap2CurrTimeMs());
                    rc = iap2TestPlayMediaLibrarySpecial(mq_fd, iap2device);
                }
                else
                {
                    /* Use PlayMediaLibraryItem if MediaLibUpdate return MediaLibraryItem */
                    printf("\n %u ms  iap2TestStartPlayMediaLibraryItem() \n", iap2CurrTimeMs());
                    rc = iap2TestStartPlayMediaLibraryItem(mq_fd, iap2device, 1, 10);
                }
            } else{ // TBDO: only in device mode ??
                /* Use SendHIDReport(Play) if MediaLibUpdate does not return MediaLibraryItem */
                printf("\n %u ms  iap2TestSendHIDReport(Play) \n", iap2CurrTimeMs());
                rc = iap2TestSendHIDReport(mq_fd, iap2device, 1); /*Press*/
                (void)iap2TestSendHIDReport(mq_fd, iap2device, 0); /*Release*/
            }
        }

#if IAP2_GST_AUDIO_STREAM
        if( (iap2GetTestStateError() != TRUE)
            && (g_iap2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE) )
        {
            /* start USBDeviceModeAudio */
            printf("\n %u ms  iap2TestStartUSBDeviceModeAudio() \n", iap2CurrTimeMs());
            rc = iap2TestStartUSBDeviceModeAudio(mq_fd, iap2device);
        }
        if((rc == IAP2_OK) && (iap2GetTestStateError() != TRUE))
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

        if( (iap2GetTestStateError() != TRUE) &&
            (g_iap2TestDevice.iap2SetElapsedTimeAvailable == TRUE) &&
            (iap2GetiOS8testing() == TRUE) )
        {
            rc = iap2TestSetNowPlayingInformation(mq_fd, iap2device);
            if(rc == IAP2_OK)
                iap2SleepMs(2000);
        }

        /* stop playback at end of smoketest */
        if(iap2GetTestStateError() != TRUE)
        {
            printf("\n %u ms  iap2TestSendHIDReport(Pause) \n", iap2CurrTimeMs());
            rc = iap2TestSendHIDReport(mq_fd, iap2device, 2); /*Pause*/
            (void)iap2TestSendHIDReport(mq_fd, iap2device, 0);
        }

        if(g_iap2TestDevice.testMediaLibInfoID != NULL)
        {
            /* check if Apple device was disconnected */
            if(iap2GetTestDeviceState() == iAP2NotConnected){
                printf("\n %u ms  Apple device was disconnected! \n", iap2CurrTimeMs());
            } else{
                /* stop PowerUpdates */
                printf("\n %u ms  iap2TestStopPowerUpdates() \n", iap2CurrTimeMs());
                rc = iap2TestStopPowerUpdates(mq_fd, iap2device);

                /* stop NowPlayingUpdates */
                printf("\n %u ms  iap2TestStopNowPlayingUpdate() \n", iap2CurrTimeMs());
                rc = iap2TestStopNowPlayingUpdate(mq_fd, iap2device);

                /* stop MediaLibraryUpdate */
                printf("\n %u ms  iap2TestStopMediaLibUpdate() \n", iap2CurrTimeMs());
                rc = iap2TestStopMediaLibUpdate(mq_fd, iap2device);

                /* stop MediaLibraryInfo */
                printf("\n %u ms  iap2TestStopMediaLibInfo() \n", iap2CurrTimeMs());
                rc = iap2TestStopMediaLibInfo(mq_fd, iap2device);

                /* stop HID report */
                printf("\n %u ms  iap2TestStopHID() \n", iap2CurrTimeMs());
                rc = iap2TestStopHID(mq_fd, iap2device);

                iap2SleepMs(1000);
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
    IAP2TESTDLTLOG(DLT_LOG_INFO, "iAP2InitDeviceStructure iap2device = %p ", iap2device);

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
                iAP2FreeiAP2MediaItem(&(g_iap2TestDevice.testMediaItem[i]));
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
        if(g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
        {
            /* Need some time for USB Host mode plugin initialization (to bring up the gadgets) */
            iap2SleepMs(1000);
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
    if(g_rc == IAP2_OK && g_loopcount == 0)
    {
        printf("iap2_endurance_test                 PASS       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 ENDURANCE TEST PASS *****");
    }
    else if (g_rc != IAP2_OK)
    {
        printf("iap2_endurance_test                 FAIL       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 ENDURANCE TEST FAIL *****");
    }

    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return g_rc;
}

S32 main(int argc, const char** argv)
{
    S32 rc;
    int var = 0;
    int socket_fds[2];
    static const int host_device_switch_process = 0;
    static const int application_process = 1;
    pid_t host_device_switch_process_pid;
    BOOL iap2_host_mode = FALSE;

    memset(&g_iap2TestDevice, 0, sizeof(g_iap2TestDevice));
    memset(&g_iap2UserConfig, 0, sizeof(g_iap2UserConfig));

    SetGlobPtr(&g_iap2TestDevice);

    /**
     * get user configuration. iap2GetArguments() gets runtime user inputs and
     * stores in global g_iap2UserConfig Structure.
     */
    rc = iap2GetArgument(argc, argv);

    if (rc == IAP2_OK) {
        socketpair(PF_LOCAL, SOCK_STREAM, 0, socket_fds);
        host_device_switch_process_pid = fork();
        while(g_loopcount > 0 && g_rc == IAP2_OK)
        {
            g_loopcount --;
            iap2_host_mode = g_iap2TestDevice.iap2USBHostMode;
            memset(&g_iap2TestDevice, 0, sizeof(g_iap2TestDevice));
            g_iap2TestDevice.iap2USBHostMode = iap2_host_mode;
        if (host_device_switch_process_pid == 0) {
            if (0 != socket_fds[application_process] ){
                close(socket_fds[application_process]);
                socket_fds[application_process]=0;
            }

            host_device_switch_process_main(socket_fds[host_device_switch_process], &g_iap2UserConfig);
            var = host_device_switch_process;
            rc = 0;
        } else {
            if (0 != socket_fds[host_device_switch_process]){
                close(socket_fds[host_device_switch_process]);
                socket_fds[host_device_switch_process] = 0;
            }

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
            var = application_process;
        }
        iap2TestFreePtr( (void**)&g_iap2UserConfig.app_thread_groups);
        }
        close(socket_fds[var]);

    }
    return rc;
}
