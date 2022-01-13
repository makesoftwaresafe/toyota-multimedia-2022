/*******************************************************
 HIDAPI - Multi-Platform library for
 communication with HID devices.

 Alan Ott
 Signal 11 Software

 8/22/2009
 Linux Version - 6/2/2010
 Libusb Version - 8/13/2010
 FreeBSD Version - 11/1/2011

 Copyright 2009, All Rights Reserved.

 At the discretion of the user of this library,
 this software may be licensed under the terms of the
 GNU Public License v3, a BSD-Style license, or the
 original HIDAPI license as outlined in the LICENSE.txt,
 LICENSE-gpl3.txt, LICENSE-bsd.txt, and LICENSE-orig.txt
 files located at the root of the source distribution.
 These files may also be found in the public source
 code repository located at:
        http://github.com/signal11/hidapi .
********************************************************/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE // needed for wcsdup() before glibc 2.10
#endif /* _GNU_SOURCE */

#pragma GCC diagnostic ignored "-Wtype-limits"

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <locale.h>
#include <errno.h>

/* Unix */
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/utsname.h>
#include <fcntl.h>
#include <pthread_adit.h>
#include <wchar.h>
#include <unistd.h>

/* GNU / LibUSB */
#include <libusb-1.0/libusb.h>
#include "iconv.h"

#include "hidapi.h"
#include "iap_hid_desc.h"
#include "iap1_dlt_log.h"


#define HID_API_BUFFER_MAX_QUEUED   30

/* Uncomment to enable the retrieval of Usage and Usage Page in
hid_enumerate(). Warning, this is very invasive as it requires the detach
and re-attach of the kernel driver. See comments inside hid_enumerate().
libusb HIDAPI programs are encouraged to use the interface number
instead to differentiate between interfaces on a composite HID device. */
/*#define INVASIVE_GET_USAGE*/

/* Linked List of input reports received from the device. */
struct input_report {
    uint8_t *data;
    size_t len;
    struct input_report *next;
};


struct hid_device_ {
    /* Handle to the actual device. */
    libusb_device_handle *device_handle;
    /* libusb transfer */
    struct libusb_transfer *transfer;

    /* Endpoint information */
    int input_endpoint;
    int output_endpoint;
    int input_ep_max_packet_size;

    /* The interface number of the HID */
    int interface;

    /* Indexes of Strings */
    int manufacturer_index;
    int product_index;
    int serial_index;

    /* Whether blocking reads are used */
    int blocking; /* boolean */

    /* Read thread objects */
    pthread_t thread;
    int thread_ready;           /* This value is set to non zero value when read_thread is initialized successfully */
    pthread_mutex_t mutex;      /* Protects input_reports */
    pthread_cond_t condition;
    pthread_barrier_t barrier;  /* Ensures correct startup sequence */
    /* 1: libusb read_thread shut down */
    int shutdown_thread;
    /* 1: read_callback or a libusb API returned NO_DEVICE */
    int libusb_no_device;
    /* 1: libusb read thread should run */
    int event_thread_run;

    /* List of received input reports. */
    struct input_report *input_reports;

    /* Flag of flow control */
    int submit;                 /* boolean */

    /* report */
    HID_DESC_INF rep_info;      /* Information for report ID */
    unsigned int rep_length;    /* Maximum report length */
};

static libusb_context *usb_context = NULL;

uint16_t get_usb_code_for_current_locale(void);
static int return_data(hid_device *dev, unsigned char *data, size_t length);

static hid_device *new_hid_device(void)
{
    hid_device *dev = calloc(1, sizeof(hid_device));
    if(dev != NULL) {
        dev->blocking = 1;

        pthread_mutex_init(&dev->mutex, NULL);
        pthread_cond_init(&dev->condition, NULL);
        pthread_barrier_init(&dev->barrier, NULL, 2);
    }
    else
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "resource error");
    }

    return dev;
}

static void free_hid_device(hid_device *dev)
{
    if(dev == NULL)
    {
        return;
    }

    /* Clean up the thread objects */
    pthread_barrier_destroy(&dev->barrier);
    pthread_cond_destroy(&dev->condition);
    pthread_mutex_destroy(&dev->mutex);

    /* Free the device itself */
    free(dev);
}

#if 0
//TODO: Implement this funciton on hidapi/libusb..
static void register_error(hid_device *device, const char *op)
{

}
#endif

#ifdef INVASIVE_GET_USAGE
/* Get bytes from a HID Report Descriptor.
   Only call with a num_bytes of 0, 1, 2, or 4. */
static uint32_t get_bytes(uint8_t *rpt, size_t len, size_t num_bytes, size_t cur)
{
    if(rpt == NULL)
    {
        return 0;
    }

    /* Return if there aren't enough bytes. */
    if (cur + num_bytes >= len)
        return 0;

    if (num_bytes == 0)
        return 0;
    else if (num_bytes == 1) {
        return rpt[cur+1];
    }
    else if (num_bytes == 2) {
        return (rpt[cur+2] * 256 + rpt[cur+1]);
    }
    else if (num_bytes == 4) {
        return (rpt[cur+4] * 0x01000000 +
                rpt[cur+3] * 0x00010000 +
                rpt[cur+2] * 0x00000100 +
                rpt[cur+1] * 0x00000001);
    }
    else
        return 0;
}

/* Retrieves the device's Usage Page and Usage from the report
   descriptor. The algorithm is simple, as it just returns the first
   Usage and Usage Page that it finds in the descriptor.
   The return value is 0 on success and -1 on failure. */
static int get_usage(uint8_t *report_descriptor, size_t size,
                     unsigned short *usage_page, unsigned short *usage)
{
    int i = 0;
    int size_code;
    int data_len, key_size;
    int usage_found = 0, usage_page_found = 0;

    if((report_descriptor == NULL) || (usage_page == NULL) || (usage == NULL))
    {
        return HID_API_NG;
    }

    while (i < size) {
        int key = report_descriptor[i];
        int key_cmd = key & 0xfc;

        //printf("key: %02hhx\n", key);

        if ((key & 0xf0) == 0xf0) {
            /* This is a Long Item. The next byte contains the
               length of the data section (value) for this key.
               See the HID specification, version 1.11, section
               6.2.2.3, titled "Long Items." */
            if (i+1 < size)
                data_len = report_descriptor[i+1];
            else
                data_len = 0; /* malformed report */
            key_size = 3;
        }
        else {
            /* This is a Short Item. The bottom two bits of the
               key contain the size code for the data section
               (value) for this key.  Refer to the HID
               specification, version 1.11, section 6.2.2.2,
               titled "Short Items." */
            size_code = key & HID_REP_DESC_SITEM_SZ_MASK;
            switch (size_code) {
            case 0:
            case 1:
            case 2:
                data_len = size_code;
                break;
            case 3:
                data_len = 4;
                break;
            default:
                /* Can't ever happen since size_code is & 0x3 */
                data_len = 0;
                break;
            };
            key_size = 1;
        }

        if (key_cmd == 0x4) {
            *usage_page  = get_bytes(report_descriptor, size, data_len, i);
            usage_page_found = 1;
            //printf("Usage Page: %x\n", (uint32_t)*usage_page);
        }
        if (key_cmd == 0x8) {
            *usage = get_bytes(report_descriptor, size, data_len, i);
            usage_found = 1;
            //printf("Usage: %x\n", (uint32_t)*usage);
        }

        if (usage_page_found && usage_found)
            return HID_API_OK; /* success */

        /* Skip over this key and it's associated data */
        i += data_len + key_size;
    }

    return HID_API_NG;   /* failure */
}
#endif // INVASIVE_GET_USAGE

#ifdef __FreeBSD__
/* The FreeBSD version of libusb doesn't have this funciton. In mainline
   libusb, it's inlined in libusb.h. This function will bear a striking
   resemblence to that one, because there's about one way to code it.

   Note that the data parameter is Unicode in UTF-16LE encoding.
   Return value is the number of bytes in data, or LIBUSB_ERROR_*.
 */
