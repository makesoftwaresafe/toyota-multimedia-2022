/**
* \file: libipodplayer_server.c
*
*
***************************************************************************** */


/* ignore GCC warning
 *  warning: variable 'buf' might be clobbered by 'longjmp' or 'vfork' [-Werror=clobbered]
 * */
#pragma GCC diagnostic ignored "-Wclobbered"

#include "iPodPlayerIPCLocal.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerUtilityLog.h"

static IPOD_PLAYER_PARAM_IPC_INFO g_ipcInfo;
static S32 iPodPlayerIPCDoRecv(U32 recvSocket, U32 size, U8* message, S32 flag);

/* Free the memory. It is set by pthread_cleanup_push */
void iPodPlayerIPCFree(U8 *buf)
{
    if(buf == NULL)
    {
        return;
    }
    
    free(buf);
    
    return;
}

/* Open semaphore */
S32 iPodPlayerIPCDoSemOpen(sem_t **semId, U8 *name, U8 *namedSem)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    /* Parameter check */
    if((semId == NULL) || (namedSem == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, semId, namedSem);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Sempahore open by name */
    if(name != NULL)
    {
        sem_unlink((const char *)name);
        
        *semId = sem_open((const char *)name, O_CREAT, S_IRWXU, 1);
        if(*semId != SEM_FAILED)
        {
            *namedSem = 1;
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    /* Semaphore open */
    else
    {
        *semId = calloc(1, sizeof(**semId));
        if(*semId != NULL)
        {
            rc = sem_init(*semId, 0, 1);
            if(rc == 0)
            {
                *namedSem = 0;
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                free(*semId);
                *semId = NULL;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    return rc;
}

/* Close semaphore */
S32 iPodPlayerIPCDoSemClose(sem_t **semId, U8 namedSem)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    /* Parameter check */
    if(semId == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, semId);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Close named semaphore */
    if(namedSem != 0)
    {
        rc = sem_close(*semId);
        if(rc == 0)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    /* Close semaphore */
    else
    {
        if(*semId != NULL)
        {
            rc = sem_destroy(*semId);
            if(rc == 0)
            {
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno); 
                rc = IPOD_PLAYER_IPC_ERROR;
            }
            free(*semId);
            *semId = NULL;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    return rc;
    
}

/* Release semaphore */
S32 iPodPlayerIPCDoSemPost(sem_t *semId)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    /* Parameter check */
    if(semId == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, semId);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    rc = sem_post(semId);
    if(rc == 0)
    {
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    return rc;
    
}

/* Wait semaphore */
S32 iPodPlayerIPCDoSemWait(sem_t *semId, U32 timeout_ms)
{
    S32 rc = IPOD_PLAYER_IPC_OK;
    S32 rcp = 0;
    S32 localErrno = 0;
    struct timespec ts;
    U32 timeout = timeout_ms;
    U32 before_ms = 0;
    U32 after_ms = 0;
    
    if(semId == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, semId);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Initialize the structure */
    memset(&ts, 0, sizeof(ts));
    
    rcp = clock_gettime(CLOCK_MONOTONIC, &ts);
    if(rcp == 0)
    {
        before_ms = ts.tv_sec * IPOD_PLAYER_IPC_MSEC + ts.tv_nsec / IPOD_PLAYER_IPC_NSEC;
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        timeout = 0;
        IPOD_DLT_ERROR("Could not get clock time using clock_gettime (error=%d)", errno);
        rc = IPOD_PLAYER_IPC_ERROR;
    }

    do
    {
        rcp = sem_trywait(semId);
        if(rcp == 0)
        {
            timeout = 0;
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            localErrno = errno;
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            if((localErrno == EINTR) || (localErrno == EAGAIN))
            {
                rcp = clock_gettime(CLOCK_MONOTONIC, &ts);
                if(rcp == 0)
                {
                    after_ms = ts.tv_sec * IPOD_PLAYER_IPC_MSEC + ts.tv_nsec / IPOD_PLAYER_IPC_NSEC;
                    rc = IPOD_PLAYER_IPC_OK;
                }
                else
                {
                    timeout = 0;
                    rc = IPOD_PLAYER_IPC_ERROR;
                    IPOD_DLT_ERROR("Could not get clock time using clock_gettime (error=%d)", errno);
                }
                
                if(rc == IPOD_PLAYER_IPC_OK)
                {
                    if((after_ms - before_ms) < timeout)
                    {
                        usleep(50 * 1000);
                    }
                    else
                    {
                        /* time has finished anyways */
                        timeout = 0;
                        rc = IPOD_PLAYER_IPC_ERROR;
                        IPOD_DLT_ERROR("Could not get clock time using clock_gettime (error=%d)", errno);
                    }
                }
            }
            else
            {
                /* other error, break the loop */ 
                timeout = 0;
                rc = IPOD_PLAYER_IPC_ERROR;
                IPOD_DLT_ERROR("other error, break the loop (error=%d)", errno);
            }
        }
    } while(timeout > 0);
    
    return rc;
}

S32 iPodPlayerIPCInfoInit(void)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 i = 0;
    U32 j = 0;
    U8 namedSem = 0;
    
    /* Initialize the global variable */
    memset(&g_ipcInfo, 0, sizeof(g_ipcInfo));
    
    rc = iPodPlayerIPCDoSemOpen(&g_ipcInfo.semId, NULL, &namedSem);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
        {
            g_ipcInfo.info[i].id = -1;
            g_ipcInfo.info[i].pid = -1;
            
            for(j = 0; j < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; j++)
            {
                g_ipcInfo.info[i].cid[j] = -1;
            }
        }
    }
    
    return rc;
}

S32 iPodPlayerIPCInfoDeinit(void)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 i = 0;
    
    /* Semaphore ID check */
    if(g_ipcInfo.semId == NULL)
    {
        /* It means that the init was not done */
        return IPOD_PLAYER_IPC_OK;
    }
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        if(g_ipcInfo.info[i].isReady != 0)
        {
            /* Opened ipc is stil existing */
            rc = IPOD_PLAYER_IPC_ERROR;
            break;
        }
        rc = IPOD_PLAYER_IPC_OK;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Push for semaphore */
        pthread_cleanup_push((void *)iPodPlayerIPCDoSemPost, g_ipcInfo.semId);
        /* Check whether other thead is using semaphore */
        rc = iPodPlayerIPCDoSemWait(g_ipcInfo.semId, IPOD_PLAYER_IPC_DATA_SEM_WAIT);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            iPodPlayerIPCDoSemPost(g_ipcInfo.semId);
        }
        
        /* Close the semaphore */
        rc = iPodPlayerIPCDoSemClose(&g_ipcInfo.semId, 0);
        /* Pop because semaphore was unlocked */
        pthread_cleanup_pop(0);
        
        /* Clear the global variable */
        memset(&g_ipcInfo, 0, sizeof(g_ipcInfo));
    }
    
    return rc;
}

IPOD_PLAYER_PARAM_DESC_INFO *iPodPlayerIPCRefIPCInfo(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U32 i = 0;
    
    /* Parameter check */
    if(g_ipcInfo.semId == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, g_ipcInfo.semId);
        return NULL;
    }
    
    /* Lock to use the global variable */
    rc = iPodPlayerIPCDoSemWait(g_ipcInfo.semId, IPOD_PLAYER_IPC_DATA_SEM_WAIT);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Loop until maximum list number */
        for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
        {
            /* Get the exising ipc info */
            if(handle != -1)
            {
                /* Current list has IPC information and handle of current list equals to handle */
                if((g_ipcInfo.info[i].isReady != 0) && (g_ipcInfo.info[i].id == handle))
                {
                    /* Set the addrres to inform target information */
                    info = &g_ipcInfo.info[i];
                    break;
                }
            }
            /* Get the empty ipc info */
            else
            {
                if(g_ipcInfo.info[i].id == handle)
                {
                    /* Set the addrees to inform empty info */
                    info = &g_ipcInfo.info[i];
                    break;
                }
            }
        }
        
        if(info == NULL)
        {
            /* Semaphore should be unlocked in case of error */
            iPodPlayerIPCDoSemPost(g_ipcInfo.semId);
        }
    }
    
    return info;
}

