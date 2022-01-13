/**
* \file: iap2_usb_vendor_request_monitor.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* <brief description>.
* <detailed description>
* \component: iAP2 USB Role Switch
*
* \author: J. Harder / ADIT/SW1 / jharder@de.adit-jv.com
*
* \copyright (c) 2013 Advanced Driver Information Technology.
* This code is developed by Advanced Driver Information Technology.
* Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
* All rights reserved.
*
* \see <related items>
*
* \history
*
***********************************************************************/

#include "iap2_usb_vendor_request.h"
#include <memory.h>
#include <libudev.h>
#include <errno.h>
#include <stdio.h>

#define IAP2_USB_ROLE_SWITCH_UDEV_SOURCE        "udev"
#define IAP2_USB_ROLE_SWITCH_UDEV_ACTION        "remove"
#define IAP2_USB_ADD_UDEV_ACTION                "add"


/* private functions */
static iAP2USBRoleSwitchStatus _findDevice(iAP2USBVendorRequestMonitor* thisPtr, U16 vid, U16 pid,
        const char* serial,  BOOL isMultiHostHub);

static iAP2USBRoleSwitchStatus _compareSysPath(struct udev_device* device, const char* sysPath,
        BOOL* matches /* out */);
static iAP2USBRoleSwitchStatus _compareDeviceIds(iAP2USBVendorRequestMonitor* thisPtr,
        struct udev_device* device, const char* path, U16 vid, U16 pid, const char* serial,
        BOOL* found);

static iAP2USBRoleSwitchStatus _compareHubIds(iAP2USBVendorRequestMonitor* thisPtr,
        struct udev_device* device, const char* path, U16 vid, U16 pid, BOOL* found);

static iAP2USBRoleSwitchStatus _checkForUdevAction(iAP2USBVendorRequestMonitor* thisPtr,
        BOOL* deviceFound /* out */);

iAP2USBRoleSwitchStatus iAP2USBVendorRequestMonitor_Begin(iAP2USBVendorRequestMonitor* thisPtr,
        U16 vid, U16 pid, const char* serial,  BOOL isMultiHostHub)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

    memset(thisPtr, 0, sizeof(iAP2USBVendorRequestMonitor));

    thisPtr->udev = udev_new();
    if (thisPtr->udev == NULL)
    {
        /* TODO error handling */
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        status = _findDevice(thisPtr, vid, pid, serial, isMultiHostHub);
        /* error logged and handled */
    }

    return status;
}

iAP2USBRoleSwitchStatus iAP2USBVendorRequestMonitor_WaitAndEnd(iAP2USBVendorRequestMonitor* thisPtr)
{
    struct timeval timeout;
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

    timeout.tv_sec = IAP2_USB_VENDOR_REQUEST_TIMEOUT_SEC;
    timeout.tv_usec = IAP2_USB_VENDOR_REQUEST_TIMEOUT_USEC;

    thisPtr->monitor = udev_monitor_new_from_netlink(thisPtr->udev,
            IAP2_USB_ROLE_SWITCH_UDEV_SOURCE);
    /* TODO error handling */

    udev_monitor_filter_add_match_subsystem_devtype(thisPtr->monitor, "usb", "usb_device");
    /* TODO error handling */

    udev_monitor_enable_receiving(thisPtr->monitor);
    /* TODO error handling */

    int file = udev_monitor_get_fd(thisPtr->monitor);
    /* TODO error handling */

    BOOL deviceFound = FALSE;
    while (status == IAP2_USB_ROLE_SWITCH_OK && deviceFound == FALSE &&
            (timeout.tv_sec > 0 || timeout.tv_usec > 0))
    {
        fd_set fileSet;

        /* PRQA: Lint Message 529: Symbol not subsequently referenced. */
        FD_ZERO(&fileSet);    /*lint !e529 */
        /* PRQA: Lint Message 530: Symbol fileSet not initialized. */
        FD_SET(file, &fileSet);    /*lint !e530 */

        /* wait; timeout is modified to the remaining time */
        int ret = select(file + 1, &fileSet, NULL, NULL, &timeout);
        if (ret > 0)
        {
            /* got some notification */
            if (FD_ISSET(file, &fileSet))
            {
                status = _checkForUdevAction(thisPtr, &deviceFound);
            }
        }
        else if (ret == 0)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN,
                    "vendor request to device vid:%04x pid:%04x serial:%s timed out",
                    thisPtr->deviceInfo.idVendor, thisPtr->deviceInfo.idProduct,
                    thisPtr->deviceInfo.serial);
            status = IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_TIMEOUT;
        }
        else if (ret == EINTR)
        {
            /* interrupt, try again */
            /* TODO need counter for max retries? */
        }
        else
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "select: %d %s", errno, strerror(errno));
            status = IAP2_USB_ROLE_SWITCH_FAILED;
        }
    }

    if (deviceFound == TRUE)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_DEBUG,
                "device disconnected after vendor request VID=%04x PID=%04x serial=%s",
                thisPtr->deviceInfo.idVendor, thisPtr->deviceInfo.idProduct,
                thisPtr->deviceInfo.serial);
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_DEBUG,
                "Hub vendor request VID=%04x PID=%04x ",
                thisPtr->hubInfo.idVendor, thisPtr->hubInfo.idProduct);
    }

    return status;
}

