/**
* \file: iPodPlayerMain.c
*
*
***************************************************************************** */

/* ignore GCC warning
 *  warning: variable 'buf' might be clobbered by 'longjmp' or 'vfork' [-Werror=clobbered]
 * */
#pragma GCC diagnostic ignored "-Wclobbered"

#include "iPodPlayerIPCLocal.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerIPCMessage.h"
#include "iPodPlayerUtilityLog.h"

static S32 iPodPlayerIPCDoSend(U32 sendSocket, U32 size, U8* message, U32 flag, S32 timeout);

/*!
 * \fn iPodPlayerIPCConnectSend(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_IPC_OK: connection success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : connection failed.</b>
 * \par DESCRIPTION
 * This function create iPodPlayerI/F socket-connection.
 */
S32 iPodPlayerIPCOpenSocketClient(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo)
{
    S32     rc = IPOD_PLAYER_IPC_ERROR;
    S32     sendAddrSize = 0;
    S32     fd = -1;
    S32     loop = 0;
    struct  sockaddr_un sendAddr;
    struct timespec waitTime;
    IPOD_PLAYER_PARAM_DESC_INFO *info;
    sem_t *semId = NULL;
    U8 namedSem = 0;
    
    /* Parameter check */
    if((handle == NULL) || (openInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, openInfo);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Parameter membe check */
    if(openInfo->prefix == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, openInfo->prefix);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Initialize the structure */
    memset(&sendAddr, 0, sizeof(sendAddr));
    memset(&waitTime, 0, sizeof(waitTime));
    
    /* Identify String is used */
    if(openInfo->identify != NULL)
    {
        snprintf((char *)sendAddr.sun_path, IPOD_PLAYER_SOCKET_NAME_LEN, "%s%s", openInfo->prefix, openInfo->identify);
    }
    /* No Identify string */
    else
    {
        snprintf(sendAddr.sun_path, IPOD_PLAYER_SOCKET_NAME_LEN, "%s", openInfo->prefix);
    }
    
    sendAddr.sun_family = AF_UNIX;
    sendAddrSize = sizeof(sendAddr);
    
    /* create socket using SOCK_SEQPACKET */
    fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if(fd != -1)
    {
        /* Waiting for connection to server */
        do
        {
            /* PRQA: Lint Message 64: Lint describes which the second parameter is "__CONST_SOCKADDR_ARG" 
            but according to spec of connect, the type of second parameter is "const struct sockaddr *". so this cast is correct */
            rc = connect(fd, (struct sockaddr *) &sendAddr, sendAddrSize); /*lint !e64 */
            if(rc == 0)
            {
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                waitTime.tv_sec = IPOD_PLAYER_IPC_CONNECT_WAIT;
                waitTime.tv_nsec = 0;
                nanosleep(&waitTime, NULL);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
            loop++;
            
        } while((loop < IPOD_PLAYER_SEND_RETRY_NUM) && (rc != IPOD_PLAYER_IPC_OK));
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = iPodPlayerIPCDoSemOpen((sem_t **)&semId, openInfo->semName, &namedSem);
            if(rc != IPOD_PLAYER_IPC_OK)
            {
                semId = NULL;
            }
        }
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Push for semaphore */
            pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
            /* Check the existence of this handle in the ipc info */
            info = iPodPlayerIPCRefIPCInfo(fd);
            if(info == NULL)
            {
                /* Lock and get the empty ipc info */
                info = iPodPlayerIPCRefIPCInfo(-1);
                if(info != NULL)
                {
                    info->isReady = 1;
                    info->id = fd;
                    info->pid = -1;
                    info->type = openInfo->type;
                    info->namedSem = namedSem;
                    info->semId = semId;
                    info->maxBufSize = openInfo->maxBufSize;
                    info->maxPacketSize = openInfo->maxPacketSize;
                    /* Set the handle */
                    *handle = info->id;
                    /* Unlock */
                    iPodPlayerIPCUnrefIPCInfo(info);
                    rc = IPOD_PLAYER_IPC_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, -1);
                    rc = IPOD_PLAYER_IPC_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, fd);
                iPodPlayerIPCUnrefIPCInfo(info);
                rc = IPOD_PLAYER_IPC_ERR_EXIST;
            }
            /* Pop because semaphore was unlocked */
            pthread_cleanup_pop(0);
        }
        
        /* Error occurred */
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, rc);
            iPodPlayerIPCDoSemClose((sem_t **)&semId, namedSem);
            semId = NULL;
            close(fd);
            fd = -1;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    return rc;
}

