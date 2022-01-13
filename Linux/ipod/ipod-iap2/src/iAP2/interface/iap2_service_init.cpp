/*
 * iap2_service_init.cpp
 *
 *  Created on: 21-Feb-2017
 *      Author: dhana
 */
#include <sys/un.h>
#include <unistd.h>
#include <errno.h>
#include <map>
#include <mutex>

#include "iAP2LinkRunLoop.h"
#include "iap2_service_init.h"
#include "iap2_service_init_private.h"
#include "iap2_service_messages.h"
#include "iap2_init_private.h"
#include "iap2_parameter_parsing.h"
#include "iap2_dlt_log.h"

/*! List of devices application has established connection*/
static std::map<uint32_t, iAP2Device_t*> connectedDevices;
static std::mutex sConnectedDevicesLock;

static int iAP2ServiceSendClientInformation(iAP2Service_t* service, iAP2ServiceClientInformation_t* iap2Client);
static void iAP2ServiceRemoveDevice(uint32_t deviceId);

iAP2Service_t* iAP2ServiceInitialize(iAP2ServiceCallbacks_t* serviceCallbacks, iAP2ServiceClientInformation_t* clientInfo)
{
    iAP2Service_t* service = (iAP2Service_t*)calloc(1, sizeof(*service));
    if((service == NULL) || (serviceCallbacks == NULL) || (clientInfo == NULL))
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "iAP2Service creation failed or callbacks:%p clientInfo:%p", serviceCallbacks, clientInfo);
        return NULL;
    }

    memcpy(&service->p_iAP2ServiceCallbacks, serviceCallbacks, sizeof(*serviceCallbacks));

    service->iAP2ServerFd = socket(PF_UNIX, SOCK_SEQPACKET, 0);

    struct sockaddr_un address;
    memset(&address, 0, sizeof(struct sockaddr_un));

    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, "/tmp/iAP2_Server", sizeof(address.sun_path));
    address.sun_path[sizeof(address.sun_path)-1] = '\0';

    if(connect(service->iAP2ServerFd, (struct sockaddr *) &address, sizeof(struct sockaddr_un)) != 0)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "connect() failed errno = %d, (%s)", errno, strerror(errno));
        close(service->iAP2ServerFd);
        service->iAP2ServerFd = -1;
    }
    else
    {
        iAP2ServiceSendClientInformation(service, clientInfo);
    }
    return service;
}

void iAP2ServiceDeinitialize(iAP2Service_t** service)
{
    sConnectedDevicesLock.lock();
    connectedDevices.clear();
    sConnectedDevicesLock.unlock();
    free(*service);
    *service = NULL;
}

int iAP2ServiceSendMessage(iAP2Service_t* service, void* data, uint32_t length)
{
    int32_t rc = 0;
    if((service) && (service->iAP2ServerFd >=0))
    {
        rc = send(service->iAP2ServerFd, data, length, 0);
        if(rc <= 0)
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "send() failed errno = %d, (%s)", errno, strerror(errno));
    }
    return rc;
}

int iAP2ServiceSendMessageToDevice(const iAP2Device_t* device, void* data, uint32_t length, enum IAP2SessionType sessionType)
{
    int32_t rc = 0;
    iAP2Device_st* device_st = (iAP2Device_st*)(device);
    iAP2Service_t* service = device_st->iAP2Service;

    iAP2AccessoryMessage msg;
    msg.header.type = MessageType::iAP2AccMsg;
    msg.header.deviceId = device_st->iAP2DeviceId;
    msg.sessionType = sessionType;
    msg.size = length;
    memcpy(msg.buffer, data, length);

    if((service) && (service->iAP2ServerFd >=0))
    {
        rc = iAP2ServiceSendMessage(service, &msg, sizeof(struct MessageHeader)+ sizeof(enum IAP2SessionType) + sizeof(uint32_t) + msg.size);

        if(rc <= 0)
            IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "service:%p device:%p length:%u", service, device, length);

        if(rc > 0) /*to match with error handling in iAP2 protocol handling*/
            rc = 0;
    }
    return rc;
}

/***************************************************************************//**
 * Processes the iAP2 message received from the device
 *
 * Parses the iAP2 message received from device and calls the callbacks
 * registered by the application. Applications should have registered the callbacks
 * during iAP2 device initialization.
 *
 * \param[in]  service The Service data structure.
 * \param[in]  device - device from which the message is received.
 * \param[in]  iAP2Msg - message received from device
 * \return Returns a signed integer value indicating success or failure
 * \see
 * \note
 *
 ******************************************************************************/
