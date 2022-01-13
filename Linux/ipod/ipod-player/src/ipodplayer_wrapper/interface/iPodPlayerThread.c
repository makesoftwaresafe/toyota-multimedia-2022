/**
* \file: iPodPlayerMain.c
*
*
***************************************************************************** */
#include <pthread_adit.h>
#include "iPodPlayerLocal.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerThread.h"
#include "iPodPlayerData.h"
#include "iPodPlayerDebug.h"

#define IPOD_PLAYER_WAIT_EPOLL_RETRY_WAIT 100000000
/* thread id for callback function */
static pthread_t g_threadMain;
static int g_threadCreate = 0;

IPOD_PLAYER_REGISTER_CB_TABLE *g_cbTable = NULL;

void iPodPlayerRemoveThreadResource(void *rdata)
{
    S32 rc = IPOD_PLAYER_OK;
    
    S32 recvSockParam = 0;
    
    rc = iPodPlayerGetSocketInfo(&recvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER);
    if(rc == IPOD_PLAYER_OK)
    {
        iPodPlayerIPCClose(recvSockParam);
    }
    
    if(rdata != NULL)
    {
        free(rdata);
    }
    
    return;
}

/*!
 * \fn iPodPlayerThreadMain(void* arg)
 * \par INPUT PARAMETERS
 * void* arg<br>
 * \par REPLY PARAMETERS
 * S32 result -
 * \li \c <b> #IPOD_PLAYER_OK: Initialized success</b>
 * \li \c <b> \ref iPodPlayerErrorCode : Initialize failed.</b>
 * \par DESCRIPTION
 * This function initializes the iPodPlayer. Application can use all API after calling this function.
 */
void iPodPlayerThreadMain(void* args)
{
    S32 rc = IPOD_PLAYER_OK;
    U8 *rdata = NULL;
    S32 recvSockParam = -1;
    S32 longRecvSockParam = -1;
    S32 recvQueueParamLocal = -1;
    S32 handles[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    volatile U32 handleNum = 0;
    U32 checkNum = 0;
    S32 retHandle[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    U8 retType[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    S32 waitHandle = -1;
    U8 *dataArray[5] = {0};
    U32 recvSize[5] = {0};
    volatile U8 dataNum = 0;
    U32 asize = 0;
    U32 i = 0;
    U32 oneData = 0;
    IPOD_PLAYER_FUNC_HEADER funcHeader;
    U32 count = 0;
    struct timespec waitTime  = {0, IPOD_PLAYER_WAIT_EPOLL_RETRY_WAIT};
    IPOD_PLAYER_PARAM_TEMP localParam;
    
    
    memset(&localParam, 0, sizeof(localParam));
    if(args == NULL)
    {
        pthread_exit(0);
    }
    
    /* Initialize the structure */
    memset(&funcHeader, 0, sizeof(funcHeader));
    
    rc = iPodPlayerGetSocketInfo(&longRecvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG);
    
    if(rc == IPOD_PLAYER_OK)
    {
        handles[handleNum] = longRecvSockParam;
        handleNum += 1;
        rc = iPodPlayerGetSocketInfo(&recvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        handles[handleNum] = recvSockParam;
        handleNum += 1;
        rc = iPodPlayerGetSocketInfo(&recvQueueParamLocal, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER_LOCAL);
    }
    if(rc == IPOD_PLAYER_OK)
    {
        handles[handleNum] = recvQueueParamLocal;
        handleNum += 1;
        rdata = malloc(sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS));
        if(NULL == rdata)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            memset((U8*)rdata, 0, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS));
            dataArray[0] = (U8 *)&funcHeader;
            recvSize[0] = sizeof(funcHeader);
            dataNum++;
        }
        
        if(rc != IPOD_PLAYER_OK)
        {
            iPodPlayerIPCClose(recvSockParam);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&waitHandle, (U8)IPOD_PLAYER_OPEN_WAIT_HANDLE);
    }

    pthread_cleanup_push(iPodPlayerRemoveThreadResource, rdata);
    /* main loop */
    while(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerIPCWait(waitHandle, handleNum, handles, &checkNum, retHandle, retType, -1, IPOD_PLAYER_WAIT_IN);

        if(rc == IPOD_PLAYER_IPC_OK)
        {
            for(count = 0; count < checkNum; count++)
            {
                if(retType[count] == IPOD_PLAYER_OPEN_SOCKET_ACCEPT)
                {
                    if(rdata != NULL)
                    {
                        /* receive a socket data */
                        rc = iPodPlayerIPCReceive(retHandle[count], rdata, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS), 0, waitHandle, -1);
                        if(rc > 0)
                        {
                            oneData = sizeof(*rdata);
                            /* perform callback function */
                            rc = iPodPlayerCallback(&((IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)rdata)->paramTemp.header, 1, (U8 **)&rdata, &oneData, g_cbTable);
                        }
                        else
                        {
                            rc = IPOD_PLAYER_OK;
                        }
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERR_NOMEM;
                    }
                    
                }
                else if(retType[count] == IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG)
                {
                    rc = iPodPlayerIPCLongReceive(retHandle[count], dataNum, dataArray, recvSize, &asize, 0, waitHandle, -1);
                    if(rc == IPOD_PLAYER_IPC_ERR_NOMEM)
                    {
                        if(dataNum < 5)
                        {
                            dataArray[dataNum] = calloc(asize - recvSize[0], sizeof(U8));
                            if(dataArray[dataNum] != NULL)
                            {
                                recvSize[dataNum] = asize - recvSize[0];
                                dataNum++;
                                rc = IPOD_PLAYER_OK;
                            }
                        }
                        
                    }
                    else if(rc == IPOD_PLAYER_IPC_ERR_MORE_PACKET)
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                    else if(rc == IPOD_PLAYER_OK)
                    {
                        if(dataNum > 1)
                        {
                            /* perform callback function */
                            rc = iPodPlayerCallback((IPOD_PLAYER_FUNC_HEADER *)(void *)dataArray[0], (dataNum - 1), &dataArray[1], &recvSize[1], g_cbTable);
                            for(i = 1; i < dataNum; i++)
                            {
                                if(dataArray[i] != NULL)
                                {
                                    free(dataArray[i]);
                                    dataArray[i] = NULL;
                                }
                            }
                            dataNum = 1;
                        }
                    }
                    else
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                }
                else if(retType[count] == IPOD_PLAYER_OPEN_QUEUE_SERVER)
                {
                    /* handler of local queue */
                    if(retHandle[count] == recvQueueParamLocal)
                    {
                        /* receive a queue data */
                        rc = iPodPlayerIPCReceive(retHandle[count], (U8 *)&localParam, sizeof(localParam), 0, waitHandle, -1);
                        if((rc > 0) && (localParam.header.funcId == IPOD_FUNC_SHUTDOWN))
                        {
                            /* end main loop */
                            rc = IPOD_PLAYER_ERROR;
                            break;
                        }
                        else
                        {
                            rc = IPOD_PLAYER_OK;
                        }
                    }
                }
                else
                {
                    /* check next handle */
                }
            }
        }
        else
        {
            /* System error may be ocurred. Wait 100ms and retry wait */
            nanosleep(&waitTime, 0);
            rc = IPOD_PLAYER_OK;
        }
    }
    pthread_cleanup_pop(0);
    
    if(rdata != NULL)
    {
        free(rdata);
        rdata = NULL;
    }
    pthread_exit(0);
}


