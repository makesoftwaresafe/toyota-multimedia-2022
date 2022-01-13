/****************************************************
 *  ipp_iap2_init.c                                         
 *  Created on: 2014/01/16 17:54:41                      
 *  Implementation of the Class ipp_iap2_init       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_init.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerUtilityLog.h"
#include "iPodPlayerCoreCfg.h"
#include "ipp_iap2_common.h"

/* ############### Temporary definition ####################### */
#define IAP2_MSG_TBL_END ((U16)0xFFFF)
#define IAP2_MSG_TBL_SIZE(a)  (sizeof(a) - sizeof(IAP2_MSG_TBL_END))

/* USB HOST MODE TABLE */
U16 USBHostModeMsgSentByAcc[] = {
    IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_START_MEDIA_LIBRARY_UPDATES,
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_UPDATES,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_CURRENT_SELECTION,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_ITEMS,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_COLLECTION,
    IAP2_MSG_ID_START_NOW_PLAYING_UPDATES,
    IAP2_MSG_ID_STOP_NOW_PLAYING_UPDATES,
    IAP2_MSG_ID_START_POWER_UPDATES,
    IAP2_MSG_ID_STOP_POWER_UPDATES,
    IAP2_MSG_ID_POWER_SOURCE_UPDATE,
    IAP2_MSG_ID_START_HID,
    IAP2_MSG_ID_ACCESSORY_HID_REPORT,
    IAP2_MSG_ID_STOP_HID,
    IAP2_MSG_ID_REQUEST_APP_LAUNCH,
    IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION,
    IAP2_MSG_ID_START_BLUETOOTH_CONNECTION_UPDATES,
    IAP2_MSG_ID_STOP_BLUETOOTH_CONNECTION_UPDATES,
    IAP2_MSG_ID_LOCATION_INFORMATION,
    IAP2_MSG_ID_VEHICLE_STATUS_UPDATE,
    IAP2_MSG_TBL_END
};

U16 USBHostModeMsgRecvFromDevice[] = {
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,
    IAP2_MSG_ID_DEVICE_LANGUAGE_UPDATE,
    IAP2_MSG_ID_POWER_UPDATE,
    IAP2_MSG_ID_DEVICE_HID_REPORT,
    IAP2_MSG_ID_START_LOCATION_INFORMATION,
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_BLUETOOTH_CONNECTION_UPDATE,
    IAP2_MSG_TBL_END
};

U16 USBHostModeMsgRecvFromDeviceWithExt[] = {
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,
    IAP2_MSG_ID_DEVICE_LANGUAGE_UPDATE,
    IAP2_MSG_ID_POWER_UPDATE,
    IAP2_MSG_ID_DEVICE_HID_REPORT,
    IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
    IAP2_MSG_ID_STOP_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
    IAP2_MSG_ID_START_LOCATION_INFORMATION,
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_BLUETOOTH_CONNECTION_UPDATE,
    IAP2_MSG_TBL_END
};

/* USB DEVICE MODE TABLE */
U16 USBDeviceModeMsgSentByAcc[] = {
    IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_START_MEDIA_LIBRARY_UPDATES,
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_UPDATES,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_CURRENT_SELECTION,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_ITEMS,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_COLLECTION,
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_SPECIAL,
    IAP2_MSG_ID_START_NOW_PLAYING_UPDATES,
    IAP2_MSG_ID_STOP_NOW_PLAYING_UPDATES,
    IAP2_MSG_ID_SET_NOW_PLAYING_INFORMATION,
    IAP2_MSG_ID_START_USB_DEVICE_MODE_AUDIO,
    IAP2_MSG_ID_STOP_USB_DEVICE_MODE_AUDIO,
    IAP2_MSG_ID_START_POWER_UPDATES,
    IAP2_MSG_ID_STOP_POWER_UPDATES,
    IAP2_MSG_ID_POWER_SOURCE_UPDATE,
    IAP2_MSG_ID_START_HID,
    IAP2_MSG_ID_ACCESSORY_HID_REPORT,
    IAP2_MSG_ID_STOP_HID,
    IAP2_MSG_ID_REQUEST_APP_LAUNCH,
    IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION,
    IAP2_MSG_ID_START_BLUETOOTH_CONNECTION_UPDATES,
    IAP2_MSG_ID_STOP_BLUETOOTH_CONNECTION_UPDATES,
    IAP2_MSG_ID_LOCATION_INFORMATION,
    IAP2_MSG_ID_VEHICLE_STATUS_UPDATE,
    IAP2_MSG_TBL_END
};

U16 USBDeviceModeMsgRecvFromDevice[] = {
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,
    IAP2_MSG_ID_DEVICE_LANGUAGE_UPDATE,
    IAP2_MSG_ID_USB_DEVICE_MODE_AUDIO_INFORMATION,
    IAP2_MSG_ID_POWER_UPDATE,
    IAP2_MSG_ID_DEVICE_HID_REPORT,
    IAP2_MSG_ID_BLUETOOTH_CONNECTION_UPDATE,
    IAP2_MSG_ID_START_LOCATION_INFORMATION,
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_TBL_END
};

U16 USBDeviceModeMsgRecvFromDeviceWithExt[] = {
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,
    IAP2_MSG_ID_DEVICE_LANGUAGE_UPDATE,
    IAP2_MSG_ID_USB_DEVICE_MODE_AUDIO_INFORMATION,
    IAP2_MSG_ID_POWER_UPDATE,
    IAP2_MSG_ID_DEVICE_HID_REPORT,
    IAP2_MSG_ID_BLUETOOTH_CONNECTION_UPDATE,
    IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
    IAP2_MSG_ID_STOP_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
    IAP2_MSG_ID_START_LOCATION_INFORMATION,
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_TBL_END
};

U16 BTMsgSentByAcc[] = {
    IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION,
    IAP2_MSG_ID_LOCATION_INFORMATION,
    IAP2_MSG_ID_VEHICLE_STATUS_UPDATE,
    IAP2_MSG_TBL_END
};

U16 BTMsgRecvFromDevice[] = {
    IAP2_MSG_ID_START_LOCATION_INFORMATION,
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_TBL_END
};

U16 BTMsgRecvFromDeviceWithExt[] = {
    IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
    IAP2_MSG_ID_STOP_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
    IAP2_MSG_ID_START_LOCATION_INFORMATION,
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
    IAP2_MSG_TBL_END
};

extern iAP2Service_t* g_service;

/* Todo */
#define IPODCORE_HID_COMPONENT_COUNT            1

/* ############################################################ */

BOOL ippiAP2CheckIdentificationTableByAcc(U16 id, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo)
{
    U16 *table = USBDeviceModeMsgSentByAcc;
    BOOL rc = FALSE;
    int i = 0;
    int tblsize = 0;

    if(threadInfo != NULL)
    {
        if(threadInfo->accInfo.MsgSentByAccSize == IPOD_PLAYER_ACCINFO_NULL)
        {
            IPOD_PLAYER_DEVICE_TYPE devType = threadInfo->nameInfo.devType;

            if(devType == IPOD_PLAYER_DEVICE_TYPE_BT)
            {
                table = BTMsgSentByAcc;
                tblsize = IAP2_MSG_TBL_SIZE(BTMsgSentByAcc) / sizeof(BTMsgSentByAcc[0]);
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE)
            {
                /* in case of USB interface (iap device mode) */
                table = USBDeviceModeMsgSentByAcc;
                tblsize = IAP2_MSG_TBL_SIZE(USBDeviceModeMsgSentByAcc) / sizeof(USBDeviceModeMsgSentByAcc[0]);
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_HOST)
            {
                /* in case of USB interface (iap host mode) */
                table = USBHostModeMsgSentByAcc;
                tblsize = IAP2_MSG_TBL_SIZE(USBHostModeMsgSentByAcc) / sizeof(USBHostModeMsgSentByAcc[0]);
            }
        }
        else
        {
            table = threadInfo->accInfo.MsgSentByAcc;
            tblsize = threadInfo->accInfo.MsgSentByAccSize / (S32)(sizeof(threadInfo->accInfo.MsgSentByAcc[0]));
        }
 
        for(i = 0; i < tblsize; i++)
        {
            if(id == table[i])
            {
                rc = TRUE;      /* Verify session ID */
                break;
            }
        }
    }

    return rc;    
}

