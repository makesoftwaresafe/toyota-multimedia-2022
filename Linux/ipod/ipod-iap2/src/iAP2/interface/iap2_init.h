/***************************************************************************//**
 * \file iap2_init.h
 * \brief interface to MC application
 *
 * This header file contains all required definition for  accessory
 * configuration and APIs exported to the application. It includes
 * iap2_callbacks.h where application can register its call backs to
 * a specific event in the iAP2 protocol stack.
 *
 * \author
 * \date
 ******************************************************************************/



#ifndef IAP2_INIT_H
#define IAP2_INIT_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Device pointer exported to application.
 */
typedef void iAP2Device_t;

#include <semaphore.h>
#include "iap2_defines.h"
#include "iap2_parameters.h"
#include "iap2_callbacks.h"
#include "iap2_datacom.h"               /*TBD: Its from transport*/
#include "iap2_file_transfer.h"

/**
 * Maximum string length to use.
 */
#define STRING_MAX                      256

/**
 * USB packet header size.
 */
#define IAP2_USB_HEADER_SIZE            3

/**
 * Link control byte size for USB packet
 */
#define IAP2_USB_LINK_CTRL_BYTE_SIZE    1

/**
 * iAP2Link max packet size
 */
#define IAP2_LINK_MAX_PACKET_SIZE       0xFFFF
/**
 * \brief A macro to define receive buffer size
 *
 * Defines receive buffer MAX size . It should be sum total of PACKET_SIZE_MAX,
 * IAP2_USB_HEADER_SIZE and IAP2_USB_LINK_CTRL_BYTE_SIZE
 */
#define RXBUF_SIZE_MAX                  (IAP2_LINK_MAX_PACKET_SIZE \
                                        + (IAP2_USB_HEADER_SIZE \
                                        + IAP2_USB_LINK_CTRL_BYTE_SIZE))

/**
 * Length of NULL char
 */
#define IAP2_NULL_CHAR_LEN              1
#define IAP2_MAX_EA_APP_SUPPORTED       10

/**
 * Bluetooth transport component identifier and name
 */
#define IAP2_BT_TRANS_COMP_ID                   0x0000
#define IAP2_BT_TRANS_COMP_NAME                 "Bluetooth Transport"
#define IAP2_BT_MAC_LENGTH                      6
/**
 * USB Device Mode transport component identifier and name
 */
#define IAP2_USB_DEVICE_MODE_TRANS_COMP_ID      0x0100
#define IAP2_USB_DEVICE_MODE_TRANS_COMP_NAME    "USB Device Transport"
/**
 * USB Host Mode transport component identifier and name
 */
#define IAP2_USB_HOST_MODE_TRANS_COMP_ID        0x0200
#define IAP2_USB_HOST_MODE_TRANS_COMP_NAME      "USB Host Transport"
/**
 * Wireless CarPlay transport component identifier and name
 */
#define IAP2_WIRELESS_CARPLAY_TRANS_COMP_ID     0x0300
#define IAP2_WIRELESS_CARPLAY_TRANS_COMP_NAME   "Wireless CarPlay Transport"
/**
 * Route Guidance Display component identifier and name
 */
#define IAP2_ROUTE_GUIDANCE_DISPLAY_COMP_ID     0x0400
#define IAP2_ROUTE_GUIDANCE_DISPLAY_COMP_NAME   "Route Guidance Display Name"

typedef void (*iAP2AuthGetCertificate) (U16 *cert_data_len, U8 *cert_data);
typedef S32 (*iAP2AuthGetDeviceID) (U32 *auth_dev_id);
typedef S32 (*iAP2AuthGetSignature) (U16 sig_data_len, U8 *sig_data);
typedef S32 (*iAP2AuthGetFirmwareVersion) (U8 *majorVer, U8 *minorVer);
typedef S32 (*iAP2AuthGetProtocolVersion)(U8 *major_ver, U8 *minor_ver);
typedef S32 (*iAP2AuthGetChallengeData) (U16 *challenge_data_len, U8 *challenge_data);
typedef S32 (*iAP2AutSelftest) (U8 *certificate, U8 *private_key, U8 *ram_check, U8 *checksum);
typedef S32 (*iAP2AutGetSignatureData) (const U8  *response_buffer, U16 response_length, U16 *sig_data_len, U8 *sig_data);


/**
 * \brief Error states of the iAP2 protocol stack.
 *
 * This defines the error states , that will be set when there is an error
 * during the operation of the iAP2 stack.
 */
