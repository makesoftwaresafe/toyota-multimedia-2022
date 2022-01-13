
#include "iPodPlayerIPCLocal.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerUtilityLog.h"

/* Function pointer for Open */
static S32 (*iPodPlayerIPCOpenFunc[IPOD_PLAYER_OPEN_TYPE_MAX])(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *info) =
{
    iPodPlayerIPCOpenSocketServer,
    iPodPlayerIPCOpenSocketClient,
    iPodPlayerIPCOpenQueueServer,
    iPodPlayerIPCOpenQueueClient,
    NULL,
    iPodPlayerIPCOpenEpoll, 
    iPodPlayerIPCOpenSocketServer,
    iPodPlayerIPCOpenSocketClient,
    iPodPlayerIPCOpenQueueServer,
    iPodPlayerIPCOpenQueueClient,
    NULL,
    NULL
};

/* Function pointer for Receive */
static S32 (*iPodPlayerIPCRecvFunc[IPOD_PLAYER_OPEN_TYPE_MAX])(S32 fd, U8 *message, U32 size, S32 flag, S32 timeout) = 
{
    iPodPlayerIPCDoRecvSocket,
    NULL,
    iPodPlayerIPCDoRecvQueue,
    NULL,
    iPodPlayerIPCDoRecvSocket,
    NULL, 
    iPodPlayerIPCDoRecvSocket,
    NULL,
    iPodPlayerIPCDoRecvQueue,
    NULL,
    iPodPlayerIPCDoRecvSocket,
    NULL
};

/* Function pointer for Send */
static S32 (*iPodPlayerIPCSendFunc[IPOD_PLAYER_OPEN_TYPE_MAX])(S32 fd, U8 *message, U32 size, U32 flag, S32 timeout) = 
{
    NULL,
    iPodPlayerIPCDoSendSocket,
    iPodPlayerIPCDoSendQueue,
    iPodPlayerIPCDoSendQueue,
    iPodPlayerIPCDoSendSocket,
    NULL, 
    NULL,
    iPodPlayerIPCDoSendSocket,
    iPodPlayerIPCDoSendQueue,
    iPodPlayerIPCDoSendQueue,
    iPodPlayerIPCDoSendSocket,
    NULL
};

/* Function pointer for Close */
static S32 (*iPodPlayerIPCCloseFunc[IPOD_PLAYER_OPEN_TYPE_MAX])(S32 fd) = 
{
    iPodPlayerIPCDoCloseSocketServer,
    iPodPlayerIPCDoCloseSocketClient,
    iPodPlayerIPCDoCloseQueueServer,
    iPodPlayerIPCDoCloseQueueClient,
    NULL,
    iPodPlayerIPCCloseEpoll, 
    iPodPlayerIPCDoCloseSocketServer,
    iPodPlayerIPCDoCloseSocketClient,
    iPodPlayerIPCDoCloseQueueServer,
    iPodPlayerIPCDoCloseQueueClient,
    NULL,
    NULL
};

/* Function pointer for Long Send */
static S32 (*iPodPlayerIPCLongSendFunc[IPOD_PLAYER_OPEN_TYPE_MAX])(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout) = 
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    iPodPlayerIPCDoLongSendSocket,
    NULL,
    NULL,
    NULL,
    NULL
};

/* Function pointer for Long Receive */
static S32 (*iPodPlayerIPCLongRecvFunc[IPOD_PLAYER_OPEN_TYPE_MAX])(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout) = 
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    iPodPlayerIPCDoLongRecvSocket,
    NULL,
    NULL,
    NULL,
    iPodPlayerIPCDoLongRecvSocket,
    NULL
};

S32 iPodPlayerIPCInit(void)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    /* Initialize the gloval variable and semaphore */
    rc = iPodPlayerIPCInfoInit();
    
    return rc;
}

S32 iPodPlayerIPCDeinit(void)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    /* Deinitialize the gloval variable and semaphore */
    rc = iPodPlayerIPCInfoDeinit();
    
    return rc;
}

S32 iPodPlayerIPCCreateHandle(S32 *handle, S32 fd)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, fd);
    
    /* Paramter Check */
    if(handle == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Check the existence of this handle in the ipc info */
    info = iPodPlayerIPCRefIPCInfo(fd);
    if(info == NULL)
    {
        /* Lock and get the ipc info */
        info = iPodPlayerIPCRefIPCInfo(-1);
        if(info != NULL)
        {
            info->isReady = 1;
            info->id = fd;
            info->pid = -1;
            info->type = IPOD_PLAYER_OPEN_USER_SELECT;
            *handle = info->id;
            /* Unlock */
            iPodPlayerIPCUnrefIPCInfo(info);
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, IPOD_PLAYER_IPC_ERR_EXIST, -1);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        /* Handle exist */
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, IPOD_PLAYER_IPC_ERR_EXIST, handle);
        rc = IPOD_PLAYER_IPC_ERR_EXIST;
    }
    
    /* Pop because semaphore is unlocked */
    pthread_cleanup_pop(0);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc, *handle);
    
    return rc;
}

