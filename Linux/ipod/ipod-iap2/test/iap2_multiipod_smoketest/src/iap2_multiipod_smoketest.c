/*
 * iap2_multiipod_smoketest.c
 *
 *  Created on: Oct 11, 2013
 *      Author: ajaykumar.s
 */

/* **********************  includes  ********************** */
#include "iap2_multiipod_smoketest.h"
#include <iap2_callbacks.h>
#include <iap2_parameter_free.h>
#include <iap2_commands.h>
#include "iap2_external_accessory_protocol_session.h"

#include <endian.h>

#include "iap2_dlt_log.h"
#include "iap2_test_bt_initialize.h"
#include "iap2_test_bt_device.h"
#include "iap2_test_bt_profile.h"
#include "iap2_test_bt_agent.h"

//   4 u64 le64_to_cpup(__le64 *);
//   5 u32 le32_to_cpup(__le32 *);
//   6 u16 le16_to_cpup(__le16 *);

/* Functions to Adhere to Apple Endianess */
#if __BYTE_ORDER == __LITTLE_ENDIAN

#define IAP2_ADHERE_TO_APPLE_ENDIANESS_64(x) htobe64(x)

#elif __BYTE_ORDER == __BIG_ENDIAN

#define IAP2_ADHERE_TO_APPLE_ENDIANESS_64(x) (x)

#else

#error - only big and little endian systems are supported.

#endif


/* **********************  defines   ********************** */

#define NUM_IPOD 8
#define IPOD_MSEC 1000
#define IPOD_NSEC 1000000

/* **********************  globals    ********************** */

typedef struct
{
    U8* Buffer;
    U8* CurPos;
} iAP2FileXferBuf;

typedef struct
{
    iAP2Device_t*     device;
    iAP2DeviceState_t iPodState;
    pthread_t         tid;
    BOOL              leavePoll;
    char              devid[DEV_DETECT_CFG_STRING_MAX];

    BOOL              mediaLibInfoSent;
    BOOL              playMediaItemSent;
    BOOL              eapTest;
    U8*               tempMediaLibraryInfoLibID ;
    iAP2FileXferBuf   coverArtBuf;

//    iAP2UserConfig_t iAP2UserConfig; /*to have separate configuration for each ipod */
} ipodlist_t ;

ipodlist_t g_ipodlist[NUM_IPOD] ;
iap2UserConfig_t g_iap2UserConfig;

S32  g_rc = IAP2_OK;
BOOL g_endPoll = FALSE;
BOOL g_EAPtesting = FALSE;
BOOL g_leaveGstWhile = FALSE;
BOOL GStreamer_test = FALSE;
U32  CurrentSongTotalDuration = 0;
U32  g_num_ipod = 0;
S32  g_exit_thread = FALSE;
S32  g_next_ipod = FALSE;

LOCAL void iap2AppThread(void* exInfo);

/* **********************  locals    ********************** */

void iPodOSSleep(U32 sleep_ms)
{
    S32 s32ReturnValue;
    struct timespec req;
    struct timespec remain;

    /* Initialize the structure */
    memset(&req, 0, sizeof(req));
    memset(&remain, 0, sizeof(remain));


    req.tv_sec = sleep_ms / IPOD_MSEC;
    req.tv_nsec = (sleep_ms % IPOD_MSEC) * IPOD_NSEC;

    while(1)
    {
        s32ReturnValue = nanosleep(&req, &remain);

        if (s32ReturnValue == 0)
        {
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                req.tv_sec = remain.tv_sec ;
                req.tv_nsec = remain.tv_nsec;
            }
            else
            {
                break;
            }
        }
    }// end while

}

U32 iap2CurrTimeValToMs(struct timeval* currTime)
{
    return (U32)(currTime->tv_sec * 1000) + (U32)(currTime->tv_usec / 1000);
}

U32 iap2CurrTimeMs(void)
{
    U32 timeMs;
    struct timeval tp;
    gettimeofday (&tp, NULL);
    timeMs = iap2CurrTimeValToMs(&tp);

    return timeMs;
}

S32 iap2DeviceState_CB(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    S32 rc = IAP2_OK;
    U32 ipod_count = 0 ;

    /* temporary to avoid compiler warings */
    context = context;

    switch(dState)
    {
        case iAP2NotConnected :

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device )
                {
                    g_ipodlist[ipod_count].iPodState = iAP2NotConnected;
                    printf("\n********\t %d ms ipod  %d   device:%p   serial: %s  iAP2NotConnected *********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device,g_ipodlist[ipod_count].devid);
                }
            }

            break;
        case iAP2TransportConnected :

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device )
                {
                    g_ipodlist[ipod_count].iPodState = iAP2TransportConnected;
                    printf("\n********\t %d ms ipod %d   device:%p  serial: %s  iAP2TransportConnected ********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device, g_ipodlist[ipod_count].devid);
                }
            }

            break;
        case iAP2LinkConnected:

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device )
                {
                    g_ipodlist[ipod_count].iPodState = iAP2LinkConnected;
                    printf("\n********\t %d ms ipod %d   device:%p  serial: %s  iAP2LinkConnected ********\n\n", iap2CurrTimeMs(),ipod_count, iap2Device, g_ipodlist[ipod_count].devid);
                }
            }

            break;
        case iAP2AuthenticationPassed :

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device)
                {
                    g_ipodlist[ipod_count].iPodState = iAP2AuthenticationPassed;

                    printf("\n********\t %d ms ipod %d   device:%p  serial: %s  iAP2AuthenticationPassed ********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device, g_ipodlist[ipod_count].devid);
                }

            }

            break;
        case iAP2IdentificationPassed :

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device )
                {
                    g_ipodlist[ipod_count].iPodState = iAP2IdentificationPassed;
                    printf("\n********\t %d ms ipod %d   device:%p  serial: %s iAP2IdentificationPassed ********\n\n", iap2CurrTimeMs(),ipod_count, iap2Device, g_ipodlist[ipod_count].devid);
                }
            }

            break;
        case iAP2DeviceReady:

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device )
                {
                    g_ipodlist[ipod_count].iPodState = iAP2DeviceReady;
                    printf("\n********\t %d ms ipod %d   device:%p  serial: %s  iAP2DeviceReady ********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device, g_ipodlist[ipod_count].devid);

                }
            }

            break;
        case iAP2LinkiAP1DeviceDetected:

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device)
                {
                    /*todo:Send bad detect to apple device*/
                    g_ipodlist[ipod_count].iPodState = iAP2LinkiAP1DeviceDetected;
                    printf("\n ********\t %d ms ipod %d   device:%p  serial: %s  is an  iAP1 Device ********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device, g_ipodlist[ipod_count].devid);

                    /*Its sad but wait until  other device is finished with authentication. A  delay of 2 sec */
                    iPodOSSleep(2000);
                    g_ipodlist[ipod_count].leavePoll = TRUE;
//                    g_next_ipod = TRUE;

                }
            }

            break;

        default:

            for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
            {
                if(g_ipodlist[ipod_count].device == iap2Device )
                {
                    g_ipodlist[ipod_count].iPodState = iAP2ComError;
                    printf("\n********\t %d ms ipod %d   device:%p  serial: %s  ERROR ********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device, g_ipodlist[ipod_count].devid);

                    /*its sad but wait until other device is finished with authentication. A  delay of 2 sec */
                    iPodOSSleep(2000);
                    g_ipodlist[ipod_count].leavePoll = TRUE;
                }
            }
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

    printf("\t %d ms %p  iap2IdentificationRejected_CB called \n", iap2CurrTimeMs(), iap2Device);

    return rc;
}