BOOL ippiAP2CheckIdentificationTableFromDevice(U16 id, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo)
{
    U16 *table = USBDeviceModeMsgRecvFromDeviceWithExt;
    BOOL rc = FALSE;
    int i = 0;
    int tblsize = 0;
    
    if(threadInfo != NULL)
    {
        IPOD_PLAYER_DEVICE_TYPE devType = threadInfo->nameInfo.devType;
        U32 iOSAppCount = 0;

        if(threadInfo->accInfo.MsgRecvFromDeviceSize == IPOD_PLAYER_ACCINFO_NULL)
        {
            /* get iOS app count in configuration. */
            iOSAppCount = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_IOSAPP_COUNT);

            if(devType == IPOD_PLAYER_DEVICE_TYPE_BT)
            {
                /* in case of Bluetooth interface */
                if(iOSAppCount > 0)
                {
                    table = BTMsgRecvFromDeviceWithExt;
                    tblsize = IAP2_MSG_TBL_SIZE(BTMsgRecvFromDeviceWithExt) / sizeof(BTMsgRecvFromDeviceWithExt[0]);
                }
                else
                {
                    table = BTMsgRecvFromDevice;
                    tblsize = IAP2_MSG_TBL_SIZE(BTMsgRecvFromDevice) / sizeof(BTMsgRecvFromDevice[0]);
                }
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE)
            {
                /* in case of USB interface (iap2 device mode) */
                if(iOSAppCount > 0)
                {
                    table = USBDeviceModeMsgRecvFromDeviceWithExt;
                    tblsize = IAP2_MSG_TBL_SIZE(USBDeviceModeMsgRecvFromDeviceWithExt) / sizeof(USBDeviceModeMsgRecvFromDeviceWithExt[0]);
                }
                else
                {
                    table = USBDeviceModeMsgRecvFromDevice;
                    tblsize = IAP2_MSG_TBL_SIZE(USBDeviceModeMsgRecvFromDevice) / sizeof(USBDeviceModeMsgRecvFromDevice[0]);
               }
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_HOST)
            {
                /* in case of USB interface (iap2 host mode) */
                if(iOSAppCount > 0)
                {
                    table = USBHostModeMsgRecvFromDeviceWithExt;
                    tblsize = IAP2_MSG_TBL_SIZE(USBHostModeMsgRecvFromDeviceWithExt) / sizeof(USBHostModeMsgRecvFromDeviceWithExt[0]);
                }
                else
                {
                    table = USBHostModeMsgRecvFromDevice;
                    tblsize = IAP2_MSG_TBL_SIZE(USBHostModeMsgRecvFromDevice) / sizeof(USBHostModeMsgRecvFromDevice[0]);
                }
            }
        }
        else
        {
            iOSAppCount = threadInfo->accInfo.SupportediOSAppCount;
            table = threadInfo->accInfo.MsgRecvFromDevice;
            tblsize = threadInfo->accInfo.MsgRecvFromDeviceSize / (S32)(sizeof(threadInfo->accInfo.MsgRecvFromDevice[0]));
        }

        for(i = 0; i < tblsize; i++)
        {
            if(id == table[i])
            {
                rc = TRUE;      /* Verify session ID */
                break;
            }
        }
    }

    return rc;    
}

static iAP2TransportType_t ippiAP2ConvertDeviceType(U8 devType)
{
    iAP2TransportType_t type = iAP2GENERICMODE;
    
    switch(devType)
    {
    case IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE:
        type = iAP2USBDEVICEMODE;
        break;
    case IPOD_PLAYER_DEVICE_TYPE_USB_HOST:
        type = iAP2USBHOSTMODE;
        break;
    case IPOD_PLAYER_DEVICE_TYPE_BT:
        type = iAP2BLUETOOTH;
        break;
    case IPOD_PLAYER_DEVICE_TYPE_UART:
        type = iAP2UART;
        break;
    default:
        type = iAP2GENERICMODE;
        break;
    }
    
    return type;
}

static iAP2USBDeviceModeAudioSampleRate ippiAP2ConvertSampleRate(U32 sampleRate)
{
    iAP2USBDeviceModeAudioSampleRate rate = IAP2_SAMPLERATE_44100HZ;
    
    switch(sampleRate)
    {
    case 8000:
        rate = IAP2_SAMPLERATE_8000HZ;
        break;
        
    case 11025:
        rate = IAP2_SAMPLERATE_11025HZ;
        break;
        
    case 12000:
        rate = IAP2_SAMPLERATE_12000HZ;
        break;
        
    case 16000:
        rate = IAP2_SAMPLERATE_16000HZ;
        break;
        
    case 22050:
        rate = IAP2_SAMPLERATE_22050HZ;
        break;
        
    case 24000:
        rate = IAP2_SAMPLERATE_24000HZ;
        break;
        
    case 32000:
        rate = IAP2_SAMPLERATE_32000HZ;
        break;
        
    case 44100:
        rate = IAP2_SAMPLERATE_44100HZ;
        break;
        
    case 48000:
        rate = IAP2_SAMPLERATE_48000HZ;
        break;
    default:
        break;
    }
    
    return rate;
    
}

static void ippiAP2FreeForAccConfig(iAP2AccessoryConfig_t *accCfg)
{
    /* Parameter check */
    if(accCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, accCfg);
        return;
    }
    
    /*Free for GPIO power if allocated */
    if(accCfg->iAP2UsbOtgGPIOPower != NULL)
    {
        free(accCfg->iAP2UsbOtgGPIOPower);
    }
    
    /* Free for GPIO reset if allocated */
    if(accCfg->iAP2AuthGPIOReset != NULL)
    {
        free(accCfg->iAP2AuthGPIOReset);
    }
    
    /* Free for GPIO ready if allocated */
    if(accCfg->iAP2AuthGPIOReady != NULL)
    {
        free(accCfg->iAP2AuthGPIOReady);
    }
    
    /* Free for Ioctl register address if allocated */
    if(accCfg->iAP2AuthIoctlRegAddr != NULL)
    {
        free(accCfg->iAP2AuthIoctlRegAddr);
    }
    
    /* Free for authetication device name if allocated */
    if(accCfg->iAP2AuthDevicename != NULL)
    {
        free(accCfg->iAP2AuthDevicename);
    }
    
    /* Free for authetication auto detect if allocated */
    if(accCfg->iAP2AuthAutoDetect != NULL)
    {
        free(accCfg->iAP2AuthAutoDetect);
    }
    
    free(accCfg);
    
    return;
}

static iAP2AccessoryConfig_t *ippiAP2AllocateForAccConfig(void)
{
    S32 rc = IPOD_PLAYER_ERR_NOMEM;
    iAP2AccessoryConfig_t *accCfg = NULL;
    
    /* Allocate for accessory configuration  */
    accCfg = calloc(1, sizeof(*accCfg));
    if(accCfg != NULL)
    {
        /* Allocate for authentication device name */
        accCfg->iAP2AuthDevicename = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
        if(accCfg->iAP2AuthDevicename != NULL)
        {
            /* Allocate for authentication auto detect */
            accCfg->iAP2AuthAutoDetect = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
            if(accCfg->iAP2AuthAutoDetect != NULL)
            {
                /* Allocate for Ioctl register address */
                accCfg->iAP2AuthIoctlRegAddr = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                if(accCfg->iAP2AuthIoctlRegAddr != NULL)
                {
                    /* Allocate for GPIO ready */
                    accCfg->iAP2AuthGPIOReady = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                    if(accCfg->iAP2AuthGPIOReady != NULL)
                    {
                        /* Allocate for GPIO reset */
                        accCfg->iAP2AuthGPIOReset = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                        if(accCfg->iAP2AuthGPIOReset != NULL)
                        {
                            /* Allocate for GPIO power */
                            accCfg->iAP2UsbOtgGPIOPower = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                            if(accCfg->iAP2UsbOtgGPIOPower != NULL)
                            {
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
            
        if(rc != IPOD_PLAYER_OK)
        {
            ippiAP2FreeForAccConfig(accCfg);
            accCfg = NULL;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
    }
    
    return accCfg;
}

/**
 * Set the accessory configuration from configuration settings.
 */
static iAP2AccessoryConfig_t* ippiAP2SetAccessoryConfig(U8 devType, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    iAP2AccessoryConfig_t *accCfg = NULL;
    
    /* Allocate memory for accessory configuration */
    accCfg = ippiAP2AllocateForAccConfig();
    if(accCfg != NULL)
    {
        /* Set transport type from devType parameter */
        accCfg->iAP2TransportType = ippiAP2ConvertDeviceType(devType);
        if(accCfg->iAP2TransportType != iAP2GENERICMODE)
        {
            /* Set authentication type iAP2AUTHI2C */
            accCfg->iAP2AuthenticationType = iAP2AUTHI2C; /* Use i2c */
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, accCfg->iAP2AuthenticationType);
            rc = IPOD_PLAYER_ERR_INVALID_CFG;
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Set iOS in the car support */
            accCfg->iAP2iOSintheCar = threadInfo->accInfo.SupportedIOSInTheCar;
            
            /* Check transport type */
            /* If type is device, iAP2 control session is version 1 as default. */
            accCfg->ManualLinkConfig = TRUE;
            accCfg->LinkConfig_SessionVersion = 1;
            if(accCfg->iAP2TransportType == iAP2USBHOSTMODE)
            {
                /* If type is host, GPIO port for role switch is set from configuration */
                iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_CONFG_TRANS_USB_OTG_GPIO_POWER_AI, IPOD_PLAYER_STRING_LEN_MAX, accCfg->iAP2UsbOtgGPIOPower);

                /* If application uses iPodPlayer to carplay, control session version is 2. */
                if(accCfg->iAP2iOSintheCar == 1)        /* check configuration for support carplay */
                {
                    accCfg->LinkConfig_SessionVersion = 2; /* set 2 to control session version */
                }
            }
            
            /* Set short wait for Authentication from configuration  */
            accCfg->iAP2AuthShortWait = 0;
            /* Set wait for authentication from configuration */
            accCfg->iAP2AuthWait = 0;
            /* Set wait for authentication from configuration */
            accCfg->iAP2AuthLongWait = 0;
            /* Set available current for device from configuration */
            accCfg->iAP2AvailableCurrentForDevice = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_CONFG_AVAILABLE_CURRENT);
            /* Set battery should charge from configuration */
            accCfg->iAP2DeviceBatteryShouldChargeIfPowerIsPresent = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_CONFG_BATTERY_SHOULD_CHARGE);
            /* Set maximum current drawn from accessory from configuration */
            accCfg->iAP2MaximumcurrentDrawnFromAccessory = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAXIMUM_CURRENT_DRWAN_FROM_ACCESSORY);
            /* Set device battery will charge from configuration */
            accCfg->iAP2DeviceBatteryWillChargeIfPowerIsPresent = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_CONFG_BATTERY_SHOULD_CHARGE);
            /* Set configFS of Gadget driver from configuration */
            accCfg->useConfigFS = TRUE;

            if(ippiAP2CheckIdentificationTableFromDevice(IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER, threadInfo) ||
               ippiAP2CheckIdentificationTableFromDevice(IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE, threadInfo))
            {
                /* Set file xfer receive as stream from configuration */
                accCfg->iAP2FileXferRcvAsStream = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_CONFIG_FILE_XFER_STREAM);
                /* Set File xfer support from configuration */
                accCfg->iAP2FileXferSupported = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_FILE_XFER_SUPPORT);
            }
            else
            {
                /* Set file xfer receive as stream from configuration */
                accCfg->iAP2FileXferRcvAsStream = FALSE;
                /* Set File xfer support from configuration */
                accCfg->iAP2FileXferSupported = FALSE;
            }
            
            /* Set accessory power mode from configuration */
            if(ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_POWER_SOURCE_UPDATE, threadInfo))
            {
                accCfg->iAP2AccessoryPowerMode = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_CONFIG_POWER_MODE);
            }
            else
            {
                accCfg->iAP2AccessoryPowerMode = 0;
            }

            if(ippiAP2CheckIdentificationTableFromDevice(IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION, threadInfo))
            {
                /* Set EA protocol support from configuration */
                accCfg->iAP2EAPSupported = TRUE;
            }
            else
            {
                /* Set EA protocol support from configuration */
                accCfg->iAP2EAPSupported = FALSE;
            }

            /* Set EA Native transport support                */
            /*   Set it to FALSE. It may change to TRUE later */
            /*   depending on the contents of accInfo.        */
            accCfg->iAP2EANativeTransport = FALSE;
            
            /* Set authentication device name from configuration */
            *(accCfg->iAP2AuthDevicename) = '\0';
            /* Set Ioctl register address from configuration */
            *(accCfg->iAP2AuthIoctlRegAddr) = '\0';
            /* Set GPIO ready port from configuration */
            *(accCfg->iAP2AuthGPIOReady) = '\0';
            /* Set GPIO reset port from configuration */
            *(accCfg->iAP2AuthGPIOReset) = '\0';
            /* Set auto detection flag for authetication chip  */
            strncpy((char *)accCfg->iAP2AuthAutoDetect, IPP_IAP2_ENABLE_AUTO_DETECT, IPOD_PLAYER_STRING_LEN_MAX);
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accCfg);
        }
        
        if(rc != IPOD_PLAYER_OK)
        {
            ippiAP2FreeForAccConfig(accCfg);
            accCfg = NULL;
        }
    }
    
    return accCfg;
}

