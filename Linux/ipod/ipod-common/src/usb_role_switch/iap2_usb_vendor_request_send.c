/**
* \file: iap2_usb_vendor_request_send.c
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

#include "iap2_usb_vendor_request.h"
#include <libusb.h>
#include <stdio.h>
#include <memory.h>

/* TODO libusb_error_name is available in libusb with GCC 4.8.3.
 *      GCC 4.8.3 is currently not used. */
static void _log_usb_error(char* log_msg, int error_code)
{
    log_msg = log_msg;
    error_code = error_code;

    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "%s failed with error %d",
            log_msg, error_code);
/*
    IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "%s failed with error %d: %s",
            log_msg, error_code, libusb_error_name(error_code));
*/
}


/* internal data structure */
typedef struct
{
    int libusb_status;
    libusb_device** devices;
    ssize_t device_count;
    libusb_context *usb_context;
    libusb_device_handle* target_device_handle;
} _USBVendorRequest;

/* private functions */
static iAP2USBRoleSwitchStatus _init(_USBVendorRequest* thisPtr);
static void _release(_USBVendorRequest* thisPtr);
static iAP2USBRoleSwitchStatus _openDevice(_USBVendorRequest* thisPtr, U16 vid, U16 pid,
        const char* serial);
static iAP2USBRoleSwitchStatus _sendVendorRequest(_USBVendorRequest* thisPtr,
        iAP2USBRoleSwitchMode mode);
static iAP2USBRoleSwitchStatus _getDeviceList(_USBVendorRequest* thisPtr);

IAP2_USB_EXPORTED_SYMBOL iAP2USBRoleSwitchStatus iAP2USBVendorRequest_Send(U16 vid, U16 pid, const char* serial,
        iAP2USBRoleSwitchMode mode)
{
    iAP2USBRoleSwitchStatus status;
    _USBVendorRequest thisVendorRequest;

    /* init libusb and structures */
    status = _init(&thisVendorRequest);

    /* get device list */
    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        status = _getDeviceList(&thisVendorRequest);
    }

    /* find and open device */
    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        status = _openDevice(&thisVendorRequest, vid, pid, serial);
    }

    /* send vendor request */
    if (status == IAP2_USB_ROLE_SWITCH_OK)
    {
        status = _sendVendorRequest(&thisVendorRequest, mode);
    }

    /* close all and release */
    _release(&thisVendorRequest);
    return status;
}

/* ====== private functions ====== */

static iAP2USBRoleSwitchStatus _init(_USBVendorRequest* thisPtr)
{
    memset(thisPtr, 0, sizeof(_USBVendorRequest));

    if (0 != (thisPtr->libusb_status = libusb_init(&thisPtr->usb_context)))
    {
        _log_usb_error("libusb_init", thisPtr->libusb_status);
        return IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED; /* ======== leave function ======== */
    }

    return IAP2_USB_ROLE_SWITCH_OK;
}

static void _release(_USBVendorRequest* thisPtr)
{
    /* close device */
    if (thisPtr->target_device_handle != NULL)
    {
        libusb_close(thisPtr->target_device_handle);
    }

    /* free device list */
    if (thisPtr->devices != NULL)
    {
        libusb_free_device_list(thisPtr->devices, 1 /* unref devices by 1 */);
    }

    /* close libusb */
    if ((thisPtr->libusb_status == 0)
        &&(thisPtr->usb_context))
    {
        libusb_exit(thisPtr->usb_context);
        thisPtr->usb_context = NULL;
    }

    memset(thisPtr, 0, sizeof(_USBVendorRequest));
}

