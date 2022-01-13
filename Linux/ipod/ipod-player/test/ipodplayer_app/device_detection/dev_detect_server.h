#ifndef DEV_DETECT_SERVER_H
#define DEV_DETECT_SERVER_H

#include "iPodPlayerIPCLib.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerDef_in.h"
#define DEV_DETECT_SERVER_PATH "dev_detection"
#define DEV_DETECT_CFG_STRING_MAX 256
#define DEV_DETECT_DEVICE_LEN 5
#define DEVDETECT_OK 0
#define DEVDETECT_ERROR -1

#define DEV_DETECT_DETACH 0
#define DEV_DETECT_ATTACH 1
#define DEV_DETECT_DEVICE_MAX 5


#define DEV_DETECT_VENDOR_MAX_LEN                   64
#define DEV_DETECT_MAX_HANDLE 8

#define DEV_DETECT_WAIT_TIME 100000000
#define DEV_DETECT_ACTION_LEN 4
#define DEV_DETECT_SOUND_NAME_PREFIX "/dev/snd/pcm"
#define DEV_DETECT_SOUND_HW_NAME "hw:%c,%c"
#define DEV_DETECT_SOUND_HW_CARD_LEN 2
#define DEV_DETECT_ACTION_STATUS_CONNECT 1
#define DEV_DETECT_ACTION_STATUS_RECONNECT 2
#define DEV_DETECT_EPOLL_SIZE 1

#define IPOD_USB_ACTION_ADD "add"
#define IPOD_USB_ATTR_VENDOR "idVendor"
#define IPOD_USB_ATTR_PRODUCT "idProduct"
#define IPOD_USB_ATTR_SERIAL "serial"
#define IPOD_APPLE_IDVENDOR "05ac"
#define IPOD_APPLE_IDPRODUCT_MIN "1200"
#define IPOD_APPLE_IDPRODUCT_MAX "1300"
#define IPOD_USB_SUBSYSTEM "usb"
#define IPOD_USB_DEVTYPE "usb_device"
#define IPOD_USB_MONITOR_LINK "udev"
#define IPOD_USB_HID_NUM_POS 15
#define IPOD_USB_HID_SUBSYSTEM_TYPE "usb"

#ifndef IPOD_HOSTPLUGIN_HIDDEV
#define IPOD_USB_MONITOR_DEVTYPE "usb"
#define IPOD_USB_FILTER_TYPE "usb_device"
#else
#define IPOD_USB_MONITOR_DEVTYPE "hid"
#define IPOD_USB_FILTER_TYPE NULL
#endif

#define IPOD_USB_SOUND_SUBSYSTEM "sound"

#define IPOD_DEV_DLT_CONTEXT                         "DEV"
#define IPOD_DEV_DLT_CONTEXT_DSP                     "iPod Device Detect Context For Logging"
#define IPOD_DEV_END_EVENT               1            /* end process event      */
#define IPOD_DEV_SEND_RETRY_MAX_NUM      10           /* send event retry times */
#define IPOD_DEV_SEND_RETRY_WAIT_TIME    500000       /* retry waite 50 msec    */

#define IPOD_DEV_VEN_bmREQ       (0x40)
#define IPOD_DEV_VEN_REQ         (0x40)
#define IPOD_DEV_VEN_VAL         (500)
#define IPOD_DEV_VEN_IX          (500)
#define IPOD_CTRL_TRANS_TOUT     (1000)
#define IPOD_DEV_STR_TO_HEX      (16)

typedef struct
{
    U8 deviceName[DEV_DETECT_CFG_STRING_MAX];
    U8 audioInName[DEV_DETECT_CFG_STRING_MAX];
    U8 serialName[DEV_DETECT_CFG_STRING_MAX];
    dev_t deviceAddr;
    U16 idVendor;
    U16 idProduct;
    U8 action;
} DEV_DETECT_DEVINFO;

typedef struct
{
    struct udev *udev;
    struct udev_monitor *moniter;
    S32 udevfd;
    S32 udevHandle;
    S32 waitHandle;
    DEV_DETECT_DEVINFO devInfo[DEV_DETECT_DEVICE_MAX];
    S32 handles[DEV_DETECT_MAX_HANDLE];
    U32 handleNum;
} DEV_DETECT_SERVER_CFG;


#endif /* DEV_DETECT_SERVER_H */
