#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreiPodCtrlFunc.h"
#include "iPodPlayerCoreFunc.h"
#include "iPodPlayerCoreCfg.h"
#include "iap_common.h"
#include "ipodcommon.h"
#include "iPodPlayerCoreCfg.h"
#include "iPodPlayerUtilityLog.h"
#include "ipp_mainloop_common.h"
#include "ipp_audiocommon.h"
#include <sys/timerfd.h>

IMPORT IPOD_PLAYER_CORE_THREAD_INFO **g_threadInfo;

#define IPOD_PLAYER_CHECK_THREAD_RETRY_MAX 5
#define IPOD_PLAYER_CHECK_THREAD_RETRY_WAIT 200000000

/* Search requested thread information */
static IPOD_PLAYER_CORE_THREAD_INFO *iPodCoreiPodCtrlGetThreadInfo(U32 iPodID)
{
    U32 i = 0;
    U32 dev_num = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    struct timespec waitTime = {0, IPOD_PLAYER_CHECK_THREAD_RETRY_WAIT};
    U32 retry = 0;
    
    /* Check the paramter */
    if(g_threadInfo == NULL)
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, g_threadInfo);
        return NULL;
    }
    
    /* get max number of dev */
    dev_num = iPodCoreGetIndexCfn(IPOD_PLAYER_CFGNUM_DEVICE_MAX_NUM, 0);
    
    for(retry = 0; retry < IPOD_PLAYER_CHECK_THREAD_RETRY_MAX; retry++)
    {
        /* Serch iPodInformation */
        for(i = 0; i < dev_num; i++)
        {
            /* iPod has connected */
            if((g_threadInfo[i] != NULL) && (g_threadInfo[i]->isReady != 0))
            {
                /* appDevID matches with iPodID */
                if(g_threadInfo[i]->iPodDevID == iPodID)
                {
                    /* Set matched thread */
                    thread_info = g_threadInfo[i];
                    break;
                }
            }
        }
        
        if(thread_info == NULL)
        {
            nanosleep(&waitTime, NULL);
        }
        else
        {
            break;
        }
    }
    
    if(thread_info != NULL)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE);
    }
    
    return thread_info;
}

/* Register the handle to handle table */
S32 iPodCoreSetHandle(S32 *handle, U32 *handleNum, S32 setHandle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, handle, handleNum, setHandle);
    
    /* Parameter check */
    if((handle == NULL) || (handleNum == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, handleNum);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(*handleNum >= IPOD_PLAYER_IPC_MAX_EPOLL_NUM)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, *handleNum);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    handle[*handleNum] = setHandle;
    *handleNum = *handleNum + 1;
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, handle[*handleNum], *handleNum);
    
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

/* Remove the registed handle in the handle table */
S32 iPodCoreClearHandle(S32 *handle, U32 *handleNum, S32 clearHandle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    U32 j = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, handleNum, clearHandle);
    
    /* Parameter check */
    if((handle == NULL) || (handleNum == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle, handleNum);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Loop until handle number */
    for(i = 0; i < *handleNum; i++)
    {
        /* handle eqauls to handle which is removed */
        if(handle[i] == clearHandle)
        {
            handle[i] = -1;
            rc = IPOD_PLAYER_OK;
            break;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Sort the handle table */
        for(j = i; j < IPOD_PLAYER_IPC_MAX_EPOLL_NUM -1; j++)
        {
            handle[j] = handle[j + 1];
        }
        *handleNum = *handleNum - 1;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, *handleNum);
    
    if(rc == IPOD_PLAYER_ERROR)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    return rc;
}

/* Register the this callback function for notify the Attach */
void iPodCoreCBAttach(const S32 success, IPOD_CONNECTION_TYPE connection, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS connectionStatus;
    U32 size = sizeof(connectionStatus);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, success, connection, iPodID);
    
    /* Initialize the structure */
    memset(&connectionStatus, 0, sizeof(connectionStatus));
    
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the notify connection command */
        connectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
        connectionStatus.header.devID  = thread_info->appDevID;
        connectionStatus.status.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
         
        switch (connection)
        {
        case IPOD_USB_HOST_CONNECT:
            /*! device type is USB device */
            connectionStatus.status.deviceType = IPOD_PLAYER_DEVICE_TYPE_USB_DEVICE;
            break;
        case IPOD_USB_FUNC_CONNECT:
            /*! device type is USB host */
            connectionStatus.status.deviceType = IPOD_PLAYER_DEVICE_TYPE_USB_HOST;
            break;
        case IPOD_BT_CONNECT:
            /*! device type is BT */
            connectionStatus.status.deviceType = IPOD_PLAYER_DEVICE_TYPE_BT;
            break;
        case IPOD_UART_CONNECT:
            /*! device type is UART */
            connectionStatus.status.deviceType = IPOD_PLAYER_DEVICE_TYPE_UART;
            break;
        default:
            /*! device type is UNKNOWN */
            connectionStatus.status.deviceType = IPOD_PLAYER_DEVICE_TYPE_UNKNOWN;
            break;
        }
        /* Check connection status */
        if(success == 1)
        {
            /* Connection status is detect */
            connectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
            connectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
            connectionStatus.status.powerStatus = IPOD_PLAYER_POWER_ON;
            connectionStatus.status.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP1;
        }
        else if(success == 0)
        {
            /* Connection status is attach */
            connectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
            connectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_SUCCESS;
            connectionStatus.status.powerStatus = IPOD_PLAYER_POWER_ON;
            connectionStatus.status.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP1;
        }
        else
        {
            /* Connection status is authentication fail */
            connectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
            connectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_FAIL;
            connectionStatus.status.powerStatus = IPOD_PLAYER_POWER_ON;
        }
        
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&connectionStatus, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

/* Register the this callback function for notify the Detach */
void iPodCoreCBDetach(const U32 iPodID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS connectionStatus;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodID);
    
    /* Initialize the structure */
    memset(&connectionStatus, 0, sizeof(connectionStatus));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the notify connection command */
        connectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
        connectionStatus.header.devID = thread_info->appDevID;
        /* Device status sets the disconnect */
        connectionStatus.status.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT;
        connectionStatus.status.authStatus = IPOD_PLAYER_AUTHENTICATION_UNKNOWN;
        connectionStatus.status.powerStatus = IPOD_PLAYER_POWER_UNKNOWN;
        connectionStatus.status.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
        
        rc = iPodPlayerIPCSend(thread_info->cmdQueueInfoClient, (U8 *)&connectionStatus, sizeof(connectionStatus), 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)sizeof(connectionStatus))
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc);
        }
    }
    
}

void iPodCoreCBGetCoverart(IPOD_ALBUM_ARTWORK* artworkData, U32 iPodID)
{
    IPOD_PLAYER_PARAM_TEMP_LONG_DATA longData;
    U8 *sendBuf[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    U32 asize = 0;
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, artworkData, iPodID);
    
    /* Initialize the structure */
    memset(&longData, 0, sizeof(longData));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        longData.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_COVERART);
        longData.header.devID = thread_info->appDevID;

        if(artworkData != NULL)
        {
            longData.header.longData = 1;
            longData.coverart.coverartHeader.formatId = 0;
            longData.coverart.coverartHeader.width = artworkData->imageWidth;
            longData.coverart.coverartHeader.height = artworkData->imageHeight;
            longData.coverart.coverartHeader.topLeftX = artworkData->topLeftPointX;
            longData.coverart.coverartHeader.topLeftY = artworkData->topLeftPointY;
            longData.coverart.coverartHeader.bottomRightX = artworkData->bottomRightX;
            longData.coverart.coverartHeader.bottomRightY = artworkData->bottomRightY;
            longData.coverart.coverartHeader.rowsize = artworkData->rowSize;
            longData.coverart.bufSize = artworkData->pixelDataLength;
            
            /* Set long data */
            sendBuf[IPODCORE_POS0] = (U8 *)&longData;
            sendSize[IPODCORE_POS0] = sizeof(longData);
            sendBuf[IPODCORE_POS1] = artworkData->pixelData;
            sendSize[IPODCORE_POS1] = artworkData->pixelDataLength;
        
            rc = iPodPlayerIPCLongSend(thread_info->longQueueClient, IPODCORE_LONG_DATA_ARRAY, sendBuf, sendSize, &asize, 0, IPODCORE_SEND_TMOUT);
            if(rc != IPOD_PLAYER_IPC_OK)
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
        else
        {
            longData.coverart.bufSize = 0;
            rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&longData, sizeof(longData), 0, IPODCORE_SEND_TMOUT);
            if(rc != (S32)sizeof(longData))
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, sizeof(longData));
            }
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);

}

/* Register the this callback function for notify the list */
void iPodCoreCBGetEntries(U32 trackIndex, U8* string, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_GET_DB_ENTRIES entries;
    U32 size = sizeof(IPOD_PLAYER_PARAM_GET_DB_ENTRIES);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, trackIndex, iPodID);
    
    /* Initialize the structure */
    memset(&entries, 0, sizeof(entries));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the notified track index */
        entries.entry.trackIndex = trackIndex;
        if(string != NULL)
        {
            strncpy((char *)&entries.entry.name[IPODCORE_POS0], (const char *)string, sizeof(entries.entry.name));
            entries.entry.name[sizeof(entries.entry.name) - 1] = '\0';
            IPOD_LOG_INFO_WRITESTR32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, string, trackIndex);
        }
        
        entries.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_DB_ENTRIES);
        entries.header.devID = thread_info->appDevID;
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&entries, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

/* Register the this callback function for notify to be opened the iOS Application */
S32 iPodCoreCBOpenApp(U8 protocolIndex, U16 sessionId, const U32 iPodID)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_NOTIFY_OPEN_APP openApp;
    U32 size = sizeof(IPOD_PLAYER_PARAM_NOTIFY_OPEN_APP);
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, protocolIndex, sessionId, iPodID);
    
    /* Initialize the structure */
    memset(&openApp, 0, sizeof(openApp));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the opend paramtetr */
        openApp.index = protocolIndex;
        openApp.session = sessionId;
        openApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_OPEN_APP;
        openApp.header.devID = thread_info->appDevID;
        
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&openApp, size, 0, IPODCORE_SEND_TMOUT);
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
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Register the this callback function for notify to be closed the iOS Application */
void iPodCoreCBCloseApp(U16 sessionId, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_NOTIFY_CLOSE_APP closeApp;
    U32 size = sizeof(IPOD_PLAYER_PARAM_NOTIFY_CLOSE_APP);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, sessionId, iPodID);
    
    /* Initialize the structure */
    memset(&closeApp, 0, sizeof(closeApp));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the closed parameter */
        closeApp.session = sessionId;
        closeApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CLOSE_APP;
        closeApp.header.devID = thread_info->appDevID;
        
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&closeApp, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

/* Register the this callback function for notify to be received the data from iOS Application */
S32 iPodCoreCBReceiveFromApp(U16 sessionId, U8 *data, U16 length, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_NOTIFY_RECEIVE_FROM_APP receiveApp;
    S32 rc = IPOD_PLAYER_OK;
    U8 *sendBuf[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    U32 asize = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, sessionId, data, length, iPodID);
    
    if(data == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, data);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&receiveApp, 0, sizeof(receiveApp));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the received parameter */
        receiveApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_RECEIVE_FROM_APP;
        receiveApp.header.devID = thread_info->appDevID;
        receiveApp.header.longData = 1;
        receiveApp.session = sessionId;
        receiveApp.dataSize = length;
        sendBuf[0] = (U8 *)&receiveApp;
        sendSize[0] = sizeof(receiveApp);
        sendBuf[1] = data;
        sendSize[1] = length;
        
        rc = iPodPlayerIPCLongSend(thread_info->longQueueClient, IPODCORE_LONG_DATA_ARRAY, sendBuf, sendSize, &asize, 0, IPODCORE_SEND_TMOUT);
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
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);

    return rc;
}

/* Register the this callback function for notify the playback status */
void iPodCoreCBNotify(IPOD_CHANGED_PLAY_STATUS status, U64 param, const U32 iPodID)
{
    IPOD_CORE_INT_PARAM_NOTIFY_STATUS intNotifyStatus;
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE64(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, status, param, iPodID);
    
    /* Initialize the structure */
    memset(&intNotifyStatus, 0, sizeof(intNotifyStatus));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the notify status change command with internal */
        intNotifyStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_INTERNAL_NOTIFY_STATUS;
        intNotifyStatus.header.devID = thread_info->appDevID;
        intNotifyStatus.status = status;
        intNotifyStatus.param = param;
        
        rc = iPodPlayerIPCSend(thread_info->cmdQueueInfoClient, (U8 *)&intNotifyStatus, sizeof(IPOD_CORE_INT_PARAM_NOTIFY_STATUS), 0, IPODCORE_SEND_TMOUT);
        if(rc != sizeof(IPOD_CORE_INT_PARAM_NOTIFY_STATUS))
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, sizeof(IPOD_CORE_INT_PARAM_NOTIFY_STATUS));
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

/* Register the this callback function for notify the sample rate */
void iPodCoreCBNotifySamplerate(U32 newSample, S32 newSound, S32 newVolume, U32 iPodID)
{
    IPOD_CORE_INT_PARAM_SET_SAMPLE setSample;
    U32 size = sizeof(IPOD_CORE_INT_PARAM_SET_SAMPLE);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, newSample, newSound, newVolume, iPodID);
    
    /* Initialize the structure */
    memset(&setSample, 0, sizeof(setSample));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the notify samplerate change command with internal */
        setSample.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_CORE_INT_FUNC_SET_SAMPLERATE;
        setSample.header.devID = thread_info->appDevID;
        setSample.sampleRate = newSample;
        
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&setSample, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

S32 iPodCoreiPodCtrlGetOldiPodTrackInfoAnalyze(IPOD_PLAYER_TRACK_INFO *trackInfo, U32 *mask, IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData, 
                        const IPOD_TRACK_RELEASE_DATE_DATA* releaseData, const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData, U8* stringBuf)
{
    S32 rc = IPOD_PLAYER_OK;

    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, trackInfo, mask, infoType, capInfoData, releaseData, artworkCountData, stringBuf);
    
    /* Check parameter */
    if((trackInfo == NULL) || (mask == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, trackInfo, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    switch(infoType)
    {
        /* Type is capability */
        case IPOD_TRACK_CAP_INFO:
            if(capInfoData != NULL)
            {
                /* Track is audiobook */
                if((capInfoData->capability & IPODCORE_MASK_BIT_0) == IPODCORE_MASK_BIT_0)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_IS_AUDIOBOOK;
                }
                
                /* Track has chapter */
                if((capInfoData->capability & IPODCORE_MASK_BIT_1) == IPODCORE_MASK_BIT_1)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_CHAPTER;
                }
                
                /* Track has cover art */
                if((capInfoData->capability & IPODCORE_MASK_BIT_2) == IPODCORE_MASK_BIT_2)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_COVERART;
                }
                
                if((capInfoData->capability & IPODCORE_MASK_BIT_3) == IPODCORE_MASK_BIT_3)
                {
                    //trackInfo->cap |= IPOD_PLAYER_CAP_MASK_IS_PODCAST;
                }
                
                /* Track is podcast */
                if((capInfoData->capability & IPODCORE_MASK_BIT_4) == IPODCORE_MASK_BIT_4)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_IS_PODCAST;
                }
                
                /* Track has release date */
                if((capInfoData->capability & IPODCORE_MASK_BIT_5) == IPODCORE_MASK_BIT_5)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_RELEASE_DATE;
                }
                
                /* Track has description */
                if((capInfoData->capability & IPODCORE_MASK_BIT_6) == IPODCORE_MASK_BIT_6)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_DESCRIPTION;
                }
                
                /* Track contaiins the video */
                if((capInfoData->capability & IPODCORE_MASK_BIT_7) == IPODCORE_MASK_BIT_7)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_CONTAINTS_VIDEO;
                }
                
                /* Track exists as video */
                if((capInfoData->capability & IPODCORE_MASK_BIT_8) == IPODCORE_MASK_BIT_8)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_AS_VIDEO;
                }
                
                /* Track can create the Genius Playlist */
                if((capInfoData->capability & IPODCORE_MASK_BIT_13) == IPODCORE_MASK_BIT_13)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_INTELLIGENT;
                }
                
                /* Track is itunesU */
                if((capInfoData->capability & IPODCORE_MASK_BIT_14) == IPODCORE_MASK_BIT_14)
                {
                    trackInfo->capa |= IPOD_PLAYER_CAP_MASK_ITUENS;
                }
                
                /* Set indicated track length */
                trackInfo->length = capInfoData->trackLength;
                /* Set the chapter count that indicated track has */
                trackInfo->chapterCount = capInfoData->chapterCount;
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY | IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH | IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT;

            }
            break;
        
        /* Type is podcast name */
        case IPOD_TRACK_PODCAST_NAME:
            /* tbd. pointer length is unknown */
            if(stringBuf != NULL)
            {
                strncpy((char *)&trackInfo->podcastName[IPODCORE_POS0], (const char *)stringBuf, sizeof(trackInfo->podcastName));
                trackInfo->podcastName[sizeof(trackInfo->podcastName) - 1] = '\0';
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME;
            }
            break;
        
        /* Type is release date */
        case IPOD_TRACK_RELEASE_DATE:
            if(releaseData != NULL)
            {
                trackInfo->date.year     = releaseData->year;
                trackInfo->date.month    = releaseData->month;
                trackInfo->date.day      = releaseData->dayOfMonth;
                trackInfo->date.hours    = releaseData->hours;
                trackInfo->date.minutes  = releaseData->minutes;
                trackInfo->date.seconds  = releaseData->seconds;
                trackInfo->date.weekday  = (IPOD_PLAYER_WEEKDAY)releaseData->weekday;
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE;
            }
            break;
            
        
        /* Type is description */
        case IPOD_TRACK_DESCRIPTION:
            if(stringBuf != NULL)
            {
                strncpy((char *)&trackInfo->description[IPODCORE_POS0], (const char *)stringBuf, sizeof(trackInfo->description));
                trackInfo->description[sizeof(trackInfo->description) - 1] = '\0';
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION;
            }
            break;
            
        
        /* Type is lyrics */
        case IPOD_TRACK_SONG_LYRICS:
            if(stringBuf != NULL)
            {
                strncpy((char *)&trackInfo->lyric[IPODCORE_POS0], (const char *)stringBuf, sizeof(trackInfo->lyric));
                trackInfo->lyric[sizeof(trackInfo->lyric) - 1] = '\0';
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_LYRIC;
            }
            break;
            
        
        /* Type is genre */
        case IPOD_TRACK_GENRE:
            if(stringBuf != NULL)
            {
                strncpy((char *)&trackInfo->genre[IPODCORE_POS0], (const char *)stringBuf, sizeof(trackInfo->genre));
                trackInfo->genre[sizeof(trackInfo->genre) - 1] = '\0';
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_GENRE;
            }
            break;
            
        
        /* Type is composer */
        case IPOD_TRACK_COMPOSER:
            if(stringBuf != NULL)
            {
                strncpy((char *)&trackInfo->composer[IPODCORE_POS0], (const char *)stringBuf, sizeof(trackInfo->composer));
                trackInfo->composer[sizeof(trackInfo->composer) - 1] = '\0';
                *mask = IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER;
            }
            break;
            
        
        /* Type is artwork count */
        case IPOD_TRACK_ARTWORK_COUNT:
            if(artworkCountData != NULL)
            {
                //trackInfo->coverartCount = artworkCountData->count;
                //trackInfo->coverartCount = IPOD_PLAYER_TRACK_INFO_MASK_COVERART;
            }
            break;
            
        default: break;
    }
    
    return rc;
}

S32 iPodCoreiPodCtrlGetiPodTrackInfoAnalyze(IPOD_PLAYER_TRACK_INFO *trackInfo, U32 *mask, U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 capa = 0;

    IPOD_LOG_INFO_WRITE64(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, trackInfo, mask, trackIndex, infoType, infoData);
    
    
    if((trackInfo == NULL) || (mask == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, trackInfo, mask);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* For lint */
    trackIndex = trackIndex;
    
    switch(infoType)
    {
        /* Type is capability */
        case CAPABILITIES:
            memcpy((char *)&capa, (const char *)&infoData->caps, sizeof(capa));
            /* Track is audiobook */
            if(infoData->caps.isAudiobook)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_IS_AUDIOBOOK;
            }
            
            /* Track has chapter */
            if(infoData->caps.hasChapter)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_CHAPTER;
            }
            
            /* Track has cover art */
            if(infoData->caps.hasArtwork)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_COVERART;
            }
            
            if(infoData->caps.hasLyric)
            {
            }
            
            /* Track is podcast */
            if(infoData->caps.isPodcastEpi)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_IS_PODCAST;
            }
            
            /* Track has release date */
            if(infoData->caps.hasReleaseDate)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_RELEASE_DATE;
            }
            
            /* Track has description */
            if(infoData->caps.hasDescription)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_HAS_DESCRIPTION;
            }
            
            /* Track contaiins the video */
            if(infoData->caps.isVideo)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_CONTAINTS_VIDEO;
            }
            
            /* Track exists as video */
            if(infoData->caps.isQueuedAsVideo)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_AS_VIDEO;
            }
            
            /* Track can create the Genius Playlist */
            if((capa & IPODCORE_MASK_BIT_13) == IPODCORE_MASK_BIT_13)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_INTELLIGENT;
            }
            
            /* Track is itunesU */
            if((capa & IPODCORE_MASK_BIT_14) == IPODCORE_MASK_BIT_14)
            {
                trackInfo->capa |= IPOD_PLAYER_CAP_MASK_ITUENS;
            }
            
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY;

            break;
        
        /* Type is track name */
        case TRACK_NAME:
            /* tbd. pointer length is unknown */
            if(infoData->trackName != NULL)
            {
                strncpy((char *)&trackInfo->trackName[IPODCORE_POS0], (const char *)infoData->trackName, sizeof(trackInfo->trackName));
                trackInfo->trackName[sizeof(trackInfo->trackName) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME;
            break;
        
        case ARTIST_NAME:
            /* tbd. pointer length is unknown */
            if(infoData->artistName != NULL)
            {
                strncpy((char *)&trackInfo->artistName[IPODCORE_POS0], (const char *)infoData->artistName, sizeof(trackInfo->artistName));
                trackInfo->artistName[sizeof(trackInfo->artistName) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME;
            break;
        
        case ALBUM_NAME:
            /* tbd. pointer length is unknown */
            if(infoData->albumName != NULL)
            {
                strncpy((char *)&trackInfo->albumName[IPODCORE_POS0], (const char *)infoData->albumName, sizeof(trackInfo->albumName));
                trackInfo->albumName[sizeof(trackInfo->albumName) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME;
            break;
        
        case GENRE_NAME:
            /* tbd. pointer length is unknown */
            if(infoData->genreName != NULL)
            {
                strncpy((char *)&trackInfo->genre[IPODCORE_POS0], (const char *)infoData->genreName, sizeof(trackInfo->genre));
                trackInfo->genre[sizeof(trackInfo->genre) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_GENRE;
            break;
        
        case COMPOSER_NAME:
            /* tbd. pointer length is unknown */
            if(infoData->composerName != NULL)
            {
                strncpy((char *)&trackInfo->composer[IPODCORE_POS0], (const char *)infoData->composerName, sizeof(trackInfo->composer));
                trackInfo->composer[sizeof(trackInfo->composer) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER;
            break;
        
        case DURATION:
            trackInfo->length = (U32)infoData->duration;
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH;
            break;
        
        case UID:
            trackInfo->uID = infoData->trackUID;
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_UID;
            break;
        
        case CHAPTER_COUNT:
            trackInfo->chapterCount = infoData->chapterCount;
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT;
            break;
            
        case CHAPTER_TIMES:
            /* Chapter times does not get by track info */
            break;
        
        case CHAPTER_NAMES:
            /* Chapter names does not get by track info */
            break;
            
        /* Type is release date */
        case LYRIC_STRING:
            if(infoData->lyrics.lyric != NULL)
            {
                strncpy((char *)&trackInfo->lyric[IPODCORE_POS0], (const char *)infoData->lyrics.lyric, sizeof(trackInfo->lyric));
                trackInfo->lyric[sizeof(trackInfo->lyric) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_LYRIC;
            break;
            
        
        /* Type is description */
        case DESCRIPTION:
            if(infoData->description != NULL)
            {
                strncpy((char *)&trackInfo->description[IPODCORE_POS0], (const char *)infoData->description, sizeof(trackInfo->description));
                trackInfo->description[sizeof(trackInfo->description) - 1] = '\0';
            }
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION;
            break;
            
        
        /* Type is lyrics */
        case ALBUM_TRACK_INDEX:
            break;
            
        
        /* Type is genre */
        case DISC_SET_ALBUM_INDEX:
            break;
            
        
        /* Type is composer */
        case PLAY_COUNT:
            break;
            
        
        /* Type is artwork count */
        case SKIP_COUNT:
            break;
            
        /* Type is artwork count */
        case PODCAST_DATA:
            break;
        /* Type is artwork count */
        case LAST_PLAYED_DATA:
            trackInfo->date.year     = infoData->lastPlayedDate.year;
            trackInfo->date.month    = infoData->lastPlayedDate.month;
            trackInfo->date.day      = infoData->lastPlayedDate.day;
            trackInfo->date.hours    = infoData->lastPlayedDate.hour;
            trackInfo->date.minutes  = infoData->lastPlayedDate.min;
            trackInfo->date.seconds  = infoData->lastPlayedDate.sec;
            trackInfo->date.weekday  = (IPOD_PLAYER_WEEKDAY)IPOD_PLAYER_WEEK_UNKNOWN;
            *mask = IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE;
            break;
            
        /* Type is artwork count */
        case YEAR:
            break;
            
        /* Type is artwork count */
        case STAR_RATING:
            break;
            
        /* Type is artwork count */
        case SERIES_NAME:
            break;
        
        /* Type is artwork count */
        case SEASON_NUMBER:
            break;
            
        /* Type is artwork count */
        case TRACK_VOLUME_ADJUST:
            break;
            
        /* Type is artwork count */
        case EQ_PRESET:
            break;
            
        /* Type is artwork count */
        case SAMPLE_RATE:
            break;
            
        /* Type is artwork count */
        case BOOKMARK_OFFSET:
            break;
            
        /* Type is artwork count */
        case START_AND_STOP_OFFSET:
            break;
        default: break;
    }
    
    return rc;
}


/* Register the this callback function for notify the playback status changing */
void iPodCoreCBNotifyOldGetTrackInfo(IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData, 
                        const IPOD_TRACK_RELEASE_DATE_DATA* releaseData, const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
                        U8* stringBuf, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_GET_TRACK_INFO trackInfo;
    U32 size = sizeof(trackInfo);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, infoType, capInfoData, releaseData, artworkCountData, stringBuf, iPodID);
    
    /* Initialize the structure */
    memset(&trackInfo, 0, sizeof(trackInfo));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the get track info function ID with notification mask */
        trackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_TRACK_INFO);
        trackInfo.resultFlag = 1;
        
        iPodCoreiPodCtrlGetOldiPodTrackInfoAnalyze(&trackInfo.info, &trackInfo.trackInfoMask, infoType, capInfoData, releaseData, artworkCountData, stringBuf);
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&trackInfo, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
    
}

void iPodCoreCBNotifyGetTrackInfo(U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_GET_TRACK_INFO trackInfo;
    U32 size = sizeof(trackInfo);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE64(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, trackIndex, infoType, infoData);
    
    /* Initialize the structure */
    memset(&trackInfo, 0, sizeof(trackInfo));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* Set the get track info function ID with notification mask */
        trackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_TRACK_INFO);
        trackInfo.resultFlag = 1;
        iPodCoreiPodCtrlGetiPodTrackInfoAnalyze(&trackInfo.info, &trackInfo.trackInfoMask, trackIndex, infoType, infoData);
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&trackInfo, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}


/* Register the this callback function for notify the playback status changing */
void iPodCoreCBNotifyOldGetCurrentTrackInfo(IPOD_TRACK_INFO_TYPE infoType, const IPOD_TRACK_CAP_INFO_DATA* capInfoData, 
                        const IPOD_TRACK_RELEASE_DATE_DATA* releaseData, const IPOD_TRACK_ARTWORK_COUNT_DATA* artworkCountData,
                        U8* stringBuf, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_NOTIFY_TRACK_INFO trackInfo;
    U32 size = sizeof(trackInfo);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, infoType, capInfoData, releaseData, artworkCountData, stringBuf, iPodID);
    
    /* Initialize the structure */
    memset(&trackInfo, 0, sizeof(trackInfo));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        trackInfo.resultFlag = 1;
        /* Set the get track info function ID with notification mask */
        trackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_TRACK_INFO);
        iPodCoreiPodCtrlGetOldiPodTrackInfoAnalyze(&trackInfo.info, &trackInfo.mask, infoType, capInfoData, releaseData, artworkCountData, stringBuf);
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&trackInfo, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
    
}

void iPodCoreCBNotifyGetCurrentTrackInfo(U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_NOTIFY_TRACK_INFO trackInfo;
    U32 size = sizeof(trackInfo);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE64(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, trackIndex, infoType, infoData);
    
    /* Initialize the structure */
    memset(&trackInfo, 0, sizeof(trackInfo));
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        trackInfo.resultFlag = 1;
        
        /* Set the get track info function ID with notification mask */
        trackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_TRACK_INFO);
        
        iPodCoreiPodCtrlGetiPodTrackInfoAnalyze(&trackInfo.info, &trackInfo.mask, trackIndex, infoType, infoData);
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&trackInfo, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
}

void iPodCoreCBNotifyiPodNotification(IPOD_NOTIFY_TYPE type, IPOD_NOTIFY_STATUS status, const U32 iPodID)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_NOTIFY_DEVICE_EVENT deviceEvent;
    U32 size = sizeof(IPOD_PLAYER_PARAM_NOTIFY_DEVICE_EVENT);
    U64 btBit = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITEBIN(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, sizeof(status), &status);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, type, iPodID);
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thread_info);
        return;
    }
    
    /* Initialize the structure */
    memset(&deviceEvent, 0, sizeof(deviceEvent));
    
    /* Set the opend paramtetr */
    deviceEvent.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_DEVICE_EVENT;
    deviceEvent.header.devID = thread_info->appDevID;
    
    /* Check the type */
    switch(type)
    {
        /* Type is radio status */
        case IPOD_EVT_RADIO_TAG_STATUS:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_TAG;
            /* Check the status */
            switch(status.notifyStatus)
            {
                /* Status is tagging success */
                case 0:
                    deviceEvent.event.tagEvent.status = IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS_SUCCESS;
                    break;
                    
                /* Status is tagging fail */
                case 1:
                    deviceEvent.event.tagEvent.status = IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS_FAIL;
                    break;
                    
                /* Status is unknown */
                default:
                    deviceEvent.event.tagEvent.status = IPOD_PLAYER_DEVICE_EVENT_TAG_STATUS_UNKNOWN;
                    break;
            }
            
            break;
            
        /* Type is camera status */
        case IPOD_EVT_CAMERA_STATUS:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_CAMERA;
            /* Check the status */
            switch(status.notifyStatus)
            {
                /* Status is Camera off */
                case 0:
                    deviceEvent.event.cameraEvent.status = IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_OFF;
                    break;
                    
                /* Status is Camera preview */
                case 3:
                    deviceEvent.event.cameraEvent.status = IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_PREVIEW;
                    break;
                
                /* Status is Camera recording */
                case 4:
                    deviceEvent.event.cameraEvent.status = IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_RECORDING;
                    break;
                    
                /* Status is unknown */
                default:
                    deviceEvent.event.cameraEvent.status = IPOD_PLAYER_DEVICE_EVENT_CAMERA_STATUS_UNKNOWN;
                    break;
            }
            
            break;
            
        /* Type is charging */
        case IPOD_EVT_CHARGING:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_CHARGING;
            deviceEvent.event.chargingEvent.type = IPOD_PLAYER_DEVICE_EVENT_CHARGING_TYPE_CUR;
            deviceEvent.event.chargingEvent.status = status.availableCurrent;
            break;
            
        /* Type is database change */
        case IPOD_EVT_DB_CHANGED:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_DATABASE;
            break;
        
        /* Type is focus application */
        case IPOD_EVT_NOW_FOCUS_APP:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPP;
            strncpy((char *)&deviceEvent.event.playingiOSAppEvent.appName[IPODCORE_POS0], (const char *)status.focusApp, sizeof(deviceEvent.event.playingiOSAppEvent.appName));
            deviceEvent.event.playingiOSAppEvent.appName[sizeof(deviceEvent.event.playingiOSAppEvent.appName) - 1] = '\0';
            
            break;
            
        /* Type is iPodOut */
        case IPOD_EVT_IPOD_OUT:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_OUT;
            
            /* Status check */
            switch(status.notifyStatus)
            {
                /* Status is iPodOut inactive */
                case 0:
                    deviceEvent.event.outEvent.status = IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS_INACTIVE;
                    break;
                    
                /* Status is iPodOut active */
                case 1:
                    deviceEvent.event.outEvent.status = IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS_ACTIVE;
                    break;
                    
                /* Status is unknown */
                default:
                    deviceEvent.event.outEvent.status = IPOD_PLAYER_DEVICE_EVENT_OUT_STATUS_UNKNOWN;
                    break;
            }
            
            break;
            
        /* Type is bluetooth status */
        case IPOD_EVT_BT_STATUS:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_BT;
            memcpy((char *)deviceEvent.event.btEvent.macAddr, (const char *)status.BTStatus, 6);
            memcpy((char *)&btBit, (const char *)&status.BTStatus[6], sizeof(btBit));
            
            /* Check the profile */
            switch(btBit)
            {
                /* Profile is HFP */
                case 0x0001:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_HFP;
                    break;
                    
                /* Profile is PBAP */
                case 0x0002:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_PBAP;
                    break;
                    
                /* Profile is AVRCP */
                case 0x0008:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_AVRCP;
                    break;
                    
                /* Profile is A2DP */
                case 0x0010:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_A2DP;
                    break;
                    
                /* Profile is HID */
                case 0x0020:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_HID;
                    break;
                    
                /* Profile is iAP */
                case 0x0080:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_AiP;
                    break;
                    
                /* Profile is PAN-NAP */
                case 0x0100:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_PANA;
                    break;
                    
                /* Profile is PAN-U */
                case 0x1000:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_PANU;
                    break;
                    
                /* Profile is unknown */
                default:
                    deviceEvent.event.btEvent.profileList = IPOD_PLAYER_PROFILE_UNKNOWN;
                    break;
            }
            break;
            
        /* Type is Application display name  */
        case IPOD_EVT_APP_DISPLAY_NAME:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_IOSAPPFULL ;
            strncpy((char *)&deviceEvent.event.iOSAppFullEvent.fullName[IPODCORE_POS0], 
                    (const char *)status.focusApp, sizeof( deviceEvent.event.iOSAppFullEvent.fullName));
            deviceEvent.event.iOSAppFullEvent.fullName[sizeof( deviceEvent.event.iOSAppFullEvent.fullName) - 1] = '\0';
            
            break;
            
        /* Type is Assistive touch */
        case IPOD_EVT_ASSIST_TOUCH:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_ASSISTIVE;
            switch(status.notifyStatus)
            {
                case 0:
                    deviceEvent.event.assitiveEvent.status = IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS_OFF;
                    break;
                default:
                    deviceEvent.event.assitiveEvent.status = IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS_ON;
                    break;
            }
            break;
            
        /* Type is unknown */
        default:
            deviceEvent.type = IPOD_PLAYER_DEVICE_EVENT_TYPE_UNKNOWN;
            break;
    }


    if(deviceEvent.type != IPOD_PLAYER_DEVICE_EVENT_TYPE_UNKNOWN)
    {
        /* Notify the device event to Application */
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&deviceEvent, size, 0, IPODCORE_SEND_TMOUT);
        if(rc == (S32)size)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);

    return;
}


