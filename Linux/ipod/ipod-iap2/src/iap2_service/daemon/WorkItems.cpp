/************************************************************************
 * @file: WorkItems.cpp
 *
 * @version: 1.0
 *
 * @description: WorkItems to be executed by the worker thread.
 * Implementation for the WorkItem types to send/receive data between
 * Device <-> iAP2Service <-> Applications.
 * This module provides implementation for various types of work items.
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

#include <iap2_service_init.h>
#include <iap2_service_messages.h>
#include <iap2_external_accessory_protocol_session.h>
#include <iap2_utility.h>

#include "Core.h"
#include "WorkItems.h"
#include "FileTransfer.h"

LOG_IMPORT_CONTEXT(disp)

namespace adit { namespace iap2service {

const char* WORKTYPE_STRINGS[] = {
        "DeviceToService",
        "ServiceToApplication",
        "ApplicationToService",
        "ServiceToDevice"
};

const char* WorkItem::getTypeString(WorkType type)
{
    return (type >= WorkType::DeviceToService && type <= WorkType::ServiceToDevice) ? WORKTYPE_STRINGS[type] : nullptr;
}

bool WorkItem::lock(std::atomic<uint32_t>& lock)
{
    uint32_t expect = 0;
    uint32_t desired = 1;
    return lock.compare_exchange_strong(expect, desired);
}

bool WorkItem::unlock(std::atomic<uint32_t>& lock)
{
    uint32_t expect = 1;
    uint32_t desired = 0;

    while(!lock.compare_exchange_strong(expect, desired)) {
        LOG_ERROR((disp, "seems unlock() called before calling lock() or lock is attempted in parallel!"));
    }
    return true;
}

DeviceDataReceiver::DeviceDataReceiver(uint32_t deviceId, int32_t fd, int32_t events, DeviceEvent* context) :
    WorkItem(WorkType::DeviceToService),
    mDeviceId(deviceId),
    mFd(fd),
    mEvents(events),
    mPollContext(context)
{
}

uint32_t DeviceDataReceiver::getDeviceId()
{
    return mDeviceId;
};

WorkItemState DeviceDataReceiver::prepareWork()
{
    mDevice = Core::instance().findDeviceInfo(mDeviceId);
    if(mDevice == nullptr)
    {
        return WorkItemState::Invalid;
    }

    bool allowed = WorkItem::lock(mDevice->lock);
    if(!allowed)
    {
        LOGD_DEBUG((disp, "Device %d(%p) already executing work! attempted work type (DeviceDataReceiver)(0x%p)", mDeviceId, mDevice->device, this));
        return WorkItemState::Retry;
    }
    return WorkItemState::Valid;
}

bool DeviceDataReceiver::cancelWork()
{
    if(mDevice == nullptr)
    {
        return false;
    }

    return WorkItem::unlock(mDevice->lock);
}

void DeviceDataReceiver::execute(WorkComplete* workItemCb)
{
    int32_t rc = iAP2HandleEvent(mDevice->device.get(), mFd, mEvents);
    if(rc < 0)
        LOG_WARN((disp, "iAP2HandleEvent failed FD(%d) with rc = %d", mFd, rc));

    if(rc == IAP2_DEV_NOT_CONNECTED)
    {
        //TODO: Is this really required? Removing from multiple place might create unpredictability.
        //Core::instance().removeDevice(device);
    }
    else
    {
        /* No parallel reads should be allowed for Device Data Read events, because iAP2 Link layer is not
         * thread-safe implementation. Once event is processed Fd is modified to receive event via epoll.
         * If any data has been written in the meantime the epoll will trigger immediately,
         * otherwise will wait for data to arrive.
         * */
        struct epoll_event evt;
        evt.events = mEvents | EPOLLONESHOT | EPOLLET;
        evt.data.u64 = mPollContext->getEventID();
        rc = epoll_ctl(Core::instance().getFd(), EPOLL_CTL_MOD, mFd, &evt);
        if(rc < 0)
            LOG_ERROR((disp, "epoll_ctl failed for Fd(%d) errno(%d)(%s)", mFd, errno, strerror(errno)));
    }

    WorkItem::unlock(mDevice->lock);

    if(workItemCb != nullptr)
        workItemCb->workCompleted();
}

DeviceDataSender::DeviceDataSender(std::unique_ptr<iAP2AccessoryMessage> msg) :
    WorkItem(WorkType::ServiceToDevice),
    mDevice(nullptr),
    mMsg(std::move(msg))
{
}

uint32_t DeviceDataSender::getDeviceId()
{
    uint32_t deviceId = 0;
    if(mMsg != nullptr)
        deviceId = mMsg->header.deviceId;
    return deviceId;
};

WorkItemState DeviceDataSender::prepareWork()
{
    mDevice = Core::instance().findDeviceInfo(mMsg->header.deviceId);
    if(mDevice == nullptr)
    {
        return WorkItemState::Invalid;
    }

    bool allowed = WorkItem::lock(mDevice->lock);
    if(!allowed)
    {
        LOGD_DEBUG((disp, "Device %d(%p) already executing work! attempted work type (DeviceDataSender)(0x%p)", mMsg->header.deviceId, mDevice->device, this));
        return WorkItemState::Retry;
    }
    return WorkItemState::Valid;
}

bool DeviceDataSender::cancelWork()
{
    if(mDevice == nullptr)
    {
        return false;
    }

    return WorkItem::unlock(mDevice->lock);
}

