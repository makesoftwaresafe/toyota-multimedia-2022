/************************************************************************//**
*\file : iap2_message_response.c
*Contains the source code implementation for parsing the messages sent by
*Contains Apple Device
*\version : $Id: iap2_message_response.c, v Exp $
*\release : $Name:$
*\component :
*\author : Konrad Gerhards/ADITG/ kgerhards@de.adit-jv.com
*\copyright (c) 2010 - 2013 Advanced Driver Information Technology.
*          This code is developed by Advanced Driver Information Technology.
*          Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
*          All rights reserved.
*****************************************************************************/

#include "iap2_message_response.h"

#include "iap2_dlt_log.h"
#include "iap2_utility.h"
#include "authentication.h"


S32 iAP2GenerateiAP2RequestAuthenticationCertificateResponse(iAP2RequestAuthenticationCertificateParameter theiAP2RequestAuthenticationCertificateParameter, iAP2AuthenticationCertificateParameter* theiAP2AuthenticationCertificateParameter, iAP2AccessoryAuthenticationSerialNumberParameter* theiAP2AuthenticationCertificateSerialNumberParameter)
{
    S32 rc = IAP2_OK;

    if(theiAP2AuthenticationCertificateParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter");
    }
    else if(theiAP2RequestAuthenticationCertificateParameter.iAP2RequestAuthenticationCertificateSerialNumber_count == 0)
    {
        U16 X509_Certificate_Len   = (U16)IAP2_INITIALIZE_TO_ZERO;
        U8  X509_Certificate[IPOD_AUTH_CP_MAX_CERTLENGTH] = {IAP2_INITIALIZE_TO_ZERO};

        memset(theiAP2AuthenticationCertificateParameter, 0, sizeof(iAP2AuthenticationCertificateParameter));
        AuthenticationGetCertificate(&X509_Certificate_Len, X509_Certificate);
        if(X509_Certificate_Len > 0)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE, "iAP2 Authentication certificate Length: %X", X509_Certificate_Len);
            rc = iAP2AllocateandUpdateData(&theiAP2AuthenticationCertificateParameter->iAP2AuthenticationCertificate,
                                           X509_Certificate,
                                           &theiAP2AuthenticationCertificateParameter->iAP2AuthenticationCertificate_count,
                                           X509_Certificate_Len,
                                           iAP2_blob, 0);
        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Authentication Certificate");
            rc = IAP2_CTL_ERROR;
        }       
    }
    else if(theiAP2RequestAuthenticationCertificateParameter.iAP2RequestAuthenticationCertificateSerialNumber_count > 0)
    {
        U8 CertificateSerialNumber[IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE];

        memset(CertificateSerialNumber, 0, IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE);
        rc = AuthenticationGetSerialNumber(CertificateSerialNumber);
        if(rc == IAP2_OK)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE, "iAP2 Authentication certificate Serial Number length: %d %s", (int)strnlen((char*)CertificateSerialNumber, IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE),CertificateSerialNumber);
            rc = iAP2AllocateandUpdateData(&theiAP2AuthenticationCertificateSerialNumberParameter->iAP2AuthenticationSerialNumber,
                                           CertificateSerialNumber,
                                           &theiAP2AuthenticationCertificateSerialNumberParameter->iAP2AuthenticationSerialNumber_count,
                                           strnlen((char*)CertificateSerialNumber, IPOD_AUTH_CP_SERIALNUMBER_MAX_SIZE),
                                           iAP2_blob, 0);
        }
        else if(rc > IAP2_OK)
        {
            /* This indicates the condition for Copro 2.0B where serial number will not be available
             * so send the certifiate */

            U16 X509_Certificate_Len   = (U16)IAP2_INITIALIZE_TO_ZERO;
            U8  X509_Certificate[IPOD_AUTH_CP_MAX_CERTLENGTH] = {IAP2_INITIALIZE_TO_ZERO};
            int rc_tmp = rc;

            memset(theiAP2AuthenticationCertificateParameter, 0, sizeof(iAP2AuthenticationCertificateParameter));
            AuthenticationGetCertificate(&X509_Certificate_Len, X509_Certificate);
            if(X509_Certificate_Len > 0)
            {
               IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE, "iAP2 Authentication certificate Length: %X", X509_Certificate_Len);
               rc = iAP2AllocateandUpdateData(&theiAP2AuthenticationCertificateParameter->iAP2AuthenticationCertificate,
                                              X509_Certificate,
                                              &theiAP2AuthenticationCertificateParameter->iAP2AuthenticationCertificate_count,
                                              X509_Certificate_Len,
                                              iAP2_blob, 0);
               if(rc == IAP2_OK)
               {
                   rc = rc_tmp;
               }
            }
            else
            {
               IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Authentication Certificate");
               rc = IAP2_CTL_ERROR;
            }

        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Authentication Certificate");
            rc = IAP2_CTL_ERROR;
        }
    }
    else
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter");
    }
    return rc;
}


S32 iAP2GenerateiAP2RequestAuthenticationChallengeResponseResponse(iAP2RequestAuthenticationChallengeResponseParameter theiAP2RequestAuthenticationChallengeResponseParameter, iAP2AuthenticationResponseParameter* theiAP2AuthenticationResponseParameter)
{
    S32 rc = IAP2_OK;

    if( (theiAP2AuthenticationResponseParameter == NULL) || (theiAP2RequestAuthenticationChallengeResponseParameter.iAP2AuthenticationChallenge[0].iAP2BlobData == NULL) )
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter. theiAP2AuthenticationResponseParameter = %p, AuthenticationResponse data: %p", theiAP2AuthenticationResponseParameter,    \
                    theiAP2RequestAuthenticationChallengeResponseParameter.iAP2AuthenticationChallenge[0].iAP2BlobData);
    }
    else
    {
        U16 ChallengeResponseLength              = (U16)IAP2_INITIALIZE_TO_ZERO;
        U8  AuthenticationChallengeResponse[128] = {IAP2_INITIALIZE_TO_ZERO};
        S32 ret = IAP2_OK;

        memset(theiAP2AuthenticationResponseParameter, 0, sizeof(iAP2AuthenticationResponseParameter));

        ret = AuthenticationGetSignatureData(theiAP2RequestAuthenticationChallengeResponseParameter.iAP2AuthenticationChallenge[0].iAP2BlobData,
                                       theiAP2RequestAuthenticationChallengeResponseParameter.iAP2AuthenticationChallenge[0].iAP2BlobLength,
                                       &ChallengeResponseLength,
                                       AuthenticationChallengeResponse);

        if( (ChallengeResponseLength != 0) && (ret == IPOD_AUTH_OK) && (ChallengeResponseLength <= (sizeof(AuthenticationChallengeResponse))) )
        {
            IAP2SESSIONDLTLOG(DLT_LOG_VERBOSE, "iAP2 Authentication Challenge Response Length: %X", ChallengeResponseLength);
            rc = iAP2AllocateandUpdateData(&theiAP2AuthenticationResponseParameter->iAP2AuthenticationResponse,
                                           AuthenticationChallengeResponse,
                                           &theiAP2AuthenticationResponseParameter->iAP2AuthenticationResponse_count,
                                           ChallengeResponseLength,
                                           iAP2_blob, 0);
        }
        else
        {
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Authentication Challenge Response Length:%#X", ChallengeResponseLength);
            rc = IAP2_CTL_ERROR;
        }
    }

    return rc;
}