void iPodCoreCBNotifyGetChapterInfo(U64 trackIndex, IPOD_TRACK_INFORMATION_TYPE infoType, IPOD_TRACK_INFORMATION_DATA *infoData, const U32 iPodID)
{
    IPOD_PLAYER_PARAM_GET_CHAPTER_INFO chapterInfo;
    U32 size = sizeof(chapterInfo);
    S32 rc = 0;
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE64(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, trackIndex, infoType, infoData);
    
    if(infoData == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, infoData);
        return;
    }
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        /* For lint */
        trackIndex = trackIndex;
        
        /* Initialize the structure */
        memset(&chapterInfo, 0, sizeof(chapterInfo));
        
        /* Set the get track info function ID with notification mask */
        chapterInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_CHAPTER_INFO);
        
        /* Check the type */
        switch(infoType)
        {
            /* Type is chapter times */
            case CHAPTER_TIMES:
                /* Set the length, index and mask */
                chapterInfo.info.length = infoData->chapterTimes.offset;
                chapterInfo.chapterIndex = infoData->chapterTimes.chapterIndex;
                chapterInfo.chapterInfoMask = IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH;
                break;
            
            /* Type is chapter name */
            case CHAPTER_NAMES:
                
                if(infoData->chapterNames.name != NULL)
                {
                    /* Set the chapter name, index and mask */
                    strncpy((char *)&chapterInfo.info.chapterName[IPODCORE_POS0], 
                            (const char*)infoData->chapterNames.name, sizeof(chapterInfo.info.chapterName));
                    chapterInfo.info.chapterName[sizeof(chapterInfo.info.chapterName) - 1] = '\0';
                    chapterInfo.chapterIndex = infoData->chapterNames.chapterIndex;
                    chapterInfo.chapterInfoMask = IPOD_PLAYER_CHAPTER_INFO_MASK_NAME;
                }
                break;
            default: break;
        }
        
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&chapterInfo, size, 0, IPODCORE_SEND_TMOUT);
        if(rc != (S32)size)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_IPC_ERROR, rc, size);
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

void iPodCoreCBRemoteEventNotification(IPOD_STATE_INFO_TYPE eventNum, IPOD_REMOTE_EVENT_NOTIFY_STATUS eventData, const U32 iPodID)
{
    IPOD_PLAYER_CORE_THREAD_INFO *thread_info = NULL;
    IPOD_CORE_INT_PARAM_REMOTE_EVENT_NOTIFICATION remoteEvent;
    U32 size = sizeof(remoteEvent);
    S32 rc = IPOD_PLAYER_OK;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, eventNum, eventData.shuffle, eventData.repeat, iPodID);
    
    memset(&remoteEvent, 0, size);
    remoteEvent.repeatStatus = 0xff;
    remoteEvent.shuffleStatus = 0xff;
    
    /* get current thread information */
    thread_info = iPodCoreiPodCtrlGetThreadInfo(iPodID);
    if(thread_info != NULL)
    {
        switch(eventNum)
        {
            case IPOD_STATE_INFO_SHUFFLE:
            {
                remoteEvent.shuffleStatus = eventData.shuffle;
                break;
            }
            case IPOD_STATE_INFO_REPEAT:
            {
                remoteEvent.repeatStatus = eventData.repeat;
                break;
            }

            /* track position */
            case IPOD_STATE_INFO_TRACK_POSITION_MS:
            /* track index */
            case IPOD_STATE_INFO_TRACK_INDEX:
            /* chapter information */
            case IPOD_STATE_INFO_CHAPTER_INFO:
            /* play status */
            case IPOD_STATE_INFO_PLAY_STATUS:
            /* mute volume */
            case IPOD_STATE_INFO_MUTE_VOLUME:
            /* power buttery */
            case IPOD_STATE_INFO_POWER_BATTERY:
            /* equalizer state */
            case IPOD_STATE_INFO_EQUALIZER_STATE:
            /* date time information */
            case IPOD_STATE_INFO_DATE_TIME:
            /* blacklight information */
            case IPOD_STATE_INFO_BACKLIGHT:
            /* hold switch information */
            case IPOD_STATE_INFO_HOLD_SWITCH:
            /* sound check state */
            case IPOD_STATE_INFO_SOUND_CHECK:
            /* audio book */
            case IPOD_STATE_INFO_AUDIOBOOK_SPEED:
            /* track position sec */
            case IPOD_STATE_INFO_TRACK_POSITION_S:
            /* mute extended volume */
            case IPOD_STATE_INFO_MUTE_EXTENDED_VOLUME:
            /* track capabilities changed */
            case IPOD_STATE_INFO_TRACK_CAPABILITIES:
            /* number of tracks in playlist */
            case IPOD_STATE_INFO_NUM_OF_TRACKS_IN_PLAYLIST:
            /* alarm information */
            case IPOD_STATE_INFO_ALARM:
            {
                IPOD_DLT_ERROR(" RemoteEventNotification not use event number = 0x%x \n", eventNum);
                break;
            }

            default:
            {
                IPOD_DLT_ERROR(" Unknown / Depreciated RemoteEventNotification 0x%x \n", eventNum);
                break;
            }
        }
        
       /* Notify the device event to Application */
        remoteEvent.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_INTERNAL_REMOTE_EVENT_NOTIFICATION;
        rc = iPodPlayerIPCSend(thread_info->queueInfoClient, (U8 *)&remoteEvent, size, 0, IPODCORE_SEND_TMOUT);
        if(rc == (S32)size)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
            IPOD_DLT_ERROR("Could not send IPC for remote event notification rc = %d", rc);
        }
    }

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

/* Register the each callback function to iPod ctrl */
S32 iPodCoreiPodCtrlRegisterCB()
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    rc = iPodRegisterCBUSBAttach(iPodCoreCBAttach);
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBUSBDetach(iPodCoreCBDetach);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBNotifyStatus(iPodCoreCBNotify);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBNewiPodTrackInfo(iPodCoreCBNotifySamplerate);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBOpenDataSession(iPodCoreCBOpenApp);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBCloseDataSession(iPodCoreCBCloseApp);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBiPodDataTransfer(iPodCoreCBReceiveFromApp);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBNotification(iPodCoreCBNotifyiPodNotification);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBRemoteEventNotification(iPodCoreCBRemoteEventNotification);
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void iPodCoreiPodCtrlDeleteCB()
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    rc = iPodRegisterCBUSBAttach(NULL);
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBUSBDetach(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBNotifyStatus(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBNewiPodTrackInfo(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBOpenDataSession(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBCloseDataSession(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBiPodDataTransfer(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBNotification(NULL);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodRegisterCBRemoteEventNotification(NULL);
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return;
}

IPOD_PLAYER_REPEAT_STATUS iPodConvertRepeatStatus(U8 repeatStatus)
{
    IPOD_PLAYER_REPEAT_STATUS status;
   
    switch(repeatStatus)
    {
        case IPOD_REPEAT_OFF:
            status = IPOD_PLAYER_REPEAT_OFF;
            break;
        case IPOD_REPEAT_ONE_TRACK:
            status = IPOD_PLAYER_REPEAT_ONE;
            break;
        case IPOD_REPEAT_ALL_TRACKS:
            status = IPOD_PLAYER_REPEAT_ALL;
            break;
        default:
            status = IPOD_PLAYER_REPEAT_UNKNOWN;
            IPOD_DLT_ERROR("Could not convert repeat status (status = %d)", repeatStatus);
            break;
    }
    
    return status;
}

IPOD_PLAYER_SHUFFLE_STATUS iPodConvertShuffleStatus(U8 shuffleStatus)
{
    IPOD_PLAYER_SHUFFLE_STATUS status;
    
    switch(shuffleStatus)
    {
        case IPOD_SHUFFLE_OFF:
            status = IPOD_PLAYER_SHUFFLE_OFF;
            break;
        case IPOD_SHUFFLE_TRACKS:
            status = IPOD_PLAYER_SHUFFLE_TRACKS;
            break;
        case IPOD_SHUFFLE_ALBUMS:
            status = IPOD_PLAYER_SHUFFLE_ALBUMS;
            break;
        default:
            status = IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;
            IPOD_DLT_ERROR("Could not convert shuffle status (status = %d)", shuffleStatus);
            break;
    }
    
    return status;
}

/* Notify the playback status to Application */
S32 iPodCoreiPodCtrlNotifyPlaybackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_NOTIFY_PLAYBACK_STATUS playbackStatus;
    U32 size = sizeof(playbackStatus);
    U8 *repeatStatus = &(iPodCtrlCfg->iPodInfo->repeatStatus);
    U8 *shuffleStatus = &(iPodCtrlCfg->iPodInfo->shuffleStatus);
    

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&playbackStatus, 0, sizeof(playbackStatus));
    
    /* check repeat mode */
    if(*repeatStatus == IPOD_PLAYER_REPEAT_UNKNOWN)
    {
        /* get repeat mode from ipod_ctrl */
        rc = iPodCoreFuncGetRepeat(iPodCtrlCfg->threadInfo->appDevID, repeatStatus);
    }
    iPodCtrlCfg->playbackStatus.repeatStatus = iPodConvertRepeatStatus(*repeatStatus);
    
    /* check shuffle mode */
    if(*shuffleStatus == IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN)
    {
        /* get shuffle mode from ipod_ctrl */
        rc = iPodCoreFuncGetShuffle(iPodCtrlCfg->threadInfo->appDevID, shuffleStatus);
    }
    iPodCtrlCfg->playbackStatus.shuffleStatus = iPodConvertShuffleStatus(*shuffleStatus);
    
    /* Set the notify playback status command */
    playbackStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_PLAYBACK_STATUS;
    playbackStatus.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    memcpy(&playbackStatus.status, &iPodCtrlCfg->playbackStatus, sizeof(playbackStatus.status));
    
    /* Notify status is iPod status. */
    playbackStatus.status.status = iPodCtrlCfg->iPodInfo->status;
    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&playbackStatus, size, 0, IPODCORE_SEND_TMOUT);
    if(rc == (S32)size)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, contents->notifyPlaybackStatus.status.status);
    
    return rc;
}


/* Application requests the init of iPodPlayer */
S32 iPodCoreiPodCtrlFuncInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, 
                             IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS notifyStatus;
    IPOD_PLAYER_PARAM_NOTIFY_OPEN_APP openApp;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&notifyStatus, 0, sizeof(notifyStatus));
    memset(&openApp, 0, sizeof(openApp));
    
    /* If this thread is created, current connection status informs to Application */
    /* Notify the current connection status to connected application */
    notifyStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
    notifyStatus.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    
    /* Copy the connection status */
    memcpy(&notifyStatus.status, &iPodCtrlCfg->deviceConnection, sizeof(notifyStatus.status));
    
    /* Send the request that informs the iPodCore of the connection status */
    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyStatus, sizeof(notifyStatus), 
                           0, IPODCORE_TMOUT_FOREVER);
    if(rc == sizeof(notifyStatus))
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        openApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_OPEN_APP;
        openApp.header.devID = iPodCtrlCfg->threadInfo->appDevID;
        
        for(i = 0; (i < IPODCORE_MAX_IOSAPPS_INFO_NUM) && (rc == IPOD_PLAYER_OK); i++)
        {
            /* iOSApplication is opened */
            if(iPodCtrlCfg->iOSAppID[i].isReady != 0)
            {
                /* Notify the Application of opened application */
                openApp.index = i;
                openApp.appID = iPodCtrlCfg->iOSAppID[i].appID;
                openApp.session = iPodCtrlCfg->iOSAppID[i].sessionID;
                /* Send the request that informs the iPodCore of the opened iOS application to iPodCore*/
                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&openApp, sizeof(openApp), 
                                       0, IPODCORE_TMOUT_FOREVER);
                if(rc == sizeof(openApp))
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
    
    /* Retunr error is always no reply to Application */
    rc = IPOD_PLAYER_ERR_NO_REPLY;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Application requests the deinit of iPodPlayer */
S32 iPodCoreiPodCtrlFuncDeInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERR_NO_REPLY;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Currently iPodPlayer is nothing to do. */
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Application requests to select the audio output */
S32 iPodCoreiPodCtrlFuncSelectAudioOut(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERR_NO_REPLY;
    U32 devID = 0;
    IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS connectionStatus;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* For lint */
    header = header;
    
    /* Initialize the structure */
    memset(&connectionStatus, 0, sizeof(connectionStatus));
    
    
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID);

    if((waitData->contents.selectAudioOut.mode == IPOD_PLAYER_AUDIO_ANALOG) ||
       (waitData->contents.selectAudioOut.mode == IPOD_PLAYER_AUDIO_DIGITAL))
    {
        rc = iPodCoreFuncSwitchAudio(devID, waitData->contents.selectAudioOut.mode);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
            
            /* Notify the current connection status to connected application */
            connectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
            connectionStatus.header.devID = iPodCtrlCfg->threadInfo->appDevID;
            
            /* Copy the connection status */
            memcpy(&connectionStatus.status, &iPodCtrlCfg->deviceConnection, sizeof(connectionStatus.status));
            rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&connectionStatus, sizeof(connectionStatus), 0, IPODCORE_TMOUT_FOREVER);
            if(rc == sizeof(connectionStatus))
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    *size = sizeof(waitData->contents.selectAudioOutResult);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Application requests the start authentication */
S32 iPodCoreiPodCtrlFuncStartAuthentication(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERR_NO_REPLY;
    U32 devID = 0;
    IPOD_PLAYER_PARAM_NOTIFY_CONNECTION_STATUS connectionStatus;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Initialize the structure */
    memset(&connectionStatus, 0, sizeof(connectionStatus));
    
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID);

    rc = iPodCoreFuncStartAuthentication(devID);
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
        
        /* Notify the current connection status to connected application */
        connectionStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CONNECTION_STATUS;
        connectionStatus.header.devID = iPodCtrlCfg->threadInfo->appDevID;
        
        /* Copy the connection status */
        memcpy(&connectionStatus.status, &iPodCtrlCfg->deviceConnection, sizeof(connectionStatus.status));
        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&connectionStatus, sizeof(connectionStatus), 0, IPODCORE_TMOUT_FOREVER);
        if(rc == sizeof(connectionStatus))
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    *size = sizeof(waitData->contents.startAuthenticationResult);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Application requests the deinit of iPodPlayer */
S32 iPodCoreiPodCtrlFuncTestReady(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERR_NO_REPLY;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Currently iPodPlayer is nothing to do. */
    
    /* todo Must remove all wait list */
    *size = sizeof(waitData->contents.testReadyResult);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

static S32 iPodCoreiPodCtrlRequestCurrentTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 devID, U32 reqIndex)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_CORE_INT_PARAM_GET_TRACK_INFO intGetTrackInfo;
    U32 trackIndex = 0;
    
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&intGetTrackInfo, 0, sizeof(intGetTrackInfo));
    
    /* Request index is unknown. In that case this function gets the current track index. */
    if(reqIndex == IPOD_PLAYER_TRACK_INDEX_UNKNOWN)
    {
        /* Get the current playing track index because Apple device may not inform the new index */
        rc = iPodCoreFuncIntGetCurrentTrackIndex(devID, &trackIndex);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCtrlCfg->playbackStatus.track.index = trackIndex;
        }
        else
        {
            iPodCtrlCfg->playbackStatus.track.index = IPOD_PLAYER_TRACK_INDEX_UNKNOWN;
        }
    }
    else
    {
        rc = IPOD_PLAYER_OK;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Application requests to get the current playing track information */
        if(iPodCtrlCfg->iPodInfo->trackInfoMask != 0)
        {
            intGetTrackInfo.header.funcId = IPOD_FUNC_TRACK_INFO;
            intGetTrackInfo.header.devID = devID;
            intGetTrackInfo.trackIndex = iPodCtrlCfg->playbackStatus.track.index;
            /* Internal request sends for current playing track information */
            rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&intGetTrackInfo, sizeof(intGetTrackInfo), 0, IPODCORE_TMOUT_FOREVER);
            if(rc == sizeof(intGetTrackInfo))
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
    
    return rc;
}

/* iPod status changes to Play */
S32 iPodCoreiPodCtrlStatusChangeToPlay(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_CORE_INT_PARAM_GET_STATUS getStatus;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents);
    
    /* Check the parameter */
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&getStatus, 0, sizeof(getStatus));
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, iPodCtrlCfg->iPodInfo->status);
    
    /* Check the iPod Status*/
    switch(iPodCtrlCfg->iPodInfo->status)
    {
        case IPOD_PLAYER_PLAY_STATUS_PLAY:
            /* Nothing to do */
            break;
        case IPOD_PLAYER_PLAY_STATUS_STOP:
            rc = iPodCoreFuncClearSelection(devID, IPOD_PLAYER_DB_TYPE_ALL, iPodCtrlCfg->iPodInfo->topType, iPodCtrlCfg->iPodInfo->curType);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Clear the category information */
                iPodCtrlCfg->iPodInfo->topType = IPOD_PLAYER_DB_TYPE_ALL;
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                iPodCtrlCfg->iPodInfo->curTotalType = IPOD_PLAYER_DB_TYPE_UNKNOWN;
                iPodCtrlCfg->iPodInfo->curTotalCount = 0;
                
                rc = iPodCoreFuncGetLowerCatList(devID, iPodCtrlCfg);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Play by direct play if current iPod status is stop */
                    rc = iPodCoreFuncPlayTrack(devID, IPOD_PLAYER_TRACK_TYPE_DATABASE, (U64)0);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        rc = iPodCoreiPodCtrlRequestCurrentTrackInfo(iPodCtrlCfg, devID, IPOD_PLAYER_TRACK_INDEX_UNKNOWN);
                    }
                    else
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                    }
                }
            }
            break;

        case IPOD_PLAYER_PLAY_STATUS_PAUSE:
        case IPOD_PLAYER_PLAY_STATUS_UNKNOWN:
            
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request the Play to iPod Ctrl */
                rc = iPodCoreFuncPlay(devID);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
            }
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_FF:
        case IPOD_PLAYER_PLAY_STATUS_RW:
            /* Request the FF/RW end to iPod Ctrl */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Connected Apple device is old */
                if(iPodCtrlCfg->oldiPod != 0)
                {
                    /* iPod status changes to Play */
                    /* Old Apple device does not inform FF/REW status. Therefore status change from FF/REW must be done here. */
                    /* Otherwise iPodPlayer does not know actual status of Apple device */
                    iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_PLAY;
                }
                
                /* Previous status is only play or pause. */
                if(iPodCtrlCfg->iPodInfo->prevStatus == IPOD_PLAYER_PLAY_STATUS_PAUSE)
                {
                    /* Remove previous status*/
                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                }
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->prevStatus);
            }
            break;
        default: break;
    }
    
    return rc;
}

/* iPod Status changes to Pause */
S32 iPodCoreiPodCtrlStatusChangeToPause(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_CORE_INT_PARAM_GET_STATUS getStatus;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents);
    
    /* Check the parameter */
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&getStatus, 0, sizeof(getStatus));
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, iPodCtrlCfg->iPodInfo->status);
    /* Check the iPodStatus*/
    switch(iPodCtrlCfg->iPodInfo->status)
    {
        case IPOD_PLAYER_PLAY_STATUS_PLAY:
        case IPOD_PLAYER_PLAY_STATUS_UNKNOWN:
            /* To avoid ATS error, anyway call the end FF/RW */
            rc =iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request the Pause to iPod Ctrl */
                rc = iPodCoreFuncPause(devID);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
            }
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_STOP:
            /* Set the error of invalid status */
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->status);
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_PAUSE:
            /* Nothing to do */
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_FF:
        case IPOD_PLAYER_PLAY_STATUS_RW:
            /* Request the FF/RW end to iPod Ctrl */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Connected Apple device is old */
                if(iPodCtrlCfg->oldiPod != 0)
                {
                    /* iPod status changes to Play */
                    /* Old Apple device does not inform FF/REW status. Therefore status change from FF/REW must be done here. */
                    /* Otherwise iPodPlayer does not know actual status of Apple device */
                    iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_PAUSE;
                }
                
                /* Previous status is only play or pause. */
                if(iPodCtrlCfg->iPodInfo->prevStatus == IPOD_PLAYER_PLAY_STATUS_PLAY)
                {
                    /* Rmove previous status*/
                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                }
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->prevStatus);
            }
            break;
        default: break;
    }
    
    return rc;
}

/* iPod status changes to Stop */
S32 iPodCoreiPodCtrlStatusChangeToStop(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_CORE_INT_PARAM_GET_STATUS getStatus;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents);
    
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&getStatus, 0, sizeof(getStatus));
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, iPodCtrlCfg->iPodInfo->status);
    /* Check the iPodStatus*/
    switch(iPodCtrlCfg->iPodInfo->status)
    {
        case IPOD_PLAYER_PLAY_STATUS_PLAY:
        case IPOD_PLAYER_PLAY_STATUS_PAUSE:
        case IPOD_PLAYER_PLAY_STATUS_UNKNOWN:
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request the Stop to iPod Ctrl */
                rc = iPodCoreFuncStop(devID);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
            }
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_STOP:
            /* Nothing to do */
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_FF:
        case IPOD_PLAYER_PLAY_STATUS_RW:
            /* Request the FF/RW end to iPod Ctrl */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request the Stop to iPod Ctrl */
                rc = iPodCoreFuncStop(devID);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
            }
            break;
        default: break;
    }
    
    return rc;
}


/* iPod status changes to FastForward */
S32 iPodCoreiPodCtrlStatusChangeToFastForward(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, iPodCtrlCfg->iPodInfo->status);
    /* Check the iPodStatus*/
    switch(iPodCtrlCfg->iPodInfo->status)
    {
        case IPOD_PLAYER_PLAY_STATUS_RW:
        case IPOD_PLAYER_PLAY_STATUS_PLAY:
        case IPOD_PLAYER_PLAY_STATUS_PAUSE:
        case IPOD_PLAYER_PLAY_STATUS_UNKNOWN:
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request to chagne the status to FastForward */
                rc = iPodCoreFuncFastForward(devID);
                if(rc == IPOD_PLAYER_OK)
                {
                    if(iPodCtrlCfg->oldiPod != 0)
                    {
                        /* If iPod is old, iPod does not notify the status change */
                        /* so, iPodPlayer changes the iPod status to FastForward internally */
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_FF;
                        /* Notify Application of current status */
                        rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
                    }
                    else
                    {
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                    }
                }
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
            }
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_STOP:
            /* If current status is Stop, iPodPlayer can not request to change the status to FastForward */
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, IPOD_PLAYER_PLAY_STATUS_STOP);
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_FF:
            /* Nothing to do */
            break;
            
        default: break;
    }
    
    return rc;
}

