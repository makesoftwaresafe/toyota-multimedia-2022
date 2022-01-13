
/************************************************************************
 *
 * \file: spi_usb_monitor.h
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

#ifndef SPI_USB_MONITOR_H
#define SPI_USB_MONITOR_H

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


#include <string>
#include <cstring>

#include "spi_usb_discoverer_types.h"
#include "spi_usb_discoverer_callbacks.h"
#include "spi_usb_discoverer_device_list.h"
#include "spi_usb_discoverer_utility.h"

/* *************  defines  ************* */

/**
 * @class UsbMonitor
 * @brief USB monitor class for monitoring the udev
 */
class UsbDiscovererUdevMonitor
{
public:
    /**
     * @brief Get the singleton instance of UsbDeviceList.
     *
     * If it does not exist, it will be created
     *
     * @return A pointer to the singleton. Cannot be NULL
     */
    static UsbDiscovererUdevMonitor* getInstance(void);

    /**
     * @brief Deletes the singleton instance of UsbDeviceList if it exits
     */
    static void deleteInstance(void);

    int startMonitoring(void* context, SpiUsbDiscovererCallbacks *pCallbacks);
    int stopMonitoring(void);

    int resetMonitoredDevice(t_usbDeviceInformation* inUsbDevParam);

private:
    UsbDiscovererUdevMonitor();
    virtual ~UsbDiscovererUdevMonitor(void);

    /**
     * @brief The usbMonitorThread
     *
     * @param *p Pointer to UsbMonitor
     * @return
     */
    static void* usbMonitorThread(void* p);

    /**
     * @brief Initiate and start the udev monitor
     *
     * @param
     * @return true if success, otherwise false
     */
    bool startUdevMonitor(void);

    /**
     * @brief Enumerate the udev
     *
     * @param outUdevDeviceList list of udev_devices which match for 'usb' subsystem
     * @return a list of the devices in the 'usb' subsystem.
     */
    struct udev_enumerate* enumerateUdevDeviceList(struct udev_list_entry** outUdevDeviceList);

    /**
     * @brief Return the devpath value
     * Use the return value to check if the udev device is an USB controller
     *
     * @param inUdevDevice the udev_device
     * @return devpath value
     */
    int getDevPath(struct udev_device* inUdevDevice);

    /**
     * @brief Parse and return the udev subsystem & sysattr settings of the connected device
     *
     * @param inUdevDevice the udev_device
     * @return structure with udev device information
     */
    std::shared_ptr<t_usbDeviceInformation> createDevInfo(struct udev_device* inUdevDevice);

    /**
     * @brief Verify the connected MD
     *
     * @param inUdevDevice the udev device of the connected device.
     * @return true if device can be supported. Otherwise false.
     */
    bool verifyAttach(struct udev_device* inUdevDevice);

    /**
     * @brief Verify the disconnected MD
     *
     * @param inUdevDevice the udev device of the disconnected device.
     * @return true if detach of device could be notified. Otherwise false.
     */
    bool verifyDetach(struct udev_device* inUdevDevice);

    /**
     * @brief Verify the connected/disconnected MD
     *
     * @param inUdevDevice the udev device of the connected device.
     * @param attach true if device was added. false if detached.
     * @return true if device can be supported. Otherwise false.
     */
    bool verifyDevice(struct udev_device* inUdevDevice, bool attach);

    /**
     * @brief Checks event list if mEventFd was triggered.
     *
     * @param inEvent the event.
     * @param outStopMonitoring shall set to true if monitoring should stop.
     * @return true if device can be supported. Otherwise false.
     */
    void checkEventList(const uint64_t inEvent, bool* outStopMonitoring);

    /* Callbacks registered by Application at startMonitoring() */
    SpiUsbDiscovererCallbacks mCallbacks;
    /* Context which could be given by Application */
    void* mContext;

    /* the singleton instance of UsbDiscovererUdevMonitor */
    static UsbDiscovererUdevMonitor* pmUsbMonitor;

    /* ID of the SPI USB discovery monitorThread */
    pthread_t mUsbMonitorThreadId;

    /* udev related variables */
    udev* mpUdev;
    udev_monitor* mpMonitor;
    int32_t mUdevMonitorFd;

    /* eventfd added in usbMonitorThread to polling. */
    int32_t mEventFd;

    /* indicate to shutdown */
    bool mShutdown;
};


#endif /* SPI_USB_MONITOR_H */