S32 iap2PowerUpdate_CB(iAP2Device_t* iap2Device, iAP2PowerUpdateParameter* powerupdateParameter, void* context)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 ipod_count = 0;

    /* temporary fix for compiler warning */
    context = context;

    for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
    {
        if(g_ipodlist[ipod_count].device == iap2Device )
        {
            printf("\n\t\t\t ######### FOR IPOD %d ######## \n\n", ipod_count);
            printf("\n******** %d ms for ipod  %d   device:%p   serial: %s  iap2PowerUpdate_CB called  *********\n\n",iap2CurrTimeMs(), ipod_count, iap2Device,g_ipodlist[ipod_count].devid);

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

            if(g_ipodlist[ipod_count].eapTest == FALSE)
            {
                /* Sending iAP2StartMediaLibraryInformation to device */
                if(g_ipodlist[ipod_count].mediaLibInfoSent == FALSE)
                {
                    iAP2StartMediaLibraryInformationParameter theiAP2StartMediaLibraryInformationParameter;

                    memset(&theiAP2StartMediaLibraryInformationParameter, 0, sizeof(iAP2StartMediaLibraryInformationParameter));
                    iAP2StartMediaLibraryInformation(iap2Device, &theiAP2StartMediaLibraryInformationParameter);
//                    iAP2FreeiAP2StartMediaLibraryInformationParameter(&theiAP2StartMediaLibraryInformationParameter);
                    g_ipodlist[ipod_count].mediaLibInfoSent = TRUE;
                }
            }

        }
    }

    return rc;
}

S32 iap2MediaLibraryInfo_CB(iAP2Device_t* iap2Device, iAP2MediaLibraryInformationParameter* MediaLibraryInfoParameter, void* context)
{
    S32 rc = IAP2_OK;
    U32 ipod_count = 0;

    /* temporary fix for compiler warning */
    context =context;

    for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
    {
        if(g_ipodlist[ipod_count].device == iap2Device )
        {
            printf("\t#### for iPod %d  device %p  iap2MediaLibraryInfo_CB called \n\n",ipod_count, iap2Device);

            g_ipodlist[ipod_count].tempMediaLibraryInfoLibID = calloc(1,  strlen((const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier))+1);
            memcpy(g_ipodlist[ipod_count].tempMediaLibraryInfoLibID, *(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier),     \
                    strlen((const char*)*(MediaLibraryInfoParameter->iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier))+1);

            /* sending iAP2StartMediaLibraryUpdates to device */
            iAP2StartMediaLibraryUpdatesParameter theiAP2StartMediaLibraryUpdatesParameter;

            memset(&theiAP2StartMediaLibraryUpdatesParameter, 0, sizeof(iAP2StartMediaLibraryUpdatesParameter));

            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier = calloc(1, sizeof(U8**));
            *(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier) = calloc(1,        \
                    strlen((const char*)g_ipodlist[ipod_count].tempMediaLibraryInfoLibID));

            memcpy(*(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier), g_ipodlist[ipod_count].tempMediaLibraryInfoLibID,     \
                    strlen((const char*)g_ipodlist[ipod_count].tempMediaLibraryInfoLibID));

            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaLibraryUniqueIdentifier_count++;

            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties = calloc(1, sizeof(iAP2MediaItemProperties));
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties_count++;

            if(theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties == NULL)
            {
                perror("iAP2MediaItemProperties NULL!!! \n");
            }
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count++;


            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle = calloc(1, sizeof(U8));
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;

            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre = calloc(1, sizeof(U8));
            theiAP2StartMediaLibraryUpdatesParameter.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count++;

            iAP2StartMediaLibraryUpdates(iap2Device, &theiAP2StartMediaLibraryUpdatesParameter);
            iAP2FreeiAP2StartMediaLibraryUpdatesParameter(&theiAP2StartMediaLibraryUpdatesParameter);
        }
    }



    return rc;
}

S32 iap2MediaLibraryUpdates_CB(iAP2Device_t* iap2Device, iAP2MediaLibraryUpdateParameter* MediaLibraryUpdateParameter, void* context)
{
    S32 rc = IAP2_OK;
    U16 i = 0;
    U32 ipod_count = 0;
    U8* BlobDataLocation = NULL;
    U16 PersistendIdentifierCount = 0;

    /* temporary fix for compiler warning */
    context =context;

    for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
    {
        if(g_ipodlist[ipod_count].device == iap2Device )
        {

            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2MediaLibraryUpdates_CB called");
            printf("\t#### for iPod %d  device %p  iap2MediaLibraryUpdates_CB called \n\n", ipod_count, iap2Device);

            if(MediaLibraryUpdateParameter->iAP2MediaItem_count > 0 && g_ipodlist[ipod_count].playMediaItemSent == FALSE)
            {


                /* Sending PlayMediaLibraryCollection to device */

                iAP2PlayMediaLibraryItemsParameter theiAP2PlayMediaLibraryItemsParameter;

                memset(&theiAP2PlayMediaLibraryItemsParameter, 0, sizeof(iAP2PlayMediaLibraryItemsParameter));
                printf("\n\n Number of songs in apple device: %d \n", MediaLibraryUpdateParameter->iAP2MediaItem_count);
                for(i = 0; i < MediaLibraryUpdateParameter->iAP2MediaItem_count; i++)
                {
                    if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count != 0)
                    {
                        PersistendIdentifierCount++;
                    }
                }

                theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers = calloc(1, sizeof(iAP2Blob));
                theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers_count = 1;
                theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobData = calloc(PersistendIdentifierCount, sizeof(U64));
                theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobLength = PersistendIdentifierCount * sizeof(U64);
                BlobDataLocation = theiAP2PlayMediaLibraryItemsParameter.iAP2ItemsPersistentIdentifiers->iAP2BlobData;
                for(i = 0; i < MediaLibraryUpdateParameter->iAP2MediaItem_count; i++)
                {
                    if(MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier_count != 0)
                    {
                        memcpy(BlobDataLocation, MediaLibraryUpdateParameter->iAP2MediaItem[i].iAP2MediaItemPersistentIdentifier, sizeof(U64) );
                        BlobDataLocation += sizeof(U64);
                    }
                }

                theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier = calloc(1, sizeof(U8**));

                *(theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier) = calloc(1, strlen((const char*)g_ipodlist[ipod_count].tempMediaLibraryInfoLibID)+1);
                memcpy(*(theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier), g_ipodlist[ipod_count].tempMediaLibraryInfoLibID,      \
                        strlen((const char*)g_ipodlist[ipod_count].tempMediaLibraryInfoLibID));
                theiAP2PlayMediaLibraryItemsParameter.MediaLibraryUniqueIdentifier_count++;

                /**
                 * Start playback
                 */
                rc = iAP2PlayMediaLibraryItems(iap2Device, &theiAP2PlayMediaLibraryItemsParameter);
                iAP2FreeiAP2PlayMediaLibraryItemsParameter(&theiAP2PlayMediaLibraryItemsParameter);


                g_ipodlist[ipod_count].playMediaItemSent = TRUE;

                /*its sad but wait until other device is finished with authentication. A  delay of 2 sec */
                iPodOSSleep(2000);
                g_ipodlist[ipod_count].leavePoll = TRUE;

            }


        }
    }

    return rc;
}

