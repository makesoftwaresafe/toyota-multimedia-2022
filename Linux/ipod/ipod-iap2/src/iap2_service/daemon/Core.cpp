/************************************************************************
 * @file: Core.cpp
 *
 * @version: 1.0
 *
 * @description: Core class is the Core of iAP2Service module.
 * This module have 4 major functionalities.
 * Connection manager - creation and removal of client & device connections
 * Message Parser - parsing & creating iAP2Service messages
 * Message Router - Routing the message between client(s) & Device
 * Dispatcher - Executing the actual work in worker thread
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

#include <sys/un.h>
#include <sys/eventfd.h>
#include <systemd/sd-daemon.h>
#include <memory>
#include <sys/stat.h>

#include <iap2_service_init.h>
#include <iap2_init_private.h>
#include <iap2_service_messages.h>

#include "Events.h"
#include "WorkItems.h"
#include "Core.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

#define IAP2_SOCK_NAME "/tmp/iAP2_Server"

int Core::initialize()
{
    mEventID = 0;
    mServerFd = -1;
    int32_t rc = createServer();
    if(rc < IAP2_OK)
        return rc; /****Failed to create listening socket****/

    mShutdown = false;

    std::unique_ptr<ThreadPool> pool(new ThreadPool(NUMBER_OF_WORKER_THREADS));
    mWorkTaskPool = std::move(pool);

    mShutdownFd = eventfd(0, 0);
    addShutdown(mShutdownFd, NULL);

    mThreadFreeFd = eventfd(0, 0);
    addThreadPoolFd(mThreadFreeFd, NULL);
    if(getFd() < 0 || mServerFd < 0 || mShutdownFd < 0 || mThreadFreeFd < 0) {
        LOG_ERROR((iap2, "Core initialize failed!! pollFd:%d, serverFd:%d, shutdownFd:%d ThredPoolFd:%d\n", getFd(), mServerFd, mShutdownFd, mThreadFreeFd));
        deinitialize();
        rc = -1;
    }
    mNextClientId = 0;
    mNextDeviceId = 1;

    return rc;
}

void Core::deinitialize()
{
    if(mShutdownFd >= 0)
    {
        removeShutdown(mShutdownFd);
        close(mShutdownFd);
    }

    if(mThreadFreeFd >= 0)
    {
        removeThreadPoolFd(mThreadFreeFd);
        close(mThreadFreeFd);
    }

    if(mServerFd >= 0)
    {
        destroyServer();
    }

    if(getFd() >= 0)
    {
        close(getFd());
    }

    if(mWorkTaskPool)
    {
        mWorkTaskPool.reset(nullptr);
    }

    Event::mEventMap.clear();

    sd_notify(0, "STATUS=iAP2Service stopped...");
}

void Core::stopRunLoop()
{
    int64_t data = 1;
    ssize_t rc = write(mShutdownFd, &data, sizeof(int64_t));
    if(rc < 0)
    {
        LOG_WARN((iap2, "Write to mShutdownFd failed"));
    }
    mShutdown = true;
}

int Core::sendDeviceStateMessage(iAP2Device_t* iap2Device, enum _iAP2DeviceState dState)
{
    iAP2Device_st* device = (iAP2Device_st*)iap2Device;
    struct DeviceState msg;
    memset(&msg, 0, sizeof(msg));
    msg.header.deviceId = device->iAP2DeviceId;
    msg.header.type = MessageType::DeviceState;
    memcpy(msg.state.serial, device->iAP2Transport.iAP2DeviceIdentifier, strnlen((char*)device->iAP2Transport.iAP2DeviceIdentifier, STRING_MAX));
    msg.state.state = dState;

    return mRouter.broadcastMessage(&msg, sizeof(msg));
}

int Core::sendConnectedDeviceList(iAP2Device_t* thisDevice, int32_t clientId)
{
    return mRouter.sendConnectedDeviceList(thisDevice, clientId);
}

