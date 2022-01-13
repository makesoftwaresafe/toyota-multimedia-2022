#include "ipp_mainloop_common.h"

S32 iPodCoreEpollCtl(S32 epollFd, S32 handle, S32 ctl, U32 event)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U32 i = 0;
    S32 fds[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    struct epoll_event epollEvent;
    U32 count = 0;
    S32 errid = 0;
    
    /* Get file descriptors associated with handle */
    rc = iPodPlayerIPCGetFDs(handle, sizeof(fds) / sizeof(fds[0]), fds);
    if(rc > 0)
    {
        count = (U32)rc;
        for(i = 0; i < count; i++)
        {
            /* Add descriptors to epoll */
            if(ctl == EPOLL_CTL_ADD)
            {
                epollEvent.events = event;
                epollEvent.data.fd = fds[i];
                rc = epoll_ctl(epollFd, ctl, fds[i], &epollEvent);
            }
            /* Delete descriptors from epoll */
            else
            {
                rc = epoll_ctl(epollFd, ctl, fds[i], NULL);
            }
            
            /* epoll_ctl returns an error */
            if(rc != 0)
            {
                errid = errno;
                IPOD_DLT_WARN("epoll_ctl returns an error (errno = %d)", errid);
            }
        }
        
    }
    
    /* Add handle to epoll */
    if(ctl == EPOLL_CTL_ADD)
    {
        epollEvent.events = event;
        epollEvent.data.fd = handle;
        rc = epoll_ctl(epollFd, ctl, handle, &epollEvent);
    }
    /* Delete handle from epoll */
    else
    {
        rc = epoll_ctl(epollFd, ctl, handle, NULL);
    }
    
    /* epoll_ctl returns an error */
    
    if(rc == 0)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

void iPodCoreAddFDs(S32 epollFd, U32 num, S32 *handles)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 j = 0;
    S32 fds[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {-1};
    
    /* Parameter check */
    if((num == 0) || (handles == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, num, handles);
        return;
    }
    
    /* Add all handles to epoll */
    for(i = 0; i < num; i++)
    {
        rc = iPodPlayerIPCGetFDs(handles[i], sizeof(fds), fds);
        if(rc > 0)
        {
            for(j = 0; j < (U32)rc; j++)
            {
                iPodCoreEpollCtl(epollFd, fds[j], EPOLL_CTL_ADD, EPOLLIN);
            }
            rc = IPOD_PLAYER_OK;
        }
        iPodCoreEpollCtl(epollFd, handles[i], EPOLL_CTL_ADD, EPOLLIN);
    }
    
    return;
}

void iPodCoreDelFDs(S32 epollFd, U32 num, S32 *handles)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 j = 0;
    S32 fds[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    
    /* Parameter check */
    if((num == 0) || (handles == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, num, handles);
        return;
    }
    
    /* Delete all handles from epoll */
    for(i = 0; i < num; i++)
    {
        rc = iPodPlayerIPCGetFDs(handles[i], sizeof(fds), fds);
        if(rc > 0)
        {
            for(j = 0; j < (U32)rc; j++)
            {
                iPodCoreEpollCtl(epollFd, fds[j], EPOLL_CTL_DEL, 0);
            }
            rc = IPOD_PLAYER_OK;
        }
        iPodCoreEpollCtl(epollFd, handles[i], EPOLL_CTL_DEL, 0);
    }
    
    return;
}

/* Set the priority of queue according to request */
static void iPodCoreSetPrior(IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    /* Check the parameter */
    if(waitData == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitData);
        return;
    }
    
    /* Request is internal request */
    if((waitData->contents.paramTemp.header.funcId == IPOD_FUNC_INTERNAL_GET_STATUS) ||
       (waitData->contents.paramTemp.header.funcId == IPOD_FUNC_INTERNAL_HMI_STATUS_SEND) ||
       (waitData->contents.paramTemp.header.funcId == IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND))
    {
        /* Set the middle low priority */
        waitData->prior = IPODCORE_QUEUE_PRIOR_MIDLOW;
    }
    /* Request is finalize */
    else if(waitData->contents.paramTemp.header.funcId == IPOD_FUNC_DEINIT)
    {
        /* Set the high priority */
        waitData->prior = IPODCORE_QUEUE_PRIOR_HIGH;
    }
    else if(waitData->contents.paramTemp.header.funcId == IPOD_FUNC_INTERNAL_NOTIFY_STATUS)
    {
        waitData->prior = IPODCORE_QUEUE_PRIOR_MIDHIGH;
    }
    else
    {
        /* Usualy request is middle priority */
        waitData->prior = IPODCORE_QUEUE_PRIOR_MID;
    }
    
    return;
}

