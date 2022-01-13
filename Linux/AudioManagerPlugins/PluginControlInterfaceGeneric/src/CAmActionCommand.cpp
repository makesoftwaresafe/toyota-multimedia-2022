/******************************************************************************
 * @file: CAmActionCommand.cpp
 *
 * This file contains the definition of Action command class (member functions
 * and data members) used to implement the leaf action class.
 * Only the leaf actions can perform asynchronous operations.
 *
 * @component: AudioManager Generic Controller
 *
 * @author: Toshiaki Isogai <tisogai@jp.adit-jv.com>
 *          Kapildev Patel  <kpatel@jp.adit-jv.com>
 *          Prashant Jain   <pjain@jp.adit-jv.com>
 *
 * @copyright (c) 2015 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 *****************************************************************************/

#include "CAmActionCommand.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmActionCommand::CAmActionCommand(const std::string& name) :
                                mName(name),
                                mStatus(AS_NOT_STARTED),
                                mError(0),
                                mParent(NULL),
                                mUndoRequired(false),
                                mTimeout(INFINITE_TIMEOUT),
                                mTimerHandle(0),
                                mpTimerCallback(this, &CAmActionCommand::timeout),
                                mExecutionTime(0),
                                mUndoTime(0)
{
    mStartTime.tv_nsec = 0;
    mStartTime.tv_sec = 0;
}

CAmActionCommand::~CAmActionCommand(void)
{
}

std::string CAmActionCommand::getName(void) const
{
    return mName;
}

void CAmActionCommand::setError(const int Error)
{
    mError = Error;
}

int CAmActionCommand::getError(void) const
{
    return mError;
}

void CAmActionCommand::setStatus(const ActionState_e state)
{
    mStatus = state;
}

ActionState_e CAmActionCommand::getStatus(void) const
{
    return mStatus;
}

int CAmActionCommand::setParent(IAmActionCommand* command)
{
    if (mParent != NULL)
    {
        unregisterObserver((IAmEventObserver*)command);
        mParent = NULL;
    }
    mParent = command;
    registerObserver((IAmEventObserver*)command);
    return 0;
}

void CAmActionCommand::_registerParam(const std::string& paramname, IAmActionParam* pParam)
{
    mMapParameters[paramname] = pParam;
}

bool CAmActionCommand::setParam(const std::string& paramName, IAmActionParam* pParam)
{
    bool returnValue = false;
    std::map<std::string, IAmActionParam* >::iterator itMapActionParameters;
    itMapActionParameters = mMapParameters.find(paramName);
    if (itMapActionParameters != mMapParameters.end())
    {
        itMapActionParameters->second->clone(pParam);
        returnValue = true;
    }
    return returnValue;
}

IAmActionParam* CAmActionCommand::getParam(const std::string& paramName)
{
    IAmActionParam* pActionParam = NULL;
    std::map<std::string, IAmActionParam* >::iterator itMapActionParameters;
    itMapActionParameters = mMapParameters.find(paramName);
    if (itMapActionParameters == mMapParameters.end())
    {
        pActionParam = itMapActionParameters->second;
    }
    return pActionParam;
}

int CAmActionCommand::execute(void)
{
    /*
     * _execute should get called only for the AS_NOT_STARTED state for all
     * the other state do nothing.
     */
    LOG_FN_ENTRY (__FILENAME__,__func__,getName());if (AS_NOT_STARTED == getStatus())
    {
        /**
         * Start the timer befor calling the execute
         */
        CAmTimerEvent* pTimer = CAmTimerEvent::getInstance();
        if (getTimeout() != INFINITE_TIMEOUT)
        {
            pTimer->setTimer(&mpTimerCallback, this, getTimeout(), mTimerHandle);
        }
        clock_gettime(CLOCK_MONOTONIC, &mStartTime);
        setError(_execute());
        if (getError() >= 0)
        {
            setStatus(AS_COMPLETED);
            mExecutionTime = _calculateTimeDifference(mStartTime);
            if (mTimerHandle)
            {
                pTimer->removeTimer(mTimerHandle);
            }
            notify(getError());
        }
        else
        {
            setStatus(AS_EXECUTING);
        }
    }
    return 0;
}

