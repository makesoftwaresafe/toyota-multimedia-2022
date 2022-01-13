/*
 * aoap.h
 *
 *  Created on: Jul 18, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_H_
#define AOAP_H_

#include <stdint.h>
#include <stdbool.h>
#include "aoap_types.h"

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * @brief Create an accessory.
 *
 * If the AOAP component isn't yet initialized so far it will do this first. AOAP component gets
 * initialized only once. Function returns the accessory id for further usage and identification of
 * accessory. Deleting an accessory is possible with aoap_delete_accessory and the accessory ID
 * returned by aoap_create_accessory.
 *
 * @param pAccessory The structure containing the identification strings and properties
 *                   of the accessory
 * @return When a positive return value is returned it is the accessory id. In case of an failure the returned value is negative
 */
int aoap_create_accessory(t_aoap_accessory_param* pAccessory);

/**
 * @brief Deletes the specified accessory.
 *
 * If the last accessory gets deleted the dynamic components of the AOAP component gets destroyed
 * as well
 *
 * @param accessoryId The accessory to delete. This ID can be retrieved from aoap_create_accessory.
 */
void aoap_delete_accessory(unsigned int accessoryId);

/**
 * @brief Deletes the device specified by deviceId and deletes the specified accessory
 *        if no devices associated to the accessory.
 *
 * Releases the device from the device list.
 * If the specified device was the last device in the internal device list,
 * the accessory will be deleted too.
 * If the last accessory gets deleted the dynamic components of the AOAP component gets destroyed
 * as well.
 *
 * @param accessoryId The accessory in which context the device was connected.
 * @param deviceId The device ID to identify the device of the specified accessory
 * @return A negative value indicates an error
 */
int aoap_defer_delete_accessory(unsigned int accessoryId, unsigned int deviceId);

/**
 * @brief Connect a device to the specified accessory
 *
 * This function is asynchronous because it switches the USB device to accessory mode which means that the USB device
 * gets detached and re-attached.
 *
 * @param accessoryId The accessory ID which identifies the accessory. See return value of 'aoap_init'
 * @param vendorId The vendor ID of the USB device
 * @param productId The product ID of the USB device
 * @param pSerial The serial of the USB device for identification of the correct device
 * @param callback The connect callback to get a notification when switching to accessory mode finishes
 * @param audioSupport Enable/disable audio support for this device. Only relevant if the audio support
 *                     is enabled for this accessory. Disabling audio support might be necessary for
 *                     some Android devices which fail to switch to accessory mode when audio is
 *                     enabled (like the Samsung S3 with Android 4.1.2).
 *                     == 0: disable audio support
 *                     >  0: enable audio support
 * @param token A token which will be returned in the callback for identification of callers context
 * @return The device ID if positive value. A negative value indicates an error
 */
int aoap_connect(unsigned int accessoryId,
                 unsigned int vendorId,
                 unsigned int productId,
                 const char *pSerial,
                 aoap_connectCB callback,
                 unsigned int audioSupport,
                 void *token);

/**
 * @brief Set a timeout for connect attempts.
 *
 * This will overwrite the default value
 *
 * @param accessoryId The accessory ID for which this timeout applies
 * @param seconds The timeout in seconds
 */
void aoap_set_connect_timeout(unsigned int accessoryId, unsigned int seconds);

/**
 * @brief Set timeout for control requests such as start accessory mode, get protocol, identifying information.
 *
 * This will overwrite the default value
 *
 * @param accessoryId The accessory ID for which this timeout applies
 * @param inTimeoutMs The timeout in miliseconds
 */
void aoap_set_control_request_timeout(unsigned int accessoryId, unsigned int inTimeoutMs);

/**
 * Read data from the specified accessory and device synchronously
 *
 * This call is blocking/synchronous
 *
 * @param accessoryId The accessory ID to identify the accessory
 * @param deviceId The device ID to identify the device of the specified accessory
 * @param pBuffer The buffer where to read data to
 * @param bufferSize The size of the buffer.
 * @param timeout The timeout for reading in milliseconds. When the timeout is specified as 0,
 *                it waits (blocks) till data is read or an error occurs
 * @return The number of bytes read if positive. Or a negative value in error case indicating the error cause
 */
int aoap_read(unsigned int accessoryId, unsigned int deviceId, unsigned char *pBuffer, unsigned int bufferSize,
              unsigned int timeout);

