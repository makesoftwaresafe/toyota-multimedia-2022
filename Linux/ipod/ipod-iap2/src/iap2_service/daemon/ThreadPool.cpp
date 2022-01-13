/************************************************************************
 * @file: ThreadPool.cpp
 *
 * @version: 1.0
 *
 * @description: Thread pool implementation for iAP2Service.
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

#include <adit_logging.h>
#include <unistd.h>

#include "Core.h"
#include "ThreadPool.h"

LOG_IMPORT_CONTEXT(disp)

namespace adit { namespace iap2service {

WorkThread::~WorkThread() {
    if (mThread != nullptr) {
        cancelWork();
        LOG_INFO((disp, "WorkThread join:%p", this));
        mThread->join();
        LOG_INFO((disp, "WorkThread deleted:%p", this));
    }
}

void WorkThread::submitWork(std::shared_ptr<WorkItem> workItem) {
    mWorkItem = workItem;
    if(mWorkItem != nullptr)
        LOGD_DEBUG((disp, "WorkItem(%p)(%s) submitted to Thread(0x%x)", mWorkItem.get(), WorkItem::getTypeString(mWorkItem->getType()), getId()));
    else
        LOG_ERROR((disp, "WorkItem is invalid in submitWork!"));
    std::unique_lock<std::mutex> lock(mConditionMutex);
    mAvailable = true;
    mConditionVariable.notify_one();
}

int WorkThread::cancelWork() {
    if(mWorkItem != nullptr)
        LOGD_DEBUG((disp, "WorkItem(%p)(%s) cancelled for the Thread", mWorkItem.get(), WorkItem::getTypeString(mWorkItem->getType())));
    mStop = true;
    std::unique_lock<std::mutex> lock(mConditionMutex);
    mAvailable = true;
    mConditionVariable.notify_one();
    return 0;
}

void WorkThread::run(WorkThread* task)
{
    task->mPid = getpid();
    while(!task->mStop)
    {
        //0. Wait for next work item
        std::unique_lock<std::mutex> lock(task->mConditionMutex);
        if(!task->mAvailable)
            task->mConditionVariable.wait(lock);
        task->mAvailable = false;

        //1. Exit thread (if shutdown)
        if(task->mStop)
        {
            LOG_INFO((disp, "WorkThread(%p) Stopped!", task));
            return ; /**************exit thread execution**************/
        }

        //2. Call WorkItems execute method
        if(task->mWorkItem != nullptr)
        {
            auto type = task->mWorkItem->getType();
            auto typeString = WorkItem::getTypeString(type);
            LOGD_DEBUG((disp, "WorkItem(%p)(%s) execution started", task->mWorkItem.get(), typeString));
            task->mWorkItem->execute(nullptr);
            LOGD_DEBUG((disp, "WorkItem(%p)(%s) execution finished", task->mWorkItem.get(), typeString));
        }
        //3. Store the result back in WorkItem object

        //4. Clear the state of the thread
        task->clearState();

        //5. add it back to the queue
        task->mThreadPool->finishedWork(task);

        //6. goto state 0
    }
}

ThreadPool::ThreadPool(uint32_t threadCount)
{
    mCount = threadCount;
    for(uint32_t i = 0; i < mCount; i++)
        mFreeThreads.push(std::shared_ptr<WorkThread>(new WorkThread(this)));
}

ThreadPool::~ThreadPool()
{
    LOG_INFO((disp, "ThreadPool Destructor"));
}

uint32_t ThreadPool::startWork(std::shared_ptr<WorkItem> work)
{
    std::unique_lock<std::mutex> lock(mThreadLock);

    if(mFreeThreads.empty())
        return 0;

    std::shared_ptr<WorkThread> thread = mFreeThreads.front();
    LOGD_DEBUG((disp, "Thread(0x%x) popped from free thread pool", thread->getId()));

    mFreeThreads.pop();
    mBusyThreads.push_back(thread);
    thread->submitWork(work);
    return thread->getId();
}

int ThreadPool::cancelWork(int32_t workerThreadId)
{
    std::unique_lock<std::mutex> lock(mThreadLock);
    for(std::shared_ptr<WorkThread> work : mBusyThreads)
    {
        if(work->getId() == workerThreadId)
            return work->cancelWork();
        LOGD_DEBUG((disp, "Cancel work"));
    }
    return -1;
}

void ThreadPool::finishedWork(WorkThread* task)
{
    std::unique_lock<std::mutex> lock(mThreadLock);
    for(auto it = mBusyThreads.begin(); it != mBusyThreads.end(); ++it)
    {
        if((*it)->getId() == task->getId())
        {
            mFreeThreads.push(*it);
            mBusyThreads.erase(it);
            LOGD_DEBUG((disp, "Thread(0x%x) pushed to free thread pool", task->getId()));
            break;
        }
        LOGD_DEBUG((disp, "Finished work"));
    }

    Core::instance().freeThreadAvailable();
}

} } //namespace adit { namespace iap2service {