/* iPod status changes to Rewind */
S32 iPodCoreiPodCtrlStatusChangeToRewind(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, iPodCtrlCfg->iPodInfo->status);
    /* Check the iPodStatus*/
    switch(iPodCtrlCfg->iPodInfo->status)
    {
        case IPOD_PLAYER_PLAY_STATUS_FF:
        case IPOD_PLAYER_PLAY_STATUS_PLAY:
        case IPOD_PLAYER_PLAY_STATUS_PAUSE:
        case IPOD_PLAYER_PLAY_STATUS_UNKNOWN:
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Apple device does not send End FF/FB status when track elapsed time is less than about 2 seconds */
                /* Because some iPods does not change the status to Rewind */
                if(iPodCtrlCfg->playbackStatus.track.time < IPODCORE_NO_END_NOTIFY_TIME)
                {
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->prevStatus);
                    if(iPodCtrlCfg->iPodInfo->prevStatus != IPOD_PLAYER_PLAY_STATUS_UNKNOWN)
                    {
                        iPodCoreFuncGotoTrackPosition(devID, 0);
                        iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
                        rc = IPOD_PLAYER_ERROR;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->prevStatus);
                    }
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Request to change the status to Rewind */
                    rc = iPodCoreFuncRewind(devID);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        if(iPodCtrlCfg->oldiPod != 0)
                        {
                            /* If iPod is old, iPod does not notify the statu change */
                            /* so, iPodPlayer changes the iPod status to Rewind internally */
                            iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_RW;
                            /* Notify Application of current status */
                            rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
                        }
                        else
                        {
                            iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                        }
                    }
                }
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
                rc = IPOD_PLAYER_OK;
            }
            
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_STOP:
            /* Set the error of invalid status */
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, IPOD_PLAYER_PLAY_STATUS_STOP);
            break;
            
        case IPOD_PLAYER_PLAY_STATUS_RW:
            /* Nothing to do */
            break;
            
        default: break;
    }
    
    return rc;
}

/* Playback status changes to Play */
S32 iPodCoreiPodCtrlPlay(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* check parameter */
        if(!(waitData->contents.play.playCurSel))
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
            /* Check the current playback status */
            if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_PLAY)
            {
                /* Current status is not play. Playback status changes to play */
                iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_PLAY;
            }
            
            /* Change the iPod status by playback status */
            rc = iPodCoreiPodCtrlStatusChangeToPlay(iPodCtrlCfg, contents);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
            IPOD_DLT_ERROR("iAP1 could not support the play current selection message.");
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_PLAY_RESULT);
    /* Set Play Current Selection flag */
    waitData->contents.playResult.playCurSel = waitData->contents.play.playCurSel;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Playback status changes to Pause */
S32 iPodCoreiPodCtrlPause(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_PAUSE)
        {
            /* Current status is not pause. Playback status changes to pause */
            iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_PAUSE;
        }
        
        /* Change the iPod status by playback status */
        rc = iPodCoreiPodCtrlStatusChangeToPause(iPodCtrlCfg, contents);
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_PAUSE_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Playback status changes to Stop */
S32 iPodCoreiPodCtrlStop(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_STOP)
        {
            /* Current status is not stop. Playback status changes to stop */
            iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_STOP;
        }

        /* Change the iPod status by playback status */
        rc = iPodCoreiPodCtrlStatusChangeToStop(iPodCtrlCfg, contents);
    }
    else
    {
        /* Status changet to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_STOP_RESULT);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Playback status changes to FastForward*/
S32 iPodCoreiPodCtrlFastForward(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_FF)
        {
            /* Current status is not fastforward. */
            if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_RW)
            {
                /* Current status is not Rewind. Current status memories as previous status */
                iPodCtrlCfg->iPodInfo->prevStatus = iPodCtrlCfg->playbackStatus.status;
            }
            
            /* Playback status changes to fastforward */
            iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_FF;

        }
        
        /* Change the iPod status by playback status */
        rc = iPodCoreiPodCtrlStatusChangeToFastForward(iPodCtrlCfg, contents);
    }
    else
    {
        /* Status changet to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_FASTFORWARD_RESULT);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Playback status changes to Rewind*/
S32 iPodCoreiPodCtrlRewind(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_RW)
        {
            /* Current status is not Rewind */
            if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_FF)
            {
                /* Current status is not fastforward. Current status memories as previous status */
                iPodCtrlCfg->iPodInfo->prevStatus = iPodCtrlCfg->playbackStatus.status;
            }
            
            /* Playback status changes to rewind */
            iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_RW;
        }
        rc = iPodCoreiPodCtrlStatusChangeToRewind(iPodCtrlCfg, contents);
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_REWIND_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the playing track to Next */
S32 iPodCoreiPodCtrlNextTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    U32 totalCount = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current status */
        if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW) ||
        (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF))
        {
            /* Current status is fastforward or rewind. These must call FF/RW end command before call the next track command. otherwise, ATS errro is occurred */
            /* Current status must change to previous status */
            iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
            /* Previous status remove. Change to unknown status */
            iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        }
        
        /* Check that the current status is not Stop */
        if(iPodCtrlCfg->playbackStatus.status != IPOD_PLAYER_PLAY_STATUS_STOP)
        {
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                rc = iPodCoreFuncGetTrackTotalCount(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, &totalCount);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Check the current playing track */
                    if(iPodCtrlCfg->playbackStatus.track.index == totalCount)
                    {
                        /* Current playing track is last of playback list */
                        /* Repeat status is repeat of */
                        if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_OFF)
                        {
                            /* Playing status changes to STOP */
                            rc = iPodCoreiPodCtrlStatusChangeToStop(iPodCtrlCfg, contents);
                        }
                        /* Repeat status is repeat one track */
                        else if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_ONE)
                        {
                            /* Call prev track */
                            rc = iPodCoreFuncGotoTrackPosition(devID, 0);
                        }
                        /* Repeat status is repeat all tracks in playback list */
                        else if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_ALL)
                        {
                            rc = iPodCoreFuncPlayTrack(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, 0);
                        }
                    }
                    else
                    {
                        if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_ONE)
                        {
                            /* Call prev track */
                            rc = iPodCoreFuncGotoTrackPosition(devID, 0);
                        }
                        else
                        {
                            /* Move next track */
                            rc = iPodCoreFuncNextTrack(devID);
                        }
                    }
                }
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_STATUS, iPodCtrlCfg->playbackStatus.status);
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_NEXTTRACK_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Change the playing track to previous track */
S32 iPodCoreiPodCtrlPrevTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 prevTrackIndex = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW) ||
            (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF))
        {
            /* Current status is fastforward or rewind. These must call FF/RW end command before call the previous track command */
            /* Current status must change to previous status */
            iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
            /* Previous status remove. Change to unknown status */
            iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        }
        
        /* Check that current status is not stop*/
        if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_STOP)
        {
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->playbackStatus.status);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                
                /* Check the current playing track */
                if(iPodCtrlCfg->playbackStatus.track.index == 0)
                {
                    /* Current playing track is top of playback list */
                    /* Repeat status is repeat of */
                    if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_OFF)
                    {
                        /* Playing status changes to STOP */
                        rc = iPodCoreiPodCtrlStatusChangeToStop(iPodCtrlCfg, contents);
                    }
                    /* Repeat status is repeat one track */
                    else if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_ONE)
                    {
                        /* Call prev track */
                        rc = iPodCoreFuncGotoTrackPosition(devID, 0);
                    }
                    /* Repeat status is repeat all tracks in playback list */
                    else if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_ALL)
                    {
                        /* Get the total count of current playback list */
                        rc = iPodCoreFuncGetTrackTotalCount(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, &prevTrackIndex);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            prevTrackIndex--;
                            rc = iPodCoreFuncPlayTrack(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, (U64)prevTrackIndex);
                            if(rc == IPOD_PLAYER_OK)
                            {
                                /* Current status is pause */
                                if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_PAUSE)
                                {
                                    /* Status is changed to play by play track. Therefore is must be changed to pause */
                                    rc = iPodCoreFuncPause(devID);
                                }
                            }
                        }
                    }
                }
                else
                {
                    if(iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_ONE)
                    {
                        /* Call prev track */
                        rc = iPodCoreFuncGotoTrackPosition(devID, 0);
                    }
                    else
                    {
                        prevTrackIndex = (U32)iPodCtrlCfg->playbackStatus.track.index - 1;
                        rc = iPodCoreFuncPlayTrack(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, (U64)prevTrackIndex);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_PAUSE)
                            {
                                /* Status is changed to play by play track. Therefore is must be changed to pause */
                                rc = iPodCoreFuncPause(devID);
                            }
                        }
                    }
                }
            }
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_PREVTRACK_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the current playing chapter to next chapter */
S32 iPodCoreiPodCtrlNextChapter(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW) ||
            (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF))
        {
            /* Current status is fastforward or rewind. These must call forward end command before call the previous track command */
            /* Current status must change to previous status */
            iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
            /* Previous status remove. Change to unknown status */
            iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        }
        
        /* Check that playback status is not Stop */
        if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_STOP)
        {
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->playbackStatus.status);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request the playing chapter change to next chapter */
                rc = iPodCoreFuncNextChapter(devID);
            }
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_NEXT_CHAPTER_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Changes the current playing chapter to previous chapter */
S32 iPodCoreiPodCtrlPrevChapter(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW) ||
            (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF))
        {
            /* Current status is fastforward or rewind. These must call forward end command before call the previous track command */
            /* Current status must change to previous status */
            iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
            /* Previous status remove. Change to unknown status */
            iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        }
        
        /* Check that playback status is not Stop */
        if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_STOP)
        {
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->playbackStatus.status);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* To avoid ATS error, anyway call the end FF/RW */
            rc = iPodCoreFuncEndForward(devID);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request the playing chapter change to previous chapter */
                rc = iPodCoreFuncPrevChapter(devID);
            }
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_PREV_CHAPTER_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the current track position to indicated position */
S32 iPodCoreiPodCtrlGotoTrackPosition(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;

        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Check the current playback status */
        if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW) ||
            (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF))
        {
            /* Current status is fastforward or rewind. These must call forward end command before call the previous track command */
            /* Current status must change to previous status */
            iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
            /* Previous status remove. Change to unknown status */
            iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        }
        
        
        /* Check that playback status is not STOP */
        if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_STOP)
        {
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->playbackStatus.status);
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Request to change the indicated positon from current position */
            rc = iPodCoreFuncGotoTrackPosition(devID, waitData->contents.gotoTrackPosition.times);
        }
        
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GOTO_TRACK_POSITION_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}


/* Change the current playing track to indicated track with type */
S32 iPodCoreiPodCtrlPlayTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Set Playing type */
        type = waitData->contents.playTrack.type;
        
        /* Type is UID */
        if(type == IPOD_PLAYER_TRACK_TYPE_UID)
        {
            /* Check whether the connected iPod is supported the UID functionality or not */
            if((iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_UID) != IPOD_PLAYER_FEATURE_MASK_UID)
            {
                /* Connected iPod does not support the UID functionality */
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type, iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_UID);
            }
        }
        else if((type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK) || (type == IPOD_PLAYER_TRACK_TYPE_DATABASE))
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, type, iPodCtrlCfg->playbackStatus.status);
            /* Check the current playback status */
            if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW) ||
                (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF))
            {
                /* Current status is fastforward or rewind. These must call forward end command before call the previous track command */
                /* Previous status remove. Change to unknown status */
                iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
            }
            
            if(type == IPOD_PLAYER_TRACK_TYPE_DATABASE)
            {
                if((iPodCtrlCfg->iPodInfo->catCountList[IPOD_PLAYER_DB_TYPE_TRACK] == 0) || 
                    (waitData->contents.playTrack.trackID > (iPodCtrlCfg->iPodInfo->catCountList[IPOD_PLAYER_DB_TYPE_TRACK] - 1)))
                {
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type, iPodCtrlCfg->iPodInfo->catCountList[IPOD_PLAYER_DB_TYPE_TRACK], waitData->contents.playTrack.trackID);
                }
            }
            else
            {
                rc = IPOD_PLAYER_OK;
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* Request to play the track */
                if((type == IPOD_PLAYER_TRACK_TYPE_DATABASE) && 
                   (iPodCtrlCfg->iPodInfo->topType == IPOD_PLAYER_DB_TYPE_AUDIOBOOK))
                {
                    rc = iPodCoreFuncSelectDBEntry(devID, iPodCtrlCfg->iPodInfo->topType, waitData->contents.playTrack.trackID);
                }
                else
                {
                    rc = iPodCoreFuncPlayTrack(devID, type, waitData->contents.playTrack.trackID);
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    iPodCtrlCfg->playbackStatus.track.index = IPOD_PLAYER_TRACK_INDEX_UNKNOWN;
                    /* Get the current playing track information if previous status is STOP because Apple device may not inform new track index */
                    if(iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_STOP)
                    {
                        rc = iPodCoreiPodCtrlRequestCurrentTrackInfo(iPodCtrlCfg, devID, IPOD_PLAYER_TRACK_INDEX_UNKNOWN);
                    }
                    /* Playback status changes to play even when current status is Stop */
                    iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_PLAY;
                }
            }
            
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_PLAY_TRACK_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}

static S32 iPodCoreiPodCtrlAudioRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, S32 cmdId, U32 size, U8 *data, U32 timeout)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    struct epoll_event epollEvent;
    IPOD_PLAYER_IPC_HANDLE_INFO inputInfo;
    IPOD_PLAYER_IPC_HANDLE_INFO outputInfo;
    U32 i = 0;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (data == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, data);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&paiResult, 0, sizeof(paiResult));
    memset(&epollEvent, 0, sizeof(epollEvent));
    memset(&inputInfo, 0, sizeof(inputInfo));
    memset(&outputInfo, 0, sizeof(outputInfo));
    
    /* Send the request */
    rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, data, size, 0, IPODCORE_SEND_TMOUT);
    if(rc == (S32)size)
    {
        /* Delete all handles from epoll */
        iPodCoreDelFDs(iPodCtrlCfg->waitHandle, iPodCtrlCfg->handleNum, iPodCtrlCfg->handle);
        /* Add handle for audio server */
        iPodCoreAddFDs(iPodCtrlCfg->waitHandle, 1, &iPodCtrlCfg->sckAudioServer);
        rc = 0;
        for(i = 0; (i < IPOD_PLAYER_WAIT_TIME_MAX_COUNT) && (rc == 0); i++)
        {
            /* Wait event for audio server */
            rc = epoll_wait(iPodCtrlCfg->waitHandle, &epollEvent, 1, timeout);
            if(rc > 0)
            {
                inputInfo.handle = epollEvent.data.fd;
                inputInfo.event = epollEvent.events;
                rc = iPodPlayerIPCIterate(iPodCtrlCfg->waitHandle, 1, &inputInfo, 1, &outputInfo);
            }
        }
        
        if(rc > 0)
        {
            /* Receive the result of request */
            rc = iPodPlayerIPCReceive(outputInfo.handle, (U8 *)&paiResult, sizeof(paiResult), 
                                      0, iPodCtrlCfg->waitHandle, timeout);
            if(rc == (S32)sizeof(paiResult))
            {
                /* Receiving data is result of cmdId. */
                if((paiResult.header.funcId == IPOD_FUNC_PAI_RESULT) && 
                   (paiResult.cmdId == (U32)cmdId) && (paiResult.result == IPOD_PLAYER_OK))
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_DLT_WARN("iPodPlayerIPCReceive :rc=%d, funcId=%d, cmdId=%u, result=%d", rc, paiResult.header.funcId, paiResult.cmdId, paiResult.result);
                    rc = IPOD_PLAYER_ERR_AUDIO;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, paiResult.header.funcId, paiResult.cmdId, paiResult.result);
                rc = IPOD_PLAYER_ERR_AUDIO;
            }
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Add all handles to epoll */
    iPodCoreAddFDs(iPodCtrlCfg->waitHandle, iPodCtrlCfg->handleNum, iPodCtrlCfg->handle);
    
    return rc;
}

static S32 iPodCoreiPodCtrlAudioInitCfg(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_PAI_CFG paiCfg;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (iPodCtrlCfg->threadInfo == NULL) || (iPodCtrlCfg->iPodInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&paiCfg, 0, sizeof(paiCfg));
    
    /* Set the configuration of audio server */
    paiCfg.header.funcId = IPOD_FUNC_PAI_SETCFG;
    paiCfg.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    /* Source is specified as "hw:x,x" */
    paiCfg.table.src_name_kind = 1;
    
    /* Set audio source name. maybe hw:x,x */
    strncpy((char *)paiCfg.table.src_name, (const char *)iPodCtrlCfg->threadInfo->nameInfo.audioInName, sizeof(paiCfg.table.src_name));
    paiCfg.table.src_name[sizeof(paiCfg.table.src_name) - 1] = '\0';
    
    /* Set audio sink name */
    iPodCoreGetCfs(IPOD_PLAYER_CFGNUM_AUDIO_OUTPUT_NAME, sizeof(paiCfg.table.sink_name), (U8 *)paiCfg.table.sink_name);
    IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, paiCfg.table.src_name);
    IPOD_LOG_INFO_WRITESTR(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, paiCfg.table.sink_name);
    
    rc = iPodCoreiPodCtrlAudioRequest(iPodCtrlCfg, IPOD_FUNC_PAI_SETCFG, sizeof(paiCfg), (U8 *)&paiCfg, IPODCORE_AUDIO_RECV_TMOUT);
    
    return rc;
}


static S32 iPodCoreiPodCtrlAudioDeinitCfg(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_TEMP paiClrCfg;
    
    /* Check Parameter */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&paiClrCfg, 0, sizeof(paiClrCfg));
    
    /* Set the configuration of audio server */
    paiClrCfg.header.funcId = IPOD_FUNC_PAI_CLRCFG;
    paiClrCfg.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    
    rc = iPodCoreiPodCtrlAudioRequest(iPodCtrlCfg, IPOD_FUNC_PAI_CLRCFG, sizeof(paiClrCfg), (U8 *)&paiClrCfg, IPODCORE_AUDIO_RECV_TMOUT);
    
    return rc;
}

/* Start audio streaming */
static S32 iPodCoreiPodCtrlAudioStartStreaming(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_PAI_START paiStart;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&paiStart, 0, sizeof(paiStart));
    
    /* Start the audio streaming */
    paiStart.header.funcId = IPOD_FUNC_PAI_START;
    /* Set the default sample rate */
    paiStart.rate = iPodCtrlCfg->iPodInfo->sampleRate;
    paiStart.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    
    rc = iPodCoreiPodCtrlAudioRequest(iPodCtrlCfg, IPOD_FUNC_PAI_START, sizeof(paiStart), (U8 *)&paiStart, IPODCORE_AUDIO_RECV_TMOUT);
    
    return rc;
}

static S32 iPodCoreiPodCtrlAudioStopStreaming(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_TEMP paiStop;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&paiStop, 0, sizeof(paiStop));
    
    paiStop.header.funcId = IPOD_FUNC_PAI_STOP;
    paiStop.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    
    rc = iPodCoreiPodCtrlAudioRequest(iPodCtrlCfg, IPOD_FUNC_PAI_STOP, sizeof(paiStop), (U8 *)&paiStop, IPODCORE_AUDIO_RECV_TMOUT);
    
    return rc;
}

/* Initialize the audio */
S32 iPodCoreiPodCtrlAudioInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
            {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the configuration */
    rc = iPodCoreiPodCtrlAudioInitCfg(iPodCtrlCfg);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void iPodCoreiPodCtrlAudioDeinit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }
    
    /* Deinitialize the configuration */
    iPodCoreiPodCtrlAudioDeinitCfg(iPodCtrlCfg);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, 0);
    
    return;
}
        
S32 iPodCoreiPodCtrlStartAudio(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Check Parameter */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Start audio streaming */
    rc = iPodCoreiPodCtrlAudioStartStreaming(iPodCtrlCfg);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Audio streaming is started */
        iPodCtrlCfg->startAudio = 1;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlStopAudio(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg);
    
    /* Check Parameter */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Audio streaming is started */
    if(iPodCtrlCfg->startAudio > 0)
    {
        /* Stop audio streaming */
        rc = iPodCoreiPodCtrlAudioStopStreaming(iPodCtrlCfg);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Audio streaming is stopped */
            iPodCtrlCfg->startAudio = 0;
        }
    }
    else
    {
        /* Audio streaming is already stopped */
        rc = IPOD_PLAYER_OK;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlSetAudioMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, 
                                 IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        if(((waitData->contents.setAudioMode.setting.mode == IPOD_PLAYER_SOUND_MODE_ON) || 
            (waitData->contents.setAudioMode.setting.mode == IPOD_PLAYER_SOUND_MODE_OFF)) && 
           ((waitData->contents.setAudioMode.setting.adjust == IPOD_PLAYER_STATE_ADJUST_ENABLE) ||
            (waitData->contents.setAudioMode.setting.adjust == IPOD_PLAYER_STATE_ADJUST_DISABLE)))
        {
            /* Requested mode and adjust is same as current setting */
            if((iPodCtrlCfg->audioSetting.mode == waitData->contents.setAudioMode.setting.mode) &&
               (iPodCtrlCfg->audioSetting.adjust == waitData->contents.setAudioMode.setting.adjust))
            {
                /* Nothing do */
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                /* Requested mode is different from current mode */
                if(iPodCtrlCfg->audioSetting.mode != waitData->contents.setAudioMode.setting.mode)
                {
                    /* Set the new mode */
                    iPodCtrlCfg->audioSetting.mode = waitData->contents.setAudioMode.setting.mode;
                    
                    /* New mode is audio on */
                    if(iPodCtrlCfg->audioSetting.mode == IPOD_PLAYER_SOUND_MODE_ON)
                    {
                        /* Start audio streaming */
                        rc = iPodCoreiPodCtrlStartAudio(iPodCtrlCfg);
                        if(rc != IPOD_PLAYER_OK)
                        {
                            iPodCtrlCfg->audioSetting.mode = IPOD_PLAYER_SOUND_MODE_OFF;
                        }
                    }
                    /* New mode is audio off */
                    else
                    {
                            /* Stop audio streaming */
                        rc = iPodCoreiPodCtrlStopAudio(iPodCtrlCfg);
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_OK;
                }
                
                /* Requested adjust is different fomr current adjust */
                if(rc == IPOD_PLAYER_OK)
                {
                    if(iPodCtrlCfg->audioSetting.adjust != waitData->contents.setAudioMode.setting.adjust)
                    {
                        /* Set the new adjust */
                        iPodCtrlCfg->audioSetting.adjust = waitData->contents.setAudioMode.setting.adjust;
                        
                        /* New adjust is enable */
                        if(iPodCtrlCfg->audioSetting.adjust == IPOD_PLAYER_STATE_ADJUST_ENABLE)
                        {
                            iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                        }
                        rc = IPOD_PLAYER_OK;
                        
                    }
                    else
                    {
                        /* Nothing do */
                        rc = IPOD_PLAYER_OK;
                    }
                }

            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.setAudioMode.setting.mode, waitData->contents.setAudioMode.setting.adjust);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_AUDIO_MODE_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}
/* Set the indicated mode */
S32 iPodCoreiPodCtrlSetMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U8 status = 0;
    U32 position = 0;
    U32 trackIndex = 0;
    U32 chapterTotal = 0;
    U32 i = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if(waitData->contents.setMode.mode <= IPOD_PLAYER_MODE_HMI_CONTROL)
        {
            /* Check the current mode */
            if(iPodCtrlCfg->iPodInfo->mode != waitData->contents.setMode.mode)
            {
                /* Current mode is different from indicated mode */
                /* Set the device event notification to iPod Ctrl. otherwise ATS error is occurred */
                rc = iPodCoreFuncSetDeviceEventNotification(devID, iPodCtrlCfg->iPodInfo->deviceEventMask);
                /* Request to change the mode to indicated mode */
                rc = iPodCoreFuncSetMode(devID, waitData->contents.setMode.mode);
                if(rc >= 0)
                {
                    /* Check the connected iPod is old */
                    if(rc == 1)
                    {
                        /* Connected iPod is old. It means iPod does not support iPodGetOptionsForLingo */
                        iPodCtrlCfg->oldiPod = 1;
                    }
                    /* Set the current mode */
                    iPodCtrlCfg->iPodInfo->mode = waitData->contents.setMode.mode;
                    rc = IPOD_PLAYER_OK;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        
        /* Set the mode change result */
        *size = sizeof(IPOD_CB_PARAM_SET_MODE_RESULT);
        waitData->contents.setModeResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_SET_MODE_RESULT;
        waitData->contents.setModeResult.result = rc;
        
        /* Send the result of iPodPlayerSetMode to Application */
        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, *size, 0, IPODCORE_TMOUT_FOREVER);
        /* Send queue successes */
        if(rc == (S32)*size)
        {
            /* Check the current mode */
            if(iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL)
            {
                /* Current mode is remote control mode (extend mode) */
                /* Get the device property with remote control mode in advance */
                /* Getting property is FormatID, Monochrome image maximum size and  Color image maximum size */
                iPodCoreFuncGetDeviceProperty(devID, IPOD_PLAYER_DEVICE_MASK_FORMAT | IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT | IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT, &iPodCtrlCfg->iPodInfo->property);
                
                if(iPodCtrlCfg->lastGetTime == 0)
                {
                    /* Get the current iPod status in advance */
                    rc = iPodCoreFuncIntGetStatus(devID, &status, &position);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, status);
                        /* Set the current iPod/playback status and current position */
                        iPodCtrlCfg->iPodInfo->status = (IPOD_PLAYER_PLAY_STATUS)status;
                        iPodCtrlCfg->playbackStatus.track.time = position;
                        iPodCtrlCfg->lastGetTime = iPodGetTime();
                        waitData->contents.paramTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_INTERNAL_GET_STATUS;
                        
                        /* Send the request that get the iPod status internally to iPodCore */
                        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, 
                                                sizeof(waitData->contents.paramTemp), 0, IPODCORE_TMOUT_FOREVER);
                        if(rc == sizeof(waitData->contents.paramTemp))
                        {
                            rc = IPOD_PLAYER_OK;
                        }
                        else
                        {
                            rc = IPOD_PLAYER_ERROR;
                        }
                    }
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Get the current track index */
                    rc = iPodCoreFuncIntGetCurrentTrackIndex(devID, &trackIndex);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Set the current track index */
                        iPodCtrlCfg->playbackStatus.track.index = trackIndex;
                    }
                    
                    /* Get the current chapter index */
                    rc = iPodCoreFuncIntGetCurrentChapterIndex(devID, &trackIndex, &chapterTotal);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Set the current chapter index */
                        iPodCtrlCfg->playbackStatus.chapter.index = trackIndex;
                        /* Get the current chapter status */
                        rc = iPodCoreFuncIntGetCurrentChapterStatus(devID, trackIndex, &position);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            /* Set the current chapter position */
                            iPodCtrlCfg->playbackStatus.chapter.time = position;
                        }
                    }
                    
                    if((rc == IPOD_PLAYER_OK) && (iPodCtrlCfg->iPodInfo->repeatStatus == (U8)IPOD_PLAYER_REPEAT_UNKNOWN))
                    {
                        rc = iPodCoreFuncGetRepeat(devID, &iPodCtrlCfg->iPodInfo->repeatStatus);
                    }
                    
                    if((rc == IPOD_PLAYER_OK) && (iPodCtrlCfg->iPodInfo->shuffleStatus == (U8)IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN))
                    {
                        rc = iPodCoreFuncGetShuffle(devID, &iPodCtrlCfg->iPodInfo->shuffleStatus);
                    }
                    
                    if((rc == IPOD_PLAYER_OK) && (iPodCtrlCfg->iPodInfo->speed == (U8)IPOD_PLAYER_PLAYING_SPEED_UNKNOWN))
                    {
                        rc = iPodCoreFuncGetSpeed(devID, &iPodCtrlCfg->iPodInfo->speed);
                    }
                    
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Get total count of all category */
                        /*
                        for(i = 0; i < (IPOD_PLAYER_DB_TYPE_ITUNESU + 1); i++)
                        {
                            rc = iPodCoreFuncGetDBCount(devID, (IPOD_PLAYER_DB_TYPE)i, &iPodCtrlCfg->iPodInfo->catCountList[i]);
                            if(rc != IPOD_PLAYER_OK)
                            {
                                iPodCtrlCfg->iPodInfo->catCountList[i] = 0;
                                rc = IPOD_PLAYER_OK;
                            }
                        }
                        */
                    }
                    
                    if((rc == IPOD_PLAYER_OK) && ((iPodCtrlCfg->iPodInfo->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_COVERART) == IPOD_PLAYER_TRACK_INFO_MASK_COVERART))
                    {
                        rc = iPodCoreFuncGetCoverartInfo(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, iPodCtrlCfg->playbackStatus.track.index, iPodCtrlCfg->iPodInfo->formatId, 
                                        &iPodCtrlCfg->iPodInfo->coverartCount, iPodCtrlCfg->iPodInfo->coverartTime, sizeof(iPodCtrlCfg->iPodInfo->coverartTime));
                        
                        if(rc == IPOD_PLAYER_OK)
                        {
                            for(i = 0; i < iPodCtrlCfg->iPodInfo->coverartCount - 1; i++)
                            {
                                if((iPodCtrlCfg->iPodInfo->coverartTime[i] < iPodCtrlCfg->playbackStatus.track.time) && (iPodCtrlCfg->iPodInfo->coverartTime[i + 1] >= iPodCtrlCfg->playbackStatus.track.time))
                                {
                                    break;
                                }
                            }
                            iPodCtrlCfg->iPodInfo->curCoverart = i;
                        }
                    }
                }
                
                /* Notify the first playback status to Application */
                rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
            }
            /* Requested mode is HMI Control mode */
            else if(iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL)
            {
                /* Sen the internal queue for HMI Control asynchronous function */
                waitData->contents.paramTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_INTERNAL_HMI_STATUS_SEND;
                /* Send the request */
                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, 
                                       sizeof(waitData->contents.paramTemp), 0, IPODCORE_TMOUT_FOREVER);
                if(rc == sizeof(waitData->contents.paramTemp))
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    rc = IPOD_PLAYER_ERROR;
                }
                
                /* Sen the internal queue for HMI Control asynchronous function */
                waitData->contents.paramTemp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND;
                /* Send the request */
                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, 
                                       sizeof(waitData->contents.paramTemp), 0, IPODCORE_TMOUT_FOREVER);
                if(rc == sizeof(waitData->contents.paramTemp))
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            
            /* Result has already replied to Application.  */
            rc = IPOD_PLAYER_ERR_NO_REPLY;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOT_CONNECT;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the repeat status to indicated status */
