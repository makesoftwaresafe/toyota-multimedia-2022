/*
 * iap2_service_messages.h
 *
 *  Created on: 30-Dec-2016
 *      Author: dhana
 */

#ifndef IAP2_SERVICE_MESSAGES_H_
#define IAP2_SERVICE_MESSAGES_H_

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
#include "iap2_service_init_private.h"

/**
 * Maximum message size over socket
 */
#define MAX_MESSAGE_SIZE ((64 * 1024) + (1 * 1024))  /*Max packet size on link layer is 64K (i.e 65535 bytes) +
                                                           iAP2 service message Header size (1024 bytes reserved )*/
/*TODO: Get the above value from Link layer*/

/**
 * \brief iAP2 feature set
 *
 *  iAP2 features as per the specification
 */
enum IAP2Features
{
    FeaturePlayback        = 0x0001, /**< Play back messages */
    FeatureMediaLibrary    = 0x0002, /**< Media Library messages */
    FeatureAppLaunch       = 0x0004, /**< App Launch messages */
    FeatureFileTransfer    = 0x0008, /**< File Transfer messages */
    FeatureEAP             = 0x0010, /**< EAP session messages */
    FeatureLocation        = 0x0020, /**< Location messages */
    FeatureVehicle         = 0x0040, /**< Vehicle messages */
    FeatureVoiceOver       = 0x0080, /**< Voice Over messages */
    FeatureCommunication   = 0x0100, /**< Communication messages */
    FeatureWiFi            = 0x0200  /**< WiFi messages */
};

/**
 * \brief iAP2 File Transfer Commands
 *
 * File Transfer Commands exchanged between iAP2-service and Application
 */
typedef enum FileTransferCmd
{
    //Messages Sent from Server to Client
    FileTransferSuccess = 0,
    FileTransferFailure,
    FileTransferCancel,
    FileTransferPause,
    FileTransferResume,
    FileTransferDataRcvd,
    FileTransferSetup,
    //Messages Sent from Application to Server
    FileTransferStartOK,
    FileTransferStartNOK
    //TBD TX msgs
}IAP2FTCmd_t;

/**
 * \brief iAP2 service Message Type
 *
 * Messages exchanged between iAP2-service and Application
 */
enum MessageType
{
    //Messages Sent from Server to Application
    Send2Client = 0,            /**< Start index of the messages sent from iAP2-Service to Application */
    DeviceConnected,            /**< List of devices connected successfully */
    DeviceDisconnected,         /**< Device disconnected successfully */
    iAP2DeviceMsg,              /**< Control Session Message */
    iAP2FileTransferMsg,        /**< FileTransfer session message */
    iAP2EAPMsg,                 /**< EAP session message */
    ConnectDeviceResp,          /**< Response for ConnectDevice message */
    DeviceState,                /**< Device states during connection> */
    EANativeTransport,          /**< EA Native Transport start/stop>*/

    MaxClientMSg,               /**< End index of the messages sent from iAP2-service to Application */

    //Messages Sent from Application to Server
    Send2Server = 128,          /**< Start index of the messages sent from Application to iAP2-Service */
    ClientInformation,          /**< Application/Client details like iOSAppName, iOSAppIdentifier etc.*/
    AccessoryConfiguration,     /**< Accessory configuration details */
    AccessoryIdentficiation,    /**< Accessory identification details */
    MessagsSentByApplication,   /**< List of messages sent by the application */
    CallbacksExpectedFromDevice,/**< List of messages expected from apple device */
    AccessorySupportedLanguages,/**< List of languages supported by the Accessory */
    AccessorySupportediOSApps,  /**< List of supported iOS applications by Accessory */
    USBDeviceAudioSampleRates,  /**< List of Audio Sample Rates supported by Accessory */
    USBDeviceTransport,         /**< USB device transport details */
    USBHostTransport,           /**< USB Host transport details */
    BluetoothTransport,         /**< Blue tooth transport details */
    WirelessCarPlayTransport,   /**< Wireless transport details */
    USBDeviceHID,               /**< USB device HID details */
    USBHostHID,                 /**< USB host HID details */
    BluetoothHID,               /**< Blue tooth HID details */
    VehicleInformation,         /**< Vehicle Information details */
    VehicleStatus,              /**< Vehicle Status details */
    LocationInformation,        /**< Location Information */
    RouteGuidanceDisplay,
    IdentificationInfoComplete, /**< Last message in Identification sequence */

    DeviceDiscovered,           /**< Master Application detects new device in the medium(USB, BT, Carplay) */
    DeviceDisappeared,          /**< Master Application detects removal of the device in the medium(USB, BT, Carplay) */