static inline int libusb_get_string_descriptor(libusb_device_handle *dev,
    uint8_t descriptor_index, uint16_t lang_id,
    unsigned char *data, int length)
{
    return libusb_control_transfer(dev,
        LIBUSB_ENDPOINT_IN | 0x0, /* Endpoint 0 IN */
        LIBUSB_REQUEST_GET_DESCRIPTOR,
        (LIBUSB_DT_STRING << 8) | descriptor_index,
        lang_id, data, (uint16_t) length, 1000);
}

#endif


/* Get the first language the device says it reports. This comes from
   USB string #0. */
static uint16_t get_first_language(libusb_device_handle *dev)
{
    uint16_t buf[32];
    int len;

    if(dev == NULL)
    {
        return 0x0;
    }

    /* Get the string from libusb. */
    len = libusb_get_string_descriptor(dev,
            0x0, /* String ID */
            0x0, /* Language */
            (unsigned char*)buf,
            sizeof(buf));
    if (len < HID_STR_DESC_LANG_ID_LEN) /* Check for Language ID length */
        return 0x0;

    return buf[1]; // First two bytes are len and descriptor type.
}

static int is_language_supported(libusb_device_handle *dev, uint16_t lang)
{
    uint16_t buf[32];
    int len;
    int i;

    if(dev == NULL)
    {
        return 0x0;
    }

    /* Get the string from libusb. */
    len = libusb_get_string_descriptor(dev,
            0x0, /* String ID */
            0x0, /* Language */
            (unsigned char*)buf,
            sizeof(buf));
    if (len < HID_STR_DESC_LANG_ID_LEN) /* Check for Language ID length */
        return 0x0;


    len /= 2; /* language IDs are two-bytes each. */
    /* Start at index 1 because there are two bytes of protocol data. */
    for (i = 1; i < len; i++) {
        if (buf[i] == lang)
            return 1;
    }

    return 0;
}


/* This function returns a newly allocated wide string containing the USB
   device string numbered by the index. The returned string must be freed
   by using free(). */
static wchar_t *get_usb_string(libusb_device_handle *dev, uint8_t idx)
{
    char buf[512] = {0};
    int len;
    wchar_t *str = NULL;
    wchar_t wbuf[256];

    if(dev == NULL)
    {
        return NULL;
    }

    /* iconv variables */
    iconv_t ic;
    size_t inbytes;
    size_t outbytes;
    size_t res;
#ifdef __FreeBSD__
    const char *inptr;
#else
    char *inptr;
#endif
    char *outptr;

    /* Determine which language to use. */
    uint16_t lang;
    lang = get_usb_code_for_current_locale();
    if (!is_language_supported(dev, lang))
        lang = get_first_language(dev);
    /* Get the string from libusb. */
    len = libusb_get_string_descriptor(dev,
            idx,
            lang,
            (unsigned char*)buf,
            sizeof(buf));
    if (len < 0)
        return NULL;

    /* buf does not need to be explicitly NULL-terminated because
       it is only passed into iconv() which does not need it. */

    /* Initialize iconv. */
    ic = iconv_open("WCHAR_T", "UTF-8");
    if (ic == (iconv_t)-1) {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iconv_open() failed");
        return NULL;
    }

    /* Convert to native wchar_t (UTF-32 on glibc/BSD systems).
       Skip the first character (2-bytes). */
    inptr = buf+2;
    inbytes = len-2;
    outptr = (char*) wbuf;
    outbytes = sizeof(wbuf);
    res = iconv(ic, &inptr, &inbytes, &outptr, &outbytes);
    if (res == (size_t)-1) {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "iconv() failed");
        goto err;
    }

    /* Write the terminating NULL. */
    wbuf[sizeof(wbuf)/sizeof(wbuf[0])-1] = 0x00000000;
    if (outbytes >= sizeof(wbuf[0]))
        wbuf[(sizeof(wbuf) - outbytes) / sizeof(wbuf[0])] = 0x00000000;


    {
        unsigned char ser[256] = {0};
        wchar_t wides[256] = {0};
        int i = 0;
        int j = 0;
        while(wbuf[i] != '\0')
        {
            ser[j] = wbuf[i];
            i += 2;
            j++;
        }
        mbstowcs(wides, (const char *)ser, 256);
        str = wcsdup(wides);
    }

    /* Allocate and copy the string. */
    //    str = wcsdup(wbuf);
err:
    iconv_close(ic);

    return str;
}

static char *make_path(libusb_device *dev, int interface_number)
{
    char str[64];

    if(dev == NULL)
    {
        return NULL;
    }

    snprintf(str, sizeof(str), "%04x:%04x:%02x",
        libusb_get_bus_number(dev),
        libusb_get_device_address(dev),
        interface_number);
    str[sizeof(str)-1] = '\0';

    return strdup(str);
}


int HID_API_EXPORT hid_init(void)
{
    const struct libusb_version *vs = NULL;

    if (!usb_context) {
        const char *locale;

        /* Init Libusb */
        if (libusb_init(&usb_context))
            return HID_API_NG;

        vs = libusb_get_version();
        if (NULL != vs){
/* PRQA: Lint Message 123: Macro major defined with arguments at line 61,
 *                         file ^/pbd-rfs/usr/include/sys/sysmacros.h
 *                         -- this is just a warning.
 * We are using a structure of the libusb,
 * were major and minor a variable names of structure libusb_version. */
            IAP1_USBP_LOG(DLT_LOG_INFO, "libusb version is %d.%d.%d%s",
                           vs->major, vs->minor, vs->micro, vs->rc); /*lint !e123 */
        }

        /* Set the locale if it's not set. */
        locale = setlocale(LC_CTYPE, NULL);
        if (!locale)
            setlocale(LC_CTYPE, "");
    }

    return HID_API_OK;
}

int HID_API_EXPORT hid_exit(void)
{
    if (usb_context == NULL){
        IAP1_USBP_LOG(DLT_LOG_ERROR, "usb_context is NULL");
        return HID_API_NG;
    }

    libusb_exit(usb_context);
    usb_context = NULL;

    return HID_API_OK;
}

struct hid_device_info  HID_API_EXPORT *hid_enumerate(unsigned short vendor_id, unsigned short product_id)
{
    libusb_device **devs = NULL;
    libusb_device *dev = NULL;
    libusb_device_handle *handle;
    ssize_t num_devs;
    int i = 0;

    struct hid_device_info *root = NULL; // return object
    struct hid_device_info *cur_dev = NULL;

    hid_init();

