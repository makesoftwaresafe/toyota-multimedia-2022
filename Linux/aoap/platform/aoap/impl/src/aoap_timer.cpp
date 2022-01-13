/*
 * aoap_timer.cpp
 *
 *  Created on: Aug 20, 2013
 *      Author: Thilo Bjoern Fickel
 *
 *  This source code is proprietary of ADIT
 *  Copyright (C) Advanced Driver Information Technology Joint Venture GmbH
 *  All rights reserved
 */

#include "aoap_timer.h"
#include "aoap_logging.h"
#include <pthread_adit.h>
#include <sys_time_adit.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/prctl.h>
#include <map>
#include <limits>


using namespace AOAP::Logging;
using namespace AOAP::Time;


/* global variables */
static pthread_mutex_t g_TimerMutex = PTHREAD_MUTEX_INITIALIZER;
/* Change values only if g_TimerMutex is locked */
static int g_TimerMapId = 0;
static std::map<int, void*> g_TimerMap;


/* APIs to lock/unlock the global mutex g_TimerMutex */
int  trylockTimerMutex(unsigned int inMaxRetries, unsigned int inWaitUsec);
int  unlockTimerMutex(void);

int trylockTimerMutex(unsigned int inMaxRetries, unsigned int inWaitUsec)
{
    /*PRQA: Lint Message 454: This is intention. Mutex will be unlocked in unlockTimerMutex() */
    /*lint -save -e454*/
    int err = -1;
    if (inMaxRetries > 0) {
        do {
            err = pthread_mutex_trylock(&g_TimerMutex);
            if (EBUSY == err) {
                if (inWaitUsec > 0) {
                    dbgPrintLine(eLogDebug, "trylockTimerMutex()  mutex already locked. wait %d usec to retry(cnt=%d)",
                                inWaitUsec, inMaxRetries);
                    usleep(inWaitUsec);
                }
            }
            inMaxRetries--;
        } while((EBUSY == err) && (inMaxRetries > 0));
        if (0 != err) {
            dbgPrintLine(eLogError, "trylockTimerMutex()  pthread_mutex_trylock() failed. err=%d, errno=%d",
                        err, errno);
        }

        return err;
    } else {
        err = pthread_mutex_lock(&g_TimerMutex);
        if (0 != err) {
            dbgPrintLine(eLogError, "trylockTimerMutex()  pthread_mutex_lock() failed. err=%d, errno=%d",
                        err, errno);
        }
        return err;
    }
    return err;
    /*lint -restore*/
}

int unlockTimerMutex()
{
    /*PRQA: Lint Message 455: This is intention. Mutex will be locked in trylockTimerMutex() */
    /*lint -save -e455*/
    int err = pthread_mutex_unlock(&g_TimerMutex);
    if (0 != err) {
        dbgPrintLine(eLogError, "unlockTimerMutex() ERROR: Failed to unlock mutex. err=%d, errno=%d",
                    err, errno);
    }
    return err;
    /*lint -restore*/
}


Timer::Timer()
: mTimerStarted(false)
, mTimerCreated(false)
, mTimerObjectId(-1)
{
    dbgPrintLine(eLogDebug, "Timer() create this=%p", this);
}

Timer::~Timer()
{
    stop();

   /* The mutex object referenced by g_TimerMutex
    * will be not destroy during process runtime. */
}