/* Open queue */
S32 iPodPlayerIPCOpenQueueClient(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    U8 tempName[IPOD_PLAYER_IPC_TEMP_MAX_LEN] = {0};
    S32 oflag = O_RDWR | O_CREAT;
    struct mq_attr attr;
    IPOD_PLAYER_PARAM_DESC_INFO *info;
    S32 fd = -1;
    sem_t *semId = NULL;
    U8 namedSem = 0;
    
    /* Paramter Check */
    if((handle == NULL) || (openInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle, openInfo);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Member of paramter check */
    if((openInfo->prefix == NULL) || (openInfo->identify == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, openInfo->prefix, openInfo->identify);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Initialize the structure */
    memset(&attr, 0, sizeof(attr));
    
    /* Set maximum connection number and maximum size */
    attr.mq_maxmsg = openInfo->connectionNum;
    attr.mq_msgsize = openInfo->maxBufSize + sizeof(IPOD_PLAYER_MESSAGE_HEADER);
    
    /* Copy the name */
    rc = snprintf((char *)tempName, sizeof(tempName), "%s_%s", openInfo->prefix, openInfo->identify);
    if(rc > 0)
    {
        /* Unlinked queue. Remove name of message queue */
        fd = mq_open((const char *)tempName, oflag, S_IRWXU, &attr);
        if(fd != -1)
        {
            /* Open send semapore */
            rc = iPodPlayerIPCDoSemOpen(&semId, openInfo->semName, &namedSem);
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
        /* Push for semaphore */
        pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
        /* Check the existence of this handle in the ipc info */
        info = iPodPlayerIPCRefIPCInfo(fd);
        if(info == NULL)
        {
            /* Lock and get ipc info */
            info = iPodPlayerIPCRefIPCInfo(-1);
            if(info != NULL)
            {
                info->isReady = 1;
                info->id = fd;
                info->pid = -1;
                info->type = openInfo->type;
                info->namedSem = 0;
                info->semId = semId;
                info->maxBufSize = openInfo->maxBufSize;
                info->maxPacketSize = openInfo->maxPacketSize;
                *handle = info->id;
                /* Unlock */
                iPodPlayerIPCUnrefIPCInfo(info);
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, -1);
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
    
    /* Error occurred */
    if(rc != IPOD_PLAYER_IPC_OK)
    {
        if(semId != NULL)
        {
            iPodPlayerIPCDoSemClose(&semId, namedSem);
        }
        
        if(fd >= 0)
        {
            mq_close(fd);
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerIPCCloseConnectSend(void)
 * \par INPUT PARAMETERS
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_IPC_OK: DeInitialized success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : DeInitialize failed.</b>
 * \par DESCRIPTION
 * This function close the socket-connection.
 */
S32 iPodPlayerIPCDoCloseSocketClient(S32 handle)
{
    S32  rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    /* Parameter check */
    if(handle < 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore  */
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
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        
        if(info->semId != NULL)
        {
            rc = iPodPlayerIPCDoSemClose((sem_t **)&info->semId, info->namedSem);
        }
        
        /* Clear ipc info */
        iPodPlayerIPCClearIPCInfo(info);
        iPodPlayerIPCUnrefIPCInfo(info);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Pop because semaphore was created  */
    pthread_cleanup_pop(0);
    
    return rc;
}

/* Close queue */
S32 iPodPlayerIPCDoCloseQueueClient(S32 handle)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    
    /* Parameter check */
    if(handle < 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Lock and get the ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        /* Queue is created */
        if(info->id >= 0)
        {
            rc = mq_close(info->id);
            if(rc != -1)
            {
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
        
        /* Semaphore is created */
        if(info->semId != NULL)
        {
            rc = iPodPlayerIPCDoSemClose(&info->semId, info->namedSem);
        }
        
        /* Clear ipc info */
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
    return rc;
}

S32 iPodPlayerIPCSendOneMsg(S32 fd, const IPOD_PLAYER_MESSAGE_HEADER *messageHeader, U32 size, U8 *data, U32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERR_AGAIN;
    U32 i = 0;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPOD_PLAYER_IPC_SEND_RETRY_WAIT};
    
    /* Check the parameter */
    if((messageHeader == NULL) || (data == NULL))
    {
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    for(i = 0; (i < IPOD_PLAYER_IPC_SEND_RETRY_MAX) && (rc == IPOD_PLAYER_IPC_ERR_AGAIN); i++)
    {
        /* Send a meesage header */
        rc = iPodPlayerIPCDoSend(fd, sizeof(*messageHeader), (U8 *)messageHeader, flag, timeout);
        if(rc == sizeof(*messageHeader))
        {
            /* send message */
            rc = iPodPlayerIPCDoSend(fd, size, data, flag, timeout);
            if(rc == (S32)size)
            {
                rc = IPOD_PLAYER_IPC_OK;
                break;
            }
        }
        
        /* Sent data size was less than expected size */
        if(rc > 0)
        {
            /* Short wait and retry */
            nanosleep(&waitTime, NULL);
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_IPCLIB, rc);
            rc = IPOD_PLAYER_IPC_ERR_AGAIN;
        }
    }
    
    if(i >= IPOD_PLAYER_IPC_SEND_RETRY_MAX)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, i);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_IPCLIB, rc, i, timeout);
    
    return rc;

}

/*!
 * \fn iPodPlayerIPCSendSocket(S32* message_pointer)
 * \par INPUT PARAMETERS
 * U32 dataSize - size of data.<br>
 * U8* data - data.<br>
 * \par REPLY PARAMETERS
 * S32 result -
* \li \c <b> #IPOD_PLAYER_IPC_OK: send() success</b>
* \li \c <b> \ref iPodPlayerErrorCode : send() failed.</b>
 * \par DESCRIPTION
 * This Function request to send message to socket.(judgement how to send)
 */
S32 iPodPlayerIPCDoSendSocket(S32 handle, U8 *message, U32 size, U32 flag, S32 timeout)
{
    S32 rc                  = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    IPOD_PLAYER_MESSAGE_HEADER    messageHeader;
    S32 sendSize            = 0;
    S32 fd = -1;
    U32 bufSize = 0;
    U8 *buf = NULL;
    sem_t *semId = NULL;
    
    /* Parameter check */
    if((message == NULL) || (size == 0))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message, size);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Lock and get the ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        fd = info->id;
        bufSize = info->maxBufSize;
        semId = info->semId;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        if(bufSize > 0)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    
    /* Push for memory allocattion */
    pthread_cleanup_push((void *)iPodPlayerIPCFree, (void *)buf);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        buf = calloc(1, bufSize + sizeof(messageHeader));
        if(buf == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERR_NOMEM;
        }
    }
    
    /* Push for semaphore */
    /* PRQA: Lint Message 578: Lint describes that Declaration of symbol __clframe hides symbol __clframe.
    However there is no problem because __clframe was used as mcro in pthread_cleanup_push(). */
    pthread_cleanup_push((void *)iPodPlayerIPCDoSemPost, (void *)semId);       /*lint !e578 */
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        if(semId != NULL)
        {
            rc = iPodPlayerIPCDoSemWait((sem_t *)semId, IPOD_PLAYER_IPC_SEMWAIT_TMO);
        }
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* initialize message Header */
        memset(&messageHeader, 0, sizeof(messageHeader));
        memcpy(messageHeader.identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader.identify));
        if(bufSize > 0)
        {
            messageHeader.totalSeqNo = ((size + bufSize - 1) / bufSize) - 1 ;
        }
        else
        {
            messageHeader.totalSeqNo = 0;
        }
        messageHeader.totalMessageSize = size;
        sendSize = 0;
        
        /* iteration of sending */
        for(messageHeader.seqNo = 0; (messageHeader.seqNo <= messageHeader.totalSeqNo) && (sendSize < (S32)size); messageHeader.seqNo++)
        {
            /* calculate a message size */
            if(messageHeader.seqNo < messageHeader.totalSeqNo)
            {
                messageHeader.messageSize = bufSize;
            }
            else
            {
                messageHeader.messageSize = size - sendSize;
            }
            rc = iPodPlayerIPCSendOneMsg(fd, &messageHeader, messageHeader.messageSize, &message[sendSize], flag, timeout);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                sendSize += messageHeader.messageSize;
            }
            else
            {
                break;
            }
        }
        
        if(semId != NULL)
        {
            iPodPlayerIPCDoSemPost((sem_t *)semId);
        }
    }
    
    if(buf != NULL)
    {
        free((U8 *)buf);
        buf = NULL;
    }
    
    /* Pop because semaphore was unlocked  */
    pthread_cleanup_pop(0);
    /* Pop because memory was freed */
    pthread_cleanup_pop(0);
    
    if(sendSize == (S32)size)
    {
        rc = sendSize;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, size, sendSize);
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerIPCSendSocket(S32* message_pointer)
 * \par INPUT PARAMETERS
 * U32 dataSize - size of data.<br>
 * U8* data - data.<br>
 * \par REPLY PARAMETERS
 * S32 result -
* \li \c <b> #IPOD_PLAYER_IPC_OK: send() success</b>
* \li \c <b> \ref iPodPlayerErrorCode : send() failed.</b>
 * \par DESCRIPTION
 * This Function request to send message to socket.(judgement how to send)
 */
S32 iPodPlayerIPCDoLongSendSocket(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout)
{
    IPOD_PLAYER_MESSAGE_HEADER    messageHeader;
    S32 rc                  = IPOD_PLAYER_IPC_ERROR;
    S32 totalSize = 0;
    U32 i = 0;
    U32 dataArray = 0;
    U32 dataPos = 0;
    U32 copySize = 0;
    U32 dataSize = 0;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    S32 fd = -1;
    U32 bufSize = 0;
    U8 *buf = NULL;
    sem_t *semId = NULL;
    
    /* Parameter check */
    if((message == NULL) || (size == NULL) || (asize == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, info, message, size, asize);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Lock and get the ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        fd = info->id;
        bufSize = info->maxBufSize;
        semId = info->semId;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        if(bufSize > 0)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, handle);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop because semaphore was unlocked  */
    pthread_cleanup_pop(0);
    
    /* Check the buffer size */
    if(bufSize == 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, bufSize);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for memory allocation  */
    pthread_cleanup_push((void *)iPodPlayerIPCFree, (void *)buf);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        buf = calloc(1, bufSize + sizeof(messageHeader));
        if(buf == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERR_NOMEM;
        }
    }
    /* Push for semaphore */
    /* PRQA: Lint Message 578: Lint describes that Declaration of symbol __clframe hides symbol __clframe.
    However there is no problem because __clframe was used as mcro in pthread_cleanup_push(). */
    pthread_cleanup_push((void *)iPodPlayerIPCDoSemPost, (void *)semId);       /*lint !e578 */
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        if(semId != NULL)
        {
            rc = iPodPlayerIPCDoSemWait((sem_t *)semId, IPOD_PLAYER_IPC_SEMWAIT_TMO);
        }
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Calicurate the total send size */
        totalSize = 0;
        for(i = 0; i < dataNum; i++)
        {
            totalSize += size[i];
        }
        
        if(totalSize > 0)
        {
            /* initialize message Header */
            memset(&messageHeader, 0, sizeof(messageHeader));
            memcpy(messageHeader.identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader.identify));
            if(bufSize > 0)
            {
                messageHeader.totalSeqNo = (totalSize + bufSize - 1) / bufSize - 1;
            }
            else
            {
                messageHeader.totalSeqNo = 0;
            }
            messageHeader.totalMessageSize = totalSize;
            totalSize = 0;
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERROR;
        }
        
        for(messageHeader.seqNo = 0; (messageHeader.seqNo <= messageHeader.totalSeqNo) && (totalSize < messageHeader.totalMessageSize) &&
            (rc == IPOD_PLAYER_IPC_OK) && (dataArray < dataNum); messageHeader.seqNo++)
        {
            /* calculate a message size */
            if(messageHeader.seqNo < messageHeader.totalSeqNo)
            {
                messageHeader.messageSize = bufSize;
            }
            else
            {
                messageHeader.messageSize = messageHeader.totalMessageSize - totalSize;
            }
            
            /* Copy the data from application prepared buffer array to buffer for send */
            copySize = 0;
            for(dataSize = 0; dataSize < (U32)messageHeader.messageSize; dataSize += copySize)
            {
                if(buf != NULL)
                {
                    /* Data of one buffer is less than send buffer  */
                    if((S32)(size[dataArray] - dataPos) >= messageHeader.messageSize - (S32)dataSize)
                    {
                        copySize = messageHeader.messageSize - dataSize;
                        memcpy(&buf[dataSize], &message[dataArray][dataPos], copySize);
                        dataPos += copySize;
                    }
                    else
                    {
                        copySize = size[dataArray] - dataPos;
                        memcpy(&buf[dataSize], &message[dataArray][dataPos], copySize);
                        dataArray++;
                        dataPos = 0;
                    }
                }
                else
                {
                    break;
                }
            }
            
            /* Send the one header and data */
            rc = iPodPlayerIPCSendOneMsg(fd, &messageHeader, messageHeader.messageSize, buf, flag, timeout);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                totalSize += messageHeader.messageSize;
            }
        }
        
        if(semId != NULL)
        {
            iPodPlayerIPCDoSemPost((sem_t *)semId);
        }
        
        if(totalSize != messageHeader.totalMessageSize)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, messageHeader.totalMessageSize, totalSize);
        }
    }
    
    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
    
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    /* Pop because memory was freed */
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        *asize = totalSize;
    }
    
    return rc;
}

