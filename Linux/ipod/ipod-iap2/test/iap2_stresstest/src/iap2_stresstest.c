/*
 * iap2_stresstest.c
 */

/* **********************  includes  ********************** */
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <sys/prctl.h>

#include "iap2_stresstest.h"
#include "iap2_test_gstreamer.h"
#include "iap2_usb_role_switch.h"
#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include <iap2_external_accessory_protocol_session.h>
#include <endian.h>
#include "iap2_dlt_log.h"


typedef struct iap2FileXferBuffer
{
    U8* Buffer;
    U8* CurPos;
    U64 CurReceived;
    U8 FileID;
    U64 FileSize;
    struct iap2FileXferBuffer* nextfile;
}iap2FileXferBuf_t;

typedef struct
{
    int socket_fd;
    iAP2InitParam_t iAP2InitParameter;
    U8* udevPath;
} iap2AppThreadInitData_t;


LOCAL S32 application_process_main (int socket_fd);
LOCAL void iap2AppThread(void* exinf);

/* test functions */
LOCAL S32 iap2TestStartMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStopNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2TestStartPlayMediaLibraryItem(S32 mq_fd, iAP2Device_t* iap2Device, U16 numSongsToPlay);
LOCAL S32 iap2TestStartUSBDeviceModeAudio(S32 mq_fd, iAP2Device_t* iap2Device);
LOCAL S32 iap2StartTest(S32 mq_fd, iAP2Device_t* iap2Device);

