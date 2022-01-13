/*
 * aoap_types.h
 *
 *  Created on: Jul 19, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#ifndef AOAP_TYPES_H_
#define AOAP_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus

/**
 * The Google vendor and accessory product IDs
 */
#define AOAP_GOOGLE_VENDOR_ID 0x18D1
#define AOAP_GOOGLE_PRODUCT_ID_ACCESSORY 0x2D00
#define AOAP_GOOGLE_PRODUCT_ID_ACCESSORY_ADB 0x2D01
#define AOAP_GOOGLE_PRODUCT_ID_AUDIO 0x2D02
#define AOAP_GOOGLE_PRODUCT_ID_AUDIO_ADB 0x2D03
#define AOAP_GOOGLE_PRODUCT_ID_ACCESSORY_AUDIO 0x2D04
#define AOAP_GOOGLE_PRODUCT_ID_ACCESSORY_AUDIO_ADB 0x2D05
#define AOAP_GOOGLE_PRODUCT_ID_LATEST AOAP_GOOGLE_PRODUCT_ID_ACCESSORY_AUDIO_ADB

/**
 * Definition of AOAP result codes.
 * libusb result codes > -100
 * AOAP specific result codes are <= -100
 */
/** The following codes match to libusb codes */
#define AOAP_SUCCESS 0
#define AOAP_ERROR_IO -1
#define AOAP_ERROR_INVALID_PARAM -2
#define AOAP_ERROR_ACCESS -3
#define AOAP_ERROR_NO_DEVICE -4
#define AOAP_ERROR_NOT_FOUND -5
#define AOAP_ERROR_BUSY -6
#define AOAP_ERROR_TIMEOUT -7
#define AOAP_ERROR_OVERFLOW -8
#define AOAP_ERROR_PIPE -9
#define AOAP_ERROR_INTERRUPTED -10
#define AOAP_ERROR_NO_MEM -11
#define AOAP_ERROR_NOT_SUPPORTED -12

/** NOTE: When adding new codes, make sure to update aoap_get_result_as_string() */

#define AOAP_ERROR_OTHER -99
/** AOAP specific codes (begin with 100)*/
#define AOAP_ERROR_GENERAL -100
#define AOAP_ERROR_MEMORY_FAULT -101
#define AOAP_ERROR_CLAIMING_USB_INTERFACE_FAILED -110
#define AOAP_ERROR_DEVICE_MISMATCH -111
#define AOAP_ERROR_ALREADY_DONE -112
#define AOAP_ERROR_ACCESSORY_NOT_FOUND -113
#define AOAP_ERROR_ERROR_NO_ENDPOINTS -114
#define AOAP_ERROR_CREATING_UDEV -115
#define AOAP_ERROR_ACCESSORY_NOT_SET -116
#define AOAP_ERROR_CONNECT_TIMEOUT -117
#define AOAP_ERROR_INVALID_PROTOCOL -118

/**
 * @typedef typedef enum LogDestination tLogDestination
 * @enum LogDestination
 * The different logging destinations
 */
typedef enum aoap_logging_destination
{
    eLogToNone =    0x00,  //!< eLogToNone (=> logging disabled)
    eLogToDLT =     0x01,  //!< eLogToDLT
    eLogToTTFis =   0x02,  //!< eLogToTTFis (currently not supported, for future use only)
    eLogToStdout =  0x04,  //!< eLogToStdout
    eLogToFile =    0x08   //!< eLogToFile
} t_aoap_logging_destination;

/**
 * @typedef struct aoap_accessory_param t_aoap_accessory_param
 * @struct aoap_accessory_param
 * The accessory parameters consisting of the six identification strings
 * and a flag for enabling audio
 */
typedef struct aoap_accessory_param
{
    char *manufacturer; ///The manufacturer of the accessory/application (for identification of Android application)
    char *modelName; ///The model name of the application (for identification of Android application)
    char *description; ///The description of the application (for identification of Android application)
    char *version; ///The version (for identification of Android application)
    char *uri; ///The URI where to download the application (for identification of Android application)
    char *serial; ///The serial (for identification of Android application)
    unsigned int enableAudio; ///=0 : disable audio. >0 : enable audio
} t_aoap_accessory_param;

/**
 * @brief Connect callback definition
 *
 * @param accessoryID The accessory id
 * @param deviceID The device id
 * @param result The result as integer. A positive value means success while a negative value
 *               indicates an error
 * @param token A user token provided in 'aoap_connect'
 * @param audio_support == 0: audio isn't enabled/supported
 *                      >  0: audio is supported and enabled
 */
typedef void (*aoap_connectCB)(int accessory_id, int device_id, int result, void *token,
        unsigned int audio_support);

/**
 * @brief USB monitor callback definition
 *
 * @param attach The value is 0 in case of a USB remove event. Another value means an USB add event
 * @param vendor_id The vendor id of the USB device
 * @param product_id The product id of the USB device
 * @param serial The serial of the USB device
 * @param token A user token provided when registering the USB monitoring with 'aoap_register_usb_monitor'
 */
typedef void (*aoap_usb_monitor_cb)(unsigned int attach, int vendor_id, int product_id, const char *serial, void *token);

#ifdef __cplusplus
}
#endif //__cplusplus

#endif /* AOAP_TYPES_H_ */