S32 iPodCoreiPodCtrlSetRepeat(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->repeatStatus, waitData->contents.setRepeat.status);
        /* Check the current repeat status */
        if(iPodCtrlCfg->iPodInfo->repeatStatus != waitData->contents.setRepeat.status)
        {
            /* Current repeat status is different from indicated status */
            rc = iPodCoreFuncSetRepeat(devID, waitData->contents.setRepeat.status);
            if(rc == IPOD_PLAYER_OK)
            {
               /* Set the current repeat status */
               iPodCtrlCfg->iPodInfo->repeatStatus = waitData->contents.setRepeat.status;
            }
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->iPodInfo->repeatStatus = IPOD_PLAYER_REPEAT_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_REPEAT_RESULT);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the shuffle status to indicated status */
S32 iPodCoreiPodCtrlSetShuffle(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->shuffleStatus, waitData->contents.setShuffle.status);
        /* Check the current shuffle status */
        if(iPodCtrlCfg->iPodInfo->shuffleStatus != waitData->contents.setShuffle.status)
        {
            /* Current shuffle status is different from indicated status */
            rc = iPodCoreFuncSetShuffle(devID, waitData->contents.setShuffle.status);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Set the current shuffle status */
                iPodCtrlCfg->iPodInfo->shuffleStatus = waitData->contents.setShuffle.status;
            }

        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->iPodInfo->shuffleStatus = IPOD_PLAYER_REPEAT_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_SHUFFLE_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}


/* Change the equalizer to indicated status */
S32 iPodCoreiPodCtrlSetEqualizer(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. This request can send without extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
    
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if((waitData->contents.setEqualizer.eq < iPodCtrlCfg->iPodInfo->maxEq) && (waitData->contents.setEqualizer.restore <= IPOD_PLAYER_RESTORE_ON))
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->equalizer, waitData->contents.setEqualizer.eq);
            /* Check the current equalizer setting */
            if(iPodCtrlCfg->iPodInfo->equalizer != waitData->contents.setEqualizer.eq)
            {
                /* Set indicated equalizer setting to iPod */
                rc = iPodCoreFuncSetEqualizer(devID, waitData->contents.setEqualizer.eq, waitData->contents.setEqualizer.restore);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Current equalizer setting is different from indicated equalizer*/
                    iPodCtrlCfg->iPodInfo->equalizer = waitData->contents.setEqualizer.eq;
                }

            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, waitData->contents.setEqualizer.eq, iPodCtrlCfg->iPodInfo->maxEq, waitData->contents.setEqualizer.restore);
        }
    }
    else
    {
        /* Status change to unknown */
        iPodCtrlCfg->iPodInfo->equalizer = IPODCORE_EQUALIZER_DEFAULT;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_EQUALIZER_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Set the Video delay */
S32 iPodCoreiPodCtrlSetVideoDelay(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Check whethre connected iPod supports video delay or not */
        if((iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_VIDEO_DELAY) == IPOD_PLAYER_FEATURE_MASK_VIDEO_DELAY)
        {
            /* Request to be delaied the video while indicated time */
            rc = iPodCoreFuncSetVideoDelay(devID, waitData->contents.setVideoDelay.delayTime);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOT_SUPPORT;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->property.supportedFeatureMask);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_VIDEO_DELAY_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the video setting to indicated setting */
S32 iPodCoreiPodCtrlSetVideoSetting(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        /* Request to change the video setting  */
        rc = iPodCoreFuncSetVideoSetting(devID, &waitData->contents.setVideoSetting.setting, waitData->contents.setVideoSetting.restore);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_VIDEO_SETTING_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

}


S32 iPodCoreiPodCtrlSetDisplayImage(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 imageSize = 0;
    U8 *imageData = NULL;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        memcpy((char *)&imageSize , (const char *)contents, sizeof(imageSize));
        imageData = &((U8 *)contents)[sizeof(imageSize)];
        rc = iPodCoreFuncSetDisplayImage(devID, imageSize, imageData);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Result is sent with short message */
    waitData->contents.paramTemp.header.longData = 0;
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_VIDEO_SETTING_RESULT);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Change the play speed to indicated speed */
S32 iPodCoreiPodCtrlSetPlaySpeed(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Check whether current speed different from indicated speed */
        if(iPodCtrlCfg->iPodInfo->speed != (U8)waitData->contents.setPlaySpeed.speed)
        {
            /* Request to change the play speed to indicated spped */
            rc = iPodCoreFuncSetPlaySpeed(devID, waitData->contents.setPlaySpeed.speed);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Speed is different. Set the indicated spped */
                iPodCtrlCfg->iPodInfo->speed = (U8)waitData->contents.setPlaySpeed.speed;
            }

        }
    }
    else
    {
        /* Speed set to unknown */
        iPodCtrlCfg->iPodInfo->speed = IPOD_PLAYER_PLAYING_SPEED_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_PLAY_SPEED_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Set the track info notification */
S32 iPodCoreiPodCtrlSetTrackInfoNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 i = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. This can request without extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if(waitData->contents.setTrackInfoNotification.trackInfoMask < (IPOD_PLAYER_TRACK_INFO_MASK_COVERART << 1))
        {
            /* Check whether current set mask is different from indicated mask */
            if(iPodCtrlCfg->iPodInfo->trackInfoMask != waitData->contents.setTrackInfoNotification.trackInfoMask)
            {
                /* Mask is different. Set the new mask */
                iPodCtrlCfg->iPodInfo->trackInfoMask = waitData->contents.setTrackInfoNotification.trackInfoMask;
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->trackInfoMask);
            }
            
            
            /* Check whether coverart bit is set in current mask */
            if((iPodCtrlCfg->iPodInfo->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_COVERART) == IPOD_PLAYER_TRACK_INFO_MASK_COVERART)
            {
                /* Set the format ID */
                iPodCtrlCfg->iPodInfo->formatId = waitData->contents.setTrackInfoNotification.formatId;
                if(iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL)
                {
                    rc = iPodCoreFuncGetCoverartInfo(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, iPodCtrlCfg->playbackStatus.track.index, iPodCtrlCfg->iPodInfo->formatId, 
                                    &iPodCtrlCfg->iPodInfo->coverartCount, iPodCtrlCfg->iPodInfo->coverartTime, sizeof(iPodCtrlCfg->iPodInfo->coverartTime));
                    if(rc == IPOD_PLAYER_OK)
                    {
                        for(i = 0; i < iPodCtrlCfg->iPodInfo->coverartCount - 1; i++)
                        {
                            if((iPodCtrlCfg->iPodInfo->coverartTime[i] < iPodCtrlCfg->playbackStatus.track.time) && (iPodCtrlCfg->iPodInfo->coverartTime[i + 1] >= iPodCtrlCfg->playbackStatus.track.time))
                            {
                                break;
                            }
                        }
                        iPodCtrlCfg->iPodInfo->curCoverart = i;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_OK;
                        iPodCtrlCfg->iPodInfo->coverartCount = 0;
                    }

                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.setTrackInfoNotification.trackInfoMask);
        }
    }
    else
    {
        /* Clear current setting */
        iPodCtrlCfg->iPodInfo->trackInfoMask = 0;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_TRACK_INFO_NOTIFICATION_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Set the event notification */
S32 iPodCoreiPodCtrlSetEventNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. This can request without entend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        if(waitData->contents.setDeviceEventNotification.deviceEventMask < IPOD_PLAYER_EVENT_MASK_ASSISTIVE << 1)
        {
            /* Check whether current set mask is different from indicated mask */
            if(iPodCtrlCfg->iPodInfo->deviceEventMask != waitData->contents.setDeviceEventNotification.deviceEventMask)
            {
                /* Request to set the event notification mask to iPod Ctrl */
                rc = iPodCoreFuncSetDeviceEventNotification(devID, waitData->contents.setDeviceEventNotification.deviceEventMask);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Mask is different. Set the new mask */
                    iPodCtrlCfg->iPodInfo->deviceEventMask = waitData->contents.setDeviceEventNotification.deviceEventMask;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Clear the current mask */
        iPodCtrlCfg->iPodInfo->deviceEventMask = 0;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SET_DEVICE_EVENT_NOTIFICATION_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the current video setting  */
S32 iPodCoreiPodCtrlGetVideoSetting(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if(waitData->contents.getVideoSetting.mask <= (IPOD_PLAYER_VIDEO_OUTPUT + IPOD_PLAYER_VIDEO_SCREEN + IPOD_PLAYER_VIDEO_SIGNAL + IPOD_PLAYER_VIDEO_LINEOUT + IPOD_PLAYER_VIDEO_CABLE +
            IPOD_PLAYER_VIDEO_CAPTION + IPOD_PLAYER_VIDEO_ASPECT + IPOD_PLAYER_VIDEO_SUBTITLE + IPOD_PLAYER_VIDEO_ALTAUDIO + IPOD_PLAYER_VIDEO_POWER + IPOD_PLAYER_VIDEO_VOICEOVER))
        {
            /* Request to get the current video setting from iPod Ctrl */
            rc = iPodCoreFuncGetVideoSetting(devID, iPodCtrlCfg, &waitData->contents.getVideoSettingResult.setting, waitData->contents.getVideoSetting.mask);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getVideoSetting.mask);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_VIDEO_SETTING_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the cover art information */
S32 iPodCoreiPodCtrlGetCoverartInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U64 trackIndex = 0;
    U16 formatID = 0;
    U32 timeSize = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        type = waitData->contents.getCoverartInfo.type;
        trackIndex = waitData->contents.getCoverartInfo.trackIndex;
        formatID = waitData->contents.getCoverartInfo.formatId;
        timeSize = sizeof(waitData->contents.getCoverartInfoResult.time);
        /* Request to get the coverart information with indicated index and formatID */
        rc = iPodCoreFuncGetCoverartInfo(devID, type, trackIndex, formatID, &waitData->contents.getCoverartInfoResult.timeCount, waitData->contents.getCoverartInfoResult.time, timeSize);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_COVERART_INFO_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the coverart data */
S32 iPodCoreiPodCtrlGetCoverart(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_PLAYER_PARAM_TEMP_LONG_DATA longData;
    IPOD_PLAYER_PARAM_INTERNAL_COVERART *intCover = NULL;
    U8 *sendData[IPODCORE_LONG_DATA_ARRAY] = {NULL};
    U32 sendSize[IPODCORE_LONG_DATA_ARRAY] = {0};
    U8 sendNum = IPODCORE_LONG_DATA_ARRAY;
    U32 asize = 0;
    IPOD_PLAYER_TRACK_TYPE trackType = IPOD_PLAYER_TRACK_TYPE_PLAYBACK;
    U64 trackIndex = 0;
    U16 formatId = 0;
    U32 coverTime = 0;
    U32 checkId = 0;
    

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (header == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&longData, 0, sizeof(longData));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            /* Request Coverart */
            case IPODCORE_QUEUE_STATUS_WAIT:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_WAIT);
                
                trackType = waitData->contents.getCoverart.type;
                trackIndex = waitData->contents.getCoverart.trackIndex;
                formatId = waitData->contents.getCoverart.formatId;
                coverTime = waitData->contents.getCoverart.time;
                /* Request to get the coverart data */
                rc = iPodCoreFuncGetCoverart(devID, trackType, trackIndex, formatId, coverTime, iPodCoreCBGetCoverart);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Set trackIndex and time */
                    waitData->contents.notifyCoverartData.trackIndex = trackIndex;
                    waitData->contents.notifyCoverartData.time = coverTime;
                    waitData->contents.notifyCoverartData.coverartHeader.formatId = formatId;
                    /* Request is not finished. Expect to receive the reply */
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                else
                {
                    /* Request is faile. status changes to finish */
                    *size = sizeof(IPOD_CB_PARAM_GET_COVERART_RESULT);
                }
                
                break;
                
            /* Received the coverart data */
            case IPODCORE_QUEUE_STATUS_RUNNING:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING);
                checkId = (U32)header->funcId;
                if(checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_COVERART))
                {
                    intCover = (IPOD_PLAYER_PARAM_INTERNAL_COVERART *)contents;
                    if(intCover->bufSize != 0)
                    {
                        longData.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_COVERART_DATA;
                        longData.header.devID = devID;
                        longData.header.appID = waitData->contents.paramTemp.header.appID;
                        longData.header.longData = 1;
                        memcpy(&longData.coverart, contents, sizeof(longData.coverart));
                        longData.coverart.trackIndex = waitData->contents.notifyCoverartData.trackIndex;
                        longData.coverart.time = waitData->contents.notifyCoverartData.time;
                        longData.coverart.coverartHeader.formatId = waitData->contents.notifyCoverartData.coverartHeader.formatId;
                        sendData[IPODCORE_POS0] = (U8 *)&longData;
                        sendSize[IPODCORE_POS0] = sizeof(longData);
                        sendData[IPODCORE_POS1] = &(((U8 *)contents)[sizeof(longData.coverart)]);
                        sendSize[IPODCORE_POS1] = longData.coverart.bufSize;
                        
                        rc = iPodPlayerIPCLongSend(iPodCtrlCfg->threadInfo->longCmdQueueClient, sendNum, sendData, sendSize, &asize, 0, IPODCORE_SEND_TMOUT);
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
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, intCover->bufSize);
                    }
                    
                    /* Set the result command size */
                    *size = sizeof(IPOD_CB_PARAM_GET_COVERART_RESULT);
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                break;
                
            case IPODCORE_QUEUE_STATUS_CANCEL:
                waitData->contents.paramResultTemp.header.funcId = IPOD_FUNC_GET_COVERART;
                
                rc = IPOD_PLAYER_ERR_API_CANCELED;
                *size = sizeof(waitData->contents.getCoverart);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                break;
                
            default:
                break;
        }

    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the current playback status */
S32 iPodCoreiPodCtrlGetPlaybackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the current playback stattus */
        waitData->contents.getPlaybackStatusResult.status = iPodCtrlCfg->playbackStatus;
    }
    else
    {
        /* Current status sets to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_PLAYBACK_STATUS_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void iPodCoreiPodCtrlCheckTrackInfo(U32 mask, const IPOD_PLAYER_TRACK_INFO *trackInfo, U32 *retMask, IPOD_PLAYER_TRACK_INFO *retInfo)
{
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, mask, trackInfo, retMask, retInfo);
    
    /* Parameter check */
    if((trackInfo == NULL) || (retMask == NULL) || (retInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, trackInfo, retMask, retInfo);
        return;
    }
    
    
    /* Podcast name bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME)
    {
        /* Copy the receiving podcast name to queued data */
        strncpy((char *)&retInfo->trackName[IPODCORE_POS0], (const char *)trackInfo->trackName, sizeof(retInfo->trackName));
        retInfo->trackName[sizeof(retInfo->trackName) - 1] = '\0';
        
        /* Add the podcast name bit to current receiving information mask */
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME;
    }
    
    /* Podcast name bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME)
    {
        /* Copy the receiving podcast name to queued data */
        strncpy((char *)&retInfo->albumName[IPODCORE_POS0], (const char *)trackInfo->albumName, sizeof(retInfo->albumName));
        retInfo->albumName[sizeof(retInfo->albumName) - 1] = '\0';
        
        /* Add the podcast name bit to current receiving information mask */
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME;
    }
    
    /* Podcast name bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME)
    {
        /* Copy the receiving podcast name to queued data */
        memcpy((char *)&retInfo->artistName[IPODCORE_POS0], (const char *)trackInfo->artistName, sizeof(retInfo->artistName));
        
        /* Add the podcast name bit to current receiving information mask */
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME;
    }
    
    
    /* Podcast name bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME) == IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME)
    {
        /* Copy the receiving podcast name to queued data */
        memcpy(&retInfo->podcastName[IPODCORE_POS0], trackInfo->podcastName, sizeof(retInfo->podcastName));
        
        /* Add the podcast name bit to current receiving information mask */
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME;
    }
    
    /* Description bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION) == IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION)
    {
        /* Copy the receiving description to queued data */
        memcpy(&retInfo->description[IPODCORE_POS0], trackInfo->description, sizeof(retInfo->description));
        
        /* Add the description bit to current receiving information mask */
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION;
    }
    
    /* Lyric bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_LYRIC) == IPOD_PLAYER_TRACK_INFO_MASK_LYRIC)
    {
        /* Copy the receiving lyric to queued data */
        memcpy(&retInfo->lyric[IPODCORE_POS0], trackInfo->lyric, sizeof(retInfo->lyric));
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_LYRIC;
    }
    
    /* Genre bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_GENRE) == IPOD_PLAYER_TRACK_INFO_MASK_GENRE)
    {
        /* Copy the receiving genre to queued data */
        memcpy(&retInfo->genre[IPODCORE_POS0], trackInfo->genre, sizeof(retInfo->genre));
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_GENRE;
    }
    
    /* Composer bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER) == IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER)
    {
        /* Copy the receiving composer to queued data */
        memcpy(&retInfo->composer[IPODCORE_POS0], trackInfo->composer, sizeof(retInfo->composer));
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER;
    }
    
    /* Release date bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE) == IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE)
    {
        /* Copy the receiving release date to queued data */
        memcpy(&retInfo->date, &trackInfo->date, sizeof(retInfo->date));
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE;
    }
    
    /* Capability bit is set in mask of recieved data */
    if(((mask & IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) == IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY))
    {
        if((retInfo->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY) == IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY)
        {
            /* Set the capability to queued data */
            retInfo->capa = trackInfo->capa;
            *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY;
        }
    }
    if(((mask & IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH) == IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH))
    {
        if((retInfo->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH) == IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH)
        {
            /* Set the track total length to queued data */
            retInfo->length = trackInfo->length;
            *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH;
        }
    }

    if(((mask & IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT) == IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT))
    {
        if((retInfo->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT) == IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT)
        {
            /* Set the chapter total count to queued data */
            retInfo->chapterCount = trackInfo->chapterCount;
            *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT;
        }
    }
    
    /* Unique ID bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_UID) == IPOD_PLAYER_TRACK_INFO_MASK_UID)
    {
        /* Copy the receiving uniqueu ID to queued data */
        retInfo->uID = trackInfo->uID;
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_UID;
    }
    
    /* Track kind bit is set in mask of recieved data */
    if((mask & IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND) == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND)
    {
        /* Copy the receiving track kind to queued data */
        retInfo->trackKind = trackInfo->trackKind;
        *retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND;
    }
    
    /* Unknown bit is set in mask of received data */
    else
    {
    }
}

