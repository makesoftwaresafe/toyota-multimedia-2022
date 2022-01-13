/************************************************************************
 *
 * \file: spi_usb_discoverer_device.cpp
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * <brief description>.
 * <detailed description>
 * \component: SPI Discovery
 *
 * \author: D. Girnus / ADIT/SW2 / dgirnus@de.adit-jv.com
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


/* *************  includes  ************* */

#include <adit_logging.h>
#include "spi_usb_discoverer_device.h"
#include <assert.h>

/* *************  defines  ************* */

LOG_IMPORT_CONTEXT(spi_usb_discoverer)

/* *************  functions  ************* */

UsbDevice::UsbDevice(std::shared_ptr<t_usbDeviceInformation> inUsbDevInfo) {
    assert(inUsbDevInfo);
    mDevParam = inUsbDevInfo;

    mpUsbContext = NULL;
    mpUsbDeviceHandle = NULL;
    mUsbInterfaceClaimed = false;
    mAoapSupportChecked = false;
    init();
}

UsbDevice::~UsbDevice() {

    if (true == isOpen()) {
        close(true);
    }

    if (mpUsbContext) {
        libusb_exit(mpUsbContext);
        mpUsbContext = NULL;
    }

    mAoapSupportChecked = false;
}

void UsbDevice::init(void) {

    if (!mpUsbContext) {
        int32_t res = libusb_init(&mpUsbContext);
        if (LIBUSB_SUCCESS != res) {
            LOG_ERROR((spi_usb_discoverer, "init() Failed to initialize libusb (res=%d) for device %s",
                       res, to_string(mDevParam).c_str()));
        }
    }
}

