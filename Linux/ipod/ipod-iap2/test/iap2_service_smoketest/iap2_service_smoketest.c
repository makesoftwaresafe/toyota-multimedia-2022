/*
 * iap2_smoketest.c
 */

/* **********************  includes  ********************** */
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>
#include <sys/stat.h>

#include "iap2_smoketest.h"
#include "iap2_test_gstreamer.h"
#include "iap2_usb_role_switch.h"

#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include <iap2_external_accessory_protocol_session.h>
#include <endian.h>
#include <iap2_initialize_usb_gadget.h>
#include <iap2_configure_ffs_gadget.h>

#include <iap2_service_init.h>
#include <sys/eventfd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>

#include <fcntl.h>
#include <sys/epoll.h>

#include "iap2_dlt_log.h"

#define IAP2_CONFIGFS_MOUNT_LOCATION "/sys/kernel/config"
#define IAP2_ADHERE_TO_APPLE_ENDIANESS_16(x) htobe16(x)
#define IAP2_EA_SESSION_IDENTFIER_LENGTH 2
#define IPOD_SHIFT_8 8

typedef struct
{
    int socket_fd;
    iAP2InitParam_t iAP2InitParameter;
    U8* udevPath;
} iap2AppThreadInitData_t;


LOCAL S32 application_process_main (int socket_fd);

/* application thread */
LOCAL void iap2AppThread(void* exinf);

