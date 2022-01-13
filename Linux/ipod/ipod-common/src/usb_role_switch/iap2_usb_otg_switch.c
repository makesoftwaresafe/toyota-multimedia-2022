/**
* \file: iap2_usb_otg_switch.c
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

#include "iap2_usb_otg_switch.h"
#include <glob.h>
#include <string.h>


static iAP2USBRoleSwitchStatus _switch(const char* otgGlob, const char* value);


iAP2USBRoleSwitchStatus iAP2USBOTGSwitch_SwitchToGadget(const char* otgGlob)
{
    return _switch(otgGlob, IAP2_USB_OTG_GADGET);
}

iAP2USBRoleSwitchStatus iAP2USBOTGSwitch_SwitchToHost(const char* otgGlob)
{
    return _switch(otgGlob, IAP2_USB_OTG_HOST);
}

static iAP2USBRoleSwitchStatus _switch(const char* otgGlob, const char* value)
{
    iAP2USBRoleSwitchStatus status;
    char otgPath[IAP2_SYS_MAX_PATH];

    status = _findOTGPath(otgPath, IAP2_SYS_MAX_PATH, otgGlob);
    /* error logged and handled */

    if (status == IAP2_USB_ROLE_SWITCH_OK &&
            TRUE != iAP2UsbRoleSwitchCommon_Write(otgPath, IAP2_USB_OTG_ROLE, value, TRUE))
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "set USB OTG %s/%s to %s failed", otgPath,
                IAP2_USB_OTG_ROLE, value);
        status = IAP2_USB_ROLE_SWITCH_OTG_SWITCH_FAILED;
    }

    return status;
}

iAP2USBRoleSwitchStatus _findOTGPath(char* otgPath, U32 len,
        const char* otgGlob)
{
    iAP2USBRoleSwitchStatus status = IAP2_USB_ROLE_SWITCH_OK;

    strncpy(otgPath, otgGlob, len);
    strncat(otgPath, "/" IAP2_USB_OTG_GADGET, len - strlen(otgPath));

    glob_t found;
    int ret;
    if (0 == (ret = glob(otgPath, 0, NULL, &found)) && found.gl_pathc > 0)
    {
        strncpy(otgPath, found.gl_pathv[0], len);
        otgPath[strlen(otgPath) - 7] = 0; /* terminate string before IAP2_USB_OTG_GADGET string */

        if (found.gl_pathc > 1)
        {
            IAP2USBROLESWITCHDLTLOG(DLT_LOG_WARN, "more than one OTG device found; use: %s",
                    otgPath);
        }

        globfree(&found);
    }
    else if (ret == GLOB_NOMATCH)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "could not find USB OTG gadget at %s",
                otgPath);
        status = IAP2_USB_ROLE_SWITCH_OTG_DEVICE_NOT_FOUND;
    }
    else
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR,
                         "USB OTG switch internal error: glob = %d", ret);
        status = IAP2_USB_ROLE_SWITCH_FAILED;
    }

    return status;
}
