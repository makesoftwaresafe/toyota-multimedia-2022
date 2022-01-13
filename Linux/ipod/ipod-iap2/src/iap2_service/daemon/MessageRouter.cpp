/************************************************************************
 * @file: MessageRouter.cpp
 *
 * @version: 1.0
 *
 * @description: MessageRouter module provides the Routing table for
 * the messages send and received. This module manages the list of connected
 * clients and devices. This also manages the EAP and FileTransfer session
 * details.
 *
 * @component: platform/ipod
 *
 * @author: Dhanasekaran Devarasu, Dhanasekaran.D@in.bosch.com  2017
 *
 * @copyright (c) 2017 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * @see <related items>
 *
 * @history
 *
 ***********************************************************************/
#include <adit_logging.h>
#include <iap2_service_messages.h>
#include "MessageRouter.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

bool MessageRouter::addClient(int32_t clientId, int32_t sockFd, std::shared_ptr<ClientEvent> event)
{
    ClientInfo client;
    client.Id = clientId;
    client.sockFd = sockFd;
    client.event = event;

    std::unique_lock<std::mutex> lock(mClientsMutex);
    mClients.insert(std::pair<int16_t, ClientInfo>(clientId, client));
    return true;
}

bool MessageRouter::removeClient(int clientId)
{
    std::unique_lock<std::mutex> lock(mClientsMutex);
    auto it = mClients.find(clientId);
    if(it != mClients.end())
    {
        removeClientForiOSAppIds(clientId);
        unsubscribeClient(it->second.sockFd);
        removeEAPSessionIdentifier(it->second.sockFd);
        mClients.erase(clientId);
    }
    return true;
}

bool MessageRouter::updateClient(int clientId, struct ClientInformation& info)
{
    std::unique_lock<std::mutex> lock(mClientsMutex);
    auto it = mClients.find(clientId);
    if(it != mClients.end())
    {
        it->second.pid  = info.client.pid;
        it->second.name = info.client.name;
        LOG_INFO((iap2, "New Application connected: %s(%d)", it->second.name.c_str(), info.client.pid));
    }
    return true;
}

int32_t MessageRouter::getClientId(int sockFd)
{
    int32_t clientId = 0;
    std::unique_lock<std::mutex> lock(mClientsMutex);
    for(auto it = mClients.begin(); it != mClients.end(); ++it)
    {
        if(it->second.sockFd == sockFd)
        {
            clientId = it->second.Id;
            break;
        }
    }

    return clientId;
}

int32_t MessageRouter::getSocketFd(int clientId)
{
    int32_t sockFd = -1;
    std::unique_lock<std::mutex> lock(mClientsMutex);
    auto it = mClients.find(clientId);
    if(it != mClients.end())
    {
        sockFd = it->second.sockFd;
    }
    return sockFd;
}

int32_t MessageRouter::sendMessage(int clientId, void* buffer, uint32_t length)
{
    int rc = 0;
    std::vector<int32_t> clientList;
    std::unique_lock<std::mutex> lock(mClientsMutex);
    auto it = mClients.find(clientId);
    if(it != mClients.end())
    {
        clientList.push_back(it->second.sockFd);
        lock.unlock();
        sendMessage(clientList, buffer, length);
    }

    return rc;
}

int32_t MessageRouter::sendMessage(std::vector<int32_t>& clientList, void* buffer, uint32_t length)
{
    int32_t rc = -1;
    for(auto sockFd : clientList)
    {
        rc = send(sockFd, buffer, length, 0);
        if(rc <= 0)
            LOG_WARN((iap2, "Fails for clientFd:%d with errno:%d (%s)", sockFd, errno, strerror(errno)));
        else
            LOGD_DEBUG((iap2, "Message sent clientFd(%u) bytes(%u)", sockFd, rc));
    }
    return rc;
}

int32_t MessageRouter::broadcastMessage(void* buffer, uint32_t length)
{
    int32_t rc = 0;
    std::vector<int32_t> clientList;
    mClientsMutex.lock();
    for(auto it = mClients.begin(); it != mClients.end(); ++it)
    {
        clientList.push_back(it->second.sockFd);
    }
    mClientsMutex.unlock();
    sendMessage(clientList, buffer, length);

    return rc;
}