S32 iPodPlayerIPCDeleteHandle(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle);
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);

    rc = IPOD_PLAYER_IPC_ERROR;

    /* Lock and get the ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        /* Cloear the ipc information from the table */
        iPodPlayerIPCClearIPCInfo(info);
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    /* Pop because semaphore is unlocked */
    pthread_cleanup_pop(0);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc, handle);
    
    return rc;
}

S32 iPodPlayerIPCOpen(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *info)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, info);
    
    /* Parameter check */
    if((handle == NULL) || (info == NULL))
    {
        /* Parameter invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, info);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, info->prefix);
    IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, info->identify);
    IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, info->semName);
    IPOD_LOG_INFO_WRITE64(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, *handle, info->type, info->connectionNum, info->maxBufSize, info->maxPacketSize);
    
    if((info->type < IPOD_PLAYER_OPEN_TYPE_MAX) && (iPodPlayerIPCOpenFunc[info->type] != NULL))
    {
        rc = iPodPlayerIPCOpenFunc[info->type](handle, info);
    }
    else
    {
        /* Parameter invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, info->type);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc, *handle);
    
    return rc;
}

S32 iPodPlayerIPCReceive(S32 handle, U8 *message, U32 size, S32 flag, S32 waitHandle, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U32 resNum = 0;
    S32 resHandle = 0;
    U8 retType = 0;
    U8 type = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, message, size, flag, waitHandle, timeout);
    
    if((handle <= 0) || (NULL == message))
    {
        /* Parameter invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, message);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Wait until target descriptor ready for reeaceive */
    if(flag == IPOD_PLAYER_IPC_FLAG_BLOCK)
    {
        rc = iPodPlayerIPCWait(waitHandle, IPOD_PLAYER_IPC_RECV_WAIT_NUM, &handle, &resNum, 
                                &resHandle, &retType, timeout, IPOD_PLAYER_WAIT_IN);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            if(resNum != IPOD_PLAYER_IPC_RECV_WAIT_NUM)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, resNum);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
    }
    else
    {
        resHandle = handle;
        rc = IPOD_PLAYER_IPC_OK;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Push for semaphore */
        pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
        type = 0;
        
        /* Lock and get the ipc info  */
        info = iPodPlayerIPCRefIPCInfo(resHandle);
        if(info != NULL)
        {
            type = info->type;
            /* Unlock */
            iPodPlayerIPCUnrefIPCInfo(info);
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, resHandle);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        
        /* Pop because semaphore was unlocked*/
        pthread_cleanup_pop(0);
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            if((type < IPOD_PLAYER_OPEN_TYPE_MAX) && (iPodPlayerIPCRecvFunc[type] != NULL))
            {
                rc = iPodPlayerIPCRecvFunc[type](resHandle, message, size, flag, timeout);
            }
            else
            {
                /* Parameter invalid */
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, type);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;

}

S32 iPodPlayerIPCSend(S32 handle, U8 *message, U32 size, U32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U8 type = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, message, size, flag, timeout);
    
    /* Parameter check */
    if((handle <= 0) || (NULL == message))
    {
        /* Parameter invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, message);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    type = 0;

    /* Lock and get ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        type = info->type;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, type);
        if((type < IPOD_PLAYER_OPEN_TYPE_MAX) && (iPodPlayerIPCSendFunc[type] != NULL))
        {
            rc = iPodPlayerIPCSendFunc[type](handle, message, size, flag, timeout);
        }
        else
        {
            /* Parameter invalid */
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, type);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;
}

S32 iPodPlayerIPCLongSend(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U8 type = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, dataNum, message, size, asize, flag, timeout);
    
    /* Parameter check */
    if((handle <= 0) || (NULL == message) || (size == NULL) || (asize == NULL))
    {
        /* Parameter invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, message, size, asize);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    type = 0;
    
    /* Lock and get ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        type = info->type;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, type);
        if((type < IPOD_PLAYER_OPEN_TYPE_MAX) && (iPodPlayerIPCSendFunc[type] != NULL))
        {
            rc = iPodPlayerIPCLongSendFunc[type](handle, dataNum, message, size, asize, flag, timeout);
        }
        else
        {
            /* Parameter invalid */
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, type);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;
}

