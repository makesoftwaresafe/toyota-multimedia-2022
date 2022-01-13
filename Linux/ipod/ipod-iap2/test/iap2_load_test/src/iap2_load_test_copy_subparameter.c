/*
 * iap2_load_test_copy_subparameter.c
 *
 *  Created on: 06-Dec-2013
 *      Author: mana
 */

#include "iap2_load_test_copy_subparameter.h"
#include "iap2_test_utility.h"

S32 iap2CopyiAP2BluetoothComponentProfiles(iAP2BluetoothComponentProfiles** pdest_iAP2BluetoothComponentProfiles, iAP2BluetoothComponentProfiles* psrc_iAP2BluetoothComponentProfiles)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2BluetoothComponentProfiles == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothAdvancedAudioDistributionProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothAdvancedAudioDistributionProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothAdvancedAudioDistributionProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothAdvancedAudioDistributionProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothAdvancedAudioDistributionProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothAudioVideoRemotecontrolProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothAudioVideoRemotecontrolProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothAudioVideoRemotecontrolProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothAudioVideoRemotecontrolProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothAudioVideoRemotecontrolProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothHandsFreeProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothHandsFreeProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothHandsFreeProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothHandsFreeProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothHandsFreeProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothHumanInterfaceDeviceProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothHumanInterfaceDeviceProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothHumanInterfaceDeviceProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothHumanInterfaceDeviceProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothHumanInterfaceDeviceProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothiAP2LinkProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothiAP2LinkProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothiAP2LinkProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothiAP2LinkProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothiAP2LinkProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothMessageAccessProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothMessageAccessProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothMessageAccessProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothMessageAccessProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothMessageAccessProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothPersonalAreaNetworkAccessPointProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothPersonalAreaNetworkAccessPointProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothPersonalAreaNetworkAccessPointProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothPersonalAreaNetworkAccessPointProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothPersonalAreaNetworkAccessPointProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothPersonalAreaNetworkClientProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothPersonalAreaNetworkClientProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothPersonalAreaNetworkClientProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothPersonalAreaNetworkClientProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothPersonalAreaNetworkClientProfile_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothPhoneBookAccessProfile_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2BluetoothPhoneBookAccessProfile_count = %d", (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothPhoneBookAccessProfile_count);
        (*pdest_iAP2BluetoothComponentProfiles)->iAP2BluetoothPhoneBookAccessProfile_count = psrc_iAP2BluetoothComponentProfiles->iAP2BluetoothPhoneBookAccessProfile_count;
    }

    return rc;
}

S32 iap2CopyiAP2BluetoothComponentStatus(iAP2BluetoothComponentStatus** pdest_iAP2BluetoothComponentStatus, iAP2BluetoothComponentStatus* psrc_iAP2BluetoothComponentStatus)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2BluetoothComponentStatus == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentStatus->iAP2BTComponentEnabled_count > 0) )
    {
        printf("\nBTComponentEnabled");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2BluetoothComponentStatus)->iAP2BTComponentEnabled ),
                                       psrc_iAP2BluetoothComponentStatus->iAP2BTComponentEnabled,
                                        &( (*pdest_iAP2BluetoothComponentStatus)->iAP2BTComponentEnabled_count ),
                                       psrc_iAP2BluetoothComponentStatus->iAP2BTComponentEnabled_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier_count > 0) )
    {
        printf("\nBTComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2BluetoothComponentStatus)->iAP2BTComponentIdentifier ),
                                       psrc_iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier,
                                        &( (*pdest_iAP2BluetoothComponentStatus)->iAP2BTComponentIdentifier_count ),
                                       psrc_iAP2BluetoothComponentStatus->iAP2BTComponentIdentifier_count, iAP2_uint16);
    }

    return rc;
}

S32 iap2CopyiAP2BluetoothTransportComponent(iAP2BluetoothTransportComponent** pdest_iAP2BluetoothTransportComponent, iAP2BluetoothTransportComponent* psrc_iAP2BluetoothTransportComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2BluetoothTransportComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothTransportComponent->iAP2BluetoothTransportMediaAccessControlAddress_count > 0) )
    {
        printf("\nBluetoothTransportMediaAccessControlAddress");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2BluetoothTransportComponent)->iAP2BluetoothTransportMediaAccessControlAddress ),
                                       psrc_iAP2BluetoothTransportComponent->iAP2BluetoothTransportMediaAccessControlAddress,
                                        &( (*pdest_iAP2BluetoothTransportComponent)->iAP2BluetoothTransportMediaAccessControlAddress_count ),
                                       psrc_iAP2BluetoothTransportComponent->iAP2BluetoothTransportMediaAccessControlAddress_count, iAP2_blob);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothTransportComponent->iAP2TransportComponentIdentifier_count > 0) )
    {
        printf("\nTransportComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2BluetoothTransportComponent)->iAP2TransportComponentIdentifier ),
                                       psrc_iAP2BluetoothTransportComponent->iAP2TransportComponentIdentifier,
                                        &( (*pdest_iAP2BluetoothTransportComponent)->iAP2TransportComponentIdentifier_count ),
                                       psrc_iAP2BluetoothTransportComponent->iAP2TransportComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothTransportComponent->iAP2TransportComponentName_count > 0) )
    {
        printf("\nTransportComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2BluetoothTransportComponent)->iAP2TransportComponentName ),
                                       psrc_iAP2BluetoothTransportComponent->iAP2TransportComponentName,
                                        &( (*pdest_iAP2BluetoothTransportComponent)->iAP2TransportComponentName_count ),
                                       psrc_iAP2BluetoothTransportComponent->iAP2TransportComponentName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2BluetoothTransportComponent->iAP2TransportSupportsiAP2Connection_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2TransportSupportsiAP2Connection_count = %d", (*pdest_iAP2BluetoothTransportComponent)->iAP2TransportSupportsiAP2Connection_count);
        (*pdest_iAP2BluetoothTransportComponent)->iAP2TransportSupportsiAP2Connection_count = psrc_iAP2BluetoothTransportComponent->iAP2TransportSupportsiAP2Connection_count;
    }

    return rc;
}