void MessageRouter::addiOSAppIdentifiers(uint32_t deviceId, uint32_t clientId, iOsAppInfo* apps)
{
    auto device = findDeviceInfo(deviceId);

    iap2_service_return_on_invalid_device(device, deviceId);
    std::string clientName("Unknown");
    std::unique_lock<std::mutex> lock(mClientsMutex);
    auto client = mClients.find(clientId);
    if(client != mClients.end())
    {
       clientName = client->second.name;
    }

    std::unique_lock<std::mutex> lock1(device->mEapMutex);
    for(uint32_t index = 0; index < apps->count; ++index)
    {
        auto eapId = apps->appId[index];
        auto entry = device->mEapIdentifier.find(eapId);
        if(entry == device->mEapIdentifier.end())
        {
            std::vector<uint32_t> clients;
            clients.push_back(clientId);
            device->mEapIdentifier.insert(std::pair<uint8_t, std::vector<uint32_t>>(eapId, clients));

            LOG_INFO((iap2, "Application(%d)(%s) registered for EAP identifier (%d) first time ", clientId,clientName.c_str(), (uint32_t)eapId));
        }
        else
        {
            entry->second.push_back(clientId);
            LOG_INFO((iap2, "Application(%d)(%s) registered for EAP identifier (%d) Added to list ", clientId, clientName.c_str(),(uint32_t)eapId));
        }
    }
}

void MessageRouter::removeClientForiOSAppIds(uint32_t clientId)
{
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    for(auto it = mDevices.begin(); it != mDevices.end(); ++it)
    {
        auto device = it->second;
        iap2_service_return_on_invalid_device(device, INVALID_DEVICE);
        std::unique_lock<std::mutex> eapLock(device->mEapMutex);
        auto eap = device->mEapIdentifier;
        for(auto it = eap.begin(); it != eap.end();++it)
        {
            for(auto idIt = it->second.begin(); idIt != it->second.end(); ++idIt)
            {
                if(*idIt == clientId)
                {
                    it->second.erase(idIt);
                    LOG_WARN((iap2, "Client ID %d removed for eap id %d for deviceId %d", clientId, it->first, device->id));
                    break;
                }
            }
        }
    }
}

void MessageRouter::removeClientForiOSAppIdsForDevice(uint32_t clientId, uint32_t deviceId)
{
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);

    std::unique_lock<std::mutex> eapLock(device->mEapMutex);
    auto eap = device->mEapIdentifier;
    for(auto it = eap.begin(); it != eap.end();++it)
    {
        for(auto idIt = it->second.begin(); idIt != it->second.end(); ++idIt)
        {
            if(*idIt == clientId)
            {
                it->second.erase(idIt);
                LOG_WARN((iap2, "Client ID %d removed for eapid %d for deviceId %d", clientId, it->first, deviceId));
                break;
            }
        }
    }
}

int32_t MessageRouter::setEAPSessionIdentifer(uint32_t deviceId, uint8_t eapId, uint16_t eapSessionId)
{
    int32_t rc = -1;
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_value_on_invalid_device(device, deviceId, rc);

    std::unique_lock<std::mutex> eapLock(device->mEapMutex);
    auto iOsApp = device->mEapIdentifier.find(eapId);
    if(iOsApp != device->mEapIdentifier.end())
    {
        std::vector<uint32_t>& clientIds = iOsApp->second;
        std::vector<int32_t> clientFds;
        for(auto Id : clientIds)
        {
            auto fd = getSocketFd(Id);
            clientFds.push_back(fd);
        }

        device->mEapSessionIdentifiers.insert(std::pair<int16_t, std::vector<int32_t>>(eapSessionId, clientFds));
        LOG_INFO((iap2, "Device(%d) starts EAP identifier(%d) session(%u)", deviceId, (uint32_t)eapId, eapSessionId));
        rc = 0;
    }
    else
    {
        LOG_ERROR((iap2, "No application registered for iOsApplicationIdentifier(%d) device(%d)", eapId, deviceId));
    }

    return rc;
}