static S32 iPodCoreiPodCtrlGetTrackInfoWaitStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U64 startID = 0;
    U32 count = 0;
    U32 trackInfoMask = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData);
    
    if((iPodCtrlCfg == NULL) || (waitData == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Save each parameter of GetTrackInfo */
    type = waitData->contents.getTrackInfo.type;
    startID = waitData->contents.getTrackInfo.startID;
    count = waitData->contents.getTrackInfo.count;
    trackInfoMask = waitData->contents.getTrackInfo.trackInfoMask;
    
    /* Set the requested type, startID, count, mask and current track ID */
    waitData->contents.getTrackInfoResult.type = type;
    waitData->contents.getTrackInfoResult.startID = startID;
    waitData->contents.getTrackInfoResult.count = count;
    waitData->contents.getTrackInfoResult.info.trackInfoMask = trackInfoMask;
    waitData->contents.getTrackInfoResult.trackID = startID;
    waitData->contents.getTrackInfoResult.mask = 0;
    waitData->contents.getTrackInfoResult.actualMask = trackInfoMask;
    
    /* Set the timer to be run the next status */
    waitData->nextRequest = 1;
    iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
    
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

static S32 iPodCoreiPodCtrlIntRequestNextTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_FUNC_ID funcId = IPOD_FUNC_INIT;
    U32 size = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData);
    
    if((iPodCtrlCfg == NULL) || (waitData == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the function ID with result mask */
    funcId = waitData->contents.getTrackInfoResult.header.funcId;
    waitData->contents.getTrackInfoResult.header.funcId = 
                                            (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | IPOD_FUNC_GET_TRACK_INFO);
    waitData->contents.getTrackInfoResult.result = IPOD_PLAYER_OK;
    size = sizeof(waitData->contents.getTrackInfoResult);
    
    /* Send the result to Application */
    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, 
                                size, 0, IPODCORE_TMOUT_FOREVER);
    if(rc == (S32)size)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Set the next track ID in queued data */
    waitData->contents.getTrackInfoResult.trackID++;
    /* Clear the current receiving data mask */
    waitData->contents.getTrackInfoResult.mask = 0;
    waitData->contents.getTrackInfoResult.sendMask = 0;
    waitData->contents.getTrackInfoResult.header.funcId = funcId;
    
    /* Set the timer to be executed the next request */
    waitData->nextRequest = 1;
    iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
    
    return rc;
}

static S32 iPodCoreiPodCtrlGetTrackInfoOld(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    U64 trackID = 0;
    U32 *trackInfoMask = NULL;
    U32 *recvMask = NULL;
    U32 supportedMask = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U32 curMask = 0;
    U32 i = 0;
    U32 mask = 0;
    U32 *waitTime = NULL;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData);
    
    if((iPodCtrlCfg == NULL) || (waitData == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    /* Save each paramters */
    type = waitData->contents.getTrackInfoResult.type;
    trackID = waitData->contents.getTrackInfoResult.trackID;
    trackInfoMask = &waitData->contents.getTrackInfoResult.info.trackInfoMask;
    recvMask = &waitData->contents.getTrackInfoResult.mask;
    supportedMask = iPodCtrlCfg->iPodInfo->property.supportedFeatureMask;
    
    /* Loop until bit maximum size. Int is 32 */
    for(i = 0; (i < IPOD_PLAYER_BIT_INT_NUM) && (rc != IPOD_PLAYER_OK); i++)
    {
        /* Check the bit which should get the information and check the bit which has already gotten the information */
        if(((*trackInfoMask & (U32)(1 << i))  == (U32)(1 << i)) && ((*recvMask & (U32)(1 << i)) != (U32)(1 << i)))
        {
            mask = 1 << i;
            
            switch(mask)
            {
                case IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME:
                    waitTime = &iPodCtrlCfg->iPodInfo->getTrackTime;
                    break;
                    
                case IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME:
                    waitTime = &iPodCtrlCfg->iPodInfo->getAlbumTime;
                    break;
                    
                case IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME:
                    waitTime = &iPodCtrlCfg->iPodInfo->getArtistTime;
                    break;
                default:
                    waitTime = &iPodCtrlCfg->iPodInfo->getInfoTime;
                    break;
            }
            
            if((mask & waitData->contents.getTrackInfoResult.sendMask) != mask)
            {
                /* Check the elapsed time from when previous one is called */
                rc = iPodCoreiPodCtrlTimedWait(iPodCtrlCfg->iPodInfo->getTrackInfoFd, waitTime);
                /* Elapsed time is over than 1 sec. */
                if(rc != IPOD_PLAYER_OK)
                {
                    /* Request to get the track information to iPod Ctrl with indicated type, trackID and mask */
                    rc = iPodCoreFuncGetTrackInfo(devID, type, trackID, &mask, &curMask, supportedMask, 
                                                    &waitData->contents.getTrackInfoResult.info, (void *)iPodCoreCBNotifyOldGetTrackInfo);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Add the current got information to received mask */
                        *recvMask |= curMask;
                        waitData->contents.getTrackInfoResult.sendMask = mask;
                    }
                    else
                    {
                        /* Requested information could not get */
                        if(mask == 0)
                        {
                            *trackInfoMask &= (~(1 << i));
                            /* Other information still remain */
                            if(*trackInfoMask != 0)
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                            else
                            {
                                /* requested information only error type */
                                break;
                            }
                        }
                        else
                        {
                            /* some error occurred */
                            break;
                        }
                    }
                    
                    /* Update the callTime */
                    *waitTime = iPodGetTime();
                }
                else
                {
                    /* Elapsed time is not over 1 sec. It was set the timer and wait the timer event */
                    break;
                }
            }
            else
            {
                rc = IPOD_PLAYER_OK;
                break;
            }
        }
        
        /* All information have already gotten */
        if(i == IPOD_PLAYER_BIT_INT_NUM -1)
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Check whether next request should send. */
    if((*trackInfoMask != *recvMask) && (rc == IPOD_PLAYER_OK) &&
        ((mask == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME) || (mask == IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME) || 
        (mask == IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME) || (mask == 0)))
    {
        /* iPod does not reply by callback. Next work is executed by timer */
        iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
        waitData->nextRequest = 1;
    }
    else
    {
        waitData->nextRequest = 0;
    }
    
    return rc;
}

static S32 iPodCoreiPodCtrlGetTrackInfoNew(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, 
                                            const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    U64 trackID = 0;
    U32 *trackInfoMask = NULL;
    U32 supportedMask = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U32 curMask = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, contents);
    
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    
    type = waitData->contents.getTrackInfoResult.type;
    trackID = waitData->contents.getTrackInfoResult.trackID;
    trackInfoMask = &waitData->contents.getTrackInfoResult.info.trackInfoMask;
    supportedMask = iPodCtrlCfg->iPodInfo->property.supportedFeatureMask;
    
    /* Input data(event) is not reply of iPodCtrl or Timer event */
    if((contents->getTrackInfo.resultFlag == 0) || (waitData->nextRequest != 0))
    {
        waitData->nextRequest = 0;
        rc = iPodCoreFuncGetTrackInfo(devID, type, trackID, trackInfoMask, 
                        &curMask, supportedMask, &waitData->contents.getTrackInfoResult.info, (IPOD_TRACK_INFORMATION_CB)iPodCoreCBNotifyGetTrackInfo);
        /* Bug of iPod nano 5G*/
        if(rc == IPOD_UNKNOWN_ID)
        {
            iPodCtrlCfg->iPodInfo->property.supportedFeatureMask &= (~IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT);
            waitData->nextRequest = 1;
            iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
            rc = IPOD_PLAYER_ERR_NO_REPLY;
        }
    }
    else
    {
        rc = IPOD_PLAYER_OK;
    }
    
    return rc;
}

/* Get the track information */
S32 iPodCoreiPodCtrlGetTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, 
                                    IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_PLAYER_PARAM_GET_TRACK_INFO trackInfo;
    IPOD_PLAYER_TRACK_INFO info;
    U32 checkId = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&trackInfo, 0, sizeof(trackInfo));
    memset(&info, 0, sizeof(info));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            /* IPODCORE_QUEUE_STATUS_WAIT: Request to get the track information to iPod Ctrl at first */
            case IPODCORE_QUEUE_STATUS_WAIT:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitData->contents.getTrackInfo.trackInfoMask);
                
                if((waitData->contents.getTrackInfo.trackInfoMask < (IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND << 1)) && 
                   (waitData->contents.getTrackInfo.type <= IPOD_PLAYER_TRACK_TYPE_UID) &&
                   (waitData->contents.getTrackInfo.count > 0))
                {
                    rc = iPodCoreiPodCtrlGetTrackInfoWaitStatus(iPodCtrlCfg, waitData);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitData->contents.getTrackInfo.trackInfoMask, waitData->contents.getTrackInfo.type, waitData->contents.getTrackInfo.count);
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                }
                break;
                
            /* IPODCORE_QUEUE_STATUS_RUNNING: Wait to receive the data from callback function */
            case IPODCORE_QUEUE_STATUS_RUNNING:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, contents->getTrackInfo.trackInfoMask);
                
                /* Current queue status is wait */
                /* If status is not wait, it means that status is wait. Current request is different request from current listed data */
                checkId = (U32)header->funcId;
                
                /* Input data is reply of iPodCtrl or timer event */
                if((checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_TRACK_INFO)) || (waitData->nextRequest != 0))
                {
                    /* Input data is reply of iPodCtrl */
                    if((contents->getTrackInfo.resultFlag == 1) && (waitData->nextRequest == 0))
                    {
                        /* Check the received info */
                        iPodCoreiPodCtrlCheckTrackInfo(contents->getTrackInfo.trackInfoMask, &contents->getTrackInfo.info,
                                                       &waitData->contents.getTrackInfoResult.mask, &waitData->contents.getTrackInfoResult.info);
                    }
                    
                    /* Check the feature or connected iPod*/
                    if((iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT) == 
                        IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT)
                    {
                        rc = iPodCoreiPodCtrlGetTrackInfoNew(iPodCtrlCfg, waitData, contents);
                    }
                    else
                    {
                        rc = iPodCoreiPodCtrlGetTrackInfoOld(iPodCtrlCfg, waitData);
                    }
                    
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Current receiving mask equals to application requested mask. 
                        It means that all of requested information could get from iPod Ctrl */
                        if(waitData->contents.getTrackInfoResult.info.trackInfoMask == waitData->contents.getTrackInfoResult.mask)
                        {
                            /* Current track is less  than total count. Next track information must get */
                            if(waitData->contents.getTrackInfoResult.trackID < 
                               (waitData->contents.getTrackInfoResult.startID + waitData->contents.getTrackInfoResult.count - 1))
                            {
                                rc = iPodCoreiPodCtrlIntRequestNextTrackInfo(iPodCtrlCfg, waitData);
                                if(rc == IPOD_PLAYER_OK)
                                {
                                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                                }
                            }
                            /* All of requested track could get the information */
                            else
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                        }
                        else
                        {
                            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        }
                    }
                    else if(rc == IPOD_PLAYER_ERR_NO_REPLY)
                    {
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                break;
            default: break;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    if(rc <= IPOD_PLAYER_OK)
    {
        IPOD_PLAYER_MESSAGE_DATA_CONTENTS *changeContents = NULL;
        changeContents = (IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)contents;
        changeContents->paramTemp.header.appID = waitData->contents.getTrackInfoResult.header.appID;
        /* Set the result command size */
        *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
    }
        
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


S32 iPodCoreiPodCtrlGetChapterInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 startChapter = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U64 startID = 0;
    U32 count = 0;
    U32 mask = 0;
    U32 curMask = 0;
    void *callback = NULL;
    U32 chapIndex = 0;
    IPOD_PLAYER_PARAM_GET_CHAPTER_INFO chapterInfo;
    IPOD_PLAYER_FUNC_ID funcId = IPOD_FUNC_INIT;
    U32 checkId = 0;

    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Initialize the structure */
    memset(&chapterInfo, 0, sizeof(chapterInfo));

    /* Todo: Refactoring */
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            /* Request to get the track information to iPod Ctrl at first */
            case IPODCORE_QUEUE_STATUS_WAIT:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitData->contents.getChapterInfo.chapterInfoMask);
                type = waitData->contents.getChapterInfo.type;
                startID = waitData->contents.getChapterInfo.trackID;
                startChapter = waitData->contents.getChapterInfo.startIndex;
                count = waitData->contents.getChapterInfo.count;
                mask = waitData->contents.getChapterInfo.chapterInfoMask;

                /* Current queue status is wait */
                /* If status is not wait, it means that status is running. Current request is different request from current listed data */
                if(count > 0)
                {
                    /* Get the current chapter index */
                    rc = iPodCoreFuncIntGetCurrentChapterIndex(devID, &chapIndex, &waitData->contents.getChapterInfoResult.chapCount);
                    if(rc == IPOD_OK)
                    {
                        /* Count from start index is more than chapter index */
                        if(waitData->contents.getChapterInfoResult.chapCount < startChapter + count)
                        {
                            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfoResult.chapCount, startChapter, count);
                        }
                    }
                }
                else
                {
                    /* Error is occurred. Queue status changes to Finish */
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, count);
                }
                
                if((rc == IPOD_PLAYER_OK) && ((IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH + IPOD_PLAYER_CHAPTER_INFO_MASK_NAME) >= waitData->contents.getChapterInfo.chapterInfoMask))
                {
                    /* Set the requesetd type, startID, count, mask and current track ID */
                    waitData->contents.getChapterInfoResult.type = type;
                    waitData->contents.getChapterInfoResult.trackIndex = startID;
                    waitData->contents.getChapterInfoResult.count = count;
                    waitData->contents.getChapterInfoResult.info.chapterInfoMask = mask;
                    waitData->contents.getChapterInfoResult.startID = startChapter;
                    
                    /* Check the type */
                    switch(type)
                    {
                        case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
                        case IPOD_PLAYER_TRACK_TYPE_DATABASE:
                        case IPOD_PLAYER_TRACK_TYPE_UID:
                            callback = (IPOD_TRACK_INFORMATION_CB)iPodCoreCBNotifyGetChapterInfo;
                            break;
                            
                        default:
                            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, type);
                            break;
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfo.chapterInfoMask);
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Request to get the track information to iPod Ctrl with indicated type, trackID and mask */
                    rc = iPodCoreFuncGetChapterInfo(devID, type, startID, startChapter, mask, &curMask, iPodCtrlCfg->iPodInfo->property.supportedFeatureMask, 
                                                        &waitData->contents.getChapterInfoResult.info, callback);
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Set the current set mask */
                    waitData->contents.getChapterInfoResult.mask = curMask;
                    /* Current queue status chagnes to Running */
                }
                else if(rc == IPOD_UNKNOWN_ID)
                {
                    iPodCtrlCfg->iPodInfo->property.supportedFeatureMask = iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & (~IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT);
                    waitData->contents.getChapterInfo.type = type;
                    waitData->contents.getChapterInfo.trackID = startID;
                    waitData->contents.getChapterInfo.startIndex = startChapter;
                    waitData->contents.getChapterInfo.count = count;
                    waitData->contents.getChapterInfo.chapterInfoMask = mask;
                    *size = sizeof(waitData->contents.getChapterInfo);
                    /* Send the request again to own */
                    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, *size, 0, IPODCORE_TMOUT_FOREVER);
                    if(rc == (S32)*size)
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfoResult.mask);
                        rc = IPOD_PLAYER_ERR_NO_REPLY;
                    }
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Current receiving mask equals to application requested mask. It means that all of requested information could get from iPod Ctrl */
                    /* Useally, old iPod can get the chapter information */
                    if(waitData->contents.getChapterInfoResult.mask == waitData->contents.getChapterInfoResult.info.chapterInfoMask)
                    {
                        /* Current track is less than total count. Next track information must get */
                        if(waitData->contents.getChapterInfoResult.chapterIndex < (waitData->contents.getChapterInfoResult.startID + waitData->contents.getChapterInfoResult.count - 1))
                        {
                            chapterInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_CHAPTER_INFO);
                            chapterInfo.header.devID = waitData->contents.getChapterInfoResult.header.devID;
                            chapterInfo.header.appID = waitData->contents.getChapterInfoResult.header.appID;
                            chapterInfo.chapterInfoMask = waitData->contents.getChapterInfoResult.mask;
                            chapterInfo.info.chapterInfoMask = waitData->contents.getChapterInfoResult.mask;
                            chapterInfo.info.length = waitData->contents.getChapterInfoResult.info.length;
                            chapterInfo.count = waitData->contents.getChapterInfoResult.count;
                            chapterInfo.chapterIndex = startID;
                            
                            strncpy((char *)&chapterInfo.info.chapterName[IPODCORE_POS0], 
                                    (const char *)waitData->contents.getChapterInfoResult.info.chapterName, sizeof(chapterInfo.info.chapterName));
                            chapterInfo.info.chapterName[sizeof(chapterInfo.info.chapterName) - 1] = '\0';
                            *size = sizeof(chapterInfo);
                            
                            rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&chapterInfo, *size, 0, IPODCORE_TMOUT_FOREVER);
                            if(rc == (S32)*size)
                            {
                                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                            }
                            
                        }
                        /* All of requested track could get the information */
                        else
                        {
                            /* Set the result command size */
                            *size = sizeof(waitData->contents.getChapterInfoResult);
                            rc = IPOD_PLAYER_OK;
                        }
                    }
                    /* Current receiving mask does not equal to application requested mask. It means that remaining request is received by callback */
                    else
                    {
                        waitData->contents.getChapterInfoResult.indexList = calloc(waitData->contents.getChapterInfoResult.chapCount, sizeof(S32));
                        if(waitData->contents.getChapterInfoResult.indexList != NULL)
                        {
                            waitData->contents.getChapterInfoResult.lengthList = calloc(waitData->contents.getChapterInfoResult.chapCount, sizeof(S32));
                            if(waitData->contents.getChapterInfoResult.lengthList != NULL)
                            {
                                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                            }
                            else
                            {
                                free(waitData->contents.getChapterInfoResult.indexList);
                                waitData->contents.getChapterInfoResult.indexList = NULL;
                                /* Set the result command size */
                                *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                                rc = IPOD_PLAYER_ERR_NOMEM;
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                            }
                        }
                        else
                        {
                            /* Set the result command size */
                            *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                            rc = IPOD_PLAYER_ERR_NOMEM;
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                        }
                    }
                }
                break;
                
            /* Wait to receive the data from callback function */
            case IPODCORE_QUEUE_STATUS_RUNNING:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, contents->getChapterInfo.chapterInfoMask);
                checkId = (U32)header->funcId;
                if(checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_CHAPTER_INFO))
                {
                    if((waitData->contents.getChapterInfoResult.indexList != NULL) && (waitData->contents.getChapterInfoResult.lengthList != NULL))
                    {
                        /* Podcast name bit is set in mask of recieved data */
                        if((contents->getChapterInfo.chapterInfoMask & IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH) == IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH)
                        {
                            if(contents->getChapterInfo.chapterIndex < waitData->contents.getChapterInfoResult.chapCount)
                            {
                                /* Copy the receiving podcast name to queued data */
                                waitData->contents.getChapterInfoResult.lengthList[contents->getChapterInfo.chapterIndex] = (U32)contents->getChapterInfo.info.length;
                                
                                /* Add the podcast name bit to current receiving information mask */
                                waitData->contents.getChapterInfoResult.mask |= IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH;
                            }
                            else
                            {
                                /* Set the result command size */
                                *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                                rc = IPOD_PLAYER_ERR_NOMEM;
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, contents->getChapterInfo.chapterIndex, waitData->contents.getChapterInfoResult.chapCount);
                            }
                        }
                        
                        if((waitData->contents.getChapterInfoResult.info.chapterInfoMask == IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH) || 
                           (waitData->contents.getChapterInfoResult.info.chapterInfoMask == IPOD_PLAYER_CHAPTER_INFO_MASK_NAME) || 
                        ((waitData->contents.getChapterInfoResult.info.chapterInfoMask == (IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH | IPOD_PLAYER_CHAPTER_INFO_MASK_NAME)) && 
                                ((contents->getChapterInfo.chapterInfoMask & IPOD_PLAYER_CHAPTER_INFO_MASK_NAME) == IPOD_PLAYER_CHAPTER_INFO_MASK_NAME)))
                        {
                            /* Set the size of result of get track information */
                            *size = sizeof(waitData->contents.getChapterInfoResult);
                            /* Set the function ID with result mask */
                            waitData->contents.getChapterInfoResult.result = rc;
                            
                            if((waitData->contents.getChapterInfoResult.indexList != NULL) && (waitData->contents.getChapterInfoResult.lengthList != NULL))
                            {
                                waitData->contents.getChapterInfoResult.info.length = waitData->contents.getChapterInfoResult.lengthList[contents->getChapterInfo.chapterIndex];
                                strncpy((char *)&waitData->contents.getChapterInfoResult.info.chapterName[IPODCORE_POS0], 
                                        (const char *)contents->getChapterInfo.info.chapterName, sizeof(waitData->contents.getChapterInfoResult.info.chapterName));
                                waitData->contents.getChapterInfoResult.info.chapterName[sizeof(waitData->contents.getChapterInfoResult.info.chapterName) - 1] = '\0';
                            }
                            
                            if(contents->getChapterInfo.chapterIndex < (waitData->contents.getChapterInfoResult.startID + waitData->contents.getChapterInfoResult.count) - 1)
                            {
                                if(contents->getChapterInfo.chapterIndex >= waitData->contents.getChapterInfoResult.startID)
                                {
                                    funcId = waitData->contents.getChapterInfoResult.header.funcId;
                                    waitData->contents.getChapterInfoResult.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | IPOD_FUNC_GET_CHAPTER_INFO);
                                    /* Send the result to Application */
                                    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, *size, 0, IPODCORE_TMOUT_FOREVER);
                                    if(rc == (S32)*size)
                                    {
                                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfoResult.mask);
                                        rc = IPOD_PLAYER_OK;
                                        waitData->contents.getChapterInfoResult.header.funcId = funcId;
                                    }
                                    waitData->contents.getChapterInfoResult.header.funcId = funcId;
                                }
                            }
                            else
                            {
                                /* Set the result command size */
                                *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                                rc = IPOD_PLAYER_OK;
                            }
                        }
                    }
                    else
                    {
                        /* Podcast name bit is set in mask of recieved data */
                        if((contents->getChapterInfo.chapterInfoMask & IPOD_PLAYER_CHAPTER_INFO_MASK_NAME) == IPOD_PLAYER_CHAPTER_INFO_MASK_NAME)
                        {
                            if(contents->getChapterInfo.chapterIndex < waitData->contents.getChapterInfoResult.chapCount)
                            {
                                /* Copy the receiving podcast name to queued data */
                                strncpy((char *)waitData->contents.getChapterInfoResult.info.chapterName, 
                                        (const char *)contents->getChapterInfo.info.chapterName, sizeof(waitData->contents.getChapterInfoResult.info.chapterName));
                                waitData->contents.getChapterInfoResult.info.chapterName[sizeof(waitData->contents.getChapterInfoResult.info.chapterName) - 1] = '\0';
                                
                                /* Add the podcast name bit to current receiving information mask */
                                waitData->contents.getChapterInfoResult.mask |= IPOD_PLAYER_CHAPTER_INFO_MASK_NAME;
                            }
                            else
                            {
                                /* Set the result command size */
                                *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                                rc = IPOD_PLAYER_ERR_NOMEM;
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, contents->getChapterInfo.chapterIndex, waitData->contents.getChapterInfoResult.chapCount);
                            }
                        }
                        
                        /* Podcast name bit is set in mask of recieved data */
                        if((contents->getChapterInfo.chapterInfoMask & IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH) == IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH)
                        {
                            if(contents->getChapterInfo.chapterIndex < waitData->contents.getChapterInfoResult.chapCount)
                            {
                                /* Copy the receiving podcast name to queued data */
                                waitData->contents.getChapterInfoResult.info.length = contents->getChapterInfo.info.length;
                                
                                /* Add the podcast name bit to current receiving information mask */
                                waitData->contents.getChapterInfoResult.mask |= IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH;
                            }
                            else
                            {
                                /* Set the result command size */
                                *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                                rc = IPOD_PLAYER_ERR_NOMEM;
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, contents->getChapterInfo.chapterIndex, waitData->contents.getChapterInfoResult.chapCount);
                            }
                        }
                        
                        /* Current receiving mask equals to application requested mask. It means that all of requested information could get from iPod Ctrl */
                        if((rc == IPOD_PLAYER_OK) && (waitData->contents.getChapterInfoResult.mask == waitData->contents.getChapterInfoResult.info.chapterInfoMask))
                        {
                            /* Current track is less  than total count. Next track information must get */
                            if(waitData->contents.getChapterInfoResult.chapterIndex < (waitData->contents.getChapterInfoResult.startID + waitData->contents.getChapterInfoResult.count - 1))
                            {
                                /* Set the size of result of get track information */
                                *size = sizeof(waitData->contents.getChapterInfoResult);
                                /* Set the function ID with result mask */
                                funcId = waitData->contents.getChapterInfoResult.header.funcId;
                                waitData->contents.getChapterInfoResult.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | IPOD_FUNC_GET_CHAPTER_INFO);
                                waitData->contents.getChapterInfoResult.result = rc;
                                
                                if((waitData->contents.getChapterInfoResult.indexList != NULL) && (waitData->contents.getChapterInfoResult.lengthList != NULL))
                                {
                                    waitData->contents.getChapterInfoResult.info.length = waitData->contents.getChapterInfoResult.lengthList[waitData->contents.getChapterInfoResult.chapterIndex];
                                }
                                
                                /* Send the result to Application */
                                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, *size, 0, IPODCORE_TMOUT_FOREVER);
                                if(rc == (S32)*size)
                                {
                                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfoResult.mask);
                                    rc = IPOD_PLAYER_OK;
                                    waitData->contents.getChapterInfoResult.header.funcId = funcId;
                                }
                                
                                /* Set the next track ID in queued data */
                                waitData->contents.getChapterInfoResult.chapterIndex++;
                                /* Clear the current receiving data mask */
                                waitData->contents.getChapterInfoResult.mask = 0;
                                
                                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitData->contents.getChapterInfoResult.type);
                                switch(waitData->contents.getChapterInfoResult.type)
                                {
                                    case IPOD_PLAYER_TRACK_TYPE_PLAYBACK:
                                        //callback = (void *)iPodCoreCBNotifyPlayingTrackInfo;
                                        break;
                                        
                                    case IPOD_PLAYER_TRACK_TYPE_DATABASE:
                                        //callback = (void *)iPodCoreCBNotifyGetTrackInfo;
                                        break;
                                        
                                    case IPOD_PLAYER_TRACK_TYPE_UID:
                            //                rc = iPodCoreFuncGetTrackInfoByUID(devID, iPodCtrlCfg, startID + i, mask);
                                        rc = IPOD_PLAYER_ERROR;
                                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfoResult.type);
                                        break;
                                        
                                    default:
                                        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getChapterInfoResult.type);
                                        break;
                                }
                                rc = iPodCoreFuncGetChapterInfo(devID, waitData->contents.getChapterInfoResult.type, waitData->contents.getChapterInfoResult.startID, waitData->contents.getChapterInfoResult.chapterIndex, waitData->contents.getChapterInfoResult.info.chapterInfoMask, &waitData->contents.getChapterInfoResult.mask, 
                                                                iPodCtrlCfg->iPodInfo->property.supportedFeatureMask, &waitData->contents.getChapterInfoResult.info, callback);
                                if(rc == IPOD_PLAYER_OK)
                                {
                                    chapterInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_CHAPTER_INFO);
                                    chapterInfo.header.devID = waitData->contents.getChapterInfoResult.header.devID;
                                    chapterInfo.header.appID = waitData->contents.getChapterInfoResult.header.appID;
                                    chapterInfo.chapterInfoMask = waitData->contents.getChapterInfoResult.mask;
                                    chapterInfo.info.chapterInfoMask = waitData->contents.getChapterInfoResult.mask;
                                    chapterInfo.count = waitData->contents.getChapterInfoResult.count;
                                    chapterInfo.info.length = waitData->contents.getChapterInfoResult.info.length;
                                    
                                    strncpy((char *)chapterInfo.info.chapterName, 
                                            (const char *)waitData->contents.getChapterInfoResult.info.chapterName, sizeof(chapterInfo.info.chapterName));
                                    chapterInfo.info.chapterName[sizeof(chapterInfo.info.chapterName) - 1] = '\0';
                                    
                                    funcId = waitData->contents.getChapterInfoResult.header.funcId;
                                    *size = sizeof(chapterInfo);
                                    
                                    /* Current track is less than total count. Next track information must get */
                                    if(waitData->contents.getChapterInfoResult.chapterIndex < (waitData->contents.getChapterInfoResult.startID + waitData->contents.getChapterInfoResult.count) - 1)
                                    {
                                        waitData->contents.getChapterInfoResult.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_CHAPTER_INFO);
                                        *size = sizeof(waitData->contents.getChapterInfoResult);
                                            
                                        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&chapterInfo, *size, 0, IPODCORE_TMOUT_FOREVER);
                                        if(rc == (S32)*size)
                                        {
                                            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                                            rc = IPOD_PLAYER_OK;
                                        }
                                        
                                        /* Queueing function ID sets */
                                        waitData->contents.getChapterInfoResult.header.funcId = funcId;
                                    }
                                    /* All of requested track could get the information */
                                    else
                                    {
                                        /* Set the result command size */
                                        *size = sizeof(waitData->contents.getChapterInfoResult);
                                    }
                                    
                                    /* Queueing function ID sets */
                                    waitData->contents.getChapterInfoResult.header.funcId = funcId;
                                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                                }
                                else
                                {
                                    /* Set the result command size */
                                    *size = sizeof(waitData->contents.getChapterInfoResult);
                                }
                            }
                            
                            /* All of requested track could get the information */
                            else
                            {
                                /* Set the result command size */
                                *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
                            }
                        }
                    }
                }
                break;
            default: break;
        }
        
    }
    else
    {
        /* Set the result command size */
        *size = sizeof(IPOD_CB_PARAM_GET_TRACK_INFO_RESULT);
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    if(rc <= IPOD_PLAYER_OK)
    {
        IPOD_PLAYER_MESSAGE_DATA_CONTENTS *changeContents = NULL;
        changeContents = (IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)contents;
        changeContents->paramTemp.header.appID = waitData->contents.getChapterInfoResult.header.appID;
        
        if(waitData->contents.getChapterInfoResult.indexList != NULL)
        {
            free(waitData->contents.getChapterInfoResult.indexList);
            waitData->contents.getChapterInfoResult.indexList = NULL;
        }
        
        if(waitData->contents.getChapterInfoResult.lengthList != NULL)
        {
            free(waitData->contents.getChapterInfoResult.lengthList);
            waitData->contents.getChapterInfoResult.lengthList = NULL;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the current mode of iPod */
S32 iPodCoreiPodCtrlGetMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. This command can use without extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* First time to get/set the mode */
        if(iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_UNKNOWN)
        {
            /* Request to get the current mode of iPod */
            rc = iPodCoreFuncGetMode(devID, &iPodCtrlCfg->iPodInfo->mode);
        }
        
        /* Set the current mode to result */
        waitData->contents.getModeResult.mode = (IPOD_PLAYER_MODE)iPodCtrlCfg->iPodInfo->mode;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_MODE_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the current repeat status */
S32 iPodCoreiPodCtrlGetRepeat(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* First time to get/set the repeat status */
        if(iPodCtrlCfg->iPodInfo->repeatStatus == IPOD_PLAYER_REPEAT_UNKNOWN)
        {
            /* Request to get the current repeat status to iPod Ctrl */
            rc = iPodCoreFuncGetRepeat(devID, &iPodCtrlCfg->iPodInfo->repeatStatus);
        }
        
        /* Set the current repeat status to result */
        waitData->contents.getRepeatResult.status = (IPOD_PLAYER_REPEAT_STATUS)iPodCtrlCfg->iPodInfo->repeatStatus;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_REPEAT_STATUS_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}



/* Get the current shuffle status */
S32 iPodCoreiPodCtrlGetShuffle(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* First time to get/set the repeat status */
        if(iPodCtrlCfg->iPodInfo->shuffleStatus == IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN)
        {
            /* Request to get the shuffle status to iPod Ctrl */
            rc = iPodCoreFuncGetShuffle(devID, &iPodCtrlCfg->iPodInfo->shuffleStatus);
        }
        
        /* Set the current shuffle status to result */
        waitData->contents.getShuffleResult.status = (IPOD_PLAYER_SHUFFLE_STATUS)iPodCtrlCfg->iPodInfo->shuffleStatus;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_SHUFFLE_STATUS_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the current playing speed */
S32 iPodCoreiPodCtrlGetPlaySpeed(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* First time to get/set the playing speed */
        if(iPodCtrlCfg->iPodInfo->speed == IPOD_PLAYER_PLAYING_SPEED_UNKNOWN)
        {
            /* Request to get the current speed to iPod Ctrl */
            rc = iPodCoreFuncGetSpeed(devID, &iPodCtrlCfg->iPodInfo->speed);
        }
        
        /* Set the current speed to result */
        waitData->contents.getPlaySpeedResult.speed = (IPOD_PLAYER_PLAYING_SPEED)iPodCtrlCfg->iPodInfo->speed;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_PLAY_SPEED_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the total track count of indicated type */
S32 iPodCoreiPodCtrlGetTrackTotalCount(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if(waitData->contents.getTrackTotalCount.type == IPOD_PLAYER_TRACK_TYPE_DATABASE)
        {
            /* Check the time  which elapsed from last call for ATS. If elapsed time does not exceed 1 sec, it must wait for 1 sec. */
            waitData->contents.getTrackTotalCountResult.count = iPodCtrlCfg->iPodInfo->catCountList[IPOD_PLAYER_DB_TYPE_TRACK];
        }
        else
        {
            /* Request to get the track total count with indicated type to iPod Ctrl */
            rc = iPodCoreFuncGetTrackTotalCount(devID, waitData->contents.getTrackTotalCount.type, &waitData->contents.getTrackTotalCountResult.count);
            
            waitData->contents.getTrackTotalCountResult.type = waitData->contents.getTrackTotalCount.type;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_TRACK_TOTAL_COUNT_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Get current equalizer setting */
S32 iPodCoreiPodCtrlGetEqaulizer(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. This command can use without extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;

        /* First time to get/set the equalizer */
        if(iPodCtrlCfg->iPodInfo->equalizer == IPODCORE_EQUALIZER_DEFAULT)
        {
            /* Request to get the current equalizer setting to iPod Ctrl */
            rc = iPodCoreFuncGetEqualizer(devID, &iPodCtrlCfg->iPodInfo->equalizer);
        }
        
        /* Set the current equalizer setting to result */
        waitData->contents.getEqualizerResult.value = iPodCtrlCfg->iPodInfo->equalizer;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_EQUALIZER_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Get the indicated equalizer name */
S32 iPodCoreiPodCtrlGetEqualizerName(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. This command can use without extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;

        /* Request to get the indicated equalizer name to iPod Ctrl */
        rc = iPodCoreFuncGetEqualizerName(devID, waitData->contents.getEqualizerName.eq, waitData->contents.getEqualizerNameResult.name);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_EQUALIZER_NAME_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Get the property of iPod */
S32 iPodCoreiPodCtrlGetDeviceProperty(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 mask = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the mask to local variable */
    mask = waitData->contents.getDeviceProperty.devicePropertyMask;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, mask);
    /* Device name bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_NAME) == IPOD_PLAYER_DEVICE_MASK_NAME)
    {
        /* Copy the device name from internal property structure */
        memcpy(&waitData->contents.getDevicePropertyResult.property.name[IPODCORE_POS0], &iPodCtrlCfg->iPodInfo->property.name[IPODCORE_POS0], sizeof(waitData->contents.getDevicePropertyResult.property.name));
    }
    
    /* Device software version bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION) == IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION)
    {
        /* Copy the device software version from internal property structure */
        memcpy(&waitData->contents.getDevicePropertyResult.property.softVer, &iPodCtrlCfg->iPodInfo->property.softVer, sizeof(waitData->contents.getDevicePropertyResult.property.softVer));
    }
    
    /* Device Serial number bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER) == IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER)
    {
        /* Copy the serial number from internal property structure */
        memcpy(&waitData->contents.getDevicePropertyResult.property.serial[IPODCORE_POS0], &iPodCtrlCfg->iPodInfo->property.serial[IPODCORE_POS0], sizeof(waitData->contents.getDevicePropertyResult.property.serial));
    }
    
    /* Device max payload size bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE) == IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE)
    {
        /* Copy the max playload size from internal property structure */
        waitData->contents.getDevicePropertyResult.property.maxPayload = iPodCtrlCfg->iPodInfo->property.maxPayload;
    }
    
    /* Device supported feature bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE) == IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE)
    {
        /* Copy the supported feature from internal property structure */
        waitData->contents.getDevicePropertyResult.property.supportedFeatureMask = iPodCtrlCfg->iPodInfo->property.supportedFeatureMask;
    }
    
    /* Device notification mask bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_EVENT) == IPOD_PLAYER_DEVICE_MASK_EVENT)
    {
        /* Copy the current notification from internal property structure */
        waitData->contents.getDevicePropertyResult.property.curEvent =iPodCtrlCfg->iPodInfo->property.curEvent;
        /* Copy the device supported notification from internal property structure */
        waitData->contents.getDevicePropertyResult.property.supportEvent =iPodCtrlCfg->iPodInfo->property.supportEvent;
    }
    
    /* Device file space bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_FILE_SPACE) == IPOD_PLAYER_DEVICE_MASK_FILE_SPACE)
    {
        /* Copy the file space from internal property structure */
        waitData->contents.getDevicePropertyResult.property.fileSpace = iPodCtrlCfg->iPodInfo->property.fileSpace;
    }
    
    /* Device coverart format bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_FORMAT) == IPOD_PLAYER_DEVICE_MASK_FORMAT)
    {
        /* Copy the count of coverart format from internal property structure */
        waitData->contents.getDevicePropertyResult.property.formatCount= iPodCtrlCfg->iPodInfo->property.formatCount;
        /* Copy the coverart format from internal property structure */
        memcpy(waitData->contents.getDevicePropertyResult.property.format, iPodCtrlCfg->iPodInfo->property.format, sizeof(waitData->contents.getDevicePropertyResult.property.format));
    }
    
    /* Device monochrome image max size bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT) == IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT)
    {
        /* Copy the monochrome image max size from internal property structure */
        memcpy(&waitData->contents.getDevicePropertyResult.property.mono, &iPodCtrlCfg->iPodInfo->property.mono, sizeof(waitData->contents.getDevicePropertyResult.property.mono));
    }
    
    /* Device the color image max size bit was set */
    if((mask & IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT) == IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT)
    {
        /* Copy the color image size from internal property structure */
        memcpy(&waitData->contents.getDevicePropertyResult.property.color, &iPodCtrlCfg->iPodInfo->property.color, sizeof(waitData->contents.getDevicePropertyResult.property.color));
    }
    
    
    waitData->contents.getDevicePropertyResult.property.devicePropertyMask = mask;
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_DEVICE_PROPERTY_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Get the current status of iPod */
S32 iPodCoreiPodCtrlGetDeviceStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 mask = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the mask to local variable */
    mask = waitData->contents.getDeviceStatus.deviceStatusMask;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, mask);
    
    /* Mode bit was set */
    if((mask & IPOD_PLAYER_IPOD_STATUS_INFO_MASK_MODE) == IPOD_PLAYER_IPOD_STATUS_INFO_MASK_MODE)
    {
        /* Current mode is unknown. It means that Application has never called set/get mode. */
        if(iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_UNKNOWN)
        {
            /* Get the current mode of iPod */
            iPodCoreFuncGetMode(iPodCtrlCfg->threadInfo->iPodDevID, &iPodCtrlCfg->iPodInfo->mode);
        }
        /* Set the current mode */
        waitData->contents.getDeviceStatusResult.info.curMode = (IPOD_PLAYER_MODE)iPodCtrlCfg->iPodInfo->mode;
    }
    
    /* Power status bit was set */
    if((mask & IPOD_PLAYER_IPOD_STATUS_INFO_MASK_STATUS) == IPOD_PLAYER_IPOD_STATUS_INFO_MASK_STATUS)
    {
        /* Copy the power status from internal status */
        waitData->contents.getDeviceStatusResult.info.powerStatus = iPodCtrlCfg->deviceConnection.powerStatus;
    }
    
    /* iPod event notifiation bit was set */
    if((mask & IPOD_PLAYER_IPOD_STATUS_INFO_MASK_NOTIFY_EVENT) == IPOD_PLAYER_IPOD_STATUS_INFO_MASK_NOTIFY_EVENT)
    {
        /* Copy the event notification from internal event notification */
        waitData->contents.getDeviceStatusResult.info.notifyEvent = iPodCtrlCfg->iPodInfo->property.curEvent;
    }
    
    /* Running application bit was set */
    if((mask & IPOD_PLAYER_IPOD_STATUS_INFO_MASK_RUNNING_APP) == IPOD_PLAYER_IPOD_STATUS_INFO_MASK_RUNNING_APP)
    {
        /* Get the now playing focus application */
        rc = iPodCoreFuncGetRunningApp(iPodCtrlCfg->threadInfo->iPodDevID, waitData->contents.getDeviceStatusResult.info.appName, sizeof(waitData->contents.getDeviceStatusResult.info.appName));
    }
    
    waitData->contents.getDeviceStatusResult.info.deviceStatusMask = mask;
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_DEVICE_PROPERTY_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}


