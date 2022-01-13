/************************************************************************
 *
 * \file: spi_usb_discoverer_device.h
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

#ifndef SPI_USB_DISCOVERER_DEVICE_H_
#define SPI_USB_DISCOVERER_DEVICE_H_

/* *************  includes  ************* */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <sys/unistd.h>
#include <sys/types.h>

#include <libudev.h>
#include <libusb-1.0/libusb.h>


#include <cstring>
#include <deque>

#include "spi_usb_discoverer_types.h"
#include "spi_usb_discoverer_utility.h"


/* *************  defines  ************* */

/**
 * @class UsbDevice
 * @brief USB device class
 * USB device abstraction
 */
class UsbDevice
{
public:
    /**
     * @brief UsbDevice constructor w/devNum
     *
     * @param t_usbDeviceInformation - structure with USB device information
     */
    UsbDevice(std::shared_ptr<t_usbDeviceInformation> inUsbDevInfo);
    virtual ~UsbDevice(void);

    /**
     * @brief Check if the device supports AOA protocol
     *
     * Checks if the device with the specified parameters supports the AOA protocol
     * and stores the information in the USB device information (aoapSupported)
     *
     * @return SPI_USB_DISCOVERY_SUCCESS - check for AOA protocol done.
     *         SPI_USB_DISCOVERY_NO_AOAP_SUPPORT - check for AOA protocol done, but AOA protocol is not supported
     *         SPI_USB_DISCOVERY_NO_DEVICE_FOUND - device not found or no longer available
     *         SPI_USB_DISCOVERY_ERROR - error on libusb level
     */
    int checkAoapSupport(void);

    /**
     * @brief Opens the device
     *
     * Checks if the device with the specified parameters exist, opens it if it exits and returns the handle to it
     *
     * @return The result of open. 0 means success. A negative value indicates an error. See error defines
     */
    int open(void);

    /**
     * @brief Closes the device
     *
     * @param usbClose When true calls libusb_close. If false, call will be skipped. This might be
     *                 useful when the device is already disconnected. Default is true
     */
    void close(bool usbClose);

    /**
     * @brief Reset the device
     *
     * Reset the libusb port where the device is connceted. This leads to disconnect and reconnect of the device.
     *
     * @param inUsbDevParam This is obsolete and is not used.
     * @return 0 means success. A negative value indicates an error. See error defines
     */
    int reset(t_usbDeviceInformation* inUsbDevParam);

    /**
     * @brief Helper function for 'libusb_claim_interface'
     *
     * @param interfaceNumber The interface number
     * @return The result. 0 means success. A negative value will indicate an error
     */
    int claimUsbInterface(uint32_t interface);

    /**
     * @brief Helper function for 'libusb_release_interface'
     *
     * @return The result. 0 means success. A negative value will indicate an error
     */
    int releaseUsbInterface(uint32_t interface);

    std::shared_ptr<t_usbDeviceInformation> getUsbDeviceInformation(void) const
    {
        return mDevParam;
    }

    /**
     * @brief Get USB vendor id
     *
     * @return USB vendor id
     */
    inline unsigned int getVendorId(void) const
    {
        return mDevParam->vendorId;
    }

    /**
     * @brief Get USB product id
     *
     * @return USB product id
     */
    inline unsigned int getProductId(void) const
    {
        return mDevParam->productId;
    }

    /**
     * @brief Get USB device number
     *
     * @return USB device number
     */
    inline unsigned int getDevNum(void) const
    {
        return mDevParam->devNum;
    }

    /**
     * @brief Get USB serial
     *
     * @return USB serial string
     */
    inline std::string getSerial(void) const
    {
        return mDevParam->serial;
    }

    /**
     * @brief Get Udev syspath
     *
     * @return udev syspath string
     */
     inline std::string getSysPath(void) const
     {
         return mDevParam->sysPath;
     }

     /**
      * @brief Get USB manufacturer
      *
      * @return USB manufacturer string
      */
     inline std::string getManufacturer(void) const
     {
         return mDevParam->manufacturer;
     }

     /**
      * @brief Get USB product
      *
      * @return USB product string
      */
     inline std::string getProduct(void) const
     {
         return mDevParam->product;
     }

     /**
      * @brief Get flag which indicates if AOAP support was checked for this device
      *
      * @return true or false
      */
     inline bool getAoapWasChecked(void) const
     {
         return mAoapSupportChecked;
     }
private:
    /**
     * @brief Initializes this device (used within constructors) and get libusb context.
     */
    void init(void);

    /**
     * @brief Checks if the device is open
     * (this is true when the device handle exists)
     *
     * @return true when the handle exists
     */
    bool isOpen(void) const { return mpUsbDeviceHandle ? true : false; };

    const struct libusb_interface_descriptor* parseUsbInterfaces(struct libusb_config_descriptor* inCdesc);

    struct libusb_context* mpUsbContext;
    struct libusb_device_handle *mpUsbDeviceHandle;

    bool mUsbInterfaceClaimed;

    std::shared_ptr<t_usbDeviceInformation> mDevParam;

    bool mAoapSupportChecked;
};



#endif /* SPI_USB_DISCOVERER_DEVICE_H_ */