/* Send queue */
S32 iPodPlayerIPCDoSendQueue(S32 handle, U8 *message, U32 size, U32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    IPOD_PLAYER_PARAM_DESC_INFO *info = NULL;
    IPOD_PLAYER_MESSAGE_HEADER messageHeader;
    U32 sendSize = 0;
    S32 fd = -1;
    U32 bufSize = 0;
    U8 *buf = NULL;
    sem_t *semId = NULL;
    
    /* Check parameter */
    if((message == NULL) || (size == 0))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* For lint */
    flag = flag;
    timeout = timeout;
    
    /* Push for semaphore */
    pthread_cleanup_push((void *)iPodPlayerIPCUnrefIPCInfo, NULL);
    /* Lock and get ipc info */
    info = iPodPlayerIPCRefIPCInfo(handle);
    if(info != NULL)
    {
        fd = info->id;
        semId = info->semId;
        bufSize = info->maxBufSize;
        /* Unlock */
        iPodPlayerIPCUnrefIPCInfo(info);
        if(bufSize > 0)
        {
            rc = IPOD_PLAYER_IPC_OK;
        }
        else
        {
            rc = IPOD_PLAYER_IPC_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    /* Pop for semaphore */
    pthread_cleanup_pop(0);
    
    if(semId == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, semId);
        rc =IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Push for memory allocation */
    pthread_cleanup_push((void *)iPodPlayerIPCFree, (void *)buf);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        buf = calloc(1, bufSize + sizeof(messageHeader));
        if(buf == NULL)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
            rc = IPOD_PLAYER_IPC_ERR_NOMEM;
        }
    }
    
    /* PRQA: Lint Message 578: Lint describes that Declaration of symbol __clframe hides symbol __clframe.
    However there is no problem because __clframe was used as mcro in pthread_cleanup_push(). */
    pthread_cleanup_push((void *)iPodPlayerIPCDoSemPost, (void *)semId);       /*lint !e578 */
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = iPodPlayerIPCDoSemWait(semId, IPOD_PLAYER_IPC_SEMWAIT_TMO);
    }
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Initialize message Header */
        memset(&messageHeader, 0, sizeof(messageHeader));
        memcpy(messageHeader.identify, IPOD_PLAYER_SOCKET_IDENTIFY, sizeof(messageHeader.identify));
        if(bufSize > 0)
        {
            messageHeader.totalSeqNo = ((size + bufSize - 1) / bufSize) - 1 ;
        }
        else
        {
            messageHeader.totalSeqNo = 0;
        }
        messageHeader.totalMessageSize = size;
        sendSize = 0;
        
        for(messageHeader.seqNo = 0; (messageHeader.seqNo <= messageHeader.totalSeqNo) && (sendSize < size); messageHeader.seqNo++)
        {
            /* calculate a message size */
            if(messageHeader.seqNo < messageHeader.totalSeqNo)
            {
                messageHeader.messageSize = bufSize;
            }
            else
            {
                messageHeader.messageSize = size - sendSize;
            }
            
            if((buf != NULL) && (bufSize >= (U32)messageHeader.messageSize))
            {
                memcpy(buf, &messageHeader, sizeof(messageHeader));
                /* PRQA: Lint Message 826: This conversion is safe because the size of buf, sizeof messageHeader and remaining size is chekced before. So this memcpy is not problem */
                memcpy(&buf[sizeof(messageHeader)], &message[sendSize], messageHeader.messageSize); /*lint !e826 */
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, bufSize, messageHeader.messageSize);
                rc = IPOD_PLAYER_IPC_ERROR;
                break;
            }
            
            rc = mq_send(fd, (const char *)buf, sizeof(messageHeader) + messageHeader.messageSize, 0);
            if(rc == 0)
            {
                sendSize += messageHeader.messageSize;
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
                rc = IPOD_PLAYER_IPC_ERROR;
                break;
            }
        }
    }
    else
    {
        IPOD_ERR_WRITE_PTR(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, rc, semId);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    iPodPlayerIPCDoSemPost(semId);
    
    if(buf != NULL)
    {
        free(buf);
        buf = NULL;
    }
    /* Pop because semaphore was unlocked */
    pthread_cleanup_pop(0);
    /* Pop because memory was freed */
    pthread_cleanup_pop(0);
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = sendSize;
    }
    
    return rc;
}

