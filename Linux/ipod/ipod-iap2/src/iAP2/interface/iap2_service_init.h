/*
 * iap2_service_init.h
 *
 *  Created on: 20-Feb-2017
 *      Author: dhana
 */

#ifndef IAP2_SERVICE_INIT_H_
#define IAP2_SERVICE_INIT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <iap2_init.h>

/**
 * Maximum size of Device Name
 */
#define DEVICE_NAME_SIZE 256

/**
 * \brief iAP2 Session message Type
 *
 * Session message type received from iAP2-Service
 */
typedef enum IAP2SessionType
{
    Control = 0,
    FileTransfer,
    EAP
}iAP2SessionType_t;

/**
 * \brief Device Information
 *
 */
typedef struct
{
    /*! Friendly name for the connected Device to show it on HMI*/
    char name[DEVICE_NAME_SIZE];
    /*! device serial number*/
    char serial[STRING_MAX];
}iAP2ServiceDeviceInformation_t;

/**
 * \brief Device connected message
 *
 * Notification on device connection with details
 *
 */
typedef struct
{
    /*! Friendly name for the connected Device to show it on HMI*/
    char name[DEVICE_NAME_SIZE];
    /*! device serial number*/
    char serial[STRING_MAX];
    /*! device identifier*/
    uint32_t id;
    /*! connected transport*/
    iAP2TransportType_t transport;
    /*! External Accessory Protocol supported*/
    BOOL eapSupported;
    /*! External Accessory Native supported*/
    BOOL eaNativeSupported;
    /*! CarPlay capable*/
    BOOL carplaySupported;
    /*! iAP2 device states
     *  This is used to distinguish between Pre and Post Identification DeviceConnected CB*/
    iAP2DeviceState_t deviceState;
}iAP2ServiceDeviceConnected_t;

/**
 * \brief DeviceList from iAP2Service
 *
 * Notification on device disconnection with details
 *
 */
typedef struct
{
    uint32_t count;
    iAP2ServiceDeviceConnected_t list[];
}iAP2ServiceDeviceList_t;

typedef void iAP2ServiceDeviceMessage_t;

/**
 * \brief response for ConnectDevice
 *
 * Provides ack/nack for connection request.
 *
 * Applications shall start sending iAP2 messages only after getting a successful connection message
 * All the messages sent in between ConnectDevice & ConnectDeviceResp are ignored by iAP2 service.
 *
 * \note All applications should send this message
 */
typedef struct
{
    int32_t result;
}iAP2ServiceConnectDeviceResp_t;

/**
 * \brief Device states during connection
 *
 * iAP2Service sends the connection state of the device detected by Master application.
 * This information shall be used by Master application to show the visual status in HMI.
 *
 */
typedef struct
{
    /*! device serial number*/
    char serial[STRING_MAX];
    /*! iAP2 device states*/
    enum _iAP2DeviceState state;
}iAP2ServiceDeviceState_t;

/*forward declaration*/
typedef struct iAP2Service iAP2Service_t;

/**
 * \addtogroup InterfaceAPIs
 * @{
 */
/**************************************************************************//**
 * This callback is called when a new Apple device is connected to iAP2Service
 *
 * \param[in] service Device structure
 * \param[in] msg information about the connected devices.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef int32_t (*iAP2ServiceDeviceConnectedCB)     (iAP2Service_t* service, iAP2ServiceDeviceList_t* msg);

/**************************************************************************//**
 * This callback is called when a new Apple device is disconnected from iAP2Service
 *
 * \param[in] service Device structure
 * \param[in] devId is the identifier of the device which was opened.
 * \param[in] msg information about the disconnected device.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef int32_t (*iAP2ServiceDeviceDisconnectedCB)  (iAP2Service_t* service, uint32_t devId);

/**************************************************************************//**
 * This callback is called when connect device is success/failure
 *
 * \param[in] service Device structure
 * \param[in] devId is the identifier of the device
 * \param[in] msg information about the connected device.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef int32_t (*iAP2ServiceConnectDeviceRespCB)   (iAP2Service_t* service, uint32_t devId, iAP2ServiceConnectDeviceResp_t* msg);

/**************************************************************************//**
 * This callback is called when connect device is success/failure
 *
 * \param[in] service Device structure
 * \param[in] devId is the identifier of the device
 * \param[in] msg information about the connected device.
 * \return Returns a signed integer value indicating success or failure
 *
 * \see
 * \note
 ******************************************************************************/
