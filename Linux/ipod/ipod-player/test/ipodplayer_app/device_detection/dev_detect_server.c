#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h> /* needed to avoid 'implicit declaration of calloc ...' compiler warning */
#include <string.h>
#include <libudev.h>
#include <errno.h>
#include <signal.h>
#include <time.h>
#include <sys/eventfd.h>
#include <libusb-1.0/libusb.h>

#include "dev_detect_server.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerAPI.h"
#include "iPodPlayerDeviceDetection.h"
#include "iPodPlayerUtilityLog.h"

#include "iap2_init.h"

//#define IPOD_SET_DEV_INFO
//#define IPOD_SET_ACC_INFO
//#define IPOD_SET_VEHICLE_INFO

#ifdef IPOD_SET_ACC_INFO
    U16 mfiMsgByAcc[] = {
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
            IAP2_MSG_ID_START_ASSISTIVE_TOUCH,
            IAP2_MSG_ID_STOP_ASSISTIVE_TOUCH,
            IAP2_MSG_ID_START_ASSISTIVE_TOUCH_INFORMATION,
            IAP2_MSG_ID_STOP_ASSISTIVE_TOUCH_INFORMATION,
            IAP2_MSG_ID_BLUETOOTH_COMPONENT_INFORMATION,
            IAP2_MSG_ID_START_BLUETOOTH_CONNECTION_UPDATES,
            IAP2_MSG_ID_STOP_BLUETOOTH_CONNECTION_UPDATES,
            IAP2_MSG_ID_LOCATION_INFORMATION,
            IAP2_MSG_ID_VEHICLE_STATUS_UPDATE
    }; 
    MFI_MSGCODES g_msgSentByAcc = 
    { 
        .msgcodes   = mfiMsgByAcc, 
        .msgnum     = sizeof(mfiMsgByAcc)
    };
    
    U16 mfiMsgFromDevice[] = {
            IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,
            IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,
            IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,
            IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,
            IAP2_MSG_ID_DEVICE_LANGUAGE_UPDATE,
            IAP2_MSG_ID_POWER_UPDATE,
            IAP2_MSG_ID_DEVICE_HID_REPORT,
            IAP2_MSG_ID_ASSISTIVE_TOUCH_INFORMATION,
            IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
            IAP2_MSG_ID_STOP_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,
            IAP2_MSG_ID_START_LOCATION_INFORMATION,
            IAP2_MSG_ID_STOP_LOCATION_INFORMATION,
            IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,
            IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,
            IAP2_MSG_ID_BLUETOOTH_CONNECTION_UPDATE
    };

    MFI_MSGCODES g_msgRecvFromDevice = 
    {
        .msgcodes    = mfiMsgFromDevice,
        .msgnum      = sizeof(mfiMsgFromDevice) 
    };

    IPOD_PLAYER_IOS_APPINFO g_iOSInfo[2] =
    {
        {
            .iOSAppIdentifier   = 1,
            .iOSAppName         = (U8 *)"jp.co.denso.NaviCon",
            .EANativeTransport  = 0,
            .MatchAction        = NO_ACTION
        },
        {
            .iOSAppIdentifier   = 2,
            .iOSAppName         = (U8 *)"jp.co.denso.NaviCon2",
            .EANativeTransport  = 0,
            .MatchAction        = NO_ACTION
        }
    };

    IPOD_PLAYER_ACC_INFO_CONFIGURATION g_AccInfo = 
    {
        .Name               = (U8 *)"Gen4 multi",
        .Hardware_version   = {  .Major_Number = 9,  .Minor_Number = 8,  .Revision_Number = 7   },
        .Software_version   = {  .Major_Number = 3,  .Minor_Number = 2,  .Revision_Number = 1   },
        .Hardware_version_iap2 = (U8 *)"ADITHardwareVersioniAP2",
        .Software_version_iap2 = (U8 *)"ADITSoftwareiVersioniAP2",
        .Manufacturer       = (U8 *)"Advanced Information Technology",
        .ModelNumber        = (U8 *)"model 1",
        .SerialNumber       = (U8 *)"1234567890",
        .VendorId           = (U8 *)"44311",
        .ProductId          = (U8 *)"1111",
        .BCDDevice          = (U8 *)"1",
        .ProductPlanUUID    = (U8 *)"123abc",
        .SupportedIOSInTheCar = 0,
        .SupportediOSAppCount = sizeof(g_iOSInfo) / sizeof(IPOD_PLAYER_IOS_APPINFO),
        .iOSAppInfo         = g_iOSInfo,
        .MsgSentByAcc       = &g_msgSentByAcc,
        .MsgRecvFromDevice  = &g_msgRecvFromDevice
    };