S32 iap2CopyiAP2ExternalAccessoryProtocol(iAP2ExternalAccessoryProtocol** pdest_iAP2ExternalAccessoryProtocol, iAP2ExternalAccessoryProtocol* psrc_iAP2ExternalAccessoryProtocol)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2ExternalAccessoryProtocol == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolIdentifier_count > 0) )
    {
        printf("\nExternalAccessoryProtocolIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolIdentifier ),
                                       psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolIdentifier,
                                        &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolIdentifier_count ),
                                       psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolIdentifier_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolName_count > 0) )
    {
        printf("\nExternalAccessoryProtocolName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolName ),
                                       psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolName,
                                        &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolName_count ),
                                       psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolMatchAction_count > 0) )
    {
        (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolMatchAction = calloc(psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolMatchAction_count, sizeof(iAP2ExternalAccessoryProtocolMatchAction) );
        if( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolMatchAction == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolMatchAction_count; count++)
            {
                memcpy( &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolMatchAction[count] ), &psrc_iAP2ExternalAccessoryProtocol->iAP2ExternalAccessoryProtocolMatchAction[count], sizeof(iAP2ExternalAccessoryProtocolMatchAction) );
                printf("\nExternalAccessoryProtocolMatchAction[%d] = %d", count, (*pdest_iAP2ExternalAccessoryProtocol)->iAP2ExternalAccessoryProtocolMatchAction[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2ExternalAccessoryProtocol->iAP2NativeTransportComponentIdentifier_count > 0) )
    {
        printf("\nNativeTransportComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2NativeTransportComponentIdentifier ),
                                       psrc_iAP2ExternalAccessoryProtocol->iAP2NativeTransportComponentIdentifier,
                                        &( (*pdest_iAP2ExternalAccessoryProtocol)->iAP2NativeTransportComponentIdentifier_count ),
                                       psrc_iAP2ExternalAccessoryProtocol->iAP2NativeTransportComponentIdentifier_count, iAP2_uint16);
    }

    return rc;
}

S32 iap2CopyiAP2iAP2HIDComponent(iAP2iAP2HIDComponent** pdest_iAP2iAP2HIDComponent, iAP2iAP2HIDComponent* psrc_iAP2iAP2HIDComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2iAP2HIDComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2iAP2HIDComponent->iAP2HIDComponentFunction_count > 0) )
    {
        (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentFunction = calloc(psrc_iAP2iAP2HIDComponent->iAP2HIDComponentFunction_count, sizeof(iAP2HIDComponentFunction) );
        if( (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentFunction == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2iAP2HIDComponent->iAP2HIDComponentFunction_count; count++)
            {
                memcpy( &( (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentFunction[count] ), &psrc_iAP2iAP2HIDComponent->iAP2HIDComponentFunction[count], sizeof(iAP2HIDComponentFunction) );
                printf("\nHIDComponentFunction[%d] = %d", count, (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentFunction[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2iAP2HIDComponent->iAP2HIDComponentIdentifier_count > 0) )
    {
        printf("\nHIDComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentIdentifier ),
                                       psrc_iAP2iAP2HIDComponent->iAP2HIDComponentIdentifier,
                                        &( (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentIdentifier_count ),
                                       psrc_iAP2iAP2HIDComponent->iAP2HIDComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2iAP2HIDComponent->iAP2HIDComponentName_count > 0) )
    {
        printf("\nHIDComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentName ),
                                       psrc_iAP2iAP2HIDComponent->iAP2HIDComponentName,
                                        &( (*pdest_iAP2iAP2HIDComponent)->iAP2HIDComponentName_count ),
                                       psrc_iAP2iAP2HIDComponent->iAP2HIDComponentName_count, iAP2_utf8);
    }

    return rc;
}

S32 iap2CopyiAP2LocationInformationComponent(iAP2LocationInformationComponent** pdest_iAP2LocationInformationComponent, iAP2LocationInformationComponent* psrc_iAP2LocationInformationComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2LocationInformationComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentGlobalPositioningSystemFixData_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2LocationInformationComponentGlobalPositioningSystemFixData_count = %d", (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentGlobalPositioningSystemFixData_count);
        (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentGlobalPositioningSystemFixData_count = psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentGlobalPositioningSystemFixData_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier_count > 0) )
    {
        printf("\nLocationInformationComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentIdentifier ),
                                       psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier,
                                        &( (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentIdentifier_count ),
                                       psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentName_count > 0) )
    {
        printf("\nLocationInformationComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentName ),
                                       psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentName,
                                        &( (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentName_count ),
                                       psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentRecommendedMinimumSpecificGPSTransitData_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2LocationInformationComponentRecommendedMinimumSpecificGPSTransitData_count = %d", (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentRecommendedMinimumSpecificGPSTransitData_count);
        (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentRecommendedMinimumSpecificGPSTransitData_count = psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentRecommendedMinimumSpecificGPSTransitData_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleAccelerometerData_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2LocationInformationComponentVehicleAccelerometerData_count = %d", (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentVehicleAccelerometerData_count);
        (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentVehicleAccelerometerData_count = psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleAccelerometerData_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleGyroData_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2LocationInformationComponentVehicleGyroData_count = %d", (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentVehicleGyroData_count);
        (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentVehicleGyroData_count = psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleGyroData_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleSpeedData_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2LocationInformationComponentVehicleSpeedData_count = %d", (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentVehicleSpeedData_count);
        (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentVehicleSpeedData_count = psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleSpeedData_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentGPSSatelliteInView_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2LocationInformationComponentGPSSatelliteInView_count = %d", (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentGPSSatelliteInView_count);
        (*pdest_iAP2LocationInformationComponent)->iAP2LocationInformationComponentGPSSatelliteInView_count = psrc_iAP2LocationInformationComponent->iAP2LocationInformationComponentGPSSatelliteInView_count;
    }

    return rc;
}

S32 iap2CopyiAP2MediaItem(iAP2MediaItem** pdest_iAP2MediaItem, iAP2MediaItem* psrc_iAP2MediaItem)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaItem == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumArtist_count > 0) )
    {
        printf("\nMediaItemAlbumArtist");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumArtist ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumArtist,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumArtist_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumArtist_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumArtistPersistentIdentifier_count > 0) )
    {
        printf("\nMediaItemAlbumArtistPersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumArtistPersistentIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumArtistPersistentIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumArtistPersistentIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumArtistPersistentIdentifier_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumDiscCount_count > 0) )
    {
        printf("\nMediaItemAlbumDiscCount");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumDiscCount ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumDiscCount,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumDiscCount_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumDiscCount_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumDiscNumber_count > 0) )
    {
        printf("\nMediaItemAlbumDiscNumber");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumDiscNumber ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumDiscNumber,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumDiscNumber_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumDiscNumber_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumPersistentIdentifier_count > 0) )
    {
        printf("\nMediaItemAlbumPersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumPersistentIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumPersistentIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumPersistentIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumPersistentIdentifier_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumTitle_count > 0) )
    {
        printf("\nMediaItemAlbumTitle");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumTitle ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumTitle,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumTitle_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumTitle_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumTrackCount_count > 0) )
    {
        printf("\nMediaItemAlbumTrackCount");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumTrackCount ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumTrackCount,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumTrackCount_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumTrackCount_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemAlbumTrackNumber_count > 0) )
    {
        printf("\nMediaItemAlbumTrackNumber");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumTrackNumber ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumTrackNumber,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemAlbumTrackNumber_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemAlbumTrackNumber_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemArtist_count > 0) )
    {
        printf("\nMediaItemArtist");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemArtist ),
                                       psrc_iAP2MediaItem->iAP2MediaItemArtist,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemArtist_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemArtist_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemArtistPersistentIdentifier_count > 0) )
    {
        printf("\nMediaItemArtistPersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemArtistPersistentIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemArtistPersistentIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemArtistPersistentIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemArtistPersistentIdentifier_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemArtworkFileTransferIdentifier_count > 0) )
    {
        printf("\nMediaItemArtworkFileTransferIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemArtworkFileTransferIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemArtworkFileTransferIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemArtworkFileTransferIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemArtworkFileTransferIdentifier_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemComposer_count > 0) )
    {
        printf("\nMediaItemComposer");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemComposer ),
                                       psrc_iAP2MediaItem->iAP2MediaItemComposer,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemComposer_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemComposer_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemComposerPersistentIdentifier_count > 0) )
    {
        printf("\nMediaItemComposerPersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemComposerPersistentIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemComposerPersistentIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemComposerPersistentIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemComposerPersistentIdentifier_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemGenre_count > 0) )
    {
        printf("\nMediaItemGenre");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemGenre ),
                                       psrc_iAP2MediaItem->iAP2MediaItemGenre,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemGenre_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemGenre_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemGenrePersistentIdentifier_count > 0) )
    {
        printf("\nMediaItemGenrePersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemGenrePersistentIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemGenrePersistentIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemGenrePersistentIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemGenrePersistentIdentifier_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemIsPartOfCompilation_count > 0) )
    {
        printf("\nMediaItemIsPartOfCompilation");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsPartOfCompilation ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsPartOfCompilation,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsPartOfCompilation_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsPartOfCompilation_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemIsResidentOnDevice_count > 0) )
    {
        printf("\nMediaItemIsResidentOnDevice");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsResidentOnDevice ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsResidentOnDevice,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsResidentOnDevice_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsResidentOnDevice_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemMediaType_count > 0) )
    {
        (*pdest_iAP2MediaItem)->iAP2MediaItemMediaType = calloc(psrc_iAP2MediaItem->iAP2MediaItemMediaType_count, sizeof(iAP2MediaType) );
        if( (*pdest_iAP2MediaItem)->iAP2MediaItemMediaType == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2MediaItem->iAP2MediaItemMediaType_count; count++)
            {
                memcpy( &( (*pdest_iAP2MediaItem)->iAP2MediaItemMediaType[count] ), &psrc_iAP2MediaItem->iAP2MediaItemMediaType[count], sizeof(iAP2MediaType) );
                printf("\nMediaItemMediaType[%d] = %d", count, (*pdest_iAP2MediaItem)->iAP2MediaItemMediaType[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemPersistentIdentifier_count > 0) )
    {
        printf("\nMediaItemPersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemPersistentIdentifier ),
                                       psrc_iAP2MediaItem->iAP2MediaItemPersistentIdentifier,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemPersistentIdentifier_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemPersistentIdentifier_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemPlaybackDurationInMilliseconds_count > 0) )
    {
        printf("\nMediaItemPlaybackDurationInMilliseconds");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemPlaybackDurationInMilliseconds ),
                                       psrc_iAP2MediaItem->iAP2MediaItemPlaybackDurationInMilliseconds,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemPlaybackDurationInMilliseconds_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemPlaybackDurationInMilliseconds_count, iAP2_uint32);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemRating_count > 0) )
    {
        printf("\nMediaItemRating");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemRating ),
                                       psrc_iAP2MediaItem->iAP2MediaItemRating,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemRating_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemRating_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemTitle_count > 0) )
    {
        printf("\nMediaItemTitle");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemTitle ),
                                       psrc_iAP2MediaItem->iAP2MediaItemTitle,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemTitle_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemTitle_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemIsLikeSupported_count > 0) )
    {
        printf("\nMediaItemIsLikeSupported");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsLikeSupported ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsLikeSupported,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsLikeSupported_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsLikeSupported_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemIsBanSupported_count > 0) )
    {
        printf("\nMediaItemIsBanSupported");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsBanSupported ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsBanSupported,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsBanSupported_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsBanSupported_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemIsLiked_count > 0) )
    {
        printf("\nMediaItemIsLiked");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsLiked ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsLiked,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsLiked_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsLiked_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItem->iAP2MediaItemIsBanned_count > 0) )
    {
        printf("\nMediaItemIsBanned");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsBanned ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsBanned,
                                        &( (*pdest_iAP2MediaItem)->iAP2MediaItemIsBanned_count ),
                                       psrc_iAP2MediaItem->iAP2MediaItemIsBanned_count, iAP2_uint8);
    }

    return rc;
}