typedef enum
{
    iAP2NoError                   =  0, /**< No error  */
    iAP2TransportConnectionFailed = -1, /**< Error in  transport connection */
    iAP2LinkConnectionFailed      = -2, /**< Link connection failed */
    iAP2AuthenticationFailed      = -3, /**< Device authentication failed */
    iAP2IdentificationFailed      = -4, /**< Device identification failed */

} iAP2DeviceErrorState_t;


/**
 * \brief iAP2 device states
 *
 * This defines the different states of the device during initialization and
 * de-initialization of the iAP2 protocol stack.
 */
enum _iAP2DeviceState
{
    iAP2NotConnected        = 0,  /**< Initial/Final state */
    iAP2TransportConnected  = 1,  /**< Transport connection established */
    iAP2LinkConnected,            /**< Link established */
    iAP2AuthenticationPassed,     /**< Authentication Passed */
    iAP2IdentificationPassed,     /**< Identification Passed */

    iAP2DeviceReady        = 100, /**< Device ready to use */

    iAP2LinkiAP1DeviceDetected = 200, /**< iAP1 device detected */
    iAP2ComError           = -1,  /**< Protocol Error */
};

/**
 * Device States.
 */
typedef enum _iAP2DeviceState iAP2DeviceState_t;
typedef S32 (*iAP2DeviceStateCB)(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context);
typedef S32 (*iAP2Send2ApplicationsCB)(iAP2Device_t* iap2Device, uint16_t msgId, uint8_t* data, uint16_t length, void* context);
typedef S32 (*iAP2SendEAP2ApplicationCB)(iAP2Device_t* iap2Device, uint16_t sessionIdentifier, uint8_t* data, uint16_t length);
typedef S32 (*iAP2SendFileTransfer2ApplicationCB)(iAP2Device_t* iap2Device, uint16_t sessionIdentifier, uint8_t* data, uint32_t length);
typedef S32 (*iAP2StartEAPsessionCB)(iAP2Device_t* iap2Device, uint8_t protocolIdentifier, uint16_t sessionIdentifier);
typedef S32 (*iAP2StopEAPsessionCB)(iAP2Device_t* iap2Device, uint16_t sessionIdentifier);

struct _iAP2StackCallbacks
{
    iAP2DeviceStateCB p_iAP2DeviceState_cb;
    iAP2Send2ApplicationsCB p_iapSend2Application_cb;
    iAP2SendEAP2ApplicationCB p_iap2SendEAP2Application_cb;
    iAP2SendFileTransfer2ApplicationCB p_iap2SendFileTransfer2Application_cb;
    iAP2StartEAPsessionCB p_iap2StartEAPsession_cb;
    iAP2StopEAPsessionCB p_iap2StopEAPsession_cb;
    /*more to be added*/
};
typedef struct _iAP2StackCallbacks iAP2StackCallbacks_t;

/**
 * \brief Transport type available
 *
 * Transport type application intends to use to communicate with apple device.
 */
typedef enum
{
    iAP2USBHOSTMODE = 1,    /**< USB:Apple device in USB Host mode */
    iAP2USBDEVICEMODE,      /**< USB:Apple device in USB function mode */
    iAP2UART,               /**< Apple device connected using UART  */
    iAP2BLUETOOTH,          /**< Apple device connected using Bluetooth */
    iAP2OVERCARPLAY,        /**< Apple device connected via CarPlay - for Apple CarPlay over WiFi */
    iAP2MULTIHOSTMODE,      /**< Apple device and Accessory in Host mode through Molex Hub */
    iAP2GENERICMODE         /**< Generic. TBD  */
} iAP2TransportType_t;


/**
 * \brief Authentication Types
 *
 * Communication type with authentication co-processor. Application must set the
 * appropriate type, depending on how the co-processor is interfaced in the
 * product.
 */
typedef enum
{
    iAP2AUTHI2C = 1,    /**< Authentication co-processor interfaced using i2c */
    iAP2AUTHSPI,        /**< Authentication co-processor interfaced iusing SPI*/
    iAP2AUTHGENERIC     /**< Generic :TBD */

} iAP2AuthenticationType_t;

/**
 * \brief Auth-Com Function table.
 *
 * Provides function pointers where application can register its functions to
 * access the apple authentication co-processor.
 */
