#include <adit_typedef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libudev.h>
#include <errno.h>
#include "device_check.h"
#include "iap2_usb_role_switch.h"

S32 CheckForDev(struct udev *udev, CheckDeviceInformation *devInfo, int *devNum)
{
    S32 ret = -1;
    struct udev_enumerate   *enumerate;
    struct udev_list_entry  *devices, *dev_list_entry;
    struct udev_device      *dev;
    int devnum = 0;

    *devNum = 0;
    if(udev == NULL || devInfo == NULL)
    {
        return ret;
    }
    
    enumerate = udev_enumerate_new(udev);
    if(enumerate == NULL)
    {
        printf("enumerate is NULL \n");
        return ret;
    }

    if((ret = udev_enumerate_add_match_subsystem(enumerate, (const char *)"usb")) != 0)
    {
        printf("enumerate_add_match_subsystem failed %d \n", ret);
        return ret;
    }

    if((ret = udev_enumerate_add_match_sysattr(enumerate, SYSATTR_IDVENDOR, APPLE_VENDOR_ID)) != 0)
    {
        printf("enumerate_add_match_sysattr failed %d \n", ret);
        return ret;
    }

    if((ret = udev_enumerate_scan_devices(enumerate)) != 0)
    {
        printf("enumerate_scan_devices failed %d \n", ret);
        return ret;
    }

    devices = udev_enumerate_get_list_entry(enumerate);
    if(devices == NULL)
    {
        ret = -1;   /* no specified Apple device available */
    }
    else
    {
        ret = -1;
        udev_list_entry_foreach(dev_list_entry, devices)
        {
            const char *path;

            path = udev_list_entry_get_name(dev_list_entry);
            if(path == NULL)
            {
                printf("path of device failed \n");
            }
            else
            {
                dev = udev_device_new_from_syspath(udev, path);
                if(dev != NULL)
                {
                    const char *vendor  = udev_device_get_sysattr_value(dev, "idVendor");
                    const char *product = udev_device_get_sysattr_value(dev, "idProduct");
                    const char *serial  = udev_device_get_sysattr_value(dev, "serial");

                    if( (vendor != NULL) && (product != NULL) && (serial != NULL) )
                    {
                        if((strncmp((const char *)vendor, (const char *)APPLE_VENDOR_ID, DEV_DETECT_VENDOR_MAX_LEN) == 0)
                        && (strncmp((const char *)product, (const char *)APPLE_PRODUCT_ID, 2) == 0))
                        {
                            strncpy((char*)(devInfo[devnum].serial), serial, DEV_DETECT_CFG_STRING_MAX);
                            strncpy((char*)(devInfo[devnum].udevPath), path, DEV_DETECT_CFG_STRING_MAX);
                            strncpy((char*)(devInfo[devnum].product), product, DEV_DETECT_VENDOR_MAX_LEN);
                            strncpy((char*)(devInfo[devnum].vendor), vendor, DEV_DETECT_VENDOR_MAX_LEN);
                            ret = 0;
                            devnum++;
                        }
                    }
                    else
                    {
                        printf("device_get_sysattr_value(idVendor or idProduct or serial) is NULL \n");
                    }
                    udev_device_unref(dev);
                }
                else
                {
                    printf("device_new_from_syspath failed \n");
                }
            }
            if(devnum >= DEV_DETECT_DEVICE_NUM_MAX)
            {
                /* Leave from loop of udev_list_entry_foreach */
                break;
            }
        }
    }
    udev_enumerate_unref(enumerate); /* Free the enumerator object */

    *devNum = devnum;

    return ret;
}
int detection_wait(S32 udevfd, struct udev_monitor *monitor)
{
    int ret = DEVICE_CHK_ERR;
    int rc = 0;
    fd_set fds;
    struct timeval tv;
    struct udev_device *dev;
    const char *action = NULL;

/*PRQA: Lint Message 530,529: Lint can't check inline assembler on the C language standard macro. */
/*lint -save -e529 -e530 */
    FD_ZERO(&fds);
    FD_SET(udevfd, &fds);
/*lint -restore */
    tv.tv_sec = UDEV_WAIT_TIMEOUT;
    tv.tv_usec = 0;
 
    rc = select(udevfd + 1, &fds, NULL, NULL, &tv);
    if (rc > 0 && FD_ISSET(udevfd, &fds))
    {
        dev = udev_monitor_receive_device(monitor);
        if (dev)
        {
            action = udev_device_get_action(dev);
            if(strncmp((const char *)action, "add", 4) == 0)
            {
                ret = DEVICE_CHK_OK;
            }
            udev_device_unref(dev);
        }
        else
        {
            printf("No iPod ");
        }
    }
    else if(rc < 0)
    {
        printf("select() failed");
    }
    else
    {
        /* this occurs if an iPod is already connected */
    }

    return ret;
}