S32 iap2NowPlayingUpdate_CB(iAP2Device_t* iap2Device, iAP2NowPlayingUpdateParameter* NowPlayingUpdateParameter, void* context)
{
    S32 rc = IAP2_OK;
    S16 i, j;
    U32 ipod_count = 0;

    /* temporary fix for compiler warning */
    context = context;
    for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
    {
        if(g_ipodlist[ipod_count].device == iap2Device )
        {
            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2NowPlayingUpdate_CB called \n");
            printf("\t#### for iPod %d  device %p  iap2NowPlayingUpdate_CB called \n", ipod_count,iap2Device);

            if(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle_count > 0)
            {
                printf(" iPod #%d now playing title:  %s \n",ipod_count, *(NowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle));
            }

            for(i = 0;i < NowPlayingUpdateParameter->iAP2MediaItemAttributes_count;i++)
            {
                if( &(NowPlayingUpdateParameter->iAP2MediaItemAttributes[i]) != NULL)
                {
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
//                            GStreamerState = IAP2_GSTREAMER_STATE_PLAYING;
//                            sem_post(&GstreamerSemaphore);
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
                            U32 milliseconds, seconds, minutes=0;
                            milliseconds = milliseconds;

                            milliseconds = NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j] % 1000;
                            seconds      = NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j] / 1000;
                            if(seconds != 0)
                            {
                                minutes      = seconds / 60;
                                seconds      = seconds % 60;
                            }

//                            if( (CurrentSongTotalDuration - 1500 < NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j]) &&
//                                    (NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j] > 1000) )
//                            {
//                                GStreamerState = IAP2_GSTREAMER_STATE_STOPPED;
//                                sem_post(&GstreamerSemaphore);
//                            }
                            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "CurrentSongTotalDuration = %d, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j] = %d",
                                    CurrentSongTotalDuration, NowPlayingUpdateParameter->iAP2PlaybackAttributes[i].iAP2PlaybackElapsedTimeInMilliseconds[j]);
                            IAP2TESTDLTLOG(DLT_LOG_DEBUG, "TimePosition:    %d:%d:%d", minutes, seconds, milliseconds);
                        }
                    }
                }
            }

        }
    }



    return rc;
}

S32 iap2FileTransferSetup_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 ipod_count = 0;

    /* To avoid compiler warnings */
    context = context;

    for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
    {
        if(g_ipodlist[ipod_count].device == iap2Device )
        {
            printf("\t#### for iPod %d  device %p  iap2FileTransferSetup_CB called \n", ipod_count,iap2Device);

            if(iAP2FileXferSession->iAP2FileXferRxLen > 0)
            {
                g_ipodlist[ipod_count].coverArtBuf.Buffer = calloc(1,iAP2FileXferSession->iAP2FileXferRxLen);
                g_ipodlist[ipod_count].coverArtBuf.CurPos = g_ipodlist[ipod_count].coverArtBuf.Buffer;
                if(g_ipodlist[ipod_count].coverArtBuf.Buffer != NULL)
                {
                    rc = IAP2_OK;
                }
                else
                {
                    printf("\n ERROR: CoverArt Buf memory allocation failed \n");
                    rc = IAP2_ERR_NO_MEM;
                }
            }
            else
            {
                printf("iPod #%d No artwork data present for now playing track \n",ipod_count);
            }

        }
    }

    return rc;
}

S32 iap2FileTransferDataRcvd_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;
    iap2Device = iap2Device;
    iAP2FileXferSession = iAP2FileXferSession;
    context = context;
//    iAP2FileTransferSession_t* fileXfr = NULL;
//    S32 ipod_count = 0;

//    for(ipod_count=0 ; ipod_count < g_num_ipod ; ipod_count++)
//    {
//        if(g_ipodlist[ipod_count].device == iap2Device )
//        {
//            printf("\t#### for iPod %d  device %p  iap2FileTransferDataRcvd_CB called \n", ipod_count,iap2Device);
//
//            /*its sad but wait until other device is finished with authentication. A  delay of 2 sec */
//            iPodOSSleep(2000);
//            g_ipodlist[ipod_count].leavePoll = TRUE;
//
//        }
//    }

//    fileXfr = (iAP2FileTransferSession_t*)context;
//
//    if(CoverArtBuf.CurPos != NULL)
//    {
//        memcpy(CoverArtBuf.CurPos, fileXfr->iAP2FileXferRxBuf, fileXfr->iAP2FileXferRxLen);
//        CoverArtBuf.CurPos += fileXfr->iAP2FileXferRxLen;
//    }
//    else
//    {
//        printf("\n CoverArt buffer is NULL \n");
//        rc = IAP2_ERR_NO_MEM;
//    }
//
//    printf("Received artwork data for File: 0x%2X \n",fileXfr->iAP2FileTransferID );
//
//    g_iap2TestDevice.testState.testStateFileTransferRecv = TRUE;
    return rc;
}

S32 iap2FileTransferSuccess_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;
//    FILE *fp;
//    iAP2FileTransferSession_t* fileXfr = NULL;

    /* To avoid compiler warnings */
    iap2Device = iap2Device;
    iAP2FileXferSession = iAP2FileXferSession;
    context = context;


