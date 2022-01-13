/*
 * iap2_service_init_private.h
 *
 *  Created on: 17-Mar-2017
 *      Author: dhana
 */

#ifndef IAP2_SERVICE_INIT_PRIVATE_H_
#define IAP2_SERVICE_INIT_PRIVATE_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "iap2_service_init.h"

#define LANGUAGE_LENGTH             (4)
#define ADDRESS_BUS_LENGTH          (sizeof(size_t) + 1)

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
    char     iAP2UsbOtgGPIOPower[PATH_MAX];

    /*! Accessory authentication  type */
    iAP2AuthenticationType_t iAP2AuthenticationType;

    /**
     * \brief Authentication co-processor device path. Takes a string value
     */
    char     iAP2AuthDevicename[STRING_MAX];

    /**
     * \brief Authentication co-processor I/O control register address. Takes a string value
     */
    char     iAP2AuthIoctlRegAddr[ADDRESS_BUS_LENGTH];

    /**
     * \brief GPIO Ready pin. Takes a string.
     */
    char     iAP2AuthGPIOReady[ADDRESS_BUS_LENGTH];

    /**
     * \brief GPIO Reset pin. Takes a string.
     */
    char     iAP2AuthGPIOReset[ADDRESS_BUS_LENGTH];

    /**
     * \brief Time to wait after Authentication co-processor operation succeeds.
     * Default value is 1 ms.
     */
    S32     iAP2AuthShortWait;

    /**
     * \brief Time to wait after Authentication co-processor operation fails. Default value is 10 ms.
     */
    S32     iAP2AuthWait;

    /**
     * \brief Time to wait after Authentication co-processor operation fails very often. Default value is 50 ms.
     */
    S32     iAP2AuthLongWait;

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
    U8 UdcDeviceName[STRING_MAX];

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
}iAP2ServiceAccessoryConfig_t;


/**
 * \brief iOS App configuration details
 */
typedef struct
{
    /*! iOS App Identifier */
    U8      iAP2iOSAppIdentifier;

    /*! iOS App Name */
    char     iAP2iOSAppName[STRING_MAX];

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
}iAP2ServiceiOSAppInfo_t;

/**
 * \brief Accessory Identification base details
 */
typedef struct
{
    /*! Accessory Name (string) Ex: "AmazingProduct"    */
    char     iAP2AccessoryName[STRING_MAX];

    /*! Accessory Model Identifier (string) Ex: "15697" */
    char     iAP2AccessoryModelIdentifier[STRING_MAX];

    /*! Accessory Manufacturer (string) Ex: "ADIT"      */
    char     iAP2AccessoryManufacturer[STRING_MAX];

    /*! Accessory Serial Number (string) Ex: "12345678" */
    char     iAP2AccessorySerialNumber[STRING_MAX];

    /*! Accessory FirmwareVersion (string) Ex: "1"      */
    char     iAP2AccessoryFirmwareVersion[STRING_MAX];

    /*! Accessory HardwareVersion (string) Ex: "1"     */
    char     iAP2AccessoryHardwareVersion[STRING_MAX];

    /*! Preferred iOS App Bundle Seed Identifier */
    char     iAP2PreferredAppBundleSeedIdentifier[STRING_MAX];

    /*! Current language used  (string) Ex: "en" */
    char     iAP2CurrentLanguage[LANGUAGE_LENGTH];

    /*! If set accessory will support iOS in the car feature */
    BOOL    iAP2SupportsiOSintheCar;

    /*! Accessory Vendor ID */
    char    iAP2AccessoryVendorId[STRING_MAX];

    /*! Accessory Product ID */
    char    iAP2AccessoryProductId[STRING_MAX];

    /*! Accessory device-defined revision number */
    char     iAP2AccessoryBcdDevice[STRING_MAX];

    /*! Filename to the init endpoint e.g. /dev/ffs/ep0 */
    char     iAP2InitEndPoint[PATH_MAX];

    /*! Maximum Current Drawn From Device */
    U16     iAP2MaximumCurrentDrawnFromDevice;

    /*! Accessory Product plan UUID    */
    char     iAP2ProductPlanUUID[STRING_MAX];

} iAP2ServiceAccIdentification_t;

/**
 * \brief Accessory supported iOS applications
 */
typedef struct
{
    /*! Preferred iOS App Bundle Seed Identifier */
    char    iAP2PreferredAppBundleSeedIdentifier[STRING_MAX];

    /*! Number of supported iOS applications */
    U32     iAP2SupportediOSAppCount;

    /*! Array of supported iOS applications */
    iAP2ServiceiOSAppInfo_t iAP2iOSAppInfo[];
}iAP2ServiceiOSSupportedAppInfo_t;

/**
 * \brief Accessory supported iOS applications
 */
