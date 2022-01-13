#include "iPodPlayerIPCLib.h"
#include "iPodPlayerUtilityLog.h"
#include "ipp_iap2_observer.h"
#include "ipp_iap2_database.h"
#include "iap2_commands.h"
#include "ipp_iap2_common.h"
#include "ipp_iap2_hidcommon.h"
#include "ipp_iap2_eventnotification.h"
#include "ipp_iap2_init.h"
#include "ipp_iap2_devinit.h"
#include "ipp_iap2_powersourceupdate.h"

#include <endian.h>

static S32 ippiAP2StartDeviceModeAudio(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartUSBDeviceModeAudioParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the strucutre */
    memset(&param, 0, sizeof(param));
    
    rc = iAP2StartUSBDeviceModeAudio(iap2Device, &param);
    
    return rc;
    
}

static S32 ippiAP2StartNowPlayingUpdates(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartNowPlayingUpdatesParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set MediaItemAttributes properties which you want to receive */
    param.iAP2MediaItemAttributes = calloc(1, sizeof(*param.iAP2MediaItemAttributes));
    if(param.iAP2MediaItemAttributes != NULL)
    {
        param.iAP2MediaItemAttributes_count++;
        
        /* Set the media item attributes */
        param.iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscCount_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscNumber_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemAlbumTitle_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackCount_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackNumber_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemArtist_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemComposer_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemGenre_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemIsBanned_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemIsBanSupported_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemIsLiked_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemIsLikeSupported_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemPlaybackDurationInMilliseconds_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemTitle_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemChapterCount_count++;
        param.iAP2MediaItemAttributes->iAP2MediaItemPersistentIdentifier_count++;
        
        /* enable information about shuffle mode and repeat mode */
        param.iAP2PlaybackAttributes = calloc(1, sizeof(*param.iAP2PlaybackAttributes));
        if(param.iAP2PlaybackAttributes != NULL)
        {
            param.iAP2PlaybackAttributes_count++;
            
            /* Set the playback attributes */
            param.iAP2PlaybackAttributes->iAP2PlaybackAppName_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count++;
            param.iAP2PlaybackAttributes->iAP2PBAppleMusicRadioAd_count++;
            param.iAP2PlaybackAttributes->iAP2PBAppleMusicRadioStationName_count++;
            param.iAP2PlaybackAttributes->iAP2PBAppeMusicRadioStationMediaPlaylistPersistentID_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackQueueChapterIndex_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackQueueCount_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackQueueIndex_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackStatus_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackMediaLibraryUniqueIdentifier_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackAppBundleID_count++;
            param.iAP2PlaybackAttributes->iAP2SetElapsedTimeAvailable_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackQueueListAvail_count++;
            param.iAP2PlaybackAttributes->iAP2PlaybackQueueListTransferID_count++;
            
            /* Call StartNowPlayingUpdates */
            rc = iAP2StartNowPlayingUpdates(iap2Device, &param);
            if(rc == IAP2_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
                rc = ippiAP2RetConvertToiPP(rc);
            }
            
            free(param.iAP2PlaybackAttributes);
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
        
        free(param.iAP2MediaItemAttributes);
    }
    
    return rc;
}

static S32 ippiAP2StartPowerUpdate(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_OK;
    S32 ret = IPOD_PLAYER_OK;
    iAP2StartMediaLibraryInformationParameter param;
    iAP2StartPowerUpdatesParameter startPowerUpdatesPara;
    U8 DeviceBatteryWillChargeIfPowerIsPresent = 1;
    U8 MaximumcurrentDrawnFromAccessory = 1;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    memset(&startPowerUpdatesPara, 0, sizeof(startPowerUpdatesPara));

    /* iAP2AccessoryPowerMode must be present if and only if the accessory draws power from apple device. */
    ret = ippiAP2AllocateandUpdateData(&startPowerUpdatesPara.iAP2DeviceBatteryWillChargeIfPowerIsPresent,
                                   &DeviceBatteryWillChargeIfPowerIsPresent,
                                   &startPowerUpdatesPara.iAP2DeviceBatteryWillChargeIfPowerIsPresent_count,
                                   1, iAP2_uint8);
    if(ret == IPOD_PLAYER_OK)
    {
        ret = ippiAP2AllocateandUpdateData(&startPowerUpdatesPara.iAP2MaximumcurrentDrawnFromAccessory,
                                       &MaximumcurrentDrawnFromAccessory,
                                       &startPowerUpdatesPara.iAP2MaximumcurrentDrawnFromAccessory_count,
                                       1, iAP2_uint8);
    }

    if(ret == IPOD_PLAYER_OK)
    {
        startPowerUpdatesPara.iAP2IsExternalChargerConnected_count++;
        startPowerUpdatesPara.iAP2BatteryChargingState_count++;
        startPowerUpdatesPara.iAP2BatteryChargeLevel_count++;

        rc = iAP2StartPowerUpdates(iap2Device, &startPowerUpdatesPara);
        if(rc != IAP2_OK)
        {
            IPOD_DLT_ERROR("Start power updates failed!  rc = %d \n", rc);
        }

        ret = ippiAP2RetConvertToiPP(rc);
    }

    iAP2FreeiAP2StartPowerUpdatesParameter(&startPowerUpdatesPara);

    return ret;
}

S32 ippiAP2StopPowerUpdate(iAP2Device_t *iap2Device)
{
    int rc = IAP2_OK;
    S32 ret = IPOD_PLAYER_OK;
    iAP2StopPowerUpdatesParameter stopPowerUpdatesPara;

    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    memset(&stopPowerUpdatesPara, 0, sizeof(stopPowerUpdatesPara));

    rc = iAP2StopPowerUpdates(iap2Device, &stopPowerUpdatesPara);
    if(rc != IAP2_OK){
        IPOD_DLT_ERROR("Stop power updates failed!  rc = %d \n", rc);
    }

    ret = ippiAP2RetConvertToiPP(rc);
    
    return ret;
}

static S32 ippiAP2StartMediaLibraryInformation(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartMediaLibraryInformationParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    /* Call StartMediaLibraryInformation */
    rc = iAP2StartMediaLibraryInformation(iap2Device, &param);
    if(rc == IAP2_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
        rc = ippiAP2RetConvertToiPP(rc);
    }
    
    return rc;
}

static S32 ippiAP2StartMediaLibraryUpdate(iAP2Device_t *iap2Device, const U8 *uid, const U8 *revision)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 length = 0;
    iAP2StartMediaLibraryUpdatesParameter param;
    
    /* Parameter check */
    if((iap2Device == NULL) || (uid == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device, uid);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    length = strnlen((const char *)uid, IPOD_PLAYER_STRING_UID_LEN_MAX);
    
    /* Allocate memory for array of unique identifier. Array is always 1. */
    param.iAP2MediaLibraryUniqueIdentifier = calloc(1, sizeof(U8*));
    if(param.iAP2MediaLibraryUniqueIdentifier != NULL)
    {
        /* Allocate memory for one unique identifier */
        param.iAP2MediaLibraryUniqueIdentifier[0] = calloc(length + 1, sizeof(U8));
        if(param.iAP2MediaLibraryUniqueIdentifier[0] != NULL)
        {
            param.iAP2MediaLibraryUniqueIdentifier_count++;
            strncpy((char *)param.iAP2MediaLibraryUniqueIdentifier[0], (const char*)uid, length);
            param.iAP2MediaItemProperties = calloc(1, sizeof(*param.iAP2MediaItemProperties));
            if(param.iAP2MediaItemProperties != NULL)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        
        /* Revision will be set if revision is not NULL */
        if((rc == IPOD_PLAYER_OK) && (revision != NULL))
        {
            /* Allocate memory for array of revisions. Array is always 1 */
            param.iAP2LastKnownMediaLibraryRevision = calloc(1, sizeof(U8*));
            if(param.iAP2LastKnownMediaLibraryRevision != NULL)
            {
                param.iAP2LastKnownMediaLibraryRevision_count++;
                
                /* Allocate memory for one revision */
                param.iAP2LastKnownMediaLibraryRevision[0] = calloc(IPOD_PLAYER_MEDIA_ITEM_REVISION_LEN_MAX, sizeof(U8));
                if(param.iAP2LastKnownMediaLibraryRevision[0] != NULL)
                {
                    strncpy((char *)param.iAP2LastKnownMediaLibraryRevision[0], (const char *)revision, IPOD_PLAYER_MEDIA_ITEM_REVISION_LEN_MAX);
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                    rc = IPOD_PLAYER_ERR_NOMEM;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Set Media Item Properties */
            param.iAP2MediaItemProperties_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtist_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscCount_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscNumber_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumPersistentIdentifier_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTitle_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackCount_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackNumber_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyArtist_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyArtistPersistentIdentifier_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyComposer_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyComposerPersistentIdentifier_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyGenrePersistenIdentifier_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyIsPartOfCompilation_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyIsResidentOndevice_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyMediaType_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyRating_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count++;
            param.iAP2MediaItemProperties->iAP2MediaItemPropertyChapterCount_count++;
            
            param.iAP2MediaLibraryUpdateProgress_count++;
            param.iAP2MediaLibraryIsHidingRemoteItems_count++;
            
            /* Allocate memory for playlist properties */
            param.iAP2MediaPlaylistProperties = calloc(1, sizeof(*param.iAP2MediaPlaylistProperties));
            if(param.iAP2MediaPlaylistProperties != NULL)
            {
                /* Set playlist properties */
                param.iAP2MediaPlaylistProperties_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListContainedMediaItemsFileTransferIdentifier_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsAppleMusicRadioStation_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsFolder_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyName_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyParentPersistentIdentifier_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPersistentIdentifier_count++;
                param.iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count++;

                /* Call StartMediaLibraryUpdates */
                rc = iAP2StartMediaLibraryUpdates(iap2Device, &param);
                if(rc == IAP2_OK)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
                    rc = ippiAP2RetConvertToiPP(rc);
                }
                
                /* Free memory for playlist properties */
                free(param.iAP2MediaPlaylistProperties);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
            
            /* Free memory for media item properties */
            free(param.iAP2MediaItemProperties);
        }
        
        if(param.iAP2LastKnownMediaLibraryRevision != NULL)
        {
            if(param.iAP2LastKnownMediaLibraryRevision[0] != NULL)
            {
                free(param.iAP2LastKnownMediaLibraryRevision[0]);
            }
            free(param.iAP2LastKnownMediaLibraryRevision);
        }
        
        if(param.iAP2MediaLibraryUniqueIdentifier[0] != NULL)
        {
            free(param.iAP2MediaLibraryUniqueIdentifier[0]);
        }
        
        /* Free memory for media library unique identifier */
        free(param.iAP2MediaLibraryUniqueIdentifier);
    }
    
    return rc;
}

S32 ippiAP2StartAssistiveTouch(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartAssistiveTouchParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    /* Call StartAssistiveTouch */
    rc = iAP2StartAssistiveTouch(iap2Device, &param);
    if(rc == IAP2_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
        rc = ippiAP2RetConvertToiPP(rc);
    }
    
    return rc;
}

static S32 ippiAP2StartAssistiveTouchInformation(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StartAssistiveTouchInformationParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    /* Call StartAssistiveTouch */
    rc = iAP2StartAssistiveTouchInformation(iap2Device, &param);
    if(rc == IAP2_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
        rc = ippiAP2RetConvertToiPP(rc);
    }
    
    return rc;
}

S32 ippiAP2StopAssistiveTouchInformation(iAP2Device_t *iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StopAssistiveTouchInformationParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    /* Call StartAssistiveTouch */
    rc = iAP2StopAssistiveTouchInformation(iap2Device, &param);
    if(rc == IAP2_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
        rc = ippiAP2RetConvertToiPP(rc);
    }
    
    return rc;
}


S32 ippiAP2BluetoothComponentInformation(iAP2Device_t* iap2Device, U32 num)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 i = 0;
    iAP2BluetoothComponentInformationParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    if(num > 0)
    {
        /* Allocate memory for StartBluetoothConnectionUpdatesParameter */
        param.iAP2BluetoothComponentStatus = calloc(num, sizeof(*param.iAP2BluetoothComponentStatus));
        if(param.iAP2BluetoothComponentStatus != NULL)
        {
            rc = IPOD_PLAYER_OK;
            for(i = 0; i < num; i++)
            {
                param.iAP2BluetoothComponentStatus[i].iAP2BTComponentEnabled = calloc(1, sizeof(*param.iAP2BluetoothComponentStatus->iAP2BTComponentEnabled));
                param.iAP2BluetoothComponentStatus[i].iAP2BTComponentIdentifier = calloc(1, sizeof(*param.iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier));
                if((param.iAP2BluetoothComponentStatus[i].iAP2BTComponentEnabled != NULL) && (param.iAP2BluetoothComponentStatus[i].iAP2BTComponentIdentifier != NULL))
                {
                    /* Set bluetooth transport component Identifier */
                    param.iAP2BluetoothComponentStatus[i].iAP2BTComponentIdentifier[0] = i;
                    param.iAP2BluetoothComponentStatus[i].iAP2BTComponentIdentifier_count++;
                    param.iAP2BluetoothComponentStatus[i].iAP2BTComponentEnabled[0] = 1;
                    param.iAP2BluetoothComponentStatus[i].iAP2BTComponentEnabled_count++;
                    param.iAP2BluetoothComponentStatus_count++;
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_NOMEM;
                    break;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Call StartBluetoothConnectionUpdates */
            rc = iAP2BluetoothComponentInformation(iap2Device, &param);
            if(rc == IAP2_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
                rc = ippiAP2RetConvertToiPP(rc);
            }
        }
        
        /* Free parameter memory */
        if(param.iAP2BluetoothComponentStatus != NULL)
        {
            if(param.iAP2BluetoothComponentStatus->iAP2BTComponentEnabled != NULL)
            {
                free(param.iAP2BluetoothComponentStatus->iAP2BTComponentEnabled);
                param.iAP2BluetoothComponentStatus->iAP2BTComponentEnabled = NULL;
            }
            if(param.iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier != NULL)
            {
                free(param.iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier);
                param.iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier = NULL;
            }
            
            free(param.iAP2BluetoothComponentStatus);
            param.iAP2BluetoothComponentStatus = NULL;
        }
    }
    else
    {
        rc = IPOD_PLAYER_OK;
    }
    
    return rc;
}


static S32 ippiAP2StartBluetoothConnectionUpdates(iAP2Device_t* iap2Device, U32 num)
{
    S32 rc = IAP2_CTL_ERROR;
    U32 i = 0;
    iAP2StartBluetoothConnectionUpdatesParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    /* Allocate memory for StartBluetoothConnectionUpdatesParameter */
    param.iAP2BluetoothTransportComponentIdentifier = calloc(num, sizeof(*param.iAP2BluetoothTransportComponentIdentifier));
    if(param.iAP2BluetoothTransportComponentIdentifier != NULL)
    {
        for(i = 0; i < num; i++)
        {
            /* Set bluetooth transport component Identifier */
            param.iAP2BluetoothTransportComponentIdentifier[i] = i;
            param.iAP2BluetoothTransportComponentIdentifier_count++;
        }
        /* Call StartBluetoothConnectionUpdates */
        rc = iAP2StartBluetoothConnectionUpdates(iap2Device, &param);
        if(rc == IAP2_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
            rc = ippiAP2RetConvertToiPP(rc);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    /* Free parameter memory */
    if(param.iAP2BluetoothTransportComponentIdentifier != NULL)
    {
        free(param.iAP2BluetoothTransportComponentIdentifier);
        param.iAP2BluetoothTransportComponentIdentifier = NULL;
    }
    return rc;
}


S32 ippiAP2StopBluetoothConnectionUpdates(iAP2Device_t* iap2Device)
{
    S32 rc = IAP2_CTL_ERROR;
    iAP2StopBluetoothConnectionUpdatesParameter param;
    
    /* Parameter check */
    if(iap2Device == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure  */
    memset(&param, 0, sizeof(param));
    
    /* Call StartAssistiveTouch */
    rc = iAP2StopBluetoothConnectionUpdates(iap2Device, &param);
    if(rc == IAP2_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iap2Device);
        rc = ippiAP2RetConvertToiPP(rc);
    }
    
    return rc;
}

S32 iPodCoreObserverStartSessionByAcc(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    size_t i = 0;
    S32 rc = IPOD_PLAYER_OK;
    U16 sessionTable[] = 
    {
        IAP2_MSG_ID_START_HID,
        IAP2_MSG_ID_START_NOW_PLAYING_UPDATES,
        IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION,
        IAP2_MSG_ID_POWER_SOURCE_UPDATE,
        IAP2_MSG_ID_START_POWER_UPDATES,
        // IAP2_MSG_ID_START_USB_DEVICE_MODE_AUDIO,
        IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION,
        IAP2_MSG_ID_START_BLUETOOTH_CONNECTION_UPDATES,
        IAP2_MSG_ID_START_ASSISTIVE_TOUCH_INFORMATION
    };
    U32 num = 0;

    for(i = 0; i < sizeof(sessionTable)/sizeof(sessionTable[0]); i++)
    { 
        /* check support message ID */
        if(ippiAP2CheckIdentificationTableByAcc(sessionTable[i], iPodCtrlCfg->threadInfo))
        {
            switch(sessionTable[i])
            {
                case IAP2_MSG_ID_START_HID:
                    /* start hid */
                    rc = ippiAP2StartHID_init(iPodCtrlCfg->iap2Param.device);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start hid error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_START_NOW_PLAYING_UPDATES:
                    /* set mask clear */
                    iPodCtrlCfg->iap2Param.playbackStatusSetMask = 0;
                    /* start now playing update */
                    rc = ippiAP2StartNowPlayingUpdates(iPodCtrlCfg->iap2Param.device);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start now playing update error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION:
                    /* start media library information */
                    rc = ippiAP2StartMediaLibraryInformation(iPodCtrlCfg->iap2Param.device);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start media library information error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_POWER_SOURCE_UPDATE:
                    /* start power updates */
                    rc = powerSourceUpdate( iPodCtrlCfg->iap2Param.device, 
                                            iPodCtrlCfg->iap2Param.powerInfo.powermA,
                                            iPodCtrlCfg->iap2Param.powerInfo.chargeButtery);

                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start power update error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_START_POWER_UPDATES:
                    /* clear power update event status */
                    iPodCtrlCfg->iap2Param.powerUpdateEventStatus = 0;
                    rc = ippiAP2StartPowerUpdate(iPodCtrlCfg->iap2Param.device);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start power update error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_START_USB_DEVICE_MODE_AUDIO:
                    /* start USB device mode audio */
                    rc = ippiAP2StartDeviceModeAudio(iPodCtrlCfg->iap2Param.device);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start USB device mode audio error rc = %d", rc);   
                    }
                    break;
                    
                case IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION:
                    /* Get bluetooth number */
                    if(iPodCtrlCfg->threadInfo->btInfo.macCount == 0)
                    {
                        num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC_COUNT);
                    }
                    else
                    {
                        num = iPodCtrlCfg->threadInfo->btInfo.macCount;
                    }
                    /* start Bluetooth component information */
                    rc = ippiAP2BluetoothComponentInformation(iPodCtrlCfg->iap2Param.device, num);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start Bluetooth component iPodCoreObserverStartSessionByAccnformation error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_START_BLUETOOTH_CONNECTION_UPDATES:
                    /* Get bluetooth number */
                    if(iPodCtrlCfg->threadInfo->btInfo.macCount == 0)
                    {
                        num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC_COUNT);
                    }
                    else
                    {
                        num = iPodCtrlCfg->threadInfo->btInfo.macCount;
                    }
                    /* start Bluetooth connection updates */
                    rc = ippiAP2StartBluetoothConnectionUpdates(iPodCtrlCfg->iap2Param.device, num);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start Bluetooth connection updates error rc = %d", rc);   
                    }
                    break;

                case IAP2_MSG_ID_START_ASSISTIVE_TOUCH_INFORMATION:
                    /* start assistive touch information */
                    rc = ippiAP2StartAssistiveTouchInformation(iPodCtrlCfg->iap2Param.device);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("start assistive touch information error rc = %d", rc);   
                    }
                    break;

                default:
                    break;
            }

            if(rc != IPOD_PLAYER_OK)
            {
                break;  /* leave for loop */
            }
        }
    }

    return rc;
    
}

static S32 iPodCoreObserverCheckConnectionStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS connectionStatus;
    U32 size = sizeof(connectionStatus);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&connectionStatus, 0, sizeof(connectionStatus));
    /* Set Device type */
    iPodCtrlCfg->deviceConnection.deviceType = iPodCtrlCfg->threadInfo->nameInfo.devType;
    
    if(iPodCtrlCfg->deviceStatusChange != 0)
    {
        iPodCtrlCfg->deviceStatusChange = 0;
        if(iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_IAP1_RESTART)
        {
            /* Notify when iAP1 is not supported */
            if(iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_SUPPORT_IAP1) == 0)
            {
                /* Set the notify connection command */
                connectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
                connectionStatus.header.devID  = iPodCtrlCfg->threadInfo->appDevID;
                memcpy(&connectionStatus.status, &iPodCtrlCfg->deviceConnection, sizeof(connectionStatus.status));
                connectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_FAIL;

                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&connectionStatus, size, 0, IPODCORE_TMOUT_FOREVER);
                /* iPodPlayerIPCSend error log is output on the function side. */
            }

            return IPOD_PLAYER_ERR_IAP1_DETECTED;
        }

        /* Set the notify connection command */
        connectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
        connectionStatus.header.devID  = iPodCtrlCfg->threadInfo->appDevID;
        memcpy(&connectionStatus.status, &iPodCtrlCfg->deviceConnection, sizeof(connectionStatus.status));
        
        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&connectionStatus, size, 0, IPODCORE_TMOUT_FOREVER);
        if(rc == (S32)size)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_DLT_ERROR("Could not send IPC about notification connection status.");   
            rc = IPOD_PLAYER_ERROR;
        }
        
        if(iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS)
        {
            if(iPodCtrlCfg->StartSession == FALSE)
            {
                rc = iPodCoreObserverStartSessionByAcc(iPodCtrlCfg); /* Is session ID applicable ? */
                if(rc == IPOD_PLAYER_OK)
                {
                    iPodCtrlCfg->StartSession = TRUE;
                }
            }
        }
    }
    
    return rc;
}

