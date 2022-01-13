#include "iap2_test_config.h"
#include "iap2_test_utility.h"
#include "iap2_test_init.h"
#include "iap2_test_configuration_pfcfg.h"
#include <iap2_parameter_free.h>
#include "iap2_dlt_log.h"

S32 iap2InitAccInfo(iAP2AccessoryInfo_t *accInfo,iap2UserConfig_t iap2UserConfig)
{
	S32 rc = IAP2_OK;

    U8  AccessoryName[]                 = {IAP2_ACC_INFO_NAME};
    U8  ModelIdentifier[]               = {IAP2_ACC_INFO_MODEL_IDENTIFIER};
    U8  Manufacturer[]                  = {IAP2_ACC_INFO_MANUFACTURER};
    U8  SerialNumber[]                  = {IAP2_ACC_INFO_SERIAL_NUM};
    U8  FirmwareVersion[]               = {IAP2_ACC_INFO_FW_VER};
    U8  HardwareVersion[]               = {IAP2_ACC_INFO_HW_VER};
    U8  idVendor[]                      = {IAP2_ACC_INFO_VENDOR_ID};
    U8  idProduct[]                     = {IAP2_ACC_INFO_PRODUCT_ID};
    U8  bcdDevice[]                     = {IAP2_ACC_INFO_BCD_DEVICE};
    U8  initEndpoint[]                  = {IAP2_ACC_INFO_EP_INIT};
    U8  CurrentLanguage[]               = {"en"};
    U16 SupportedLanguageCnt = 3;
    U8  SupportedLanguage[][3]          = {"en", "de", "fr"};
    U8  ProductPlanUUID[]                  = {IAP2_ACC_INFO_PPUUID};
    U16 i;

    memset(accInfo, 0, sizeof(iAP2AccessoryInfo_t) );

    accInfo->iAP2AccessoryName                 = (U8*)strndup( (const char*)AccessoryName, strnlen((const char*)AccessoryName, STRING_MAX) );
    accInfo->iAP2AccessoryModelIdentifier      = (U8*)strndup( (const char*)ModelIdentifier, strnlen((const char*)ModelIdentifier, STRING_MAX) );
    accInfo->iAP2AccessoryManufacturer         = (U8*)strndup( (const char*)Manufacturer, strnlen((const char*)Manufacturer, STRING_MAX) );
    accInfo->iAP2AccessorySerialNumber         = (U8*)strndup( (const char*)SerialNumber, strnlen((const char*)SerialNumber, STRING_MAX) );
    accInfo->iAP2AccessoryFirmwareVersion      = (U8*)strndup( (const char*)FirmwareVersion, strnlen((const char*)FirmwareVersion, STRING_MAX) );
    accInfo->iAP2AccessoryHardwareVersion      = (U8*)strndup( (const char*)HardwareVersion, strnlen((const char*)HardwareVersion, STRING_MAX) );
    accInfo->iAP2AccessoryVendorId             = (U8*)strndup( (const char*)idVendor, strnlen((const char*)idVendor, STRING_MAX) );
    accInfo->iAP2AccessoryProductId            = (U8*)strndup( (const char*)idProduct, strnlen((const char*)idProduct, STRING_MAX) );
    accInfo->iAP2AccessoryBcdDevice            = (U8*)strndup( (const char*)bcdDevice, strnlen((const char*)bcdDevice, STRING_MAX) );
    accInfo->iAP2InitEndPoint                  = (U8*)strndup( (const char*)initEndpoint, strnlen((const char*)initEndpoint, STRING_MAX) );
    accInfo->iAP2CurrentLanguage               = (U8*)strndup( (const char*)CurrentLanguage, strnlen((const char*)CurrentLanguage, STRING_MAX) );
    accInfo->iAP2ProductPlanUUID               = (U8*)strndup( (const char*)ProductPlanUUID, strnlen((const char*)ProductPlanUUID, STRING_MAX) );

    if( (accInfo->iAP2AccessoryName            == NULL) ||
        (accInfo->iAP2AccessoryModelIdentifier == NULL) ||
        (accInfo->iAP2AccessoryManufacturer    == NULL) ||
        (accInfo->iAP2AccessorySerialNumber    == NULL) ||
        (accInfo->iAP2AccessoryFirmwareVersion == NULL) ||
        (accInfo->iAP2AccessoryHardwareVersion == NULL) ||
        (accInfo->iAP2AccessoryVendorId        == NULL) ||
        (accInfo->iAP2AccessoryProductId       == NULL) ||
        (accInfo->iAP2AccessoryBcdDevice       == NULL) ||
        (accInfo->iAP2InitEndPoint             == NULL) ||
        (accInfo->iAP2CurrentLanguage          == NULL) )
    {
        rc = IAP2_ERR_NO_MEM;
        IAP2TESTDLTLOG(DLT_LOG_ERROR,
                       "Failed to Initialize Acc Info        \
                        iAP2AccessoryName = %p,              \
                        iAP2AccessoryModelIdentifier = %p,   \
                        iAP2AccessoryManufacturer = %p,      \
                        iAP2AccessorySerialNumber = %p,      \
                        iAP2AccessoryFirmwareVersion = %p,   \
                        iAP2AccessoryHardwareVersion = %p,   \
                        iAP2AccessoryVendorId = %p,          \
                        iAP2AccessoryProductId = %p,         \
                        iAP2AccessoryBcdDevice = %p,         \
                        iAP2InitEndPoint = %p,               \
                        iAP2CurrentLanguage = %p,",
                        accInfo->iAP2AccessoryName,
                        accInfo->iAP2AccessoryModelIdentifier,
                        accInfo->iAP2AccessoryManufacturer,
                        accInfo->iAP2AccessorySerialNumber,
                        accInfo->iAP2AccessoryFirmwareVersion,
                        accInfo->iAP2AccessoryHardwareVersion,
                        accInfo->iAP2AccessoryVendorId,
                        accInfo->iAP2AccessoryProductId,
                        accInfo->iAP2AccessoryBcdDevice,
                        accInfo->iAP2InitEndPoint,
                        accInfo->iAP2CurrentLanguage);
    }

    if(rc == IAP2_OK)
    {
        accInfo->iAP2SupportedLanguageCount = SupportedLanguageCnt;
        accInfo->iAP2SupportedLanguage      = calloc(accInfo->iAP2SupportedLanguageCount, sizeof(U8*) );
        if(accInfo->iAP2SupportedLanguage != NULL)
        {
            for(i=0;( (i<accInfo->iAP2SupportedLanguageCount) && (rc == IAP2_OK) );i++)
            {
                accInfo->iAP2SupportedLanguage[i] = (U8*)strndup( (const char*)&SupportedLanguage[i][0], strnlen((const char*)&SupportedLanguage[i][0], STRING_MAX) );
                if(accInfo->iAP2SupportedLanguage[i] == NULL)
                {
                    rc = IAP2_ERR_NO_MEM;
                    IAP2TESTDLTLOG(DLT_LOG_ERROR, "iAP2SupportedLanguage[%d] is NULL",i);
                }
            }
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
    }

    if(rc == IAP2_OK && iap2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE)
    {
        iAP2USBDeviceModeAudioSampleRate  USBDeviceMode_TransCompAudSampRate[]  = {IAP2_SAMPLERATE_8000HZ,
                                                                                   IAP2_SAMPLERATE_11025HZ,
                                                                                   IAP2_SAMPLERATE_12000HZ,
                                                                                   IAP2_SAMPLERATE_16000HZ,
                                                                                   IAP2_SAMPLERATE_22050HZ,
                                                                                   IAP2_SAMPLERATE_24000HZ,
                                                                                   IAP2_SAMPLERATE_32000HZ,
                                                                                   IAP2_SAMPLERATE_44100HZ,
                                                                                   IAP2_SAMPLERATE_48000HZ};

        accInfo->iAP2USBDeviceSupportedAudioSampleRate = calloc(1, sizeof(USBDeviceMode_TransCompAudSampRate) );
        if(accInfo->iAP2USBDeviceSupportedAudioSampleRate == NULL)
        {
            rc = IAP2_ERR_NO_MEM;
        }
        else
        {
            memcpy(accInfo->iAP2USBDeviceSupportedAudioSampleRate, USBDeviceMode_TransCompAudSampRate, sizeof(USBDeviceMode_TransCompAudSampRate) );
            accInfo->iAP2USBDeviceSupportedAudioSampleRate_count = (sizeof(USBDeviceMode_TransCompAudSampRate) / 4);
        }
    }

    return rc;
}

S32 iap2ConfigAcc(iAP2InitParam_t* iAP2InitParam,iap2UserConfig_t iAP2UserConfig)
{
    S32 rc = IAP2_OK ;

    if(NULL != iAP2InitParam)
    {
    /*Configure Accessory*/

        iAP2InitParam->p_iAP2AccessoryConfig = (iAP2AccessoryConfig_t*)calloc(1, sizeof(iAP2AccessoryConfig_t));
        if(NULL != iAP2InitParam->p_iAP2AccessoryConfig)
        {
            rc =iap2ConfigureAccessory(iAP2InitParam->p_iAP2AccessoryConfig, iAP2UserConfig );
        }
        else
        {
            rc = IAP2_ERR_NO_MEM ;
        }

        if(rc == IAP2_OK)
        {
            iAP2InitParam->p_iAP2AccessoryConfig->iAP2FileXferSupported = TRUE;
        iAP2InitParam->iAP2ContextCallback = NULL;
        /*CR: memory allocation and deallocation must be done at a single place*/
        iAP2InitParam->p_iAP2AccessoryInfo = (iAP2AccessoryInfo_t*)calloc(1, sizeof(iAP2AccessoryInfo_t));
        iAP2InitParam->p_iAP2CSCallbacks = (iAP2SessionCallbacks_t*)calloc(1, sizeof(iAP2SessionCallbacks_t));
        iAP2InitParam->p_iAP2StackCallbacks = (iAP2StackCallbacks_t*)calloc(1, sizeof(iAP2StackCallbacks_t));
        if(iAP2InitParam->p_iAP2AccessoryConfig->iAP2FileXferSupported == TRUE)
        {
            iAP2InitParam->p_iAP2FileTransferCallbacks = (iAP2FileTransferCallbacks_t*)calloc(1, sizeof(iAP2FileTransferCallbacks_t));
        }
        if (iAP2InitParam->p_iAP2AccessoryConfig->iAP2EAPSupported == TRUE && iAP2UserConfig.iAP2TransportType == iAP2USBDEVICEMODE)
        {
            iAP2InitParam->p_iAP2EAPSessionCallbacks = (iAP2EAPSessionCallbacks_t*)calloc(1, sizeof(iAP2EAPSessionCallbacks_t));
            iAP2InitParam->p_iAP2MultiEAPSessionCallbacks = (iAP2MultiEAPSessionCallbacks_t*)calloc(1, sizeof(iAP2MultiEAPSessionCallbacks_t));
        }
        if (iAP2UserConfig.iap2EANativeTransport == TRUE && iAP2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
        {
            iAP2InitParam->p_iAP2EANativeTransportCallbacks = (iAP2EANativeTransportCallbacks_t*)calloc(1, sizeof(iAP2EANativeTransportCallbacks_t));
        }
        if( (iAP2UserConfig.iap2EANativeTransport == FALSE) &&
            (iAP2UserConfig.iAP2TransportType == iAP2USBHOSTMODE) &&
            (iAP2UserConfig.iap2EAPSupported == TRUE) )
        {
            iAP2InitParam->p_iAP2EAPSessionCallbacks = (iAP2EAPSessionCallbacks_t*)calloc(1, sizeof(iAP2EAPSessionCallbacks_t));
            iAP2InitParam->p_iAP2MultiEAPSessionCallbacks = (iAP2MultiEAPSessionCallbacks_t*)calloc(1, sizeof(iAP2MultiEAPSessionCallbacks_t));
        }
        }
    }
    else
    {
      rc = IAP2_BAD_PARAMETER;
    }
    return rc;
}
/*Free Accessory Identification Values*/
void iap2FreeAccInfo(iAP2AccessoryInfo_t *accInfo)
{
    U16 i;

    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryName);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryModelIdentifier);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryManufacturer);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessorySerialNumber);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryFirmwareVersion);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryHardwareVersion);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryVendorId);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryProductId);
    iap2TestFreePtr( (void**)&accInfo->iAP2AccessoryBcdDevice);
    iap2TestFreePtr( (void**)&accInfo->iAP2InitEndPoint);
    iap2TestFreePtr( (void**)&accInfo->iAP2CommandsUsedByApplication);
    iap2TestFreePtr( (void**)&accInfo->iAP2CallbacksExpectedFromDevice);

    for(i=0; i<accInfo->iAP2SupportediOSAppCount; i++)
    {
        if(accInfo->iAP2iOSAppInfo != NULL)
        {
            accInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = 0;
            iap2TestFreePtr( (void**)&accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName);
        }
    }
    iap2TestFreePtr( (void**)&accInfo->iAP2iOSAppInfo);
    accInfo->iAP2SupportediOSAppCount = 0;

    iap2TestFreePtr( (void**)&accInfo->iAP2PreferredAppBundleSeedIdentifier);
    iap2TestFreePtr( (void**)&accInfo->iAP2CurrentLanguage);
    iap2TestFreePtr( (void**)&accInfo->iAP2iOSAppInfo);

    if(accInfo->iAP2SupportedLanguage != NULL)
    {
        for(i=0; i<accInfo->iAP2SupportedLanguageCount; i++)
        {
            iap2TestFreePtr( (void**)&accInfo->iAP2SupportedLanguage[i]);
        }
        iap2TestFreePtr( (void**)&accInfo->iAP2SupportedLanguage);
    }

    iap2TestFreePtr( (void**)&accInfo->iAP2USBDeviceSupportedAudioSampleRate);

    if(accInfo->iAP2USBHIDComponent != NULL)
    {
        for(i = 0; i < accInfo->iAP2USBHIDComponent_count; i++)
        {
            iAP2FreeiAP2iAP2HIDComponent(&accInfo->iAP2USBHIDComponent[i]);
        }
        iap2TestFreePtr( (void**)&accInfo->iAP2USBHIDComponent);
        accInfo->iAP2USBHIDComponent_count = 0;
    }