void DeviceDataSender::execute(WorkComplete* workItemCb)
{
    int32_t rc = 0;
    switch(mMsg->sessionType)
    {
    case IAP2SessionType::Control:
    {
        //Parse message for msgId and check whether this message is identified by Apple device
        uint16_t msgId = IAP2_ADHERE_TO_HOST_ENDIANESS_16(*(uint16_t*)(mMsg->buffer + IAP2_MSG_ID_OFFSET));
        auto it = mDevice->identifiedMessageMap.find(msgId);
        if(it == mDevice->identifiedMessageMap.end())
            LOG_WARN((disp, "iAP2Message(0x%x) is not an identified message with Apple Device!! Certification issue!!!", msgId));
    //Continue below to send data to Link
    // "Fall through" used below tells the compiler that fallthrough in this case is explicit and avoids error on gcc7.0 
    }
    //Fall through
    case IAP2SessionType::EAP:
    {
        rc = mDevice->device->iAP2Link->send(mDevice->device.get(), mMsg->buffer, mMsg->size, mMsg->sessionType);
        break;
    }
    case IAP2SessionType::FileTransfer:
    {
        iAP2FTMessage_t *iap2msg = (iAP2FTMessage_t*)mMsg->buffer;
        rc = handleFTClientMsg(mDevice->device.get(),iap2msg);
        break;
    }
    default:
        LOG_ERROR((disp, "Invalid Message type(%d) for DeviceDataSender", mMsg->sessionType));
    }

    if(rc < 0)
        LOG_WARN((disp, "Failed to send application message in iAP2*SendMsgToLink %d", rc));

    if(workItemCb != nullptr)
        workItemCb->workCompleted();

    WorkItem::unlock(mDevice->lock);
}

AppDataReceiver::AppDataReceiver(uint32_t clientId, int32_t fd, int32_t events, ClientEvent* context) :
    WorkItem(WorkType::ApplicationToService),
    mClientId(clientId),
    mFd(fd),
    mEvents(events),
    mPollContext(context)
{
}

void AppDataReceiver::execute(WorkComplete* workItemCb)
{
    /* Message handling done in a following way
     * 1. Service messages(AccConfig & others) are executed in mainloop context
     * 2. Device messages are executed as WorkItem
     */
    uint8_t msg[MAX_MESSAGE_SIZE]; //TODO: get from memory pool (based on number of threads)
    int rc = recv(mFd, msg, sizeof(msg), MSG_DONTWAIT);
    if(rc > 0)
    {
        Core::instance().processClientMessage(mClientId, mFd, msg);
        /* No parallel reads should be allowed for Application Data Read events, because Identification
         * information is send as multiple messages which will be received by different worker threads.
         * Writing to device also done only one message at a time in Link layer.
         * */
        struct epoll_event evt;
        evt.events = mEvents | EPOLLONESHOT;
        evt.data.u64 = mPollContext->getEventID();
        rc = epoll_ctl(Core::instance().getFd(), EPOLL_CTL_MOD, mFd, &evt);
    }
    else
    {
        LOG_WARN((iap2, "Connection with client broken!!errno = %d (%s)", errno, strerror(errno)));
        Core::instance().removeClient(mClientId, mFd, mPollContext->getEventID());
    }

    if(workItemCb != nullptr)
        workItemCb->workCompleted();
}

AppDataSender::AppDataSender(std::unique_ptr<iAP2DeviceMessage> msg, uint16_t msgId, uint16_t sessionId) :
        WorkItem(WorkType::ServiceToApplication),
        mMsg(std::move(msg)),
        mMsgId(msgId),
        mSessionId(sessionId)
{
}

uint32_t AppDataSender::getDeviceId()
{
    uint32_t deviceId = 0;
    if(mMsg != nullptr)
        deviceId = mMsg->header.deviceId;
    return deviceId;
};

WorkItemState AppDataSender::prepareWork()
{
    mDevice = Core::instance().findDeviceInfo(mMsg->header.deviceId);
    if(mDevice == nullptr)
    {
        return WorkItemState::Invalid;
    }

    switch(mMsg->sessionType)
    {
        case IAP2SessionType::Control:
        {
            Core::instance().getMessageRouter().getSubscribedClients(mMsgId, mMsg->header.deviceId, mApplications);
            LOGD_DEBUG((iap2, "iAP2Message(0x%x) subscribed by %zu applications", mMsgId, mApplications.size()));
            break;
        }
        case IAP2SessionType::FileTransfer:
        {
            std::unique_lock<std::mutex> lock(mDevice->clientsMutex);
            //TODO: forward only to the acknowledged client based on mSessionId
            for(auto client : mDevice->clients)
            {
                auto fd = Core::instance().getMessageRouter().getSocketFd(client);
                mApplications.push_back(fd);
            }
            break;
        }
        case IAP2SessionType::EAP:
        {
            Core::instance().getMessageRouter().getClientFdsForiOSApp(mDevice->id, mSessionId, mApplications);
        }
        break;
    }

    if(mApplications.size() > 0)
        LOGD_DEBUG((iap2, "AppDataSender: Device %d connected to (%zu) applications", mDevice->id, mApplications.size()));
    else
        LOG_WARN((iap2, "Device(%d) is not connected by Applications!", mDevice->id));

    return WorkItemState::Valid;
}

bool AppDataSender::cancelWork()
{
   mApplications.clear();
   return true;
}

void AppDataSender::execute(WorkComplete* workItemCb)
{
    Core::instance().getMessageRouter().sendMessage(mApplications, mMsg.get(), mMsg->header.length);

    if(workItemCb != nullptr)
        workItemCb->workCompleted();
}

} } //namespace adit { namespace iap2service {
