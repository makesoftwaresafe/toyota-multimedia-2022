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


#ifndef ROUTINGADAPTERTHREAD_H_
#define ROUTINGADAPTERTHREAD_H_

#include <string>
#include <pthread.h>
#include <sched.h>

#define PTHREAD_NAME_LEN    15


namespace am
{

class CAmRoutingAdapterThread
{
public:
    struct __cpu_sched
    {
        int policy;
        int priority;

        __cpu_sched() : policy(SCHED_OTHER), priority(0) {};
        __cpu_sched(int pol, int prio) : policy(pol), priority(prio) {};
    };
private:

    typedef enum __state
        {
            STATE_JOINED   = 0x01,
            STATE_FORKING  = 0x02,
            STATE_RUNNING  = 0x04,
            STATE_STOPPING = 0x08,
            STATE_STOPPED  = 0x10,
            STATE_JOINING  = 0x20
        } eState;
    pthread_t               mId;
    std::string             mName;
    __cpu_sched             mCpuSched;
    eState                  mState;
    int                     mThreadErr;

    pthread_mutex_t         mMtx;
    pthread_cond_t          mCond;

public:
    CAmRoutingAdapterThread();
    virtual ~CAmRoutingAdapterThread();

    /**
     * @brief This function sets the thread name.
     * @param[in] name name of thread
     */
    void setThreadName(std::string name);

    /**
     * @brief This function sets the scheduling mode.
     * @param[in] name name of thread
     */
    void setThreadSched(__cpu_sched & cpuSched);
    void setThreadSched(int policy, int priority);

    /**
     * @brief This function has to be called to start the thread initially.
     * Following sequence will be executed afterwards:
     *   if (0 == @see initThread())
     *      while (@see workerThread() == 0)
     *   @see deinitThread()
     *
     * @return 0 on success or <0 on failure
     */
    int startThread();

    /**
     * @brief This function stops the working thread. Another startThread() will continue the working.
     * @return 0 on success or <0 on failure
     */
    int stopThread();

    /**
     * @brief This function stops and joins the execution thread.
     * @return result of initThread() or workerThread()
     */
    int joinThread();

protected:
    /**
     * @brief This function will be called on startThread() of thread and can be used to allocated worker specific resources.
     * In case the resource allocation fails the function shall return != 0 which will directly call deinitThread().
     * @return 0 on success, other on error which will lead to end of thread execution
     */
    virtual int initThread() = 0;

     /**
      * @brief This function will be called after initThread() was successfully started.
      * This is the worker function and will be called periodically until:
      * a) joinThread() was called or
      * b) the function returns != 0.
      * @return 0 on success, other on error which will lead to end of thread execution
      */
    virtual int workerThread() = 0;

    /**
     * @brief This function will be called in case joinThread() was called or in case of failure in initThread() or workerThread().
     * It can be used to cleanup resources which were created within initThread().
     * @param[in] errInit return value of initThread() or workerThread()
     */
    virtual void deinitThread(int errInit) = 0;

private:
    int startWorkerThread();
    static void* _WorkerThread(void* arg) {
        static_cast<CAmRoutingAdapterThread*>(arg)->WorkerThread();
        return NULL;
    };
    void WorkerThread();

    void setState(const eState state);
    eState getState();
    bool isStateForked();

    void waitForStateChange(const eState state);
    void setStateAndWaitForStateChange(const eState state);
};

} /* namespace am */

#endif /* ROUTINGADAPTERTHREAD_H_ */