struct _iAP2AUTHCOMTABLE
{
    iAP2AuthGetCertificate      GetCertificate;/**< Get authentication certificate */
    iAP2AuthGetDeviceID         GetDeviceID;   /**< Get device ID */
    iAP2AuthGetSignature        GetSignature;  /**< Get Signature */
    iAP2AuthGetFirmwareVersion  GetFirmwareVersion; /**< Get firmware version */
    iAP2AuthGetProtocolVersion  GetProtocolVersion; /**< Get protocol version */
    iAP2AuthGetChallengeData    GetChallengeData;   /**< Get challenge data */
    iAP2AutSelftest             Selftest;           /**< Self test */
    iAP2AutGetSignatureData     GetSignatureData;   /**< Get signature data */
};

/**
 * Auth-Com function table.
 */
typedef struct _iAP2AUTHCOMTABLE iAP2AUTHCOMTABLE;

/**
 * \brief  Transport connection details.
 *
 * Holds the transport connection details for a specific device.
 */
typedef struct
{
    /**
     * iPod device handle from transport. This will be initialized by
     * iPodUSBDeviceComInit()in USB device-mode and by  iPodUSBHostComInit()
     * in USB Host-mode.
     * @note To make USB device-mode and host-mode plug-ins thread safe.
     */
    void*                   iAP2TransportHdl;
    void*                   iAP2DeviceIdentifier; /**< device Name/ID */
    iAP2TransportType_t     iAP2TransportType;    /**< Used transport type */
    /**
     * plug-in function table. open(), close(),read(), write(),abort(),ioctl(),
     * hdlevent(),getfds().
     */
    IPOD_IAP2_DATACOM_FUNC_TABLE iAP2DataComFunction;
    IPOD_IAP2_DATACOM_GET_FDS    iAP2PollFds;         /**< Fds for polling */
    IPOD_IAP2_DATACOM_IOCTL_CONFIG iAP2IoCtlConfig;   /**< I/O ctl configuration */
    IPOD_IAP2_DATACOM_ALTERNATE_IF_CB iAP2EAnativetransport_cb;  /**< Start/Stop Alternate Settings */
} iAP2Transport_t;

/**
 * \brief Authentication Details
 *
 * Provides authentication details such as authentication type used and function
 * table to access authentication co-processor.
 */
typedef struct
{
    iAP2AUTHCOMTABLE          iAP2AuthComFunction; /**<auth-com function table*/
    iAP2AuthenticationType_t  iAP2AuthenticationType;/**<auth-com type*/

} iAP2Authentication_t;

/**
 * \brief Accessory configuration details
 */
