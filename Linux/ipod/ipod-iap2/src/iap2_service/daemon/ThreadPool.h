/************************************************************************
 * @file: ThreadPool.h
 *
 * @version: 1.0
 *
 * @description: Thread pool class declaration for iAP2Service.
 * Creates and manages number of threads for executing the WorkItems created by the mainloop.
 * This module will be used by the Dispatcher for assigning the work items.
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

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <condition_variable>
#include <thread>
#include <mutex>
#include <queue>
#include <vector>
#include <chrono>
#include <functional>
#include "WorkItems.h"

namespace adit { namespace iap2service {

class ThreadPool;

class WorkThread
{
public:
    WorkThread(ThreadPool* threadPool) :
        mAvailable(false),
        mThreadPool(threadPool),
        mThread(new std::thread(std::bind(&WorkThread::run, this))),
        mStop(false)
    {
    }
    ~WorkThread();
    void submitWork(std::shared_ptr<WorkItem> workItem);
    int cancelWork();
    static void run(WorkThread* exinf);
    int getId() {return mThread->native_handle();}
    int getPid() {return mPid;}

private:
    void clearState() {
        mWorkItem.reset();
        mStop = false;
    }

private:
    std::shared_ptr<WorkItem> mWorkItem;
    std::condition_variable mConditionVariable;
    std::mutex mConditionMutex;
    bool mAvailable;
    ThreadPool* mThreadPool;
    std::unique_ptr<std::thread> mThread;
    int mPid;
    bool mStop;
};

class ThreadPool
{
public:
    ThreadPool(uint32_t threadCount);
    ~ThreadPool();

    uint32_t startWork(std::shared_ptr<WorkItem> work);
    int cancelWork(int32_t workerThreadId);
    void finishedWork(WorkThread* thread);

private:
    uint32_t mCount;
    std::mutex mThreadLock;
    std::queue<std::shared_ptr<WorkThread>> mFreeThreads;
    std::vector<std::shared_ptr<WorkThread>> mBusyThreads;
};

} } //namespace adit { namespace iap2service {

#endif /* THREADPOOL_H_ */