/**
 * Read data from the specified accessory and device synchronously
 *
 * In contrast to aoap_read, the API return error codes in case of an I/O error (AOAP_ERROR_IO)
 * or timeout (AOAP_ERROR_TIMEOUT) and not the number of bytes transferred.
 * The number of bytes transferred will be returned in param 'pTransferred'.
 * This call is blocking/synchronous.
 *
 * @param accessoryId The accessory ID to identify the accessory
 * @param deviceId The device ID to identify the device of the specified accessory
 * @param pBuffer The buffer where to read data to
 * @param bufferSize The size of the buffer.
 * @param pTransferred Contains the number of bytes read.
 * @param timeout The timeout for reading in milliseconds. When the timeout is specified as 0,
 *                it waits (blocks) till data is read or an error occurs
 * @return 0 indicates success. Or a negative value in error case indicating the error cause.
 */
int aoap_read1(unsigned int accessoryId, unsigned int deviceId, unsigned char *pBuffer, unsigned int bufferSize,
               unsigned int *pTransferred, unsigned int timeout);

/**
 * @brief Write data to the specified accessory and device synchronously
 *
 * If not all data could be written in one operation, the call will try to write the remaining data in another operation
 * till it finishes to write all data or an error occurs.
 * This call is blocking.
 *
 * @param accessoryId Accessory ID to use
 * @param deviceId Device ID to use
 * @param pBuffer The buffer contains the data to be written
 * @param bufferSize The size of the buffer which is equal to the bytes to be written
 * @param timeout The timeout of a single write operation in milliseconds. A value of 0 disables the timeout
 * @return The number of bytes written in case it is positive. A negative value indicates an error
 */
int aoap_write(unsigned int accessoryId, unsigned int deviceId, const unsigned char *pBuffer, unsigned int bufferSize,
               unsigned int timeout);

/**
 * @brief Write data to the specified accessory and device synchronously
 *
 * If not all data could be written in one operation, the call will try to write the remaining data in another operation
 * till it finishes to write all data or an error occurs.
 * In contrast to aoap_write, the API return error codes in case of an I/O error (AOAP_ERROR_IO)
 * or timeout (AOAP_ERROR_TIMEOUT) and not the number of bytes transferred.
 * The number of bytes transferred will be returned in param 'pTransferred'.
 * This call is blocking.
 *
 * @param accessoryId Accessory ID to use
 * @param deviceId Device ID to use
 * @param pBuffer The buffer contains the data to be written
 * @param bufferSize The size of the buffer which is equal to the bytes to be written
 * @param pTransferred Contains the  number of bytes written
 * @param timeout The timeout of a single write operation in milliseconds. A value of 0 disables the timeout
 * @return 0 indicates success. A negative value indicates an error
 */
int aoap_write1(unsigned int accessoryId, unsigned int deviceId, const unsigned char *pBuffer, unsigned int bufferSize,
                unsigned int *pTransferred, unsigned int timeout);

/**
 * @brief Check if the specified device supports AOAP and in which major version
 *
 * @param accessoryId The accessory ID to be used
 * @param vendorId The vendor ID of the USB device
 * @param productId The product ID of the USB device
 * @param pSerial The serial of the USB device
 * @param[out] pMajor A pointer to the major version number. The value gets only populated when result is AOAP_SUCCESS
 * @param[out] pMinor A pointer to the minor version number. The value gets only populated when result is AOAP_SUCCESS
 * @return The result of the operation. When successful it returns AOAP_SUCCESS. In this case major and minor gets
 *         populated. Check both values to find out, if the device does or does not support AOAP and which version.
 *         In any error case the value is negative. In this case major and minor might not get populated
 */
int aoap_check_support(unsigned int accessoryId,
                       unsigned int vendorId,
                       unsigned int productId,
                       const char *pSerial,
                       unsigned int *pMajor,
                       unsigned int *pMinor);

/**
 * @brief Translate the integer result into a human readable string
 *
 * @param result The result/error code to be translated
 * @return The human readable string of the result/error code
 */
const char* aoap_get_result_as_string(int result);

/**
 * @brief Sets the log level for activating the different levels of DLT or print outs to stdout 
 *
 * @param logLevel The log level (valid range is 0..6). A value of 6 will
 *                 print everything including all data send/received
 * @param prependTime Prepend a timestamp to every debug message when printing to stdout
 */
void aoap_set_log_level(int logLevel, bool prependTime);

/**
 * @brief Enables/disables additional log destinations. Per default only DLT logging is enabled.
 *
 * @param destination The logging destination
 * @param enable TRUE to enable log destination, FALSE to disable log destination.
 * @param filename In case of enabled logging to file, with this function the log file including path can be specified.
 *                 If you do not want to change the currently set or default file location specify NULL
 */
void aoap_set_log_destination(t_aoap_logging_destination destination, bool enable, const char* filename);

/**
 * @brief Enables performance measurements. Attention: Do not call this in production code!
 */
void aoap_enable_performance_measurement(void);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* AOAP_H_ */