S32 iPodPlayerIPCLongReceive(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, S32 waitHandle, U32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U32 resNum = 0;
    S32 resHandle = 0;
    U8 retType = 0;
    U8 type = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, dataNum, message, size, asize, waitHandle, flag, timeout);
    
    /* Parameter check  */
    if((handle <= 0) || (message == NULL) || (size == NULL) || (asize == NULL))
    {
        /* Parameter invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, message, size, asize);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    
    /* Wait until target descriptor ready for receive */
    if(flag == IPOD_PLAYER_IPC_FLAG_BLOCK)
    {
        rc = iPodPlayerIPCWait(waitHandle, IPOD_PLAYER_IPC_RECV_WAIT_NUM, &handle, &resNum, 
                                &resHandle, &retType, timeout, IPOD_PLAYER_WAIT_IN);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            if(resNum != IPOD_PLAYER_IPC_RECV_WAIT_NUM)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, resNum);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
    }
    else
    {
        resHandle = handle;
        rc = IPOD_PLAYER_IPC_OK;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Push for semaphore */
        pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
        type = 0;

        /* Lock and get the ipc info */
        info = iPodPlayerIPCRefIPCInfo(handle);
        if(info != NULL)
        {
            type = info->type;
            /* Unlock  */
            iPodPlayerIPCUnrefIPCInfo(info);
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        /* Pop because semaphore was unlocked */
        pthread_cleanup_pop(0);
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, type);
            if((type < IPOD_PLAYER_OPEN_TYPE_MAX) && (iPodPlayerIPCLongRecvFunc[type] != NULL))
            {
                rc = iPodPlayerIPCLongRecvFunc[type](handle, dataNum, message, size, asize, flag, timeout);
            }
            else
            {
                /* Parameter invalid */
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, type);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;
}

S32 iPodPlayerIPCClose(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    U8 type = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle);
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    type = 0;

    /* Lock and get the ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        type = info->type;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, type);
        if((type < IPOD_PLAYER_OPEN_TYPE_MAX) && (iPodPlayerIPCCloseFunc[type] != NULL))
        {
            rc = iPodPlayerIPCCloseFunc[type](handle);
        }
        else
        {
            /* Parameter invalid */
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, type);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;
}

S32 iPodPlayerIPCWait(S32 waitHandle, U32 handleNum, S32 *handles, U32 *resNum, S32 *resHandle, U8 *retType, S32 timeout, U32 flag)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    S32 resEvent[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, waitHandle, handleNum, handles, resNum, resHandle, retType, timeout, flag);
    
    if((handles == NULL) || (resNum == NULL) || (resHandle == NULL) || (retType == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handles, resNum, resHandle, resHandle, retType);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    do
    {
        rc = iPodPlayerIPCModEpoll(waitHandle, handleNum, handles, flag);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = iPodPlayerIPCWaitEpoll(waitHandle, handleNum, resNum, resHandle, resEvent, timeout);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = iPodPlayerIPCEventCheck(waitHandle, resNum, resHandle, resEvent, retType);
            }
        }
    } while((rc == IPOD_PLAYER_IPC_OK) && (*resNum == 0));
    
    IPOD_LOG_INFO_WRITEBIN(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, (sizeof(S32) * handleNum), (U8 *)handles);
    IPOD_LOG_INFO_WRITEBIN(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, *resNum, retType);
    IPOD_LOG_INFO_WRITEBIN(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, (sizeof(S32) * (*resNum)), (U8 *)resHandle);
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc, *resNum);
    
    return rc;
}
    
S32 iPodPlayerIPCIterate(S32 waitHandle, U32 handleNum, IPOD_PLAYER_IPC_HANDLE_INFO *inputInfo, U32 num, IPOD_PLAYER_IPC_HANDLE_INFO *outputInfo)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    volatile U32 i = 0;
    volatile U32 j = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, waitHandle, handleNum, inputInfo, num, outputInfo);
    
    /* Parameter check */
    if((handleNum == 0) || (inputInfo == NULL) || (outputInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handleNum, inputInfo, outputInfo);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    for(i = 0; i < handleNum; i++)
    {
        /* Check each handle whether it should be accepted or closed */
        rc = iPodPlayerIPCCheckHandle(waitHandle, &inputInfo[i]);
        if(rc == IPOD_PLAYER_IPC_NO_ACCEPT)
        {
            rc = IPOD_PLAYER_IPC_OK;
            if(j < num)
            {
                /* Push for semaphore */
                pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
                /* Lock and get the ipc info */
                info = iPodPlayerIPCRefIPCInfo(inputInfo[i].handle);
                if(info != NULL)
                {
                    outputInfo[j].type = info->type;
                    /* Unlock */
                    iPodPlayerIPCUnrefIPCInfo(info);
                    rc = IPOD_PLAYER_IPC_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, inputInfo->handle);
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
                /* Pop because semaphore was unlocked */
                pthread_cleanup_pop(0);
                
                /* Set the handle and event to output structure because haldle is no accept no close.*/
                outputInfo[j].handle = inputInfo[i].handle;
                outputInfo[j].event = inputInfo[i].event;
                j++;
            }
            else
            {
                break;
            }
        }
        else if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Socket was opened or accpeted fd was closed */
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Return number of file descriptor which is not accepted or closed */
        rc = j;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;
}

S32 iPodPlayerIPCGetFDs(S32 handle, S32 num, S32 *fds)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_IPCLIB);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_IPCLIB, handle, num, fds);
    
    /* Parameter check */
    if((num < IPOD_PLAYER_IPC_MAX_EPOLL_NUM) || (fds == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Get file descriptors which parent is handle */
    rc = iPodPlayerIPCGetIPCfds(num, fds, handle);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_IPCLIB, rc);
    
    return rc;
}