void iAP2USBVendorRequestMonitor_Release(iAP2USBVendorRequestMonitor* thisPtr)
{
    if (thisPtr->monitor != NULL)
    {
        udev_monitor_unref(thisPtr->monitor);
    }

    if (thisPtr->udev != NULL)
    {
        udev_unref(thisPtr->udev);
    }
}

static iAP2USBRoleSwitchStatus _findDevice(iAP2USBVendorRequestMonitor* thisPtr, U16 vid, U16 pid,
        const char* serial,  BOOL isMultiHostHub)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;
    struct udev_enumerate* enumerate;
    char udev_vid[5];

    /* scan hidraw devices */
    enumerate = udev_enumerate_new(thisPtr->udev);
    if (enumerate == NULL)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "udev_enumerate_new");
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        int ret;
        if (0 > (ret = udev_enumerate_add_match_subsystem(enumerate, "usb")))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "udev_enumerate_add_match_subsystem failed: %d", ret);
            status = IAP2_USB_ROLE_SWITCH_FAILED;
        }

        /* search only for Apple devices */
        sprintf(udev_vid, "%04x", vid);
        if (0 > (ret = udev_enumerate_add_match_sysattr(enumerate, (const char*)"idVendor", (const char*)udev_vid)))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "udev_enumerate_add_match_sysattr failed: %d", ret);
            status = IAP2_USB_ROLE_SWITCH_FAILED;
        }

        if (0 > (ret = udev_enumerate_scan_devices(enumerate)))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "udev_enumerate_scan_devices failed: %d", ret);
            status = IAP2_USB_ROLE_SWITCH_FAILED;
        }
    }

    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        BOOL found = FALSE;
        struct udev_list_entry* deviceIterator = udev_enumerate_get_list_entry(enumerate);
        if (deviceIterator == NULL)
        {
            /* happens when no device was found */
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "_findDevice():  enumerate list entry failed.");
        }

        while ((deviceIterator != NULL) && (found == FALSE))
        {
            const char* path = udev_list_entry_get_name(deviceIterator);
            if(path != NULL)
            {
                struct udev_device* device = udev_device_new_from_syspath(thisPtr->udev, path);
                if (device != NULL)
                {
                    if(isMultiHostHub)
                    {
                        _compareHubIds(thisPtr, device, path, vid, pid, &found);
                    }
                    else
                    {
                        _compareDeviceIds(thisPtr, device, path, vid, pid, serial, &found);
                    }
                    /* error logged and handled */
                    udev_device_unref(device);
                }
                else
                {
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "could not get udev device from syspath %s",
                            path);
                    /* ignore error, handle next */
                }
            }
            else
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "could not get udev path from deviceIterator");
            }

            deviceIterator = udev_list_entry_get_next(deviceIterator);
        }

        if (found == FALSE)
        {
            /* device not found */
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "_findDevice():  USB device with VID=%04x, PID=%04x and " \
                    "serial=%s not found", vid, pid, serial);
            status = IAP2_USB_ROLE_SWITCH_DEVICE_NOT_FOUND;
        }
    }

    if (enumerate != NULL)
    {
        udev_enumerate_unref(enumerate);
    }

    return status;
}

static iAP2USBRoleSwitchStatus _compareSysPath(struct udev_device* device, const char* sysPath,
        BOOL* matches /* out */)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;
    const char* path = udev_device_get_syspath(device);
    if (path == NULL)
    {
        /* TODO error handling */
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }
    else
    {
        if (0 == strncmp(sysPath, path, strlen(sysPath)))
        {
            *matches = TRUE;
        }
        else
        {
            *matches = FALSE;
        }
    }

    return status;
}

static iAP2USBRoleSwitchStatus _compareHubIds(iAP2USBVendorRequestMonitor* thisPtr,
        struct udev_device* device, const char* path, U16 vid, U16 pid, BOOL* found)
{

    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