IPOD_PLAYER_PARAM_DESC_INFO * iPodPlayerIPCRefAllIPCInfo(S32 *num)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    /* Parameter check */
    if(num == NULL)
    {
        return NULL;
    }
    /* Lock to use the global variable */
    rc = iPodPlayerIPCDoSemWait(g_ipcInfo.semId, IPOD_PLAYER_IPC_DATA_SEM_WAIT);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        info = g_ipcInfo.info;
        *num = IPOD_PLAYER_IPC_MAX_EPOLL_NUM;
    }
    
    return info;
}

S32 iPodPlayerIPCUnrefIPCInfo(IPOD_PLAYER_PARAM_DESC_INFO *info)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    /* Parameter check */
    if((info == NULL) || (g_ipcInfo.semId == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, info, g_ipcInfo.semId);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Unlock */
    iPodPlayerIPCDoSemPost(g_ipcInfo.semId);
    
    return rc;
}

/* This function must be called after calling the iPodPlayerIPCRefIPCInfo or the iPodPlayerIPCRefAllIPCInfo */
void iPodPlayerIPCClearIPCInfo(IPOD_PLAYER_PARAM_DESC_INFO *info)
{
    U32 i = 0;
    
    /* Parameter check */
    if(info == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, info);
        return;
    }
    
    memset(info, 0, sizeof(*info));
    info->id = -1;
    info->pid = -1;
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        info->cid[i] = -1;
    }
    
    return;
}

/* This function must be called after calling the iPodPlayerIPCRefIPCInfo or the iPodPlayerIPCRefAllIPCInfo */
S32 iPodPlayerIPCGetIPCfds(S32 num, S32 *fds, S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 i = 0;
    U32 count = 0;
    
    /* Parameter check */
    if((num < IPOD_PLAYER_IPC_MAX_EPOLL_NUM) || (fds == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        /* There is ipc information in this table */
        if(g_ipcInfo.info[i].isReady != 0)
        {
            /* Get all ipc information */
            if(handle == -1)
            {
                if(g_ipcInfo.info[i].id != -1)
                {
                    fds[count] = g_ipcInfo.info[i].id;
                    count++;
                }
            }
            /* Get the ipc information that parent id is handle */
            else
            {
                if(g_ipcInfo.info[i].pid == handle)
                {
                    fds[count] = g_ipcInfo.info[i].id;
                    count++;
                }
            }
        }
    }
    
    rc = count;
    
    return rc;
}

void iPodPlayerIPCSort(S32 *handle, U32 handleNum)
{
    U32 i = 0;
    U32 j = 0;
    S32 tempHandle = 0;
    
    if((handle == NULL) || (handleNum < 1))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, handleNum);
        return ;
    }
    
    /* Loop until maximum queuing size */
    for(i = 0; i < handleNum - 1; i++)
    {
        /* Loop until maximum queuing size - 1 */
        for(j = handleNum - 1; j > i; j--)
        {
            /* Queuing status is still running */
            if(handle[j] > handle[j - 1])
            {
                tempHandle = handle[j - 1];
                /* Change the queue */
                handle[j - 1] = handle[j];
                handle[j] = tempHandle;
            }
        }
    }
}