int UsbDevice::checkAoapSupport(void) {
    int32_t res = SPI_USB_DISCOVERY_ERROR;

    /* will be set to true, if there is a device
       which supports AOA protocol */
    mDevParam->aoapSupported = false;
    /* indicate that check for AOAP support was done */
    mAoapSupportChecked = true;

    /* check for Apple device which does not support AOAP */
    if ((mDevParam->vendorId == APPLE_VENDOR_ID) && ((mDevParam->productId & APPLE_PRODUCT_ID_MIN) == APPLE_PRODUCT_ID_MIN)) {
        LOGD_DEBUG((spi_usb_discoverer, "checkAoapSupport() device %s is an Apple device",
                to_string(mDevParam).c_str()));
        return SPI_USB_DISCOVERY_NO_AOAP_SUPPORT;
    }
    /* https://source.android.com/accessories/aoa.html
     * The vendor ID should match Google's ID (0x18D1)
     * and the product ID should be 0x2D00 or 0x2D01 (AOAP 2.0 0x2D00 - 0x2D05)
     * if the device is already in accessory mode. */
    if ((mDevParam->vendorId == 0x18d1) &&
        ((mDevParam->productId >= 0x2d00) && (mDevParam->productId <= 0x2d05))) {
        LOGD_DEBUG((spi_usb_discoverer, "checkAoapSupport() device %s is in AOAP. Don't send vendor request.",
                to_string(mDevParam).c_str()));

        mDevParam->aoapSupported = true;
        return SPI_USB_DISCOVERY_SUCCESS;
    }

    /* Open the device */
    if (SPI_USB_DISCOVERY_SUCCESS == (res = open())) {
        /* Get the underlying libusb_device for the libusb_device_handle. */
        libusb_device *usb_dev = libusb_get_device(mpUsbDeviceHandle);
        if (NULL != usb_dev) {
            struct libusb_config_descriptor *conf_desc = NULL;

            /* Get the USB configuration descriptor for the currently active configuration */
            if (LIBUSB_SUCCESS == (res = libusb_get_active_config_descriptor(usb_dev, &conf_desc))) {
                /* check is secondary since several mobile device have no such interface but support AOAP */
                const struct libusb_interface_descriptor *idesc = parseUsbInterfaces(conf_desc);

                uint8_t buffer[2];
                /* send vendor request to get protocol version */
                res = libusb_control_transfer(mpUsbDeviceHandle,
                                                  USB_DISCOVERY_HOST_TO_DEVICE_TYPE, USB_DISCOVERY_GET_AOA_PROTOCOL,
                                                  0, 0, (uint8_t*) buffer, 2, 1000);
                if ((res >= LIBUSB_SUCCESS) && (buffer[0] >= 1)) {
                    LOG_INFO((spi_usb_discoverer, "checkAoapSupport() device %s supports AOA Protocol version %d.%d",
                            to_string(mDevParam).c_str(), buffer[0], buffer[1]));
                    if (idesc != nullptr) {
                        mDevParam->interface = idesc->bInterfaceNumber;
                    }
                    mDevParam->aoapSupported = true;
                    res = SPI_USB_DISCOVERY_SUCCESS;
                } else {
                    if (res >= LIBUSB_SUCCESS) {
                        LOG_INFO((spi_usb_discoverer, "checkAoapSupport() Device %s does not support AOA Protocol.",
                                to_string(mDevParam).c_str()));
                        res = SPI_USB_DISCOVERY_NO_AOAP_SUPPORT;
                    } else if ((res == LIBUSB_ERROR_PIPE) || (res == LIBUSB_ERROR_TIMEOUT)) {
                        LOG_WARN((spi_usb_discoverer, "checkAoapSupport() Device %s does not support control request =%d",
                            to_string(mDevParam).c_str(), res));
                        res = SPI_USB_DISCOVERY_NO_AOAP_SUPPORT;
                    } else if (res == LIBUSB_ERROR_NO_DEVICE) {
                        LOG_ERROR((spi_usb_discoverer, "checkAoapSupport() Device %s has been disconnected =%d",
                            to_string(mDevParam).c_str(), res));
                        res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;
                    } else {
                        LOG_ERROR((spi_usb_discoverer, "checkAoapSupport() Get AOA Protocol version from device %s failed =%d",
                            to_string(mDevParam).c_str(), res));
                        res = SPI_USB_DISCOVERY_ERROR;
                    }
                }

                libusb_free_config_descriptor(conf_desc);
            } else {
                LOG_ERROR((spi_usb_discoverer, "checkAoapSupport() libusb_get_active_config_descriptor() for device %s failed=%d",
                        to_string(mDevParam).c_str(), res));
                /* indicate that check for AOAP support was not done */
                mAoapSupportChecked = false;
                res = SPI_USB_DISCOVERY_ERROR;
            }
        } else {
            LOG_ERROR((spi_usb_discoverer, "checkAoapSupport() Get libusb device by libusb_device_handle %p failed",
                    mpUsbDeviceHandle));
            /* indicate that check for AOAP support was not done */
            mAoapSupportChecked = false;
            res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;
        }

        /* close libusb device */
        close(true);
    } else {
        LOG_ERROR((spi_usb_discoverer, "checkAoapSupport() Open device failed=%d", res));
        /* set mAoapSupportChecked to true even if open failed */
        res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;
    }

    return res;
}