S32 iAP2GenerateiAP2StartIdentificationResponse(iAP2Device_st* this_iAP2Device, iAP2IdentificationInformationParameter* theiAP2IdentificationInformationParameter)
{
    S32 rc = IAP2_OK;

    if(theiAP2IdentificationInformationParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter. DevID:%p", this_iAP2Device);
    }
    else
    {
        memset(theiAP2IdentificationInformationParameter, 0, sizeof(iAP2IdentificationInformationParameter));
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryName != NULL)
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AccessoryName,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryName,
                                           &theiAP2IdentificationInformationParameter->iAP2AccessoryName_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryModelIdentifier != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AccessoryModelIdentifier,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryModelIdentifier,
                                           &theiAP2IdentificationInformationParameter->iAP2AccessoryModelIdentifier_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryManufacturer != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AccessoryManufacturer,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryManufacturer,
                                           &theiAP2IdentificationInformationParameter->iAP2AccessoryManufacturer_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2AccessorySerialNumber != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AccessorySerialNumber,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2AccessorySerialNumber,
                                           &theiAP2IdentificationInformationParameter->iAP2AccessorySerialNumber_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryFirmwareVersion != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AccessoryFirmwareVersion,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryFirmwareVersion,
                                           &theiAP2IdentificationInformationParameter->iAP2AccessoryFirmwareVersion_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryHardwareVersion != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AccessoryHardwareVersion,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryHardwareVersion,
                                           &theiAP2IdentificationInformationParameter->iAP2AccessoryHardwareVersion_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2MessagesSentByAccessory,
                                           this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication,
                                           &theiAP2IdentificationInformationParameter->iAP2MessagesSentByAccessory_count,
                                           this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication_length, iAP2_blob, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2MessagesRecievedfromDevice,
                                           this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice,
                                           &theiAP2IdentificationInformationParameter->iAP2MessagesRecievedfromDevice_count,
                                           this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice_length, iAP2_blob, 0);
        }

        if(rc == IAP2_OK)
        {
            theiAP2IdentificationInformationParameter->iAP2PowerProvidingCapability = calloc(1, sizeof(iAP2PowerProvidingCapability));
            if(theiAP2IdentificationInformationParameter->iAP2PowerProvidingCapability != NULL)
            {
                if( (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2USBDEVICEMODE) ||
                    (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2USBHOSTMODE) || (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2MULTIHOSTMODE))
                {
                    *(theiAP2IdentificationInformationParameter->iAP2PowerProvidingCapability) = IAP2_POWER_ADVANCED;
                }
                else
                {
                    *(theiAP2IdentificationInformationParameter->iAP2PowerProvidingCapability) = IAP2_POWER_NONE;
                }
                theiAP2IdentificationInformationParameter->iAP2PowerProvidingCapability_count++;
            }
        }

        if(rc == IAP2_OK)
        {
            theiAP2IdentificationInformationParameter->iAP2MaximumCurrentDrawnFromDevice = calloc(1, sizeof(U16*));
            if(theiAP2IdentificationInformationParameter->iAP2MaximumCurrentDrawnFromDevice != NULL)
            {
                *(theiAP2IdentificationInformationParameter->iAP2MaximumCurrentDrawnFromDevice) = this_iAP2Device->iAP2AccessoryInfo.iAP2MaximumCurrentDrawnFromDevice;
                IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2 MaximumCurrentDrawnFromDevice: %d DevID:%p", *(theiAP2IdentificationInformationParameter->iAP2MaximumCurrentDrawnFromDevice), this_iAP2Device);
                theiAP2IdentificationInformationParameter->iAP2MaximumCurrentDrawnFromDevice_count++;
            }
        }

        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2SupportediOSAppCount = %d DevID:%p", this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount, this_iAP2Device);
        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo != NULL) && (this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount > 0) && (rc == IAP2_OK) )
        {
            theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol_count = this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount;
            theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol = calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount, sizeof(iAP2ExternalAccessoryProtocol) );
            if(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol != NULL)
            {
                U16 i;

                for(i = 0; ( (i < this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount) && (rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppName != NULL) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolName,
                                                   &this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppName,
                                                   &theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolName_count,
                                                   1, iAP2_utf8, 0);
                    if(rc == IAP2_OK)
                    {
                        theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolMatchAction = calloc(1, sizeof(iAP2ExternalAccessoryProtocolMatchAction) );
                        if(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolMatchAction != NULL)
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolMatchAction) = this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2EAPMatchAction;
                            theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolMatchAction_count++;
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if(rc == IAP2_OK)
                    {
                        theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolIdentifier = calloc(1, sizeof(U8) );
                        if(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolIdentifier != NULL)
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolIdentifier) = this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
                            theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolIdentifier_count= 1;
                            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2ExternalAccessoryProtocolIdentifier = %d DevID:%p", *(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolIdentifier), this_iAP2Device);
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if( (rc == IAP2_OK) && (TRUE == this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2EANativeTransport) )
                    {
                        theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2NativeTransportComponentIdentifier = calloc(1, sizeof(U16) );
                        if(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2NativeTransportComponentIdentifier != NULL)
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2NativeTransportComponentIdentifier) = IAP2_USB_HOST_MODE_TRANS_COMP_ID;
                            theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2NativeTransportComponentIdentifier_count++;
                            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2NativeTransportComponentIdentifier = %d DevID:%p", *(theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2NativeTransportComponentIdentifier), this_iAP2Device);
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if( (rc == IAP2_OK) && (TRUE == this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay) )
                    {
                        theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolCarPlay_count++;
                        IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2ExternalAccessoryProtocolCarPlay_count = %d DevID:%p", (theiAP2IdentificationInformationParameter->iAP2ExternalAccessoryProtocol[i].iAP2ExternalAccessoryProtocolCarPlay_count), this_iAP2Device);
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2PreferredAppBundleSeedIdentifier != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2AppMatchTeamID,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2PreferredAppBundleSeedIdentifier,
                                           &theiAP2IdentificationInformationParameter->iAP2AppMatchTeamID_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2CurrentLanguage != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2CurrentLanguage,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2CurrentLanguage,
                                           &theiAP2IdentificationInformationParameter->iAP2CurrentLanguage_count,
                                           1, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage != NULL) && (rc == IAP2_OK) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2SupportedLanguage,
                                           this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage,
                                           &theiAP2IdentificationInformationParameter->iAP2SupportedLanguage_count,
                                           this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguageCount, iAP2_utf8, 0);
        }

        if( (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2USBDEVICEMODE) && (rc == IAP2_OK) )
        {
            U8  USBDeviceMode_TransCompName[] = IAP2_USB_DEVICE_MODE_TRANS_COMP_NAME;

            theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent   = calloc(1, sizeof(iAP2USBDeviceTransportComponent));
            if(theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent != NULL)
            {
                theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent_count++;
                theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier = calloc(1, sizeof(U16*));
                if(theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier != NULL)
                {
                    *(theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier) = IAP2_USB_DEVICE_MODE_TRANS_COMP_ID;
                    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2 USBDevice Transport Component Identifier: %d DevID:%p", *(theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier), this_iAP2Device);
                    theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentIdentifier_count++;
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
                U8* USBDeviceModeTransCompName = USBDeviceMode_TransCompName;

                rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentName,
                                               &USBDeviceModeTransCompName,
                                               &theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportComponentName_count,
                                               1, iAP2_utf8, 0);
            }

            if( (rc == IAP2_OK) &&
                (this_iAP2Device->iAP2AccessoryInfo.iAP2USBDeviceSupportedAudioSampleRate_count > 0) &&
                (this_iAP2Device->iAP2AccessoryInfo.iAP2USBDeviceSupportedAudioSampleRate != NULL) )
            {
                theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate = calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2USBDeviceSupportedAudioSampleRate_count,
                                                                                                                                           (sizeof(iAP2USBDeviceModeAudioSampleRate)));
                if(theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate != NULL)
                {
                    memcpy(theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate,
                           this_iAP2Device->iAP2AccessoryInfo.iAP2USBDeviceSupportedAudioSampleRate,
                           (this_iAP2Device->iAP2AccessoryInfo.iAP2USBDeviceSupportedAudioSampleRate_count * sizeof(iAP2USBDeviceModeAudioSampleRate) ) );
                    theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2USBDeviceSupportedAudioSampleRate_count = this_iAP2Device->iAP2AccessoryInfo.iAP2USBDeviceSupportedAudioSampleRate_count;
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
            if(rc == IAP2_OK)
            {
                theiAP2IdentificationInformationParameter->iAP2USBDeviceTransportComponent->iAP2TransportSupportsiAP2Connection_count++;
            }
        }

        if(( (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2USBHOSTMODE) || (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2MULTIHOSTMODE))&& (rc == IAP2_OK) )
        {
            U8  USBHostMode_TransCompName[] = IAP2_USB_HOST_MODE_TRANS_COMP_NAME;

            theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent   = calloc(1, sizeof(iAP2USBHostTransportComponent));
            if(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent != NULL)
            {
                theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent_count++;
                theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier = calloc(1, sizeof(U16));
                if(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier != NULL)
                {
                    *(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier) = IAP2_USB_HOST_MODE_TRANS_COMP_ID;
                    IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "iAP2 USBHost Transport Component Identifier: %d DevID:%p", *(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier), this_iAP2Device);
                    theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentIdentifier_count++;
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
                U8* USBHostModeTransCompName = USBHostMode_TransCompName;

                rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentName,
                                               &USBHostModeTransCompName,
                                               &theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportComponentName_count,
                                               1, iAP2_utf8, 0);
            }
            if(rc == IAP2_OK)
            {
                theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportSupportsiAP2Connection_count++;
                if(this_iAP2Device->iAP2AccessoryInfo.iAP2SupportsiOSintheCar == TRUE)
                {
                    theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2TransportSupportsCarPlay_count++;
                    theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber = calloc(1, sizeof(U8));
                    if(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber != NULL)
                    {
                        U16 ean_count = 0;
                        U16 i;
                        theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber_count++;
                        if(this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount > 0)
                        {
                            for(i = 0; i < this_iAP2Device->iAP2AccessoryInfo.iAP2SupportediOSAppCount; i++)
                            {
                                if(this_iAP2Device->iAP2AccessoryInfo.iAP2iOSAppInfo[i].iAP2EANativeTransport == TRUE)
                                {
                                    ean_count++;
                                }
                            }
                            *(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber) = ean_count+1;
                        }
                        else
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber) = 0x01;
                        }
                        IAP2SESSIONDLTLOG(DLT_LOG_INFO, "ea_native app count = %d CarPlayInterfaceNumber = %d", ean_count, *(theiAP2IdentificationInformationParameter->iAP2USBHostTransportComponent->iAP2USBHostTransportCarPlaytInterfaceNumber));
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                }
            }
        }

        if((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportMAC_count > 0))
        {
            U16 i = 0;

            theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent_count = this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportMAC_count;
            theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent = calloc(theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent_count,
                                                                                                sizeof(iAP2BluetoothTransportComponent) );
            if(theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                for(i = 0; ( (i < theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent_count) && (rc == IAP2_OK) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                                   &theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));

                    if((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2TransportComponentName != NULL))
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                       &theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName_count,
                                                       1, iAP2_utf8, 0);
                    }

                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                                       &this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportMAC[i],
                                                       &theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress_count,
                                                       IAP2_BT_MAC_LENGTH, iAP2_blob, 0);
                    }

                    if((rc == IAP2_OK) && (this_iAP2Device->iAP2Transport.iAP2TransportType == iAP2BLUETOOTH))
                    {
                        theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
                    }
                }
            }
        }

        if((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent_count > 0))
        {
            U16 i = 0;

            theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent_count = this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent_count;
            theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent = calloc(theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent_count,
                                                                                                 sizeof(iAP2BluetoothTransportComponent) );
            if(theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent == NULL)
            {
                rc = IAP2_ERR_NO_MEM;
            }
            else
            {
                for(i = 0; ( (i < theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent_count) && (rc == IAP2_OK) ); i++)
                {
                    theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier = calloc(1, sizeof(U16));
                    if (NULL != theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier)
                    {
                        *(theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier) = *(this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier);
                        theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier_count++;
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                        this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                        &theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName_count,
                                                        1, iAP2_utf8, 0);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress->iAP2BlobData,
                                                       &theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress_count,
                                                       IAP2_BT_MAC_LENGTH, iAP2_blob, 0);
                    }

                    if((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count > 0))
                    {
                        theiAP2IdentificationInformationParameter->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
                    }
                }
            }
        }

        if ((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent != NULL))
        {
            theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent_count = this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent_count;
            theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent = (iAP2iAP2HIDComponent*)calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent_count, sizeof(iAP2iAP2HIDComponent));
            if (NULL != theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent)
            {
                U16 i;

                for(i = 0; ( (i < this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent_count) && (rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent[i].iAP2HIDComponentName != NULL) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentName,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent[i].iAP2HIDComponentName,
                                                   &theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentName_count,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent[i].iAP2HIDComponentName_count, iAP2_utf8, 0);
                    if (rc == IAP2_OK)
                    {
                        theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentIdentifier = calloc(1, sizeof(U16));
                        if (NULL != theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentIdentifier)
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentIdentifier) = *(this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier);
                            theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentIdentifier_count++;
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if (rc == IAP2_OK)
                    {
                        theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                        if (NULL != theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentFunction)
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentFunction) = *(this_iAP2Device->iAP2AccessoryInfo.iAP2USBHIDComponent[i].iAP2HIDComponentFunction);
                            theiAP2IdentificationInformationParameter->iAP2iAP2HIDComponent[i].iAP2HIDComponentFunction_count++;
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent != NULL) && (rc == IAP2_OK) )
        {
            theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent = calloc(1, sizeof(iAP2VehicleInformationComponent));
            if(theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent != NULL)
            {
                theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent_count++;
                theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2Identifier= calloc(1, sizeof(U16));
                if(theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2Identifier != NULL)
                {
                    theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2Identifier_count++;
                    *(theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2Identifier) = IAP2_VEHICLE_INFORMATION_COMP_ID;
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
                if(rc == IAP2_OK)
                {
                    U8  VehicleInformationName[] = {"Vehicle Information"};
                    U8* VehicleInformationPtr    = VehicleInformationName;

                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2Name,
                                                   &VehicleInformationPtr,
                                                   &theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2Name_count,
                                                   1, iAP2_utf8, 0);
                }
                if( (this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2DisplayName != NULL) && (rc == IAP2_OK) )
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2DisplayName,
                                                   &this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2DisplayName,
                                                   &theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2DisplayName_count,
                                                   1, iAP2_utf8, 0);
                }
                if( (this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2MapsDisplayName != NULL) && (rc == IAP2_OK) )
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2MapsDisplayName,
                                                                   &this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2MapsDisplayName,
                                                                   &theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2MapsDisplayName_count,
                                                                   1, iAP2_utf8, 0);
                }
                if(rc == IAP2_OK)
                {
                    theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2EngineType = calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType_count,
                                                                                                                        sizeof(iAP2EngineTypes) );
                    if(theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2EngineType != NULL)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2EngineType_count = this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType_count;
                        memcpy(theiAP2IdentificationInformationParameter->iAP2VehicleInformationComponent->iAP2EngineType,
                               this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType,
                               (this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType_count * sizeof(iAP2EngineTypes) ) );
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent != NULL) && (rc == IAP2_OK) )
        {
            theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent = calloc(1, sizeof(iAP2VehicleStatusComponent));
            if(theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent != NULL)
            {
                theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent_count++;
                theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Identifier = calloc(1, sizeof(U16));
                if(theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Identifier != NULL)
                {
                    theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Identifier_count++;
                    *(theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Identifier) = IAP2_VEHICLE_STATUS_COMP_ID;
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
                if(rc == IAP2_OK)
                {
                    U8 VehicleStatusInformationName[] = {"Vehicle Status"};
                    U8* VehicleStatusInformationPtr = VehicleStatusInformationName;

                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Name,
                                                   &VehicleStatusInformationPtr,
                                                   &theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Name_count,
                                                   1, iAP2_utf8, 0);
                }
                if(rc == IAP2_OK)
                {
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2InsideTemperature == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2InsideTemperature_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2NightMode == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2NightMode_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2OutsideTemperature == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2OutsideTemperature_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeWarning == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeWarning_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2Range == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2Range_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeGasoline == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeGasoline_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeDiesel == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeDiesel_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeElectric == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeElectric_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeCNG == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeCNG_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeWarningGasoline == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeWarningGasoline_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeWarningDiesel == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeWarningDiesel_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeWarningElectric == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeWarningElectric_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent->iAP2RangeWarningCNG == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2VehicleStatusComponent->iAP2RangeWarningCNG_count++;
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent != NULL) && (rc == IAP2_OK) )
        {
            theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent = calloc(1, sizeof(iAP2LocationInformationComponent));
            if(theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent != NULL)
            {
                theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent_count++;
                theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier = calloc(1, sizeof(U16));
                if(theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier != NULL)
                {
                    theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier_count++;
                    *(theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentIdentifier) = IAP2_LOCATION_INFORMATION_COMP_ID;
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
                if(rc == IAP2_OK)
                {
                    U8 LocationInformationName[] = {"Location Information"};
                    U8* LocationInformationPtr = LocationInformationName;

                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentName,
                                                   &LocationInformationPtr,
                                                   &theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentName_count,
                                                   1, iAP2_utf8, 0);
                }
                if(rc == IAP2_OK)
                {
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2GlobalPositioningSystemFixData == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentGlobalPositioningSystemFixData_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2RecommendedMinimumSpecificGPSTransitData == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentRecommendedMinimumSpecificGPSTransitData_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2GPSSatelliteInView == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentGPSSatelliteInView_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2VehicleSpeedData == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleSpeedData_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2VehicleGyroData == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleGyroData_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2VehicleAccelerometerData == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleAccelerometerData_count++;
                    }
                    if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent->iAP2VehicleHeadingData == TRUE)
                    {
                        theiAP2IdentificationInformationParameter->iAP2LocationInformationComponent->iAP2LocationInformationComponentVehicleHeadingData_count++;
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if ((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent != NULL))
        {
            theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent_count = this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent_count;
            theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent = (iAP2USBHostHIDComponent*)calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent_count, sizeof(iAP2USBHostHIDComponent));
            if (NULL != theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent)
            {
                U16 i;

                for(i = 0; ( (i < this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent_count) && (rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent[i].iAP2HIDComponentName != NULL) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentName,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent[i].iAP2HIDComponentName,
                                                   &theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentName_count,
                                                   1, iAP2_utf8, 0);
                    if (rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier,
                                                        this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier,
                                                        &theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier_count,
                                                        1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK)
                    {
                        theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                        if (NULL != theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction)
                        {
                            *(theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction) = *(this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction);
                            theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction_count++;
                        }
                        else
                        {
                            rc = IAP2_ERR_NO_MEM;
                        }
                    }
                    if (rc == IAP2_OK)
                    {
                        U16 USBHost_TransComp_Ident = IAP2_USB_HOST_MODE_TRANS_COMP_ID;
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2USBHostTransportComponentIdentifier,
                                                       &USBHost_TransComp_Ident,
                                                       &theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2USBHostTransportComponentIdentifier_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK)
                    {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber,
                                                   &theiAP2IdentificationInformationParameter->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber_count,
                                                   1, iAP2_uint16, sizeof(U16));
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if ((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent != NULL))
        {
            theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent_count = this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent_count;
            theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent = (iAP2WirelessCarPlayTransportComponent*)calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent_count, sizeof(iAP2WirelessCarPlayTransportComponent));
            if (NULL != theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent)
            {
                U16 i = 0;

                for(i = 0; ( (i < this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent_count) && (rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName != NULL) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName,
                                                   &theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName_count,
                                                   1, iAP2_utf8, 0);
                    if (rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier,
                                                       &theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if(rc == IAP2_OK)
                    {
                        if(this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsCarPlay_count > 0)
                        {
                            theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsCarPlay_count++;
                        }
                        if(this_iAP2Device->iAP2AccessoryInfo.iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsiAP2Connection_count > 0)
                        {
                            theiAP2IdentificationInformationParameter->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
                        }
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothHIDComponent != NULL) && (rc == IAP2_OK) )
        {
            theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent = calloc(1, sizeof(iAP2BluetoothHIDComponent));
            if(NULL != theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent)
            {
                theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent_count++;
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier,
                                                   &theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentName,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothHIDComponent->iAP2HIDComponentName,
                                                   &theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentName_count,
                                                   1, iAP2_utf8, 0);
                }
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2BluetoothTransportComponentIdentifier,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothHIDComponent->iAP2BluetoothTransportComponentIdentifier,
                                                   &theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2BluetoothTransportComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if(rc == IAP2_OK)
                {
                    theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                    if (NULL != theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction)
                    {
                        *(theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction) = *(this_iAP2Device->iAP2AccessoryInfo.iAP2BluetoothHIDComponent->iAP2HIDComponentFunction);
                        theiAP2IdentificationInformationParameter->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction_count++;
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if ((rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent != NULL))
        {
            theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent_count = this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent_count;
            theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent = (iAP2RouteGuidanceDisplayComponent*)calloc(this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent_count, sizeof(iAP2RouteGuidanceDisplayComponent));
            if (NULL != theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent)
            {
                U16 i = 0;

                for(i = 0; ( (i < this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent_count) && (rc == IAP2_OK) ); i++)
                {
                    rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                                   this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                                   &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2Name_count,
                                                   1, iAP2_utf8, 0);
                    if (rc == IAP2_OK)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                    if (rc == IAP2_OK && this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count == 1)
                    {
                        rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                       this_iAP2Device->iAP2AccessoryInfo.iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                       &theiAP2IdentificationInformationParameter->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count,
                                                       1, iAP2_uint16, sizeof(U16));
                    }
                }
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }

        if( (this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID != NULL) && (rc == IAP2_OK) && (this_iAP2Device->iAP2AccessoryInfo.iAP2SupportsiOSintheCar == TRUE) )
        {
            rc = iAP2AllocateandUpdateData(&theiAP2IdentificationInformationParameter->iAP2ProductPlanUUID,
                                           &this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID,
                                           &theiAP2IdentificationInformationParameter->iAP2ProductPlanUUID_count,
                                           1, iAP2_utf8, 0);
        }
    }

    return rc;
}


S32 iAP2GenerateiAP2PowerSourceUpdate(iAP2Device_st* this_iAP2Device, iAP2PowerSourceUpdateParameter* theiAP2PowerSourceUpdateParameter)
{
    S32 rc = IAP2_OK;

    if( (this_iAP2Device == NULL) || (theiAP2PowerSourceUpdateParameter == NULL) )
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter. this_iAP2Device = %p, theiAP2PowerSourceUpdateParameter = %p", this_iAP2Device, theiAP2PowerSourceUpdateParameter);
    }
    else
    {
        memset(theiAP2PowerSourceUpdateParameter, 0, sizeof(iAP2PowerSourceUpdateParameter));
        if(this_iAP2Device->iAP2AccessoryConfig.iAP2AvailableCurrentForDevice != 0)
        {
            theiAP2PowerSourceUpdateParameter->iAP2AvailableCurrentForDevice  = calloc(1, sizeof(U16));
            if(theiAP2PowerSourceUpdateParameter->iAP2AvailableCurrentForDevice != NULL)
            {
                *theiAP2PowerSourceUpdateParameter->iAP2AvailableCurrentForDevice = this_iAP2Device->iAP2AccessoryConfig.iAP2AvailableCurrentForDevice;
                theiAP2PowerSourceUpdateParameter->iAP2AvailableCurrentForDevice_count++;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if( (this_iAP2Device->iAP2AccessoryConfig.iAP2DeviceBatteryShouldChargeIfPowerIsPresent == TRUE) && (rc == IAP2_OK) )
        {
            theiAP2PowerSourceUpdateParameter->iAP2DeviceBatteryShouldChargeIfPowerIsPresent  = calloc(1, sizeof(U8));
            if(theiAP2PowerSourceUpdateParameter->iAP2DeviceBatteryShouldChargeIfPowerIsPresent != NULL)
            {
                *theiAP2PowerSourceUpdateParameter->iAP2DeviceBatteryShouldChargeIfPowerIsPresent = 1;
                theiAP2PowerSourceUpdateParameter->iAP2DeviceBatteryShouldChargeIfPowerIsPresent_count++;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
    }

    return rc;
}


S32 iAP2GenerateiAP2StartPowerUpdates(iAP2Device_st* this_iAP2Device, iAP2StartPowerUpdatesParameter* theiAP2StartPowerUpdatesParameter)
{
    S32 rc = IAP2_OK;

    if( (this_iAP2Device == NULL) || (theiAP2StartPowerUpdatesParameter == NULL) )
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter. this_iAP2Device = %p, theiAP2StartPowerUpdatesParameter = %p", this_iAP2Device, theiAP2StartPowerUpdatesParameter);
    }
    else
    {
        memset(theiAP2StartPowerUpdatesParameter, 0, sizeof(iAP2StartPowerUpdatesParameter));
        if(this_iAP2Device->iAP2AccessoryConfig.iAP2MaximumcurrentDrawnFromAccessory == TRUE)
        {
            theiAP2StartPowerUpdatesParameter->iAP2MaximumcurrentDrawnFromAccessory  = calloc(1, sizeof(U8));
            if(theiAP2StartPowerUpdatesParameter->iAP2MaximumcurrentDrawnFromAccessory != NULL)
            {
                *theiAP2StartPowerUpdatesParameter->iAP2MaximumcurrentDrawnFromAccessory = 1;
                theiAP2StartPowerUpdatesParameter->iAP2MaximumcurrentDrawnFromAccessory_count++;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if( (this_iAP2Device->iAP2AccessoryConfig.iAP2DeviceBatteryWillChargeIfPowerIsPresent == TRUE) && (rc == IAP2_OK) )
        {
            theiAP2StartPowerUpdatesParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent  = calloc(1, sizeof(U8));
            if(theiAP2StartPowerUpdatesParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent != NULL)
            {
                *theiAP2StartPowerUpdatesParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent = 1;
                theiAP2StartPowerUpdatesParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count++;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
        if( (this_iAP2Device->iAP2AccessoryConfig.iAP2AccessoryPowerMode == TRUE) && (rc == IAP2_OK) )
        {
            theiAP2StartPowerUpdatesParameter->iAP2AccessoryPowerMode  = calloc(1, sizeof(U8));
            if(theiAP2StartPowerUpdatesParameter->iAP2AccessoryPowerMode != NULL)
            {
                *theiAP2StartPowerUpdatesParameter->iAP2AccessoryPowerMode = 1;
                theiAP2StartPowerUpdatesParameter->iAP2AccessoryPowerMode_count++;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
    }

    return rc;
}


S32 iAP2GenerateiAP2DeviceAuthenticationCertificateResponse(iAP2DeviceAuthenticationCertificateParameter theiAP2DeviceAuthenticationCertificateParameter, iAP2RequestDeviceAuthenticationChallengeResponseParameter* theiAP2RequestDeviceAuthenticationChallengeResponseParameter)
{
    S32 rc = IAP2_OK;

    if(theiAP2RequestDeviceAuthenticationChallengeResponseParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter");
    }
    else
    {
        //To Avoid Compiler Warning
        theiAP2DeviceAuthenticationCertificateParameter = theiAP2DeviceAuthenticationCertificateParameter;

        memset(theiAP2RequestDeviceAuthenticationChallengeResponseParameter, 0, sizeof(iAP2RequestDeviceAuthenticationChallengeResponseParameter));
    }

    return rc;
}


S32 iAP2GenerateiAP2DeviceAuthenticationResponseResponse(iAP2DeviceAuthenticationResponseParameter theiAP2DeviceAuthenticationResponseParameter, iAP2DeviceAuthenticationSucceededParameter* theiAP2DeviceAuthenticationSucceededParameter)
{
    S32 rc = IAP2_OK;

    if(theiAP2DeviceAuthenticationSucceededParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter");
    }
    else
    {
        //To Avoid Compiler Warning
        theiAP2DeviceAuthenticationResponseParameter = theiAP2DeviceAuthenticationResponseParameter;

        memset(theiAP2DeviceAuthenticationSucceededParameter, 0, sizeof(iAP2DeviceAuthenticationSucceededParameter));
    }

    return rc;
}


S32 iAP2GenerateiAP2PowerUpdateResponse(iAP2PowerUpdateParameter theiAP2PowerUpdateParameter, iAP2StopPowerUpdatesParameter* theiAP2StopPowerUpdatesParameter)
{
    S32 rc = IAP2_OK;

    if(theiAP2StopPowerUpdatesParameter == NULL)
    {
        rc = IAP2_INVALID_INPUT_PARAMETER;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Invalid Input parameter");
    }
    else
    {
        //To Avoid Compiler Warning
        theiAP2PowerUpdateParameter = theiAP2PowerUpdateParameter;

        memset(theiAP2StopPowerUpdatesParameter, 0, sizeof(iAP2StopPowerUpdatesParameter));
    }

    return rc;
}

static S32 iAP2AllocateSPtr(U8** dest_ptr, U8* src_ptr)
{
    S32 rc = IAP2_OK;
    U32 StringLength;

    StringLength = strnlen( (const char*)src_ptr, STRING_MAX) ;
    *dest_ptr = (U8*)calloc( (StringLength + IAP2_NULL_CHAR_LEN), sizeof(U8));
    if(*dest_ptr == NULL)
    {
        rc = IAP2_ERR_NO_MEM;
    }
    else
    {
        memcpy(*dest_ptr, src_ptr, StringLength);
        (*dest_ptr)[StringLength] = '\0';
    }

    return rc;
}

inline static void iAP2FreePointer(void** iAP2PointerToFree)
{
    if(iAP2PointerToFree != NULL)
    {
        free(*iAP2PointerToFree);
        *iAP2PointerToFree = NULL;
    }
}

S32 iAP2ReplaceRejectedIdentificationInformationParameter(iAP2Device_st* this_iAP2Device, iAP2IdentificationRejectedParameter theiAP2IdentificationRejectedParameter)
{
    S32 rc = IAP2_OK;
    U32 par_count = 0;

    if(theiAP2IdentificationRejectedParameter.iAP2AccessoryName_count == 1)
    {
        U8 iAP2AccessoryName[] = {"Product"};

        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryName);
        rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryName, iAP2AccessoryName);
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2AccessoryModelIdentifier_count == 1) && (rc == IAP2_OK) )
    {
        U8 iAP2AccessoryModelIdentifier[] = {"Model_1"};

        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryModelIdentifier);
        rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryModelIdentifier, iAP2AccessoryModelIdentifier);
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2AccessoryManufacturer_count == 1) && (rc == IAP2_OK) )
    {
        U8 iAP2AccessoryManufacturer[] = {"Manufacturer"};

        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryManufacturer);
        rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryManufacturer, iAP2AccessoryManufacturer);
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2AccessorySerialNumber_count == 1) && (rc == IAP2_OK) )
    {
        U8 iAP2AccessorySerialNumber[] = {"12345"};

        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessorySerialNumber);
        rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessorySerialNumber, iAP2AccessorySerialNumber);
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2AccessoryFirmwareVersion_count == 1) && (rc == IAP2_OK) )
    {
        U8 iAP2AccessoryFirmwareVersion[] = {"1 0 0"};

        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryFirmwareVersion);
        rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryFirmwareVersion, iAP2AccessoryFirmwareVersion);
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2AccessoryHardwareVersion_count == 1) && (rc == IAP2_OK) )
    {
        U8 iAP2AccessoryHardwareVersion[] = {"1 0 0"};

        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryHardwareVersion);
        rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2AccessoryHardwareVersion, iAP2AccessoryHardwareVersion);
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory_count == 1) && (rc == IAP2_OK) )
    {
        if( (theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory->iAP2BlobLength == this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication_length) ||
            (theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory->iAP2BlobLength == 0) )
        {
            rc = IAP2_CTL_ERROR;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "MessagesSentByAccessory rejection error iAP2BlobLength = %d", theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory->iAP2BlobLength);
        }
        else
        {
            U16  NoOfRejectedMessages = 0;
            U16* RejectedMessages;
            U16* NewMessages;

            if(theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory->iAP2BlobLength != 0)
            {
                NoOfRejectedMessages = (theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory->iAP2BlobLength / sizeof(U16) );
            }
            RejectedMessages = calloc(NoOfRejectedMessages, sizeof(U16));
            NewMessages = calloc( ( ( this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication_length / sizeof(U16) ) - NoOfRejectedMessages), sizeof(U16));
            if( (RejectedMessages == NULL) || (NewMessages == NULL) )
            {
                rc = IAP2_ERR_NO_MEM;
                IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Error in Allocating memory");
                iAP2FreePointer((void**)&RejectedMessages);
                iAP2FreePointer((void**)&NewMessages);
            }
            else
            {
                U16 count;
                U16 msg_rej_count;
                U16 copy_new_msg_cnt = 0;
                BOOL copy;

                for(count = 0; count < NoOfRejectedMessages; count++)
                {
                    void* RejMsg = &theiAP2IdentificationRejectedParameter.iAP2MessagesSentByAccessory->iAP2BlobData[count * 2];
                    U16* RejectedMsg = (U16*)RejMsg;

                    /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
                    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                    RejectedMessages[count] = IAP2_ADHERE_TO_HOST_ENDIANESS_16(*RejectedMsg);    /*lint !e160 !e644 */
                }
                for(count = 0; (count < this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication_length / sizeof(U16) ); count++)
                {
                    copy = TRUE;
                    for(msg_rej_count = 0; msg_rej_count < NoOfRejectedMessages; msg_rej_count++)
                    {
                        if(this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication[count] == RejectedMessages[msg_rej_count])
                        {
                            copy = FALSE;
                            break;
                        }
                    }
                    if(copy == TRUE)
                    {
                        NewMessages[copy_new_msg_cnt] = this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication[count];
                        copy_new_msg_cnt++;
                    }
                }
                iAP2FreePointer(( void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication);
                this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication = NewMessages;
                this_iAP2Device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication_length = ( copy_new_msg_cnt * sizeof(U16) );
                iAP2FreePointer( (void**)&RejectedMessages);
            }
        }
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice_count == 1) && (rc == IAP2_OK) )
    {
        if( (theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice->iAP2BlobLength == this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice_length) ||
            (theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice->iAP2BlobLength == 0) )
        {
            rc = IAP2_CTL_ERROR;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "MessagesSentByAccessory rejection error iAP2BlobLength = %d", theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice->iAP2BlobLength);
        }
        else
        {
            U16  NoOfRejectedMessages = 0;
            U16* RejectedMessages;
            U16* NewMessages;

            if(theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice->iAP2BlobLength != 0)
            {
                NoOfRejectedMessages = (theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice->iAP2BlobLength / sizeof(U16) );
            }
            RejectedMessages = calloc(NoOfRejectedMessages, sizeof(U16));
            NewMessages = calloc( ( ( this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice_length / sizeof(U16) ) - NoOfRejectedMessages), sizeof(U16));
            if( (RejectedMessages == NULL) || (NewMessages == NULL) )
            {
                rc = IAP2_ERR_NO_MEM;
                IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "Error in Allocating memory");
                iAP2FreePointer( (void**)&RejectedMessages);
                iAP2FreePointer( (void**)&NewMessages);
            }
            else
            {
                U16 count;
                U16 msg_rej_count;
                U16 copy_new_msg_cnt = 0;
                BOOL copy;

                for(count = 0; count < NoOfRejectedMessages; count++)
                {
                    void* RejMsg = &theiAP2IdentificationRejectedParameter.iAP2MessagesRecievedfromDevice->iAP2BlobData[count * 2];
                    U16* RejectedMsg = (U16*)RejMsg;

                    /* PRQA: Lint Message 160: The sequence ( { is non standard and is taken to introduce a GNU statement expression. */
                    /* PRQA: Lint Message 644: Variable __v may not have been initialized. */
                    RejectedMessages[count] = IAP2_ADHERE_TO_HOST_ENDIANESS_16(*RejectedMsg);    /*lint !e160 !e644 */
                }
                for(count = 0; (count < this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice_length / sizeof(U16) ); count++)
                {
                    copy = TRUE;
                    for(msg_rej_count = 0; msg_rej_count < NoOfRejectedMessages; msg_rej_count++)
                    {
                        if(this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice[count] == RejectedMessages[msg_rej_count])
                        {
                            copy = FALSE;
                            break;
                        }
                    }
                    if(copy == TRUE)
                    {
                        NewMessages[copy_new_msg_cnt] = this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice[count];
                        copy_new_msg_cnt++;
                    }
                }
                iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice);
                this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice = NewMessages;
                this_iAP2Device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice_length = ( copy_new_msg_cnt * sizeof(U16) );
                iAP2FreePointer( (void**)&RejectedMessages);
            }
        }
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2PowerSourceType_count == 1) && (rc == IAP2_OK) )
    {
        /* Nothing could be done */
        rc = IAP2_CTL_ERROR;
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "PowerSourceType is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2MaximumCurrentDrawnFromDevice_count == 1) && (rc == IAP2_OK) )
    {
        this_iAP2Device->iAP2AccessoryInfo.iAP2MaximumCurrentDrawnFromDevice = 0;
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2SupportedExternalAccessoryProtocol_count == 1) && (rc == IAP2_OK) )
    {
        /* we have 2 possibilities either EAP not there or one of the EAP is not supported */
        /* case 1: EAP not there is not possible, because we would have returned error during initialization */
        /* case 2: one of the EAP is not supported, then we will remove one by one */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "SupportedExternalAccessoryProtocol is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2PreferredAppBundleSeedIdentifier_count == 1) && (rc == IAP2_OK) )
    {
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2PreferredAppBundleSeedIdentifier == NULL)
        {
            rc = IAP2_CTL_ERROR;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "PreferredAppBundleSeedIdentifier is NULL but still rejected, we dont have solution yet");
        }
        else
        {
            /* Case - 2: PreferredAppBundleSeedIdentifier was not accepted, so disable it */
            free(this_iAP2Device->iAP2AccessoryInfo.iAP2PreferredAppBundleSeedIdentifier);
        }
        par_count++;
    }
    if( ( (theiAP2IdentificationRejectedParameter.iAP2CurrentLanguage_count == 1) ||
          (theiAP2IdentificationRejectedParameter.iAP2SupportedLanguage_count == 1) ) &&
        (rc == IAP2_OK) )
    {
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguageCount == 1)
        {
            /* Nothing could be done, we have only one supported language & that could be the current language */
            rc = IAP2_CTL_ERROR;
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "PowerSourceType is rejected, we dont have solution yet");
        }
        else
        {
            U8  Language[] = {"en"};
            U8* SuppLanguage  = Language;
            U16 count;

            /* Current Language is not supported so choose one from the supported language */
            iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2CurrentLanguage);
            /* Copy the first one */
            rc = iAP2AllocateSPtr(&this_iAP2Device->iAP2AccessoryInfo.iAP2CurrentLanguage, Language);

            if( (NULL != this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage) && (rc == IAP2_OK) )
            {
                /* Free supported language before forming*/
                for(count = 0; count < this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguageCount; count++)
                {
                    if (NULL != this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage[count])
                    {
                        iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage[count]);
                        this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage[count] = NULL;
                    }
                }
                iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage);
                rc = iAP2AllocateandUpdateData(&this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguage,
                                               &SuppLanguage,
                                               &this_iAP2Device->iAP2AccessoryInfo.iAP2SupportedLanguageCount,
                                               1, iAP2_utf8, 0);
            }
            IAP2SESSIONDLTLOG(DLT_LOG_DEBUG, "Current Language & Supported Language are set to English");
        }
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2SerialTransportComponent_count == 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "SerialTransportComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2USBDeviceTransportComponent_count == 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "USBDeviceTransportComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2USBHostTransportComponent_count == 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "USBHostTransportComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2BluetoothTransportComponent_count == 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "BluetoothTransportComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2iAP2HIDComponent_count >= 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2HIDComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2VehicleInformationComponent_count == 1) && (rc == IAP2_OK) )
    {
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent != NULL)
        {
            if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType != NULL)
            {
                iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType);
            }
            this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent->iAP2EngineType_count = 0;
            iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent);
            this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleInformationComponent = NULL;
        }
        else
        {
            rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "VehicleInformationComponent is NULL & still got rejected, we dont have solution yet");
        }
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2VehicleStatusComponent_count == 1) && (rc == IAP2_OK) )
    {
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent != NULL)
        {
            iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent);
            this_iAP2Device->iAP2AccessoryInfo.iAP2VehicleStatusComponent = NULL;
        }
        else
        {
            rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "VehicleStatusComponent is NULL & still got rejected, we dont have solution yet");
        }
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2LocationInformationComponent_count == 1) && (rc == IAP2_OK) )
    {
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent != NULL)
        {
            iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent);
            this_iAP2Device->iAP2AccessoryInfo.iAP2LocationInformationComponent = NULL;
        }
        else
        {
            rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "LocationInformationComponent is NULL & still got rejected, we dont have solution yet");
        }
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2USBHostHIDComponent_count >= 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "USBHostHIDComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2WirelessCarPlayTransportComponent_count >= 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "WirelessCarPlayTransportComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2BluetoothHIDComponent_count == 1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "BluetoothHIDComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2RouteGuidanceDisplayComponent_count >=1) && (rc == IAP2_OK) )
    {
        rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
        IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "RouteGuidanceDisplayComponent is rejected, we dont have solution yet");
        par_count++;
    }
    if( (theiAP2IdentificationRejectedParameter.iAP2ProductPlanUUID_count >= 1) && (rc == IAP2_OK) )
    {
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID != NULL)
        {
            iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID);
            this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID = NULL;
        }
        else
        {
            rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2ProductPlanUUID is NULL & still got rejected, we dont have solution yet");
        }
        par_count++;
    }

    if( (par_count == 0) && (rc == IAP2_OK) )
    {
        /* There are no parameters.
         * It could be Apple issue related to PPUUID
         * iOS 12.1.3 and 10.3.3 will respond with ID34
         * iOS 11.2.6 and 11.2.5 respond without any parameter
         * PPUUID is assumed to be the rejected reason and hence remove the PPUUID
         * for the Re-attempt SWGIII-33608 */
        if(this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID != NULL)
        {
            IAP2SESSIONDLTLOG(DLT_LOG_WARN, "No Parameter in IdentificationRejected message. PPUUID could be the reason so removing ");
            iAP2FreePointer( (void**)&this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID);
            this_iAP2Device->iAP2AccessoryInfo.iAP2ProductPlanUUID = NULL;
        }
        else
        {
            rc = IAP2_CTL_ERROR;    /* No IDEA, What could be done */
            IAP2SESSIONDLTLOG(DLT_LOG_ERROR, "iAP2ProductPlanUUID is NULL & still got rejected, we dont have solution yet");
        }
    }

    return rc;
}