    ConnectDevice,              /**< Request to connect to the device */
    DisconnectDevice,           /**< Request to disconnect to the device */
    iAP2AccMsg,                 /**< Last message in Identification sequence */

    MaxServerMsg
};

/**
 * \brief EA Native Transport start/stop
 */
enum EANativeAction
{
    EAN_Unknown = 0,

    EAN_Start,  /**< Start EA Native Transport */
    EAN_Stop    /**< Stop EA Native Transport */
};

/**
 * \brief File Transfer message
 *
 * File Transfer message sent by Application as well as by iAP2-service
 *
 */
typedef struct iAP2FTMessage_st
{
    U32 deviceid;
    U64 len;
    IAP2FTCmd_t ftcmd;
    uint8_t ftid;
    U8 buff[MAX_MESSAGE_SIZE];
}iAP2FTMessage_t;
/**
 * \brief Message header
 *
 * Message header for iAP2 service message
 *
 */
struct MessageHeader
{
    /*! Message Type Identifier */
    enum MessageType type;

    /*! Device Identifier */
    uint32_t deviceId;

    /*! Payload size/length*/
    uint32_t length;
};


struct DeviceState
{
    /*! Message Header*/
    struct MessageHeader header;
    iAP2ServiceDeviceState_t state;
};

/**
 * \brief Device connected message
 *
 * Notification on device connection with details
 *
 */
struct DeviceConnected
{
    /*! Message Header*/
    struct MessageHeader header;
    /*! Information about connected device*/
    iAP2ServiceDeviceList_t devices;
};

/**
 * \brief Device Disconnected message
 *
 * Notification on device disconnection with details
 *
 */
struct DeviceDisconnected
{
    /*! Message Header*/
    struct MessageHeader header;
};

//TODO: use either DeviceDiscovered or AccessoryConfiguration message

/**
 * \brief Device Discovered message
 *
 * Notification to iAP2-service to know inform the arrival of new device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct DeviceDiscovered
{
    /*! Message Header*/
    struct MessageHeader header;
    char serial[STRING_MAX];
};

/**
 * \brief Accessory Configuration message
 *
 * First message in the sequence of providing configuration details to iAP2-service to start Authentication and Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct AccessoryConfiguration
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Apple device serial number*/
    char deviceSerial[DEVICE_NAME_SIZE];

    /*! Accessory configuration information*/
    iAP2ServiceAccessoryConfig_t accConfig;
};

/**
 * \brief Accessory Identification message
 *
 * Accessory Identification details to iAP2-service to start Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct AccessoryIdentficiation
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Accessory identification information*/
    iAP2ServiceAccIdentification_t identification;
};

/**
 * \brief iAP2 message Sent by Application message
 *
 * List of iAP2 messages sent by Accessory for Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct MessagsSentByApplication
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! List of messages to be send by the application to device*/
    iAP2ServiceMessages_t msgList;
};

/**
 * \brief iAP2 message expected From Device message
 *
 * List of iAP2 messages expected to be received from device for Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct CallbacksExpectedFromDevice
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! List of messages expected to be received by the application from device*/
    iAP2ServiceMessages_t msgList;
};

/**
 * \brief Accessory Supported Languages message
 *
 * List of supported languages by the Accessory for Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct AccessorySupportedLanguages
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! List of supported languages in Accessory*/
    iAP2ServiceSupportedLanguages_t languages;
};

/**
 * \brief Accessory Supported iOS applications message
 *
 * List of supported iOS applications in the Accessory for Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct AccessorySupportediOSApps
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! List of supported iOS applications by the Accessory*/
    iAP2ServiceiOSSupportedAppInfo_t apps;
};

/**
 * \brief USB device Audio Sample Rates message
 *
 * List of supported Audio Sample Rates by the Accessory for Identification with device.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct USBDeviceAudioSampleRates
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! List of Audio Sample rates supported by Accessory*/
    iAP2ServiceAudioSampleRate_t sampleRates;
};

/**
 * \brief USB Device Transport message
 *
 * Provides details about USB device Transport
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct USBDeviceTransport
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! USB Device transport component details*/
    iAP2ServiceUSBDeviceTransport_t usb;
};

/**
 * \brief Bluetooth Transport message
 *
 * Provides details about Bluetooth Transport
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct BluetoothTransport
{
    /*! Message Header*/
    struct MessageHeader header;

    /* Number of  iAP2ServiceBluetoothTransport_t elements*/
    uint32_t count;

    /*! Bluetooth transport component details*/
    iAP2ServiceBluetoothTransport_t bt[];
};