static S32 iPodCoreObserverCheckMediaLibraryInformation(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_CORE_IAP2_DB_STATUS *dbStatus)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_IAP2_DB_MEDIAINFO mediaInfo;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (dbHandleOfMediaLibrary == NULL) || (dbStatus == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, dbHandleOfMediaLibrary, dbStatus);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&mediaInfo, 0, sizeof(mediaInfo));

    if(dbStatus->mediaChange && !dbStatus->startMediaLib)
    {
        rc = ippiAP2DBGetMediaLibraryInformation(dbHandleOfMediaLibrary, iPodCtrlCfg->threadInfo->nameInfo.deviceName, &mediaInfo);
        if(mediaInfo.mediaRevision[0] == '\0')
        {
            IPOD_DLT_INFO("StartMediaLibraryUpdates with revision null. :%s", mediaInfo.mediaID);
            rc = ippiAP2StartMediaLibraryUpdate(iPodCtrlCfg->iap2Param.device, mediaInfo.mediaID, NULL);
        }
        else
        {
            IPOD_DLT_INFO("StartMediaLibraryUpdates with revision saved in file. :%s, revision=%s", mediaInfo.mediaID, mediaInfo.mediaRevision);
            rc = ippiAP2StartMediaLibraryUpdate(iPodCtrlCfg->iap2Param.device, mediaInfo.mediaID, mediaInfo.mediaRevision);
        }
        /* enable startMediaLib flag */
        dbStatus->startMediaLib = TRUE;
    }
    dbStatus->mediaChange = FALSE;

    return rc;
}

