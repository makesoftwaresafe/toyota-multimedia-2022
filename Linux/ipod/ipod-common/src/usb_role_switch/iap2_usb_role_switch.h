/**
* \file: iap2_usb_role_switch.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* USB OTG role switch for iAP2 / Digital iPod Out.
*
* Known limitations:
*  - not thread-safe
*  - power switch over GPIO only, not working with all boards; this is planned to be changed
*
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

#ifndef IAP2_USB_ROLE_SWITCH_H
#define IAP2_USB_ROLE_SWITCH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <adit_typedef.h>
#include "iap2_usb_udc.h"
#include <string.h>
#include <sys/utsname.h>
#include <libusb-1.0/libusb.h>

#define STR_MAX_LENGTH                          256
#define STR_DISABLE                             "disable"
#define STR_VBUS_AUTO                           "vbus_auto"
#define STR_ENABLE                              "enable"
#define STR_USB_DYN_NUM                         "usb*"

#ifdef IPOD_ARCH_ARM
    #define STR_NATIVE_OTG                          "ci_hdrc.0"
#elif defined(IPOD_ARCH_ARM64)
    #define STR_NATIVE_OTG                          "e6590000.usb"
#else
    #define STR_NATIVE_OTG                          "dwc3."
#endif /* IPOD_ARCH_ARM */

/* Kernel 3.14 has the additional sub-path "soc0" inside */
#define IAP2_USB_ROLE_SWITCH_OTG_GLOB_314       "/sys/devices/soc0/soc.0/21*/2184*/*"
/* Kernel 3.8 OTG path */
#define IAP2_USB_ROLE_SWITCH_OTG_GLOB           "/sys/devices/soc.0/21*/2184*/*"
/* Kernel 4.1.22 with extcon(broxton) */
#define IAP2_USB_ROLE_SWITCH_OTG_GLOB_EXTCON    "/sys/bus/platform/devices/intel-mux-drcfg/portmux.0/state"
/* Kernel OTG path for R-CAR*/
#define IAP2_USB_ROLE_SWITCH_OTG_GLOB_RCAR      "/sys/devices/platform/soc/ee080200.usb-phy/role"

#ifdef IPOD_ARCH_ARM
    /* vbus is not available in case we are not for example on a imx6-qai target */
    #define IAP2_VBUS_POWER                         "/sys/class/udc/*/device/vbus"
#elif defined(IPOD_ARCH_ARM64)
    /* vbus control is available via GPIO-384 on R-Car */
    #define IAP2_VBUS_POWER                         "/sys/class/gpio/gpio384/value"
    #define IAP2_VBUS_POWER_GPIO                    384
#else
    #define IAP2_VBUS_POWER                         "/dev/cbc-raw0"
#endif /* IPOD_ARCH_ARM */

/* return codes public exported functions, for exporting functions see below */
typedef enum
{
    IAP2_USB_ROLE_SWITCH_OK = 0,

    IAP2_USB_ROLE_SWITCH_FAILED = -1,
    IAP2_USB_ROLE_SWITCH_INVALID_ARGUMENT = -2,

    IAP2_USB_ROLE_SWITCH_DEVICE_NOT_FOUND = -3,
    IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED = -4,
    IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_TIMEOUT = -7,
    IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_NOT_SUPPORTED = -9,

    IAP2_USB_ROLE_SWITCH_POWER_CHANGE_FAILED = -5,
    IAP2_USB_ROLE_SWITCH_OTG_SWITCH_FAILED = -6,
    IAP2_USB_ROLE_SWITCH_OTG_DEVICE_NOT_FOUND = -8,

    IAP2_USB_ROLE_SWITCH_STATUS_END = 0xff
} iAP2USBRoleSwitchStatus;

typedef enum
{
    IAP2_USB_ROLE_SWITCH_WITHOUT_DIGITAL_IPOD_OUT = 0,
    IAP2_USB_ROLE_SWITCH_WITH_DIGITAL_IPOD_OUT = 1,
    IAP2_USB_ROLE_SWITCH_WITH_EA = 2,
    /* Used when Multi-Host Hub is required to support BDCL app */

    IAP2_USB_ROLE_SWITCH_MODE_END = 0xff
} iAP2USBRoleSwitchMode;

typedef struct
{
    /* mode (width dipo or without) */
    iAP2USBRoleSwitchMode mode;

    /* iOS device identifier */
    U16 vendorId;
    U16 productId;
    const char* serialNumber;

    /* OTG port identifier */
    const char* otgGlob;

    /* Depreciated: */
    /* TODO currently done via GPIO, common configuration method not defined yet */
    S32 powerGPIO;

    /* USB power switch identifier */
    char* vbusPower;

    U8 portnumber; // in case of molex-hub
} iAP2USBRoleSwitchInfo;

