/*
 * iap2_load_test_copy_parameters.c
 *
 *  Created on: 06-Dec-2013
 *      Author: mana
 */

#include "iap2_load_test_copy_parameter.h"
#include "iap2_load_test_copy_subparameter.h"
#include "iap2_test_utility.h"

S32 iap2CopyiAP2RequestAuthenticationChallengeResponseParameter(iAP2RequestAuthenticationChallengeResponseParameter** pdest_iAP2RequestAuthenticationChallengeResponseParameter, iAP2RequestAuthenticationChallengeResponseParameter* psrc_iAP2RequestAuthenticationChallengeResponseParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2RequestAuthenticationChallengeResponseParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2RequestAuthenticationChallengeResponseParameter = calloc(1, sizeof(iAP2RequestAuthenticationChallengeResponseParameter) );
        if(*pdest_iAP2RequestAuthenticationChallengeResponseParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2RequestAuthenticationChallengeResponseParameter->iAP2AuthenticationChallenge_count > 0) )
        {
            printf("\nAuthenticationChallenge");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2RequestAuthenticationChallengeResponseParameter)->iAP2AuthenticationChallenge ),
                                           psrc_iAP2RequestAuthenticationChallengeResponseParameter->iAP2AuthenticationChallenge,
                                            &( (*pdest_iAP2RequestAuthenticationChallengeResponseParameter)->iAP2AuthenticationChallenge_count ),
                                           psrc_iAP2RequestAuthenticationChallengeResponseParameter->iAP2AuthenticationChallenge_count, iAP2_blob);
        }
    }

    return rc;
}

S32 iap2CopyiAP2IdentificationRejectedParameter(iAP2IdentificationRejectedParameter** pdest_iAP2IdentificationRejectedParameter, iAP2IdentificationRejectedParameter* psrc_iAP2IdentificationRejectedParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2IdentificationRejectedParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2IdentificationRejectedParameter = calloc(1, sizeof(iAP2IdentificationRejectedParameter) );
        if(*pdest_iAP2IdentificationRejectedParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryFirmwareVersion_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2AccessoryFirmwareVersion_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryFirmwareVersion_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryFirmwareVersion_count = psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryFirmwareVersion_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryHardwareVersion_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2AccessoryHardwareVersion_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryHardwareVersion_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryHardwareVersion_count = psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryHardwareVersion_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryManufacturer_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2AccessoryManufacturer_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryManufacturer_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryManufacturer_count = psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryManufacturer_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryModelIdentifier_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2AccessoryModelIdentifier_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryModelIdentifier_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryModelIdentifier_count = psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryModelIdentifier_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryName_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2AccessoryName_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryName_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessoryName_count = psrc_iAP2IdentificationRejectedParameter->iAP2AccessoryName_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2AccessorySerialNumber_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2AccessorySerialNumber_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessorySerialNumber_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2AccessorySerialNumber_count = psrc_iAP2IdentificationRejectedParameter->iAP2AccessorySerialNumber_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2BluetoothTransportComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2BluetoothTransportComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2BluetoothTransportComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2BluetoothTransportComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2BluetoothTransportComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2CurrentLanguage_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2CurrentLanguage_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2CurrentLanguage_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2CurrentLanguage_count = psrc_iAP2IdentificationRejectedParameter->iAP2CurrentLanguage_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2iAP2HIDComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2iAP2HIDComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2iAP2HIDComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2iAP2HIDComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2iAP2HIDComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2LocationInformationComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2LocationInformationComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2LocationInformationComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2LocationInformationComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2LocationInformationComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2MaximumCurrentDrawnFromDevice_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2MaximumCurrentDrawnFromDevice_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2MaximumCurrentDrawnFromDevice_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2MaximumCurrentDrawnFromDevice_count = psrc_iAP2IdentificationRejectedParameter->iAP2MaximumCurrentDrawnFromDevice_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2MessagesRecievedfromDevice_count > 0) )
        {
            printf("\nMessagesRecievedfromDevice");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2IdentificationRejectedParameter)->iAP2MessagesRecievedfromDevice ),
                                           psrc_iAP2IdentificationRejectedParameter->iAP2MessagesRecievedfromDevice,
                                            &( (*pdest_iAP2IdentificationRejectedParameter)->iAP2MessagesRecievedfromDevice_count ),
                                           psrc_iAP2IdentificationRejectedParameter->iAP2MessagesRecievedfromDevice_count, iAP2_blob);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2MessagesSentByAccessory_count > 0) )
        {
            printf("\nMessagesSentByAccessory");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2IdentificationRejectedParameter)->iAP2MessagesSentByAccessory ),
                                           psrc_iAP2IdentificationRejectedParameter->iAP2MessagesSentByAccessory,
                                            &( (*pdest_iAP2IdentificationRejectedParameter)->iAP2MessagesSentByAccessory_count ),
                                           psrc_iAP2IdentificationRejectedParameter->iAP2MessagesSentByAccessory_count, iAP2_blob);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2PowerSourceType_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2PowerSourceType_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2PowerSourceType_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2PowerSourceType_count = psrc_iAP2IdentificationRejectedParameter->iAP2PowerSourceType_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2PreferredAppBundleSeedIdentifier_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2PreferredAppBundleSeedIdentifier_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2PreferredAppBundleSeedIdentifier_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2PreferredAppBundleSeedIdentifier_count = psrc_iAP2IdentificationRejectedParameter->iAP2PreferredAppBundleSeedIdentifier_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2SerialTransportComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2SerialTransportComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2SerialTransportComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2SerialTransportComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2SerialTransportComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2SupportedExternalAccessoryProtocol_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2SupportedExternalAccessoryProtocol_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2SupportedExternalAccessoryProtocol_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2SupportedExternalAccessoryProtocol_count = psrc_iAP2IdentificationRejectedParameter->iAP2SupportedExternalAccessoryProtocol_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2SupportedLanguage_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2SupportedLanguage_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2SupportedLanguage_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2SupportedLanguage_count = psrc_iAP2IdentificationRejectedParameter->iAP2SupportedLanguage_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2USBDeviceTransportComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2USBDeviceTransportComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2USBDeviceTransportComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2USBDeviceTransportComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2USBDeviceTransportComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2USBHostHIDComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2USBHostHIDComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2USBHostHIDComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2USBHostHIDComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2USBHostHIDComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2USBHostTransportComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2USBHostTransportComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2USBHostTransportComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2USBHostTransportComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2USBHostTransportComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2VehicleInformationComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2VehicleInformationComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2VehicleInformationComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2VehicleInformationComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2VehicleInformationComponent_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2IdentificationRejectedParameter->iAP2VehicleStatusComponent_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2VehicleStatusComponent_count = %d", (*pdest_iAP2IdentificationRejectedParameter)->iAP2VehicleStatusComponent_count);
            (*pdest_iAP2IdentificationRejectedParameter)->iAP2VehicleStatusComponent_count = psrc_iAP2IdentificationRejectedParameter->iAP2VehicleStatusComponent_count;
        }
    }

    return rc;
}

