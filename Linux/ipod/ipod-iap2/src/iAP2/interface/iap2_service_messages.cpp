/*
 * iap2_service_messages.cpp
 *
 *  Created on: 06-Apr-2017
 *      Author: dhana
 */
#include <iap2_dlt_log.h>

#include "iap2_service_init.h"
#include "iap2_service_init_private.h"
#include "iap2_service_messages.h"
#include "iap2_utility.h"

#include <map>

std::map<MessageType, std::string> messageTypeString {
        {MessageType::DeviceConnected,            "DeviceConnected"},
        {MessageType::DeviceDisconnected,         "DeviceDisconnected"},
        {MessageType::iAP2DeviceMsg,              "iAP2DeviceMsg"},
        {MessageType::iAP2FileTransferMsg,        "iAP2FileTransferMsg"},
        {MessageType::iAP2EAPMsg,                 "iAP2EAPMsg"},
        {MessageType::ConnectDeviceResp,          "ConnectDeviceResp"},
        {MessageType::DeviceState,                "DeviceState"},
        {MessageType::EANativeTransport,          "EANativeTransport"},
        {MessageType::ClientInformation,          "ClientInformation"},
        {MessageType::AccessoryConfiguration,     "AccessoryConfiguration"},
        {MessageType::AccessoryIdentficiation,    "AccessoryIdentficiation"},
        {MessageType::MessagsSentByApplication,   "MessagsSentByApplication"},
        {MessageType::CallbacksExpectedFromDevice,"CallbacksExpectedFromDevice"},
        {MessageType::AccessorySupportedLanguages,"AccessorySupportedLanguages"},
        {MessageType::AccessorySupportediOSApps,  "AccessorySupportediOSApps"},
        {MessageType::USBDeviceAudioSampleRates,  "USBDeviceAudioSampleRates"},
        {MessageType::USBDeviceTransport,         "USBDeviceTransport"},
        {MessageType::USBHostTransport,           "USBHostTransport"},
        {MessageType::BluetoothTransport,         "BluetoothTransport"},
        {MessageType::WirelessCarPlayTransport,   "WirelessCarPlayTransport"},
        {MessageType::USBDeviceHID,               "USBDeviceHID"},
        {MessageType::USBHostHID,                 "USBHostHID"},
        {MessageType::BluetoothHID,               "BluetoothHID"},
        {MessageType::VehicleInformation,         "VehicleInformation"},
        {MessageType::VehicleStatus,              "VehicleStatus"},
        {MessageType::LocationInformation,        "LocationInformation"},
        {MessageType::RouteGuidanceDisplay,       "RouteGuidanceDisplay"},
        {MessageType::IdentificationInfoComplete, "IdentificationInfoComplete"},
        {MessageType::DeviceDiscovered,           "DeviceDiscovered"},
        {MessageType::DeviceDisappeared,          "DeviceDisappeared"},
        {MessageType::ConnectDevice,              "ConnectDevice"},
        {MessageType::DisconnectDevice,           "DisconnectDevice"},
        {MessageType::iAP2AccMsg,                 "iAP2AccMsg"}
};

void iAP2ServiceGetMessageTypeString(enum MessageType type, char* string, size_t length)
{
    std::string msgTypeString("Unknown MessageType!");
    auto element = messageTypeString.find(type);
    if(element != messageTypeString.end())
    {
        msgTypeString = element->second;
    }

    memcpy(string, msgTypeString.c_str(), strnlen(msgTypeString.c_str(), length));
}

int32_t sendAccessoryConfiguration(iAP2Service_t* service, iAP2AccessoryConfig_t* initAccCfg, iAP2InitParam_t* iap2InitParam)
{
    /* Accessory configuration info*/
    struct AccessoryConfiguration accConf;
    memset(&accConf, 0, sizeof(accConf));
    accConf.header.type = MessageType::AccessoryConfiguration;
    iAP2ServiceAccessoryConfig_t* accConfig = &accConf.accConfig;

    if(iap2InitParam->iAP2DeviceId)
    {
        strncpy(accConf.deviceSerial, (char*)iap2InitParam->iAP2DeviceId, strnlen((char*)iap2InitParam->iAP2DeviceId, STRING_MAX));
    }

    accConfig->iAP2iOSintheCar          = initAccCfg->iAP2iOSintheCar;
    accConfig->iAP2EANativeTransport    = initAccCfg->iAP2EANativeTransport;
    accConfig->iAP2TransportType        = initAccCfg->iAP2TransportType;
    accConfig->iAP2AuthenticationType   = initAccCfg->iAP2AuthenticationType;
    accConfig->iAP2EAPSupported         = initAccCfg->iAP2EAPSupported;

    if(initAccCfg->iAP2AuthDevicename)
    {
        strncpy(accConfig->iAP2AuthDevicename,  (const char*)initAccCfg->iAP2AuthDevicename,    strnlen((char*)initAccCfg->iAP2AuthDevicename, STRING_MAX));
    }
    if(initAccCfg->iAP2AuthIoctlRegAddr)
    {
        strncpy(accConfig->iAP2AuthIoctlRegAddr,(const char*)initAccCfg->iAP2AuthIoctlRegAddr,  strnlen((char*)initAccCfg->iAP2AuthIoctlRegAddr, STRING_MAX));
    }
    if(initAccCfg->iAP2AuthGPIOReset)
    {
        strncpy(accConfig->iAP2AuthGPIOReset,   (const char*)initAccCfg->iAP2AuthGPIOReset,     strnlen((char*)initAccCfg->iAP2AuthGPIOReset, STRING_MAX));
    }
    if(initAccCfg->iAP2AuthGPIOReady)
    {
        strncpy(accConfig->iAP2AuthGPIOReady,   (const char*)initAccCfg->iAP2AuthGPIOReady,     strnlen((char*)initAccCfg->iAP2AuthGPIOReady, STRING_MAX));
    }
    if(initAccCfg->iAP2UsbOtgGPIOPower)
    {
        strncpy(accConfig->iAP2UsbOtgGPIOPower, (const char*)initAccCfg->iAP2UsbOtgGPIOPower,   strnlen((char*)initAccCfg->iAP2UsbOtgGPIOPower, STRING_MAX));
    }

    accConfig->iAP2AuthShortWait    = initAccCfg->iAP2AuthShortWait;
    accConfig->iAP2AuthWait         = initAccCfg->iAP2AuthWait;
    accConfig->iAP2AuthLongWait     = initAccCfg->iAP2AuthLongWait;

    accConfig->iAP2FileXferSupported    = initAccCfg->iAP2FileXferSupported;
    accConfig->iAP2FileXferRcvAsStream  = initAccCfg->iAP2FileXferRcvAsStream;

    accConfig->iAP2AvailableCurrentForDevice                 = initAccCfg->iAP2AvailableCurrentForDevice;
    accConfig->iAP2DeviceBatteryShouldChargeIfPowerIsPresent = initAccCfg->iAP2DeviceBatteryShouldChargeIfPowerIsPresent;
    accConfig->iAP2MaximumcurrentDrawnFromAccessory          = initAccCfg->iAP2MaximumcurrentDrawnFromAccessory;
    accConfig->iAP2DeviceBatteryWillChargeIfPowerIsPresent   = initAccCfg->iAP2DeviceBatteryWillChargeIfPowerIsPresent;
    accConfig->iAP2AccessoryPowerMode                        = initAccCfg->iAP2AccessoryPowerMode;

    accConfig->useConfigFS                  = initAccCfg->useConfigFS;
    accConfig->ManualLinkConfig             = initAccCfg->ManualLinkConfig;
    accConfig->LinkConfig_SessionVersion    = initAccCfg->LinkConfig_SessionVersion;

    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Accessory Configuration");
    return iAP2ServiceSendMessage(service, &accConf, sizeof(accConf));
}