int device_detection(CheckDeviceInformation *devInfo, int *devNum)
{
    struct udev *udev;
    struct udev_monitor *monitor;
    S32 udevfd = -1;
    int ret = DEVICE_CHK_ERR;
    int retryCount = 0;

    udev = udev_new();
    if (!udev) /* udev will be created */
    {
        printf("Could not create resource of udev\n");
    }
    else
    {
        /* check USB port for connected device already. */
        if((ret = CheckForDev(udev, devInfo, devNum)) != 0)
        {
            monitor = udev_monitor_new_from_netlink(udev, (const char *)"udev");
            if(monitor != NULL)
            {
                if(udev_monitor_filter_add_match_subsystem_devtype(monitor, (const char *)"usb", "usb_device") == 0)
                {
                    if(udev_monitor_enable_receiving(monitor) == 0)
                    {
                        udevfd = udev_monitor_get_fd(monitor);
                        fprintf(stderr, "\nPlease connect device!\n");

                        while((ret == DEVICE_CHK_ERR) && (retryCount < USB_SELECT_RETRY_CNT))
                        {
                            retryCount++;
                            ret = detection_wait(udevfd, monitor);
                        }

                        if(ret == DEVICE_CHK_ERR)
                        {
                            printf("Apple device did not connect.(retry count = %d return = %d)\n", retryCount, ret);
                        }
                        else /* IPOD_OK */
                        {
                            if(CheckForDev(udev, devInfo, devNum) != 0)
                            {
                                printf("Error: enumerate connected iPod failed | ret = %d \n", ret);
                            }
                        }
                    }
                }
                udev_monitor_unref(monitor);
            }
        }
    }

    udev_unref(udev);

    return ret;
}

int USBSwitchOffOn(CheckDeviceInformation *devInfo)
{
    usbConnectStateType_t type;
    iAP2USBRoleSwitchInfo info;
    udcParamInfo_t udcParam;
    int ret = DEVICE_CHK_OK;

    memset(&udcParam, 0, sizeof(udcParamInfo_t));

    printf(" Detect device:\n");
    printf("    Information  : VID: %s  PID: %s  SERIAL: %s \n", (char *)devInfo->vendor, (char *)devInfo->product, (char *)devInfo->serial);
    printf("    UdevPath     : %s \n", (char *)devInfo->udevPath);

    /* Identify the USB Connection type of Apple device */
    type = getConnectStateType((char *)devInfo->udevPath, &udcParam);

    switch(type)
    {
        case UNWIRED_HUB_CONNECTED:
        {
            printf("INFO: Device Connected to Unwired Hub.\n");
            switchBridgePower(udcParam.pUtBridgePath, udcParam.pDevicePortNum, FALSE);
            sleep(1);
            switchBridgePower(udcParam.pUtBridgePath, udcParam.pDevicePortNum, TRUE);
            sleep(1);
            break;
        }
        case OTG_CONNECTED:
        {
            printf("INFO: Device Connected to Native OTG port. \n");
            iap2FindVbus(&info);
            iAP2USBPower_Switch(info.vbusPower, "off");
            sleep(1);
            iAP2USBPower_Switch(info.vbusPower, "on");
            sleep(1);
            break;
        }
        default:
        {
            printf("ERROR: Apple device is not connected to OTG Port.\n");
            ret = DEVICE_CHK_ERR;
            break;
        }
    }

    return ret;
}

int main(int argc, char *argv[])
{
    int rc = 0;
    int ret = DEVICE_CHK_OK;
    int count = 0;
    int devnum = 0;

    /* For lint */
    argc = argc;
    argv = argv;

    CheckDeviceInformation *devInfo = (CheckDeviceInformation*)calloc(DEV_DETECT_DEVICE_NUM_MAX, sizeof(CheckDeviceInformation));
    if(devInfo == NULL)
    {
        printf("could not allocate resource of device information \n");
    }
    else
    {
        if((ret = device_detection(devInfo, &devnum)) == DEVICE_CHK_OK)
        {
            for(count = 0; devnum > count; count++)
            {
                if((rc = USBSwitchOffOn(&(devInfo[count]))) != DEVICE_CHK_OK)
                {
                    /* Leave loop for checking of detected device. */
                }
            }
        }

        free(devInfo);
    }

    return ret;
}