    num_devs = libusb_get_device_list(usb_context, &devs);
    if ((num_devs < 0) || (devs == NULL))
        return NULL;
    while ((dev = devs[i++]) != NULL) {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf_desc = NULL;
        int j, k;
        int interface_num = 0;

        int res = libusb_get_device_descriptor(dev, &desc);
        if (res < 0)
            continue;
        unsigned short dev_vid = desc.idVendor;
        unsigned short dev_pid = desc.idProduct;

        /* HID's are defined at the interface level. */
        if (desc.bDeviceClass != LIBUSB_CLASS_PER_INTERFACE)
            continue;

        res = libusb_get_active_config_descriptor(dev, &conf_desc);
        if (res < 0)
            libusb_get_config_descriptor(dev, 0, &conf_desc);
        if (conf_desc) {
            for (j = 0; j < conf_desc->bNumInterfaces; j++) {
                const struct libusb_interface *intf = &conf_desc->interface[j];
                for (k = 0; k < intf->num_altsetting; k++) {
                    const struct libusb_interface_descriptor *intf_desc;
                    intf_desc = &intf->altsetting[k];
                    if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
                        interface_num = intf_desc->bInterfaceNumber;

                        /* Check the VID/PID against the arguments */
                        if ((vendor_id == 0x0 && product_id == 0x0) ||
                            (vendor_id == dev_vid && product_id == dev_pid)) {
                            struct hid_device_info *tmp;

                            /* VID/PID match. Create the record. */
                            tmp = calloc(1, sizeof(struct hid_device_info));
                            if (tmp == NULL) {
                                hid_free_enumeration(root);
                                IAP1_USBP_LOG(DLT_LOG_ERROR, "resource error");
                                return NULL;
                            }
                            if (cur_dev) {
                                cur_dev->next = tmp;
                            }
                            else {
                                root = tmp;
                            }
                            cur_dev = tmp;

                            /* Fill out the record */
                            cur_dev->next = NULL;
                            cur_dev->path = make_path(dev, interface_num);

                            res = libusb_open(dev, &handle);

                            if (res >= 0) {
                                /* Serial Number */
                                if (desc.iSerialNumber > 0)
                                    cur_dev->serial_number =
                                        get_usb_string(handle, desc.iSerialNumber);

                                /* Manufacturer and Product strings */
                                if (desc.iManufacturer > 0)
                                    cur_dev->manufacturer_string =
                                        get_usb_string(handle, desc.iManufacturer);
                                if (desc.iProduct > 0)
                                    cur_dev->product_string =
                                        get_usb_string(handle, desc.iProduct);

#ifdef INVASIVE_GET_USAGE
                            /*
                            This section is removed because it is too
                            invasive on the system. Getting a Usage Page
                            and Usage requires parsing the HID Report
                            descriptor. Getting a HID Report descriptor
                            involves claiming the interface. Claiming the
                            interface involves detaching the kernel driver.
                            Detaching the kernel driver is hard on the system
                            because it will unclaim interfaces (if another
                            app has them claimed) and the re-attachment of
                            the driver will sometimes change /dev entry names.
                            It is for these reasons that this section is
                            #if 0. For composite devices, use the interface
                            field in the hid_device_info struct to distinguish
                            between interfaces. */
                                int detached = 0;
                                unsigned char data[256];

                                /* Usage Page and Usage */
                                res = libusb_kernel_driver_active(handle, interface_num);
                                if (res == HID_KERNEL_DRV_ACTIVE) {
                                    res = libusb_detach_kernel_driver(handle, interface_num);
                                    if (res < LIBUSB_SUCCESS)
                                        IAP1_USBP_LOG(DLT_LOG_ERROR, "Couldn't detach kernel driver, even though a kernel driver was attached.");
                                    else
                                        detached = 1;
                                }
                                res = libusb_claim_interface(handle, interface_num);
                                if (res >= LIBUSB_SUCCESS) {
                                    /* Get the HID Report Descriptor. */
                                    res = libusb_control_transfer(handle, LIBUSB_ENDPOINT_IN|LIBUSB_RECIPIENT_INTERFACE,
                                                     LIBUSB_REQUEST_GET_DESCRIPTOR, (LIBUSB_DT_REPORT << 8)|interface_num,
                                                     0, data, sizeof(data), 5000);
                                    if (res >= 0) {
                                        unsigned short page=0, usage=0;
                                        /* Parse the usage and usage page
                                           out of the report descriptor. */
                                        get_usage(data, res,  &page, &usage);
                                        cur_dev->usage_page = page;
                                        cur_dev->usage = usage;
                                    }
                                    else{
                                            IAP1_USBP_LOG(DLT_LOG_ERROR, "libusb_control_transfer() for getting the HID report failed with %d", res);
                                    }

                                    /* Release the interface */
                                    res = libusb_release_interface(handle, interface_num);
                                    if (res < 0)
                                        IAP1_USBP_LOG(DLT_LOG_ERROR, "Can't release the interface.");
                                }
                                else{
                                        IAP1_USBP_LOG(DLT_LOG_ERROR, "Can't claim interface %d", res);
                                }

                                /* Re-attach kernel driver if necessary. */
                                if (detached) {
                                    res = libusb_attach_kernel_driver(handle, interface_num);
                                    if (res < 0){
                                        IAP1_USBP_LOG(DLT_LOG_ERROR, "Couldn't re-attach kernel driver.");
                                    }
                                }
#endif /* INVASIVE_GET_USAGE */
                                libusb_close(handle);
                            }
                            /* VID/PID */
                            cur_dev->vendor_id = dev_vid;
                            cur_dev->product_id = dev_pid;

                            /* Release Number */
                            cur_dev->release_number = desc.bcdDevice;

                            /* Interface Number */
                            cur_dev->interface_number = interface_num;
                        }
                    }
                } /* altsettings */
            } /* interfaces */
            libusb_free_config_descriptor(conf_desc);
        }
    }

    libusb_free_device_list(devs, 1);

    return root;
}

void  HID_API_EXPORT hid_free_enumeration(struct hid_device_info *devs)
{
    struct hid_device_info *d = devs;
    while (d) {
        struct hid_device_info *next = d->next;
        if(d->path)
            free(d->path);
        if(d->serial_number)
            free(d->serial_number);
        if(d->manufacturer_string)
            free(d->manufacturer_string);
        if(d->product_string)
            free(d->product_string);
        free(d);
        d = next;
    }
}

hid_device * hid_open(unsigned short vendor_id, unsigned short product_id, const wchar_t *serial_number)
{
    struct hid_device_info *devs, *cur_dev;
    const char *path_to_open = NULL;
    hid_device *handle = NULL;

    devs = hid_enumerate(vendor_id, product_id);
    IAP1_USBP_LOG(DLT_LOG_DEBUG, "hid_enumerate() returns : devs = %p",devs);
    cur_dev = devs;
    while (cur_dev) {
        if (cur_dev->vendor_id == vendor_id &&
            cur_dev->product_id == product_id) {
            if ((serial_number != NULL) && (cur_dev->serial_number != NULL)) {
                if (wcscmp(serial_number, cur_dev->serial_number) == 0) {
                    path_to_open = cur_dev->path;
                    break;
                }
            }
            else {
                path_to_open = cur_dev->path;
                break;
            }
        }
        cur_dev = cur_dev->next;
    }

    if (path_to_open) {
        /* Open the device */
        handle = hid_open_path(path_to_open);
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "hid_open_path() returns : handle = %p",handle);
    }

    hid_free_enumeration(devs);

    return handle;
}

static int hid_submit_interrupt(hid_device *dev)
{
    int res = 0;

    if((dev == NULL) || (dev->transfer == NULL) || (dev->shutdown_thread))
    {
        return HID_API_NG;
    }

    /* Check submit flag for interrupt transfer. */
    if (!dev->submit)
    {
        int num_queued = 0;

        /* Calculate the number of reports */
        if (dev->input_reports != NULL)
        {
            struct input_report *cur = dev->input_reports;
            while (cur->next != NULL)
            {
                cur = cur->next;
                num_queued++;
            }
        }

        /* Check if queue is not full */
        if (num_queued <= HID_API_BUFFER_MAX_QUEUED)
        {
            /* Re-submit the transfer object. */
            res = libusb_submit_transfer(dev->transfer);
            if(LIBUSB_SUCCESS == res)
            {
                /* Enables for submitting interrupt transfer.(flow control) */
                dev->submit = 1;
            }
            else
            {
                /* submit error */
                IAP1_USBP_LOG(DLT_LOG_ERROR, "submit transfer %p failed %d", dev->transfer, res);
                if (LIBUSB_ERROR_NO_DEVICE == res)
                {
                    dev->libusb_no_device = 1;
                }
                
                /* free libusb transfer */
                libusb_free_transfer(dev->transfer);
                dev->transfer = NULL;
                
                dev->shutdown_thread = 1;
            }
        }
    }
    
    return res;

}