S32 iap2CopyiAP2AssistiveTouchInformationParameter(iAP2AssistiveTouchInformationParameter** pdest_iAP2AssistiveTouchInformationParameter, iAP2AssistiveTouchInformationParameter* psrc_iAP2AssistiveTouchInformationParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2AssistiveTouchInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2AssistiveTouchInformationParameter = calloc(1, sizeof(iAP2AssistiveTouchInformationParameter) );
        if(*pdest_iAP2AssistiveTouchInformationParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2AssistiveTouchInformationParameter->IsEnabled_count > 0) )
        {
            printf("\nIsEnabled");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2AssistiveTouchInformationParameter)->IsEnabled ),
                                           psrc_iAP2AssistiveTouchInformationParameter->IsEnabled,
                                            &( (*pdest_iAP2AssistiveTouchInformationParameter)->IsEnabled_count ),
                                           psrc_iAP2AssistiveTouchInformationParameter->IsEnabled_count, iAP2_uint8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2BluetoothConnectionUpdateParameter(iAP2BluetoothConnectionUpdateParameter** pdest_iAP2BluetoothConnectionUpdateParameter, iAP2BluetoothConnectionUpdateParameter* psrc_iAP2BluetoothConnectionUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2BluetoothConnectionUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2BluetoothConnectionUpdateParameter = calloc(1, sizeof(iAP2BluetoothConnectionUpdateParameter) );
        if(*pdest_iAP2BluetoothConnectionUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2BluetoothConnectionUpdateParameter->iAP2BluetoothComponentProfiles_count > 0) )
        {
            (*pdest_iAP2BluetoothConnectionUpdateParameter)->iAP2BluetoothComponentProfiles = calloc(psrc_iAP2BluetoothConnectionUpdateParameter->iAP2BluetoothComponentProfiles_count, sizeof(iAP2BluetoothComponentProfiles) );
            if( (*pdest_iAP2BluetoothConnectionUpdateParameter)->iAP2BluetoothComponentProfiles == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                rc = iap2CopyiAP2BluetoothComponentProfiles( &( (*pdest_iAP2BluetoothConnectionUpdateParameter)->iAP2BluetoothComponentProfiles ), psrc_iAP2BluetoothConnectionUpdateParameter->iAP2BluetoothComponentProfiles);
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2BluetoothConnectionUpdateParameter->iAP2BluetoothTransportComponentIdentifier_count > 0) )
        {
            printf("\nBluetoothTransportComponentIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2BluetoothConnectionUpdateParameter)->iAP2BluetoothTransportComponentIdentifier ),
                                           psrc_iAP2BluetoothConnectionUpdateParameter->iAP2BluetoothTransportComponentIdentifier,
                                            &( (*pdest_iAP2BluetoothConnectionUpdateParameter)->iAP2BluetoothTransportComponentIdentifier_count ),
                                           psrc_iAP2BluetoothConnectionUpdateParameter->iAP2BluetoothTransportComponentIdentifier_count, iAP2_uint16);
        }
    }

    return rc;
}

S32 iap2CopyiAP2DeviceAuthenticationCertificateParameter(iAP2DeviceAuthenticationCertificateParameter** pdest_iAP2DeviceAuthenticationCertificateParameter, iAP2DeviceAuthenticationCertificateParameter* psrc_iAP2DeviceAuthenticationCertificateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2DeviceAuthenticationCertificateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2DeviceAuthenticationCertificateParameter = calloc(1, sizeof(iAP2DeviceAuthenticationCertificateParameter) );
        if(*pdest_iAP2DeviceAuthenticationCertificateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2DeviceAuthenticationCertificateParameter->iAP2DeviceCertificate_count > 0) )
        {
            printf("\nDeviceCertificate");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2DeviceAuthenticationCertificateParameter)->iAP2DeviceCertificate ),
                                           psrc_iAP2DeviceAuthenticationCertificateParameter->iAP2DeviceCertificate,
                                            &( (*pdest_iAP2DeviceAuthenticationCertificateParameter)->iAP2DeviceCertificate_count ),
                                           psrc_iAP2DeviceAuthenticationCertificateParameter->iAP2DeviceCertificate_count, iAP2_blob);
        }
    }

    return rc;
}