S32 g_rc = IAP2_OK;
iap2FileXferBuf_t *g_CoverArtBuf;
U32 CurrentSongTotalDuration = 0;
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
            g_iap2TestDevice.testDeviceState = iAP2NotConnected;
            printf("\t %d ms %p  Device state : iAP2NotConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2TransportConnected :
            g_iap2TestDevice.testDeviceState = iAP2TransportConnected;
            printf("\t %d ms %p  Device state : iAP2TransportConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkConnected:
            g_iap2TestDevice.testDeviceState = iAP2LinkConnected;
            printf("\t %d ms %p  Device state : iAP2LinkConnected \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2AuthenticationPassed :
            g_iap2TestDevice.testDeviceState = iAP2AuthenticationPassed;
            printf("\t %d ms %p  Device state : iAP2AuthenticationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2IdentificationPassed :
            g_iap2TestDevice.testDeviceState = iAP2IdentificationPassed;
            printf("\t %d ms %p  Device state : iAP2IdentificationPassed \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2DeviceReady:
            g_iap2TestDevice.testDeviceState = iAP2DeviceReady;
            printf("\t %d ms %p  Device state : iAP2DeviceReady  \n", iap2CurrTimeMs(), iap2Device);
            break;
        case iAP2LinkiAP1DeviceDetected:
            g_iap2TestDevice.testDeviceState = iAP2LinkiAP1DeviceDetected;
            printf("\t %d ms %p  Device state : iAP2LinkiAP1DeviceDetected\n", iap2CurrTimeMs(), iap2Device);
            break;
        default:
            g_iap2TestDevice.testDeviceState = iAP2ComError;
            printf("\t %d ms %p  Device state : unknown %d \n", iap2CurrTimeMs(), iap2Device, dState);
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
    printf("\t %d ms %p  iap2AuthenticationSucceeded_CB called \n", iap2CurrTimeMs(), iap2Device);

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
    printf("\t %d ms %p  iap2AuthenticationFailed_CB called \n", iap2CurrTimeMs(), iap2Device);

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
    printf("\t %d ms %p  iap2IdentificationAccepted_CB called \n", iap2CurrTimeMs(), iap2Device);

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
    printf("\t %d ms %p  iap2IdentificationRejected_CB called \n", iap2CurrTimeMs(), iap2Device);

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
        printf("\t %d ms %p  iap2PowerUpdate_CB CurrentDrawn: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2AccessoryPowerMode_count != 0)
    {
        printf("\t %d ms %p  iap2PowerUpdate_CB AccessoryPowerMode: %d \n", iap2CurrTimeMs(), iap2Device,
            *(powerupdateParameter->iAP2AccessoryPowerMode));
        rc = IAP2_OK;
    }
    if (powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count != 0)
    {
        printf("\t %d ms %p  iap2PowerUpdate_CB BatteryWillCharge: %d \n", iap2CurrTimeMs(), iap2Device,
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
    printf("\t %d ms %p  iap2MediaLibraryInfo_CB called \n", iap2CurrTimeMs(), iap2Device);

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
    printf("\t %d ms %p  iap2MediaLibraryUpdates_CB called \n", iap2CurrTimeMs(), iap2Device);

    if((iap2GetTestState(RUNNING) == FALSE)
       ||(iap2GetTestState(STOPPED) == TRUE))
    {
        /* iAP2 Stresstest already stopped */
        return IAP2_OK;
    }

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
                if(g_iap2TestDevice.testMediaLibLastKnownRev != NULL)
                {
                    if(strncmp((char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryRevision),
                           (char*)g_iap2TestDevice.testMediaLibLastKnownRev, strlen((char*)g_iap2TestDevice.testMediaLibLastKnownRev))!= 0)
                    {
                        memcpy(g_iap2TestDevice.testMediaLibLastKnownRev, *(MediaLibraryUpdateParameter->iAP2MediaLibraryRevision), strlen((const char*)*(MediaLibraryUpdateParameter->iAP2MediaLibraryRevision)));
                    }
                }
                else
                {
                    g_iap2TestDevice.testMediaLibLastKnownRev = calloc(1, strlen((const char*)*(MediaLibraryUpdateParameter->iAP2MediaLibraryRevision)));
                    if(g_iap2TestDevice.testMediaLibLastKnownRev != NULL)
                    {
                        memcpy(g_iap2TestDevice.testMediaLibLastKnownRev, *(MediaLibraryUpdateParameter->iAP2MediaLibraryRevision), strlen((const char*)*(MediaLibraryUpdateParameter->iAP2MediaLibraryRevision)));
                    }
                }
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

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemAlbumTitle_count > 0) && (rc == IAP2_OK) )
                        {
                           g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle_count++;
                           g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle = (U8**)calloc(1,sizeof(U8*));

                           if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle != NULL)
                           {
                               size = strlen((char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemAlbumTitle));
                               *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle) = calloc(size,sizeof(U8));
                               if(*(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle) != NULL)
                               {
                                   strncpy((char*)*(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle),
                                           (char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemAlbumTitle),
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

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds_count > 0) && (rc == IAP2_OK) )
                        {
                           g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds_count++;
                           g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds = calloc(1,sizeof(U32));

                           if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds != NULL)
                           {
                               *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds) = *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds);
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

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemRating_count > 0) && (rc == IAP2_OK) )
                        {
                           g_iap2TestDevice.testMediaItem[i].iAP2MediaItemRating_count++;
                           g_iap2TestDevice.testMediaItem[i].iAP2MediaItemRating = calloc(1,sizeof(U8));

                           if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemRating != NULL)
                           {
                               *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemRating) = *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemRating);
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
                    printf(" %d ms:  Error g_MediaItem.test_MediaItem is NULL \n", iap2CurrTimeMs());
                }
            }
            else if( (g_iap2TestDevice.testMediaLibInfoID != NULL)
                     &&(strncmp((char*)g_iap2TestDevice.testMediaLibInfoID,
                                (char*)(*MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier), size) == 0) )
            {
                printf(" %d ms:  iap2MediaLibraryUpdates_CB received for same MediaLibraryID \n", iap2CurrTimeMs());

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

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemAlbumTitle_count > 0) && (rc == IAP2_OK) )
                        {
                           g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemAlbumTitle_count++;
                           g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemAlbumTitle = (U8**)calloc(1,sizeof(U8*));

                           if(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemAlbumTitle != NULL)
                           {
                               size = strlen((char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemAlbumTitle));
                               *(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemAlbumTitle) = calloc(size,sizeof(U8));
                               if(*(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemAlbumTitle) != NULL)
                               {
                                   strncpy((char*)*(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemAlbumTitle),
                                           (char*)*(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemAlbumTitle),
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

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds_count > 0) && (rc == IAP2_OK) )
                        {
                           g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPlaybackDurationInMilliseconds_count++;
                           g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPlaybackDurationInMilliseconds = calloc(1,sizeof(U32));

                           if(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPlaybackDurationInMilliseconds != NULL)
                           {
                               *(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemPlaybackDurationInMilliseconds) = *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds);
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

                        if( (MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemRating_count > 0) && (rc == IAP2_OK) )
                        {
                           g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemRating_count++;
                           g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemRating = calloc(1,sizeof(U8));

                           if(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemRating != NULL)
                           {
                               *(g_iap2TestDevice.testMediaItem[t_index].iAP2MediaItemRating) = *(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemRating);
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
                    printf(" %d ms:  Error realloc g_MediaItem.test_MediaItem is NULL \n", iap2CurrTimeMs());
                }
            }
            else
            {
                printf(" %d ms:  Error g_MediaItem.test_MediaItem already allocated \n", iap2CurrTimeMs());
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
    printf("\t %d ms %p  iap2NowPlayingUpdate_CB called \n", iap2CurrTimeMs(), iap2Device);

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

LOCAL S32 iap2CheckIfNotPresent(const U32 fileID)
{
    S32 rc = IAP2_OK;
    iap2FileXferBuf_t* t_buf = NULL;

    t_buf = g_CoverArtBuf;
    while(t_buf != NULL)
    {
        if(t_buf->FileID == fileID)
        {
            rc = IAP2_CTL_ERROR;
            break;
        }
        t_buf = t_buf->nextfile;
    }
    return rc;
}

LOCAL S32 iap2AddToList(iAP2FileTransferSession_t* iAP2FileXferSession)
{
    S32 rc = IAP2_OK;
    iap2FileXferBuf_t* t_buf = NULL;

    if(g_CoverArtBuf == NULL)
    {
        g_CoverArtBuf = calloc(1, sizeof(iap2FileXferBuf_t));
        if(g_CoverArtBuf == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            g_CoverArtBuf->Buffer = calloc(iAP2FileXferSession->iAP2FileXferRxLen, sizeof(U8));
            if(g_CoverArtBuf->Buffer != NULL)
            {
                g_CoverArtBuf->CurPos = g_CoverArtBuf->Buffer;
                g_CoverArtBuf->FileID = iAP2FileXferSession->iAP2FileTransferID;
                g_CoverArtBuf->FileSize = iAP2FileXferSession->iAP2FileXferRxLen;
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
        t_buf = g_CoverArtBuf;
        while(t_buf->nextfile != NULL)
        {
            t_buf = t_buf->nextfile;
        }
        t_buf->nextfile = calloc(1, sizeof(iap2FileXferBuf_t));
        if(t_buf->nextfile != NULL)
        {
            t_buf->nextfile->Buffer = calloc(1,iAP2FileXferSession->iAP2FileXferRxLen);


            if(t_buf->nextfile->Buffer != NULL)
            {
                t_buf->nextfile->CurPos = t_buf->nextfile->Buffer;
                t_buf->nextfile->FileID = iAP2FileXferSession->iAP2FileTransferID;
                t_buf->nextfile->FileSize = iAP2FileXferSession->iAP2FileXferRxLen;
            }
            else
            {
                printf("\t   memory allocation failed. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
                rc = IAP2_ERR_NO_MEM;
            }
        }
        else
        {
            printf("\t   memory allocation failed. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
            rc = IAP2_ERR_NO_MEM;
        }
    }

    return rc;
}

S32 iap2CopyToBuffer(iAP2FileTransferSession_t* iAP2FileXferSession)
{
    S32 rc = IAP2_CTL_ERROR;
    iap2FileXferBuf_t* t_buf = NULL;

    if((g_CoverArtBuf != NULL) && (iAP2FileXferSession->iAP2FileXferRxLen > 0))
    {
        t_buf = g_CoverArtBuf;
        while(t_buf != NULL)
        {
            if(t_buf->FileID == iAP2FileXferSession->iAP2FileTransferID)
            {
                rc = IAP2_OK;
                break;
            }
            t_buf = t_buf->nextfile;
        }
        if(rc != IAP2_OK)
        {
            printf("\n setup for FileID:%d is not received", iAP2FileXferSession->iAP2FileTransferID);
        }
        else
        {
            if((t_buf->CurReceived+iAP2FileXferSession->iAP2FileXferRxLen) <= t_buf->FileSize)
            {
                memcpy(t_buf->CurPos, iAP2FileXferSession->iAP2FileXferRxBuf, iAP2FileXferSession->iAP2FileXferRxLen);
                t_buf->CurPos += iAP2FileXferSession->iAP2FileXferRxLen;
                t_buf->CurReceived += iAP2FileXferSession->iAP2FileXferRxLen;
            }
            else
            {
                printf("\n Exceeding total file size provided during setup");
                rc = IAP2_CTL_ERROR;
            }

            iap2SetTestState(FILE_TRANSFER_DATA_RECV,TRUE);
            printf("\t   received artwork data for FileID: %d \n",iAP2FileXferSession->iAP2FileTransferID);
        }
    }
    else
    {
      printf("\n CoverArt Buffer is NULL");
    }

    return rc;
}

LOCAL void iap2TestDeleteFileTransferList()
{
    iap2FileXferBuf_t* curr_file = NULL;

    if(g_CoverArtBuf != NULL)
    {
        curr_file = g_CoverArtBuf->nextfile;
        while(curr_file != NULL)
        {
            g_CoverArtBuf->nextfile = curr_file->nextfile;
            iap2TestFreePtr( (void**)&curr_file->Buffer);
            iap2TestFreePtr( (void**)&curr_file);
            curr_file = g_CoverArtBuf->nextfile;
        }
        if(g_CoverArtBuf != NULL)
        {
          iap2TestFreePtr( (void**)&g_CoverArtBuf->Buffer);
          iap2TestFreePtr( (void**)&g_CoverArtBuf);
        }
    }
    else
    {
        printf("\n CoverArt List is already empty");
    }
}

S32 iap2FileTransferSetup_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t* iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferSetup_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        if(iAP2FileXferSession->iAP2FileXferRxLen > 0)
        {
            rc = iap2CheckIfNotPresent(iAP2FileXferSession->iAP2FileTransferID);
            if(rc == IAP2_OK)
            {
                printf("\t   artwork available. FileID: %d  FileSize: %llu  \n",
                      iAP2FileXferSession->iAP2FileTransferID, iAP2FileXferSession->iAP2FileXferRxLen);

                rc = iap2AddToList(iAP2FileXferSession);
            }
            else
            {
                printf("\n Duplicate File transfer received, fileID:%d", iAP2FileXferSession->iAP2FileTransferID);
            }
        }
        else
        {
            printf("\t no artwork available. FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        }
    }

    return rc;
}

S32 iap2FileTransferDataRcvd_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferDataRcvd_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        rc = iap2CopyToBuffer(iAP2FileXferSession);
    }

    return rc;
}

LOCAL S32 iap2WriteCoverArtToFile(iAP2FileTransferSession_t* iAP2FileXferSession)
{
    S32 rc = IAP2_CTL_ERROR;
    iap2FileXferBuf_t *cur_file = NULL;
    FILE *fp;
    char fileName[25] = {0};

    if(g_CoverArtBuf != NULL)
    {
        cur_file = g_CoverArtBuf;
        while(cur_file != NULL)
        {
            if(cur_file->FileID == iAP2FileXferSession->iAP2FileTransferID)
            {
                rc = IAP2_OK;
                break;
            }
            cur_file = cur_file->nextfile;
        }

        if(rc != IAP2_OK)
        {
            printf("File transfer is not yet started for this ID: %d", iAP2FileXferSession->iAP2FileTransferID);
        }
        else if(cur_file->CurReceived != cur_file->FileSize)
        {
            rc = IAP2_CTL_ERROR;
            printf("\n Received file size is less than the file size declared during setup!!!");
        }
        else
        {
            sprintf(&fileName[0], "%s%d%s", "/tmp/CoverArt", iAP2FileXferSession->iAP2FileTransferID, ".jpg");

            fp = fopen(&fileName[0], "w");
            if(fp != NULL)
            {
                fwrite(cur_file->Buffer, 1, cur_file->FileSize, fp);
                fclose(fp);

                printf("\n   File Transfer Success!  Please check %s \n", &fileName[0]);
            }
            else
            {
                rc = IAP2_CTL_ERROR;
                printf("\n Unable to open file:%s", fileName);
            }
        }
    }
    else
    {
        printf("\n File Transfer is not yet started");
    }

    return rc;
}
LOCAL void iap2TestDeleteFileFromList(iAP2FileTransferSession_t* iAP2FileXferSession)
{
    S32 rc = IAP2_CTL_ERROR;
    iap2FileXferBuf_t *filetoremove = NULL, *prevfile = NULL;

    if(g_CoverArtBuf != NULL)
    {
        if(g_CoverArtBuf->FileID == iAP2FileXferSession->iAP2FileTransferID)
        {
            filetoremove = g_CoverArtBuf;
            g_CoverArtBuf = g_CoverArtBuf->nextfile;
            rc = IAP2_OK;
        }
        else
        {
            filetoremove = g_CoverArtBuf->nextfile;
            prevfile = g_CoverArtBuf;
            while(filetoremove != NULL)
            {
                if(filetoremove->FileID == iAP2FileXferSession->iAP2FileTransferID)
                {
                    rc = IAP2_OK;
                    prevfile->nextfile = filetoremove->nextfile;
                    break;
                }
                prevfile = filetoremove;
                filetoremove = filetoremove->nextfile;
            }
            if(rc != IAP2_OK)
            {
                printf("File transfer is not yet started for this ID: %d", iAP2FileXferSession->iAP2FileTransferID);
            }
        }

        if(rc == IAP2_OK)
        {
            iap2TestFreePtr( (void**)&filetoremove->Buffer);
            iap2TestFreePtr( (void**)&filetoremove);
            prevfile = NULL;
            iap2SetTestState(FILE_TRANSFER_SUCCESS_RECV,TRUE);
        }
    }
    else
    {
        printf("\n Cover Art List is empty!!!");
    }
}
S32 iap2FileTransferSuccess_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferSuccess_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        rc = iap2WriteCoverArtToFile(iAP2FileXferSession);
        iap2TestDeleteFileFromList(iAP2FileXferSession);
    }

    return rc;
}

S32 iap2FileTransferFailure_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferFailure_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer failed.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);

        iap2TestDeleteFileFromList(iAP2FileXferSession);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_FAILED_RECV,TRUE);

    return rc;
}

S32 iap2FileTransferCancel_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferCancel_CB called \n", iap2CurrTimeMs(), iap2Device);

    if(iAP2FileXferSession != NULL)
    {
        printf("\t   FileTransfer canceled.  FileID: %d \n", iAP2FileXferSession->iAP2FileTransferID);
        iap2TestDeleteFileFromList(iAP2FileXferSession);
    }
    else
    {
        rc = IAP2_CTL_ERROR;
    }

    iap2SetTestState(FILE_TRANSFER_CANCELED_RECV,TRUE);
    return rc;
}