/* Get the list of database */
S32 iPodCoreiPodCtrlGetDBEntries(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 tempTotal = 0;
    U32 tempStart = 0;
    IPOD_CB_PARAM_NOTIFY_DB_ENTRIES entries;
    U32 checkId = 0;
    U32 length = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&entries, 0, sizeof(entries));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            /* Request the list of database. Status changes to Running from Wait */
            case IPODCORE_QUEUE_STATUS_WAIT:
                waitData->contents.getDBEntries.curNum = 0;
                waitData->contents.getDBEntries.listNum = 0;
                if(waitData->storeBuf == NULL)
                {
                    waitData->storeBuf = calloc(iPodCtrlCfg->threadInfo->maxEntry, sizeof(IPOD_PLAYER_ENTRY_LIST));
                    /* Memory is not allocated */
                    if (waitData->storeBuf == NULL)
                    {
                        rc = IPOD_PLAYER_ERR_NOMEM;
                        waitData->contents.getDBEntriesResult.result = rc;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    }
                }
                
                /* Check the parameter */
                if((rc == IPOD_PLAYER_OK) && (iPodCtrlCfg->iPodInfo->catCountList[waitData->contents.getDBEntries.type] != 0) && 
                   ((waitData->contents.getDBEntries.num > 0) || (waitData->contents.getDBEntries.num == -1)))
                {
                    waitData->contents.getDBEntries.totalNum = iPodCtrlCfg->iPodInfo->catCountList[waitData->contents.getDBEntries.type];
                    /* Application requests to get the all list with indicated category */
                    if(waitData->contents.getDBEntries.num == -1)
                    {
                        /* Total number of list that this command should get is track total count - started index */
                        waitData->contents.getDBEntries.totalNum = waitData->contents.getDBEntries.totalNum - waitData->contents.getDBEntries.start;
                    }
                    else
                    {
                        /* Check the requested number is less than total number */
                        if(waitData->contents.getDBEntries.totalNum >= 
                           (waitData->contents.getDBEntries.num + waitData->contents.getDBEntries.start))
                        {
                            waitData->contents.getDBEntries.totalNum = waitData->contents.getDBEntries.num;
                        }
                        else
                        {
                            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                            waitData->contents.getDBEntriesResult.result = rc;
                            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getDBEntries.totalNum, waitData->contents.getDBEntries.num, waitData->contents.getDBEntries.start);
                        }
                    }
                    
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Check whether currrent requested list total number more than max entry or not */
                        if(waitData->contents.getDBEntries.totalNum >= iPodCtrlCfg->threadInfo->maxEntry)
                        {
                            /* requested list number is separeted to each max entry number */
                            tempTotal = iPodCtrlCfg->threadInfo->maxEntry;
                        }
                        else
                        {
                            /* requested list number is set the remaing list number */
                            tempTotal = waitData->contents.getDBEntries.totalNum;
                        }
                        
                        /* Started index that get the list set the start index that is indicated by Application */
                        tempStart = waitData->contents.getDBEntries.start;
                        
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, tempStart, tempTotal);
                        
                        /* Request to get the list */
                        rc = iPodCoreFuncGetDBEntries(devID, waitData->contents.getDBEntries.type, tempStart, tempTotal, iPodCoreCBGetEntries);
                        if(rc != IPOD_PLAYER_OK)
                        {
                            waitData->contents.getDBEntriesResult.result = rc;
                            waitData->contents.paramResultTemp.header.funcId = IPOD_FUNC_GET_DB_ENTRIES;
                        }
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    *size = sizeof(waitData->contents.getDBEntriesResult);
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->catCountList[waitData->contents.getDBEntries.type], waitData->contents.getDBEntries.num, waitData->contents.getDBEntries.num);
                }
                
                /* Command success. Request still continue */
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
            
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_WAIT, rc);
                break;
                
            case IPODCORE_QUEUE_STATUS_RUNNING:
                checkId = (U32)header->funcId;
                if(checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_DB_ENTRIES))
                {
                    waitData->contents.getDBEntries.list = (IPOD_PLAYER_ENTRY_LIST *)(void *)waitData->storeBuf;
                    
                    if(waitData->contents.getDBEntries.list != NULL)
                    {
                        waitData->contents.getDBEntries.list[waitData->contents.getDBEntries.listNum].trackIndex = contents->getDBEntries.entry.trackIndex;
                        
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitData->contents.getDBEntries.listNum);
                        memcpy(&waitData->contents.getDBEntries.list[waitData->contents.getDBEntries.listNum].name[IPODCORE_POS0], 
                                &contents->getDBEntries.entry.name[IPODCORE_POS0], sizeof(waitData->contents.getDBEntries.list[waitData->contents.getDBEntries.listNum].name));
                        waitData->contents.getDBEntries.listNum++;
                        waitData->contents.getDBEntries.curNum++;
                        
                        /* Maximum entry size is reached. Notify the list of Application */
                        if(waitData->contents.getDBEntries.listNum >= iPodCtrlCfg->threadInfo->maxEntry)
                        {
                            /* Set GetEntry function ID */
                            entries.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_DB_ENTRIES;
                            entries.header.devID = waitData->contents.getDBEntries.header.devID;
                            entries.header.appID = waitData->contents.getDBEntries.header.appID;
                            entries.type = waitData->contents.getDBEntries.type;
                            entries.count = iPodCtrlCfg->threadInfo->maxEntry;
                            
                            if(sizeof(entries.entryList) >= (iPodCtrlCfg->threadInfo->maxEntry * sizeof(IPOD_PLAYER_ENTRY_LIST)))
                            {
                                length = iPodCtrlCfg->threadInfo->maxEntry * sizeof(IPOD_PLAYER_ENTRY_LIST);
                            }
                            else
                            {
                                length = sizeof(entries.entryList);
                            }
                            
                            memcpy(entries.entryList, waitData->contents.getDBEntries.list, length);
                            
                            /* Send to Command Handler queue */
                            rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&entries, sizeof(entries), 0, IPODCORE_TMOUT_FOREVER);
                            if(rc == sizeof(entries))
                            {
                                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
                                rc = IPOD_PLAYER_OK;
                            }
                            
                            /* More entries are requested from APplication */
                            if(waitData->contents.getDBEntries.totalNum > waitData->contents.getDBEntries.curNum)
                            {
                                waitData->contents.getDBEntries.listNum = 0;
                                
                                /* Next request number of entry is more than maximum entry number */
                                if(waitData->contents.getDBEntries.totalNum - waitData->contents.getDBEntries.curNum >= iPodCtrlCfg->threadInfo->maxEntry)
                                {
                                    /* Set maximum entry number */
                                    tempTotal = iPodCtrlCfg->threadInfo->maxEntry;
                                }
                                else
                                {
                                    /* Set remaining entry number */
                                    tempTotal = waitData->contents.getDBEntries.totalNum - waitData->contents.getDBEntries.curNum;
                                }
                                
                                tempStart = waitData->contents.getDBEntries.start + waitData->contents.getDBEntries.curNum;
                                
                                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, tempStart, tempTotal);
                                rc = iPodCoreFuncGetDBEntries(devID, waitData->contents.getDBEntries.type, tempStart, tempTotal, iPodCoreCBGetEntries);
                                if(rc == IPOD_PLAYER_OK)
                                {
                                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                                }
                            }
                            else
                            {
                                waitData->contents.getDBEntriesResult.result = IPOD_PLAYER_OK;
                                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_FINISH);
                                rc = IPOD_PLAYER_OK;
                            }
                        }
                        else
                        {
                            if(waitData->contents.getDBEntries.totalNum <= waitData->contents.getDBEntries.curNum)
                            {
                                entries.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_DB_ENTRIES;
                                entries.header.devID = waitData->contents.getDBEntries.header.devID;
                                entries.header.appID = waitData->contents.getDBEntries.header.appID;
                                entries.type = waitData->contents.getDBEntries.type;
                                entries.count = waitData->contents.getDBEntries.listNum;
                                
                                if(sizeof(entries.entryList) >= (iPodCtrlCfg->threadInfo->maxEntry * sizeof(IPOD_PLAYER_ENTRY_LIST)))
                                {
                                    length = iPodCtrlCfg->threadInfo->maxEntry * sizeof(IPOD_PLAYER_ENTRY_LIST);
                                }
                                else
                                {
                                    length = sizeof(entries.entryList);
                                }
                                
                                memcpy(entries.entryList, waitData->contents.getDBEntries.list, length);
                                
                                rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&entries, sizeof(entries), 0, IPODCORE_TMOUT_FOREVER);
                                if(rc == sizeof(entries))
                                {
                                    rc = IPOD_PLAYER_OK;
                                }
                                waitData->contents.getDBEntriesResult.result = IPOD_PLAYER_OK;
                                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_FINISH, rc);
                            }
                            else
                            {
                                /* Reply is still needed */
                                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                            }
                            
                            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_FINISH, rc);
                        }
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_FINISH, waitData->contents.getDBEntries.totalNum, waitData->contents.getDBEntries.curNum);
                    }
                    else
                    {
                        waitData->contents.paramResultTemp.header.funcId = IPOD_FUNC_GET_DB_ENTRIES;
                        rc = IPOD_PLAYER_ERR_NOMEM;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    }
                }
                else
                {
                    /* Expected reply is still not received */
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                
                
                if(rc <= IPOD_PLAYER_OK)
                {
                    *size = sizeof(waitData->contents.getDBEntriesResult);
                }
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_FINISH, rc);
                break;
                
            case IPODCORE_QUEUE_STATUS_CANCEL:
                if(waitData->contents.getDBEntries.listNum != 0)
                {
                    entries.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_DB_ENTRIES;
                    entries.header.devID = waitData->contents.getDBEntries.header.devID;
                    entries.header.appID = waitData->contents.getDBEntries.header.appID;
                    entries.type = waitData->contents.getDBEntries.type;
                    entries.count = waitData->contents.getDBEntries.listNum;
                    
                    if(sizeof(entries.entryList) >= (iPodCtrlCfg->threadInfo->maxEntry * sizeof(IPOD_PLAYER_ENTRY_LIST)))
                    {
                        length = iPodCtrlCfg->threadInfo->maxEntry * sizeof(IPOD_PLAYER_ENTRY_LIST);
                    }
                    else
                    {
                        length = sizeof(entries.entryList);
                    }
                    
                    memcpy(entries.entryList, waitData->contents.getDBEntries.list, length);
                    
                    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&entries, sizeof(entries), 0, IPODCORE_TMOUT_FOREVER);
                    if(rc == sizeof(entries))
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                }
                waitData->contents.paramResultTemp.header.funcId = IPOD_FUNC_GET_DB_ENTRIES;
                
                rc = IPOD_PLAYER_ERR_API_CANCELED;;
                *size = sizeof(waitData->contents.getDBEntriesResult);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_CANCEL, rc);
                break;
                
            default:
                break;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        *size = sizeof(waitData->contents.getDBEntriesResult);
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    if(rc <= IPOD_PLAYER_OK)
    {
        IPOD_PLAYER_MESSAGE_DATA_CONTENTS *changeContents = NULL;
        changeContents = (IPOD_PLAYER_MESSAGE_DATA_CONTENTS *)contents;
        changeContents->paramTemp.header.appID = waitData->contents.paramResultTemp.header.appID;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Get the total count of indicated type in current database */
S32 iPodCoreiPodCtrlGetDBCount(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        if(waitData->contents.getDBCount.type <= IPOD_PLAYER_DB_TYPE_ITUNESU)
        {
            waitData->contents.getDBCountResult.num = iPodCtrlCfg->iPodInfo->catCountList[waitData->contents.getDBCount.type];
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.getDBCount.type);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_GET_DB_COUNT_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}



void iPodCoreiPodCtrlMoveType(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_DB_TYPE type, S32 entry)
{
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, type, entry);
    
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }
    
    if((iPodCtrlCfg->iPodInfo->curType == type) && (entry == -1))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->iPodInfo->curType, iPodCtrlCfg->iPodInfo->topType);
        switch(iPodCtrlCfg->iPodInfo->curType)
        {
            case IPOD_PLAYER_DB_TYPE_ALL:
                break;
                
            case IPOD_PLAYER_DB_TYPE_PLAYLIST:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_ARTIST:
                switch(iPodCtrlCfg->iPodInfo->topType)
                {
                    case IPOD_PLAYER_DB_TYPE_ARTIST:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_GENRE:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_GENRE;
                        break;
                        
                    default:
                        break;
                }
                break;
                
            case IPOD_PLAYER_DB_TYPE_ALBUM:
                switch(iPodCtrlCfg->iPodInfo->topType)
                {
                    case IPOD_PLAYER_DB_TYPE_ARTIST:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ARTIST;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_GENRE:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ARTIST;
                        break;
                        
                    default:
                        break;
                }
                break;
                
            case IPOD_PLAYER_DB_TYPE_GENRE:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_TRACK:
                switch(iPodCtrlCfg->iPodInfo->topType)
                {
                    case IPOD_PLAYER_DB_TYPE_PLAYLIST:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_PLAYLIST;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_ARTIST:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALBUM;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_ALBUM:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALBUM;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_GENRE:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALBUM;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_COMPOSER:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_COMPOSER;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_AUDIOBOOK;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_PODCAST:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_PODCAST;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_INTELLIGENT;
                        break;
                        
                    case IPOD_PLAYER_DB_TYPE_ITUNESU:
                        iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ITUNESU;
                        break;
                    default:
                        break;
                }
                break;
                
            case IPOD_PLAYER_DB_TYPE_COMPOSER:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_AUDIOBOOK:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_PODCAST:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_NESTED_PLAYLIST:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_INTELLIGENT:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
                
            case IPOD_PLAYER_DB_TYPE_ITUNESU:
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                break;
            
            default:
                break;
        }
    }
    else
    {
        iPodCtrlCfg->iPodInfo->curType = type;
    }
    
}

/* Select the database entry to indicated entry */
S32 iPodCoreiPodCtrlSelectDBEntry(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if((waitData->contents.selectDBEntry.type != IPOD_PLAYER_DB_TYPE_TRACK) && (waitData->contents.selectDBEntry.type != IPOD_PLAYER_DB_TYPE_AUDIOBOOK))
        {
            if ((S32)iPodCtrlCfg->iPodInfo->catCountList[waitData->contents.selectDBEntry.type] > waitData->contents.selectDBEntry.entry)
            {
                /* Request to select the database entry with indicated entry to iPod Ctrl */
                rc = iPodCoreFuncSelectDBEntry(devID, waitData->contents.selectDBEntry.type, waitData->contents.selectDBEntry.entry);
            }
            else
            {
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->iPodInfo->catCountList[waitData->contents.selectDBEntry.type], waitData->contents.selectDBEntry.entry);
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Current selected type is top */
            if(iPodCtrlCfg->iPodInfo->topType == IPOD_PLAYER_DB_TYPE_ALL)
            {
                /* Set the top type */
                iPodCtrlCfg->iPodInfo->topType = waitData->contents.selectDBEntry.type;
            }
            
            iPodCoreiPodCtrlMoveType(iPodCtrlCfg, waitData->contents.selectDBEntry.type, waitData->contents.selectDBEntry.entry);
            
            rc = iPodCoreFuncGetLowerCatList(devID, iPodCtrlCfg);
        }
        
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SELECT_DB_ENTRY_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlCancelDB(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_CORE_WAIT_LIST *waitList = NULL;
    U32 i = 0;
    U32 curListNum = 0;
    IPOD_PLAYER_PARAM_GET_DB_ENTRIES entries;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&entries, 0, sizeof(entries));
    
    /* Database group is running  */
    if(iPodCtrlCfg->curRunMask & IPOD_PLAYER_DATABASE_GROUP_MASK)
    {
        /* Set the ID of iPodID of ipod ctrl */
        waitList = iPodCtrlCfg->waitList;
        
        rc = IPOD_PLAYER_ERROR;
        
        /* Serch that cancel is queued to which number */
        for(i = 0; (i < IPODCORE_WAIT_LIST_MAX) && (rc != IPOD_PLAYER_OK); i++)
        {
            /* current indicated queue equal to cance queue and status is wait */
            if((waitList[i].contents.paramTemp.header.funcId == waitData->contents.paramTemp.header.funcId) &&
               (waitList[i].status == IPODCORE_QUEUE_STATUS_WAIT))
            {
                /* Set the cancel queue number */
                curListNum = i;
                rc = IPOD_PLAYER_OK;
            }
        }
        
        /* Cance queue found */
        if(rc == IPOD_PLAYER_OK)
        {
            rc = IPOD_PLAYER_ERROR;
            
            /* Search whether running queue is exsited  before cancel queue */
            for(i = 0; (i < curListNum) && (rc != IPOD_PLAYER_OK); i++)
            {
                /* Database groupr is running  */
                if(iPodCtrlCfg->curRunMask & IPOD_PLAYER_DATABASE_GROUP_MASK)
                {
                    /* function of queue of current indicated number is get database */
                    if(waitList[i].contents.paramTemp.header.funcId == IPOD_FUNC_GET_DB_ENTRIES)
                    {
                        /* Status of queue of indicated number changes to IPODCORE_QUEUE_STATUS_CANCEL. */
                        waitList[i].status = IPODCORE_QUEUE_STATUS_CANCEL;
                        /* Send the get database entry to give the trigger */
                        entries.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_DB_ENTRIES);
                        entries.header.devID = waitData->contents.getDBEntries.header.devID;
                        entries.header.appID = waitData->contents.getDBEntries.header.appID;
                        entries.type = waitData->contents.getDBEntries.type;
                        rc = IPOD_PLAYER_OK;
                    }
                }
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* Set the timer to be executed the canceled queue */
                waitData->nextRequest = 1;
                iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->curRunMask);
    }
    
    return rc;
}

S32 iPodCoreiPodCtrlCancelCoverart(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_PLAYER_CORE_WAIT_LIST *waitList = NULL;
    U32 i = 0;
    U32 curListNum = 0;
    IPOD_PLAYER_PARAM_GET_COVERART coverart;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&coverart, 0, sizeof(coverart));
    
    /* Caverart group is running  */
    if((iPodCtrlCfg->curRunMask & IPOD_PLAYER_COVERART_GROUP_MASK) == IPOD_PLAYER_COVERART_GROUP_MASK)
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        waitList = iPodCtrlCfg->waitList;
        
        rc = IPOD_PLAYER_ERROR;
        
        /* Serch that cancel is queued to which number */
        for(i = 0; (i < IPODCORE_WAIT_LIST_MAX) && (rc != IPOD_PLAYER_OK); i++)
        {
            /* current indicated queue equal to cance queue and status is wait */
            if((waitList[i].contents.paramTemp.header.funcId == waitData->contents.paramTemp.header.funcId) &&
               (waitList[i].status == IPODCORE_QUEUE_STATUS_WAIT))
            {
                /* Set the cancel queue number */
                curListNum = i;
                rc = IPOD_PLAYER_OK;
            }
        }
        
        /* Cance queue found */
        if(rc == IPOD_PLAYER_OK)
        {
            rc = IPOD_PLAYER_ERROR;
            /* Search whether running queue is exsited  before cancel queue */
            for(i = 0; (i < curListNum) && (rc != IPOD_PLAYER_OK); i++)
            {
                /* Database groupr is running  */
                if(iPodCtrlCfg->curRunMask & IPOD_PLAYER_COVERART_GROUP_MASK)
                {
                    /* function of queue of current indicated number is get database */
                    if(waitList[i].contents.paramTemp.header.funcId == IPOD_FUNC_GET_COVERART)
                    {
                        rc = iPodCoreFuncCancel(devID, waitData->contents.cancel.type);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            /* Status of queue of indicated number changes to IPODCORE_QUEUE_STATUS_CANCEL. */
                            waitList[i].status = IPODCORE_QUEUE_STATUS_CANCEL;
                            waitList[i].nextRequest = 1;
                            /* Send the get database entry to give the trigger */
                            coverart.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_COVERART);
                            coverart.header.devID = waitData->contents.getCoverart.header.devID;
                            coverart.header.appID = waitData->contents.getCoverart.header.appID;
                            coverart.type = waitData->contents.getCoverart.type;
                        }

                    }
                }
            }
            
            iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->curRunMask);
    }
    
    return rc;
}

/* Cancel the current request */
S32 iPodCoreiPodCtrlCancel(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        if(waitData->contents.cancel.type == IPOD_PLAYER_CANCEL_DB_ENTRY)
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_CANCEL_DB_ENTRY);
            rc = iPodCoreiPodCtrlCancelDB(iPodCtrlCfg, waitData, contents, size);
        }
        else if(waitData->contents.cancel.type == IPOD_PLAYER_CANCEL_COVERART)
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_CANCEL_COVERART);
            rc = iPodCoreiPodCtrlCancelCoverart(iPodCtrlCfg, waitData, contents, size);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->contents.cancel.type);
        }

    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_CANCEL_RESULT);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


