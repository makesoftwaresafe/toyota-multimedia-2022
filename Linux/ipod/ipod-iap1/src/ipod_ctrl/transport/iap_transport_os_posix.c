#include <adit_typedef.h>

#include "ipodcommon.h"
#include "iap_transport_message.h"
#include "iap_transport_os.h"
#include "iap1_dlt_log.h"
#include <pthread_adit.h>
#include <sys_time_adit.h>
#include <unistd.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mqueue.h>
#include <errno.h>
#include "iap_general.h"


#define IPOD_POSIX_MAX_LEN 256
#define IPOD_POSIX_STACKSIZE_STEPSIZE 16384
#define IPOD_POSIX_STACKSIZE_MAXSTEPS 8

void iPodOSSleep(U32 sleep_ms)
{
    S32 s32ReturnValue;
    struct timespec req;
    struct timespec remain;
    
    /* Initialize the structure */
    memset(&req, 0, sizeof(req));
    memset(&remain, 0, sizeof(remain));
    
    
    req.tv_sec = sleep_ms / IPOD_MSEC;
    req.tv_nsec = (sleep_ms % IPOD_MSEC) * IPOD_NSEC;
    
    while(1)
    {
        s32ReturnValue = nanosleep(&req, &remain);

        if (s32ReturnValue == 0)
        {
            break;
        }
        else
        {
            if (errno == EINTR)
            {
                req.tv_sec = remain.tv_sec ;
                req.tv_nsec = remain.tv_nsec;
            }
            else
            {
                break;
            }
        }
    }// end while

}

S32 iPodOSDeleteTask(IPOD_TASK_ID task_id)
{
    int rc = 0;
    
    rc = pthread_cancel((pthread_t)task_id);
    IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "pthread_cancel() returns : rc = %d",rc);
    if (rc == 0)
    {
        rc = pthread_join((pthread_t)task_id, NULL);
        IAP1_TRANSPORT_LOG(DLT_LOG_VERBOSE, "pthread_join() returns : rc = %d",rc);
    }
    
    return rc;
}

void iPodOSCancelPoint()
{
    pthread_testcancel();
}
void iPodOSExitTask(IPOD_TASK_ID task_id)
{
    task_id = task_id;
    pthread_exit(NULL);
}

S32 iPodOSJoinTask(IPOD_TASK_ID task_id)
{
    return pthread_join((pthread_t)task_id, NULL);
}

IPOD_TASK_ID iPodOSCreateTask(void* tsk_addr, const U8 *tsk_name, S32 prio, S32 stack_size, S32 CPU_id, void* exinf)
{
    U32 retries = 0;
    pthread_t thread = 0;
    pthread_attr_t attr;
    memset(&attr, 0, sizeof(pthread_attr_t));


    S32 rc = 0;
    IPOD_TASK_ID task_id = 0;
    CPU_id = CPU_id;
    struct sched_param param;
    
    /* For lint */
    prio = prio;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    
    if ((tsk_addr != NULL) && (tsk_name != NULL))
    {
        rc = pthread_attr_init(&attr);
        if (rc == 0)
        {
            param.sched_priority = prio;
            rc = pthread_attr_setschedparam(&attr, &param);
        }
        else
        {
            rc = IPOD_ERROR;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "pthread_attr_init() returns rc = %d",rc);
        }
        
        if (rc == 0)
        {
            rc = pthread_attr_setstacksize(&attr, stack_size);
            while((retries < IPOD_POSIX_STACKSIZE_MAXSTEPS)
                  && (rc != 0))
            {
                stack_size += IPOD_POSIX_STACKSIZE_STEPSIZE;
                retries ++;
                IAP1_TRANSPORT_LOG(DLT_LOG_DEBUG, "Attention! Configured stack size too low, stack size was increased to %d",stack_size);
                rc = pthread_attr_setstacksize(&attr, stack_size);
            }
        }
        else
        {
            rc = IPOD_ERROR;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod error rc = %d",rc);
        }
        if (rc == 0)
        {
            rc = pthread_create(&thread, &attr, (void *(*)(void *))tsk_addr, exinf);
        }
        (void)pthread_attr_destroy(&attr);
        if (rc == 0)
        {
            task_id = (IPOD_TASK_ID)thread;
            /* set thread name */
            pthread_setname_np(thread, (const char*)tsk_name);
        }
        else
        {
            rc = IPOD_ERROR;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "iPod error rc = %d",rc );
        }
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "tsk_addr = %p tsk_name = %p",tsk_addr,tsk_name );
    }

    if (rc != 0)
    {
        task_id = 0;
    }
    
    return task_id;
}