S32 iPodPlayerThreadCreate(U8 *name, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable)
{
    S32 rc = IPOD_PLAYER_OK;
    
    if((name == NULL) || (cbTable == NULL))
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(g_cbTable != NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    g_cbTable = calloc(1, sizeof(IPOD_PLAYER_REGISTER_CB_TABLE));
    if(g_cbTable == NULL)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        memcpy(g_cbTable, cbTable, sizeof(IPOD_PLAYER_REGISTER_CB_TABLE));
        
        /* create a message receive thread */
        rc = pthread_create(&g_threadMain, NULL, (void *(*)(void *))iPodPlayerThreadMain, (void*)name);
        if(rc != IPOD_PLAYER_OK)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            g_threadCreate = 1;
        }
    }
    
    return rc;
}


S32 iPodPlayerThreadDestroy(void)
{
    S32 thread_return = 0;
    void *pRetValue = (void *)&thread_return;
    S32 rc = IPOD_PLAYER_ERROR;
    S32 sendQueueParamLocal = -1;                   /* for send queue handle    */
    IPOD_PLAYER_PARAM_TEMP param;                   /* for send queue parameter */
    
    /* initialize prameter */
    memset(&param, 0, sizeof(param));
    
    if(g_threadCreate != 0)
    {
        /* get local send queue handle */
        rc = iPodPlayerGetSocketInfo(&sendQueueParamLocal, (U8)IPOD_PLAYER_OPEN_QUEUE_CLIENT_LOCAL);
        if(IPOD_PLAYER_OK != rc)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Set the shutdown command */
            param.header.funcId = IPOD_FUNC_SHUTDOWN;
            rc = iPodPlayerIPCSend(sendQueueParamLocal, (U8 *)&param, sizeof(param), 0, -1);
            if(rc == sizeof(param))
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        if(rc != IPOD_PLAYER_OK)
        {
            /* force to destroy callback thread */
            rc = pthread_cancel(g_threadMain);
        }
        rc = pthread_join(g_threadMain, &pRetValue);
        if(rc != 0)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = thread_return;
        }
        g_threadCreate = 0;
        
    }

    if(g_cbTable != NULL)
    {
        free(g_cbTable);
        g_cbTable = NULL;
    }
    
    return rc;
}

