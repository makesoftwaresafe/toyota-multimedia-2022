/**
* \file: iap2_usb_role_switch_common.h
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

#ifndef IAP2_USB_ROLE_SWITCH_COMMON_H
#define IAP2_USB_ROLE_SWITCH_COMMON_H

#include "iap2_dlt_log.h"
#include "iap2_usb_role_switch.h"
#include "iap2_usb_role_switch_config.h"

#define IAP2_USB_EXPORTED_SYMBOL __attribute__((visibility("default")))
#define IAP2_USB_HIDDEN_SYMBOL __attribute__((visibility("hidden")))


#define IAP2_SYS_MAX_PATH           256

typedef enum {
    IAP2_FS_READ = 0,
    IAP2_FS_WRITE
} IAP2_FS_ACCESS;


IAP2_USB_HIDDEN_SYMBOL BOOL iAP2UsbRoleSwitchCommon_Exists(const char* name);
IAP2_USB_HIDDEN_SYMBOL BOOL iAP2UsbRoleSwitchCommon_Write(const char* path, const char* subPath,
        const char* value, BOOL checkBeforeWrite);

IAP2_USB_HIDDEN_SYMBOL BOOL iAP2UsbRoleSwitchCommon_WriteValue(const char* path,
        const char* value, size_t len, BOOL checkBeforeWrite);

#endif /* IAP2_USB_ROLE_SWITCH_COMMON_H */