void Timer::start(unsigned int seconds, unsigned int mseconds /*= 0*/)
{
    dbgPrintLine(eLogDebug, "start(%ds, %dms) called", seconds, mseconds);
    /* make sure there is no running timer */
    stop();

    if (0 == trylockTimerMutex(0, 0))
    {
        /* wrap global identifier to 0 if INT_MAX was reached */
        if (g_TimerMapId >= std::numeric_limits<int>::max()) {
            g_TimerMapId = 0;
        }
        /* Increase global identifier 'g_TimerMapId' to have
         * a new identifier for the next Timer. */
        mTimerObjectId = g_TimerMapId++;

        memset(&mTimerEvent, 0, sizeof(mTimerEvent));
        /* Set the sigevent structure to cause the signal to be
         * delivered by creating a new thread. */
        mTimerEvent.sigev_notify = SIGEV_THREAD;
        mTimerEvent.sigev_notify_function = Timer::handler;
        /* assign data passed with notification to Timer::handler */
        mTimerEvent.sigev_value.sival_int = mTimerObjectId;

        /* Create timer */
        if (-1 == timer_create(CLOCK_MONOTONIC, &mTimerEvent , &mTimerId))
        {
            dbgPrintLine(eLogError, "start() ERROR: Failed to create timer");
            mTimerCreated = false;
        }
        else
        {
            mTimerCreated = true;

            gettimeofday(&mStartTime, NULL);
            dbgPrintLine(eLogDebug, "start(%ds, %dms) at %1d timerId is 0x%1x",
                    seconds, mseconds, mStartTime.tv_sec, (long)mTimerId);

            /* Start timer */
            mITimerSpec.it_value.tv_sec = static_cast<__time_t>(seconds);
            mITimerSpec.it_value.tv_nsec = static_cast<long int>(mseconds*1000*1000); //nano seconds
            mITimerSpec.it_interval.tv_sec = 0; //no interval
            mITimerSpec.it_interval.tv_nsec = 0; //no interval

            if (-1 == timer_settime(mTimerId, 0, &mITimerSpec, NULL))
            {
                dbgPrintLine(eLogError, "start() ERROR: Failed to start timer");
                mTimerStarted = false;
            }
            else
            {
                /* Timer was created and started -> insert Timer object into map */
                g_TimerMap.insert(std::pair<int, void*>(mTimerObjectId, this));

                mTimerStarted = true;

                dbgPrintLine(eLogDebug, "start(timerId=0x%1x) started", (long)mTimerId);
            }
        }
        unlockTimerMutex();
    }
    else
    {
        dbgPrintLine(eLogError, "start() ERROR: trylockTimerMutex failed");
    }
}

void Timer::stop(void)
{
    if (0 == trylockTimerMutex(0, 0)) {
        if (mTimerCreated) {
            mTimerStarted = false;
            try
            {
                memset(&mITimerSpec, 0, sizeof(mITimerSpec));
                if (-1 == timer_settime(mTimerId, 0, &mITimerSpec, NULL)) {
                    dbgPrintLine(eLogError, "stop() ERROR: Failed to stop timer ", strerror(errno));
                } else {
                    dbgPrintLine(eLogDebug, "stop() timerId=0x%1x stopped", (long)mTimerId);
                }

                /* Delete the timer given in mTimerId.
                 * If the timer was armed at the time of this call,
                 * it is disarmed before being deleted.
                 * The treatment of any pending signal generated
                 * by the deleted timer is unspecified.
                 * */
                if (-1 == timer_delete(mTimerId)) {
                    dbgPrintLine(eLogError, "stop() ERROR: Failed to call timer_delete with errno=%s",
                            strerror(errno));
                }
                mTimerCreated = false;

                /* find Timer object in map and erase element from map */
                if (mTimerObjectId >= 0) {
                    std::map<int, void*>::iterator itr = g_TimerMap.find(mTimerObjectId);
                    if(itr != g_TimerMap.end()) {
                        g_TimerMap.erase(itr);
                        mTimerObjectId = -1;
                    }
                }
            }
            catch (...)
            { /*PRQA: Lint Message 1775: This is intention. Use Catch-all handler, which is activated for any exception. */
              //lint !e1775
                dbgPrintLine(eLogFatal, "stop() FATAL ERROR: 'timer_delete' caused exception");
            }
        } else {
            if (mTimerStarted) {
                dbgPrintLine(eLogError, "stop() ERROR: mTimerId is NULL but mTimerStarted is set");
                mTimerStarted = false;
            } else {
                dbgPrintLine(eLogDebug, "stop() nothing to stop");
            }
        }
        unlockTimerMutex();
    } else {
        dbgPrintLine(eLogError, "stop()  trylockTimerMutex failed");
    }
}

/* Note:
 * Calling printf() from a signal handler is not
 * strictly correct, since printf() is not async-signal-safe; see signal(7) */
/*static*/ void Timer::handler(union sigval sig)
{
    prctl(PR_SET_NAME, "aoapTimerHdlr", 0, 0, 0);

    /* lock the mutex */
    if (0 == trylockTimerMutex(0, 0)) {
        /* check if Timer object was closed or if it is still active */
        std::map<int, void*>::iterator itr = g_TimerMap.find(sig.sival_int);
        if(itr != g_TimerMap.end()) {
            /* cast pointer passed with notification to class Timer */
            Timer* pTimer = static_cast<Timer*>(itr->second);
            if (pTimer) {
                if (pTimer->mTimerStarted) {
                    /* call timeout handler only if running */
                    pTimer->timeoutHandler();
                }
            }
        }
        unlockTimerMutex();
    }
}
