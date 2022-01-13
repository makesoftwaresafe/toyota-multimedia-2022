/******************************************************************************
 * @file: CAmTimerEvent.cpp
 *
 * This file contains the definition of timer event class (member functions
 * and data members) used to implement the logic of handling the timeout of actions
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

#include "CAmTimerEvent.h"
#include "CAmLogger.h"
#include "CAmControlSend.h"

namespace am {
namespace gc {

CAmTimerEvent* CAmTimerEvent::mpTimerInstance = NULL;
CAmTimerEvent::CAmTimerEvent() :
                                mpCAmSocketHandler(0),
                                mpTimerCallback(this, &CAmTimerEvent::timerCallback),
                                mpControlSend(NULL)
{
}

CAmTimerEvent* CAmTimerEvent::getInstance(void)
{
    if (mpTimerInstance == NULL)
    {
        mpTimerInstance = new CAmTimerEvent;
    }
    return mpTimerInstance;
}

void CAmTimerEvent::setSocketHandle(CAmSocketHandler* psocketHandler, CAmControlSend* pControlSend)
{
    mpCAmSocketHandler = psocketHandler;
    mpControlSend = pControlSend;
}

CAmTimerEvent::~CAmTimerEvent()
{
    if (mpCAmSocketHandler != NULL)
    {
        mpCAmSocketHandler = NULL;
    }
}

void CAmTimerEvent::timerCallback(const sh_timerHandle_t handle, void* userData)
{
    CAmTimerEvent* pTimerEvent = (CAmTimerEvent*)userData;
    std::list<gc_TimerClient_s* >::iterator itListClients;

    if (userData == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"NULL user data in timer callback");
        return;
    }

    // get mutex of controller
    itListClients = (pTimerEvent->mListClients).begin();
    while (itListClients != (pTimerEvent->mListClients).end())
    {
        if ((*itListClients)->handle == handle)
        {
            LOG_FN_INFO(__FILENAME__,__func__,": handle=", handle, " itListTimerClient->handle=",
                        (*itListClients)->handle);
            // notify timer event
            (*itListClients)->pClient->Call(handle, (*itListClients)->pParam);
            break;
        }
        else
        {
            itListClients++;
        }
    }
    mpControlSend->iterateActions();
}

bool CAmTimerEvent::setTimer(IAmShTimerCallBack* pClient, void* pParam, int32_t msec,
                             sh_timerHandle_t& handle)
{
    gc_TimerClient_s* pTimerClient;
    am_Error_e error;

    // MAX 10 sec
    if (msec > MAX_TIMER_VALUE)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  OUT:  out of range", msec);
        return false;
    }

    pTimerClient = new gc_TimerClient_s();
    if (pTimerClient == NULL)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  OUT:  Client=NULL");
        return false;
    }

    pTimerClient->pClient = pClient;
    pTimerClient->pParam = pParam;
    pTimerClient->handle = 0;
    (pTimerClient->timeSpecInstance).tv_sec = msec / SEC;
    (pTimerClient->timeSpecInstance).tv_nsec = (msec % SEC) * NSEC;

    error = mpCAmSocketHandler->addTimer(pTimerClient->timeSpecInstance, &mpTimerCallback, handle,
                                         (void*)this);
    if (error != E_OK)
    {
        LOG_FN_ERROR(__FILENAME__,__func__,"  OUT:  addTimer Error");
        delete pTimerClient;
        return false;
    }

    pTimerClient->handle = handle;
    mListClients.push_back(pTimerClient);

    LOG_FN_DEBUG(__FILENAME__,__func__,"  OUT handle=", handle);
    return true;
}

void CAmTimerEvent::removeTimer(sh_timerHandle_t& handle)
{
    std::list<gc_TimerClient_s* >::iterator itListClients;

    itListClients = mListClients.begin();
    while (itListClients != mListClients.end())
    {
        if (((*itListClients)->handle) == handle)
        {
            mpCAmSocketHandler->removeTimer((*itListClients)->handle);

            // delete timer object
            if (*itListClients != NULL)
            {
                delete (*itListClients);
            }
            // remove from the list
            itListClients = mListClients.erase(itListClients);
            handle = 0;
            break;
        }
        // set interval time and increment iterator
        else
        {
            itListClients++;
        }
    }

}

} /* namespace gc */
} /* namespace am */
