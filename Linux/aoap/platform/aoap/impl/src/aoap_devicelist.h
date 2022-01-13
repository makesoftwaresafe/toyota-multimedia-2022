/*
 * aoap_devicelist.h
 *
 *  Created on: Sep 11, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_DEVICELIST_H_
#define AOAP_DEVICELIST_H_

#include "aoap_usbobserver.h"
#include <string>
#include <vector>
#include <pthread.h>
#include <memory>

class Device;

/**
 * @class DeviceList aoap_devicelist.h "aoap_devicelist.h"
 * The maintainer of the list of USB devices. Implements the UsbObserver.
 * Implemented as singleton
 */
class DeviceList : public UsbObserver
{
public:

    /**
     * @brief Get the singleton instance of DeviceList.
     *
     * If it does not exist, it will be created
     *
     * @return A pointer to the singleton. Cannot be NULL
     */
    static DeviceList* getInstance(void);

    /**
     * @brief Deletes the singleton instance of DeviceList if it exits
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
     * @brief Get the number of devices in the device list
     *
     * @return The number of devices
     */
    inline unsigned int getNumDevices(void) const { return mDevices.size(); }

    /**
     * @brief Adds the pointer of the device to the device list.
     *
     * No check will be done, if the device already exists
     *
     * @param pDevice The pointer of the device to be added
     */
    inline void addDevice(std::shared_ptr<Device> pDevice) { mDevices.push_back(pDevice); }

    /**
     * @brief Removes the specified device from the device list
     *
     * @param[in,out] pDevice The pointer of the device to be removed.
     *                        When deleted, the pointer gets NULL
     */
    void removeDevice(std::shared_ptr<Device> pDevice);

    /**
     * @brief Tries to find the device in the device list of this accessory
     *
     * @param vendorId The USB vendor id
     * @param productId The USB product id
     * @param serial The USB serial
     * @return The device pointer if found or NULL
     */
    std::shared_ptr<Device> findDevice(unsigned int vendorId, unsigned int productId,
            const std::string& serial);

    /**
     * @brief Tries to find the device based on the device number from libudev
     *
     * @param devNum The USB device number
     * @return The pointer to the device if found otherwise NULL
     */
    std::shared_ptr<Device> findDeviceByDeviceNumber(unsigned int devNum);

    /**
     * @brief Tries to find the device based on the serial number
     *
     * @param serial The serial of the USB device
     * @return The pointer to the device if found otherwise NULL
     */
    std::shared_ptr<Device> findDeviceBySerial(const std::string& serial);

    /**
     * @brief Find the device with the specified device id
     *
     * @param deviceId The ID of the device to be found
     * @return The pointer to the device or NULL if not found
     */
    std::shared_ptr<Device> findDevice(unsigned int deviceId);

    /**
     * @implements update
     */
    virtual void update(bool attach, unsigned int vendorId,
            unsigned int productId, const std::string &serial,
            unsigned int devNum);

private:

    /**
     * @brief Constructor of DeviceList
     */
    DeviceList(void);

    /**
     * @brief Destructor of DeviceList
     */
    virtual ~DeviceList(void);

    /**
     * @brief Clear the device list
     */
    void clearDevices(void);

    /** The device list of all devices */
    std::vector<std::shared_ptr<Device>> mDevices;

    /** Mutex for protecting access while adding or removing devices */
    pthread_mutex_t mMutex;

    /** The singleton instance */
    static DeviceList* gpSelf;
};

#endif /* AOAP_DEVICELIST_H_ */