/* Close all child(accepted) id */
void iPodPlayerIPCCloseSocketChild(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 i = 0;
    S32 infoCount = 0;
    S32 errID = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    IPOD_PLAYER_PARAM_DESC_INFO *parentInfo = NULL;
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Lock and get ipc info */
    info = iPodPlayerIPCRefAllIPCInfo(&infoCount);
    if(info != NULL)
    {
        /* Search the parent ipc info */
        for(i = 0; i < (U32)infoCount; i++)
        {
            if((info[i].isReady != 0) && (info[i].id == handle))
            {
                /* Set parent info */
                parentInfo = &info[i];
                break;
            }
        }
        
        /* Found out the parent info */
        if(parentInfo != NULL)
        {
            /* Close childs */
            for(i = 0; i < (U32)infoCount; i++)
            {
                if((info[i].isReady != 0) && (info[i].pid == parentInfo->id))
                {
                    rc = close(info[i].id);
                    if(rc == -1)
                    {
                        errID = errno;
                    }
                    iPodPlayerIPCClearIPCInfo(&info[i]);
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERR_NOINFO;
        }
        
        /* Unlock  */
        iPodPlayerIPCUnrefIPCInfo(info);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle);
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, rc, errID);
    
    return;
}

/* Open epoll */
S32 iPodPlayerIPCOpenEpoll(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    S32 fd = -1;
    
    /* Parameter check */
    if(handle == NULL)
    {
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    openInfo = openInfo;
    
    /* Create epoll */
    fd = epoll_create(IPOD_PLAYER_IPC_MAX_EPOLL_NUM);
    if(fd != -1)
    {
        pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
        /* Check the existence of this handle in the ipc info */
        info = iPodPlayerIPCRefIPCInfo(fd);
        if(info == NULL)
        {
            /* Lock and get the empty infomation */
            info = iPodPlayerIPCRefIPCInfo(-1);
            if(info != NULL)
            {
                /* Register epoll information */
                info->isReady = 1;
                info->id = fd;
                info->pid = -1;
                info->type = IPOD_PLAYER_OPEN_WAIT_HANDLE;
                *handle = info->id;
                /* Unlock the ipc information */
                iPodPlayerIPCUnrefIPCInfo(info);
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, fd);
                rc = IPOD_PLAYER_IPC_ERR_NOINFO;
            }
        }
        else
        {
            iPodPlayerIPCUnrefIPCInfo(info);
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERR_EXIST);
            rc = IPOD_PLAYER_IPC_ERR_EXIST;
        }
        pthread_cleanup_pop(0);
        
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            close(fd);
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Return the handle id or error */
    return rc;
}

/* Close epoll */
S32 iPodPlayerIPCCloseEpoll(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        /* Close epoll */
        close(info->id);
        /* Clear ipc info */
        iPodPlayerIPCClearIPCInfo(info);
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    pthread_cleanup_pop(0);
    return rc;
}

S32 iPodPlayerIPCModEpoll(S32 waitHandle, U32 handleNum, S32 *handle, U32 flag)
{
    S32 rc = IPOD_PLAYER_IPC_OK;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    S32 fds[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {-1, };
    U32 i = 0;
    U32 j = 0;
    U32 maxNum = 0;
    struct epoll_event epollEv;
    U32 event = 0;
    
    /* Parameter check */
    if(handle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Poll wait for receive */
    if((flag & IPOD_PLAYER_WAIT_IN) == IPOD_PLAYER_WAIT_IN)
    {
        event |= EPOLLIN;
    }
    
    /* Poll wait for send */
    if((flag & IPOD_PLAYER_WAIT_OUT) == IPOD_PLAYER_WAIT_OUT)
    {
        event |= EPOLLOUT;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Get wait handle information */
    info = iPodPlayerIPCRefIPCInfo(waitHandle);
    if(info != NULL)
    {
        /* Get all ipc information */
        maxNum = iPodPlayerIPCGetIPCfds(IPOD_PLAYER_IPC_MAX_EPOLL_NUM, fds, -1);
        /* Remove all ipc from epoll instance */
        for(i = 0; i < maxNum; i++)
        {
            epoll_ctl(info->id, EPOLL_CTL_DEL, fds[i], NULL);
        }
        
        /* Add the target ipc to epoll instance */
        memset(&epollEv, 0, sizeof(epollEv));
        for(i = 0; i < handleNum; i++)
        {
            /* Add the handle[i] to epoll instance */
            epollEv.events = event;
            epollEv.data.fd = handle[i];
            epoll_ctl(info->id, EPOLL_CTL_ADD, handle[i], &epollEv);
            
            /* Get ipc information that parent is handle[i] */
            maxNum = iPodPlayerIPCGetIPCfds(IPOD_PLAYER_IPC_MAX_EPOLL_NUM, fds, handle[i]);
            /* Add the child ipc to epoll instance */
            for(j = 0; j < maxNum; j++)
            {
                epollEv.events = event;
                epollEv.data.fd = fds[j];
                epoll_ctl(info->id, EPOLL_CTL_ADD, fds[j], &epollEv);
            }
        }
        
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, info);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    IPOD_LOG_INFO_WRITEBIN(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, (handleNum * sizeof(handleNum)), handle);
    
    return rc;
    
}

S32 iPodPlayerIPCWaitEpoll(S32 waitHandle, U32 handleNum, U32 *resNum, S32 *resHandle, S32 *resEvent, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    struct epoll_event events[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    U32 i = 0;
    U32 count = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    S32 error = 0;
    S32 fd = -1;
    
    if((resNum == NULL) || (handleNum > IPOD_PLAYER_IPC_MAX_EPOLL_NUM) || (resHandle == NULL) || (resEvent == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, resNum, resHandle, resEvent);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Initialize the structure */
    memset(&events, 0, sizeof(events));
    
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(waitHandle);
    if(info != NULL)
    {
        fd = info->id;
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, info);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        do
        {
            rc = epoll_wait(fd, events, handleNum, timeout);
            if(rc > 0)
            {
                for(i = 0; i < (U32)rc; i++)
                {
                    resHandle[i] = events[i].data.fd;
                    resEvent[i] = events[i].events;
                    *resNum = rc;
                }
                rc = IPOD_PLAYER_IPC_OK;
            }
            else if(rc == 0)
            {
                error = 0;
                rc = IPOD_PLAYER_IPC_TMO;
            }
            else
            {
                error = errno;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, error);
                pthread_testcancel();
                rc = IPOD_PLAYER_IPC_ERROR;
            }
            count++;
        } while((rc < 0) && (error == EINTR) && (count < IPOD_PLAYER_IPC_ERPOLL_RETRY_MAX));
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, rc, error, *resNum);
    
    return rc;
}

S32 iPodPlayerIPCSetAccept(S32 handle, S32 waitHandle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *acceptInfo;
    IPOD_PLAYER_PARAM_DESC_INFO *parentInfo = NULL;
    S32 fd = -1;
    S32 parentId = -1;
    U8 parentType = 0;
    U32 parentMaxSize = 0;
    U32 parentMacPacSize = 0;
    U32 i = 0;
    socklen_t     recvAddrSize = 0;
    struct  sockaddr_un recvAddr;
    
    /* Lock and get the ipc information */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    parentInfo = iPodPlayerIPCRefIPCInfo(handle);
    if(parentInfo != NULL)
    {
        /* Copy to temporary variable */
        parentId = parentInfo->id;
        parentType = parentInfo->type;
        parentMaxSize = parentInfo->maxBufSize;
        parentMacPacSize = parentInfo->maxPacketSize;
        
        /* Unlock the ipc info */
        iPodPlayerIPCUnrefIPCInfo(parentInfo);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle, waitHandle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    pthread_cleanup_pop(0);
    
    /* Check the ipc type */
    if((rc == IPOD_PLAYER_IPC_OK) && (parentType != IPOD_PLAYER_OPEN_SOCKET_SERVER) && (parentType != IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG))
    {
        /* ipc type is not server. It means that this handle does not do accept */
        rc = IPOD_PLAYER_IPC_NO_ACCEPT;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Initialize the structure */
        memset(&recvAddr, 0, sizeof(recvAddr));
        
        /* PRQA: Lint Message 64: Lint describes which the second parameter is "__SOCKADDR_ARG" 
       but according to spec of accept, the type of second parameter is "struct sockaddr *". so this cast is correct */
        fd = accept(parentId, (struct sockaddr *) &recvAddr, &recvAddrSize); /*lint !e64 */
        if(fd >= 0)
        {
            /* Lock and get ipc info */
            pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
            /* Check the existence of this handle in the ipc info */
            acceptInfo = iPodPlayerIPCRefIPCInfo(fd);
            if(acceptInfo == NULL)
            {
                /* Accept success. Get the empty ipc info */
                acceptInfo = iPodPlayerIPCRefIPCInfo(-1);
                if(acceptInfo != NULL)
                {
                    acceptInfo->isReady = 1;
                    acceptInfo->id = fd;
                    acceptInfo->pid = parentId;
                    if(parentType == IPOD_PLAYER_OPEN_SOCKET_SERVER)
                    {
                        acceptInfo->type = IPOD_PLAYER_OPEN_SOCKET_ACCEPT;
                    }
                    else
                    {
                        acceptInfo->type = IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG;
                    }
                    acceptInfo->maxBufSize = parentMaxSize;
                    acceptInfo->maxPacketSize = parentMacPacSize;
                    
                    /* Unlock the ipc info */
                    iPodPlayerIPCUnrefIPCInfo(acceptInfo);
                    rc = IPOD_PLAYER_IPC_OK;
                }
                else
                {
                    IPOD_ERR_WRITE_PTR(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, acceptInfo);
                    close(fd);
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
            }
            else
            {
                iPodPlayerIPCUnrefIPCInfo(acceptInfo);
                close(fd);
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, fd);
                rc = IPOD_PLAYER_IPC_ERR_EXIST;
            }
            pthread_cleanup_pop(0);
            
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
                /* Update the parent info for child ipc */
                parentInfo = NULL;
                parentInfo = iPodPlayerIPCRefIPCInfo(handle);
                if(parentInfo != NULL)
                {
                    for(i = 0; i < sizeof(parentInfo->cid) / sizeof(parentInfo->cid[0]); i++)
                    {
                        if(parentInfo->cid[i] == -1)
                        {
                            parentInfo->cid[i] = fd;
                            rc = IPOD_PLAYER_IPC_OK;
                            break;
                        }
                        rc = IPOD_PLAYER_IPC_ERROR;
                    }
                    /* Unlock the ipc info */
                    iPodPlayerIPCUnrefIPCInfo(parentInfo);
                    if(rc != IPOD_PLAYER_IPC_OK)
                    {
                        close(fd);
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, fd);
                    }
                }
                else
                {
                    IPOD_ERR_WRITE_PTR(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, parentInfo);
                }
                
                pthread_cleanup_pop(0);
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    return rc;
}

S32 iPodPlayerIPCClearAccept(S32 handle, S32 waitHandle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    S32 fd = -1;
    S32 parentId = -1;
    U32 i = 0;
    U8 childType = 0;
    
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    IPOD_PLAYER_PARAM_DESC_INFO *parentInfo = NULL;
    
    /* For lint */
    waitHandle = waitHandle;

    /* Lock and get the ipc information */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        fd = info->id;
        parentId = info->pid;
        childType = info->type;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Id is different from parent ID. It means that this id is opened by accept().*/
        if((fd != -1) && ((childType == IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG) || (childType == IPOD_PLAYER_OPEN_SOCKET_ACCEPT)))
        {
            /* Close accepted fd */
            rc = close(fd);
            if(rc == -1)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno, fd);
            }
            
            /* Lock and get the ipc information */
            pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
            info = iPodPlayerIPCRefIPCInfo(handle);
            if(info != NULL)
            {
                iPodPlayerIPCClearIPCInfo(info);
                /* Unlock */
                iPodPlayerIPCUnrefIPCInfo(info);
            }
            pthread_cleanup_pop(0);
            
            pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
            /* Get the parent info */
            parentInfo = iPodPlayerIPCRefIPCInfo(parentId);
            if(parentInfo != NULL)
            {
                for(i = 0; i < sizeof(parentInfo->cid) / sizeof(parentInfo->cid[0]); i++)
                {
                    if(parentInfo->cid[i] == fd)
                    {
                        parentInfo->cid[i] = -1;
                        break;
                    }
                }
                
                iPodPlayerIPCUnrefIPCInfo(parentInfo);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, parentId);
            }
            pthread_cleanup_pop(0);
            /* Anyway OK */
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            /* ID equals with parent id. It means that this is the id of server. Application has the responsibility which close the server */
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, rc, fd, parentId);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno, fd);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    return rc;
}

S32 iPodPlayerIPCEventCheck(S32 waitHandle, U32 *handleNum, S32 *handle, S32 *resEvent, U8 *retType)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 i = 0;
    U32 maxNum = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    /* Check the parameter */
    if((handleNum == NULL) || (handle == NULL) || (resEvent == NULL) || (retType == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handleNum, handle, resEvent, retType);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Save the maximum number because handleNum may be decreased*/
    maxNum = *handleNum;
    
    for(i = 0; i < maxNum; i++)
    {
        /* Event is not error */
        if(((resEvent[i] & EPOLLERR) != EPOLLERR) && ((resEvent[i] & EPOLLHUP) != EPOLLHUP))
        {
            /* Try to accept */
            rc = iPodPlayerIPCSetAccept(handle[i], waitHandle);
        }
        /* Event is error. In this case id should be cleared */
        else
        {
            /* Clear the accepted id */
            rc = iPodPlayerIPCClearAccept(handle[i], waitHandle);
        }
        /* Accept is success  */
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            handle[i] = -1;
            *handleNum = *handleNum - 1;
        }
        else if(rc == IPOD_PLAYER_IPC_NO_ACCEPT)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
    }
    
    iPodPlayerIPCSort(handle, maxNum);
    
    /* Set the type */
    for(i = 0; i < *handleNum; i++)
    {
        info = NULL;
        pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
        info = iPodPlayerIPCRefIPCInfo(handle[i]);
        if(info != NULL)
        {
            retType[i] = info->type;
            iPodPlayerIPCUnrefIPCInfo(info);
        }
        pthread_cleanup_pop(0);
    }
    
    return rc;
}

S32 iPodPlayerIPCCheckHandle(S32 waitHandle, IPOD_PLAYER_IPC_HANDLE_INFO *info)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    S32 fds[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {-1, };
    struct epoll_event epollEv;
    IPOD_PLAYER_PARAM_DESC_INFO *descInfo = NULL;
    U32 maxNum = 0;
    U32 i = 0;
    
    /* Parameter check */
    if(info == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, info);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* type is socket or accepted descriptor */
    if((info->type == IPOD_PLAYER_OPEN_SOCKET_SERVER) || (info->type == IPOD_PLAYER_OPEN_SOCKET_ACCEPT) ||
       (info->type == IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG) || (info->type == IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG))
    {
        /* Event is not error */
        if(((info->event & EPOLLERR) != EPOLLERR) && ((info->event & EPOLLHUP) != EPOLLHUP))
        {
            /* Try to accept */
            rc = iPodPlayerIPCSetAccept(info->handle, waitHandle);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                /* Push for semaphore */
                pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
                /* Get wait handle information */
                descInfo = iPodPlayerIPCRefIPCInfo(waitHandle);
                if(descInfo != NULL)
                {
                    /* Get ipc information that parent is handle[i] */
                    maxNum = iPodPlayerIPCGetIPCfds(IPOD_PLAYER_IPC_MAX_EPOLL_NUM, fds, info->handle);
                    /* Add the child ipc to epoll instance */
                    for(i = 0; i < maxNum; i++)
                    {
                        epollEv.events = info->event;
                        epollEv.data.fd = fds[i];
                        epoll_ctl(waitHandle, EPOLL_CTL_ADD, fds[i], &epollEv);
                    }
                    /* Unlock */
                    iPodPlayerIPCUnrefIPCInfo(descInfo);
                }
                else
                {
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, waitHandle, descInfo);
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
                /* Pop because semaphore was unlocked */
                pthread_cleanup_pop(0);
            }
        }
        /* Event is error. In this case id should be cleared */
        else
        {
            /* Clear the accepted id */
            rc = iPodPlayerIPCClearAccept(info->handle, waitHandle);
        }
    }
    else
    {
        /* No any opeartions */
        rc = IPOD_PLAYER_IPC_OK;
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerIPCConnectRecv(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_IPC_OK: connection success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : connection failed.</b>
 * \par DESCRIPTION
 * This function create iPodPlayerI/F socket-connection.
 */
S32 iPodPlayerIPCOpenSocketServer(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo)
{
    S32  rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    S32 fd = -1;
    S32  recvAddrSize = 0;
    struct sockaddr_un recvAddr;
    
    
    /* Paramter Check */
    if((handle == NULL) || (openInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, openInfo);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Member of paramter check */
    if(openInfo->prefix == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, openInfo->prefix);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Initialize the structure */
    memset(&recvAddr, 0, sizeof(recvAddr));
    
    /* Identify string is used */
    if(openInfo->identify != NULL)
    {
        snprintf(recvAddr.sun_path, sizeof(recvAddr.sun_path), "%s%s", openInfo->prefix, openInfo->identify);
    }
    /* No Idneitfy string */
    else
    {
        snprintf(recvAddr.sun_path, sizeof(recvAddr.sun_path), "%s", openInfo->prefix);
    }
    
    recvAddr.sun_family = AF_UNIX;
    recvAddrSize = sizeof(recvAddr);
    unlink(recvAddr.sun_path);
    /* create socket using SOCK_SEQPACKET */
    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if(fd != -1)
    {
        /* recv socket bind */
        /* PRQA: Lint Message 64: Lint describes which the second parameter is "__CONST_SOCKADDR_ARG" 
          but according to spec of bind, the type of second parameter is "const struct sockaddr *". so this cast is correct */
        rc = bind(fd, (const struct sockaddr *) &recvAddr, recvAddrSize); /*lint !e64 */
        if(rc != -1)
        {
            chmod( recvAddr.sun_path,
                   (S_IRUSR | S_IWUSR | S_IXUSR |    /* rwx */
                    S_IRGRP | S_IXGRP |              /* rwx */
                    S_IROTH | S_IWOTH | S_IXOTH) );  /* rwx */

            /* listen */
            rc = listen(fd, openInfo->connectionNum);
            if(rc != -1)
            {
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                unlink((char *)recvAddr.sun_path);
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
            /* Check the existence of this handle in the ipc info */
            info = iPodPlayerIPCRefIPCInfo(fd);
            if(info == NULL)
            {
                /* Lock and get the empty ipc info */
                info = iPodPlayerIPCRefIPCInfo(-1);
                if(info != NULL)
                {
                    U32 i = 0;
                    info->isReady = 1;
                    info->id = fd;
                    info->pid = -1;
                    info->type = openInfo->type;
                    info->maxBufSize = openInfo->maxBufSize;
                    info->maxPacketSize = openInfo->maxPacketSize;
                    snprintf((char *)info->sockFilePath, sizeof(info->sockFilePath), "%s", recvAddr.sun_path);
                    for(i = 0; i < sizeof(info->cid) / sizeof(info->cid[0]); i++)
                    {
                        info->cid[i] = -1;
                    }
                    /* Unlock */
                    *handle = info->id;
                    iPodPlayerIPCUnrefIPCInfo(info);
                    rc = IPOD_PLAYER_IPC_OK;
                }
                else
                {
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, fd);
                iPodPlayerIPCUnrefIPCInfo(info);
                rc = IPOD_PLAYER_IPC_ERR_EXIST;
            }
            pthread_cleanup_pop(0);
        }
        
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            close(fd);
            unlink(recvAddr.sun_path);
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    return rc;
}

S32 iPodPlayerIPCOpenQueueServer(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U8 tempName[IPOD_PLAYER_IPC_TEMP_MAX_LEN] = {0};
    S32 oflag = O_RDWR | O_CREAT;
    struct mq_attr attr;
    S32 fd = -1;
    sem_t *semId = NULL;
    U8 namedSem = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info;
    
    /* Paramter Check */
    if((handle == NULL) || (openInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, openInfo);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Paramter memeber check */
    if((openInfo->prefix == NULL) || (openInfo->identify == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, openInfo->prefix, openInfo->identify);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Initialize the structure */
    memset(&attr, 0, sizeof(attr));
    
    /* Set maximum connection number and maximum buffer size */
    attr.mq_maxmsg = openInfo->connectionNum;
    attr.mq_msgsize = openInfo->maxBufSize + sizeof(IPOD_PLAYER_MESSAGE_HEADER);
    
    /* Copy the name */
    rc = snprintf((char *)tempName, sizeof(tempName), "%s_%s", openInfo->prefix, openInfo->identify);
    if(rc > 0)
    {
        /* Unlinked queue. Remove name of message queue */
        (void)mq_unlink((const char *)tempName);
        rc = mq_open((const char *)tempName, oflag, S_IRWXU, &attr);
        if(rc != -1)
        {
            /* Set queue id to info table */
            fd = rc;
            /* Open the semaphore with no name */
            rc = iPodPlayerIPCDoSemOpen(&semId, NULL, &namedSem);
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
        /* Check the existence of this handle in the ipc info */
        info = iPodPlayerIPCRefIPCInfo(fd);
        if(info == NULL)
        {
            /* Get teh empty ipc info */
            info = iPodPlayerIPCRefIPCInfo(-1);
            if(info != NULL)
            {
                info->isReady = 1;
                info->id = fd;
                info->pid = -1;
                info->type = openInfo->type;
                info->semId = semId;
                info->namedSem = namedSem;
                info->maxBufSize = openInfo->maxBufSize + sizeof(IPOD_PLAYER_MESSAGE_HEADER);
                info->maxPacketSize = openInfo->maxPacketSize + sizeof(IPOD_PLAYER_MESSAGE_HEADER);
                *handle = info->id;
                iPodPlayerIPCUnrefIPCInfo(info);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, fd);
            iPodPlayerIPCUnrefIPCInfo(info);
            rc = IPOD_PLAYER_IPC_ERR_EXIST;
        }
        
        pthread_cleanup_pop(0);
    }
    
    /* Something error occurred */
    if(rc != IPOD_PLAYER_IPC_OK)
    {
        if(semId != NULL)
        {
            iPodPlayerIPCDoSemClose(&semId, namedSem);
            semId = NULL;
        }
        
        if(fd >= 0)
        {
            mq_close(fd);
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerIPCCloseConnect(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_IPC_OK: DeInitialized success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : DeInitialize failed.</b>
 * \par DESCRIPTION
 * This function close the socket-connection.
 */
S32 iPodPlayerIPCDoCloseSocketServer(S32 handle)
{
    S32   rc = IPOD_PLAYER_IPC_ERROR;
    S32 errID = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    if(handle < 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        /* Parameter invalid */
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Child ids which created by accept are also deleted */
    iPodPlayerIPCCloseSocketChild(handle);
    
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        /* close a socket */
        rc = close(info->id);
        if(rc != -1)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            errID = errno;
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        unlink((char *)info->sockFilePath);

        /* Clear IPC info */
        iPodPlayerIPCClearIPCInfo(info);
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    pthread_cleanup_pop(0);
    
    if(rc != IPOD_PLAYER_IPC_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, errID);
    }
    
    return rc;
}

S32 iPodPlayerIPCDoCloseQueueServer(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    S32 errID = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    if(handle < 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        /* Parameter invalid */
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        if(info->semId != NULL)
        {
            iPodPlayerIPCDoSemClose(&info->semId, info->namedSem);
            info->semId = NULL;
        }
        
        if(info->id >= 0)
        {
            rc = mq_close(info->id);
            if(rc != -1)
            {
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                errID = errno;
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERR_NOINFO;
        }
        
        iPodPlayerIPCClearIPCInfo(info);
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    pthread_cleanup_pop(0);
    
    if(rc != IPOD_PLAYER_IPC_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, errID);
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerIPCDoRecvSocket(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_IPC_OK: Recv success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : Recv failed.</b>
 * \par DESCRIPTION
 * This Function request to recv message from socket.
 */
S32 iPodPlayerIPCDoRecvSocket(S32 fd, U8 *message, U32 size, S32 flag, S32 timeout)
{
    S32 rc               = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_MESSAGE_HEADER    messageHeader;
    S32 totalMessageSize = 0;
    S32 totalSeqNo        = 0;
    S32 recvSize         = 0;
    
    if(message == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message);
        /* Parameter error */
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* For lint */
    flag = flag;
    timeout = timeout;
    
    /* Initialize the structure */
    memset(&messageHeader, 0, sizeof(messageHeader));
    
    do
    {
        /*** try receive socket Header ***/
        rc = iPodPlayerIPCDoRecv(fd, (U32)sizeof(messageHeader), (U8*)&messageHeader, 0);
        if(rc == sizeof(messageHeader))
        {
            /* The first data is "IPP". */
            if(strncmp((const char *)messageHeader.identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader.identify)) == 0)
            {
                /* Receive the single data or first of multiple data. */
                if(messageHeader.seqNo == 0)
                {
                    totalMessageSize = messageHeader.totalMessageSize;
                    totalSeqNo = messageHeader.totalSeqNo;
                }
                
                /* Analyze and Check the header */
                if((totalMessageSize <= (S32)size) && (totalMessageSize >= recvSize + messageHeader.messageSize) && 
                   (totalSeqNo >= messageHeader.seqNo))
                {
                    /* Receive the data */
                    rc = iPodPlayerIPCDoRecv(fd, messageHeader.messageSize, &message[recvSize], 0);
                    if(rc == messageHeader.messageSize)
                    {
                        recvSize += rc;
                        rc = IPOD_PLAYER_IPC_OK;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_IPC_ERROR;
                    }
                    
                }
                else
                {
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
            }
            else
            {
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            /* receive socket error */
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        
    } while((messageHeader.seqNo <= messageHeader.totalSeqNo) && (totalMessageSize <= (S32)size) && (recvSize < totalMessageSize) && (rc == IPOD_PLAYER_IPC_OK));
    
    if(recvSize != totalMessageSize)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, totalMessageSize, recvSize);
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = recvSize;
    }
    
    return rc;
}

static S32 iPodPlayerIPCTryReceive(S32 fd, IPOD_PLAYER_MESSAGE_HEADER * messageHeader)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U8 tempData = 0;
    
    /* Parameter check */
    if(messageHeader == NULL)
    {
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /*** try receive socket Header. Data is not deleted(MSG_PEEK) ***/
    rc = iPodPlayerIPCDoRecv(fd, sizeof(*messageHeader), (U8 *)messageHeader, MSG_PEEK);
    if(rc == sizeof(*messageHeader))
    {
        /* The first data is "IPP". */
        if(strncmp((const char *)messageHeader->identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader->identify)) == 0)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
    }
    
    if(rc > 0)
    {
        /* Receive one byte and this packet is not used */
        iPodPlayerIPCDoRecv(fd, sizeof(tempData), &tempData, 0);
        rc = IPOD_PLAYER_IPC_ERR_MORE_PACKET;
    }
    
    return rc;
}

S32 iPodPlayerIPCDoLongRecvSocket(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout)
{
    S32 rc               = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_MESSAGE_HEADER    messageHeader;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    S32 bufSize = 0;
    S32 packetSize = 0;
    U8 *buf = NULL;
    S32 seqNo            = 0;
    S32 totalMessageSize = 0;
    S32 totalSeqNo        = 0;
    S32 recvSize         = 0;
    U32 dataArray = 0;
    S32 totalRecvSize = 0;
    U32 i = 0;
    U32 dataSize = 0;
    U32 copySize = 0;
    S32 totalRequestSize = 0;
    
    /* For lint */
    flag = flag;
    timeout = timeout;
    
    /* Parameter check */
    if((message == NULL) || (size == NULL) || (asize == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message, size, asize);
        /* Parameter error */
        return IPOD_PLAYER_IPC_ERROR;
    }

    /* Initialize the structure */
    memset(&messageHeader, 0, sizeof(messageHeader));

    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        bufSize = info->maxBufSize;
        packetSize = info->maxPacketSize;
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    pthread_cleanup_pop(0);
    pthread_cleanup_push((void*)iPodPlayerIPCFree, (void *)buf);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        if((bufSize != 0) && (packetSize != 0))
        {
            buf = calloc(1, bufSize + sizeof(messageHeader));
            if(buf == NULL)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                rc = IPOD_PLAYER_IPC_ERR_NOMEM;
            }
        }
    }
    
    /* Parameter member check */
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Calicurate the total request receive size */
        totalRequestSize = 0;
        for(i = 0; i < dataNum; i ++)
        {
            totalRequestSize += size[i];
        }
        
        /* Try to receive header */
        rc = iPodPlayerIPCTryReceive(handle, &messageHeader);
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Set the total size and sequence no from header */
        totalMessageSize = messageHeader.totalMessageSize;
        totalSeqNo = messageHeader.totalSeqNo;
        seqNo = messageHeader.seqNo;
        
        if(totalMessageSize <= totalRequestSize)
        {
            /* Calicurate the received size and current position */
            dataArray = 0;
            recvSize = 0;
            totalRecvSize = 0;
            
            for(i = 0; i < dataNum; i++)
            {
                if((seqNo * bufSize) < (recvSize + (S32)size[i]))
                {
                    dataArray = i;
                    recvSize = (seqNo * bufSize) - recvSize;
                    totalRecvSize = (seqNo * bufSize);
                    rc = IPOD_PLAYER_IPC_OK;
                    break;
                }
                recvSize += (S32)size[i];
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            /* Inform the total messagesize to apllication */
            *asize = totalMessageSize;
            rc = IPOD_PLAYER_IPC_ERR_NOMEM;
        }
    }
    
    for(; (seqNo <= totalSeqNo) && (totalRecvSize < totalMessageSize) && (rc == IPOD_PLAYER_IPC_OK) && (dataArray < dataNum); seqNo++)
    {
        /* Receive the header */
        rc = iPodPlayerIPCDoRecv(handle, (U32)sizeof(messageHeader), (U8*)&messageHeader, 0);
        if(rc == sizeof(messageHeader))
        {
            /* Receive the data */
            rc = iPodPlayerIPCDoRecv(handle, messageHeader.messageSize, buf, 0);
            if(rc == messageHeader.messageSize)
            {
                /* Set the data to user buffer */
                copySize = 0;
                for(dataSize = 0; dataSize < (U32)messageHeader.messageSize; dataSize += copySize)
                {
                    if(buf != NULL)
                    {
                        /* All received data can set in current buffer */
                        if((S32)size[dataArray] - recvSize >= messageHeader.messageSize - (S32)dataSize)
                        {
                            copySize = messageHeader.messageSize - dataSize;
                            memcpy(&message[dataArray][recvSize], &buf[dataSize], copySize);
                            recvSize += copySize;
                        }
                        else
                        {
                            copySize = size[dataArray] - recvSize;
                            memcpy(&message[dataArray][recvSize], &buf[dataSize], copySize);
                            dataArray++;
                            recvSize = 0;
                        }
                        rc = IPOD_PLAYER_IPC_OK;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_IPC_ERROR;
                        break;
                    }
                }
                
                if(rc == IPOD_PLAYER_IPC_OK)
                {
                    totalRecvSize += messageHeader.messageSize;
                }
            }
        }
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Received data is over than max packet size  */
            if((packetSize > 0) && ((dataSize / (U32)packetSize) > 0) && (seqNo <= totalSeqNo))
            {
                /* Return to application even if data is stil remaining */
                rc = IPOD_PLAYER_IPC_ERR_MORE_PACKET;
            }
        }
        else if(rc > 0)
        {
            rc = IPOD_PLAYER_IPC_ERR_MORE_PACKET;
        }
    }
    
    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
    pthread_cleanup_pop(0);
    
    *asize = totalMessageSize;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_IPCLIB, totalRequestSize);
    
    return rc;
}


S32 iPodPlayerIPCDoGetNextSocketMsgSize(S32 fd, U32 *size, U32 flag, S32 timeout)
{
    S32 rc               = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_MESSAGE_HEADER    messageHeader;
    S32 totalMessageSize = 0;
    
    if(size == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, size);
        /* Parameter error */
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* For lint */
    flag = flag;
    timeout = timeout;
    
    /* Initialize the structure */
    memset(&messageHeader, 0, sizeof(messageHeader));
    
    /*** try receive socket Header ***/
    rc = iPodPlayerIPCDoRecv(fd, (U32)sizeof(messageHeader), (U8*)&messageHeader, flag);
    if(rc == sizeof(messageHeader))
    {
        /* The first data is "IPP". */
        if(strncmp((const char *)messageHeader.identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader.identify)) == 0)
        {
            /* Receive the single data or first of multiple data. */
            if(messageHeader.seqNo == 0)
            {
                totalMessageSize = messageHeader.totalMessageSize;
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        else
        {
            rc =  IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        /* receive socket error */
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        *size = totalMessageSize;
    }
    
    return rc;
}

S32 iPodPlayerIPCDoRecvQueue(S32 handle, U8 *message, U32 size, S32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_MESSAGE_HEADER messageHeader;
    U32 prior = 0;
    S32 errorID = 0;
    S32 totalMessageSize = 0;
    S32 totalSeqNo = 0;
    sem_t *semId = NULL;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U32 bufSize = 0;
    U32 recvSize = 0;
    U8 *buf = NULL;
    
    struct timespec waitTime;
    
    /* Check Parameter */
    if(message == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* For lint */
    timeout = timeout;
    flag = flag;
    
    /* Initialize the structure */
    memset(&waitTime, 0, sizeof(waitTime));
    memset(&messageHeader, 0, sizeof(messageHeader));

    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        bufSize = info->maxBufSize;
        semId = info->semId;
        if(semId != NULL)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERR_NOMEM;
        }
        iPodPlayerIPCUnrefIPCInfo((IPOD_PLAYER_PARAM_DESC_INFO *)info);
    }
    pthread_cleanup_pop(0);
    
    if(rc != IPOD_PLAYER_IPC_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, rc);
    }
    
    pthread_cleanup_push((void *)iPodPlayerIPCFree, (void *)buf);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        buf = calloc(1, bufSize + sizeof(messageHeader));
        if(buf == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    /* PRQA: Lint Message 578: Lint describes that Declaration of symbol __clframe hides symbol __clframe.
    However there is no problem because __clframe was used as mcro in pthread_cleanup_push(). */
    pthread_cleanup_push((void *)iPodPlayerIPCDoSemPost, (void *)semId);       /*lint !e578 */
    if(rc == IPOD_PLAYER_IPC_OK)
    {
      rc = iPodPlayerIPCDoSemWait((sem_t *)semId, IPOD_PLAYER_IPC_SEMWAIT_TMO);
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        do
        {
            /* Receive the data from queue */
            rc = mq_receive(handle, (char *)buf, bufSize + sizeof(messageHeader), &prior);
            if((rc != -1) && (buf != NULL))
            {
                memcpy(&messageHeader, (U8 *)buf, sizeof(messageHeader));
                /* The first data is "IPP". */
                if(strncmp((const char *)messageHeader.identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader.identify)) == 0)
                {
                    /* Receive the single data or first of multiple data. */
                    if(messageHeader.seqNo == 0)
                    {
                        totalMessageSize = messageHeader.totalMessageSize;
                        totalSeqNo = messageHeader.totalSeqNo;
                    }
                    
                    /* Analyze and Check the header */
                    if((totalMessageSize <= (S32)size) && (totalMessageSize >= (S32)recvSize + messageHeader.messageSize) && 
                       (totalSeqNo >= messageHeader.seqNo))
                    {
                        /* PRQA: Lint Message 826: This conversion is safe because the size of message, recvSize and remaining size is chekced before. So this memcpy is not problem */
                        memcpy(&message[recvSize], (U8 *)&buf[sizeof(messageHeader)], messageHeader.messageSize); /*lint !e826 */
                        recvSize += messageHeader.messageSize;
                        rc = IPOD_PLAYER_IPC_OK;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_IPC_ERROR;
                    }
                }
            }
            else
            {
                errorID = errno;
                if((errorID == EAGAIN) || (errorID == EINTR))
                {
                    waitTime.tv_sec = 0;
                    waitTime.tv_nsec = IPOD_PLAYER_IPC_INTR_RETRY_WAIT;
                    nanosleep(&waitTime, NULL);
                    rc = IPOD_PLAYER_IPC_OK;
                }
                else
                {
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
            }
        } while((messageHeader.seqNo <= messageHeader.totalSeqNo) && (totalMessageSize <= (S32)size) && (recvSize < (U32)totalMessageSize) && (rc == IPOD_PLAYER_IPC_OK));
        
        iPodPlayerIPCDoSemPost((sem_t *)semId);
    }
    
    if(buf != NULL)
    {
        free((U8 *)buf);
        buf = NULL;
    }
    pthread_cleanup_pop(0);
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = recvSize;
    }
    
    return rc;
}

/*!
 * \fn iPodDoPlayerRecv(void)
 * \par INPUT PARAMETERS
 * U32 size - recv size.<br>
 * U8* message - recv buffer pointer.<br>
 * S32 flag - recv flag.<br>
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_IPC_OK: Recv success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : Recv failed.</b>
 * \par DESCRIPTION
 * This Function request to recv message from socket. If it occurs recv()-ret is error, this function continue recv().
 */
static S32 iPodPlayerIPCDoRecv(U32 recvSocket, U32 size, U8* message, S32 flag)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 recvSize = 0;
    U32 recvFlag = flag;
    U32 i = 0;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPOD_PLAYER_IPC_INTR_RETRY_WAIT};
    
    /* For lint */
    message = message;

    /* Parameter check */
    if((size == 0) || (message == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message, size);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* recv process */
    recvSize = 0;
    for(i = 0; (i < IPOD_PLAYER_RECV_RETRY_NUM) && (rc != IPOD_PLAYER_IPC_OK) && (rc != IPOD_PLAYER_IPC_TMO); i++)
    {
        /* Receive socket data */
        rc = recv(recvSocket, message, size, recvFlag);
        /* Send success */
        if(rc >= 0)
        {
            recvSize += rc;
            rc = IPOD_PLAYER_IPC_OK;
            break;
        }
        else
        {
            /* Error occured by timeout */
            if((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))
            {
                nanosleep(&waitTime, NULL);
                rc = IPOD_PLAYER_IPC_TMO;
            }
            /* Erro occurred by interrupt */
            else if((errno == EPIPE) || (errno == EINTR))
            {
                /* Short wait and retry */
                nanosleep(&waitTime, NULL);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
    }
    
    /* Loop was done by retry count */
    if(i >= IPOD_PLAYER_RECV_RETRY_NUM)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, i);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = recvSize;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, recvSize, size);
    }
    
    return rc;
}