static iAP2USBRoleSwitchStatus _openDevice(_USBVendorRequest* thisPtr, U16 vid, U16 pid,
        const char* serial)
{
    ssize_t i;
    unsigned char device_serial[IAP2_USB_MAX_STRING_DESCRIPTOR_LENGTH];

    /* go through all device descriptors */
    for (i = 0; (i < thisPtr->device_count) && (thisPtr->devices[i] != NULL); i++)
    {
        struct libusb_device_descriptor descriptor;
        int status = libusb_get_device_descriptor(thisPtr->devices[i], &descriptor);
        if (status == 0)
        {
            /* check vendor id and product id */
            if (descriptor.idVendor == vid &&
                    ((descriptor.idProduct & 0xff00) == (pid & 0xff00)))
            {
                /* open device */
                status = libusb_open(thisPtr->devices[i], &thisPtr->target_device_handle);
                if (status != 0)
                {
                    _log_usb_error("_openDevice():  libusb_open", status);
                }

                /* TODO: can we check the serial number before opening the device? */
                /* check serial number (if available) */
                if (status == 0)
                {
                    status = libusb_get_string_descriptor_ascii(thisPtr->target_device_handle,
                            descriptor.iSerialNumber, device_serial,
                            IAP2_USB_MAX_STRING_DESCRIPTOR_LENGTH);
                    int serial_cmp = -1;
                    if ((serial != NULL) && (status > 0)){
                        serial_cmp = strncmp(serial, (const char*)&device_serial[0], strlen(serial));
                    } else{
                        serial_cmp = 0;
                    }
                    if (0 == serial_cmp)
                    {
                        /* serial match, do nothing */
                    }
                    else
                    {
                        /* wrong serial number, close device again */
                        libusb_close(thisPtr->target_device_handle);
                        thisPtr->target_device_handle = NULL;
                    }
                }

                /* device found */
                if (thisPtr->target_device_handle != NULL)
                {
                    pid = descriptor.idProduct; /* complete our pid */
                    break; /* ======== break loop ======== */
                }
            }
        }
        else
        {
            /* error, but continue anyway */
            _log_usb_error("_openDevice():  libusb_get_device_descriptor", status);
        }
    }

    if (thisPtr->target_device_handle == NULL)
    {
        /* device not found */
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR,
                "_openDevice():  USB device with VID=%04x, PID=%04x and serial=%s not found", vid, pid, serial);
        return IAP2_USB_ROLE_SWITCH_DEVICE_NOT_FOUND;
    }

    /* device found and open */
    return IAP2_USB_ROLE_SWITCH_OK;
}

static iAP2USBRoleSwitchStatus _sendVendorRequest(_USBVendorRequest* thisPtr,
        iAP2USBRoleSwitchMode mode)
{
    int status = 0;

    if (mode == IAP2_USB_ROLE_SWITCH_WITH_DIGITAL_IPOD_OUT)
    {
        status = libusb_control_transfer(thisPtr->target_device_handle,
                IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_WITH_DIPO,
                IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_TIMEOUT_MSEC);
    }
    else
    {
        status = libusb_control_transfer(thisPtr->target_device_handle,
                IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_WITHOUT_DIPO,
                IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_TIMEOUT_MSEC);
    }

    if (status == LIBUSB_ERROR_PIPE)
    {
        IAP2USBROLESWITCHDLTLOG(DLT_LOG_ERROR, "vendor request not supported");
        return IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_NOT_SUPPORTED; /* ======== leave function ==== */
    }
    else if (status != 0)
    {
        _log_usb_error("libusb_control_transfer", status);
        return IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED; /* ======== leave function ======== */
    }

    return IAP2_USB_ROLE_SWITCH_OK;
}

static iAP2USBRoleSwitchStatus _getDeviceList(_USBVendorRequest* thisPtr)
{
    thisPtr->device_count = libusb_get_device_list(thisPtr->usb_context, &thisPtr->devices);
    if (thisPtr->device_count < 0 || *thisPtr->devices == NULL)
    {
        _log_usb_error("libusb_get_device_list", thisPtr->device_count);
        return IAP2_USB_ROLE_SWITCH_VENDOR_REQUEST_FAILED; /* ======== leave function ======== */
    }

    return IAP2_USB_ROLE_SWITCH_OK;
}