/**
 * \func iAP2USBVendorRequest_Send
 *
 * Send vendor request to iOS device to switch to host mode.
 *
 * \param vid                   vendor id of iOS device
 *        pid                   product id of iOS device
 *        serial                serial number of iOS device
 *        mode                  send vendor request with or without Digital iPod Out support
 *
 * \return error code
 *          IAP2_USB_ROLE_SWITCH_OK:                            successful
 *
 *          IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED          failed to send vendor request
 *          IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_NOT_SUPPORTED   vendor request not supported by the
 *                                                              device
 *          IAP2_USB_ROLE_SWITCH_DEVICE_NOT_FOUND               iOS device not found
 *          IAP2_USB_ROLE_SWITCH_FAILED:                        other error
 */
iAP2USBRoleSwitchStatus iAP2USBVendorRequest_Send(U16 vid, U16 pid,
                      const char* serial, iAP2USBRoleSwitchMode mode);

/* **********************  functions ********************** */
/* return TRUE (1) if 4.14 Kernel. Otherwise FALSE (0). */
static inline BOOL iap2IsKernel414(void) {
    struct utsname buf;
    if (0 == uname(&buf)) {
        if(0 == strncmp(buf.release, "4.14", strlen("4.14")))
            return TRUE;
    }
    return FALSE;
}

/* return TRUE (1) if 3.14 Kernel. Otherwise FALSE (0). */
static inline BOOL iap2IsKernel314(void) {
    struct utsname buf;
    if (0 == uname(&buf)) {
        if(0 == strncmp(buf.release, "3.14", strlen("3.14")))
            return TRUE;
    }
    return FALSE;
}

static inline BOOL iap2IsKernel4xx(void) {
    struct utsname buf;
    if (0 == uname(&buf)) {
        if(0 == strncmp(buf.release, "4.", strlen("4.")))
            return TRUE;
    }
    return FALSE;
}

char* iap2GetSwtichOtgGlobPath(void);
iAP2USBRoleSwitchStatus iAP2USBPower_Switch(const char* path, const char* value);
S32  iap2SwitchToHostMode(iAP2USBRoleSwitchInfo* info, udcParamInfo_t* pUdcParam);
S32  iap2SwitchToDeviceMode(iAP2USBRoleSwitchInfo* info, udcParamInfo_t* pUdcParam);

/***************************************************************************//**
 * S32  iap2SwitchToMultiHostMode(iAP2USBRoleSwitchInfo* info)
 *
 * Performs role switch operation of Apple device.
 * Then enables multi-host session mode for the Molex Hub by sending vendor request command to HFC
 *
 * \param[in-out]  info - Role switch information structure passed by application.
 * Only Port number is filled here and it is an out parameter which should be retained by application to pass the same to TerminateMultiHostMode function
 * Because once apple device switches to Host mode, it is not possible to get this port number. This is required to send disable multi host command to the Molex Hub.
 * Other members of this structure are filled by application
 * NOTE: For Multi-Host mode, info.otgGlob,info.powerGPIO,info.vbusPower are not used so need not be filled
 * \param[in]  pUdcParam - Not relevant to this function. Just interface is retained.
 *
 * \return Returns a signed integer value indicating Zero on success and less than Zero on failure
 *
 ******************************************************************************/
S32  iap2SwitchToMultiHostMode(iAP2USBRoleSwitchInfo* info, udcParamInfo_t* pUdcParam);

/***************************************************************************//**
 * S32  iap2TerminateMultiHostMode(iAP2USBRoleSwitchInfo* info)
 *
 * Disables multi-host session mode for the Molex Hub by sending vendor request command to HFC
 * Note : Vbus reset functionality is not supported by the Hub. So vbus reset is not done.
 * Note : Port number should be the same value which was obtained in function iap2SwitchToMultiHostMode earlier.
 *
 * \param[in]  info - Role switch information structure passed by application
 * \return Returns a signed integer value indicating success or failure
 *
 ******************************************************************************/
S32  iap2TerminateMultiHostMode(iAP2USBRoleSwitchInfo* info);

S32  iap2FindVbus(iAP2USBRoleSwitchInfo* info);

void iap2SwitchOTGInitialize(void);

#ifdef __cplusplus
}
#endif

#endif /* IAP2_USB_ROLE_SWITCH_H */