typedef struct
{
    /*! If set  iAP2iOSintheCar feature*/
    BOOL    iAP2iOSintheCar;

    /*! If set  EA native transport is required. */
    BOOL    iAP2EANativeTransport;

    /*! Accessory transport type */
    iAP2TransportType_t      iAP2TransportType;

    /*! GPIO power pin. Takes a string. */
    U8*     iAP2UsbOtgGPIOPower;

    /*! Accessory authentication  type */
    iAP2AuthenticationType_t iAP2AuthenticationType;

    /**
     * \brief Authentication co-processor device path. Takes a string value
     *  This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     */
    U8*     iAP2AuthDevicename;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief Authentication co-processor I/O control register address. Takes a string value
     */
    U8*     iAP2AuthIoctlRegAddr;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief GPIO Ready pin. Takes a string.
     */
    U8*     iAP2AuthGPIOReady;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief GPIO Reset pin. Takes a string.
     */
    U8*     iAP2AuthGPIOReset;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief Time to wait after Authentication co-processor operation succeeds.
     * Default value is 1 ms.
     */
    S32     iAP2AuthShortWait;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief Time to wait after Authentication co-processor operation fails. Default value is 10 ms.
     */
    S32     iAP2AuthWait;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief Time to wait after Authentication co-processor operation fails very often. Default value is 50 ms.
     */
    S32     iAP2AuthLongWait;

    /** This parameter is deprecated. The values passed will have no effect.
     *  Authentication configurations are directly read from Config file
     * \brief I2C address will be auto-detected if the below is set TRUE(For coprocessor version 2.0C and above)
     */
    U8*     iAP2AuthAutoDetect;

    /**
     * \brief PowerSourceUpdate.
     *
     * How much current the apple device must draw. Must be one of the following
     * values - 0,1000,2100,2400
     */
    U16     iAP2AvailableCurrentForDevice;

    /**
     * \brief PowerSourceUpdate.
     *
     * If set device battery will charge
     */
    BOOL    iAP2DeviceBatteryShouldChargeIfPowerIsPresent;

    /**
     * \brief StartPowerUpdate
     *
     * If set, accessory is requesting this information from apple device . In
     * response, apple device would send a U16 value as part of PowerUpdate msg
     * saying  how much current apple device would draw from the accessory
     */
    BOOL    iAP2MaximumcurrentDrawnFromAccessory;/**< Pointer to link timer handle*/

    /**
     * \brief StartPowerUpdate
     *
     * If set, accessory would request to apple device whether, its battery would
     * charge if power is present.
     */
    BOOL    iAP2DeviceBatteryWillChargeIfPowerIsPresent;

    /**
     * \brief StartPowerUpdate
     *
     * If set, accessory would request the power mode information from the apple
     * device.Power modes : Reserved, Low Power mode, Intermitent High Power mode
     *
     */
    BOOL    iAP2AccessoryPowerMode;

    /**
    * \brief FileXferRcvAsStream
    *
    * If set, During file transfer, Each stream of file data will be  provided
    * to  application
    */

    BOOL    iAP2FileXferRcvAsStream;

    /**
     * \brief iAP2EAPSupported
     *
     * If set, accessory will enable the configurations in link layer and control session
     * related to ExternalAccessoryProtocol Session
     *
     */
    BOOL    iAP2EAPSupported;

    /**
     * \brief iAP2FileXferSupported
     *
     * If set, accessory will enable the configurations in link layer and control session
     * related to FileTransfer Session
     *
     */
    BOOL    iAP2FileXferSupported;

    /**
     * \brief useConfigFS
     *
     * If set, accessory will use configfs for gadget creation
     * If USB Audio gadget is required, then this has to be enabled.
     *
     */
    BOOL useConfigFS;

    /**
     * \brief UdcDeviceName
     *
     * Name of the UDC Device to which the Apple device is connected.
     * If useConfigFS is set to TRUE, then UdcDeviceName should not be NULL.
     * CAUTION: This Parameter is deprecated, iAP2 Stack no longer uses it.
     *
     */
    U8* UdcDeviceName;

    /**
     * \brief ManualLinkCofig
     *
     * If set, accessory will use "LinkConfig_SessionVersion" value
     * for configuring the link negotiation parameter.
     *
     */
    BOOL ManualLinkConfig;

    /**
     * \brief LinkConfig_SessionVersion
     *
     * Will be used to set the session version during Link negotiation if "ManualLinkConfig"
     * is set to true else will be configured to default value of 2.
     *
     */
    U8 LinkConfig_SessionVersion;

    /**
     * \brief FileTransfer_SessionVersion
     *
     * Will be used to set the session version during Link negotiation if "iAP2FileXferSupported"
     * is set to true else will be configured to default value of 1.
     * For file transfer version 2 only one direction was tested,
     * because at the time of implementation there was no use case from Apple
     * for the direction Accessory to Device.
     *
     */
    U8 FileTransfer_SessionVersion;

    /**
     * \brief FileTransferConfig
     *
     * If set, accessory will use "FileTransfer_SessionVersion" value
     * for configuring the link negotiation parameter.
     *
     */
    BOOL FileTransferConfig;
} iAP2AccessoryConfig_t;

/**
 * \brief iOS App configuration details
 */
typedef struct
{
    /*! iOS App Identifier */
    U8      iAP2iOSAppIdentifier;

    /*! iOS App Name */
    U8*     iAP2iOSAppName;

    /*! iOS App Session Identifier - will be provided by Apple*/
    U16*    iAP2iOSAppSessionIdentifier; /* Will be provided by Apple Device */

    /*! If set iOS App require EA native transport */
    BOOL    iAP2EANativeTransport;

    /*! This must be set by a CarPlay accessory where this protocol
     * is intended to match with an app that supports CarPlay
     * If its TRUE, ExternalAccessoryProtocolCarPlay will be set in the identification
     * information under SupportedExternalAccessoryProtocol information. */
    BOOL    iAP2ExternalAccessoryProtocolCarPlay;

    /**
    * \brief It specifies whether device will attempt to find match app.
    *
    * It refers to enum iAP2ExternalAccessoryProtocolMatchAction
    */
    iAP2ExternalAccessoryProtocolMatchAction iAP2EAPMatchAction;
} iAP2iOSAppInfo_t;

/**
 * \brief VehicleInformationComponent configuration details
 */
typedef struct
{
    /**
    * \brief Engine type of the vehicle.
    *
    *It refers to enum iAP2EngineTypes
    */
    iAP2EngineTypes* iAP2EngineType;
    /**
     * \brief Number of occurrences of iAP2EngineType parameter
     */
    U16     iAP2EngineType_count;
    /**
    * \brief DisplayName
    */
    U8* iAP2DisplayName;
    /**
    * \brief MapsDisplayName
    */
    U8* iAP2MapsDisplayName;
}iAP2VehicleInformationComponent_t;

