/*******************************************************************************
 *  \copyright (c) 2016 Advanced Driver Information Technology.
 *                   ADIT is a joint venture company of
 *   Robert Bosch GmbH/Robert Bosch Car Multimedia GmbH and DENSO Corporation
 *
 *  \author: Jens Lorenz, jlorenz@de.adit-jv.com 2015-2016
 *           Mattia Guerra, mguerra@de.adit-jv.com 2016
 *
 *
 *  \copyright The MIT License (MIT)
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"),
 *  to deal in the Software without restriction, including without limitation
 *  the rights to use, copy, modify, merge, publish, distribute, sublicense,
 *  and/or sell copies of the Software, and to permit persons to whom the
 *  Software is furnished to do so, subject to the following conditions:
 *  The above copyright notice and this permission notice shall be included in
 *  all copies or substantial portions of the Software.
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 *  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 *  DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 *  OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR
 *  THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 *  For further information see http://www.genivi.org/.
 ******************************************************************************/


#include <stdexcept>
#include <cassert>
#include <string.h>
#include <time.h>
#include "CAmRoutingAdapterThread.h"

using namespace am;

#define THROW_ASSERT_NEQ(CALL, COND) \
    do { \
        int _err = COND; \
        if (!((_err = (CALL)) == (COND))) throw std::runtime_error( \
            std::string(__func__) + ": (" + std::string(#CALL) + " = " + std::string(strerror(_err)) + ")"); \
    } while(0)


CAmRoutingAdapterThread::CAmRoutingAdapterThread()
    : mId(), mName(), mState(STATE_JOINED)
{
    THROW_ASSERT_NEQ(pthread_mutex_init(&mMtx, NULL), 0);
    THROW_ASSERT_NEQ(pthread_cond_init(&mCond, NULL), 0);
}

CAmRoutingAdapterThread::~CAmRoutingAdapterThread()
{
    CAmRoutingAdapterThread::joinThread();
    pthread_cond_destroy(&mCond);
    pthread_mutex_destroy(&mMtx);
}

void CAmRoutingAdapterThread::setThreadName(std::string name)
{
    mName = name;
    mName.resize(PTHREAD_NAME_LEN);

    if (isStateForked())
    {
        // the cast inside is for Lint
        pthread_setname_np(mId, static_cast<const char*>(mName.c_str()));
    }
}

void CAmRoutingAdapterThread::setThreadSched(int policy, int priority)
{
    __cpu_sched cpuSched(policy, priority);
    setThreadSched(cpuSched);
}

void CAmRoutingAdapterThread::setThreadSched(__cpu_sched & cpuSched)
{
    mCpuSched = cpuSched;

    if (isStateForked())
    {
        int policy;
        struct sched_param param;
        pthread_getschedparam(mId, &policy, &param);
        param.__sched_priority = mCpuSched.priority;
        pthread_setschedparam(mId, mCpuSched.policy, &param);
    }
}

int CAmRoutingAdapterThread::startThread()
{
    int ret = 0;

    switch (getState())
    {
        case STATE_JOINED:
            ret = startWorkerThread();
            break;
        case STATE_JOINING:
            waitForStateChange(STATE_JOINING);
            ret = startWorkerThread();
            break;
        case STATE_FORKING:
            waitForStateChange(STATE_FORKING);
            break;
        case STATE_STOPPING:
            waitForStateChange(STATE_STOPPING);
            setState(STATE_RUNNING);
            break;
        case STATE_STOPPED:
            setState(STATE_RUNNING);
            break;
        default:
            break;
    }

    return ret;
}

int CAmRoutingAdapterThread::stopThread()
{
    switch(getState())
    {
        case STATE_FORKING:
            waitForStateChange(STATE_FORKING);
            setStateAndWaitForStateChange(STATE_STOPPING);
            break;
        case STATE_RUNNING:
            setStateAndWaitForStateChange(STATE_STOPPING);
            break;
        case STATE_STOPPING:
            waitForStateChange(STATE_STOPPING);
            break;
        case STATE_STOPPED:
            break;
        case STATE_JOINING:
        case STATE_JOINED:
        default:
            return -1;
    }

    return 0;
}

int CAmRoutingAdapterThread::joinThread()
{
    switch (getState())
    {
        case STATE_FORKING:
            waitForStateChange(STATE_FORKING);
            setState(STATE_JOINING);
            break;
        case STATE_RUNNING:
        case STATE_STOPPING:
        case STATE_STOPPED:
            setState(STATE_JOINING);
            break;
        case STATE_JOINED:
            return mThreadErr;
        case STATE_JOINING:
        default:
            break;
    }

    pthread_join(mId, NULL);
    setState(STATE_JOINED);
    return mThreadErr;
}

int CAmRoutingAdapterThread::startWorkerThread(void)
{
    pthread_attr_t attr;

    /* initialize thread attributes */
    THROW_ASSERT_NEQ(pthread_attr_init(&attr), 0);
    THROW_ASSERT_NEQ(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE), 0);
    if ((mCpuSched.policy != SCHED_OTHER) || (mCpuSched.priority != 0))
    {
        struct sched_param param;
        THROW_ASSERT_NEQ(pthread_attr_getschedparam(&attr, &param), 0);
        param.sched_priority = mCpuSched.priority;
        THROW_ASSERT_NEQ(pthread_attr_setschedpolicy(&attr, mCpuSched.policy), 0);
        THROW_ASSERT_NEQ(pthread_attr_setschedparam(&attr, &param), 0);
        THROW_ASSERT_NEQ(pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED), 0);
    }

    /* start thread */
    setState(STATE_FORKING);
    THROW_ASSERT_NEQ(pthread_create(&mId, &attr, &CAmRoutingAdapterThread::_WorkerThread, this), 0);
    THROW_ASSERT_NEQ(pthread_attr_destroy(&attr), 0);

    waitForStateChange(STATE_FORKING);
    if (getState() != STATE_RUNNING)
    {
        int err = pthread_join(mId, NULL);
        if ((err != 0) && (mThreadErr == 0))
        {
            mThreadErr = err;
        }
        setState(STATE_JOINED);
        return mThreadErr;
    }

    return 0;
}

