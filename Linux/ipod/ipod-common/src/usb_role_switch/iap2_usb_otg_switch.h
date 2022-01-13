/**
* \file: iap2_usb_otg_switch.h
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

#ifndef IAP2_USB_OTG_SWITCH_H
#define IAP2_USB_OTG_SWITCH_H

#include "iap2_usb_role_switch_common.h"

#define IAP2_USB_OTG_ROLE       "role"
#define IAP2_USB_OTG_HOST       "host"
#define IAP2_USB_OTG_GADGET     "gadget"

iAP2USBRoleSwitchStatus _findOTGPath(char* otgPath, U32 len, const char* otgGlob);

IAP2_USB_HIDDEN_SYMBOL iAP2USBRoleSwitchStatus iAP2USBOTGSwitch_SwitchToGadget(const char* otgGlob);
IAP2_USB_HIDDEN_SYMBOL iAP2USBRoleSwitchStatus iAP2USBOTGSwitch_SwitchToHost(const char* otgGlob);

#endif /* IAP2_USB_OTG_SWITCH_H */