/* test functions */
LOCAL S32 iap2TestStopPollThread(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartPlayMediaLibraryItem(S32 mq_fd, iAP2Device_t* iap2Device, U32 startIndex, U16 numSongsToPlay);

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

LOCAL S32 iap2TestStartCommunicationsUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopCommunicationsUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartCallStateUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopCallStateUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartListUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopListUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestInitiateCall(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestAcceptCall(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestMergeCalls(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestSwapCalls(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestEndCall(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestSendDTMF(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestHoldStatusUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestMuteStatusUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestAccessoryWiFiConfigurationInformation(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestOOBBTPairingAccessoryInformation(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestOOBBTPairingCompletionInformation(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestBluetoothPairingAccessoryInformation(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestBluetoothPairingStatus(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartAppDiscoveryUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopAppDiscoveryUpdates(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestRequestAppDiscoveryAppIcons(S32 mq_fd, iAP2Device_t* iap2Device);
/*                      Global Variables                    */
S32 g_rc = IAP2_OK;
U8 g_shutdown1 = 0;
iap2UserConfig_t g_iap2UserConfig;
iap2TestAppleDevice_t g_iap2TestDevice;
char c_srate[] = {"44100,48000"};
char p_srate[] = {"44100,48000"};

/* iAP2 service adaptation */
int g_eventFd = -1;
iAP2Device_t* g_iAP2Device;
iAP2Service_t* g_service;
volatile uint32_t g_deviceId = 0;
iAP2InitParam_t* g_iAP2InitParam;
/**/

/* **********************  callbacks ********************** */
S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    switch(dState)
    {
        case iAP2NotConnected :
            printf("\t %u ms 0x%p  Device state : iAP2NotConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2TransportConnected :
            printf("\t %u ms 0x%p  Device state : iAP2TransportConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkConnected:
            printf("\t %u ms 0x%p  Device state : iAP2LinkConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2AuthenticationPassed :
            printf("\t %u ms 0x%p  Device state : iAP2AuthenticationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2IdentificationPassed :
            printf("\t %u ms 0x%p  Device state : iAP2IdentificationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2DeviceReady:
            printf("\t %u ms 0x%p  Device state : iAP2DeviceReady  \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkiAP1DeviceDetected:
            printf("\t %u ms 0x%p  Device state : iAP2LinkiAP1DeviceDetected\n", iap2CurrTimeMs(), iap2Device);
            break;
        default:
            printf("\t %u ms 0x%p  Device state : unknown %d \n", iap2CurrTimeMs(), iap2Device, dState);
            switch(iAP2GetDeviceErrorState(iap2Device, NULL))
            {
                case iAP2NoError:
                    printf("\t %u ms 0x%p  Device ErrorState:  iAP2NoError \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2TransportConnectionFailed:
                    printf("\t %u ms 0x%p  DeviceErrorState:  iAP2TransportConnectionFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2LinkConnectionFailed:
                    printf("\t %u ms 0x%p  DeviceErrorState:  iAP2LinkConnectionFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2AuthenticationFailed:
                    printf("\t %u ms 0x%p  DeviceErrorState:  iAP2AuthenticationFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                case iAP2IdentificationFailed:
                    printf("\t %u ms 0x%p  DeviceErrorState:  iAP2IdentificationFailed \n", iap2CurrTimeMs(), iap2Device);
                    break;
                default:
                    printf("\t %u ms 0x%p  DeviceErrorState: unknown \n", iap2CurrTimeMs(), iap2Device);
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
    printf("\t %u ms 0x%p  iap2AuthenticationSucceeded_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2AuthenticationFailed_CB(iAP2Device_t* iap2Device, iAP2AuthenticationFailedParameter* authParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    authParameter = authParameter;
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AuthenticationFailed_CB called");
    printf("\t %u ms 0x%p  iap2AuthenticationFailed_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2IdentificationAccepted_CB(iAP2Device_t* iap2Device, iAP2IdentificationAcceptedParameter* idParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    idParameter = idParameter;
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2IdentificationAccepted_CB called");
    printf("\t %u ms 0x%p  iap2IdentificationAccepted_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2IdentificationRejected_CB(iAP2Device_t* iap2Device, iAP2IdentificationRejectedParameter* idParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    idParameter = idParameter;
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2IdentificationRejected_CB called");
    printf("\t %u ms 0x%p  iap2IdentificationRejected_CB called \n", iap2CurrTimeMs(), iap2Device);

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
        printf("\t %u ms 0x%p  iap2PowerUpdate_CB CurrentDrawn: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2AccessoryPowerMode_count != 0)
    {
        printf("\t %u ms 0x%p  iap2PowerUpdate_CB AccessoryPowerMode: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2AccessoryPowerMode));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count != 0)
    {
        printf("\t %u ms 0x%p  iap2PowerUpdate_CB BatteryWillCharge: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2IsExternalChargerConnected_count != 0)
    {
        printf("\t %u ms 0x%p  iap2PowerUpdate_CB ExternalChargerConnected\n", iap2CurrTimeMs(), iap2Device);
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2BatteryChargingState_count != 0)
    {
        printf("\t %u ms 0x%p  iap2PowerUpdate_CB BatteryChargingState: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2BatteryChargingState));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2BatteryChargeLevel_count != 0)
    {
        printf("\t %u ms 0x%p  iap2PowerUpdate_CB BatteryChargeLevel: %d \n", iap2CurrTimeMs(), iap2Device,
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
    printf("\t %u ms 0x%p  iap2MediaLibraryInfo_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter != NULL)
    {
        if(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier != NULL)
        {
            g_iap2TestDevice.testMediaLibInfoID = (U8*)strndup( (const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier),
                                        strnlen((const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier), STRING_MAX) );
        }
    }
    if (g_iap2TestDevice.testMediaLibInfoID == NULL)
    {
        IAP2TESTDLTLOG(DLT_LOG_WARN, "testMediaLibInfoID is NULL");
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
    printf("\t %u ms 0x%p  iap2MediaLibraryUpdates_CB called \n", iap2CurrTimeMs(), iap2Device);

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
        printf("\t %u ms 0x%p  Device is PlayAllSongsCapable \n", iap2CurrTimeMs(), iap2Device);
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
    printf("\t %u ms 0x%p  iap2NowPlayingUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

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
            printf("\t %u ms 0x%p  iap2NowPlayingUpdate_CB():  iAP2MediaItemAttributes is NULL.\n",
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
                           i, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds_count, *(NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds) );
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
            printf("\t %u ms 0x%p  iap2NowPlayingUpdate_CB():  iAP2PlaybackAttributes is NULL.\n",
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

    printf("\t %u ms 0x%p  iap2FileTransferSetup_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %u ms 0x%p  iap2FileTransferDataRcvd_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %u ms 0x%p  iap2FileTransferSuccess_CB called \n", iap2CurrTimeMs(), iap2Device);

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
                free(g_iap2TestDevice.coverArtBuf.Buffer);
                g_iap2TestDevice.coverArtBuf.Buffer = NULL;
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

    printf("\t %u ms 0x%p  iap2FileTransferFailure_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %u ms 0x%p  iap2FileTransferCancel_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %u ms 0x%p  iap2FileTransferPause_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %u ms 0x%p  iap2FileTransferResume_CB called \n", iap2CurrTimeMs(), iap2Device);

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
    printf("\t %u ms 0x%p  iap2StartExternalAccessoryProtocolSession_CB called \n", iap2CurrTimeMs(), iap2Device);
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
    printf("\t %u ms 0x%p  iap2StopExternalAccessoryProtocolSession_CB called \n", iap2CurrTimeMs(), iap2Device);
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
    printf("\t %u ms 0x%p  iap2iOSAppDataReceived_CB called. iAP2iOSAppIdentifier = %d, iAP2iOSAppDataLength = %d \n",
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

S32 iap2iOSMultiAppDataReceived_CB(iAP2Device_t* iap2Device, U8* iAP2iOSAppDataRxd, U16 iAP2iOSAppDataLength, void* context)
{
    S32 rc = IAP2_OK;
    U16 i;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2iOSMultiAppDataReceived_CB called \n");
    printf("\t %u ms 0x%p  iap2iOSMultiAppDataReceived_CB called. iAP2iOSAppDataLength = %d \n",
            iap2CurrTimeMs(), iap2Device, iAP2iOSAppDataLength);

    printf(" Following EAPSession Datagram received from device: \n");
    for(i = 0; i < iAP2iOSAppDataLength; i++)
    {
        if( (i % 10) == 0)
            printf("\n");
        printf(" 0x%.2X, ", iAP2iOSAppDataRxd[i]);
    }
    printf("\n");

    U16 EASessionIdentifier = ( ( ((U16)iAP2iOSAppDataRxd[0]) << IPOD_SHIFT_8) | (U16)iAP2iOSAppDataRxd[1]);
    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2iOSMultiAppDataReceived_CB called : Session Identifier %d", EASessionIdentifier);

    iap2SetTestState(EAP_SESSION_DATA_RECV, TRUE);
    return rc;
}

S32 iap2StartEANativeTransport_CB(iAP2Device_t* iap2Device, U8 iAP2iOSAppIdentifier, U8 sinkEndpoint, U8 sourceEndpoint, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    printf("\t %u ms 0x%p  iap2StartEANativeTransport_CB called \n", iap2CurrTimeMs(), iap2Device);
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

    printf("\t %u ms 0x%p  iap2StopEANativeTransport_CB called \n", iap2CurrTimeMs(), iap2Device);
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
    printf("\t %u ms 0x%p  iap2USBDeviceModeAudioInformation_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %u ms 0x%p  iap2DeviceInformationUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);
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
    printf("\t %u ms 0x%p  iap2DeviceTimeUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);
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

S32 iap2CommunicationsUpdate_CB(iAP2Device_t* iap2Device, iAP2CommunicationsUpdateParameter* communicationsupdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2CommunicationsUpdate_CB called");
    printf("\t %u ms 0x%p  iap2CommunicationsUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    if ((communicationsupdateParameter->iAP2AirplaneModeStatus_count != 0) && (communicationsupdateParameter->iAP2AirplaneModeStatus != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2AirplaneModeStatus: %d", *(communicationsupdateParameter->iAP2AirplaneModeStatus));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2CarrierName_count != 0) && (communicationsupdateParameter->iAP2CarrierName != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2CarrierName : %s \n", *(communicationsupdateParameter->iAP2CarrierName));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2CellularSupported_count != 0) && (communicationsupdateParameter->iAP2CellularSupported != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2CellularSupported: %d \n", *(communicationsupdateParameter->iAP2CellularSupported));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2CurrentCallCount_count != 0) && (communicationsupdateParameter->iAP2CurrentCallCount != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2CurrentCallCount: %d \n", *(communicationsupdateParameter->iAP2CurrentCallCount));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2EndAndAcceptAvailable_count != 0) && (communicationsupdateParameter->iAP2EndAndAcceptAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2EndAndAcceptAvailable: %d \n", +*(communicationsupdateParameter->iAP2EndAndAcceptAvailable));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2FaceTimeAudioEnabled_count != 0) && (communicationsupdateParameter->iAP2FaceTimeAudioEnabled != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2FaceTimeAudioEnabled: %d \n", *(communicationsupdateParameter->iAP2FaceTimeAudioEnabled));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2FaceTimeVideoEnabled_count != 0) && (communicationsupdateParameter->iAP2FaceTimeVideoEnabled != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2FaceTimeVideoEnabled: %d \n", *(communicationsupdateParameter->iAP2FaceTimeVideoEnabled));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2HoldAndAcceptAvailable_count != 0) && (communicationsupdateParameter->iAP2HoldAndAcceptAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2HoldAndAcceptAvailable: %d \n", *(communicationsupdateParameter->iAP2HoldAndAcceptAvailable));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2InitiateCallAvailable_count != 0) && (communicationsupdateParameter->iAP2InitiateCallAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2InitiateCallAvailable: %d \n", *(communicationsupdateParameter->iAP2InitiateCallAvailable));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2MergeAvailable_count != 0) && (communicationsupdateParameter->iAP2MergeAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2MergeAvailable: %d \n", *(communicationsupdateParameter->iAP2MergeAvailable));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2MuteStatus_count != 0) && (communicationsupdateParameter->iAP2MuteStatus != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2MuteStatus: %d \n", *(communicationsupdateParameter->iAP2MuteStatus));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2NewVoicemailCount_count != 0) && (communicationsupdateParameter->iAP2NewVoicemailCount != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2NewVoicemailCount: %d \n", *(communicationsupdateParameter->iAP2NewVoicemailCount));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2RegistrationStatus_count != 0) && (communicationsupdateParameter->iAP2RegistrationStatus != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2RegistrationStatus: %d \n", *(communicationsupdateParameter->iAP2RegistrationStatus));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2SignalStrength_count != 0) && (communicationsupdateParameter->iAP2SignalStrength != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2SignalStrength: %d \n", *(communicationsupdateParameter->iAP2SignalStrength));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2SwapAvailable_count != 0) && (communicationsupdateParameter->iAP2SwapAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2SwapAvailable: %d \n", *(communicationsupdateParameter->iAP2SwapAvailable));
        rc = IAP2_OK;
    }
    if ((communicationsupdateParameter->iAP2TelephonyEnabled_count != 0) && (communicationsupdateParameter->iAP2TelephonyEnabled != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2TelephonyEnabled: %d \n", *(communicationsupdateParameter->iAP2TelephonyEnabled));
        rc = IAP2_OK;
    }

    return rc;
}

S32 iap2CallStateUpdate_CB(iAP2Device_t* iap2Device, iAP2CallStateUpdateParameter* callstateupdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2CallStateUpdate_CB called");
    printf("\t %u ms 0x%p  iap2CallStateUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    if ((callstateupdateParameter->iAP2AddressBookID_count != 0) && (callstateupdateParameter->iAP2AddressBookID != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2AddressBookID: %s \n", *(callstateupdateParameter->iAP2AddressBookID));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2CallUUID_count != 0) && (callstateupdateParameter->iAP2AddressBookID != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2CallUUID : %s \n", *(callstateupdateParameter->iAP2CallUUID));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2ConferenceGroup_count != 0) && (callstateupdateParameter->iAP2AddressBookID != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2ConferenceGroup: %d \n", *(callstateupdateParameter->iAP2ConferenceGroup));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2Direction_count != 0) && (callstateupdateParameter->iAP2Direction != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Direction: %d \n", *(callstateupdateParameter->iAP2Direction));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2DisconnectReason_count != 0) && (callstateupdateParameter->iAP2DisconnectReason != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2DisconnectReason: %d \n", *(callstateupdateParameter->iAP2DisconnectReason));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2DisplayName_count != 0) && (callstateupdateParameter->iAP2DisplayName != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2DisplayName: %s \n", *(callstateupdateParameter->iAP2DisplayName));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2IsConferenced_count != 0) && (callstateupdateParameter->iAP2IsConferenced != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2IsConferenced: %d \n", *(callstateupdateParameter->iAP2IsConferenced));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2Label_count != 0) && (callstateupdateParameter->iAP2Label != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Label: %s \n", *(callstateupdateParameter->iAP2Label));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2RemoteID_count != 0) && (callstateupdateParameter->iAP2RemoteID != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2RemoteID: %s \n", *(callstateupdateParameter->iAP2RemoteID));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2Service_count != 0) && (callstateupdateParameter->iAP2Service != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Service: %d \n", *(callstateupdateParameter->iAP2Service));
        rc = IAP2_OK;
    }
    if ((callstateupdateParameter->iAP2Status_count != 0) && (callstateupdateParameter->iAP2Status != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Status: %d \n", *(callstateupdateParameter->iAP2Status));
        rc = IAP2_OK;
    }

    return rc;
}

S32 iap2ListUpdate_CB(iAP2Device_t* iap2Device, iAP2ListUpdateParameter* listUpdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2ListUpdate_CB called");
    printf("\t %u ms 0x%p  iap2ListUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (listUpdateParameter->iAP2FavoritesList_count != 0)
    {
        if (listUpdateParameter->iAP2FavoritesList != NULL)
        {
            if ((listUpdateParameter->iAP2FavoritesList->iAP2AddressBookID_count != 0) && (listUpdateParameter->iAP2FavoritesList->iAP2AddressBookID != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2AddressBookID: %s \n", *(listUpdateParameter->iAP2FavoritesList->iAP2AddressBookID));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2FavoritesList->iAP2DisplayName_count != 0) && (listUpdateParameter->iAP2FavoritesList->iAP2DisplayName != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2DisplayName: %s \n", *(listUpdateParameter->iAP2FavoritesList->iAP2DisplayName));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2FavoritesList->iAP2Index_count != 0) && (listUpdateParameter->iAP2FavoritesList->iAP2Index != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Index: %d \n", *(listUpdateParameter->iAP2FavoritesList->iAP2Index));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2FavoritesList->iAP2Label_count != 0) && (listUpdateParameter->iAP2FavoritesList->iAP2Label != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Label: %s \n", *(listUpdateParameter->iAP2FavoritesList->iAP2Label));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2FavoritesList->iAP2RemoteID_count != 0) && (listUpdateParameter->iAP2FavoritesList->iAP2RemoteID != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2RemoteID: %s \n", *(listUpdateParameter->iAP2FavoritesList->iAP2RemoteID));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2FavoritesList->iAP2Service_count != 0) && (listUpdateParameter->iAP2FavoritesList->iAP2Service != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Service: %d \n", *(listUpdateParameter->iAP2FavoritesList->iAP2Service));
                rc = IAP2_OK;
            }
        }
        else
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2ListUpdate_CB(): iAP2FavoritesList is NULL.\n");
            rc = IAP2_CTL_ERROR;
        }
    }

    if ((listUpdateParameter->iAP2FavoritesListAvailable_count != 0) && (listUpdateParameter->iAP2FavoritesListAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2FavoritesListAvailable : %d \n", *(listUpdateParameter->iAP2FavoritesListAvailable));
        rc = IAP2_OK;
    }

    if ((listUpdateParameter->iAP2FavoritesListCount_count != 0) && (listUpdateParameter->iAP2FavoritesListCount != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2FavoritesListCount: %d \n", *(listUpdateParameter->iAP2FavoritesListCount));
        rc = IAP2_OK;
    }

    if ((listUpdateParameter->iAP2RecentsListAvailable_count != 0) && (listUpdateParameter->iAP2RecentsListAvailable != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2RecentsListAvailable: %d \n", *(listUpdateParameter->iAP2RecentsListAvailable));
        rc = IAP2_OK;
    }

    if ((listUpdateParameter->iAP2RecentsListCount_count != 0) && (listUpdateParameter->iAP2RecentsListCount != NULL))
    {
        IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2RecentsListCount: %d \n", *(listUpdateParameter->iAP2RecentsListCount));
        rc = IAP2_OK;
    }

    if (listUpdateParameter->iAP2RecentsList_count != 0)
    {
        if (listUpdateParameter->iAP2RecentsList != NULL)
        {
            if ((listUpdateParameter->iAP2RecentsList->iAP2AddressBookID_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2AddressBookID != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2AddressBookID: %s \n", *(listUpdateParameter->iAP2RecentsList->iAP2AddressBookID));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2DisplayName_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2DisplayName != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2DisplayName: %s \n", *(listUpdateParameter->iAP2RecentsList->iAP2DisplayName));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2Duration_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2Duration != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Duration: %d \n", *(listUpdateParameter->iAP2RecentsList->iAP2Duration));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2Index_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2Index != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Index: %d \n", *(listUpdateParameter->iAP2RecentsList->iAP2Index));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2Label_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2Label != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Label: %s \n", *(listUpdateParameter->iAP2RecentsList->iAP2Label));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2Occurrences_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2Occurrences != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Occurrences: %d \n", *(listUpdateParameter->iAP2RecentsList->iAP2Occurrences));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2RemoteID_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2RemoteID != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2RemoteID: %s \n", *(listUpdateParameter->iAP2RecentsList->iAP2RemoteID));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2Service_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2Service != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Service: %d \n", *(listUpdateParameter->iAP2RecentsList->iAP2Service));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2Type_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2Type != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2Type: %d \n", *(listUpdateParameter->iAP2RecentsList->iAP2Type));
                rc = IAP2_OK;
            }
            if ((listUpdateParameter->iAP2RecentsList->iAP2UnixTimestamp_count != 0) && (listUpdateParameter->iAP2RecentsList->iAP2UnixTimestamp != NULL))
            {
                IAP2TESTDLTLOG(DLT_LOG_VERBOSE, "iAP2UnixTimestamp: %lld \n", *(listUpdateParameter->iAP2RecentsList->iAP2UnixTimestamp));
                rc = IAP2_OK;
            }
        }
        else
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2ListUpdate_CB():  iAP2RecentsList is NULL.\n");
            rc = IAP2_CTL_ERROR;
        }
    }

    return rc;
}

S32 iap2DevUUIDUpdate_CB(iAP2Device_t* iap2Device, iAP2DeviceUUIDUpdateParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2DevUUIDUpdate_CB called");
    printf("\t %u ms 0x%p  iap2DevUUIDUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (thisParameter != NULL)
    {
        if ( (thisParameter->iAP2UUID_count != 0) && (thisParameter->iAP2UUID != NULL) )
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2UUID : %s \n", *(thisParameter->iAP2UUID));
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

S32 iap2WirelessCplayUpdate_CB(iAP2Device_t* iap2Device, iAP2WirelessCarPlayUpdateParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2WirelessCplayUpdate_CB called");
    printf("\t %u ms 0x%p  iap2WirelessCplayUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (thisParameter != NULL)
    {
        if ( (thisParameter->iAP2Status_count != 0) && (thisParameter->iAP2Status != NULL) )
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2Status : %d \n", *(thisParameter->iAP2Status));
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

S32 iap2AppDiscoveryAppIcon_CB(iAP2Device_t* iap2Device, iAP2AppDiscoveryAppIconParameter* thisParameter, void* context)
{

    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AppDiscoveryAppIcon_CB called");
    printf("\t %u ms 0x%p  iap2AppDiscoveryAppIcon_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (thisParameter != NULL)
    {
        if((thisParameter->iAP2AppBundleID_count != 0) && (thisParameter->iAP2AppBundleID != NULL))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2AppBundleID : %d", *(thisParameter->iAP2AppBundleID));
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2AppIconAvailable_count != 0) && (thisParameter->iAP2AppIconAvailable != NULL))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2AppIconAvaialble : %d", *(thisParameter->iAP2AppIconAvailable));
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2AppIconFileTransferID != 0) && (thisParameter->iAP2AppIconFileTransferID != NULL))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2AppIconFileTransferID : %d", *(thisParameter->iAP2AppIconFileTransferID));
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2AppIconIdentifier_count != 0) && (thisParameter->iAP2AppIconIdentifier != NULL))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2AppIconIdentifier : %d", *(thisParameter->iAP2AppIconIdentifier));
            rc = IAP2_OK;
        }
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2AppDiscoveryAppIcon_CB is NULL");
        rc = IAP2_CTL_ERROR;
    }
    return rc;
}

S32 iap2AppDiscoveryUpdates_CB(iAP2Device_t* iap2Device, iAP2AppDiscoveyUpdatesParameter* thisParameter, void* context)
{

    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AppDiscoveryUpdates_CB called");
    printf("\t %u ms 0x%p  iap2AppDiscoveryUpdates_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (thisParameter != NULL)
    {
        if((thisParameter->iAP2CarPlayAppList_count != 0) && (thisParameter->iAP2CarPlayAppList != NULL))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2CarPlayAppList : %d", *(thisParameter->iAP2CarPlayAppList));
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2CarPlayAppListAvailable_count != 0) && (thisParameter->iAP2CarPlayAppListAvailable))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2CarPlayAppListAvailable : %d", *(thisParameter->iAP2CarPlayAppListAvailable));
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2CarPlayAppListCount_count != 0) && (thisParameter->iAP2CarPlayAppListCount != NULL))
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2CarPlayAppListCount : %d", *(thisParameter->iAP2CarPlayAppListCount));
            rc = IAP2_OK;
        }
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2AppDiscoveryUpdates_CB is NULL");
        rc = IAP2_CTL_ERROR;
    }
    return rc;
}

S32 iap2StartOOBBTPairing_CB(iAP2Device_t* iap2Device, iAP2StartOOBBTPairingParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    thisParameter = thisParameter;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StartOOBBTPairing_CB called");
    printf("\t %u ms 0x%p  iap2StartOOBBTPairing_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2StopOOBBTPairing_CB(iAP2Device_t* iap2Device, iAP2StopOOBBTPairingParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    thisParameter = thisParameter;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StopOOBBTPairing_CB called");
    printf("\t %u ms 0x%p  iap2StopOOBBTPairing_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2StartBluetoothPairing_CB(iAP2Device_t* iap2Device, iAP2StartBluetoothPairingParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StartBluetoothPairing_CB called");
    printf("\t %u ms 0x%p  iap2StartBluetoothPairing_CB called \n", iap2CurrTimeMs(), iap2Device);
    if(thisParameter != NULL)
    {
        if((thisParameter->iAP2BluetoothTransportComponentIdentifier_count != 0) && (thisParameter->iAP2BluetoothTransportComponentIdentifier != NULL))
        {
            printf("iAP2BluetoothTransportComponentIdentifier : %d", (thisParameter->iAP2BluetoothTransportComponentIdentifier));
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2DeviceMACAddress_count != 0) && (thisParameter->iAP2DeviceMACAddress != NULL))
        {
            printf("iAP2DeviceMACAddress  : %d", (thisParameter->iAP2DeviceMACAddress));
            rc = IAP2_OK;
        }
    }
    else{
        printf("StartBluetoothPairing is null");
        rc = IAP2_CTL_ERROR;
    }
    return rc;
}

S32 iap2StopBluetoothPairing_CB(iAP2Device_t* iap2Device, iAP2StopBluetoothPairingParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2StopBluetoothPairing_CB called");
    printf("\t %u ms 0x%p  iap2StopBluetoothPairing_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(thisParameter != NULL)
    {
        if((thisParameter->iAP2BluetoothTransportComponentIdentifier_count != 0) && (thisParameter->iAP2BluetoothTransportComponentIdentifier != NULL))
        {
            printf("iAP2BluetoothTransportComponentIdentifier %d", thisParameter->iAP2BluetoothTransportComponentIdentifier);
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2AlreadyPaired_count != 0) && (thisParameter->iAP2AlreadyPaired != NULL))
        {
            printf("iAP2AlreadyPaired : %d", thisParameter->iAP2AlreadyPaired);
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2PairingCancel_count != 0) && (thisParameter->iAP2PairingCancel != NULL))
        {
            printf("iAP2PairingCancel: %d", thisParameter->iAP2PairingCancel);
            rc = IAP2_OK;
        }
        if((thisParameter->iAP2Success_count != 0) && (thisParameter->iAP2Success != NULL))
        {
            printf("iAP2Success : %d", thisParameter->iAP2Success);
            rc = IAP2_OK;
        }
    }
    else
    {
        printf("iap2StopBluetoothPairing_CB is null");
        rc = IAP2_CTL_ERROR;
    }
    return rc;
}

S32 iap2OOBBTPairingLinkKeyInformation_CB(iAP2Device_t* iap2Device, iAP2OOBBTPairingLinkKeyInformationParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    thisParameter = thisParameter;
    int i = 0;
    int j = 0;
    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2OOBBTPairingLinkKeyInformation_CB called");
    printf("\t %u ms 0x%p  iap2OOBBTPairingLinkKeyInformation_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (thisParameter != NULL)
    {
        if ( (thisParameter->iAP2AppleDeviceMACAddress_count != 0) && (thisParameter->iAP2AppleDeviceMACAddress != NULL) )
        {
            for(i = 0; i < thisParameter->iAP2AppleDeviceMACAddress->iAP2BlobLength; i++)
            {
                printf("%02x", *(thisParameter->iAP2AppleDeviceMACAddress->iAP2BlobData + i));
            }
            printf("\t Apple Device Mac Address \n");
        }
        if ( (thisParameter->iAP2LinkKey_count != 0) && (thisParameter->iAP2LinkKey != NULL) )
        {
            for(j = 0; j < thisParameter->iAP2LinkKey->iAP2BlobLength; j++)
            {
                printf("%02x", *(thisParameter->iAP2LinkKey->iAP2BlobData + j));
            }
            printf("\t Link Key from Device \n");
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2GPRMCDataStatusValuesNotification_CB(iAP2Device_t* iap2Device, iAP2GPRMCDataStatusValuesNotificationParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    thisParameter = thisParameter;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2GPRMCDataStatusValuesNotification_CB called");
    printf("\t %u ms 0x%p  iap2GPRMCDataStatusValuesNotification_CB called \n", iap2CurrTimeMs(), iap2Device);

    if (thisParameter != NULL)
    {
        if (thisParameter->iAP2GPRMCDataStatusValueA_count != 0)
        {
            printf("Apple Device can support GPRMC Data Status field as 'A' \n");
        }
        if (thisParameter->iAP2GPRMCDataStatusValueV_count != 0)
        {
            printf("Apple Device can support GPRMC Data Status field as 'V' \n");
        }
        if (thisParameter->iAP2GPRMCDataStatusValueX_count != 0)
        {
            printf("Apple Device can support GPRMC Data Status field as 'X' \n");
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    return rc;
}

S32 iap2RequestAccWiFiConfig_CB(iAP2Device_t* iap2Device, iAP2RequestAccessoryWiFiConfigurationInformationParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    thisParameter = thisParameter;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2RequestAccWiFiConfig_CB called");
    printf("\t %u ms 0x%p  iap2RequestAccWiFiConfig_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}
/* **********************  register callback functions ********************** */

void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
}

void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks, BOOL iap2EAPSupported, BOOL iap2iOSintheCar)
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
    iap2CSCallbacks->iAP2CommunicationsUpdate_cb                          = &iap2CommunicationsUpdate_CB;
    iap2CSCallbacks->iAP2CallStateUpdate_cb                               = &iap2CallStateUpdate_CB;
    iap2CSCallbacks->iAP2ListUpdate_cb                                    = &iap2ListUpdate_CB;
    iap2CSCallbacks->iAP2DeviceUUIDUpdate_cb                              = &iap2DevUUIDUpdate_CB;
    if(iap2iOSintheCar == TRUE)
    {
        iap2CSCallbacks->iAP2StartOOBBTPairing_cb                             = &iap2StartOOBBTPairing_CB;
        iap2CSCallbacks->iAP2StopOOBBTPairing_cb                              = &iap2StopOOBBTPairing_CB;
        iap2CSCallbacks->iAP2OOBBTPairingLinkKeyInformation_cb                = &iap2OOBBTPairingLinkKeyInformation_CB;
        iap2CSCallbacks->iAP2StartBluetoothPairing_cb                         = &iap2StartBluetoothPairing_CB;
        iap2CSCallbacks->iAP2StopBluetoothPairing_cb                          = &iap2StopBluetoothPairing_CB;
    }
    iap2CSCallbacks->iAP2GPRMCDataStatusValuesNotification_cb             = &iap2GPRMCDataStatusValuesNotification_CB;
    iap2CSCallbacks->iAP2RequestAccessoryWiFiConfigurationInformation_cb  = &iap2RequestAccWiFiConfig_CB;
    iap2CSCallbacks->iAP2WirelessCarPlayUpdate_cb                         = &iap2WirelessCplayUpdate_CB;
    iap2CSCallbacks->iAP2AppDiscoveryAppIcon_cb                           = &iap2AppDiscoveryAppIcon_CB;
    iap2CSCallbacks->iAP2AppDiscoveryUpdates_cb                           = &iap2AppDiscoveryUpdates_CB;
}

void iap2InitEAPSessionCallbacks(iAP2EAPSessionCallbacks_t* iAP2EAPSessionCallbacks)
{
    iAP2EAPSessionCallbacks->iAP2iOSAppDataReceived_cb = NULL;
}

void iap2InitMultiEAPSessionCallbacks(iAP2MultiEAPSessionCallbacks_t* iAP2MultiEAPSessionCallbacks)
{
    iAP2MultiEAPSessionCallbacks->iAP2iOSMultiAppDataReceived_cb = &iap2iOSMultiAppDataReceived_CB;
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

LOCAL S32 iap2TestStartCommunicationsUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2StartCommunicationsUpdatesParameter theiAP2StartCommunicationsUpdatesParameter;

    memset(&theiAP2StartCommunicationsUpdatesParameter, 0, sizeof(iAP2StartCommunicationsUpdatesParameter));
    theiAP2StartCommunicationsUpdatesParameter.iAP2SignalStrength_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2AirplaneModeStatus_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2CarrierName_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2CurrentCallCount_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2EndAndAcceptAvailable_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2FaceTimeAudioEnabled_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2FaceTimeVideoEnabled_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2HoldAndAcceptAvailable_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2HoldAvailable_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2CellularSupported_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2InitiateCallAvailable_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2MergeAvailable_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2MuteStatus_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2NewVoiceMailCount_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2RegistrationStatus_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2SwapAvailable_count++;
    theiAP2StartCommunicationsUpdatesParameter.iAP2TelephonyEnabled_count++;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_START_COMMUNICATIONS_UPDATE,
                           &theiAP2StartCommunicationsUpdatesParameter, sizeof(theiAP2StartCommunicationsUpdatesParameter));
    return rc;
}

LOCAL S32 iap2TestStopCommunicationsUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2StopCommunicationsUpdatesParameter theiAP2StopPowerUpdatesParameter;

    memset(&theiAP2StopPowerUpdatesParameter, 0, sizeof(iAP2StopCommunicationsUpdatesParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_COMMUNICATIONS_UPDATE,
                           &theiAP2StopPowerUpdatesParameter, sizeof(theiAP2StopPowerUpdatesParameter));
    return rc;
}

LOCAL S32 iap2TestStartCallStateUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
   int rc = IAP2_OK;
   iap2Device = iap2Device;
   iAP2StartCallStateUpdatesParameter theiAP2StartCallStateUpdatesParameter;

   memset(&theiAP2StartCallStateUpdatesParameter, 0, sizeof(iAP2StartCallStateUpdatesParameter));
   theiAP2StartCallStateUpdatesParameter.iAP2AddressBookID_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2CallUUID_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2ConferenceGroup_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2Direction_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2DisconnectReason_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2DisplayName_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2IsConferenced_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2Label_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2RemoteID_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2Service_count ++;
   theiAP2StartCallStateUpdatesParameter.iAP2Status_count ++;

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_START_CALL_STATE_UPDATES,
                           &theiAP2StartCallStateUpdatesParameter, sizeof(theiAP2StartCallStateUpdatesParameter));
    return rc;
}

LOCAL S32 iap2TestStopCallStateUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2StopCallStateUpdatesParameter theiAP2StopPowerUpdatesParameter;

    memset(&theiAP2StopPowerUpdatesParameter, 0, sizeof(iAP2StopCallStateUpdatesParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_CALL_STATE_UPDATES,
                           &theiAP2StopPowerUpdatesParameter, sizeof(theiAP2StopPowerUpdatesParameter));
    return rc;
}

LOCAL S32 iap2TestStartListUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2StartListUpdatesParameter theiAP2StartListUpdatesParameter;
    U8 RecentsListCombine = 1;
    U16 RecentsListMax = 0;
    U16 FavoritesListMax = 0;
    memset(&theiAP2StartListUpdatesParameter, 0, sizeof(iAP2StartListUpdatesParameter));

    rc = iap2AllocateandUpdateData(&theiAP2StartListUpdatesParameter.iAP2FavoritesListMax,
                                   &FavoritesListMax,
                                   &theiAP2StartListUpdatesParameter.iAP2FavoritesListMax_count,
                                   1, iAP2_uint16);
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartListUpdatesParameter.iAP2RecentsListMax,
                                       &RecentsListMax,
                                       &theiAP2StartListUpdatesParameter.iAP2RecentsListMax_count,
                                       1, iAP2_uint16);
    }
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartListUpdatesParameter.iAP2RecentListCombine,
                                       &RecentsListCombine,
                                       &theiAP2StartListUpdatesParameter.iAP2RecentListCombine_count,
                                       1, iAP2_uint8);
    }
    if (rc == IAP2_OK)
    {
        theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties_count++;
        theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties = (iAP2FavoritesListProperties*) calloc(1, sizeof(iAP2FavoritesListProperties));
        if (theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties != NULL)
        {
            theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties->iAP2AddressBookID_count++;
            theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties->iAP2DisplayName_count++;
            theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties->iAP2Index_count++;
            theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties->iAP2Label_count++;
            theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties->iAP2RemoteID_count++;
            theiAP2StartListUpdatesParameter.iAP2FavoritesListProperties->iAP2Service_count++;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    if (rc == IAP2_OK)
    {
        theiAP2StartListUpdatesParameter.iAP2RecentListProperties_count++;
        theiAP2StartListUpdatesParameter.iAP2RecentListProperties = (iAP2RecentsListProperties*) calloc(1, sizeof(iAP2RecentsListProperties));
        if (theiAP2StartListUpdatesParameter.iAP2RecentListProperties != NULL)
        {
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2AddressBookID_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2DisplayName_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2Duration_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2Index_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2RemoteID_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2Label_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2Occurrences_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2Service_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2Type_count++;
            theiAP2StartListUpdatesParameter.iAP2RecentListProperties->iAP2UnixTimestamp_count++;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_START_LIST_UPDATES,
                               &theiAP2StartListUpdatesParameter, sizeof(theiAP2StartListUpdatesParameter));
    }

    iAP2FreeiAP2StartListUpdatesParameter(&theiAP2StartListUpdatesParameter);
    return rc;
}

LOCAL S32 iap2TestStopListUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iAP2StopListUpdatesParameter theiAP2StopListUpdatesParameter;

    iap2Device = iap2Device;

    memset(&theiAP2StopListUpdatesParameter, 0, sizeof(iAP2StopListUpdatesParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_LIST_UPDATES,
                           &theiAP2StopListUpdatesParameter, sizeof(theiAP2StopListUpdatesParameter));
    return rc;
}

LOCAL S32 iap2TestInitiateCall(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2InitiateCallParameter theiAP2InitiateCallParameter;
    U8 DestinationID[] = {"0123456789"};
    U8* p_DestinationID = DestinationID;
    U8 AddressBookID[] = {"01234"};
    U8* p_AddressBookID = AddressBookID;

    memset(&theiAP2InitiateCallParameter, 0, sizeof(iAP2InitiateCallParameter));

    rc = iap2AllocateandUpdateData(&theiAP2InitiateCallParameter.iAP2DestinationID,
                                   &p_DestinationID,
                                   &theiAP2InitiateCallParameter.iAP2DestinationID_count,
                                   1, iAP2_utf8);

    if (rc == IAP2_OK)
    {
        theiAP2InitiateCallParameter.iAP2Type = calloc(1, sizeof(iAP2InitiateCallParameter));
        if (theiAP2InitiateCallParameter.iAP2Type != NULL)
        {
            *(theiAP2InitiateCallParameter.iAP2Type) = IAP2_INITIATE_CALL_TYPE_DESTINATION;
            theiAP2InitiateCallParameter.iAP2Type_count = 1;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }

    if (rc == IAP2_OK)
    {
        theiAP2InitiateCallParameter.iAP2Service = calloc(1, sizeof(iAP2InitiateCallParameter));
        if (theiAP2InitiateCallParameter.iAP2Service != NULL)
        {
            *(theiAP2InitiateCallParameter.iAP2Service) = IAP2_INITIATE_CALL_SERVICE_TELEPHONY;
            theiAP2InitiateCallParameter.iAP2Service_count = 1;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }

    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2InitiateCallParameter.iAP2AddressBookID,
                                       &p_AddressBookID,
                                       &theiAP2InitiateCallParameter.iAP2AddressBookID_count,
                                       1, iAP2_utf8);
    }

    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_INITIATE_CALL,
                               &theiAP2InitiateCallParameter, sizeof(theiAP2InitiateCallParameter));
    }

    iAP2FreeiAP2InitiateCallParameter(&theiAP2InitiateCallParameter);
    return rc;
}

LOCAL S32 iap2TestAcceptCall(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2AcceptCallParameter theiAP2AcceptCallParameter;
    U8  CallID[] = {"0123456789"};
    U8* p_CallID    = CallID;

    memset(&theiAP2AcceptCallParameter, 0, sizeof(iAP2AcceptCallParameter));

    rc = iap2AllocateandUpdateData(&theiAP2AcceptCallParameter.iAP2CallUUID,
                                   &p_CallID,
                                   &theiAP2AcceptCallParameter.iAP2CallUUID_count,
                                   1, iAP2_utf8);

    if (rc == IAP2_OK)
    {
        theiAP2AcceptCallParameter.iAP2AcceptAction = calloc(1, sizeof(iAP2AcceptCallAcceptAction));
        if (theiAP2AcceptCallParameter.iAP2AcceptAction != NULL)
        {
            *(theiAP2AcceptCallParameter.iAP2AcceptAction) = IAP2_ACCEPT_CALL_ACCEPT;
            theiAP2AcceptCallParameter.iAP2AcceptAction_count = 1;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }

    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_ACCEPT_CALL,
                               &theiAP2AcceptCallParameter, sizeof(theiAP2AcceptCallParameter));
    }

    iAP2FreeiAP2AcceptCallParameter(&theiAP2AcceptCallParameter);
    return rc;
}

LOCAL S32 iap2TestMergeCalls(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2MergeCallsParameter theiAP2MergeCallsParameter;

    memset(&theiAP2MergeCallsParameter, 0, sizeof(iAP2MergeCallsParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_MERGE_CALLS,
                           &theiAP2MergeCallsParameter, sizeof(theiAP2MergeCallsParameter));
    return rc;
}

LOCAL S32 iap2TestSwapCalls(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2SwapCallsParameter theiAP2SwapCallsParameter;

    memset(&theiAP2SwapCallsParameter, 0, sizeof(iAP2SwapCallsParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_SWAP_CALLS,
                           &theiAP2SwapCallsParameter, sizeof(theiAP2SwapCallsParameter));
    return rc;
}


LOCAL S32 iap2TestEndCall(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2EndCallParameter theiAP2EndCallParameter;
    U8 CallID[] = {"0123456789"};
    U8* p_CallID = CallID;

    memset(&theiAP2EndCallParameter, 0, sizeof(iAP2EndCallParameter));

    rc = iap2AllocateandUpdateData(&theiAP2EndCallParameter.iAP2CallUUID,
                                   &p_CallID,
                                   &theiAP2EndCallParameter.iAP2CallUUID_count,
                                   1, iAP2_utf8);

    if (rc == IAP2_OK)
    {
        theiAP2EndCallParameter.iAP2EndAction = calloc(1, sizeof(iAP2EndCallEndAction));
        if (theiAP2EndCallParameter.iAP2EndAction != NULL)
        {
            *(theiAP2EndCallParameter.iAP2EndAction) = IAP2_END;
            theiAP2EndCallParameter.iAP2EndAction_count = 1;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }

    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_END_CALL,
                               &theiAP2EndCallParameter, sizeof(theiAP2EndCallParameter));
    }

    iAP2FreeiAP2EndCallParameter(&theiAP2EndCallParameter);
    return rc;
}

LOCAL S32 iap2TestSendDTMF(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2SendDTMFParameter theiAP2SendDTMFParameter;
    U8 CallID[] = {"0123456789"};
    U8* p_CallID = CallID;

    memset(&theiAP2SendDTMFParameter, 0, sizeof(iAP2SendDTMFParameter));

    rc = iap2AllocateandUpdateData(&theiAP2SendDTMFParameter.iAP2CallUUID,
                                   &p_CallID,
                                   &theiAP2SendDTMFParameter.iAP2CallUUID_count,
                                   1, iAP2_utf8);

    if (rc == IAP2_OK)
    {
        theiAP2SendDTMFParameter.iAP2Tone = calloc(1, sizeof(iAP2EndCallEndAction));
        if (theiAP2SendDTMFParameter.iAP2Tone != NULL)
        {
            *(theiAP2SendDTMFParameter.iAP2Tone) = IAP2_DTMF_TONE_NUMBER_0;
            theiAP2SendDTMFParameter.iAP2Tone_count = 1;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }

    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_SEND_DTMF,
                               &theiAP2SendDTMFParameter, sizeof(theiAP2SendDTMFParameter));
    }

    iAP2FreeiAP2SendDTMFParameter(&theiAP2SendDTMFParameter);
    return rc;
}

LOCAL S32 iap2TestHoldStatusUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2HoldStatusUpdateParameter theiAP2HoldStatusUpdateParameter;
    U8 CallID[] = {"0123456789"};
    U8* p_CallID = CallID;
    U8 HoldStatus = 1;

    memset(&theiAP2HoldStatusUpdateParameter, 0, sizeof(iAP2HoldStatusUpdateParameter));
    rc = iap2AllocateandUpdateData(&theiAP2HoldStatusUpdateParameter.iAP2HoldStatusUpdate,
                                   &HoldStatus,
                                   &theiAP2HoldStatusUpdateParameter.iAP2HoldStatusUpdate_count,
                                   1, iAP2_uint8);

    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2HoldStatusUpdateParameter.iAP2CallUUID,
                                       &p_CallID,
                                       &theiAP2HoldStatusUpdateParameter.iAP2CallUUID_count,
                                       1, iAP2_utf8);
    }

    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_HOLD_STATUS_UPDATE,
                               &theiAP2HoldStatusUpdateParameter, sizeof(theiAP2HoldStatusUpdateParameter));
    }

    iAP2FreeiAP2HoldStatusUpdateParameter(&theiAP2HoldStatusUpdateParameter);
    return rc;
}

LOCAL S32 iap2TestMuteStatusUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2MuteStatusUpdateParameter theiAP2MuteStatusUpdateParameter;
    U8 MuteStatus = 1;

    memset(&theiAP2MuteStatusUpdateParameter, 0, sizeof(iAP2MuteStatusUpdateParameter));
    rc = iap2AllocateandUpdateData(&theiAP2MuteStatusUpdateParameter.iAP2MuteStatus,
                                   &MuteStatus,
                                   &theiAP2MuteStatusUpdateParameter.iAP2MuteStatus_count,
                                   1, iAP2_uint8);

    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_MUTE_STATUS_UPDATE,
                               &theiAP2MuteStatusUpdateParameter, sizeof(theiAP2MuteStatusUpdateParameter));
    }

    iAP2FreeiAP2MuteStatusUpdateParameter(&theiAP2MuteStatusUpdateParameter);
    return rc;
}

LOCAL S32 iap2TestAccessoryWiFiConfigurationInformation(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2AccessoryWiFiConfigurationInformationParameter theiAP2AccessoryWiFiConfigurationInformationParameter;
    U8 iAP2WiFiSSID[] = { "wifi" };
    U8 iAP2Passphrase[] = { "YourPassPhrase" };
    U8 iAP2Channel = 1;
    U8* p_iAP2WiFiSSID = iAP2WiFiSSID;
    U8* p_iAP2Passphrase = iAP2Passphrase;

    memset(&theiAP2AccessoryWiFiConfigurationInformationParameter, 0, sizeof(iAP2AccessoryWiFiConfigurationInformationParameter));

    /* set WiFiSSID */
    rc = iap2AllocateandUpdateData(&theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2WiFiSSID,
                                   &p_iAP2WiFiSSID,
                                   &theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2WiFiSSID_count,
                                   1, iAP2_utf8);
    if (rc == IAP2_OK)
    {
        /* set PassPhrase */
        rc = iap2AllocateandUpdateData(&theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2Passphrase,
                                       &p_iAP2Passphrase,
                                       &theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2Passphrase_count,
                                       1, iAP2_utf8);
    }
    if (rc == IAP2_OK)
    {
        /* set SecurityType */
        theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2SecurityType = calloc(1, sizeof(iAP2WiFiSecurityType));
        if (theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2SecurityType != NULL)
        {
            *(theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2SecurityType) = IAP2_WIFI_SECURITY_WPA2;
            theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2SecurityType_count = 1;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    if (rc == IAP2_OK)
    {
        /* set Channel */
        rc = iap2AllocateandUpdateData(&theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2Channel,
                                       &iAP2Channel,
                                       &theiAP2AccessoryWiFiConfigurationInformationParameter.iAP2Channel_count,
                                       1, iAP2_uint8);

    }
    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_WIFI_CONFIGURATION_INFORMATION,
                               &theiAP2AccessoryWiFiConfigurationInformationParameter, sizeof(theiAP2AccessoryWiFiConfigurationInformationParameter));
    }

    iAP2FreeiAP2AccessoryWiFiConfigurationInformationParameter(&theiAP2AccessoryWiFiConfigurationInformationParameter);
    return rc;
}

LOCAL S32 iap2TestOOBBTPairingAccessoryInformation(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iap2Device = iap2Device;

    U16 BT_TransCompIdentifier = IAP2_BT_TRANS_COMP_ID;
    U32 BT_DeviceClass = 0;
    iAP2OOBBTPairingAccessoryInformationParameter theiAP2OOBBTPairingAccessoryInformationParameter;
    memset(&theiAP2OOBBTPairingAccessoryInformationParameter, 0, sizeof(iAP2OOBBTPairingAccessoryInformationParameter));

    rc = iap2AllocateandUpdateData(&theiAP2OOBBTPairingAccessoryInformationParameter.iAP2BluetoothTransportComponentIdentifier ,
                                   &BT_TransCompIdentifier,
                                   &theiAP2OOBBTPairingAccessoryInformationParameter.iAP2BluetoothTransportComponentIdentifier_count,
                                   1, iAP2_uint16);
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2OOBBTPairingAccessoryInformationParameter.iAP2DeviceClass,
                                       &BT_DeviceClass,
                                       &theiAP2OOBBTPairingAccessoryInformationParameter.iAP2DeviceClass_count,
                                       1, iAP2_uint32);
    }
    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_OOB_BT_PAIRING_ACC_INFO,
                               &theiAP2OOBBTPairingAccessoryInformationParameter, sizeof(theiAP2OOBBTPairingAccessoryInformationParameter));
    }

    return rc;
}

LOCAL S32 iap2TestOOBBTPairingCompletionInformation(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iap2Device = iap2Device;

    U8 HoldStatus = 1;
    iAP2OOBBTPairingCompletionInformationParameter theiAP2OOBBTPairingCompletionInformationParameter;
    memset(&theiAP2OOBBTPairingCompletionInformationParameter, 0, sizeof(iAP2OOBBTPairingCompletionInformationParameter));

    rc = iap2AllocateandUpdateData(&theiAP2OOBBTPairingCompletionInformationParameter.iAP2ResultCode ,
                                   &HoldStatus,
                                   &theiAP2OOBBTPairingCompletionInformationParameter.iAP2ResultCode_count,
                                   1, iAP2_uint8);
    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_OOB_BT_PAIRING_COMPLETION_INFO,
                               &theiAP2OOBBTPairingCompletionInformationParameter, sizeof(theiAP2OOBBTPairingCompletionInformationParameter));
    }

    return rc;
}

LOCAL S32 iap2TestBluetoothPairingAccessoryInformation(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iap2Device = iap2Device;

    U16 BT_TransCompIdentifier = IAP2_BT_TRANS_COMP_ID;
    U32 BT_DeviceClass = 0;

    iAP2BluetoothPairingAccessoryInformationParameter theiAP2BluetoothPairingAccessoryInformationParameter;
    memset(&theiAP2BluetoothPairingAccessoryInformationParameter, 0, sizeof(iAP2BluetoothPairingAccessoryInformationParameter));

    rc = iap2AllocateandUpdateData(&theiAP2BluetoothPairingAccessoryInformationParameter.iAP2BluetoothTransportComponentIdentifier ,
                                   &BT_TransCompIdentifier,
                                   &theiAP2BluetoothPairingAccessoryInformationParameter.iAP2BluetoothTransportComponentIdentifier_count,
                                   1, iAP2_uint16);
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2BluetoothPairingAccessoryInformationParameter.iAP2PairingDataP192,
                                       &BT_DeviceClass,
                                       &theiAP2BluetoothPairingAccessoryInformationParameter.iAP2PairingDataP192_count,
                                       1, iAP2_blob);//TODO : fill data accordingly
    }
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2BluetoothPairingAccessoryInformationParameter.iAP2PairingDataP256,
                                       &BT_DeviceClass,
                                       &theiAP2BluetoothPairingAccessoryInformationParameter.iAP2PairingDataP256_count,
                                       1, iAP2_blob);//TODO : fill data accordingly
    }
    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                               MQ_CMD_BT_PAIRING_ACC_INFO,
                               &theiAP2BluetoothPairingAccessoryInformationParameter, sizeof(theiAP2BluetoothPairingAccessoryInformationParameter));
    }
    iAP2FreeiAP2BluetoothPairingAccessoryInformationParameter(&theiAP2BluetoothPairingAccessoryInformationParameter);
    return rc;
}

LOCAL S32 iap2TestBluetoothPairingStatus(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iap2Device = iap2Device;

    U8 SuccessStatus = 1;
    U8 CancelReason = 1;
    U16 BT_TransCompIdentifier = IAP2_BT_TRANS_COMP_ID;

    iAP2BluetoothPairingCompletionInformationParameter theiAP2BluetoothPairingCompletionInformationParameter;
    memset(&theiAP2BluetoothPairingCompletionInformationParameter, 0, sizeof(iAP2BluetoothPairingCompletionInformationParameter));

    rc = iap2AllocateandUpdateData(&theiAP2BluetoothPairingCompletionInformationParameter.iAP2Success ,
                                   &SuccessStatus,
                                   &theiAP2BluetoothPairingCompletionInformationParameter.iAP2Success_count,
                                   1, iAP2_uint8);
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2BluetoothPairingCompletionInformationParameter.iAP2CancelReason ,
                                   &CancelReason,
                                   &theiAP2BluetoothPairingCompletionInformationParameter.iAP2CancelReason_count,
                                   1, iAP2_uint8);
    }
    if (rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2BluetoothPairingCompletionInformationParameter.iAP2BluetoothTransportComponentIdentifier ,
                                   &BT_TransCompIdentifier,
                                   &theiAP2BluetoothPairingCompletionInformationParameter.iAP2BluetoothTransportComponentIdentifier_count,
                                   1, iAP2_uint8);
    }
    if (rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                MQ_CMD_BT_PAIRING_STATUS,
                               &theiAP2BluetoothPairingCompletionInformationParameter, sizeof(theiAP2BluetoothPairingCompletionInformationParameter));
    }
    iAP2FreeiAP2BluetoothPairingCompletionInformationParameter(&theiAP2BluetoothPairingCompletionInformationParameter);
    return rc;
}