S32 iap2CopyiAP2DeviceAuthenticationResponseParameter(iAP2DeviceAuthenticationResponseParameter** pdest_iAP2DeviceAuthenticationResponseParameter, iAP2DeviceAuthenticationResponseParameter* psrc_iAP2DeviceAuthenticationResponseParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2DeviceAuthenticationResponseParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2DeviceAuthenticationResponseParameter = calloc(1, sizeof(iAP2DeviceAuthenticationResponseParameter) );
        if(*pdest_iAP2DeviceAuthenticationResponseParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2DeviceAuthenticationResponseParameter->iAP2ChallengeResponse_count > 0) )
        {
            printf("\nChallengeResponse");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2DeviceAuthenticationResponseParameter)->iAP2ChallengeResponse ),
                                           psrc_iAP2DeviceAuthenticationResponseParameter->iAP2ChallengeResponse,
                                            &( (*pdest_iAP2DeviceAuthenticationResponseParameter)->iAP2ChallengeResponse_count ),
                                           psrc_iAP2DeviceAuthenticationResponseParameter->iAP2ChallengeResponse_count, iAP2_blob);
        }
    }

    return rc;
}

S32 iap2CopyiAP2DeviceInformationUpdateParameter(iAP2DeviceInformationUpdateParameter** pdest_iAP2DeviceInformationUpdateParameter, iAP2DeviceInformationUpdateParameter* psrc_iAP2DeviceInformationUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2DeviceInformationUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2DeviceInformationUpdateParameter = calloc(1, sizeof(iAP2DeviceInformationUpdateParameter) );
        if(*pdest_iAP2DeviceInformationUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2DeviceInformationUpdateParameter->iAP2DeviceName_count > 0) )
        {
            printf("\nDeviceName");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2DeviceInformationUpdateParameter)->iAP2DeviceName ),
                                           psrc_iAP2DeviceInformationUpdateParameter->iAP2DeviceName,
                                            &( (*pdest_iAP2DeviceInformationUpdateParameter)->iAP2DeviceName_count ),
                                           psrc_iAP2DeviceInformationUpdateParameter->iAP2DeviceName_count, iAP2_utf8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2DeviceLanguageUpdateParameter(iAP2DeviceLanguageUpdateParameter** pdest_iAP2DeviceLanguageUpdateParameter, iAP2DeviceLanguageUpdateParameter* psrc_iAP2DeviceLanguageUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2DeviceLanguageUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2DeviceLanguageUpdateParameter = calloc(1, sizeof(iAP2DeviceLanguageUpdateParameter) );
        if(*pdest_iAP2DeviceLanguageUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2DeviceLanguageUpdateParameter->iAP2DeviceName_count > 0) )
        {
            printf("\nDeviceName");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2DeviceLanguageUpdateParameter)->iAP2DeviceName ),
                                           psrc_iAP2DeviceLanguageUpdateParameter->iAP2DeviceName,
                                            &( (*pdest_iAP2DeviceLanguageUpdateParameter)->iAP2DeviceName_count ),
                                           psrc_iAP2DeviceLanguageUpdateParameter->iAP2DeviceName_count, iAP2_utf8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2StartExternalAccessoryProtocolSessionParameter(iAP2StartExternalAccessoryProtocolSessionParameter** pdest_iAP2StartExternalAccessoryProtocolSessionParameter, iAP2StartExternalAccessoryProtocolSessionParameter* psrc_iAP2StartExternalAccessoryProtocolSessionParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2StartExternalAccessoryProtocolSessionParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2StartExternalAccessoryProtocolSessionParameter = calloc(1, sizeof(iAP2StartExternalAccessoryProtocolSessionParameter) );
        if(*pdest_iAP2StartExternalAccessoryProtocolSessionParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartExternalAccessoryProtocolSessionParameter->iAP2ExternalAccesoryProtocolIdentifier_count > 0) )
        {
            printf("\nExternalAccesoryProtocolIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2StartExternalAccessoryProtocolSessionParameter)->iAP2ExternalAccesoryProtocolIdentifier ),
                                           psrc_iAP2StartExternalAccessoryProtocolSessionParameter->iAP2ExternalAccesoryProtocolIdentifier,
                                            &( (*pdest_iAP2StartExternalAccessoryProtocolSessionParameter)->iAP2ExternalAccesoryProtocolIdentifier_count ),
                                           psrc_iAP2StartExternalAccessoryProtocolSessionParameter->iAP2ExternalAccesoryProtocolIdentifier_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartExternalAccessoryProtocolSessionParameter->iAP2ExternalAccessoryProtocolSessionIdentifier_count > 0) )
        {
            printf("\nExternalAccessoryProtocolSessionIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2StartExternalAccessoryProtocolSessionParameter)->iAP2ExternalAccessoryProtocolSessionIdentifier ),
                                           psrc_iAP2StartExternalAccessoryProtocolSessionParameter->iAP2ExternalAccessoryProtocolSessionIdentifier,
                                            &( (*pdest_iAP2StartExternalAccessoryProtocolSessionParameter)->iAP2ExternalAccessoryProtocolSessionIdentifier_count ),
                                           psrc_iAP2StartExternalAccessoryProtocolSessionParameter->iAP2ExternalAccessoryProtocolSessionIdentifier_count, iAP2_uint16);
        }
    }

    return rc;
}