/**
 * Memory allocated by ippiAP2ClearAccessoryConfig will be freed by this function.
 */
static S32 ippiAP2ClearAccessoryConfig(iAP2AccessoryConfig_t * accCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(accCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, accCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Free accessory configuration memory */
    ippiAP2FreeForAccConfig(accCfg);
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

static void ippiAP2FreeForAccInfo(iAP2AccessoryInfo_t *accInfo)
{
    U32 i = 0;
    
    /* Parameter check */
    if(accInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, accInfo);
        return;
    }
    
    /* Free for Vehicle status if allocated */
    if(accInfo->iAP2VehicleStatusComponent != NULL)
    {
        free(accInfo->iAP2VehicleStatusComponent);
    }

    /* Free for Vehicle information if allocated */
    if(accInfo->iAP2VehicleInformationComponent != NULL)
    {
        /* Free for Vehicle information display name if allocated */
        if(accInfo->iAP2VehicleInformationComponent->iAP2DisplayName != NULL)
        {
            free(accInfo->iAP2VehicleInformationComponent->iAP2DisplayName);
        }

        if(accInfo->iAP2VehicleInformationComponent->iAP2EngineType != NULL)
        {
            free(accInfo->iAP2VehicleInformationComponent->iAP2EngineType);
        }

        free(accInfo->iAP2VehicleInformationComponent);
    }

    /* Free for location information if allocated */
    if(accInfo->iAP2LocationInformationComponent != NULL)
    {
        free(accInfo->iAP2LocationInformationComponent);
    }

    /* Free for preferred application bunlde seed identifier if allocated */
    if(accInfo->iAP2PreferredAppBundleSeedIdentifier != NULL)
    {
        free(accInfo->iAP2PreferredAppBundleSeedIdentifier);
    }
    
    /* Free for iOS application information if allocated */
    if(accInfo->iAP2iOSAppInfo != NULL)
    {
        for(i = 0; i < accInfo->iAP2SupportediOSAppCount; i++)
        {
            /* Free for iOS application name if allocated */
            if(accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName != NULL)
            {
                free(accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName);
            }
        }
        free(accInfo->iAP2iOSAppInfo);
    }
    
    if(accInfo->iAP2USBHIDComponent != NULL)
    {
        for(i = 0; i < IPODCORE_HID_COMPONENT_COUNT; i++)
        {
            if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName != NULL)
            {
                if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName[0] != NULL)
                {
                    free(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName[0]);
                    accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName[0] = NULL;
                }
                free(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName);
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName = NULL;
            }
            
            if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier != NULL)
            {
                free(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier);
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier = NULL;
            }
            
            if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction != NULL)
            {
                free(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction);
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction = NULL;
            }
        }
        
        free(accInfo->iAP2USBHIDComponent);
    }
    
    /* Free for bluetooth transport mac if allocated */
    if(accInfo->iAP2BluetoothTransportMAC != NULL)
    {
        free(accInfo->iAP2BluetoothTransportMAC);
    }
    
    /* Free for supported audio sample rate if allocated */
    if(accInfo->iAP2USBDeviceSupportedAudioSampleRate != NULL)
    {
        free(accInfo->iAP2USBDeviceSupportedAudioSampleRate);
    }
    
    /* Free for supported language if allocated */
    if(accInfo->iAP2SupportedLanguage != 0)
    {
        for(i = 0; i < IPODCORE_MAX_LANG_NUM; i++)
        {
            /* Free for one language if allocated */
            if(accInfo->iAP2SupportedLanguage[i] != NULL)
            {
                free(accInfo->iAP2SupportedLanguage[i]);
            }
        }
        free(accInfo->iAP2SupportedLanguage);
    }
    
    /* Free for current language if allocated */
    if(accInfo->iAP2CurrentLanguage != NULL)
    {
        free(accInfo->iAP2CurrentLanguage);
    }
    
    /* Free for callback expected from device if allocated */
    if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
    {
        free(accInfo->iAP2CallbacksExpectedFromDevice);
    }
    
    /* Free for commands used by application if allocated */
    if(accInfo->iAP2CommandsUsedByApplication != NULL)
    {
        free(accInfo->iAP2CommandsUsedByApplication);
    }
    
    /* Free for accessory bcd device if allocated */
    if(accInfo->iAP2AccessoryBcdDevice != NULL)
    {
        free(accInfo->iAP2AccessoryBcdDevice);
    }
   
    /* Free for accessory product plan uuid if allocated */
    if(accInfo->iAP2ProductPlanUUID != NULL)
    {
        free(accInfo->iAP2ProductPlanUUID);
    }
 
    /* Free for product id if allocated */
    if(accInfo->iAP2AccessoryProductId != NULL)
    {
        free(accInfo->iAP2AccessoryProductId);
    }
    
    /* Free for endpoint path if allocated */
    if(accInfo->iAP2InitEndPoint != NULL)
    {
        free(accInfo->iAP2InitEndPoint);
    }
    
    /* Free for vendor id if allocated */
    if(accInfo->iAP2AccessoryVendorId != NULL)
    {
        free(accInfo->iAP2AccessoryVendorId);
    }
    
    /* Free for hardware version if allocated */
    if(accInfo->iAP2AccessoryHardwareVersion != NULL)
    {
        free(accInfo->iAP2AccessoryHardwareVersion);
    }
    
    /* Free for firmware version if allocated */
    if(accInfo->iAP2AccessoryFirmwareVersion != NULL)
    {
        free(accInfo->iAP2AccessoryFirmwareVersion);
    }
    
    /* Free for serial number if allocated */
    if(accInfo->iAP2AccessorySerialNumber != NULL)
    {
        free(accInfo->iAP2AccessorySerialNumber);
    }
    
    /* Free for manufacturer if allocated */
    if(accInfo->iAP2AccessoryManufacturer != NULL)
    {
        free(accInfo->iAP2AccessoryManufacturer);
    }
    
    /* Free for model identifier if allocated */
    if(accInfo->iAP2AccessoryModelIdentifier != NULL)
    {
        free(accInfo->iAP2AccessoryModelIdentifier);
    }
    
    /* Free for BluetoothTransportComponent if allocated */
    if(accInfo->iAP2BluetoothTransportComponent != NULL)
    {
        free(accInfo->iAP2BluetoothTransportComponent);
    }

    /* Free for RouteGuidanceDisplayComponent if allocated */
    if(accInfo->iAP2RouteGuidanceDisplayComponent != NULL)
    {
        free(accInfo->iAP2RouteGuidanceDisplayComponent);
    }

    /* Free for accessory name if allocated */
    if(accInfo->iAP2AccessoryName != NULL)
    {
        free(accInfo->iAP2AccessoryName);
    }
    
    free(accInfo);
    
    return;
}

