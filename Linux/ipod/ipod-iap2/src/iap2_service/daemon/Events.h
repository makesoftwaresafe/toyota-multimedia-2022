/************************************************************************
 * @file: Events.h
 *
 * @version: 1.0
 *
 * @description: Events module declares context for each epoll event.
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

#ifndef EVENTS_H_
#define EVENTS_H_

#include <iap2_init_private.h>
#include <iap2_service_init.h>
#include <memory>
#include <map>
#include <mutex>

namespace adit { namespace iap2service {

class Event
{
public:
    Event();
    Event(int32_t fd, uint64_t evtID) : mFd(fd), mEventID(evtID){}
    virtual ~Event(){}
    virtual int process(int32_t events) = 0;
    int getFd() { return mFd;}
    uint64_t getEventID() { return mEventID; }
    static std::mutex mEventMapLock;
    static std::map<uint64_t, std::shared_ptr<Event>> mEventMap;
    static bool addEvent(uint64_t eventID, std::shared_ptr<Event> evt);
    static void removeEvent(uint64_t eventID);
protected:
    int mFd;
    uint64_t mEventID;
};

class ServerEvent : public Event
{
public:
    ServerEvent();
    ~ServerEvent(){}
    ServerEvent(int32_t fd, uint64_t evtID) : Event(fd, evtID){}
    int process(int32_t events) override;
};

class ClientEvent : public Event
{
public:
    ClientEvent();
    ~ClientEvent(){}
    ClientEvent(int32_t fd, int32_t clientId, uint64_t evtID) : Event(fd, evtID), mClientId(clientId){}
    int process(int32_t events) override;
private:
    int mClientId;
};

class DeviceEvent : public Event
{
public:
    DeviceEvent();
    ~DeviceEvent(){}
    DeviceEvent(int32_t fd, uint64_t evtID, std::shared_ptr<iAP2Device_st> device) : Event(fd, evtID), mDevice(device) {}
    int process(int32_t events) override;
private:
    std::shared_ptr<iAP2Device_st> mDevice;
};

class ShutdownEvent: public Event
{
public:
    ShutdownEvent();
    ~ShutdownEvent(){}
    ShutdownEvent(int fd, uint64_t evtID) : Event(fd, evtID){}
    int process(int32_t events) override;
};

class ThreadFreeEvent: public Event
{
public:
    ThreadFreeEvent();
    ~ThreadFreeEvent(){}
    ThreadFreeEvent(int fd, uint64_t evtID) : Event(fd, evtID){}
    int process(int32_t events) override;
};

} } //namespace adit { namespace iap2service {

#endif /* EVENTS_H_ */
