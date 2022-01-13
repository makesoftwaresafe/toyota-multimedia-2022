/**
* \file: iap2_usb_role_switch_config.h
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

/* TODO replace defines with const */

/* time between power off and power on again for switch to device mode */
#define IAP2_USB_ROLE_SWITCH_POWEROFF_TIME      1000 /* ms */

/* time out after vendor request */
#define IAP2_USB_VENDOR_REQUEST_TIMEOUT_SEC     2 /* s */
#define IAP2_USB_VENDOR_REQUEST_TIMEOUT_USEC    0 /* us */

/* vendor request; see Apple specification */
#define IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_WITH_DIPO      0x40, 0x51, 0x01, 0x00, NULL, 0
#define IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_WITHOUT_DIPO   0x40, 0x51, 0x00, 0x00, NULL, 0
/* old vendor request; for backward compatibility */
#define IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_OLD            0x40, 0x50, 0x00, 0x00, NULL, 0



/* timeout of the vendor request */
#define IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_TIMEOUT_MSEC   3000 /* ms */
