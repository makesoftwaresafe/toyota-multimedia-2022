/************************************************************************
 * @file: WorkItems.h
 *
 * @version: 1.0
 *
 * @description: Declaration of WorkItems to be executed by the worker thread.
 * WorkItems are used to send/receive data between Device <-> iAP2Service <-> Applications.
 * This module provides various types of work items to transfer data.
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

#ifndef WORKITEMS_H_
#define WORKITEMS_H_

#include <memory>
#include <iap2_service_init.h>
#include "DeviceConfig.h"

namespace adit { namespace iap2service {

enum WorkType
{
    DeviceToService = 0,
    ServiceToApplication,
    ApplicationToService,
    ServiceToDevice
};

enum class WorkItemState
{
    Retry = 0, // Device is already executing a work. Try again
    Valid,     // Device is not executing any work. Work item can be executed
    Invalid    // Device with specific id not available. Invalid work item received
};

class WorkComplete
{
public:
    virtual ~WorkComplete(){}
    virtual void workCompleted() = 0;
};

class WorkItem
{
public:
    WorkItem(WorkType dir) : mDirection(dir){}
    virtual ~WorkItem() {}
    virtual void execute(WorkComplete* workItemCb) = 0;
    virtual WorkItemState prepareWork(){return WorkItemState::Valid;}
    virtual bool cancelWork(){return true;}
    virtual uint32_t getDeviceId(){return 0;};
    WorkType getType(){return mDirection;}
    static const char* getTypeString(WorkType);
    static bool lock(std::atomic<uint32_t>& lock);
    static bool unlock(std::atomic<uint32_t>& lock);
protected:
    WorkType mDirection;
};

class DeviceDataReceiver : public WorkItem
{
public:
    DeviceDataReceiver(uint32_t deviceId, int32_t fd, int32_t events, DeviceEvent* context);
    ~DeviceDataReceiver(){}
    void execute(WorkComplete* workItemCb) override;
    WorkItemState prepareWork() override;
    bool cancelWork() override;
    uint32_t getDeviceId() override;
private:
    uint32_t mDeviceId;
    std::shared_ptr<DeviceInfo> mDevice;
    int32_t mFd;
    int32_t mEvents; //events to be processed by iAP2HandleEvent
    DeviceEvent* mPollContext;
};

class DeviceDataSender : public WorkItem
{
public:
    DeviceDataSender(std::unique_ptr<iAP2AccessoryMessage> msg);
    ~DeviceDataSender(){}
    void execute(WorkComplete* workItemCb) override;
    WorkItemState prepareWork() override;
    bool cancelWork() override;
    uint32_t getDeviceId() override;
private:
    std::shared_ptr<DeviceInfo>  mDevice;
    std::unique_ptr<iAP2AccessoryMessage> mMsg;
};

class AppDataReceiver : public WorkItem
{
public:
    AppDataReceiver(uint32_t clientId, int32_t fd, int32_t events, ClientEvent* context);
    ~AppDataReceiver(){}
    void execute(WorkComplete* workItemCb) override;

private:
    uint32_t mClientId;
    std::shared_ptr<DeviceInfo> mDevice;
    int32_t mFd;
    int32_t mEvents;
    ClientEvent* mPollContext;
};

class AppDataSender : public WorkItem
{
public:
    AppDataSender(std::unique_ptr<iAP2DeviceMessage> msg, uint16_t msgId, uint16_t sessionId = 0);
    ~AppDataSender(){}
    void execute(WorkComplete* workItemCb) override;
    WorkItemState prepareWork() override;
    uint32_t getDeviceId() override;
    bool cancelWork() override;

private:
    std::shared_ptr<DeviceInfo> mDevice;
    std::unique_ptr<iAP2DeviceMessage> mMsg;
    std::vector<int32_t> mApplications; //Fd list
    uint16_t mMsgId; //Control session message identifier
    uint16_t mSessionId; //EAP & FileTransfer session identifiers
};

} } //namespace adit { namespace iap2service {

#endif /* WORKITEMS_H_ */