S32 iap2CopyiAP2MediaItemAttributes(iAP2MediaItemAttributes** pdest_iAP2MediaItemAttributes, iAP2MediaItemAttributes* psrc_iAP2MediaItemAttributes)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaItemAttributes == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscCount_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemAlbumDiscCount_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumDiscCount_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumDiscCount_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscCount_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscNumber_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemAlbumDiscNumber_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumDiscNumber_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumDiscNumber_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumDiscNumber_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumTitle_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemAlbumTitle_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumTitle_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumTitle_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumTitle_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackCount_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemAlbumTrackCount_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumTrackCount_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumTrackCount_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackCount_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackNumber_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemAlbumTrackNumber_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumTrackNumber_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemAlbumTrackNumber_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemAlbumTrackNumber_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemArtist_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemArtist_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemArtist_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemArtist_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemArtist_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemArtworkFileTransferIdentifier_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemArtworkFileTransferIdentifier_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemArtworkFileTransferIdentifier_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemComposer_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemComposer_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemComposer_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemComposer_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemComposer_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemGenre_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemGenre_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemGenre_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemGenre_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemGenre_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemIsBanned_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemIsBanned_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsBanned_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsBanned_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemIsBanned_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemIsBanSupported_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemIsBanSupported_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsBanSupported_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsBanSupported_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemIsBanSupported_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemIsLiked_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemIsLiked_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsLiked_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsLiked_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemIsLiked_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemIsLikeSupported_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemIsLikeSupported_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsLikeSupported_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemIsLikeSupported_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemIsLikeSupported_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemPlaybackDurationInMilliseconds_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPlaybackDurationInMilliseconds_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemPlaybackDurationInMilliseconds_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemPlaybackDurationInMilliseconds_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemPlaybackDurationInMilliseconds_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemAttributes->iAP2MediaItemTitle_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemTitle_count = %d", (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemTitle_count);
        (*pdest_iAP2MediaItemAttributes)->iAP2MediaItemTitle_count = psrc_iAP2MediaItemAttributes->iAP2MediaItemTitle_count;
    }

    return rc;
}