/* Clear the current selection until indicated entry */
S32 iPodCoreiPodCtrlClearSelection(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                   IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Current category is not top or clear type is not top.*/
        if((iPodCtrlCfg->iPodInfo->curType != IPOD_PLAYER_DB_TYPE_ALL) || 
                (waitData->contents.clearSelection.type != IPOD_PLAYER_DB_TYPE_ALL))
        {
            /* Request to clear the current selection until indicated type */
            rc = iPodCoreFuncClearSelection(devID, waitData->contents.clearSelection.type, iPodCtrlCfg->iPodInfo->topType, iPodCtrlCfg->iPodInfo->curType);
            if(rc == IPOD_PLAYER_OK)
            {
                /* clear type is all that means clear until top entry */
                if(waitData->contents.clearSelection.type == IPOD_PLAYER_DB_TYPE_ALL)
                {
                    /* Clear the top type */
                    iPodCtrlCfg->iPodInfo->topType = IPOD_PLAYER_DB_TYPE_ALL;
                }
                
                /* Set the current type */
                iPodCtrlCfg->iPodInfo->curType = waitData->contents.clearSelection.type;
                
                iPodCtrlCfg->iPodInfo->curTotalType = IPOD_PLAYER_DB_TYPE_UNKNOWN;
                /* Set the current type */
                iPodCtrlCfg->iPodInfo->curTotalCount = 0;
                
                rc = iPodCoreFuncGetLowerCatList(devID, iPodCtrlCfg);
                
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_CLEAR_SELECTION_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}


/* Change the Audio/Video database */
S32 iPodCoreiPodCtrlSelectAV(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                             IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Request to change the Audio/Video database */
        rc = iPodCoreFuncSelectAV(devID, waitData->contents.selectAV.avType);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCtrlCfg->iPodInfo->avType = waitData->contents.selectAV.avType;
        }
        else
        {
            /* Previous mode is audio (also default mode is audio) and selected mode is audio */
            /* This case always returns success because Apple device must have audio database */
            if((iPodCtrlCfg->iPodInfo->avType == IPOD_PLAYER_AUDIO_MODE) && 
               (waitData->contents.selectAV.avType == IPOD_PLAYER_AUDIO_MODE))
            {
                rc = IPOD_PLAYER_OK;
            }
        }
        
        /* SelectAV is success */
        if(rc == IPOD_PLAYER_OK)
        {
            /* Request to clear the current selection by ALL */
            rc = iPodCoreFuncClearSelection(devID, IPOD_PLAYER_DB_TYPE_ALL,
                                            iPodCtrlCfg->iPodInfo->topType, iPodCtrlCfg->iPodInfo->curType);
            if(rc == IPOD_PLAYER_OK)
            {
                /* Clear the top type */
                iPodCtrlCfg->iPodInfo->topType = IPOD_PLAYER_DB_TYPE_ALL;
                /* Set the current type to top */
                iPodCtrlCfg->iPodInfo->curType = IPOD_PLAYER_DB_TYPE_ALL;
                iPodCtrlCfg->iPodInfo->curTotalType = IPOD_PLAYER_DB_TYPE_UNKNOWN;
                iPodCtrlCfg->iPodInfo->curTotalCount = 0;
                rc = iPodCoreFuncGetLowerCatList(devID, iPodCtrlCfg);
            }
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_SELECT_AV_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

/* Open the song tag file*/
S32 iPodCoreiPodCtrlOpenSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Request to change the Audio/Video database */
        rc = iPodCoreFuncOpenSongTag(devID, iPodCtrlCfg, waitData->contents.openSongTagFile.tagOptionsMask, waitData->contents.openSongTagFile.optionLen, 
                                        waitData->contents.openSongTagFile.optionData, &waitData->contents.openSongTagFileResult.handle);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCtrlCfg->tagHandle = waitData->contents.openSongTagFileResult.handle;
            iPodCtrlCfg->tagWriteFlag = 0;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_OPEN_SONG_TAG_FILE_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

/* Close the song tag file*/
S32 iPodCoreiPodCtrlCloseSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    IPOD_PLAYER_PARAM_CLOSE_SONG_TAG_FILE closeTag;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&closeTag, 0, sizeof(closeTag));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            case IPODCORE_QUEUE_STATUS_WAIT:
            case IPODCORE_QUEUE_STATUS_RUNNING:
                /* Check whether tag is arelady opened */
                if(iPodCtrlCfg->tagHandle == waitData->contents.closeSongTagFile.handle)
                {
                    /* Application requested to write the tag */
                    if(iPodCtrlCfg->tagWriteFlag != 0)
                    {
                        /* Request to write the tagData */
                        rc = iPodCoreFuncWriteTagging(devID, iPodCtrlCfg, waitData->contents.closeSongTagFile.handle);
                        if(rc > IPOD_PLAYER_OK)
                        {
                            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        }
                    }
                    else
                    {
                        rc = IPOD_PLAYER_OK;
                    }
                    
                    if(rc == IPOD_PLAYER_ERR_REQUEST_CONTINUE)
                    {
                        waitData->nextRequest = 1;
                        iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
                    }
                    else
                    {
                        rc = iPodCoreFuncCloseSongTag(devID, iPodCtrlCfg, waitData->contents.closeSongTagFile.handle);
                        iPodCtrlCfg->tagHandle = IPODCORE_TAG_HANDLE_DEFAULT;
                        /* Set the result command size */
                        *size = sizeof(IPOD_CB_PARAM_CLOSE_SONG_TAG_FILE_RESULT);
                    }
                }
                else
                {
                    /* Set the result command size */
                    *size = sizeof(IPOD_CB_PARAM_CLOSE_SONG_TAG_FILE_RESULT);
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->tagHandle, waitData->contents.closeSongTagFile.handle);
                }
                break;
           default:
            break;
        }
    }
    else
    {
        /* Set the result command size */
        *size = sizeof(IPOD_CB_PARAM_CLOSE_SONG_TAG_FILE_RESULT);
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

/* Close the song tag file*/
S32 iPodCoreiPodCtrlSongTag(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        if(iPodCtrlCfg->tagHandle == waitData->contents.songTag.handle)
        {
            /* Request to change the Audio/Video database */
            rc = iPodCoreFuncSongTag(devID, iPodCtrlCfg, waitData->contents.songTag.handle, waitData->contents.songTag.type, &waitData->contents.songTag.info);
            if(rc == IPOD_PLAYER_OK)
            {
                iPodCtrlCfg->tagWriteFlag = 1;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->tagHandle, waitData->contents.songTag.handle);
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_OPEN_SONG_TAG_FILE_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

/* Send the data to iOS Application */
S32 iPodCoreiPodCtrlSendToApp(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 i = 0;
    U32 handle = 0;
    U32 dataSize = 0;
    U8 *data = NULL;
    U32 sendSize = 0;
    IPOD_PLAYER_PARAM_SEND_TO_APP sendApp;
    U32 checkId = 0;

    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(waitData->storeBuf == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, waitData->storeBuf);
        return IPOD_PLAYER_ERR_NOMEM;
    }
    
    memset(&sendApp, 0, sizeof(sendApp));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        memcpy((char *)&handle, (const char*)waitData->storeBuf, sizeof(handle));
        memcpy((char *)&dataSize, (const char*)&waitData->storeBuf[sizeof(dataSize)], sizeof(dataSize));

        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            case IPODCORE_QUEUE_STATUS_WAIT:
            case IPODCORE_QUEUE_STATUS_RUNNING:
                checkId = (U32)header->funcId;
                if((waitData->status == IPODCORE_QUEUE_STATUS_WAIT) || (waitData->nextRequest != 0) || 
                  ((waitData->status == IPODCORE_QUEUE_STATUS_RUNNING) && (checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_SEND_TO_APP))))
                {
                    waitData->contents.sendToApp.handle = handle;
                    waitData->contents.sendToApp.dataSize = dataSize;
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING);
                    if(iPodCtrlCfg->iPodInfo->property.maxPayload <= (waitData->contents.sendToApp.dataSize - waitData->storeData))
                    {
                        sendSize = iPodCtrlCfg->iPodInfo->property.maxPayload;
                    }
                    else
                    {
                        sendSize = waitData->contents.sendToApp.dataSize - waitData->storeData;
                    }
                    
                    if(waitData->storeBuf != NULL)
                    {
                        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                        data = &waitData->storeBuf[sizeof(handle) + sizeof(dataSize) + sizeof(waitData->contents.sendToApp.data)];
                        for(i = 0; (i < IPODCORE_MAX_IOSAPPS_INFO_NUM) && (rc != IPOD_PLAYER_OK); i++)
                        {
                            if(iPodCtrlCfg->iOSAppID[i].appID == waitData->contents.sendToApp.handle)
                            {
                                /* Request to change the Audio/Video database */
                                rc = iPodCoreFuncSendToApp(devID, iPodCtrlCfg->iOSAppID[i].sessionID, sendSize, &data[waitData->storeData]);
                            }
                        }
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
                    }
                    
                    if(rc == IPOD_PLAYER_OK)
                    {
                        if(waitData->storeData + sendSize < waitData->contents.sendToApp.dataSize)
                        {
                            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING);
                            waitData->storeData += sendSize;
                            waitData->nextRequest = 1;
                            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                            iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
                        }
                        else
                        {
                            waitData->nextRequest = 0;
                            /* Set the result command size */
                            *size = sizeof(IPOD_CB_PARAM_SEND_TO_APP_RESULT);
                        }
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                break;
                
            default :
                rc = IPOD_PLAYER_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->status);
                break;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        /* Set the result command size */
        *size = sizeof(IPOD_CB_PARAM_SEND_TO_APP_RESULT);
    }
    
    if(rc <= IPOD_PLAYER_OK)
    {
        handle = waitData->contents.sendToApp.handle;
        waitData->contents.sendToAppResult.header.funcId = IPOD_FUNC_SEND_TO_APP;
        waitData->contents.sendToAppResult.handle = handle;
        waitData->contents.sendToAppResult.header.longData = 0;
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    }

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}


/* Request to start the iOS Application that is indicated by Application */
S32 iPodCoreiPodCtrlRequestAppStart(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        /* Request to change the Audio/Video database */
        rc = iPodCoreFuncRequestAppStart(devID, (U8 *)waitData->contents.requestAppStart.appName);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus);
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_REQUEST_APP_START_RESULT);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

S32 iPodCoreiPodCtrlSetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_PAI_SET_VOLUME setVolume;
    U32 sendSize = 0;
    U32 checkId = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&setVolume, 0, sizeof(setVolume));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    /* And audio mode is ON. It means that audio streaming is running */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->audioSetting.mode == IPOD_PLAYER_SOUND_MODE_ON))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitData->status);
        switch(waitData->status)
        {
            case IPODCORE_QUEUE_STATUS_WAIT:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_WAIT);
                setVolume.header.funcId = IPOD_FUNC_PAI_SETVOL;
                setVolume.header.appID = waitData->contents.paramTemp.header.appID;
                setVolume.header.devID = iPodCtrlCfg->threadInfo->appDevID;
                setVolume.volume = waitData->contents.setVolume.volume;
                sendSize = sizeof(setVolume);
                
                if(setVolume.volume <= IPODCORE_VOLUME_MAX)
                {
                    rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, (U8 *)&setVolume, sendSize, 0, IPODCORE_TMOUT_FOREVER);
                    if(rc == (S32)sendSize)
                    {
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, setVolume.volume);
                }
                break;
                
            case IPODCORE_QUEUE_STATUS_RUNNING:
                checkId = (U32)header->funcId;
                if(checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_SET_VOLUME))
                {
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING, checkId);
                    rc = contents->paiSetVolume.result;
                    /* Set the result command size */
                    *size = sizeof(IPOD_CB_PARAM_SET_VOLUME_RESULT);
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                break;
                
            default:
                rc = IPOD_PLAYER_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->status);
                break;
                
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->audioSetting.mode);
    }
    
    if(rc < IPOD_PLAYER_OK)
    {
        /* Set the result command size */
        *size = sizeof(IPOD_CB_PARAM_SET_VOLUME_RESULT);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

S32 iPodCoreiPodCtrlGetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_PAI_GET_VOLUME getVolume;
    U32 sendSize = 0;
    U32 checkId = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    memset(&getVolume, 0, sizeof(getVolume));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    /* And audio mode is ON. It means that audio streaming is running */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->audioSetting.mode == IPOD_PLAYER_SOUND_MODE_ON))
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, waitData->status);
        switch(waitData->status)
        {
            case IPODCORE_QUEUE_STATUS_WAIT:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_WAIT);
                
                /* Audio mode is ON. It means that audio streaming is running */
                if(iPodCtrlCfg->audioSetting.mode == IPOD_PLAYER_SOUND_MODE_ON)
                {
                    getVolume.header.funcId = IPOD_FUNC_PAI_GETVOL;
                    getVolume.header.appID = waitData->contents.paramTemp.header.appID;
                    getVolume.header.devID = iPodCtrlCfg->threadInfo->appDevID;
                    sendSize = sizeof(getVolume);
                    rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, (U8 *)&getVolume, sendSize, 0, IPODCORE_TMOUT_FOREVER);
                    if(rc == (S32)sendSize)
                    {
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_INVALID_MODE;
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->audioSetting.mode);
                }
                
                break;
                
            case IPODCORE_QUEUE_STATUS_RUNNING:
                checkId = (U32)header->funcId;
                if(checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_VOLUME))
                {
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING, checkId);
                     rc = contents->paiGetVolume.result;
                    waitData->contents.getVolumeResult.volume = contents->paiGetVolume.volume;
                    
                    /* Set the result command size */
                    *size = sizeof(IPOD_CB_PARAM_GET_VOLUME_RESULT);
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                break;
                
            default:
                rc = IPOD_PLAYER_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, waitData->status);
                break;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->audioSetting.mode);
    }
    
    if(rc < IPOD_PLAYER_OK)
    {
        /* Set the result command size */
        *size = sizeof(IPOD_CB_PARAM_GET_VOLUME_RESULT);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
    
}

static S32 iPodCoreiPodCtrlIntGetTrackInfoWaitStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    U64 startID = 0;
    U32 trackInfoMask = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;

    /* Save each parameter of GetTrackInfo */
    trackInfoMask = iPodCtrlCfg->iPodInfo->trackInfoMask & (~IPOD_PLAYER_TRACK_INFO_MASK_COVERART);
    startID = waitData->contents.intGetTrackInfo.trackIndex;

    /* Set the requested type, startID, count, mask and current track ID */
    waitData->contents.notifyTrackInfo.trackIndex = startID;
    waitData->contents.notifyTrackInfo.info.trackInfoMask = trackInfoMask;
    waitData->contents.notifyTrackInfo.header.funcId = IPOD_FUNC_TRACK_INFO;
    waitData->contents.notifyTrackInfo.header.devID = devID;
    
    waitData->nextRequest = 1;
    iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
    
    rc = IPOD_PLAYER_OK;
    
    return rc;
}

static S32 iPodCoreiPodCtrlIntGetTrackInfoOld(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    U64 trackID = 0;
    U32 *trackInfoMask = NULL;
    U32 *recvMask = NULL;
    U32 supportedMask = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U32 curMask = 0;
    U32 i = 0;
    U32 mask = 0;
    U32 *waitTime = NULL;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    /* Save each paramters */
    type = IPOD_PLAYER_TRACK_TYPE_PLAYBACK;
    trackID = waitData->contents.notifyTrackInfo.trackIndex;
    trackInfoMask = &waitData->contents.notifyTrackInfo.info.trackInfoMask;
    recvMask = &waitData->contents.notifyTrackInfo.mask;
    supportedMask = iPodCtrlCfg->iPodInfo->property.supportedFeatureMask;
    
    /* Loop until bit maximum size. Int is 32 */
    for(i = 0; (i < IPOD_PLAYER_BIT_INT_NUM) && (rc != IPOD_PLAYER_OK); i++)
    {
        /* Check the bit which should get the information and check the bit which has already gotten the information */
        if(((*trackInfoMask & (U32)(1 << i))  == (U32)(1 << i)) && ((*recvMask & (U32)(1 << i)) != (U32)(1 << i)))
        {
            mask = 1 << i;
            
            switch(mask)
            {
                case IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME:
                    waitTime = &iPodCtrlCfg->iPodInfo->getTrackTime;
                    break;
                    
                case IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME:
                    waitTime = &iPodCtrlCfg->iPodInfo->getAlbumTime;
                    break;
                    
                case IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME:
                    waitTime = &iPodCtrlCfg->iPodInfo->getArtistTime;
                    break;
                default:
                    waitTime = &iPodCtrlCfg->iPodInfo->getInfoTime;
                    break;
            }
            
            if((mask & waitData->contents.getTrackInfoResult.sendMask) != mask)
            {
                rc = iPodCoreiPodCtrlTimedWait(iPodCtrlCfg->iPodInfo->getintTrackInfoFd, waitTime);
                if(rc != IPOD_PLAYER_OK)
                {
                    /* Request to get the track information to iPod Ctrl with indicated type, trackID and mask */
                    rc = iPodCoreFuncGetTrackInfo(devID, type, trackID, &mask, &curMask, supportedMask, 
                                                    &waitData->contents.notifyTrackInfo.info, (void *)iPodCoreCBNotifyOldGetCurrentTrackInfo);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        /* Add the current got information to received mask */
                        *recvMask |= curMask;
                        waitData->contents.notifyTrackInfo.sendMask = mask;
                    }
                    else
                    {
                        /* Requested information could not get */
                        if(mask == 0)
                        {
                            *trackInfoMask &= (~(1 << i));
                            /* Other information still remain */
                            if(*trackInfoMask != 0)
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                        }
                        else
                        {
                            break;
                        }
                    }
                    *waitTime = iPodGetTime();
                }
                else
                {
                    break;
                }
            }
            else
            {
                rc = IPOD_PLAYER_OK;
                break;
            }
            
        }
        
        /* All information have already gotten */
        if(i == IPOD_PLAYER_BIT_INT_NUM -1)
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Check whether next request should send. */
    if((*trackInfoMask != *recvMask) && (rc == IPOD_PLAYER_OK) &&
        ((mask == IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME) || (mask == IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME) || 
        (mask == IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME) || (mask == 0)))
    {
        iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
        waitData->nextRequest = 1;
    }
    else
    {
        waitData->nextRequest = 0;
    }
    
    /* Current receiving mask equals to application requested mask. It means that all of requested information could get from iPod Ctrl */
    if(*trackInfoMask == *recvMask)
    {
        /* Current queue status changes to Finish */
        rc = IPOD_PLAYER_OK;
    }
    
    return rc;
}

static S32 iPodCoreiPodCtrlIntGetTrackInfoNew(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, 
                                            const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 devID = 0;
    U64 trackID = 0;
    U32 supportedMask = 0;
    IPOD_PLAYER_TRACK_TYPE type = IPOD_PLAYER_TRACK_TYPE_UNKNOWN;
    U32 curMask = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set the ID of iPodID of ipod ctrl */
    devID = iPodCtrlCfg->threadInfo->iPodDevID;
    
    /* Save each paramters */
    type = IPOD_PLAYER_TRACK_TYPE_PLAYBACK;
    trackID = waitData->contents.notifyTrackInfo.trackIndex;
    supportedMask = iPodCtrlCfg->iPodInfo->property.supportedFeatureMask;
    
    if((contents->notifyTrackInfo.resultFlag == 0) || (waitData->nextRequest != 0))
    {
        waitData->nextRequest = 0;
        rc = iPodCoreFuncGetTrackInfo(devID, type, trackID, &waitData->contents.notifyTrackInfo.info.trackInfoMask, 
                        &curMask, supportedMask, &waitData->contents.notifyTrackInfo.info, (IPOD_TRACK_INFORMATION_CB)iPodCoreCBNotifyGetCurrentTrackInfo);
        if(rc == IPOD_UNKNOWN_ID)
        {
            iPodCtrlCfg->iPodInfo->property.supportedFeatureMask &= (~IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT);
            /* Set the requested type, startID, count, mask and current track ID */
            /* Current request is finished and new request is sent for old get track infor */
            waitData->nextRequest = 1;
            iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
            rc = IPOD_PLAYER_ERR_NO_REPLY;
        }
    }
    else
    {
        rc = IPOD_PLAYER_OK;
    }
    
    /* Current receiving mask equals to application requested mask. It means that all of requested information could get from iPod Ctrl */
    if(waitData->contents.notifyTrackInfo.mask == waitData->contents.notifyTrackInfo.info.trackInfoMask)
    {
        rc = IPOD_PLAYER_OK;
    }
    
    return rc;
}


S32 iPodCoreiPodCtrlIntGetTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U64 startID = 0;
    U32 mask = 0;
    IPOD_PLAYER_FUNC_ID funcId = IPOD_FUNC_INIT;
    U32 checkId = 0;

    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL) && (iPodCtrlCfg->iPodInfo->trackInfoMask != 0))
    {
         /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        startID = waitData->contents.intGetTrackInfo.trackIndex;
        mask = iPodCtrlCfg->iPodInfo->trackInfoMask;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            /* Request to get the track information to iPod Ctrl at first */
            case IPODCORE_QUEUE_STATUS_WAIT:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_WAIT, mask);
                if((mask & IPOD_PLAYER_TRACK_INFO_MASK_COVERART) == IPOD_PLAYER_TRACK_INFO_MASK_COVERART)
                {
                    iPodCoreFuncGetCoverartInfo(devID, IPOD_PLAYER_TRACK_TYPE_PLAYBACK, startID, iPodCtrlCfg->iPodInfo->formatId, 
                                        &iPodCtrlCfg->iPodInfo->coverartCount, iPodCtrlCfg->iPodInfo->coverartTime, sizeof(iPodCtrlCfg->iPodInfo->coverartTime));
                    mask = mask & (~IPOD_PLAYER_TRACK_INFO_MASK_COVERART);
                }
                
                rc = iPodCoreiPodCtrlIntGetTrackInfoWaitStatus(iPodCtrlCfg, waitData);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Current queue status chagnes to Running */
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                else
                {
                    /* Result is not sent to application because this function is internal */
                    rc = IPOD_PLAYER_ERR_NO_REPLY;
                }
                break;
                
            /* Wait to receive the data from callback function */
            case IPODCORE_QUEUE_STATUS_RUNNING:
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING, contents->notifyTrackInfo.mask);
                checkId = (U32)header->funcId;
                
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, checkId, waitData->nextRequest);
                if((checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_TRACK_INFO)) || (waitData->nextRequest != 0))
                {
                    if((contents->notifyTrackInfo.resultFlag == 1) && (waitData->nextRequest == 0))
                    {
                        iPodCoreiPodCtrlCheckTrackInfo(contents->notifyTrackInfo.mask, &contents->notifyTrackInfo.info,
                                        &waitData->contents.notifyTrackInfo.mask, &waitData->contents.notifyTrackInfo.info);
                    }
                    
                     /* Check the feature or connected iPod*/
                    if((iPodCtrlCfg->iPodInfo->property.supportedFeatureMask & IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT) == 
                        IPOD_PLAYER_FEATURE_MASK_TRACK_INFO_RESTRICT)
                    {
                        rc = iPodCoreiPodCtrlIntGetTrackInfoNew(iPodCtrlCfg, waitData, contents);
                    }
                    else
                    {
                        rc = iPodCoreiPodCtrlIntGetTrackInfoOld(iPodCtrlCfg, waitData);
                    }
                    
                    /* Current receiving mask equals to application requested mask. It means that all of requested information could get from iPod Ctrl */
                    if(waitData->contents.notifyTrackInfo.mask == waitData->contents.notifyTrackInfo.info.trackInfoMask)
                    {
                        funcId = waitData->contents.getChapterInfoResult.header.funcId;
                        waitData->contents.notifyTrackInfo.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_RESULT_ID_MASK | waitData->contents.notifyTrackInfo.header.funcId);
                        /* Set the result command size */
                        *size = sizeof(IPOD_PLAYER_PARAM_NOTIFY_TRACK_INFO);
                        
                        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&waitData->contents, *size, 0, IPODCORE_TMOUT_FOREVER);
                        if(rc == (S32)*size)
                        {
                            waitData->contents.getChapterInfoResult.header.funcId = funcId;
                        }
                        rc = IPOD_PLAYER_ERR_NO_REPLY;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                    }
                }
                else
                {
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
                break;
            default: break;
        }
    }
    else
    {
        *size = sizeof(IPOD_PLAYER_PARAM_NOTIFY_TRACK_INFO);
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg->deviceConnection.deviceStatus, iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->audioSetting.mode);
        rc = IPOD_PLAYER_ERR_NO_REPLY;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlIntGetDeviceProperty(U32 devID, U32 mask, IPOD_PLAYER_DEVICE_PROPERTY *property)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, devID, mask, property);
    
    rc = iPodCoreFuncGetDeviceProperty(devID, mask, property);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlIntGetCaps(U32 devID, U64 *totalSpace, U32 *maxFileSize, U16 *maxWriteSize)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, devID, totalSpace, maxFileSize, maxWriteSize);
    
    rc = iPodCoreFuncGetCaps(devID, totalSpace, maxFileSize, maxWriteSize);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlIntGetNumEQ(U32 devID, U32 *maxEQ)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, devID, maxEQ);
    
    rc = iPodCoreFuncGetNumEQ(devID, maxEQ);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlInitConnection(void)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    rc = iPodCoreFuncInitConnection();
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlDisconnect(void)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    
    iPodCoreFuncDisconnect();
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlIntGetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, devID, setting);
    
    if(setting == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, setting);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    rc = iPodCoreFuncIntGetVideoSetting(devID, setting);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlEndForward(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, devID);
    
    rc = iPodCoreFuncEndForward(devID);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlStatusCheck(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, contents, size);
    
    if((iPodCtrlCfg == NULL) || (contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, contents);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->audioSetting.adjust == IPOD_PLAYER_STATE_ADJUST_ENABLE)
    {
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->playbackStatus.status);
        /* Status check by polling. If iPod status was not changed to expected status, status changing of iPod is sent again */
        switch(iPodCtrlCfg->playbackStatus.status)
        {
            case IPOD_PLAYER_PLAY_STATUS_PLAY:
                rc = iPodCoreiPodCtrlStatusChangeToPlay(iPodCtrlCfg, contents);
                break;
                
            case IPOD_PLAYER_PLAY_STATUS_PAUSE:
                rc = iPodCoreiPodCtrlStatusChangeToPause(iPodCtrlCfg, contents);
                break;
                
            case IPOD_PLAYER_PLAY_STATUS_STOP:
                rc = iPodCoreiPodCtrlStatusChangeToStop(iPodCtrlCfg, contents);
                break;
                
            case IPOD_PLAYER_PLAY_STATUS_FF:
                rc = iPodCoreiPodCtrlStatusChangeToFastForward(iPodCtrlCfg, contents);
                break;
                
            case IPOD_PLAYER_PLAY_STATUS_RW:
                rc = iPodCoreiPodCtrlStatusChangeToRewind(iPodCtrlCfg, contents);
                break;
                
            default: 
                rc = IPOD_PLAYER_OK;
                break;
        }
    }
    else
    {
        rc = IPOD_PLAYER_OK;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Initialize the timer */
S32 iPodCoreiPodCtrlTimerInit(S32 *fd, U32 *handleNum, S32 *handle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 timerfd = -1;
    S32 getHandle = -1;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, fd, handleNum, handle);
    
    /* Parameter check */
    if((fd == NULL) || (handleNum == NULL) || (handle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, fd, handleNum, handle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Create the timerfd */
    timerfd = timerfd_create(CLOCK_MONOTONIC, 0);
    if(timerfd != -1)
    {
        *fd = timerfd;
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        S32 errID = errno;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, timerfd, errID);
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Register the fd into IPC library */
        rc = iPodPlayerIPCCreateHandle(&getHandle, *fd);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Register the fd into handle table */
            rc = iPodCoreSetHandle(handle, handleNum, *fd);
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* De-initialize the timer */
S32 iPodCoreiPodCtrlTimerDeinit(S32 fd, U32 *handleNum, S32 *handle)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, fd, handleNum, handle);
    
    /* Parameter check */
    if((handleNum == NULL) || (handle == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handleNum, handle);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Clear the handle from handle table */
    rc = iPodCoreClearHandle(handle, handleNum, fd);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Delete the fd from IPC library */
        rc = iPodPlayerIPCDeleteHandle(fd);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = close(fd);
            if(rc != -1)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                S32 errID = errno;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, errID);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

/* Set the timer */
S32 iPodCoreiPodCtrlSetTimer(S32 fd, U32 timems, U32 interval)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct itimerspec val;
    
    /* Initialize the structure */
    memset(&val, 0, sizeof(val));
    
    /* Set the alarm time */
    val.it_value.tv_sec     = timems / IPOD_PLAYER_TIME_MSEC_TO_SEC;
    val.it_value.tv_nsec    = (timems % IPOD_PLAYER_TIME_MSEC_TO_SEC) * IPOD_PLAYER_TIME_MSEC_TO_NSEC;
    val.it_interval.tv_sec  = interval / IPOD_PLAYER_TIME_MSEC_TO_SEC;
    val.it_interval.tv_nsec = (interval % IPOD_PLAYER_TIME_MSEC_TO_SEC) * IPOD_PLAYER_TIME_MSEC_TO_NSEC;
    
    rc = timerfd_settime(fd, 0, &val, NULL);
    if(rc != -1)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        S32 errID = errno;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, errID);
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
    
}

/* Get the current set timer */
S32 iPodCoreiPodCtrlGetTimer(S32 fd, U32 *timems)
{
    S32 rc = IPOD_PLAYER_ERROR;
    struct itimerspec val;
    
    /* Paramter check */
    if(timems == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, timems);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&val, 0, sizeof(val));
    
    /* Get the remaing time */
    rc = timerfd_gettime(fd, &val);
    if(rc != -1)
    {
        *timems = val.it_value.tv_sec * IPOD_PLAYER_TIME_MSEC_TO_SEC;
        *timems += (val.it_value.tv_nsec / IPOD_PLAYER_TIME_MSEC_TO_NSEC);
    }
    else
    {
        S32 errID = errno;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, errID);
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;

}