S32 iPodCoreObserverGetFileXferLocalDevicePlaylistCount(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list)
{
    S32 count = 0;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;

    if(list == NULL)
    {
        IPOD_DLT_ERROR("Parameter error. :list=%p", list);
        return count;
    }

    fileList = *list;
    for(; fileList != NULL; fileList = fileList->next)
    {
        if(fileList->info.type == IPOD_PLAYER_XFER_TYPE_PLAYLIST_LOCAL)
        {
            count++;
        }
    }
    if(count > 0)
    {
        IPOD_DLT_VERBOSE("count=%d", count);
    }

    return count;
}

S32 iPodCoreObserverGetFileXferAppleMusicRadioPlaylistCount(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list)
{
    S32 count = 0;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;

    if(list == NULL)
    {
        IPOD_DLT_ERROR("Parameter error. :list=%p", list);
        return count;
    }

    fileList = *list;
    for(; fileList != NULL; fileList = fileList->next)
    {
        if(fileList->info.type == IPOD_PLAYER_XFER_TYPE_PLAYLIST_AMR)
        {
            count++;
        }
    }
    if(count > 0)
    {
        IPOD_DLT_VERBOSE("count=%d", count);
    }

    return count;
}

static S32 iPodCoreObserverCheckFileXfer(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                                         IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST *fileList = NULL;
    U64 *trackId = NULL;
    U32 count = 0;
    U32 i = 0;
    U8 fileIDs[IPP_IAP2_FILE_ID_MAX] = {0};
    IPOD_PLAYER_IAP2_DB_IDLIST  idList;
    U8 last_coverart_fileid = 0xff;
    U8 notifyCoverArt = 0;
    
    if(iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    fileList = *list;
    
    for(; fileList != NULL; fileList = fileList->next)
    {
        //IPOD_DLT_INFO("[DBG]fileID=%u, type=%d, status=%d, toltalSize=%llu", fileList->info.fileID, fileList->info.type, fileList->info.status, fileList->info.totalSize);

        if(fileList->info.status == IPOD_PLAYER_XFER_STATUS_DONE)
        {
            switch(fileList->info.type)
            {
            case IPOD_PLAYER_XFER_TYPE_COVERART:
                if(fileList->info.totalSize == 0)
                {
                    /* Notify coverart data */
                    rc = iPodCoreObserverCheckIsNotifyDataMaskSet(iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE_TRACKINFO, IPOD_PLAYER_TRACK_INFO_MASK_COVERART);
                    if((rc == IPOD_PLAYER_OK) && (((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyCoverArtData != NULL))
                    {
                        rc = ((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyCoverArtData(iPodCtrlCfg, 0, NULL);
                    }
                    IPOD_DLT_INFO("Notify size zero coverart :fileID=%u", fileList->info.fileID);
                }
                else
                {
                    /* save fileID of recv coverart data */
                    last_coverart_fileid = fileList->info.fileID;
                    notifyCoverArt = 1;

                    /* Notify coverart data */
                    rc = iPodCoreObserverCheckIsNotifyDataMaskSet(iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE_TRACKINFO, IPOD_PLAYER_TRACK_INFO_MASK_COVERART);
                    if((rc == IPOD_PLAYER_OK) && (((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyCoverArtData != NULL))
                    {
                        rc = ((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyCoverArtData(iPodCtrlCfg, fileList->info.totalSize, fileList->info.buf);
                        notifyCoverArt = 0;
                        IPOD_DLT_INFO("Notify coverart data reception :fileID=%u, toltalSize=%llu", fileList->info.fileID, fileList->info.totalSize);
                    }
                }
                break;

            case IPOD_PLAYER_XFER_TYPE_PLAYLIST_LOCAL:
                if(fileList->info.totalSize == 0)
                {
                    IPOD_DLT_INFO("Received empty playlist for Local Device media library :fileID=%u, toltalSize=%llu", fileList->info.fileID, fileList->info.totalSize);
                }
                else
                {
                    /* Set Playlist for Local Device media library */
                    count = fileList->info.totalSize / sizeof(U64);
                    trackId = calloc(count, sizeof(*trackId));
                    if(trackId != NULL)
                    {
                        for(i = 0; i < count; i++)
                        {
                            trackId[i] = be64toh(((U64 *)(void *)fileList->info.buf)[i]);
                        }

                        ippiAP2DBSetPlaylistTracks(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, fileList->info.targetID, count, trackId);
                        free(trackId);
                        /* update file DB */
                        iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbUpdatePlaylist = TRUE;
                        IPOD_DLT_INFO("Received playlist for Local Device media library :fileID=%u, toltalSize=%llu", fileList->info.fileID, fileList->info.totalSize);
                    }
                    else
                    {
                        IPOD_DLT_ERROR("trackId is null.");
                    }
                }
                break;

            case IPOD_PLAYER_XFER_TYPE_PLAYLIST_AMR:
                if(fileList->info.totalSize == 0)
                {
                    IPOD_DLT_INFO("Received empty playlist for Apple Music Radio media library :fileID=%u, toltalSize=%llu", fileList->info.fileID, fileList->info.totalSize);
                }
                else
                {
                    /* Set Playlist for Apple Music radio media library */
                    count = fileList->info.totalSize / sizeof(U64);
                    trackId = calloc(count, sizeof(*trackId));
                    if(trackId != NULL)
                    {
                        for(i = 0; i < count; i++)
                        {
                            trackId[i] = be64toh(((U64 *)(void *)fileList->info.buf)[i]);
                        }

                        ippiAP2DBSetPlaylistTracks(&iPodCtrlCfg->iap2Param.dbHandle->appleMusicRadio, fileList->info.targetID, count, trackId);
                        free(trackId);
                        /* update file DB */
                        iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbUpdatePlaylist = TRUE;
                        IPOD_DLT_INFO("Received playlist for Apple Music Radio media library :fileID=%u, toltalSize=%llu", fileList->info.fileID, fileList->info.totalSize);
                    }
                    else
                    {
                        IPOD_DLT_ERROR("trackId is null.");
                    }
                }
                break;

            case IPOD_PLAYER_XFER_TYPE_QUEUELIST:
                /* Set Queuelist */
                count = fileList->info.totalSize / sizeof(U64);
                trackId = calloc(count, sizeof(*trackId));
                if(trackId != NULL)
                {
                    for(i = 0; i < count; i++)
                    {
                        trackId[i] = be64toh(((U64 *)(void *)fileList->info.buf)[i]);
                    }
                    
                    idList.mediaId = trackId;
                    idList.count = count;
                    ippiAP2DBSetNowPlayingItemID(iPodCtrlCfg->iap2Param.dbHandle, &idList);
                    free(trackId);
                    /* update playback list event mask update */
                    iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_UPDATE_PBLIST, 0);
                    IPOD_DLT_INFO("Notify playback queue list update :fileID=%u, toltalSize=%llu", fileList->info.fileID, fileList->info.totalSize);
                }
                break;

            default:
                break;
            }
        }
    }
    
    fileList = *list;
    count = 0;
    for(; fileList != NULL; fileList = fileList->next)
    {
        if(((fileList->info.status == IPOD_PLAYER_XFER_STATUS_DONE) && (fileList->info.type != IPOD_PLAYER_XFER_TYPE_UNKNOWN)) ||
           (fileList->info.status == IPOD_PLAYER_XFER_STATUS_FAIL) ||
           (fileList->info.status == IPOD_PLAYER_XFER_STATUS_CANCEL))
        {
            /* check last recv coverart data */
            if( last_coverart_fileid == fileList->info.fileID )
            {
                if ( notifyCoverArt == 0 )
                {
                    fileIDs[count] = fileList->info.fileID;
                    count++;
                }
                else
                {
                    /* keep coverart data */
                    continue;
                }
            }
            else
            {
                fileIDs[count] = fileList->info.fileID;
                count++;
            }
        }
    }
    
    for(i = 0; i < count; i++)
    {
        ippiAP2FileXferDeinit(list, fileIDs[i]);
    }
    
    return rc;
}

static S32 iPodCoreObserverCheckSamplerate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_PAI_SET_SAMPLE paiSetSample;
    U32 rate = 0;
    
    /* Check Parameter */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&paiSetSample, 0, sizeof(paiSetSample));
    
    /* Check Sample */
    rc = ippiAP2DBGetSample(iPodCtrlCfg->iap2Param.dbHandle, &rate);
    if(rc == IPOD_PLAYER_OK)
    {
        if((iPodCtrlCfg->iPodInfo->sampleRate != rate) && (rate != 0))
        {
            iPodCtrlCfg->iPodInfo->sampleRate = rate;
            if(iPodCtrlCfg->startAudio != 0)
            {
                paiSetSample.header.funcId = IPOD_FUNC_PAI_SETSR;
                paiSetSample.header.devID = iPodCtrlCfg->threadInfo->appDevID;
                paiSetSample.sampleRate = iPodCtrlCfg->iPodInfo->sampleRate;
                
                rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, (U8 *)&paiSetSample, sizeof(paiSetSample), 0, IPODCORE_TMOUT_FOREVER);
                if(rc >= 0)
                {
                    rc = IPOD_PLAYER_OK;
                }
                
            }
        }

        if(iPodCtrlCfg->iap2Param.sampleRateStatus)
        {
            iPodCtrlCfg->iap2Param.sampleRate = rate;
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_SAMPLE_RATE, 0);
            iPodCtrlCfg->iap2Param.sampleRateStatus = FALSE;
        }

    }
    
    return rc;
}


/* Set notify track info data mask */
S32 iPodCoreObserverSetTrackInfoNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, TrackInfoMask_t *dataMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if((iPodCtrlCfg == NULL) || (dataMask == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set data mask */
    iPodCtrlCfg->iap2Param.notifyTrackInfoMask.mask = dataMask->mask;
    iPodCtrlCfg->iap2Param.notifyTrackInfoMask.formatId = dataMask->formatId;
    
    rc = IPOD_PLAYER_OK;
    
    return rc;
}


/* Set notify device event data mask */
S32 iPodCoreObserverSetDeviceEventNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if(iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set data mask */
    iPodCtrlCfg->iap2Param.notifyDeviceEventMask |= dataMask;
    rc = IPOD_PLAYER_OK;
    return rc;
}


/* Get notify device event data mask */
U32 iPodCoreObserverGetDeviceEventNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 retMask = 0;
    
    if(iPodCtrlCfg == NULL)
    {
        return retMask;
    }
    
    /* Get data mask */
    retMask = iPodCtrlCfg->iap2Param.notifyDeviceEventMask;
    return retMask;
}

/* Set update data mask */
void iPodCoreObserverSetUpdateDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask, U32 subMask, U32 extMask)
{
    if(iPodCtrlCfg == NULL)
    {
        return;
    }
    
    /* Set main data mask */
    iPodCtrlCfg->iap2Param.updateDataMask.dataMask |= dataMask;
    
    /* Set data mask */
    if(dataMask == IPP_IAP2_DATA_MASK_DEVICE_EVENT)
    {
        iPodCtrlCfg->iap2Param.updateDataMask.deviceEventMask |= subMask;
        if(IPP_IAP2_IS_MASK_SET(subMask, IPOD_PLAYER_EVENT_MASK_BT))
        {
            iPodCtrlCfg->iap2Param.updateDataMask.blueToothIDMask |= extMask;
        }
    }
    else if(dataMask == IPP_IAP2_DATA_MASK_TRACK_INFO)
    {
        iPodCtrlCfg->iap2Param.updateDataMask.trackInfoMask.mask |= subMask;
    }
    else
    {
        /* do nothing */
    }
    
    return;
}


/* Clear update data mask */
void iPodCoreObserverClearUpdateDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask, U32 subMask)
{
    if(iPodCtrlCfg == NULL)
    {
        return;
    }
    
    /* Clear main data mask */
    iPodCtrlCfg->iap2Param.updateDataMask.dataMask &= ~dataMask;
    
    /* Clear data mask */
    if(dataMask == IPP_IAP2_DATA_MASK_DEVICE_EVENT)
    {
        iPodCtrlCfg->iap2Param.updateDataMask.deviceEventMask &= ~subMask;
        if(IPP_IAP2_IS_MASK_SET(subMask, IPOD_PLAYER_EVENT_MASK_BT))
        {
            iPodCtrlCfg->iap2Param.updateDataMask.blueToothIDMask = 0;
        }
    }
    else if(dataMask == IPP_IAP2_DATA_MASK_TRACK_INFO)
    {
        iPodCtrlCfg->iap2Param.updateDataMask.trackInfoMask.mask &= ~subMask;
    }
    else
    {
        /* do nothing */
    }
    
    return;
}


/* Check whether update data mask is set or not */
S32 iPodCoreObserverCheckIsUpdateDataMaskSet(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask, U32 subMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if(iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Check main data mask */
    if(IPP_IAP2_IS_MASK_SET(iPodCtrlCfg->iap2Param.updateDataMask.dataMask, dataMask))
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Check data mask */
    if(dataMask == IPP_IAP2_DATA_MASK_DEVICE_EVENT)
    {
        if(IPP_IAP2_IS_MASK_SET(iPodCtrlCfg->iap2Param.updateDataMask.deviceEventMask, subMask))
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else if(dataMask == IPP_IAP2_DATA_MASK_TRACK_INFO)
    {
        if(IPP_IAP2_IS_MASK_SET(iPodCtrlCfg->iap2Param.updateDataMask.trackInfoMask.mask, subMask))
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        /* do nothing */
    }
    
    return rc;
}


/* Set update data mask */
S32 iPodCoreObserverCheckIsNotifyDataMaskSet(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE type, U32 dataMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if(iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Check data mask */
    if(type == IPP_IAP2_NOTIFY_TYPE_DEVEVENT)
    {
        if(IPP_IAP2_IS_MASK_SET(iPodCtrlCfg->iap2Param.notifyDeviceEventMask, dataMask))
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else if(type == IPP_IAP2_NOTIFY_TYPE_TRACKINFO)
    {
        if(IPP_IAP2_IS_MASK_SET(iPodCtrlCfg->iap2Param.notifyTrackInfoMask.mask, dataMask))
        {
            rc = IPOD_PLAYER_OK;
            if(IPP_IAP2_IS_MASK_SET(dataMask, IPOD_PLAYER_TRACK_INFO_MASK_COVERART) && (iPodCtrlCfg->iap2Param.notifyTrackInfoMask.formatId != IPP_IAP2_COVERART_FORMAT))
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
        /* do nothing */
    }
    
    return rc;
}


/*  Check whether notify data mask is set or not */
U32 iPodCoreObserverGetNeedToNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE type)
{
    S32 retMask = 0;
    
    
    if(iPodCtrlCfg == NULL)
    {
        return retMask;
    }
    
    
    /* Check data type */
    if(type == IPP_IAP2_NOTIFY_TYPE_DEVEVENT)
    {
        retMask = iPodCtrlCfg->iap2Param.notifyDeviceEventMask & iPodCtrlCfg->iap2Param.updateDataMask.deviceEventMask;
    }
    else if(type == IPP_IAP2_NOTIFY_TYPE_TRACKINFO)
    {
        retMask = iPodCtrlCfg->iap2Param.notifyTrackInfoMask.mask & iPodCtrlCfg->iap2Param.updateDataMask.trackInfoMask.mask;
        if(IPP_IAP2_IS_MASK_SET(retMask, IPOD_PLAYER_TRACK_INFO_MASK_COVERART) && (iPodCtrlCfg->iap2Param.notifyTrackInfoMask.formatId != IPP_IAP2_COVERART_FORMAT))
        {
            retMask &= ~IPOD_PLAYER_TRACK_INFO_MASK_COVERART;
        }
    }
    else
    {
        retMask = 0;
    }
    
    return retMask;
}

static S32 iPodCoreObserverCheckNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IAP2_EVENTNOTIFICATION_FUNC_TABLE *notifyFuncTable = NULL;
    U32 checkMask = 0;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    notifyFuncTable = (IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable;
    
    rc = iPodCoreObserverCheckIsUpdateDataMaskSet(iPodCtrlCfg, IPP_IAP2_DATA_MASK_PLAYBACK_STATUS, 0);
    if((rc == IPOD_PLAYER_OK) && (notifyFuncTable->notifyPlayBackStatus != NULL))
    {
        rc = notifyFuncTable->notifyPlayBackStatus(iPodCtrlCfg);
    }
    
    checkMask = iPodCoreObserverGetNeedToNotifyDataMask(iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE_TRACKINFO);
    if((checkMask > 0) && (notifyFuncTable->notifyTrackInformation != NULL))
    {
        rc = notifyFuncTable->notifyTrackInformation(iPodCtrlCfg, checkMask);
    }
    
    checkMask = iPodCoreObserverGetNeedToNotifyDataMask(iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE_DEVEVENT);
    if((checkMask > 0) && (notifyFuncTable->notifyDeviceEvent != NULL))
    {
        rc = notifyFuncTable->notifyDeviceEvent(iPodCtrlCfg, checkMask);
    }
    
    return rc;
}

static S32 iPodCoreObserverCheckUpdateDataBase(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *dbHandleOfMediaLibrary, IPOD_PLAYER_CORE_IAP2_DB_STATUS *dbStatus)
{
    S32 rc = IPOD_PLAYER_OK;
    BOOL updateDB = FALSE;
    BOOL plistflag = FALSE;

    if(dbStatus->startMediaLib)
    {
        if(dbStatus->dbUpdateProgress || dbStatus->dbUpdatePlaylist || dbStatus->mediaLibraryReset)
        {
            ippiAP2PlaylistTrackCheck(iPodCtrlCfg, dbHandleOfMediaLibrary, dbStatus, &iPodCtrlCfg->iap2Param.xferList, &plistflag);
            //IPOD_DLT_INFO("[DBG]mediaLibraryType=%d, plistflag=%d, UpdateProgress=%d, UpdatePlaylist=%d, MediaLibraryReset=%d", dbStatus->mediaLibraryType, plistflag, dbStatus->dbUpdateProgress, dbStatus->dbUpdatePlaylist, dbStatus->mediaLibraryReset);
            if(plistflag)
            {
                /* compare revision of file DB and revision of memory DB */
                if(ippiAP2DBCheckRevision(dbHandleOfMediaLibrary, dbStatus) == FALSE)    /* different ? */
                {
                    updateDB = TRUE;
                }
                else
                {
                    dbStatus->dbUpdateProgress = FALSE;
                    if(dbStatus->mediaLibraryReset)
                    {
                        updateDB = TRUE;
                    }
                }
            }
        }
        if(updateDB)
        {
            /* copy DataBase */
            rc = ippiAP2ProgressCheckDBUpdate(iPodCtrlCfg, dbHandleOfMediaLibrary);
            if(rc == IPOD_PLAYER_OK)
            {
                dbStatus->dbUpdateProgress = FALSE;
                dbStatus->mediaLibraryReset = FALSE;
                dbStatus->dbUpdatePlaylist = FALSE;
            }
        }
    }

    return rc;
}

S32 iPodCoreObserver(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = iPodCoreObserverCheckConnectionStatus(iPodCtrlCfg);
    if(rc != IPOD_PLAYER_ERR_IAP1_DETECTED)
    {
        iPodCoreObserverCheckMediaLibraryInformation(iPodCtrlCfg, &iPodCtrlCfg->iap2Param.dbHandle->localDevice, &iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice);
        iPodCoreObserverCheckMediaLibraryInformation(iPodCtrlCfg, &iPodCtrlCfg->iap2Param.dbHandle->appleMusicRadio, &iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio);

        iPodCoreObserverCheckFileXfer(iPodCtrlCfg, &iPodCtrlCfg->iap2Param.xferList);

        iPodCoreObserverCheckSamplerate(iPodCtrlCfg);

        if(iPodCtrlCfg->threadInfo->nameInfo.devType != IPOD_PLAYER_DEVICE_TYPE_BT)
        {
            rc = iPodCoreObserverCheckUpdateDataBase(iPodCtrlCfg, &iPodCtrlCfg->iap2Param.dbHandle->localDevice, &iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice);
            if((rc != IPOD_PLAYER_OK) && (rc != IPOD_PLAYER_ERR_DB_NOT_UPDATE))
            {
                IPOD_DLT_ERROR("Could not check Local Device media library progress. :rc=%d", rc);
            }
            rc = iPodCoreObserverCheckUpdateDataBase(iPodCtrlCfg, &iPodCtrlCfg->iap2Param.dbHandle->appleMusicRadio, &iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio);
            if((rc != IPOD_PLAYER_OK) && (rc != IPOD_PLAYER_ERR_DB_NOT_UPDATE))
            {
                IPOD_DLT_ERROR("Could not check Apple Music Radio media library progress. :rc=%d", rc);
            }
        }

        iPodCoreObserverCheckNotification(iPodCtrlCfg);

        rc = ippiAP2DelayedNotification(iPodCtrlCfg);
        if(rc != IPOD_PLAYER_OK)
        {
            IPOD_DLT_ERROR("Delayed notification error rc = %d.", rc);   
        }
    }
    
    return rc;
}