//iAP2 core layer provides this FD set
int Core::addDevice(std::shared_ptr<iAP2Device_st> device, iAP2GetPollFDs_t* fdSet, const char* deviceName)
{
    std::shared_ptr<DeviceInfo> dev(new DeviceInfo());
    dev->device = device;
    dev->id = mNextDeviceId++;
    strncpy(dev->name, deviceName, sizeof(dev->name));
    device->iAP2DeviceId = dev->id;

    /* First device is added to the table and then pollfds are fetched.
    Earlier there were chances for the fds to be triggered even before the device is added to the table. So the work may get rejected as invalid. */
    mRouter.addDevice(dev);


    //add for epoll watch list for device event handling
    LOG_INFO((iap2, "New device connected! device(%p) Id(%d) FDs(%u)", device, dev->id, fdSet->numberFDs));
    for(int i = 0; i < fdSet->numberFDs; i++)
    {
        uint64_t evtID = mEventID++;
        std::shared_ptr<Event> evt(new DeviceEvent(fdSet->fds[i].fd, evtID, device));
        bool ret = Event::addEvent(evtID, evt); // this is moved ahead to make sure that event is available in the map before any event is triggered.
        if(ret == false)
        {
            LOG_ERROR((iap2, "Event could not be pushed into the map. Overflow occured while adding device event!!"));
        }

        /* No parallel reads should be allowed for Device Data Read events.
         * Because the Link layer is not thread-safe implementation.
         * */
        addFd(fdSet->fds[i].fd, (uint32_t)fdSet->fds[i].event | EPOLLONESHOT | EPOLLET, evtID);
        dev->events.push_back(evt);
    }

    //create message map for checking whether it is an identified message by Apple device
    for(size_t i = 0; i < device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication_length/sizeof(uint16_t); ++i)
    {
        dev->identifiedMessageMap.insert(std::pair<uint16_t, bool>(device->iAP2AccessoryInfo.iAP2CommandsUsedByApplication[i], true));
    }

    for(size_t i = 0; i < device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice_length/sizeof(uint16_t); ++i)
    {
        dev->identifiedMessageMap.insert(std::pair<uint16_t, bool>(device->iAP2AccessoryInfo.iAP2CallbacksExpectedFromDevice[i], true));
    }

    return 0;
}

int Core::addClient(int sockFd)
{
    uint64_t evtID = mEventID++;
    uint32_t clientID = ++mNextClientId;
    //Add new client event to main loop
    std::shared_ptr<ClientEvent> evt(new ClientEvent(sockFd, clientID, evtID));
    LOG_INFO((iap2, "New Client connected! Id(%u) sockFd(%d) eventID(%" PRIu64 ")", clientID, sockFd, evtID));
    mRouter.addClient(clientID, sockFd, evt);

    int32_t rc = addFd(sockFd, EPOLLIN | EPOLLONESHOT, evtID);
    bool ret = Event::addEvent(evtID, evt);
    if(ret == false)
    {
        LOG_ERROR((iap2, "Event could not be pushed into the map. Overflow occured while adding client event!!"));
    }

    //Notify application on already connected devices
    sendConnectedDeviceList(nullptr, clientID);

    return rc;
}

int Core::addServer()
{
    uint64_t evtID = mEventID++;
    std::shared_ptr<ServerEvent> serverEvent(new ServerEvent(mServerFd, evtID));
    bool ret = Event::addEvent(evtID, serverEvent);
    if(ret == false)
    {
        LOG_ERROR((iap2, "Event could not be pushed into the map. Overflow occured while adding server event!!"));
    }
    return addFd(mServerFd, EPOLLIN, evtID);
}

int Core::addShutdown(int sockFd, void* context)
{
    (void)context;
    uint64_t evtID = mEventID++;
    std::shared_ptr<ShutdownEvent> shutdownEvent(new ShutdownEvent(sockFd, evtID));
    bool ret = Event::addEvent(evtID, shutdownEvent);
    if(ret == false)
    {
        LOG_ERROR((iap2, "Event could not be pushed into the map. Overflow occured while adding shutdown event!!"));
    }
    return addFd(sockFd, EPOLLIN, evtID);
}