#if IAP2_ENABLE_BLUETOOTH_COMPONENT_TEST
    if(accInfo->iAP2BluetoothTransportComponent != NULL)
    {
        for(i=0;i<accInfo->iAP2BluetoothTransportComponent_count; i++)
        {
            iAP2FreeiAP2BluetoothTransportComponent(&accInfo->iAP2BluetoothTransportComponent[i]);
        }
        iap2TestFreePtr( (void**)&accInfo->iAP2BluetoothTransportComponent);
        accInfo->iAP2BluetoothTransportComponent_count = 0;
    }
#else
    iap2TestFreePtr( (void**)&accInfo->iAP2BluetoothTransportMAC);
#endif

#if IAP2_ENABLE_VEHICLE_INFORMATION_COMPONENT_TEST
    if(accInfo->iAP2VehicleInformationComponent != NULL)
    {
        iap2TestFreePtr( (void**)&accInfo->iAP2VehicleInformationComponent->iAP2DisplayName);
        iap2TestFreePtr( (void**)&accInfo->iAP2VehicleInformationComponent->iAP2EngineType);
        iap2TestFreePtr( (void**)&accInfo->iAP2VehicleInformationComponent);
    }
#endif

#if IAP2_ENABLE_VEHICLE_STATUS_COMPONENT_TEST
    iap2TestFreePtr( (void**)&accInfo->iAP2VehicleStatusComponent);
