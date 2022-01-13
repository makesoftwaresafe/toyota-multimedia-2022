
/************************************************************************
 *
 * \file: SignalHandler.cpp
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * <brief description>.
 * <detailed description>
 * \component: AOAP
 *
 * \author: J. Harder / ADITG/SW1 / jharder@de.adit-jv.com
 *          D. Girnus / ADITG/ESM / dgirnus@de.adit-jv.com
 *
 * \copyright (c) 2017 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \see <related items>
 *
 * \history
 *
 ***********************************************************************/

#include <adit_logging.h>

#include <sys/signalfd.h>
#include <signal.h>
// memset
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "SignalHandler.h"


SignalHandler::SignalHandler()
{
    mRunning  = false;
    mSignaled = false;
    mSignalFd = -1;
    mEpollFd  = -1;
    mThreadId = -1;
}

SignalHandler::~SignalHandler()
{
    if (true == mRunning) {
        stopSignalHandler();
    }
}

int32_t SignalHandler::startSignalHandler()
{
    int32_t res = 0;

    sigset_t mask;

    //  initializes the signal mask to empty
    sigemptyset(&mask);
    // add respectively signal to signal mask
    sigaddset(&mask, SIGTERM);
    sigaddset(&mask, SIGUSR1);
    sigaddset(&mask, SIGINT);
    sigaddset(&mask, SIGQUIT);
    sigaddset(&mask, SIGHUP);

    // use sigprocmask to fetch the signal mask
    if (sigprocmask(SIG_BLOCK, &mask, NULL) == -1) {
        fprintf(stderr, "%s()  Could not set procmask \n", __func__);
        assert(&mask);
    }

    // create new file descriptor and associates the signal mask with that descriptor.
    mSignalFd = signalfd(-1, &mask, 0);
    if (mSignalFd < 0) {
        fprintf(stderr, "%s()  Wrong parameter to add event fd=%d \n", __func__, mSignalFd);
        assert(&mSignalFd);
    }

    /* create and open epoll descriptor */
    mEpollFd = epoll_create1(0);
    if (0 > mEpollFd) {
        fprintf(stderr, "%s()  epoll_create1 error: %d \n", __func__, mEpollFd);
        assert(&mEpollFd);
    }

    mThreadId = -1;
    mRunning = true;
    std::unique_lock<std::mutex> guard(mSyncMutex);
    // create thread to poll signals targeted at the caller
    int32_t err = pthread_create(&mThreadId, nullptr, &signalHandlerThread, this);
    if (0 != err) {
        fprintf(stderr, "%s()  Create signalHandlerThread failed err=%d, errno=%d \n", __func__, err, errno);
        assert(&mThreadId);
    } else {

        fprintf(stderr, "%s()  signalHandlerThread created \n", __func__);
        mSyncCondVar.wait(guard);
        guard.unlock();

        fprintf(stderr, "%s()  Thread sync done \n", __func__);
    }

    return res;
}

int32_t SignalHandler::stopSignalHandler()
{
    int32_t res = 0;

    mRunning = false;

    // close epoll descriptor
    if (mEpollFd >= 0) {
        close(mEpollFd);
        mEpollFd = -1;
    }

    if (mThreadId > 0) {
        pthread_join(mThreadId, nullptr);
        mThreadId = -1;
    }

    // close signal file descriptor
    if (mSignalFd >= 0) {
        close(mSignalFd);
        mSignalFd = -1;
    }

    return res;
}

int32_t SignalHandler::signalHandlerCb(void* context)
{
    int32_t res = 0;
    fprintf(stderr, "%s()  Signal received \n", __func__);

    if (nullptr != context) {
        auto me = static_cast<SignalHandler*>(context);

        fprintf(stderr, "%s()  Signal AoapDiscoveryTest to stop \n", __func__);
        me->signaling();
        if (!me->mSignaled) {
            me->mSignaled= true;
        } else {
            fprintf(stderr, "%s()  Signalhandler already signaled \n", __func__);
        }
    } else {
        fprintf(stderr, "%s()  Could not cast input pointer \n", __func__);
        res = -1;
    }

    return res;
}