S32 iap2CopyiAP2MediaItemProperties(iAP2MediaItemProperties** pdest_iAP2MediaItemProperties, iAP2MediaItemProperties* psrc_iAP2MediaItemProperties)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaItemProperties == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtist_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumArtist_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumArtist_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumArtist_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtist_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumArtistPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscCount_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumDiscCount_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumDiscCount_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumDiscCount_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscCount_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscNumber_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumDiscNumber_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumDiscNumber_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumDiscNumber_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumDiscNumber_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumPersistentIdentifier_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumPersistentIdentifier_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumPersistentIdentifier_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTitle_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumTitle_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumTitle_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumTitle_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTitle_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackCount_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumTrackCount_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumTrackCount_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumTrackCount_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackCount_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackNumber_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyAlbumTrackNumber_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumTrackNumber_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyAlbumTrackNumber_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyAlbumTrackNumber_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyArtist_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyArtist_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyArtist_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyArtist_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyArtist_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyArtistPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyArtistPersistentIdentifier_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyArtistPersistentIdentifier_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyArtistPersistentIdentifier_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyArtistPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyComposer_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyComposer_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyComposer_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyComposer_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyComposer_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyComposerPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyComposerPersistentIdentifier_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyComposerPersistentIdentifier_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyComposerPersistentIdentifier_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyComposerPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyGenre_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyGenre_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyGenre_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyGenre_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyGenrePersistenIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyGenrePersistenIdentifier_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyGenrePersistenIdentifier_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyGenrePersistenIdentifier_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyGenrePersistenIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanned_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyIsBanned_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsBanned_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsBanned_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanned_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanSupported_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyIsBanSupported_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsBanSupported_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsBanSupported_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsBanSupported_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsLiked_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyIsLiked_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsLiked_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsLiked_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsLiked_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsLikeSupported_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyIsLikeSupported_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsLikeSupported_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsLikeSupported_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsLikeSupported_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsPartOfCompilation_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyIsPartOfCompilation_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsPartOfCompilation_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsPartOfCompilation_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsPartOfCompilation_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsResidentOndevice_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyIsResidentOndevice_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsResidentOndevice_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyIsResidentOndevice_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyIsResidentOndevice_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyMediaType_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyMediaType_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyMediaType_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyMediaType_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyMediaType_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyPersistentIdentifier_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyPersistentIdentifier_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyPersistentIdentifier_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyPlaybackDurationInMilliseconds_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyPlaybackDurationInMilliseconds_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyRating_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyRating_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyRating_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyRating_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyRating_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaItemPropertyTitle_count = %d", (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyTitle_count);
        (*pdest_iAP2MediaItemProperties)->iAP2MediaItemPropertyTitle_count = psrc_iAP2MediaItemProperties->iAP2MediaItemPropertyTitle_count;
    }

    return rc;
}