#endif

#if IAP2_ENABLE_LOCATION_INFORMATION_COMPONENT_TEST
    iap2TestFreePtr( (void**)&accInfo->iAP2LocationInformationComponent);
#endif
}
/* Configure Accessory*/
S32 iap2ConfigureAccessory(iAP2AccessoryConfig_t* accConfig, iap2UserConfig_t iap2UserConfig)
{
    S32 rc =IAP2_OK;
    U8 num_cfg = 0;
    IAP2TEST_Cfg *dcInfo = (IAP2TEST_Cfg *)iAP2TestGetDevconfParameter(&num_cfg);
    /*Get user configuration values from g_iAP2UserConfig */
    accConfig->iAP2iOSintheCar        = iap2UserConfig.iap2iOSintheCar;
    accConfig->iAP2EANativeTransport  = iap2UserConfig.iap2EANativeTransport;
    accConfig->iAP2TransportType      = iap2UserConfig.iAP2TransportType;
    accConfig->iAP2AuthenticationType = iap2UserConfig.iAP2AuthenticationType;
    accConfig->iAP2EAPSupported       = iap2UserConfig.iap2EAPSupported;
    if (dcInfo != NULL)
    {
        /*Set Authentication values*/
        if(dcInfo[IAP2_DC_AUTH_DEV_NAME].para.p_val != NULL)
        {
            accConfig->iAP2AuthDevicename = (U8*)strndup( (const char*) dcInfo[IAP2_DC_AUTH_DEV_NAME].para.p_val,
                     strnlen((const char*)dcInfo[IAP2_DC_AUTH_DEV_NAME].para.p_val, STRING_MAX) );
        }
        if(dcInfo[IAP2_DC_AUTH_IOCTL_REG].para.p_val != NULL)
        {
            accConfig->iAP2AuthIoctlRegAddr = (U8*)strndup( (const char*) dcInfo[IAP2_DC_AUTH_IOCTL_REG].para.p_val,
                    strnlen((const char*)dcInfo[IAP2_DC_AUTH_IOCTL_REG].para.p_val, STRING_MAX) );
        }
        if(dcInfo[IAP2_DC_AUTH_RESET].para.p_val != NULL)
        {
            accConfig->iAP2AuthGPIOReset = (U8*)strndup( (const char*) dcInfo[IAP2_DC_AUTH_RESET].para.p_val,
                    strnlen((const char*)dcInfo[IAP2_DC_AUTH_RESET].para.p_val, STRING_MAX) );
        }
        if(dcInfo[IAP2_DC_AUTH_READY].para.p_val != NULL)
        {
            accConfig->iAP2AuthGPIOReady = (U8*)strndup( (const char*) dcInfo[IAP2_DC_AUTH_READY].para.p_val,
                    strnlen((const char*)dcInfo[IAP2_DC_AUTH_READY].para.p_val, STRING_MAX) );
        }
        if(dcInfo[IAP2_DC_COPRO_AUTODETECT].para.p_val != NULL)
        {
            accConfig->iAP2AuthAutoDetect = (U8*)strndup( (const char*) dcInfo[IAP2_DC_COPRO_AUTODETECT].para.p_val,
                    strnlen((const char*)dcInfo[IAP2_DC_COPRO_AUTODETECT].para.p_val, STRING_MAX) );
        }
        if ( (accConfig->iAP2AuthDevicename   == NULL) ||
             (accConfig->iAP2AuthIoctlRegAddr == NULL) ||
             (accConfig->iAP2AuthGPIOReset    == NULL) ||
             (accConfig->iAP2AuthGPIOReady    == NULL) ||
             (accConfig->iAP2AuthAutoDetect   == NULL) )
        {
            rc = IAP2_ERR_NO_MEM;
            IAP2TESTDLTLOG(DLT_LOG_ERROR,
                           "Failed to set Authentication values     \
                           iAP2AuthDevicename = %p,                 \
                           iAP2AuthIoctlRegAddr = %p,               \
                           iAP2AuthGPIOReset = %p,                  \
                           iAP2AuthGPIOReady = %p,                  \
                           iAP2AuthAutoDetect= %p",
                           accConfig->iAP2AuthDevicename,
                           accConfig->iAP2AuthIoctlRegAddr,
                           accConfig->iAP2AuthGPIOReset,
                           accConfig->iAP2AuthGPIOReady,
                           accConfig->iAP2AuthAutoDetect);
        }
        if(rc == IAP2_OK)
        {
            accConfig->iAP2AuthShortWait    = dcInfo[IAP2_DC_AUTH_SHORT_WAIT].para.val;
            accConfig->iAP2AuthWait         = dcInfo[IAP2_DC_AUTH_WAIT].para.val;
            accConfig->iAP2AuthLongWait     = dcInfo[IAP2_DC_AUTH_LONG_WAIT].para.val;
        }
    iAP2TestFreeDevconfParameter(dcInfo,num_cfg);
    }
    else
    {
        rc = IAP2_BAD_PARAMETER;
    }
    accConfig->iAP2FileXferRcvAsStream                       = TRUE;

    /* Note:
     * Set the Power configuration is no longer necessary,
     * because the Power configuration appears by the Application
     * and not as automatic response from within the iAP2 Library. */
//    accConfig->iAP2AvailableCurrentForDevice                 = IAP2_CURRENT_DRAW_FOR_DEVICE;
//    accConfig->iAP2DeviceBatteryShouldChargeIfPowerIsPresent = TRUE;
//    accConfig->iAP2MaximumcurrentDrawnFromAccessory          = TRUE;
//    accConfig->iAP2DeviceBatteryWillChargeIfPowerIsPresent   = TRUE;
//    accConfig->iAP2AccessoryPowerMode                        = TRUE;

    accConfig->iAP2UsbOtgGPIOPower  = iap2UserConfig.iap2UsbOtgGPIOPower;
#ifdef IAP2_USE_CONFIGFS
    if(accConfig->iAP2TransportType == iAP2USBHOSTMODE)
    {
        accConfig->useConfigFS  = TRUE;
    }
#else
    accConfig->useConfigFS  = FALSE;
#endif

#ifdef IAP2_CONFIGURE_CONTROL_SESSION_VERSION_MANUALLY
    if(accConfig->iAP2TransportType != iAP2USBHOSTMODE)
    {
        accConfig->ManualLinkConfig = TRUE;
        accConfig->LinkConfig_SessionVersion = 1;
    }
#endif
    return rc;
}

