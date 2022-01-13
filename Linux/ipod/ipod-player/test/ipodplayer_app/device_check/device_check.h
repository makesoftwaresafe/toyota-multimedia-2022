#ifndef DEVICE_CHECK_H
#define DEVICE_CHECK_H

#define DEVICE_CHK_OK    0
#define DEVICE_CHK_ERR  -1

/* udev enumeration definition */
#define DEV_DETECT_DEVICE_NUM_MAX   8

#define DEV_DETECT_CFG_STRING_MAX   256

#define DEV_DETECT_VENDOR_MAX_LEN   5
#define USB_SELECT_RETRY_CNT        5
#define SYSATTR_IDVENDOR            "idVendor"
#define APPLE_VENDOR_ID             "05ac"
#define APPLE_PRODUCT_ID            "1200"
#define UDEV_WAIT_TIMEOUT           5

typedef struct device_information_type
{
    U8 udevPath[DEV_DETECT_CFG_STRING_MAX];
    U8 serial[DEV_DETECT_CFG_STRING_MAX];
    U8 vendor[DEV_DETECT_VENDOR_MAX_LEN];
    U8 product[DEV_DETECT_VENDOR_MAX_LEN];
} CheckDeviceInformation;

#endif /* DEV_DETECT_SERVER_H */