//
//    fileXfr = (iAP2FileTransferSession_t*)context;
//
//    /*
//     * This will be not necessary anymore as we have receive buffer as part of
//     *  iAP2FileTransferSession_t structure.
//     *
//     *  TBD: remove
//     */
//
//    fp = fopen("/opt/CoverArt.jpg", "w");
//    fwrite(CoverArtBuf.Buffer, 1, fileXfr->iAP2FileXferRxLen, fp);
//    fclose(fp);
//    printf("\n File Transfer Success!!!, please check /opt/CoverArt.jpg  \n");
//
//    if(CoverArtBuf.Buffer != NULL)
//    {
//        free(CoverArtBuf.Buffer);
//        CoverArtBuf.Buffer = NULL;
//        CoverArtBuf.CurPos = NULL;
//    }
//
//    g_iap2TestDevice.testState.testStateFileTransferSuccessed = TRUE;
    return rc;
}

S32 iap2FileTransferFailure_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    S32 rc = IAP2_OK;

    /* To avoid compiler warnings */
    iap2Device = iap2Device;
    iAP2FileXferSession = iAP2FileXferSession;
    context = context;

    printf("\n File Transfer Failed \n");

//    g_iap2TestDevice.testState.testStateFileTransferFailed = TRUE;
    return rc;
}

S32 iap2FileTransferCancel_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    /* To avoid compiler warnings */
    iap2Device = iap2Device;
    iAP2FileXferSession = iAP2FileXferSession;
    context = context;

    printf("\n FileTransfer cancelled from Accessory side \n");

//    g_iap2TestDevice.testState.testStateFileTransferCanceled = TRUE;
    return TRUE;
}

S32 iap2FileTransferPause_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
 {
    /* To avoid compiler warnings */
    iap2Device = iap2Device;
    context = context;
    iAP2FileXferSession = iAP2FileXferSession;

    printf("\n FileTransfer Pause sent from Accessory side \n");

    return TRUE;
}

S32 iap2FileTransferResume_CB(iAP2Device_t* iap2Device, iAP2FileTransferSession_t*  iAP2FileXferSession, void* context)
{
    /* To avoid compiler warnings */
    iap2Device = iap2Device;
    context = context;
    iAP2FileXferSession = iAP2FileXferSession;

    printf("\n FileTransfer Resume sent from Accessory side \n");

    return TRUE;
}

S32 iap2StartExternalAccessoryProtocolSession_CB(iAP2Device_t* iap2Device, iAP2StartExternalAccessoryProtocolSessionParameter* theiAP2StartExternalAccessoryProtocolSessionParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    theiAP2StartExternalAccessoryProtocolSessionParameter = theiAP2StartExternalAccessoryProtocolSessionParameter;
    context = context;

   printf("\t %p  iap2StartExternalAccessoryProtocolSession_CB called \n", iap2Device);

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
        if( (i % 26) == 0)
            printf("\n");
        printf(" 0x%.2X, ", iAP2iOSAppDataRxd[i]);
    }

    return rc;
}

S32 iap2USBDeviceModeAudioInformation_CB(iAP2Device_t* iap2Device, iAP2USBDeviceModeAudioInformationParameter* theiAP2USBDeviceModeAudioInformationParameter, void* context)
{
    S32 rc = IAP2_OK;

    /* temporary fix for compiler warning */
    context = context;
    theiAP2USBDeviceModeAudioInformationParameter = theiAP2USBDeviceModeAudioInformationParameter;

    IAP2TESTDLTLOG(DLT_LOG_DEBUG, "iap2USBDeviceModeAudioInformation_CB called \n");
    printf("\t %p  iap2USBDeviceModeAudioInformation_CB called \n", iap2Device);
//
//    g_iap2TestDevice.testGstSampleRate = *(theiAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate);
//
//    g_iap2TestDevice.testGstState = IAP2_GSTREAMER_STATE_PLAYING;
//    sem_post(&g_iap2TestDevice.testGstSemaphore);
//    printf("Sending PLAY Notification to gstreamer\n");
//
//    g_iap2TestDevice.testState.testStateUSBDeviceModeAudioInformationReceived = TRUE;
    return rc;
}

void iap2InitStackCallbacks(iAP2StackCallbacks_t* iap2StackCb)
{
    iap2StackCb->p_iAP2DeviceState_cb = &iap2DeviceState_CB;
}

