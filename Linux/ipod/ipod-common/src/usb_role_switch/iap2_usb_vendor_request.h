/**
* \file: iap2_usb_vendor_request.h
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

#ifndef IAP2_USB_VENDOR_REQUEST_H
#define IAP2_USB_VENDOR_REQUEST_H

#include "iap2_usb_role_switch_common.h"

#define IAP2_USB_MAX_STRING_DESCRIPTOR_LENGTH              255

#define WVALUE_CMD          0x0000
#define SWITCH_ENABLE       1
#define SWITCH_DISABLE      0
#define MOLEX_HUB_VID       0x0424
#define MOLEX_HUB_PID       0x4940
#define MOLEX_MHR_PID       0x4910
#define APPLE_DEV_VID       0x05ac
#define APPLE_DEV_PID       0x12a8
#define SET_ROLE_SWITCH     0x90
#define VENDOR_CLASS_REQ    0x41
#define EA_ENABLE           1
#define ENUM_TIMEOUT        0b100 /* 1 second */
#define LIBUSB_CONTROL_TIMEOUT 5000

typedef struct
{
    U16 idVendor;
    U16 idProduct;
    char serial[IAP2_USB_MAX_STRING_DESCRIPTOR_LENGTH];
    char sysPath[IAP2_SYS_MAX_PATH];
} iAP2USBDeviceInfo;

typedef struct
{
    U16 idVendor;
    U16 idProduct;
    char sysPath[IAP2_SYS_MAX_PATH];
} iAP2USBHubInfo;


typedef struct
{
    struct udev* udev;
    struct udev_monitor* monitor;
    iAP2USBDeviceInfo deviceInfo;
    iAP2USBHubInfo hubInfo;
} iAP2USBVendorRequestMonitor;


IAP2_USB_HIDDEN_SYMBOL iAP2USBRoleSwitchStatus iAP2USBVendorRequestMonitor_Begin(
        iAP2USBVendorRequestMonitor* monitor, U16 vid, U16 pid, const char* serial, BOOL isMultiHostHub);
IAP2_USB_HIDDEN_SYMBOL iAP2USBRoleSwitchStatus iAP2USBVendorRequestMonitor_WaitAndEnd(
        iAP2USBVendorRequestMonitor* thisPtr);
IAP2_USB_HIDDEN_SYMBOL void iAP2USBVendorRequestMonitor_Release(iAP2USBVendorRequestMonitor* thisPtr);

#endif /* IAP2_USB_VENDOR_REQUEST_H */