S32 iap2CopyiAP2MediaLibraryInformationSubParameter(iAP2MediaLibraryInformationSubParameter** pdest_iAP2MediaLibraryInformationSubParameter, iAP2MediaLibraryInformationSubParameter* psrc_iAP2MediaLibraryInformationSubParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaLibraryInformationSubParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryName_count > 0) )
    {
        printf("\nMediaLibraryName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaLibraryName ),
                                       psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryName,
                                        &( (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaLibraryName_count ),
                                       psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier_count > 0) )
    {
        printf("\nMediaUniqueIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaUniqueIdentifier ),
                                       psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier,
                                        &( (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaUniqueIdentifier_count ),
                                       psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaUniqueIdentifier_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryType_count > 0) )
    {
        (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaLibraryType = calloc(psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryType_count, sizeof(iAP2MediaLibraryType) );
        if( (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaLibraryType == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryType_count; count++)
            {
                memcpy( &( (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaLibraryType[count] ), &psrc_iAP2MediaLibraryInformationSubParameter->iAP2MediaLibraryType[count], sizeof(iAP2MediaLibraryType) );
                printf("\nMediaLibraryType[%d] = %d", count, (*pdest_iAP2MediaLibraryInformationSubParameter)->iAP2MediaLibraryType[count]);
            }
        }
    }

    return rc;
}

S32 iap2CopyiAP2MediaPlayList(iAP2MediaPlayList** pdest_iAP2MediaPlayList, iAP2MediaPlayList* psrc_iAP2MediaPlayList)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaPlayList == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier_count > 0) )
    {
        printf("\nMediaPlaylistContainedMediaItemsFileTransferIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsiTunesRadioStation_count > 0) )
    {
        printf("\nMediaPlaylistIsiTunesRadioStation");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistIsiTunesRadioStation ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsiTunesRadioStation,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistIsiTunesRadioStation_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsiTunesRadioStation_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsFolder_count > 0) )
    {
        printf("\nMediaPlaylistIsFolder");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistIsFolder ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsFolder,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistIsFolder_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsFolder_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsGeniusMix_count > 0) )
    {
        printf("\nMediaPlaylistIsGeniusMix");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistIsGeniusMix ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsGeniusMix,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistIsGeniusMix_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistIsGeniusMix_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistName_count > 0) )
    {
        printf("\nMediaPlaylistName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistName ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistName,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistName_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistParentPersistentIdentifer_count > 0) )
    {
        printf("\nMediaPlaylistParentPersistentIdentifer");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistParentPersistentIdentifer ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistParentPersistentIdentifer,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistParentPersistentIdentifer_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistParentPersistentIdentifer_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlayList->iAP2MediaPlaylistPersistentIdentifier_count > 0) )
    {
        printf("\nMediaPlaylistPersistentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistPersistentIdentifier ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistPersistentIdentifier,
                                        &( (*pdest_iAP2MediaPlayList)->iAP2MediaPlaylistPersistentIdentifier_count ),
                                       psrc_iAP2MediaPlayList->iAP2MediaPlaylistPersistentIdentifier_count, iAP2_uint64);
    }

    return rc;
}

S32 iap2CopyiAP2MediaPlaylistProperties(iAP2MediaPlaylistProperties** pdest_iAP2MediaPlaylistProperties, iAP2MediaPlaylistProperties* psrc_iAP2MediaPlaylistProperties)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaPlaylistProperties == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListContainedMediaItems_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListContainedMediaItems_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListContainedMediaItems_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListContainedMediaItems_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListContainedMediaItems_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsiTunesRadioStation_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListPropertyIsiTunesRadioStation_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyIsiTunesRadioStation_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyIsiTunesRadioStation_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsiTunesRadioStation_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsFolder_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListPropertyIsFolder_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyIsFolder_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyIsFolder_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyIsFolder_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyName_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListPropertyName_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyName_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyName_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyName_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyParentPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListPropertyParentPersistentIdentifier_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyParentPersistentIdentifier_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyParentPersistentIdentifier_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyParentPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPersistentIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListPropertyPersistentIdentifier_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyPersistentIdentifier_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyPersistentIdentifier_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPersistentIdentifier_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2MediaPlayListPropertyPropertyIsGeniusMix_count = %d", (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count);
        (*pdest_iAP2MediaPlaylistProperties)->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count = psrc_iAP2MediaPlaylistProperties->iAP2MediaPlayListPropertyPropertyIsGeniusMix_count;
    }

    return rc;
}