    /* check vid, pid, serial */
    const char* device_vid = udev_device_get_sysattr_value(device, "idVendor");
    const char* device_pid = udev_device_get_sysattr_value(device, "idProduct");
    if ((device_vid != NULL) && (device_pid != NULL))
    {
        /* convert ids to string for comparison */
        char _vid[5], _pid[5];
        sprintf(_vid, "%04x", vid);
        sprintf(_pid, "%04x", pid);

        int vid_cmp = -1;
        int pid_cmp = -1;
        vid_cmp = strncmp(_vid, device_vid, 4);
        pid_cmp = strncmp(_pid, device_pid, 2);

        if ((0 == vid_cmp) && (0 == pid_cmp))
        {
            /* found the device; fill the structure */
            *found = TRUE;
            thisPtr->hubInfo.idVendor = vid;
            thisPtr->hubInfo.idProduct = pid;
            strncpy(thisPtr->hubInfo.sysPath, path, IAP2_SYS_MAX_PATH);
        }
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "failed to get device attributes: %s %s", device_vid,
                device_pid);
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    return status;
}

static iAP2USBRoleSwitchStatus _compareDeviceIds(iAP2USBVendorRequestMonitor* thisPtr,
        struct udev_device* device, const char* path, U16 vid, U16 pid, const char* serial,
        BOOL* found)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

    /* check vid, pid, serial */
    const char* device_vid = udev_device_get_sysattr_value(device, "idVendor");
    const char* device_pid = udev_device_get_sysattr_value(device, "idProduct");
    const char* device_serial = udev_device_get_sysattr_value(device, "serial");
    if ((device_vid != NULL) && (device_pid != NULL) && (device_serial != NULL))
    {
        /* convert ids to string for comparison */
        char _vid[5], _pid[5];
        sprintf(_vid, "%04x", vid);
        sprintf(_pid, "%04x", pid);

        int vid_cmp = -1;
        int pid_cmp = -1;
        int serial_cmp = -1;
        vid_cmp = strncmp(_vid, device_vid, 4);
        pid_cmp = strncmp(_pid, device_pid, 2);
        if (serial != NULL){
            serial_cmp = strncmp(serial, device_serial, strlen(serial));
        } else{
            serial_cmp = 0;
        }

        if ((0 == vid_cmp) && (0 == pid_cmp) && (serial_cmp == 0))
        {
            /* found the device; fill the structure */
            *found = TRUE;
            thisPtr->deviceInfo.idVendor = vid;
            thisPtr->deviceInfo.idProduct = pid;
            strncpy(thisPtr->deviceInfo.sysPath, path, IAP2_SYS_MAX_PATH);
            strncpy(thisPtr->deviceInfo.serial, device_serial,
                    IAP2_USB_MAX_STRING_DESCRIPTOR_LENGTH);
        }
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "failed to get device attributes: %s %s %s", device_vid,
                device_pid, device_serial);
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    return status;
}

static iAP2USBRoleSwitchStatus _checkForUdevAction(iAP2USBVendorRequestMonitor* thisPtr,
        BOOL* deviceFound)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;
    struct udev_device* device = udev_monitor_receive_device(thisPtr->monitor);

    if (device != NULL)
    {
        /* check if there is any udev action */
        const char* action = udev_device_get_action(device);
        if (action != NULL)
        {
            if (0 == strncmp(action, IAP2_USB_ROLE_SWITCH_UDEV_ACTION,
                    strlen(IAP2_USB_ROLE_SWITCH_UDEV_ACTION)))
            {
                if(thisPtr->hubInfo.idVendor == MOLEX_HUB_VID)
                {
                    status = _compareSysPath(device, thisPtr->hubInfo.sysPath, deviceFound);
                }
                else
                {
                    status = _compareSysPath(device, thisPtr->deviceInfo.sysPath, deviceFound);
                }
                /* error logged and handled */
            }
            else if(0 == strncmp(action, IAP2_USB_ADD_UDEV_ACTION,
                    strlen(IAP2_USB_ADD_UDEV_ACTION)))
            {
                if(thisPtr->hubInfo.idVendor == MOLEX_HUB_VID)
                {
                    status = _compareSysPath(device, thisPtr->hubInfo.sysPath, deviceFound);
                }
            }

        }
        else
        {
            /* TODO more detailed error */
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "udev_device_get_action failed");
            status = IAP2_USB_ROLE_SWITCH_FAILED;
        }
        udev_device_unref(device);
    }
    else
    {
        /* TODO more detailed error */
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "udev_monitor_receive_device failed");
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    return status;
}