S32 iap2CopyiAP2StopExternalAccessoryProtocolSessionParameter(iAP2StopExternalAccessoryProtocolSessionParameter** pdest_iAP2StopExternalAccessoryProtocolSessionParameter, iAP2StopExternalAccessoryProtocolSessionParameter* psrc_iAP2StopExternalAccessoryProtocolSessionParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2StopExternalAccessoryProtocolSessionParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2StopExternalAccessoryProtocolSessionParameter = calloc(1, sizeof(iAP2StopExternalAccessoryProtocolSessionParameter) );
        if(*pdest_iAP2StopExternalAccessoryProtocolSessionParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StopExternalAccessoryProtocolSessionParameter->iAP2ExternalAccessoryProtocolSessionIdentifier_count > 0) )
        {
            printf("\nExternalAccessoryProtocolSessionIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2StopExternalAccessoryProtocolSessionParameter)->iAP2ExternalAccessoryProtocolSessionIdentifier ),
                                           psrc_iAP2StopExternalAccessoryProtocolSessionParameter->iAP2ExternalAccessoryProtocolSessionIdentifier,
                                            &( (*pdest_iAP2StopExternalAccessoryProtocolSessionParameter)->iAP2ExternalAccessoryProtocolSessionIdentifier_count ),
                                           psrc_iAP2StopExternalAccessoryProtocolSessionParameter->iAP2ExternalAccessoryProtocolSessionIdentifier_count, iAP2_uint16);
        }
    }

    return rc;
}

S32 iap2CopyiAP2DeviceHIDReportParameter(iAP2DeviceHIDReportParameter** pdest_iAP2DeviceHIDReportParameter, iAP2DeviceHIDReportParameter* psrc_iAP2DeviceHIDReportParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2DeviceHIDReportParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2DeviceHIDReportParameter = calloc(1, sizeof(iAP2DeviceHIDReportParameter) );
        if(*pdest_iAP2DeviceHIDReportParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2DeviceHIDReportParameter->iAP2HIDComponentIdentifier_count > 0) )
        {
            printf("\nHIDComponentIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2DeviceHIDReportParameter)->iAP2HIDComponentIdentifier ),
                                           psrc_iAP2DeviceHIDReportParameter->iAP2HIDComponentIdentifier,
                                            &( (*pdest_iAP2DeviceHIDReportParameter)->iAP2HIDComponentIdentifier_count ),
                                           psrc_iAP2DeviceHIDReportParameter->iAP2HIDComponentIdentifier_count, iAP2_uint16);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2DeviceHIDReportParameter->iAP2HIDReport_count > 0) )
        {
            printf("\nHIDReport");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2DeviceHIDReportParameter)->iAP2HIDReport ),
                                           psrc_iAP2DeviceHIDReportParameter->iAP2HIDReport,
                                            &( (*pdest_iAP2DeviceHIDReportParameter)->iAP2HIDReport_count ),
                                           psrc_iAP2DeviceHIDReportParameter->iAP2HIDReport_count, iAP2_blob);
        }
    }

    return rc;
}

S32 iap2CopyiAP2StartLocationInformationParameter(iAP2StartLocationInformationParameter** pdest_iAP2StartLocationInformationParameter, iAP2StartLocationInformationParameter* psrc_iAP2StartLocationInformationParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2StartLocationInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2StartLocationInformationParameter = calloc(1, sizeof(iAP2StartLocationInformationParameter) );
        if(*pdest_iAP2StartLocationInformationParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartLocationInformationParameter->iAP2GlobalPositioningSystemFixData_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2GlobalPositioningSystemFixData_count = %d", (*pdest_iAP2StartLocationInformationParameter)->iAP2GlobalPositioningSystemFixData_count);
            (*pdest_iAP2StartLocationInformationParameter)->iAP2GlobalPositioningSystemFixData_count = psrc_iAP2StartLocationInformationParameter->iAP2GlobalPositioningSystemFixData_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartLocationInformationParameter->iAP2RecommendedMinimumSpecificGPSTransitData_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2RecommendedMinimumSpecificGPSTransitData_count = %d", (*pdest_iAP2StartLocationInformationParameter)->iAP2RecommendedMinimumSpecificGPSTransitData_count);
            (*pdest_iAP2StartLocationInformationParameter)->iAP2RecommendedMinimumSpecificGPSTransitData_count = psrc_iAP2StartLocationInformationParameter->iAP2RecommendedMinimumSpecificGPSTransitData_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartLocationInformationParameter->iAP2GPSSatellitesInView_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2GPSSatellitesInView_count = %d", (*pdest_iAP2StartLocationInformationParameter)->iAP2GPSSatellitesInView_count);
            (*pdest_iAP2StartLocationInformationParameter)->iAP2GPSSatellitesInView_count = psrc_iAP2StartLocationInformationParameter->iAP2GPSSatellitesInView_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartLocationInformationParameter->iAP2VehicleSpeedData_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2VehicleSpeedData_count = %d", (*pdest_iAP2StartLocationInformationParameter)->iAP2VehicleSpeedData_count);
            (*pdest_iAP2StartLocationInformationParameter)->iAP2VehicleSpeedData_count = psrc_iAP2StartLocationInformationParameter->iAP2VehicleSpeedData_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartLocationInformationParameter->iAP2VehicleGyroData_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2VehicleGyroData_count = %d", (*pdest_iAP2StartLocationInformationParameter)->iAP2VehicleGyroData_count);
            (*pdest_iAP2StartLocationInformationParameter)->iAP2VehicleGyroData_count = psrc_iAP2StartLocationInformationParameter->iAP2VehicleGyroData_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartLocationInformationParameter->iAP2VehicleAccelerometerData_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2VehicleAccelerometerData_count = %d", (*pdest_iAP2StartLocationInformationParameter)->iAP2VehicleAccelerometerData_count);
            (*pdest_iAP2StartLocationInformationParameter)->iAP2VehicleAccelerometerData_count = psrc_iAP2StartLocationInformationParameter->iAP2VehicleAccelerometerData_count;
        }
    }

    return rc;
}

