/************************************************************************
 * @file: AudioAlsaStateMachine.cpp
 *
 * @version: 0.1
 *
 * @description: This source file contains class definition of StateMachine and fo
 * Classes inherited from IState like AlsaStreamStateClosed, AlsaStreamStateStop,
 * AlsaStreamStateFadeIn, AlsaStreamStateRun and AlsaStreamStateFadeOut. StateMachine
 * class will be called by BackendALSA class for handling the Alsa stream states.
 * IState class will be used by StateMachine class.
 *
 * @authors: Jens Lorenz, jlorenz@de.adit-jv.com 2015
 *           Thouseef Ahamed, tahamed@de.adit-jv.com 2015
 *           Vijay Palaniswamy, vijay.palaniswamy@in.bosch.com 2015
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/
#include <sys/time.h>
#include "AudioAlsa.h"
#include "AudioAlsaStateMachine.h"
#include "AudioAlsaImpl.h"
#include "Logging.h"

using namespace adit::utility::audio;

#define _assert(cond) do{int32_t _cond_res = (cond); assert(_cond_res); _cond_res = _cond_res;}while(0)

StateMachine::StateMachine(BackendAlsaImpl* beAlsaImpl, LoggingInterface& loggingHandle) : mCurrentState(ALSA_SSTATE_CLOSED), mLoggingHandle(loggingHandle)
{
    pthread_condattr_t attr;

    mStateTable[ALSA_SSTATE_CLOSED]       = new AlsaStreamStateClosed   (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_STOP]         = new AlsaStreamStateStop     (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_STOPPING]     = new AlsaStreamStateStopping (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_RUN_FADE_IN]  = new AlsaStreamStateFadeIn   (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_RUNNING]      = new AlsaStreamStateRun      (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_RUN_FADE_OUT] = new AlsaStreamStateFadeOut  (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_ABORTING]     = new AlsaStreamStateAborting (*this, beAlsaImpl, mLoggingHandle);
    mStateTable[ALSA_SSTATE_CLOSING]      = new AlsaStreamStateClosing  (*this, beAlsaImpl, mLoggingHandle);

    /* Setting up the condvar attributes to use CLOCK_MONOTONIC */
    _assert(pthread_condattr_init( &attr)                      == 0);
    _assert(pthread_condattr_setclock( &attr, CLOCK_MONOTONIC) == 0);

    _assert(pthread_mutex_init(&mMtx, nullptr)                 == 0);
    _assert(pthread_cond_init(&mCond, &attr)                   == 0);
}

StateMachine::~StateMachine()
{
    /* Looping through all state table objects and deleting */
    std::map<AlsaStreamState, IState*>::iterator itr = mStateTable.begin();
    for (; itr != mStateTable.end(); ++itr)
    {
        delete itr->second;
    }

    /* Destroying the Mutex and Condition variables */
    pthread_mutex_destroy(&mMtx);
    pthread_cond_destroy(&mCond);
}

AudioError StateMachine::openEvent(StreamInfo &data)
{
    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::openEvent mCurrentState " << mCurrentState << Logging::endl;
    return mStateTable[mCurrentState]->openEvent(data);
}

AudioError StateMachine::closeEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::closeEvent mCurrentState " << mCurrentState << Logging::endl;
    return mStateTable[mCurrentState]->closeEvent();
}

AudioError StateMachine::startEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::startEvent mCurrentState " << mCurrentState << Logging::endl;
    return mStateTable[mCurrentState]->startEvent();
}

AudioError StateMachine::stopEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::stopEvent mCurrentState " << mCurrentState << Logging::endl;
    return mStateTable[mCurrentState]->stopEvent();
}

AudioError StateMachine::abortEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::abortEvent mCurrentState " << mCurrentState << Logging::endl;
    return mStateTable[mCurrentState]->abortEvent();
}

void StateMachine::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::workerEvent event " << event << " current state " << mCurrentState << Logging::endl;
    mStateTable[mCurrentState]->workerEvent(event);
}

void StateMachine::changeState(AlsaStreamState newState)
{
    if (newState == mCurrentState)
        return;

    sLock();

    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::changeState current state -> new state : " << mCurrentState <<  " -> " << newState << Logging::endl;
    mCurrentState = newState;
    pthread_cond_signal(&mCond); /* Event Trigger */

    sUnLock();
}

AlsaStreamState StateMachine::getState()
{
    return  mCurrentState;
};

#define MICRO_SECONDS 1000000         /* Seconds representing in Micro seconds */
#define MILLI_SECONDS    1000         /* Seconds representing in Milli seconds */

static uint32_t ts2ms(struct timespec &ts)
{
    return ((ts.tv_sec * MILLI_SECONDS) + (ts.tv_nsec / MICRO_SECONDS));
}

static void ms2ts(uint32_t ms, struct timespec &ts)
{
    ts.tv_sec  = ms / MILLI_SECONDS;
    ts.tv_nsec = (ms % 1000) * MICRO_SECONDS;
}