/**
 * \brief VehicleStatusComponent configuration details
 */
typedef struct
{
    /**
    * \brief Should be enabled if application wish to receive night mode status of the vehicle
    */
    BOOL    iAP2NightMode;

    /**
    * \brief Should be enabled if application wish to receive remaining vehicle range
    */
    BOOL    iAP2Range;

    /**
    * \brief Should be enabled if application wish to receive outside temperature of the vehicle
    */
    BOOL    iAP2OutsideTemperature;

    /**
    * \brief Should be enabled if application wish to receive inside temperature of the vehicle
    */
    BOOL    iAP2InsideTemperature;

    /**
    * \brief Should be enabled if application wish to receive whether low range warning is set or not
    */
    BOOL    iAP2RangeWarning;
    /**
	* \brief Should be enabled if application wish to receive remaining vehicle range using Gasoline
	*/
	BOOL    iAP2RangeGasoline;

	/**
	* \brief Should be enabled if application wish to receive remaining vehicle range using Diesel
	*/
	BOOL    iAP2RangeDiesel;

	/**
	* \brief Should be enabled if application wish to receive remaining vehicle range using Electric
	*/
	BOOL    iAP2RangeElectric;

	/**
	* \brief Should be enabled if application wish to receive remaining vehicle range using CNG
	*/
	BOOL    iAP2RangeCNG;

	/**
	* \brief Should be enabled if application wish to set low range warning indicator for gasoline
	*/
	BOOL    iAP2RangeWarningGasoline;

	/**
	* \brief Should be enabled if application wish to set low range warning indicator for diesel
	*/
	BOOL    iAP2RangeWarningDiesel;

	/**
	* \brief Should be enabled if application wish to set low range warning indicator for electricity
	*/
	BOOL    iAP2RangeWarningElectric;

	/**
	* \brief Should be enabled if application wish to set low range warning indicator for CNG
	*/
	BOOL    iAP2RangeWarningCNG;
}iAP2VehicleStatusComponent_t;

/**
 * \brief LocationInformationComponent configuration details
 */
typedef struct
{
    /**
    * \brief Should be enabled, if application is ready to send NMEA GPGGA sentences
    */
    BOOL    iAP2GlobalPositioningSystemFixData;

    /**
    * \brief Should be enabled, if application is ready to send NMEA GPRMC sentences
    */
    BOOL    iAP2RecommendedMinimumSpecificGPSTransitData;

    /**
    * \brief Should be enabled, if application is ready to send NMEA GPGSV sentences
    */
    BOOL    iAP2GPSSatelliteInView;

    /**
    * \brief Should be enabled, if application is ready to send NMEA PASCD sentences
    */
    BOOL    iAP2VehicleSpeedData;

    /**
    * \brief Should be enabled, if application is ready to send NMEA PAGCD sentences
    */
    BOOL    iAP2VehicleGyroData;

    /**
    * \brief Should be enabled, if application is ready to send NMEA PAAACD sentences
    */
    BOOL    iAP2VehicleAccelerometerData;

    /**
    * \brief Should be enabled, if application is ready to send NMEA GPHDT sentences
    */
    BOOL    iAP2VehicleHeadingData;
}iAP2LocationInformationComponent_t;

/**
 * \brief Accessory identification information
 */