int32_t recvAccessoryConfiguration(struct AccessoryConfiguration* msg, iAP2AccessoryConfig_t* acc)
{
    acc->iAP2iOSintheCar        = msg->accConfig.iAP2iOSintheCar;
    acc->iAP2EANativeTransport  = msg->accConfig.iAP2EANativeTransport;
    acc->iAP2TransportType      = msg->accConfig.iAP2TransportType;
    acc->iAP2AuthenticationType = msg->accConfig.iAP2AuthenticationType;
    acc->iAP2AuthShortWait      = msg->accConfig.iAP2AuthShortWait;
    acc->iAP2AuthWait           = msg->accConfig.iAP2AuthWait;
    acc->iAP2AuthLongWait       = msg->accConfig.iAP2AuthLongWait;
    acc->iAP2AccessoryPowerMode = msg->accConfig.iAP2AccessoryPowerMode;
    acc->iAP2FileXferRcvAsStream= msg->accConfig.iAP2FileXferRcvAsStream;
    acc->iAP2EAPSupported       = msg->accConfig.iAP2EAPSupported;
    acc->iAP2FileXferSupported  = msg->accConfig.iAP2FileXferSupported;
    acc->useConfigFS            = msg->accConfig.useConfigFS;
    acc->ManualLinkConfig       = msg->accConfig.ManualLinkConfig;
    acc->LinkConfig_SessionVersion = msg->accConfig.LinkConfig_SessionVersion;
    acc->iAP2AvailableCurrentForDevice = msg->accConfig.iAP2AvailableCurrentForDevice;
    acc->iAP2MaximumcurrentDrawnFromAccessory = msg->accConfig.iAP2MaximumcurrentDrawnFromAccessory;
    acc->iAP2DeviceBatteryWillChargeIfPowerIsPresent = msg->accConfig.iAP2DeviceBatteryWillChargeIfPowerIsPresent;
    acc->iAP2DeviceBatteryShouldChargeIfPowerIsPresent = msg->accConfig.iAP2DeviceBatteryShouldChargeIfPowerIsPresent;

    if(strnlen((const char*)msg->accConfig.iAP2UsbOtgGPIOPower, STRING_MAX) > 0)
    {
        acc->iAP2UsbOtgGPIOPower = (U8*)strndup( (const char*) msg->accConfig.iAP2UsbOtgGPIOPower, strnlen((const char*) msg->accConfig.iAP2UsbOtgGPIOPower, STRING_MAX) );
    }
    if(strnlen((const char*)msg->accConfig.iAP2AuthDevicename, STRING_MAX) > 0)
    {
        acc->iAP2AuthDevicename = (U8*)strndup( (const char*) msg->accConfig.iAP2AuthDevicename, strnlen((const char*) msg->accConfig.iAP2AuthDevicename, STRING_MAX) );
    }
    if(strnlen((const char*)msg->accConfig.iAP2AuthIoctlRegAddr, STRING_MAX) > 0)
    {
        acc->iAP2AuthIoctlRegAddr = (U8*)strndup( (const char*) msg->accConfig.iAP2AuthIoctlRegAddr, strnlen((const char*) msg->accConfig.iAP2AuthIoctlRegAddr, STRING_MAX) );
    }
    if(strnlen((const char*)msg->accConfig.iAP2AuthGPIOReady, STRING_MAX) > 0)
    {
        acc->iAP2AuthGPIOReady = (U8*)strndup( (const char*) msg->accConfig.iAP2AuthGPIOReady, strnlen((const char*) msg->accConfig.iAP2AuthGPIOReady, STRING_MAX) );
    }
    if(strnlen((const char*)msg->accConfig.iAP2AuthGPIOReset, STRING_MAX) > 0)
    {
        acc->iAP2AuthGPIOReset = (U8*)strndup( (const char*) msg->accConfig.iAP2AuthGPIOReset, strnlen((const char*) msg->accConfig.iAP2AuthGPIOReset, STRING_MAX) );
    }
    if(strnlen((const char*)msg->accConfig.UdcDeviceName, STRING_MAX) > 0)
    {
        acc->UdcDeviceName = (U8*)strndup( (const char*) msg->accConfig.UdcDeviceName, strnlen((const char*) msg->accConfig.UdcDeviceName, STRING_MAX) );
    }

    return 0;
}