AudioError StateMachine::waitTillTargetState(AlsaStreamState targetState, const uint32_t timeInMs)
{
    struct timespec   ts;
    uint32_t timeout = 0;
    int32_t rc = -1;
    AudioError ret = AudioError::OK;

    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::waitTillTargetState Curr state " << mCurrentState << ", wait for state " << targetState << Logging::endl;

    sLock();

    while (mCurrentState != targetState)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);

        timeout = ts2ms(ts) + timeInMs;
        ms2ts(timeout,ts);

        rc = pthread_cond_timedwait(&mCond, &mMtx, &ts);
        if (rc != 0)
        {
            Logging(mLoggingHandle, LL_ERROR) << "StateMachine::waitTillTargetState timed out, err " << rc << " and current state is " << mCurrentState << Logging::endl;
            ret = AudioError::FAILURE;
            break;
        }
    }

    sUnLock();

    return ret;
}

AudioError StateMachine::changeStateAndWaitTillStateChange(AlsaStreamState newState, const uint32_t timeInMs)
{
    struct timespec   ts;
    uint32_t timeout = 0;
    int32_t rc = -1;
    AudioError ret = AudioError::OK;

    Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::changeStateAndWaitTillStateChange Curr state " << mCurrentState << ", wait till state change from " << newState << Logging::endl;

    sLock();

    mCurrentState = newState;
    pthread_cond_signal(&mCond); /* Event Trigger */

    while (mCurrentState == newState)
    {
        clock_gettime(CLOCK_MONOTONIC, &ts);

        timeout = ts2ms(ts) + timeInMs;
        ms2ts(timeout,ts);

        rc = pthread_cond_timedwait(&mCond, &mMtx, &ts);
        if (rc != 0)
        {
            Logging(mLoggingHandle, LL_ERROR) << "StateMachine::changeStateAndWaitTillStateChange timed out, err " << rc << " and current state is " << mCurrentState << Logging::endl;
            ret = AudioError::FAILURE;
            break;
        }
    }

    sUnLock();

    return ret;
}

void StateMachine::waitTillStateChange(const AlsaStreamState waitState)
{
    sLock();

    /* Wait in case wait state is same as current state */
    if (mCurrentState == waitState)
    {
        /*wait for any command to evaluate next round.*/
        Logging(mLoggingHandle, LL_DEBUG) << "StateMachine::waitTillStateChange go to WAIT mode, Curr state is " << mCurrentState << Logging::endl;

        pthread_cond_wait(&mCond, &mMtx);
    }

    sUnLock();

}

void StateMachine::sLock()
{
    _assert(pthread_mutex_lock(&mMtx) == 0);
}

void StateMachine::sUnLock()
{
    _assert(pthread_mutex_unlock(&mMtx) == 0);
}

/* State: CLOSED */
AudioError AlsaStreamStateClosed::openEvent(StreamInfo &data)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateClosed::openEvent Enter" << Logging::endl;
    AudioError ret = mBeAlsaImpl->doOpenStream(data);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateClosed::openEvent failed with err, ret " <<  ret << Logging::endl;
        return ret;
    }

    return mState.waitTillTargetState(ALSA_SSTATE_STOP);
}

AudioError AlsaStreamStateClosed::closeEvent()
{
    return AudioError::OK;
}

void AlsaStreamStateClosed::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateClosed::workerEvent event " << event << Logging::endl;
    if (event == WORKER_EVENT_STOP)
    {
        mState.changeState(ALSA_SSTATE_STOP);
    }
}

/* State: STOP */

AudioError AlsaStreamStateStop::startEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateStop::startEvent Enter" << Logging::endl;
    AudioError ret = mBeAlsaImpl->startEventHandler();
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateStop::startEvent failed with err, ret " << ret << Logging::endl;
        return ret;
    }

    mState.changeState(ALSA_SSTATE_RUN_FADE_IN);
    return ret;
}

AudioError AlsaStreamStateStop::closeEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateStop::closeEvent Enter" << Logging::endl;

    AudioError ret = mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_CLOSING);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateStop::closeEvent failed with err, ret " << ret << Logging::endl;
        return ret;
    }

    return mBeAlsaImpl->closeEventHandler();
}

AudioError AlsaStreamStateStop::stopEvent()
{
    return AudioError::OK;
}

AudioError AlsaStreamStateStop::abortEvent()
{
    return AudioError::OK;
}

/* State: FADE IN */

AudioError AlsaStreamStateFadeIn::startEvent()
{
    return AudioError::OK;
}

AudioError AlsaStreamStateFadeIn::stopEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateFadeIn::stopEvent Enter" << Logging::endl;

    AudioError ret = mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_RUN_FADE_OUT, mBeAlsaImpl->calcFadeTimeOut());
    if (ret == AudioError::OK)
    {
        ret = mState.waitTillTargetState(ALSA_SSTATE_STOP);
    }

    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateFadeIn::stopEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

AudioError AlsaStreamStateFadeIn::abortEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateFadeIn::abortEvent Enter" << Logging::endl;

    AudioError ret = mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_ABORTING);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateFadeIn::abortEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