LOCAL S32 iap2TestStartAppDiscoveryUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iAP2StartAppDiscoveryUpdatesParameter theiAP2StartAppDiscoveryUpdatesParameter;

    iap2Device = iap2Device;
    U16 CarPlayAppListMax = 0;//No limit
    U16 CarPlayAppIconSize = 90;//max 180

    memset(&theiAP2StartAppDiscoveryUpdatesParameter, 0, sizeof(iAP2StartAppDiscoveryUpdatesParameter));
    /*incrementing count alone would be sufficient for type none*/
    theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppCategories_count ++;
    theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppCategories = (iAP2CarPlayAppDiscoveryCategories*)calloc(1, sizeof(iAP2CarPlayAppDiscoveryCategories));
    if(theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppCategories != NULL)
    {
        theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppCategories->iAP2AllCarPlayApps_count++;
    }

    rc = iap2AllocateandUpdateData(&theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppListMax,
                                    &CarPlayAppListMax,
                                    &theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppListMax_count,
                                    1, iAP2_uint16);
    if(rc == IAP2_OK)
    {
        rc = iap2AllocateandUpdateData(&theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppIconSize,
                                        &CarPlayAppIconSize,
                                        &theiAP2StartAppDiscoveryUpdatesParameter.iAP2CarPlayAppIconSize_count,
                                        1, iAP2_uint16);
    }
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                    MQ_CMD_START_APP_DISCOVERY,
                    &theiAP2StartAppDiscoveryUpdatesParameter, sizeof(theiAP2StartAppDiscoveryUpdatesParameter));
    }

    iAP2FreeiAP2StartAppDiscoveryUpdatesParameter(&theiAP2StartAppDiscoveryUpdatesParameter);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

