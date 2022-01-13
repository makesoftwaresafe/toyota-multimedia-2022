/******************************************************************************
 * @file: CAmActionContainer.cpp
 *
 * This file contains the definition of Action container class (member functions
 * and data members) used to implement the action parent classes.
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

#include "IAmAction.h"
#include "CAmActionContainer.h"
#include "CAmLogger.h"

namespace am {
namespace gc {

CAmActionContainer::CAmActionContainer(const std::string& name) :
                                mName(name),
                                mStatus(AS_NOT_STARTED),
                                mIndex(0),
                                mError(0),
                                mParent(NULL),
                                mUndoRequired(false),
                                mTimeout(INFINITE_TIMEOUT),
                                mExecutionTime(0),
                                mUndoTime(0)
{
    mStartTime.tv_nsec = mStartTime.tv_sec = 0;
}

CAmActionContainer::~CAmActionContainer(void)
{
}

void CAmActionContainer::setError(const int error)
{
    mError = error;
}

int CAmActionContainer::getError(void) const
{
    return mError;
}

std::string CAmActionContainer::getName(void) const
{
    return mName;
}

int CAmActionContainer::setParent(IAmActionCommand* pParentAction)
{
    if (mParent != NULL)
    {
        unregisterObserver((IAmEventObserver*)mParent);
        mParent = NULL;
    }
    mParent = pParentAction;
    registerObserver((IAmEventObserver*)pParentAction);
    return 0;
}

int CAmActionContainer::append(IAmActionCommand* command)
{
    if (command != NULL)
    {
        LOG_FN_ENTRY(__FILENAME__,__func__,mName, "appending", command->getName());
        mListChildActions.push_back(command);
        command->setParent(this);
    }
    return 0;
}

int CAmActionContainer::insert(IAmActionCommand* command)
{
    if (isEmpty())
    {
        append(command);
    }
    else
    {
        /*
         * Get Current executing child action
         * if none are executing add at the begining
         */
        std::vector<IAmActionCommand* >::iterator itListChildActions;
        itListChildActions = mListChildActions.begin();
        for (; itListChildActions != mListChildActions.end(); ++itListChildActions)
        {
            if ((*itListChildActions)->getStatus() == AS_NOT_STARTED)
            {
                break;
            }
        }
        mListChildActions.insert(itListChildActions, command);
        command->setParent(this);
    }
    return 0;
}

ActionState_e CAmActionContainer::getStatus(void) const
{
    return mStatus;
}

void CAmActionContainer::setStatus(const ActionState_e state)
{
    mStatus = state;
}

bool CAmActionContainer::setParam(const std::string& paramName, IAmActionParam* pParam)
{
    bool returnvalue = false;
    std::map<std::string, IAmActionParam* >::iterator itMapActionParams;
    itMapActionParams = mMapParameters.find(paramName);
    if (itMapActionParams != mMapParameters.end())
    {
        itMapActionParams->second->clone(pParam);
        returnvalue = true;
    }
    return returnvalue;
}
IAmActionParam* CAmActionContainer::getParam(const std::string& paramName)
{
    IAmActionParam* pActionParam = NULL;
    std::map<std::string, IAmActionParam* >::iterator itMapActionParameters;
    itMapActionParameters = mMapParameters.find(paramName);
    if (itMapActionParameters != mMapParameters.end())
    {
        pActionParam = itMapActionParameters->second;
    }
    return pActionParam;
}

void CAmActionContainer::_registerParam(const std::string& paramName, IAmActionParam* pParam)
{
    mMapParameters[paramName] = pParam;
}

int CAmActionContainer::execute(void)
{
    int index;
    LOG_FN_ENTRY(__FILENAME__,__func__,mName);
    int32_t elapsedTimeinms;
    /**
     * first call self execute
     */
    if (AS_NOT_STARTED == getStatus())
    {
        /*
         * Before executing get the timestamp of the start of execution
         * note if down as
         */
        clock_gettime(CLOCK_MONOTONIC, &mStartTime);
        setError(_execute());
        mExecutionTime = _calculateTimeDifference(mStartTime);
        setStatus(AS_EXECUTING);
        if (getError() != 0)
        {
            update(getError());
        }
    }
    if (AS_EXECUTING == getStatus())
    {
        index = _getIndex();
        while (index < _getNumChildActions())
        {
            index = _getIndex();
            /*
             * calculate the time in milli-second
             */
            elapsedTimeinms = getExecutionTime();
            if (getTimeout() != INFINITE_TIMEOUT)
            {
                if (elapsedTimeinms > getTimeout())
                {
                    /*
                     * declare a timeout
                     */
                    setError(1);
                    setStatus(AS_COMPLETED);
                }
                else
                {
                    mListChildActions[mIndex]->setTimeout(getTimeout() - elapsedTimeinms);
                }
            }
            mListChildActions[index]->execute();
            if (mListChildActions[index]->getStatus() == AS_EXECUTING)
            {
                break;
            }
            if (getStatus() != AS_EXECUTING)
            {
                break;
            }
        }
        if ((index >= _getNumChildActions()) && (getStatus() == AS_EXECUTING))
        {
            update(getError());
        }
    }
    else if (AS_COMPLETED == getStatus())
    {
        update(getError());
    }
    LOG_FN_EXIT(__FILENAME__,__func__,mName, getStatus());
    return 0;
}