#endif /* IPOD_SET_ACC_INFO */

#ifdef IPOD_SET_DEV_INFO
IPOD_PLAYER_BT_MAC_ADDR g_btmac[3] = {
    {.addr = { 0x11, 0x12, 0x13, 0x14, 0x15, 0x16 }},
    {.addr = { 0x21, 0x22, 0x23, 0x24, 0x25, 0x26 }},
    {.addr = { 0x31, 0x32, 0x33, 0x34, 0x35, 0x36 }}
};

IPOD_PLAYER_DEV_INFO g_devinf = {
    .macCount = 3,
    .macAddr = g_btmac
};
#endif /* IPOD_SET_DEV_INFO */

#ifdef IPOD_SET_VEHICLE_INFO
IPOD_PLAYER_VEHICLE_INFO g_vehicleInfo = {
    .displayName = (U8 *)"DisplayName of ADIT"
};
#endif /* IPOD_SET_VEHICLE_INFO */

static DEV_DETECT_SERVER_CFG *g_detectCfg = NULL;
static S32 g_eventHandle = -1;

S32 dev_detect_deinit(DEV_DETECT_SERVER_CFG **cfg);

S32 dev_power_request(U16 vid, U16 pid)
{
    S32 ret = IPOD_PLAYER_ERROR;
    libusb_device_handle *device_handle;
    int res = -1;
    libusb_context *usb_context = NULL;
    uint8_t bmRequestType = IPOD_DEV_VEN_bmREQ;
    uint8_t bRequest = IPOD_DEV_VEN_REQ;
    uint16_t wValue   = IPOD_DEV_VEN_VAL;
    uint16_t wIndex   = IPOD_DEV_VEN_IX ;
    unsigned char *buffer   = NULL;
    uint16_t wLength  = 0;
    unsigned int timeout  = IPOD_CTRL_TRANS_TOUT;
    
    /* Initialize libusb */
    res = libusb_init(&usb_context);
    if((res != 0) || (usb_context == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, res);
        return IPOD_PLAYER_ERROR;
    }
    
    /* Open libusb with vendor id and product id */
    device_handle = libusb_open_device_with_vid_pid(usb_context, vid, pid);
    if(device_handle != NULL)
    {
        /* Send control transfer for power request */
        res = libusb_control_transfer(device_handle,
                                      bmRequestType,
                                      bRequest,
                                      wValue,
                                      wIndex,
                                      buffer,
                                      wLength,
                                      timeout);
        /* Transfer success */
        if(res == wLength)
        {
            ret = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, res);
            ret = IPOD_PLAYER_ERROR;
        }
        /* Close device */
        libusb_close(device_handle);
    }
    else
    {
        IPOD_ERR_WRITE_PTR(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERROR, device_handle);
        ret = IPOD_PLAYER_ERROR;
    }
    libusb_exit(usb_context);
    
    return ret;
}

