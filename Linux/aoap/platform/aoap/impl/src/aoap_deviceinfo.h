/*
 * aoap_deviceinfo.h
 *
 *  Created on: Aug 14, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_DEVICEINFO_H_
#define AOAP_DEVICEINFO_H_

#include <string>
#include <libudev.h>

/**
 * @class DeviceInfo aoap_deviceinfo.h "aoap_deviceinfo.h"
 * Device info class to get information from 'libudev' for a specific USB device.
 * Implemented as singleton
 */
class DeviceInfo
{
public:
    /**
     * @brief Get the singleton instance of DeviceInfo class.
     *
     * If not yet created, it creates the instance.
     *
     * @return A pointer to the singleton
     */
    static DeviceInfo* getInstance(void);

    /**
     * @brief Deletes the singleton instance of DeviceInfo if it exists
     */
    static void deleteInstance(void);

    /**
     * @brief Get the device number for the specified device by enumerating
     * through the devices
     *
     * @param vendorId The vendor ID of the device to be looked for
     * @param productId The product ID of the device to be looked for
     * @param serial The serial number of the device to be looked for
     * @return The device number if positive. A negative value indicates an error
     */
    int getDevNum(unsigned int vendorId, unsigned int productId, const std::string& serial);

private:

    /**
     * @brief Private constructor of DeviceInfo
     */
    DeviceInfo(void);

    /**
     * @brief Private desctructor of DeviceInfo
     */
    virtual ~DeviceInfo();

    /**
     * @brief Creates udev reference
     * @return true when the reference is present or could be created, otherwise false
     */
    bool createUdev(void);

    /** A pointer to the udev reference */
    udev *mpUdev;

    /** The static pointer to DeviceInfo singleton instance */
    static DeviceInfo *mpSelf;
};

#endif /* AOAP_DEVICEINFO_H_ */