static void read_callback(struct libusb_transfer *transfer)
{
    hid_device *dev = NULL;

    /* NULL check */
    if (transfer == NULL)
    {
       IAP1_USBP_LOG(DLT_LOG_ERROR, "transfer is NULL");
       return;
    }
    if (transfer->user_data == NULL)
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR, "transfer->user_data is NULL");
        return;
    }

    dev = transfer->user_data;

    if(dev->transfer != transfer){
        IAP1_USBP_LOG(DLT_LOG_DEBUG, "dev->transfer %p != transfer %p",
                dev->transfer, transfer);
    }

    if (transfer->status == LIBUSB_TRANSFER_COMPLETED)
    {
        pthread_mutex_lock(&dev->mutex);
        dev->submit = 0;
        if(transfer->buffer == NULL)
        {
            IAP1_USBP_LOG(DLT_LOG_ERROR, "read_callback(%p): transfer->buffer is NULL", transfer);
            dev->shutdown_thread = 1;
            pthread_mutex_unlock(&dev->mutex);
            return;
        }

        struct input_report *rpt = malloc(sizeof(*rpt));
        if (rpt != NULL) {
            rpt->data = malloc(transfer->actual_length);
            if(rpt->data == NULL) {
                IAP1_USBP_LOG(DLT_LOG_ERROR, "buffer resource error rpt->data is NULL");
                dev->shutdown_thread = 1;
                pthread_mutex_unlock(&dev->mutex);
                free(rpt);
                return;
            }
        } else {
            IAP1_USBP_LOG(DLT_LOG_ERROR, "buffer resource error rpt is NULL");
            dev->shutdown_thread = 1;
            pthread_mutex_unlock(&dev->mutex);
            return;
        }

        memcpy(rpt->data, transfer->buffer, transfer->actual_length);
        rpt->len = transfer->actual_length;
        rpt->next = NULL;


        /* Attach the new report object to the end of the list. */
        if (dev->input_reports == NULL) {
            /* The list is empty. Put it at the root. */
            dev->input_reports = rpt;
            pthread_cond_signal(&dev->condition);
        }
        else {
            /* Find the end of the list and attach. */
            struct input_report *cur = dev->input_reports;
            int num_queued = 0;
            while (cur->next != NULL) {
                cur = cur->next;
                num_queued++;
            }
            cur->next = rpt; /* add to tail */
        }

        /* Submits transfer for flow control */
        hid_submit_interrupt(dev);
        pthread_mutex_unlock(&dev->mutex);
    }
    else if (transfer->status == LIBUSB_TRANSFER_TIMED_OUT)
    {
        pthread_mutex_lock(&dev->mutex);
        IAP1_USBP_LOG(DLT_LOG_ERROR, "read_callback(%p): Timeout transfer", transfer);
        /* Submits transfer for flow control */
        hid_submit_interrupt(dev);
        pthread_mutex_unlock(&dev->mutex);
    }
    else if (transfer->status == LIBUSB_TRANSFER_ERROR)
    {
        pthread_mutex_lock(&dev->mutex);
        IAP1_USBP_LOG(DLT_LOG_INFO, "read_callback(%p): Transfer error", transfer);
        dev->submit = 0;
        /* Submits transfer for flow control */
        hid_submit_interrupt(dev);
        pthread_mutex_unlock(&dev->mutex);
    }

    else
    {
           IAP1_USBP_LOG(DLT_LOG_DEBUG, "read_callback(%p): transfer->stauts = %d", transfer,transfer->status);
        pthread_mutex_lock(&dev->mutex);
        switch(transfer->status)
        {
            case LIBUSB_TRANSFER_CANCELLED:
            {
                break;
            }
            case LIBUSB_TRANSFER_NO_DEVICE:
            {
                dev->libusb_no_device = 1;
                break;
            }
            default:
            {
                /* in case of such an error it is better to break up */
                dev->libusb_no_device = 1;
                break;
            }
        }

        libusb_free_transfer(transfer);
        dev->transfer = NULL;

        pthread_mutex_unlock(&dev->mutex);
    }
}


static void *read_thread(void *param)
{
    hid_device *dev = NULL;
    unsigned char *buf = NULL;
    size_t length;
    int res = 0;
    struct timeval tv;

    if(param == NULL)
    {
        return NULL;
    }

    dev = param;

    /* Get length for report ID  */
    res = hid_get_maximum_reportID(dev);
    IAP1_USBP_LOG(DLT_LOG_DEBUG, "hid_get_maximum_reportID() returns : res = %d",res);
    if(res < HID_DESC_OK) {
        pthread_barrier_wait(&dev->barrier);
        /* leave function */
        return NULL;
    }
    if(dev->rep_length + 1 < HID_BUFF_MAX){
        length = dev->rep_length + 1; /* Maximum report length + 1 */
    } else {
        length = HID_BUFF_MAX;
    }

    /* Set up the transfer object. */
    buf = malloc(length);
    if (buf == NULL) {
        pthread_barrier_wait(&dev->barrier);
        IAP1_USBP_LOG(DLT_LOG_ERROR,"buffer resource error");
        return NULL;
    }
    dev->transfer = libusb_alloc_transfer(0);
    if(dev->transfer == NULL) {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"libusb_alloc_transfer resource error");
        free(buf);
        pthread_barrier_wait(&dev->barrier);
        return NULL;
    }
    /* libusb shall free the transfer buffer at libusb_free_transfer */
    dev->transfer->flags |= LIBUSB_TRANSFER_FREE_BUFFER;

    libusb_fill_interrupt_transfer(dev->transfer,
        dev->device_handle,
        dev->input_endpoint,
        buf,
        length,
        read_callback,
        dev,
        0/*timeout*/);

    /* Make the first submission. Further submissions are made
       from inside read_callback() */
    dev->submit = 0;
    res = hid_submit_interrupt(dev);
    if(LIBUSB_SUCCESS != res){
        IAP1_USBP_LOG(DLT_LOG_ERROR,"hid_submit_interrupt() returns res = %d ", res);
        free(buf);
        pthread_barrier_wait(&dev->barrier);
        return NULL;
    }

    /* Initialiation is finished */
    dev->thread_ready = 1;

    /* Notify the main thread that the read thread is up and running. */
    pthread_barrier_wait(&dev->barrier);

    /* Handle all the events. */
    while (dev->event_thread_run) {
        tv.tv_sec = 0;
        tv.tv_usec = 100000;
        /* TODO: return value in case of time out ?? */
        res = libusb_handle_events_timeout(usb_context, &tv);
        if (res < 0) {
            /* There was an error. */
            IAP1_USBP_LOG(DLT_LOG_ERROR,"libusb reports error # %d", res);


            /* Break out of this loop only on fatal error.*/
            if (res != LIBUSB_ERROR_BUSY &&
                res != LIBUSB_ERROR_TIMEOUT &&
                res != LIBUSB_ERROR_OVERFLOW &&
                res != LIBUSB_ERROR_INTERRUPTED) {
                break;
            }
        }
        if (dev->shutdown_thread){
            IAP1_USBP_LOG(DLT_LOG_DEBUG,"shutdown_thread = %d. Exit", dev->shutdown_thread);
            break;
        } else if (dev->libusb_no_device){
            IAP1_USBP_LOG(DLT_LOG_DEBUG,"libusb_no_device = %d. Exit", dev->libusb_no_device);
            break;
        } else if (!dev->transfer){
            IAP1_USBP_LOG(DLT_LOG_DEBUG,"!transfer. Exit");
            break;
        } else if (!dev->event_thread_run){
            IAP1_USBP_LOG(DLT_LOG_DEBUG,"event_thread_run = %d", dev->event_thread_run);
        } else{
            /* everything is fine, keep running */
        }
    }

    dev->shutdown_thread = 1;

    /* Now that the read thread is stopping, Wake any threads which are
       waiting on data (in hid_read_timeout()). Do this under a mutex to
       make sure that a thread which is about to go to sleep waiting on
       the condition acutally will go to sleep before the condition is
       signaled. */
    pthread_mutex_lock(&dev->mutex);
    pthread_cond_broadcast(&dev->condition);
    pthread_mutex_unlock(&dev->mutex);

    /* The dev->transfer->buffer and dev->transfer objects are cleaned up
       in hid_close(). They are not cleaned up here because this thread
       could end either due to a disconnect or due to a user
       call to hid_close(). In both cases the objects can be safely
       cleaned up after the call to pthread_join() (in hid_close()), but
       since hid_close() calls libusb_cancel_transfer(), on these objects,
       they can not be cleaned up here. */

