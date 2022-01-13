/*
 * aoap_device.h
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_DEVICE_H_
#define AOAP_DEVICE_H_

#include "aoap_usbobserver.h"
#include "aoap_types.h"
#include "aoap_timer.h"
#include <libusb-1.0/libusb.h>
#include <string>
#include <mutex>

class Accessory;

/**
 * @class Device aoap_device.h "aoap_device.h"
 * @brief USB device class
 * USB device abstraction
 */
class Device : public AOAP::Time::Timer
{
public:

    /**
     * @typedef typedef struct Endpoint tEndpoint
     * @struct Endpoint
     * Definition of an USB endpoint consisting of address and max packet size
     */
    typedef struct Endpoint
    {
        unsigned char address;
        int maxPacketSize;
    } tEndpoint;

    /**
     * @typedef typedef enum State tState
     * @enum State
     * Definition of device states
     */
    typedef enum State
    {
        AOAP_DEVICE_STATE_IDLE, //!< AOAP_DEVICE_STATE_IDLE
        AOAP_DEVICE_STATE_SWITCHING,//!< AOAP_DEVICE_STATE_SWITCHING
        AOAP_DEVICE_STATE_ACCESSORY,//!< AOAP_DEVICE_STATE_ACCESSORY
        AOAP_DEVICE_STATE_UNKNOWN //!< AOAP_DEVICE_STATE_UNKNOWN
    //!< AOAP_DEVICE_STATE_UNKNOWN
    } tState;

    /**
     * @typedef typedef enum ControlRequestTypes tControlRequestTypes
     * @enum ControlRequestTypes
     * Definition of the vendor control request types used for AOAP. The numbers must match the correct one in the
     * AOAP 2.0 specification!
     */
    typedef enum ControlRequestTypes
    {
        AOAP_ACCESSORY_PROTOCOL_REQUEST = 51,    //!< AOAP_ACCESSORY_PROTOCOL_REQUEST
        AOAP_ACCESSORY_SEND_IDENTIFICATION = 52, //!< AOAP_ACCESSORY_SEND_IDENTIFICATION
        AOAP_ACCESSORY_START_MODE = 53,          //!< AOAP_ACCESSORY_START_MODE
        AOAP_ACCESSORY_REGISTER_HID = 54,        //!< AOAP_ACCESSORY_REGISTER_HID
        AOAP_ACCESSORY_UNREGISTER_HID = 55,      //!< AOAP_ACCESSORY_UNREGISTER_HID
        AOAP_ACCESSORY_SET_HID_REPORT_DESC = 56, //!< AOAP_ACCESSORY_SET_HID_REPORT_DESC
        AOAP_ACCESSORY_SEND_HID_EVENT = 57,      //!< AOAP_ACCESSORY_SEND_HID_EVENT
        AOAP_ACCESSORY_ENABLE_AUDIO = 58         //!< AOAP_ACCESSORY_ENABLE_AUDIO
    } tControlRequestTypes;

    /**
     * @typedef typedef enum ControlString tControlString
     * @enum ControlString
     * Definition of the control strings sent when switching to accessory mode. The assigned values are specified
     * in the AOAP 1.0 specification!
     */
    typedef enum ControlString
    {
        MANUFACURER_CONTROL_STRING = 0,//!< MANUFACURER_CONTROL_STRING
        MODEL_NAME_CONTROL_STRING = 1, //!< MODEL_NAME_CONTROL_STRING
        DESCRIPTION_CONTROL_STRING = 2,//!< DESCRIPTION_CONTROL_STRING
        VERSION_CONTROL_STRING = 3, //!< VERSION_CONTROL_STRING
        URI_CONTROL_STRING = 4, //!< URI_CONTROL_STRING
        SERIAL_CONTROL_STRING = 5 //!< SERIAL_CONTROL_STRING
    //!< SERIAL_CONTROL_STRING
    } tControlString;

    /**
     * @brief Device constructor w/o devNum
     *
     * @param vendorId The USB vendor ID
     * @param productId The USB product ID
     * @param serial The USB serial
     */
    Device(unsigned int vendorId, unsigned int productId, const std::string &serial);

