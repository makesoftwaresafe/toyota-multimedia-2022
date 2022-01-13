/*
 *  File: iAP2TimeImplementation.c
 *  Package: iAP2UtilityImplementation
 *  Abstract: n/a
 *
 *  Created and implemented by ADIT
 *
 */

#include "iAP2TimeImplementation.h"
#include <iAP2Link.h>
#include <iAP2LinkRunLoop.h>
#include <iAP2Log.h>
#include "iap2_init.h"
#include "iap2_init_private.h"

/*
****************************************************************
**
** Functions implemented by ADIT
**
****************************************************************
*/


/*
****************************************************************
**
**  _iAP2TimeCleanupCallback
**
**  Input:
**      timer:      pointer to timer to cleanup
**
**  Output:
**      None
**
**  Return:
**      None
**
****************************************************************
*/
void _iAP2TimeCleanupCallback (iAP2Timer_t* timer)
{
    /* ADIT: no need to implement anything, since _iAP2TimeCancelCallback
     * cancels timer and sets callback to NULL */
#if DEBUG
    iAP2LogDbg("%s:%d _iAP2TimeCleanupCallback was called \n",
            __FILE__, __LINE__);
#endif

    timer = timer;
}


/*
****************************************************************
**
**  _iAP2TimeCallbackAfter
**
**  Input:
**      timer:      timer
**      delayMs:    amount of time to wait before callback is called
**      callback:   function to callback on timeout
**
**  Output:
**      None
**
**  Return:
**      BOOL    return TRUE if successful in registering callback, else FALSE
**
**  Note:   This must be implemented by user of the library.
**          Call must return without blocking...
**          timer should be processed asynchronously.
**          Call to callback must occur either on the same thread
**          as the one processing link layer state machine.
**
****************************************************************
*/
BOOL _iAP2TimeCallbackAfter (iAP2Timer_t* timer,
                             uint32_t     delayMs,
                             iAP2TimeCB_t callback)
{
    BOOL rc = TRUE;
    struct itimerspec new_value;
    iAP2Link_t* link = (iAP2Link_t*)timer->link;
    iAP2LinkRunLoop_t* runLoop = (iAP2LinkRunLoop_t*)link->context;
    iAP2Device_st* device = (iAP2Device_st*)runLoop->context;

    new_value.it_value.tv_sec = (delayMs/1000);
    new_value.it_value.tv_nsec = 1000000 * (delayMs%1000);
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;

    if(timerfd_settime(device->iAP2RunLoop.iAP2RunLoopTimerFd,
                       0, &new_value, NULL) == -1)
    {
        rc = FALSE;
    }
    else
    {
        device->iAP2RunLoop.iAP2RunLoopTimerCallback = (void*)callback;
        device->iAP2RunLoop.iAP2RunLoopTimer = (void*)timer;
    }
#if DEBUG
    iAP2LogDbg("%s:%d _iAP2TimeCallbackAfter was called (delayMs %d.%d) \n",
            __FILE__, __LINE__,
            new_value.it_value.tv_sec, new_value.it_value.tv_nsec);
#endif

    return rc;
}


/*
****************************************************************
**
**  _iAP2TimePerformCallback
**
**  Input:
**      timer:      timer
**      callback:   function to callback on timeout
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:   This must be implemented by user of the library.
**          Call must return without blocking...
**          callback should be processed asynchronously.
**          Call to callback must occur either on the same thread
**          as the one processing link layer state machine.
**
****************************************************************
*/
/*
 * ADITG:  Currently not used inside LinkLayer.
 *         Therefore, this function is not implemented.
 */
void _iAP2TimePerformCallback (iAP2Timer_t* timer,
                               iAP2TimeCB_t callback)
{
    /* ADIT: to be discussed and implemented */
#if DEBUG
    iAP2LogDbg("%s:%d _iAP2TimePerformCallback was called \n",
            __FILE__, __LINE__);
#endif

    timer = timer;
    callback = callback;
}


/*
****************************************************************
**
**  _iAP2TimeCancelCallback
**
**  Input:
**      timer:      timer
**
**  Output:
**      None
**
**  Return:
**      None
**
**  Note:   This must be implemented by user of the library.
**
****************************************************************
*/
void _iAP2TimeCancelCallback (iAP2Timer_t* timer)
{
    struct itimerspec new_value;
    iAP2Link_t* link = (iAP2Link_t*)timer->link;
    iAP2LinkRunLoop_t* runLoop = (iAP2LinkRunLoop_t*)link->context;
    iAP2Device_st* device = (iAP2Device_st*)runLoop->context;

    /* set all timer values to 0 to disarm timer */
    new_value.it_value.tv_sec = 0;
    new_value.it_value.tv_nsec = 0;
    new_value.it_interval.tv_sec = 0;
    new_value.it_interval.tv_nsec = 0;

    (void)timerfd_settime(device->iAP2RunLoop.iAP2RunLoopTimerFd,
                          0, &new_value, NULL);
    device->iAP2RunLoop.iAP2RunLoopTimerCallback = NULL;
#if DEBUG
    iAP2LogDbg("%s:%d _iAP2TimeCancelCallback was called \n",
            __FILE__, __LINE__);
#endif
}

