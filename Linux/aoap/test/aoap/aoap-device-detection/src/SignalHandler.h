
/************************************************************************
 *
 * \file: SignalHandler.h
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


#ifndef TEST_SIGNALHANDLER_H
#define TEST_SIGNALHANDLER_H

#include <sys/epoll.h>
#include <pthread_adit.h>

#include <condition_variable>
#include <mutex>

class SignalHandler
{
public:
    SignalHandler();
    virtual ~SignalHandler();

    int32_t startSignalHandler();
    int32_t stopSignalHandler();

    virtual void signaling(void) = 0;
protected:
//    static void handler(int inSignal);

private:
    bool mSignaled;

    static int32_t signalHandlerCb(void* context);
    int32_t epollRegisterSignalFd(int32_t epollFd);
    int32_t epollWaitSignalFd(int32_t epollFd, struct epoll_event* epollEvents);

    int32_t mEpollFd;

    static void* signalHandlerThread(void* context);

    /* ID of the EventThread */
    pthread_t mThreadId;

    int32_t mSignalFd;

    bool mRunning;

    std::condition_variable mSyncCondVar;
    std::mutex mSyncMutex;
};


#endif /* TEST_SIGNALHANDLER_H */