    /**
     * @brief Device constructor w/devNum
     *
     * @param vendorId The USB vendor ID
     * @param productId The USB product ID
     * @param serial The USB serial
     * @param devNum The device number
     */
    Device(unsigned int vendorId, unsigned int productId, const std::string &serial, unsigned int devNum);

    /**
     * @brief Device destructor
     */
    virtual ~Device(void);

    /**
     * @brief Get unique id of the device instance
     *
     * @return The unique id
     */
    inline unsigned int getId(void) const
    {
        return mId;
    }

    /**
     * @brief Get audio status
     *
     * @return true when enabled, otherwise false
     */
    inline bool getAudioStatus(void) const
    {
        return mAudioEnabled;
    }

    /**
     * @brief Get state of device
     *
     * @return The device state
     */
    inline tState getState(void) const
    {
        return mState;
    }

    /**
     * @brief Get USB vendor id
     *
     * @return USB vendor id
     */
    inline unsigned int getVendorId(void) const
    {
        return mVendorId;
    }

    /**
     * @brief Set the vendor ID
     *
     * @param vendorId The new vendor ID
     */
    inline void setVendorId(unsigned int vendorId) { mVendorId = vendorId; }

    /**
     * @brief Get USB product id
     *
     * @return USB product id
     */
    inline unsigned int getProductId(void) const
    {
        return mProductId;
    }

    /**
     * @brief Set the product ID
     *
     * @param productId The new product ID
     */
    inline void setProductId(unsigned int productId) { mProductId = productId; }

    /**
     * @brief Get USB serial
     *
     * @return USB serial string
     */
    inline std::string getSerial(void) const
    {
        return mSerial;
    }

    /**
     * @brief Checks if the devNum is already set
     *
     * @return true when the devNum is already set otherwise false
     */
    inline bool getDevNumSet(void) const
    {
        return mDevNumSet;
    }

    /**
     * @brief Tries to retrieve the device number using 'udev'
     *
     * If device number could not be determined the flag mDevNumSet will be set to false
     */
    void retrieveDevNum(void);

    /**
     * @brief Set the pointer to the accessory used for retrieving ID,
     *        identification strings etc.
     *
     * @param pAccessory The pointer of the accessory
     */
    void setAccessory(Accessory *pAccessory);

    /**
     * @brief Get the accessory ID belonging to the device
     *
     * @return If the device is not yet assigned to an accessory, the value will be -1
     */
    int getAccessoryId(void) const;

    /**
     * @brief Reset interface number (will be automatically released)
     */
    inline void resetInterface(void) { mInterfaceNumber = -1; }

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
    void close(bool usbClose = true);

    /**
     * @brief Connect to this device
     *
     * @param pConnectCb Callback called when successfully done
     * @param audioSupport Enable/disable audio support
     * @param pToken The token returned within the callback
     * @param seconds The connect timeout in seconds
     * @param ctrlReqTmoMs The control request timeout in miliseconds
     * @return The result code of connect
     */
    int connect(aoap_connectCB pConnectCb, bool audioSupport, void *pToken, unsigned int seconds,
                unsigned int ctrlReqTmoMs);

    /**
     * @brief Read data synchronously
     *
     * The function read is a blocking call.
     *
     * @param pBuffer The pointer to the read buffer
     * @param bufferSize The size of the read buffer
     * @param timeout Timeout to read in milliseconds
     * @return The number of bytes read in case the value is 0 or positive. In case the return value is negative
     *         an error occurred
     */
    int read(unsigned char *pBuffer, int bufferSize, unsigned int timeout);

    /**
     * @brief Read data synchronously
     *
     * The function read is a blocking call.
     *
     * @param pBuffer The pointer to the read buffer
     * @param bufferSize The size of the read buffer
     * @param pTransferred The number of bytes read
     * @param timeout Timeout to read in milliseconds
     * @return 0 indicates success. In case the number is negative, an error occurred
     */
    int read(unsigned char *pBuffer, int bufferSize, unsigned int *pTransferred, unsigned int timeout);

