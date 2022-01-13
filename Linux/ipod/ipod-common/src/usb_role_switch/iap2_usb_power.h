/**
* \file: iap2_usb_power.h
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

#ifndef IAP2_USB_POWER_H
#define IAP2_USB_POWER_H

#include "iap2_usb_role_switch_common.h"

IAP2_USB_HIDDEN_SYMBOL iAP2USBRoleSwitchStatus iAP2USBPower_SwitchOn(const S32 powerGPIO);
IAP2_USB_HIDDEN_SYMBOL iAP2USBRoleSwitchStatus iAP2USBPower_SwitchOff(const S32 powerGPIO);

#endif