int Core::addThreadPoolFd(int threadFreeFd, void* context)
{
    (void)context;
    uint64_t evtID = mEventID++;
    std::shared_ptr<ThreadFreeEvent> threadFreeEvent(new ThreadFreeEvent(threadFreeFd, evtID));
    bool ret = Event::addEvent(evtID, threadFreeEvent);
    if(ret == false)
    {
        LOG_ERROR((iap2, "Event could not be pushed into the map. Overflow occured while adding threadpool event!!"));
    }
    return addFd(threadFreeFd, EPOLLIN, evtID);
}

void Core::removeDevice(std::shared_ptr<DeviceInfo> deviceInfo)
{
    while(!deviceInfo->events.empty())
    {
        /* Event pointer is referenced in epoll_event->data->ptr */
        int fd = deviceInfo->events.front()->getFd();
        removeFd(fd);
        Event::removeEvent(deviceInfo->events.front()->getEventID());
        deviceInfo->events.pop_front();
        LOG_INFO((iap2, "Device fd:%d removed", fd));
    }

    LOG_INFO((iap2, "Device removed: device(%p) Id(%u)", deviceInfo->device.get(), deviceInfo->id));
    mRouter.removeDevice(deviceInfo);
}

void Core::removeClient(int clientId, int sockFd, uint64_t eventID)
{
    removeFd(sockFd);
    Event::removeEvent(eventID);
    LOG_INFO((iap2, "Client removed: client(%d) Id(%d)", clientId, sockFd));
    mRouter.removeClient(clientId);
}

void Core::removeServer()
{
    removeFd(mServerFd);
    /*
     * Note: mEventMap for ServerEvent is cleared during deinitialize()
     */
}

void Core::removeShutdown(int sockFd)
{
    removeFd(sockFd);
    /*
     * Note: mEventMap for ShutDownEvent is cleared during deinitialize()
     */
}

void Core::removeThreadPoolFd(int eventFd)
{
    removeFd(eventFd);
    /*
     * Note: mEventMap for ThreadFreeEvent is cleared during deinitialize()
     */
}

int Core::createServer()
{
    int rc = IAP2_OK;

    if (sd_listen_fds(0) == 1) //socket based activation
    {
        mServerFd = SD_LISTEN_FDS_START + 0;
        sd_notify(0, "READY=1\nSTATUS=iAP2 Service initialized");
        LOG_WARN((iap2, "sd_listen_fds() succeeded! ServerFd:%d", mServerFd));
        rc = addServer();
    }
    else //TODO: Remove this section when socket based activation validated!
    {
        LOG_INFO((iap2, "sd_listen_fds() failed! Fallback to socket creation by iAP2Service"));
        rc = IAP2_CTL_ERROR;

        unlink(IAP2_SOCK_NAME);

        mServerFd = socket(PF_UNIX, SOCK_SEQPACKET, 0);
        if(mServerFd >= 0)
        {
            struct sockaddr_un address;
            memset(&address, 0, sizeof(struct sockaddr_un));
            address.sun_family = AF_UNIX;
            strncpy(address.sun_path, IAP2_SOCK_NAME, sizeof(address.sun_path));
            address.sun_path[sizeof(address.sun_path)-1] = '\0';

            rc = bind(mServerFd, (struct sockaddr *) &address, sizeof(struct sockaddr_un));
            if(rc != 0)
            {
                LOG_ERROR((iap2, "bind() failed with errno:%d (%s)\n", errno, strerror(errno)));
                rc = IAP2_CTL_ERROR;
            }
            else
            {
                chmod( IAP2_SOCK_NAME,
                       (S_IRUSR | S_IWUSR | S_IXUSR |    /* rwx */
                        S_IRGRP | S_IXGRP |              /* rwx */
                        S_IROTH | S_IWOTH | S_IXOTH) );  /* rwx */

                rc = listen(mServerFd, SOMAXCONN);
                if(rc < 0)
                    LOG_ERROR((iap2, "listen() failed errno:%d (%s)\n", errno, strerror(errno)));

                rc = addServer();

                sd_notify(0, "READY=1\nSTATUS=iAP2 Service initialized");
                LOG_INFO((iap2, "bind() with listening socket successful"));
            }
        }
    }

    return rc;
}