/* PRQA: Lint Message 429: The allocation memory is freed in other place. */
    return NULL;        /*lint !e429 */
}


hid_device * HID_API_EXPORT hid_open_path(const char *path)
{
    libusb_device **devs = NULL;
    libusb_device *usb_dev = NULL;
    ssize_t num_devs = 0;
    int res;
    int d = 0;
    int good_open = 0;
    hid_device *dev = NULL;

    if(path == NULL)
    {
        return NULL;
    }

    dev = new_hid_device();
    if(dev == NULL)
    {
        return NULL;
    }

    hid_init();

    num_devs = libusb_get_device_list(usb_context, &devs);
    if((num_devs < 0) || (devs == NULL))
    {
        if(devs != NULL)
        {
            libusb_free_device_list(devs, 1);
        }

        return NULL;
    }
    while ((usb_dev = devs[d++]) != NULL) {
        struct libusb_device_descriptor desc;
        struct libusb_config_descriptor *conf_desc = NULL;
        int i,j,k;
        libusb_get_device_descriptor(usb_dev, &desc);

        if (libusb_get_active_config_descriptor(usb_dev, &conf_desc) < 0)
            continue;
        for (j = 0; j < conf_desc->bNumInterfaces; j++) {
            const struct libusb_interface *intf = &conf_desc->interface[j];
            for (k = 0; k < intf->num_altsetting; k++) {
                const struct libusb_interface_descriptor *intf_desc;
                intf_desc = &intf->altsetting[k];
                if (intf_desc->bInterfaceClass == LIBUSB_CLASS_HID) {
                    char *dev_path = make_path(usb_dev, intf_desc->bInterfaceNumber);
                    if(dev_path == NULL)
                    {
                        break;
                    }

                    if (!strcmp(dev_path, path)) {
                        /* Matched Paths. Open this device */

                        // OPEN HERE //
                        res = libusb_open(usb_dev, &dev->device_handle);
                        if (res < LIBUSB_SUCCESS) {
                            IAP1_USBP_LOG(DLT_LOG_ERROR,"can't open device");
                            free(dev_path);
                            break;
                        }
                        good_open = 1;

                        /* Detach the kernel driver, but only if the
                           device is managed by the kernel */
                        if (libusb_kernel_driver_active(dev->device_handle, intf_desc->bInterfaceNumber) == HID_KERNEL_DRV_ACTIVE) {
                            res = libusb_detach_kernel_driver(dev->device_handle, intf_desc->bInterfaceNumber);
                            if (res < LIBUSB_SUCCESS) {
                                libusb_close(dev->device_handle);
                                IAP1_USBP_LOG(DLT_LOG_ERROR,"Unable to detach Kernel Driver");
                                free(dev_path);
                                good_open = 0;
                                break;
                            }
                        }

                        res = libusb_claim_interface(dev->device_handle, intf_desc->bInterfaceNumber);
                        if (res < LIBUSB_SUCCESS) {
                            IAP1_USBP_LOG(DLT_LOG_ERROR,"can't claim interface %d: %d", intf_desc->bInterfaceNumber, res);
                            free(dev_path);
                            libusb_close(dev->device_handle);
                            good_open = 0;
                            break;
                        }

                        /* Store off the string descriptor indexes */
                        dev->manufacturer_index = desc.iManufacturer;
                        dev->product_index      = desc.iProduct;
                        dev->serial_index       = desc.iSerialNumber;

                        /* Store off the interface number */
                        dev->interface = intf_desc->bInterfaceNumber;

                        /* Find the INPUT and OUTPUT endpoints. An
                           OUTPUT endpoint is not required. */
                        for (i = 0; i < intf_desc->bNumEndpoints; i++) {
                            const struct libusb_endpoint_descriptor *ep
                                = &intf_desc->endpoint[i];

                            /* Determine the type and direction of this
                               endpoint. */
                            int is_interrupt =
                                (ep->bmAttributes & LIBUSB_TRANSFER_TYPE_MASK)
                                  == LIBUSB_TRANSFER_TYPE_INTERRUPT;
                            int is_output =
                                (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
                                  == LIBUSB_ENDPOINT_OUT;
                            int is_input =
                                (ep->bEndpointAddress & LIBUSB_ENDPOINT_DIR_MASK)
                                  == LIBUSB_ENDPOINT_IN;

                            /* Decide whether to use it for intput or output. */
                            if (dev->input_endpoint == 0 &&
                                is_interrupt && is_input) {
                                /* Use this endpoint for INPUT */
                                dev->input_endpoint = ep->bEndpointAddress;
                                dev->input_ep_max_packet_size = ep->wMaxPacketSize;
                            }
                            if (dev->output_endpoint == 0 &&
                                is_interrupt && is_output) {
                                /* Use this endpoint for OUTPUT */
                                dev->output_endpoint = ep->bEndpointAddress;
                            }
                        }

                        /* Thread is not still initialized */
                        dev->thread_ready = 0;
                        dev->event_thread_run = 1;
                        if(!pthread_create(&dev->thread, NULL, read_thread, dev))
                        {
                            // Wait here for the read thread to be initialized.
                            pthread_barrier_wait(&dev->barrier);
                        }

                        /* Initialize failed */
                        if(dev->thread_ready == 0)
                        {
                            dev->event_thread_run = 0;
                            free(dev_path);
                            libusb_close(dev->device_handle);
                            good_open = 0;
                            break;
                        }
                    }
                    free(dev_path);
                }
            }
        }
        libusb_free_config_descriptor(conf_desc);

    }

    libusb_free_device_list(devs, 1);

    // If we have a good handle, return it.
    if (good_open) {
        return dev;
    }
    else {
        // Unable to open any devices.
        free_hid_device(dev);
        return NULL;
    }
}


int HID_API_EXPORT hid_write(hid_device *dev, const unsigned char *data, size_t length)
{
    int res;
    int report_number = data[0];
    int skipped_report_id = 0;

    if((dev == NULL) || (data == NULL) || (length == 0))
    {
        return HID_API_NG;
    }

    /* check if libusb communication was cancelled */
    if ((dev->shutdown_thread) || (dev->libusb_no_device)){
        return HID_API_NOT_CON;
    }

    if (report_number == 0x0) {
        data++;
        length--;
        skipped_report_id = 1;
    }

    if (dev->output_endpoint <= 0) {
        /* No interrput out endpoint. Use the Control Endpoint */
        res = libusb_control_transfer(dev->device_handle,
            LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
            0x09/*HID Set_Report*/,
            (2/*HID output*/ << 8) | report_number,
            dev->interface,
            (unsigned char *)data, length,
            1000/*timeout millis*/);

        if (res == LIBUSB_ERROR_NO_DEVICE) {
            IAP1_USBP_LOG(DLT_LOG_ERROR,"No Device");
            dev->libusb_no_device = 1;
            return HID_API_NOT_CON;
        }
        else if (0 > res) {
            IAP1_USBP_LOG(DLT_LOG_DEBUG,"libusb_control_transfer = %d ", res);
            return HID_API_NG;
        }
        else {
            if (skipped_report_id)
                length++;
        }

        return length;
    }
    else {
        /* Use the interrupt out endpoint */
        int actual_length;
        res = libusb_interrupt_transfer(dev->device_handle,
            dev->output_endpoint,
            (unsigned char*)data,
            length,
            &actual_length, 1000);

        IAP1_USBP_LOG(DLT_LOG_DEBUG,"libusb_interrupt_transfer() returns res = %d",res);
        if (res == LIBUSB_ERROR_NO_DEVICE) {
            IAP1_USBP_LOG(DLT_LOG_ERROR,"No Device");
            dev->libusb_no_device = 1;
            return HID_API_NOT_CON;
        }
        else if (0 > res) {
            return HID_API_NG;
        }
        else {
            if (skipped_report_id)
                length++;
        }

        return actual_length;
    }
}

/* Helper function, to simplify hid_read().
   This should be called with dev->mutex locked. */
static int return_data(hid_device *dev, unsigned char *data, size_t length)
{
    /* Copy the data out of the linked list item (rpt) into the
       return buffer (data), and delete the liked list item. */
    struct input_report *rpt = NULL;
    size_t len = 0;

    if((dev == NULL) || ( (data == NULL) && (length > 0) ) || (dev->input_reports == NULL))
    {
        return HID_API_NG;
    }

    rpt = dev->input_reports;
    len = (length < rpt->len)? length: rpt->len;

    if ( (len > 0) && (data != NULL) )
        memcpy(data, rpt->data, len);
    dev->input_reports = rpt->next;
    free(rpt->data);
    free(rpt);
    return len;
}

static void cleanup_mutex(void *param)
{
    hid_device *dev = param;

    if(dev == NULL)
    {
        return;
    }

/* PRQA: Lint Message 455: Lint describes that mutex has been not locked.
   However there isn't problem because mutex only unlocks when thread was finished. */
    pthread_mutex_unlock(&dev->mutex);      /*lint !e455 */
}


int HID_API_EXPORT hid_read_timeout(hid_device *dev, unsigned char *data, size_t length, int milliseconds)
{
    int bytes_read = HID_API_NG;
    int res = HID_API_OK;

#if 0
    int transferred;
    int res = libusb_interrupt_transfer(dev->device_handle, dev->input_endpoint, data, length, &transferred, 5000);
    IAP1_USBP_LOG(DLT_LOG_DEBUG,"transferred: %d", transferred);
    return transferred;
#endif

    if((dev == NULL) || (data == NULL))
    {
        return HID_API_NG;
    }

    pthread_mutex_lock(&dev->mutex);
    pthread_cleanup_push(&cleanup_mutex, dev);

    bytes_read = HID_API_NG;

    /* There's an input report queued up. Return it. */
    if (dev->input_reports) {
        /* Return the first one */
        bytes_read = return_data(dev, data, length);
        /* Submits transfer for flow control */
        res = hid_submit_interrupt(dev);
        if(res == LIBUSB_ERROR_NO_DEVICE)
        {
            bytes_read = HID_API_NOT_CON;
        }
    }
    /* check if libusb communication was cancelled */
    else if ((dev->shutdown_thread) || (dev->libusb_no_device)) {
        /* This means the device has been disconnected.
           An error code of -2 should be returned. */
        bytes_read = HID_API_NOT_CON;
    }
    else if (milliseconds == -1) {
        /* Blocking */
        while ((!dev->input_reports) && (!dev->shutdown_thread) && (!dev->libusb_no_device)) {
            pthread_cond_wait(&dev->condition, &dev->mutex);
        }
        if ((dev->input_reports) && (!dev->shutdown_thread) && (!dev->libusb_no_device)){
            bytes_read = return_data(dev, data, length);
            /* Submits transfer for flow control */
            res = hid_submit_interrupt(dev);
            if(res == LIBUSB_ERROR_NO_DEVICE)
            {
                bytes_read = HID_API_NOT_CON;
            }
        }
    }
    else if (milliseconds > 0) {
        /* Non-blocking, but called with timeout. */
        struct timespec ts;
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += milliseconds / 1000;
        ts.tv_nsec += (milliseconds % 1000) * 1000000;
        if (ts.tv_nsec >= 1000000000L) {
            ts.tv_sec++;
            ts.tv_nsec -= 1000000000L;
        }

        while ((!dev->input_reports) && (!dev->shutdown_thread) && (!dev->libusb_no_device)) {
            res = pthread_cond_timedwait(&dev->condition, &dev->mutex, &ts);
            if (res == 0) {
                if ((dev->input_reports) && (!dev->shutdown_thread) && (!dev->libusb_no_device)) {
                    bytes_read = return_data(dev, data, length);
                    /* Submits transfer for flow control */
                    res = hid_submit_interrupt(dev);
                    if(res == LIBUSB_ERROR_NO_DEVICE)
                    {
                        bytes_read = HID_API_NOT_CON;
                    }
                    break;
                }

                /* If we're here, there was a spurious wake up
                   or the read thread was shutdown. Run the
                   loop again (ie: don't break). */
            }
            else if (res == ETIMEDOUT) {
                IAP1_USBP_LOG(DLT_LOG_ERROR,"Error Timeout");
                /* Timed out. */
                bytes_read = 0;
                break;
            }
            else {
                /* Error. */
                IAP1_USBP_LOG(DLT_LOG_ERROR,"pthread_cond_timedwait() returns res = %d",res);
                bytes_read = HID_API_NG;
                break;
            }
        }
    }
    else {
        /* Purely non-blocking */
        bytes_read = 0;
    }
    
    pthread_mutex_unlock(&dev->mutex);
    pthread_cleanup_pop(0);

    return bytes_read;
}

int HID_API_EXPORT hid_read(hid_device *dev, unsigned char *data, size_t length)
{
    if((dev == NULL) || (data == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,data);
        return HID_API_NG;
    }

    return hid_read_timeout(dev, data, length, dev->blocking ? -1 : 0);
}

int HID_API_EXPORT hid_set_nonblocking(hid_device *dev, int nonblock)
{
    if(dev == NULL)
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev is NULL");
        return HID_API_NG;
    }

    dev->blocking = !nonblock;

    return HID_API_OK;
}


