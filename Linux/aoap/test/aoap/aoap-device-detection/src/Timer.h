/**
* \file: Timer.h
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

#ifndef TEST_TIMER_H_
#define TEST_TIMER_H_

#include <sys_time_adit.h>

#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <signal.h>

class TestTimer
{
public:
    TestTimer();
    ~TestTimer();

    /* handle timer */
    int32_t createSignalTimer(void);
    int32_t startSignalTimer(int32_t inTimeoutMs);
    int32_t deleteSignalTimer(void);

private:
    timer_t    mTimerId;
    bool       mTimerCreated;
    int32_t    mTimeoutMs;
};


#endif /* TEST_TIMER_H_ */