int CAmActionContainer::undo(void)
{
    int error;
    ActionState_e state = getStatus();
    LOG_FN_ENTRY(__FILENAME__,__func__,mName);
    if ((state == AS_ERROR_STOPPED) || (state == AS_COMPLETED && getError() == 0))
    {
        if (getUndoRequired() == true)
        {
            clock_gettime(CLOCK_MONOTONIC, &mStartTime);
            setError(_undo());
            mUndoTime = _calculateTimeDifference(mStartTime);
        }
        if (mListChildActions.size() > 0)
        {
            setStatus(AS_UNDOING);
        }
        else
        {
            setStatus(AS_UNDO_COMPLETE);
        }
    }
    state = getStatus();
    if (state == AS_UNDOING)
    {
        if (_getIndex() >= mListChildActions.size())
        {
            _setIndex(mListChildActions.size() - 1);
        }
        while (_getIndex() >= 0)
        {
            state = mListChildActions[_getIndex()]->getStatus();
            error = mListChildActions[_getIndex()]->getError();
            if ((state == AS_ERROR_STOPPED) || (state == AS_COMPLETED && error == 0)
                || (state == AS_UNDOING))
            {
                mListChildActions[_getIndex()]->undo();
                state = mListChildActions[_getIndex()]->getStatus();
                if ((state != AS_UNDO_COMPLETE))
                {
                    break;
                }
            }
            _decrementIndex();
        }
        if (_getIndex() < 0)
        {
            update(getError());
        }
    }
    else if (state == AS_UNDO_COMPLETE)
    {
        update(getError());
    }
    LOG_FN_EXIT(__FILENAME__,__func__,mName, mStatus);
    return 0;
}

int CAmActionContainer::update(const int result)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,"update called for ", mName, "result=", result, getStatus());

    setError(result);
    _update(result, _getIndex());
    ActionState_e state = getStatus();
    if (state == AS_EXECUTING)
    {
        if (getError() > 0)
        {
            setStatus(AS_ERROR_STOPPED);
            _update(getError());
            notify(getError());
        }
        else
        {
            if (_incrementIndex() >= _getNumChildActions())
            {
                setStatus(AS_COMPLETED);
                _update(getError());
                notify(getError());
            }
        }
    }
    else if (state == AS_UNDOING)
    {
        if (_getIndex() < 0)
        {
            setStatus(AS_UNDO_COMPLETE);
            _update(getError());
            notify(getError());
        }
    }
    LOG_FN_EXIT(__FILENAME__,__func__,mName);
    return 0;
}

int CAmActionContainer::cleanup(void)
{
    LOG_FN_ENTRY(__FILENAME__,__func__,mName);
    std::vector<IAmActionCommand* >::iterator itListChildActions;
    ActionState_e state;
    /**
     * If the current state of the action is undo complete then change the state of
     * all the child actions which are not started to complete.
     */
    for (itListChildActions = mListChildActions.begin();
                    itListChildActions != mListChildActions.end();)
    {
        state = (*itListChildActions)->getStatus();
        if ((state == AS_UNDO_COMPLETE) || (state == AS_COMPLETED)
            || ((state == AS_NOT_STARTED) && (getStatus() == AS_UNDO_COMPLETE)))
        {
            (*itListChildActions)->cleanup();
            delete (*itListChildActions);
            itListChildActions = mListChildActions.erase(itListChildActions);
        }
        else
        {
            break;
        }
    }
    if (itListChildActions == mListChildActions.end())
    {
        _cleanup();
    }
    _setIndex(0);
    mStartTime.tv_nsec = 0;
    mStartTime.tv_sec = 0;
    mExecutionTime = 0;
    mUndoTime = 0;
    LOG_FN_EXIT(__FILENAME__,__func__,mName);
    return 0;
}

int CAmActionContainer::_execute(void)
{
    return 0;
}

int CAmActionContainer::_undo(void)
{
    return 0;
}

int CAmActionContainer::_update(const int result)
{
    return 0;
}
int CAmActionContainer::_update(const int result, const int index)
{
    return 0;
}

int CAmActionContainer::_cleanup(void)
{
    return 0;
}

bool CAmActionContainer::isEmpty(void)
{
    return mListChildActions.empty();
}

bool CAmActionContainer::getUndoRequired(void)
{
    return mUndoRequired;
}
void CAmActionContainer::setUndoRequried(const bool undoRequired)
{
    mUndoRequired = undoRequired;
}

int CAmActionContainer::_incrementIndex()
{
    ++mIndex;
    return mIndex;
}

int CAmActionContainer::_decrementIndex()
{
    --mIndex;
    return mIndex;
}

int CAmActionContainer::_getIndex()
{
    return mIndex;
}
void CAmActionContainer::_setIndex(int index)
{
    mIndex = index;
}
int CAmActionContainer::_getNumChildActions(void)
{
    return mListChildActions.size();
}

void CAmActionContainer::setTimeout(uint32_t timeout)
{
    mTimeout = timeout;
}

uint32_t CAmActionContainer::getTimeout(void)
{
    return mTimeout;
}

uint32_t CAmActionContainer::getExecutionTime(void)
{
    uint32_t totalChildExecutionTime = 0;
    std::vector<IAmActionCommand* >::iterator itListChildActions;
    for (itListChildActions = mListChildActions.begin();
                    itListChildActions != mListChildActions.end(); ++itListChildActions)
    {
        totalChildExecutionTime += (*itListChildActions)->getExecutionTime();
    }
    return mExecutionTime + totalChildExecutionTime;
}

uint32_t CAmActionContainer::getUndoTime(void)
{
    uint32_t totalChildUndoTime = 0;
    std::vector<IAmActionCommand* >::iterator itListChildActions;
    for (itListChildActions = mListChildActions.begin();
                    itListChildActions != mListChildActions.end(); ++itListChildActions)
    {
        totalChildUndoTime += (*itListChildActions)->getExecutionTime();
    }
    return mUndoTime + totalChildUndoTime;
}

uint32_t CAmActionContainer::_calculateTimeDifference(timespec startTime)
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