typedef int32_t (*iAP2ServiceDeviceStateCB) (iAP2Service_t* service, uint32_t devId, iAP2ServiceDeviceState_t* msg);

/** @} */

/**
 * \brief  iAP2 Service message callbacks
 *
 * Holds the iAP2 service messages handlers. Registered during iAP2ServiceInitialize.
 */
typedef struct
{
    iAP2ServiceDeviceConnectedCB        p_iAP2ServiceDeviceConnected_cb;
    iAP2ServiceDeviceDisconnectedCB     p_iAP2ServiceDeviceDisconnected_cb;
    iAP2ServiceDeviceStateCB            p_iAP2ServiceDeviceState_cb;

    iAP2ServiceConnectDeviceRespCB      p_iAP2ServiceConnectDeviceResp_cb;
}iAP2ServiceCallbacks_t;

/**
 * \brief Application/Client information
 *
 * Provides details on the Applications information in human readable format e.g. application name is "MediaPlayer".
 *
 * Note: Any application can send this message
 */
typedef struct
{
    /*! Unique identifier for each connected client*/
    uint32_t pid;

    /*! Friendly Name of the application/client*/
    char name[64];
}iAP2ServiceClientInformation_t;

/**
 * \brief The service structure
 *
 * Holds information about connected iAP2 Service. Such as Transport
 * file descriptor to communicate, Callbacks from applications etc...
 */
struct iAP2Service
{
    int iAP2ServerFd;
    iAP2ServiceCallbacks_t p_iAP2ServiceCallbacks;
};

/**
 * \addtogroup InterfaceAPIs
 * @{
 */
/***************************************************************************//**
 * Creates and returns the Socket File Descriptor to iAP2 service
 *
 * This should be the first call from Application to establish the connection
 * with iAP2-service daemon. This Fd will be used further to send and receive
 * iAP2 client-server and apple device messages.
 *
 * Please refer iap2_service_messages.h for list of messages which can be
 * send and received with iAP2-service-daemon.
 *
 * This is a blocking call. Please make sure iAP2-server is started before this
 * call.
 *
 * \param[in]  serviceCallbacks - callbacks for iAP2 service messages
 *
 * If Fd is negative, then application should retry to establish the connection
 *
 * \return File Descriptor to communicate with iAP2 Server
 *
 * \see
 * \note
 ******************************************************************************/
iAP2Service_t* iAP2ServiceInitialize(iAP2ServiceCallbacks_t* serviceCallbacks, iAP2ServiceClientInformation_t* clientInfo);


/***************************************************************************//**
 * De-Initializes the Service structure.
 *
 * This must be called during the shut-down process of the iAP2 stack and it
 * must be called after iAP2DisconnectDevice() of all connected devices.
 * This will de-initialize the service structure.
 *
 * \param[in]  service The Service device structure
 * \see
 * \note
 *
 ******************************************************************************/
void iAP2ServiceDeinitialize(iAP2Service_t** service);

/***************************************************************************//**
 * Send accessory information to iAP2Service for establishing connection
 *
 * Accessory configuration and identification informations are parsed and send
 * to iAP2 service as multiple iAP2Service messages. After sending configuration
 * iAP2 service establishes the connection with device based on the configuration.
 * When connection is successful iAP2 service will notify all Connected applications
 * about the new device connection. After that applications can start sending and
 * receiving iAP2 messages.
 *
 * \param[in]  service The Service data structure.
 * \param[in]  iap2InitParam - Initialize parameters to create device structure.
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note This must be called only from Master Application
 *
 ******************************************************************************/