int32_t MessageRouter::resetEAPSessionIdentifer(uint32_t deviceId, uint16_t eapSessionId)
{
    int32_t rc = -1;
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_value_on_invalid_device(device, deviceId, rc);

    std::unique_lock<std::mutex> eapLock(device->mEapMutex);
    auto entry = device->mEapSessionIdentifiers.find(eapSessionId);
    if(entry != device->mEapSessionIdentifiers.end())
    {
        device->mEapSessionIdentifiers.erase(entry);
        LOG_INFO((iap2, "Device(%d) stopped EAP identifier for session(%u)", deviceId, eapSessionId));
        rc = 0;
    }
    else
    {
        LOG_WARN((iap2, "EAPSessionIdentifier(%u) not found for deviceId(%u)", eapSessionId, deviceId));
    }
    return rc;
}

void MessageRouter::removeEAPSessionIdentifier(int32_t clientFd)
{
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    for(auto it = mDevices.begin(); it != mDevices.end(); ++it)
    {
        auto device = it->second;
        iap2_service_return_on_invalid_device(device, INVALID_DEVICE);
        std::unique_lock<std::mutex> lock(device->mEapMutex);
        for(auto it = device->mEapSessionIdentifiers.begin(); it != device->mEapSessionIdentifiers.end(); ++it)
        {
            for(auto fdIt = it->second.begin(); fdIt != it->second.end(); ++fdIt)
            {
                if(*fdIt == clientFd)
                {
                    it->second.erase(fdIt);
                    LOGD_DEBUG((iap2, "clientFd(%u) removed for EAP Session id(%u) for device %d ", clientFd, it->first, device->id));
                    break;
                }
            }
        }
    }
}

void MessageRouter::removeEAPSessionIdentifierForDevice(int32_t clientFd, uint32_t deviceId)
{
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);
    std::unique_lock<std::mutex> lock(device->mEapMutex);
    for(auto it = device->mEapSessionIdentifiers.begin(); it != device->mEapSessionIdentifiers.end(); ++it)
    {
        for(auto fdIt = it->second.begin(); fdIt != it->second.end(); ++fdIt)
        {
            if(*fdIt == clientFd)
            {
                it->second.erase(fdIt);
                LOGD_DEBUG((iap2, "clientFd(%u) removed for EAPSessionid(%u) for device %d ", clientFd, it->first, deviceId));
                break;
            }
        }
    }
}

void  MessageRouter::getClientFdsForiOSApp(uint32_t deviceId, int16_t eapSessionId, std::vector<int32_t>& clients)
{
    clients.clear();
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);

    std::unique_lock<std::mutex> eapLock(device->mEapMutex);
    auto entry = device->mEapSessionIdentifiers.find(eapSessionId);
    if(entry != device->mEapSessionIdentifiers.end())
        clients = entry->second;
    else
        LOG_WARN((iap2, "EAPSessionIdentifier(%u) not found for deviceId(%u)", eapSessionId, deviceId));
}

void MessageRouter::getClientIdsForEANativeApp(uint32_t deviceId, uint8_t eapIdentifier, std::vector<uint32_t>& clients )
{
    clients.clear();
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);

    std::unique_lock<std::mutex> eapLock(device->mEapMutex);
    auto entry = device->mEapIdentifier.find(eapIdentifier);
    if(entry != device->mEapIdentifier.end())
        clients = entry->second;
    else
        LOG_WARN((iap2, "EAPIdentifier(%u) for deviceId(%u) has no clients", eapIdentifier, deviceId));
}