void iap2InitCSCallbacks(iAP2SessionCallbacks_t* iap2CSCallbacks, BOOL iAP2EAPSupported)
{
    iap2CSCallbacks->iAP2AssistiveTouchInformation_cb              = NULL;
    iap2CSCallbacks->iAP2AuthenticationFailed_cb                   = &iap2AuthenticationFailed_CB;
    iap2CSCallbacks->iAP2AuthenticationSucceeded_cb                = &iap2AuthenticationSucceeded_CB;
    iap2CSCallbacks->iAP2BluetoothConnectionUpdate_cb              = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationCertificate_cb        = NULL;
    iap2CSCallbacks->iAP2DeviceAuthenticationResponse_cb           = NULL;
    iap2CSCallbacks->iAP2DeviceHIDReport_cb                        = NULL;
    iap2CSCallbacks->iAP2IdentificationAccepted_cb                 = &iap2IdentificationAccepted_CB;
    iap2CSCallbacks->iAP2IdentificationRejected_cb                 = &iap2IdentificationRejected_CB;
    iap2CSCallbacks->iAP2MediaLibraryInformation_cb                = &iap2MediaLibraryInfo_CB;
    iap2CSCallbacks->iAP2MediaLibraryUpdate_cb                     = &iap2MediaLibraryUpdates_CB;
    iap2CSCallbacks->iAP2NowPlayingUpdateParameter_cb              = &iap2NowPlayingUpdate_CB;
    iap2CSCallbacks->iAP2PowerUpdate_cb                            = &iap2PowerUpdate_CB;
    iap2CSCallbacks->iAP2RequestAuthenticationCertificate_cb       = NULL;
    iap2CSCallbacks->iAP2RequestAuthenticationChallengeResponse_cb  = NULL;
    iap2CSCallbacks->iAP2StartIdentification_cb                    = NULL;
    iap2CSCallbacks->iAP2StartLocationInformation_cb               = NULL;
    iap2CSCallbacks->iAP2StopLocationInformation_cb                = NULL;
    iap2CSCallbacks->iAP2USBDeviceModeAudioInformation_cb          = &iap2USBDeviceModeAudioInformation_CB;
    iap2CSCallbacks->iAP2VoiceOverUpdate_cb                        = NULL;
    iap2CSCallbacks->iAP2WiFiInformation_cb                        = NULL;
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
LOCAL S32 iap2AddFDsToFDset(iAP2GetPollFDs_t* getPollFDs, S32* maxfd, fd_set* to_readfds, fd_set* to_writefds)
{
    S32 rc = 0;
    S32 i = 0;

    if((getPollFDs == NULL) || (to_readfds == NULL)
        || (to_writefds == NULL) || (maxfd == NULL))
    {
        rc = -1;
    }
    else
    {
        /* adds the file descriptors to the fd_set */
        for(i = 0; i < getPollFDs->numberFDs; i++)
        {
            /* find highest-numbered file descriptor */
            if(getPollFDs->fds[i].fd > *maxfd)
            {
                *maxfd = getPollFDs->fds[i].fd;
            }

            if(getPollFDs->fds[i].event == POLLIN)
            {
                /* FD_SET() adds the file descriptor to the fd_set */
                FD_SET(getPollFDs->fds[i].fd, to_readfds);
//                printf("  fd %d is set to read_fds, event %d \n",
//                        getPollFDs->fds[i].fd, getPollFDs->fds[i].event);
            }
            else if(getPollFDs->fds[i].event == POLLOUT)
            {
                /* FD_SET() adds the file descriptor to the fd_set */
                FD_SET(getPollFDs->fds[i].fd, to_writefds);
//                printf("  fd %d is set to write_fds, event %d \n",
//                        getPollFDs->fds[i].fd, getPollFDs->fds[i].event);
            }
            else
            {
                printf("  fd %d is used for unknown event %d \n",
                        getPollFDs->fds[i].fd, getPollFDs->fds[i].event);
            }
        }
        rc = 0;
    }

    return rc;
}
LOCAL void iap2AppThread(void* exInfo)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2GetPollFDs_t getPollFDs;
    S32 j = 0;
    S32 cntfds = 0;
    S32 nfds = 0;       /* highest-numbered file descriptor in any of the three sets, plus 1 */
    fd_set read_fds;    /* will be watched to see if characters become available for reading */
    fd_set write_fds;

    iAP2InitParam_t iAP2InitParameter;
    S32 ipod_count = (S32)*((S32*)exInfo);

    printf("iap2AppThread id %d : ipod No. %d : iPodID : %s \n",
            (S32)g_ipodlist[ipod_count].tid,
            ipod_count,
            g_ipodlist[ipod_count].devid);


    memset(&iAP2InitParameter, 0, sizeof(iAP2InitParam_t) );
    rc = iap2SetInitialParameter(&iAP2InitParameter, g_iap2UserConfig);
    printf(" ipod  %d tid %ld iap2SetInitialParameter:   rc  = %d \n",ipod_count,g_ipodlist[ipod_count].tid,rc);

    S32 len = (S32)strnlen(g_ipodlist[ipod_count].devid, DEV_DETECT_CFG_STRING_MAX);
    strncpy((char*)&iAP2InitParameter.iAP2DeviceId[0], g_ipodlist[ipod_count].devid, len+1);
    iAP2InitParameter.iAP2DeviceId[len] = '\0';

    printf("ipod  %d Device ID : %s   len = %d \n",ipod_count, iAP2InitParameter.iAP2DeviceId, len);

    g_ipodlist[ipod_count].device = iAP2InitDeviceStructure(&iAP2InitParameter);
    printf(" ipod  %d tid %ld  %d ms  iAP2InitDeviceStructure = %p \n", ipod_count,g_ipodlist[ipod_count].tid,iap2CurrTimeMs(), g_ipodlist[ipod_count].device);

    rc = iAP2InitDeviceConnection(g_ipodlist[ipod_count].device);
    printf("ipod  %d tid %ld %d ms  iAP2InitDeviceConnection:   rc  = %d \n",ipod_count,g_ipodlist[ipod_count].tid, iap2CurrTimeMs(), rc);


    /* Poll and Process */
    if(rc == IAP2_OK)
    {
        /* get file descriptors */
        rc = iAP2GetPollFDs(g_ipodlist[ipod_count].device, &getPollFDs);
        printf("\n iAP2GetPollFDs = %d \n", rc);

        cntfds = getPollFDs.numberFDs;
    }

    if(rc == IAP2_OK)
    {
        /* main loop */
        while (FALSE == g_ipodlist[ipod_count].leavePoll)
        {
            /* FD_ZERO() clears out the fd_set, so it doesn't contain any file descriptors */
            FD_ZERO(&read_fds);
            FD_ZERO(&write_fds);

            rc = iap2AddFDsToFDset(&getPollFDs, &nfds, &read_fds, &write_fds);
            if(rc != IAP2_OK)
            {
                printf(" iap2AddFDsToFDset = %d \n", rc);
            }

            rc = select(nfds+1, &read_fds, &write_fds, NULL, NULL);
            if(rc > 0)
            {
                for(j = 0; j < cntfds; j++)
                {
                    if( (j < getPollFDs.numberFDs) && (FD_ISSET(getPollFDs.fds[j].fd, &read_fds)) )
                    {
                        printf(" TID [%d] iPod #%d   fd[%d] %d (event: %d) is set \n",(U32)pthread_self(),ipod_count,
                                j, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);

                        rc = iAP2HandleEvent(g_ipodlist[ipod_count].device, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);
                    }
                    if( (j < getPollFDs.numberFDs) && (FD_ISSET(getPollFDs.fds[j].fd, &write_fds)) )
                    {
                        printf(" TID [%d] iPod #%d  fd[%d] %d (event: %d) is set \n",(U32)pthread_self(),ipod_count,
                                j, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);

                        rc = iAP2HandleEvent(g_ipodlist[ipod_count].device, getPollFDs.fds[j].fd, getPollFDs.fds[j].event);
                    }
                }
            }
            else
            {
                printf("  myPollFDs failed = %d \n", rc);

                if(rc < 0)
                {
                    //g_leaveWhile = TRUE;
                }
            }
        }/* while-select */
    }

    rc = iAP2DisconnectDevice(g_ipodlist[ipod_count].device);
    printf("ipod  %d tid %ld %d ms  iAP2DisconnectDevice:   rc  = %d \n",ipod_count,g_ipodlist[ipod_count].tid, iap2CurrTimeMs(), rc);


    /* de-initialize the device structure */
    rc = iAP2DeInitDeviceStructure(g_ipodlist[ipod_count].device);
    printf("ipod  %d tid %ld  %d ms  iAP2DeInitDeviceStructure:   rc  = %d \n",ipod_count,g_ipodlist[ipod_count].tid, iap2CurrTimeMs(), rc);
    /* de-initialize the iAP2InitParam_t structure */
    iap2ResetInitialParameter(&iAP2InitParameter);

    pthread_exit((void*)g_ipodlist[ipod_count].devid);
}

