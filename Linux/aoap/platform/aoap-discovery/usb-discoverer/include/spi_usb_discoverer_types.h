/************************************************************************
 *
 * \file: spi_usb_discoverer_types.h
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

#ifndef SPI_USB_DISCOVERER_TYPES_H_
#define SPI_USB_DISCOVERER_TYPES_H_

/* *************  includes  ************* */

#include <adit_typedef.h>
#include <sys_time_adit.h>
#include <pthread_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <cstring>

#include <sys/unistd.h>
#include <sys/types.h>

#include <atomic>
/* *************  defines  ************* */

using std::string;

/* @brief SPI USB Discovery return values */
#define SPI_USB_DISCOVERY_SUCCESS               0
#define SPI_USB_DISCOVERY_ERROR                 -1
#define SPI_USB_DISCOVERY_BAD_PARAMETER         -2
#define SPI_USB_DISCOVERY_NO_DEVICE             -3
#define SPI_USB_DISCOVERY_NO_DEVICE_FOUND       -4
#define SPI_USB_DISCOVERY_NO_AOAP_SUPPORT       -5
#define SPI_USB_DISCOVERY_ALREADY_IN_AOAP       -6
#define SPI_USB_DISCOVERY_UDEV_ERROR            -7
#define SPI_USB_DISCOVERY_LIBUSB_ERROR          -8
#define SPI_USB_DISCOVERY_LIBUSB_RESET_FAILED   -9


/**
 * @struct UsbDeviceInformation
 * Definition of USB device information consisting of vendorId, productId and serial.
 */
typedef struct UsbDeviceInformation
{
    /**
     * @brief vendorId the vendorId of the usb device (e.g. 18d1 for Google)
     * @note The vendorId must be provided to aauto/AoapTransport
     *       to switch the usb device into AOAP
     */
    uint32_t vendorId;
    /**
     * @brief productId the productId of the usb device (e.g. 0x2d00 - 0x2d05 for usb device in AOAP)
     * @note The productId must be provided to aauto/AoapTransport
     *       to switch the usb device into AOAP
     */
    uint32_t productId;
    /**
     * @brief serial the serial number of the usb device
     * @note The serial must be provided to aauto/AoapTransport
     *       to switch the usb device into AOAP
     */
    string   serial;
    /**
     * @brief product the product of the usb device (e.g. Nexus 4)
     */
    string   product;
    /**
     * @brief manufacturer the manufacturer of the usb device (e.g. LGE)
     */
    string   manufacturer;
    /**
     * @brief devNum udev device major/minor number
     */
    uint32_t devNum;
    /**
     * @brief sysPath the system path of the usb device
     */
    string   sysPath;

    /**
     * @brief interface usb interface number where the AOAP endpoints assigned
     */
    uint32_t interface;

    /**
     * @brief aoapSupported indicates if the MD supports AOAP (true) or not (false)
     */
     bool aoapSupported;

    /**
     * @brief Used in SpiUsbDiscovery
     */
    UsbDeviceInformation()
    {
        vendorId = 0;
        productId = 0;
        devNum = 0;
        interface = 0;
        aoapSupported = false;
        _count.fetch_add(1);
    }
    /**
     * @brief Used in SpiUsbDiscovery
     */
    UsbDeviceInformation(const char* inSysPath)
    {
        vendorId = 0;
        productId = 0;
        devNum = 0;
        interface = 0;
        aoapSupported = false;
        sysPath = inSysPath;
        _count.fetch_add(1);
    }

    /**
     * @brief Obsolete. You must use release()
     */
    ~UsbDeviceInformation()
    {
        _count.fetch_sub(1);
    }
private:

    static std::atomic<uint32_t> _count; // definition in spi_usb_discoverer.cpp
} t_usbDeviceInformation;


#endif /* SPI_USB_DISCOVERER_TYPES_H_ */