typedef struct
{
    /*! Accessory Name (string) Ex: "AmazingProduct"    */
    U8*     iAP2AccessoryName;

    /*! Accessory Model Identifier (string) Ex: "15697" */
    U8*     iAP2AccessoryModelIdentifier;

    /*! Accessory Manufacturer (string) Ex: "ADIT"      */
    U8*     iAP2AccessoryManufacturer;

    /*! Accessory Serial Number (string) Ex: "12345678" */
    U8*     iAP2AccessorySerialNumber;

    /*! Accessory FirmwareVersion (string) Ex: "1"      */
    U8*     iAP2AccessoryFirmwareVersion;

    /*! Accessory HardwareVersion (string) Ex: "1"     */
    U8*     iAP2AccessoryHardwareVersion;

    /**
     * \brief Apple commands application intends to send to device.
     *
     * Should be provided as an array of message ID .
     * Ex : { 0x4C,0x00,0x4C, 0x03,...}
     */
    U16*    iAP2CommandsUsedByApplication;

    /*! Total number of commands application intends to use     */
    U16     iAP2CommandsUsedByApplication_length;

    /**
     * \brief Apple messages application intends to receive from device.
     *
     * Should be provided as an array of message ID .
     * Ex : { 0x4C, 0x01,0x4C, 0x04,...}
     */
    U16*    iAP2CallbacksExpectedFromDevice;

    /*! Total number of messages  application intends to receive */
    U16     iAP2CallbacksExpectedFromDevice_length;

    /*! Maximum Current Drawn From Device */
    U16     iAP2MaximumCurrentDrawnFromDevice;

    /*! iOS App Information */
    iAP2iOSAppInfo_t*   iAP2iOSAppInfo;

    /*! Total number of supported iOS App count */
    U32     iAP2SupportediOSAppCount;

    /*! Preferred iOS App Bundle Seed Identifier */
    U8*     iAP2PreferredAppBundleSeedIdentifier;

    /*! Current language used  (string) Ex: "en" */
    U8*     iAP2CurrentLanguage;

    /*! Supported languages by accessory ( Array of string) Ex: {"en","de"} */
    U8**    iAP2SupportedLanguage;

    /*! Number of supported languages provided   */
    U16     iAP2SupportedLanguageCount;

    /**
    * \brief Array of multiple sample rate values possible
    *
    * It refers to enum iAP2USBDeviceModeAudioSampleRate
    * Must be provided if the transport type is USBDeviceMode
    */
    iAP2USBDeviceModeAudioSampleRate* iAP2USBDeviceSupportedAudioSampleRate;

    /**
     * \brief Number of occurrences of iAP2USBDeviceSupportedAudioSampleRate parameter
     */
    U16     iAP2USBDeviceSupportedAudioSampleRate_count;

    /*! Deprecated:
     *  Use iAP2BluetoothTransportComponent* iAP2BluetoothTransportComponent instead.
     *  Bluetooth MAC address */
    U64*    iAP2BluetoothTransportMAC;

    /*! Deprecated:
     *  Use iAP2BluetoothTransportComponent* iAP2BluetoothTransportComponent instead.
     *  Number of occurrences of Bluetooth MAC address */
    U16     iAP2BluetoothTransportMAC_count;

    /**
    * \brief USB HID component of type group
    */
    iAP2iAP2HIDComponent* iAP2USBHIDComponent;
    /**
     * \brief Number of occurrences of iAP2USBHIDComponent parameter
     */
    U16     iAP2USBHIDComponent_count;

    /**
    * \brief VehicleInformationComponent.
    *
    * It refers to VehicleInformationComponent information that the application
    * would like to inform the Apple device.
    */
    iAP2VehicleInformationComponent_t* iAP2VehicleInformationComponent;

    /**
    * \brief VehicleStatusComponent.
    *
    * It refers to VehicleStatusComponent information that the application
    * would like to inform the Apple device.
    */
    iAP2VehicleStatusComponent_t* iAP2VehicleStatusComponent;

    /**
    * \brief LocationInformationComponent.
    *
    * It refers to LocationInformationComponent information that the application
    * would like to inform the Apple device.
    */
    iAP2LocationInformationComponent_t* iAP2LocationInformationComponent;

    /*! If set accessory will support iOS in the car feature */
    BOOL    iAP2SupportsiOSintheCar;

    /*! Accessory Vendor ID */
    U8*     iAP2AccessoryVendorId;

    /*! Accessory Product ID */
    U8*     iAP2AccessoryProductId;

    /*! Accessory device-defined revision number */
    U8*     iAP2AccessoryBcdDevice;

    /*! Filename to the init endpoint e.g. /dev/ffs/ep0 */
    U8*     iAP2InitEndPoint;

    /**
    * \brief USB Host HID component of type group
    */
    iAP2USBHostHIDComponent* iAP2USBHostHIDComponent;
    /**
    * \brief Number of occurrences of iAP2USBHostHIDComponent parameter
    */
    U16     iAP2USBHostHIDComponent_count;

    /**
    * \brief Bluetooth HID component of type group
    */
    iAP2BluetoothHIDComponent* iAP2BluetoothHIDComponent;

    /**
    * \brief iAP2BluetoothTransportComponent.
    *
    * It refers to iAP2BluetoothTransportComponent information.
    * Must be provided if the application would like to connect the Apple device via Bluetooth.
    */
    iAP2BluetoothTransportComponent* iAP2BluetoothTransportComponent;
    /**
     * \brief Number of occurrences of iAP2BluetoothTransportComponent parameter
     */
    U16     iAP2BluetoothTransportComponent_count;

    /**
    * \brief iAP2WirelessCarPlayTransportComponent.
    *
    * It refers to iAP2WirelessCarPlayTransportComponent information.
    * Must be provided if the application would like to support Carplay in Wireless mode.
    */
    iAP2WirelessCarPlayTransportComponent* iAP2WirelessCarPlayTransportComponent;
    /**
    * \brief Number of occurrences of iAP2WirelessCarPlayTransportComponent parameter
    */
    U16     iAP2WirelessCarPlayTransportComponent_count;

    /**
    * \brief Route Guidance Display Component of type group
    *
    * It must be declared if Route Guidance feature is used
    */
    iAP2RouteGuidanceDisplayComponent* iAP2RouteGuidanceDisplayComponent;
    /**
     * \brief Number of occurrences of iAP2RouteGuidanceDisplayComponent parameter
     */
    U16 iAP2RouteGuidanceDisplayComponent_count;

    /*! Accessory Product plan UUID    */
    U8* iAP2ProductPlanUUID;

} iAP2AccessoryInfo_t;


