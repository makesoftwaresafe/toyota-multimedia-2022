/*
 * aoap_accessory.h
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_ACCESSORY_H_
#define AOAP_ACCESSORY_H_

#include "aoap_types.h"
#include <string>

class Device;

/**
 * @class Accessory aoap_accessory.h "aoap_accessory.h"
 * @brief Representation of an Android USB accessory.
 *
 * More than one accessory can exist
 */
class Accessory
{
public:
    /**
     * @brief Constructor of Accessory
     *
     * @param manufacturer Manufacturer string of the accessory
     * @param modelName Model name of the accessory application
     * @param description Description of the accessory application (also called long name)
     * @param version Version of the accessory application
     * @param uri URI of the application (where to download accessory application if not installed on Android)
     * @param serial Serial of the accessory application
     * @param enableAudio Enable audio support when set to true. Otherwise this accessory does not use audio
     */
    Accessory(const std::string &manufacturer,
            const std::string &modelName,
            const std::string &description, const std::string &version,
            const std::string &uri, const std::string &serial,
            bool enableAudio);

    /**
     * @brief Destructor of Accessory
     */
    virtual ~Accessory();

    /**
     * @brief Get accessory id
     *
     * @return The accessory id
     */
    inline unsigned int getId(void)
    {
        return mId;
    }

    /**
     * @brief Get manufacturer string
     *
     * @return Manufacturer string
     */
    inline std::string getManufacturer(void)
    {
        return mManufacturer;
    }

    /**
     * @brief Get model name
     *
     * @return Model name string
     */
    inline std::string getModelName(void)
    {
        return mModelName;
    }

    /**
     * @brief Get description
     *
     * @return Description string
     */
    inline std::string getDescription(void)
    {
        return mDescription;
    }

    /**
     * @brief Get version
     *
     * @return Version string
     */
    inline std::string getVersion(void)
    {
        return mVersion;
    }

    /**
     * @brief Get URI
     *
     * @return URI string
     */
    inline std::string getUri(void)
    {
        return mUri;
    }

    /**
     * @brief Get serial number
     *
     * @return Serial number string
     */
    inline std::string getSerial(void)
    {
        return mSerial;
    }

    /**
     * @brief Get audio enabled
     *
     * @return true when audio is for this accessory is enabled otherwise false
     */
    inline bool getAudio(void)
    {
        return mAudio;
    }

    /**
     * @brief Check if this accessory supports accessory mode
     *
     * Accessory support is disabled when one of the identification strings (manufacturer, model name, description,
     * version, URI or serial) is empty
     *
     * @return true when this accessory supports accessory mode otherwise false
     */
    bool getAccessory(void);

    /**
     * @brief Get the number of device associated to Accessory
     *
     * @return The number of devices which are still associated to the accessory.
     */
    unsigned int getNumDevices();

    /**
     * @brief Release/delete the specified device
     *
     * @param deviceId Device ID of the device which shall be deleted from device list
     * @return  If success, AOAP_SUCCESS. In error cases the value is negative.
     */
    int disconnectDevice(unsigned int deviceId);

    /**
     * @brief Connect the accessory to the specified USB device.
     *
     * This function is implemented/designed asynchronous because the USB device gets detached and re-attached while
     * switching to accessory mode
     *
     * @param vendorId The vendor id of the USB device
     * @param productId The product id of the USB device
     * @param serial The serial of the USB device
     * @param callback The callback to get notified when the connection gets established
     * @param audioSupport Enable/disable audio support
     * @param token A token returned in the callback. Can be NULL
     * @return
     */
    int connectDevice(unsigned int vendorId, unsigned int productId,
            const std::string &serial, aoap_connectCB callback, bool audioSupport, void *token);

    /**
     * @brief Sets the connect timeout for this accessory
     *
     * @param seconds The timeout in seconds
     */
    void setConnectTimeout(unsigned int seconds);

    /**
     * @brief Sets timeout for the control requests
     *
     * @param inTimeoutMs The timeout in miliseconds
     */
    void setControlRequestTmo(unsigned int inTimeoutMs);

    /**
     * @brief Check if specified device supports AOAP
     *
     * @param vendorId The ID of the vendor of the USB device
     * @param productId The ID of the product of the USB device
     * @param serial The serial number of the USB device
     * @param majorVersion The major version number of AOAP
     * @param minorVersion The minor version number of AOAP
     * @return The result code. When result is AOAP_SUCCESS major and minor gets populated
     */
    int checkAoapSupport(unsigned int vendorId, unsigned int productId,
                    const std::string &serial, unsigned int &majorVersion,
                    unsigned int &minorVersion);

    /**
     * @brief Returns the current state of the Accessory reference counter
     *
     * @return The Accessory reference counter
     */
    inline unsigned int getRefCnt() { return mReferenceCounter; };
    /**
     * @brief Increase the Accessory reference counter
     *
     */
    inline void refCntUp() { mReferenceCounter++; };

    /**
     * @brief Decrease the Accessory reference counter
     *
     */
    inline void refCntDown() { mReferenceCounter--; };
private:

    unsigned int mId;
    const std::string mManufacturer;
    const std::string mModelName;
    const std::string mDescription;
    const std::string mVersion;
    const std::string mUri;
    const std::string mSerial;
    bool mAudio;
    unsigned int mConnectTimeout;
    unsigned int mControlRequestTimeout;

    /* Accessory reference counter */
    unsigned int mReferenceCounter;

    static unsigned int gCounter;
};

#endif /* AOAP_ACCESSORY_H_ */