    /**
     * @brief Write data synchronously
     *
     * If not all data could be written in one operation, this function will continue till all data is written or
     * an error occurs.
     * The function write is a blocking call.
     *
     * @param pBuffer The buffer to be written. In case internally not all bytes are written with the first attempt, the
     *        method will try to write the remaining data as well
     * @param bufferSize The buffer size which should be the number of bytes to be written
     * @param timeout Timeout for a single write operation in milliseconds
     * @return A positive value or 0 indicates success where the value is the number of written bytes. In case the number
     *         is negative, an error occurred
     */
    int write(const unsigned char *pBuffer, int bufferSize,
            unsigned int timeout);

    /**
     * @brief Write data synchronously
     *
     * If not all data could be written in one operation, this function will continue till all data is written or
     * an error occurs.
     * The function write is a blocking call.
     *
     * @param pBuffer The buffer to be written. In case internally not all bytes are written with the first attempt, the
     *        method will try to write the remaining data as well
     * @param bufferSize The buffer size which should be the number of bytes to be written
     * @param pTransferred The number of bytes written
     * @param timeout Timeout for a single write operation in milliseconds
     * @return 0 indicates success. In case the number is negative, an error occurred
     */
    int write(const unsigned char *pBuffer, int bufferSize,
            unsigned int *pTransferred, unsigned int timeout);

    /**
     * @brief Check if specified device supports AOAP
     *
     * @param major The major version number of AOAP
     * @param minor The minor version number of AOAP
     * @return The result of getting AOA protocol version. If successful, the result is AOAP_SUCCESS and the parameters
     *         major and minor contain the major and minor version number or 0 is not supported
     */
    int checkAoapSupport(unsigned int &majorVersion, unsigned int &minorVersion);

    /**
     * @brief Set the device number retrieved from 'udev' and updates the flag
     *
     * @param devNum The device number to be set
     */
    inline void setDevNum(unsigned int devNum) { mDevNumSet = true; mDevNum = devNum; }

    /**
     * @brief Get the device number
     *
     * @return The device number
     */
    inline unsigned int getDevNum(void) const { return mDevNum; }

    /**
     * @brief Set the previous device number
     *
     * @param devNum The new previous device number
     */
    inline void setPreviousDevNum(unsigned int devNum) {  mPrevDevNum = devNum; }

    /**
     * @brief Get the previous device number
     *
     * @return The previous device number. If not set, it is 0.
     */
    inline unsigned int getPreviousDevNum(void) const { return mPrevDevNum; }

    /**
     * @brief Finish switching to accessory mode
     *
     * Condition is that the device is in accessory mode
     */
    void finishSwitching(void);

    /**
     * @brief Send identifying string information using control transfers
     *
     * @param sendStringTmoMs The timeout of 'libusb_conrtol_transfer'to  in millisecond to send the string information to the MD.
     * @return 0 when successful. Otherwise the negative error code
     */
    int sendStringInformation(unsigned int sendStringTmoMs);

    /**
     * @implements timeoutHandler
     */
    virtual void timeoutHandler(void);

    static void enablePerformanceMeasurements(void);

private:

    /**
     * @brief Default constructor - prevent usage
     */
    Device(void); //lint !e1704

    /**
     * @brief Checks if the device is open
     * (this is true when the device handle exists)
     *
     * @return true when the handle exists
     */
    bool isOpen(void) const { if (mpDeviceHandle) { return true; } else { return false; } };

    /**
     * @brief Initializes this device (used within constructors)
     */
    void init(void);

    /**
     * @brief Switch the device to accessory mode
     *
     * @param seconds The connect timeout in seconds. Only required when switching
     *                the USB device to accessory mode. In other cases it can be set to 0
     * @param inTimeoutMs The control request timeout in miliseconds
     * @return 0 when successful. Otherwise the negative error code
     */
    int switchToAccessory(unsigned int seconds, unsigned int inTimeoutMs);

    /**
     * @brief Switch USB device to accessory mode using control transfers
     *
     * @param inTimeoutMs The control request timeout in miliseconds
     * @return 0 when successful. Otherwise the negative error code
     */
    int bringToAccessoryMode(unsigned int inTimeoutMs);