void MessageRouter::subscribeiAP2Messages(int32_t clientFd, uint32_t deviceId, iAP2ServiceMessages_t* msg)
{
    LOG_INFO((iap2, "Application Id:(%d) subscribed for %zu iAP2Messages", getClientId(clientFd), msg->length/sizeof(uint16_t)));

    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);

    std::unique_lock<std::mutex> lock(device->mMessageMapMutex);
    for(uint32_t index = 0; index < msg->length/sizeof(uint16_t); ++index)
    {
        uint16_t msgId = msg->commandList[index];
        auto it = device->mMessageMap.find(msgId);
        if(it == device->mMessageMap.end()) //Create new entry.
        {
            device->mMessageMap.insert(std::pair<int16_t, std::vector<int32_t>>(msgId, std::vector<int32_t>()));
        }
        device->mMessageMap[msgId].push_back(clientFd);
        LOGD_DEBUG((iap2, "iAP2Message:0x%x subscribed by (%zu) Application", msgId, device->mMessageMap[msgId].size()));
    }
}

void MessageRouter::getSubscribedClients(uint16_t msgId, uint32_t deviceId, std::vector<int32_t>& clients)
{
    clients.clear();

    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);

    std::unique_lock<std::mutex> lock(device->mMessageMapMutex);
    auto it = device->mMessageMap.find(msgId);
    if(it != device->mMessageMap.end())
    {
        clients = it->second;
    }
}

void MessageRouter::unsubscribeClientForDevice(int32_t clientFd, uint32_t deviceId)
{
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_on_invalid_device(device, deviceId);

    std::unique_lock<std::mutex> lock(device->mMessageMapMutex);
    for(auto it = device->mMessageMap.begin(); it != device->mMessageMap.end(); ++it)
    {
        for(auto fdIt = it->second.begin(); fdIt != it->second.end(); ++fdIt)
        {
            if(*fdIt == clientFd)
            {
                it->second.erase(fdIt);
                break;
            }
        }
    }
}

void MessageRouter::unsubscribeClient(int32_t clientFd)
{
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    for(auto it = mDevices.begin(); it != mDevices.end(); ++it)
    {
        auto device = it->second;
        std::unique_lock<std::mutex> lock(device->mMessageMapMutex);
        for(auto it = device->mMessageMap.begin(); it != device->mMessageMap.end(); ++it)
        {
            for(auto fdIt = it->second.begin(); fdIt != it->second.end(); ++fdIt)
            {
                if(*fdIt == clientFd)
                {
                    it->second.erase(fdIt);
                    break;
                }
            }
        }
    }
}

ClientSubscription MessageRouter::subscribeClientForDevice(int32_t clientId, int32_t clientFd, uint32_t deviceId)
{
    (void)clientFd;
    auto device = findDeviceInfo(deviceId);
    iap2_service_return_value_on_invalid_device(device, deviceId, ClientSubscription::DeviceNotConnected);

    std::unique_lock<std::mutex> lock(device->clientsMutex);
    if(std::find(device->clients.begin(), device->clients.end(), clientId) == device->clients.end())
    {
        device->clients.push_back(clientId);
        return ClientSubscription::Success;
    }
    else
    {
        LOG_WARN((iap2, "Client (%d) is already connected to the device (%d). Multiple connect request sent", clientId, deviceId));
        return ClientSubscription::ClientAlreadyConnected;
    }
}

std::shared_ptr<DeviceInfo> MessageRouter::findDeviceInfo(uint32_t deviceId)
{
    std::shared_ptr<DeviceInfo> devInfoFound = nullptr;
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    auto it = mDevices.find(deviceId);
    if(it != mDevices.end())
    {
        devInfoFound = it->second;
    }
    else
    {
        LOG_WARN((iap2, "Device Id(%d) not found", deviceId));
    }
    return devInfoFound;
}

std::shared_ptr<DeviceInfo> MessageRouter::findDeviceInfo(const char* serial)
{
    std::shared_ptr<DeviceInfo> deviceInfoFound = nullptr;
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    for(auto it = mDevices.begin(); it != mDevices.end(); ++it)
    {
        auto devInfo = it->second;
        int diff = strncmp(serial, (char*)devInfo->device->iAP2Transport.iAP2DeviceIdentifier, STRING_MAX);
        if(diff == 0)
        {
            deviceInfoFound = devInfo;
            LOGD_DEBUG((iap2, "Device with serial %s found", serial));
            break;
        }
    }
    if(deviceInfoFound == nullptr)
    {
        LOG_WARN((iap2, "Device with serial %s not found", serial));
    }
    return deviceInfoFound;
}