int HID_API_EXPORT hid_send_feature_report(hid_device *dev, const unsigned char *data, size_t length)
{
    int res = HID_API_NG;
    int skipped_report_id = 0;
    int report_number = data[0];

    if((dev == NULL) || (data == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,data);
        return HID_API_NG;
    }

    if (report_number == 0x0) {
        data++;
        length--;
        skipped_report_id = 1;
    }

    res = libusb_control_transfer(dev->device_handle,
        LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_OUT,
        0x09/*HID set_report*/,
        (3/*HID feature*/ << 8) | report_number,
        dev->interface,
        (unsigned char *)data, length,
        1000/*timeout millis*/);
        IAP1_USBP_LOG(DLT_LOG_DEBUG,"libusb_control_transfer returns res = %d",res);

    if (res == LIBUSB_ERROR_NO_DEVICE) {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"libusb_control_transfer returns LIBUSB_ERROR_NO_DEVICE ");
        dev->libusb_no_device = 1;
        return HID_API_NOT_CON;
    }
    else if (0 > res) {
        return HID_API_NG;
    }
    else {
        /* Account for the report ID */
        if (skipped_report_id)
            length++;
    }

    return length;
}

int HID_API_EXPORT hid_get_feature_report(hid_device *dev, unsigned char *data, size_t length)
{
    int res = HID_API_NG;
    int skipped_report_id = 0;
    int report_number = data[0];

    if((dev == NULL) || (data == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,data);
        return HID_API_NG;
    }

    if (report_number == 0x0) {
        /* Offset the return buffer by 1, so that the report ID
           will remain in byte 0. */
        data++;
        length--;
        skipped_report_id = 1;
    }
    res = libusb_control_transfer(dev->device_handle,
        LIBUSB_REQUEST_TYPE_CLASS|LIBUSB_RECIPIENT_INTERFACE|LIBUSB_ENDPOINT_IN,
        0x01/*HID get_report*/,
        (3/*HID feature*/ << 8) | report_number,
        dev->interface,
        (unsigned char *)data, length,
        1000/*timeout millis*/);
        IAP1_USBP_LOG(DLT_LOG_DEBUG,"libusb_control_transfer returns res = %d",res);

    if (res == LIBUSB_ERROR_NO_DEVICE) {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"libusb_control_transfer returns LIBUSB_ERROR_NO_DEVICE");
        dev->libusb_no_device = 1;
        return HID_API_NOT_CON;
    }
    else if (0 > res) {
        return HID_API_NG;
    }
    else {
        if (skipped_report_id)
            res++;
    }

    return res;
}


