/**
* \file: iap2_usb_power.c
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

#include "iap2_usb_power.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/file.h>
#include <string.h>
#include <unistd.h>

#define IAP2_USB_GPIO_OTG_PATH              "/sys/class/gpio"
#define IAP2_USB_GPIO_OTG_EXPORT            "export"
#define IAP2_USB_GPIO_OTG_VALUE             "value"
#define IAP2_USB_GPIO_OTG_DIRECTION         "direction"
#define IAP2_USB_GPIO_OTG_OUT               "out"
#define IAP2_USB_GPIO_OTG_PWR_ON            "1"
#define IAP2_USB_GPIO_OTG_PWR_OFF           "0"

/* private functions */
static iAP2USBRoleSwitchStatus _switch(const S32 powerGPIO, const char* value);
static iAP2USBRoleSwitchStatus _exportGPIO(const S32 powerGPIO, char* path);


iAP2USBRoleSwitchStatus iAP2USBPower_SwitchOn(const S32 powerGPIO)
{
    return _switch(powerGPIO, IAP2_USB_GPIO_OTG_PWR_ON);
}

iAP2USBRoleSwitchStatus iAP2USBPower_SwitchOff(const S32 powerGPIO)
{
    iAP2USBRoleSwitchStatus status = _switch(powerGPIO, IAP2_USB_GPIO_OTG_PWR_OFF);
    /* status already handled */

    /* TODO do we want to un-export the GPIO ? */
    /* do not un-export GPIO during system life time */

    return status;
}

iAP2USBRoleSwitchStatus iAP2USBPower_Switch(const char* path, const char* value)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

#ifdef IPOD_ARCH_ARM

    if (TRUE == iAP2UsbRoleSwitchCommon_Exists(path))
    {
        if (TRUE != iAP2UsbRoleSwitchCommon_WriteValue(path, value, 0, FALSE))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "switch USB power %s to %s failed",
                                    path, value);
            status = IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED;
        }
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "path %s does not exist", path);
    }

#elif defined(IPOD_ARCH_ARM64)
    if (strcmp(value, "on") == 0)
    {
        status = iAP2USBPower_SwitchOn(IAP2_VBUS_POWER_GPIO);
    }
    else
    {
        status = iAP2USBPower_SwitchOff(IAP2_VBUS_POWER_GPIO);
    }

    if (status != IAP2_USB_ROLE_SWITCH_OK)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "switch USB power %s to %s failed", path, value);
    }

#else
    char offData[] = {0x0, 0x1, 0x0, 0x0};
    char onData[]  = {0x0, 0x1, 0x0, 0x1};
    char *modeValue = NULL;
    size_t len = 0;

    if(NULL == path)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "switch USB power: Illegal argument\n");
        status = IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED;
    }
    
    if (strcmp(value, "on") == 0){
        modeValue = onData;
        len = sizeof(onData);
    } else {
        modeValue = offData;
        len = sizeof(offData);
    }
    
    if (!iAP2UsbRoleSwitchCommon_WriteValue(path, modeValue, len, FALSE))
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "switch USB power:  set USB power failed \n");
        status = IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED;
    }

#endif /*IPOD_ARCH_ARM */

    return status;
}


/* ====== private functions ====== */

static iAP2USBRoleSwitchStatus _switch(const S32 powerGPIO, const char* value)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;
    char gpioPath[IAP2_SYS_MAX_PATH];

    /* export GPIO if it does not exists */
    status = _exportGPIO(powerGPIO, gpioPath);
    /* status already handled */

    /* set the write direction */
    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        /* depending on a kernel patch, the directory "direction" exists or not */
        char valuePath[IAP2_SYS_MAX_PATH];
        int ret = snprintf(valuePath, IAP2_SYS_MAX_PATH, "%s/%s", gpioPath, IAP2_USB_GPIO_OTG_DIRECTION);
        if (ret >= 0 && ret < IAP2_SYS_MAX_PATH)
        {
            if (TRUE == iAP2UsbRoleSwitchCommon_Exists(valuePath))
            {
                if (TRUE != iAP2UsbRoleSwitchCommon_Write(gpioPath, IAP2_USB_GPIO_OTG_DIRECTION,
                        IAP2_USB_GPIO_OTG_OUT, FALSE))
                {
                    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "set USB power %s/%s failed",
                                            gpioPath, IAP2_USB_GPIO_OTG_DIRECTION);
                    status = IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED;
                }
            }
            else
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "path %s does not exist", valuePath);
            }
        }
    }

    /* write GPIO value */
    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        if (TRUE != iAP2UsbRoleSwitchCommon_Write(gpioPath, IAP2_USB_GPIO_OTG_VALUE, value, FALSE))
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "set USB power %s/%s failed",
                                    gpioPath, IAP2_USB_GPIO_OTG_VALUE);
            status = IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED;
        }
    }

    return status;
}

static iAP2USBRoleSwitchStatus _exportGPIO(const S32 gpioNumber, char* path)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

    /* construct GPIO path */
    int ret = snprintf(path, IAP2_SYS_MAX_PATH, "%s/gpio%d",
            IAP2_USB_GPIO_OTG_PATH, gpioNumber);
    if (ret < 0 || ret >= IAP2_SYS_MAX_PATH)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "USB power internal error");
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    /* check if the GPIO is exported */
    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        if (TRUE != iAP2UsbRoleSwitchCommon_Exists(path))
        {
            char number[32];
            snprintf(number, 32, "%d", gpioNumber); /* no need to check this */

            if (TRUE != iAP2UsbRoleSwitchCommon_Write(IAP2_USB_GPIO_OTG_PATH,
                    IAP2_USB_GPIO_OTG_EXPORT, number, FALSE))
            {
                IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "export USB power GPIO %d failed", gpioNumber);
                status = IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED;
            }
        }
    }

    return status;
}