static int iAP2ServiceProcessDeviceMessage(iAP2Service_t* service, iAP2Device_st* device, iAP2ServiceDeviceMessage_t* msg)
{
    if(service == NULL || device == NULL || msg == NULL)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "service:%p device:%p iAP2Msg:%p", service, device, msg);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    /***************************** WARNING!!!!!!!!***********************************
     * For interface model: Link expected function prototype to receive message
     * Only context is used. All other members are reset. Do not use other members!!!
     ********************************************************************************/
    iAP2Link_t link;
    memset(&link, 0, sizeof(link));
    link.context = device;

    iAP2DeviceMessage* iAP2Msg = (iAP2DeviceMessage*)msg;
    return device->iAP2Link->recv(&link, iAP2Msg->buffer, iAP2Msg->size, iAP2Msg->sessionType);
}

int iAP2ServiceHandleEvents(iAP2Service_t* service)
{
    static uint8_t msg[MAX_MESSAGE_SIZE];

    int rc = recv(service->iAP2ServerFd, msg, sizeof(msg), 0);
    if(rc <= 0)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Connection with Server got broken! rc(%d) error(%d)(%s)", rc, errno, strerror(errno));
        return IAP2_CTL_ERROR; /*********************return*************************/
    }

    MessageHeader* msgBase = (MessageHeader*)&msg;
    char msgTypeString[STRING_MAX]= {0};
    iAP2ServiceGetMessageTypeString(msgBase->type, msgTypeString, STRING_MAX);
    IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "MessageType:(%s)(%u) deviceId:%u", msgTypeString, (uint32_t)msgBase->type, msgBase->deviceId);

    switch(msgBase->type)
    {
    case MessageType::DeviceConnected:
    {
        struct DeviceConnected* device = (struct DeviceConnected*)&msg;
        IAP2INTERFACEDLTLOG(DLT_LOG_DEBUG, "New device(s) connected! Count:%d", device->devices.count);

        if(service->p_iAP2ServiceCallbacks.p_iAP2ServiceDeviceConnected_cb)
            service->p_iAP2ServiceCallbacks.p_iAP2ServiceDeviceConnected_cb(service, &device->devices);
        break;
    }
    case MessageType::DeviceDisconnected:
    {
        iAP2ServiceRemoveDevice(msgBase->deviceId);
        if(service->p_iAP2ServiceCallbacks.p_iAP2ServiceDeviceDisconnected_cb)
            service->p_iAP2ServiceCallbacks.p_iAP2ServiceDeviceDisconnected_cb(service, msgBase->deviceId);
        break;
    }
    case MessageType::iAP2DeviceMsg:
    {
        sConnectedDevicesLock.lock();
        auto device = connectedDevices.find(msgBase->deviceId);
        auto mapEnd = connectedDevices.end();
        sConnectedDevicesLock.unlock();
        if(device != mapEnd)
        {
            rc = iAP2ServiceProcessDeviceMessage(service, (iAP2Device_st*)device->second, &msg);
        }
        else
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_WARN, "Matching device not found for deviceId(%d)!!", msgBase->deviceId);
        }
        break;
    }
    case MessageType::ConnectDeviceResp:
    {
        struct ConnectDeviceResp* serverResp = (struct ConnectDeviceResp*)&msg;
        iAP2ServiceConnectDeviceResp_t resp;
        resp.result = serverResp->result;
        if(service->p_iAP2ServiceCallbacks.p_iAP2ServiceConnectDeviceResp_cb)
            service->p_iAP2ServiceCallbacks.p_iAP2ServiceConnectDeviceResp_cb(service, msgBase->deviceId, &resp);
        break;
    }
    case MessageType::DeviceState:
    {
        struct DeviceState* dState = (struct DeviceState*)&msg;
        if(service->p_iAP2ServiceCallbacks.p_iAP2ServiceDeviceState_cb)
            service->p_iAP2ServiceCallbacks.p_iAP2ServiceDeviceState_cb(service, msgBase->deviceId, &dState->state);
        break;
    }
    case MessageType::EANativeTransport:
    {
        struct EANativeTransport* ean = (struct EANativeTransport*)&msg;
        sConnectedDevicesLock.lock();
        auto device = connectedDevices.find(msgBase->deviceId);
        auto mapEnd = connectedDevices.end();
        sConnectedDevicesLock.unlock();

        if(device != mapEnd)
        {
            iAP2Device_st* iap2Device = (iAP2Device_st*)device->second;
            if((ean->action == EAN_Start) && (iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StartEANativeTransport_cb!=NULL))
            {
                iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StartEANativeTransport_cb(iap2Device, ean->eapIdentifier, ean->sinkEndPoint, ean->sourceEndPoint, iap2Device->iAP2ContextCallback);
            }
            else if((ean->action == EAN_Stop) && (iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StopEANativeTransport_cb!=NULL))
            {
                iap2Device->p_iAP2EANativeTransportCallbacks.p_iAP2StopEANativeTransport_cb(iap2Device, ean->eapIdentifier, ean->sinkEndPoint, ean->sourceEndPoint,  iap2Device->iAP2ContextCallback);
            }
            else
            {
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Invalid Parameter for EA Native Transport Action deviceId(%d)!!", msgBase->deviceId);
                rc = IAP2_INVALID_INPUT_PARAMETER;
            }
        }
        else
        {
            IAP2INTERFACEDLTLOG(DLT_LOG_WARN, "Matching device not found for deviceId(%d)!!", msgBase->deviceId);
        }

        break;
    }
    default:
        break;
    }

    return rc;
}