int UsbDevice::open() {
    int32_t res = SPI_USB_DISCOVERY_ERROR;
    int32_t i = 0;

    /* bFound is true, if the correct device was found */
    bool bFound = false;

    libusb_device **devs = NULL;
    libusb_device *usb_dev = NULL;
    ssize_t num_devs = 0;

    if (!mpUsbContext) {
        return SPI_USB_DISCOVERY_LIBUSB_ERROR;
    }
    if (!isOpen()) {
        num_devs = libusb_get_device_list(mpUsbContext, &devs);
        if ((num_devs < 0) || (devs == NULL)) {
            return SPI_USB_DISCOVERY_LIBUSB_ERROR;
        }
        LOGD_DEBUG((spi_usb_discoverer, "open() number of usb devices=%zd", num_devs));
        while (((usb_dev = devs[i++]) != NULL) && (true != bFound))
        {
            struct libusb_device_descriptor desc;

            if (0 > libusb_get_device_descriptor(usb_dev, &desc)) {
                /* start next iteration of while() */
                continue;
            }
            /* Check the USB base class to identify USB Hub(s). */
            if ((desc.bDeviceClass == USB_HUB_BASE_CLASS) && (desc.bDeviceSubClass == USB_HUB_SUB_CLASS)) {
                LOGD_VERBOSE((spi_usb_discoverer, "open() Found a USB Hub/Port (USB DevClass=%d,%d,%d)",
                        desc.bDeviceClass, desc.bDeviceSubClass, desc.bDeviceProtocol));
            } else {
                /* check the vendorId and productId */
                if ((mDevParam->vendorId == desc.idVendor)
                    && (mDevParam->productId == desc.idProduct)) {

                    /* open libusb device to check if this is the correct device */
                    if (LIBUSB_SUCCESS == (res = libusb_open(usb_dev, &mpUsbDeviceHandle))) {
                        if (desc.iSerialNumber > 0) {
                            char retrievedSerial[64];
                            int len = libusb_get_string_descriptor_ascii(mpUsbDeviceHandle,
                                                                         desc.iSerialNumber,
                                                                         (unsigned char*) retrievedSerial,
                                                                         sizeof(retrievedSerial));
                            /* furthermore, compare the serial number */
                            if ((len < 0) || (SPI_USB_DISCOVERY_SUCCESS != mDevParam->serial.compare(retrievedSerial))) {
                                LOGD_DEBUG((spi_usb_discoverer, "open() SerialNumber=%s does not match. Start next iteration",
                                        &retrievedSerial[0]));
                                res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;
                            } else {
                                bFound = true;
                            }
                        } else {
                            res = SPI_USB_DISCOVERY_ERROR;
                            LOG_ERROR((spi_usb_discoverer, "open() No iSerialNumber available"));
                        }
                    } else {
                       LOG_ERROR((spi_usb_discoverer, "open() libusb_open() of device 0x%X:0x%X failed = %d",
                                  desc.idVendor, desc.idProduct, res));
                    }
                } else {
                    LOGD_DEBUG((spi_usb_discoverer, "open() vendorId & productId (0x%X:0x%X) does not match to (0x%X:0x%X). Start next iteration",
                            desc.idVendor, desc.idProduct, mDevParam->vendorId, mDevParam->productId));
                    res = SPI_USB_DISCOVERY_ERROR;
                }
            }
            if (true != bFound) {
                /* did not found our device but libusb was opened, close it */
                close(true);
            }
        }/* while-loop */
        /* free and unreference all devices */
        libusb_free_device_list(devs, 1);

        if (true != bFound) {
            /* did not found the correct device */
            LOG_WARN((spi_usb_discoverer, "open() Could not find device %s",
                    to_string(mDevParam).c_str()));
            res = SPI_USB_DISCOVERY_NO_DEVICE_FOUND;
        } else {
            /* found the correct device and it supports AOA protocol */
            LOGD_DEBUG((spi_usb_discoverer, "open() Found and open device %s ",
                    to_string(mDevParam).c_str()));

            res = SPI_USB_DISCOVERY_SUCCESS;
        }
    } else {
        LOGD_DEBUG((spi_usb_discoverer, "open() libusb_device_handle %p already open", mpUsbDeviceHandle));
        res = SPI_USB_DISCOVERY_SUCCESS;
    }

    return res;
}

void UsbDevice::close(bool usbClose) {
    int res = SPI_USB_DISCOVERY_ERROR;

    if (!mpUsbContext) {
        return;
    }
    if (true == usbClose) {
        if (mpUsbDeviceHandle) {
            if (SPI_USB_DISCOVERY_SUCCESS != (res = releaseUsbInterface(mDevParam->interface))) {
                LOGD_DEBUG((spi_usb_discoverer, "close() releaseUsbInterface(%d) failed = %d", mDevParam->interface, res));
            }
            libusb_close(mpUsbDeviceHandle);
            mpUsbDeviceHandle = NULL;
        }
    } else {
        LOG_INFO((spi_usb_discoverer, "close() usbClose set to false."));
        mpUsbDeviceHandle = NULL;
    }
}

