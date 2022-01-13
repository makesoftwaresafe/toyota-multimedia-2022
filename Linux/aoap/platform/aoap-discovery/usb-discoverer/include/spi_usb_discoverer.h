
/************************************************************************
 *
 * \file: spi_usb_discoverer.h
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

#ifndef SPI_USB_DISCOVERY_H
#define SPI_USB_DISCOVERY_H

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

#include <string>
#include <cstring>

#include "spi_usb_discoverer_types.h"
#include "spi_usb_discoverer_callbacks.h"

/* *************  defines  ************* */

namespace AOAP {

using std::string;

/**
 * @class UsbDiscoverer
 * @brief UsbDiscoverer class for monitoring the udev
 * @Note: Unexpected system behavior like undervoltage or arbitrarily disconnect-reconnect,
 * can irritate the UsbDiscoverery in such a way that the callbacks are triggered out of sequence.
 */
class UsbDiscoverer
{
public:
    /**
     * @brief Register the callbacks and register the DLT context
     *
     * @param context context to use when calling callback
     * @param pCallbacks Callbacks which are registered by Application an called by SpiUsbDiscovery
     */
    UsbDiscoverer(void* context, SpiUsbDiscovererCallbacks *pCallbacks);
    virtual ~UsbDiscoverer(void);

    /**
     * @brief Start monitoring. Create the monitor thread.
     *
     * @return 0 if success, otherwise negative return value
     */
    int startMonitoring(void);

    /**
     * @brief Stop the monitoring. Cancel the monitor thread.
     *
     * @return 0 if success, otherwise negative return value
     */
    int stopMonitoring(void);

    /**
     * @brief Reset the (libusb) device specified by param inUsbDevParam
     * @Note:
     * Do not use the API from within one of the usb-discoverer callback
     * which are defined by structure SpiUsbDiscovererCallbacks.
     *
     * @Note:
     * Perform a USB port reset to reinitialize a device.
     * The system will attempt to restore the previous configuration and alternate settings after the reset has completed.
     * If the reset fails, the descriptors change, or the previous state cannot be restored,
     * the device will appear to be disconnected and reconnected.
     * This means that the device handle is no longer valid (you should close it) and rediscover the device.
     * A return code of LIBUSB_ERROR_NOT_FOUND indicates when this is the case.
     * This is a blocking function which usually incurs a noticeable delay.
     *
     * @Note:
     * Result of testing:
     *  - libusb command triggers proper reset of USB port
     *  - works also on USB hub
     *  - does not affect other devices on USB hub (only intended target device to reset)
     *
     * @param inUsbDevParam structure with usb device information
     * which where received with the attach callback
     * @return 0 - if success
     *         SPI_USB_DISCOVERY_LIBUSB_RESET_FAILED - if re-enumeration is not required,
     *                                                 or if the device has not been disconnected
     *         otherwise - a negative return value.
     */
    int resetDevice(t_usbDeviceInformation* inUsbDevParam);
private:

    /**
     * @brief mCallbacks contains the callbacks which a registered by Application
     */
    SpiUsbDiscovererCallbacks mCallbacks;

    /**
     * @brief mContext is the context which could be provided by Application
     */
    void* mContext;

    bool mMonitoringActive;
};

} // namespace AOAP {


#endif /* SPI_USB_DISCOVERY_H */
