/************************************************************************
 * @file: Core.h
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

#ifndef IAP2_SERVICE_CORE_H_
#define IAP2_SERVICE_CORE_H_

#include <memory>
#include <list>
#include <map>
#include <adit_logging.h>
#include <inttypes.h>

#include "EpollUtility.h"

#include "Singleton.h"
#include "MessageRouter.h"
#include "ThreadPool.h"
#include "DeviceConfig.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

#define MAX_EVENTS      4
#define MAX_TIMEOUT   1000

/* Fixing the number of threads to 4, since it is tested
 * working in the current environment
 */
#define NUMBER_OF_WORKER_THREADS 4

enum FileDescriptorType
{
    Monitor,
    Client,
    Server,
    Device,
    Shutdown,
    ThreadFree
};

typedef void (*thread_handler)(void*);

class Core : public Singleton<Core>, public CEpoll
{
public:
    int initialize();
    void deinitialize();

    int addDevice(std::shared_ptr<iAP2Device_st> device, iAP2GetPollFDs_t* fdSet, const char* deviceName);
    int addClient(int sockFd);

    void removeDevice(std::shared_ptr<DeviceInfo> deviceInfo);
    void removeClient(int clientId, int sockFd, uint64_t eventID);

    int runLoop();
    void stopRunLoop();

    int sendControlSessionMessageToApplication(iAP2Device_t* iap2Device, uint16_t msgId, uint8_t* data, uint16_t length, void* context);
    int sendEAPSessionMessageToApplication(iAP2Device_t* iap2Device, uint16_t sessionId, uint8_t* data, uint16_t length);
    int sendFileTransferSessionMessageToApplication(iAP2Device_t* iap2Device, uint16_t fileTransferId, uint8_t* data, uint32_t length);

    int sendConnectedDeviceList(iAP2Device_t* device, int32_t clientId = -1);
    int sendDeviceStateMessage(iAP2Device_t* iap2Device, enum _iAP2DeviceState dState);

    int setEAPSessionId(iAP2Device_t* iap2Device, uint8_t iOsAppIdentifier, uint16_t sessionIdentifier);
    int resetEAPSessionId(iAP2Device_t* iap2Device, uint16_t sessionIdentifier);

    std::shared_ptr<DeviceInfo> findDeviceInfo(uint32_t deviceId);
    std::shared_ptr<DeviceInfo> findDeviceInfo(const char* serial);

    MessageRouter& getMessageRouter(){return mRouter;}
    friend class DeviceDataReceiver;
    friend class AppDataReceiver;

public:
    int  processClientMessage(int32_t clientId, int32_t fd, uint8_t* msg);
    void pushWorkItem(std::shared_ptr<WorkItem> work);
    void freeThreadAvailable();
    void removePendingWork(uint32_t deviceId);

private:
    bool processiAP2AccessoryMessage(int32_t clientId, int32_t fd, uint8_t* msg);
    bool processServiceMessage(int32_t clientId, int32_t fd, uint8_t* msg);
    bool processAccessoryConfigMessage(int32_t clientId, int32_t fd, uint8_t* msg);
    int processConnectDeviceMessage(int32_t clientId, int32_t fd, struct ConnectDevice* msg);

private:
    void dispatchWorkItem();

    int createServer();
    void destroyServer();

    int addServer();
    void removeServer();

    int addShutdown(int sockFd, void* context);
    void removeShutdown(int sockFd);

    int addThreadPoolFd(int eventFd, void* context);
    void removeThreadPoolFd(int eventFd);

private:
    int mShutdownFd;
    int mServerFd;
    int mThreadFreeFd;
    bool mShutdown;

    DeviceConfig mIAP2Device;
    MessageRouter mRouter;

    std::unique_ptr<ThreadPool> mWorkTaskPool;
    std::mutex mWorkLock;
    std::deque<std::shared_ptr<WorkItem>> mWorkQueue;
    std::atomic<uint32_t> mNextDeviceId;
    std::atomic<uint32_t> mNextClientId;

    std::atomic<uint64_t> mEventID;
};

} } //namespace adit { namespace iap2service {

#endif /* IAP2_SERVICE_CORE_H_ */
