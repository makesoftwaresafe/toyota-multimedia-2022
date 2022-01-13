/************************************************************************
 * @file: MessageRouter.h
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

#ifndef MESSAGE_ROUTER_H_
#define MESSAGE_ROUTER_H_

#include <string>
#include <memory>
#include <map>
#include <vector>
#include <algorithm>
#include <stdint.h>
#include <pthread.h>
#include <iap2_service_messages.h>
#include <Events.h>
#include "DeviceConfig.h"

namespace adit { namespace iap2service {

#define INVALID_DEVICE          -1
#define IAP2_SERVICE_CONNECT_DEVICE_FAIL -2

#define iap2_service_return_on_invalid_device(device, deviceId) do { if(device == nullptr) { \
        LOG_WARN((iap2, "Device not found! deviceId(%d) in %s", deviceId, __FUNCTION__)); return; } } while(0);

#define iap2_service_return_value_on_invalid_device(device, deviceId, retVal) do { if(device == nullptr) { \
        LOG_WARN((iap2, "Device not found! deviceId(%d) in %s", deviceId, __FUNCTION__)); return retVal; } } while(0);

enum class ClientSubscription
{
    Success = 0,               // Client connection to the device is success
    DeviceNotConnected,        // Device is not connected to iAP2Service
    ClientAlreadyConnected     // Client is already connected to the given device
};

class MessageRouter
{
public:
    bool addClient(int32_t clientId, int sockFd, std::shared_ptr<ClientEvent> event);
    bool removeClient(int clientId);
    bool updateClient(int clientID, struct ClientInformation& info);

    int32_t getClientId(int sockFd);
    int32_t getSocketFd(int clientId);

    void subscribeiAP2Messages(int32_t clientFd, uint32_t deviceId, iAP2ServiceMessages_t* msg);
    void unsubscribeClient(int32_t clientFd);

    ClientSubscription subscribeClientForDevice(int32_t clientId, int32_t clientFd, uint32_t deviceId);
    void unsubscribeClientForDevice(int32_t clientFd, uint32_t deviceId);

    void getSubscribedClients(uint16_t msgId, uint32_t deviceId, std::vector<int32_t>& clients);

    void addiOSAppIdentifiers(uint32_t deviceId, uint32_t clientId, iOsAppInfo* apps);
    void removeClientForiOSAppIds(uint32_t clientId);
    void removeClientForiOSAppIdsForDevice(uint32_t clientId, uint32_t deviceId);

    void getClientFdsForiOSApp(uint32_t deviceId, int16_t eapSessionId, std::vector<int32_t>& clients);
    void getClientIdsForEANativeApp(uint32_t deviceId, uint8_t eapIdentifier, std::vector<uint32_t>& clients);

    int32_t setEAPSessionIdentifer(uint32_t deviceId, uint8_t eapId, uint16_t eapSessionId);
    int32_t resetEAPSessionIdentifer(uint32_t deviceId, uint16_t eapSessionId);
    void removeEAPSessionIdentifier(int32_t clientFd);
    void removeEAPSessionIdentifierForDevice(int32_t clientFd, uint32_t deviceId);
    int32_t sendMessage(int clientId, void* buffer, uint32_t length);
    int32_t sendMessage(std::vector<int32_t>& clientList, void* buffer, uint32_t length);
    int32_t broadcastMessage(void* buffer, uint32_t length);

public:
    std::shared_ptr<DeviceInfo> findDeviceInfo(uint32_t deviceId);
    std::shared_ptr<DeviceInfo> findDeviceInfo(const char* serial);
    int sendConnectedDeviceList(iAP2Device_t* thisDevice, int32_t clientId = -1);
    int sendDeviceDisconnected(uint32_t id);

    bool addDevice(std::shared_ptr<DeviceInfo> device);
    bool removeDevice(std::shared_ptr<DeviceInfo> device);

private:
    struct ClientInfo
    {
        uint32_t Id;
        uint32_t pid;
        uint32_t features;
        std::string name;
        int32_t sockFd;
        std::shared_ptr<ClientEvent> event;
    };

    std::map<int16_t, ClientInfo> mClients;
    std::mutex mClientsMutex;

    std::map<uint32_t, std::shared_ptr<DeviceInfo>> mDevices;
    std::mutex mDevicesMutex;
};

} } //namespace adit { namespace iap2service {

#endif /* MESSAGE_ROUTER_H_ */