typedef struct
{
    /*! Number of supported messages   */
    U16  length;

    /*! List of supported messages   */
    U16 commandList[];
}iAP2ServiceMessages_t;

/**
 * \brief Accessory supported languages
 */
typedef struct
{
    /*! Number of supported languages provided   */
    U16     iAP2SupportedLanguageCount;

    /*! Supported languages by accessory Ex: {en:de} */
    char    iAP2SupportedLanguage[];
}iAP2ServiceSupportedLanguages_t;

/**
* \brief Audio Sample rates
*/
typedef struct
{
    /**
     * \brief Number of occurrences of iAP2USBDeviceSupportedAudioSampleRate parameter
     */
    U16     iAP2USBDeviceSupportedAudioSampleRate_count;

    /**
    * \brief Array of multiple sample rate values possible
    *
    * It refers to enum iAP2USBDeviceModeAudioSampleRate
    * Must be provided if the transport type is USBDeviceMode
    */
    iAP2USBDeviceModeAudioSampleRate iAP2USBDeviceSupportedAudioSampleRate[];
}iAP2ServiceAudioSampleRate_t;

/**
* \brief Vehicle Information details
*/
typedef struct
{
    /**
    * \brief DisplayName
    */
    char iAP2DisplayName[STRING_MAX];

    /**
    * \brief MapsDisplayName
    */
    char iAP2MapsDisplayName[STRING_MAX];

    /**
     * \brief Number of occurrences of iAP2EngineType parameter
     */
    U16     iAP2EngineType_count;

    /**
    * \brief Engine type of the vehicle.
    *
    *It refers to enum iAP2EngineTypes
    */
    iAP2EngineTypes iAP2EngineType[];
}iAP2ServiceVehicleInformation_t;

/**
 * \brief USBDeviceTransportComponent group
 */
typedef struct
{
    /**
    * \brief Component identifier of USB device transport.
    *
    * All iAP2TransportComponentIdentifiers must be unique.
    */
    U16 iAP2TransportComponentIdentifier;

    /**
    * \brief Component name of USB device transport.
    *
    * It takes null-terminated UTF-8 string.
    */
    U8 iAP2TransportComponentName[STRING_MAX];

    /**
    * \brief Present, if USB device transport supports iAP2 connection
    */
    BOOL iAP2TransportSupportsiAP2Connection;
} iAP2ServiceUSBDeviceTransport_t;

/**
* \brief HID component group
*/
typedef struct
{
    /**
    * \brief It specifies HID component function type.
    *
    * It refers to enum iAP2HIDComponentFunction
    */
    iAP2HIDComponentFunction iAP2HIDComponentFunction;
    /**
    * \brief Identifier of HID component.
    *
    * Must be unique HIDComponentIdentifier specified by the Accessory
    */
    U16 iAP2HIDComponentIdentifier;
    /**
    * \brief Name of HID component.
    *
    * It takes null-terminated UTF-8 string
    */
    U8  iAP2HIDComponentName[STRING_MAX];
}iAP2ServiceHID_t;

/**
 * \brief USBHostHIDComponent group
 */
typedef struct
{
    /**
    * \brief It specifies USB Host HID component function type.
    *
    * It refers to enum iAP2HIDComponentFunction
    */
    iAP2HIDComponentFunction iAP2HIDComponentFunction;

    /**
    * \brief Identifier of USB Host HID component.
    *
    * Must be unique HIDComponentIdentifier specified by the Accessory
    */
    U16 iAP2HIDComponentIdentifier;

    /**
    * \brief Name of USB Host HID component.
    *
    * It takes null-terminated UTF-8 string
    */
    U8 iAP2HIDComponentName[STRING_MAX];

    /**
    * \brief Refers to iAP2USBHostTransportComponentIdentifier of an iAP2USBHostTransportComponent
    */
    U16 iAP2USBHostTransportComponentIdentifier;

    /**
    * \brief Must match the accessory's corresponding USB device interface descriptor.
    *
    * If more than one USBHostHIDComponent is present, the accessory must present multiple USB HID interfaces with unique interface numbers.
    */
    U16 iAP2USBHostTransportInterfaceNumber;
} iAP2ServiceUSBHostHID_t;

/**
 * \brief BluetoothHIDComponent group
 */
typedef struct
{
    /**
    * \brief BT Transport Component Identifier
    */
    U16 iAP2BluetoothTransportComponentIdentifier;

    /**
    * \brief HID Component function
    */
    iAP2HIDComponentFunction iAP2HIDComponentFunction;

    /**
    * \brief HID component Identifier
    * All HID Component Identifiers must be unique
    */
    U16 iAP2HIDComponentIdentifier;

    /**
    * \brief Name of Bluetooth HID component
     */
    U8 iAP2HIDComponentName[STRING_MAX];
} iAP2ServiceBluetoothHID_t;


/**
 * \brief BluetoothTransportComponent group
 */