/**
 * \brief Wireless CarPlay Transport message
 *
 * Provides details about Wireless Transport
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct WirelessCarPlayTransport
{
    /*! Message Header*/
    struct MessageHeader header;

    /* Number of  iAP2WirelessCarPlayTransportComponent_t elements*/
    uint32_t count;

    /*! Wireless transport component details*/
    iAP2ServiceWirelessCarPlayTransportComponent_t wireless[];
};

/**
 * \brief USB device HID message
 *
 * Provides details about USB device HID component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct USBDeviceHID
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! number of iAP2ServiceHID_t elements */
    uint8_t count;

    /*! HID component details*/
    iAP2ServiceHID_t hid[];
};

/**
 * \brief USB host HID message
 *
 * Provides details about USB host HID component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct USBHostHID
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! number of iAP2ServiceHostHID_t elements */
    uint8_t count;

    /*! USB Host HID component details*/
    iAP2ServiceUSBHostHID_t hid[];
};

/**
 * \brief Bluetooth HID message
 *
 * Provides details about Bluetooth HID component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct BluetoothHID
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Bluetooth HID component details*/
    iAP2ServiceBluetoothHID_t hid;
};

/**
 * \brief Vehicle Information message
 *
 * Provides details about Vehicle Information component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct VehicleInformation
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Vehicle information details*/
    iAP2ServiceVehicleInformation_t vehicle;
};

/**
 * \brief Vehicle Status message
 *
 * Provides details about Vehicle Status component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct VehicleStatus
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Vehicle status details*/
    iAP2VehicleStatusComponent_t iAP2ServiceVehicleStatus;
};

/**
 * \brief Location Information message
 *
 * Provides details about Location Information component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct LocationInformation
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Location information details*/
    iAP2LocationInformationComponent_t iAP2ServiceLocationInformation;
};

/**
 * \brief Route Guidance Display Component message
 *
 * Provides details about Route Guidance Display component
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct RouteGuidanceDisplay
{
    /*! Message Header*/
    struct MessageHeader header;

    /* Number of  iAP2ServiceRouteGuidanceDiplay_t elements*/
    uint32_t count;

    /*! Route Guidance Display details*/
    iAP2ServiceRouteGuidanceDiplay_t iAP2ServiceRouteGuidanceDisplay[];
};

/**
 * \brief Identification Info complete message
 *
 * Last message to be sent to start the Authentication and Identification process on iAP2-service
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct IdentificationInfoComplete
{
    /*! Message Header*/
    struct MessageHeader header;
};

/**
 * \brief Device Disappeared message
 *
 * Notification to the iAP2-service about device removal. iAP2-service also get the disconnection when FDs are closed.
 *
 * Attention: Must be sent only by Master application.
 *
 */
struct DeviceDisappeared
{
    /*! Message Header*/
    struct MessageHeader header;
    char serial[STRING_MAX];
};

/**
 * \brief Connect Device message
 *
 * Provides details to establish the connection with iAP2-service for communicating with Apple device.
 *
 * Note: Any application can send this message
 */
struct ClientInformation
{
    /*! Message Header*/
    struct MessageHeader header;
    /*! Client information in human readable format*/
    iAP2ServiceClientInformation_t client;
};

struct iOsAppInfo
{
    uint32_t count;
    uint8_t appId[];
};

/**
 * \brief Connect Device message
 *
 * Provides details to establish the connection with iAP2-service for communicating with Apple device.
 *
 * Note: Any application can send this message
 */
struct ConnectDevice
{
    /*! Message Header*/
    struct MessageHeader header;
    /*! Payload is multiple messages (iOsAppInfo, iAP2ServiceMessages_t(command), iAP2ServiceMessages_t(callback)*/
    uint8_t payload[];
};

/**
 * \brief response for ConnectDevice
 *
 * Provides ack/nack for connection request.
 *
 * Attention: Applications shall start sending iAP2 messages only after getting a successful connection message
 * All the messages sent in between ConnectDevice & ConnectDeviceResp are ignored by iAP2 service.
 *
 * Note: Any application can send this message
 */
struct ConnectDeviceResp
{
    /*! Message Header*/
    struct MessageHeader header;

    int32_t result;
};

/**
 * \brief Disconnect Device message
 *
 * Provides details to disconnect with iAP2-service.
 *
 * Attention: Applications must not sent any messages after sending DisconnectDevice message.
 *
 * Note: Any application can send this message
 */
struct DisconnectDevice
{
    /*! Message Header*/
    struct MessageHeader header;
};

