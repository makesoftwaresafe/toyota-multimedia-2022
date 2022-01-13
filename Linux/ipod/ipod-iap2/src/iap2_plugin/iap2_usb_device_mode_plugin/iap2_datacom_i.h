#ifndef _IPOD_DATACOM_I_H
#define _IPOD_DATACOM_I_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/socket.h>
#include <sys/un.h>

#include "iap2_hidapi.h"
#define IPOD_USB_LINK_BYTE_POS 19
#define IPOD_USB_LENGTH_POS 12
#define IPOD_LINK_TRANSPORT_BYTES 0x04 
#define IPOD_DEVICE_NAME_LIMIT 256
#define IPOD_USB_HEADER_SIZE 3
#define IPOD_USB_FOOTER_SIZE 0
#define IPOD_USB_LINK_CTRL_BYTE_SIZE 1

#define IPOD_LINK_CTRL_ONLY_ONE         0x00
#define IPOD_LINK_CTRL_FRST_FOLLOW      0x02
#define IPOD_LINK_CTRL_MIDDLE           0x03
#define IPOD_LINK_CTRL_LAST             0x01

#define IPOD_USB_REPORT_TYPE            0x02

#define IPOD_USB_VENDOR_ID              0x05AC
#define IPOD_USB_PRODUCT_ID_MASK        0xFF00
#define IPOD_USB_PRODUCT_ID             0x1200

#define IPOD_COM0 0
#define IPOD_COM1 1
#define IPOD_COM2 2
#define IPOD_COM3 3
#define IPOD_COM_MSEC 1000
#define IPOD_COM_NSEC 1000000

#define IPOD_MESSAGE_START_LENGTH 2

#define IPOD_COM_DELAY_50MS                 50

#define IPOD_COM_DEVICE_NAME_MAX 256

/* Extended lingo short message: 0x55, length byte, lingo byte, two byte command ID = 5 */
#define IPOD_TRANSLEN_WITH_EXTEND_ACK_SHORT 5
/* Other lingo short message: 0x55, length byte, lingo byte, one byte command ID = 4 */
#define IPOD_TRANSLEN_WITH_ACK_SHORT 4
/* Extended lingo long  message: 0x55, three length bytes, lingo byte, two byte command ID = 7 
   (-1 for other Lingo) */
#define IPOD_TRANSLEN_WITH_ACK_LONG  7
/* Other lingo short message: 0x55, length byte, lingo byte, one byte command ID = 4 */
#define IPOD_TRANSLEN_WITH_NO_ACK_SHORT 4
/* Other lingo long message: 0x55, length byte, three lingo bytes, one byte command ID = 6 
   (does not apply for Extended lingo, as Extended lingo always includes reply) */
#define IPOD_TRANSLEN_WITH_NO_ACK_LONG 6

#define IPOD_HID_DEVICE_NAME_LEN_MAX 256
typedef struct _IPOD_IAP2_REPORT_DATA
{
    U32  rep_size;    /* the size of the report */
    U8  rep_id;      /* Report-ID              */
} IPOD_IAP2_REPORT_DATA;

/*
 * general socket configuration
 */
struct iAP2HidSck
{
    U8          Buff[HID_BUFF_MAX]; /* transfer buffer */
    U32         BuffTransLen;       /* size of bytes to be send / received */
};

typedef struct _IPOD_IAP2_HID_DEV_INFO
{
    iap2_hid_device *dd;
    U16 vid;
    U16 pid;
    U8  name[IPOD_HID_DEVICE_NAME_LEN_MAX];         /* device name or serial number */

    struct iAP2HidSck Evnt;

    IPOD_IAP2_REPORT_DATA* reportData; /* HID report table for outgoing reports */
    U32 dataCounter; /* Number of entries in table above */
    U32 maxDataLen;
    U8 *outBuf;

    IPOD_IAP2_REPORT_DATA* inReportData; /* HID report table for incoming reports */
    U32 inDataCounter; /* Number of entries in table above */

    libusb_context *usb_context;

} IPOD_IAP2_HID_DEV_INFO;


S32 iPodiAP2USBDeviceLibUSBHandleEvent(void *iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 pollFd);
S32 iPodiAP2USBDeviceLibUSBGetFDs(void *iPodHdl, IPOD_IAP2_DATACOM_FD* getFDs, S32* fd_count);
S32 iPodiAP2USBDeviceOpenPlugin(void *iPodHdl,const U8* device_name, S32 flags, S32 mode);
S32 iPodiAP2USBDeviceClosePlugin(void *iPodHdl);
S32 iPodiAP2CloseMsgHandling(void *iPodHdl);
S32 iPodiAP2SendtoUSB(void *iPodHdl, U32 msgLenTotal, const U8 *iPod_msg, S32 flags);
S32 iPodiAP2ReceiveMessage(void *iPodHdl, U32 buffer_size, U8 *msgBuffer, S32 flags);
S32 iPodiAP2USBDeviceIoCtl(void* iPodHdl, S32 request, void* argp);

S32  iPodiAP2HIDInit(IPOD_IAP2_HID_DEV_INFO *devInfo);
void iPodiAP2HIDDeInit(IPOD_IAP2_HID_DEV_INFO *devInfo);

S32 iPodiAP2HIDLibUsbHandleEvent(IPOD_IAP2_HID_DEV_INFO *devInfo, U32 buffer_size, U8* msgBuffer,S32 pollFd);
S32 iPodiAP2HIDLibUSBGetFDs(IPOD_IAP2_HID_DEV_INFO* devInfo,iap2_hid_poll_fd *get_pollFDs, S32* fd_count);
S32 iPodiAP2HIDOpen(IPOD_IAP2_HID_DEV_INFO* devInfo, const U8* drivername, S32 mode);
S32 iPodiAP2HIDClose(IPOD_IAP2_HID_DEV_INFO* devInfo);
S32 iPodiAP2HIDWaitEvent(IPOD_IAP2_HID_DEV_INFO* devInfo, U32 buffer_size, U8* msgBuffer);
S32 iPodiAP2HIDWriteReport(IPOD_IAP2_HID_DEV_INFO* devInfo, U32 totalLen, const U8* iPod_msg);
S32 iPodiAP2USBGetReportLenMax(IPOD_IAP2_HID_DEV_INFO* devInfo, U32 *reportLen);
S32 iPodiAP2USBGetProperty(void* iPodHdl, IPOD_IAP2_DATACOM_PROPERTY *property);

S32 iPodiAP2HIDTrnspInit(IPOD_IAP2_HID_DEV_INFO* devInfo);
S32 iPodiAP2HIDTrnspDeInit(IPOD_IAP2_HID_DEV_INFO* devInfo);

void iPodiAP2USBComSleep(U32 sleep_ms);
S32 iPodiAP2USBGetReportId_internal(IPOD_IAP2_HID_DEV_INFO* devInfo);

#endif /* _IPOD_DATACOM_I_H */