S32 iap2CopyiAP2MediaLibraryInformationParameter(iAP2MediaLibraryInformationParameter** pdest_iAP2MediaLibraryInformationParameter, iAP2MediaLibraryInformationParameter* psrc_iAP2MediaLibraryInformationParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaLibraryInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2MediaLibraryInformationParameter = calloc(1, sizeof(iAP2MediaLibraryInformationParameter) );
        if(*pdest_iAP2MediaLibraryInformationParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryInformationParameter->iAP2MediaLibraryInformationSubParameter_count > 0) )
        {
            (*pdest_iAP2MediaLibraryInformationParameter)->iAP2MediaLibraryInformationSubParameter = calloc(psrc_iAP2MediaLibraryInformationParameter->iAP2MediaLibraryInformationSubParameter_count, sizeof(iAP2MediaLibraryInformationSubParameter) );
            if( (*pdest_iAP2MediaLibraryInformationParameter)->iAP2MediaLibraryInformationSubParameter == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                rc = iap2CopyiAP2MediaLibraryInformationSubParameter( &( (*pdest_iAP2MediaLibraryInformationParameter)->iAP2MediaLibraryInformationSubParameter ), psrc_iAP2MediaLibraryInformationParameter->iAP2MediaLibraryInformationSubParameter);
            }
        }
    }

    return rc;
}

S32 iap2CopyiAP2MediaLibraryUpdateParameter(iAP2MediaLibraryUpdateParameter** pdest_iAP2MediaLibraryUpdateParameter, iAP2MediaLibraryUpdateParameter* psrc_iAP2MediaLibraryUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2MediaLibraryUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2MediaLibraryUpdateParameter = calloc(1, sizeof(iAP2MediaLibraryUpdateParameter) );
        if(*pdest_iAP2MediaLibraryUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaItem_count > 0) )
        {
            (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaItem = calloc(psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaItem_count, sizeof(iAP2MediaItem) );
            if( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaItem == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                rc = iap2CopyiAP2MediaItem( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaItem ), psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaItem);
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier_count > 0) )
        {
            printf("\nMediaItemDeletePersistentIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaItemDeletePersistentIdentifier ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier,
                                            &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaItemDeletePersistentIdentifier_count ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaItemDeletePersistentIdentifier_count, iAP2_uint64);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryIsHidingRemoteItems_count > 0) )
        {
            printf("\nMediaLibraryIsHidingRemoteItems");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryIsHidingRemoteItems ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryIsHidingRemoteItems,
                                            &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryIsHidingRemoteItems_count ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryIsHidingRemoteItems_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryReset_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2MediaLibraryReset_count = %d", (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryReset_count);
            (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryReset_count = psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryReset_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryRevision_count > 0) )
        {
            printf("\nMediaLibraryRevision");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryRevision ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryRevision,
                                            &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryRevision_count ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryRevision_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier_count > 0) )
        {
            printf("\nMediaLibraryUniqueIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryUniqueIdentifier ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier,
                                            &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryUniqueIdentifier_count ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count > 0) )
        {
            printf("\nMediaLibraryUpdateProgress");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryUpdateProgress ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress,
                                            &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaLibraryUpdateProgress_count ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaLibraryUpdateProgress_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaPlayList_count > 0) )
        {
            (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaPlayList = calloc(psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaPlayList_count, sizeof(iAP2MediaPlayList) );
            if( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaPlayList == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                rc = iap2CopyiAP2MediaPlayList( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaPlayList ), psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaPlayList);
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaPlaylistDeletePersistentIdentifier_count > 0) )
        {
            printf("\nMediaPlaylistDeletePersistentIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaPlaylistDeletePersistentIdentifier ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaPlaylistDeletePersistentIdentifier,
                                            &( (*pdest_iAP2MediaLibraryUpdateParameter)->iAP2MediaPlaylistDeletePersistentIdentifier_count ),
                                           psrc_iAP2MediaLibraryUpdateParameter->iAP2MediaPlaylistDeletePersistentIdentifier_count, iAP2_uint64);
        }
    }

    return rc;
}

S32 iap2CopyiAP2NowPlayingUpdateParameter(iAP2NowPlayingUpdateParameter** pdest_iAP2NowPlayingUpdateParameter, iAP2NowPlayingUpdateParameter* psrc_iAP2NowPlayingUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2NowPlayingUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2NowPlayingUpdateParameter = calloc(1, sizeof(iAP2NowPlayingUpdateParameter) );
        if(*pdest_iAP2NowPlayingUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2NowPlayingUpdateParameter->iAP2MediaItemAttributes_count > 0) )
        {
            (*pdest_iAP2NowPlayingUpdateParameter)->iAP2MediaItemAttributes = calloc(psrc_iAP2NowPlayingUpdateParameter->iAP2MediaItemAttributes_count, sizeof(iAP2MediaItem) );
            if( (*pdest_iAP2NowPlayingUpdateParameter)->iAP2MediaItemAttributes == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                rc = iap2CopyiAP2MediaItem( &( (*pdest_iAP2NowPlayingUpdateParameter)->iAP2MediaItemAttributes ), psrc_iAP2NowPlayingUpdateParameter->iAP2MediaItemAttributes);
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2NowPlayingUpdateParameter->iAP2PlaybackAttributes_count > 0) )
        {
            (*pdest_iAP2NowPlayingUpdateParameter)->iAP2PlaybackAttributes = calloc(psrc_iAP2NowPlayingUpdateParameter->iAP2PlaybackAttributes_count, sizeof(iAP2PlaybackAttributes) );
            if( (*pdest_iAP2NowPlayingUpdateParameter)->iAP2PlaybackAttributes == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                rc = iap2CopyiAP2PlaybackAttributes( &( (*pdest_iAP2NowPlayingUpdateParameter)->iAP2PlaybackAttributes ), psrc_iAP2NowPlayingUpdateParameter->iAP2PlaybackAttributes);
            }
        }
    }

    return rc;
}