void iap2GetArguments(int argc, const char** argv)
{

    if(argc == 3)
    {
        if (0 == strncmp(argv[1], "host", 4))
        {
            g_iap2UserConfig.iAP2TransportType = iAP2USBHOSTMODE;
            printf("Host Mode selected. %s %s\n", argv[1], argv[2]);
            if (0 == strncmp(argv[2], "dipo", 4))
            {
                /* --- host mode with DiPO support--- */
                g_iap2UserConfig.iap2iOSintheCar = TRUE;
            }
            else
            {
                /* --- host mode without DiPO --- */
                g_iap2UserConfig.iap2iOSintheCar = FALSE;
            }
        }
    }
    else
    {
        if(argc == 2)
        {
            /*
             * Device mode with EAP Session Testing / iOS App Communication Testing
             * To test, provide the second argument as "eap", i.e., ipod-iap2_smoketest.out eap
             */
            if (strncmp(argv[1], "eap", 3) == 0)
            {
                g_EAPtesting = TRUE;
            }
            else
            {
                g_EAPtesting = FALSE;
            }
        }
        if (strncmp(argv[1], "bt", 2) == 0)
        {
            g_iap2UserConfig.iAP2TransportType = iAP2BLUETOOTH;
        }
        else
        {
            /* --- device mode --- */
            g_iap2UserConfig.iAP2TransportType = iAP2USBDEVICEMODE;
        }
        g_iap2UserConfig.iap2iOSintheCar   = FALSE;

        printf("Device Mode selected.\n");
    }
}
LOCAL S32 iAP2CheckForDevice(struct udev *udev)
{
    S32 ret = -1;

    struct udev_enumerate *enumerate;
    struct udev_list_entry *devices, *dev_list_entry;
    struct udev_device *dev;

    const char *idVendor = NULL;
    const char *idProduct = NULL;
    const char *serial = NULL;

    if(udev != NULL)
    {
        /* Create a list of the devices in the 'hidraw' subsystem. */
        enumerate = udev_enumerate_new(udev);
        if(enumerate == NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate is NULL");
            exit(1);
        }
        ret = udev_enumerate_add_match_subsystem(enumerate, (const char *)IPOD_USB_MONITOR_DEVTYPE);
        if(ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_add_match_subsystem failed: ret = %d", ret);
            exit(1);
        }
        /* search only for Apple devices */
        ret = udev_enumerate_add_match_sysattr(enumerate, IPOD_SYSATTR_IDVENDOR, IPOD_APPLE_IDVENDOR);
        if(ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_add_match_sysattr failed: ret = %d", ret);
            exit(1);
        }
        ret = udev_enumerate_scan_devices(enumerate);
        if(ret != 0)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "enumerate_scan_devices failed: ret = %d", ret);
            exit(1);
        }
        devices = udev_enumerate_get_list_entry(enumerate);
        if(devices == NULL)
        {
            IAP2TESTDLTLOG(DLT_LOG_ERROR, "devices is NULL (list_entry)");
            /* no specified Apple device available */
            ret = -1;
        }
        else
        {
            ret = -1;
            /* For each item enumerated, print out its information.
               udev_list_entry_foreach is a macro which expands to
               a loop. The loop will be executed for each member in
               devices, setting dev_list_entry to a list entry
               which contains the device's path in /sys. */
            udev_list_entry_foreach(dev_list_entry, devices)
            {
                const char *path;

                /* Get the filename of the /sys entry for the device
                   and create a udev_device object (dev) representing it */
                path = udev_list_entry_get_name(dev_list_entry);
                if(path == NULL)
                {
                    /* Get the path of device failed*/
                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "path of device failed");
                }
                else
                {
                    dev = udev_device_new_from_syspath(udev, path);
                    if(dev != NULL)
                    {
                        /* usb_device_get_devnode() returns the path to the device node
                           itself in /dev. */
//                         printf("Device Node Path: %s\n", udev_device_get_devnode(dev));

                        /* From here, we can call get_sysattr_value() for each file
                           in the device's /sys entry. The strings passed into these
                           functions (idProduct, idVendor, serial, etc.) correspond
                           directly to the files in the directory which represents
                           the USB device. Note that USB strings are Unicode, UCS2
                           encoded, but the strings returned from
                           udev_device_get_sysattr_value() are UTF-8 encoded. */
                        idVendor = udev_device_get_sysattr_value(dev,"idVendor");
                        idProduct = udev_device_get_sysattr_value(dev, "idProduct");
                        serial = udev_device_get_sysattr_value(dev, "serial");
//                        printf("VID: %s  PID: %s  SERIAL: %s \n", idVendor, idProduct, serial);

                        if( (idVendor != NULL) && (idProduct != NULL) && (serial != NULL) )
                        {
                            /* Vendor ID equal to Apple Vendor ID and Product ID equal to Apple Product ID */
                            if((strncmp((const char *)idVendor, (const char *)IPOD_APPLE_IDVENDOR, DEV_DETECT_VENDOR_MAX_LEN) == 0) &&
                               (strncmp((const char *)idProduct, (const char *)IPOD_APPLE_IDPRODUCT_MIN, 2) == 0))
                            {
                                ret = 0;

//                                strncpy((char*)(&g_serialNum[g_detectediPods][0]), serial, DEV_DETECT_CFG_STRING_MAX);
                                strncpy((char*)(&g_ipodlist[g_num_ipod].devid[0]), serial, DEV_DETECT_CFG_STRING_MAX);

                                printf(" VID: %s  PID: %s  SERIAL: %s \n", idVendor, idProduct, &g_ipodlist[g_num_ipod].devid[0]);
                                IAP2TESTDLTLOG(DLT_LOG_INFO, "Device details: VID: %s  PID: %s  SERIAL: %s", idVendor, idProduct, &g_ipodlist[g_num_ipod].devid[0]);
                                g_num_ipod++;

                            }
                        }
                        else
                        {
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "device_get_sysattr_value(idVendor or idProduct or serial) is NULL");
                        }

                        udev_device_unref(dev);
                    }
                    else
                    {
                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "device_new_from_syspath failed");
                    }
                }
            }
        }
        /* Free the enumerator object */
        udev_enumerate_unref(enumerate);
    }

    return ret;
}

