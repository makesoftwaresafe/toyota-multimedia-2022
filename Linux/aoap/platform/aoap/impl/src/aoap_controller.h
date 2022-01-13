/*
 * aoap_controller.h
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_CONTROLLER_H_
#define AOAP_CONTROLLER_H_

#include "aoap_types.h"
#include <pthread_adit.h>
#include <vector>
#include <string>

//Forward declarations
class Accessory;
class ExternalObserver;

namespace AOAP { namespace Control {

/**
 * @class Controller aoap_controller.h "aoap_controller.h"
 * The controller class is the entry point for all public APIs. It is implemented as singleton to
 * ensure that it is the entry for all API calls
 */
class Controller
{
public:

    /**
     * @brief Get the singleton instance of Controller.
     *
     * If it does not exist, it will be created
     *
     * @return An object to the singleton. Cannot be NULL
     */
    static Controller& getInstance(void);

    /**
     * @brief The mutex object referenced by mMutex shall be locked.
     * If the mutex is already locked, the calling thread
     * shall block until the mutex becomes available.
     * */
    void lock(void) { pthread_mutex_lock(&mMutex); };

    /**
     * @brief The mutex object referenced by mMutex shall be released.
     */
    void unlock(void) { pthread_mutex_unlock(&mMutex); };

    /**
     * @brief Create and initialize accessory with the provided data
     *
     * @param pAccessoryParam The accessory parameters
     * @return The accessory id in case of success. A negative value in case of a failure
     */
    int createAccessory(t_aoap_accessory_param *pAccessoryParam);

    /**
     * @brief Deinitialize the specified accessory
     *
     * @param accessoryId The ID of the accessory to be deinitialized
     */
    void deinitAccessory(unsigned int accessoryId);

    /**
     * @brief Release/delete the specified device
     *
     * @param deviceId Device ID of the device which shall be deleted from device list
     * @return  If success, the return value is AOAP_SUCCESS. In error cases the value is negative.
     */
    int disconnectDevice(unsigned int accessory_id, unsigned int deviceId);

    /**
     * @brief Connect the specified USB device by switching to accessory mode
     *
     * Because switching to accessory mode means that the device becomes detached and re-attached with Google's
     * vendor and product ids a callback can be provided to get a notification when the switching is done
     *
     * @param accessoryId The accessory ID to be used. See result of 'init_accessory' call
     * @param vendorId The vendor id of the USB device (usually the one before switching to accessory mode)
     * @param productId The product id of the USB device
     * @param serial The serial of the USB device
     * @param callback The callback which will be called after switching is complete
     * @param token A token can be specified which will be returned in the callback
     * @param audioSupport Enable/disable audio support for this device
     * @return The result of switching to accessory mode.0 means success. A negative value is an error. The result contains only errors till switching to accessory
     * mode. If everything else works so far success (=0) is returned
     */
    int connectDevice(unsigned int accessoryId, unsigned int vendorId,
            unsigned int productId, const std::string &serial,
            aoap_connectCB callback, bool audioSupport, void *token);

    /**
     * @brief Sets the connect timeout for the specified accessory
     *
     * @param accessoryId The ID of the accessory
     * @param seconds The timeout in seconds
     */
    void setConnectTimeout(unsigned int accessoryId, unsigned int seconds);

    /**
     * @brief Sets timeout for the control requests
     *
     * @param accessoryId The ID of the accessory
     * @param inTimeoutMs The timeout in miliseconds
     */
    void setControlRequestTimeout(unsigned int accessoryId, unsigned int inTimeoutMs);

    /**
     * @brief Read data synchronously
     *
     * @param accessoryId Accessory ID
     * @param deviceId Device ID
     * @param pBuffer Buffer to read to
     * @param bufferSize Size of the buffer
     * @param timeout The timeout in milliseconds. 0 means no timeout
     * @return The read bytes in case the value is positive. In error cases the value is negative
     */
    int read(unsigned int accessoryId, unsigned int deviceId,
            unsigned char *pBuffer, unsigned int bufferSize, unsigned int timeout);