static S32 ippiAP2AllocateForAccInfo(iAP2AccessoryInfo_t *accInfo, U8 devType, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    size_t msg_cnt = 0;
    
    /* Allocate for accessory infomation */
    /* Allocate for accessory name */
    accInfo->iAP2AccessoryName = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
    if(accInfo->iAP2AccessoryName != NULL)
    {
        /* Allocate for model identifier */
        accInfo->iAP2AccessoryModelIdentifier = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
        if(accInfo->iAP2AccessoryModelIdentifier != NULL)
        {
            /* Allocate for manufacturer */
            accInfo->iAP2AccessoryManufacturer = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
            if(accInfo->iAP2AccessoryManufacturer != NULL)
            {
                /* Allocate for serial number */
                accInfo->iAP2AccessorySerialNumber = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                if(accInfo->iAP2AccessorySerialNumber != NULL)
                {
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
    
    if( (rc == IPOD_PLAYER_OK && ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION, threadInfo)) || 
        (rc == IPOD_PLAYER_OK && devType == IPOD_PLAYER_DEVICE_TYPE_BT)
        )
    {
        /* Allocate for Bluetooth Transport Component */
        accInfo->iAP2BluetoothTransportComponent = (iAP2BluetoothTransportComponent*)
                                                calloc(accInfo->iAP2BluetoothTransportComponent_count,
                                                sizeof(iAP2BluetoothTransportComponent));
        if(accInfo->iAP2BluetoothTransportComponent == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate for firmware version */
        accInfo->iAP2AccessoryFirmwareVersion = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
        if(accInfo->iAP2AccessoryFirmwareVersion != NULL)
        {
            /* Allocate for hardware version */
            accInfo->iAP2AccessoryHardwareVersion = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
            if(accInfo->iAP2AccessoryHardwareVersion != NULL)
            {
                /* Allocate for vendor id */
                accInfo->iAP2AccessoryVendorId = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                if(accInfo->iAP2AccessoryVendorId != NULL)
                {
                    /* Allocate for product id */
                    accInfo->iAP2AccessoryProductId = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                    if(accInfo->iAP2AccessoryProductId != NULL)
                    {
                        /* Allocate for endpoint path */
                        accInfo->iAP2InitEndPoint = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                        if(accInfo->iAP2InitEndPoint != NULL)
                        {
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
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate for bcd device*/
        accInfo->iAP2AccessoryBcdDevice = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
        if(accInfo->iAP2AccessoryBcdDevice == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
   
    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate for product plan uuid*/
        accInfo->iAP2ProductPlanUUID = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
        if(accInfo->iAP2ProductPlanUUID == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    } 
    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate for commands used by application */
        if(threadInfo->accInfo.MsgSentByAccSize == IPOD_PLAYER_ACCINFO_NULL)
        {
            /* Use configuration parameter */
            if(devType == IPOD_PLAYER_DEVICE_TYPE_BT)                   /* Bluetooth */
            {
                accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(BTMsgSentByAcc));
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE)        /* iAP2 USB device */
            {
                accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(USBDeviceModeMsgSentByAcc));
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_HOST)      /* iAP2 USB host */
            {
                accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(USBHostModeMsgSentByAcc));
            }
            if(accInfo->iAP2CommandsUsedByApplication == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        else
        {
            /* Use API argument */
            accInfo->iAP2CommandsUsedByApplication = calloc(1, threadInfo->accInfo.MsgSentByAccSize);
            if(accInfo->iAP2CommandsUsedByApplication == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
    }

    /* RouteGuidanceUpdate  */
    if(rc == IPOD_PLAYER_OK && ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_START_ROUTE_GUIDANCE_UPDATE, threadInfo))
    {
        if( accInfo->iAP2RouteGuidanceDisplayComponent_count > 0 )
        {
            accInfo->iAP2RouteGuidanceDisplayComponent = (iAP2RouteGuidanceDisplayComponent*)
                                                      calloc(accInfo->iAP2RouteGuidanceDisplayComponent_count,
                                                      sizeof(iAP2RouteGuidanceDisplayComponent));
            if(accInfo->iAP2RouteGuidanceDisplayComponent == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
    }
    else
    {
        accInfo->iAP2RouteGuidanceDisplayComponent_count = 0;
    }

    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate for callbacks expected from device */
        /* An array of message ID is set.*/
        if(threadInfo->accInfo.MsgRecvFromDeviceSize == IPOD_PLAYER_ACCINFO_NULL)
        {
            /* Use configuration parameter */
            if(devType == IPOD_PLAYER_DEVICE_TYPE_BT)               /* Bluetooth */
            {
                if(accInfo->iAP2SupportediOSAppCount > 0)
                {
                    msg_cnt = sizeof(BTMsgRecvFromDeviceWithExt);
                }
                else
                {
                    msg_cnt = sizeof(BTMsgRecvFromDevice);
                }
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE)    /* iAP2 USB device */
            {
                if(accInfo->iAP2SupportediOSAppCount > 0)
                {
                    msg_cnt = sizeof(USBDeviceModeMsgRecvFromDeviceWithExt);
                }
                else
                {
                    msg_cnt = sizeof(USBDeviceModeMsgRecvFromDevice);
                }
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_HOST)  /* iAP2 USB host */
            {
                if(accInfo->iAP2SupportediOSAppCount > 0)
                {
                    msg_cnt = sizeof(USBHostModeMsgRecvFromDeviceWithExt);
                }
                else
                {
                    msg_cnt = sizeof(USBHostModeMsgRecvFromDevice);
                }
            }

            accInfo->iAP2CallbacksExpectedFromDevice = calloc(1, msg_cnt);
            if(accInfo->iAP2CallbacksExpectedFromDevice == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        else
        {
            /* Use API argument */
            accInfo->iAP2CallbacksExpectedFromDevice = calloc(1, threadInfo->accInfo.MsgRecvFromDeviceSize);
            if(accInfo->iAP2CallbacksExpectedFromDevice == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate for current language */
        accInfo->iAP2CurrentLanguage = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
        if(accInfo->iAP2CurrentLanguage != NULL)
        {
            /* Allocate for supported languages */
            accInfo->iAP2SupportedLanguage = calloc(IPODCORE_MAX_LANG_NUM, sizeof(U8 *));
            if(accInfo->iAP2SupportedLanguage != NULL)
            {
                for(i = 0; i < IPODCORE_MAX_LANG_NUM; i++)
                {
                    /* Allocate for supported language */
                    accInfo->iAP2SupportedLanguage[i] = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                    if(accInfo->iAP2SupportedLanguage[i] == NULL)
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                        rc = IPOD_PLAYER_ERR_NOMEM;
                        break;
                    }
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Allocate for supported sample rate */
                    accInfo->iAP2USBDeviceSupportedAudioSampleRate = calloc(IPODCORE_MAX_SAMPLE_NUM, sizeof(*accInfo->iAP2USBDeviceSupportedAudioSampleRate));
                    if(accInfo->iAP2USBDeviceSupportedAudioSampleRate != NULL)
                    {
                        /* Allocate for bluetooth mac */
                        accInfo->iAP2BluetoothTransportMAC = calloc(IPOD_PLAYER_BT_MAC_COUNT_MAX, sizeof(U64));
                        if(accInfo->iAP2BluetoothTransportMAC != NULL)
                        {
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
    
    /* Human Interface Device */
    if(rc == IPOD_PLAYER_OK && ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_START_HID, threadInfo))
    {
        accInfo->iAP2USBHIDComponent = (iAP2iAP2HIDComponent*)calloc(IPODCORE_HID_COMPONENT_COUNT, sizeof(iAP2iAP2HIDComponent));
        if(accInfo->iAP2USBHIDComponent != NULL)
        {
            for(i = 0; (i < IPODCORE_HID_COMPONENT_COUNT); i++)
            {
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
                if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction != NULL)
                {
                    accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier = calloc(1, sizeof(U16));
                    if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier != NULL)
                    {
                        accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName = calloc(1, sizeof(U8 *));
                        if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName != NULL)
                        {
                            accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName[0] = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                            if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName[0] != NULL)
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                            else
                            {
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                                rc = IPOD_PLAYER_ERR_NOMEM;
                                break;
                            }
                        }
                        else
                        {
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                            rc = IPOD_PLAYER_ERR_NOMEM;
                            break;
                        }
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                        rc = IPOD_PLAYER_ERR_NOMEM;
                        break;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                    rc = IPOD_PLAYER_ERR_NOMEM;
                    break;
                }
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }

    /* External Accessory session */
    if(rc == IPOD_PLAYER_OK)
    {
        /* check availability regarding defined iOS application */
        if(accInfo->iAP2SupportediOSAppCount > 0)
        {
            accInfo->iAP2iOSAppInfo = calloc(accInfo->iAP2SupportediOSAppCount, sizeof(*accInfo->iAP2iOSAppInfo));
            if(accInfo->iAP2iOSAppInfo != NULL)
            {
                for(i = 0; i < accInfo->iAP2SupportediOSAppCount; i++)
                {
                    accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                    if(accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName == NULL)
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                        rc = IPOD_PLAYER_ERR_NOMEM;
                        break;
                    }
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    accInfo->iAP2PreferredAppBundleSeedIdentifier = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
                    if(accInfo->iAP2PreferredAppBundleSeedIdentifier != NULL)
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                        rc = IPOD_PLAYER_ERR_NOMEM;
                    }
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
    }
    
    /* location information component */
    if(rc == IPOD_PLAYER_OK && ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_LOCATION_INFORMATION, threadInfo))
    {
        accInfo->iAP2LocationInformationComponent = (iAP2LocationInformationComponent_t*)calloc(1, sizeof(iAP2LocationInformationComponent_t) );
        if(accInfo->iAP2LocationInformationComponent == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo->iAP2LocationInformationComponent);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }

    /* vehicle information component */
    accInfo->iAP2VehicleInformationComponent = (iAP2VehicleInformationComponent_t *)calloc(1, sizeof(iAP2VehicleInformationComponent_t) );
    if(accInfo->iAP2VehicleInformationComponent == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo->iAP2VehicleInformationComponent);
        rc = IPOD_PLAYER_ERR_NOMEM;
    }else{
        /* Defining multiple engine types */
        int EngineTypeNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_VINFO_ENGINE_COUNT);
        if(EngineTypeNum > IPODCORE_VINFO_ENGINE_MAX)
        {
            EngineTypeNum = IPODCORE_VINFO_ENGINE_MAX;
        }

        accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count = EngineTypeNum;
        accInfo->iAP2VehicleInformationComponent->iAP2EngineType = (iAP2EngineTypes*)calloc(accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count, sizeof(iAP2EngineTypes));
        if(accInfo->iAP2VehicleInformationComponent->iAP2EngineType != NULL)
        {
            /* Allocate for display name */
            accInfo->iAP2VehicleInformationComponent->iAP2DisplayName = calloc(IPOD_PLAYER_STRING_LEN_MAX, sizeof(U8));
            if(accInfo->iAP2VehicleInformationComponent->iAP2DisplayName == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo->iAP2VehicleInformationComponent->iAP2DisplayName);
                rc = IAP2_ERR_NO_MEM;
            }
        }else{
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo);
            rc = IAP2_ERR_NO_MEM;
        }
    }

    /* vehicle status component */
    if(rc == IPOD_PLAYER_OK && ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_VEHICLE_STATUS_UPDATE, threadInfo))
    {
        accInfo->iAP2VehicleStatusComponent = (iAP2VehicleStatusComponent_t*)calloc(1, sizeof(iAP2VehicleStatusComponent_t) );
        if(accInfo->iAP2VehicleStatusComponent == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo->iAP2VehicleStatusComponent);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        ippiAP2FreeForAccInfo(accInfo);
    }
    
    return rc;
}

U32 getNMEAType(char *type)
{
    unsigned int i = 0;
    U32 rc = 0;

    typedef struct NMEATable_type
    {
        char    *name;
        U32     mask;
    } NMEATable;
    
    NMEATable table[] = {
        { "GPGGA", IPOD_PLAYER_NMEA_MASK_GPGGA},
        { "GPRMC", IPOD_PLAYER_NMEA_MASK_GPRMC},
        { "GPGSV", IPOD_PLAYER_NMEA_MASK_GPGSV},
        { "PASCD", IPOD_PLAYER_NMEA_MASK_PASCD},
        { "PAGCD", IPOD_PLAYER_NMEA_MASK_PAGCD},
        { "PAACD", IPOD_PLAYER_NMEA_MASK_PAACD},
        { "GPHDT", IPOD_PLAYER_NMEA_MASK_GPHDT}
    };

    for(i = 0; i < sizeof(table) / sizeof(table[0]); i++)
    {
        if(strncmp(type, table[i].name, 5) == 0)
        {
            rc = table[i].mask;
        }
    }

    return rc;
}

/**
 * Accessory information parameter is set  from configuration settings by this
 * function.
 * Accessory information 
 */
static iAP2AccessoryInfo_t* ippiAP2SetAccessoryInfo(U8 devType, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo, iAP2AccessoryConfig_t *accCfg)
{
    iAP2AccessoryInfo_t *accInfo = NULL;
    U32 i = 0;
    U8 tempName[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    S32 rc = IPOD_PLAYER_ERROR;
    U8  BT_TransCompName[] = IAP2_BT_TRANS_COMP_NAME;
    U16 BT_TransCompIdentifier = IAP2_BT_TRANS_COMP_ID;
    U32 locInfoNum = 0;
    U32 VInfoEngNum = 0;
    U32 VStatusNum = 0;
    uint64_t bt_mac_addr = 0;
    char NMEAtype[6] = {0};
    char ENGType[10] = {0};
    char VStatusType[32] = {0};
    U32 type = 0;
    size_t MsgSentByAcc_size = 0;
    size_t MsgRecvFromDevice_size = 0;
    U16 *MsgSentByAcc = USBDeviceModeMsgSentByAcc;
    U16 *MsgRecvFromDevice = USBDeviceModeMsgRecvFromDeviceWithExt;
    iAP2TransportType_t TransType = ippiAP2ConvertDeviceType(devType);

    const IPOD_PLAYER_CORE_CFG_INFO *cfgInfo = iPodCoreGetCfgs();
    char temp[40] = {0};
    
    /* Parameter check */
    if(cfgInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, accInfo);
        return NULL;
    }

    /* Allocate accessory information memory */
    accInfo = calloc(1, sizeof(*accInfo));
    if(accInfo == NULL){
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo);
        return NULL;
    }

    /* set ios app count */
    accInfo->iAP2SupportediOSAppCount = 0;
    /* set supported app count to accessory infomation object. */
    if((threadInfo->accInfo.MsgRecvFromDeviceSize == IPOD_PLAYER_ACCINFO_NULL) &&
                                    (threadInfo->accInfo.MsgSentByAccSize == IPOD_PLAYER_ACCINFO_NULL))
    {
        if(ippiAP2CheckIdentificationTableFromDevice(IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION, threadInfo))
        {
            /* use configuration */
            accInfo->iAP2SupportediOSAppCount = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_IOSAPP_COUNT);
        }
    }
    else
    {
        /* use API argument */
        accInfo->iAP2SupportediOSAppCount = threadInfo->accInfo.SupportediOSAppCount;
    }

    /* set Bluetooth mac count */
    if( ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION, threadInfo) || 
        (devType == IPOD_PLAYER_DEVICE_TYPE_BT))
    {
        /* Check the MAC count in Bluetooth */
        /* more than one iAP2BluetoothTransportComponent possible */
        /* if devInfo == NULL,Configuration Setting is applied*/
        if(threadInfo->btInfo.macCount == 0)
        {
            accInfo->iAP2BluetoothTransportComponent_count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC_COUNT);
        }
        else
        {
            accInfo->iAP2BluetoothTransportComponent_count = threadInfo->btInfo.macCount;
        }
    }
    else
    {
        accInfo->iAP2BluetoothTransportComponent_count = 0;
    }

    accInfo->iAP2RouteGuidanceDisplayComponent_count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_COUNT);

    /* allocation for accessory information */
    rc = ippiAP2AllocateForAccInfo(accInfo, devType, threadInfo);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Check the beginning of string array to select sources of the accessory identification information name. 
           If beginning of array is "0", configuration data is used to iAP2 library. */
        /* accessory name */
        if(threadInfo->accInfo.Name[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_NAME, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, accInfo->iAP2AccessoryName);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryName, (char *)threadInfo->accInfo.Name, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* Serial Number */
        if(threadInfo->accInfo.SerialNumber[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_SERIAL_NUM, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, accInfo->iAP2AccessorySerialNumber);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessorySerialNumber, (char *)threadInfo->accInfo.SerialNumber, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* Manufacture */
        if(threadInfo->accInfo.Manufacturer[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_MANUFACTURE, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, accInfo->iAP2AccessoryManufacturer);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryManufacturer, (char *)threadInfo->accInfo.Manufacturer, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* Model Number */
        if(threadInfo->accInfo.ModelNumber[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_MODEL_NUM, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, accInfo->iAP2AccessoryModelIdentifier);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryModelIdentifier, (char *)threadInfo->accInfo.ModelNumber, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* HW Version */
        if(threadInfo->accInfo.Hardware_version_iap2[0] == 0)
        {
            for(i = 0; i < 3; i++)
            {
                iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_HWVER, i, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, (U8 *)temp);
                strcat((char *)accInfo->iAP2AccessoryHardwareVersion, (char *)temp);
                if(i != 2)
                {
                    strcat((char *)accInfo->iAP2AccessoryHardwareVersion, " ");
                }
            }
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryHardwareVersion, (char *)threadInfo->accInfo.Hardware_version_iap2, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* SW Version */
        if(threadInfo->accInfo.Software_version_iap2[0] == 0)
        {
            for(i = 0; i < 3; i++)
            {
                iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_TOKEN_ACCINFO_FWVER, i,  IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, (U8 *)temp);
                strcat((char *)accInfo->iAP2AccessoryFirmwareVersion, (char *)temp);
                if(i != 2)
                {
                    strcat((char *)accInfo->iAP2AccessoryFirmwareVersion, " ");
                }
            }
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryFirmwareVersion, (char *)threadInfo->accInfo.Software_version_iap2, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }

        /* vendor id */
        if(threadInfo->accInfo.VendorId[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_VENDOR_ID, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2AccessoryVendorId);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryVendorId, (char *)threadInfo->accInfo.VendorId, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* Product ID */ 
        if(threadInfo->accInfo.ProductId[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_PRODUCT_ID, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2AccessoryProductId);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryProductId, (char *)threadInfo->accInfo.ProductId, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* BCD DEVICE */ 
        if(threadInfo->accInfo.BCDDevice[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_BCD_DEVICE, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2AccessoryBcdDevice);
        }
        else
        {
            strncpy((char *)accInfo->iAP2AccessoryBcdDevice, (char *)threadInfo->accInfo.BCDDevice, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }
        /* Product Plan UUID */ 
        if(threadInfo->accInfo.ProductPlanUUID[0] == 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_PRODUCT_PLAN_UUID, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2ProductPlanUUID);
        }
        else
        {
            strncpy((char *)accInfo->iAP2ProductPlanUUID, (char *)threadInfo->accInfo.ProductPlanUUID, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX-1);
        }

        
        if(threadInfo->accInfo.MsgSentByAccSize == IPOD_PLAYER_ACCINFO_NULL)
        {
           /* Message ID Definition */
            if (devType == IPOD_PLAYER_DEVICE_TYPE_BT)
            {
                /* Bluetooth */
                MsgSentByAcc = BTMsgSentByAcc;
                MsgSentByAcc_size = IAP2_MSG_TBL_SIZE(BTMsgSentByAcc);
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE)
            {
                /* iAP2 USB device */
                MsgSentByAcc = USBDeviceModeMsgSentByAcc;
                MsgSentByAcc_size = IAP2_MSG_TBL_SIZE(USBDeviceModeMsgSentByAcc);
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_HOST)
            {
                /* iAP2 USB host */
                MsgSentByAcc = USBHostModeMsgSentByAcc;
                MsgSentByAcc_size = IAP2_MSG_TBL_SIZE(USBHostModeMsgSentByAcc);
            }
        }
        else
        {
            /* API argument */
            MsgSentByAcc = threadInfo->accInfo.MsgSentByAcc;
            MsgSentByAcc_size = threadInfo->accInfo.MsgSentByAccSize;
        }


        /* Message ID (sent by message, receive from device) */
        if(threadInfo->accInfo.MsgRecvFromDeviceSize == IPOD_PLAYER_ACCINFO_NULL)
        {
           /* Message ID Definition */
            if (devType == IPOD_PLAYER_DEVICE_TYPE_BT)
            {
                /* Bluetooth */
                if(accInfo->iAP2SupportediOSAppCount > 0)
                {
                    MsgRecvFromDevice = BTMsgRecvFromDeviceWithExt;
                    MsgRecvFromDevice_size = IAP2_MSG_TBL_SIZE(BTMsgRecvFromDeviceWithExt);
                }
                else
                {
                    MsgRecvFromDevice = BTMsgRecvFromDevice;
                    MsgRecvFromDevice_size = IAP2_MSG_TBL_SIZE(BTMsgRecvFromDevice);
                }
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE)
            {
                /* iAP2 USB device */
                if(accInfo->iAP2SupportediOSAppCount > 0)
                {
                    MsgRecvFromDevice = USBDeviceModeMsgRecvFromDeviceWithExt;
                    MsgRecvFromDevice_size = IAP2_MSG_TBL_SIZE(USBDeviceModeMsgRecvFromDeviceWithExt);
                }
                else
                {
                    MsgRecvFromDevice = USBDeviceModeMsgRecvFromDevice;
                    MsgRecvFromDevice_size = IAP2_MSG_TBL_SIZE(USBDeviceModeMsgRecvFromDevice);
                }
            }
            else if(devType == IPOD_PLAYER_DEVICE_TYPE_USB_HOST)
            {
                
                if(accInfo->iAP2SupportediOSAppCount > 0)
                {
                    MsgRecvFromDevice = USBHostModeMsgRecvFromDeviceWithExt;
                    MsgRecvFromDevice_size = IAP2_MSG_TBL_SIZE(USBHostModeMsgRecvFromDeviceWithExt);
                }
                else
                {
                    MsgRecvFromDevice = USBHostModeMsgRecvFromDevice;
                    MsgRecvFromDevice_size = IAP2_MSG_TBL_SIZE(USBHostModeMsgRecvFromDevice);
                }
            }
        }
        else
        {
            /* API argument */
            MsgRecvFromDevice = threadInfo->accInfo.MsgRecvFromDevice;
            MsgRecvFromDevice_size= threadInfo->accInfo.MsgRecvFromDeviceSize;
        }

        memcpy(accInfo->iAP2CommandsUsedByApplication, MsgSentByAcc, MsgSentByAcc_size );
        accInfo->iAP2CommandsUsedByApplication_length = MsgSentByAcc_size;
        
        memcpy(accInfo->iAP2CallbacksExpectedFromDevice, MsgRecvFromDevice, MsgRecvFromDevice_size );
        accInfo->iAP2CallbacksExpectedFromDevice_length = MsgRecvFromDevice_size;
        
        /* Current language */
        iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_CURRENT_LANGUAGE, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2CurrentLanguage);
        
        /* Supported language */
        for(i = 0; i < cfgInfo[IPOD_PLAYER_CFGNUM_ACC_INFO_SUPPORTED_LANGUAGE].count; i++)
        {
            if(cfgInfo[IPOD_PLAYER_CFGNUM_ACC_INFO_SUPPORTED_LANGUAGE].value.strValue[i].strVal[0] != '\0')
            {
                strncpy((char *)accInfo->iAP2SupportedLanguage[i], (const char *)cfgInfo[IPOD_PLAYER_CFGNUM_ACC_INFO_SUPPORTED_LANGUAGE].value.strValue[i].strVal, IPOD_PLAYER_STRING_LEN_MAX - 1);
            }
            else
            {
                break;
            }
        }
        accInfo->iAP2SupportedLanguageCount = i;
        
        /* Support Sample Rate */
        for(i = 0; i < (U32)*cfgInfo[IPOD_PLAYER_CFGNUM_ACC_INFO_SAMPLE_RATE_COUNT].value.intValue; i++)
        {
            accInfo->iAP2USBDeviceSupportedAudioSampleRate[i] = ippiAP2ConvertSampleRate(iPodCoreGetIndexCfn(IPOD_PLAYER_CFGNUM_ACC_INFO_SUPPORT_SAMPLE_RATE, i));
        }
        accInfo->iAP2USBDeviceSupportedAudioSampleRate_count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ACC_INFO_SAMPLE_RATE_COUNT);

        /* Bluetooth Transport Component */
        if(ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION, threadInfo) || 
            (devType == IPOD_PLAYER_DEVICE_TYPE_BT))
        {
            for(i=0; i < accInfo->iAP2BluetoothTransportComponent_count; i++)
            {
                if(threadInfo->btInfo.macCount == 0)
                {
                    U64 tempVal;
                    memset(tempName, 0, sizeof(tempName));
                    iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_BT_MAC, i, sizeof(tempName), tempName);
                    tempVal = strtoull((const char*)tempName, NULL, 16);
                    tempVal <<= 16;
                    bt_mac_addr = be64toh(tempVal);
                }
                else
                {
                    memcpy(&bt_mac_addr,&threadInfo->btInfo.macAddr[i].addr,IPOD_PLAYER_BT_MAC_ADDR_LEN_MAX);
                }    
                accInfo->iAP2BluetoothTransportMAC[i] = bt_mac_addr;

                rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                                &bt_mac_addr,
                                                &accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress_count,
                                                IAP2_BT_MAC_LENGTH, iAP2_blob);

                if(rc == IPOD_PLAYER_OK)
                {
                    /* each iAP2BluetoothTransportComponent must have an unique identifier */
                    BT_TransCompIdentifier = i;
                    rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                                      &BT_TransCompIdentifier,
                                                      &accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                                      1, iAP2_uint16);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* provide the name of the Bluetooth transport (e.g. My Car) */
                        U8* BTTransCompName = BT_TransCompName;
                        rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                        &BTTransCompName,
                                                        &accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName_count,
                                                        1, iAP2_utf8);
                    }

                    if(rc == IPOD_PLAYER_OK)
                    {
                       /* Set if Bluetooth component supports iAP2 connection */
                       accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
                    }
                }
            }
            accInfo->iAP2BluetoothTransportMAC_count = i;
        }
        
        /* USB HID Component */
        if(ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_START_HID, threadInfo))
        {
            accInfo->iAP2USBHIDComponent_count = IPODCORE_HID_COMPONENT_COUNT;
            for(i = 0; (i < IPODCORE_HID_COMPONENT_COUNT); i++)
            {
                *(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction) = IAP2_HID_COMPONENT_PLAYBACK_REMOTE;
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction_count = 1;
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier[0] = 0x0000;
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier_count = 1;
                strncpy((char *)accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName[0], (const char *)IPOD_PLAYER_CORE_HID_COMPONENT_NAME, IPOD_PLAYER_STRING_LEN_MAX - 1);
                accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName_count = 1;
            }
        }
        
        /* set Host Mode endpoint path */
        if(TransType == iAP2USBHOSTMODE)
        {
            strncpy((char *)accInfo->iAP2InitEndPoint, (const char *)threadInfo->endpointPath, IPOD_PLAYER_STRING_LEN_MAX - 1);
        }

        for(i=0; i < accInfo->iAP2RouteGuidanceDisplayComponent_count; i++)
        {
            U16 RG_Display_Identifier = IAP2_ROUTE_GUIDANCE_DISPLAY_COMP_ID;
            U8  RG_Display_Name[1][256] = {IAP2_ROUTE_GUIDANCE_DISPLAY_COMP_NAME};
            U16 RG_Max_Current_Road_Name_Length[] = {256};
            U16 RG_Max_Destination_Road_Name_Length[] = {256};
            U16 RG_Max_After_Maneuver_Road_Name_Length[] = {256};
            U16 RG_Max_Maneuver_Description_Length[] = {512};
            U16 RG_Max_GuidanceManeuver_Storage_Capacity[] = {20};
            U16 RG_Max_LaneGuidance_Description_Length[] = {15};
            U16 RG_Max_LaneGuidance_Storage_Capacity[] = {1};
            U16 RG_Count;

            /* Route Guidance Display Component */
            /* set Identifier */
            RG_Display_Identifier = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_ID);
            /* set Identifier count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_ID_COUNT);

            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                              &RG_Display_Identifier,
                                              &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier_count,
                                              RG_Count, iAP2_uint16);

            /* set Name */
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_NAME, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX, RG_Display_Name[i]);
            /* set Name count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_NAME_COUNT);
            U8* RGDisplay_Name = RG_Display_Name[i];

            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                              &RGDisplay_Name,
                                              &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Name_count,
                                              RG_Count, iAP2_utf8);

            /* set MaxCurrentRoadNameLength */
            RG_Max_Current_Road_Name_Length[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_CURRENTROADNAME_LEN);
            /* set MaxCurrentRoadNameLength count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_CURRENTROADNAME_COUNT);

            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                              &RG_Max_Current_Road_Name_Length,
                                              &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count,
                                              RG_Count, iAP2_uint16);

            /* set MaxDestinationRoadNameLength */
            RG_Max_Destination_Road_Name_Length[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_DESTROADNAME_LEN);
            /* set MaxDestinationRoadNameLength count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_DESTROADNAME_LEN_COUNT);
            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                              &RG_Max_Destination_Road_Name_Length,
                                              &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count,
                                              RG_Count, iAP2_uint16);

            /* set MaxAfterManeuverRoadNameLength */
            RG_Max_After_Maneuver_Road_Name_Length[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_AFTERMANEROADNAME_LEN);
            /* set MaxAfterManeuverRoadNameLength count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_AFTERMANEROADNAME_LEN_COUNT);
            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                              &RG_Max_After_Maneuver_Road_Name_Length,
                                              &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count,
                                              RG_Count, iAP2_uint16);

            /* set MaxManeuverDescriptionLength */
            RG_Max_Maneuver_Description_Length[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_MANEDESCRIPT_LEN);
            /* set MaxManeuverDescriptionLength count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_MANEDESCRIPT_LEN_COUNT);
            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                           &RG_Max_Maneuver_Description_Length,
                                           &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count,
                                           RG_Count, iAP2_uint16);

            /* set MaxGuidanceManeuverStorageCapacity */
            RG_Max_GuidanceManeuver_Storage_Capacity[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_GUIDMANESTORCAP);
            /* set MaxGuidanceManeuverStorageCapacity count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_GUIDMANESTORCAP_COUNT);
            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                           &RG_Max_GuidanceManeuver_Storage_Capacity,
                                           &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count,
                                           RG_Count, iAP2_uint16);

            /* set MaxLaneGuidanceDescriptionLength */
            RG_Max_LaneGuidance_Description_Length[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANEDESCRIPT_LEN);
            /* set MaxManeuverDescriptionLength count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANEDESCRIPT_LEN_COUNT);
            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                           &RG_Max_LaneGuidance_Description_Length,
                                           &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count,
                                           RG_Count, iAP2_uint16);

            /* set MaxLaneGuidanceStorageCapacity */
            RG_Max_LaneGuidance_Storage_Capacity[0] = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANESTORCAP);
            /* set MaxLaneGuidanceStorageCapacity count */
            RG_Count = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_ROUTE_GUID_COMP_MAX_LANESTORCAP_COUNT);
            rc = ippiAP2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                           &RG_Max_LaneGuidance_Storage_Capacity,
                                           &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count,
                                           RG_Count, iAP2_uint16);
        }

        /* iOS in the car */
        accInfo->iAP2SupportsiOSintheCar = threadInfo->accInfo.SupportedIOSInTheCar;
        
        /* iOS App */
        if((threadInfo->accInfo.MsgRecvFromDeviceSize == IPOD_PLAYER_ACCINFO_NULL) &&
                                        (threadInfo->accInfo.MsgSentByAccSize == IPOD_PLAYER_ACCINFO_NULL))
        {
            /* use configuration */
            for(i = 0; i < accInfo->iAP2SupportediOSAppCount; i++)
            {
                iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_IOSAPP_URL, i, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName);
                strncpy((char *)threadInfo->iosInfo[i].iOSAppName, (const char *)accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName, IPOD_PLAYER_STRING_LEN_MAX);
                threadInfo->iosInfo[i].iOSAppIdentifier     = accInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier   = i + 1;
                accInfo->iAP2iOSAppInfo[i].iAP2EAPMatchAction     = IAP2_NO_APP_MATCH;
                threadInfo->iosInfo[i].MatchAction          = (EAPMatchAction)accInfo->iAP2iOSAppInfo[i].iAP2EAPMatchAction;
                threadInfo->iosInfo[i].EANativeTransport    = accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport  = 0;
            }
        }
        else
        {
            /* use api argument */
            for(i = 0; i < accInfo->iAP2SupportediOSAppCount; i++)
            {
                strncpy((char *)accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName, (const char *)threadInfo->iosInfo[i].iOSAppName, IPOD_PLAYER_STRING_LEN_MAX);
                accInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = threadInfo->iosInfo[i].iOSAppIdentifier;
                accInfo->iAP2iOSAppInfo[i].iAP2EAPMatchAction = (iAP2ExternalAccessoryProtocolMatchAction)threadInfo->iosInfo[i].MatchAction;
                accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport = threadInfo->iosInfo[i].EANativeTransport;
                if(accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport != 0)
                {
                    /* Set EA Native transport support */
                    accCfg->iAP2EANativeTransport = TRUE;
                }
            }
        }

        if(accInfo->iAP2SupportediOSAppCount > 0)
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_ACC_INFO_PREFFERD_APP_BUNDLE, IPOD_PLAYER_STRING_LEN_MAX, accInfo->iAP2PreferredAppBundleSeedIdentifier);
        }
        
        /* Set location information component */
        /* get location information count */
        if(ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_LOCATION_INFORMATION, threadInfo))
        {
            locInfoNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_LOCATION_INFO_COUNT);
            if(locInfoNum > IPODCORE_LOCATION_NMEA_MAX)
            {
                locInfoNum =  IPODCORE_LOCATION_NMEA_MAX;
            }
             
            /* initialize of location information component */
            accInfo->iAP2LocationInformationComponent->iAP2GlobalPositioningSystemFixData = FALSE;
            accInfo->iAP2LocationInformationComponent->iAP2RecommendedMinimumSpecificGPSTransitData = FALSE;
            accInfo->iAP2LocationInformationComponent->iAP2GPSSatelliteInView = FALSE;
            accInfo->iAP2LocationInformationComponent->iAP2VehicleSpeedData = FALSE;
            accInfo->iAP2LocationInformationComponent->iAP2VehicleGyroData = FALSE;
            accInfo->iAP2LocationInformationComponent->iAP2VehicleAccelerometerData = FALSE;
            accInfo->iAP2LocationInformationComponent->iAP2VehicleHeadingData = FALSE;

            for(i = 0; i < locInfoNum; i++)
            {
                iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_LOCATION_INFO_NMEA, i, sizeof(NMEAtype), (U8 *)NMEAtype);
                type = getNMEAType(NMEAtype);      /* get NMEA sentence type mask */
                switch(type)
                {
                case IPOD_PLAYER_NMEA_MASK_GPGGA:
                    /* GlobalPositioningSystemFixData - NMEA GPGGA sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2GlobalPositioningSystemFixData = TRUE;
                    break;
                case IPOD_PLAYER_NMEA_MASK_GPRMC:
                    /* RecommendedMinimumSpecificGPSTransitData - NMEA GPRMC sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2RecommendedMinimumSpecificGPSTransitData = TRUE;
                    break;
                case IPOD_PLAYER_NMEA_MASK_GPGSV:
                    /* GPSSatellitesInView - NMEA GPGSV sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2GPSSatelliteInView = TRUE;
                    break;
                case IPOD_PLAYER_NMEA_MASK_PASCD:
                    /* VehicleSpeedData - NMEA PASCD sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2VehicleSpeedData = TRUE;
                    break;
                case IPOD_PLAYER_NMEA_MASK_PAGCD:
                    /* VehicleGyroData - NMEA PAGCD sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2VehicleGyroData = TRUE;
                    break;
                case IPOD_PLAYER_NMEA_MASK_PAACD:
                    /* VehicleAccelerometerData - NMEA PAACD sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2VehicleAccelerometerData = TRUE;
                    break;
                case IPOD_PLAYER_NMEA_MASK_GPHDT:
                    /* VehicleHeadingData - NMEA GPHDT sentences */
                    accInfo->iAP2LocationInformationComponent->iAP2VehicleHeadingData = TRUE;
                    break;
                default:
                    IPOD_DLT_ERROR("Invalid configuration data in NMEA sentence type(type = %d)", type);
                    rc = IPOD_PLAYER_ERROR;
                    break;
                }

                if(rc == IPOD_PLAYER_ERROR)
                {
                    return NULL;
                }
            }
        }

        /* Set vehicle information component */
        /*     get vehicle display name */
        //IPOD_DLT_INFO("[DBG]threadInfo->vehicleInfo.displayName_valid=%d", threadInfo->vehicleInfo.displayName_valid);
        if(threadInfo->vehicleInfo.displayName_valid)
        {
            strncpy((char *)accInfo->iAP2VehicleInformationComponent->iAP2DisplayName, (char *)threadInfo->vehicleInfo.displayName, IPOD_PLAYER_STRING_LEN_MAX);
        }
        else
        {
            iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_VINFO_DISPLAY_NAME, sizeof(tempName), tempName);
            strncpy((char *)accInfo->iAP2VehicleInformationComponent->iAP2DisplayName, (char *)tempName, sizeof(tempName));
        }
        //IPOD_DLT_INFO("[DBG]iAP2DisplayName=%s", accInfo->iAP2VehicleInformationComponent->iAP2DisplayName);

        /*     Defining multiple engine types */
        VInfoEngNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_VINFO_ENGINE_COUNT);
        if(VInfoEngNum > IPODCORE_VINFO_ENGINE_MAX)
        {
            VInfoEngNum =  IPODCORE_VINFO_ENGINE_MAX;
        }

        accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count = VInfoEngNum;
        for(i = 0; i < accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count; i++)
        {
            memset(ENGType, 0, sizeof(ENGType));
            iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_VINFO_ENGINE_TYPE, i, sizeof(ENGType), (U8 *)ENGType);

            if(strncmp(IPOD_PLAYER_ENGINE_TYPE_GASOLINE, ENGType, sizeof(ENGType)) == 0)
            {
                accInfo->iAP2VehicleInformationComponent->iAP2EngineType[i] = IAP2_GASOLINE;
            }
            else if(strncmp(IPOD_PLAYER_ENGINE_TYPE_DIESEL, ENGType, sizeof(ENGType)) == 0)
            {
                accInfo->iAP2VehicleInformationComponent->iAP2EngineType[i] = IAP2_DIESEL;
            }
            else if(strncmp(IPOD_PLAYER_ENGINE_TYPE_ELECTRIC, ENGType, sizeof(ENGType)) == 0)
            {
                accInfo->iAP2VehicleInformationComponent->iAP2EngineType[i] = IAP2_ELECTRIC;
            }
            else if(strncmp(IPOD_PLAYER_ENGINE_TYPE_CNG, ENGType, sizeof(ENGType)) == 0)
            {
                accInfo->iAP2VehicleInformationComponent->iAP2EngineType[i] = IAP2_CNG;
            }
        }

        /* Set vehicle status component */
        if(ippiAP2CheckIdentificationTableByAcc(IAP2_MSG_ID_VEHICLE_STATUS_UPDATE, threadInfo))
        {
            /* get vehicle status count */
            VStatusNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_VSTATUS_TYPE_COUNT);
            if(VStatusNum > IPODCORE_VSTATUS_TYPE_MAX)
            {
                VStatusNum =  IPODCORE_VSTATUS_TYPE_MAX;
            }
         
            /* initialize of vehicle status component */
            accInfo->iAP2VehicleStatusComponent->iAP2InsideTemperature  = FALSE;
            accInfo->iAP2VehicleStatusComponent->iAP2NightMode          = FALSE;
            accInfo->iAP2VehicleStatusComponent->iAP2OutsideTemperature = FALSE;
            accInfo->iAP2VehicleStatusComponent->iAP2RangeWarning       = FALSE;
            accInfo->iAP2VehicleStatusComponent->iAP2Range              = FALSE;

            /* Set vehicle status component */
            for(i = 0; i < VStatusNum; i++)
            {
                memset(VStatusType, 0, sizeof(VStatusType));
                iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_VSTATUS_TYPE, i, sizeof(VStatusType), (U8 *)VStatusType);
                if(strncmp(IPOD_PLAYER_VSTATUS_TYPE_RANGE, VStatusType, sizeof(VStatusType)) == 0)
                {
                    accInfo->iAP2VehicleStatusComponent->iAP2Range = TRUE;
                }
                else if(strncmp(IPOD_PLAYER_VSTATUS_TYPE_OUTSIDETEMP, VStatusType, sizeof(VStatusType)) == 0)
                {
                    accInfo->iAP2VehicleStatusComponent->iAP2OutsideTemperature = TRUE;
                }
                else if(strncmp(IPOD_PLAYER_VSTATUS_TYPE_RANGE_WARNING, VStatusType, sizeof(VStatusType)) == 0)
                {
                    accInfo->iAP2VehicleStatusComponent->iAP2RangeWarning = TRUE;
                }
            }
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, accInfo);
    }

    return accInfo;
}