static void dev_sigterm(S32 para)
{
    U32 i = 0;                                                      /* for retry loop    */
    S32 rc = IPOD_PLAYER_ERROR;                                     /* for return code   */
    static U32 running_flag = 0;                                    /* for check running */
    struct timespec waitTime = {0, IPOD_DEV_SEND_RETRY_WAIT_TIME};  /* for waite time    */
    
    /* for lint */
    para = para;
    
    /* is running */
    if(running_flag > 0)
    {
        /* leave the function immediately if running */
        return;
    }
    running_flag += 1;
    
    /* try to trigger event */
    for(i = 0; i < IPOD_DEV_SEND_RETRY_MAX_NUM; i++)
    {
        /* send end event */
        rc = eventfd_write(g_eventHandle, IPOD_DEV_END_EVENT);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_OK;
            break;
        }
        else
        {
            /* failed to send event */
            rc = IPOD_PLAYER_ERROR;
            if(errno == EINTR)
            {
                /* wait for 50 msec */
                nanosleep(&waitTime, NULL);
            }
            else
            {
                break;
            }
        }
    }
    
    /* failed to send end event */
    if(rc != IPOD_PLAYER_OK)
    {
        /* force to free resource */
        dev_detect_deinit(&g_detectCfg);
        
        /* close event fd */
        if(g_eventHandle >= 0)
        {
            close(g_eventHandle);
        }
        exit(rc);
    }
}

void dev_get_sound_info(DEV_DETECT_SERVER_CFG *cfg, const U8 *idVendor, const U8 *idProduct, U8 emptyNum)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct udev_enumerate *enumerate = NULL;
    struct udev_list_entry *devices = NULL;
    struct udev_list_entry *dev_list_entry = NULL;
    struct udev_device *dev = NULL;
    struct udev_device *parentDev = NULL;
    const char *devnode = NULL;
    const char *idVendor2 = NULL;
    const char *idProduct2 = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT);
    
    /* Check the parameter */
    if((cfg == NULL) || (idVendor == NULL) || (idProduct == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return;
    }
    
    if(cfg->udev == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return;
    }
    
    enumerate = udev_enumerate_new(cfg->udev);
    if(enumerate != NULL)
    {
        /* Add the subsystem to enumerate object */
        rc = udev_enumerate_add_match_subsystem(enumerate, IPOD_USB_SOUND_SUBSYSTEM);
        if(rc == 0)
        {
            /* Scan enumerate object */
            rc = udev_enumerate_scan_devices(enumerate);
            if(rc == 0)
            {
                /* Get the list of devices from enumerate object */
                devices = udev_enumerate_get_list_entry(enumerate);
                if(devices != NULL)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERROR);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
            rc = IPOD_PLAYER_ERROR;
        }
        
        if((devices != NULL) && (rc == IPOD_PLAYER_OK))
        {
            /* This is macro of libudev */
            udev_list_entry_foreach(dev_list_entry, devices)
            {
                const char *path = NULL;
                /* Previous information is cleared here */
                dev = NULL;
                devnode = NULL;
                idVendor2 = NULL;
                idProduct2 = NULL;
                parentDev = NULL;
                
                /* PRQA: Lint Message 774: udev_list_entry_foreach is the macro of libudev. this pointer is depended on this library.
                 * Therefore we must check the pointer whether dev_list_entry is NULL */
                if(dev_list_entry != NULL) /*lint !e774 */
                {
                    /* Get the path of device */
                    path = udev_list_entry_get_name(dev_list_entry);
                    if(path != NULL)
                    {
                        dev = udev_device_new_from_syspath(cfg->udev, path);
                    }
                }
                
                if(dev != NULL)
                {
                    devnode = udev_device_get_devnode(dev);
                    if(devnode != NULL)
                    {
                        parentDev = udev_device_get_parent_with_subsystem_devtype(dev, (const char *)IPOD_USB_SUBSYSTEM, (const char *)IPOD_USB_DEVTYPE);
                        if(parentDev != NULL)
                        {
                            idVendor2 = udev_device_get_sysattr_value(parentDev, IPOD_USB_ATTR_VENDOR);
                            idProduct2 = udev_device_get_sysattr_value(parentDev, IPOD_USB_ATTR_PRODUCT);
                            if((idVendor2 != NULL) && (idProduct2 != NULL))
                            {
                                if((strncmp((const char *)idVendor, (const char *)idVendor2, DEV_DETECT_DEVICE_LEN) == 0) &&
                                   (strncmp((const char *)idProduct, (const char *)idProduct2, DEV_DETECT_DEVICE_LEN) == 0))
                                {
                                    if(strncmp(DEV_DETECT_SOUND_NAME_PREFIX, (const char *)devnode, sizeof(DEV_DETECT_SOUND_NAME_PREFIX) - 1) == 0)
                                    {
                                        snprintf((char *)cfg->devInfo[emptyNum].audioInName, sizeof(cfg->devInfo->audioInName), DEV_DETECT_SOUND_HW_NAME,
                                                 devnode[sizeof(DEV_DETECT_SOUND_NAME_PREFIX)], devnode[sizeof(DEV_DETECT_SOUND_NAME_PREFIX) + DEV_DETECT_SOUND_HW_CARD_LEN]);
                                    }
                                }
                            }
                        }
                    }
                    udev_device_unref(dev);
                }
            }
        }
        udev_enumerate_unref(enumerate);
        enumerate = NULL;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERROR);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, 0);
}