S32 iap2CopyiAP2PlaybackAttributes(iAP2PlaybackAttributes** pdest_iAP2PlaybackAttributes, iAP2PlaybackAttributes* psrc_iAP2PlaybackAttributes)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2PlaybackAttributes == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackAppName_count > 0) )
    {
        printf("\nPlaybackAppName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackAppName ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackAppName,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackAppName_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackAppName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count > 0) )
    {
        printf("\nPlaybackElapsedTimeInMilliseconds");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackElapsedTimeInMilliseconds ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackElapsedTimeInMilliseconds_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count, iAP2_uint32);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioAd_count > 0) )
    {
        printf("\nPBiTunesRadioAd");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PBiTunesRadioAd ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioAd,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PBiTunesRadioAd_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioAd_count, iAP2_uint8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioStationMediaPlaylistPersistentID_count > 0) )
    {
        printf("\nPBiTunesRadioStationMediaPlaylistPersistentID");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PBiTunesRadioStationMediaPlaylistPersistentID ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioStationMediaPlaylistPersistentID,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PBiTunesRadioStationMediaPlaylistPersistentID_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioStationMediaPlaylistPersistentID_count, iAP2_uint64);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioStationName_count > 0) )
    {
        printf("\nPBiTunesRadioStationName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PBiTunesRadioStationName ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioStationName,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PBiTunesRadioStationName_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBiTunesRadioStationName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueChapterIndex_count > 0) )
    {
        printf("\nPlaybackQueueChapterIndex");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackQueueChapterIndex ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueChapterIndex,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackQueueChapterIndex_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueChapterIndex_count, iAP2_uint32);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueCount_count > 0) )
    {
        printf("\nPlaybackQueueCount");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackQueueCount ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueCount,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackQueueCount_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueCount_count, iAP2_uint32);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueIndex_count > 0) )
    {
        printf("\nPlaybackQueueIndex");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackQueueIndex ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueIndex,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackQueueIndex_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PlaybackQueueIndex_count, iAP2_uint32);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count > 0) )
    {
        (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackRepeatMode = calloc(psrc_iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count, sizeof(iAP2PlaybackRepeat) );
        if( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackRepeatMode == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count; count++)
            {
                memcpy( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackRepeatMode[count] ), &psrc_iAP2PlaybackAttributes->iAP2PlaybackRepeatMode[count], sizeof(iAP2PlaybackRepeat) );
                printf("\nPlaybackRepeatMode[%d] = %d", count, (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackRepeatMode[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count > 0) )
    {
        (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackShuffleMode = calloc(psrc_iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count, sizeof(iAP2PlaybackShuffle) );
        if( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackShuffleMode == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count; count++)
            {
                memcpy( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackShuffleMode[count] ), &psrc_iAP2PlaybackAttributes->iAP2PlaybackShuffleMode[count], sizeof(iAP2PlaybackShuffle) );
                printf("\nPlaybackShuffleMode[%d] = %d", count, (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackShuffleMode[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PlaybackStatus_count > 0) )
    {
        (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackStatus = calloc(psrc_iAP2PlaybackAttributes->iAP2PlaybackStatus_count, sizeof(iAP2PlaybackStatus) );
        if( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackStatus == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2PlaybackAttributes->iAP2PlaybackStatus_count; count++)
            {
                memcpy( &( (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackStatus[count] ), &psrc_iAP2PlaybackAttributes->iAP2PlaybackStatus[count], sizeof(iAP2PlaybackStatus) );
                printf("\nPlaybackStatus[%d] = %d", count, (*pdest_iAP2PlaybackAttributes)->iAP2PlaybackStatus[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2PlaybackAttributes->iAP2PBMediaLibraryUniqueIdentifier_count > 0) )
    {
        printf("\nPBMediaLibraryUniqueIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PlaybackAttributes)->iAP2PBMediaLibraryUniqueIdentifier ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBMediaLibraryUniqueIdentifier,
                                        &( (*pdest_iAP2PlaybackAttributes)->iAP2PBMediaLibraryUniqueIdentifier_count ),
                                       psrc_iAP2PlaybackAttributes->iAP2PBMediaLibraryUniqueIdentifier_count, iAP2_utf8);
    }

    return rc;
}

S32 iap2CopyiAP2SerialTransportComponent(iAP2SerialTransportComponent** pdest_iAP2SerialTransportComponent, iAP2SerialTransportComponent* psrc_iAP2SerialTransportComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2SerialTransportComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2SerialTransportComponent->iAP2TransportComponentIdentifier_count > 0) )
    {
        printf("\nTransportComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2SerialTransportComponent)->iAP2TransportComponentIdentifier ),
                                       psrc_iAP2SerialTransportComponent->iAP2TransportComponentIdentifier,
                                        &( (*pdest_iAP2SerialTransportComponent)->iAP2TransportComponentIdentifier_count ),
                                       psrc_iAP2SerialTransportComponent->iAP2TransportComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2SerialTransportComponent->iAP2TransportComponentName_count > 0) )
    {
        printf("\nTransportComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2SerialTransportComponent)->iAP2TransportComponentName ),
                                       psrc_iAP2SerialTransportComponent->iAP2TransportComponentName,
                                        &( (*pdest_iAP2SerialTransportComponent)->iAP2TransportComponentName_count ),
                                       psrc_iAP2SerialTransportComponent->iAP2TransportComponentName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2SerialTransportComponent->iAP2TransportSupportsiAP2Connection_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2TransportSupportsiAP2Connection_count = %d", (*pdest_iAP2SerialTransportComponent)->iAP2TransportSupportsiAP2Connection_count);
        (*pdest_iAP2SerialTransportComponent)->iAP2TransportSupportsiAP2Connection_count = psrc_iAP2SerialTransportComponent->iAP2TransportSupportsiAP2Connection_count;
    }

    return rc;
}

S32 iap2CopyiAP2StartNowPlayingPlaybackAttributes(iAP2StartNowPlayingPlaybackAttributes** pdest_iAP2StartNowPlayingPlaybackAttributes, iAP2StartNowPlayingPlaybackAttributes* psrc_iAP2StartNowPlayingPlaybackAttributes)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2StartNowPlayingPlaybackAttributes == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackAppName_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackAppName_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackAppName_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackAppName_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackAppName_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackElapsedTimeInMilliseconds_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackElapsedTimeInMilliseconds_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackElapsedTimeInMilliseconds_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PBiTunesRadioAd_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PBiTunesRadioAd_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PBiTunesRadioAd_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PBiTunesRadioAd_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PBiTunesRadioAd_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PBiTunesStationMediaPlaylistPersistentID_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PBiTunesStationMediaPlaylistPersistentID_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PBiTunesStationMediaPlaylistPersistentID_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PBiTunesStationMediaPlaylistPersistentID_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PBiTunesStationMediaPlaylistPersistentID_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PBiTunesStationName_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PBiTunesStationName_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PBiTunesStationName_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PBiTunesStationName_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PBiTunesStationName_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackQueueChapterIndex_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackQueueChapterIndex_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackQueueChapterIndex_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackQueueChapterIndex_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackQueueChapterIndex_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackQueueCount_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackQueueCount_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackQueueCount_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackQueueCount_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackQueueCount_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackQueueIndex_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackQueueIndex_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackQueueIndex_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackQueueIndex_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackQueueIndex_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackRepeatMode_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackRepeatMode_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackRepeatMode_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackRepeatMode_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackRepeatMode_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackShuffleMode_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackShuffleMode_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackShuffleMode_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackShuffleMode_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackShuffleMode_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackStatus_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackStatus_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackStatus_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackStatus_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackStatus_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackMediaLibraryUniqueIdentifier_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2PlaybackMediaLibraryUniqueIdentifier_count = %d", (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackMediaLibraryUniqueIdentifier_count);
        (*pdest_iAP2StartNowPlayingPlaybackAttributes)->iAP2PlaybackMediaLibraryUniqueIdentifier_count = psrc_iAP2StartNowPlayingPlaybackAttributes->iAP2PlaybackMediaLibraryUniqueIdentifier_count;
    }

    return rc;
}

S32 iap2CopyiAP2USBDeviceTransportComponent(iAP2USBDeviceTransportComponent** pdest_iAP2USBDeviceTransportComponent, iAP2USBDeviceTransportComponent* psrc_iAP2USBDeviceTransportComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2USBDeviceTransportComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier_count > 0) )
    {
        printf("\nTransportComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBDeviceTransportComponent)->iAP2TransportComponentIdentifier ),
                                       psrc_iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier,
                                        &( (*pdest_iAP2USBDeviceTransportComponent)->iAP2TransportComponentIdentifier_count ),
                                       psrc_iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBDeviceTransportComponent->iAP2TransportComponentName_count > 0) )
    {
        printf("\nTransportComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBDeviceTransportComponent)->iAP2TransportComponentName ),
                                       psrc_iAP2USBDeviceTransportComponent->iAP2TransportComponentName,
                                        &( (*pdest_iAP2USBDeviceTransportComponent)->iAP2TransportComponentName_count ),
                                       psrc_iAP2USBDeviceTransportComponent->iAP2TransportComponentName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBDeviceTransportComponent->iAP2TransportSupportsiAP2Connection_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2TransportSupportsiAP2Connection_count = %d", (*pdest_iAP2USBDeviceTransportComponent)->iAP2TransportSupportsiAP2Connection_count);
        (*pdest_iAP2USBDeviceTransportComponent)->iAP2TransportSupportsiAP2Connection_count = psrc_iAP2USBDeviceTransportComponent->iAP2TransportSupportsiAP2Connection_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate_count > 0) )
    {
        (*pdest_iAP2USBDeviceTransportComponent)->iAP2USBDeviceSupportedAudioSampleRate = calloc(psrc_iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate_count, sizeof(iAP2USBDeviceModeAudioSampleRate) );
        if( (*pdest_iAP2USBDeviceTransportComponent)->iAP2USBDeviceSupportedAudioSampleRate == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate_count; count++)
            {
                memcpy( &( (*pdest_iAP2USBDeviceTransportComponent)->iAP2USBDeviceSupportedAudioSampleRate[count] ), &psrc_iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate[count], sizeof(iAP2USBDeviceModeAudioSampleRate) );
                printf("\nUSBDeviceSupportedAudioSampleRate[%d] = %d", count, (*pdest_iAP2USBDeviceTransportComponent)->iAP2USBDeviceSupportedAudioSampleRate[count]);
            }
        }
    }

    return rc;
}

S32 iap2CopyiAP2USBHostHIDComponent(iAP2USBHostHIDComponent** pdest_iAP2USBHostHIDComponent, iAP2USBHostHIDComponent* psrc_iAP2USBHostHIDComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2USBHostHIDComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostHIDComponent->iAP2HIDComponentFunction_count > 0) )
    {
        (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentFunction = calloc(psrc_iAP2USBHostHIDComponent->iAP2HIDComponentFunction_count, sizeof(iAP2HIDComponentFunction) );
        if( (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentFunction == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2USBHostHIDComponent->iAP2HIDComponentFunction_count; count++)
            {
                memcpy( &( (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentFunction[count] ), &psrc_iAP2USBHostHIDComponent->iAP2HIDComponentFunction[count], sizeof(iAP2HIDComponentFunction) );
                printf("\nHIDComponentFunction[%d] = %d", count, (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentFunction[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostHIDComponent->iAP2HIDComponentIdentifier_count > 0) )
    {
        printf("\nHIDComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentIdentifier ),
                                       psrc_iAP2USBHostHIDComponent->iAP2HIDComponentIdentifier,
                                        &( (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentIdentifier_count ),
                                       psrc_iAP2USBHostHIDComponent->iAP2HIDComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostHIDComponent->iAP2HIDComponentName_count > 0) )
    {
        printf("\nHIDComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentName ),
                                       psrc_iAP2USBHostHIDComponent->iAP2HIDComponentName,
                                        &( (*pdest_iAP2USBHostHIDComponent)->iAP2HIDComponentName_count ),
                                       psrc_iAP2USBHostHIDComponent->iAP2HIDComponentName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostHIDComponent->iAP2USBHostTransportComponentIdentifier_count > 0) )
    {
        printf("\nUSBHostTransportComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostHIDComponent)->iAP2USBHostTransportComponentIdentifier ),
                                       psrc_iAP2USBHostHIDComponent->iAP2USBHostTransportComponentIdentifier,
                                        &( (*pdest_iAP2USBHostHIDComponent)->iAP2USBHostTransportComponentIdentifier_count ),
                                       psrc_iAP2USBHostHIDComponent->iAP2USBHostTransportComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostHIDComponent->iAP2USBHostTransportInterfaceNumber_count > 0) )
    {
        printf("\nUSBHostTransportInterfaceNumber");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostHIDComponent)->iAP2USBHostTransportInterfaceNumber ),
                                       psrc_iAP2USBHostHIDComponent->iAP2USBHostTransportInterfaceNumber,
                                        &( (*pdest_iAP2USBHostHIDComponent)->iAP2USBHostTransportInterfaceNumber_count ),
                                       psrc_iAP2USBHostHIDComponent->iAP2USBHostTransportInterfaceNumber_count, iAP2_uint16);
    }

    return rc;
}

S32 iap2CopyiAP2USBHostTransportComponent(iAP2USBHostTransportComponent** pdest_iAP2USBHostTransportComponent, iAP2USBHostTransportComponent* psrc_iAP2USBHostTransportComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2USBHostTransportComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier_count > 0) )
    {
        printf("\nTransportComponentIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostTransportComponent)->iAP2TransportComponentIdentifier ),
                                       psrc_iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier,
                                        &( (*pdest_iAP2USBHostTransportComponent)->iAP2TransportComponentIdentifier_count ),
                                       psrc_iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostTransportComponent->iAP2TransportComponentName_count > 0) )
    {
        printf("\nTransportComponentName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostTransportComponent)->iAP2TransportComponentName ),
                                       psrc_iAP2USBHostTransportComponent->iAP2TransportComponentName,
                                        &( (*pdest_iAP2USBHostTransportComponent)->iAP2TransportComponentName_count ),
                                       psrc_iAP2USBHostTransportComponent->iAP2TransportComponentName_count, iAP2_utf8);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostTransportComponent->iAP2TransportSupportsiAP2Connection_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2TransportSupportsiAP2Connection_count = %d", (*pdest_iAP2USBHostTransportComponent)->iAP2TransportSupportsiAP2Connection_count);
        (*pdest_iAP2USBHostTransportComponent)->iAP2TransportSupportsiAP2Connection_count = psrc_iAP2USBHostTransportComponent->iAP2TransportSupportsiAP2Connection_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber_count > 0) )
    {
        printf("\nUSBHostTransportDigitaliPodOutInterfaceNumber");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2USBHostTransportComponent)->iAP2USBHostTransportCarPlaytInterfaceNumber ),
                                       psrc_iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber,
                                        &( (*pdest_iAP2USBHostTransportComponent)->iAP2USBHostTransportCarPlaytInterfaceNumber_count ),
                                       psrc_iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber_count, iAP2_uint8);
    }

    return rc;
}