/**
 * Memory allocated by ippiAP2ClearAccessoryInfo will be freed by this function
 */
static S32 ippiAP2ClearAccessoryInfo(iAP2AccessoryInfo_t * accInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(accInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, accInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Free accessory information memory */
    ippiAP2FreeForAccInfo(accInfo);
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

S32 ippiAP2HandleEvent(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param, S32 fd, S16 event)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    rc = iAP2HandleEvent(iap2Param->device, fd, event);
    
    return  rc;
} 

S32 ippiAP2GetPollFDs(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param)
{
    S32 rc = IAP2_CTL_ERROR;
    
    /* Parameter check */
    if((iap2Param == NULL) || (iap2Param->device == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get file descriptors to wait in poll */
    rc = iAP2GetPollFDs(iap2Param->device, &iap2Param->pollFDs);
    if(rc == IAP2_OK)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        /* Todo: Add suitable error number */
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

/**
 * Initialize the iAP2 Service.
 */
S32 ippiAP2Init(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 rca = 0;
    
    /* Parameter check */
    if((iap2Param == NULL) || (iap2Param->initParam == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    
    rca = iAP2ServiceDeviceDiscovered(g_service, iap2Param->initParam);
    if(rca >= 0)
    {
        IPOD_DLT_INFO("iAP2ServiceDeviceDiscovered:%d", rca);
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_DLT_ERROR("iAP2ServiceDeviceDiscovered failed :%d", rca);
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

/**
 * Finalize the iAP2 Service
 */
S32 ippiAP2Deinit(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param, U8 *serial)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rca = 0;
    S32 rcb = 0;
    S32 rcc = 0;
    iAP2ServiceDeviceInformation_t deviceInfo;
    
    /* Parameter check */
    if((iap2Param == NULL) || (serial == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param, serial);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    strncpy(deviceInfo.serial, (const char*)serial, sizeof(deviceInfo.serial));

    rca = iAP2ServiceDisconnectDevice(g_service, iap2Param->deviceId);
    rcb = iAP2ServiceDeviceDisappeared(g_service, &deviceInfo);
    rcc = iAP2ServiceDeInitDeviceStructure(g_service, iap2Param->device);
    if((rca >= 0) && (rcb >= 0) && (rcc >= 0))
    {
        IPOD_DLT_INFO("iAP2ServiceDisconnectDevice=%d, iAP2ServiceDeviceDisappeared=%d, iAP2ServiceDeInitDeviceStructure=%d, deviceId=%u, device=%p", rca, rcb, rcc, iap2Param->deviceId, iap2Param->device);
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_DLT_WARN("iAP2Service API failed :iAP2ServiceDisconnectDevice=%d, iAP2ServiceDeviceDisappeared=%d, iAP2ServiceDeInitDeviceStructure=%d, deviceId=%u, device=%p", rca, rcb, rcc, iap2Param->deviceId, iap2Param->device);
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}

/**
 * Initialize the parameter for iAP2 library.
 * Some settings will be gotten by configuration.
 */
S32 ippiAP2InitParam(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param, U8 devType, U8 * deviceName, void * callbackContext)
{
    S32 rc = IPOD_PLAYER_ERROR;
    iAP2InitParam_t *initParam = NULL;
    
    /* Parameter check */
    if((iap2Param == NULL) || (iap2Param->initParam != NULL) || (deviceName == NULL) || (callbackContext == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    initParam = calloc(1, sizeof(*initParam));
    if(initParam != NULL)
    {
        IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)callbackContext;
        /* Todo: No use this value */
        initParam->iAP2NumDevice = 1;
        
        /* Set device name */
        strncpy((char *)initParam->iAP2DeviceId, (const char*)deviceName, sizeof(initParam->iAP2DeviceId));
        initParam->iAP2DeviceId[STRING_MAX - 1] = '\0';
        
        /* Set accessory configuration */
        initParam->p_iAP2AccessoryConfig = ippiAP2SetAccessoryConfig(devType, iPodCtrlCfg->threadInfo);
        if(initParam->p_iAP2AccessoryConfig != NULL)
        {
            /* Set accessory information */
            initParam->p_iAP2AccessoryInfo = ippiAP2SetAccessoryInfo(devType, iPodCtrlCfg->threadInfo, initParam->p_iAP2AccessoryConfig);
            if(initParam->p_iAP2AccessoryInfo != NULL)
            {
                initParam->iAP2ContextCallback = callbackContext;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                /* Memory is not enough amount for accessory information */
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        else
        {
            /* Memory is not enough amount for accessory configuration */
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* configuration data is loaded to use power source updates message */
            iap2Param->powerInfo.chargeButtery = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_POWER_SUPPLY_CHARGE_BUTTERY);
            iap2Param->powerInfo.powermA = (IPOD_PLAYER_CURRENT)(iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_POWER_SUPPLY_CURRENT));
        }

        if(rc == IPOD_PLAYER_OK)
        {
            iap2Param->initParam = initParam;
        }
        /* Function failed */
        else
        {
            /* Memory for accessory configuration was allocated */
            if(initParam->p_iAP2AccessoryConfig != NULL)
            {
                /* Free memory */
                ippiAP2ClearAccessoryConfig(initParam->p_iAP2AccessoryConfig);
                initParam->p_iAP2AccessoryConfig = NULL;
            }
            free(initParam);
        }
    }
    else
    {
        /* Memory is not enough amount for initilize parameter */
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    return rc;
}

/**
 * Finalize the parameter for iAP2 library.
 * Memory allocated by ippiAP2InitParam will be freed by this function.
 */
void ippiAP2DeinitParam(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param)
{
    /* Parameter check */
    if((iap2Param == NULL) || (iap2Param->initParam == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param);
        return;
    }
    
    if(iap2Param->initParam->p_iAP2AccessoryInfo != NULL)
    {
        ippiAP2ClearAccessoryInfo(iap2Param->initParam->p_iAP2AccessoryInfo);
        iap2Param->initParam->p_iAP2AccessoryInfo = NULL;
    }
    
    if(iap2Param->initParam->p_iAP2AccessoryConfig != NULL)
    {
        ippiAP2ClearAccessoryConfig(iap2Param->initParam->p_iAP2AccessoryConfig);
        iap2Param->initParam->p_iAP2AccessoryConfig = NULL;
    }
    
    free(iap2Param->initParam);
    iap2Param->initParam = NULL;
    
    return;
}

/**
 * Initialize device connection by the iAP2 Service.
 */
S32 ippiAP2ServiceInitDeviceConnection(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 rca = 0;

    /* Parameter check */
    if((iap2Param == NULL) || (iap2Param->initParam == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Initialize structure for iAP2 */
    iap2Param->device = iAP2ServiceInitDeviceStructure(g_service, iap2Param->deviceId, iap2Param->initParam);
    if(iap2Param->device != NULL)
    {
        /* Initialize device connection for iAP2 */
        rca = iAP2ServiceInitDeviceConnection(g_service, iap2Param->device, iap2Param->initParam);
        if(rca >= 0)
        {
            IPOD_DLT_INFO("iAP2ServiceInitDeviceConnection:%d, deviceId=%u, device=%p", rca, iap2Param->deviceId, iap2Param->device);
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_DLT_ERROR("iAP2ServiceInitDeviceConnection failed :%d, deviceId=%u, device=%p", rca, iap2Param->deviceId, iap2Param->device);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_DLT_ERROR("iAP2ServiceInitDeviceStructure failed :deviceId=%u, device=%p", iap2Param->deviceId, iap2Param->device);
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}