S32 iPodOSVerifyTaskID(IPOD_TASK_ID task_id)
{
    S32 rc = IPOD_ERROR;
    
    if (task_id != 0)
    {
        rc = IPOD_OK;
    }
    return rc;
}

void iPodOSClearTaskID(IPOD_TASK_ID* task_id)
{
    if(task_id != NULL)
    {
        *task_id = 0;
    }
}

S32 iPodOSCreateSemaphore(const U8 *semaphore_name, IPOD_SEM_ID *semId)
{
    S32 rc = IPOD_ERROR;

    semaphore_name = semaphore_name;

    if (NULL != semId)
    {
        *semId = malloc(sizeof(sem_t));
        if (NULL != *semId)
        {
            rc = sem_init(*semId, 1, IPOD_SEM_DEFAULT);
            if (-1 != rc)
            {
                rc = IPOD_OK;
            }
            else
            {
                free(*semId);
                *semId = NULL;
                rc = IPOD_ERROR;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error in semaphore initialization");
            }
        }
    }
    return rc;
}

S32 iPodOSDeleteSemaphore(IPOD_SEM_ID semaphore_id, const U8 *semaphore_name)
{
    S32 rc = IPOD_OK;

    semaphore_name = semaphore_name;
    if (NULL != semaphore_id)
    {
        free(semaphore_id);
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error semaphore_id is NULL");
    }
    return rc;
}

S32 iPodOSWaitSemaphore(IPOD_SEM_ID semaphore_id, S32 timeout_ms)
{
    S32 rc = IPOD_OK;
    S32 localErrno = 0;
    struct timespec rData;
    struct timeval oCur;
    struct timespec ts;
    U32 timeout = timeout_ms;
    U32 before_ms = 0;
    U32 after_ms = 0;
 
    /* Initialize the structure */
    memset(&rData, 0, sizeof(rData));
    memset(&oCur, 0, sizeof(oCur));
    memset(&ts, 0, sizeof(ts));
    
    
    while((NULL != semaphore_id)&&(IPOD_OK == rc))
    {
        /* calculate the absolut end time for the timedwait */
        if (clock_gettime(CLOCK_REALTIME, &rData) != 0) { 
            rc = IPOD_ERROR;
        }
        if (rc == IPOD_OK)
        {
            rData.tv_sec += timeout/IPOD_MSEC;
            rData.tv_nsec += (timeout%IPOD_MSEC)*IPOD_NSEC;
            if (rData.tv_nsec > 1000000000)
            { 
                rData.tv_nsec -= 1000000000; 
                rData.tv_sec += 1; 
            }
        }
        if ( clock_gettime(CLOCK_MONOTONIC, &ts))
        {
            gettimeofday(&oCur, NULL);
            before_ms = oCur.tv_sec * IPOD_MSEC + oCur.tv_usec / IPOD_MSEC;
        }
        else
        {
            before_ms = ts.tv_sec * IPOD_MSEC + ts.tv_nsec / IPOD_NSEC;
        }

        if (rc == IPOD_OK)
        {
            rc = sem_timedwait(semaphore_id, &rData);
            localErrno = errno;
        }

        /* timed wait had no error */
        if (IPOD_OK == rc)
        {
            break; 
        }
        else
        {
            /* timedwait was interrupted */
            if (localErrno == EINTR)
            {
                if ( clock_gettime(CLOCK_MONOTONIC, &ts))
                {
                    gettimeofday(&oCur, NULL);
                    after_ms = oCur.tv_sec * IPOD_MSEC + oCur.tv_usec / IPOD_MSEC;
                }
                else
                {
                    after_ms = ts.tv_sec * IPOD_MSEC + ts.tv_nsec / IPOD_NSEC;
                }
                if ((after_ms - before_ms) >= timeout)
                    /* time has finished anyways */
                {
                    break;
                }
                else
                {
                    timeout = after_ms - before_ms;
                }
            }
            else
            {
                /* other error, break the loop */ 
                rc = IPOD_ERROR;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - timed wait has error other than EINTR");
                break;
            }
        }
    }//end while

    return rc;
}