S32 iPodPlayerIPCSetTimeout(U32 fd, S32 optName, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    struct timeval timeval;
    
    /* No timeout */
    if(timeout == -1)
    {
        timeval.tv_sec = 0;
        timeval.tv_usec = 0;
    }
    else
    {
        timeval.tv_sec = timeout / IPOD_PLAYER_IPC_MSEC;
        timeval.tv_usec = (timeout % IPOD_PLAYER_IPC_MSEC) * IPOD_PLAYER_IPC_MSEC;
    }
    
    /* Set the timeout */
    rc = setsockopt(fd, SOL_SOCKET, optName, &timeval, sizeof(timeval));
    if(rc == 0)
    {
        rc = IPOD_PLAYER_IPC_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, errno);
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    return rc;
}
/*!
 * \fn iPodPlayerIPCDoSend()
 * \par INPUT PARAMETERS
 * U32 size - send size.<br>
 * U8* message - send buffer pointer.<br>
 * \par REPLY PARAMETERS
 * S32 result -
* \li \c <b> #IPOD_PLAYER_IPC_OK: send() success</b>
* \li \c <b> \ref iPodPlayerErrorCode : send() failed.</b>
 * \par DESCRIPTION
 * This Function request to send message from socket. If it occurs send()-ret is error, this function continue send().
 */