int32_t iAP2ServiceInitDeviceConnection(iAP2Service_t* service, iAP2Device_t* device, iAP2InitParam_t* iap2InitParam)
{
    iAP2Device_st* iap2Device = (iAP2Device_st*)device;

    size_t eapIdsLength = sizeof(iOsAppInfo)+ sizeof(uint8_t) * iap2InitParam->p_iAP2AccessoryInfo->iAP2SupportediOSAppCount;
    size_t commandLength = sizeof(iAP2ServiceMessages_t) + iap2InitParam->p_iAP2AccessoryInfo->iAP2CommandsUsedByApplication_length;
    size_t callbacksLength = sizeof(iAP2ServiceMessages_t) + iap2InitParam->p_iAP2AccessoryInfo->iAP2CallbacksExpectedFromDevice_length;

    size_t length = sizeof(MessageHeader) + eapIdsLength + commandLength + callbacksLength;
    uint8_t *buf = (uint8_t*)calloc(1, length);
    if(!buf)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "ConnectDevice memory allocation fails for device(%d)!", iap2Device->iAP2DeviceId);
        return IAP2_ERR_NO_MEM;
    }

    struct ConnectDevice* connect = (struct ConnectDevice*)buf;
    connect->header.type     = MessageType::ConnectDevice;
    connect->header.deviceId = iap2Device->iAP2DeviceId;
    connect->header.length = length;

    iOsAppInfo* eapIds = (iOsAppInfo*)(buf + sizeof(MessageHeader));
    iAP2ServiceMessages_t* commands  = (iAP2ServiceMessages_t*)(buf + sizeof(struct ConnectDevice) + eapIdsLength);
    iAP2ServiceMessages_t* callbacks = (iAP2ServiceMessages_t*)(buf + sizeof(struct ConnectDevice) + eapIdsLength + commandLength);

    //iOS applications supported
    eapIds->count = iap2InitParam->p_iAP2AccessoryInfo->iAP2SupportediOSAppCount;
    for(uint32_t index = 0; index < iap2InitParam->p_iAP2AccessoryInfo->iAP2SupportediOSAppCount; ++index)
    {
        eapIds->appId[index] = iap2InitParam->p_iAP2AccessoryInfo->iAP2iOSAppInfo[index].iAP2iOSAppIdentifier;
        IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "EAP(%u) registered for Device(%u)!", eapIds->appId[index], iap2Device->iAP2DeviceId);
    }

    //Messages send by Accessory
    commands->length = iap2InitParam->p_iAP2AccessoryInfo->iAP2CommandsUsedByApplication_length;
    memcpy(&commands->commandList, iap2InitParam->p_iAP2AccessoryInfo->iAP2CommandsUsedByApplication, commands->length);

    //Messages expected from device
    callbacks->length = iap2InitParam->p_iAP2AccessoryInfo->iAP2CallbacksExpectedFromDevice_length;
    memcpy(&callbacks->commandList, iap2InitParam->p_iAP2AccessoryInfo->iAP2CallbacksExpectedFromDevice, callbacks->length);

    int32_t rc = iAP2ServiceSendMessage(service, connect, connect->header.length);
    if(rc > 0)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_INFO, "ConnectDevice message sent successfully for device(%u)", iap2Device->iAP2DeviceId);
        rc = IAP2_OK;
    }

    sConnectedDevicesLock.lock();
    connectedDevices.insert(std::pair<uint32_t, iAP2Device_t*>(iap2Device->iAP2DeviceId, device));
    sConnectedDevicesLock.unlock();

    return rc;
}