S32 iap2CopyiAP2VehicleInformationComponent(iAP2VehicleInformationComponent** pdest_iAP2VehicleInformationComponent, iAP2VehicleInformationComponent* psrc_iAP2VehicleInformationComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2VehicleInformationComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleInformationComponent->iAP2EngineType_count > 0) )
    {
        (*pdest_iAP2VehicleInformationComponent)->iAP2EngineType = calloc(psrc_iAP2VehicleInformationComponent->iAP2EngineType_count, sizeof(iAP2EngineTypes) );
        if( (*pdest_iAP2VehicleInformationComponent)->iAP2EngineType == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            U16 count = 0;

            for(count = 0; count < psrc_iAP2VehicleInformationComponent->iAP2EngineType_count; count++)
            {
                memcpy( &( (*pdest_iAP2VehicleInformationComponent)->iAP2EngineType[count] ), &psrc_iAP2VehicleInformationComponent->iAP2EngineType[count], sizeof(iAP2EngineTypes) );
                printf("\nEngineType[%d] = %d", count, (*pdest_iAP2VehicleInformationComponent)->iAP2EngineType[count]);
            }
        }
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleInformationComponent->iAP2Identifier_count > 0) )
    {
        printf("\nIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VehicleInformationComponent)->iAP2Identifier ),
                                       psrc_iAP2VehicleInformationComponent->iAP2Identifier,
                                        &( (*pdest_iAP2VehicleInformationComponent)->iAP2Identifier_count ),
                                       psrc_iAP2VehicleInformationComponent->iAP2Identifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleInformationComponent->iAP2Name_count > 0) )
    {
        printf("\nName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VehicleInformationComponent)->iAP2Name ),
                                       psrc_iAP2VehicleInformationComponent->iAP2Name,
                                        &( (*pdest_iAP2VehicleInformationComponent)->iAP2Name_count ),
                                       psrc_iAP2VehicleInformationComponent->iAP2Name_count, iAP2_utf8);
    }

    return rc;
}