LOCAL S32 iap2DetectiPods()
{
    S32 ret = -1;
    int retryCount = 0;

    struct udev *udev;
    struct udev_device *dev;
    struct udev_monitor *monitor;
    S32 udevfd = -1;
    const char *action = NULL;

    /* Create the udev object */
    udev = udev_new();
    if (!udev) {
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Can't create udev");
        exit(1);
    }
    else
    {
        /* check if iPod is already connected */
        ret = iAP2CheckForDevice(udev);
        /* no iPod available. wait for connecting iPod */
        if(ret != 0)
        {
            /* Link the monitor to "udev" devices */
            monitor = udev_monitor_new_from_netlink(udev, (const char *)IPOD_USB_MONITOR_LINK);
            if(monitor != NULL)
            {
                /* Filter only for get the "hiddev" devices */
                ret = udev_monitor_filter_add_match_subsystem_devtype(monitor, (const char *)IPOD_USB_MONITOR_DEVTYPE, IPOD_USB_FILTER_TYPE);
                if(ret == 0)
                {
                    /* Start receiving */
                    ret = udev_monitor_enable_receiving(monitor);
                    if(ret == 0)
                    {
                        /* Get the file descriptor for the monitor. This fd is used by select() */
                        udevfd = udev_monitor_get_fd(monitor);
                        ret = -1;
                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Please connect iPod!");
                        while((ret == -1) && (retryCount < IPOD_USB_SELECT_RETRY_CNT))
                        {
                            retryCount++;

                            fd_set fds;
                            struct timeval tv;

                            FD_ZERO(&fds);
                            FD_SET(udevfd, &fds);
                            tv.tv_sec = 5;
                            tv.tv_usec = 0;

                            ret = select(udevfd+1, &fds, NULL, NULL, &tv);
                            /* Check if our file descriptor has received data. */
                            if (ret > 0 && FD_ISSET(udevfd, &fds))
                            {
                                /* Make the call to receive the device.
                                   select() ensured that this will not block. */
                                dev = udev_monitor_receive_device(monitor);
                                if (dev)
                                {
//                                    printf("Got Device\n");
//                                    printf("   Node: %s\n", udev_device_get_devnode(dev));
//                                    printf("   Subsystem: %s\n", udev_device_get_subsystem(dev));
//                                    printf("   Devtype: %s\n", udev_device_get_devtype(dev));
                                    action = udev_device_get_action(dev);
                                    if(strncmp((const char *)action, IPOD_USB_ACTION_ADD, 4) == 0)
                                    {
                                        ret = 0;
                                        IAP2TESTDLTLOG(DLT_LOG_DEBUG, "Action is %s -> ret = %d", action, ret);
                                    }
                                    else
                                    {
                                        ret = -1;
                                        IAP2TESTDLTLOG(DLT_LOG_ERROR, "Action is %s -> ret = %d", action, ret);
                                    }

                                    udev_device_unref(dev);
                                }
                                else
                                {
                                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: No iPod ");
                                }
                            }
                            else if(ret < 0)
                            {
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: select() failed");
                            }
                            else
                            {
                                /* this occurs if an iPod is already connected */
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: No iPod found");
                                ret = -1;
                            }
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "retry %d", retryCount);
                        }

                        if((ret == -1) || (retryCount >= 5))
                        {
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: max retries %d | ret = %d",  retryCount, ret);
                        }
                        else /* IPOD_OK */
                        {
                            /* get Serial-Nr. VendorID and ProductID of newly connected iPod */
                            ret = iAP2CheckForDevice(udev);
                            /* no iPod available. wait for connecting iPod */
                            if(ret != 0)
                            {
                                IAP2TESTDLTLOG(DLT_LOG_ERROR, "Error: enumerate connected iPod failed | ret = %d", ret);
                            }
                        }
                    }
                }
            }
            else
            {
                IAP2TESTDLTLOG(DLT_LOG_ERROR, "monitor NULL");
                exit(1);
            }
        }
    }

    udev_unref(udev);

    return ret;
}

/* application thread */
LOCAL void iAP2BT_MainLoopThread(void* exinf)
{
    iAP2BTInit *iAP2BT = (iAP2BTInit*)exinf;

    g_main_loop_run(iAP2BT->gmain_loop);
}