/**
 * \brief iAP2 device message
 *
 * iAP2 message received from Apple device in byte-stream format.
 * Application(s) receive this message and send it for further processing.
 *
 * Source: iAP2-service
 */
struct iAP2DeviceMessage
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Type of iAP2 message (Control, FileTransfer or EAP session)*/
    enum IAP2SessionType sessionType;

    /*! Actual size used in the buffer*/
    uint32_t size;

    /*! Buffer contains the actual message as byte stream as received from Apple Device*/
    uint8_t buffer[MAX_MESSAGE_SIZE];
};

/**
 * \brief iAP2 Accessory message
 *
 * iAP2 message sent to Apple device in byte-stream format.
 *
 * Source: Applications
 */
struct iAP2AccessoryMessage
{
    /*! Message Header*/
    struct MessageHeader header;

    /*! Type of iAP2 message (Control, FileTransfer or EAP session)*/
    enum IAP2SessionType sessionType;

    /*! Actual size used in the buffer*/
    uint32_t size;

    /*! Buffer contains the actual message as byte stream to Send it to the Apple Device*/
    uint8_t buffer[MAX_MESSAGE_SIZE];
};

/**
 * \brief EA Native Transport Start/Stop
 *
 * USB Host End point details to start/stop the communication with Apple device
 *
 * Note: Must be sent only by Master application.
 *
 */
struct EANativeTransport
{
    /*! Message Header*/
    struct MessageHeader header;
    /*! start / stop*/
    uint8_t action;
    /*! Protocol identifier for EA application*/
    uint8_t eapIdentifier;
    /*! USB Host OUT end point*/
    uint8_t sourceEndPoint;
    /*! USB Host IN end point*/
    uint8_t sinkEndPoint;
};

void iAP2ServiceGetMessageTypeString(enum MessageType type, char* string, size_t length);

int32_t sendAccessoryConfiguration(iAP2Service_t* service, iAP2AccessoryConfig_t* initAccCfg, iAP2InitParam_t* iap2InitParam);
int32_t sendAccessoryIdentificationBasic(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendAccessorySupportedLanguages(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendAccessorySupportedAudioRates(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendAccessoryVehicleInformation(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendAccessoryVehicleStatus(iAP2Service_t* service, iAP2AccessoryInfo_t *initAccIdParams);
int32_t sendAccessoryLocationInformation(iAP2Service_t* service, iAP2AccessoryInfo_t *initAccIdParams);
int32_t sendMessagesSentByApplication(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendCallbacksExpectedFromDevice(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendAccessorySupportediOSApps(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendUSBHID(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendUSBHostHID(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendBluetoothHID(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendBluetoothTransport(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendWirelessCarPlayTransport(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
int32_t sendRouteGuidanceDisplay(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);

int32_t sendInformationComplete(iAP2Service_t* service);

int32_t recvAccessoryConfiguration(struct AccessoryConfiguration* msg, iAP2AccessoryConfig_t* acc);
int32_t recvAccessoryIdentification(struct AccessoryIdentficiation* param, iAP2AccessoryInfo_t* accInfo);
int32_t recvSupportedLanguages(struct AccessorySupportedLanguages* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvSupportedAudioRates(struct USBDeviceAudioSampleRates* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvVehicleInformation(struct VehicleInformation* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvVehicleStatus(struct VehicleStatus* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvLocationInformation(struct LocationInformation* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvMessageSentByApplication(struct MessagsSentByApplication* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvCallbacksExpectedFromDevice(struct CallbacksExpectedFromDevice* msg, iAP2AccessoryInfo_t* accInfo);
int32_t recvSupportediOSAppInfo(struct AccessorySupportediOSApps* appInfo, iAP2AccessoryInfo_t* accInfo);
int32_t recvUSBHIDInformation(struct USBDeviceHID* message, iAP2AccessoryInfo_t* accInfo);
int32_t recvUSBHostHIDInformation(struct USBHostHID* message, iAP2AccessoryInfo_t* accInfo);
int32_t recvBluetoothHIDInformation(struct BluetoothHID* message, iAP2AccessoryInfo_t* accInfo);
int32_t recvBluetoothTransport(struct BluetoothTransport* message, iAP2AccessoryInfo_t* accInfo);
int32_t recvWirelessCarPlayTransport(struct WirelessCarPlayTransport* message, iAP2AccessoryInfo_t* accInfo);
int32_t recvRouteGuidanceDisplay(struct RouteGuidanceDisplay* message, iAP2AccessoryInfo_t* accInfo);
#ifdef __cplusplus
}
#endif

#endif /* IAP2_SERVICE_MESSAGES_H_ */
