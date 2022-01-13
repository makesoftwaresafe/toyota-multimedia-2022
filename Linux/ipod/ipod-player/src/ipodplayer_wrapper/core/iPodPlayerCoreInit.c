#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/eventfd.h>
#include <mcheck.h>
#include <getopt.h>

#include <iap1_dlt_log.h>
#include "iap2_dlt_log.h"
#include "adit_typedef.h"
#include "pthread_adit.h"
#include "iPodPlayerUtilityLog.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreCommonFunc.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerCoreFunc.h"
#include "iPodPlayerCoreCmdHandler.h"
#include "iPodPlayerCoreCfg.h"
#include "ipp_mainloop_common.h"
#include "ipp_iap2_callback.h"
#include "ipp_iap2_ctrlcfglist.h"

iAP2Service_t* g_service = NULL;
IPOD_PLAYER_CORE_CFG *g_iPodCoreCfg = NULL;
static volatile sig_atomic_t g_iPodCoreShutdown = -1;
static volatile sig_atomic_t g_iPodCoreIAP2SWaitExit = -1;
static volatile sig_atomic_t g_iPodCoreShutdownError = -1;

static IPOD_PLAYER_CORE_CFG *iPodCoreCreateResource(void);
void iPodCoreDeleteThread(IPOD_PLAYER_CORE_CFG *iPodCfg, U8 cancel);
S32 iPodCoreDeleteResource(IPOD_PLAYER_CORE_CFG *iPodCfg);

static void iPodCoreSigTerm(S32 para)
{
    U64 data = 1;
    ssize_t ret = 0;
    
    /* for lint */
    para = para;
    
    if(g_iPodCoreShutdown != -1)
    {
        ret = write(g_iPodCoreShutdown, &data, sizeof(data));
        if (ret < 0)
        {
            g_iPodCoreShutdownError = 1;    /* error (cannot output log directly in handler) */
        }
    }
    g_iPodCoreIAP2SWaitExit = 1;
    
    return;
}

/* Search requested application */
IPOD_PLAYER_CORE_APP_INFO *iPodCoreGetAppInfo(IPOD_PLAYER_CORE_CFG *iPodCfg, U32 appID)
{
    U32 i = 0;
    U32 num = 0;
    IPOD_PLAYER_CORE_APP_INFO *appInfo = NULL;
    
    /* Check the parameter */
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return NULL;
    }
    
    if(iPodCfg->appInfo == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg->appInfo);
        return NULL;
    }
    
    /* get max number of app */
    num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_APP_MAX_NUM);
    /* Serch the application information from appID */
    for(i = 0; i < num; i++)
    {
        /* Application has connected */
        if(iPodCfg->appInfo[i].isReady != 0)
        {
            /* Application ID matches to appID */
            if(iPodCfg->appInfo[i].appID == appID)
            {
                appInfo = &iPodCfg->appInfo[i];
                break;
            }
        }
    }
    
    if(appInfo != NULL)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    }
    
    return appInfo;
}

IPOD_PLAYER_CORE_APP_INFO *iPodCoreGetAppInfoWithHandle(IPOD_PLAYER_CORE_CFG *iPodCfg, S32 handle)
{
    U32 i = 0;
    U32 num = 0;
    IPOD_PLAYER_CORE_APP_INFO *appInfo = NULL;
    
    /* Check the parameter */
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return NULL;
    }
    
    if(iPodCfg->appInfo == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg->appInfo);
        return NULL;
    }
    
    
    /* get max number of app */
    num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_APP_MAX_NUM);
    /* Serch the application information from Handle */
    for(i = 0; i < num; i++)
    {
        /* Application has connected */
        if(iPodCfg->appInfo[i].isReady != 0)
        {
            /* Application ID matches to appID */
            if(iPodCfg->appInfo[i].sckOut == handle)
            {
                appInfo = &iPodCfg->appInfo[i];
                break;
            }
        }
    }
    
    if(appInfo != NULL)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    }
    
    return appInfo;

}

/* Search requested iPod */
IPOD_PLAYER_CORE_THREAD_INFO *iPodCoreGetiPodInfo(IPOD_PLAYER_CORE_CFG *iPodCfg, U32 iPodID)
{
    U32 i = 0;
    U32 num = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *iPodInfo = NULL;
    
    /* Check the paramter */
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return NULL;
    }
    
    if(iPodCfg->threadInfo == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg->threadInfo);
        return NULL;
    }
    
    /* get max number of dev */
    num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
    /* Serch iPodInformation */
    for(i = 0; i < num; i++)
    {
        /* iPod has connected */
        if(iPodCfg->threadInfo[i].isReady != 0)
        {
            /* appDevID matches with iPodID */
            if(iPodCfg->threadInfo[i].appDevID == iPodID)
            {
                /* Set matched thread */
                iPodInfo = &iPodCfg->threadInfo[i];
                break;
            }
        }
    }
    
    if(iPodInfo != NULL)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    }
    
    return iPodInfo;
}

