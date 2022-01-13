/************************************************************************
 * @file: Events.cpp
 *
 * @version: 1.0
 *
 * @description: Events module implements context for each epoll event.
 * Entry functions for all events received by epoll_wait().
 * Event class further creates the WorkItems for processing of
 * occurred events.
 *
 * @component: platform/ipod
 *
 * @author: Dhanasekaran Devarasu, Dhanasekaran.D@in.bosch.com 2017
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
#include <inttypes.h>

#include "Events.h"
#include "Core.h"

LOG_IMPORT_CONTEXT(iap2)

namespace adit { namespace iap2service {

int ServerEvent::process(int32_t events)
{
    (void)events;
    struct sockaddr_un address;
    socklen_t address_length = sizeof(struct sockaddr_un);
    memset(&address, 0, sizeof(struct sockaddr_un));
    int fd = accept(mFd, (struct sockaddr *) &address, &address_length);
    if(fd < 0)
    {
        LOG_ERROR((iap2, "accept call failed with errno = %d (%s)", errno, strerror(errno)));
    }
    else
    {
        Core::instance().addClient(fd); //add to epoll
    }

    return 0;
}

int ClientEvent::process(int32_t events)
{
    try {
        std::shared_ptr<AppDataReceiver> msgReceiver(new AppDataReceiver(mClientId, mFd, events, this));
        LOGD_DEBUG((iap2, "ClientEvent triggered for ID(%d) FD(%d) EventID(%" PRIu64 ")", mClientId, mFd, mEventID));
        Core::instance().pushWorkItem(msgReceiver);
    } catch (const std::exception& e) {
        LOG_ERROR((iap2, "Exception in handleClientEvent: (%s)", e.what()));
    }

    return 0;
}

int DeviceEvent::process(int32_t events)
{
    try {
        std::shared_ptr<DeviceDataReceiver> msgReceiver(new DeviceDataReceiver(mDevice->iAP2DeviceId, mFd, events, this));
        LOGD_DEBUG((iap2, "DeviceEvent triggered for ID(%d) FD(%d) EventID(%" PRIu64 ")", mDevice->iAP2DeviceId, mFd, mEventID));
        Core::instance().pushWorkItem(msgReceiver);
    } catch (const std::exception& e) {
        LOG_ERROR((iap2, "Exception in handleDeviceEvent: (%s)", e.what()));
    }
    return 0;
}

int ShutdownEvent::process(int32_t events)
{
    (void)events;
    uint64_t data = 0;
    ssize_t rc = read(mFd, &data, sizeof(uint64_t));
    if(rc > 0)
    {
        LOG_WARN((iap2, "FileDescriptorType::Shutdown :%" PRIu64 "!", data));
    }
    return data;
}

int ThreadFreeEvent::process(int32_t events)
{
    (void)events;
    uint64_t data = 0;
    ssize_t rc = read(mFd, &data, sizeof(uint64_t));
    if(rc > 0)
    {
        LOGD_DEBUG((iap2, "FileDescriptorType::ThreadFreeEvent"));
    }
    return data;
}

std::mutex Event::mEventMapLock;
std::map<uint64_t, std::shared_ptr<Event>> Event::mEventMap;

bool Event::addEvent(uint64_t eventID, std::shared_ptr<Event> evt)
{
    mEventMapLock.lock();
    auto rc = mEventMap.insert(std::pair<uint64_t, std::shared_ptr<Event>>(eventID, evt));
    mEventMapLock.unlock();

    return rc.second;
}

void Event::removeEvent(uint64_t eventID)
{
    mEventMapLock.lock();
    int rc = mEventMap.erase(eventID);
    mEventMapLock.unlock();

    LOGD_DEBUG((iap2, "%d events removed", rc));
}

} } //namespace adit { namespace iap2service {