S32 dev_get_device_info(DEV_DETECT_SERVER_CFG *cfg, const U8 *action)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct udev_enumerate *enumerate = NULL;
    struct udev_list_entry *devices = NULL;
    struct udev_list_entry *dev_list_entry = NULL;
    struct udev_device *dev = NULL;
    const char *idVendor = NULL;
    const char *idProduct = NULL;
    const char *serial = NULL;
    dev_t deviceAddr = 0;
    
    U8 count = 0;
    S32 emptyDev = -1;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT);
    
    if((cfg == NULL) || (action == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(cfg->udev == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Create new enumerate object */
    enumerate = udev_enumerate_new(cfg->udev);
    if(enumerate != NULL)
    {
        /* Add the subsystem to enumerate object */
        rc = udev_enumerate_add_match_subsystem(enumerate, (const char *)IPOD_USB_HID_SUBSYSTEM_TYPE);
        if(rc == 0)
        {
            /* Scan enumerate object */
            rc = udev_enumerate_scan_devices(enumerate);
            if(rc == 0)
            {
                /* Get the list of devices from enumerate object */
                devices = udev_enumerate_get_list_entry(enumerate);
                if(devices != NULL)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERROR);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
            rc = IPOD_PLAYER_ERROR;
        }
        
        if((devices != NULL) && (rc == IPOD_PLAYER_OK))
        {
            /* This is macro of libudev */
            udev_list_entry_foreach(dev_list_entry, devices)
            {
                const char *path = NULL;
                
                /* Previous information is cleared here */
                dev = NULL;
                idVendor = NULL;
                idProduct = NULL;
                serial = NULL;
                deviceAddr = 0;
                
                /* PRQA: Lint Message 774: udev_list_entry_foreach is the macro of libudev. this pointer is depended on this library.
                 * Therefore we must check the pointer whether dev_list_entry is NULL */
                if(dev_list_entry != NULL) /*lint !e774 */
                {
                    /* Get the path of device */
                    path = udev_list_entry_get_name(dev_list_entry);
                    if(path != NULL)
                    {
                        /* Get the device object from path */
                        dev = udev_device_new_from_syspath(cfg->udev, path);
                    }
                }
                
                if(dev != NULL)
                {
                    /* Get the device Vendor ID */
                    idVendor = udev_device_get_sysattr_value(dev, IPOD_USB_ATTR_VENDOR);
                    /* Get the device Product ID */
                    idProduct = udev_device_get_sysattr_value(dev, IPOD_USB_ATTR_PRODUCT);
                    /* Get the device serial name */
                    serial = udev_device_get_sysattr_value(dev, IPOD_USB_ATTR_SERIAL);
                    if((idVendor != NULL) && (idProduct != NULL) && (serial != NULL))
                    {
                        /* Vendor ID equal to Apple Vendor ID and Product ID equal to Apple Product ID */
                        if((strncmp((const char *)idVendor, (const char *)IPOD_APPLE_IDVENDOR, DEV_DETECT_VENDOR_MAX_LEN) == 0) && 
                           (strncmp((const char *)idProduct, (const char *)IPOD_APPLE_IDPRODUCT_MIN, 2) == 0))
                        {
                             sleep(1);
                            if(strncmp((const char *)action, IPOD_USB_ACTION_ADD, DEV_DETECT_ACTION_LEN) == 0)
                            {
                                emptyDev = -1;
                                /* Get the current connected device address */
                                deviceAddr = udev_device_get_devnum(dev);
                                for(count = 0; count < DEV_DETECT_DEVICE_MAX; count++)
                                {
                                    /* Detected device is found in the table */
                                    if(strncmp((const char *)cfg->devInfo[count].serialName, serial, DEV_DETECT_CFG_STRING_MAX) == 0)
                                    {
                                        /* Same serial name is connected but device address is diferrent. */
                                        /* It means that device is disconnected and immediately connected. */
                                        if(cfg->devInfo[count].deviceAddr != deviceAddr)
                                        {
                                            cfg->devInfo[count].deviceAddr = deviceAddr;
                                            cfg->devInfo[count].action = DEV_DETECT_ACTION_STATUS_RECONNECT;
                                        }
                                        emptyDev = -1;
                                        break;
                                    }
                                    else if(cfg->devInfo[count].serialName[0] == '\0')
                                    {
                                        /* First table number  */
                                        if(emptyDev == -1)
                                        {
                                            emptyDev = (S32)count;
                                        }
                                    }
                                }
                                
                                /* Empty table was found  */
                                if(emptyDev != -1)
                                {
                                    strncpy((char *)cfg->devInfo[emptyDev].serialName, serial, DEV_DETECT_CFG_STRING_MAX);
                                    cfg->devInfo[emptyDev].serialName[DEV_DETECT_CFG_STRING_MAX - 1] = '\0';
                                    cfg->devInfo[emptyDev].deviceAddr = deviceAddr;
                                    cfg->devInfo[emptyDev].action = DEV_DETECT_ACTION_STATUS_CONNECT;
                                    cfg->devInfo[emptyDev].idVendor = strtol(idVendor, NULL, IPOD_DEV_STR_TO_HEX);
                                    cfg->devInfo[emptyDev].idProduct = strtol(idProduct, NULL, IPOD_DEV_STR_TO_HEX);
                                    
                                    /* Get sound information */
                                    dev_get_sound_info(cfg, (const U8*)idVendor, (const U8 *)idProduct, (U8)emptyDev);
                                }
                            }
                            else
                            {
                                for(count = 0; count < DEV_DETECT_DEVICE_MAX; count++)
                                {
                                    if(strncmp((const char *)cfg->devInfo[count].serialName, (const char *)serial, DEV_DETECT_CFG_STRING_MAX) == 0)
                                    {
                                        cfg->devInfo[count].action = 1;
                                    }
                                }
                            }
                        }
                    }
                }
                
                if(dev != NULL)
                {
                    udev_device_unref(dev);
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        udev_enumerate_unref(enumerate);
        enumerate = NULL;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, rc);
    
    return rc;
}

void devCBDeviceDetection(U32 devID, S32 result)
{
    devID = devID;
    result = result;
}

S32 dev_read_udev(DEV_DETECT_SERVER_CFG *cfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct udev_device *udevice = NULL;
    const char *action = NULL;
    static IPOD_PLAYER_DEVICE_DETECTION_INFO info;
    U32 i = 0;
 
    memset(&info, 0, sizeof(info));
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT);
    
    if(cfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(cfg->moniter == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the device that recieved data */
    udevice = udev_monitor_receive_device(cfg->moniter);
    if (udevice != NULL) 
    {
        /* Get the device action. "add" or "remove" */
        action = udev_device_get_action(udevice);
        if(action != NULL)
        {
            /* Get the serial name */
            rc = dev_get_device_info(cfg, (const U8 *)action);
            
            for(i = 0; (i < DEV_DETECT_DEVICE_MAX) && (rc == IPOD_PLAYER_OK); i++)
            {
                /* Check the device Action. */
                if(strcmp((const char *)action, IPOD_USB_ACTION_ADD) == 0)
                {
                    strncpy((char *)info.devPath, (const char *)cfg->devInfo[i].serialName, sizeof(info.devPath));
                    info.devPath[sizeof(info.devPath) - 1] = '\0';
                    strncpy((char *)info.audioPath, (const char *)cfg->devInfo[i].audioInName, sizeof(info.audioPath));
                    info.audioPath[sizeof(info.audioPath) - 1] = '\0';
#ifdef IPOD_SET_ACC_INFO
                    info.accInfo = &g_AccInfo;
#else
                    info.accInfo = NULL;
#endif /* IPOD_SET_ACC_INFO */

#ifdef IPOD_SET_DEV_INFO
                    info.devInfo = &g_devinf;
#endif /* IPOD_SET_DEV_INFO */

#ifdef IPOD_SET_VEHICLE_INFO
                    info.vehicleInfo = &g_vehicleInfo;
#endif /* IPOD_SET_VEHICLE_INFO */

                    /* Action is "add" */
                    if(cfg->devInfo[i].action == DEV_DETECT_ACTION_STATUS_CONNECT)
                    {
                        /* This device information still not inform */
                        info.detectType = IPOD_PLAYER_DETECTION_TYPE_CONNECT;
                        rc = dev_power_request(cfg->devInfo[i].idVendor, cfg->devInfo[i].idProduct);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            rc = iPodPlayerSetDeviceDetection((i + 1), &info);
                        }
                    }
                    else if(cfg->devInfo[i].action == DEV_DETECT_ACTION_STATUS_RECONNECT)
                    {
                        info.detectType = IPOD_PLAYER_DETECTION_TYPE_DISCONNECT;
                        rc = iPodPlayerSetDeviceDetection((i + 1), &info);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            info.detectType = IPOD_PLAYER_DETECTION_TYPE_CONNECT;
                            rc = iPodPlayerSetDeviceDetection((i +1), &info);
                        }
                    }
                    else
                    {
                        /* Nothing to do */
                    }
                    
                    if(rc == IPOD_PLAYER_OK)
                    {
                        cfg->devInfo[i].action = 0;
                    }
                }
                else
                {
                    /* Action is "remove" */
                    if(cfg->devInfo[i].action == 0)
                    {
                        if(cfg->devInfo[i].serialName[0] != '\0')
                        {
                            /* This device information still not inform */
                            info.detectType = IPOD_PLAYER_DETECTION_TYPE_DISCONNECT;
                            strncpy((char *)info.devPath, (const char *)cfg->devInfo[i].serialName, sizeof(info.devPath));
                            info.devPath[sizeof(info.devPath) - 1] = '\0';
                            strncpy((char *)info.audioPath, (const char *)cfg->devInfo[i].audioInName, sizeof(info.audioPath));
                            info.audioPath[sizeof(info.audioPath) - 1] = '\0';
                            memset(&cfg->devInfo[i], 0, sizeof(cfg->devInfo[0]));
#ifdef IPOD_SET_ACC_INFO
                            info.accInfo = &g_AccInfo; 
#else
                            info.accInfo = NULL; 
#endif /* IPOD_SET_ACC_INFO */

#ifdef IPOD_SET_DEV_INFO
                            info.devInfo = &g_devinf;
#endif /* IPOD_SET_DEV_INFO */

#ifdef IPOD_SET_VEHICLE_INFO
                            info.vehicleInfo = &g_vehicleInfo;
#endif /* IPOD_SET_VEHICLE_INFO */

                            rc = iPodPlayerSetDeviceDetection((i + 1), &info);
                        }
                    }
                    cfg->devInfo[i].action = 0;
                }
            }
        }
        udev_device_unref(udevice);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, rc);
    
    return rc;
}

S32 devStart(DEV_DETECT_SERVER_CFG *cfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    static IPOD_PLAYER_DEVICE_DETECTION_INFO info;
    struct epoll_event event;
    U32 i = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT);
    
    if(cfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    memset(&event, 0, sizeof(event));
    
    /* Create the udev object */
    cfg->udev = udev_new();
    if (cfg->udev != NULL)
    {
        /* Link the monitor to "udev" devices */
        cfg->moniter = udev_monitor_new_from_netlink(cfg->udev, (const char *)IPOD_USB_MONITOR_LINK);
        if(cfg->moniter != NULL)
        {
            /* Filter only for get the "hiddev" devices */
            rc = udev_monitor_filter_add_match_subsystem_devtype(cfg->moniter, (const char *)IPOD_USB_MONITOR_DEVTYPE, IPOD_USB_FILTER_TYPE);
            if(rc == 0)
            {
                /* Start receiving */
                rc = udev_monitor_enable_receiving(cfg->moniter);
                if(rc == 0)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Set unknown descriptor */
        cfg->udevfd = -1;
        /* Get the file descriptor for the monitor. This fd is used by select() */
        cfg->udevfd = udev_monitor_get_fd(cfg->moniter);
        
        if(cfg->udevfd >= 0)
        {
            event.events = EPOLLIN;
            event.data.fd = cfg->udevfd;
            /* Add the uden descriptor to epoll */
            rc = epoll_ctl(cfg->waitHandle, EPOLL_CTL_ADD, cfg->udevfd, &event);
            if(rc == 0)
            {
                /* Increment the wait descriptor number */
                cfg->handleNum++;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, errno);
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = dev_get_device_info(cfg, (U8 *)IPOD_USB_ACTION_ADD);
        for(i = 0; (i < DEV_DETECT_DEVICE_MAX) && (rc == IPOD_PLAYER_OK); i++)
        {
            memset(&info, 0, sizeof(info));
            /* Action is "add" */
            /* Read the device property from connecting device */
            if(cfg->devInfo[i].action != 0)
            {
                info.detectType = IPOD_PLAYER_DETECTION_TYPE_CONNECT;
                strncpy((char *)info.devPath, (const char *)cfg->devInfo[i].serialName, sizeof(info.devPath));
                info.devPath[sizeof(info.devPath) - 1] = '\0';
                strncpy((char *)info.audioPath, (const char *)cfg->devInfo[i].audioInName, sizeof(info.audioPath));
                info.audioPath[sizeof(info.audioPath) - 1] = '\0';
                cfg->devInfo[i].action = 0;
#ifdef IPOD_SET_ACC_INFO
                info.accInfo = &g_AccInfo; 
#else
                info.accInfo = NULL;
#endif /* IPOD_SET_ACC_INFO */

#ifdef IPOD_SET_DEV_INFO
                info.devInfo = &g_devinf;
#endif /* IPOD_SET_DEV_INFO */

#ifdef IPOD_SET_VEHICLE_INFO
                info.vehicleInfo = &g_vehicleInfo;
#endif /* IPOD_SET_VEHICLE_INFO */
                
                rc = dev_power_request(cfg->devInfo[i].idVendor, cfg->devInfo[i].idProduct);
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = iPodPlayerSetDeviceDetection((i + 1), &info);
                }
            }
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, rc);
    
    return rc;
}

S32 devEnd(DEV_DETECT_SERVER_CFG *cfg)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT, rc);
    
    if(cfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(cfg->udev != NULL)
    {
        udev_unref(cfg->udev);
        cfg->udev = NULL;
    }
    
    if(cfg->moniter != NULL)
    {
        udev_monitor_unref(cfg->moniter);
        cfg->moniter = NULL;
    }

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, rc);
    return rc;

}

DEV_DETECT_SERVER_CFG *dev_detect_init(void)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_REGISTER_CB_TABLE table;
    DEV_DETECT_SERVER_CFG *cfg = NULL;
    DEV_DETECT_SERVER_CFG *retCfg = NULL;
    struct sigaction sig;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT);
    
    /* Initialize the structure */
    memset(&table, 0, sizeof(table));
    memset(&sig, 0, sizeof(sig));
    
    /* Ignore the signal of SIGPIPE */
    sig.sa_handler = SIG_IGN;
    sig.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sig, NULL);
    
    cfg = calloc(1, sizeof(DEV_DETECT_SERVER_CFG));
    if(cfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return NULL;
    }
    
    table.cbDeviceDetectionResult = devCBDeviceDetection;
    
    rc = epoll_create(DEV_DETECT_EPOLL_SIZE);
    if(rc > -1)
    {
        cfg->waitHandle = rc;
        rc = iPodPlayerInit(IPOD_PLAYER_USE_DEVICE_USB, &table);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = devStart(cfg);
            if(rc != IPOD_PLAYER_OK)
            {
                devEnd(cfg);
                iPodPlayerDeinit();
                close(cfg->waitHandle);
                cfg->waitHandle = -1;
            }
        }
        else
        {
            close(cfg->waitHandle);
            cfg->waitHandle = -1;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, errno);
        cfg->waitHandle = -1;
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        retCfg = cfg;
    }
    else
    {
      free(cfg);
      retCfg = NULL;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, rc);
    
    return retCfg;
}

S32 dev_detect_deinit(DEV_DETECT_SERVER_CFG **cfg)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_DEVDETECT);
    
    if(cfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(*cfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    devEnd(*cfg);
    
    iPodPlayerDeinit();
    
    if((*cfg)->waitHandle >= 0)
    {
        close((*cfg)->waitHandle);
        (*cfg)->waitHandle = -1;
    }
    
    if(*cfg != NULL)
    {
        free(*cfg);
        *cfg = NULL;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_DEVDETECT, rc);
    
    return rc;
}



int main(int argc, char *argv[])
{
    S32 rc = IPOD_PLAYER_ERROR;
    DEV_DETECT_SERVER_CFG *detectCfg = NULL;
    struct sigaction sig;
    struct epoll_event events[DEV_DETECT_MAX_HANDLE];
    S32 i = 0;
    U32 endFlag = 0;
    struct timespec waitTime = {0, DEV_DETECT_WAIT_TIME};
    S32 event_handl = -1;                               /* for event handle */
    eventfd_t value = 0;                                /* for event value  */
    struct epoll_event event;

    /* Initialize the structure */
    memset(&sig, 0, sizeof(sig));
    memset(&events, 0, sizeof(events));
    memset(&event, 0, sizeof(event));

    /* set signal handle */
    sig.sa_handler = dev_sigterm;
    sigaction(SIGTERM, &sig, NULL);
    sig.sa_handler = dev_sigterm;
    sigaction(SIGINT, &sig, NULL);
    
    
    /* For lint */
    argc = argc;
    argv = argv;
    
    detectCfg = dev_detect_init();
    if(detectCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, IPOD_PLAYER_ERROR);
        return rc;
    }
    
    /* initialize event */
    event_handl = eventfd(0, EFD_NONBLOCK);
    if(event_handl >= 0)
    {
        event.events = EPOLLIN;
        event.data.fd = event_handl;
        /* Add the uden descriptor to epoll */
        rc = epoll_ctl(detectCfg->waitHandle, EPOLL_CTL_ADD, event_handl, &event);
        if(rc == 0)
        {
            /* set global parameter */
            g_eventHandle = event_handl;
            /* Increment the wait descriptor number */
            detectCfg->handleNum++;
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, errno);
            endFlag = 1;
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        /* failed to init event */
        endFlag = 1;
        rc = IPOD_PLAYER_ERROR;
    }
    
    g_detectCfg = detectCfg;
    while(endFlag == 0)
    {
        rc = epoll_wait(detectCfg->waitHandle, events, detectCfg->handleNum, -1);
        if(rc > 0)
        {
            for(i = 0; i < rc; i++)
            {
                if(events[i].data.fd == detectCfg->udevfd)
                {
                    dev_read_udev(detectCfg);
                }
                else if(events[i].data.fd == g_eventHandle)
                {
                    /* read event */
                    eventfd_read(g_eventHandle, &value);
                    if(value >= IPOD_DEV_END_EVENT)
                    {
                        /* end loop */
                        endFlag = 1;
                        break;
                    }
                }
                else
                {
                    /* check next fd */
                }
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_DEVDETECT, rc);
            /* Some system error may be occurred. Wait 100ms and retry wait */
            nanosleep(&waitTime, NULL);
        }
        
    }
    
    /* close event fd */
    if(event_handl >= 0)
    {
        epoll_ctl(detectCfg->waitHandle, EPOLL_CTL_DEL, event_handl, NULL);
        close(event_handl);
        g_eventHandle = -1;
    }

    dev_detect_deinit(&detectCfg);
   
    return rc;
}