int32_t iAP2ServiceDeviceDiscovered(iAP2Service_t* service, iAP2InitParam_t* iap2InitParam);

/***************************************************************************//**
 * Send device disappeared notification to iAP2Service to cleanup device resources
 *
 * Master application notifies iAP2Service about device removal to iAP2Service.
 * iAP2Service cleans up the resources created/used for the device communication.
 * Notifies all connected devices about the removal of the device.
 *
 * \param[in]  service The Service data structure.
 * \param[in]  deviceInfo - device information to identify unique
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note This must be called only by Master Application
 *
 ******************************************************************************/
int32_t iAP2ServiceDeviceDisappeared(iAP2Service_t* service, iAP2ServiceDeviceInformation_t* deviceInfo);


/**************************************************************************//**
 * Initialize the device structure for iAP2 application to use iAP2-service.
 *
 * This would initialize the iap2Device structure with necessary information
 * from iap2InitParam, which would be provided by the MC application.
 *
 * \param[in]  iap2InitParam is the source structure and application
 * is expected to provide the necessary values.
 * \param[in]  service is the iAP2Service_t structure
 * containing a file descriptor to communicate with iAP2Service
 * and the callbacks the application wants to register
 * \param[in]  deviceId is the unique identifier for connected apple device
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
iAP2Device_t* iAP2ServiceInitDeviceStructure(iAP2Service_t* service, uint32_t deviceId, iAP2InitParam_t* iap2InitParam);

/***************************************************************************//**
 * De-Initializes the device structure.
 *
 * This must be called during the shut-down process of the iAP2 stack and it
 * must be called after iAP2DisconnectDevice(). This will de-initialize the
 * device structure.
 *
 * \param[in]  service The service data structure.
 * \param[in]  device The device structure.
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
S32 iAP2ServiceDeInitDeviceStructure(iAP2Service_t* service, iAP2Device_t* device);

/***************************************************************************//**
 * Handles the receive event from iAP2 service
 *
 * This should be called when an Server FD is triggered by epoll_wait.
 * This function processes the data received from the iAP2 service and calls
 * message handler callbacks registered in iAP2ServiceInitialize function.
 *
 * \param[in]  service The Service data structure.
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
int iAP2ServiceHandleEvents(iAP2Service_t* service);

/***************************************************************************//**
 * Establish connection to the device
 *
 * This should be called with a valid iAP2Device_t structure.
 * Maintaining the Active device list is application's responsibility.
 *
 * \param[in]  service The Service data structure.
 * \param[in]  device - device to which application wants to connect.
 * \param[in]  initParam - init parameter used for creating device structure
 *
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
int iAP2ServiceInitDeviceConnection(iAP2Service_t* service, iAP2Device_t* device, iAP2InitParam_t* iap2InitParam);

/***************************************************************************//**
 * Disconnect the device for this application
 *
 * This should be called with a valid deviceId. DeviceID information is sent to
 * the application as soon as connected to the iAP2Service and also each device
 * connection or disconnection there after. Maintaining the Active device list
 * is application's responsibility.
 *
 * \param[in]  service The Service data structure.
 * \param[in]  deviceId device identifier to connect.
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
int iAP2ServiceDisconnectDevice(iAP2Service_t* service, uint32_t deviceId);

/***************************************************************************//**
 * Sends the iAP2Message to the iAP2 service for routing to device.
 *
 * This function sends the Message data to the iAP2 service.
 *
 * \param[in]  service The Service data structure.
 * \param[in]  data - message to send.
 * \param[in]  length - length of the data to send.
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
int iAP2ServiceSendMessageToDevice(const iAP2Device_t* device, void* data, uint32_t length, enum IAP2SessionType msgType);

/** @} */
#ifdef __cplusplus
}
#endif

#endif /* IAP2_SERVICE_INIT_H_ */