void AlsaStreamStateFadeIn::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateFadeIn::workerEvent event " << event << Logging::endl;
    if (event == WORKER_EVENT_ABORTING)
    {
        mState.changeState(ALSA_SSTATE_ABORTING);
    }
    else if (event == WORKER_EVENT_RUNNING)
    {
        mState.changeState(ALSA_SSTATE_RUNNING);
    }
    else if (event == WORKER_EVENT_FADE_OUT)
    {
        mState.changeState(ALSA_SSTATE_RUN_FADE_OUT);
    }
}

/* State: RUN */

AudioError AlsaStreamStateRun::startEvent()
{
    return AudioError::OK;
}

AudioError AlsaStreamStateRun::stopEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateRun::stopEvent Enter" << Logging::endl;

    AudioError ret =  mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_RUN_FADE_OUT, mBeAlsaImpl->calcFadeTimeOut());
    if (ret == AudioError::OK)
    {
        ret = mState.waitTillTargetState(ALSA_SSTATE_STOP);
    }

    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateRun::stopEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

AudioError AlsaStreamStateRun::abortEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) <<  "AlsaStreamStateRun::abortEvent Enter" << Logging::endl;

    AudioError ret = mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_ABORTING);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateRun::abortEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

void AlsaStreamStateRun::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateRun::workerEvent event " << event << Logging::endl;
    if (event == WORKER_EVENT_ABORTING)
    {
        mState.changeState(ALSA_SSTATE_ABORTING);
    }
    else if (event == WORKER_EVENT_FADE_OUT)
    {
        mState.changeState(ALSA_SSTATE_RUN_FADE_OUT);
    }
}

/* State: FADE OUT */

AudioError AlsaStreamStateFadeOut::abortEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateFadeOut::abortEvent Enter" << Logging::endl;

    AudioError ret = mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_ABORTING);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateFadeOut::abortEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

AudioError AlsaStreamStateFadeOut::stopEvent()
{
    /* This case can happen,If Process Callback returns STOP but still stop event is called during fade out */
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateFadeOut::stopEvent Enter" << Logging::endl;

    AudioError ret = mState.waitTillTargetState(ALSA_SSTATE_STOP, mBeAlsaImpl->calcFadeTimeOut());
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateFadeOut::stopEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

void AlsaStreamStateFadeOut::workerEvent(WorkerEvent event)
{
    if (event == WORKER_EVENT_STOPPING)
    {
        mState.changeState(ALSA_SSTATE_STOPPING);
    }
    else if (event == WORKER_EVENT_ABORTING)
    {
        mState.changeState(ALSA_SSTATE_ABORTING);
    }
}

/*State: Closing */

AudioError AlsaStreamStateClosing::openEvent(StreamInfo& data)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateClosing::openEvent Enter" << Logging::endl;
    /* Open event during closing ,will be waiting till state changes to closed and gives open event command */
    mState.waitTillTargetState(ALSA_SSTATE_CLOSED);
    return mState.openEvent(data);
}

AudioError AlsaStreamStateClosing::closeEvent()
{
    return mState.waitTillTargetState(ALSA_SSTATE_CLOSED);
}

void AlsaStreamStateClosing::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateClosing::workerEvent event " << event << Logging::endl;
    if (event == WORKER_EVENT_CLOSE)
    {
        mState.changeState(ALSA_SSTATE_CLOSED);
    }
}

/*State: Aborting */
AudioError AlsaStreamStateAborting::stopEvent()
{
    return mState.waitTillTargetState(ALSA_SSTATE_STOP);
}

AudioError AlsaStreamStateAborting::abortEvent()
{
    return mState.waitTillTargetState(ALSA_SSTATE_STOP);
}

void AlsaStreamStateAborting::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateAborting::workerEvent event " << event << Logging::endl;
    if (event == WORKER_EVENT_ABORT)
    {
        mBeAlsaImpl->abortEventHandler();
        mState.changeState(ALSA_SSTATE_STOP);
    }
}

/*State: Stopping */
AudioError AlsaStreamStateStopping::stopEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateStopping::stopEvent Enter " << Logging::endl;
    AudioError ret =  mState.waitTillTargetState(ALSA_SSTATE_STOP);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateStopping::stopEvent failed with err, ret =  " << ret << Logging::endl;
    }

    return ret;
}

AudioError AlsaStreamStateStopping::abortEvent()
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateStopping::abortEvent Enter " << Logging::endl;
    AudioError ret = mState.changeStateAndWaitTillStateChange(ALSA_SSTATE_ABORTING);
    if (ret != AudioError::OK)
    {
        Logging(mLoggingHandle, LL_ERROR) << "AlsaStreamStateStopping::abortEvent failed with err, ret " << ret << Logging::endl;
    }

    return ret;
}

void AlsaStreamStateStopping::workerEvent(WorkerEvent event)
{
    Logging(mLoggingHandle, LL_DEBUG) << "AlsaStreamStateStopping::workerEvent event " << event << Logging::endl;
    if (event == WORKER_EVENT_STOP)
    {
        mBeAlsaImpl->stopEventHandler();
        mState.changeState(ALSA_SSTATE_STOP);
    }
    else if (event == WORKER_EVENT_ABORT)
    {
        mBeAlsaImpl->abortEventHandler();
        mState.changeState(ALSA_SSTATE_STOP);
    }
}