void iap2FreeAccConfig(iAP2AccessoryConfig_t* p_iAP2AccessoryConfig)
{
    iap2TestFreePtr( (void**)&p_iAP2AccessoryConfig->iAP2AuthDevicename);
    iap2TestFreePtr( (void**)&p_iAP2AccessoryConfig->iAP2AuthIoctlRegAddr);
    iap2TestFreePtr( (void**)&p_iAP2AccessoryConfig->iAP2AuthGPIOReset);
    iap2TestFreePtr( (void**)&p_iAP2AccessoryConfig->iAP2AuthGPIOReady);
    iap2TestFreePtr( (void**)&p_iAP2AccessoryConfig->UdcDeviceName);
}

void iap2ResetInitialParameter(iAP2InitParam_t* iAP2InitParam)
{

    /*TODO: Add more de-initialization*/
    if(NULL != iAP2InitParam->p_iAP2AccessoryInfo)
    {
        iap2FreeAccInfo(iAP2InitParam->p_iAP2AccessoryInfo);
        iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2AccessoryInfo);
    }

    if(NULL != iAP2InitParam->p_iAP2AccessoryConfig)
    {
        iap2FreeAccConfig(iAP2InitParam->p_iAP2AccessoryConfig);
        iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2AccessoryConfig);
    }

    iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2CSCallbacks);
    iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2FileTransferCallbacks);
    iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2EAPSessionCallbacks);
    iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2MultiEAPSessionCallbacks);
    iap2TestFreePtr( (void**)&iAP2InitParam->p_iAP2StackCallbacks);

}