int main(int argc, char *argv[])
{
    S32 rc = IAP2_OK, ret;
    U32 ipod_count = 0;
    char tsk_name[8];
    pthread_attr_t attr;
    void* status;

    /* LINT and Compiler warning */
    argc = argc;
    argv = argv;

    printf("\nStarting iPod Smoketest \n");
    g_num_ipod = 0;

    IAP2REGISTERAPPWITHDLT(DLT_IPOD_IAP2_APID, "iAP2 Logging");
    IAP2REGISTERCTXTWITHDLT();

    /**
     * get user configuration. iap2GetArguments() gets runtime user inputs and
     * stores in global g_iap2UserConfig Structure.
     */
    iap2GetArguments(argc, (const char **)argv);
    g_iap2UserConfig.iAP2AuthenticationType = iAP2AUTHI2C;

    if(g_iap2UserConfig.iAP2TransportType == iAP2BLUETOOTH)
    {
        iAP2BTInit iAP2BT;

        memset(&iAP2BT, 0, sizeof(iAP2BTInit) );

        ret = system("hciconfig hci0 up");
        if(ret < 0)
        {
            printf("system funtion returns ret=%d\n",ret);
        }

        char SEM_NAME[]= "iAP2BT";
        sem_t *iAP2OverBT;

        //create & initialize semaphore
        iAP2OverBT = sem_open(SEM_NAME, O_CREAT, 0644, 0);
        if(iAP2OverBT == SEM_FAILED)
        {
            printf("unable to create semaphore\n");
            sem_unlink(SEM_NAME);
            rc = -1;
        }

        if(rc == IAP2_OK)
        {
            rc = iAP2InitializeBTConnection(&iAP2BT);
        }

        if(rc == IAP2_OK)
        {
            TEST_THREAD_ID BTTaskID = 0;
            char threadName[8];

            /* set thread name */
            memset(&threadName[0], 0, (sizeof(threadName)));
            sprintf(&threadName[0], "%s%d", "iBTd", 1);

            /* -----------  Start BT - GMainLoop thread  ----------- */
            BTTaskID = iap2CreateThread(iAP2BT_MainLoopThread, &threadName[0], &iAP2BT);
            if(iap2VerifyThreadId(BTTaskID) != IAP2_OK)
            {
                rc = IAP2_CTL_ERROR;
                printf("  create thread %s failed %d \n", threadName, rc);
            }
            else
            {
#if 0
                U32 i;
                /* Identify Whether we already have devices with Apple CarPlay Support */
                for(i = 0; i < iAP2BT.DeviceProp_count; i++)
                {
                    if(iAP2BT.DeviceProperties[i].CarPlaySupported == TRUE)
                    {
                        rc = iAP2BTEstablishConnection(iAP2BT.DeviceProperties[i].Address);
                        sleep(2);
                    }
                }
#endif
                /* Scan for devices */
                rc = iAP2BTScanForDevices(&iAP2BT);

                if(rc == IAP2_OK)
                {
                    U32 i;

                    memset(&attr, 0, (sizeof(pthread_attr_t)));

                    rc = pthread_attr_init(&attr);
                    if(rc != IAP2_OK)
                        printf("attr_init failed with %d\n", rc);
                    rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                    if(rc != IAP2_OK)
                        printf("attr_setdetachstate failed with %d\n", rc);

                    while( (iAP2BT.CarPlayDeviceFound == FALSE) ||
                           (iAP2BT_RequestConfirmationStatus() == FALSE) )
                    {
                        sleep(1);
                    }

                    while(1)
                    {
                        sem_wait(iAP2OverBT);
                        printf("Got the semaphore\n");

                        for(i = 0; i < iAP2BT.DeviceProp_count; i++)
                        {
                            printf("i = %d\n", i);
                            printf("CarPlaySupported = %d\n", iAP2BT.DeviceProperties[i].CarPlaySupported);
                            printf("Connection       = %d\n", iAP2BT.DeviceProperties[i].Connection);
                            printf("Address          = %s\n", iAP2BT.DeviceProperties[i].Address);
                            if( (iAP2BT.DeviceProperties[i].CarPlaySupported == TRUE) &&
                                (iAP2BT.DeviceProperties[i].Connection == FALSE) )
                            {
                                iAP2BT.DeviceProperties[i].Connection = TRUE;
                                rc = iAP2BTEstablishConnection(iAP2BT.DeviceProperties[i].Address);
                                if(rc == IAP2_OK)
                                {
                                    U8 *ExtractedString = NULL;
                                    U8 LocalBTAdapterMAC_String[16] = {0};

                                    strcpy((char*)LocalBTAdapterMAC_String, (const char*)"0x");
                                    ExtractedString = (U8*)strtok(iAP2BT.AdapterProperties.Address, ":");
                                    while(ExtractedString != NULL)
                                    {
                                        strcpy((char*)&(LocalBTAdapterMAC_String[strlen((const char*)LocalBTAdapterMAC_String)]), (const char*)ExtractedString);
                                        ExtractedString = (U8*)strtok(NULL, ":");
                                    }
                                    g_iap2UserConfig.BTAdapterMAC = (U64)IAP2_ADHERE_TO_APPLE_ENDIANESS_64(strtoull((const char *)LocalBTAdapterMAC_String, NULL, 16));
                                    g_iap2UserConfig.BTAdapterMAC =  g_iap2UserConfig.BTAdapterMAC >> 16;

                                    strncpy(g_ipodlist[ipod_count].devid, iAP2BT.DeviceProperties[i].Address, DEV_DETECT_CFG_STRING_MAX);
                                }
                                sleep(5);

                                g_ipodlist[ipod_count].leavePoll = FALSE;
                                g_ipodlist[ipod_count].mediaLibInfoSent = FALSE;
                                g_ipodlist[ipod_count].playMediaItemSent = FALSE;

                                if(g_EAPtesting == TRUE)
                                {
                                    g_ipodlist[ipod_count].eapTest = TRUE;
                                }
                                else
                                {
                                    g_ipodlist[ipod_count].eapTest = FALSE;
                                }
                                g_ipodlist[ipod_count].tempMediaLibraryInfoLibID = NULL;

                                printf("ipod %d : ID : %s  \n", ipod_count, g_ipodlist[ipod_count].devid);

                                rc = pthread_create(&g_ipodlist[ipod_count].tid, &attr, (void *(*)(void *))iap2AppThread, (void*)&ipod_count);

                                if(rc == 0)
                                {
                                    /* set thread name */
                                    sprintf(&tsk_name[0], "%s%d", "iTest_", ipod_count);
                                    rc = pthread_setname_np(g_ipodlist[ipod_count].tid, (const char*)tsk_name);
                                    sleep(1);
                                    ipod_count++;
                                }
                            }
                        }
                        sleep(1);
                    }
                }
                pthread_join(BTTaskID, NULL);
            }
        }
    }
    else
    {
        rc =  iap2DetectiPods();

        printf("Number of iPod detected = %d  rc = %d \n", g_num_ipod, rc);
        /*Print the serial numbers*/
        for(ipod_count = 0; ipod_count < g_num_ipod; ipod_count++)
        {
            g_ipodlist[ipod_count].leavePoll = FALSE;
            g_ipodlist[ipod_count].mediaLibInfoSent = FALSE;
            g_ipodlist[ipod_count].playMediaItemSent = FALSE;

            if(g_EAPtesting == TRUE)
            {
                g_ipodlist[ipod_count].eapTest = TRUE;
            }
            else
            {
                g_ipodlist[ipod_count].eapTest = FALSE;
            }
            g_ipodlist[ipod_count].tempMediaLibraryInfoLibID = NULL;

            printf("ipod %d : ID : %s  \n", ipod_count, g_ipodlist[ipod_count].devid);
        }


        memset(&attr, 0, (sizeof(pthread_attr_t)));
    //    memset(iPods, 0 , sizeof(iPods));

        if(rc == IAP2_OK)
        {
            rc = pthread_attr_init(&attr);
            if(rc != IAP2_OK)
                printf("attr_init failed with %d\n", rc);
            rc = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
            if(rc != IAP2_OK)
                printf("attr_setdetachstate failed with %d\n", rc);

            /* create for each available iPod an own test thread */
            for(ipod_count = 0; ipod_count < g_num_ipod; ipod_count++)
            {
                if (rc == 0)
                {
                    printf("thread  %d created \n",ipod_count );
                    rc = pthread_create(&g_ipodlist[ipod_count].tid, &attr, (void *(*)(void *))iap2AppThread, (void*)&ipod_count);
                    if(rc == 0)
                    {
                        /* set thread name */
                        sprintf(&tsk_name[0], "%s%d", "iTest_", ipod_count);
                        rc = pthread_setname_np(g_ipodlist[ipod_count].tid, (const char*)tsk_name);
                    }
                }
            }


            (void)pthread_attr_destroy(&attr);
            U32 ipod_count2;
            for(ipod_count2 = 0; ipod_count2 < g_num_ipod; ipod_count2++)
            {
                if(g_ipodlist[ipod_count2].tid > 0)
                {
                    rc = pthread_join(g_ipodlist[ipod_count2].tid, &status);
                    printf("thread  %d joined \n",ipod_count2 );
                    printf("main() completed with %lu returns %d with ipodid %s\n", g_ipodlist[ipod_count2].tid, rc, (char*)status);
                }
            }
        }
    }

    g_exit_thread = FALSE;
    memset(g_ipodlist,0,sizeof(g_ipodlist));

    if(rc == IAP2_OK)
    {
        printf(" \n\n ***** iAP2 MULTI-IPOD SMOKETEST PASS ***** \n\n");
        IAP2TESTDLTLOG(DLT_LOG_INFO, "***** iAP2 MULTI-IPOD SMOKETEST PASS *****");


    }
    else
    {
        printf("\n\n ***** iAP2 MULTI-IPOD SMOKETEST FAIL ***** \n\n");
        IAP2TESTDLTLOG(DLT_LOG_ERROR, "***** iAP2 MULTI-IPOD SMOKETEST FAIL *****");
    }


    IAP2DEREGISTERCTXTWITHDLT();
    IAP2DEREGISTERAPPWITHDLT();

    return rc;
}