int32_t SignalHandler::epollRegisterSignalFd(int32_t epollFd)
{
    struct epoll_event event;

    event.data.ptr = (void*)SignalHandler::signalHandlerCb;
    event.events = EPOLLIN;

    return epoll_ctl(epollFd, EPOLL_CTL_ADD, mSignalFd, &event);
}

int32_t SignalHandler::epollWaitSignalFd(int32_t epollFd, struct epoll_event* epollEvents)
{
    int32_t n = 0;
    int32_t res = -1;
    int32_t i = 0;

    if (!epollEvents) {
        fprintf(stderr, "%s()  No memory available for events \n", __func__);
        return -1;
    }

    n = epoll_wait(epollFd, epollEvents, 10, 500);
    if (n < 0) {
        fprintf(stderr, "%s()  epoll_wait error: n=%d, %s \n", __func__, n, strerror(errno));

        if (errno == EINTR) {
            /* Only exit if the daemon has received QUIT/INT/TERM */
            return 0;
        }
        return -1;
    } else if (n == 0) {
//        fprintf(stderr, "epoll_wait timed out: n=%d \n", n);
        res = 0;
    } else {
        for (i = 0 ; i < n ; i++)
        {
            int32_t (*callback)(void *context) = (int32_t (*)(void*))epollEvents[i].data.ptr;
            if (!callback)
            {
                fprintf(stderr, "%s()  Callback not found, exiting \n", __func__);
                break;
            }

            if (!(epollEvents[i].events & (EPOLLIN | EPOLLET)))
            {
                fprintf(stderr, "%s()  Error while polling. Event received: 0x%X \n",
                        __func__, epollEvents[i].events);

                /* We only support one event producer.
                 * Error means that this producer died.
                 */
                break;
            }

            struct signalfd_siginfo info;
            ssize_t bytes = read(mSignalFd, &info, sizeof(info));
            if (bytes > 0) {
                uint32_t sig = info.ssi_signo;
                switch(sig)
                {
                    case SIGTERM:
                        printf("\n%s()  Got SIGUSR1 \n", __func__);
                        break;
                    case SIGUSR1:
                        printf("\n%s()  Got SIGUSR1 \n", __func__);
                        break;
                    case SIGINT:
                        printf("\n%s()  Got SIGINT \n", __func__);
                        break;
                    case SIGQUIT:
                        printf("\n%s()  Got SIGQUIT \n", __func__);
                        break;
                    case SIGHUP:
                        printf("\n%s()  Got SIGHUP \n", __func__);
                        break;
                    default:
                        printf("\n%s()  Got unknown signal %u \n", __func__, sig);
                        break;
                }
            } else {
                printf("%s()  read(%d) returned %zd) \n", __func__, mSignalFd, bytes);
            }


            if (callback(this) < 0)
            {
                fprintf(stderr, "%s()  Error while calling the callback, exiting \n", __func__);
                break;
            }
            else
            {
                fprintf(stderr, "%s()  Callback return successfully \n", __func__);
                res = 0;
            }
        }
    }

    return res;
}

void* SignalHandler::signalHandlerThread(void* context)
{
    int32_t res = 0;

    auto me = static_cast<SignalHandler*>(context);
    if (me == nullptr) {
        fprintf(stderr, "%s()  Could not cast input pointer \n", __func__);
        return nullptr;
    }
    std::unique_lock<std::mutex> guard(me->mSyncMutex);

    if (0 != me->epollRegisterSignalFd(me->mEpollFd)) {
        fprintf(stderr, "%s()  Could not register epoll event for fd=%d \n", __func__, me->mEpollFd);
        res = -1;
    } else {
        fprintf(stderr, "%s()  Registered epoll event \n", __func__);
    }
    guard.unlock();
    me->mSyncCondVar.notify_one();

    if (0 == res) {
        struct epoll_event* epollEvents = new epoll_event[10];
        memset(epollEvents, 0, 10 * sizeof(epoll_event));

        while ((me->mRunning) && (0 == res))
        {
            res = me->epollWaitSignalFd(me->mEpollFd, epollEvents);
            if (0 > res) {
                fprintf(stderr, "%s()  epollWaitSignalFd error: %d \n", __func__, res);
            }
            memset(epollEvents, 0, 10 * sizeof(epoll_event));
        }

        delete[] epollEvents;
        epollEvents = nullptr;
    }

    return nullptr;
}


