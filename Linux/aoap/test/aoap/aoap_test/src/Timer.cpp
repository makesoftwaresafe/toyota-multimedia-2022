/**
* \file: Timer.cpp
*
* \version: 0.1
*
* \release: $Name:$
*
* Includes the necessary timer implementation.
*
* \component: AOAP
*
* \author: D. Girnus / ADIT/ESM / dgirnus@de.adit-jv.com
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
#include <adit_dlt.h>
#include <time.h>
#include "Timer.h"


AoapTestTimer::AoapTestTimer()
{
    mTimerId      = nullptr;
    mTimerCreated = false;
    mTimeoutMs    = 0;
}

AoapTestTimer::~AoapTestTimer()
{
    mTimerId      = nullptr;
    mTimerCreated = false;
    mTimeoutMs    = 0;
}

int32_t AoapTestTimer::createSignalTimer(void)
{
    int32_t ret = 0;

    struct sigevent sev;
    sev.sigev_notify = SIGEV_SIGNAL;
    sev.sigev_signo  = SIGINT;
    sev.sigev_value.sival_ptr = &mTimerId;

    ret = timer_create(CLOCK_MONOTONIC, &sev, &mTimerId);
    if (ret == -1)
        fprintf(stdout, "%s()  timer_create failed \n", __func__);
    else
        mTimerCreated = true;

    return ret;
}

int32_t AoapTestTimer::startSignalTimer(int32_t inTimeoutMs)
{
    int32_t ret = 0;
    struct itimerspec its;

    if (mTimerCreated)
    {
        mTimeoutMs = inTimeoutMs;

        its.it_value.tv_sec     = mTimeoutMs;
        its.it_value.tv_nsec    = 0;
        its.it_interval.tv_sec  = 0;
        its.it_interval.tv_nsec = 0;

        ret = timer_settime(mTimerId, 0, &its, nullptr);
        if (ret == -1)
            fprintf(stdout, "%s()  timer_settime failed \n", __func__);
    }
    else
    {
        fprintf(stdout, "%s()  Timer was created \n", __func__);
        ret = -1;
    }
    return ret;
}

int32_t AoapTestTimer::deleteSignalTimer(void)
{
    int32_t ret = 0;

    if (mTimerCreated)
    {
        fprintf(stdout, "%s()  Stop/delete timer \n", __func__);
        ret = timer_delete(mTimerId);
        mTimerCreated = false;
    }
    else
    {
        fprintf(stdout, "%s()  Timer was not created \n", __func__);
        ret = -1;
    }

    return ret;
}