/* Set the queue to request queue list */
S32 iPodCoreSetQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Check the parameter */
    if((waitList == NULL) || (header == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitList, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Loop until maximum queuing size or request is success */
    for(i = 0; (i < IPODCORE_WAIT_LIST_MAX) && (rc != IPOD_PLAYER_OK); i++)
    {
        /* Current queue status is none. It means that current queue is empty */
        if(waitList[i].status == IPODCORE_QUEUE_STATUS_NONE)
        {
            /* Request is short data */
            if(header->longData == 0)
            {
                /* Request data size equal or less than queuing data maximum size */
                if(size <= sizeof(waitList[i].contents))
                {
                    /* Set the request to queue */
                    memcpy(&waitList[i].contents, contents, size);
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    break;
                }
            }
            /* Request is large data */
            else
            {
                U32 storeSize = size - sizeof(IPOD_PLAYER_FUNC_HEADER);
                waitList[i].storeBuf = calloc(storeSize, sizeof(U8));
                if(waitList[i].storeBuf != NULL)
                {
                    memcpy(waitList[i].storeBuf, contents, storeSize);
                    IPOD_DLT_VERBOSE("Long IPC data size = %u", storeSize);
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_NOMEM;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    break;
                }
            }
            memcpy(&waitList[i].contents.paramTemp.header, header, sizeof(waitList[i].contents.paramTemp.header));
            
            /* Status changes to WAIT */
            waitList[i].status = IPODCORE_QUEUE_STATUS_WAIT;
            
            /* Set the priority */
            iPodCoreSetPrior(&waitList[i]);
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, i, header->funcId);
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERR_QUEUE_FULL;
        }
    }
    
    if(rc == IPOD_PLAYER_ERR_QUEUE_FULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreCheckQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList, IPOD_PLAYER_FUNC_ID funcId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    if(waitList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    for(i = 0; (i < IPODCORE_WAIT_LIST_MAX - 1) && (rc == IPOD_PLAYER_ERROR); i++)
    {
        /* Check the queue. Requested funcID is in queue */
        if(waitList[i].contents.paramTemp.header.funcId == funcId)
        {
            /* Set found queue number */
            rc = i;
        }
    }
    
    return rc;
}

void iPodCoreSortQueue(U32 listNum, IPOD_PLAYER_CORE_WAIT_LIST *waitList)
{
    U32 i = 0;
    U32 j = 0;
    IPOD_PLAYER_CORE_WAIT_LIST temp;
    
    if((listNum == 0) || (waitList == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, listNum, waitList);
        return;
    }
    
    /* Queue which has a request is filled from head queue */
    /* Loop until maximum queuing size */
    for(i = 0; i < listNum; i++)
    {
        if((waitList[i].status != IPODCORE_QUEUE_STATUS_WAIT) && (waitList[i].status != IPODCORE_QUEUE_STATUS_RUNNING))
        {
            /* Loop until maximum queuing size from current queue location + 1 */
            for(j = i + 1; j < listNum; j++)
            {
                /* Queuing status is still running */
                if((waitList[j].status == IPODCORE_QUEUE_STATUS_WAIT) || (waitList[j].status == IPODCORE_QUEUE_STATUS_RUNNING))
                {
                    /* Change the queue */
                    temp = waitList[i];
                    waitList[i] = waitList[j];
                    waitList[j] = temp;
                    break;
                }
            }
        }
    }
    
    /* Queue which has a request is sorted by priority */
    /* Loop until maximum queuing size */
    for(i = 0; i < listNum - 1; i++)
    {
        /* Loop until maximum queuing size - 1 */
        for(j = listNum - 1; j > i; j--)
        {
            /* Priority of current queue is higher than previous queue */
            if(waitList[j].prior > waitList[j - 1].prior)
            {
                temp = waitList[j - 1];
                waitList[j - 1] = waitList[j];
                waitList[j] = temp;
            }
        }
    }
    
    return;
}

/* Delete the queue from request queue list */
S32 iPodCoreDelQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 i = 0;
    
    /* Check the parameter */
    if(waitList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Loop until maximum queuing sizes */
    for(i = 0; i < IPODCORE_WAIT_LIST_MAX - 1; i++)
    {
        /* Queuing status is finish */
        if(waitList[i].status == IPODCORE_QUEUE_STATUS_FINISH)
        {
            /* Remove the queue */
            waitList[i].status = IPODCORE_QUEUE_STATUS_NONE;
            if(waitList[i].storeBuf != NULL)
            {
                free(waitList[i].storeBuf);
                waitList[i].storeBuf = NULL;
            }
            
            memset(&waitList[i], 0, sizeof(waitList[i]));
        }
    }
    
    /* Sort the List */
    iPodCoreSortQueue(IPODCORE_WAIT_LIST_MAX, waitList);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreCheckResultAndUpdateStatus(S32 result, IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(waitData == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(waitData->status == IPODCORE_QUEUE_STATUS_NONE)
    {
        return IPOD_PLAYER_OK;
    }
    
    switch(result)
    {
    case IPOD_PLAYER_OK:
        /* Queue was finished with success */
        /* Status changes to FINISH*/
        waitData->status = IPODCORE_QUEUE_STATUS_FINISH;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_ERR_NO_REPLY:
        /* Queue was finished but not reply to application */
        /* Status change to FINISH */
        waitData->status = IPODCORE_QUEUE_STATUS_FINISH;
        rc = IPOD_PLAYER_ERR_NO_REPLY;
        break;
        
    case IPOD_PLAYER_ERR_BACK_TO_WAIT:
        /* Queue status backs to wait */
        /* Status change to WAIT */
        waitData->status = IPODCORE_QUEUE_STATUS_WAIT;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_ERR_REQUEST_CONTINUE:
        /* Queue is still running */
        /* Status changes to RUNNING */
        waitData->status = IPODCORE_QUEUE_STATUS_RUNNING;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IPOD_PLAYER_ERR_STILL_WAIT:
        /* Queue is not still executed */
        /* Status changes to WAIT */
        waitData->status = IPODCORE_QUEUE_STATUS_WAIT;
        rc = IPOD_PLAYER_OK;
        break;
        
    default:
        /* Queue was finished due to some error */
        /* Status changes to FINISH*/
        waitData->status = IPODCORE_QUEUE_STATUS_FINISH;
        rc = result;
        break;
    }
    
    return rc;
}

S32 iPodCoreSetSampleRate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 sample)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_PAI_SET_SAMPLE paiSetSample;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&paiSetSample, 0, sizeof(paiSetSample));
    
    if(iPodCtrlCfg->iPodInfo->sampleRate != sample)
    {
        iPodCtrlCfg->iPodInfo->sampleRate = sample;
    }
    
    if(iPodCtrlCfg->startAudio != 0)
    {
        paiSetSample.header.funcId = IPOD_FUNC_PAI_SETSR;
        paiSetSample.header.devID = iPodCtrlCfg->threadInfo->appDevID;
        paiSetSample.sampleRate = iPodCtrlCfg->iPodInfo->sampleRate;
        
        rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, (U8 *)&paiSetSample, sizeof(paiSetSample), 0, IPODCORE_TMOUT_FOREVER);
        if(rc >= 0)
        {
            rc = IPOD_PLAYER_OK;
        }
        
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreSetSampleRateResult(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    /* For lint */
    header = header;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Check Queue Status */
    if(waitData->status == IPODCORE_QUEUE_STATUS_FINISH)
    {
        /* If current queue is not requested id and queue status is not wait, */
        /* Nothing to do */
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
        return rc;
    }
    
    if(waitData->contents.paiResult.cmdId == IPOD_FUNC_PAI_RESULT)
    {
        rc = waitData->contents.paiResult.result;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
        IPOD_DLT_WARN("cmdId is invalid :%d, cmdId=%u", rc, waitData->contents.paiResult.cmdId);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = IPOD_PLAYER_ERR_NO_REPLY;
    }
    
    /* Current queue status changes to Finish */
    waitData->status = IPODCORE_QUEUE_STATUS_FINISH;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Initialize the long receive information */
void iPodCoreInitLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList)
{
    U32 i = 0;
    
    /* Parameter check */
    if(recvInfoList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, recvInfoList);
        return;
    }
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        /* Clear all structure information */
        memset(&recvInfoList[i], 0, sizeof(recvInfoList[i]));
        recvInfoList[i].fd = -1;
        recvInfoList[i].num = IPODCORE_LONG_HEADER_NUM;
        recvInfoList[i].buf[IPODCORE_LONG_HEADER_POS] = (U8 *)&recvInfoList[i].header;
        recvInfoList[i].size[IPODCORE_LONG_HEADER_POS] = sizeof(recvInfoList[i].header);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    
    return;
}

/* Get the long receive information by file descriptor */
IPOD_PLAYER_CORE_LONG_RECEIVE_INFO * iPodCoreGetLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd)
{
    U32 i = 0;
    IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *info = NULL;
    S32 empty = -1;
    
    /* Parameter check */
    if(recvInfoList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, recvInfoList);
        return NULL;
    }
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        /* File descriptor is not registered or same descriptor exsits */
        if(recvInfoList[i].fd == -1)
        {
            if(empty == -1)
            {
                empty = i;
            }
        }
        else if(recvInfoList[i].fd == fd)
        {
            empty = i;
            break;
        }
    }
    
    if(empty != -1)
    {
        /* If strucuture is empty, register the information */
        if(recvInfoList[empty].fd == -1)
        {
            recvInfoList[empty].fd = fd;
        }
        /* Copy the long receive information */
        info = &recvInfoList[empty];
    }
    
    if(info != NULL)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    
    }
    
    return info;
    
}

