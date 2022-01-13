/************************************************************************
 * @file: EpollUtility.h
 *
 * @version: 1.0
 *
 * @description: CEPoll utility class provides convenience functions for
 * adding File descriptors to epoll_ctl.
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
#ifndef EPOLL_UTILITY_H_
#define EPOLL_UTILITY_H_

#include <adit_logging.h>

#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

namespace adit { namespace iap2service {

class CEpoll
{
public:
    CEpoll() {
        mFd = epoll_create(10);
    }
    virtual ~CEpoll() {
        close(mFd);
    }

    virtual int addFd(int fd, int events, uint64_t user_info)
    {
        struct epoll_event ev = {0, 0}; /* Content will be copied by the kernel */
        ev.events =  events;
        ev.data.u64 = user_info;

        makeSocketNonBlocking(fd);
        return epoll_ctl(mFd, EPOLL_CTL_ADD, fd, &ev);
    }
    virtual int removeFd(int fd)
    {
        return epoll_ctl(mFd, EPOLL_CTL_DEL, fd, NULL);
    }
    virtual int getFd() {return mFd;}

private:
    int makeSocketNonBlocking(int sockFd)
    {
        int flags = 0;
        flags = fcntl (sockFd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        return fcntl (sockFd, F_SETFL, flags);
    }

private:
    int mFd;
};

} } //namespace adit { namespace iap2service {

#endif /* EPOLL_UTILITY_H_ */