int32_t sendAccessoryIdentificationBasic(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    struct AccessoryIdentficiation msg;
    memset(&msg, 0, sizeof(struct AccessoryIdentficiation));
    msg.header.type = MessageType::AccessoryIdentficiation;

    iAP2ServiceAccIdentification_t* accId = &msg.identification;

    if(initAccIdParams->iAP2AccessoryName)
    {
        strncpy(accId->iAP2AccessoryName, (const char*)initAccIdParams->iAP2AccessoryName, strnlen((const char*)initAccIdParams->iAP2AccessoryName, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessoryModelIdentifier)
    {
        strncpy(accId->iAP2AccessoryModelIdentifier, (const char*)initAccIdParams->iAP2AccessoryModelIdentifier, strnlen((const char*)initAccIdParams->iAP2AccessoryModelIdentifier, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessoryManufacturer)
    {
        strncpy(accId->iAP2AccessoryManufacturer, (const char*)initAccIdParams->iAP2AccessoryManufacturer, strnlen((const char*)initAccIdParams->iAP2AccessoryManufacturer, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessorySerialNumber)
    {
        strncpy(accId->iAP2AccessorySerialNumber, (const char*)initAccIdParams->iAP2AccessorySerialNumber, strnlen((const char*)initAccIdParams->iAP2AccessorySerialNumber, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessoryFirmwareVersion)
    {
        strncpy(accId->iAP2AccessoryFirmwareVersion, (const char*)initAccIdParams->iAP2AccessoryFirmwareVersion, strnlen((const char*)initAccIdParams->iAP2AccessoryFirmwareVersion, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessoryHardwareVersion)
    {
        strncpy(accId->iAP2AccessoryHardwareVersion, (const char*)initAccIdParams->iAP2AccessoryHardwareVersion, strnlen((const char*)initAccIdParams->iAP2AccessoryHardwareVersion, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessoryBcdDevice)
    {
        strncpy(accId->iAP2AccessoryBcdDevice, (const char*)initAccIdParams->iAP2AccessoryBcdDevice, strnlen((const char*)initAccIdParams->iAP2AccessoryBcdDevice, STRING_MAX));
    }
    if(initAccIdParams->iAP2InitEndPoint)
    {
        strncpy(accId->iAP2InitEndPoint, (const char*)initAccIdParams->iAP2InitEndPoint, strnlen((const char*)initAccIdParams->iAP2InitEndPoint, STRING_MAX));
    }
    if(initAccIdParams->iAP2CurrentLanguage)
    {
        strncpy(accId->iAP2CurrentLanguage, (const char*)initAccIdParams->iAP2CurrentLanguage, strnlen((const char*)initAccIdParams->iAP2CurrentLanguage, LANGUAGE_LENGTH));
    }
    if(initAccIdParams->iAP2AccessoryVendorId)
    {
        strncpy(accId->iAP2AccessoryVendorId, (const char*)initAccIdParams->iAP2AccessoryVendorId, strnlen((const char*)initAccIdParams->iAP2AccessoryVendorId, STRING_MAX));
    }
    if(initAccIdParams->iAP2AccessoryProductId)
    {
        strncpy(accId->iAP2AccessoryProductId, (const char*)initAccIdParams->iAP2AccessoryProductId, strnlen((const char*)initAccIdParams->iAP2AccessoryProductId, STRING_MAX));
    }
    if(initAccIdParams->iAP2ProductPlanUUID)
    {
        strncpy(accId->iAP2ProductPlanUUID, (const char*)initAccIdParams->iAP2ProductPlanUUID, strnlen((const char*)initAccIdParams->iAP2ProductPlanUUID, STRING_MAX));
    }

    accId->iAP2SupportsiOSintheCar    = initAccIdParams->iAP2SupportsiOSintheCar;
    accId->iAP2MaximumCurrentDrawnFromDevice = initAccIdParams->iAP2MaximumCurrentDrawnFromDevice;

    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Accessory Identification");
    return iAP2ServiceSendMessage(service, &msg, sizeof(msg));
}

int32_t recvAccessoryIdentification(struct AccessoryIdentficiation* param, iAP2AccessoryInfo_t* accInfo)
{
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    iAP2ServiceAccIdentification_t* src = &param->identification;
    int32_t rc = IAP2_OK;

    if(strnlen((const char*)src->iAP2AccessoryName, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryName = (U8*)strndup( (const char*)src->iAP2AccessoryName, strnlen((const char*)src->iAP2AccessoryName, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessoryModelIdentifier, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryModelIdentifier = (U8*)strndup( (const char*)src->iAP2AccessoryModelIdentifier, strnlen((const char*)src->iAP2AccessoryModelIdentifier, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessoryManufacturer, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryManufacturer = (U8*)strndup( (const char*)src->iAP2AccessoryManufacturer, strnlen((const char*)src->iAP2AccessoryManufacturer, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessorySerialNumber, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessorySerialNumber = (U8*)strndup( (const char*)src->iAP2AccessorySerialNumber, strnlen((const char*)src->iAP2AccessorySerialNumber, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessoryFirmwareVersion, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryFirmwareVersion = (U8*)strndup( (const char*)src->iAP2AccessoryFirmwareVersion, strnlen((const char*)src->iAP2AccessoryFirmwareVersion, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessoryHardwareVersion, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryHardwareVersion = (U8*)strndup( (const char*)src->iAP2AccessoryHardwareVersion, strnlen((const char*)src->iAP2AccessoryHardwareVersion, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessoryBcdDevice, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryBcdDevice = (U8*)strndup( (const char*)src->iAP2AccessoryBcdDevice, strnlen((const char*)src->iAP2AccessoryBcdDevice, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2InitEndPoint, STRING_MAX) > 0)
    {
        accInfo->iAP2InitEndPoint = (U8*)strndup( (const char*)src->iAP2InitEndPoint, strnlen((const char*)src->iAP2InitEndPoint, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2CurrentLanguage, LANGUAGE_LENGTH) > 0)
    {
        accInfo->iAP2CurrentLanguage = (U8*)strndup( (const char*)src->iAP2CurrentLanguage, strnlen((const char*) src->iAP2CurrentLanguage, LANGUAGE_LENGTH) );
    }
    if(strnlen((const char*)src->iAP2AccessoryVendorId, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryVendorId = (U8*)strndup( (const char*)src->iAP2AccessoryVendorId, strnlen((const char*) src->iAP2AccessoryVendorId, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2AccessoryProductId, STRING_MAX) > 0)
    {
        accInfo->iAP2AccessoryProductId = (U8*)strndup( (const char*)src->iAP2AccessoryProductId, strnlen((const char*) src->iAP2AccessoryProductId, STRING_MAX) );
    }
    if(strnlen((const char*)src->iAP2ProductPlanUUID, STRING_MAX) > 0)
    {
        accInfo->iAP2ProductPlanUUID = (U8*)strndup( (const char*)src->iAP2ProductPlanUUID, strnlen((const char*)src->iAP2ProductPlanUUID, STRING_MAX) );
    }

    accInfo->iAP2SupportsiOSintheCar = src->iAP2SupportsiOSintheCar;
    accInfo->iAP2MaximumCurrentDrawnFromDevice = src->iAP2MaximumCurrentDrawnFromDevice;
    return rc;
}

int32_t sendAccessorySupportedLanguages(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    int32_t rc = IAP2_OK;
    if(initAccIdParams->iAP2SupportedLanguage && (initAccIdParams->iAP2SupportedLanguageCount > 0))
    {
        struct AccessorySupportedLanguages* languages;
        char languageString[STRING_MAX] = {'\0'};
        memset(languageString, 0, sizeof(languageString));
        for(int i = 0; i < initAccIdParams->iAP2SupportedLanguageCount; ++i)
        {
            int len = strnlen((const char*)initAccIdParams->iAP2SupportedLanguage[i], STRING_MAX);
            if(len > (LANGUAGE_LENGTH - 1))
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "sendConfigurationMessage: Language Length is greater than 3 characters");
                return IAP2_CTL_ERROR;
            }
            strncat(languageString, (const char*)initAccIdParams->iAP2SupportedLanguage[i], len);
            strcat(languageString, ":");
        }
        int languageStringLength = strnlen(languageString, STRING_MAX);
        /* Replace the last colon ":" by NULL character
         * Eg. Replace "en:de:" by "en:de"
         */
        languageString[languageStringLength - 1] = '\0';
        languageStringLength -= 1;

        IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "languages: %s length:%d", languageString, languageStringLength);

        /* Allocate one byte extra for NULL character */
        languages = (struct AccessorySupportedLanguages*)calloc(1, sizeof(*languages) + languageStringLength + 1);
        if(languages)
        {
            languages->header.type = MessageType::AccessorySupportedLanguages;
            languages->languages.iAP2SupportedLanguageCount = initAccIdParams->iAP2SupportedLanguageCount;

            memcpy(languages->languages.iAP2SupportedLanguage, languageString, languageStringLength + 1);

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Languages supported");

            rc = iAP2ServiceSendMessage(service, languages, sizeof(*languages) + languageStringLength + 1);
            free(languages);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "languages is NULL, rc=%d",rc);
        }
    }
    return rc;
}

int32_t recvSupportedLanguages(struct AccessorySupportedLanguages* msg, iAP2AccessoryInfo_t* accInfo)
{
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    int32_t rc = IAP2_OK;
    accInfo->iAP2SupportedLanguageCount = msg->languages.iAP2SupportedLanguageCount;
    accInfo->iAP2SupportedLanguage      = (U8**)calloc(sizeof(U8*), accInfo->iAP2SupportedLanguageCount);
    if(accInfo->iAP2SupportedLanguage != NULL)
    {
        char* str = (char*)msg->languages.iAP2SupportedLanguage;
        int i = 0;
        //tockenize the language string e.g. {en:de:fr} and store it
        do
        {
            const char* begin = str;

            while(*str != ':' && *str)
                str++;

            accInfo->iAP2SupportedLanguage[i] = (U8*)calloc(1, (size_t)(str-begin+1));
            if(accInfo->iAP2SupportedLanguage[i] != NULL)
            {
                strncpy((char*)accInfo->iAP2SupportedLanguage[i], begin, (size_t)(str-begin));
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "Language: %s Index:%d", accInfo->iAP2SupportedLanguage[i], i);
                ++i;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2SupportedLanguage[%d] is NULL", i);
            }
        } while ((0 != *str++) && (rc == IAP2_OK) && (i < accInfo->iAP2SupportedLanguageCount));
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2SupportedLanguage is NULL, rc=%d", rc);
    }

    return rc;
}

int32_t sendAccessorySupportedAudioRates(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    int32_t rc = IAP2_OK;
    if((initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate) && (initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count > 0))
    {
        struct USBDeviceAudioSampleRates* audioRates;
        uint32_t length = sizeof(iAP2USBDeviceModeAudioSampleRate) * initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count;
        audioRates = (struct USBDeviceAudioSampleRates*)calloc(1, sizeof(struct USBDeviceAudioSampleRates)+length);
        if(audioRates == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "audioRates is NULL, rc=%d", rc);
        }
        else
        {
            memcpy(audioRates->sampleRates.iAP2USBDeviceSupportedAudioSampleRate, initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate, length);
            audioRates->sampleRates.iAP2USBDeviceSupportedAudioSampleRate_count = initAccIdParams->iAP2USBDeviceSupportedAudioSampleRate_count;
            audioRates->header.type = MessageType::USBDeviceAudioSampleRates;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Audio Sample Rates");

            rc = iAP2ServiceSendMessage(service, audioRates, sizeof(*audioRates) + length);

            free(audioRates);
        }
    }
    return rc;
}

int32_t recvSupportedAudioRates(struct USBDeviceAudioSampleRates* msg, iAP2AccessoryInfo_t* accInfo)
{
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    int32_t rc = IAP2_OK;

    if(msg->sampleRates.iAP2USBDeviceSupportedAudioSampleRate_count > 0)
    {
        accInfo->iAP2USBDeviceSupportedAudioSampleRate_count = msg->sampleRates.iAP2USBDeviceSupportedAudioSampleRate_count;
        accInfo->iAP2USBDeviceSupportedAudioSampleRate = (iAP2USBDeviceModeAudioSampleRate*)calloc(1, sizeof(iAP2USBDeviceModeAudioSampleRate) * accInfo->iAP2USBDeviceSupportedAudioSampleRate_count);
        if(accInfo->iAP2USBDeviceSupportedAudioSampleRate == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2USBDeviceSupportedAudioSampleRate is NULL, rc=%d", rc);
        }
        else
        {
            memcpy(accInfo->iAP2USBDeviceSupportedAudioSampleRate, msg->sampleRates.iAP2USBDeviceSupportedAudioSampleRate, sizeof(iAP2USBDeviceModeAudioSampleRate) * accInfo->iAP2USBDeviceSupportedAudioSampleRate_count);
        }
    }

    return rc;
}

int32_t sendAccessoryVehicleInformation(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    int32_t rc = IAP2_OK;
    if(initAccIdParams->iAP2VehicleInformationComponent)
    {
        struct VehicleInformation* vehicleInfo;
        uint32_t length = initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count * sizeof(iAP2EngineTypes);
        vehicleInfo = (struct VehicleInformation*)calloc(1, sizeof(struct VehicleInformation) + length);
        if(vehicleInfo)
        {
            vehicleInfo->header.type = MessageType::VehicleInformation;

            if(initAccIdParams->iAP2VehicleInformationComponent->iAP2DisplayName)
            {
                strncpy(vehicleInfo->vehicle.iAP2DisplayName, (const char*)initAccIdParams->iAP2VehicleInformationComponent->iAP2DisplayName, strnlen((const char*)initAccIdParams->iAP2VehicleInformationComponent->iAP2DisplayName, STRING_MAX));
            }
            if(initAccIdParams->iAP2VehicleInformationComponent->iAP2MapsDisplayName)
            {
                strncpy(vehicleInfo->vehicle.iAP2MapsDisplayName, (const char*)initAccIdParams->iAP2VehicleInformationComponent->iAP2MapsDisplayName, strnlen((const char*)initAccIdParams->iAP2VehicleInformationComponent->iAP2MapsDisplayName, STRING_MAX));
            }
            vehicleInfo->vehicle.iAP2EngineType_count = initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType_count;
            if(initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType)
            {
                memcpy(vehicleInfo->vehicle.iAP2EngineType, initAccIdParams->iAP2VehicleInformationComponent->iAP2EngineType, length);
            }
            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Vehicle Information");

            rc = iAP2ServiceSendMessage(service, vehicleInfo, sizeof(*vehicleInfo) + length);
            free(vehicleInfo);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "vehicleInfo is NULL, rc=%d", rc);
        }
    }
    return rc;
}

int32_t recvVehicleInformation(struct VehicleInformation* msg, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    accInfo->iAP2VehicleInformationComponent = (iAP2VehicleInformationComponent_t*)calloc(1, sizeof(iAP2VehicleInformationComponent_t));
    if(accInfo->iAP2VehicleInformationComponent != NULL)
    {
        if(strnlen((const char *)msg->vehicle.iAP2DisplayName, STRING_MAX) > 0)
        {
            accInfo->iAP2VehicleInformationComponent->iAP2DisplayName = (U8*)strndup(msg->vehicle.iAP2DisplayName, strnlen(msg->vehicle.iAP2DisplayName, STRING_MAX));
        }
        if(strnlen((const char *)msg->vehicle.iAP2MapsDisplayName, STRING_MAX) > 0)
        {
            accInfo->iAP2VehicleInformationComponent->iAP2MapsDisplayName = (U8*)strndup(msg->vehicle.iAP2MapsDisplayName, strnlen(msg->vehicle.iAP2MapsDisplayName, STRING_MAX));
        }
        accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count = msg->vehicle.iAP2EngineType_count;
        accInfo->iAP2VehicleInformationComponent->iAP2EngineType = (iAP2EngineTypes*)calloc(1, sizeof(iAP2EngineTypes) * accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count);
        if(accInfo->iAP2VehicleInformationComponent->iAP2EngineType != NULL)
        {
            memcpy(accInfo->iAP2VehicleInformationComponent->iAP2EngineType, msg->vehicle.iAP2EngineType, sizeof(iAP2EngineTypes) * msg->vehicle.iAP2EngineType_count);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "setVehicleInformation Failed!! rc=%d", rc);
        }
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "setVehicleInformation Failed!! rc=%d", rc);
    }

    return rc;
}

int32_t sendAccessoryVehicleStatus(iAP2Service_t* service, iAP2AccessoryInfo_t *initAccIdParams)
{
    int32_t rc = IAP2_OK;
    if(initAccIdParams->iAP2VehicleStatusComponent)
    {
        struct VehicleStatus* vehicleStatus;
        vehicleStatus = (struct VehicleStatus*)calloc(1, sizeof(struct VehicleStatus));
        if(vehicleStatus)
        {
            vehicleStatus->header.type = MessageType::VehicleStatus;
            vehicleStatus->iAP2ServiceVehicleStatus = *initAccIdParams->iAP2VehicleStatusComponent;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Vehicle Status");

            rc = iAP2ServiceSendMessage(service, vehicleStatus, sizeof(*vehicleStatus));
            free(vehicleStatus);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "sendVehicleStatus Failed!! rc=%d", rc);
        }
    }
    return rc;
}

int32_t recvVehicleStatus(struct VehicleStatus* msg, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    accInfo->iAP2VehicleStatusComponent = (iAP2VehicleStatusComponent_t*)calloc(1,sizeof(iAP2VehicleStatusComponent_t));
    if(accInfo->iAP2VehicleStatusComponent != NULL)
    {
        *accInfo->iAP2VehicleStatusComponent = msg->iAP2ServiceVehicleStatus;
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "setVehicleStatus Failed!! rc=%d", rc);
    }

    return rc;
}

int32_t sendAccessoryLocationInformation(iAP2Service_t* service, iAP2AccessoryInfo_t *initAccIdParams)
{
    int32_t rc = IAP2_OK;
    if(initAccIdParams->iAP2LocationInformationComponent)
    {
        struct LocationInformation* locationData;
        locationData = (struct LocationInformation*)calloc(1, sizeof(struct LocationInformation));
        if(locationData)
        {
            locationData->header.type = MessageType::LocationInformation;
            locationData->iAP2ServiceLocationInformation = *initAccIdParams->iAP2LocationInformationComponent;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Location Information");

            rc = iAP2ServiceSendMessage(service, locationData, sizeof(*locationData));
            free(locationData);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "sendLocationInformation Failed!! rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvLocationInformation(struct LocationInformation* msg, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    accInfo->iAP2LocationInformationComponent = (iAP2LocationInformationComponent_t*)calloc(1,sizeof(iAP2LocationInformationComponent_t));
    if(accInfo->iAP2LocationInformationComponent != NULL)
    {
        *accInfo->iAP2LocationInformationComponent = msg->iAP2ServiceLocationInformation;
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "setLocationInformation Failed!! rc=%d", rc);
    }

    return rc;
}

int32_t sendMessagesSentByApplication(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    uint32_t rc = IAP2_OK;
    if(initAccIdParams->iAP2CommandsUsedByApplication)
    {
        struct MessagsSentByApplication* appMessages = NULL;
        uint16_t length = initAccIdParams->iAP2CommandsUsedByApplication_length;
        appMessages = (struct MessagsSentByApplication*) calloc(1, sizeof(struct MessagsSentByApplication) + length);
        if(appMessages != NULL)
        {
            appMessages->header.type = MessageType::MessagsSentByApplication;
            appMessages->msgList.length = length;
            memcpy(appMessages->msgList.commandList, initAccIdParams->iAP2CommandsUsedByApplication, length);

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Messages Send By Application length:%d", length);

            rc = iAP2ServiceSendMessage(service, appMessages, sizeof(*appMessages) + length);
            free(appMessages);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "appMessages is NULL, rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvMessageSentByApplication(struct MessagsSentByApplication* msg, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;

    accInfo->iAP2CommandsUsedByApplication = (U16*)calloc(1, msg->msgList.length);
    if(accInfo->iAP2CommandsUsedByApplication != NULL)
    {
        memcpy(accInfo->iAP2CommandsUsedByApplication, msg->msgList.commandList, msg->msgList.length);
        accInfo->iAP2CommandsUsedByApplication_length = msg->msgList.length;
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2CommandsUsedByApplication is NULL, rc=%d", rc);
    }
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "iAP2CommandsUsedByApplication: %p iAP2CommandsUsedByApplication_length:%d", accInfo->iAP2CommandsUsedByApplication, msg->msgList.length);
    return rc;
}


int32_t sendCallbacksExpectedFromDevice(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    uint32_t rc = IAP2_OK;
    if(initAccIdParams->iAP2CallbacksExpectedFromDevice)
    {
        struct CallbacksExpectedFromDevice* callbackMessages = NULL;
        int16_t length = initAccIdParams->iAP2CallbacksExpectedFromDevice_length;
        callbackMessages = (struct CallbacksExpectedFromDevice*) calloc(1, sizeof(struct CallbacksExpectedFromDevice) + length);
        if(callbackMessages != NULL)
        {
            callbackMessages->header.type = MessageType::CallbacksExpectedFromDevice;
            callbackMessages->msgList.length = length;
            memcpy(callbackMessages->msgList.commandList, initAccIdParams->iAP2CallbacksExpectedFromDevice, length);

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Callbacks expected length:%d", length);

            rc = iAP2ServiceSendMessage(service, callbackMessages, sizeof(*callbackMessages) + length);
            free(callbackMessages);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "callbackMessages is NULL, rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvCallbacksExpectedFromDevice(struct CallbacksExpectedFromDevice* msg, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;

    accInfo->iAP2CallbacksExpectedFromDevice = (U16*)calloc(1, msg->msgList.length);
    if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
    {
        memcpy(accInfo->iAP2CallbacksExpectedFromDevice, msg->msgList.commandList, msg->msgList.length);
        accInfo->iAP2CallbacksExpectedFromDevice_length = msg->msgList.length;
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2CallbacksExpectedFromDevice is NULL, rc=%d", rc);
    }
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "iAP2CallbacksExpectedFromDevice: %p iAP2CallbacksExpectedFromDevice_length:%d", accInfo->iAP2CallbacksExpectedFromDevice, msg->msgList.length);

    return rc;
}


int32_t sendAccessorySupportediOSApps(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    uint32_t rc = IAP2_OK;
    struct AccessorySupportediOSApps* iOsApps = NULL;
    if(initAccIdParams->iAP2SupportediOSAppCount > 0)
    {
        uint16_t length = initAccIdParams->iAP2SupportediOSAppCount * sizeof(iAP2ServiceiOSAppInfo_t);
        iOsApps = (struct AccessorySupportediOSApps*)calloc(1, sizeof(*iOsApps) + length);
        if(iOsApps)
        {
            iOsApps->header.type = MessageType::AccessorySupportediOSApps;
            for(uint32_t i = 0; i < initAccIdParams->iAP2SupportediOSAppCount; ++i)
            {
                iOsApps->apps.iAP2iOSAppInfo[i].iAP2EANativeTransport = initAccIdParams->iAP2iOSAppInfo[i].iAP2EANativeTransport;
                iOsApps->apps.iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay = initAccIdParams->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay;
                iOsApps->apps.iAP2iOSAppInfo[i].iAP2EAPMatchAction = initAccIdParams->iAP2iOSAppInfo[i].iAP2EAPMatchAction;
                iOsApps->apps.iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier;
                strncpy(iOsApps->apps.iAP2iOSAppInfo[i].iAP2iOSAppName, (const char*)initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName, strnlen((char*)initAccIdParams->iAP2iOSAppInfo[i].iAP2iOSAppName, STRING_MAX));
            }
            iOsApps->apps.iAP2SupportediOSAppCount = initAccIdParams->iAP2SupportediOSAppCount;

            if(initAccIdParams->iAP2PreferredAppBundleSeedIdentifier != NULL)
                strncpy(iOsApps->apps.iAP2PreferredAppBundleSeedIdentifier, (char*)initAccIdParams->iAP2PreferredAppBundleSeedIdentifier, strnlen((char*)initAccIdParams->iAP2PreferredAppBundleSeedIdentifier, STRING_MAX));

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: Supported_iOS_applications");

            rc = iAP2ServiceSendMessage(service, iOsApps, sizeof(*iOsApps)+length);
            free(iOsApps);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iOsApps is NULL, rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvSupportediOSAppInfo(struct AccessorySupportediOSApps* appInfo, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    accInfo->iAP2SupportediOSAppCount = appInfo->apps.iAP2SupportediOSAppCount;
    if(accInfo->iAP2SupportediOSAppCount > 0)
    {
        accInfo->iAP2iOSAppInfo = (iAP2iOSAppInfo_t*)calloc(accInfo->iAP2SupportediOSAppCount, sizeof(iAP2iOSAppInfo_t) );
        if(accInfo->iAP2iOSAppInfo)
        {
            for(uint32_t i=0;( (i<accInfo->iAP2SupportediOSAppCount) && (rc == IAP2_OK) );i++)
            {
                iAP2ServiceiOSAppInfo_t* app = &appInfo->apps.iAP2iOSAppInfo[i];
                accInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = app->iAP2iOSAppIdentifier;
                accInfo->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay   = app->iAP2ExternalAccessoryProtocolCarPlay;
                accInfo->iAP2iOSAppInfo[i].iAP2EAPMatchAction   = app->iAP2EAPMatchAction;
                accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName = (U8*)strndup((const char*)&app->iAP2iOSAppName, strnlen((const char*)&app->iAP2iOSAppName, STRING_MAX));

                /* the first specified iOS App communicates via EA native transport */
                accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport = app->iAP2EANativeTransport;
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2iOSAppInfo/iAP2PreferredAppBundleSeedIdentifier is NULL, rc=%d", rc);
        }

        if( '\0' != appInfo->apps.iAP2PreferredAppBundleSeedIdentifier[0] )
        {
            accInfo->iAP2PreferredAppBundleSeedIdentifier = (U8*)calloc(1,STRING_MAX);

            if(accInfo->iAP2PreferredAppBundleSeedIdentifier)
            {
                strncpy((char*)accInfo->iAP2PreferredAppBundleSeedIdentifier, (char*)appInfo->apps.iAP2PreferredAppBundleSeedIdentifier, strnlen((char*)appInfo->apps.iAP2PreferredAppBundleSeedIdentifier, STRING_MAX));
            }
        }
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "No iOS App configured");
    }
    return rc;
}

int32_t sendInformationComplete(iAP2Service_t* service)
{
    struct IdentificationInfoComplete complete;
    complete.header.type = MessageType::IdentificationInfoComplete;
    complete.header.deviceId = 0;
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: IdentificationInfoComplete");
    return iAP2ServiceSendMessage(service, &complete, sizeof(complete));
}

int32_t sendUSBHID(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    uint32_t rc = IAP2_OK;
    if((initAccIdParams->iAP2USBHIDComponent) && (initAccIdParams->iAP2USBHIDComponent_count > 0))
    {
        struct USBDeviceHID* msg;
        int32_t length = 0;
        length = sizeof(*msg) + sizeof(iAP2ServiceHID_t) * initAccIdParams->iAP2USBHIDComponent_count;
        msg = (struct USBDeviceHID*)calloc(1, length);
        if(msg)
        {
            for(int i=0; ((i<initAccIdParams->iAP2USBHIDComponent_count) && (rc == IAP2_OK)); i++)
            {
                if(initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName)
                {
                    strncpy((char*)msg->hid[i].iAP2HIDComponentName, (char*)*initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName, strnlen((char*)*initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentName, STRING_MAX));
                }
                msg->hid[i].iAP2HIDComponentIdentifier = *initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier;
                if(initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction)
                {
                    msg->hid[i].iAP2HIDComponentFunction = *initAccIdParams->iAP2USBHIDComponent[i].iAP2HIDComponentFunction;
                }

                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: USBDeviceHID count:%u name:%s id:%d", initAccIdParams->iAP2USBHIDComponent_count, msg->hid[i].iAP2HIDComponentName, msg->hid[i].iAP2HIDComponentIdentifier);
            }

            msg->header.type = MessageType::USBDeviceHID;
            msg->count = initAccIdParams->iAP2USBHIDComponent_count;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: USBDeviceHID count:%u", initAccIdParams->iAP2USBHIDComponent_count);
            if(length > 0)
            {
                rc = iAP2ServiceSendMessage(service, msg, length);
                free(msg);
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "USBDeviceHID msg is NULL, rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvUSBHIDInformation(struct USBDeviceHID* message, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;
    if(message->count > 0)
    {
        accInfo->iAP2USBHIDComponent = (iAP2iAP2HIDComponent*)calloc(message->count, sizeof(iAP2iAP2HIDComponent));
        if(accInfo->iAP2USBHIDComponent != NULL)
        {
            for(int i = 0; ((i < message->count) && (rc == IAP2_OK)); i++)
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "name: %s %d %d", message->hid[i].iAP2HIDComponentName, message->hid[i].iAP2HIDComponentFunction, message->hid[i].iAP2HIDComponentIdentifier);

                U8* p_HIDCompName = message->hid[i].iAP2HIDComponentName;
                rc = iAP2AllocateandUpdateData(&accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName,
                                               &p_HIDCompName,
                                               &accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName_count,
                                               1, iAP2_utf8, 0);
                if(rc == IAP2_OK)
                {
                    accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                    if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction != NULL)
                    {
                        *(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction) = message->hid[i].iAP2HIDComponentFunction;
                        accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction_count++;
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2HIDComponentFunction is NULL, rc=%d", rc);
                    }
                }
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier,
                                                   &message->hid[i].iAP2HIDComponentIdentifier,
                                                   &accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2USBHIDComponent is NULL, rc=%d", rc);
        }
        if(rc == IAP2_OK)
        {
            accInfo->iAP2USBHIDComponent_count = message->count;
        }
    }
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    return rc;
}


int32_t sendUSBHostHID(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    uint32_t rc = IAP2_OK;
    struct USBHostHID* msg;
    int32_t length = 0;
    if((initAccIdParams->iAP2USBHostHIDComponent) && (initAccIdParams->iAP2USBHostHIDComponent_count > 0))
    {
        length = sizeof(*msg) + sizeof(iAP2ServiceUSBHostHID_t) * initAccIdParams->iAP2USBHostHIDComponent_count;
        msg = (struct USBHostHID*)calloc(1, length);
        if(msg != NULL)
        {
            for(int i=0; ((i<initAccIdParams->iAP2USBHostHIDComponent_count) && (rc == IAP2_OK)); i++)
            {
                memcpy(msg->hid[i].iAP2HIDComponentName, initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentName, strnlen((char*)*initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentName, STRING_MAX));
                msg->hid[i].iAP2HIDComponentIdentifier              = *initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier;
                msg->hid[i].iAP2HIDComponentFunction                = *initAccIdParams->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction;
                msg->hid[i].iAP2USBHostTransportComponentIdentifier = *initAccIdParams->iAP2USBHostHIDComponent[i].iAP2USBHostTransportComponentIdentifier;
                msg->hid[i].iAP2USBHostTransportInterfaceNumber     = *initAccIdParams->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber;
            }

            msg->header.type = MessageType::USBHostHID;
            msg->count = initAccIdParams->iAP2USBHostHIDComponent_count;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: USBHostHID");
            if(length > 0)
            {
                rc = iAP2ServiceSendMessage(service, msg, length);
                free(msg);
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "USBHostHID msg is NULL, rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvUSBHostHIDInformation(struct USBHostHID* message, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_OK;
    if(message->count > 0)
    {
        accInfo->iAP2USBHostHIDComponent = (iAP2USBHostHIDComponent*)calloc(message->count, sizeof(iAP2USBHostHIDComponent));
        if(accInfo->iAP2USBHostHIDComponent != NULL)
        {
            for(int i = 0; ((i < message->count) && (rc == IAP2_OK)); i++)
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "name: %s %d %d", message->hid[i].iAP2HIDComponentName, message->hid[i].iAP2HIDComponentFunction, message->hid[i].iAP2HIDComponentIdentifier);

                U8* p_HIDCompName = message->hid[i].iAP2HIDComponentName;
                rc = iAP2AllocateandUpdateData(&accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentName,
                                               &p_HIDCompName,
                                               &accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentName_count,
                                               1, iAP2_utf8, 0);
                if(rc == IAP2_OK)
                {
                    accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                    if(accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction != NULL)
                    {
                        *(accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction) = message->hid[i].iAP2HIDComponentFunction;
                        accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentFunction_count++;
                    }
                    else
                    {
                        rc = IAP2_ERR_NO_MEM;
                    }
                }
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier,
                                                   &message->hid[i].iAP2HIDComponentIdentifier,
                                                   &accInfo->iAP2USBHostHIDComponent[i].iAP2HIDComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2USBHostHIDComponent[i].iAP2USBHostTransportComponentIdentifier,
                                                   &message->hid[i].iAP2USBHostTransportComponentIdentifier,
                                                   &accInfo->iAP2USBHostHIDComponent[i].iAP2USBHostTransportComponentIdentifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber,
                                                   &message->hid[i].iAP2USBHostTransportInterfaceNumber,
                                                   &accInfo->iAP2USBHostHIDComponent[i].iAP2USBHostTransportInterfaceNumber_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2USBHostHIDComponent is NULL, rc=%d", rc);
        }
        if(rc == IAP2_OK)
        {
            accInfo->iAP2USBHostHIDComponent_count = message->count;
        }
    }
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, " entered");
    return rc;
}


int32_t sendBluetoothHID(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    uint32_t rc = IAP2_OK;
    struct BluetoothHID* msg;
    int32_t length = 0;
    if(initAccIdParams->iAP2BluetoothHIDComponent)
    {
        length = sizeof(*msg);
        msg = (struct BluetoothHID*)calloc(1, length);
        if(msg != NULL)
        {
            memcpy(msg->hid.iAP2HIDComponentName, initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentName, strnlen((char*)*initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentName, STRING_MAX));
            msg->hid.iAP2HIDComponentIdentifier = *initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier;
            msg->hid.iAP2HIDComponentFunction = *initAccIdParams->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction;
            msg->hid.iAP2BluetoothTransportComponentIdentifier = *initAccIdParams->iAP2BluetoothHIDComponent->iAP2BluetoothTransportComponentIdentifier;

            msg->header.type = MessageType::BluetoothHID;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: BluetoothHID");
            if(length > 0)
            {
                rc = iAP2ServiceSendMessage(service, msg, length);
                free(msg);
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "BluetoothHID msg is NULL, rc=%d", rc);
        }
    }

    return rc;
}

int32_t recvBluetoothHIDInformation(struct BluetoothHID* message, iAP2AccessoryInfo_t* accInfo)
{
    int32_t rc = IAP2_ERR_NO_MEM;
    accInfo->iAP2BluetoothHIDComponent = (iAP2BluetoothHIDComponent*)calloc(1, sizeof(iAP2BluetoothHIDComponent));
    if(accInfo->iAP2BluetoothHIDComponent)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "name: %s %d %d", message->hid.iAP2HIDComponentName, message->hid.iAP2HIDComponentFunction, message->hid.iAP2HIDComponentIdentifier);

        U8* p_HIDCompName = message->hid.iAP2HIDComponentName;
        rc = iAP2AllocateandUpdateData(&accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentName,
                                       &p_HIDCompName,
                                       &accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentName_count,
                                       1, iAP2_utf8, 0);
        if(rc == IAP2_OK)
        {
            accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
            if(accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction != NULL)
            {
                *(accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction) = message->hid.iAP2HIDComponentFunction;
                accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentFunction_count++;
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2BluetoothHIDComponent->iAP2HIDComponentFunction is NULL, rc=%d", rc);
            }
        }
        if(rc == IAP2_OK)
        {
            rc = iAP2AllocateandUpdateData(&accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier,
                                           &message->hid.iAP2HIDComponentIdentifier,
                                           &accInfo->iAP2BluetoothHIDComponent->iAP2HIDComponentIdentifier_count,
                                           1, iAP2_uint16, sizeof(U16));
        }
        if(rc == IAP2_OK)
        {
            rc = iAP2AllocateandUpdateData(&accInfo->iAP2BluetoothHIDComponent->iAP2BluetoothTransportComponentIdentifier,
                                           &message->hid.iAP2BluetoothTransportComponentIdentifier,
                                           &accInfo->iAP2BluetoothHIDComponent->iAP2BluetoothTransportComponentIdentifier_count,
                                           1, iAP2_uint16, sizeof(U16));
        }
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2BluetoothHIDComponent is NULL, rc=%d", rc);
    }

    return rc;
}

int32_t sendBluetoothTransport(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    S32 rc = IAP2_OK;
    if((initAccIdParams->iAP2BluetoothTransportComponent) && (initAccIdParams->iAP2BluetoothTransportComponent_count > 0))
    {
        size_t length = sizeof(struct BluetoothTransport) + sizeof(iAP2ServiceBluetoothTransport_t) * initAccIdParams->iAP2BluetoothTransportComponent_count;
        struct BluetoothTransport* msg = (struct BluetoothTransport*)calloc(1, length);
        if(msg)
        {
            for(int i = 0; i < initAccIdParams->iAP2BluetoothTransportComponent_count; ++i)
            {
                msg->bt[i].iAP2TransportComponentIdentifier = *initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier;
                msg->bt[i].iAP2TransportSupportsiAP2Connection = initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count;
                if(initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName)
                {
                    strncpy((char*)msg->bt[i].iAP2TransportComponentName, (char*)*initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName, strnlen((char*)*initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName, STRING_MAX));
                }
                if((initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress) && (initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress->iAP2BlobData))
                {
                    memcpy(msg->bt[i].iAP2BluetoothTransportMediaAccessControlAddress, initAccIdParams->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress->iAP2BlobData, IAP2_BT_MAC_LENGTH);
                }
            }
            msg->count = initAccIdParams->iAP2BluetoothTransportComponent_count;

            msg->header.type = MessageType::BluetoothTransport;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: BluetoothTransport");
            rc = iAP2ServiceSendMessage(service, msg, length);
            free(msg);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "BluetoothTransport msg is NULL, rc=%d", rc);
        }
    }
    return rc;
}

int32_t recvBluetoothTransport(struct BluetoothTransport* message, iAP2AccessoryInfo_t* accInfo)
{
    S32 rc = IAP2_OK;

    /* more than one iAP2BluetoothTransportComponent possible */
    accInfo->iAP2BluetoothTransportComponent = (iAP2BluetoothTransportComponent*)calloc(message->count, sizeof(iAP2BluetoothTransportComponent) );
    if(accInfo->iAP2BluetoothTransportComponent != NULL)
    {
        for(uint32_t i = 0; ((i< message->count) && (rc == IAP2_OK)); i++)
        {
            U8* p_MACAddress = message->bt[i].iAP2BluetoothTransportMediaAccessControlAddress;
            rc = iAP2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                            p_MACAddress,
                                           &accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress_count,
                                           IAP2_BT_MAC_LENGTH, iAP2_blob, 0);

            if(rc == IAP2_OK)
            {
                /* each iAP2BluetoothTransportComponent must have an unique identifier */
                rc = iAP2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                               &message->bt[i].iAP2TransportComponentIdentifier,
                                               &accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                               1, iAP2_uint16, sizeof(U16));
            }
            if(rc == IAP2_OK)
            {
                U8* pCompName = message->bt[i].iAP2TransportComponentName;
                rc = iAP2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                &pCompName,
                                                &accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName_count,
                                                1, iAP2_utf8, 0);
            }
            if(rc == IAP2_OK && message->bt[i].iAP2TransportSupportsiAP2Connection)
            {
                /* Set if Bluetooth component supports iAP2 connection */
                accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
            }

            accInfo->iAP2BluetoothTransportComponent_count++;
        }
    }
    else
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2BluetoothTransportComponent is NULL, rc=%d", rc);
    }
    return rc;
}

int32_t sendWirelessCarPlayTransport(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    S32 rc = IAP2_OK;
    if(initAccIdParams->iAP2WirelessCarPlayTransportComponent != NULL)
    {
        size_t length = sizeof(struct WirelessCarPlayTransport) + sizeof(iAP2ServiceWirelessCarPlayTransportComponent_t) * initAccIdParams->iAP2WirelessCarPlayTransportComponent_count;
        struct WirelessCarPlayTransport* message = (struct WirelessCarPlayTransport*)calloc(1, length);
        if(message)
        {
            for(uint32_t i = 0; i < initAccIdParams->iAP2WirelessCarPlayTransportComponent_count; i++)
            {
                message->wireless[i].iAP2TransportComponentIdentifier = *initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier;
                if(initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName != NULL)
                {
                    strncpy((char*)message->wireless[i].iAP2TransportComponentName, (char*)*initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName, strnlen((char*)*initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName, STRING_MAX));
                }
                message->wireless[i].iAP2TransportSupportsCarPlay = initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsCarPlay_count;
                message->wireless[i].iAP2TransportSupportsiAP2Connection = initAccIdParams->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsiAP2Connection_count;
            }

            message->count = initAccIdParams->iAP2WirelessCarPlayTransportComponent_count;

            message->header.type = MessageType::WirelessCarPlayTransport;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: BluetoothTransport");

            rc = iAP2ServiceSendMessage(service, message, length);
            free(message);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "WirelessCarPlayTransport message is NULL, rc=%d", rc);
        }
    }
    return rc;
}

int32_t recvWirelessCarPlayTransport(struct WirelessCarPlayTransport* message, iAP2AccessoryInfo_t* accInfo)
{
    S32 rc = IAP2_OK;

    if( message->count > 0 )
    {
        accInfo->iAP2WirelessCarPlayTransportComponent = (iAP2WirelessCarPlayTransportComponent*)calloc(message->count, sizeof(iAP2WirelessCarPlayTransportComponent));
        if(accInfo->iAP2WirelessCarPlayTransportComponent != NULL)
        {
            for(uint32_t i = 0; i < message->count; i++)
            {
                rc = iAP2AllocateandUpdateData(&accInfo->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier,
                                               &message->wireless[i].iAP2TransportComponentIdentifier,
                                               &accInfo->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                               1, iAP2_uint16, sizeof(U16));
                if(rc == IAP2_OK)
                {
                    U8* WCplyTransportComponentName = message->wireless[i].iAP2TransportComponentName;
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName,
                                                   &WCplyTransportComponentName,
                                                   &accInfo->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportComponentName_count,
                                                   1, iAP2_utf8, 0);
                }
                if((rc == IAP2_OK) && (message->wireless[i].iAP2TransportSupportsCarPlay))
                {
                    accInfo->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsCarPlay_count++;
                }
                if((rc == IAP2_OK) && (message->wireless[i].iAP2TransportSupportsiAP2Connection))
                {
                    accInfo->iAP2WirelessCarPlayTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
                }
            }
            accInfo->iAP2WirelessCarPlayTransportComponent_count = message->count;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2WirelessCarPlayTransportComponent is NULL, rc=%d", rc);
        }
    }
    return rc;
}


int32_t sendRouteGuidanceDisplay(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams)
{
    S32 rc = IAP2_OK;
    if(initAccIdParams->iAP2RouteGuidanceDisplayComponent)
    {
        size_t length = sizeof(struct RouteGuidanceDisplay) + sizeof(iAP2ServiceRouteGuidanceDiplay_t) * initAccIdParams->iAP2RouteGuidanceDisplayComponent_count;
        struct RouteGuidanceDisplay* message = (struct RouteGuidanceDisplay*)calloc(1,length);
        if(message)
        {
            for(uint32_t i = 0 ; i<initAccIdParams->iAP2RouteGuidanceDisplayComponent_count; i++)
            {
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2Identifier = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2Identifier_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxCurrentRoadNameLength = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxCurrentRoadNameLength_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxDestinationRoadNameLength = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxDestinationRoadNameLength_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxAfterManeuverRoadNameLength = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxAfterManeuverRoadNameLength_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxManeuverDescriptionLength = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxManeuverDescriptionLength_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxGuidanceManeuverStorageCapacity = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxGuidanceManeuverStorageCapacity_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceDescriptionLength = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceDescriptionLength_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count;
                }
                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity)
                {
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceStorageCapacity = *initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity;
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceStorageCapacity_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count;
                }

                if(initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name)
                {
                    strncpy((char*)message->iAP2ServiceRouteGuidanceDisplay[i].iAP2Name, (char*)*initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name, strnlen((char*)*initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name, STRING_MAX));
                    message->iAP2ServiceRouteGuidanceDisplay[i].iAP2Name_count = initAccIdParams->iAP2RouteGuidanceDisplayComponent[i].iAP2Name_count;
                }
            }

            message->count = initAccIdParams->iAP2RouteGuidanceDisplayComponent_count;

            message->header.type = MessageType::RouteGuidanceDisplay;

            IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "sendConfigurationMessage: RouteGuidanceDisplay");

            rc = iAP2ServiceSendMessage(service, message, length);
            free(message);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "RouteGuidanceDisplay message is NULL, rc=%d", rc);
        }
    }
    return rc;
}

int32_t recvRouteGuidanceDisplay(struct RouteGuidanceDisplay* message, iAP2AccessoryInfo_t* accInfo)
{
    S32 rc = IAP2_OK;

    if( message->count > 0 )
    {
        accInfo->iAP2RouteGuidanceDisplayComponent = (iAP2RouteGuidanceDisplayComponent*)calloc(message->count, sizeof(iAP2RouteGuidanceDisplayComponent));
        if(accInfo->iAP2RouteGuidanceDisplayComponent != NULL)
        {
            for(uint32_t i = 0; i < message->count; i++)
            {
                U8* RGDisplay_Name = message->iAP2ServiceRouteGuidanceDisplay[i].iAP2Name;
                rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                               &RGDisplay_Name,
                                               &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Name_count,
                                               1, iAP2_utf8, 0);
                if(rc == IAP2_OK)
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2Identifier,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxCurrentRoadNameLength_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxCurrentRoadNameLength,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxDestinationRoadNameLength_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxDestinationRoadNameLength,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxAfterManeuverRoadNameLength_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxAfterManeuverRoadNameLength,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxManeuverDescriptionLength_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxManeuverDescriptionLength,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxGuidanceManeuverStorageCapacity_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceDescriptionLength_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
                if((rc == IAP2_OK) && (message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceStorageCapacity_count == 1))
                {
                    rc = iAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                   &message->iAP2ServiceRouteGuidanceDisplay[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count,
                                                   1, iAP2_uint16, sizeof(U16));
                }
            }
            accInfo->iAP2RouteGuidanceDisplayComponent_count = message->count;
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2RouteGuidanceDisplayComponent is NULL, rc=%d", rc);
        }
    }
    return rc;
}