int iAP2ServiceDisconnectDevice(iAP2Service_t* service, uint32_t deviceId)
{
    struct DisconnectDevice disconnect_st;
    memset(&disconnect_st, 0, sizeof(disconnect_st));
    disconnect_st.header.type = MessageType::DisconnectDevice;
    disconnect_st.header.deviceId = deviceId;
    int32_t rc = iAP2ServiceSendMessage(service, &disconnect_st, sizeof(disconnect_st));
    if(rc <= 0)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "DisconnectDevice send failed with errno:%d (%s)", errno, strerror(errno));
    }

    iAP2ServiceRemoveDevice(deviceId);

    return rc;
}

static int iAP2ServiceSendClientInformation(iAP2Service_t* service, iAP2ServiceClientInformation_t* iap2Client)
{
    struct ClientInformation client;
    memset(&client, 0, sizeof(client));
    client.header.type = MessageType::ClientInformation;
    memcpy(&client.client, iap2Client, sizeof(*iap2Client));

    return iAP2ServiceSendMessage(service, &client, sizeof(client));
}

typedef int32_t (*sendIdentificationFunction)(iAP2Service_t* service, iAP2AccessoryInfo_t* initAccIdParams);
static sendIdentificationFunction senders[] = {
        sendAccessoryIdentificationBasic,
        sendAccessorySupportedLanguages,
        sendAccessorySupportedAudioRates,
        sendAccessoryVehicleInformation,
        sendAccessoryVehicleStatus,
        sendAccessoryLocationInformation,
        sendMessagesSentByApplication,
        sendCallbacksExpectedFromDevice,
        sendAccessorySupportediOSApps,
        sendUSBHID,
        sendUSBHostHID,
        sendBluetoothHID,
        sendBluetoothTransport,
        sendWirelessCarPlayTransport,
        sendRouteGuidanceDisplay
};

int32_t iAP2ServiceDeviceDiscovered(iAP2Service_t* service, iAP2InitParam_t* iap2InitParam)
{
    int32_t rc = IAP2_OK ;

    iAP2AccessoryInfo_t* initAccIdParams   = NULL;
    iAP2AccessoryConfig_t* initAccCfg      = NULL;

    initAccIdParams = iap2InitParam->p_iAP2AccessoryInfo;
    initAccCfg      = iap2InitParam->p_iAP2AccessoryConfig;

    if(NULL != initAccIdParams )
    {
        sendAccessoryConfiguration(service, initAccCfg, iap2InitParam);

        /*Accessory Identification Details*/
        for(uint32_t i =0; (i < (sizeof(senders)/sizeof(sendIdentificationFunction)) && (rc == IAP2_OK)); ++i)
        {
            rc = senders[i](service, initAccIdParams);
            if(rc < IAP2_OK)
                IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Failed in sending Identification Information");

            if(rc > 0)
                rc = 0;
        }

        sendInformationComplete(service);

    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_FATAL, "Invalid Parameter");
        rc = IAP2_BAD_PARAMETER;
    }

    if(rc != IAP2_OK)
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_ERROR, "Failed  rc:%d", rc);
    }

    return rc;
}

int32_t iAP2ServiceDeviceDisappeared(iAP2Service_t* service, iAP2ServiceDeviceInformation_t* deviceInfo)
{
    struct DeviceDisappeared msg;
    msg.header.type = DeviceDisappeared;
    msg.header.deviceId = 0;
    msg.header.length = sizeof(msg);
    strncpy(msg.serial, deviceInfo->serial, sizeof(deviceInfo->serial));

    int32_t rc = iAP2ServiceSendMessage(service, &msg, sizeof(msg));

    if(rc > 0)
        rc = IAP2_OK;

    return rc;
}

static void iAP2ServiceRemoveDevice(uint32_t deviceId)
{
    sConnectedDevicesLock.lock();
    auto device = connectedDevices.find(deviceId);
    auto mapEnd = connectedDevices.end();
    if(device != mapEnd)
    {
        connectedDevices.erase(deviceId);
    }
    else
    {
        IAP2INTERFACEDLTLOG(DLT_LOG_WARN, "Matching device not found for deviceId(%d)!!", deviceId);
    }
    sConnectedDevicesLock.unlock();
}