typedef struct
{
    /**
    * \brief Unique identifier for Bluetooth connection
    */
    U16 iAP2TransportComponentIdentifier;

    /**
    * \brief Bluetooth Transport component name.
    *
    * It takes null-terminated UTF-8 string
    */
    U8 iAP2TransportComponentName[STRING_MAX];

    /**
    * \brief Present, If Bluetooth component supports iAP2 connection.
    */
    U8 iAP2TransportSupportsiAP2Connection;

    /**
    * \brief A valid 6 byte IEEE EUI-48 identifier
    *
    * It contains MAC address of the bluetooth component.
    */
    U8 iAP2BluetoothTransportMediaAccessControlAddress[IAP2_BT_MAC_LENGTH];

} iAP2ServiceBluetoothTransport_t;

/**
 * \brief RouteGuidanceDisplayComponent group
 */
typedef struct
{
    /**
    * \brief Route Guidance Display Component Identifier
    */
    U16 iAP2Identifier;

    /**
    * \brief Number of occurrences of iAP2Identifier parameter
    */
    U16 iAP2Identifier_count;

    /**
    * \brief Route Guidance Display Component Name
    */
    U8 iAP2Name[STRING_MAX];

    /**
     * \brief Number of occurrences of iAP2Name parameter
     */
    U16 iAP2Name_count;

    /**
    * \brief Maximum number of characters that can be displayed by the accessory for Current Road Name
    */
    U16 iAP2MaxCurrentRoadNameLength;

    /**
     * \brief Number of occurrences of iAP2MaxCurrentRoadNameLength parameter
     */
    U16 iAP2MaxCurrentRoadNameLength_count;

    /**
    * \brief Maximum number of characters that can be displayed by the accessory for Destination Road Name
    */
    U16 iAP2MaxDestinationRoadNameLength;

    /**
     * \brief Number of occurrences of iAP2MaxDestinationRoadNameLength parameter
     */
    U16 iAP2MaxDestinationRoadNameLength_count;

    /**
    * \brief Maximum number of characters that can be displayed by the accessory for After Maneuver Road Name
    */
    U16 iAP2MaxAfterManeuverRoadNameLength;

    /**
     * \brief Number of occurrences of iAP2MaxAfterManeuverRoadNameLength parameter
     */
    U16 iAP2MaxAfterManeuverRoadNameLength_count;

    /**
    * \brief Maximum number of characters that can be displayed by the accessory for Maneuver Description
    */
    U16 iAP2MaxManeuverDescriptionLength;

    /**
     * \brief Number of occurrences of iAP2MaxManeuverDescriptionLength parameter
     */
    U16 iAP2MaxManeuverDescriptionLength_count;

    /**
    * \brief Maximum number of maneuvers the accessory can handle
    */
    U16 iAP2MaxGuidanceManeuverStorageCapacity;

    /**
     * \brief Number of occurrences of iAP2MaxGuidanceManeuverStorageCapacity parameter
     */
    U16 iAP2MaxGuidanceManeuverStorageCapacity_count;

    /**
    * \brief Maximum number of LaneGuidanceInfo the accessory can store
    */
    U16 iAP2MaxLaneGuidanceDescriptionLength;

    /**
     * \brief Number of occurrences of iAP2MaxLaneGuidanceDescriptionLength parameter
     */
    U16 iAP2MaxLaneGuidanceDescriptionLength_count;

    /**
    * \brief Maximum number of maneuvers the accessory can handle
    */
    U16 iAP2MaxLaneGuidanceStorageCapacity;

    /**
     * \brief Number of occurrences of iAP2MaxLaneGuidanceStorageCapacity parameter
     */
    U16 iAP2MaxLaneGuidanceStorageCapacity_count;


} iAP2ServiceRouteGuidanceDiplay_t;

/**
 * \brief WirelessCarPlayTransportComponent group
 */
typedef struct
{
    /**
    * \brief Transport Component Identifier
    *
    * All Transport Component identifiers must be unique
    */
    U16 iAP2TransportComponentIdentifier;

    /**
    * \brief Transport Component Name
    */
    U8 iAP2TransportComponentName[STRING_MAX];

    /**
    * \brief Whether Transport Supports CarPlay
    */
    U8 iAP2TransportSupportsCarPlay;

    /**
    * \brief Whether Transport Supports iAP2 Connection
    */
    U8 iAP2TransportSupportsiAP2Connection;
} iAP2ServiceWirelessCarPlayTransportComponent_t;

iAP2Device_t* iAP2InitDeviceStructureService( iAP2InitParam_t* iap2InitParam);

int iAP2ServiceSendMessage(iAP2Service_t* service, void* data, uint32_t length);

#ifdef __cplusplus
}
#endif


#endif /* IAP2_SERVICE_INIT_PRIVATE_H_ */