int MessageRouter::sendConnectedDeviceList(iAP2Device_t* thisDevice, int32_t clientId)
{
    uint32_t count = 1;
    int32_t rc = -1;
    if(thisDevice == NULL)
    {
        mDevicesMutex.lock();
        count = mDevices.size();
        mDevicesMutex.unlock();
    }

    uint32_t length = sizeof(struct DeviceConnected) + sizeof(iAP2ServiceDeviceConnected_t) * count;
    struct DeviceConnected *msg = (struct DeviceConnected*)calloc(1, length);
    if((msg == NULL) || (count == 0))
    {
        LOG_WARN((iap2, "sendConnectedDeviceList failed! msg(%p) Device count(%u)", msg, count));
        free(msg);
        return IAP2_ERR_NO_MEM;
    }

    msg->header.type = MessageType::DeviceConnected;
    msg->header.length = length;
    uint32_t i = 0;
    mDevicesMutex.lock();
    for(auto it = mDevices.begin(); it != mDevices.end(); ++it)
    {
        auto device = it->second;
        iAP2Device_st* iap2Device = (iAP2Device_st*)device->device.get();
        if((thisDevice == NULL) || (thisDevice == iap2Device))
        {
            memcpy(msg->devices.list[i].serial, iap2Device->iAP2Transport.iAP2DeviceIdentifier, strnlen((char*)iap2Device->iAP2Transport.iAP2DeviceIdentifier, STRING_MAX));
            msg->devices.list[i].transport = iap2Device->iAP2Transport.iAP2TransportType;
            msg->devices.list[i].eapSupported = iap2Device->iAP2AccessoryConfig.iAP2EAPSupported;
            msg->devices.list[i].eaNativeSupported = iap2Device->iAP2AccessoryConfig.iAP2EANativeTransport;
            msg->devices.list[i].carplaySupported = iap2Device->iAP2AccessoryConfig.iAP2iOSintheCar;
            msg->devices.list[i].id = iap2Device->iAP2DeviceId;
            msg->devices.list[i].deviceState = iap2Device->iAP2DeviceState;
            msg->devices.count++;
            ++i;
        }
    }
    mDevicesMutex.unlock();

    if(clientId != -1)
        rc = sendMessage(clientId, msg, length);
    else
        rc =  broadcastMessage(msg, length);

    free(msg);
    return rc;

}

int MessageRouter::sendDeviceDisconnected(uint32_t deviceId)
{
    struct DeviceDisconnected msg;

    msg.header.type = MessageType::DeviceDisconnected;
    msg.header.length = sizeof(msg);
    msg.header.deviceId = deviceId;

    return broadcastMessage(&msg, sizeof(msg));
}

bool MessageRouter::addDevice(std::shared_ptr<DeviceInfo> device)
{
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    auto entry = mDevices.find(device->id);
    if(entry == mDevices.end())
    {
        mDevices.insert(std::pair<uint32_t, std::shared_ptr<DeviceInfo>>(device->id, device));
        lock.unlock();
        LOGD_DEBUG((iap2, "Device(%u) added!", device->id));
    }
    else
    {
        LOG_ERROR((iap2, "Device(%u) already exist!", device->id));
    }
    return true;
}

bool MessageRouter::removeDevice(std::shared_ptr<DeviceInfo> device)
{
    std::unique_lock<std::mutex> lock(mDevicesMutex);
    auto deviceId = device->id;
    auto entry = mDevices.find(deviceId);
    if(entry != mDevices.end())
    {
        mDevices.erase(entry);
        sendDeviceDisconnected(deviceId);
    }
    else
    {
        LOG_ERROR((iap2, "Device(%u) not found", device->id));
    }

    return true;
}

} } //namespace adit { namespace iap2service {