int UsbDevice::claimUsbInterface(uint32_t interface) {
    int res = SPI_USB_DISCOVERY_ERROR;

    if ((!mpUsbContext) || (!mpUsbDeviceHandle)) {
        LOG_ERROR((spi_usb_discoverer, "claimUsbInterface() mpUsbContext=%p, mpUsbDeviceHandle=%p are NULL",
                mpUsbContext, mpUsbDeviceHandle));
        return res;
    }
    if (isOpen()) {
        /* Detach the kernel driver, but only if the device is managed by the kernel
         * 0 if no kernel driver is active
         * 1 if a kernel driver is active */
        if (libusb_kernel_driver_active(mpUsbDeviceHandle, interface) == 1) {
            res = libusb_detach_kernel_driver(mpUsbDeviceHandle, interface);
            if (res != LIBUSB_SUCCESS) {
                LOG_ERROR((spi_usb_discoverer, "claimUsbInterface() libusb_detach_kernel_driver() failed = %d", res));
            } else {
                LOG_INFO((spi_usb_discoverer, "claimUsbInterface() libusb_detach_kernel_driver() success"));
            }
        }
        if (res == LIBUSB_SUCCESS) {
            if (LIBUSB_SUCCESS == (res = libusb_claim_interface(mpUsbDeviceHandle, interface))) {
                /* do stuff */
                mUsbInterfaceClaimed = true;
                LOGD_DEBUG((spi_usb_discoverer, "claimUsbInterface() libusb_claim_interface(IF=%d) success", interface));
            } else {
                LOG_ERROR((spi_usb_discoverer, "claimUsbInterface() libusb_claim_interface() failed=%d", res));
            }
        }
    } else {
        LOG_ERROR((spi_usb_discoverer, "claimUsbInterface() Cannot claim interface=%d. libusb device not open.", interface));
    }

    return res;
}

int UsbDevice::releaseUsbInterface(uint32_t interface) {
    int res = SPI_USB_DISCOVERY_ERROR;

    if ((!mpUsbContext) || (!mpUsbDeviceHandle)) {
        LOG_ERROR((spi_usb_discoverer, "releaseUsbInterface() mpUsbContext=%p, mpUsbDeviceHandle=%p are NULL",
                mpUsbContext, mpUsbDeviceHandle));
        return res;
    }
    if (true == mUsbInterfaceClaimed) {
        if (isOpen()) {
            res = libusb_release_interface(mpUsbDeviceHandle, interface);
            switch (res)
            {
                case LIBUSB_SUCCESS:
                {
                    /* no error returned */
                    break;
                }
                case LIBUSB_ERROR_NOT_FOUND:
                case LIBUSB_ERROR_NO_DEVICE:
                {
                    /* these are no errors. they returned if
                     *  - the device is not available
                     *  - or the interface was not claimed. */
                    LOGD_DEBUG((spi_usb_discoverer, "releaseUsbInterface() libusb_release_interface(%d) failed=%d",
                            interface, res));
                    res = LIBUSB_SUCCESS;
                    break;
                }
                default:
                {
                    LOG_ERROR((spi_usb_discoverer, "releaseUsbInterface() libusb_release_interface(%d) failed=%d",
                            interface, res));
                    break;
                }
            }
        } else {
            /* no libusb device open. therefore, cannot release interface. */
            LOG_WARN((spi_usb_discoverer, "releaseUsbInterface() Cannot release interface=%d. libusb device not open",
                    interface));
            res = LIBUSB_SUCCESS;
        }
    } else {
        LOGD_VERBOSE((spi_usb_discoverer, "releaseUsbInterface() No interface claimed"));
        res = SPI_USB_DISCOVERY_SUCCESS;
    }

    return res;
}