/**
 * \brief RunLoop handle
 */
typedef struct
{
    S32     iAP2RunLoopTimerFd;      /**< Timer fd for the main control loop  */
    void*   iAP2RunLoopTimerCallback;/**< Callback to be called on timer event*/
    void*   iAP2RunLoopTimer;        /**< Pointer to link timer handle*/
    U8      iAP2RunLoopCall;         /**< Set to call iAP2LinkRunLoopRunOnce()*/
    void*   iAP2RunLoopCallArg;      /**< Argument to iAP2LinkRunLoopRunOnce()*/
} iAP2RunLoop_t;

/**
 * \brief File descriptor and event on the file descriptor.
 */
typedef struct
{
    int     fd;     /**< file descriptor to be polled           */
    S16     event;  /**< event generated on the file descriptor */
} iAP2PollFDs_t;

/**
 * \brief List of file descriptors for polling.
 */
typedef struct
{
    iAP2PollFDs_t   *fds;       /**< FD list head        */
    S32             numberFDs;  /**< Total number of FDs */
} iAP2GetPollFDs_t;

/**
 * \brief Initial parameter structure.
 *
 * Information to be filled by the application. Later, iAP2 device structure
 * will be initialized with these values.
 */
struct _iAP2InitialParameter
{

    U32                      iAP2NumDevice ; /**< number of iAP2 devices */
    U8                       iAP2DeviceId[STRING_MAX];  /**< device ID/Name */
    void*                    iAP2ContextCallback;       /**< TBD            */

    /*! Accessory configuration details */
    iAP2AccessoryConfig_t*       p_iAP2AccessoryConfig;

    /*! Accessory Identification information details */
    iAP2AccessoryInfo_t*         p_iAP2AccessoryInfo;

    /*! Callbacks for iAP2 session message  */
    iAP2SessionCallbacks_t*      p_iAP2CSCallbacks;

    /*! Callbacks for iAP2 protocol stack generic events */
    iAP2StackCallbacks_t*        p_iAP2StackCallbacks;

    /*! Callbacks for iAP2 file transfer session  events */
    iAP2FileTransferCallbacks_t* p_iAP2FileTransferCallbacks;

    /* Note : MC should register for only one of the below callbacks for an instance
    p_iAP2EAPSessionCallbacks for single EAP session or p_iAP2MultiEAPSessionCallbacks for multiple EAP sessions */

    /*! Callbacks for iAP2 ExternalAccessoryProtocol session  events */
    iAP2EAPSessionCallbacks_t*   p_iAP2EAPSessionCallbacks;

    /*! Callbacks for iAP2 ExternalAccessoryProtocol Multi session  events */
    iAP2MultiEAPSessionCallbacks_t*   p_iAP2MultiEAPSessionCallbacks;

    /*! Callbacks for iAP2 ExternalAccessory Native Transport  events */
    iAP2EANativeTransportCallbacks_t*  p_iAP2EANativeTransportCallbacks;
};

/**
 * Initial parameter structure.
 */
typedef struct _iAP2InitialParameter iAP2InitParam_t;

/**
 * \addtogroup InterfaceAPIs
 * @{
 */

/**************************************************************************//**
 * Initialize the device structure.
 *
 * This would initialize the iap2Device structure with necessary information
 * from iap2InitParam, which would be provided by the MC application.
 *
 * \param[in]  iap2InitParam  is the soucce structure and application
 * is expected to provide the necessary values.
 *
 * \param[out] iap2Device is destination structure which gets
 * initialized from the values provided in iap2InitParam.
 *
 * \return Returns a pointer to iAP2Device_t device structure on success,
 * or NULL on failure
 *
 * \see
 * \note
 ******************************************************************************/