S32 iPodOSSignalSemaphore(IPOD_SEM_ID semaphore_id)
{
    S32 rc = 0;

    if (NULL != semaphore_id)
    {
        rc = sem_post(semaphore_id);
    }
    else
    {
        rc = IPOD_ERROR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error  semaphore_id is NULL");
    }

    return rc;
}

/* This function was introduced because of a type mismatch between T-Kernel and POSIX semaphores */
S32 iPodOSTestSemaphore(IPOD_SEM_ID semaphore_id)
{
    S32 rc = IPOD_ERROR;

    if (NULL != semaphore_id)
    {
        rc = IPOD_OK;
    }
    return rc;
}

S32 iPodOSCreateFlag(const U8 *flg_name, IPOD_FLG_ID *flg_id, U8 direction)
{
    mqd_t mq = 0;
    int oflag = O_RDWR | O_CREAT;// | O_EXCL;
    char name[IPOD_POSIX_MAX_LEN] = {0};
    char name2[IPOD_POSIX_MAX_LEN] = {0};
    struct mq_attr attr;
    S32 rc = 0;

    /* Initialize the structure */
    memset(&attr, 0, sizeof(attr));
    
    if ( (flg_name != NULL) && (flg_id != NULL) &&
        ((direction == IPOD_FLG_ONE_WAY) || (direction == IPOD_FLG_TWO_WAY)))
    {
        snprintf(name, IPOD_POSIX_MAX_LEN-1, "/%s", flg_name);
      
        attr.mq_flags = 0;
        attr.mq_maxmsg = IPOD_MSG_MAX_DEFULT;
        attr.mq_msgsize = sizeof(U32);

    (void)mq_unlink(name);
        mq = mq_open(name, oflag, S_IRWXU, &attr);
        if (mq != -1)
        {
            flg_id->readerSend = mq;

            if (direction == IPOD_FLG_ONE_WAY)
            {
                flg_id->readerWait = mq;
            }
            else
            {
                snprintf(name2, IPOD_POSIX_MAX_LEN-2, "%s2", name);

        (void)mq_unlink(name2);
                mq = mq_open(name2, oflag, S_IRWXU, &attr);
                if (mq != -1)
                {
                    flg_id->readerWait = mq;
                }
                else
                {
                    mq_close(flg_id->readerSend);
                    mq_unlink(name);
                    rc = IPOD_ERROR;
                    IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - mq_open returns -1");
                }
            }
        }
        else
        {
            rc = IPOD_ERROR;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - mq_open returns -1");
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error Parameter flg_name = %p flg_id = %p direction = %d",flg_name,flg_id,direction);
    }
    
    return rc;
}


S32 iPodOSDeleteFlag(IPOD_FLG_ID flg_id, const U8 *flg_name, U8 direction)
{
    S32 rc = 0;
    char name[IPOD_POSIX_MAX_LEN] = {0};
    char name2[IPOD_POSIX_MAX_LEN] = {0};
    mqd_t mq = 0;
    
    if ( (flg_name != NULL) &&
         (flg_id.readerSend >= 0) && (flg_id.readerWait >= 0) &&
        ((direction == IPOD_FLG_ONE_WAY) || (direction == IPOD_FLG_TWO_WAY)))
    {
        snprintf(name, IPOD_POSIX_MAX_LEN-1, "/%s", flg_name);

        mq = (mqd_t)flg_id.readerSend;
        rc = mq_close(mq);
        if (rc == 0)
        {
            rc = mq_unlink(name);
        }

        if (direction == IPOD_FLG_TWO_WAY)
        {
            snprintf(name2, IPOD_POSIX_MAX_LEN-2, "%s2", name);
            
            mq = (mqd_t)flg_id.readerWait;
            rc = mq_close(mq);
            if (rc == 0)
            {
                rc = mq_unlink(name2);
            }
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error Parameter flg_name = %p flg_id.readerSend = %d flg_id.readerWait = %d direction = %d",
                          flg_name,flg_id.readerSend,flg_id.readerWait,direction);
    }

    return rc;
}

S32 iPodOSSetFlag(IPOD_FLG_ID flg_id, U32 bit_mask, U8 direction)
{
    mqd_t mq = 0;
    U8 msg_ptr[4] = {0};
    S32 size = 0;
    U8 msg_prio = 0;
    S32 rc = 0;
    struct timespec tv;
    
    /* Initialize the structure */
    memset(&tv, 0, sizeof(tv));
    
    if ((flg_id.readerSend >= 0) && (flg_id.readerWait >= 0))
    {
        size = sizeof(msg_ptr);
        memcpy((void*)msg_ptr, (void*)&bit_mask, size);
        
        if (direction == IPOD_FLG_TWO_WAY)
        {
            if (bit_mask == IPOD_READ_HANDSHAKE_READ)
            {
                mq = (mqd_t)flg_id.readerSend;
            }
            else
            {
                mq = (mqd_t)flg_id.readerWait;
            } 
        }
        else
        {
            mq = (mqd_t)flg_id.readerSend;
        }
        
        rc = mq_timedsend(mq, (VP)msg_ptr, size, msg_prio, &tv);
    }
    else
    {
        rc = IPOD_BAD_PARAMETER;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter flg_id.readerSend = %d flg_id.readerWait = %d",flg_id.readerSend,flg_id.readerWait);
    }
    return rc;
}

S32 iPodOSWaitFlag(IPOD_FLG_ID flg_id, U32 bit_mask, U32 options, U32 *flg, S32 timeout_ms, U8 direction)
{
    mqd_t mq = 0;
    S32 size = 0;
    U32 prio = 60;
    S32 rc = 0;
    struct timeval tv;
    struct timespec ts;
    U32 times = (U32)timeout_ms;

    /* LINT and Compiler warning */
    options = options;

    /* Initialize the structure */
    memset(&tv, 0, sizeof(tv));
    memset(&tv, 0, sizeof(ts));
    if (flg != NULL)
    {
        rc = gettimeofday(&tv, NULL);

        if ((flg_id.readerSend < 0) || (flg_id.readerWait < 0))
        {
            rc = IPOD_BAD_PARAMETER;
            IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Bad Parameter flg_id.readerSend = %d flg_id.readerWait = %d",flg_id.readerSend,flg_id.readerWait);
        }
    }
    else
    {
        rc = IPOD_ERR_PAR;
        IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "flg is NULL");
    }

    if (rc == 0)
    {
        tv.tv_sec += times / IPOD_MSEC;
        tv.tv_usec += (times % IPOD_MSEC) * IPOD_MSEC;
        
        /* Check over 1000000 nsec. means 1 sec and it sould add to tv_sec 1. */
        if (tv.tv_usec >= IPOD_TIME_NSEC_1000000)
        {
            tv.tv_sec += 1;
            tv.tv_usec -= IPOD_TIME_NSEC_1000000;
        }
        
        TIMEVAL_TO_TIMESPEC(&tv, &ts);
        size = sizeof(U32);
        
        if (direction == IPOD_FLG_TWO_WAY)
        {
            if (bit_mask == IPOD_READ_HANDSHAKE_READ)
            {
                mq = (mqd_t)flg_id.readerSend;
            }
            else
            {
                mq = (mqd_t)flg_id.readerWait;
            }
        }
        else
        {
            mq = (mqd_t)flg_id.readerSend;
        }
        
        rc = mq_timedreceive(mq, (char*)flg, size, &prio, &ts);
        if (rc == size)
        {
            rc = 0;
        }
        else if (rc == -1)
        {
            rc = errno;
            if (rc == ETIMEDOUT)
            {
                rc = IPOD_ERR_TMOUT;
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "Error - Timeout");
            }
            else
            {
                IAP1_TRANSPORT_LOG(DLT_LOG_ERROR, "mq_timedreceive sets errno other than ETIMEDOUT, errno = %d",rc);
                rc = IPOD_ERROR;
            }
        }
    }
    
    return rc;
}

/* Currently don't do something */
S32 iPodOSGetTsp(IPOD_TASK_ID tskId, void* pk_tskspc)
{
    S32 rc = 0;
    tskId = tskId;
    pk_tskspc = pk_tskspc;
    
    return rc;
}

/* Currently don't do something */
S32 iPodOSSetTsp(IPOD_TASK_ID tskId, void* pk_tskspc)
{
    S32 rc = 0;
    tskId = tskId;
    pk_tskspc = pk_tskspc;

    return rc;
}

/* Currently don't do something */
S32 iPodOSGetRid(IPOD_TASK_ID tskId)
{
    S32 rc = 1;
    tskId = tskId;

    return rc;
}

/* Currently don't do something */
S32 iPodOSSetRid(IPOD_TASK_ID tskId, S32 resid)
{
    S32 rc = 0;
    tskId = tskId;
    resid = resid;
    
    return rc;
}