static S32 iPodPlayerIPCDoSend(U32 sendSocket, U32 size, U8* message, U32 flag, S32 timeout)
{
    S32 rc = IPOD_PLAYER_IPC_ERROR;
    S32 i = 0;
    S32 sendSize = 0;
    U32 sendFlag = flag | MSG_NOSIGNAL;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPOD_PLAYER_IPC_INTR_RETRY_WAIT};
    
    /* Parameter check */
    if((size == 0) || (message == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_IPCLIB, IPOD_PLAYER_IPC_ERROR, message, size);
        return IPOD_PLAYER_IPC_ERROR;
    }
    
    /* Set the timeout for send */
    rc = iPodPlayerIPCSetTimeout(sendSocket, SO_SNDTIMEO, timeout);
    
    /* send process */
    sendSize = 0;
    for(i = 0; (i < IPOD_PLAYER_SEND_RETRY_NUM) && (rc == IPOD_PLAYER_IPC_OK); i++)
    {
        /* Send socket data */
        rc = send(sendSocket, message, size, sendFlag);
        /* Send success */
        if(rc >= 0)
        {
            sendSize += rc;
            rc = IPOD_PLAYER_IPC_OK;
            break;
        }
        else
        {
            /* Error occured by timeout */
            if((errno == EAGAIN) || (errno == EWOULDBLOCK) || (errno == EINPROGRESS))
            {
                rc = IPOD_PLAYER_IPC_TMO;
            }
            /* Error occurred by interrupt */
            else if((errno == EPIPE) || (errno == EINTR))
            {
                /* Short wait and retry */
                nanosleep(&waitTime, NULL);
                rc = IPOD_PLAYER_IPC_OK;
            }
            else
            {
                rc = IPOD_PLAYER_IPC_ERROR;
            }
        }
    }
    
    /* Loop was done by retry count */
    if(i >= IPOD_PLAYER_SEND_RETRY_NUM)
    {
        rc = IPOD_PLAYER_IPC_ERROR;
    }
    
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = sendSize;
    }
    else
    {
        IPOD_DLT_ERROR("send error :fd=%u, rc=%d, errno=%d, i=%d, sendSize=%d, size=%u", sendSocket, rc, errno, i, sendSize, size);
    }
    
    return rc;
}