S32 iPodCoreiPodCtrlRemoteEventNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* set repeat mode to internal ipod info */
        if(contents->intRemoteEvent.repeatStatus != 0xff)
        {
            iPodCtrlCfg->iPodInfo->repeatStatus = contents->intRemoteEvent.repeatStatus;
        }
        
       /* set shuffle mode to internal ipod info */
        if(contents->intRemoteEvent.shuffleStatus != 0xff)
        {
           iPodCtrlCfg->iPodInfo->shuffleStatus = contents->intRemoteEvent.shuffleStatus;
        }

        rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
        if(rc != IPOD_PLAYER_OK)
        {
            IPOD_DLT_ERROR("Could not notify playback status (return = %d)", rc);
        }
    }

    /* Set the result command size */
    rc = IPOD_PLAYER_ERR_NO_REPLY;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlNotifyStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    IPOD_PLAYER_PARAM_GET_COVERART coverart;
    U32 i = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&coverart, 0, sizeof(coverart));
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
    
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;

        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, contents->intNotifyStatus.status);
        switch(contents->intNotifyStatus.status)
        {
            case IPOD_STATUS_PLAYBACK_STOPPED:
                /* Change iPod status to STOP*/
                if((iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_FF) ||
                   (iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_RW))
                {
                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                }
                iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_STOP;
                iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_STOP;
                /* Status check whether iPodStatus match with playbackstatus or not */
                rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_STATUS_PLAYBACK_STOPPED, rc);
                break;
                
            case IPOD_STATUS_TRACK_CHANGED:
                /* trackIndex changes to new index */
                iPodCtrlCfg->playbackStatus.track.index = (U32)contents->intNotifyStatus.param;
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_STATUS_TRACK_CHANGED, iPodCtrlCfg->playbackStatus.track.index);
                iPodCtrlCfg->iPodInfo->curCoverart = 0;
                //iPodCtrlCfg->playbackStatus.track.time = 0;
                
                /* End must be send to Apple Device if current playback status is FF or FB. */
                if((iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_FF) ||
                   (iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_RW))
                {
                    /* current playback status chagens to previous status. previous status is only play or pause */
                    iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_STATUS_TRACK_CHANGED, iPodCtrlCfg->playbackStatus.status);
                    /* Remove previous status */
                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                    /* Send EF to iPodCtrl Handler */
                    rc = iPodCoreiPodCtrlEndForward(devID);
                }
                
                if(rc == IPOD_PLAYER_OK)
                {
                    rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
                }
                
                iPodCoreiPodCtrlRequestCurrentTrackInfo(iPodCtrlCfg, devID, iPodCtrlCfg->playbackStatus.track.index);
                /* Clear the timer event because track was changed */
                iPodCtrlCfg->threadInfo->prevPlayTime = 0;
                
                break;
                
            case IPOD_STATUS_FWD_SEEK_STOP:
                if(iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_FF)
                {
                    if(iPodCtrlCfg->iPodInfo->prevStatus != IPOD_PLAYER_PLAY_STATUS_UNKNOWN)
                    {
                        iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
                    }
                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                    iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_STATUS_FWD_SEEK_STOP, iPodCtrlCfg->playbackStatus.status);
                }
                
                break;
                
            case IPOD_STATUS_BWD_SEEK_STOP:
                if(iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_RW)
                {
                    if(iPodCtrlCfg->iPodInfo->prevStatus != IPOD_PLAYER_PLAY_STATUS_UNKNOWN)
                    {
                        iPodCtrlCfg->playbackStatus.status = iPodCtrlCfg->iPodInfo->prevStatus;
                    }
                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                    iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_STATUS_BWD_SEEK_STOP, iPodCtrlCfg->playbackStatus.status);
                }
                
                
                /* Nothing to do */
                break;
                
            case IPOD_STATUS_TRACK_POSITION:
                
                /* Copy the notfied time */
                iPodCtrlCfg->playbackStatus.track.time = contents->intNotifyStatus.param;
                /* Set the timer only when status is play */
                if((iPodCtrlCfg->iPodInfo->trackInfoMask & IPOD_PLAYER_TRACK_INFO_MASK_COVERART) == IPOD_PLAYER_TRACK_INFO_MASK_COVERART)
                {
                    if((iPodCtrlCfg->iPodInfo->coverartCount > 0) && (iPodCtrlCfg->iPodInfo->coverartCount > iPodCtrlCfg->iPodInfo->curCoverart))
                    {
                        if(iPodCtrlCfg->iPodInfo->coverartCount > 1)
                        {
                            for(i = iPodCtrlCfg->iPodInfo->curCoverart; i < iPodCtrlCfg->iPodInfo->coverartCount - 1; i++)
                            {
                                if((iPodCtrlCfg->iPodInfo->coverartTime[i] < iPodCtrlCfg->playbackStatus.track.time) && (iPodCtrlCfg->iPodInfo->coverartTime[i + 1] >= iPodCtrlCfg->playbackStatus.track.time))
                                {
                                    coverart.header.funcId = IPOD_FUNC_GET_COVERART;
                                    coverart.header.devID = iPodCtrlCfg->threadInfo->iPodDevID;
                                    coverart.trackIndex = iPodCtrlCfg->playbackStatus.track.index;
                                    coverart.formatId = iPodCtrlCfg->iPodInfo->formatId;
                                    coverart.time = iPodCtrlCfg->iPodInfo->coverartTime[i];
                                    
                                    rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&coverart, sizeof(coverart), 0, IPODCORE_TMOUT_FOREVER);
                                    if(rc == sizeof(coverart))
                                    {
                                        rc = IPOD_PLAYER_OK;
                                    }
                                    else
                                    {
                                        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc, sizeof(coverart));
                                        rc = IPOD_PLAYER_ERROR;
                                    }
                                    
                                    iPodCtrlCfg->iPodInfo->curCoverart = i + 1;

                                    break;
                                }
                            }
                        }
                        else
                        {
                            coverart.header.funcId = IPOD_FUNC_GET_COVERART;
                            coverart.header.devID = iPodCtrlCfg->threadInfo->iPodDevID;
                            coverart.trackIndex = iPodCtrlCfg->playbackStatus.track.index;
                            coverart.formatId = iPodCtrlCfg->iPodInfo->formatId;
                            coverart.time = iPodCtrlCfg->iPodInfo->coverartTime[i];
                            
                            rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&coverart, sizeof(coverart), 0, IPODCORE_TMOUT_FOREVER);
                            if(rc == sizeof(coverart))
                            {
                                rc = IPOD_PLAYER_OK;
                            }
                            else
                            {
                                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rc, sizeof(coverart));
                                rc = IPOD_PLAYER_ERROR;
                            }
                            iPodCtrlCfg->iPodInfo->curCoverart = i + 1;
                        }
                    }
                }
                
                if(iPodCtrlCfg->playbackStatus.track.index == IPOD_PLAYER_TRACK_INDEX_UNKNOWN)
                {
                    iPodCoreiPodCtrlRequestCurrentTrackInfo(iPodCtrlCfg, devID, IPOD_PLAYER_TRACK_INDEX_UNKNOWN);
                }
                
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPOD_STATUS_TRACK_POSITION, contents->intNotifyStatus.param);
                break;
                
            case IPOD_STATUS_CHAPTER_CHANGED:
                iPodCtrlCfg->playbackStatus.chapter.index = contents->intNotifyStatus.param;
                break;
                
            case IPOD_STATUS_PLAYBACK_EXTENDED:
                
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, contents->intNotifyStatus.param);
                switch(contents->intNotifyStatus.param)
                {
                    case 0x02:
                        /* Change iPod status to STOP */
                        if((iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_FF) ||
                           (iPodCtrlCfg->iPodInfo->status == IPOD_PLAYER_PLAY_STATUS_RW))
                        {
                            /* If status is changed to Stop, previous stauts must be cleared */
                            /* Because Stop status must not change previous status */
                            iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                        }
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_STOP;
                        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_STOP;
                        rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                        break;
                        
                    case 0x05:
                        /* Change iPod status to FF */
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_FF;
                        rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                        break;
                        
                    case 0x06:
                        /* Change iPod status to FB */
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_RW;
                        rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                        break;
                        
                    //case 0x07:
                        /* Nothing to do */
                       // break;
                        
                    case 0x0A:
                        /* Change iPod status to PLAY */
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_PLAY;
                        rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                        break;
                        
                    case 0x0B:
                        /* Chane iPod status to PAUSE */
                        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_PAUSE;
                        rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                        break;
                        
                    default : 
                        rc = IPOD_PLAYER_ERROR;
                        break;
                }
                
                break;
                
            case IPOD_STATUS_TRACK_TIME_OFFSET:
                break;
            case IPOD_STATUS_CHAPTER_MS_OFFSET:
                iPodCtrlCfg->playbackStatus.chapter.time = contents->intNotifyStatus.param;
                break;
            case IPOD_STATUS_CHAPTER_SEC_OFFSET:
                iPodCtrlCfg->playbackStatus.chapter.time = contents->intNotifyStatus.param;
                break;
            case IPOD_STATUS_TRACK_UID:
            case IPOD_STATUS_TRACK_PLAYBACK_MODE:
            case IPOD_STATUS_TRACK_LYRICS_READY:
                /* tdb */
                break;

            default : break;
        }
        if(rc == IPOD_PLAYER_OK)
        {
            rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
        }
    }
    /* Set the result command size */
    rc = IPOD_PLAYER_ERR_NO_REPLY;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlIntGetStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 devID = 0;
    U32 curTime = 0;
    U8 status = 0;
    U32 pos = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, waitData, header, contents, size);
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, waitData, contents, size);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Apple device is connected and finish the authentication. Apple device is extend mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_REMOTE_CONTROL))
    {
        /* Set the ID of iPodID of ipod ctrl */
        devID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, devID, waitData->status);
        switch(waitData->status)
        {
            case IPODCORE_QUEUE_STATUS_WAIT:
            case IPODCORE_QUEUE_STATUS_RUNNING:
                curTime = iPodGetTime();
                /* Current time is bigger than last time */
                if(curTime >= iPodCtrlCfg->lastGetTime)
                {
                    /* Current time elapses more than interval time from last time */
                    if((curTime - iPodCtrlCfg->lastGetTime) >= IPODCORE_GET_STATUS_INTERVAL)
                    {
                        /* Get the playback status */
                        rc = iPodCoreFuncIntGetStatus(devID, &status, &pos);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            /* Update the last time */
                            iPodCtrlCfg->lastGetTime = iPodGetTime();
                            /* Current status is Stop */
                            if(status == IPOD_PLAYER_PLAY_STATUS_STOP)
                            {
                                /* If previous status is FF or RW, it must call End FF/RW. */
                                if((iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_FF) ||
                                   (iPodCtrlCfg->playbackStatus.status == IPOD_PLAYER_PLAY_STATUS_RW))
                                {
                                    iPodCtrlCfg->iPodInfo->prevStatus = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
                                }
                                /* Update the iPod status to Stop */
                                if(iPodCtrlCfg->iPodInfo->status != IPOD_PLAYER_PLAY_STATUS_STOP)
                                {
                                    iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_STOP;
                                    iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_STOP;
                                    rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
                                }
                                else
                                {
                                    /* Update the iPodPlayer status to Stop */
                                    iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_STOP;
                                }
                            }
                            /* Current status is playing or pause */
                            else
                            {
                                /* Previous status is not FF/RW. 
                                 * FF/RW is never informed because GetPlayStatus informs only Play, Pause or Stop */
                                if((iPodCtrlCfg->iPodInfo->status != IPOD_PLAYER_PLAY_STATUS_FF) &&
                                   (iPodCtrlCfg->iPodInfo->status != IPOD_PLAYER_PLAY_STATUS_RW))
                                {
                                    /* Status is different from before */
                                    if(iPodCtrlCfg->iPodInfo->status != (IPOD_PLAYER_PLAY_STATUS)status)
                                    {
                                        /* Update status of iPod */
                                        iPodCtrlCfg->iPodInfo->status = (IPOD_PLAYER_PLAY_STATUS)status;
                                        /* Notify the current status */
                                        rc = iPodCoreiPodCtrlNotifyPlaybackStatus(iPodCtrlCfg, contents);
                                    }
                                    
                                    if(rc == IPOD_PLAYER_OK)
                                    {
                                        /* Check the stutus of iPodPlayer and iPod */
                                        rc = iPodCoreiPodCtrlStatusCheck(iPodCtrlCfg, contents, *size);
                                    }
                                }
                            }
                        }
                        
                    }
                }
                else
                {
                    /* Time was overflow. Just update the current time */
                    iPodCtrlCfg->lastGetTime = iPodGetTime();
                }
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            
                break;
                
            default:
                break;
        }
    }
    else
    {
        waitData->status = IPODCORE_QUEUE_STATUS_FINISH;
        rc  = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlWaitInit(IPOD_PLAYER_CORE_IPOD_DEVICE_INFO *iPodInfo)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 curTime = 0;
    pthread_condattr_t cond_attr;
    clockid_t clock_id = CLOCK_MONOTONIC;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodInfo);
    
    /* Parameter check */
    if(iPodInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, iPodInfo);
        return IPOD_PLAYER_ERROR;
    }
    
    /* Create mutex */
    rc = pthread_mutex_init(&iPodInfo->waitMutex, NULL);
    if(rc == 0)
    {
        /* initialize the condition attribute*/
        pthread_condattr_init(&cond_attr);
        /* Change the clock to CLOCK_MONOTONIC from CLOCK_REALTIME */
        pthread_condattr_setclock(&cond_attr, clock_id);
        /* Create the condition */
        rc = pthread_cond_init(&iPodInfo->waitCond, &cond_attr);
        if(rc == 0)
        {
            /* Get the time */
            curTime = iPodGetTime();
            iPodInfo->getDBprevTime = curTime;
            iPodInfo->getTrackTime = curTime;
            iPodInfo->getAlbumTime = curTime;
            iPodInfo->getArtistTime = curTime;
            iPodInfo->getInfoTime = curTime;
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            pthread_mutex_destroy(&iPodInfo->waitMutex);
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        
        pthread_condattr_destroy(&cond_attr);
    }
    else
    {
        S32 errID = errno;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, errID);
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

void iPodCoreiPodCtrlWaitDeinit(IPOD_PLAYER_CORE_IPOD_DEVICE_INFO *iPodInfo)
{
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodInfo);
    
    /* Parameter check */
    if(iPodInfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, iPodInfo);
        return;
    }
    
    
    /* Remove Mutex and Condition */
    iPodInfo->getDBprevTime = 0;
    iPodInfo->getTrackTime = 0;
    iPodInfo->getAlbumTime = 0;
    iPodInfo->getArtistTime = 0;
    iPodInfo->getInfoTime = 0;
    pthread_cond_destroy(&iPodInfo->waitCond);
    pthread_mutex_destroy(&iPodInfo->waitMutex);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE);
}

S32 iPodCoreiPodCtrlTimedWait(S32 fd, U32 *prevTime)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 curTime = 0;
    U32 waitTime = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, fd, prevTime);
    
    /* Paramter check */
    if(prevTime == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, prevTime);
        return IPOD_PLAYER_ERROR;
    }
    
    /* Get the current iPodPlayer internal time */
    curTime = iPodGetTime();
    /* 1 sec is not elapsed during current time and previous call time */
    if((curTime >= *prevTime) && ((curTime - *prevTime) < IPOD_PLAYER_ATS_WAIT_TIME))
    {
        /* Set the time which should wait */
        waitTime = IPOD_PLAYER_ATS_WAIT_TIME - (curTime - *prevTime);
        rc = iPodCoreiPodCtrlSetTimer(fd, waitTime, 0);
    }
    else
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

#if 0
S32 iPodCoreiPodCtrlTimedWait(IPOD_PLAYER_CORE_IPOD_DEVICE_INFO *iPodInfo, U32 *prevTime)
{
    S32 rc = 0;
    U32 count = 0;
    U32 curTime = 0;
    struct itimerspec timeout;
    U32 waitTime = 0;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodInfo, prevTime);
    
    /* Paramter check */
    if((iPodInfo == NULL) || (prevTime == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, iPodInfo, prevTime);
        return IPOD_PLAYER_ERROR;
    }
    
    memset(&timeout, 0, sizeof(timeout));
    
    /* Add the mutex unlock to the clanup */
    pthread_cleanup_push((void *)pthread_mutex_unlock, (void *)&iPodInfo->waitMutex);
    pthread_mutex_lock(&iPodInfo->waitMutex);
    
    for(count = 0; count < IPOD_PLAYER_WAIT_TIME_MAX_COUNT; count++)
    {
        /* Get the current time */
        rc = clock_gettime(CLOCK_MONOTONIC, &timeout);
        if(rc == 0)
        {
            /* Get the current iPodPlayer internal time */
            curTime = iPodGetTime();
            
            /* 1 sec is not elapsed during current time and previous call time */
            if((curTime >= *prevTime) && ((curTime - *prevTime) < IPOD_PLAYER_ATS_WAIT_TIME))
            {
                /* Set the time which should wait */
                waitTime = IPOD_PLAYER_ATS_WAIT_TIME - (curTime - *prevTime);
                timeout.tv_sec += waitTime / IPOD_PLAYER_TIME_MSEC;
                timeout.tv_nsec += (waitTime % IPOD_PLAYER_TIME_MSEC) * IPOD_PLAYER_TIME_NSEC;
                /* nsec exceed 1 sec*/
                if(timeout.tv_nsec > IPOD_PLAYER_TIME_NSEC_TO_SEC)
                {
                    timeout.tv_sec += IPOD_PLAYER_TIME_SEC;
                    timeout.tv_nsec -= IPOD_PLAYER_TIME_NSEC_TO_SEC;
                }
                
                rc = pthread_cond_timedwait(&iPodInfo->waitCond, &iPodInfo->waitMutex, &timeout);
                if(rc == ETIMEDOUT)
                {
                    waitTime = 0;
                    break;
                }
            }
            else
            {
                break;
            }
        }
        else
    {
        S32 errID = errno;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, errID);
            rc = IPOD_PLAYER_ERROR;
            break;
        }
    }
    
    pthread_mutex_unlock(&iPodInfo->waitMutex);
    pthread_cleanup_pop(0);
    
    if(waitTime == 0)
    {
        /* Update the current calling time to previous calling time */
        *prevTime = iPodGetTime();
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}
#endif

S32 iPodCoreiPodCtrlGetDBNumber(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 devID, IPOD_PLAYER_DB_TYPE type, U32 *totalNum)
{
    S32 rc = IPOD_PLAYER_OK;
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, devID, type, totalNum);
    
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
     /* Check the time  which elapsed from last call for ATS. If elapsed time does not exceed 1 sec, it must wait for 1 sec. */
    iPodCoreiPodCtrlTimedWait(iPodCtrlCfg->iPodInfo->getDBprevTimeFd, &iPodCtrlCfg->iPodInfo->getDBprevTime);
    /* Request to get the total track count of indicated category */
    rc = iPodCoreFuncGetDBCount(devID, type, totalNum);
    if(rc == IPOD_PLAYER_OK)
    {
        iPodCtrlCfg->iPodInfo->curTotalType = type;
        iPodCtrlCfg->iPodInfo->curTotalCount = *totalNum;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiPodCtrlHMIGetSupportedFeature(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                           IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 iPodID = 0;
    IPOD_PLAYER_HMI_FEATURE_TYPE types = IPOD_PLAYER_HMI_FEATURE_TYPE_ALL;
    U32 optionsBits = 0;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->threadInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    /* For lint */
    header = header;
    
    /* Check the Apple device status and mode .
     * Status and mode must be connect, authentication success and HMI control mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        iPodID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        types = waitData->contents.hmiGetSupportedFeature.type;
        
        rc = iPodCoreFuncGetiPodOutOptions(iPodID, (U32)types, &optionsBits);
        if(rc == IPOD_PLAYER_OK)
        {
            waitData->contents.hmiGetSupportedFeatureResult.optionsBits = optionsBits;
            waitData->contents.hmiGetSupportedFeatureResult.type = types;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_HMI_GET_SUPPORTED_FEATURE_RESULT);
    
    return rc;
}

S32 iPodCoreiPodCtrlHMISetSupportedFeature(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                           IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 iPodID = 0;
    U32 optionsBits = 0;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->threadInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* For lint */
    header = header;
    
    /* Check the Apple device status and mode .
     * Status and mode must be connect, authentication success and HMI control mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        iPodID = iPodCtrlCfg->threadInfo->iPodDevID;
        optionsBits = waitData->contents.hmiSetSupportedFeature.bitmask;
        rc = iPodCoreFuncSetiPodOutOptions(iPodID, optionsBits);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_HMI_SET_SUPPORTED_FEATURE_RESULT);
    
    return rc;
}

S32 iPodCoreiPodCtrlHMISetApplicationStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                            IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 iPodID = 0;
    U8 status = 0;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->threadInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* For lint */
    header = header;
    
    /* Check the Apple device status and mode .
     * Status and mode must be connect, authentication success and HMI control mode */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        iPodID = iPodCtrlCfg->threadInfo->iPodDevID;
        status = waitData->contents.hmiSetApplicationStatus.status;
        rc = iPodCoreFuncDevStateChangeEvent(iPodID,status);
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_HMI_SET_SUPPORTED_FEATURE_RESULT);
    
    return rc;
}

S32 iPodCoreiPodCtrlHMIButtonStatusSend(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                        IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 iPodID = 0;
    U32 eventsval=0;
    U8 sourceval = 0;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* For lint */
    header = header;
    
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        if(iPodCtrlCfg->threadInfo == NULL)
        {
            return IPOD_PLAYER_ERROR;
        }
        
        iPodID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        switch(waitData->status)
        {
        case IPODCORE_QUEUE_STATUS_WAIT:
            /* Clear the Button info */
            waitData->contents.hmiButtonInput.source = IPOD_PLAYER_HMI_BUTTON_SOURCE_CONSOLE;
            waitData->contents.hmiButtonInput.event = 0;
            rc = IPOD_PLAYER_ERR_BACK_TO_WAIT;
            break;
        case IPODCORE_QUEUE_STATUS_RUNNING:
            /* Input data is request of from Application or timer event */
            if((contents->hmiButtonInput.header.funcId == IPOD_FUNC_HMI_BUTTON_INPUT) ||
               ((U32)contents->hmiButtonInput.header.funcId == (IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND)))
            {
                /* Update the button info */
                if(contents->hmiButtonInput.header.funcId == IPOD_FUNC_HMI_BUTTON_INPUT)
                {
                    waitData->contents.hmiButtonInput.source = contents->hmiButtonInput.source;
                    waitData->contents.hmiButtonInput.event = contents->hmiButtonInput.event;
                }
                
                sourceval = waitData->contents.hmiButtonInput.source;
                eventsval = waitData->contents.hmiButtonInput.event;
                
                rc = iPodCoreFuncOutButtonStatus(iPodID, sourceval, eventsval);
                
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Action release means user release the button input */
                    if(waitData->contents.hmiButtonInput.event == IPOD_PLAYER_HMI_ROTATION_ACTION_RELEASE)
                    {
                        /* Moving the button input device is stopped */
                        rc = IPOD_PLAYER_ERR_BACK_TO_WAIT;
                    }
                    else
                    {
                        waitData->nextRequest = 1;
                        /* Set the timer event */
                        rc = iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_HMI_WAIT_STATUS, 0);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        }
                    }
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            }
            
            break;
        default:
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            break;
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    return rc;
}

/* This function updates the queue status of asynchronous function of HMI control */
static S32 iPodCoreiPodCtrlHMIButtonStatusUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 event, IPOD_PLAYER_HMI_BUTTON_SOURCE source)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(source > IPOD_PLAYER_HMI_BUTTON_SOURCE_DASHBOARD)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    for(i = 0; i < IPODCORE_WAIT_LIST_MAX; i++)
    {
        /* internal queue for hmi control is set in this queue */
        if(iPodCtrlCfg->waitList[i].contents.paramTemp.header.funcId == IPOD_FUNC_INTERNAL_HMI_BUTTON_STATUS_SEND)
        {
            rc = IPOD_PLAYER_OK;
            break;
        }
    }
    
    /* Found out the internal queue */
    if(rc < IPODCORE_WAIT_LIST_MAX)
    {
        switch(event)
        {
        case IPOD_PLAYER_HMI_BUTTON_EVENT_RELEASE:
        case IPOD_PLAYER_HMI_BUTTON_EVENT_UP:
        case IPOD_PLAYER_HMI_BUTTON_EVENT_DOWN:
        case IPOD_PLAYER_HMI_BUTTON_EVENT_RIGHT:
        case IPOD_PLAYER_HMI_BUTTON_EVENT_LEFT:
        case IPOD_PLAYER_HMI_BUTTON_EVENT_SELECT:
        case IPOD_PLAYER_HMI_BUTTON_EVENT_MENU:
            /* If current status is wait, status changes to running */
            if((iPodCtrlCfg->waitList[i].status == IPODCORE_QUEUE_STATUS_WAIT) ||
               (iPodCtrlCfg->waitList[i].status == IPODCORE_QUEUE_STATUS_RUNNING))
            {
                iPodCtrlCfg->waitList[i].status = IPODCORE_QUEUE_STATUS_RUNNING;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            }
            break;
        default:
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            break;
        }
    }
    
    return rc;
}

S32 iPodCoreiPodCtrlHMIButtonInput(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                   IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->threadInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* For lint */
    header = header;
    
    /* Check the status of connected device */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        /* Update the queue status */
        rc = iPodCoreiPodCtrlHMIButtonStatusUpdate(iPodCtrlCfg, waitData->contents.hmiButtonInput.event, waitData->contents.hmiButtonInput.source);
    }
    else
    {
        /* Current status sets to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_HMI_BUTTON_INPUT_RESULT);
    
    return rc;
}


/* Update the rotation device */
static S32 iPodCoreiPodCtrlRotationUpdate(const IPOD_PLAYER_HMI_ROTATION_INFO *rotationIn,
                                          IPOD_PLAYER_HMI_ROTATION_INFO *rotationOut)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if((rotationIn == NULL) || (rotationOut == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Update the these members */
    rotationOut->device = rotationIn->device;
    rotationOut->direction = rotationIn->direction;
    rotationOut->action = rotationIn->action;
    rotationOut->type= rotationIn->type;
    rotationOut->max = rotationIn->max;
    rc = IPOD_PLAYER_OK;
    
    return rc;
    
}

/* This is internal function for asynchronous functions */
S32 iPodCoreiPodCtrlHMIStatusSend(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                         IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 iPodID = 0;
    U32 duration = 0;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->threadInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* For lint */
    header = header;
    
    /* Check the connected Apple device's status. */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        
       iPodID = iPodCtrlCfg->threadInfo->iPodDevID;
        
        switch(waitData->status)
        {
        case IPODCORE_QUEUE_STATUS_WAIT:
            /* Clear the device info */
            memset(&waitData->contents.hmiRotationInput.info, 0, sizeof(waitData->contents.hmiRotationInput.info));
            waitData->contents.hmiRotationInput.move = 0;
            waitData->contents.hmiRotationInput.duration = 0;
            rc = IPOD_PLAYER_ERR_BACK_TO_WAIT;
            break;
            
        case IPODCORE_QUEUE_STATUS_RUNNING:
            /* Input data is request of from Application or timer event */
            if((contents->hmiRotationInput.header.funcId == IPOD_FUNC_HMI_ROTATION_INPUT) ||
               ((U32)contents->hmiRotationInput.header.funcId ==  (IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_INTERNAL_HMI_STATUS_SEND)))
            {
                /* Input data is request of from Application */
                if(contents->hmiRotationInput.header.funcId == IPOD_FUNC_HMI_ROTATION_INPUT)
                {
                    /* Update the rotation info */
                    rc = iPodCoreiPodCtrlRotationUpdate(&contents->hmiRotationInput.info, &waitData->contents.hmiRotationInput.info);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        waitData->contents.hmiRotationInput.move = contents->hmiRotationInput.move;
                    }
                }
                
                /* Duration 0 means rotation device start to move */
                if(waitData->contents.hmiRotationInput.duration == 0)
                {
                    /* Duration is 0 */
                    duration = 0;
                }
                else
                {
                    duration = iPodGetTime();
                    if(duration > waitData->contents.hmiRotationInput.duration)
                    {
                        /* duration is the elapsed time since previous call */
                        duration = duration - waitData->contents.hmiRotationInput.duration;
                    }
                }
                rc = iPodCoreFuncRotationInputStatus(iPodID, duration, waitData->contents.hmiRotationInput.info,
                                                     waitData->contents.hmiRotationInput.move);
                if(rc == IPOD_PLAYER_OK)
                {
                    /* Action release means user release the rotation device */
                    if(waitData->contents.hmiRotationInput.info.action == IPOD_PLAYER_HMI_ROTATION_ACTION_RELEASE)
                    {
                        rc = IPOD_PLAYER_ERR_BACK_TO_WAIT;
                    }
                    else
                    {
                        /* current duration is updated */
                        waitData->contents.hmiRotationInput.duration = iPodGetTime();
                        waitData->nextRequest = 1;
                        /* Set the timer event */
                        rc = iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_HMI_WAIT_STATUS, 0);
                        if(rc == IPOD_PLAYER_OK)
                        {
                            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        }
                    }
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            }
            break;
            
        default:
            rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            break;
            
        }
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    return rc;
}


/* This function updates the queue status of asynchronous function of HMI control */
static S32 iPodCoreiPodCtrlHMIStatusUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_HMI_ROTATION_ACTION action)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    for(i = 0; i < IPODCORE_WAIT_LIST_MAX; i++)
    {
        /* internal queue for hmi control is set in this queue */
        if(iPodCtrlCfg->waitList[i].contents.paramTemp.header.funcId == IPOD_FUNC_INTERNAL_HMI_STATUS_SEND)
        {
            rc = IPOD_PLAYER_OK;
            break;
        }
    }
    
    /* Found out the internal queue */
    if(rc < IPODCORE_WAIT_LIST_MAX)
    {
        switch(action)
        {
        case IPOD_PLAYER_HMI_ROTATION_ACTION_RELEASE:
        case IPOD_PLAYER_HMI_ROTATION_ACTION_PROGRESS:
        case IPOD_PLAYER_HMI_ROTATION_ACTION_NOTHING:
            /* If current status is wait, status changes to running */
            if((iPodCtrlCfg->waitList[i].status == IPODCORE_QUEUE_STATUS_WAIT) ||
               (iPodCtrlCfg->waitList[i].status == IPODCORE_QUEUE_STATUS_RUNNING))
            {
                iPodCtrlCfg->waitList[i].status = IPODCORE_QUEUE_STATUS_RUNNING;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERR_INVALID_STATUS;
            }
            break;
        default:
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            break;
        }
    }
    
    return rc;
}


S32 iPodCoreiPodCtrlHMIRotationInput(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData,
                                     IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Check Parameter */
    if((iPodCtrlCfg == NULL) || (waitData == NULL) || (contents == NULL) || (size == NULL))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(iPodCtrlCfg->threadInfo == NULL)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* For lint */
    header = header;
    
    /* Check the status of connected device */
    if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
       (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS) &&
       (iPodCtrlCfg->iPodInfo->mode == IPOD_PLAYER_MODE_HMI_CONTROL))
    {
        if((waitData->contents.hmiRotationInput.info.device <= IPOD_PLAYER_HMI_ROTATION_DEVICE_FREE) &&
           (waitData->contents.hmiRotationInput.info.direction <= IPOD_PLAYER_HMI_ROTATION_DIRECTION_LEFT) &&
           (waitData->contents.hmiRotationInput.info.action <= IPOD_PLAYER_HMI_ROTATION_ACTION_NOTHING) &&
           (waitData->contents.hmiRotationInput.info.type <= IPOD_PLAYER_HMI_ROTATION_TYPE_DEGREE) &&
           (waitData->contents.hmiRotationInput.info.max <= 360))
        {
            /* Update the queue status */
            rc = iPodCoreiPodCtrlHMIStatusUpdate(iPodCtrlCfg, waitData->contents.hmiRotationInput.info.action);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
    }
    else
    {
        /* Current status sets to unknown */
        iPodCtrlCfg->playbackStatus.status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        iPodCtrlCfg->iPodInfo->status = IPOD_PLAYER_PLAY_STATUS_UNKNOWN;
        rc = IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    /* Set the result command size */
    *size = sizeof(IPOD_CB_PARAM_HMI_ROTATION_INPUT_RESULT);
    
    return rc;
}