void CAmRoutingAdapterThread::WorkerThread()
{
    enum __sub_state
    {
        SUB_STATE_INIT = 0,
        SUB_STATE_WORKING,
        SUB_STATE_WAITING
    } sub_state = SUB_STATE_INIT;
    
    if (mName.size() != 0)
    {
        // the cast inside is for Lint
        pthread_setname_np(mId, static_cast<const char*>(mName.c_str()));
    }
    
    setState(STATE_RUNNING);
    do
    {
        switch (sub_state)
        {
        case SUB_STATE_INIT:
            mThreadErr = static_cast<CAmRoutingAdapterThread*>(this)->initThread();
            if (mThreadErr == 0)
            {
                sub_state = SUB_STATE_WORKING;
            }
            break;
        case SUB_STATE_WORKING:
            mThreadErr = static_cast<CAmRoutingAdapterThread*>(this)->workerThread();
            break;
        case SUB_STATE_WAITING:
            struct timespec req;
            req.tv_sec = 0;
            req.tv_nsec = 100000000;
            mThreadErr = nanosleep(&req, NULL);
            if (mThreadErr == 0)
            {
                sub_state = SUB_STATE_INIT;
            }
            break;
        default:
            mThreadErr = 0;
            break;
        }
        if (mThreadErr != 0)
        {
            static_cast<CAmRoutingAdapterThread*>(this)->deinitThread(mThreadErr);
            sub_state = SUB_STATE_WAITING;
        }
        
        if (getState() == STATE_STOPPING)
        {
            setStateAndWaitForStateChange(STATE_STOPPED);
        }
    } while (isStateForked());
    
    setState(STATE_JOINING);
    static_cast<CAmRoutingAdapterThread*>(this)->deinitThread(mThreadErr);

    pthread_exit(NULL);
}

void CAmRoutingAdapterThread::setState(const eState state)
{
    THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);
    mState = state;
    THROW_ASSERT_NEQ(pthread_cond_signal(&mCond), 0);
    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);
}

CAmRoutingAdapterThread::eState CAmRoutingAdapterThread::getState(void)
{
    return mState;
}

bool CAmRoutingAdapterThread::isStateForked()
{
    return static_cast<bool>(mState & (STATE_RUNNING | STATE_STOPPING | STATE_STOPPED));
}

void CAmRoutingAdapterThread::waitForStateChange(const eState state)
{
    THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);
    while (mState == state)
    {
        THROW_ASSERT_NEQ(pthread_cond_wait(&mCond, &mMtx), 0);
    }
    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);
}

void CAmRoutingAdapterThread::setStateAndWaitForStateChange(const eState state)
{
    THROW_ASSERT_NEQ(pthread_mutex_lock(&mMtx), 0);
    mState = state;
    THROW_ASSERT_NEQ(pthread_cond_signal(&mCond), 0);
    while (mState == state)
    {
        THROW_ASSERT_NEQ(pthread_cond_wait(&mCond, &mMtx), 0);
    }
    THROW_ASSERT_NEQ(pthread_mutex_unlock(&mMtx), 0);
}