void HID_API_EXPORT hid_close(hid_device *dev)
{
    if (!dev)
        return;

    /* Cause read_thread() to stop. */
    dev->event_thread_run = 0;
    if (NULL != dev->device_handle){
        int res =0;

        /* release the interface */
        res = libusb_release_interface(dev->device_handle, dev->interface);
        IAP1_USBP_LOG(DLT_LOG_DEBUG,"libusb_control_transfer returns res = %d",res);
        if (LIBUSB_SUCCESS != res){
            IAP1_USBP_LOG(DLT_LOG_ERROR,"libusb_release_interface = %d", res);
        }

        /* Close the handle */
        libusb_close(dev->device_handle);
        dev->device_handle = NULL;
    } else{
        IAP1_USBP_LOG(DLT_LOG_ERROR,"no libusb device handle open");
    }

    /* Wait for read_thread() to end. */
    pthread_join(dev->thread, NULL);

    if (NULL != dev->transfer){
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev->transfer was not freed!");
        /* Clean up the Transfer objects allocated in read_thread().
         * dev->transfer->buffer will be freed inside the libusb. */
        libusb_free_transfer(dev->transfer);
        dev->transfer = NULL;
    }

    pthread_mutex_lock(&dev->mutex);
    /* Clear out the queue of received reports. */
    while (dev->input_reports) {
        return_data(dev, NULL, 0);
    }
    pthread_mutex_unlock(&dev->mutex);

    free_hid_device(dev);
}


int HID_API_EXPORT_CALL hid_get_manufacturer_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    if((dev == NULL) || (string == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,string);
        return HID_API_NG;
    }

    return hid_get_indexed_string(dev, dev->manufacturer_index, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_product_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    if((dev == NULL) || (string == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,string);
        return HID_API_NG;
    }

    return hid_get_indexed_string(dev, dev->product_index, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_serial_number_string(hid_device *dev, wchar_t *string, size_t maxlen)
{
    if((dev == NULL) || (string == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,string);
        return HID_API_NG;
    }

    return hid_get_indexed_string(dev, dev->serial_index, string, maxlen);
}

int HID_API_EXPORT_CALL hid_get_indexed_string(hid_device *dev, int string_index, wchar_t *string, size_t maxlen)
{
    wchar_t *str;

    if((dev == NULL) || (string == NULL))
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev = %p data = %p",dev,string);
        return HID_API_NG;
    }

    str = get_usb_string(dev->device_handle, string_index);
    if (str) {
        wcsncpy(string, str, maxlen);
        string[maxlen-1] = L'\0';
        free(str);
        return HID_API_OK;
    }
    else
        return HID_API_NG;
}

#if 0
HID_API_EXPORT const wchar_t * HID_API_CALL  hid_error(hid_device *dev)
{
    return NULL;
}
#endif /* if 0 */

/* The special function for ipod_ctrl */
void* HID_API_EXPORT HID_API_CALL HidGetReportInformation(hid_device *dev)
{
    if(dev == NULL)
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev is NULL");
        return NULL;
    }

    return (void *)&dev->rep_info;
}

int HID_API_EXPORT HID_API_CALL hid_get_maximum_reportID(hid_device *dev)
{
    int             res = HID_DESC_ERROR;
    unsigned int    i = 0;
    unsigned char   *tmpBuffer;
    size_t          max_length = 0;

    if(dev == NULL)
    {
        IAP1_USBP_LOG(DLT_LOG_ERROR,"dev is NULL");
        return HID_API_NG;
    }

    max_length = dev->input_ep_max_packet_size;

    tmpBuffer = calloc(1, HID_BUFF_MAX);                 /* allocate for report descriptor buffer */
    if(tmpBuffer != NULL) {
        res = libusb_control_transfer(dev->device_handle,
                                      0x81,
                                      LIBUSB_REQUEST_GET_DESCRIPTOR,
                                      (LIBUSB_DT_REPORT << 8),
                                      2,
                                      tmpBuffer,
                                      HID_BUFF_MAX,
                                      CTRL_TRANS_TOUT);      /* timeout 1 sec */
        if (res > 0)
        {
            dev->rep_info.size = res;
            dev->rep_info.buffer = tmpBuffer;

            res = hid_report(&dev->rep_info);
            if(res == HID_DESC_OK)
            {
                dev->rep_length = 0;
                for (i = 0; i < dev->rep_info.act_sz_in; i++)
                {
                    if(dev->rep_length < dev->rep_info.rep[i].len) {
                        dev->rep_length = (int)dev->rep_info.rep[i].len;
                    }
                    if((dev->rep_info.rep[i].len + 1) % max_length == 0) {
                        dev->rep_length = (int)dev->rep_info.rep[i].len;
                        break;
                    }
                }
            }
        }
        free(tmpBuffer);
    }

    return res;
}

/* PRQA: Lint Message 754: Lint describes that "*name" of the structure does not use
  but "*name" is necessary in lang_map_entry because the language code is searched by "*name". */
/*lint -esym(754, lang_map_entry::name) */
struct lang_map_entry {
    const char *name;
    const char *string_code;
    uint16_t usb_code;
};