S32 iap2CopyiAP2PowerUpdateParameter(iAP2PowerUpdateParameter** pdest_iAP2PowerUpdateParameter, iAP2PowerUpdateParameter* psrc_iAP2PowerUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2PowerUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2PowerUpdateParameter = calloc(1, sizeof(iAP2PowerUpdateParameter) );
        if(*pdest_iAP2PowerUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2PowerUpdateParameter->iAP2MaximumCurrentDrawnFromAccessory_count > 0) )
        {
            printf("\nMaximumCurrentDrawnFromAccessory");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PowerUpdateParameter)->iAP2MaximumCurrentDrawnFromAccessory ),
                                           psrc_iAP2PowerUpdateParameter->iAP2MaximumCurrentDrawnFromAccessory,
                                            &( (*pdest_iAP2PowerUpdateParameter)->iAP2MaximumCurrentDrawnFromAccessory_count ),
                                           psrc_iAP2PowerUpdateParameter->iAP2MaximumCurrentDrawnFromAccessory_count, iAP2_uint16);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2PowerUpdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count > 0) )
        {
            printf("\nDeviceBatteryWillChargeIfPowerIsPresent");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2PowerUpdateParameter)->iAP2DeviceBatteryWillChargeIfPowerIsPresent ),
                                           psrc_iAP2PowerUpdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent,
                                            &( (*pdest_iAP2PowerUpdateParameter)->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count ),
                                           psrc_iAP2PowerUpdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2PowerUpdateParameter->iAP2AccessoryPowerMode_count > 0) )
        {
            (*pdest_iAP2PowerUpdateParameter)->iAP2AccessoryPowerMode = calloc(psrc_iAP2PowerUpdateParameter->iAP2AccessoryPowerMode_count, sizeof(iAP2AccessoryPowerModes) );
            if( (*pdest_iAP2PowerUpdateParameter)->iAP2AccessoryPowerMode == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2PowerUpdateParameter->iAP2AccessoryPowerMode_count; count++)
                {
                    memcpy( &( (*pdest_iAP2PowerUpdateParameter)->iAP2AccessoryPowerMode[count] ), &psrc_iAP2PowerUpdateParameter->iAP2AccessoryPowerMode[count], sizeof(iAP2AccessoryPowerModes) );
                    printf("\nAccessoryPowerMode[%d] = %d", count, (*pdest_iAP2PowerUpdateParameter)->iAP2AccessoryPowerMode[count]);
                }
            }
        }
    }

    return rc;
}