/* Set the long receive information */
S32 iPodCoreSetLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd, IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *info)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Parameter check */
    if((recvInfoList == NULL) || (info == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, recvInfoList, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        /* File descriptor of registered information equals with file descriptor */
        if(recvInfoList[i].fd == fd)
        {
            memcpy(&recvInfoList[i], info, sizeof(recvInfoList[i]));
            rc = IPOD_PLAYER_OK;
            break;
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Clear the long receive information */
S32 iPodCoreClearLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd)
{
    S32 rc = IPOD_PLAYER_ERROR;;
    U32 i = 0;
    
    /* Parameter check */
    if(recvInfoList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, recvInfoList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    for(i = 0; i < IPOD_PLAYER_IPC_MAX_EPOLL_NUM; i++)
    {
        /* File descriptor is registered */
        if(recvInfoList[i].fd == fd)
        {
            /* Clear long receive information */
            recvInfoList[i].fd = -1;
            recvInfoList[i].num = IPODCORE_LONG_HEADER_NUM;
            recvInfoList[i].buf[IPODCORE_LONG_DATA_POS] = NULL;
            recvInfoList[i].size[IPODCORE_LONG_DATA_POS] = 0;
            memset(&recvInfoList[i].header, 0, sizeof(recvInfoList[i].header));
            rc = IPOD_PLAYER_OK;
            break;
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCorePollWait(S32 pollFd, U32 waitTime, U32 infoNum, IPOD_PLAYER_IPC_HANDLE_INFO *outputInfo)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct epoll_event epollEvent[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    IPOD_PLAYER_IPC_HANDLE_INFO inputInfo[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    U32 i = 0;
    
    /* Parameter check */
    if(outputInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, outputInfo);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize structures */
    memset(epollEvent, 0, sizeof(epollEvent));
    memset(inputInfo, 0, sizeof(inputInfo));
    
    /* Wait the I/O event */
    rc = epoll_wait(pollFd, epollEvent, sizeof(epollEvent), waitTime);
    if(rc > 0)
    {
        for(i = 0; i < (U32)rc; i++)
        {
            /* Set the receiving file descriptors and events to inputInfo */
            inputInfo[i].handle = epollEvent[i].data.fd;
            inputInfo[i].event = epollEvent[i].events;
        }
        /* Interate the IPC library for receiving descriptors */
        rc = iPodPlayerIPCIterate(pollFd, i, inputInfo, infoNum, outputInfo);
    }
    else if(rc == 0)
    {
        /* Timeout occured */
        rc = IPOD_PLAYER_ERR_TMOUT;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}

void iPodCoreCheckAndRemoveLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *longRecvInfo, S32 result, IPOD_PLAYER_IPC_HANDLE_INFO *handleInfo)
{
    IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfo = NULL;
    U32 i = 0;
    
    /* Parameter check */
    if((longRecvInfo == NULL) || (handleInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, longRecvInfo, handleInfo);
        return;
    }
    
    /* Long receive is finished */
    if(((result != IPOD_PLAYER_ERR_NOMEM) && (result != IPOD_PLAYER_ERR_MORE_PACKET)) && 
       (handleInfo->type == IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG))
    {
        recvInfo = iPodCoreGetLongRecvInfo(longRecvInfo, handleInfo->handle);
        if(recvInfo != NULL)
        {
            for(i = IPODCORE_LONG_DATA_POS; i < recvInfo->num; i++)
            {
                /* Buffer is allocated */
                if(recvInfo->buf[i] != NULL)
                {
                    free(recvInfo->buf[i]);
                    recvInfo->buf[i] = NULL;
                }
            }
            
            recvInfo->num = IPODCORE_LONG_HEADER_NUM;
        }
        
        /* Clear long receive information from receive information structure */
        iPodCoreClearLongRecvInfo(longRecvInfo, handleInfo->handle);
    }
    
    return;
}

void iPodCoreClearData(IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    if(contents == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, contents);
        return;
    }
    
    memset(contents, 0, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS));
    
}

