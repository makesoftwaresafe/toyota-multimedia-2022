
#ifndef IAP2_USB_UDC_H_
#define IAP2_USB_UDC_H_

/* **********************  includes  ********************** */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>
#include <adit_dlt.h>

#include <libudev.h>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>
#include <fcntl.h>
#include <errno.h>

#include "iap2_dlt_log.h"

/* **********************  defines   ********************** */

#define STR_SYS_BUS_USB_DEVICES                 "/sys/bus/usb/devices"
#define STR_SYS_CLASS_UDC                       "/sys/class/udc"
#define STR_UDC_FORCE                           "udc_force"

#define STR_DWC3_UDC                            "/dwc3.*"
#define STR_DWC                                 "dwc"
#define STR_CI_HDRC                             "ci_hdrc"
#define STR_OTG_PORT                            "1-1"

#define STR_UTBRIDGE_UDC                        "utbridge_udc*"
#define STR_UNWIRED_HUB                         "Unwired Hub"
#define STR_HOST_TO_HOST_BRIDGE                 "Host to Host Bridge"

#define STR_BRIDGE_PORT_POWER                   "portpower"
#define STR_BRIDGE_PORT                         "bridgeport"

#define STR_BRIDGE_PORT_POWER_OFF               "=0"
#define STR_BRIDGE_PORT_POWER_ON                "=1"

#define STR_BRIDGE_PORT_POWER_CTRL_OFF          "off"
#define STR_BRIDGE_PORT_POWER_CTRL_ON           "on"

#define STR_BRIDGE_PORT_NUMBER                  ".5"
#define STR_BRIDGE_CONFIG_PORT                  ".5:1.0"    /* .5:1.0 or .5\:1.0 ? */

#define STR_SLASH                               '/'
#define STR_DOT                                 '.'
#define STR_PRODUCT                             "product"
#define STR_NULL_TERMINATED                     '\0'
#define STR_PORT                                "port"
#define STR_CONTROL                             "control"


typedef enum
{
    CON_ERROR             = -1,
    NOT_CONNECTED         = 0,
    OTG_CONNECTED         = 1,
    UNWIRED_HUB_CONNECTED = 2
} usbConnectStateType_t;

typedef struct
{
    /* connection type. e.g OTG or Unwired Hub */
    usbConnectStateType_t type;
    /* udev path of the connected Apple device */
    char* pUdevPath;
    /* /sys/bus/usb/device path of the utbridge */
    char* pUtBridgePath;
    /* /sys/bus/usb/device/:1.0/ path of the utbridge */
    char* pUtBridgeConfigPath;
    /* Name of the udc device. e.g. utbridge_udc.0 or ci_hdrc.0 */
    char* pUdcDevice;
    /* /sys/devices/.. port number. e.g. 2-1.2 */
    char* pSysDevicesPortNum;
    /* utBridge port number. e.g. 1-1 or 2-1 */
    char* pBridgePortNum;
    /* device port number on the Unwired Hub. e.g. 2, 3 or 4 */
    char* pDevicePortNum;
} udcParamInfo_t;

/* **********************  functions ********************** */

void freeUdcParam(udcParamInfo_t* pUdcParam);
usbConnectStateType_t getConnectStateType(char* pUdevPath, udcParamInfo_t* pUdcParam);
BOOL setUdcForce(char* pUdcDevice);
S32 switchBridgePower(char* pUtBridgeBusDev, const char* pPortNum, BOOL bOnOff);
S32 udcSwitchToHostMode(udcParamInfo_t* pUdcParam);
S32 udcSwitchToDeviceMode(udcParamInfo_t* pUdcParam);


#endif /* IAP2_USB_UDC_H_ */
