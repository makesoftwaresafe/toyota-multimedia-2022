/************************************************************************
 *
 * \file: spi_usb_discoverer_device_list.h
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

#ifndef SPI_USB_DISCOVERER_DEVICE_LIST_H_
#define SPI_USB_DISCOVERER_DEVICE_LIST_H_


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
#include "spi_usb_discoverer_device.h"
#include "spi_usb_discoverer_eventlist.h"

/* *************  defines  ************* */

class UsbDevice;

/**
 * @class UsbDeviceList
 * The maintainer of the list of USB devices.
 * singleton instance.
 */
class UsbDeviceList
{
public:

    /**
     * @brief Get the singleton instance of UsbDeviceList.
     *
     * If it does not exist, it will be created
     *
     * @return A pointer to the singleton. Cannot be NULL
     */
    static UsbDeviceList* getInstance(void);

    /**
     * @brief Deletes the singleton instance of UsbDeviceList if it exits
     */
    static void deleteInstance(void);

    /**
     * @brief Get a lock before modifying this list
     *
     * @return true when it was successful otherwise false
     */
    bool getLock();

    /**
     * @brief Release the lock when modifying this list is done
     *
     * @return true when it was successful otherwise false
     */
    bool releaseLock();

    /**
     * @brief Adds the pointer of the device to the device list.
     *
     * No check will be done, if the device already exists
     *
     * @param pDevice The pointer of the device to be added
     * @param inEventFd The eventFd of the usbMonitorThread
     */
    void addDevice(UsbDevice* pDevice, int32_t inEventFd);

    /**
     * @brief Removes the specified device from the device list
     *
     * @param[in,out] pDevice The pointer of the device to be removed.
     *                        When deleted, the pointer gets NULL
     */
    void removeDevice(UsbDevice* pDevice);

    /**
     * @brief Tries to find the device in the device list of this accessory
     *
     * @param vendorId The USB vendor id
     * @param productId The USB product id
     * @param serial The USB serial
     * @return The device pointer if found or NULL
     */
    UsbDevice* findDevice(uint32_t vendorId, uint32_t productId,
                          const std::string& serial);

    /**
     * @brief Tries to find the device in the device list of this accessory
     *
     * @param sysPath The sysPath "/sys/devices/soc ../ci_hdrc.*"
     * @return The device pointer if found or NULL
     */
    UsbDevice* findDeviceById(const char* sysPath);

    /**
     * @brief Tries to find the device in the device list of this accessory
     *
     * @param devNum  The (unique) device number of the device
     * @return The device pointer if found or NULL
     */
    UsbDevice* findDeviceById(const uint32_t devNum);

    /**
     * @brief Tries to find the device in the device list of this accessory
     *
     * @param sysPath The sysPath "/sys/devices/soc ../ci_hdrc.*"
     * @param devNum  The (unique) device number of the device
     * @return The device pointer if found or NULL
     */
    UsbDevice* findDeviceById(const char* sysPath, const uint32_t devNum);
private:

    struct aoapSupportThreadParam {
        aoapSupportThreadParam();
        ~aoapSupportThreadParam();
        /* basically the 'this' pointer to the UsbDeviceList */
        UsbDeviceList* pMe;
        /* USB device information of the UsbDevice
         * which should be check for AOAP support */
        std::shared_ptr<t_usbDeviceInformation> pUsbDevInfo;
        /* The eventFd of UsbDiscovererUdevMonitor to trigger
         * the call of getEventItem() by usbMonitorThread */
        int32_t monitorEventFd;
    };

    UsbDeviceList(void);
    virtual ~UsbDeviceList(void);

    /**
     * @brief The aoapSupportThread
     *
     * @param *p Pointer to UsbDeviceList
     * @return
     */
    static void* aoapSupportThread(void* p);

    std::deque<UsbDevice*> mUsbDevices;

    /** Mutex to protect access while adding or removing devices */
    pthread_mutex_t mMutex;

    /* the singleton instance */
    static UsbDeviceList* pmUsbDeviceList;
    bool mUsbDeviceListCreated;

    /* The eventFd of UsbDiscovererUdevMonitor to trigger
     * the call of getEventItem() by usbMonitorThread */
    int32_t mMonitorEventFd;

    /* ID of the SPI USB discovery aoapSupportThread */
    pthread_t mAoapSupportThreadId;

    /* class object to synchronize thread communication
     * between usbMonitorThread and aoapSupportThread */
    static SyncContext mSyncAoapSupportThread;

    void getRef() {
        __sync_add_and_fetch(&mRefCount, 1);
    }

    void putRef() {
        if (__sync_add_and_fetch(&mRefCount, -1) == 0) {
            delete (pmUsbDeviceList);
            pmUsbDeviceList = NULL;
        }
    }

    volatile unsigned int mRefCount;
};


#endif /* SPI_USB_DISCOVERER_DEVICE_LIST_H_ */
