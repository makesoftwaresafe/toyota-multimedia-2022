/******************************************************************************
 * @file: CAmTimerEvent.h
 *
 * This file contains the declaration of timer event class (member functions
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

#ifndef GC_TIMEREVENT_H_
#define GC_TIMEREVENT_H_

#include <time.h>
#include <unistd.h>
#include "CAmTypes.h"
#include "CAmEventSubject.h"
#include "CAmSocketHandler.h"


namespace am {
namespace gc {
class CAmControlSend;
template <class TClass> class Tcallback : public IAmShTimerCallBack
{
private:
    TClass* mInstance;
    void (TClass::*mFunction)(void* userData);

public:
    Tcallback(TClass* instance, void (TClass::*function)(void* userData)) :
                                    mInstance(instance), //
                                    mFunction(function)
    {
    }
    ;
    void Call(sh_timerHandle_t handle, void* userData)
    {
        (void)handle;
        (*mInstance.*mFunction)(userData);
    }
};

class CAmTimerEvent
{
public:
    enum
    {
        MAX_UINT_VALUE = 2147483647
    };
    enum
    {
        USER_TIMER = 0
    };
    enum
    {
        NSEC = 1000 * 1000
    };            // nsec = 1 (1,000 * 1,000) msec
    enum
    {
        SEC = 1000
    };                    // sec = 1,000 msec
    enum
    {
        MAX_TIMER_VALUE = 10 * 1000
    };   // MAX 10 sec

    virtual ~CAmTimerEvent();
    static CAmTimerEvent* getInstance();

    bool setTimer(IAmShTimerCallBack* pClient, void* pParam, int32_t msec,
                  sh_timerHandle_t& handle);
    void removeTimer(sh_timerHandle_t& handle);
    void setSocketHandle(CAmSocketHandler* psocketHandler, CAmControlSend* pControlSend);
protected:
    struct gc_TimerClient_s
    {
        IAmShTimerCallBack* pClient;
        void* pParam;
        sh_timerHandle_t handle;
        timespec timeSpecInstance;
    };
private:
    CAmTimerEvent();
    void timerCallback(sh_timerHandle_t handle, void* userData);

    static CAmTimerEvent* mpTimerInstance;
    CAmSocketHandler* mpCAmSocketHandler;
    TAmShTimerCallBack<CAmTimerEvent > mpTimerCallback;
    std::list<gc_TimerClient_s* > mListClients;
    CAmControlSend* mpControlSend;
};

} /* namespace gc */
} /* namespace am */
#endif /* GC_TIMEREVENT_H_ */