/* Check to change the status */
S32 iPodCoreCheckStatusChange(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_THREAD_INFO *iPodInfo = NULL;
    IPOD_PLAYER_PLAYBACK_STATUS *playbackStatus = NULL;
    
    /* Parameter check */
    if((iPodCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    playbackStatus = &contents->notifyPlaybackStatus.status;
    
    iPodInfo = iPodCoreGetiPodInfo(iPodCfg, contents->paramTemp.header.devID);
    if(iPodInfo != NULL)
    {
        /* Update the status */
        iPodInfo->playbackStatus.status = playbackStatus->status;
        iPodInfo->playbackStatus.track.index = playbackStatus->track.index;
        iPodInfo->playbackStatus.track.time = playbackStatus->track.time;
        iPodInfo->playbackStatus.chapter.index = playbackStatus->chapter.index;
        iPodInfo->playbackStatus.chapter.time = playbackStatus->chapter.time;
        rc = IPOD_PLAYER_OK;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreNotifyDeviceStatusToApp(IPOD_PLAYER_CORE_APP_INFO *appInfo, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo, 
                                    U8 iPodNum, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Check the paramter */
    if((appInfo == NULL) || (threadInfo == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, appInfo, threadInfo, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Loop until maximum number of iPod */
    for(i = 0; (i < iPodNum); i++)
    {
        /* Search created thread */
        if(threadInfo[i].isReady == 0)
        {
            /* Thread is not created. It means iPod is not connected. */
            /* Set the notify connection status command with disconnect */
            contents->notifyConnectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
            contents->notifyConnectionStatus.header.devID = i + 1;
            contents->notifyConnectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT;
            contents->notifyConnectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_UNKNOWN;
            contents->notifyConnectionStatus.status.powerStatus = IPOD_PLAYER_POWER_UNKNOWN;
            contents->notifyConnectionStatus.status.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
            contents->notifyConnectionStatus.status.deviceType = IPOD_PLAYER_DEVICE_TYPE_UNKNOWN;    
            
            /* Send the device status to Application */
            rc = iPodPlayerIPCSend(appInfo->sckOut, (U8 *)contents, sizeof(IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS), 0, IPODCORE_SEND_TMOUT);
            if(rc == sizeof(IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS))
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, appInfo->sckOut);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            /* Thread is created. It means iPod is connected. */
            /* In this case, thread notifies connection status */
            rc = IPOD_PLAYER_OK;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreNotifyDeviceDetectionToApp(IPOD_PLAYER_CORE_APP_INFO *appInfo, U8 appNum, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Parameter check */
    if((appInfo == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, appInfo, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Loop until maximum number of application */
    for(i = 0; i < appNum; i++)
    {
        /* Application is connected */
        if(appInfo[i].isReady != 0)
        {
            /* Notify the device detection to connected Application */
            rc = iPodPlayerIPCSend(appInfo[i].sckOut, (U8 *)contents, size, 0, IPODCORE_SEND_TMOUT);
            if(rc == (S32)size)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreSendToApp(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 rc2 = IPOD_PLAYER_OK;
    U32 i = 0;
    U32 num = 0;
    U8 *sendData[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    U8 sendNum = IPODCORE_LONG_DATA_ARRAY;
    U32 asize = 0;
    U32 checkId = 0;
    
    /* Check Parameter */
    if((iPodCfg == NULL) || (header == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* get max number of app */
    num = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_APP_MAX_NUM);
    /* Loop until maximum Application number */
    for(i = 0; i < num; i++)
    {
        /* Application is connected */
        if(iPodCfg->appInfo[i].isReady != 0)
        {
            checkId = (U32)header->funcId;
            /* Connected Application is equal with requested Application Or */
            /* Command ID which send to Application is equal with notification */
            /* If command is notification, it must send to all Application */
            if((iPodCfg->appInfo[i].appID == header->appID) ||
              ((checkId >= (U32)IPOD_FUNC_NOTIFY_PLAYBACK_STATUS) && (checkId <= (U32)IPOD_FUNC_NOTIFY_DEVICE_EVENT)))
            {
                /* Data send by short send */
                if(header->longData == 0)
                {
                    /* Send the result to connected Application */
                    rc = iPodPlayerIPCSend(iPodCfg->appInfo[i].sckOut, (U8 *)contents, size, 0, IPODCORE_SEND_TMOUT);
                    if(rc == (S32)size)
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                /* Data send by long send */
                else
                {
                    if(size >= sizeof(*header))
                    {
                        sendData[IPODCORE_POS0] = (U8 *)header;
                        sendSize[IPODCORE_POS0] = sizeof(*header);
                        sendData[IPODCORE_POS1] = (U8 *)contents;
                        sendSize[IPODCORE_POS1] = size - sizeof(*header);
                        rc = iPodPlayerIPCLongSend(iPodCfg->appInfo[i].sckOutLong, sendNum, sendData, sendSize, 
                                                    &asize, 0, IPODCORE_SEND_TMOUT);
                        if(rc == IPOD_PLAYER_IPC_OK)
                        {
                            if((sendSize[IPODCORE_POS0] + sendSize[IPODCORE_POS1]) == asize)
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                            else
                            {
                                rc = IPOD_PLAYER_ERROR;
                            }
                        }
                        else
                        {
                            rc = IPOD_PLAYER_ERROR;
                        }
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, size, sizeof(*header));
                    }
                }
            }
            
            if(rc != IPOD_PLAYER_OK)
            {
                rc2 = rc;
            }
        }
    }
    
    if(rc2 != IPOD_PLAYER_OK)
    {
        rc = rc2;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Connect to Application */
S32 iPodCoreInit(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 appSend = -1;
    S32 appSendLong = -1;
    U32 appNum = 0;
    U32 devNum = 0;
    U32 maxMsgSize = 0;
    U32 maxPktSize = 0;
    U32 i = 0;
    U32 sendSize = 0;
    IPOD_PLAYER_CORE_APP_INFO *appInfo = NULL;
    IPOD_PLAYER_IPC_OPEN_INFO info;
    U8 long_path[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    U8 semTemp[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    struct epoll_event epollEvent;
    U32 retlen = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg, contents, size);
    
    /* Check Parameter */
    if((iPodCfg == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    memset(&epollEvent, 0, sizeof(epollEvent));

    /* get max number of app */
    appNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_APP_MAX_NUM);
    devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
    maxMsgSize = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_MSG_SIZE);
    maxPktSize = maxMsgSize;
    /* Connect with Application via socket for sending the result or notification */
    info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
    info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
    info.identify = contents->init.uniqueName;
    info.connectionNum = 1;
    info.semName = NULL;
    info.maxBufSize = maxMsgSize;
    info.maxPacketSize = maxPktSize;
    rc = iPodPlayerIPCOpen(&appSend, &info);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Connect with Application via socket for sending the large data */
        info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        retlen = snprintf((char *)long_path, sizeof(long_path), "%s_long", contents->init.uniqueName);
        if(retlen <= 0 || sizeof(long_path) <= retlen)
            IPOD_DLT_WARN("Unalbe to generate %s_long",contents->init.uniqueName);
        //Ensuring path is generated via snprintf, as a fix to avoid format-truncation error 
        retlen = snprintf((char *)semTemp, sizeof(semTemp), "/tmp%s_long", contents->init.uniqueName);
        if(retlen <= 0 || sizeof(long_path) <= retlen)
            IPOD_DLT_WARN("Unalbe to generate /tmp%s_long",contents->init.uniqueName);
        
        info.identify = long_path;
        info.connectionNum = 1;
        info.semName = semTemp;
        info.maxBufSize = maxMsgSize;
        info.maxPacketSize = maxPktSize;
        rc = iPodPlayerIPCOpen(&appSendLong, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Check the current connected application */
            appInfo = iPodCoreGetAppInfo(iPodCfg, contents->paramTemp.header.appID);
            if(appInfo == NULL)
            {
                /* Loop until maximum number of application */
                for(i = 0; i < appNum; i++)
                {
                    /* Application is not connected in the current table */
                    if(iPodCfg->appInfo[i].isReady == 0)
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCfg->appInfo[i].appID);
                        iPodCfg->appInfo[i].isReady = 1;
                        iPodCfg->appInfo[i].appID = contents->paramTemp.header.appID;
                        /* Get Application info */
                        appInfo = &iPodCfg->appInfo[i];
                        rc = IPOD_PLAYER_OK;
                        break;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERR_MAX_APP_CONNECT;
                    }
                }
            }
            else
            {
                appInfo = NULL;
                rc = IPOD_PLAYER_ERR_APP_ALREADY_CONNECT;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Mask check */
        if(contents->init.connectionMask <= (IPOD_PLAYER_USE_DEVICE_USB + IPOD_PLAYER_USE_DEVICE_BT + IPOD_PLAYER_USE_DEVICE_UART))
        {
            for(i = 0; i < devNum; i++)
            {
                /* Thread is created. */
                if(iPodCfg->threadInfo[i].isReady != 0)
                {
                    /* Send the Init commands to each created thread */
                    rc = iPodPlayerIPCSend(iPodCfg->threadInfo[i].queueInfoClient, (U8 *)contents, size, 0, IPODCORE_SEND_TMOUT);
                    if(rc == (S32)size)
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, contents->init.connectionMask);
        }
    }
    
    if(appSend != -1)
    {
        /* Set result command */
        contents->initResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_INIT_RESULT;
        contents->initResult.result = rc;
        sendSize = sizeof(IPOD_PLAYER_PARAM_INIT_RESULT);
        rc = iPodPlayerIPCSend(appSend, (U8 *)contents, sendSize, 0, IPODCORE_SEND_TMOUT);
        if(rc == (S32)sendSize)
        {
            /* Register the application IPC information */
            if(appInfo != NULL)
            {
                appInfo->sckOut = appSend;
                appInfo->sckOutLong = appSendLong;
                rc = iPodCoreNotifyDeviceStatusToApp(appInfo, iPodCfg->threadInfo, devNum, contents);
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = contents->initResult.result;
                }
                
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        /* Remove all of opened application information */
        if(appSend != -1)
        {
            iPodCoreDelFDs(iPodCfg->waitHandle, iPodCfg->handleNum, iPodCfg->handle);
            iPodCoreEpollCtl(iPodCfg->waitHandle, appSend, EPOLL_CTL_ADD, EPOLLERR | EPOLLHUP);
            rc = epoll_wait(iPodCfg->waitHandle, &epollEvent, 1, IPOD_PLAYER_WAIT_IN);
            iPodPlayerIPCClose(appSend);
            iPodCoreAddFDs(iPodCfg->waitHandle, iPodCfg->handleNum, iPodCfg->handle);
        }
        
        if(appSendLong != -1)
        {
            iPodPlayerIPCClose(appSendLong);
        }
        
        if(appInfo != NULL)
        {
            /* Target socket is removed from epoll */
            appInfo->appID = 0;
            appInfo->isReady = 0;
            appInfo->sckOut = -1;
            appInfo->sckOutLong = -1;
        }
        
        rc = IPOD_PLAYER_ERROR;
    }

    IPOD_DLT_INFO("appID=%u, rc=%d, appSend=%d, appSendLong=%d", contents->paramTemp.header.appID, rc, appSend, appSendLong);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}



/* Disconnect the Application */
S32 iPodCoreDeinit(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 sendSize = 0;
    IPOD_PLAYER_CORE_APP_INFO *appInfo = NULL;
    struct epoll_event epollEvent;
    U32 appIDforLog = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg, contents, size);
    
    /* Check Parameter */
    if((iPodCfg == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&epollEvent, 0, sizeof(epollEvent));
    
    /* Search deinit application */
    appInfo = iPodCoreGetAppInfo(iPodCfg, contents->paramTemp.header.appID);
    if(appInfo != NULL)
    {
        /* Set the result command */
        contents->deinitResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_DEINIT_RESULT;
        contents->deinitResult.result = IPOD_PLAYER_OK;
        sendSize = sizeof(IPOD_PLAYER_PARAM_DEINIT_RESULT);
        rc = iPodPlayerIPCSend(appInfo->sckOut, (U8 *)contents, sendSize, 0, IPODCORE_SEND_TMOUT);
        if(rc == (S32)sendSize)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR);
            rc = IPOD_PLAYER_ERROR;
        }
        /* Target socket is removed from epoll */
        appIDforLog = appInfo->appID;
        appInfo->appID = 0;
        appInfo->isReady = 0;
        
        iPodCoreDelFDs(iPodCfg->waitHandle, iPodCfg->handleNum, iPodCfg->handle);
        iPodCoreEpollCtl(iPodCfg->waitHandle, appInfo->sckOut, EPOLL_CTL_ADD, EPOLLERR | EPOLLHUP);
        rc = epoll_wait(iPodCfg->waitHandle, &epollEvent, 1, IPOD_PLAYER_WAIT_IN);
        iPodPlayerIPCClose(appInfo->sckOut);
        iPodPlayerIPCClose(appInfo->sckOutLong);
        appInfo->sckOut = -1;
        appInfo->sckOutLong = -1;
        iPodCoreAddFDs(iPodCfg->waitHandle, iPodCfg->handleNum, iPodCfg->handle);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }

    IPOD_DLT_INFO("appID=%u, rc=%d", appIDforLog, rc);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Disconnect the Application */
S32 iPodCoreTestReady(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 sendSize = 0;
    IPOD_PLAYER_CORE_APP_INFO *appInfo = NULL;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg, contents, size);
    
    /* Check Parameter */
    if((iPodCfg == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    appInfo = iPodCoreGetAppInfo(iPodCfg, contents->paramTemp.header.appID);
    if(appInfo != NULL)
    {
        /* Set the result command */
        contents->testReadyResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_TEST_READY_RESULT;
        contents->testReadyResult.result = IPOD_PLAYER_OK;
        sendSize = sizeof(contents->testReadyResult);
        rc = iPodPlayerIPCSend(appInfo->sckOut, (U8 *)contents, sendSize, 0, IPODCORE_TMOUT_FOREVER);
        if(rc == (S32)sendSize)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreDeviceDetection(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION devInfo;
    IPOD_PLAYER_CORE_APP_INFO *appInfo = NULL;
    U32 sendSize = 0;
    U32 appNum = 0;
    U32 devNum = 0;
    S32 i = 0;
    U8 s[8] = {};
    U8 logstr[8 * 16] = {};
    
    /* Check parameter */
    if((iPodCfg == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&devInfo, 0, sizeof(devInfo));
    memcpy((char *)&devInfo, (const char *)&contents->setDeviceDetection, sizeof(devInfo));
    
    appNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_APP_MAX_NUM);
    devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);

    IPOD_DLT_INFO("devID=%u(0x%x), devType=%d, detectType=%d, devPath=%s, audioPath=%s, endPointPath=%s",
                    devInfo.header.devID, devInfo.header.devID, devInfo.info.devType, devInfo.info.detectType,
                    devInfo.info.devPath, devInfo.info.audioPath, devInfo.info.endPointPath);
    if(devInfo.info.detectType == IPOD_PLAYER_DETECTION_TYPE_CONNECT)
    {
        for(i = 0; i < devInfo.devInfo.macCount; i++)
        {
            IPOD_DLT_INFO("devInfo :macAddr[%u]=%02x:%02x:%02x:%02x:%02x:%02x", i, devInfo.macAddr[i].addr[0], devInfo.macAddr[i].addr[1], devInfo.macAddr[i].addr[2], devInfo.macAddr[i].addr[3], devInfo.macAddr[i].addr[4], devInfo.macAddr[i].addr[5]);
        }
        if((devInfo.accInfo.MsgSentByAccSize != IPOD_PLAYER_ACCINFO_NULL) || (devInfo.accInfo.MsgRecvFromDeviceSize != IPOD_PLAYER_ACCINFO_NULL))
        {
            IPOD_DLT_INFO("accInfo :SupportedIOSInTheCar=%u, SupportediOSAppCount=%u, MsgSentByAccSize=%d, MsgRecvFromDeviceSize=%d",
                            devInfo.accInfo.SupportedIOSInTheCar, devInfo.accInfo.SupportediOSAppCount,
                            devInfo.accInfo.MsgSentByAccSize, devInfo.accInfo.MsgRecvFromDeviceSize);
        }
        for(i = 0; i < (devInfo.accInfo.MsgSentByAccSize / 2); i++)
        {
            snprintf((char *)s, 8, "%04x, ", devInfo.accInfo.MsgSentByAcc[i]);
            strncat((char *)logstr, (const char *)s, 8);
            if((i & 0xf) == 0xf)
            {
                IPOD_DLT_INFO("accInfo :MsgSentByAcc=%s", logstr);
                logstr[0] = '\0';
            }
        }
        if(logstr[0] != '\0')
        {
            IPOD_DLT_INFO("accInfo :MsgSentByAcc=%s", logstr);
            logstr[0] = '\0';
        }
        for(i = 0; i < (devInfo.accInfo.MsgRecvFromDeviceSize / 2); i++)
        {
            snprintf((char *)s, 8, "%04x, ", devInfo.accInfo.MsgRecvFromDevice[i]);
            strncat((char *)logstr, (const char *)s, 8);
            if((i & 0xf) == 0xf)
            {
                IPOD_DLT_INFO("accInfo :MsgRecvFromDevice=%s", logstr);
                logstr[0] = '\0';
            }
        }
        if(logstr[0] != '\0')
        {
            IPOD_DLT_INFO("accInfo :MsgRecvFromDevice=%s", logstr);
            logstr[0] = '\0';
        }
        if(devInfo.vehicleInfo.displayName_valid)
        {
            IPOD_DLT_INFO("vehicleInfo :displayName=%s", devInfo.vehicleInfo.displayName);
        }
    }
    
    appInfo = iPodCoreGetAppInfo(iPodCfg, contents->paramTemp.header.appID);
    if(appInfo != NULL)
    {
        /* Set the result command */
        contents->setDeviceDetectionResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_SET_DEVICE_DETECTION_RESULT;
        contents->setDeviceDetectionResult.result = IPOD_PLAYER_OK;
        sendSize = sizeof(contents->setDeviceDetectionResult);
        rc = iPodPlayerIPCSend(appInfo->sckOut, (U8 *)contents, sendSize, 0, IPODCORE_SEND_TMOUT);
        if(rc == (S32)sendSize)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc, sendSize);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    contents->notifyConnectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
    contents->notifyConnectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_UNKNOWN;
    contents->notifyConnectionStatus.status.powerStatus = IPOD_PLAYER_POWER_UNKNOWN;
    contents->notifyConnectionStatus.status.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
    contents->notifyConnectionStatus.status.deviceType = devInfo.info.devType;
    
    if(devInfo.info.detectType == IPOD_PLAYER_DETECTION_TYPE_CONNECT )
    {
        IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devInfo.info.devPath);
        if(iPodCfg->curiPodNum < devNum)
        {
            rc = iPodCoreCreateiPodCtrlHandler(iPodCfg, &devInfo, devInfo.header.devID);
            if(rc == IPOD_PLAYER_OK)
            {
                iPodCfg->curiPodNum++;
                contents->notifyConnectionStatus.header.devID = devInfo.header.devID;
                contents->notifyConnectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
                rc = iPodCoreNotifyDeviceDetectionToApp(iPodCfg->appInfo, appNum, contents, sizeof(IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS));
            }
        }
    }
    else
    {
        IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devInfo.info.devPath);
        rc = iPodCoreDeleteiPodCtrlHandler(iPodCfg, &devInfo, devInfo.header.devID);
        if(rc == IPOD_PLAYER_OK)
        {
            if(iPodCfg->curiPodNum > 0)
            {
                iPodCfg->curiPodNum--;
            }
            contents->notifyConnectionStatus.header.devID = devInfo.header.devID;
            contents->notifyConnectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT;
            rc = iPodCoreNotifyDeviceDetectionToApp(iPodCfg->appInfo, appNum, contents, sizeof(IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS));
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}



S32 iPodCoreSendQueue(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 *queueInfo = NULL;
    U32 count = 0;
    U8 *sendData[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    U8 sendNum = IPODCORE_LONG_DATA_ARRAY;
    U32 asize = 0;
    U32 devNum = 0;
    
    /* Check Parameter */
    if((iPodCfg == NULL) || (header == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Size set the size of message data header */
    switch(header->funcId)
    {
        case IPOD_FUNC_INIT:
            rc = iPodCoreInit(iPodCfg, contents, size);
            break;
            
        case IPOD_FUNC_DEINIT:
            rc = iPodCoreDeinit(iPodCfg, contents, size);
            break;
        case IPOD_FUNC_TEST_READY:
            rc = iPodCoreTestReady(iPodCfg, contents, size);
            break;
        case IPOD_FUNC_SET_IOS_APPS_INFO:
            rc = IPOD_PLAYER_OK;
            break;
        case IPOD_FUNC_SET_DEVICE_DETECTION:
            rc = iPodCoreDeviceDetection(iPodCfg, contents);
            break;
        case IPOD_FUNC_SHUTDOWN:
            rc = IPOD_PLAYER_ERR_SHUTDOWN;
            break;
        default:
            devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
            /* Application sends the iPodPlayer API */
            for(count = 0; count < devNum; count++)
            {
                /* Search indicated iPod */
                if((iPodCfg->threadInfo[count].isReady != 0) && (header->devID == iPodCfg->threadInfo[count].appDevID))
                {
                    if(header->longData == 0)
                    {
                        queueInfo = &iPodCfg->threadInfo[count].queueInfoClient;
                    }
                    else
                    {
                        queueInfo = &iPodCfg->threadInfo[count].longQueueClient;
                        sendData[0] = (U8 *)header;
                        sendSize[0] = sizeof(*header);
                        sendData[1] = (U8 *)contents;
                        sendSize[1] = size - sizeof(*header);
                    }
                    break;
                }
            }
            
            if(queueInfo == NULL)
            {
                contents->paramResultTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | header->funcId);
                contents->paramResultTemp.header.devID = header->devID;
                contents->paramResultTemp.header.appID = header->appID;
                header->longData = 0;
                contents->paramResultTemp.result = IPOD_PLAYER_ERR_NOT_CONNECT;
                size = sizeof(contents->paramResultTemp);
                queueInfo = &iPodCfg->cmdQueueInfoClient;
            }
            rc = IPOD_PLAYER_OK;
            break;
    }


    if(rc == IPOD_PLAYER_OK)
    {
        if(queueInfo != NULL)
        {
            if(header->longData == 0)
            {
                /* Send to indicated messege queue */
                rc = iPodPlayerIPCSend(*queueInfo, (U8 *)contents, size, 0, IPODCORE_SEND_TMOUT);
                if(rc == (S32)size)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                rc = iPodPlayerIPCLongSend(*queueInfo, sendNum, sendData, sendSize, &asize, 0, IPODCORE_SEND_TMOUT);
                if(rc == IPOD_PLAYER_IPC_OK)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}

S32 iPodCoreCmdHandlerDataAnalyze(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 funcId = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *threadInfo = NULL;
    IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION devInfo;
    
    /* Check parameter */
    if((iPodCfg == NULL) || (header == NULL) || (contents == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, header, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the parameter */
    memset(&devInfo, 0, sizeof(devInfo));
    
    funcId = (U32)header->funcId;

    if((funcId & IPOD_PLAYER_RESULT_ID_MASK) != IPOD_PLAYER_RESULT_ID_MASK)
    {
        rc = iPodCoreSendQueue(iPodCfg, header, contents, size);
    }
    else if(funcId == IPOD_FUNC_NOTIFY_PLAYBACK_STATUS)
    {
        rc = iPodCoreCheckStatusChange(iPodCfg, contents);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreSendToApp(iPodCfg, header, contents, size);
        }
    }
    else if(funcId == IPOD_FUNC_NOTIFY_CONNECTION_STATUS)
    {
        /* Disconnect was notified */
        if(contents->notifyConnectionStatus.status.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT)
        {
            threadInfo = iPodCoreGetiPodInfo(iPodCfg, contents->notifyConnectionStatus.header.devID);
            /* Thread is still remaing. */
            if(threadInfo != NULL)
            {
                devInfo.header.funcId = contents->notifyConnectionStatus.header.funcId;
                devInfo.header.devID = contents->notifyConnectionStatus.header.devID;
                devInfo.header.appID = contents->notifyConnectionStatus.header.appID;
                devInfo.info.devType = threadInfo->nameInfo.devType;
                devInfo.info.detectType = IPOD_PLAYER_DETECTION_TYPE_DISCONNECT;
                strncpy((char *)devInfo.info.devPath, (const char *)threadInfo->nameInfo.deviceName, sizeof(devInfo.info.devPath) - 1);
                strncpy((char *)devInfo.info.audioPath, (const char *)threadInfo->nameInfo.audioInName, sizeof(devInfo.info.audioPath) -1);
                /* Remove the thread */
                rc = iPodCoreDeleteiPodCtrlHandler(iPodCfg, &devInfo, devInfo.header.devID);
                if(rc == IPOD_PLAYER_OK)
                {
                    if(iPodCfg->curiPodNum > 0)
                    {
                        iPodCfg->curiPodNum--;
                    }
                    /* Notify the status. If thread has already been removed, disconnect status also has already been notified. */
                    rc = iPodCoreSendToApp(iPodCfg, header, contents, size);
                }
            }
            
        }
        else
        {
           rc = iPodCoreSendToApp(iPodCfg, header, contents, size); 
        }
    }
    else
    {
        rc = iPodCoreSendToApp(iPodCfg, header, contents, size);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc, funcId);
    
    return rc;
}

S32 iPodCoreDeleteResource(IPOD_PLAYER_CORE_CFG *iPodCfg)
{
    S32 rc = IPOD_PLAYER_OK;
        
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg);
    
    
    /* Check Parameter */
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCfg->contents != NULL)
    {
        free(iPodCfg->contents);
        iPodCfg->contents = NULL;
    }
    
    if(iPodCfg->detectInfo != NULL)
    {
        free(iPodCfg->detectInfo);
        iPodCfg->detectInfo = NULL;
    }
    

    iPodPlayerIPCClose(iPodCfg->serverInfo);
    iPodCoreClearHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->serverInfo);

    if(iPodCfg->threadInfo != NULL)
    {
        free(iPodCfg->threadInfo);
        iPodCfg->threadInfo = NULL;
    }
    
    /* deinitialize thread information stack */
    iPodCoreiPodCtrlDeInitThreadInfoStack();
    
    if(iPodCfg->cmdQueueInfo >= 0)
    {
        iPodPlayerIPCClose(iPodCfg->cmdQueueInfoClient);
        iPodPlayerIPCClose(iPodCfg->cmdQueueInfo);
        iPodPlayerIPCClose(iPodCfg->longQueueServer);
        iPodPlayerIPCClose(iPodCfg->longQueueClient);
        iPodPlayerIPCClose(iPodCfg->longServerInfo);
        iPodCoreClearHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->cmdQueueInfo);
        iPodCoreClearHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->longQueueServer);
        iPodCoreClearHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->longServerInfo);
        
        iPodCoreClearHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->shutdownEvent);
        iPodPlayerIPCDeleteHandle(iPodCfg->shutdownEvent);
        close(iPodCfg->shutdownEvent);
        g_iPodCoreShutdown = -1;
        
        iPodCoreClearHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->waitHandle);
        iPodPlayerIPCDeleteHandle(iPodCfg->waitHandle);
        
        close(iPodCfg->waitHandle);
    }
    
    if(iPodCfg->appInfo != NULL)
    {
        free(iPodCfg->appInfo);
        iPodCfg->appInfo = NULL;
    }
    
    /* Clear mask of using device */
    iPodCfg->mask = 0;
    
    free(iPodCfg);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* S32 iPodCoreCreateResource
 * 
 * Create the resrouce of iPodPlayerCore.
 * It may be shared from some threads.
 */
static IPOD_PLAYER_CORE_CFG *iPodCoreCreateResource(void)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 devNum = 0;
    U32 appNum = 0;
    U32 iosAppNum = 0;
    U32 maxMsgSize = 0;
    U32 maxPktSize = 0;
    IPOD_PLAYER_CORE_CFG *iPodCfg = NULL;
    IPOD_PLAYER_IPC_OPEN_INFO info;
    U8 semTemp[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    S32 handle = -1;
    S32 shutdownHandle = -1;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    
    /*Allocate memory for configuration of iPodPlayerCore */
    iPodCfg = calloc(1, sizeof(IPOD_PLAYER_CORE_CFG));
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if memory was not allocated */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return NULL;
    }
    
    rc = iPodPlayerIPCInit();
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        iPodCfg->curiPodNum = 0;
        
        /* get cfg number */
        devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
        appNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_APP_MAX_NUM);
        iosAppNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_IOSAPP_COUNT);
        maxMsgSize = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_MAX_MSG_SIZE);
        maxPktSize = maxMsgSize;
        
        /* Get the iOSApplication */
        if(iosAppNum <= IPODCORE_MAX_IOSAPPS_INFO_NUM)
        {
            for(i = 0; i < iosAppNum; i++)
            {
                rc = iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_IOSAPP_NAME, i, sizeof(iPodCfg->iOSAppInfo[i].appName), iPodCfg->iOSAppInfo[i].appName);
                if(rc == IPOD_PLAYER_OK)
                {
                    iPodCfg->iOSAppInfo[i].appName[sizeof(iPodCfg->iOSAppInfo[i].appName) - 1] = '\0';
                    rc = iPodCoreGetIndexCfs(IPOD_PLAYER_CFGNUM_IOSAPP_URL, i, sizeof(iPodCfg->iOSAppInfo[i].appURL), iPodCfg->iOSAppInfo[i].appURL);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        iPodCfg->iOSAppInfo[i].appURL[sizeof(iPodCfg->iOSAppInfo[i].appURL) - 1] = '\0';
                        rc = IPOD_PLAYER_OK;
                    }
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iosAppNum);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Initialize the descriptor to -1 */
        iPodCfg->cmdQueueInfo = IPODCORE_DEFAULT_VALUE;
        memset(iPodCfg->handle, 0, sizeof(iPodCfg->handle));
        iPodCfg->handleNum = 0;
        
        /* Allocate a number of socket information memory of appNum */
        iPodCfg->appInfo = calloc(appNum, sizeof(*iPodCfg->appInfo));
        if(iPodCfg->appInfo == NULL)
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = epoll_create(IPOD_PLAYER_IPC_MAX_EPOLL_NUM);
        if(rc > 0)
        {
            iPodCfg->waitHandle = rc;
            /* Register the handle to iPodCtrlCfg handles */
            rc = iPodPlayerIPCCreateHandle(&handle, iPodCfg->waitHandle);
            if(rc == IPOD_PLAYER_OK)
            {
                rc = iPodCoreSetHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->waitHandle);
            }
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = eventfd(0, EFD_NONBLOCK);
        if(rc >= 0)
        {
            iPodCfg->shutdownEvent = rc;
            g_iPodCoreShutdown = iPodCfg->shutdownEvent;
            rc = iPodPlayerIPCCreateHandle(&shutdownHandle, iPodCfg->shutdownEvent);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = iPodCoreSetHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->shutdownEvent);
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = (U8 *)IPOD_PLAYER_CORE_SOCKET_NAME;
        info.connectionNum = 10;
        info.semName = NULL;
        info.maxBufSize = maxMsgSize;
        info.maxPacketSize = maxPktSize;
        /* Create receive socket for communicate with iPodPlayerIF */
        rc = iPodPlayerIPCOpen(&iPodCfg->serverInfo, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = iPodCoreSetHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->serverInfo);
            if(rc == IPOD_PLAYER_OK)
            {
                for(i = 0; i < appNum; i++)
                {
                    iPodCfg->appInfo[i].sckIn = iPodCfg->serverInfo;
                }
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = (U8 *)IPOD_PLAYER_CORE_SOCKET_LONG_NAME;
        info.connectionNum = 10;
        info.semName = NULL;
        info.maxBufSize = maxMsgSize;
        info.maxPacketSize = maxPktSize;
        /* Create receive socket for communicate with iPodPlayerIF */
        rc = iPodPlayerIPCOpen(&iPodCfg->longServerInfo, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = iPodCoreSetHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->longServerInfo);
            if(rc == IPOD_PLAYER_OK)
            {
                for(i = 0; i < appNum; i++)
                {
                    iPodCfg->appInfo[i].sckIn = iPodCfg->serverInfo;
                }
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = (U8 *)IPODCORE_COMMAND_QUEUE_IDENTIFY;
        info.connectionNum = IPODCORE_QUEUE_MAX_NUM;
        info.semName = NULL;
        info.maxBufSize = maxMsgSize;
        info.maxPacketSize = maxPktSize;
        /* Create message queue for command handler thread */
        rc = iPodPlayerIPCOpen(&iPodCfg->cmdQueueInfo, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = iPodCoreSetHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->cmdQueueInfo);
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
            rc = iPodPlayerIPCOpen(&iPodCfg->cmdQueueInfoClient, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = (U8 *)IPODCORE_COMMAND_QUEUE_LONG_IDENTIFY;
        info.connectionNum = IPODCORE_QUEUE_MAX_NUM;
        info.semName = NULL;
        info.maxBufSize = maxMsgSize;
        info.maxPacketSize = maxPktSize;
        /* Create message queue for command handler thread */
        rc = iPodPlayerIPCOpen(&iPodCfg->longQueueServer, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = iPodCoreSetHandle(iPodCfg->handle, &iPodCfg->handleNum, iPodCfg->longQueueServer);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            snprintf((char *)semTemp, sizeof(semTemp), "/tmp%s", IPODCORE_COMMAND_QUEUE_LONG_IDENTIFY);
            info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG;
            info.semName = semTemp;
            rc = iPodPlayerIPCOpen(&iPodCfg->longQueueClient, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }

    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate the memory of thread information for number of iPodNum */
        iPodCfg->threadInfo = calloc(devNum, sizeof(IPOD_PLAYER_CORE_THREAD_INFO));
        if(iPodCfg->threadInfo == NULL)
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCfg->detectInfo = calloc(1, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS));
        if(iPodCfg->detectInfo == NULL)
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
        
    if(rc == IPOD_PLAYER_OK)
    {
        /* Allocate message data buffer */
        iPodCfg->contents = calloc(1, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS));
        if(iPodCfg->contents == NULL)
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize thread information stack */
        rc = iPodCoreiPodCtrlInitThreadInfoStack();
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCoreInitLongRecvInfo(iPodCfg->longRecvInfo);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCoreAddFDs(iPodCfg->waitHandle, iPodCfg->handleNum, iPodCfg->handle);
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        iPodCoreDeleteResource(iPodCfg);
        iPodCfg = NULL;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return iPodCfg;
}

void  iPodCoreDeleteThread(IPOD_PLAYER_CORE_CFG *iPodCfg, U8 cancel)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 ret = IPOD_PLAYER_OK;
    S32 *retVal = NULL;
    U32 i = 0;
    U32 devNum = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg, cancel);
    
    /* Parameter Check */
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return;
    }
    
    retVal = &ret;
    /* get cfg number */
    devNum = iPodCoreGetCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM);
    for(i = 0; i < devNum; i++)
    {
        /* iPodControl Thread is created */
        if(iPodCfg->threadInfo[i].isReady != 0)
        {
            /* Thread is canceled */
            if(cancel != 0)
            {
                /* Wait iPodControl Thread is deleted by itself */
                rc = pthread_cancel(iPodCfg->threadInfo[i].id);
            }
            
            /* Wait iPodControl Thread is deleted by iteself */
            rc = pthread_join(iPodCfg->threadInfo[i].id, (void **)&retVal);
            iPodCfg->threadInfo[i].isReady = 0;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return;
}

void iPodCoreMainClearData(IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    /* Parameter check */
    if(contents == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, contents);
        return;
    }
    
    memset(contents, 0, sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS));
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    
    return;
}

S32 iPodCoreConnectToApp(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_CORE_APP_INFO *info)
{
    S32 rc = IPOD_PLAYER_OK;
    
    /* Parameter check */
    if((iPodCfg == NULL) || (info == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

static void *ippiAP2ServiceHandleEventsThread(void *arg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 endFlag = 0;
    U32 errorCount = 0;
    U32 count = 0;
    int fd = epoll_create(IPOD_PLAYER_IPC_MAX_EPOLL_NUM);
    struct epoll_event epollEvent[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    struct epoll_event event;
    S32 rcp = 0;
    U64 data = 1;
    ssize_t ret = 0;
    (void)arg;

    event.events = EPOLLIN;
    event.data.fd = g_service->iAP2ServerFd;
    rcp = epoll_ctl(fd, EPOLL_CTL_ADD, g_service->iAP2ServerFd, &event);
    if(rcp != 0)
    {
        IPOD_DLT_ERROR("epoll_ctl failed :rcp=%d", rcp);
        return NULL;
    }

    while(endFlag == 0)
    {
        rcp = epoll_wait(fd, epollEvent, IPOD_PLAYER_IPC_MAX_EPOLL_NUM, -1);
        if(rcp > 0)
        {
            for(count = 0; count < (U32)rcp; count++)
            {
                if(epollEvent[count].data.fd == g_service->iAP2ServerFd)
                {
                    rc = iAP2ServiceHandleEvents(g_service);
                    if(rc < 0)
                    {
                        if(errorCount == 0)
                        {
                            IPOD_DLT_WARN("iAP2ServiceHandleEvents failed :rc=%d", rc);
                        }
                        else if(errorCount >= (10 - 1))
                        {
                            endFlag = 1;
                            rc = IPOD_PLAYER_ERROR;
                            IPOD_DLT_ERROR("iAP2ServiceHandleEvents failed 10 consecutive times :rc=%d", rc);
                            break;
                        }
                        errorCount++;
                        /* In case of error, wait 2sec. */
                        sleep(2);
                    }
                    else
                    {
                        errorCount = 0;
                    }
                }
                else
                {
                    IPOD_DLT_ERROR("Unknown event :fd=%d", epollEvent[count].data.fd);
                }
            }
        }
    }

    if(g_iPodCoreShutdown != -1)
    {
        ret = write(g_iPodCoreShutdown, &data, sizeof(data));
        if (ret < 0)
        {
            g_iPodCoreShutdownError = 1;    /* error (cannot output log directly in handler) */
            IPOD_DLT_ERROR("shutdown error :ret=%zd", ret);
        }
    }

    return NULL;
}

S32 iPodCoreMain(IPOD_PLAYER_CORE_CFG *iPodCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U8 endFlag = 0;
    U32 errorCount = 0;
    U32 checkNum = 0;
    U32 count = 0;
    U32 size = 0;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents = NULL;
    U32 i = 0;
    IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfo = NULL;
    struct timespec waitTime = {.tv_sec = 0, .tv_nsec = IPODCORE_EPOLL_RETRY_WAIT};
    struct epoll_event epollEvent[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    IPOD_PLAYER_IPC_HANDLE_INFO inputInfo[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    IPOD_PLAYER_IPC_HANDLE_INFO outputInfo[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCfg);
    iAP2ServiceCallbacks_t serviceCallbacks;
    iAP2ServiceClientInformation_t client;
    pthread_t p;
    S32 rcp = 0;
    
    /* Parameter check */
    if(iPodCfg == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(inputInfo, 0, sizeof(inputInfo));
    memset(outputInfo, 0, sizeof(outputInfo));

    /* Connects to iAP2Service */
    g_service = NULL;
    memset(&serviceCallbacks, 0, sizeof(iAP2ServiceCallbacks_t));
    rc = ippiAP2ServiceSetCallbacks(&serviceCallbacks);
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        return rc;
    }
    memset(&client, 0, sizeof(client));
    client.pid = getpid();
    strncpy(client.name, "iPodPlayerWrapper", strnlen("iPodPlayerWrapper", sizeof(client.name)));
    endFlag = 0;
    errorCount = 0;
    while(endFlag == 0)
    {
        g_service = iAP2ServiceInitialize(&serviceCallbacks, &client);
        if(g_service != NULL)
        {
            if(g_service->iAP2ServerFd != -1)
            {
                endFlag = 1;
                IPOD_DLT_INFO("iAP2ServiceInitialize success");
            }
            else
            {
                if(errorCount == 0)
                {
                    IPOD_DLT_WARN("iAP2ServiceInitialize failed, retry.:iAP2ServerFd=%d", g_service->iAP2ServerFd);
                }
                else if(errorCount >= (60 - 1))
                {
                    IPOD_DLT_ERROR("iAP2ServiceInitialize failed 60 consecutive times (1min).:iAP2ServerFd=%d", g_service->iAP2ServerFd);
                    return IPOD_PLAYER_ERROR;
                }
                errorCount++;
                /* iAP2Service may not be running. Wait 1sec and retry. */
                sleep(1);
                if(g_iPodCoreIAP2SWaitExit == 1)
                {
                     return IPOD_PLAYER_ERROR;
                }
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, g_service);
            return IPOD_PLAYER_ERROR;
        }
    }

    rc = ippiAP2InitCtrlCfgList();
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        return rc;
    }

    rcp = pthread_create(&p, NULL, ippiAP2ServiceHandleEventsThread, NULL);
    if(rcp != 0)
    {
        IPOD_DLT_ERROR("iAP2ServiceHandleEvents thread create failed :rcp=%d", rcp);
        return IPOD_PLAYER_ERROR;
    }

    endFlag = 0;
    while(endFlag == 0)
    {
        /* Wait the event and get the data */
        /* Wait the I/O event */
        rcp = epoll_wait(iPodCfg->waitHandle, epollEvent, IPOD_PLAYER_IPC_MAX_EPOLL_NUM, -1);
        if(rcp > 0)
        {
            for(count = 0; count < (U32)rcp; count++)
            {
                inputInfo[count].handle = epollEvent[count].data.fd;
                inputInfo[count].event = epollEvent[count].events;
            }
            rc = iPodPlayerIPCIterate(iPodCfg->waitHandle, count, inputInfo, IPOD_PLAYER_IPC_MAX_EPOLL_NUM, outputInfo);
            if(rc > 0)
            {
                checkNum = rc;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else if(rc == 0)
        {
            /* Timeout occured */
            rc = IPOD_PLAYER_IPC_TMO;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Loop until receive the data from readying descriptor */
            for(count = 0; (count < checkNum) && (endFlag == 0); count++)
            {
                /* Shutdown signal is issued */
                if(outputInfo[count].handle == iPodCfg->shutdownEvent)
                {
                    endFlag = 1;
                    rc = IPOD_PLAYER_ERR_SHUTDOWN;
                    break;
                }
                
                if(outputInfo[count].type != IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG)
                {
                    rc = iPodPlayerIPCReceive(outputInfo[count].handle, (U8 *)iPodCfg->contents, 
                                              sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS), 0, iPodCfg->waitHandle, IPODCORE_TMOUT_FOREVER);
                    if(rc > 0)
                    {
                        contents = iPodCfg->contents;
                        size = rc;
                        rc = IPOD_PLAYER_OK;
                    }
                    else
                    {
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    /* Get the long receive information from file descriptor */
                    recvInfo = iPodCoreGetLongRecvInfo(iPodCfg->longRecvInfo, outputInfo[count].handle);
                    if(recvInfo != NULL)
                    {
                        rc = iPodPlayerIPCLongReceive(recvInfo->fd, recvInfo->num, &recvInfo->buf[IPODCORE_LONG_HEADER_POS], 
                                                        &recvInfo->size[IPODCORE_LONG_HEADER_POS], &size, 0, iPodCfg->waitHandle, -1);
                        if(rc == IPOD_PLAYER_IPC_OK)
                        {
                            memcpy(&iPodCfg->contents->paramTemp.header, &recvInfo->header, sizeof(iPodCfg->contents->paramTemp.header));
                            contents = (IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)(void *)recvInfo->buf[IPODCORE_LONG_DATA_POS];
                            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCfg->contents->paramTemp.header.funcId);
                        }
                        else if(rc == IPOD_PLAYER_IPC_ERR_NOMEM)
                        {
                            /* Previous buffer still remain */
                            if(recvInfo->buf[IPODCORE_LONG_DATA_POS] != NULL)
                            {
                                free(recvInfo->buf[IPODCORE_LONG_DATA_POS]);
                                recvInfo->buf[IPODCORE_LONG_DATA_POS] = NULL;
                            }
                            
                            /* Allocate buffer until size - header size */
                            recvInfo->buf[IPODCORE_LONG_DATA_POS] = calloc(size - recvInfo->size[IPODCORE_LONG_HEADER_POS], sizeof(U8));
                            if(recvInfo->buf[IPODCORE_LONG_DATA_POS] != NULL)
                            {
                                recvInfo->size[IPODCORE_LONG_DATA_POS] = size - recvInfo->size[IPODCORE_LONG_HEADER_POS];
                                recvInfo->num = IPODCORE_LONG_DATA_NUM;
                                /* Register long receive information again */
                                iPodCoreSetLongRecvInfo(iPodCfg->longRecvInfo, outputInfo[count].handle, recvInfo);
                                rc = IPOD_PLAYER_ERR_MORE_MEM;
                            }
                            else
                            {
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                                rc = IPOD_PLAYER_ERR_NOMEM;
                            }
                        }
                        else if(rc == IPOD_PLAYER_IPC_ERR_MORE_PACKET)
                        {
                            rc = IPOD_PLAYER_ERR_MORE_PACKET;
                        }
                        else
                        {
                            rc = IPOD_PLAYER_ERROR;
                        }
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = iPodCoreCmdHandlerDataAnalyze(iPodCfg, &iPodCfg->contents->paramTemp.header, contents, size);
                }
                
                if(((rc != IPOD_PLAYER_ERR_MORE_MEM) && (rc != IPOD_PLAYER_ERR_MORE_PACKET)) && (outputInfo[count].type == IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG))
                {
                    if(recvInfo != NULL)
                    {
                        for(i = 1; i < recvInfo->num; i++)
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
                    /* Clear long receive information */
                    iPodCoreClearLongRecvInfo(iPodCfg->longRecvInfo, outputInfo[count].handle);
                }
                
                /* end process */
                if(rc == IPOD_PLAYER_ERR_SHUTDOWN)
                {
                    /* end main loop */
                    endFlag = 1;
                    rc = IPOD_PLAYER_ERROR;
                    break;
                }
            }
        }
        else
        {
            /* System erro may be occurred. Wait 100ms and retry wait */
            nanosleep(&waitTime, NULL);
        }
        
        if(rc != IPOD_PLAYER_ERR_MORE_PACKET)
        {
            iPodCoreMainClearData(iPodCfg->contents);
        }
    }
    
    rcp = pthread_cancel(p);
    if(rcp != 0)
    {
        IPOD_DLT_WARN("pthread_cancel failed :rcp=%d", rcp);
    }
    rcp = pthread_join(p, NULL);
    if(rcp != 0)
    {
        IPOD_DLT_WARN("pthread_join failed :rcp=%d", rcp);
    }

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void ippDisplayHelp()
{
    printf("Usage: iPodPlayerCore.out [option]... \n");
    printf("       -d application name\n");
    printf("          The specified application is disabled.It can specify following application name.\n");
    printf("            "IPP_COM_OPT_DIS_DETECTION"\n");
    printf("            "IPP_COM_OPT_DIS_AUDIO"\n");
}

int ippCheckArgument(int argc, char* argv[], IPPCOMOPT *comOpt)
{
  int opt;
  int ix;
  int rc = IPOD_PLAYER_OK;
  char *app = NULL;
  
  static struct option options[] =
  {
    {"help",    no_argument,        NULL, 'h'},
    {"disable", required_argument,  NULL, 'd'},
    {0, 0, 0, 0}
  };
 
    while((opt = getopt_long(argc, argv, "hd:", options, &ix)) != -1){
        switch(opt)
        {
            case 'h':
            {
                ippDisplayHelp();
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                break;          
            }
            case 'd':
            {
                app = optarg;
                break;
            }
            default:
            {
                printf("Error: An unknown option is appointed.\n");
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                break;          
            }
        }
        
        if(app != NULL)
        {
            if(strcmp(IPP_COM_OPT_DIS_DETECTION, app) == 0)
            {
                comOpt->disDetectionApp = TRUE;
            }else if(strcmp(IPP_COM_OPT_DIS_AUDIO, app) == 0)
            {
                comOpt->disAudioApp = TRUE;
            }else{
                printf("Error: application name is unmatched.\n");
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
           }
        }

        if(rc != IPOD_PLAYER_OK)
        {
            break;
        }
    }

    return rc;
}

/** int main
 * 
 * Process is started from this function
 */
int main(int argc, char* argv[])
{
    S32 rc = IPOD_PLAYER_OK;
    U8 cancel = 0;
    IPPCOMOPT   comOpt = {};
    IPOD_LOG_INT_PARAM log_param;
    struct sigaction sig_action;                                   /* for initialze signal  */
    
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    U8 arg[1] = {0};
    S32 i = 0;                                                      /* for children pid loop */
    S32 j = 0;                                                      /* for waitpid loop      */
    S32 status = -1;                                                /* for waitpid status    */
    U32 children_num = 0;                                           /* for children number   */
    pid_t pid = 0;                                                  /* for children pid      */
    pid_t rc_pid = 0;                                               /* for waitpid return    */
    pid_t children_pid[IPOD_PLAYER_MAX_CHILDREN_PID_NUM] = {0};     /* for save children pid */
    struct timespec waitTime = {0, (IPOD_PLAYER_TIME_MSEC_TO_NSEC * 5)};    /* for waite time        */
#endif /* IPOD_PLAYER_CORE_SELF_RUNNING */
    
    rc = ippCheckArgument(argc, argv, &comOpt);
    if(rc != IPOD_PLAYER_OK)
    {
        exit(1);
    }
    
    /* initialize parameter */
    memset(&log_param, 0, sizeof(log_param));
    memset(&sig_action, 0, sizeof(sig_action));
#ifdef IPOD_PLAYER_CORE_DAEMON_RUN
    daemon(0,0);
#endif
    
    /* initialize children number */
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    /* start iPodPlayerCore.out as daemon process */
    
    /* start audio streaming application */
    if(!comOpt.disAudioApp)
    {
        pid = fork();
        if(pid == 0)
        {
            rc = execl(IPODCORE_AUDIO_STREAMING_PATH, (const char *)arg, NULL);
            if(rc < 0)
            {
                exit(1);
            }
        }
        else
        {
            /* set children process id and count up */
            children_pid[children_num] = pid;
            children_num++;
        }
    }
    
    /* start device detection application */
    if(!comOpt.disDetectionApp)
    {
        pid = fork();
        if(pid == 0)
        {
            rc = execl(IPODCORE_DEVICE_DETECTION_PATH, (const char *)arg, NULL);
            if(rc < 0)
            {
                exit(1);
            }
        }
        else
        {
            /* set children process id and count up */
            children_pid[children_num] = pid;
            children_num++;
        }
    }

#endif /* IPOD_PLAYER_CORE_SELF_RUNNING */
    
    /* set handler of SIGTERM */
    sig_action.sa_handler = iPodCoreSigTerm;
    sig_action.sa_flags |= SA_RESTART;
    
    /* set handler */
    rc = sigaction(SIGTERM, &sig_action, NULL);
    if(rc == 0)
    {
        rc = sigaction(SIGINT, &sig_action, NULL);
    }
    if(rc != 0)
    {
        exit(1);
    }

#ifdef IPOD_HAS_DLT
    /* set log init parameter */
    strncpy((char *)log_param.dltCtx, IPOD_PLAYER_DLT_CONTEXT, sizeof(log_param.dltCtx));
    strncpy((char *)log_param.dltCtxDsp, IPOD_PLAYER_DLT_CONTEXT_DSP, sizeof(log_param.dltCtxDsp));

    /* initialize iPod logging */
    iPodLogInitialize(&log_param);

    /* register iap2 library log */
    IAP2REGISTERCTXTWITHDLT();
    
    /* register ipod control log */
    IAP1REGISTERCTXTWITHDLT();

#endif /* IPOD_HAS_DLT */
    
    /* initialize cfg */
    rc = iPodCoreInitCfg();
    if(rc == IPOD_PLAYER_OK)
    {
        /* Create the iPodPlayerCore resources */
        g_iPodCoreCfg = iPodCoreCreateResource();
        
        if(g_iPodCoreCfg != NULL)
        {
            /* Initialize iPod Control */
            rc = iPodCoreiPodCtrlInitConnection();
            if(rc == IPOD_PLAYER_OK)
            {
                g_iPodCoreCfg->iPodCtrlReady = 1;
            }
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreMain(g_iPodCoreCfg);
    }
    
    /* Something error occur */
    if(rc != IPOD_PLAYER_OK)
    {
        cancel = 1;
    }
    
    /* Remove the all threads */
    iPodCoreDeleteThread(g_iPodCoreCfg, cancel);
    
    
    if(g_iPodCoreCfg != NULL)
    {
        if(g_iPodCoreCfg->iPodCtrlReady != 0)
        {
            /* Deinitialize iPodCtrl*/
            iPodCoreiPodCtrlDisconnect();
            g_iPodCoreCfg->iPodCtrlReady = 0;
        }
    }
    
    /* Remove the all resources*/
    rc = iPodCoreDeleteResource(g_iPodCoreCfg);
    
    /* finalize cfg */
    iPodCoreDeInitCfg();
    
#ifdef IPOD_HAS_DLT
    /* finalize iPod logging */
    iPodLogDeinitialize();
    IAP2DEREGISTERCTXTWITHDLT();
    IAP1DEREGISTERCTXTWITHDLT();
#endif // IPOD_HAS_DLT
    
#ifdef IPOD_PLAYER_CORE_SELF_RUNNING
    /* loop for all of children process */
    for(i = IPOD_PLAYER_MAX_CHILDREN_PID_NUM -1 ; i >= 0 ; i--)
    {
        /* get one child process id */
        pid = children_pid[i];
        if(pid > 0)
        {
            /* kill child process */
            kill(pid, SIGTERM);
            nanosleep(&waitTime, NULL);
            /* loop for waiting chidren processes killed */
            for(j = 0; j < IPOD_PLAYER_WAIT_CHILDREN_KILL_COUNT; j++)
            {
                /* wait for child process done */
                rc_pid = waitpid(pid, &status, WNOHANG);
                if((rc_pid == -1) || (rc_pid > 0))
                {
                    /* child process was killed */
                    break;
                }else{
                    /* wait for 5 msec */
                    nanosleep(&waitTime, NULL);
                }
            }
        }
    }
#endif /* IPOD_PLAYER_CORE_SELF_RUNNING */
    
    return rc;
}