void Core::destroyServer()
{
    removeServer();
    LOG_INFO((iap2, "Destroying iAP2Service"));
}

std::shared_ptr<DeviceInfo> Core::findDeviceInfo(uint32_t deviceId)
{
    return mRouter.findDeviceInfo(deviceId);
}

std::shared_ptr<DeviceInfo> Core::findDeviceInfo(const char* serial)
{
    return mRouter.findDeviceInfo(serial);
}

int Core::setEAPSessionId(iAP2Device_t* iap2Device, uint8_t iOsAppIdentifier, uint16_t sessionIdentifier)
{
    iAP2Device_st* device = (iAP2Device_st*)iap2Device;
    return mRouter.setEAPSessionIdentifer(device->iAP2DeviceId, iOsAppIdentifier, sessionIdentifier);
}

int Core::resetEAPSessionId(iAP2Device_t* iap2Device, uint16_t sessionIdentifier)
{
    iAP2Device_st* device = (iAP2Device_st*)iap2Device;
    return mRouter.resetEAPSessionIdentifer(device->iAP2DeviceId, sessionIdentifier);
}

int Core::sendControlSessionMessageToApplication(iAP2Device_t* iap2Device, uint16_t msgId, uint8_t* data, uint16_t length, void* context)
{
	LOGD_VERBOSE((iap2, "sendControlSessionMessageToApplication params iap2Device:%p msgId:0x%4x data:%p length=%u context=%p", iap2Device, msgId, data, length, context));
    iAP2Device_st* thisDevice = (iAP2Device_st*)iap2Device;

    try
    {
        std::unique_ptr<iAP2DeviceMessage> msg(new iAP2DeviceMessage());
        msg->header.type = MessageType::iAP2DeviceMsg;
        msg->sessionType = IAP2SessionType::Control;
        msg->header.deviceId = thisDevice->iAP2DeviceId;
        msg->header.length = sizeof(MessageHeader) + sizeof(IAP2SessionType) + sizeof(uint32_t) + length;
        msg->size = length;
        memcpy(msg->buffer, data, msg->header.length);

        std::unique_ptr<AppDataSender> sender(new AppDataSender(std::move(msg), msgId));

        /* Execute the AppDataSender work in the same thread */
        WorkItemState state = sender->prepareWork();
        if(state == WorkItemState::Valid)
        {
            sender->execute(nullptr);
        }
        else
        {
            LOG_ERROR((iap2, "Error in AppDataSender->prepareWork(), called in sendControlSessionMessageToApplication"));
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR((iap2, "Exception in sendControlSessionMessageToApplication: (%s)", e.what()));
    }

    return 0;
}

int Core::sendEAPSessionMessageToApplication(iAP2Device_t* iap2Device, uint16_t sessionId, uint8_t* data, uint16_t length)
{
    int rc = IAP2_OK;
    LOGD_VERBOSE((iap2, "sendEAPSessionMessageToApplication iap2Device:%p sessionId:%u data:%p length=%u", iap2Device, sessionId, data, length));
    iAP2Device_st* thisDevice = (iAP2Device_st*)iap2Device;

    try
    {
        std::unique_ptr<iAP2DeviceMessage> msg(new iAP2DeviceMessage());
        msg->header.type = MessageType::iAP2DeviceMsg;
        msg->sessionType = IAP2SessionType::EAP;
        msg->header.deviceId = thisDevice->iAP2DeviceId;
        msg->header.length = sizeof(MessageHeader) + sizeof(IAP2SessionType) + sizeof(uint32_t) + length;
        msg->size = length;
        memcpy(msg->buffer, data, length);

        std::unique_ptr<AppDataSender> sender(new AppDataSender(std::move(msg), 0, sessionId));

        /* Execute the AppDataSender work in the same thread */
        WorkItemState state = sender->prepareWork();
        if(state == WorkItemState::Valid)
        {
            sender->execute(nullptr);
        }
        else
        {
            LOG_ERROR((iap2, "Error in AppDataSender->prepareWork(), called in sendEAPSessionMessageToApplication"));
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR((iap2, "Exception in sendEAPSessionMessageToApplication: (%s)", e.what()));
        rc = IAP2_ERR_NO_MEM;
    }

    return rc;
}

int Core::sendFileTransferSessionMessageToApplication(iAP2Device_t* iap2Device, uint16_t fileTransferId, uint8_t* data, uint32_t length)
{
    int rc = IAP2_OK;
    LOGD_VERBOSE((iap2, "sendFileTransferSessionMessageToApplication iap2Device:%p transferId:%u data:%p length=%u", iap2Device, fileTransferId, data, length));

    try
    {
        std::unique_ptr<iAP2DeviceMessage> msg(new iAP2DeviceMessage());
        msg->header.type = MessageType::iAP2DeviceMsg;
        msg->sessionType = IAP2SessionType::FileTransfer;
        msg->header.deviceId = ((iAP2Device_st*)iap2Device)->iAP2DeviceId;
        msg->header.length = sizeof(MessageHeader) + sizeof(IAP2SessionType) + sizeof(uint32_t) + length;
        msg->size = length;
        memcpy(msg->buffer, data, length);

        std::unique_ptr<AppDataSender> sender(new AppDataSender(std::move(msg), 0, fileTransferId));

        /* Execute the AppDataSender work in the same thread */
        WorkItemState state = sender->prepareWork();
        if(state == WorkItemState::Valid)
        {
            sender->execute(nullptr);
        }
        else
        {
            LOG_ERROR((iap2, "Error in AppDataSender->prepareWork(), called in sendFileTransferSessionMessageToApplication"));
        }
    }
    catch(const std::exception& e)
    {
        LOG_ERROR((iap2, "Exception in sendFileTransferSessionMessageToApplication: (%s)", e.what()));
        rc = IAP2_ERR_NO_MEM;
    }

    return rc;
}

bool Core::processAccessoryConfigMessage(int32_t clientId, int32_t fd, uint8_t* msg)
{
    (void)clientId;
    (void)fd;
    bool isAccMsg = true;
    MessageHeader* header = (MessageHeader*)msg;
    switch(header->type)
    {
        case MessageType::AccessoryConfiguration:
        {
            mIAP2Device.setAccessoryConfiguration((struct AccessoryConfiguration*)msg);
            break;
        }
        case MessageType::AccessoryIdentficiation:
        {
            mIAP2Device.setAccessoryIdentification((struct AccessoryIdentficiation*)msg);
            break;
        }
        case MessageType::AccessorySupportedLanguages:
        {
            mIAP2Device.setSupportedLanguages((struct AccessorySupportedLanguages*)msg);
            break;
        }
        case MessageType::AccessorySupportediOSApps:
        {
            mIAP2Device.setSupportediOSAppInfo((struct AccessorySupportediOSApps*)msg);
            break;
        }
        case MessageType::MessagsSentByApplication:
        {
            mIAP2Device.setMessageSentByApplication((struct MessagsSentByApplication*)msg);
            break;
        }
        case MessageType::CallbacksExpectedFromDevice:
        {
            mIAP2Device.setCallbacksExpectedFromDevice((struct CallbacksExpectedFromDevice*)msg);
            break;
        }
        case MessageType::USBDeviceAudioSampleRates:
        {
            mIAP2Device.setSupportedAudioRates((struct USBDeviceAudioSampleRates*)msg);
            break;
        }
        case MessageType::VehicleInformation:
        {
            mIAP2Device.setVehicleInformation((struct VehicleInformation*)msg);
            break;
        }
        case MessageType::VehicleStatus:
        {
            mIAP2Device.setVehicleStatus((struct VehicleStatus*)msg);
            break;
        }
        case MessageType::LocationInformation:
        {
            mIAP2Device.setLocationInformation((struct LocationInformation*)msg);
            break;
        }
        case MessageType::USBDeviceHID:
        {
            mIAP2Device.setUSBHIDInformation((struct USBDeviceHID*)msg);
            break;
        }
        case MessageType::USBHostHID:
        {
            mIAP2Device.setUSBHostHIDInformation((struct USBHostHID*)msg);
            break;
        }
        case MessageType::BluetoothHID:
        {
            mIAP2Device.setBluetoothHIDInformation((struct BluetoothHID*)msg);
            break;
        }
        case MessageType::BluetoothTransport:
        {
            mIAP2Device.setBluetoothTransport((struct BluetoothTransport*)msg);
            break;
        }
        case MessageType::WirelessCarPlayTransport:
        {
            mIAP2Device.setWirelessCarPlayTransport((struct WirelessCarPlayTransport*)msg);
            break;
        }
        case MessageType::RouteGuidanceDisplay:
        {
            mIAP2Device.setRouteGuidanceDisplay((struct RouteGuidanceDisplay*)msg);
            break;
        }
        case MessageType::IdentificationInfoComplete:
        {
            std::shared_ptr<iAP2Device_st> device = mIAP2Device.initializeDevice();
            if(device != nullptr)
            {
                iAP2GetPollFDs_t getPollFDs;
                iAP2GetPollFDs(device.get(), &getPollFDs);
                addDevice(device, &getPollFDs, (const char*)"Apple Device");
                sendConnectedDeviceList(device.get());
            }
            else
            {
                LOG_ERROR((iap2, "CIAP2Device::initializeDevice Failed!!"));
            }
            break;
        }
        default:
            isAccMsg = false;
            break;
    }
    return isAccMsg;
}

int Core::processConnectDeviceMessage(int32_t clientId, int32_t fd, struct ConnectDevice* msg)
{
    uint32_t length = 0;
    struct iOsAppInfo* iOsAppsSupported = ((struct iOsAppInfo*)(msg->payload + length));
    mRouter.addiOSAppIdentifiers(msg->header.deviceId, clientId, iOsAppsSupported);
    length += sizeof(iOsAppInfo) + iOsAppsSupported->count * sizeof(uint8_t);

    iAP2ServiceMessages_t* commands = (iAP2ServiceMessages_t*)(msg->payload + length);
    mRouter.subscribeiAP2Messages(fd, msg->header.deviceId, commands);
    length += sizeof(iAP2ServiceMessages_t) + commands->length;

    iAP2ServiceMessages_t* callbacks = (iAP2ServiceMessages_t*)(msg->payload + length);
    mRouter.subscribeiAP2Messages(fd, msg->header.deviceId, callbacks);

    return 0;
}

bool Core::processServiceMessage(int32_t clientId, int32_t fd, uint8_t* msg)
{
    bool processed = true;
    MessageHeader* header = (MessageHeader*)msg;
    switch(header->type)
    {
    case MessageType::ClientInformation:
    {
        int clientId = mRouter.getClientId(fd);
        struct ClientInformation* clientInfo = (struct ClientInformation*)msg;
        mRouter.updateClient(clientId, *clientInfo);
        break;
    }
    case MessageType::ConnectDevice:
    {
        LOG_INFO((iap2, "ConnectDevice requested! Client(%u) for deviceId(%d)", clientId, header->deviceId));
        struct ConnectDevice* device_st = (struct ConnectDevice*)msg;
        //Add clientId into the connected clients list in deviceInfo
        ClientSubscription isConnected = mRouter.subscribeClientForDevice(clientId, fd, device_st->header.deviceId);

        //Send reply message to ConnectDevice request
        struct ConnectDeviceResp resp;
        resp.header.deviceId = device_st->header.deviceId;
        resp.header.type = MessageType::ConnectDeviceResp;

        if(isConnected == ClientSubscription::Success)
        {
            resp.result = IAP2_OK;
            processConnectDeviceMessage(clientId, fd, device_st);

            int clientId = mRouter.getClientId(fd);
            mRouter.sendMessage(clientId, &resp, sizeof(resp));
            LOG_INFO((iap2, "ConnectDevice request Finished! ClientId(%u) deviceId(%d)", clientId, device_st->header.deviceId));
        }
        else
        {
            resp.result = IAP2_SERVICE_CONNECT_DEVICE_FAIL;
            mRouter.sendMessage(clientId, &resp, sizeof(resp));
        }

        break;
    }
    case MessageType::DisconnectDevice:
    {
        LOG_INFO((iap2, "DisconnectDevice requested! Client(%u) for deviceId(%d)", clientId, header->deviceId));
        struct DisconnectDevice* device_st = (struct DisconnectDevice*)msg;
        //remove clientId from connected clients list in deviceInfo
        mRouter.removeClientForiOSAppIdsForDevice(clientId, device_st->header.deviceId);
        mRouter.unsubscribeClientForDevice(fd, device_st->header.deviceId);
        mRouter.removeEAPSessionIdentifierForDevice(mRouter.getSocketFd(clientId), device_st->header.deviceId);
        break;
    }
    case MessageType::DeviceDiscovered:
    {
        //MessageType::DeviceDiscovered now assumed as MessageType::AccessoryConfiguration message.
        break;
    }
    case MessageType::DeviceDisappeared:
    {
        struct DeviceDisappeared* message = (struct DeviceDisappeared*)msg;
        std::shared_ptr<DeviceInfo> deviceInfo = findDeviceInfo(message->serial);
        if(deviceInfo != nullptr)
        {
            removePendingWork(deviceInfo->id);
            removeDevice(deviceInfo);
        }
        else
        {
            LOG_ERROR((iap2, "Device with serial(%s) not found for Disconnection", message->serial));
        }
        break;
    }
    default:
        processed = processAccessoryConfigMessage(clientId, fd, msg);
    }

    return processed;
}

bool Core::processiAP2AccessoryMessage(int32_t clientId, int32_t fd, uint8_t* msg)
{
    bool processed = false;
    MessageHeader* header = (MessageHeader*)msg;
    if(header->type == MessageType::iAP2AccMsg)
    {
        iAP2AccessoryMessage* iap2Msg = (iAP2AccessoryMessage*)msg;

        //TODO: optimise memory usage
        try {
            iAP2AccessoryMessage* newiAP2Msg = new iAP2AccessoryMessage();
            memcpy(newiAP2Msg, iap2Msg, sizeof(MessageHeader)+sizeof(IAP2SessionType)+sizeof(uint32_t)+iap2Msg->size);

            std::unique_ptr<iAP2AccessoryMessage> accMsg(newiAP2Msg);

            //Create work Item
            std::shared_ptr<DeviceDataSender> msgSender(new DeviceDataSender(std::move(accMsg)));
            pushWorkItem(msgSender);
        } catch (const std::exception& e) {
            LOG_ERROR((iap2, "Exception in sending iAP2 message to Device: (%s)", e.what()));
        }

        processed = true;
    }
    else
    {
        processed = processServiceMessage(clientId, fd, msg);
    }
    return processed;
}

int Core::processClientMessage(int32_t clientId, int32_t fd, uint8_t* msg)
{
    MessageHeader* header = (MessageHeader*)msg;
    char msgTypeString[STRING_MAX]= {0};
    iAP2ServiceGetMessageTypeString(header->type, msgTypeString, STRING_MAX);
    LOGD_DEBUG((iap2, "MessageType:(%s)(%u) deviceId:%u length:%d", msgTypeString, (uint32_t)header->type, header->deviceId, header->length));

    bool processed = processiAP2AccessoryMessage(clientId, fd, msg);
    if(!processed)
        LOG_WARN((iap2, "Unhandled message (%s)(%d) received!", msgTypeString, header->type));

    return 0;
}

void Core::freeThreadAvailable()
{
	int64_t data = 1;
	ssize_t rc = write(mThreadFreeFd, &data, sizeof(int64_t));
	if(rc < 0)
	{
	    LOG_WARN((iap2, "Write to mThreadFreeFd failed"));
	}
}

void Core::removePendingWork(uint32_t deviceId)
{
    std::deque<std::shared_ptr<WorkItem>> pendingQueue;
    std::unique_lock<std::mutex> lock(mWorkLock);
    (void)deviceId;

    while(!mWorkQueue.empty())
    {
        std::shared_ptr<WorkItem> work = mWorkQueue.front();
        mWorkQueue.pop_front();
        uint32_t devId = work->getDeviceId();
        if(devId == deviceId)
        {
            LOG_WARN((iap2, "Removed the workitem(%p) of type(%s) as Device is disconnected!", work.get(), WorkItem::getTypeString(work->getType())));
            work.reset();
        }
        else
        {
            pendingQueue.push_back(work);
        }
    }
    mWorkQueue = std::move(pendingQueue);
}

void Core::pushWorkItem(std::shared_ptr<WorkItem> work)
{
    std::unique_lock<std::mutex> lock(mWorkLock);
    LOGD_DEBUG((iap2, "WorkerType(%s)(%p) pushed to worker queue", WorkItem::getTypeString(work->getType()), work.get()));
    mWorkQueue.push_back(work);
}

void Core::dispatchWorkItem()
{
    std::deque<std::shared_ptr<WorkItem>> pendingQueue;
    std::unique_lock<std::mutex> lock(mWorkLock);
    while(!mWorkQueue.empty())
    {
        std::shared_ptr<WorkItem> work = mWorkQueue.front();
        mWorkQueue.pop_front();
        LOGD_DEBUG((iap2, "WorkerType(%s)(%p) popped from worker queue", WorkItem::getTypeString(work->getType()), work.get()));
        WorkItemState state = work->prepareWork();
        if(state == WorkItemState::Valid)
        {
            int32_t workId = mWorkTaskPool->startWork(work);
            if(workId == 0)
            {
                LOG_INFO((iap2, "ThreadPool is empty!"));
                work->cancelWork();
                pendingQueue.push_back(work);
                break; /***************No Free threads available*********************/
            }
        }
        else if(state == WorkItemState::Retry)
        {
            /* Work items to the iAP2 device should be serialized to avoid data collision.
             * If 2 work items try to write to the same device parallel, then the second one will
             * be put on hold until first one is finished
             * */
            pendingQueue.push_back(work);
        }
        else
        {
            LOG_INFO((iap2, "Invalid work item received. Device not available!!"));
        }
    }

    /* Add pending items back to the WorkQueue for retry.
     * Items are added to the front so that the work item \
     * order remains the same.
     */
    while(!pendingQueue.empty())
    {
        auto work = pendingQueue.back();
        pendingQueue.pop_back();
        mWorkQueue.push_front(work);
    }
}

int Core::runLoop()
{
    struct epoll_event *events;
    events = (epoll_event*)calloc(MAX_EVENTS, sizeof(struct epoll_event));
    if (!events)
    {
        LOG_ERROR((iap2, "No memory available for events errno = %d (%s)", errno, strerror(errno)));
        return -1;
    }

    LOGD_DEBUG((iap2, "mainloop started"));

    while(!mShutdown)
    {
        int n = epoll_wait(getFd(), events, MAX_EVENTS, -1);
        for(int i = 0; (i < n) && (!mShutdown); i++)
        {
            /* Validate whether if the event triggered is valid or not */
            Event::mEventMapLock.lock();

            auto it = Event::mEventMap.find(events[i].data.u64);
            if(it != Event::mEventMap.end())
            {
                auto shared_eventptr = it->second;
                Event::mEventMapLock.unlock();
                Event* evt = (Event*)shared_eventptr.get();
                if(evt != nullptr)
                {
                    evt->process(events[i].events);
                }
                else
                {
                    LOG_ERROR((iap2, "epoll_wait event ptr is null!!"));
                }
            }
            else
            {
                Event::mEventMapLock.unlock();
                LOG_WARN((iap2, "Invalid event triggered. This event does not exist in event map!!"));
            }
        }
        dispatchWorkItem();
    }
    LOG_INFO((iap2, "returned from mainloop"));
    free(events);
    return 0;
}

} } //namespace adit { namespace iap2service {