    /**
     * @brief Find the endpoints and store them in the corresponding member variables
     *
     * @return true when identifying endpoints succeeds otherwise false
     */
    int findEndpoints(void);

    /**
     * @brief Send control string for identification
     *
     * @param message The message text to be sent
     * @param type The type of the message (for the index of 'libusb_conrtol_transfer')
     * @param timeout The timeout of 'libusb_conrtol_transfer'. A timeout of 0 means it is unlimited. This is default
     * @return
     */
    int sendControlString(const std::string& message, tControlString type,
            unsigned int timeout = 0);

    /**
     * @brief Get AOA protocol version
     *
     * @param pBuffer The buffer for the version
     * @param bufferSize The size of the buffer (2 bytes are required)
     * @param timeout Request's timeout in milliseconds. Use 0 for unlimited timeout which is default
     * @return The result. A positive result means success
     */
    int getAoapProtocolVersion(uint8_t *pBuffer, int bufferSize,
            unsigned int timeout = 0);

    /**
     * @brief Set device state
     *
     * @param state The new device state
     */
    void setState(tState state);

    /**
     * Wrapper for 'libusb_control_transfer'
     *
     * @param dev_handle
     * @param request_type
     * @param bRequest
     * @param wValue
     * @param wIndex
     * @param data
     * @param wLength
     * @param timeout
     * @return
     */
    inline int sendControlTransfer(uint8_t request_type, uint8_t bRequest,
            uint16_t wValue, uint16_t wIndex, const unsigned char *data,
            uint16_t wLength, unsigned int timeout)
    {
        return libusb_control_transfer(mpDeviceHandle, request_type, bRequest,
                wValue, wIndex, const_cast<unsigned char *> (data), wLength,
                timeout);
    }

    /**
     * @brief Wrapper for 'libusb_claim_interface'
     *
     * @param interfaceNumber The interface number
     * @return The result. 0 means success. A negative value will indicate an error
     */
    int claimInterface(int interfaceNumber);

    /**
     * @brief Wrapper for 'libusb_release_interface'
     *
     * @return The result. 0 means success. A negative value will indicate an error
     */
    int releaseInterface();

    /**
     * @brief Get state as string
     *
     * @param state the state to be translated to string
     * @return The string describing the state
     */
    static std::string getStateString(tState state);

    /**
     * @brief Convert the control string type to a more readable text
     *
     * @param type The control string type
     * @return The readable text representing the control string type
     */
    std::string convertControlStringType(tControlString type) const;

    /**
     * @brief Prints the buffer depending on log level
     *
     * @param pBuffer The buffer to be printed
     * @param bufferSize The number of bytes to be printed
     *                   (typically the size of the buffer)
     */
    void printBytes(const unsigned char *pBuffer, int bufferSize) const;

    unsigned int mVendorId;
    unsigned int mProductId;
    const std::string mSerial;
    unsigned int mDevNum;
    unsigned int mPrevDevNum;
    bool mDevNumSet;
    unsigned int mId;
    libusb_device_handle *mpDeviceHandle;
    Accessory *mpAccessory;
    tState mState;
    tEndpoint mAccessoryEndpointIn;
    tEndpoint mAccessoryEndpointOut;
    bool mEnableAudio; //device shall enable audio
    bool mAudioEnabled; //status of enabling audio

    aoap_connectCB mpConnectCb;
    void *mpToken;

    int mInterfaceNumber;
    bool mAoapSupportQueried;
    unsigned int mMajorAoapVersion;
    unsigned int mMinorAoapVersion;
    int mClaimedInterfaceNumber;
    const int mMaxPrintBufferSize;

    /** Mutex for protecting access to write function from multiple threads */
    std::mutex mWriteMutex;
    libusb_context* mpUsbContext;

    static unsigned int gCounter;
    static bool gPerformanceMeasurements;

    unsigned int mIOErrCounter;
    unsigned int mReadIOErrCounter;
};

#endif /* AOAP_DEVICE_H_ */