S32 iap2CopyiAP2TelephonyCallStateInformationParameter(iAP2TelephonyCallStateInformationParameter** pdest_iAP2TelephonyCallStateInformationParameter, iAP2TelephonyCallStateInformationParameter* psrc_iAP2TelephonyCallStateInformationParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2TelephonyCallStateInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2TelephonyCallStateInformationParameter = calloc(1, sizeof(iAP2TelephonyCallStateInformationParameter) );
        if(*pdest_iAP2TelephonyCallStateInformationParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStatePhoneNumber_count > 0) )
        {
            printf("\nCallStatePhoneNumber");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStatePhoneNumber ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStatePhoneNumber,
                                            &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStatePhoneNumber_count ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStatePhoneNumber_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateCallerName_count > 0) )
        {
            printf("\nCallStateCallerName");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateCallerName ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateCallerName,
                                            &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateCallerName_count ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateCallerName_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateStatus_count > 0) )
        {
            (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateStatus = calloc(psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateStatus_count, sizeof(iAP2CallStateStatusValue) );
            if( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateStatus == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateStatus_count; count++)
                {
                    memcpy( &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateStatus[count] ), &psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateStatus[count], sizeof(iAP2CallStateStatusValue) );
                    printf("\nCallStateStatus[%d] = %d", count, (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateStatus[count]);
                }
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateDirection_count > 0) )
        {
            (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateDirection = calloc(psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateDirection_count, sizeof(iAP2CallStateDirectionValue) );
            if( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateDirection == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateDirection_count; count++)
                {
                    memcpy( &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateDirection[count] ), &psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStateDirection[count], sizeof(iAP2CallStateDirectionValue) );
                    printf("\nCallStateDirection[%d] = %d", count, (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStateDirection[count]);
                }
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyCallStateInformationParameter->iAP2UniqueCallID_count > 0) )
        {
            printf("\nUniqueCallID");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2UniqueCallID ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2UniqueCallID,
                                            &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2UniqueCallID_count ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2UniqueCallID_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStatevCardFileTransferIdentifier_count > 0) )
        {
            printf("\nCallStatevCardFileTransferIdentifier");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStatevCardFileTransferIdentifier ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStatevCardFileTransferIdentifier,
                                            &( (*pdest_iAP2TelephonyCallStateInformationParameter)->iAP2CallStatevCardFileTransferIdentifier_count ),
                                           psrc_iAP2TelephonyCallStateInformationParameter->iAP2CallStatevCardFileTransferIdentifier_count, iAP2_uint8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2TelephonyUpdateParameter(iAP2TelephonyUpdateParameter** pdest_iAP2TelephonyUpdateParameter, iAP2TelephonyUpdateParameter* psrc_iAP2TelephonyUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2TelephonyUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2TelephonyUpdateParameter = calloc(1, sizeof(iAP2TelephonyUpdateParameter) );
        if(*pdest_iAP2TelephonyUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyUpdateParameter->iAP2TelephonySignalStrength_count > 0) )
        {
            (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonySignalStrength = calloc(psrc_iAP2TelephonyUpdateParameter->iAP2TelephonySignalStrength_count, sizeof(iAP2TelephonySignalStrengthValue) );
            if( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonySignalStrength == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2TelephonyUpdateParameter->iAP2TelephonySignalStrength_count; count++)
                {
                    memcpy( &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonySignalStrength[count] ), &psrc_iAP2TelephonyUpdateParameter->iAP2TelephonySignalStrength[count], sizeof(iAP2TelephonySignalStrengthValue) );
                    printf("\nTelephonySignalStrength[%d] = %d", count, (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonySignalStrength[count]);
                }
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyRegistrationStatus_count > 0) )
        {
            (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyRegistrationStatus = calloc(psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyRegistrationStatus_count, sizeof(iAP2TelephonyRegistrationStatusValue) );
            if( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyRegistrationStatus == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyRegistrationStatus_count; count++)
                {
                    memcpy( &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyRegistrationStatus[count] ), &psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyRegistrationStatus[count], sizeof(iAP2TelephonyRegistrationStatusValue) );
                    printf("\nTelephonyRegistrationStatus[%d] = %d", count, (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyRegistrationStatus[count]);
                }
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyAirplaneModeStatus_count > 0) )
        {
            printf("\nTelephonyAirplaneModeStatus");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyAirplaneModeStatus ),
                                           psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyAirplaneModeStatus,
                                            &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyAirplaneModeStatus_count ),
                                           psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyAirplaneModeStatus_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyTTYStatus_count > 0) )
        {
            printf("\nTelephonyTTYStatus");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyTTYStatus ),
                                           psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyTTYStatus,
                                            &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyTTYStatus_count ),
                                           psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyTTYStatus_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyMobileOperator_count > 0) )
        {
            printf("\nTelephonyMobileOperator");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyMobileOperator ),
                                           psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyMobileOperator,
                                            &( (*pdest_iAP2TelephonyUpdateParameter)->iAP2TelephonyMobileOperator_count ),
                                           psrc_iAP2TelephonyUpdateParameter->iAP2TelephonyMobileOperator_count, iAP2_utf8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2USBDeviceModeAudioInformationParameter(iAP2USBDeviceModeAudioInformationParameter** pdest_iAP2USBDeviceModeAudioInformationParameter, iAP2USBDeviceModeAudioInformationParameter* psrc_iAP2USBDeviceModeAudioInformationParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2USBDeviceModeAudioInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2USBDeviceModeAudioInformationParameter = calloc(1, sizeof(iAP2USBDeviceModeAudioInformationParameter) );
        if(*pdest_iAP2USBDeviceModeAudioInformationParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate_count > 0) )
        {
            (*pdest_iAP2USBDeviceModeAudioInformationParameter)->iAP2SampleRate = calloc(psrc_iAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate_count, sizeof(iAP2USBDeviceModeAudioSampleRate) );
            if( (*pdest_iAP2USBDeviceModeAudioInformationParameter)->iAP2SampleRate == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate_count; count++)
                {
                    memcpy( &( (*pdest_iAP2USBDeviceModeAudioInformationParameter)->iAP2SampleRate[count] ), &psrc_iAP2USBDeviceModeAudioInformationParameter->iAP2SampleRate[count], sizeof(iAP2USBDeviceModeAudioSampleRate) );
                    printf("\nSampleRate[%d] = %d", count, (*pdest_iAP2USBDeviceModeAudioInformationParameter)->iAP2SampleRate[count]);
                }
            }
        }
    }

    return rc;
}

