#ifndef _IPOD_DEV_DATACOM_I_H
#define _IPOD_DEV_DATACOM_I_H

#ifdef __cplusplus
extern "C" {
#endif

#include <iap2_datacom.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>

#include "iap2_configure_ffs_gadget.h"

#define IPOD_IAP2_USBHOST_MAX_SEND_BYTES    65535
#define IPOD_IAP2_USBHOST_POLL_TIMEOUT      2000
#define IPOD_IAP2_USBHOST_STRING_MAX        256

typedef struct
{
    int iOSAppIdentifier;
    int interfaceNumber;
    int inEndpoint;
    int outEndpoint;
} alternate_interface_t;

typedef struct _IPOD_IAP2_HID_HOSTDEV_INFO
{
    S32 ep0_fd;
    S32 read_fd;
    S32 write_fd;
    U8  name[IPOD_IAP2_USBHOST_STRING_MAX];  /* device name or serial number */

    /* filename of write endpoint e.g. /dev/ffs/ep1 */
    U8* EndPointSink;
    /* filename of read endpoint e.g. /dev/ffs/ep2 */
    U8* EndPointSource;
    IPOD_IAP2_DATACOM_IOCTL_CONFIG ioctl_config;

    IPOD_IAP2_DATACOM_ALTERNATE_IF_CB EAcallbacks;

    alternate_interface_t* alternate_interface;
    g_ffs_status_t isReady;
    iAP2_ffs_config_t iAP2_ffs_config;
} IPOD_IAP2_HID_HOSTDEV_INFO;

#endif /* _IPOD_DATACOM_I_H */

