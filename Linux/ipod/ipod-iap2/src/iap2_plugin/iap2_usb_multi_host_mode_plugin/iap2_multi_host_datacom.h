#ifndef _IAP2_USB_MULTI_HOST_PLUGIN_H
#define _IAP2_USB_MULTI_HOST_PLUGIN_H


#ifdef __cplusplus
extern "C" {
#endif


#include <iap2_datacom.h>
#include <string.h>
#include <unistd.h>
#include <poll.h>
#include <libusb-1.0/libusb.h>
#include <time.h>
#include <errno.h>
#include <sys/eventfd.h>

#define IPOD_MULTI_HOST_DEVICE_NAME_LEN_MAX        256
#define KERNEL_DRV_ACTIVE                          1          /* kernel driver is active */
#define IPOD_IAP2_USB_MULTI_HOST_MAX_SEND_BYTES    65535
#define LINK_UP                                    0x01
#define LINK_DOWN                                  0x00
#define INTERRUPT_DATA                             16
#define INTERFACE_IAP                              0x00
#define INTERFACE_LINK                             0x02
#define IAP2_MULTIHOST_MSEC                        1000
#define IAP2_MULTIHOST_NSEC                        1000000
#define RETRY_TRIAL                                10
#define ALT_INTERFACE_EAN                          0xFF
#define LINK_STATUS                                2
#define INTERFACE_STATUS                           5


typedef struct _iap2_multi_host_poll_fd
{
    S32 fd;
    short int events;
}iap2_multi_host_poll_fd;


/* Buffer used for async bulk read */
typedef struct _iap2_multi_host_bulk_trsfr
{
    U8* rcv_buff;
    S32 length;
    S32 bytes_read;
}iap2_multi_host_bulk_trsfr;


typedef struct _iap2_multi_host_endpt_cfg
{
    S32 input_endpoint;
    S32 output_endpoint;
    S32 input_ep_max_packet_size;
}iap2_multi_host_endpt_cfg;


typedef struct _alternate_interface_ea
{
    int iOSAppIdentifier;
    int interfaceNumber;
    int inEndpoint;
    int outEndpoint;
} alternate_interface_ea;


typedef struct _IPOD_IAP2_MULTI_HOST_DEV_INFO
{
    libusb_context *usb_context, *usb_context_interrupt;
    libusb_device_handle *device_handle, *device_handle_interrupt;

    iap2_multi_host_endpt_cfg *endpt_info, *link_endpt_info;

    IPOD_IAP2_DATACOM_ALTERNATE_IF_CB EAcallbacks;
    alternate_interface_ea *alternate_interface_ea;
    IPOD_IAP2_DATACOM_IOCTL_CONFIG ioctl_config;
    U8 *buffer;
    S32 nfds_iap, nfds_interrupt;
    iap2_multi_host_poll_fd *pollFDs_interrupt;

    U8  name[IPOD_MULTI_HOST_DEVICE_NAME_LEN_MAX];

    iap2_multi_host_bulk_trsfr *blk_tfr_data;
    int event_fd; /*usb read_callback called trigger to iAP2EventLoop*/
    struct libusb_transfer *transfer, *transfer_interrupt;
} IPOD_IAP2_MULTI_HOST_DEV_INFO;

static void iap2_multihost_init_transfer(void *devinfo);
static void iap2_multihost_init_interrupt_transfer(void *devInfo);
static void iap2_multihost_libusb_add_pollfd_cb(S32 fd, short events, void *user_context);
static void iap2_multihost_libusb_remove_pollfd_cb(S32 fd, void *user_context);
static void read_interrupt_callback(struct libusb_transfer *transfer);
static void iap2_multihost_read_callback(struct libusb_transfer *transfer);
void iAP2USBMultiHostHandle_EAP(void* devinfo);
void iAP2USBMultiHostFreeIoCtlConfig(IPOD_IAP2_MULTI_HOST_DEV_INFO *devinfo);
S32 iAP2USBMultiHostInitIoCtlConfig(IPOD_IAP2_MULTI_HOST_DEV_INFO *devinfo, IPOD_IAP2_DATACOM_IOCTL_CONFIG* config);
S32 iap2_multihost_libusb_handleevent(libusb_context *usb_context, S32 pollFD);
S32 iap2_multihost_libusb_get_pollfds(void *devinfo, iap2_multi_host_poll_fd *pollFDs, S32 *fd_count);
S32 iap2_multihost_submit_interrupt_tranfer(void *devInfo);
S32 iap2_multihost_submit_bulk_tranfer(void *devinfo);
void iap2_multihost_mSleep(U32 sleep_ms);

S32 iPodiAP2USBMultiHostOpenPlugin(void* devinfo, const U8* device_name, S32 flags, S32 mode);
S32 iPodiAP2USBMultiHostLibUSBGetFDs(void* devinfo, IPOD_IAP2_DATACOM_FD* getFDs, S32* fd_count);
S32 iPodiAP2USBMultiHostIoCtl(void* devinfo, S32 request, void* argp);
S32 iPodiAP2USBMultiHostWrite(void *devinfo, U32 msgLenTotal, const U8 *iPod_msg, S32 flags);
S32 iPodiAP2USBMultiHostLibUSBHandleEvent(void* devinfo, U32 buffer_size, U8 *msgBuffer, S32 pollFD);
S32 iPodiAP2USBMultiHostClosePlugin(void *devinfo);
S32 iPodiAP2USBMultiHostCloseMsgHandling(void *devinfo);
S32 iPodiAP2USBMultiHostGetProperty(void* devinfo, IPOD_IAP2_DATACOM_PROPERTY *property);
S32 iPodiAP2USBMultiHostReceiveMessage(void* devinfo, U32 buffer_size, U8 *msgBuffer, S32 flags);

#endif