LOCAL S32 iap2TestStopAppDiscoveryUpdates(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    iAP2StopAppDiscoveryUpdatesParameter theiAP2StopAppDiscoveryUpdatesParameter;
    iap2Device = iap2Device;

    memset(&theiAP2StopAppDiscoveryUpdatesParameter, 0, sizeof(theiAP2StopAppDiscoveryUpdatesParameter));

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                MQ_CMD_STOP_APP_DISCOVERY,
                                &theiAP2StopAppDiscoveryUpdatesParameter, sizeof(theiAP2StopAppDiscoveryUpdatesParameter));

    /* No need to free as there are no associated parameters */

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

LOCAL S32 iap2TestRequestAppDiscoveryAppIcons(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U8 CarPlayAppBundleID[] = {"com.apple.Maps"};
    U8* p_CarPlayAppBundleID = CarPlayAppBundleID;
    iAP2RequestAppDiscoveryAppIconsParameter theiAP2RequestAppDiscoveryAppIconsParameter;
    iap2Device = iap2Device;

    memset(&theiAP2RequestAppDiscoveryAppIconsParameter, 0, sizeof(theiAP2RequestAppDiscoveryAppIconsParameter));

    rc = iap2AllocateandUpdateData(&theiAP2RequestAppDiscoveryAppIconsParameter.iAP2CarPlayAppBundleID,
                                            &p_CarPlayAppBundleID,
                                            &theiAP2RequestAppDiscoveryAppIconsParameter.iAP2CarPlayAppBundleID_count,
                                            1, iAP2_uint16);
    if(rc == IAP2_OK)
    {
        rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                        MQ_CMD_REQUEST_APP_DISCOVERY_UPDATES,
                                        &theiAP2RequestAppDiscoveryAppIconsParameter, sizeof(theiAP2RequestAppDiscoveryAppIconsParameter));
    }

    iAP2FreeiAP2RequestAppDiscoveryAppIconsParameter(&theiAP2RequestAppDiscoveryAppIconsParameter);

    if(rc != IAP2_OK)
    {
        iap2SetTestStateError(TRUE);
    }
    return rc;
}

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
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties = calloc(1, sizeof(iAP2MediaPlaylistProperties));
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties_count++;
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPersistentIdentifier_count++;
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
    eapMsg.EAPmsg = calloc(1,(eapMsg.EAPmsglen)+IAP2_EA_SESSION_IDENTFIER_LENGTH);

    U16 iAP2MatchingEAPSessionIdentifier = g_iap2TestDevice.EAPSessionIdentifier;
    iAP2MatchingEAPSessionIdentifier = IAP2_ADHERE_TO_APPLE_ENDIANESS_16(iAP2MatchingEAPSessionIdentifier);    /*lint !e160 !e644 */

    memcpy(eapMsg.EAPmsg, &iAP2MatchingEAPSessionIdentifier, sizeof(U16));
    memcpy(&eapMsg.EAPmsg[IAP2_EA_SESSION_IDENTFIER_LENGTH], msg, eapMsg.EAPmsglen);

    printf(" Following EAPSession Datagram sent to device: \n");
    for(int i = 0; i < (eapMsg.EAPmsglen+IAP2_EA_SESSION_IDENTFIER_LENGTH); i++)
    {
        if( (i % 10) == 0)
            printf("\n");
        printf(" 0x%.2X, ", eapMsg.EAPmsg[i]);
    }
    printf("\n");

    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_SEND_EAP_SESSION_MSG, &(eapMsg), sizeof(eapMsg));

    if(rc != IAP2_OK){
        iap2SetTestStateError(TRUE);
    }

    if(eapMsg.EAPmsg != NULL)
    {
        free(eapMsg.EAPmsg);
        eapMsg.EAPmsg = NULL;
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
            printf(" iap2TestSendEAPSessionMessage(len: %d) rc = %d \n",
                    (int)strlen((const char*)testbuf), rc);

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
                printf(" %u ms  Send message (len: %u) to iOS App  rc = %d \n",
                       (unsigned int)iap2CurrTimeMs(), (unsigned int)strlen((const char*)&testbuf[0]), rc);

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
            case MQ_CMD_START_COMMUNICATIONS_UPDATE:
            {
                /* start CommunicationsUpdates */
                rc = iAP2StartCommunicationsUpdates(iap2Device, (iAP2StartCommunicationsUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StartCommunicationsUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_STOP_COMMUNICATIONS_UPDATE:
            {
                /* stop CommunicationsUpdates */
                rc = iAP2StopCommunicationsUpdates(iap2Device, (iAP2StopCommunicationsUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StopCommunicationsUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_START_CALL_STATE_UPDATES:
            {
                /* start CallStateUpdates */
                rc = iAP2StartCallStateUpdates(iap2Device, (iAP2StartCallStateUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StartCallStateUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_STOP_CALL_STATE_UPDATES:
            {
                /* stop CallStateUpdates */
                rc = iAP2StopCallStateUpdates(iap2Device, (iAP2StopCallStateUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StopCallStateUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_START_LIST_UPDATES:
            {
                /* start ListUpdates */
                rc = iAP2StartListUpdates(iap2Device, (iAP2StartListUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StartListUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_STOP_LIST_UPDATES:
            {
                /* stop ListUpdates */
                rc = iAP2StopListUpdates(iap2Device, (iAP2StopListUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StopListUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_INITIATE_CALL:
            {
                /* Initiate Call */
                rc = iAP2InitiateCall(iap2Device, (iAP2InitiateCallParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2InitiateCall failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_ACCEPT_CALL:
            {
                /* Initiate Call */
                rc = iAP2AcceptCall(iap2Device, (iAP2AcceptCallParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2AcceptCall failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_MERGE_CALLS:
            {
                /* Merge Calls */
                rc = iAP2MergeCalls(iap2Device, (iAP2MergeCallsParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2MergeCalls failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_SWAP_CALLS:
            {
                /* Merge Calls */
                rc = iAP2SwapCalls(iap2Device, (iAP2SwapCallsParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2SwapCalls failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_END_CALL:
            {
                /* End Call */
                rc = iAP2EndCall(iap2Device, (iAP2EndCallParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2EndCall failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_SEND_DTMF:
            {
                /* Hold Status Update */
                rc = iAP2SendDTMF(iap2Device, (iAP2SendDTMFParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2SendDTMF failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_HOLD_STATUS_UPDATE:
            {
                /* Hold Status Update */
                rc = iAP2HoldStatusUpdate(iap2Device, (iAP2HoldStatusUpdateParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2HoldStatusUpdate failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_MUTE_STATUS_UPDATE:
            {
                /* Mute Status Update */
                rc = iAP2MuteStatusUpdate(iap2Device, (iAP2MuteStatusUpdateParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2MuteStatusUpdate failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_WIFI_CONFIGURATION_INFORMATION:
            {
                /* Accessory's WiFi Configuration Information */
                rc = iAP2AccessoryWiFiConfigurationInformation(iap2Device, (iAP2AccessoryWiFiConfigurationInformationParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2AccessoryWiFiConfigurationInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_OOB_BT_PAIRING_ACC_INFO:
            {
                /* Accessory's OOB BT Pairing Information */
                rc = iAP2OOBBTPairingAccessoryInformation(iap2Device, (iAP2OOBBTPairingAccessoryInformationParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2OOBBTPairingAccessoryInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_OOB_BT_PAIRING_COMPLETION_INFO:
            {
                /* Accessory's OOB BT Pairing CompletionInformation */
                rc = iAP2OOBBTPairingCompletionInformation(iap2Device, (iAP2OOBBTPairingCompletionInformationParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2OOBBTPairingCompletionInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_BT_PAIRING_ACC_INFO:
            {
                /* Accessory's BluetoothPairingAccessoryInformation*/
                rc = iAP2BluetoothPairingAccessoryInformation(iap2Device, (iAP2BluetoothPairingAccessoryInformationParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2BluetoothPairingAccessoryInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_BT_PAIRING_STATUS:
            {
                /* Accessory's BluetoothPairingStatus*/
                rc = iAP2BluetoothPairingStatus(iap2Device, (iAP2BluetoothPairingCompletionInformationParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2BluetoothPairingStatus failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_START_APP_DISCOVERY:
            {
                /* Accessory's StartAppDiscoveryUpdates*/
                rc = iAP2StartAppDiscoveryUpdates(iap2Device, (iAP2StartAppDiscoveryUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StartAppDiscoveryUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_STOP_APP_DISCOVERY:
            {
                /* Accessory's StopAppDiscoveryUpdates */
                rc = iAP2StopAppDiscoveryUpdates(iap2Device, (iAP2StopAppDiscoveryUpdatesParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2StopAppDiscoveryUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                }
                break;
            }
            case MQ_CMD_REQUEST_APP_DISCOVERY_UPDATES:
            {
                /* Accessory's RequestAppDiscoveryAppIcons*/
                rc = iAP2RequestAppDiscoveryAppIcons(iap2Device, (iAP2RequestAppDiscoveryAppIconsParameter*)mq_st->param);
                if(rc != IAP2_OK){
                    printf(" %u ms  iAP2RequestAppDiscoveryAppIcons failed!  rc = %d \n", iap2CurrTimeMs(), rc);
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

LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2device)
{
    S32 rc = IAP2_OK;
#if IAP2_GST_AUDIO_STREAM
    U32 retry_count = 0;
    TEST_THREAD_ID GstTskID = 0;
    U16 retry = 0;
    char threadName[8];
    void* status;
#endif

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
            /* start CommunicationsUpdates */
            printf("\n %u ms  iap2TestStartCommunicationsUpdates \n", iap2CurrTimeMs());
            rc = iap2TestStartCommunicationsUpdates(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* start CallStateUpdates */
            printf("\n %u ms  iap2TestStartCallStateUpdates \n", iap2CurrTimeMs());
            rc = iap2TestStartCallStateUpdates(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* start ListUpdates */
            printf("\n %u ms  iap2TestStartListUpdates \n", iap2CurrTimeMs());
            rc = iap2TestStartListUpdates(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Initiate Call */
            printf("\n %u ms  iap2TestInitiateCall \n", iap2CurrTimeMs());
            rc = iap2TestInitiateCall(mq_fd, iap2device);
        }


        if(iap2GetTestStateError() != TRUE)
        {
            iap2SleepMs(500); /* Wait for 500ms before disconnecting the call */
            /* End Call */
            printf("\n %u ms  iap2TestEndCall \n", iap2CurrTimeMs());
            rc = iap2TestEndCall(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Accept Call */
            printf("\n %u ms  iap2TestAcceptCall \n", iap2CurrTimeMs());
            rc = iap2TestAcceptCall(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Merge Calls */
            printf("\n %u ms  iap2TestMergeCalls \n", iap2CurrTimeMs());
            rc = iap2TestMergeCalls(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Swap Calls */
            printf("\n %u ms  iap2TestSwapCalls \n", iap2CurrTimeMs());
            rc = iap2TestSwapCalls(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Send DTMF Tone */
            printf("\n %u ms  iap2TestSendDTMF \n", iap2CurrTimeMs());
            rc = iap2TestSendDTMF(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Mute Status Update */
            printf("\n %u ms  iap2TestMuteStatusUpdate \n", iap2CurrTimeMs());
            rc = iap2TestMuteStatusUpdate(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Hold Status Update */
            printf("\n %u ms  iap2TestHoldStatusUpdate \n", iap2CurrTimeMs());
            rc = iap2TestHoldStatusUpdate(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE)
        {
            /* Accessory's WiFi Configuration Information */
            printf("\n %u ms  iap2TestAccessoryWiFiConfigurationInformation \n", iap2CurrTimeMs());
            rc = iap2TestAccessoryWiFiConfigurationInformation(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE && g_iap2UserConfig.iap2iOSintheCar == TRUE)
        {
            /* Accessory's OOB BT Pairing Information */
            printf("\n %u ms  iap2TestOOBBTPairingAccessoryInformation \n", iap2CurrTimeMs());
            rc = iap2TestOOBBTPairingAccessoryInformation(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE && g_iap2UserConfig.iap2iOSintheCar == TRUE)
        {
            /* Accessory's OOB BT Pairing Completion Information */
            printf("\n %u ms  iap2TestOOBBTPairingCompletionInformation \n", iap2CurrTimeMs());
            rc = iap2TestOOBBTPairingCompletionInformation(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE && g_iap2UserConfig.iap2iOSintheCar == TRUE)
        {
            /* Accessory's BT Pairing Information */
            printf("\n %u ms  iap2TestBluetoothPairingAccessoryInformation \n", iap2CurrTimeMs());
            rc = iap2TestBluetoothPairingAccessoryInformation(mq_fd, iap2device);
        }

        if(iap2GetTestStateError() != TRUE && g_iap2UserConfig.iap2iOSintheCar == TRUE)
        {
            /* Accessory's BT Pairing Completion Information */
            printf("\n %u ms  iap2TestBluetoothPairingCompletionInformation \n", iap2CurrTimeMs());
            rc = iap2TestBluetoothPairingStatus(mq_fd, iap2device);
        }

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

        if((iap2GetTestStateError() != TRUE) && (g_iap2TestDevice.testMediaLibInfoID != NULL))
        {
            /* start MediaLibrarayUpdate */
            printf("\n %u ms  iap2TestStartMediaLibUpdate() \n", iap2CurrTimeMs());
            rc = iap2TestStartMediaLibUpdate(mq_fd, iap2device);
        }

#if IAP2_GST_AUDIO_STREAM
        if(iap2GetTestStateError() != TRUE)
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
                iap2SleepMs(100);
                iap2SetGstState(IAP2_GSTREAMER_STATE_INITIALIZE);
                retry = 0;
                /* wait until GStreamer is initialized */
                while( (iap2GetGstState() < IAP2_GSTREAMER_INITIALIZED) &&
                       (retry < 60) &&
                       (iap2GetGlobalQuit() == FALSE) )
                {
                    iap2SleepMs(1000);
                    retry++;
                }
                if(retry >= 60){
                    printf(" %u ms Timeout. GStreamer is not initialized within 60 seconds\n", iap2CurrTimeMs());
                    rc = IAP2_CTL_ERROR;
                }
            }
        }
#endif

        if((iap2GetTestStateError() != TRUE) && (g_iap2TestDevice.testMediaLibInfoID != NULL))
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
        if( (iap2GetTestStateError() != TRUE) &&
            (g_iap2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE) )
        {
            /* start USBDeviceModeAudio */
            printf("\n %u ms  iap2TestStartUSBDeviceModeAudio() \n", iap2CurrTimeMs());
            rc = iap2TestStartUSBDeviceModeAudio(mq_fd, iap2device);
        }
        if(iap2GetTestStateError() != TRUE)
        {
            iap2SetGstState(IAP2_GSTREAMER_STATE_PLAYING);
            printf("Sending PLAY Notification to gstreamer\n");
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
        if( (rc == IAP2_OK) &&
            (iap2GetTestStateError() != TRUE) &&
            (iap2GetGstState() == IAP2_GSTREAMER_PLAYING) )
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
        if((iap2GetTestStateError() != TRUE) && (g_iap2TestDevice.testMediaLibInfoID != NULL))
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

                /* stop CommunicationsUpdates */
                printf("\n %u ms  iap2TestStopCommunicationsUpdates() \n", iap2CurrTimeMs());
                rc = iap2TestStopCommunicationsUpdates(mq_fd, iap2device);

                /* stop CallStateUpdates */
                printf("\n %u ms  iap2TestStopCallStateUpdates() \n", iap2CurrTimeMs());
                rc = iap2TestStopCallStateUpdates(mq_fd, iap2device);

                /* stop ListUpdates */
                printf("\n %u ms  iap2TestStopListUpdates() \n", iap2CurrTimeMs());
                rc = iap2TestStopListUpdates(mq_fd, iap2device);

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
#if IAP2_GST_AUDIO_STREAM
        if(GstTskID > 0)
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
    return rc;
}

LOCAL inline void iap2FreePtr(void** iap2PtrToFree)
{
    if(*iap2PtrToFree != NULL)
    {
        free(*iap2PtrToFree);
        *iap2PtrToFree = NULL;
    }
}

#ifdef IAP2_USE_CONFIGFS
LOCAL void iap2DeInitGadgetConfiguration(iAP2_usbg_config_t* usb_gadget_configuration)
{
    U32 i;

    iap2FreePtr((void**)&usb_gadget_configuration->iAP2GadgetName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFS_InstanceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryModelIdentifier);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryManufacturer);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessorySerialNumber);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryVendorId);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryProductId);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2AccessoryBcdDevice);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2ConfigFS_MountLocation);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2UdcDeviceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2UAC2_InstanceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2_UAC2_Attrs);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2NCM_InstanceName);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFSConfig.initEndPoint);
    iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFSConfig.iOSAppIdentifier);
    if(usb_gadget_configuration->iAP2FFSConfig.iOSAppNames != NULL)
    {
        for(i = 0; i < usb_gadget_configuration->iAP2FFSConfig.iOSAppCnt; i++)
        {
            iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFSConfig.iOSAppNames[i]);
        }
        iap2FreePtr((void**)&usb_gadget_configuration->iAP2FFSConfig.iOSAppNames);
    }
}

LOCAL S32 iap2InitGadgetConfiguration(iAP2_usbg_config_t* usb_gadget_configuration, iAP2AccessoryInfo_t* p_iAP2AccessoryInfo, U8 DeviceInstance, BOOL nativetransport)
{
    S32 rc = IAP2_OK;
    U32 i;
    U8 iAP2GadgetName[STRING_MAX - 1]         = {"iAP_Interface_"};
    U8 iAP2FFS_InstanceName[STRING_MAX - 5]   = {"ffs_"};

    (void)snprintf((char*)&iAP2GadgetName[strlen((const char*)iAP2GadgetName)],
                   ( sizeof(iAP2GadgetName) - strlen((const char*)iAP2GadgetName) ),
                   "%d",
                   DeviceInstance);
    (void)snprintf((char*)&iAP2FFS_InstanceName[strlen((const char*)iAP2FFS_InstanceName)],
                   ( sizeof(iAP2FFS_InstanceName) - strlen((const char*)iAP2FFS_InstanceName) ),
                   "%d",
                   DeviceInstance);
    usb_gadget_configuration->iAP2GadgetName                = (U8*)strndup((const char*)iAP2GadgetName, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2FFS_InstanceName          = (U8*)strndup((const char*)iAP2FFS_InstanceName, STRING_MAX - 4);
    usb_gadget_configuration->iAP2AccessoryName             = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessoryName, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryModelIdentifier  = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessoryModelIdentifier, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryManufacturer     = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessoryManufacturer, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessorySerialNumber     = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessorySerialNumber, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryVendorId         = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessoryVendorId, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryProductId        = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessoryProductId, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2AccessoryBcdDevice        = (U8*)strndup((const char*)p_iAP2AccessoryInfo->iAP2AccessoryBcdDevice, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2ConfigFS_MountLocation    = (U8*)strndup((const char*)IAP2_CONFIGFS_MOUNT_LOCATION, (STRING_MAX - 1) );
    usb_gadget_configuration->iAP2UdcDeviceName             = (U8*)strndup((const char*)g_iap2UserConfig.UdcDeviceName, (STRING_MAX - 1) );

    if( (usb_gadget_configuration->iAP2GadgetName == NULL)                  ||
        (usb_gadget_configuration->iAP2FFS_InstanceName == NULL)            ||
        (usb_gadget_configuration->iAP2AccessoryName == NULL)               ||
        (usb_gadget_configuration->iAP2AccessoryModelIdentifier == NULL)    ||
        (usb_gadget_configuration->iAP2AccessoryManufacturer == NULL)       ||
        (usb_gadget_configuration->iAP2AccessorySerialNumber == NULL)       ||
        (usb_gadget_configuration->iAP2AccessoryVendorId == NULL)           ||
        (usb_gadget_configuration->iAP2AccessoryProductId == NULL)          ||
        (usb_gadget_configuration->iAP2AccessoryBcdDevice == NULL)          ||
        (usb_gadget_configuration->iAP2ConfigFS_MountLocation == NULL)      ||
        (usb_gadget_configuration->iAP2UdcDeviceName == NULL) )
    {
        rc = IAP2_ERR_NO_MEM;
        printf("\n %u ms  iap2InitGadgetConfiguration() - Not Enough Memory \n", iap2CurrTimeMs());
    }
    if(rc == IAP2_OK)
    {


        if(usb_gadget_configuration->CarPlayEnabled == FALSE)
        {
            U8 iAP2UAC2_InstanceName[STRING_MAX - 6]  = {"uac2_"};

            (void)snprintf((char*)&iAP2UAC2_InstanceName[strlen((const char*)iAP2UAC2_InstanceName)],
                           ( sizeof(iAP2UAC2_InstanceName) - strlen((const char*)iAP2UAC2_InstanceName) ),
                           "%d",
                           DeviceInstance);
            usb_gadget_configuration->iAP2UAC2_InstanceName = (U8*)strndup((const char*)iAP2UAC2_InstanceName, STRING_MAX - 5);
            usb_gadget_configuration->iAP2_UAC2_Attrs = calloc(1, sizeof(usbg_f_uac2_attrs) );
            if( (usb_gadget_configuration->iAP2_UAC2_Attrs == NULL) ||
                (usb_gadget_configuration->iAP2UAC2_InstanceName == NULL) )
            {
                rc = IAP2_ERR_NO_MEM;
                printf("\n %u ms  iap2InitGadgetConfiguration() - Not Enough Memory \n", iap2CurrTimeMs());
            }
            else
            {
                usb_gadget_configuration->iAP2_UAC2_Attrs->c_chmask     = 3;
                usb_gadget_configuration->iAP2_UAC2_Attrs->c_srate_def  = 44100;
                usb_gadget_configuration->iAP2_UAC2_Attrs->c_ssize      = 2;
                usb_gadget_configuration->iAP2_UAC2_Attrs->delay_tout   = 80;
                usb_gadget_configuration->iAP2_UAC2_Attrs->p_chmask     = 0;
                usb_gadget_configuration->iAP2_UAC2_Attrs->p_srate_def  = 44100;
                usb_gadget_configuration->iAP2_UAC2_Attrs->p_ssize      = 2;
                usb_gadget_configuration->iAP2_UAC2_Attrs->c_srate      = c_srate;
                usb_gadget_configuration->iAP2_UAC2_Attrs->p_srate      = p_srate;
            }
        }

        else
        {
            U8 iAP2NCM_InstanceName[STRING_MAX - 5]   = {"usb_"};

            (void)snprintf((char*)&iAP2NCM_InstanceName[strlen((const char*)iAP2NCM_InstanceName)],
                           ( sizeof(iAP2NCM_InstanceName) - strlen((const char*)iAP2NCM_InstanceName) ),
                           "%d",
                           DeviceInstance);
            usb_gadget_configuration->iAP2NCM_InstanceName = (U8*)strndup((const char*)iAP2NCM_InstanceName, STRING_MAX - 4);
            if(usb_gadget_configuration->iAP2NCM_InstanceName == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
                printf("\n %u ms  iap2InitGadgetConfiguration() - Not Enough Memory \n", iap2CurrTimeMs());
            }
        }
    }
    if( (rc == IAP2_OK) && (nativetransport == TRUE) )
    {
        usb_gadget_configuration->iAP2FFSConfig.iOSAppNames = calloc(p_iAP2AccessoryInfo->iAP2SupportediOSAppCount, sizeof(U8*));
        usb_gadget_configuration->iAP2FFSConfig.iOSAppIdentifier = calloc(p_iAP2AccessoryInfo->iAP2SupportediOSAppCount, sizeof(U8));
        if( (usb_gadget_configuration->iAP2FFSConfig.iOSAppNames == NULL) ||
            (usb_gadget_configuration->iAP2FFSConfig.iOSAppIdentifier == NULL) )
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }
    if( (rc == IAP2_OK) && (nativetransport == TRUE) )
    {
        usb_gadget_configuration->iAP2FFSConfig.nativeTransport = TRUE;
        usb_gadget_configuration->iAP2FFSConfig.iOSAppCnt = p_iAP2AccessoryInfo->iAP2SupportediOSAppCount;
        for(i = 0; ( (i < p_iAP2AccessoryInfo->iAP2SupportediOSAppCount) && (rc == IAP2_OK) ); i++)
        {
            usb_gadget_configuration->iAP2FFSConfig.iOSAppNames[i] = (U8*)strdup((const char*)p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppName);
            if(usb_gadget_configuration->iAP2FFSConfig.iOSAppNames[i] == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                usb_gadget_configuration->iAP2FFSConfig.iOSAppIdentifier[i] = p_iAP2AccessoryInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
            }
        }
    }
    printf("\n %u ms  iap2InitGadgetConfiguration() - returns rc = %d \n", iap2CurrTimeMs(), rc);

    return rc;
}
#endif

#define IAP2_SERVICE
#ifdef IAP2_SERVICE
/*iAP2 service adaptation*/
int32_t Demo_iAP2ServiceDeviceConnected(iAP2Service_t* service, iAP2ServiceDeviceList_t* msg)
{
    (void)service;
    int32_t rc = IAP2_OK;

    IAP2TESTDLTLOG(DLT_LOG_INFO, "Demo_iAP2ServiceDeviceConnected");
    uint32_t i = 0;
    for(; i < msg->count; ++i)
    {
        IAP2TESTDLTLOG(DLT_LOG_INFO, "AppleDevice:%s DeviceId(%u) Serial:%s Transport:%d, EAP:%d EAN:%d \
                       CarPlay:%d msg->list[i].deviceState:%d",
                       msg->list[i].name, msg->list[0].id, msg->list[i].serial, msg->list[i].transport,
                       msg->list[i].eapSupported, msg->list[i].eaNativeSupported, msg->list[i].carplaySupported,
                       msg->list[i].deviceState);

    }

    g_deviceId = msg->list[0].id;
    printf("New Device(%d) connected @ iAP2Service!\n", g_deviceId);

    int64_t data = msg->list[0].id;
    rc = write(g_eventFd, &data, sizeof(int64_t));
    if(rc < 0)
    {
        IAP2TESTDLTLOG(DLT_LOG_WARN, "Writing to g_eventFd failed in Demo_iAP2ServiceDeviceConnected");
    }
    else
    {
        rc = IAP2_OK;
    }

    return rc;
}

int32_t Demo_iAP2ServiceDeviceDisconnected(iAP2Service_t* service, uint32_t deviceId)
{
    (void)deviceId;
    (void)service;

    int32_t rc = IAP2_OK;
    IAP2TESTDLTLOG(DLT_LOG_INFO, "Demo_iAP2ServiceDeviceDisconnected");

    return rc;
}

int32_t Demo_iAP2ServiceConnectDeviceResp(iAP2Service_t* service, uint32_t deviceId, iAP2ServiceConnectDeviceResp_t* msg)
{
    int32_t rc = IAP2_OK;
    (void)service;
    (void)msg;
    (void)deviceId;

    IAP2TESTDLTLOG(DLT_LOG_INFO, "Demo_iAP2ServiceConnectDeviceRespCB");
    return rc;
}

int32_t Demo_iAP2ServiceDeviceState(iAP2Service_t* service, uint32_t deviceId, iAP2ServiceDeviceState_t* msg)
{
    int32_t rc = IAP2_OK;
    (void)service;
    (void)msg;

    g_iap2TestDevice.testDeviceState = msg->state;
    if(msg->state == iAP2ComError)
    {
        int64_t data = -1;
        rc = write(g_eventFd, &data, sizeof(int64_t));
        if(rc < 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "Writing to g_eventFd failed in Demo_iAP2ServiceDeviceState");
        }
        else
        {
            rc = IAP2_OK;
        }
    }

    IAP2TESTDLTLOG(DLT_LOG_INFO, "Demo_iAP2ServiceDeviceState");
    IAP2TESTDLTLOG(DLT_LOG_INFO, "iap2DeviceState_CB: %d for deviceId:%u serial:%s", msg->state, deviceId, msg->serial);

    return rc;
}

int makeSocketNonBlocking(int sockFd)
{
    int flags, s;

    flags = fcntl (sockFd, F_GETFL, 0);
    if (flags == -1)
    {
        perror ("fcntl");
        return -1;
    }

    flags |= O_NONBLOCK;
    s = fcntl (sockFd, F_SETFL, flags);
    if (s == -1)
    {
        perror ("fcntl");
        return -1;
    }

    return 0;
}

void iap2ServiceEventHandlerThread(void* exinf)
{
#define MAX_EVENTS 10
#define MAX_TIMEOUT 1000

    S32 rc = IAP2_CTL_ERROR;
    iAP2Service_t* service = (iAP2Service_t*)exinf;
    int fd = epoll_create(10);

    struct epoll_event *events;
    events = (struct epoll_event*)calloc(MAX_EVENTS, sizeof(struct epoll_event));

    struct epoll_event ev; /* Content will be copied by the kernel */
    ev.events =  EPOLLIN;
    ev.data.ptr = NULL;
    makeSocketNonBlocking(service->iAP2ServerFd);

    rc = epoll_ctl(fd, EPOLL_CTL_ADD, service->iAP2ServerFd, &ev);

    printf("smoketest serverFd: %d, epollCtr:%d\n", service->iAP2ServerFd, rc);
    while(!g_shutdown1)
    {
        rc = epoll_wait(fd, events, MAX_EVENTS, MAX_TIMEOUT);
        printf("epoll_wait returned: %d\n", rc);

        if(rc > 0)
        {
            rc = iAP2ServiceHandleEvents(service);
        }
        if(rc < 0)
        {
            g_shutdown1 = 1;
        }
    }
    free(events);
}

/*iAP2 service adaptation - end*/
#endif

/* application thread */
LOCAL void iap2AppThread(void* exinf)
{
    S32 rc = IAP2_OK;
    S32 rc_tmp;

    iap2AppThreadInitData_t* iAP2AppThreadInitData = (iap2AppThreadInitData_t*)exinf;
    iAP2InitParam_t* iAP2InitParameter = &(iAP2AppThreadInitData->iAP2InitParameter);
    g_iAP2InitParam = iAP2InitParameter;

    iAP2Device_t* iap2device = NULL;
    char threadName[8];
    U32 retry_count = 0;
    void* status;
    U16 i = 0;
    mqd_t mq_fd = -1;
    int shmid_rc;
    int key_rc = 6;
    char *shm = NULL;

    if ((shmid_rc = shmget(key_rc, 2, 0666)) >= 0)
    {
        printf("Opened the shared memory\n");
        if ((shm = shmat(shmid_rc, NULL, 0)) != (char *) -1)
        {
            printf("Shared memory attached successfully\n");
        }
        else
        {
            printf("Error in attaching the shared memory %s\n",strerror(errno));
            shm = NULL;
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        printf("Error in accessing the shared memory %s\n",strerror(errno));
        rc = IAP2_CTL_ERROR;
    }

    if (rc == IAP2_OK)
    {
        /* create mutex to protect MediaLib access */
        pthread_mutex_init(&g_iap2TestDevice.testMediaLibMutex, NULL);

        g_iap2TestDevice.testMediaLibUpdateProgress = 0;

        if( (g_iap2UserConfig.app_thread_user != 0) ||
            (g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)||(g_iap2UserConfig.iAP2TransportType == iAP2MULTIHOSTMODE) )
        {
            printf ("Setting app thread user %d, group %d and %d supplementary groups\n", g_iap2UserConfig.app_thread_user, g_iap2UserConfig.app_thread_prim_group, g_iap2UserConfig.app_thread_groups_cnt);

            setgroups( g_iap2UserConfig.app_thread_groups_cnt, g_iap2UserConfig.app_thread_groups );
            if(setgid( g_iap2UserConfig.app_thread_prim_group ) < 0 )
            {
                printf("setgid() failed %s\n", strerror(errno));
            }
            if(setuid( g_iap2UserConfig.app_thread_user ) < 0 )
            {
                printf("setuid() failed %s\n", strerror(errno));
            }
            /* don't ask why */
            prctl (PR_SET_DUMPABLE, 1);
        }

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
    }
    if(rc  == IAP2_OK)
    {
        /*iAP2 service adaptation*/
        if(g_eventFd < 0)
            g_eventFd = eventfd(0, 0);

        iAP2ServiceCallbacks_t serviceCallbacks;
        memset(&serviceCallbacks, 0, sizeof(iAP2ServiceCallbacks_t));

        serviceCallbacks.p_iAP2ServiceDeviceConnected_cb    = &Demo_iAP2ServiceDeviceConnected;
        serviceCallbacks.p_iAP2ServiceDeviceDisconnected_cb = &Demo_iAP2ServiceDeviceDisconnected;
        serviceCallbacks.p_iAP2ServiceConnectDeviceResp_cb  = &Demo_iAP2ServiceConnectDeviceResp;
        serviceCallbacks.p_iAP2ServiceDeviceState_cb        = &Demo_iAP2ServiceDeviceState;

        iAP2ServiceClientInformation_t client;
        memset(&client, 0, sizeof(client));
        client.pid = getpid();
        strncpy(client.name, "iAP2 Smoketest App", strnlen("iAP2 Smoketest App", sizeof(client.name)));
        g_service = iAP2ServiceInitialize(&serviceCallbacks, &client);
        printf(" iAP2ServiceInitialize:  service  = %p \n", g_service);
        /**/

        TEST_THREAD_ID eventThreadID = 0, comThreadID = 0;
        /* -----------  start iAP2Service polling thread  ----------- */
        eventThreadID = iap2CreateThread(iap2ServiceEventHandlerThread, "service", g_service);
        if(iap2VerifyThreadId(eventThreadID) != IAP2_OK)
        {
            rc = IAP2_CTL_ERROR;
            printf("  create thread %s failed %d \n", "poll", rc);
        }

        printf("Waiting for Device Connected notification!\n");

        if(g_service)
        {
            /*Send configuration information to iAP2 Service*/
            rc = iAP2ServiceDeviceDiscovered(g_service, iAP2InitParameter);
            printf(" iAP2ServiceDeviceDiscovered:  rc  = %d \n",rc);
        }

        int64_t data = 0;
        rc = read(g_eventFd, &data, sizeof(data));
        if(rc < 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_WARN, "reading from fd failed in iap2AppThread, rc=%d ", rc);
        }
        else
        {
            rc = IAP2_OK;
        }
        if(data <= 0)
        {
            printf("  eventFD read failed data:%d \n", (int32_t)data);
            return; /******************early return**********************/
        }

        printf("Device (%u) Connected successfully!\n", g_deviceId);

        /**
         * create internal device structure and initialize
         * with iAP2InitParameter provided by application.
         */
        /*iAP2 service adaptation*/
        iap2device = iAP2ServiceInitDeviceStructure(g_service, g_deviceId, iAP2InitParameter);
        g_iAP2Device = iap2device;
        /**/
        printf(" %u ms  iAP2ServiceInitDeviceStructure = 0x%p \n", iap2CurrTimeMs(), iap2device);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "iAP2ServiceInitDeviceStructure rc = %p ", iap2device);

        if(NULL != iap2device)
        {
            if(rc  == IAP2_OK)
            {
                rc = iAP2ServiceInitDeviceConnection(g_service, iap2device, iAP2InitParameter);
                printf(" %u ms  iAP2ServiceInitDeviceConnection:   rc  = %d \n", iap2CurrTimeMs(), rc);
                IAP2TESTDLTLOG(DLT_LOG_DEBUG,"iAP2ServiceInitDeviceConnection rc = %d ",rc);
            }

            if(rc == IAP2_OK)
            {
                /* set thread name */
                memset(&threadName[0], 0, (sizeof(threadName)));
                sprintf(&threadName[0], "%s%d", "iCom", 1);

                /* -----------  start polling thread  ----------- */
                comThreadID  = iap2CreateThread(iap2ServiceComThread, &threadName[0], iap2device);
                if(iap2VerifyThreadId(comThreadID) != IAP2_OK)
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
            if(comThreadID > 0)
            {
                (void)iap2TestStopPollThread(mq_fd, iap2device);
            }

            if(eventThreadID > 0)
            {
                g_shutdown1 = 1;
                rc = pthread_join(eventThreadID, &status);
            }

            /* -----------  clean thread and mq  ----------- */
            if(comThreadID > 0)
            {
                rc = pthread_join(comThreadID, &status);
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

         /* iAP2Service Disapper device for disconnection */
            iAP2ServiceDeviceInformation_t deviceInfo;
            strncpy(deviceInfo.serial, (const char*)g_iAP2InitParam->iAP2DeviceId, sizeof(deviceInfo.serial));
            rc = iAP2ServiceDeviceDisappeared(g_service, &deviceInfo);
            printf(" %u ms  iAP2ServiceDeviceDisappeared:   rc  = %d \n", iap2CurrTimeMs(), rc);

            /* de-initialize the device structure */
            rc_tmp = iAP2ServiceDeInitDeviceStructure(g_service, iap2device);
            printf(" %u ms  iAP2ServiceDeInitDeviceStructure:   rc  = %d \n", iap2CurrTimeMs(), rc_tmp);
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2ServiceDeInitDeviceStructure = %d ", rc);
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

        printf("\n");
        if(g_rc == IAP2_OK)
        {
            printf("iap2_smoketest                 PASS       %d\n", g_rc);
            IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 SMOKETEST PASS *****");
            if(shm != NULL)
            {
                *shm = '1';
            }
        }
        else
        {
            printf("iap2_smoketest                 FAIL       %d\n", g_rc);
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 SMOKETEST FAIL *****");
            if(shm != NULL)
            {
                *shm = '0';
            }
        }

        /* destroy mutex */
        pthread_mutex_destroy(&g_iap2TestDevice.testMediaLibMutex);
    }
    printf(" %u ms  exit iap2AppThread \n", iap2CurrTimeMs());

}

LOCAL void iap2RootDaemonProcess(void* exinf)
{
    S32 rc = IAP2_OK;
    pid_t App_pid;
    int App_status;

#ifdef IAP2_USE_CONFIGFS
    iAP2_usbg_config_t usb_gadget_configuration1 = {0};
    iAP2_usbg_config_t usb_gadget_configuration2 = {0};
#endif

    iap2AppThreadInitData_t* iAP2AppThreadInitData = (iap2AppThreadInitData_t*)exinf;
    iAP2InitParam_t* iAP2InitParameter = &(iAP2AppThreadInitData->iAP2InitParameter);
    int socket_fd = iAP2AppThreadInitData->socket_fd;

    /* Switch the role first & identify the UDC name first */
    if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)||(g_iap2UserConfig.iAP2TransportType == iAP2MULTIHOSTMODE))
    {
        rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_HOST, socket_fd,
        iAP2AppThreadInitData->udevPath, iAP2InitParameter);
    }

#ifdef IAP2_USE_CONFIGFS
    if( (rc  == IAP2_OK) &&
        (iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE) )
    {
        /* Initialize - 1st Gadget Configuration */
        rc = iap2InitGadgetConfiguration(&usb_gadget_configuration1, iAP2InitParameter->p_iAP2AccessoryInfo, 1, iAP2InitParameter->p_iAP2AccessoryConfig->iAP2EANativeTransport);
        if(rc == IAP2_OK)
        {
            /* Initialize - 2nd Gadget Configuration */
            rc = iap2InitGadgetConfiguration(&usb_gadget_configuration2, iAP2InitParameter->p_iAP2AccessoryInfo, 2, FALSE);
        }
        if(rc == IAP2_OK)
        {
            rc = iAP2InitializeGadget(&usb_gadget_configuration1);
            printf(" %u ms  iAP2InitializeGadget = %d\n", iap2CurrTimeMs(), rc);
            IAP2TESTDLTLOG(DLT_LOG_INFO, "iAP2InitializeGadget rc = %d ", rc);
        }
    }
    if( (rc  == IAP2_OK) &&
        (iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE) )
    {
        rc = mkdir("/dev/ffs", 0750);
        if ( (rc != 0) && (rc != EEXIST) )
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("mkdir(/dev/ffs)  rc = %d\n", rc);
    }
    if( (rc  == IAP2_OK) &&
        (iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE) )
    {
        char options[30] = {0};

        snprintf(options, sizeof(options), "uid=%d,gid=%d",g_iap2UserConfig.app_thread_user, g_iap2UserConfig.app_thread_prim_group);
        rc = mount("ffs_1", "/dev/ffs", "functionfs", MS_NOEXEC, options);
        if (rc != 0)
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("mount(ffs_1)  rc = %d, errno=%d, %s\n", rc, errno, strerror(errno));
    }

    if( (rc  == IAP2_OK) &&
        (iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE) )
    {
        /* after mount we need to set the mode again */
        rc = chmod("/dev/ffs", 0777);
        if (rc != 0)
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("chmod(/dev/ffs)  rc = %d\n", rc);
    }

    if( (rc  == IAP2_OK) &&
        (iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE) )
    {
        /* after mount we need to set the mode again */
        rc = chmod("/dev/ffs/ep0", 0777);
        if (rc != 0)
        {
            rc = IAP2_CTL_ERROR;
        }
        printf("chmod(/dev/ffs/ep0)  rc = %d\n", rc);
    }

    if( (rc  == IAP2_OK) &&
        (iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE) )
    {
        U8 initEndPoint[] = {"/dev/ffs/ep0"};

        usb_gadget_configuration1.iAP2FFSConfig.initEndPoint = (U8*)strdup((const char*)initEndPoint);
        rc = iAP2InitializeFFSGadget(&usb_gadget_configuration1);
        printf("iAP2ConfigureFFSGadget  rc = %d\n", rc);
    }
#endif

    if(rc == IAP2_OK)
    {
        App_pid = fork();
        if(App_pid == 0)
        {
            /* if fork() is called from an application, dlt is reset
               and user application needs to re-register application and
               contexts in child-process */
            IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
            IAP2REGISTERCTXTWITHDLT();

            iap2AppThread(iAP2AppThreadInitData);

            IAP2DEREGISTERCTXTWITHDLT();
            IAP2DEREGISTERAPPWITHDLT();
        }
        else
        {
            waitpid(App_pid, &App_status, 0);

#ifdef IAP2_USE_CONFIGFS
            if(iAP2InitParameter->p_iAP2AccessoryConfig->useConfigFS == TRUE)
            {
                rc = iAP2DeInitializeGadget(&usb_gadget_configuration1);
                printf(" %u ms  iAP2DeInitializeGadget:   rc  = %d \n", iap2CurrTimeMs(), rc);
                iap2DeInitGadgetConfiguration(&usb_gadget_configuration1);
                printf(" %u ms  iAP2DeInitialized Gadget1\n", iap2CurrTimeMs() );
                iap2DeInitGadgetConfiguration(&usb_gadget_configuration2);
                printf(" %u ms  iAP2DeInitialized Gadget2\n", iap2CurrTimeMs() );
            }
#endif

            if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)||(g_iap2UserConfig.iAP2TransportType == iAP2MULTIHOSTMODE))
            {
                rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_DEVICE, socket_fd,
                iAP2AppThreadInitData->udevPath, iAP2InitParameter);
            }

            printf(" %u ms  exit iap2RootDaemonProcess \n", iap2CurrTimeMs());
        }
    }
}

LOCAL S32 application_process_main (int socket_fd)
{
    S32 rc = IAP2_CTL_ERROR;

    int RDStatus;
    pid_t pid = 0;

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

        pid = fork();
        if(pid == 0)
        {
            /* if fork() is called from an application, dlt is reset
               and user application needs to re-register application and
               contexts in child-process */
            IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
            IAP2REGISTERCTXTWITHDLT();

            if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)||(g_iap2UserConfig.iAP2TransportType == iAP2MULTIHOSTMODE))
            {
                iap2RootDaemonProcess(&iAP2AppThreadInitData);
            }
            else
            {
                iap2AppThread(&iAP2AppThreadInitData);
            }

            IAP2DEREGISTERCTXTWITHDLT();
            IAP2DEREGISTERAPPWITHDLT();
        }
        else
        {
            waitpid(pid, &RDStatus, 0);
        }
    }
    else
    {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "iap2Udev failed with %d \n", rc);
        g_rc = rc;
    }
    if(pid != 0)
    {
        iap2DeinitDev(iAP2InitParameter);


        IAP2DEREGISTERCTXTWITHDLT();
        IAP2DEREGISTERAPPWITHDLT();
    }

    return rc;
}

S32 main(int argc, const char** argv)
{
    S32 rc;
    int socket_fds[2];
    static const int host_device_switch_process = 0;
    static const int application_process = 1;
    pid_t host_device_switch_process_pid;
    int shmid;
    int shmkey = 5;
    int shmid_rc;
    int key_rc = 6;
    char *shm = NULL;

    if ((shmid_rc = shmget(key_rc, 2, IPC_CREAT | 0666)) >= 0)
    {
        printf("Shared memory created successfully\n");
        if ((shm = shmat(shmid_rc, NULL, 0)) != (char *) -1)
        {
            printf("Shared memory attached successfully\n");
            if(shm != NULL)
            {
                *shm = '0';
            }
            else
            {
                return IAP2_CTL_ERROR;
            }
        }
        else
        {
            printf("Error in attaching the shared memory %s\n",strerror(errno));
            shm = NULL;
            return IAP2_CTL_ERROR;
        }
    }
    else
    {
        printf("Error in creating the shared memory %s\n",strerror(errno));
        return IAP2_CTL_ERROR;
    }

    memset(&g_iap2TestDevice, 0, sizeof(g_iap2TestDevice));
    memset(&g_iap2UserConfig, 0, sizeof(g_iap2UserConfig));

    SetGlobPtr(&g_iap2TestDevice);

    /**
     * get user configuration. iap2GetArguments() gets runtime user inputs and
     * stores in global g_iap2UserConfig Structure.
     */
    rc = iap2GetArguments(argc, argv, &g_iap2UserConfig);

    if (rc == IAP2_OK)
    {
        socketpair(PF_LOCAL, SOCK_STREAM, 0, socket_fds);

        shmid = shmget(shmkey, 255, IPC_CREAT);

        //now attach a memory to this share memory
        g_iap2UserConfig.UdcDeviceName = shmat(shmid, NULL, 0);

        host_device_switch_process_pid = fork();
        if (host_device_switch_process_pid == 0)
        {
            close(socket_fds[application_process]);

            host_device_switch_process_main(socket_fds[host_device_switch_process], &g_iap2UserConfig);
            close(socket_fds[host_device_switch_process]);
            rc = 0;
        }
        else
        {
            close(socket_fds[host_device_switch_process]);

            rc = application_process_main(socket_fds[application_process]);
            close(socket_fds[application_process]);
        }

        //after your work is done deattach the pointer
        shmdt(g_iap2UserConfig.UdcDeviceName);
    }

    iap2TestFreePtr( (void**)&g_iap2UserConfig.app_thread_groups);

    if(*shm == '1')
    {
        rc = IAP2_OK;
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }
    if(shmdt(shm) == IAP2_OK)
    {
        printf("Shared memory detached successfully\n");
    }
    else
    {
        printf("Error in detaching the shared memory\n");
    }

    return rc;
}