void CAmActionCommand::timeout(void* data)
{
    CAmActionCommand* pSelf = (CAmActionCommand*)data;
    if (pSelf == this)
    {
        /*
         * call the concrete action _timeout if implemented
         */
        _timeout();
        this->update(E_ABORTED);
    }
}
int CAmActionCommand::undo(void)
{
    /*
     * Undo could be performed only for completed actions.
     */
    CAmTimerEvent* pTimer = CAmTimerEvent::getInstance();
    if (AS_COMPLETED == getStatus())
    {
        if (getUndoRequired() == true)
        {
            clock_gettime(CLOCK_MONOTONIC, &mStartTime);
            pTimer->setTimer(&mpTimerCallback, this, DEFAULT_UNDO_TIMEOUT, mTimerHandle);
            setError(_undo());
        }
        if (getError() >= 0)
        {
            setStatus(AS_UNDO_COMPLETE);
                mUndoTime = _calculateTimeDifference(mStartTime);
                pTimer->removeTimer(mTimerHandle);
                notify(getError());
        }
        else
        {
            setStatus(AS_UNDOING);
        }
    }
    return 0;
}

int CAmActionCommand::update(const int result)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,"update called for ", mName, "result=", result, getStatus());
    setError(result);
    if (mTimerHandle)
    {
        CAmTimerEvent* pTimer = CAmTimerEvent::getInstance();
        pTimer->removeTimer(mTimerHandle);
    }
    if (AS_EXECUTING == getStatus())
    {
        setStatus (AS_COMPLETED);
        mExecutionTime = _calculateTimeDifference(mStartTime);
    }
    else
    {
        setStatus (AS_UNDO_COMPLETE);
        mUndoTime = _calculateTimeDifference(mStartTime);
    }
    _update (getError());notify
    (getError());
    return 0;
}

int CAmActionCommand::cleanup(void)
{
    _cleanup();
    mStartTime.tv_nsec = 0;
    mStartTime.tv_sec = 0;
    mExecutionTime = 0;
    mUndoTime = 0;
    return 0;
}

int CAmActionCommand::_execute(void)
{
    return 0;
}

int CAmActionCommand::_undo(void)
{
    return 0;
}

int CAmActionCommand::_update(const int result)
{
    (void)result;
    return 0;
}

int CAmActionCommand::_cleanup(void)
{
    return 0;
}
bool CAmActionCommand::getUndoRequired(void)
{
    return mUndoRequired;
}

void CAmActionCommand::setUndoRequried(const bool undoRequired)
{
    mUndoRequired = undoRequired;
}

void CAmActionCommand::setTimeout(uint32_t timeout)
{
    mTimeout = timeout;
}

uint32_t CAmActionCommand::getTimeout(void)
{
    return mTimeout;
}

void CAmActionCommand::_timeout(void)
{
    return;
}

uint32_t CAmActionCommand::getExecutionTime(void)
{
    return mExecutionTime;
}

uint32_t CAmActionCommand::getUndoTime(void)
{
    return mUndoTime;
}

uint32_t CAmActionCommand::_calculateTimeDifference(timespec startTime)
{
    timespec delta;
    int32_t elapsedTimeinms = 0;
    timespec endTime;
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    delta.tv_nsec = delta.tv_sec = 0;
    delta.tv_sec = endTime.tv_sec - startTime.tv_sec;
    if (startTime.tv_nsec < startTime.tv_nsec)
    {
        delta.tv_nsec = endTime.tv_nsec + MAX_NS - startTime.tv_nsec;
        delta.tv_sec--;
    }
    else
    {
        delta.tv_nsec = endTime.tv_nsec - startTime.tv_nsec;
    }
    elapsedTimeinms = (delta.tv_sec * 1000) + (delta.tv_nsec / 1000000);
    return elapsedTimeinms;
}

} /* namespace gc */
} /* namespace am */