iAP2Device_t* iAP2InitDeviceStructure(iAP2InitParam_t*    iap2InitParam);


/**************************************************************************//**
 * Initialize the device connection.
 *
 * This must be called after iAP2InitDeviceStructure(). It takes the iap2Device
 * structure as input parameter. This would internally initialize the transport
 * connection and establish link with the iAP2 device.
 *
 * \param[in] iap2Device  the device structure to initiate the device connection
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2InitDeviceConnection(iAP2Device_t* iap2Device);

/***************************************************************************//**
 * Disconnect the transport and reset link
 *
 * This must be called when application intends to shut down the iAP2 stack .
 * It would reset the link and disconnect the already established transport
 * connection.
 *
 * \param[in] iap2Device
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
S32 iAP2DisconnectDevice(iAP2Device_t* iap2Device);


/***************************************************************************//**
 * De-Initializes the device structure.
 *
 * This must be called during the shut-down process of the iAP2 stack and it
 * must be called after iAP2DisconnectDevice(). This will de-initialize the
 * device structure.
 *
 * \param[in]  iap2Device The device structure.
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
S32 iAP2DeInitDeviceStructure(iAP2Device_t* iap2Device);


/***************************************************************************//**
 * Handles an event on a specific file descriptor
 *
 * This must be called inside the main control loop after select() and
 * should be called both for read file descriptors and write file descriptors.

 * \param[in] iap2Device    Initialized device structure
 * \param[in] fd    File descriptor for which the event will be handled
 * \param[in] event Event to be handled (currently unused)
 * \return A signed integer value indicating success or failure
 *
 * \see iAP2GetPollFDs()
 * \see select()
 * \see http://linux.die.net/man/2/select
 * \note
 ******************************************************************************/
S32 iAP2HandleEvent(iAP2Device_t* iap2Device, S32 fd, S16 event);


/***************************************************************************//**
 * Get the file descriptor to be polled .
 *
 * Application is expected to use runloop concept where in a single control
 * loop events from different sources must be polled and based on event source
 * (file descriptor) and event, it will be handled by iAP2HandleEvent().
 *
 * This must be called, before entering into the main control loop , to get get
 * all the file descriptors, those must be polled for events.
 *
 * Application must use select() or poll() system call to poll for events on the
 * file descriptors.
 *
 * select() must be called inside the main control loop after we have added all
 * the file descriptors to FD_SET()
 *
 * \param[in] iap2Device Initialized device structure. Its an input parameter
 * \param[out] getPollFDs Address to get poll fds. Its an output parameter
 * \return A signed integer value indicating success or failure
 *
 *
 * \see iAP2HandleEvent()
 * \see select()
 * \see http://linux.die.net/man/2/select
 * \note
 ******************************************************************************/
S32 iAP2GetPollFDs(iAP2Device_t* iap2Device, iAP2GetPollFDs_t* getPollFDs);

/***************************************************************************//**
 * Send Bad DETECT Ack to connected iAP1 apple device.
 *
 * If an iAP1 device detected and the accessory does not support iAP1 then it
 * must send the following byte sequence to the the device to indicate lack of
 * backward compatibility.
 *
 * Bad DETECT Ack byte sequence:
 *                              [ 0xFF, 0x55, 0x0E, 0x00, 0x13, 0xFF,
 *                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
 *                                0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xEB ]
 *
 * \param[in] iap2Device Initialized device structure.
 *
 * \return A signed integer value indicating success or failure
 *
 * \see  MFi Accessory Interface Specification R7
 * \note
 ******************************************************************************/
S32 iAP2CanceliAP1Support(iAP2Device_t* device);


/***************************************************************************//**
 * Get error state from the iAP2  Library
 *
 * Application may query the error state of the device from the library by this API
 * ErrorStates:
 *       iAP2NoError,
 *       iAP2TransportConnectionFailed,
 *       iAP2LinkConnectionFailed,
 *       iAP2AuthenticationFailed,
 *       iAP2IdentificationFailed
 *
 * \param[in] iap2Device Device pointer.
 * \param[in] context currently unused.
 *
 * \return Error state from iAP2 stack
 *
 * \see
 * \note
 ******************************************************************************/
iAP2DeviceErrorState_t iAP2GetDeviceErrorState(iAP2Device_t* device, void* context);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* IAP2_INIT_H */