S32 iap2CopyiAP2VehicleStatusComponent(iAP2VehicleStatusComponent** pdest_iAP2VehicleStatusComponent, iAP2VehicleStatusComponent* psrc_iAP2VehicleStatusComponent)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2VehicleStatusComponent == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2InsideTemperature_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2InsideTemperature_count = %d", (*pdest_iAP2VehicleStatusComponent)->iAP2InsideTemperature_count);
        (*pdest_iAP2VehicleStatusComponent)->iAP2InsideTemperature_count = psrc_iAP2VehicleStatusComponent->iAP2InsideTemperature_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2NightMode_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2NightMode_count = %d", (*pdest_iAP2VehicleStatusComponent)->iAP2NightMode_count);
        (*pdest_iAP2VehicleStatusComponent)->iAP2NightMode_count = psrc_iAP2VehicleStatusComponent->iAP2NightMode_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2OutsideTemperature_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2OutsideTemperature_count = %d", (*pdest_iAP2VehicleStatusComponent)->iAP2OutsideTemperature_count);
        (*pdest_iAP2VehicleStatusComponent)->iAP2OutsideTemperature_count = psrc_iAP2VehicleStatusComponent->iAP2OutsideTemperature_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2Range_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2Range_count = %d", (*pdest_iAP2VehicleStatusComponent)->iAP2Range_count);
        (*pdest_iAP2VehicleStatusComponent)->iAP2Range_count = psrc_iAP2VehicleStatusComponent->iAP2Range_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2RangeWarning_count > 0) )
    {
        /* For type none just update the count value */
        printf("\niAP2RangeWarning_count = %d", (*pdest_iAP2VehicleStatusComponent)->iAP2RangeWarning_count);
        (*pdest_iAP2VehicleStatusComponent)->iAP2RangeWarning_count = psrc_iAP2VehicleStatusComponent->iAP2RangeWarning_count;
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2Identifier_count > 0) )
    {
        printf("\nIdentifier");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VehicleStatusComponent)->iAP2Identifier ),
                                       psrc_iAP2VehicleStatusComponent->iAP2Identifier,
                                        &( (*pdest_iAP2VehicleStatusComponent)->iAP2Identifier_count ),
                                       psrc_iAP2VehicleStatusComponent->iAP2Identifier_count, iAP2_uint16);
    }
    if( (rc == IAP2_OK) && (psrc_iAP2VehicleStatusComponent->iAP2Name_count > 0) )
    {
        printf("\nName");
        rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VehicleStatusComponent)->iAP2Name ),
                                       psrc_iAP2VehicleStatusComponent->iAP2Name,
                                        &( (*pdest_iAP2VehicleStatusComponent)->iAP2Name_count ),
                                       psrc_iAP2VehicleStatusComponent->iAP2Name_count, iAP2_utf8);
    }

    return rc;
}