S32 iap2FileTransferPause_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    context = context;

    printf("\t %d ms %p  iap2FileTransferPause_CB called \n", iap2CurrTimeMs(), iap2Device);

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

    printf("\t %d ms %p  iap2FileTransferResume_CB called \n", iap2CurrTimeMs(), iap2Device);

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

/* **********************  functions ********************** */

void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
}

void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks)
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

/* Sending iAP2StartMediaLibraryInformation to device */
LOCAL S32 iap2TestStartMediaLibInfo(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 retry_count = 0;
    iAP2StartMediaLibraryInformationParameter theiAP2StartMediaLibraryInformationParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

    memset(&theiAP2StartMediaLibraryInformationParameter, 0, sizeof(iAP2StartMediaLibraryInformationParameter));
    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                          MQ_CMD_START_MEDIALIB_INFO,
                          &theiAP2StartMediaLibraryInformationParameter, sizeof(theiAP2StartMediaLibraryInformationParameter));

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StartMediaLibraryInformationParameter(&theiAP2StartMediaLibraryInformationParameter); */

    if(rc != IAP2_OK)
    {
        printf(" %d ms:  iap2TestStartMediaLibInfo(): failed. iAP2StartMediaLibraryInformation  rc = %d \n", iap2CurrTimeMs(), rc);
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
            printf(" %d ms:  iap2TestStartMediaLibInfo(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
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
    int rc = IAP2_CTL_ERROR;
    iAP2StopMediaLibraryInformationParameter StopMediaLibraryInformationParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

    memset(&StopMediaLibraryInformationParameter, 0, sizeof(iAP2StopMediaLibraryInformationParameter));
    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_MEDIALIB_INFO,
                           &StopMediaLibraryInformationParameter, sizeof(StopMediaLibraryInformationParameter));

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopMediaLibraryInformationParameter(&StopMediaLibraryInformationParameter); */
    if(rc != IAP2_OK)
    {
        printf(" %d ms:  iap2TestStopMediaLibInfo(): failed. iAP2StopMediaLibraryInformation  rc = %d \n", iap2CurrTimeMs(), rc);
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* sending iAP2StartMediaLibraryUpdates to device */
LOCAL S32 iap2TestStartMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_OK;
    U32 retry_count = 0;
    iAP2StartMediaLibraryUpdatesParameter theiAP2StartMediaLibraryUpdatesParameter;
    FILE *fp = NULL;
    U32 i=0;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

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

                if(g_iap2TestDevice.testMediaLibLastKnownRev != NULL)
                {
                    theiAP2StartMediaLibraryUpdatesParameter.iAP2LastKnownMediaLibraryRevision =  calloc(1, sizeof(U8**));
                    if(theiAP2StartMediaLibraryUpdatesParameter.iAP2LastKnownMediaLibraryRevision != NULL)
                    {
                        *(theiAP2StartMediaLibraryUpdatesParameter.iAP2LastKnownMediaLibraryRevision) = calloc(1, strlen((const char*)g_iap2TestDevice.testMediaLibLastKnownRev));
                        if(*(theiAP2StartMediaLibraryUpdatesParameter.iAP2LastKnownMediaLibraryRevision) != NULL)
                        {
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2LastKnownMediaLibraryRevision_count++;
                            memcpy(*(theiAP2StartMediaLibraryUpdatesParameter.iAP2LastKnownMediaLibraryRevision), g_iap2TestDevice.testMediaLibLastKnownRev, strlen((const char*)g_iap2TestDevice.testMediaLibLastKnownRev));
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
                if(rc == IAP2_OK)
                {
                    /* set MediaItem properties which you want to receive */
                    theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties = calloc(1, sizeof(iAP2MediaItemProperties));
                    if(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties != NULL)
                    {
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties_count++;
                        /* MediaItem property PersistentIdentifier must be received */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count++;
                        /* set which MediaItem property (title, artist, etc.) should receive */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count++;
                        /* set Media type */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyMediaType_count++;
                        /* set Rating */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyRating_count++;
                        /* set Playback duration */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count++;
                        /* set Album persistent ID */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumPersistentIdentifier_count++;
                        /* set Album title */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTitle_count++;
                        /* Set Album Track Number */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackNumber_count++;
                        /* Album Track count */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackCount_count++;
                        /* Set Album disc number */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscNumber_count++;
                        /* Set Album disc count */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscCount_count++;
                        /* Set Artist Persistent ID */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyArtistPersistentIdentifier_count++;
                        /* Set Artist */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyArtist_count++;
                        /* Set Album artist persistent ID*/
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count++;
                        /* Set Album artist */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtist_count++;
                        /* Set Genre Persistent ID */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenrePersistenIdentifier_count++;
                        /* Set Composer Persistent Id */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyComposerPersistentIdentifier_count++;
                        /* Set composer */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyComposer_count++;
                        /* get whether part of compilation option is enabled */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsPartOfCompilation_count++;
                        /* get whether like option is supported */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsLikeSupported_count++;
                        /* get whether Ban option is supported */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanSupported_count++;
                        /* get whether the track is liked */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsLiked_count++;
                        /* get whether the track is banned */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanned_count++;
                        /* get whether the resident on device option is enabled */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyIsResidentOndevice_count++;

                        /* set MediaPlaylist properties which you want to receive */
                        theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties = calloc(1, sizeof(iAP2MediaPlaylistProperties));
                        if(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties != NULL)
                        {
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties_count++;
                            /* Get playlist persistent ID */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPersistentIdentifier_count++;
                            /* Get playlist name */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyName_count++;
                            /* Get playlist parent persistent ID */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyParentPersistentIdentifier_count++;
                            /* Get whether Genius Mix is enabled */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count++;
                            /* Get whether playlist contains folder */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsFolder_count++;
                            /* Get playlist contained media items */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListContainedMediaItems_count++;
                            /* Get iTunes Radio station */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsiTunesRadioStation_count++;

                            /* get percentage completion for current set of MediaLibraryUpdates */
                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUpdateProgress_count++;

                            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryIsHidingRemoteItems_count++;

                            /* start MediaLibraryUpdates */
                            rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                                   MQ_CMD_START_MEDIALIB_UPDATE,
                                                   &theiAP2StartMediaLibraryUpdatesParameter, sizeof(theiAP2StartMediaLibraryUpdatesParameter));
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
                else
                {
                	iap2SetTestStateError(TRUE);
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
        iAP2FreeiAP2StartMediaLibraryUpdatesParameter(&theiAP2StartMediaLibraryUpdatesParameter);
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        printf(" %d ms:  iap2TestStartMediaLibUpdates(): failed. iAP2StartMediaLibraryUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        while( (rc == IAP2_OK) && (g_iap2TestDevice.testMediaLibUpdateProgress < 100)
                &&(iap2GetTestStateError() != TRUE) && (retry_count < 2000) )
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

            /* Write track details to /tmp/Song_details.txt */
            fp = fopen("/tmp/songs_details.txt","w");
            if(fp != NULL)
            {
                fprintf(fp, "================================================================================================"
                            "======================================================================================================\n");
                fprintf(fp, "%-60s\t%-100s\t%-22s\t%-6s\n", "Title","Album title","Playback Duration(ms)","Rating");
                fprintf(fp, "================================================================================================"
                            "======================================================================================================\n");

                for(i=0; i<g_iap2TestDevice.testMediaItemCnt;i++)
                {
                    if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle_count > 0)
                    {
                        fprintf(fp, "%-60s\t", *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemTitle));
                    }
                    if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle_count > 0)
                    {
                        fprintf(fp, "%-100s\t", *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemAlbumTitle));
                    }
                    if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds_count > 0)
                    {
                        fprintf(fp, "%-22d\t", *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemPlaybackDurationInMilliseconds));
                    }
                    if(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemRating_count > 0)
                    {
                        fprintf(fp, "%-6d\t", *(g_iap2TestDevice.testMediaItem[i].iAP2MediaItemRating));
                    }
                    fprintf(fp, "\n");
                }
                fprintf(fp, "\nTotal number of songs:%d\n", g_iap2TestDevice.testMediaItemCnt);
                fclose(fp);
            }
        }
        else
        {
            printf(" %d ms:  iap2TestStartMediaLibUpdate(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
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
LOCAL S32 iap2TestStopMediaLibUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StopMediaLibraryUpdatesParameter StopMediaLibraryUpdatesParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

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
            rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                   MQ_CMD_STOP_MEDIALIB_UPDATE,
                                   &StopMediaLibraryUpdatesParameter, sizeof(StopMediaLibraryUpdatesParameter));
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
        printf(" %d ms:  iap2TestStopMediaLibUpdate(): failed. iAP2StopMediaLibraryUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* Sending iAP2StartNowPlayingUpdates to device */
LOCAL S32 iap2TestStartNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StartNowPlayingUpdatesParameter theiAP2StartNowPlayingUpdatesParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

    memset(&theiAP2StartNowPlayingUpdatesParameter, 0, sizeof(iAP2StartNowPlayingUpdatesParameter));

    /* set MediaItemAttributes properties which you want to receive */
    theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes = (iAP2MediaItemAttributes*)calloc(1, sizeof(iAP2MediaItemAttributes));
    if(theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes != NULL)
    {
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscCount_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscNumber_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackCount_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackNumber_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemComposer_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemGenre_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemIsBanned_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemIsBanSupported_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemIsLiked_count++;
        theiAP2StartNowPlayingUpdatesParameter.iAP2MediaItemAttributes->iAP2MediaItemIsLikeSupported_count++;
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
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PBiTunesRadioAd_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PBiTunesStationMediaPlaylistPersistentID_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PBiTunesStationName_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackQueueChapterIndex_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackQueueCount_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackQueueIndex_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackStatus_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackMediaLibraryUniqueIdentifier_count++;
            theiAP2StartNowPlayingUpdatesParameter.iAP2PlaybackAttributes->iAP2PlaybackAppBundleID_count++;

            rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                   MQ_CMD_START_NOWPLAYING_UPDATE,
                                   &theiAP2StartNowPlayingUpdatesParameter, sizeof(theiAP2StartNowPlayingUpdatesParameter));
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
    iAP2FreeiAP2StartNowPlayingUpdatesParameter(&theiAP2StartNowPlayingUpdatesParameter);

    if(rc != IAP2_OK)
    {
        printf(" %d ms:  iap2StartNowPlayingUpdate(): failed. iAP2StartNowPlayingUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
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
LOCAL S32 iap2TestStopNowPlayingUpdate(S32 mq_fd, iAP2Device_t* iap2Device)
{
    int rc = IAP2_CTL_ERROR;
    iAP2StopNowPlayingUpdatesParameter theiAP2StopNowPlayingUpdatesParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

    memset(&theiAP2StopNowPlayingUpdatesParameter, 0, sizeof(iAP2StopNowPlayingUpdatesParameter));
    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_STOP_NOWPLAYING_UPDATE,
                           &theiAP2StopNowPlayingUpdatesParameter, sizeof(theiAP2StopNowPlayingUpdatesParameter));

    /* No need to free as there are no associated parameters */
    /* iAP2FreeiAP2StopNowPlayingUpdatesParameter(&theiAP2StopNowPlayingUpdatesParameter); */

    if(rc != IAP2_OK)
    {
        printf(" %d ms:  iap2TestStopNowPlayingUpdate(): failed. iAP2StopNowPlayingUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
        iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* Sending iAP2PlayMediaLibraryItems to device */
LOCAL S32 iap2TestStartPlayMediaLibraryItem(S32 mq_fd, iAP2Device_t* iap2Device, U16 numSongsToPlay)
{
    int rc = IAP2_OK;
    U32 retry_count = 0;
    U16 tmpNumSongs = 0;
    U16 i = 0;
    U16 PersistendIdentifierCount = 0;
    U8* BlobDataLocation = NULL;
    iAP2PlayMediaLibraryItemsParameter theiAP2PlayMediaLibraryItemsParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

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
            rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                                   MQ_CMD_START_PLAY_MEDIALIB_ITEM,
                                   &theiAP2PlayMediaLibraryItemsParameter, sizeof(theiAP2PlayMediaLibraryItemsParameter));
        }

        iAP2FreeiAP2PlayMediaLibraryItemsParameter(&theiAP2PlayMediaLibraryItemsParameter);

        if(rc != IAP2_OK)
        {
            printf(" %d ms:  iap2TestStartPlayMediaLibraryItem(): failed. iAP2PlayMediaLibraryItems  rc = %d \n", iap2CurrTimeMs(), rc);
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
                printf(" %d ms:  iap2TestStartPlayMediaLibraryItem(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
                rc = IAP2_CTL_ERROR;
            }
        }
    }
    else
    {
        printf(" %d ms:  iap2TestStartPlayMediaLibraryItem():  PersistendIdentifierCount is %d \n ",
                iap2CurrTimeMs(),PersistendIdentifierCount);
        rc = IAP2_CTL_ERROR;
    }

    if(rc != IAP2_OK)
    {
    	iap2SetTestStateError(TRUE);
    }

    return rc;
}

/* Sending iAP2StartUSBDeviceModeAudio to device */
LOCAL S32 iap2TestStartUSBDeviceModeAudio(S32 mq_fd, iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_OK;
    U32 retry_count = 0;
    iAP2StartUSBDeviceModeAudioParameter theiAP2StartUSBDeviceModeAudioParameter;

    /* temporary fix for compiler warning */
    iap2Device = iap2Device;

    memset(&theiAP2StartUSBDeviceModeAudioParameter, 0, sizeof(theiAP2StartUSBDeviceModeAudioParameter) );
    rc = iap2SendMqRecvAck(mq_fd, g_iap2TestDevice.mqAppTskFd,
                           MQ_CMD_START_USB_DEVICEMODE_AUDIO,
                           &theiAP2StartUSBDeviceModeAudioParameter, sizeof(theiAP2StartUSBDeviceModeAudioParameter));

    if(rc != IAP2_OK)
    {
        printf(" %d ms:  iap2TestStartUSBDeviceModeAudio(): failed. iAP2StartNowPlayingUpdates  rc = %d \n", iap2CurrTimeMs(), rc);
    }
    else
    {
        /* wait for callback with requested information */
        while( (rc == IAP2_OK) && (iap2GetTestState(USB_DEVICE_MODE_AUDIO_INFO_RECV) != TRUE)
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
        if(iap2GetTestState(USB_DEVICE_MODE_AUDIO_INFO_RECV) == TRUE)
        {
            rc = IAP2_OK;
        }
        else
        {
            printf(" %d ms:  iap2TestStartUSBDeviceModeAudio(): failed. retry: %d \n", iap2CurrTimeMs(), retry_count);
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
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StartNowPlayingUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_STOP_NOWPLAYING_UPDATE:
                {
                    /* Sending iap2TestStopNowPlayingUpdate to device */
                    rc = iAP2StopNowPlayingUpdates(iap2Device, (iAP2StopNowPlayingUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StopNowPlayingUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_START_MEDIALIB_INFO:
                {
                    /* Sending iAP2StartMediaLibraryInformation to device */
                    rc = iAP2StartMediaLibraryInformation(iap2Device, (iAP2StartMediaLibraryInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StartMediaLibraryInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_STOP_MEDIALIB_INFO:
                {
                    /* Sending iAP2StopMediaLibraryInformation to device */
                    rc = iAP2StopMediaLibraryInformation(iap2Device, (iAP2StopMediaLibraryInformationParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StopMediaLibraryInformation failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_START_MEDIALIB_UPDATE:
                {
                    /* start MediaLibraryUpdates */
                    rc = iAP2StartMediaLibraryUpdates(iap2Device, (iAP2StartMediaLibraryUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StartMediaLibraryUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_STOP_MEDIALIB_UPDATE:
                {
                    /* stop MediaLibraryUpdates */
                    rc = iAP2StopMediaLibraryUpdates(iap2Device, (iAP2StopMediaLibraryUpdatesParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StopMediaLibraryUpdates failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_START_USB_DEVICEMODE_AUDIO:
                {
                    /* send start USB device mode audio to device */
                    rc = iAP2StartUSBDeviceModeAudio(iap2Device, (iAP2StartUSBDeviceModeAudioParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2StartUSBDeviceModeAudio failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_START_PLAY_MEDIALIB_ITEM:
                {
                    /* start playback */
                    rc = iAP2PlayMediaLibraryItems(iap2Device, (iAP2PlayMediaLibraryItemsParameter*)mq_st->param);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2PlayMediaLibraryItems failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
                    }
                    break;
                }
                case MQ_CMD_CANCEL_IAP1_SUPPORT:
                {
                    /* send CanceliAP1Support */
                    rc = iAP2CanceliAP1Support(iap2Device);
                    if(rc != IAP2_OK)
                    {
                        printf(" %d ms:  iAP2CanceliAP1Support failed!  rc = %d \n", iap2CurrTimeMs(), rc);
                        iap2SetTestStateError(TRUE);
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
    if(iap2GetTestStateError() != TRUE)
    {
        /* start MediaLibraryInfo */
        printf("\n %d ms  iap2TestStartMediaLibInfo() \n", iap2CurrTimeMs());
        rc = iap2TestStartMediaLibInfo(mq_fd, iap2device);
        /* stop MediaLibraryInfo */
        printf("\n %d ms  iap2TestStopMediaLibInfo() \n", iap2CurrTimeMs());
        rc = iap2TestStopMediaLibInfo(mq_fd, iap2device);
    }

    if(iap2GetTestStateError() != TRUE)
    {
        /* start MediaLibrarayUpdate */
        printf("\n %d ms  iap2TestStartMediaLibUpdate() \n", iap2CurrTimeMs());
        rc = iap2TestStartMediaLibUpdate(mq_fd, iap2device);
//                iap2SleepMs(2000);
    }

    if(iap2GetTestStateError() != TRUE)
    {
        /* start NowPlayingUpdates */
        printf("\n %d ms  iap2TestStartNowPlayingUpdate() \n", iap2CurrTimeMs());
        rc = iap2TestStartNowPlayingUpdate(mq_fd, iap2device);
    }

    if(iap2GetTestStateError() != TRUE)
    {
        /* PlayMediaLibraryItem */
        printf("\n %d ms  iap2TestStartPlayMediaLibraryItem() \n", iap2CurrTimeMs());
        rc = iap2TestStartPlayMediaLibraryItem(mq_fd, iap2device, 10);
        iap2SleepMs(2000);
    }

    if( (iap2GetTestStateError() != TRUE)
        && (g_iap2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE) )
    {
        /* start USBDeviceModeAudio */
        printf("\n %d ms  iap2TestStartUSBDeviceModeAudio() \n", iap2CurrTimeMs());
        rc = iap2TestStartUSBDeviceModeAudio(mq_fd, iap2device);
        iap2SleepMs(1000);
    }

    if( (iap2GetTestStateError() != TRUE)
        && (iap2GetGstState() == IAP2_GSTREAMER_PLAYING))
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

    if(g_iap2TestDevice.testMediaLibInfoID != NULL)
    {
        /* check if Apple device was disconnected */
        if(g_iap2TestDevice.testDeviceState == iAP2NotConnected){
            printf("\n %d ms  Apple device was disconnected! \n", iap2CurrTimeMs());
        } else{
            /* stop NowPlayingUpdates */
            printf("\n %d ms  iap2TestStopNowPlayingUpdate() \n", iap2CurrTimeMs());
            rc = iap2TestStopNowPlayingUpdate(mq_fd, iap2device);

            /* stop MediaLibraryUpdate */
            printf("\n %d ms  iap2TestStopMediaLibUpdate() \n", iap2CurrTimeMs());
            rc = iap2TestStopMediaLibUpdate(mq_fd, iap2device);
        }
        iap2SleepMs(1000);
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
    printf(" %d ms  iAP2InitDeviceStructure = %p \n", iap2CurrTimeMs(), iap2device);
    IAP2TESTDLTLOG(DLT_LOG_ERROR, "iAP2InitDeviceStructure iap2device = %p ", iap2device);

    if(NULL != iap2device)
    {
        if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
           &&(rc == IAP2_OK))
        {
            rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_HOST, socket_fd,
            iAP2AppThreadInitData->udevPath, iAP2InitParameter);
        }
        if(rc == IAP2_OK)
        {
            rc = iAP2InitDeviceConnection(iap2device);
            printf(" %d ms  iAP2InitDeviceConnection:   rc  = %d \n", iap2CurrTimeMs(), rc);
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
            while( (g_iap2TestDevice.testDeviceState != iAP2DeviceReady) && (g_iap2TestDevice.testDeviceState != iAP2LinkiAP1DeviceDetected)
                    && (g_iap2TestDevice.testDeviceState != iAP2ComError) && (retry_count < 300) )
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
                printf(" %d ms  device attached  retry: %d \n", iap2CurrTimeMs(), retry_count);
                rc = IAP2_OK;
            }
            else if(g_iap2TestDevice.testDeviceState == iAP2LinkiAP1DeviceDetected)
            {
                printf(" %d ms  iAP1 device detected  retry: %d \n", iap2CurrTimeMs(), retry_count);

                printf(" iAP1 not supported. send Bad DETECT Ack to device \n");
                rc = iap2TestCanceliAP1Support(mq_fd, iap2device);

                iap2SetTestStateError(TRUE);
            }
            else if(g_iap2TestDevice.testDeviceState == iAP2ComError)
            {
               printf(" %d ms  Error in Device Authentication or Identification  retry: %d \n", iap2CurrTimeMs(), retry_count);
               iap2SetTestStateError(TRUE);
               rc = IAP2_CTL_ERROR;
            }
            else
            {
                printf(" %d ms  device not attached [state: %d | retry: %d] \n", iap2CurrTimeMs(), g_iap2TestDevice.testDeviceState, retry_count);
                iap2SetTestStateError(TRUE);
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

        /* exit pollThread */
        if(threadID > 0)
        {
           (void)iap2TestStopPollThread(mq_fd, iap2device);
        }
        iap2SetTestState(STOPPED, TRUE);
        iap2SetTestState(RUNNING, FALSE);

#if IAP2_GST_AUDIO_STREAM
        /* stop gstreamer main loop */
        iap2SetGstState(IAP2_GSTREAMER_STATE_STOP);
#endif

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
        }
        pthread_mutex_unlock(&g_iap2TestDevice.testMediaLibMutex);

        iap2TestDeleteFileTransferList();

        rc = iAP2DisconnectDevice(iap2device);
        printf(" %d ms  iAP2DisconnectDevice:   rc  = %d \n", iap2CurrTimeMs(), rc);
        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iAP2DisconnectDevice = %d", rc);

        if((g_iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
           &&(rc == IAP2_OK))
        {
            rc = iap2AppSendSwitchMsg (IAP2_HOST_DEVICE_SWITCH_CMD__SWITCH_DEVICE, socket_fd,
            iAP2AppThreadInitData->udevPath, iAP2InitParameter);
        }

        /* de-initialize the device structure */
        rc = iAP2DeInitDeviceStructure(iap2device);
        printf(" %d ms  iAP2DeInitDeviceStructure:   rc  = %d \n", iap2CurrTimeMs(), rc);
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

    /* destroy mutex */
    pthread_mutex_destroy(&g_iap2TestDevice.testMediaLibMutex);

    printf(" %u ms  exit iap2AppThread \n", iap2CurrTimeMs());
    pthread_exit((void*)exinf);
}

LOCAL S32 application_process_main (int socket_fd) {
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
        printf("iap2_stresstest                 PASS       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 STRESSTEST PASS *****");
    }
    else
    {
        printf("iap2_stresstest                 FAIL       %d\n", g_rc);
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 STRESSTEST FAIL *****");
    }

    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return rc;
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