#define LANG(name,code,usb_code) { name, code, usb_code }
static struct lang_map_entry lang_map[] = {
    LANG("Afrikaans", "af", 0x0436),
    LANG("Albanian", "sq", 0x041C),
    LANG("Arabic - United Arab Emirates", "ar_ae", 0x3801),
    LANG("Arabic - Bahrain", "ar_bh", 0x3C01),
    LANG("Arabic - Algeria", "ar_dz", 0x1401),
    LANG("Arabic - Egypt", "ar_eg", 0x0C01),
    LANG("Arabic - Iraq", "ar_iq", 0x0801),
    LANG("Arabic - Jordan", "ar_jo", 0x2C01),
    LANG("Arabic - Kuwait", "ar_kw", 0x3401),
    LANG("Arabic - Lebanon", "ar_lb", 0x3001),
    LANG("Arabic - Libya", "ar_ly", 0x1001),
    LANG("Arabic - Morocco", "ar_ma", 0x1801),
    LANG("Arabic - Oman", "ar_om", 0x2001),
    LANG("Arabic - Qatar", "ar_qa", 0x4001),
    LANG("Arabic - Saudi Arabia", "ar_sa", 0x0401),
    LANG("Arabic - Syria", "ar_sy", 0x2801),
    LANG("Arabic - Tunisia", "ar_tn", 0x1C01),
    LANG("Arabic - Yemen", "ar_ye", 0x2401),
    LANG("Armenian", "hy", 0x042B),
    LANG("Azeri - Latin", "az_az", 0x042C),
    LANG("Azeri - Cyrillic", "az_az", 0x082C),
    LANG("Basque", "eu", 0x042D),
    LANG("Belarusian", "be", 0x0423),
    LANG("Bulgarian", "bg", 0x0402),
    LANG("Catalan", "ca", 0x0403),
    LANG("Chinese - China", "zh_cn", 0x0804),
    LANG("Chinese - Hong Kong SAR", "zh_hk", 0x0C04),
    LANG("Chinese - Macau SAR", "zh_mo", 0x1404),
    LANG("Chinese - Singapore", "zh_sg", 0x1004),
    LANG("Chinese - Taiwan", "zh_tw", 0x0404),
    LANG("Croatian", "hr", 0x041A),
    LANG("Czech", "cs", 0x0405),
    LANG("Danish", "da", 0x0406),
    LANG("Dutch - Netherlands", "nl_nl", 0x0413),
    LANG("Dutch - Belgium", "nl_be", 0x0813),
    LANG("English - Australia", "en_au", 0x0C09),
    LANG("English - Belize", "en_bz", 0x2809),
    LANG("English - Canada", "en_ca", 0x1009),
    LANG("English - Caribbean", "en_cb", 0x2409),
    LANG("English - Ireland", "en_ie", 0x1809),
    LANG("English - Jamaica", "en_jm", 0x2009),
    LANG("English - New Zealand", "en_nz", 0x1409),
    LANG("English - Phillippines", "en_ph", 0x3409),
    LANG("English - Southern Africa", "en_za", 0x1C09),
    LANG("English - Trinidad", "en_tt", 0x2C09),
    LANG("English - Great Britain", "en_gb", 0x0809),
    LANG("English - United States", "en_us", 0x0409),
    LANG("Estonian", "et", 0x0425),
    LANG("Farsi", "fa", 0x0429),
    LANG("Finnish", "fi", 0x040B),
    LANG("Faroese", "fo", 0x0438),
    LANG("French - France", "fr_fr", 0x040C),
    LANG("French - Belgium", "fr_be", 0x080C),
    LANG("French - Canada", "fr_ca", 0x0C0C),
    LANG("French - Luxembourg", "fr_lu", 0x140C),
    LANG("French - Switzerland", "fr_ch", 0x100C),
    LANG("Gaelic - Ireland", "gd_ie", 0x083C),
    LANG("Gaelic - Scotland", "gd", 0x043C),
    LANG("German - Germany", "de_de", 0x0407),
    LANG("German - Austria", "de_at", 0x0C07),
    LANG("German - Liechtenstein", "de_li", 0x1407),
    LANG("German - Luxembourg", "de_lu", 0x1007),
    LANG("German - Switzerland", "de_ch", 0x0807),
    LANG("Greek", "el", 0x0408),
    LANG("Hebrew", "he", 0x040D),
    LANG("Hindi", "hi", 0x0439),
    LANG("Hungarian", "hu", 0x040E),
    LANG("Icelandic", "is", 0x040F),
    LANG("Indonesian", "id", 0x0421),
    LANG("Italian - Italy", "it_it", 0x0410),
    LANG("Italian - Switzerland", "it_ch", 0x0810),
    LANG("Japanese", "ja", 0x0411),
    LANG("Korean", "ko", 0x0412),
    LANG("Latvian", "lv", 0x0426),
    LANG("Lithuanian", "lt", 0x0427),
    LANG("F.Y.R.O. Macedonia", "mk", 0x042F),
    LANG("Malay - Malaysia", "ms_my", 0x043E),
    LANG("Malay - Brunei", "ms_bn", 0x083E),
    LANG("Maltese", "mt", 0x043A),
    LANG("Marathi", "mr", 0x044E),
    LANG("Norwegian - Bokml", "no_no", 0x0414),
    LANG("Norwegian - Nynorsk", "no_no", 0x0814),
    LANG("Polish", "pl", 0x0415),
    LANG("Portuguese - Portugal", "pt_pt", 0x0816),
    LANG("Portuguese - Brazil", "pt_br", 0x0416),
    LANG("Raeto-Romance", "rm", 0x0417),
    LANG("Romanian - Romania", "ro", 0x0418),
    LANG("Romanian - Republic of Moldova", "ro_mo", 0x0818),
    LANG("Russian", "ru", 0x0419),
    LANG("Russian - Republic of Moldova", "ru_mo", 0x0819),
    LANG("Sanskrit", "sa", 0x044F),
    LANG("Serbian - Cyrillic", "sr_sp", 0x0C1A),
    LANG("Serbian - Latin", "sr_sp", 0x081A),
    LANG("Setsuana", "tn", 0x0432),
    LANG("Slovenian", "sl", 0x0424),
    LANG("Slovak", "sk", 0x041B),
    LANG("Sorbian", "sb", 0x042E),
    LANG("Spanish - Spain (Traditional)", "es_es", 0x040A),
    LANG("Spanish - Argentina", "es_ar", 0x2C0A),
    LANG("Spanish - Bolivia", "es_bo", 0x400A),
    LANG("Spanish - Chile", "es_cl", 0x340A),
    LANG("Spanish - Colombia", "es_co", 0x240A),
    LANG("Spanish - Costa Rica", "es_cr", 0x140A),
    LANG("Spanish - Dominican Republic", "es_do", 0x1C0A),
    LANG("Spanish - Ecuador", "es_ec", 0x300A),
    LANG("Spanish - Guatemala", "es_gt", 0x100A),
    LANG("Spanish - Honduras", "es_hn", 0x480A),
    LANG("Spanish - Mexico", "es_mx", 0x080A),
    LANG("Spanish - Nicaragua", "es_ni", 0x4C0A),
    LANG("Spanish - Panama", "es_pa", 0x180A),
    LANG("Spanish - Peru", "es_pe", 0x280A),
    LANG("Spanish - Puerto Rico", "es_pr", 0x500A),
    LANG("Spanish - Paraguay", "es_py", 0x3C0A),
    LANG("Spanish - El Salvador", "es_sv", 0x440A),
    LANG("Spanish - Uruguay", "es_uy", 0x380A),
    LANG("Spanish - Venezuela", "es_ve", 0x200A),
    LANG("Southern Sotho", "st", 0x0430),
    LANG("Swahili", "sw", 0x0441),
    LANG("Swedish - Sweden", "sv_se", 0x041D),
    LANG("Swedish - Finland", "sv_fi", 0x081D),
    LANG("Tamil", "ta", 0x0449),
    LANG("Tatar", "tt", 0X0444),
    LANG("Thai", "th", 0x041E),
    LANG("Turkish", "tr", 0x041F),
    LANG("Tsonga", "ts", 0x0431),
    LANG("Ukrainian", "uk", 0x0422),
    LANG("Urdu", "ur", 0x0420),
    LANG("Uzbek - Cyrillic", "uz_uz", 0x0843),
    LANG("Uzbek - Latin", "uz_uz", 0x0443),
    LANG("Vietnamese", "vi", 0x042A),
    LANG("Xhosa", "xh", 0x0434),
    LANG("Yiddish", "yi", 0x043D),
    LANG("Zulu", "zu", 0x0435),
    LANG(NULL, NULL, 0x0),
};

uint16_t get_usb_code_for_current_locale(void)
{
    char *locale;
    char search_string[64];
    char *ptr;

    /* Get the current locale. */
    locale = setlocale(0, NULL);
    if (!locale)
        return 0x0;

    /* Make a copy of the current locale string. */
    strncpy(search_string, locale, sizeof(search_string));
    search_string[sizeof(search_string)-1] = '\0';

    /* Chop off the encoding part, and make it lower case. */
    ptr = search_string;
    while (*ptr) {
        *ptr = tolower(*ptr);
        if (*ptr == '.') {
            *ptr = '\0';
            break;
        }
        ptr++;
    }

    /* Find the entry which matches the string code of our locale. */
    struct lang_map_entry *lang = lang_map;
    while (lang->string_code) {
        if (!strcmp(lang->string_code, search_string)) {
            return lang->usb_code;
        }
        lang++;
    }

    /* There was no match. Find with just the language only. */
    /* Chop off the variant. Chop it off at the '_'. */
    ptr = search_string;
    while (*ptr) {
        *ptr = tolower(*ptr);
        if (*ptr == '_') {
            *ptr = '\0';
            break;
        }
        ptr++;
    }

#if 0 // TODO: Do we need this?
    /* Find the entry which matches the string code of our language. */
    lang = lang_map;
    while (lang->string_code) {
        if (!strcmp(lang->string_code, search_string)) {
            return lang->usb_code;
        }
        lang++;
    }
#endif

    /* Found nothing. */
    return 0x0;
}