    /**
     * @brief Read data synchronously
     *
     * @param accessoryId Accessory ID
     * @param deviceId Device ID
     * @param pBuffer Buffer to read to
     * @param bufferSize Size of the buffer
     * @param pTransferred Number of read bytes
     * @param timeout The timeout in milliseconds. 0 means no timeout
     * @return 0 indicates success. In case the number is negative, an error occurred
     */
    int read(unsigned int accessoryId, unsigned int deviceId,
            unsigned char *pBuffer, unsigned int bufferSize, unsigned int *pTransferred, unsigned int timeout);

    /**
     * @brief Write data synchronously
     *
     * @param accessoryId Accessory ID
     * @param deviceId Device ID
     * @param pBuffer Buffer to write
     * @param bufferSize Size of the buffer which is equal to the number of bytes to write
     * @param timeout The timeout in milliseconds of a single write operation
     * @return The number of bytes written in case the value is positive. In error cases the value is negative
     */
    int write(unsigned int accessoryId, unsigned int deviceId,
            const unsigned char *pBuffer, unsigned int bufferSize, unsigned int timeout);

    /**
     * @brief Write data synchronously
     *
     * @param accessoryId Accessory ID
     * @param deviceId Device ID
     * @param pBuffer Buffer to write
     * @param bufferSize Size of the buffer which is equal to the number of bytes to write
     * @param pTransferred Number of bytes written
     * @param timeout The timeout in milliseconds of a single write operation
     * @return 0 indicates success. In error cases the value is negative
     */
    int write(unsigned int accessoryId, unsigned int deviceId,
            const unsigned char *pBuffer, unsigned int bufferSize, unsigned int *pTransferred, unsigned int timeout);

    /**
     * @brief Check AOAP support
     *
     * @param accessoryId The accessory ID
     * @param vendorId The vendor ID of the USB device
     * @param productId The product ID of the USB device
     * @param pSerial The serial of the USB device
     * @param majorVersion The major AOAP version number
     * @param minorVersion The minor AOAP version number
     * @return The result of the operation. When result is AOAP_SUCCESS the parameters major and minor will tell you if
     *         the USB device support AOAP and which version
     */
    int checkAoapSupport(unsigned int accessoryId, unsigned int vendorId,
            unsigned int productId, const std::string& pSerial,
            unsigned int &majorVersion, unsigned int &minor);

    /**
     * @brief Get the number of accessories
     *
     * @return The number of accessories
     */
    inline unsigned int getAccessoryNumber(void) const
    {
        return mAccessories.size();
    }

    /**
     * @brief Get the number of device associated to Accessory
     *
     * @param accessoryId The accessory ID
     * @return The number of devices which are still associated to the accessory. In error cases the value is negative
     */
    unsigned int getNumDevices(unsigned int accessoryId);

    static void enablePerformanceMeasurements(void);

private:

    /**
     * @brief Hidden constructor for singleton
     */
    Controller(void); //lint !e1704

    /**
     * @brief Hidden destructor for singleton
     */
    virtual ~Controller();

    /**
     * @brief Hidden copy constructor
     */
    Controller(const Controller&)= delete;

    /**
     * @brief Hidden assignment operator
     */
    Controller& operator=(const Controller&)= delete;

    /**
     * @brief Clears the accessory list
     */
    void clearAccessories(void);

    /**
     * @brief Find the specified accessory and return a pointer to it
     *
     * @param accessoryId The id of the accessory to be searched
     * @return The pointer to the accessory if found or NULL if not found
     */
    Accessory* findAccessory(unsigned int accessoryId);

    /**
     * @brief Finds the specified accessory
     *
     * @param pAccessoryParam A pointer to the accessory parameters
     * @return The accessory if all parameters match. Otherwise it returns NULL
     */
    Accessory* findAccessory(t_aoap_accessory_param *pAccessoryParam);

    //Members

    /** The accessory list of all accessories used so far */
    std::vector<Accessory*> mAccessories;

    /* the mutex object */
    pthread_mutex_t mMutex;
};

} }/* namespace AOAP { namespace Control { */

#endif /* AOAP_CONTROLLER_H_ */