int UsbDevice::reset(t_usbDeviceInformation* inUsbDevParam) {
    int res = SPI_USB_DISCOVERY_ERROR;
    /* obsolete */
    inUsbDevParam = inUsbDevParam;

    if (!mpUsbContext) {
        return res;
    }
    if (SPI_USB_DISCOVERY_SUCCESS == (res = open())) {
        /* Perform a USB port reset to reinitialize a device.
         * The system will attempt to restore the previous configuration
         * and alternate settings after the reset has completed.
         *
         * The device will appear to be disconnected and reconnected if:
         *  - the reset fails
         *  - the descriptors change
         *  - or the previous state cannot be restored
         * This means that the device handle is no longer valid.
         * You should close it and rediscover the device.
         *
         * Return LIBUSB_ERROR_NOT_FOUND if re-enumeration is required,
         * or if the device has been disconnected
         */
        res = libusb_reset_device(mpUsbDeviceHandle);
        if (LIBUSB_ERROR_NOT_FOUND == res) {
            /* libusb reset done and re-enumeration is required,
             * or if the device has been disconnected */
            LOG_INFO((spi_usb_discoverer, "reset() libusb_reset_device() of device %s succeed",
                    to_string(mDevParam).c_str()));
            res = SPI_USB_DISCOVERY_SUCCESS;
        } else if (LIBUSB_SUCCESS == res) {
            /* If libusb_reset_device() returns with LIBUSB_SUCCESS,
             * it is an indication that the mobile device may does not
             * disconnect and reconnect. Reset not done. Trace warning */
            LOG_WARN((spi_usb_discoverer, "reset() The device %s might not be able to reset. libusb_reset_device()=%d.",
                    to_string(mDevParam).c_str(), res));
            res = SPI_USB_DISCOVERY_LIBUSB_RESET_FAILED;
        } else {
            LOG_ERROR((spi_usb_discoverer, "reset() libusb_reset_device() of device %s failed=%d",
                    to_string(mDevParam).c_str(), res));
        }
        close(true);
    } else {
        LOG_WARN((spi_usb_discoverer, "reset() Open device %s failed=%d", to_string(mDevParam).c_str(), res));
    }
    return res;
}

const struct libusb_interface_descriptor* UsbDevice::parseUsbInterfaces(struct libusb_config_descriptor* inCdesc) {
    int j, k = 0;

    /* Each device has multiple interfaces, loop through them. */
    for (j = 0; j < inCdesc->bNumInterfaces; ++j) {
        const struct libusb_interface* intf = &inCdesc->interface[j];

        /* Each interface has a bunch of alternate settings. */
        for (k = 0; k < intf->num_altsetting; ++k) {
            const struct libusb_interface_descriptor* idesc = &intf->altsetting[k];

            /* We have a match for the AOA protocol. */
            if ((idesc->bInterfaceClass == USB_INTERFACE_CLASS_AOA)
                && (idesc->bInterfaceSubClass == USB_INTERFACE_SUBCLASS_AOA)
                && (idesc->bInterfaceProtocol == USB_INTERFACE_PROTOCOL_AOA)) {
                LOGD_DEBUG((spi_usb_discoverer, "parseUsbInterfaces():  bInterface match (bIfClass=%X, bIfSubClass=%X, bIfProtocol=%X)",
                        idesc->bInterfaceClass, idesc->bInterfaceSubClass, idesc->bInterfaceProtocol));
                return idesc;
            } else {
                LOGD_DEBUG((spi_usb_discoverer, "parseUsbInterfaces():  bInterfaces does not match (bIfClass=%X, bIfSubClass=%X, bIfProtocol=%X)",
                            idesc->bInterfaceClass, idesc->bInterfaceSubClass, idesc->bInterfaceProtocol));
            }
        }
    }

    return nullptr;
}