S32 iap2CopyiAP2StartVehicleStatusUpdatesParameter(iAP2StartVehicleStatusUpdatesParameter** pdest_iAP2StartVehicleStatusUpdatesParameter, iAP2StartVehicleStatusUpdatesParameter* psrc_iAP2StartVehicleStatusUpdatesParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2StartVehicleStatusUpdatesParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2StartVehicleStatusUpdatesParameter = calloc(1, sizeof(iAP2StartVehicleStatusUpdatesParameter) );
        if(*pdest_iAP2StartVehicleStatusUpdatesParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2NightMode_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2NightMode_count = %d", (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2NightMode_count);
            (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2NightMode_count = psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2NightMode_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2Range_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2Range_count = %d", (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2Range_count);
            (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2Range_count = psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2Range_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2InsideTemperature_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2InsideTemperature_count = %d", (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2InsideTemperature_count);
            (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2InsideTemperature_count = psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2InsideTemperature_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2OutsideTemperature_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2OutsideTemperature_count = %d", (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2OutsideTemperature_count);
            (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2OutsideTemperature_count = psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2OutsideTemperature_count;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2RangeWarning_count > 0) )
        {
            /* For type none just update the count value */
            printf("\niAP2RangeWarning_count = %d", (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2RangeWarning_count);
            (*pdest_iAP2StartVehicleStatusUpdatesParameter)->iAP2RangeWarning_count = psrc_iAP2StartVehicleStatusUpdatesParameter->iAP2RangeWarning_count;
        }
    }

    return rc;
}

S32 iap2CopyiAP2VoiceOverCursorUpdateParameter(iAP2VoiceOverCursorUpdateParameter** pdest_iAP2VoiceOverCursorUpdateParameter, iAP2VoiceOverCursorUpdateParameter* psrc_iAP2VoiceOverCursorUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2VoiceOverCursorUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2VoiceOverCursorUpdateParameter = calloc(1, sizeof(iAP2VoiceOverCursorUpdateParameter) );
        if(*pdest_iAP2VoiceOverCursorUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverHint_count > 0) )
        {
            printf("\nVoiceOverHint");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverHint ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverHint,
                                            &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverHint_count ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverHint_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverLabel_count > 0) )
        {
            printf("\nVoiceOverLabel");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverLabel ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverLabel,
                                            &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverLabel_count ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverLabel_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverTraits_count > 0) )
        {
            printf("\nVoiceOverTraits");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverTraits ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverTraits,
                                            &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverTraits_count ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverTraits_count, iAP2_blob);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverValue_count > 0) )
        {
            printf("\nVoiceOverValue");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverValue ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverValue,
                                            &( (*pdest_iAP2VoiceOverCursorUpdateParameter)->iAP2VoiceOverValue_count ),
                                           psrc_iAP2VoiceOverCursorUpdateParameter->iAP2VoiceOverValue_count, iAP2_utf8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2VoiceOverUpdateParameter(iAP2VoiceOverUpdateParameter** pdest_iAP2VoiceOverUpdateParameter, iAP2VoiceOverUpdateParameter* psrc_iAP2VoiceOverUpdateParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2VoiceOverUpdateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2VoiceOverUpdateParameter = calloc(1, sizeof(iAP2VoiceOverUpdateParameter) );
        if(*pdest_iAP2VoiceOverUpdateParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverEnabled_count > 0) )
        {
            printf("\nVoiceOverEnabled");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverUpdateParameter)->iAP2VoiceOverEnabled ),
                                           psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverEnabled,
                                            &( (*pdest_iAP2VoiceOverUpdateParameter)->iAP2VoiceOverEnabled_count ),
                                           psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverEnabled_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverSpeakingRate_count > 0) )
        {
            printf("\nVoiceOverSpeakingRate");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverUpdateParameter)->iAP2VoiceOverSpeakingRate ),
                                           psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverSpeakingRate,
                                            &( (*pdest_iAP2VoiceOverUpdateParameter)->iAP2VoiceOverSpeakingRate_count ),
                                           psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverSpeakingRate_count, iAP2_uint8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverSpeakingVolume_count > 0) )
        {
            printf("\nVoiceOverSpeakingVolume");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2VoiceOverUpdateParameter)->iAP2VoiceOverSpeakingVolume ),
                                           psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverSpeakingVolume,
                                            &( (*pdest_iAP2VoiceOverUpdateParameter)->iAP2VoiceOverSpeakingVolume_count ),
                                           psrc_iAP2VoiceOverUpdateParameter->iAP2VoiceOverSpeakingVolume_count, iAP2_uint8);
        }
    }

    return rc;
}

S32 iap2CopyiAP2WiFiInformationParameter(iAP2WiFiInformationParameter** pdest_iAP2WiFiInformationParameter, iAP2WiFiInformationParameter* psrc_iAP2WiFiInformationParameter)
{
    S32 rc = IAP2_OK;

    if(psrc_iAP2WiFiInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
    }
    if(rc == IAP2_OK)
    {
        *pdest_iAP2WiFiInformationParameter = calloc(1, sizeof(iAP2WiFiInformationParameter) );
        if(*pdest_iAP2WiFiInformationParameter == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if( (rc == IAP2_OK) && (psrc_iAP2WiFiInformationParameter->iAP2RequestStatus_count > 0) )
        {
            (*pdest_iAP2WiFiInformationParameter)->iAP2RequestStatus = calloc(psrc_iAP2WiFiInformationParameter->iAP2RequestStatus_count, sizeof(iAP2WiFiRequestStatus) );
            if( (*pdest_iAP2WiFiInformationParameter)->iAP2RequestStatus == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                U16 count = 0;

                for(count = 0; count < psrc_iAP2WiFiInformationParameter->iAP2RequestStatus_count; count++)
                {
                    memcpy( &( (*pdest_iAP2WiFiInformationParameter)->iAP2RequestStatus[count] ), &psrc_iAP2WiFiInformationParameter->iAP2RequestStatus[count], sizeof(iAP2WiFiRequestStatus) );
                    printf("\nRequestStatus[%d] = %d", count, (*pdest_iAP2WiFiInformationParameter)->iAP2RequestStatus[count]);
                }
            }
        }
        if( (rc == IAP2_OK) && (psrc_iAP2WiFiInformationParameter->iAP2WiFiSSID_count > 0) )
        {
            printf("\nWiFiSSID");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2WiFiInformationParameter)->iAP2WiFiSSID ),
                                           psrc_iAP2WiFiInformationParameter->iAP2WiFiSSID,
                                            &( (*pdest_iAP2WiFiInformationParameter)->iAP2WiFiSSID_count ),
                                           psrc_iAP2WiFiInformationParameter->iAP2WiFiSSID_count, iAP2_utf8);
        }
        if( (rc == IAP2_OK) && (psrc_iAP2WiFiInformationParameter->iAP2WiFiPassphrase_count > 0) )
        {
            printf("\nWiFiPassphrase");
            rc = iap2AllocateandUpdateData( &( (*pdest_iAP2WiFiInformationParameter)->iAP2WiFiPassphrase ),
                                           psrc_iAP2WiFiInformationParameter->iAP2WiFiPassphrase,
                                            &( (*pdest_iAP2WiFiInformationParameter)->iAP2WiFiPassphrase_count ),
                                           psrc_iAP2WiFiInformationParameter->iAP2WiFiPassphrase_count, iAP2_utf8);
        }
    }

    return rc;
}

