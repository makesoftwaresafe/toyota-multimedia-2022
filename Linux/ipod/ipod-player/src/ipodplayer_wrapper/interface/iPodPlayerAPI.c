/**
 * \file iPodPlayerAPI.c
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize
 * \date 19/Mar/2012 iPodPlayerInit changes the parameter.<br>
 *                   Added the new API.(#iPodPlayerSetiOSAppsInfo, #iPodPlayerStartAuthentication)
 */

#include "iPodPlayerLocal.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerAPI.h"
#include "iPodPlayerDebug.h"
#include "iPodPlayerThread.h"
#include "iPodPlayerDeviceDetection.h"
#include "iPodPlayerData.h"

#define IPOD_PLAYER_SEM_NAME "/tmp_ippif_sem"

/**
 * \addtogroup Init Initialize
 * This group is function of Initialize/Deinitialize of iPodPlayer.<br>
 * Application can call the API of this group any time.<br>
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerInit </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerDeinit </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSelectAudioOut </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerTestReady </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetiOSAppsInfo </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerStartAuthentication </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 *
 * </TR>
 * </TABLE>
 */

/*\{*/

/*!
 * \fn iPodPlayerInit(U32 connectionMask, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable)
 * This function initializes the iPodPlayer. Application can use all API after calling this function.<br>
 * Almost all API must receive the result by callback. Also some notification is received by callback.<br>
 * These callback functions can be registered by this function with cbTable.<br>
 * \param [in] connectionMask Type of this value is U32. This is a mask which decides the using device. Usable mask is below.<br>
 * \li \c <b> #IPOD_PLAYER_USE_DEVICE_USB - Application uses USB. </b>
 * \li \c <b> #IPOD_PLAYER_USE_DEVICE_BT - Application uses Bluetooth. </b>
 * \li \c <b> #IPOD_PLAYER_USE_DEVICE_UART - Application uses UART. </b>
 * \param [in] *cbTable Type of this value is #IPOD_PLAYER_REGISTER_CB_TABLE pointer. This is a table which register the callback function.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERR_INVALID_PARAMETER Invaid parameter is used
 * \retval #IPOD_PLAYER_ERR_NOMEM Memory allocation failed
 * \retval #IPOD_PLAYER_ERR_MAX_APP_CONNECT Connected application exceeded the maximum connection
 * \retval #IPOD_PLAYER_ERROR Other error
 * \warning
 * If application calls this function and receive the function success , iPodPlayer will notify the current connection status to Application automatically.
 * (Using #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS)<br>
 */
S32 iPodPlayerInit(U32 connectionMask, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 sckHandle = -1;
    S32 waitHandle = -1;
    S32 intWaitHandle = -1;
    S32 recvSockParam = -1;
    S32 recvQueueParam = -1;
    S32 sendQueueParam = -1;
    S32 recvQueueParamLocal = -1;
    S32 sendQueueParamLocal = -1;
    S32 longRecvSockParam = -1;
    S32 longSendSockParam = -1;
    IPOD_PLAYER_PARAM_INIT param;
    IPOD_PLAYER_PARAM_INIT_RESULT result;
    U8 uniqueName[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    U8 longUniqueName[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    IPOD_PLAYER_IPC_OPEN_INFO info;
    pid_t pid = getpid();
    U8 semTemp[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    U32 length = 0;
    
    if(cbTable == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(connectionMask > (IPOD_PLAYER_USE_DEVICE_USB + IPOD_PLAYER_USE_DEVICE_BT + IPOD_PLAYER_USE_DEVICE_UART))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    memset(&result, 0, sizeof(result));
    memset(&info, 0, sizeof(info));
    
//    snprintf((char *)uniqueName, sizeof(uniqueName), "%d", pid);
    snprintf((char *)uniqueName, sizeof(uniqueName), "AppServ%d", pid);
    snprintf((char *)longUniqueName, sizeof(longUniqueName), "AppServ%d_long", pid);
    /* init socket */
    //iPodPlayerInitSocketInfo();
    
    rc = iPodPlayerIPCInit();
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        info.type = IPOD_PLAYER_OPEN_WAIT_HANDLE;
    info.prefix = NULL;
    info.identify = NULL;
    info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
    info.semName = NULL;
    info.maxBufSize = 0;
    rc = iPodPlayerIPCOpen(&waitHandle, &info);
    }
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = longUniqueName;
        info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
        info.semName = NULL;
        info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
        info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
        /* create socket(client program) */
        rc = iPodPlayerIPCOpen(&longRecvSockParam, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
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
    
    if(rc == IPOD_PLAYER_OK)
    {
        snprintf((char *)semTemp, sizeof(semTemp), "/tmp_sem%d", pid);
        info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = (U8 *)IPOD_PLAYER_CORE_SOCKET_LONG_NAME;
        info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
        info.semName = semTemp;
        info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
        info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
        /* create socket(client program) */
        rc = iPodPlayerIPCOpen(&longSendSockParam, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
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
    
    if(rc == IPOD_PLAYER_OK)
    {
        info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
        info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
        info.identify = (U8 *)IPOD_PLAYER_CORE_SOCKET_NAME;
        info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
        info.semName = (U8 *)IPOD_PLAYER_SEM_NAME;
        info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
        info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
        /* create socket(client program) */
        rc = iPodPlayerIPCOpen(&sckHandle, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Send the data to the iPodPlayer Core */
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER;
        info.identify = (U8 *)uniqueName;
        info.semName = NULL;
        info.connectionNum = 1;
        /* create socket(server program) */
        rc = iPodPlayerIPCOpen(&recvSockParam, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* set the value to data*/
            length = sizeof(param.uniqueName);
            strncpy((char *)param.uniqueName, (const char *)uniqueName, length);
            param.uniqueName[length - 1] = '\0';
            param.connectionMask = connectionMask;
            param.header.funcId = IPOD_FUNC_INIT;
            param.header.appID = pid;
          
            /* send command */
            rc = iPodPlayerIPCSend(sckHandle, (U8 *)&param, sizeof(IPOD_PLAYER_PARAM_INIT), 0, -1);
                    
            if(rc == sizeof(IPOD_PLAYER_PARAM_INIT))
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
        
        /* Wait for a data from the iPodPlayerCore */
        if(rc == IPOD_PLAYER_OK)
        {
            /* wait for receiving a socket data */
            rc = iPodPlayerIPCReceive(recvSockParam, (U8*)&result, sizeof(IPOD_PLAYER_PARAM_INIT_RESULT), 1, waitHandle, -1);
            if(rc == sizeof(IPOD_PLAYER_PARAM_INIT_RESULT))
            {
                rc = result.result;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_WAIT_HANDLE;
            info.prefix = (U8 *)"/IPP_QUEUE";
            info.identify = (U8 *)"intQueue";
            info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
            info.semName = NULL;
            info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
            info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
            /* create socket(server program) */
            rc = iPodPlayerIPCOpen(&intWaitHandle, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_QUEUE_SERVER;
            info.prefix = (U8 *)"/IPP_QUEUE";
            info.identify = (U8 *)"intQueue";
            info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
            info.semName = NULL;
            info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
            info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
            /* create socket(server program) */
            rc = iPodPlayerIPCOpen(&recvQueueParam, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_QUEUE_CLIENT;
            info.prefix = (U8 *)"/IPP_QUEUE";
            info.identify = (U8 *)"intQueue";
            info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
            info.semName = NULL;
            info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
            info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
            /* create socket(server program) */
            rc = iPodPlayerIPCOpen(&sendQueueParam, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        /* create a queue for local message receive*/
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_QUEUE_SERVER;
            info.prefix = (U8 *)"/IPP_QUEUE_LOCAL";
            info.identify = (U8 *)"intQueueLocal";
            info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
            info.semName = NULL;
            info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
            info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
            /* create socket(server program) */
            rc = iPodPlayerIPCOpen(&recvQueueParamLocal, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        /* create a queue for local message send*/
        if(rc == IPOD_PLAYER_OK)
        {
            info.type = IPOD_PLAYER_OPEN_QUEUE_CLIENT;
            info.prefix = (U8 *)"/IPP_QUEUE_LOCAL";
            info.identify = (U8 *)"intQueueLocal";
            info.connectionNum = IPOD_PLAYER_CONNECTION_NUM;
            info.semName = NULL;
            info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
            info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
            /* create socket(server program) */
            rc = iPodPlayerIPCOpen(&sendQueueParamLocal, &info);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
        
        if(rc == IPOD_PLAYER_OK)
        {
            /* Register the IPC information */
            iPodPlayerSetSocketInfo(waitHandle, (U8)IPOD_PLAYER_OPEN_WAIT_HANDLE);
            iPodPlayerSetSocketInfo(longRecvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG);
            iPodPlayerSetSocketInfo(longSendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG);
            iPodPlayerSetSocketInfo(sckHandle, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
            iPodPlayerSetSocketInfo(recvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER);
            iPodPlayerSetSocketInfo(intWaitHandle, (U8)IPOD_PLAYER_OPEN_QUEUE_WAIT);
            iPodPlayerSetSocketInfo(recvQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
            iPodPlayerSetSocketInfo(sendQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_CLIENT);
            iPodPlayerSetSocketInfo(recvQueueParamLocal, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER_LOCAL);
            iPodPlayerSetSocketInfo(sendQueueParamLocal, (U8)IPOD_PLAYER_OPEN_QUEUE_CLIENT_LOCAL);
            /* start a receive thread(server program) */
            rc = iPodPlayerThreadCreate(uniqueName, cbTable);
        }

        if(rc != IPOD_PLAYER_OK)
        {
            /* Clear all of opened IPC information */
            if(waitHandle != -1)
            {
                iPodPlayerIPCClose(waitHandle);
            }
            
            if(longRecvSockParam != -1)
            {
                iPodPlayerIPCClose(longRecvSockParam);
            }
            
            if(longSendSockParam != -1)
            {
                iPodPlayerIPCClose(longSendSockParam);
            }
            
            if(sckHandle != -1)
            {
                iPodPlayerIPCClose(sckHandle);
            }
            
            if(recvSockParam != -1)
            {
                iPodPlayerIPCClose(recvSockParam);
            }
            
            if(intWaitHandle != -1)
            {
                iPodPlayerIPCClose(intWaitHandle);
            }
            
            if(recvQueueParam != -1)
            {
                iPodPlayerIPCClose(recvQueueParam);
            }
            
            if(sendQueueParam != -1)
            {
                iPodPlayerIPCClose(sendQueueParam);
            }
            
            if(recvQueueParamLocal != -1)
            {
                iPodPlayerIPCClose(recvQueueParamLocal);
            }
            
            if(sendQueueParamLocal != -1)
            {
                iPodPlayerIPCClose(sendQueueParamLocal);
            }

        iPodPlayerIPCDeinit();
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerDeinit(void)
 * This function de-initializes the iPodPlayer. Application must not use other API(except function group of \ref Init ) after calling this function.<br>
 * Registered callback function is removed by this function.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERROR Other error
 */
S32 iPodPlayerDeinit(void)
{
    S32 i = 0;
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam = -1;
    S32 recvSockParam = -1;
    S32 sendQueueParam = -1;
    S32 recvQueueParam = -1;
    S32 sendQueueParamLocal = -1;
    S32 recvQueueParamLocal = -1;
    S32 waitHandle = -1;
    S32 intWaitHandle = -1;
    S32 longRecvSockParam = -1;
    S32 longSendSockParam = -1;
    IPOD_PLAYER_PARAM_DEINIT param;
    IPOD_PLAYER_PARAM_DEINIT_RESULT result;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    memset(&result, 0, sizeof(result));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&recvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&waitHandle, (U8)IPOD_PLAYER_OPEN_WAIT_HANDLE);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&sendQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&recvQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&sendQueueParamLocal, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&recvQueueParamLocal, (U8)IPOD_PLAYER_OPEN_QUEUE_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&intWaitHandle, (U8)IPOD_PLAYER_OPEN_QUEUE_WAIT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&longRecvSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    rc = iPodPlayerGetSocketInfo(&longSendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    
    /* destroy a receive thread(server program) */
    iPodPlayerThreadDestroy();
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        if(sendSockParam >= 0)
        {
            /* set the value to data_head */
            param.header.funcId = IPOD_FUNC_DEINIT;
            param.header.appID = pid;

            /* send command */
            rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(IPOD_PLAYER_PARAM_DEINIT), 0, -1);
            if(rc == sizeof(IPOD_PLAYER_PARAM_DEINIT))
            {
                for(i = 0; i < IPOD_PLAYER_RECV_RETRY_NUM; i++)
                {
                    rc = iPodPlayerIPCReceive(recvSockParam, (U8*)&result, sizeof(IPOD_PLAYER_PARAM_DEINIT_RESULT), 1, waitHandle, 100);
                    if((rc == sizeof(IPOD_PLAYER_PARAM_DEINIT_RESULT)) && ((IPOD_PLAYER_FUNC_RESULT_ID)(result.header.funcId) == IPOD_FUNC_DEINIT_RESULT))
                    {
                        rc = result.result;
                        break;
                    }
                    else
                    {
                        rc = IPOD_PLAYER_ERROR;
                    }
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    iPodPlayerIPCClose(recvQueueParam);
    iPodPlayerIPCClose(sendQueueParam);
    iPodPlayerIPCClose(recvQueueParamLocal);
    iPodPlayerIPCClose(sendQueueParamLocal);
    /* close socket(client program) */
    iPodPlayerIPCClose(recvSockParam);
    iPodPlayerIPCClose(sendSockParam);
    iPodPlayerIPCClose(waitHandle);
    iPodPlayerIPCClose(intWaitHandle);
    iPodPlayerIPCClose(longRecvSockParam);
    iPodPlayerIPCClose(longSendSockParam);
    iPodPlayerInitSocketInfo();
    
    return rc;
}

/*!
 * \fn iPodPlayerSelectAudioOut(U32 devID, IPOD_PLAYER_AUDIO_SELECT mode)
 * This function is used to change the audio output.<br>
 * When application called this API, Connection Status is changed to #IPOD_PLAYER_AUTHENTICATION_RUNNING. 
 * Threfore, application must not call iPodPlayerAPI except #iPodPlayerInit, #iPodPlayerDeinit until status is changed to #IPOD_PLAYER_AUTHENTICATION_SUCCESS.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] mode Type is #IPOD_PLAYER_AUDIO_SELECT. Select the mode of Apple device's audio output.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used
 * \retval #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected
 * \retval #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected
 * \retval #IPOD_PLAYER_ERROR Other error
 * \warning
 * If this function is called when status is not #IPOD_PLAYER_AUTHENTICATION_SUCCESS, iPodPlayer replies error.<br>
 */
S32 iPodPlayerSelectAudioOut(U32 devID, IPOD_PLAYER_AUDIO_SELECT mode)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam = -1;
    S32 recvQueueParam = -1;
    S32 waitHandle = -1;
    IPOD_PLAYER_PARAM_SELECT_AUDIO_OUT param;
    IPOD_PLAYER_PARAM_SELECT_AUDIO_OUT_RESULT result;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    memset(&result, 0, sizeof(result));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&waitHandle, (U8)IPOD_PLAYER_OPEN_QUEUE_WAIT);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&recvQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SELECT_AUDIO_OUT;
        param.header.appID = pid;
        param.header.devID = devID;
        param.mode = mode;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(IPOD_PLAYER_PARAM_SELECT_AUDIO_OUT), 0, -1);
        if(rc != sizeof(IPOD_PLAYER_PARAM_SELECT_AUDIO_OUT))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Wait for a result data from the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* wait for receiving a socket data */
        rc = iPodPlayerIPCReceive(recvQueueParam, (U8*)&result, sizeof(IPOD_PLAYER_PARAM_SELECT_AUDIO_OUT_RESULT), 1, waitHandle, -1);
        if(rc == sizeof(IPOD_PLAYER_PARAM_SELECT_AUDIO_OUT_RESULT))
        {
            rc = result.result;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerTestReady(void)
 * This function is used to check whether the iPodPlayer have been ready.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERROR Other error
 */
S32 iPodPlayerTestReady(void)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam = -1;
    S32 waitHandle = -1;
    S32 recvQueueParam = -1;
    IPOD_PLAYER_PARAM_TEST_READY param;
    IPOD_PLAYER_PARAM_TEST_READY_RESULT result;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    memset(&result, 0, sizeof(result));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&waitHandle, (U8)IPOD_PLAYER_OPEN_QUEUE_WAIT);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&recvQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_TEST_READY;
        param.header.appID = pid;

        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(IPOD_PLAYER_PARAM_TEST_READY), 0, -1);
        if(rc != sizeof(IPOD_PLAYER_PARAM_TEST_READY))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Wait for a result data from the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* wait for receiving a socket data */
        rc = iPodPlayerIPCReceive(recvQueueParam, (U8*)&result, sizeof(result), 1, waitHandle, -1);
        if(rc == sizeof(result))
        {
            rc = result.result;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    return rc;
}
/*!
 * \fn iPodPlayerSetiOSAppsInfo(U32 devID, U8 appCount, IPOD_PLAYER_IOSAPP_INFO *appInfo)
 * This function is used to set the iOS Apps information.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device. If this value is set to -1, appInfo aplies to all Apple devices.<br>
 * \param [in] appCount Type is U8. A number of registering iOS Apps information.<br>
 * \param [in] *appInfo Type is #IPOD_PLAYER_IOSAPP_INFO pointer. It is structure array of iOS Application information.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used
 * \retval #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected
 * \retval #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected
 * \retval #IPOD_PLAYER_ERROR Other error
 */
S32 iPodPlayerSetiOSAppsInfo(U32 devID, U8 appCount, IPOD_PLAYER_IOSAPP_INFO *appInfo)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam = -1;
    S32 waitHandle = -1;
    S32 recvQueueParam = -1;
    IPOD_PLAYER_PARAM_SET_IOSAPPS_INFO param;
    IPOD_PLAYER_PARAM_SET_IOSAPPS_INFO_RESULT result;
    
    if((appCount > 10) || (appInfo == NULL))
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    memset(&result, 0, sizeof(result));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&waitHandle, (U8)IPOD_PLAYER_OPEN_QUEUE_WAIT);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&recvQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_IOS_APPS_INFO;
        param.header.appID = pid;

        param.header.devID = devID;
        param.appCount = appCount;
        memcpy(param.info, appInfo, sizeof(IPOD_PLAYER_IOSAPP_INFO) * appCount);
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(IPOD_PLAYER_PARAM_SET_IOSAPPS_INFO), 0, -1);
        if(rc != sizeof(IPOD_PLAYER_PARAM_SET_IOSAPPS_INFO))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Wait for a result data from the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* wait for receiving a socket data */
        rc = iPodPlayerIPCReceive(recvQueueParam, (U8*)&result, sizeof(result), 1, waitHandle, -1);
        if(rc == sizeof(result))
        {
            rc = result.result;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerStartAuthentication(U32 devID)
 * This function is used to start the authentication of Apple device.<br>
 * If Apple device's status is already #IPOD_PLAYER_AUTHENTICATION_SUCCESS, 
 * this status is changed to #IPOD_PLAYER_AUTHENTICATION_RUNNING.<br>
 * Changing status is notified by #IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device. 
 * If this value is set to -1, start authentication aplies to all Apple devices.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used
 * \retval #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected
 * \retval #IPOD_PLAYER_ERROR Other error
 */
S32 iPodPlayerStartAuthentication(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam = -1;
    S32 waitHandle = -1;
    S32 recvQueueParam = -1;
    IPOD_PLAYER_PARAM_START_AUTHENTICATION param;
    IPOD_PLAYER_PARAM_START_AUTHENTICATION_RESULT result;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    memset(&result, 0, sizeof(result));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&waitHandle, (U8)IPOD_PLAYER_OPEN_QUEUE_WAIT);
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodPlayerGetSocketInfo(&recvQueueParam, (U8)IPOD_PLAYER_OPEN_QUEUE_SERVER);
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_START_AUTHENTICATION;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(IPOD_PLAYER_PARAM_START_AUTHENTICATION), 0, -1);
        if(rc != sizeof(IPOD_PLAYER_PARAM_START_AUTHENTICATION))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Wait for a result data from the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* wait for receiving a socket data */
        rc = iPodPlayerIPCReceive(recvQueueParam, (U8*)&result, sizeof(result), 1, waitHandle, -1);
        if(rc == sizeof(result))
        {
            rc = result.result;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    return rc;
}

/*\}*/

/**
 * \addtogroup PlaybackAPI Playback
 * This group is API that is used to do the playback operation of the Apple device's music by application. API of this group returns the result immediately when application calls API.<br>
 * API of this group returns the result that it sent the command to Apple device.<br>
 * Application can receive the result of apple device's operation by callback function.<br>
 * Operation of this group can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Other mode can not be used.<br>
 * Therefore, application must send the #iPodPlayerSetMode with #IPOD_PLAYER_MODE_REMOTE_CONTROL at the begining.
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerPlay </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerPause </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerStop </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerFastForward </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerRewind </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerNextTrack </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerPrevTrack </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerNextChapter </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGotoTrackPosition </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerPlayTrack </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the Playback function
 */
/*\{*/

/*!
 * \fn S32 iPodPlayerPlay(U32 devID);<br>
 * This function is used to change the status to play.<br>
 * If apple device has a playable track, it starts the playback.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_PLAY_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function plays from first track in database.<br>
 */
S32 iPodPlayerPlay(U32 devID)
{
    return iPodPlayerPlayCommon(devID, FALSE);
}

/*!
 * \fn S32 iPodPlayerPlayCurrentSelection(U32 devID);<br>
 * This function plays from current selected item of media library.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_PLAY_CURRENT_SELECTION_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function is used with iAP2 device only. Otherwise if application launches this function with iAP1 device, it returns an error of #IPOD_PLAYER_ERR_NOT_APPLICABLE. <br>
 */
S32 iPodPlayerPlayCurrentSelection(U32 devID)
{
    return iPodPlayerPlayCommon(devID, TRUE);
}

/*!
 * \fn iPodPlayerPause(U32 devID)
 * This function is used to change the status to pause.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_PAUSE_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning 
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerPause(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_PAUSE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_PAUSE;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerStop(U32 devID)
 * This function is used to change the status to stop.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_STOP_RESULT<br>
 * If track is stopping, application can only use #iPodPlayerPlay.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used  when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If playback status is changed to stop, iPodPlayer will remove the playback list.<br>
 */
S32 iPodPlayerStop(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_STOP param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_STOP;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerFastForward(U32 devID)
 * This function is used to change the status to fast forward.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_FASTFORWARD_RESULT.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * FastForward is stopped if application call #iPodPlayerPlay, #iPodPlayerPause, #iPodPlayerStop, #iPodPlayerRewind or track is changed.<br>
 * If Application uses this function and track arrives to last position of track length, Next track is played and FastFoward is stopped.<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerFastForward(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_FAST_FORWARD param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_FASTFORWARD;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerRewind(U32 devID)
 * This function is used to change the status to rewind.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_REWIND_RESULT.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * FastBackward is stopped if application calls #iPodPlayerPlay, #iPodPlayerPause, #iPodPlayerStop, #iPodPlayerFastForward or track is changed.<br>
 * FastBackward is stopped when track backed to start position(0:00).<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerRewind(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_REWIND param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_REWIND;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerNextTrack(U32 devID)
 * This function is used to change the track to next track.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_NEXTTRACK_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function changes the track to next track even if track has chapter.
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 * If current playing track is tail in current playlist, playback status changes to #IPOD_PLAYER_PLAY_STATUS_STOP.<br>
 */
S32 iPodPlayerNextTrack(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_NEXT_TRACK param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_NEXT_TRACK;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerPrevTrack(U32 devID)
 * This function is used to change the track to previous track.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_PREVTRACK_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function changes the track to previous track even if track has chapter.<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 * If current playing track is top in current plyalist, playback status changes to #IPOD_PLAYER_PLAY_STATUS_STOP.
 */
S32 iPodPlayerPrevTrack(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_PREV_TRACK param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_PREV_TRACK;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerNextChapter(U32 devID)
 * This function is used to change the chapter to next chapter.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_NEXT_CHAPTER_RESULT 
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If current playing track does not have a chapter or current chapter is tail in track, this function returns an error of #IPOD_PLAYER_ERR_NO_CHAPTER.<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerNextChapter(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_NEXT_CHAPTER param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_NEXTCHAPTER;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerPrevChapter(U32 devID)
 * This function is used to change the chapter to previous chapter.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_PREV_CHAPTER_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If current playing track does not have a chapter or current chapter is top in track, this function returns an error of #IPOD_PLAYER_ERR_NO_CHAPTER.<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerPrevChapter(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_PREV_CHAPTER param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_PREVCHAPTER;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}



/*!
 * \fn iPodPlayerGotoTrackPosition(U32 devID, U32 timems)
 * This function is used to move the track position.<br>
 * Track position is moved with millisecond.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GOTO_TRACKPOSITION_RESULT.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] timems Type is U32. This is the time with millisecond. Track is moved to location of this time position.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function will return an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerGotoTrackPosition(U32 devID, U32 timems)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GOTO_TRACK_POSITION param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GOTO_TRACK_POSITION;
        param.header.appID = pid;

        param.header.devID = devID;
        param.times = timems;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerPlayTrack(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID)
 * This function is used to play the track directly in Playback list, Database list or UniqueID.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_PLAY_TRACK_RESULT.
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Playing track is selected in this type.<br>
 * \param [in] trackID Type is U64. Identify the track.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * Some apple device's does not support opearation by unique ID. Application can know whether unique ID can be used or not by #iPodPlayerGetDeviceProperty with #IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE.<br>
 */
S32 iPodPlayerPlayTrack(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_PLAY_TRACK param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_PLAYTRACK;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.trackID = trackID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerRelease(U32 devID)
 * This function is used to release after device is actived with FastForward or Rewind.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_RELEASE_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning 
 * application shuld be called this function after call #iPodPlayerFastForward or<br>
 * #iPodPlayerRewind in order to cancel fast-forword and rewind.<br>
 */
S32 iPodPlayerRelease(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_RELRASE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_RELEASE;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*\}*/

/**
 * \addtogroup SetProperty Set Property
 * This group is API that application can use and change the status of Apple device.<br>
 * API of this group returns the result immediately when application calls API.<br>
 * API of this group returns the result that it sent the command to Apple device.<br>
 * Application can receive the result of Apple device's operation by callback function.<br>
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerSetAudioMode </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetMode </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetRepeatStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetShuffleStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetEqualizer </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetVideoDelay </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetVideoSetting </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetDisplayImage </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetPlaySpeed </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetTrackInfoNotification </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetDeviceEventNotification </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the Set Property function
 */
/*\{*/

/*!
 * \fn iPodPlayerSetAudioMode(U32 devID, IPOD_PLAYER_AUDIO_SETTING setting)
 * This function is used to set the audio mode. <br>
 * The default mode is #IPOD_PLAYER_SOUND_MODE_OFF when Apple device is connected.<br>
 * The setting has a mode and adjust.<br>
 * If current state adjustment is #IPOD_PLAYER_STATE_ADJUST_ENABLE, state adjustment will be enabled.<br>
 * e.g. Application requests Play to Apple device but Apple device notifies of the Pause. 
 * Then iPodPlayer will change the status of Apple device to play automatically<br>
 * If current state adjustment is #IPOD_PLAYER_STATE_ADJUST_DISABLE, state adjustment will be disabled.<br>
 * e.g. Application requests Play to Apple device but Apple device notifies the Pause.
 * then iPodPlayer does not do anything.<br>
 * If adjustment is changed to #IPOD_PLAYER_STATE_ADJUST_ENABLE from #IPOD_PLAYER_STATE_ADJUST_DISABLE, 
 * last status is reset. <br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_AUDIO_MODE_RESULT<br>
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] setting - Type is IPOD_PLAYER_AUDIO_SETTING. This is a setting for audio mode. <br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * Adjust settings has an effect only when mode is #IPOD_PLAYER_SOUND_MODE_ON<br>
 * #iPodPlayerSetVolume and #iPodPlayerGetVolume can be used when mode is #IPOD_PLAYER_SOUND_MODE_ON.<br>
 * Otherwise these API returns an error.<br>
 * If audio mode is changed to #IPOD_PLAYER_SOUND_MODE_OFF from #IPOD_PLAYER_SOUND_MODE_ON,
 * Current volume shall be reset<br>
 * If audio mode is changed to #IPOD_PLAYER_SOUND_MODE_ON from #IPOD_PLAYER_SOUND_MODE_OFF,
 * default volume shalle be set.<br>
 * \attention
 * If Apple device is playing when current sound mode is #IPOD_PLAYER_SOUND_MODE_OFF, Apple device may be broken.<br>
 * Also, Apple device may not do audio streaming. <br>
 * 
 */
S32 iPodPlayerSetAudioMode(U32 devID, IPOD_PLAYER_AUDIO_SETTING setting)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_AUDIO_MODE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_AUDIO_MODE;
        param.header.appID = pid;

        param.header.devID = devID;
        param.setting = setting;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    return rc;
}

/*!
 * \fn iPodPlayerSetMode(U32 devID, IPOD_PLAYER_MODE mode)
 * This function is used to change the mode status. Remote Control and HMI Control mode can not operate Apple device by itself.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_MODE_RESULT<br>
 * If application want to operate the Apple device by remote, Application must call this function with #IPOD_PLAYER_MODE_REMOTE_CONTROL at first. <br>
 * After it, Application can operate the Apple device by remote control. (e.g. Playback, Database browsing)<br>
 * Also, If application want to operate the Apple device by remote but HMI displayed by Apple device, application must call this function with #IPOD_PLAYER_MODE_HMI_CONTROL<br>
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param mode - Type is #IPOD_PLAYER_MODE. This is a mode which decides the control mode.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If current mode is changed to Remote Control mode by this function, 
 * iPodPlayer will notify the current playback status to Application automatically(Using #IPOD_PLAYER_CB_NOTIFY_CONNECTION_STATUS).<br>
 */
S32 iPodPlayerSetMode(U32 devID, IPOD_PLAYER_MODE mode)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_MODE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_MODE;
        param.header.appID = pid;

        param.header.devID = devID;
        param.mode = mode;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn S32 iPodPlayerSetRepeatStatus(U32 devID, IPOD_PLAYER_REPEAT_STATUS status)
 * This function is used to change the repeat status.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_REPEAT_STATUS_RESULT.
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] status Type is #IPOD_PLAYER_REPEAT_STATUS. This mode is used to decide the repeat mode.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerSetRepeatStatus(U32 devID, IPOD_PLAYER_REPEAT_STATUS status)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_REPEAT param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_REPEAT;
        param.header.appID = pid;

        param.header.devID = devID;
        param.status = status;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetShuffleStatus(U32 devID, IPOD_PLAYER_SHUFFLE_STATUS status)
 * This function is used to set the shuffle mode.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_SHUFFLE_STATUS_RESULT.
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] status - Type is #IPOD_PLAYER_SHUFFLE_STATUS. This value is used to select the shuffle mode.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This shuffle status is changed only playback list but not affect the database list.<br>
 * Therefore, application must not use #iPodPlayerGetDBEntries with #IPOD_PLAYER_TRACK_TYPE_DATABASE to get the shuffled track namelist. 
 * Application must use the #iPodPlayerGetTrackInfo with #IPOD_PLAYER_TRACK_TYPE_PLAYBACK instead.<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerSetShuffleStatus(U32 devID, IPOD_PLAYER_SHUFFLE_STATUS status)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_SHUFFLE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_SHUFFLE;
        param.header.appID = pid;

        param.header.devID = devID;
        param.status = status;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerChangeRepeatStatus(U32 devID)
 * This function is used to change the status of Repeat.<br>
 * The repeat status of Apple device is changed by Apple device specification.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_CHANGE_REPEAT_STATUS_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This API is used in just iAP2 device. <br>
 */
S32 iPodPlayerChangeRepeatStatus(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_CHANGE_REPEAT param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_CHANGE_REPEAT;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    
    return rc;
}

/*!
 * \fn iPodPlayerChangeShuffleStatus(U32 devID)
 * This function is used to change the status of shuffle(random mode).<br>
 * The shuffle status of Apple device is changed by Apple device specification.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_CHANGE_SHUFFLE_STATUS_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This API is used in just iAP2 device. <br>
 */
S32 iPodPlayerChangeShuffleStatus(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_CHANGE_SHUFFLE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_CHANGE_SHUFFLE;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    
    return rc;
}

/*!
 * \fn iPodPlayerSetEqualizer(U32 devID, U32 eq, U8 restore)
 * This function is used to set the equalizer.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_EQUALIZER_RESULT.
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] eq Type is U32. index of equalizer.<br>
 * \param [in] restore Type is U8. Value of restore<br>
 * \li \c <b> #IPOD_PLAYER_RESTORE_OFF - New setting is restored when Apple device is disconnected.</b>
 * \li \c <b> #IPOD_PLAYER_RESTORE_ON - Original setting is restored when Apple device is disconnected.</b><br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerSetEqualizer(U32 devID, U32 eq, U8 restore)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_EQUALIZER param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_EQUALIZER;
        param.header.appID = pid;

        param.header.devID = devID;
        param.eq = eq;
        param.restore = restore;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetVideoDelay(U32 devID, U32 delayTime)
 * This function is used to delay the video.<br>
 * Some Apple devices may not support this function.<br>
 * Application can know whether Apple device supports this function by using #iPodPlayerGetDeviceProperty.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_VIDEO_DELAY_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] delayTime Type is U32. Time is milisecond.<br>
 * \retval #IPOD_PLAYER_OK Function Success
 * \retval #IPOD_PLAYER_ERR_INVALID_ID Invalid device ID is selected
 * \retval #IPOD_PLAYER_ERR_INVALID_MODE Current mode is invalid mode
 * \retval #IPOD_PLAYER_ERR_INVALID_PARAMETER Invalid parameter is used
 * \retval #IPOD_PLAYER_ERR_NOT_SUPPORT This function is not supported
 * \retval #IPOD_PLAYER_ERR_NOT_CONNECT Apple device is not connected
 * \retval #IPOD_PLAYER_ERROR Other error
 */
S32 iPodPlayerSetVideoDelay(U32 devID, U32 delayTime)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_VIDEO_DELAY param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_VIDEO_DELAY;
        param.header.appID = pid;

        param.header.devID = devID;
        param.delayTime = delayTime;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
    
}

/*!
 * \fn iPodPlayerSetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting, U32 restore)
 * This function is used to set the vide setting.<br>
 * Some Apple devices may not affect the setting.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_VIDEO_SETTING_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] setting Type is #IPOD_PLAYER_VIDEO_SETTING pointer. Kind of video setting<br>
 * \param [in] restore - Value of restore
 * \li \c <b> #IPOD_PLAYER_RESTORE_OFF - New setting is restored when Apple device is disconnected.</b>
 * \li \c <b> #IPOD_PLAYER_RESTORE_ON - Original setting is restored when Apple device is disconnected.<br><br></b>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerSetVideoSetting(U32 devID, IPOD_PLAYER_VIDEO_SETTING *setting, U32 restore)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_VIDEO_SETTING param;
    
    if(setting == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_VIDEO_SETTING;
        param.header.appID = pid;

        param.header.devID = devID;
        param.restore = restore;
        memcpy(&param.setting, setting, sizeof(param.setting));
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerSetDisplayImage(U32 devID, U32 imageSize, U8 *image)
 * This function is used to set image to display of Apple device.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_DISPLAY_IMAGE_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] imageSize Type is U32. This is size of image.<br>
 * \param [in] *image Type is U8 pointer. Data of Bitmap image.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * Application must send this API with data of bitmap image. Other kind of image does not know what is happen.<br>
 * Some Apple devices can not display the image. Application can be known by using #iPodPlayerGetDeviceProperty.<br>
 * also, application can know the limitation of image size by using #iPodPlayerGetDeviceProperty<br>
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * The bitmap image must be a "top-down" bitmap and format must be 24bit uncompressed.
 */
S32 iPodPlayerSetDisplayImage(U32 devID, U32 imageSize, U8 *image)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_DISPLAY_IMAGE param;
    U8 *sendData[2] = {NULL};
    U32 sendSize[2] = {0};
    U8 sendNum = 2;
    U32 asize = 0;
    
    if(image == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_DISPLAY_IMAGE;
        param.header.appID = pid;
        param.header.longData = 1;

        param.header.devID = devID;
        param.imageSize = imageSize;
        
        sendData[0] = (U8 *)&param;
        sendSize[0] = sizeof(param);
        sendData[1] = image;
        sendSize[1] = imageSize;
        
        /* send command */
        rc = iPodPlayerIPCLongSend(sendSockParam, sendNum, sendData, sendSize, &asize, 0, IPOD_PLAYER_LONG_SEND_TMOUT);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerSetPlaySpeed(U32 devID, IPOD_PLAYER_PLAYING_SPEED speed)
 * This function is used to change the playing speed.<br>
 * Currently, this speed is aplied only audiobook.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_PLAY_SPEED_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] speed Type is #IPOD_PLAYER_PLAYING_SPEED. Value of setting of audiobook speed<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used when mode is #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise this function will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application calls this function when playback status is #IPOD_PLAYER_PLAY_STATUS_STOP, this function returns an error of #IPOD_PLAYER_ERR_INVALID_STATUS.<br>
 */
S32 iPodPlayerSetPlaySpeed(U32 devID, IPOD_PLAYER_PLAYING_SPEED speed)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_PLAY_SPEED param;
    
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_PLAY_SPEED;
        param.header.appID = pid;

        param.header.devID = devID;
        param.speed = speed;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetTrackInfoNotification(U32 devID, U32 trackInfoMask, U16 formatId)
 * This function is used to decide what notification is enabled.<br>
 * When track is changed to other track, Application can get the information of changed track from bitmask(trackInfoMask) that is set by this function.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_TRACK_INFO_NOTIFICATION_RESULT<br>
 * If bitmask is set to non-zero value, iPodPlayer notify the track information by #IPOD_PLAYER_CB_NOTIFY_TRACK_INFO when track is changed.<br>
 * To set the bit of #IPOD_PLAYER_TRACK_INFO_MASK_COVERART, Application must get the formatId by using #iPodPlayerGetDeviceProperty with #IPOD_PLAYER_DEVICE_MASK_FORMAT.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] trackInfoMask Type is U32. Bitmask of notification that application want to know. Please see below.<br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME - Notify the changed track name</b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME - Notify the album name of changed track</b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME - Notify the artist name of changed track</b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME - Notify the podcast name of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION - Notify the description of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_LYRIC - Notify the Lyric of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_GENRE - Notify the Genre of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER - Notify the composer of chagned track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE - Notify the release date of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY - Notify the capability of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH - Notify the track length of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT - Notify the chapter contents that chagned track has </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_UID - Notify the unique ID of changed track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND - Notify the kind of changed track</b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_COVERART - Notify the coverart data if coverart is changed</b><br>
 * \param [in] formatId Type is U16. This parameter is only enable when #IPOD_PLAYER_TRACK_INFO_MASK_COVERART is set. coverart is sent by this format.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * Default setting is not notified the track info even if track is changed.<br>
 */
S32 iPodPlayerSetTrackInfoNotification(U32 devID, U32 trackInfoMask, U16 formatId)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_TRACK_INFO_NOTIFICATION param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_TRACK_INFO_NOTIFICATION;
        param.header.appID = pid;

        param.header.devID = devID;
        param.trackInfoMask = trackInfoMask;
        param.formatId = formatId;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetDeviceEventNotification(U32 devID, U32 deviceEventMask)
 * This function is used to decide what Apple device event is notified.<br>
 * When Apple device works by some request, Application can get the changing event of Apple device .<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_DEVICE_EVENT_NOTIFICATION_RESULT.<br>
 * If bitmask(deviceEventMask) is set by this function and when indicated bit status is changed, iPodPlayer shall notify to Application by #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT.<br>
 * Bit clear operation is invalid only for iAP2.
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] deviceEventMask Type is U32. Bitmask of notification that application want to know. Please see \ref AppleDeviceEventMask.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * Default setting can get by #iPodPlayerGetDeviceProperty.<br>
 */
S32 iPodPlayerSetDeviceEventNotification(U32 devID, U32 deviceEventMask)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_DEVICE_EVENT_NOTIFICATION param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_DEVICE_EVENT_NOTIFICATION;
        param.header.appID = pid;

        param.header.devID = devID;
        param.deviceEventMask = deviceEventMask;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
    
}

/*\}*/

/**
 * \addtogroup GetProperty Get Property
 * This group is API that application can use and get the information of Apple device.<br>
 * API of this group returns the result immediately when application calls API.<br>
 * API of this group returns the result that it sent the command to Apple device.<br>
 * Application can receive the result of Apple device operation by callback function.<br>
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerGetVideoSetting </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetCoverArtInfo </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetCoverArt </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetPlaybackStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetTrackInfo </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetChapterInfo </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetMode </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetRepeatStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetShuffleStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetPlaySpeed </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetTrackTotalCount </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetEqualizer </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetEqualizerName </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetDeviceProperty </TD>
 *      <TD> Valid(Limited) </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetDeviceStatus </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the Get Property function
 */

/*\{*/


/*!
 * \fn iPodPlayerGetVideoSetting(U32 devID, U32 mask)
 * This function is used to get the current video setting of Apple device.<br>
 * Result of this API can be known by using the #IPOD_PLAYER_CB_GET_VIDEO_SETTING_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] mask Type is U32. Mask of video setting. See \ref VideSettingDefines.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * Some Apple devices may not support some video settings. Application can know usable settings by #iPodPlayerGetDeviceProperty.
 */
S32 iPodPlayerGetVideoSetting(U32 devID, U32 mask)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_VIDEO_SETTING param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_VIDEO_SETTING;
        param.header.appID = pid;

        param.header.devID = devID;
        param.mask = mask;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetCoverArtInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId)
 * This function is used to get the information of Cover Art that is indicated by formatId.<br>
 * This information is time of coverart and One track has some coverarts to each time.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_COVERART_INFO_RESULT
 * To use the this function, application must get the formatId by #iPodPlayerGetDeviceProperty with #IPOD_PLAYER_DEVICE_MASK_FORMAT.<br>
 * It is included the each format Id, width of each id and height of each id. Application can select the format Id that application want to use.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Coverart information of indicated type and trackindex can be gotten.
 * \param [in] trackIndex Type is U64. Index that application gets the coverart<br>
 * \param [in] formatId Type is U16. ID of coverart image format<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerGetCoverArtInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_COVERART_INFO param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_COVERART_INFO;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.trackIndex = trackIndex;
        param.formatId = formatId;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetCoverArt(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId, U32 coverartTime)
 * This function is used to get the Cover Art.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_COVERART_RESULT.
 * Coverart Data is sent by #IPOD_PLAYER_CB_NOTIFY_COVERART_DATA.
 * To use the this function, application must get the formatId by #iPodPlayerGetDeviceProperty with #IPOD_PLAYER_DEVICE_MASK_FORMAT.<br>
 * It is included the each format Id, width of each id and height of each id. Application can select the format Id that application want to use.<br>
 * After that, Application can know the infomration of coverart of target track by #iPodPlayerGetCoverArt.<br>
 * It's information is included the time where is showed in track.<br>
 * Application can use this function to get the CoverArt data with acquired format Id and time.<br>
 * #IPOD_PLAYER_CB_GET_COVERART_RESULT is sent after finish the sending coverart data
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Coverart of indicated type and trackindex can be gotten.
 * \param [in] trackIndex Type is U64. Index that application get coverart<br>
 * \param [in] formatId Type is U16. ID of coverart image format<br>
 * \param [in] coverartTime Type is U32. Time at millisecond that changing the coverart<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If this funcition is canceled by #iPodPlayerCancel, coverart data may not be sent 
 */
S32 iPodPlayerGetCoverArt(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackIndex, U16 formatId, U32 coverartTime)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_COVERART param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_COVERART;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.trackIndex = trackIndex;
        param.formatId = formatId;
        param.time = coverartTime;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerGetPlaybackStatus(U32 devID)
 * This function is used to get the playback status.<br>
 * Application can also get the playback status by #IPOD_PLAYER_CB_NOTIFY_PLAYBACK_STATUS<br>
 * Same information is included in these function <br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_PLAYBACK_STATUS_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL or #IPOD_PLAYER_MODE_HMI_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetPlaybackStatus(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_PLAYBACK_STATUS param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_PLAYBACK_STATUS;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetTrackInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 startID, U32 count, U32 trackInfoMask)
 * This function is used to get the information of indicated track ID until count.<br>
 * This can get the track number, song title, artist name and so on.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT<br>
 * Also, #IPOD_PLAYER_CB_GET_TRACK_INFO_RESULT is sent as much as value of count.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Kind of using track ID. e.g(playback track ID, database track ID or unique track ID).<br>
 * \param [in] startID Type is U64. Beginning track id which is gotten the track information.<br>
 * \param [in] count Type is U32. This value is used to specify the range acquired from startID.
 * \param [in] trackInfoMask Type is U32. These bit is below.
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME - Get the track name </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME - Get the album name </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME - Get the artist name </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME - Get the podcast name </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION - Get the description </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_LYRIC - Get the lyric. </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_GENRE - Get the genre </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER - Get the composer </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE - Get teh release date </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY - Get the capability of indicated track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH - Get the length of indicated track </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT - Get the number of chapter that Apple device has </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_UID  - Get the unique ID </b><br>
 * \li \c <b> #IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND - Get the kind of track. </b><br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used in #IPOD_PLAYER_MODE_REMOTE_CONTROL or #IPOD_PLAYER_MODE_HMI_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If below mask is set at the same time, iPodPlayer is slow to reply to Application. iPodPlayer recommends to get the below information only when Application actually need.<br>
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_PODCAST_NAME
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_DESCRIPTION
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_LYRIC
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_GENRE
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_RELEASE_DATE
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_UID
 * \li \c #IPOD_PLAYER_TRACK_INFO_MASK_TRACK_KIND
 *
 */
S32 iPodPlayerGetTrackInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 startID, U32 count, U32 trackInfoMask)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_TRACK_INFO param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_TRACK_INFO;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.startID = startID;
        param.count = count;
        param.trackInfoMask = trackInfoMask;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetChapterInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U32 trackID, U32 startIndex, U32 count,  U32 chapterInfoMask)
 * This function is used to get the information of indexed chapter.<br>
 * This can get the track number, song title, artist name and so on.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT
 * Also, #IPOD_PLAYER_CB_GET_CHAPTER_INFO_RESULT is sent as much as value of count.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Kind of track.<br>
 * \param [in] trackID Type is U32. Get the information of this track id<br>
 * \param [in] startIndex Type is U32. Get the information from this chapter Index until count<br>
 * \param [in] count Type is U32. Get the chapter intformation until this value.
 * \param [in] chapterInfoMask - Mask of kind of information for get. Meaning of each bit is below. <br>
 * \li \c <b> #IPOD_PLAYER_CHAPTER_INFO_MASK_LENGTH - Total length of indicated chapter index</b><br>
 * \li \c <b> #IPOD_PLAYER_CHAPTER_INFO_MASK_NAME - Name of indicated chapter index </b><br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL or #IPOD_PLAYER_MODE_HMI_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetChapterInfo(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U32 trackID, U32 startIndex, U32 count, U32 chapterInfoMask)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_CHAPTER_INFO param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_CHAPTER_INFO;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.trackID = trackID;
        param.startIndex = startIndex;
        param.count = count;
        param.chapterInfoMask = chapterInfoMask;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetMode(U32 devID)
 * This function is used to get the mode of player.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_MODE_RESULT<br>
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetMode(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_MODE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_MODE;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetRepeatStatus(U32 devID)
 * This function is used to get the current repeat status.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_REPEAT_STATUS_RESULT
 * \param devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetRepeatStatus(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_REPEAT_STATUS param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_REPEAT;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerGetShuffleStatus(U32 devID)
 * This function is used to get the current shuffle status.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_SHUFFLE_STATUS_RESULT
 * \param devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetShuffleStatus(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_SHUFFLE_STATUS param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_SHUFFLE;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetPlaySpeed(U32 devID)
 * This function is used to get current playing speed.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_PLAY_SPEED_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetPlaySpeed(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_PLAY_SPEED param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_PLAY_SPEED;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetTrackTotalCount(U32 devID, IPOD_PLAYER_TRACK_TYPE type)
 * This function is used to get the total number of tracks in the list of tracks queued to play.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_TRACK_TOTAL_COUNT_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type - Type is #IPOD_PLAYER_TRACK_TYPE. This value is used to select the kind of track. This function can get the total count ofthis type.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL or #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application use this function in #IPOD_PLAYER_MODE_REMOTE_CONTROL, type must be set to #IPOD_PLAYER_TRACK_TYPE_PLAYBACK.<br>
 * This function does not use the type of #IPOD_PLAYER_TRACK_TYPE_UID. If application set it, this function returns an error of #IPOD_PLAYER_ERR_INVALID_PARAMETER<br>
 */
S32 iPodPlayerGetTrackTotalCount(U32 devID, IPOD_PLAYER_TRACK_TYPE type)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_TRACK_TOTAL_COUNT param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_TRACK_TOTAL_COUNT;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetMediaItemInformation(U32 devID, U64 trackID)
 * This function is used to get information of media item in iAP2 protocol.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_MEDIA_ITEM_INFO_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] trackID - Type is U64. This value is used to select media item data. This function can get media information(media type)<br>.
 * Please get trackID by IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY parameter in #IPOD_PLAYER_CB_NOTIFY_TRACK_INFO 
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function have to use after media library update was finished. Please check progress in #IPOD_PLAYER_CB_NOTIFY_DEVICE_EVENT<br>
 */
S32 iPodPlayerGetMediaItemInformation(U32 devID, U64 trackID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_MEDIA_ITEM_INFO param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_MEDIA_ITEM_INFO;
        param.header.appID = pid;

        param.header.devID = devID;
        param.trackID = trackID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetEqualizer(U32 devID)
 * This function is used to get the current equalizer setting.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_EQUALIZER_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerGetEqualizer(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_EQUALIZER param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_EQUALIZER;
        param.header.appID = pid;

        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetEqualizerName(U32 devID, U32 eq)
 * This function is used to get the name of equalizer that indicated by eq.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_EQUALIZER_NAME_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] eq Type is U32. Equalizer index.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If eq is 0, it means always "off".<br>
 */
S32 iPodPlayerGetEqualizerName(U32 devID, U32 eq)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_EQUALIZER_NAME param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_EQUALIZER_NAME;
        param.header.appID = pid;

        param.header.devID = devID;
        param.eq = eq;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}



/*!
 * \fn iPodPlayerGetDeviceProperty(U32 devID, U32 devicePropertyMask)
 * This function is used to get the Apple device information .<br>
 * Application can know the Apple device information by this API.<br>
 * Result of this API can be known by receiving #IPOD_PLAYER_CB_GET_DEVICE_PROPERTY_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] devicePropertyMask Type is U32. Bitmask of Apple device information to want.
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_NAME : Name of Apple device</b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION : Software Version of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER : Serial Number of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE : Max Payload Size of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE : Supported feature of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_EVENT : Event of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_FILE_SPACE : File Space of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_FORMAT : Support Coverart format of Apple device </b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT : Max Image Size of Apple device Monochrome Image</b>
 * \li \c <b> #IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT: Max Image Size of Apple device Color Image </b>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * 
 * * The following list shows that the devicePropertyMask can be used by iAP1 or iAP2.<br>
 * <TABLE>
 * <TR>
 *      <TD>devicePropertyMask </TD>
 *      <TD> iAP1</TD>
 *      <TD> iAP2</TD>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_NAME </TD>
 *      <TD> Supported </TD>
 *      <TD> Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_SOFT_VERSION </TD>
 *      <TD> Supported </TD>
 *      <TD> Not Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_SERIAL_NUMBER </TD>
 *      <TD> Supported </TD>
 *      <TD> Not Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_MAX_PAYLOAD_SIZE </TD>
 *      <TD> Supported </TD>
 *      <TD> Not Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE </TD>
 *      <TD> Supported </TD>
 *      <TD> Supported </TD>
 * </TR>
 *<TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_EVENT </TD>
 *      <TD> Supported </TD>
 *      <TD> Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_FILE_SPACE </TD>
 *      <TD> Supported </TD>
 *      <TD> Not Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_FORMAT </TD>
 *      <TD> Supported </TD>
 *      <TD> Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_MONO_LIMIT </TD>
 *      <TD> Supported </TD>
 *      <TD> Not Supported </TD>
 * </TR>
 * <TR>
 *      <TD> #IPOD_PLAYER_DEVICE_MASK_COLOR_LIMIT </TD>
 *      <TD> Supported </TD>
 *      <TD> Not Supported </TD>
 * </TR>
 * </TABLE>
 * 
 * \author H.Tanaka
 * \date 19/Jan/2016 
 */
S32 iPodPlayerGetDeviceProperty(U32 devID, U32 devicePropertyMask)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_DEVICE_PROPERTY param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_DEVICE_PROPERTY;
        param.header.appID = pid;

        param.header.devID = devID;
        param.devicePropertyMask = devicePropertyMask;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetDeviceStatus(U32 devID, U32 deviceStatusMask)
 * This function is used to get the current status of Apple device.<br>
 * Application can know the status of Apple device by this API.<br>
 * Result of this API can be known by receiving #IPOD_PLAYER_CB_GET_DEVICE_STATUS_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] deviceStatusMask Type is U32. Bitmask of Apple device Status to want.
 * \li \c <b> #IPOD_PLAYER_IPOD_STATUS_INFO_MASK_MODE : Current mode</b><br>
 * \li \c <b> #IPOD_PLAYER_IPOD_STATUS_INFO_MASK_STATUS : Current power status </b><br>
 * \li \c <b> #IPOD_PLAYER_IPOD_STATUS_INFO_MASK_NOTIFY_EVENT : Current notify event </b><br>
 * \li \c <b> #IPOD_PLAYER_IPOD_STATUS_INFO_MASK_RUNNING_APP : Current Running Application</b><br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerGetDeviceStatus(U32 devID, U32 deviceStatusMask)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_DEVICE_STATUS param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_DEVICE_STATUS;
        param.header.appID = pid;

        param.header.devID = devID;
        param.deviceStatusMask = deviceStatusMask;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

 /*\}*/


/**
 * \addtogroup Database Database Browsing
 * Function of this group is used to browse the database or retrieve the entry.<br>
 * If application uses this group, application must use either the #iPodPlayerClearSelection with #IPOD_PLAYER_DB_TYPE_ALL or #iPodPlayerSelectAV at first.<br>
 * Then application can use this group for the browsing or retrieving the entry.<br>
 * To browse the database, below function can be used.
 * \arg #iPodPlayerClearSelection It is used to clear the database also it is used to back to previous database.<br>
 * \arg #iPodPlayerGetDBCount It is used to acquire the number of entry in database type that current database has.<br>
 * \arg #iPodPlayerSelectDBEntry It is used to select the another database.<br>
 * \arg #iPodPlayerSelectAV It is used to select the Audio database or Video database.<br>
 *
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerGetDBEntries </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetDBCount </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerCancel </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSelectDBEntry </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerClearSelection </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSelectAV </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the Database Browsing function
 */
/*\{*/


 /*!
 * \fn iPodPlayerGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 start, S32 num)
 * This function is used to retrieve the entry list from database by database type.<br>
 * Entry can be retrieved only from current database list. Therefore if application want to retrieve the next/previous database,
 * application must use #iPodPlayerSelectDBEntry or #iPodPlayerClearSelection.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_DB_ENTRIES.<br>
 * Database entries are notified by #IPOD_PLAYER_CB_NOTIFY_DB_ENTRIES.<br>
 * Entries are notified from start index until num values.
 * #IPOD_PLAYER_CB_GET_DB_ENTRIES is sent after finish the all entries are sent.<br>
 * \param [in] devID Type is U32. Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_DB_TYPE. This is type that applicatoon want to retrieve the entries.<br>
 * \param [in] start Type is U32. From this value, iPodPlayer gets the entries from database. It is counted from 0.<br>
 * \param [in] num Type is S32. Number of entry that application gets. If this value sets to -1, all entry of indicated type is listed.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If num is set to -1, this function can get the all entry in type.<br>
 */
S32 iPodPlayerGetDBEntries(U32 devID, IPOD_PLAYER_DB_TYPE type, U32 start, S32 num)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_DB_ENTRIES param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_DB_ENTRIES;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.start = start;
        param.num = num;
        
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerGetDBCount(U32 devID, IPOD_PLAYER_DB_TYPE type)
 * This function is used to get the number of Apple device tracks with type.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_DB_COUNT_RESULT.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_DB_TYPE. Can get the total count of indicated type in current database.
 * \warning
 * Number of this result depends on the current entry.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerGetDBCount(U32 devID, IPOD_PLAYER_DB_TYPE type)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_DB_COUNT param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_DB_COUNT;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerCancel(U32 devID, IPOD_PLAYER_CANCEL_TYPE type)
 * This function is used to cancel the some requests.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_CANCEL_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_CANCEL_TYPE. Latest request of this type is canceled.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerCancel(U32 devID, IPOD_PLAYER_CANCEL_TYPE type)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_CANCEL param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_CANCEL;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerSelectDBEntry(U32 devID, IPOD_PLAYER_DB_TYPE type, S32 entry)
 * This function is used to change the database to indicated entry.<br>
 * Result of this function can be known by receiving the #IPOD_PLAYER_CB_SELECT_DB_ENTRY_RESULT<br>
 * If application want to select the third album of current database, application can use this function with type is #IPOD_PLAYER_DB_TYPE_ALBUM and entry is 2(counted from 0).<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_DB_TYPE. Database is changed by kind of Selected type. If entry is set -1, this type must be set with current category type.<br>
 * \param [in] entry Type is U32. Select the this entry in type. It is counted from 0. If entry is set -1, it can back to previous dababase.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_REMOTE_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 * If application want to back to previous entry, Application can call this API with current type and entry is -1.<br>
 * (e.g. If current type is IPOD_PLAYER_DB_TYPE_ALBUM in IPOD_PLAYER_DB_TYPE_ARTIST, Application can call iPodPlayerSelectDBEntry(devID, IPOD_PLAYER_DB_TYPE_ALBUM, -1)<br>
 */
S32 iPodPlayerSelectDBEntry(U32 devID, IPOD_PLAYER_DB_TYPE type, S32 entry)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SELECT_DB_ENTRY param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SELECT_DB_ENTRY;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.entry = entry;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerClearSelection(U32 devID, IPOD_PLAYER_DB_TYPE type)
 * This function is used in order to back to the previous database.<br>
 * Result of this API can be  known by receiving the #IPOD_PLAYER_CB_CLEAR_SELECTION<br>
 * If application want to back to top, application can use this function with #IPOD_PLAYER_DB_TYPE_ALL<br>
 * If application want to back to genre, application can use this function with #IPOD_PLAYER_DB_TYPE_GENRE<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_DB_TYPE. Current entry is cleared until selected type.
 * \warning
 * Index counter is initialized through a call to #iPodPlayerGetDBCount<br>
 * if type is set to #IPOD_PLAYER_DB_TYPE_ALL, database is cleared until top entry.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerClearSelection(U32 devID, IPOD_PLAYER_DB_TYPE type)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_CLEAR_SELECTION param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_CLEAR_SELECTION;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*!
 * \fn iPodPlayerSelectAV(U32 devID, U8 avType)
 * This function is used to select the database mode.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SELECT_AV_RESULT.<br>
 * If application calls this function and operation is success, current database category is cleared and becomes to top category.
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] avType Type is U8. It can select the Audio or Video. <br>
 * \li \c <b> #IPOD_PLAYER_AUDIO_MODE - database mode is audio. </b><br>
 * \li \c <b> #IPOD_PLAYER_VIDEO_MODE - database mode is video. </b><br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerSelectAV(U32 devID, U8 avType)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SELECT_AV param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SELECT_AV;
        param.header.appID = pid;

        param.header.devID = devID;
        param.avType = avType;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}


/*\}*/

/**
 * \addtogroup RemoteHMI HMI Control
 * Function of this group is used to operate the Apple device by external device but displayed HMI is transfered from Apple device.<br>
 * If application uses this group, application does not have to display itself. If can be displayed by Apple device.<br>
 * If application uses this group, application must change the mode to HIM control mode by using #iPodPlayerSetMode with #IPOD_PLAYER_MODE_HMI_CONTROL.<br>
 * Some apple devices does not support these function. Application can know whether Apple device support it or not by using #iPodPlayerGetDeviceProperty.<br>
 * Currently, this group does not support the touch device operation. It supports only button device, rotation device and event.<br>
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerHMISetSupportedFeature </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMIGetSupportedFeature </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMIButtonInput </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMIRotationInput </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMIPlaybackInput </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMISetApplicationStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMISetEventNotification </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMIGetEventChange </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerHMIGetDeviceStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the HMI Control function
 * \warning
 * If button, rotation or event operation use, application must call the same function at least 2 times.<br>
 * bacause the first function is used to send event, and after that, application must send the same function with release.<br>
 * Otherwise, iPodPlayer is continuing to send the event to apple device.<br>
 */
/*\{*/

/*!
 * \fn iPodPlayerHMISetSupportedFeature(U32 devID, U32 hmiSupportedMask)
 * This function is used to set the feature that displaies in HMI.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_SET_SUPPORTED_FEATURE_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] hmiSupportedMask Type is U32. Value that application sets the option for out mode. Meaning of these bit is below.<br>
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_AUDIO_CONTENTS
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_PHONE_CALL
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_SMS_MMS
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_VOICEMAIL
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_PUSH_NOTIFY
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_ALARM
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_TEST_PATTERN
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_MINIMAM_UI
 * \li \c #IPOD_PLAYER_HMI_FEATURE_MASK_FULL_UI
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_HMI_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerHMISetSupportedFeature(U32 devID, U32 hmiSupportedMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_HMI_SET_SUPPORTED_FEATURE param;
    S32 sockParam = -1;
    pid_t pid = getpid();
    
    /* For lint */
    devID = devID;
    hmiSupportedMask = hmiSupportedMask;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Initialize the structure */
        memset(&param, 0, sizeof(param));
    
    /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_HMI_SET_SUPPORTED_FEATURE;
    
    /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        param.bitmask = hmiSupportedMask;
    
    /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
    IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
    {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMIGetSupportedFeature(U32 devID, IPOD_PLAYER_HMI_FEATURE_TYPE type)
 * This function is used to get the feature that Apple device can display to HMI.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_GET_SUPPORTED_FEATURE_RESULT<br>
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_HMI_FEATURE_TYPE. It indicates the kind of getting feature.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function can be used only in #IPOD_PLAYER_MODE_HMI_CONTROL. Otherwise it will return an error of #IPOD_PLAYER_ERR_INVALID_MODE.<br>
 */
S32 iPodPlayerHMIGetSupportedFeature(U32 devID, IPOD_PLAYER_HMI_FEATURE_TYPE type)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_HMI_GET_SUPPORTED_FEATURE param;
    S32 sockParam = -1;
    pid_t pid = getpid();
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
    /* initialize */
        memset(&param, 0, sizeof(param));
    
    /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_HMI_GET_SUPPORTED_FEATURE;
    
    /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        param.type = type;
    
    /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
    IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
    {
            rc = IPOD_PLAYER_ERROR;
    }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    return rc;
}

/*!
 * \fn iPodPlayerHMIButtonInput(U32 devID, U32 eventMask, IPOD_PLAYER_HMI_BUTTON_SOURCE source)
 * This function is used to operate the Apple device by button device on HMI Control mode.<br>
 * Application can control the Apple device by two or more event.(e.g. Application can set the #IPOD_PLAYER_HMI_BUTTON_EVENT_UP and
 * If Application want to release the all event, Application must send this function is eventMask is 0, which means release.
 * #IPOD_PLAYER_HMI_BUTTON_EVENT_MENU at the same time.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_BUTTON_INPUT_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] eventMask Type is U32. Apple device is operated with selected event. Please see \ref HMIControlDefines<br>
 * \param [in] source Type is #IPOD_PLAYER_HMI_BUTTON_SOURCE. Apple device is operated from selected source.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function will be continuing the buttan press until this funciton is sent with eventMask is 0.
 */
S32 iPodPlayerHMIButtonInput(U32 devID, U32 eventMask, IPOD_PLAYER_HMI_BUTTON_SOURCE source)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_BUTTON_INPUT param;
    S32 sockParam = -1;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
    /* initialize */
        memset(&param, 0, sizeof(param));
    
    /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_BUTTON_INPUT;
    
    /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        param.event = eventMask;
        param.source = source;
    
    /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
    IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
    {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMIRotationInput(U32 devID, IPOD_PLAYER_HMI_ROTATION_INFO *info, U16 move)
 * This function is used to operate the Apple device by rotation device on Remote HMI mode.<br>
 * This API means that user press the button and immediately release.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_ROTATION_INPUT_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] info is #IPOD_PLAYER_HMI_ROTATION_INFO. Structure of rotation information.<br>
 * \param [in] move Type is U16. Rotation device moved until this value.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function will be tunning until this funciton is sent with info->action sets to #IPOD_PLAYER_HMI_ROTATION_ACTION_RELEASE.<br>
 */
S32 iPodPlayerHMIRotationInput(U32 devID, IPOD_PLAYER_HMI_ROTATION_INFO *info, U16 move)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_ROTATION_INPUT param;
    S32 sockParam = -1;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize */
        memset(&param, 0, sizeof(param));
        
        /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_ROTATION_INPUT;
    
        /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        
        if(NULL != info)
        {
            param.info.device = info->device;
            param.info.direction = info->direction;
            param.info.action = info->action;
            param.info.type = info->type;
            param.info.max = info->max;
            param.move = move;
        }
        else
        {
            return IPOD_PLAYER_ERROR;
        }
        
        /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
        IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMIPlaybackInput(U32 devID, IPOD_PLAYER_HMI_PLAYBACK_EVENT event)
 * This function is used to operate the Apple device by playback event on Remote HMI mode.<br>
 * This API means that user press the button and immediately release.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_PLAYBACK_INPUT_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] event is #IPOD_PLAYER_HMI_PLAYBACK_EVENT. playback event.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * This function will be continuing the event until this funciton is sent with info->action sets to #IPOD_PLAYER_HMI_PLAYBACK_EVENT_RELEASE.<br>
 */
S32 iPodPlayerHMIPlaybackInput(U32 devID, IPOD_PLAYER_HMI_PLAYBACK_EVENT event)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_PLAYBACK_INPUT param;
    S32 sockParam = -1;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize */
        memset(&param, 0, sizeof(param));
        
        /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_PLAYBACK_INPUT;
    
        /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        param.event = event;
        
        /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
        IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMISetApplicationStatus(U32 devID, IPOD_PLAYER_HMI_APP_STATUS status)
 * This function is used to operate the Apple device by playback event on Remote HMI mode.<br>
 * This API means that user press the button and immediately release.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_SET_APPLICATION_STATUS_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] status is #IPOD_PLAYER_HMI_APP_STATUS. application status.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerHMISetApplicationStatus(U32 devID, IPOD_PLAYER_HMI_APP_STATUS status)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_SET_APPLICATION_STATUS param;
    S32 sockParam;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize */
        memset(&param, 0, sizeof(param));
        
        /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_SET_APPLICATION_STATUS;
        
        /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        
        param.status = status;
        
        /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
        IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMISetEventNotification(U32 devID, U32 hmiEventNotificationMask)
 * This function is used to set the event notification in #IPOD_PLAYER_MODE_HMI_CONTROL.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_SET_EVENT_NOTIFICATION_RESULT.<br>
 * If Apple device change the some status(e.g track change, playback status change) and bitmask is set, iPodPlayer notifies the changed status to application by #IPOD_PLAYER_CB_NOTIFY_HMI_EVENT.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] hmiEventNotificationMask is bitmask. Set event bitmask. Please see \ref HMIEventMask.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerHMISetEventNotification(U32 devID, U32 hmiEventNotificationMask)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_SET_EVENT_NOTIFICATION param;
    S32 sockParam = -1;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize */
        memset(&param, 0, sizeof(param));
        
        /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_SET_EVENT_NOTIFICATION;
        
        /* set the value to data*/
        
        param.header.appID = pid;
        param.header.devID = devID;
        
        param.bitmask = hmiEventNotificationMask;
        
        /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
        IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMIGetEventChange(U32 devID)
 * This function is used to get the changed event from previous this function calling on Remote HMI mode.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_GET_EVENT_CHANGE_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerHMIGetEventChange(U32 devID)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_GET_EVENT_CHANGE param;
    S32 sockParam = -1;
    
    /* For lint */
    devID = devID;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize */
        memset(&param, 0, sizeof(param));
        
        /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_GET_EVENT_CHANGE;
        
        /* set the value to data*/
        param.header.appID = pid;
        param.header.devID = devID;
        
        /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
        IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerHMIGetDeviceStatus(U32 devID, IPOD_PLAYER_HMI_STATUS_TYPE type)
 * This function is used to get the Apple device status on Remote HMI mode.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_HMI_GET_DEVICE_STATUS_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type is #IPOD_PLAYER_HMI_STATUS_TYPE. device status type.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerHMIGetDeviceStatus(U32 devID, IPOD_PLAYER_HMI_STATUS_TYPE type)
{
    S32 rc = IPOD_PLAYER_ERROR;
    pid_t pid = getpid();
    IPOD_PLAYER_PARAM_HMI_GET_DEVICE_STATUS param;
    S32 sockParam = -1;
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(rc < 0)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize */
        memset(&param, 0, sizeof(param));
        
        /* set the value to data_head */
        param.header.funcId  = IPOD_FUNC_HMI_GET_DEVICE_STATUS;
        
        /* set the value to data*/
        
        param.header.appID = pid;
        param.header.devID = devID;
        param.type = type;
        
        /* send command */
        rc = iPodPlayerIPCSend(sockParam, (U8*)&param, sizeof(param), 0, -1);
        IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSendrc=%d\n",rc, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}
/*\}*/

/**
 * \addtogroup NonPlayerAPI NonPlayer
 * The following list shows that the functions of this group can be use by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerOpenSongTagFile </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerCloseSongTagFile </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSongTag </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSendToApp </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerRequestAppStart </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetPowerSupply </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetVolume </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetVolume </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerLocationInformation </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerVehicleStatus </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the Non Player function<br>
 */

/*\{*/

/*!
 * \fn iPodPlayerOpenSongTagFile(U32 devID, U32 tagOptionsMask, U32 optionLen, U8 *optionData)
 * This function is used to open the file which writes the song tag data<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_OPEN_SONG_TAG_RESULT<br>
 * Handle which is received by this function has effect until #iPodPlayerCloseSongTagFile is called or Apple device is disconnected<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device. The detail of each bit can refer in \ref SongTagOptionsMask.<br>
 * \param [in] tagOptionsMask Type is U32 bitmask. Meaning of each bit is blow:<br>
 * \param [in] optionLen Type is U32. This is the length of option data.<br>
 * \param [in] *optionData Type is U32 pointer. This is used when #IPOD_PLAYER_SONG_TAG_OPTIONS_BINALY is set in tagOptionsMask parameter.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * Maximum data length is 128 bytes. If optionLen is set the value more than 128, this function returns an error with #IPOD_PLAYER_ERR_INVALID_PARAMETER<br>
 * If #IPOD_PLAYER_SONG_TAG_OPTIONS_BINALY is set in tagOptionsMask, optionLen and optionData must set the valid value.<br>
 * Other bits does not use optionLen and optionData.
 */
S32 iPodPlayerOpenSongTagFile(U32 devID, U32 tagOptionsMask, U32 optionLen, U8 *optionData)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam = 0;
    IPOD_PLAYER_PARAM_OPEN_SONG_TAG_FILE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_OPEN_SONG_TAG_FILE;
        param.header.appID = pid;

        param.header.devID = devID;
        param.tagOptionsMask = tagOptionsMask;
        param.optionLen = optionLen;
        if(optionLen <= sizeof(param.optionData))
        {
            if(optionData != NULL)
            {
                memcpy(param.optionData, optionData, optionLen);
            }
            /* send command */
            rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
            if(rc != sizeof(param))
            {
                rc = IPOD_PLAYER_ERROR;
            }
            else
            {
                rc = IPOD_PLAYER_OK;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
        
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerCloseSongTagFile(U32 devID, U32 handle)
 * This function is used to close the file which is opened by #iPodPlayerOpenSongTagFile<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_CLOSE_SONG_TAG_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] handle Type is U32. This handle is the handle which received by #iPodPlayerOpenSongTagFile<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerCloseSongTagFile(U32 devID, U32 handle)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_CLOSE_SONG_TAG_FILE param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_CLOSE_SONG_TAG_FILE;
        param.header.appID = pid;

        param.header.devID = devID;
        param.handle = handle;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSongTag(U32 devID, U32 handle, IPOD_PLAYER_TAG_TYPE type, IPOD_PLAYER_TAG_INFO *info)
 * This function is used to write a tagging data<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SONG_TAG_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] handle Type is U32. This handle must set the value which received by #iPodPlayerOpenSongTagFile.
 * \param [in] type Type is #IPOD_PLAYER_TAG_TYPE This is kind of radio.<br>
 * \param [in] *info Type is #IPOD_PLAYER_TAG_INFO. This is structure of information of song tag.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerSongTag(U32 devID, U32 handle, IPOD_PLAYER_TAG_TYPE type, IPOD_PLAYER_TAG_INFO *info)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SONG_TAG param;
    
    if(info == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SONG_TAG;
        param.header.appID = pid;

        param.header.devID = devID;
        param.type = type;
        param.handle = handle;
        memcpy((char *)&param.info, (const char*)info, sizeof(param.info));
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSendToApp(U32 devID, U32 handle, U32 dataSize, U8 *data)
 * This function is used to send the data to iOS application.<br>
 * Application must know which is operated application. handle is equal to array number + 1 of appTable of #iPodPlayerInit.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SEND_TO_APP_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] handle Type is U32. Handle is used to send the data to target application. It is notified by #IPOD_PLAYER_CB_NOTIFY_OPEN_APP.<br>
 * \param [in] dataSize Type is U32. size of sending data<br>
 * \param [in] *data Type is U8 pointer. data that application sends<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerSendToApp(U32 devID, U32 handle, U32 dataSize, U8 *data)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SEND_TO_APP param;
    U8 *sendData[2] = {NULL};
    U32 sendSize[2] = {0};
    U8 sendNum = 2;
    U32 asize = 0;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));

    if (data == NULL)
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    else
    {
        /* set the value to sendSockParam */
        rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG);
        if(IPOD_PLAYER_OK != rc)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        /* Send the deinit command to the iPodPlayer Core */
        if(rc == IPOD_PLAYER_OK)
        {
            /* set the value to data_head */
            param.header.funcId = IPOD_FUNC_SEND_TO_APP;
            param.header.appID = pid;
            param.header.longData = 1;

            param.header.devID = devID;
            param.handle = handle;
            param.dataSize = dataSize;
            
            sendData[0] = (U8 *)&param;
            sendSize[0] = sizeof(param);
            sendData[1] = data;
            sendSize[1] = dataSize;
            
            /* send command */
            rc = iPodPlayerIPCLongSend(sendSockParam, sendNum, sendData, sendSize, &asize, 0, IPOD_PLAYER_LONG_SEND_TMOUT);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerRequestAppStart(U32 devID, U8 *appName)
 * This function is used to launch the iOS application.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_REQUEST_APP_START
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] *appName Type is U8 pointer. This is iOS Application name. It is NULL terminated UTF-8 string. Also this is restricted by 256 bytes.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerRequestAppStart(U32 devID, U8 *appName)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_REQUEST_APP_START param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));

    if (appName == NULL)
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    else
    {
        /* set the value to sendSockParam */
        rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
        if(IPOD_PLAYER_OK != rc)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        /* Send the deinit command to the iPodPlayer Core */
        if(rc == IPOD_PLAYER_OK)
        {
            /* set the value to data_head */
            param.header.funcId = IPOD_FUNC_REQUEST_APP_START;
            param.header.appID = pid;

            param.header.devID = devID;
            if(sizeof(param.appName) > strnlen((const char *)appName, IPOD_PLAYER_STRING_LEN_MAX))
            {
                strncpy((char *)param.appName, (const char *)appName, IPOD_PLAYER_STRING_LEN_MAX);
                param.appName[IPOD_PLAYER_STRING_LEN_MAX - 1] = '\0';
            }
            else
            {
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* send command */
                rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
                if(rc != sizeof(param))
                {
                    rc = IPOD_PLAYER_ERROR;
                }
                else
                {
                    rc = IPOD_PLAYER_OK;
                }
            }
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetPowerSupply(U32 devID, IPOD_PLAYER_CURRENT powermA, U8 chargeButtery)
 * This function is used to set the poewr supply to Apple device with MA.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_POWER_SUPPLY_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] powermA Type is #IPOD_PLAYER_CURRENT. This value must be one of the following values.
 * \param [in] chargeButtery Type is U8. This value is used to judgment that charge buttery.
 * If this value is set to 1, iPodPlayer charges power to internal buttery, Or if it is 0, device isn't charged   
 * \retval #IPOD_PLAYER_OK                      Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \retval #IPOD_PLAYER_ERR_INVALID_PARAMETER   The argument is invalid parameter.
 * \warning
 */
S32 iPodPlayerSetPowerSupply(U32 devID, IPOD_PLAYER_CURRENT powermA, U8 chargeButtery)
{
    S32 rc  = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    pid_t pid = getpid();
    IPOD_PLAYER_CURRENT checkTable[] = {IPOD_PLAYER_CURRENT_0mA,    IPOD_PLAYER_CURRENT_500mA,
                                        IPOD_PLAYER_CURRENT_1000mA, IPOD_PLAYER_CURRENT_1500mA,
                                        IPOD_PLAYER_CURRENT_2100mA, IPOD_PLAYER_CURRENT_2400mA };
    IPOD_PLAYER_PARAM_SET_POWER_SUPPLY param;
    S32 sendSockParam;
    size_t i = 0;
   
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));

    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
   
    if(rc == IPOD_PLAYER_OK)
    {
        /* check current parameter */
        rc  = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        for(i = 0; i < sizeof(checkTable)/sizeof(checkTable[0]); i++)
        {
           if(checkTable[i] == powermA)
           {
                rc = IPOD_PLAYER_OK;
                break;
           }
        }

        if(rc == IPOD_PLAYER_OK)
        {
            /* set the value to data_head */
            param.header.funcId = IPOD_FUNC_SET_POWER_SUPPLY;
            param.header.appID = pid;
            param.header.devID = devID;

            /* set the value to power source supply */
            param.powermA = powermA;
            param.chargeButtery = chargeButtery;

            /* send command */
            rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
            if(rc != sizeof(param))
            {
                rc = IPOD_PLAYER_ERROR;
            }
            else
            {
                rc = IPOD_PLAYER_OK;
            }
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetVolume(U32 devID, U8 volume)
 * This function is used to set the volume of target sound.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_VOLUME_RESULT. <br>
 * This function has an effect only when audio mode is #IPOD_PLAYER_SOUND_MODE_ON. <br>
 * Otherwise this function returns an error.<br>
 * audio mode can change by #iPodPlayerSetAudioMode<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] volume Type is U8. This is only valid between from 0 to 100. 0 means the mute. 100 mean the maximum volume.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If audio mode is changed to #IPOD_PLAYER_SOUND_MODE_ON, current volume is reset.<br>
 * 
 */
S32 iPodPlayerSetVolume(U32 devID, U8 volume)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_VOLUME param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_VOLUME;
        param.header.appID = pid;

        param.header.devID = devID;
        param.volume = volume;

        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerGetVolume(U32 devID)
 * This function is used to get the volume of target sound.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_VOLUME_RESULT. <br>
 * This function has an effect only when audio mode is #IPOD_PLAYER_SOUND_MODE_ON. <br>
 * Otherwise this function returns an error.<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerGetVolume(U32 devID)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_GET_VOLUME param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_GET_VOLUME;
        param.header.appID = pid;

        param.header.devID = devID;
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetLocationInformation(U32 devID, size_t size, U8 *data)
 * This function is used to send the location information data to apple device.<br>
 * Application must use NMEA sentences.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_LOCATION_INFO_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] size Type is size_t. size of sending NMEA sentence<br>
 * \param [in] *NMEAdata Type is U8 pointer. NMEA sentence data that application sends<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerSetLocationInformation(U32 devID, size_t size, U8 *NMEAdata)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_LOCATION_INFO param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));

    if (NMEAdata == NULL)
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    else
    {
        /* set the value to sendSockParam */
        rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
        if(IPOD_PLAYER_OK != rc)
        {
            rc = IPOD_PLAYER_ERROR;
        }
        
        /* Send the deinit command to the iPodPlayer Core */
        if(rc == IPOD_PLAYER_OK)
        {
            /* set the value to data_head */
            param.header.funcId = IPOD_FUNC_SET_LOCATION_INFO;
            param.header.appID = pid;
            param.header.devID = devID;
            param.size = size;

            if(sizeof(param.NMEAdata) < size)
            {
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            }
            else
            {
                memcpy(param.NMEAdata, NMEAdata, size);
            }
            
            if(rc == IPOD_PLAYER_OK)
            {
                /* send command */
                rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
                if(rc != sizeof(param))
                {
                    rc = IPOD_PLAYER_ERROR;
                }
                else
                {
                    rc = IPOD_PLAYER_OK;
                }
            }
        }
    }
    
    return rc;
}

/*!
 * \fn iPodPlayerSetVehicleStatus(U32 devID, IPOD_PLAYER_SET_VEHICLE_STATUS *status)
 * This function is used to send the vehicle status data to apple device.<br>
 * Application must use following Sensor data.<br>
 *  - Range                :remaining vehicle range in kilometers.<br>
 *  - Outside Temperature  :measured outside temperature in C.<br>
 *  - Range Warning        :if True, the vehicle's low range warning indicator is set.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_VEHICLE_STATUS_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] data Type is IPOD_PLAYER_SET_VEHICLE_DATA structure.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerSetVehicleStatus(U32 devID, IPOD_PLAYER_SET_VEHICLE_STATUS *status)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    IPOD_PLAYER_PARAM_SET_VEHICLE_STATUS param;
    
    /* Initialize the structure */
    memset(&param, 0, sizeof(param));
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_VEHICLE_STATUS;
        param.header.appID = pid;

        param.header.devID = devID;
        param.status = *status;
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

#if 0
/**
 * \addtogroup InteligentAPI Ingeligent PlayList API
 * Function of this group is used to operate the inteligent playlist.<br>
 * By using this function, application can create/reflsh inteligent playlist.<br>
 * Inteligent playlist means that is list collected track approximate the indicated track<br>
 * The following list showns that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerCreateIntelligentPL </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerTrackSupportsIntelligentPL </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerRefreshIntelligentPL </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerGetPLProperties </TD>
 *      <TD> Invalid </TD>
 *      <TD> Valid </TD>
 *      <TD> Invalid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the Non Player function<br>
 */
/*\{*/

/*!
 * \fn iPodPlayerCreateIntelligentPL(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID)
 * This function is used to create the intelligent playlist. <br>
 * This function creates the playlist which has a track approximate indicated track.
 * also if this function success, playback status is changed to play.
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_CREATE_INTELLIGENT_PL_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Type is DB or PB.<br>
 * \param [in] trackID Type is U64. Create the playlist by indicated trackID.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * type can use #IPOD_PLAYER_TRACK_TYPE_PLAYBACK or #IPOD_PLAYER_TRACK_TYPE_DATABASE.<br>
 * #IPOD_PLAYER_TRACK_TYPE_UID must not use. otherwise this function returns an error of #IPOD_PLAYER_ERR_INVALID_PARAMETER.
 */
S32 iPodPlayerCreateIntelligentPL(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID)
{
    S32 rc = IPOD_PLAYER_OK;
    
    /* For lint */
    devID = devID;
    type = type;
    trackID = trackID;

    rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
    
    return rc;
}
/*!
 * \fn iPodPlayerTrackSupportsIntelligentPL(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID)
 * This function is used to confirm whether indicated track can create the intelligent playlist.
 * If indicated track can create the playlist, application can use #iPodPlayerCreateIntelligentPL to create the intelligent playlist with this track.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_TRACK_SUPPORTS_INTELLIGENT_PL_RESULT
 * \param [in] devID - Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_TRACK_TYPE. Type is DB or PB.<br>
 * \param [in] trackID Type is U64. Confirm whether indicated track can create the intelligent playlist.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerTrackSupportsIntelligentPL(U32 devID, IPOD_PLAYER_TRACK_TYPE type, U64 trackID)
{
    S32 rc = IPOD_PLAYER_OK;
    
    /* For lint */
    devID = devID;
    type = type;
    trackID = trackID;
    
    rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
    
    return rc;
}

/*!
 * \fn iPodPlayerRefreshIntelligentPL(U32 devID, U32 plIndex)
 * This function is used to reflesh the created inteligenet playlist.<br>
 * If application use this function, created intelligent playlist is removed.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_REFRESH_INTELLIGENT_PL_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] plIndex Type is U32. This index is index of created playlist.
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerRefreshIntelligentPL(U32 devID, U32 plIndex)
{
    S32 rc = IPOD_PLAYER_OK;
    /* For lint */
    devID = devID;
    plIndex = plIndex;

    rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;

    return rc;
}

/*!
 * \fn iPodPlayerGetPLProperties(U32 devID, U32 plIndex, U32 plPropertyMask)
 * This function is used to get the properties of created inteligenet playlist.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_GET_PL_PROPERTIES_RESULT<br>
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] plIndex Type is U32. This index is used to get the property.<br>
 * \param [in] plPropertyMask Type is U32. This mask is used to get the properties. See \ref IntelligentPropertyMask<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 */
S32 iPodPlayerGetPLProperties(U32 devID, U32 plIndex, U32 plPropertyMask)
{
    S32 rc = IPOD_PLAYER_OK;
    /* For lint */
    devID = devID;
    plIndex = plIndex;
    plPropertyMask = plPropertyMask;

    rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;

    return rc;
}

/*\}*/

/**
 * \addtogroup GPSSupport GPS Support
 * The following list shows that the functions of this group can be used by which mode.<br>
 * <TABLE>
 * <TR>
 *      <TD>Function Name / Mode </TD>
 *      <TD> #IPOD_PLAYER_MODE_SELF_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_REMOTE_CONTROL</TD>
 *      <TD> #IPOD_PLAYER_MODE_HMI_CONTROL</TD>
 * <TR>
 *      <TD> #iPodPlayerSetGPSData </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * <TR>
 *      <TD> #iPodPlayerSetGPSCurrentSystemTime </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 *      <TD> Valid </TD>
 * </TR>
 * </TABLE>
 *
 * \author M.Shibata
 * \date 29/Feb/2012 Initialize the GPS Support function<br>
 */
/*\{*/

/*!
 * \fn iPodPlayerSetGPSData(U32 devID, IPOD_PLAYER_GPS_TYPE type, U32 dataSize, U8 *data)
 * This function is used to set the GPS data to Apple device.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_GPS_DATA_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] type Type is #IPOD_PLAYER_GPS_TYPE Type of GPS data.<br>
 * \param [in] dataSize Type is U32. Size of sending data<br>
 * \param [in] *data Type is U8. pointer Sending data.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 * \warning
 * If type is set with #IPOD_PLAYER_GPS_TYPE_EPHEMERIS, data must be NULL-terminated URL string. also URL must include "http://".<br>
 */
S32 iPodPlayerSetGPSData(U32 devID, IPOD_PLAYER_GPS_TYPE type, U32 dataSize, U8 *data)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* For lint */
    devID = devID;
    type = type;
    dataSize = dataSize;
    data = data;
    
#if 1
    size_t mSize = 0;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS* pdata = NULL;
    S32 sockParam;
    
    IPOD_PLAYER_DEBUG_PRINT("START\n");
    /* calculate memory allocation size */
    mSize = sizeof(pdata->info)+sizeof(pdata->param.setLocationInfo);
    IPOD_PLAYER_DEBUG_PRINT("mSize = %d\n",mSize);
    pdata = malloc(mSize);
    if(NULL == pdata)
    {
        return IPOD_PLAYER_ERROR;
    }
    
    /* initialize */
    memset((U8*)pdata, 0, mSize);
    
    /* set the value to data_head */
    pdata->info.funcId = IPOD_FUNC_SET_GPS_DATA;
    pdata->info.callback = 0;
    
    /* set the value to data*/
    pdata->param.setLocationInfo.devID = devID;
    pdata->param.setLocationInfo.infoType = infoType;
    pdata->param.setLocationInfo.dataType = dataType;
    pdata->param.setLocationInfo.size = size;
    IPOD_PLAYER_DEBUG_PRINT("sizeof = %d\n",size);
    memcpy(pdata->param.setLocationInfo.data, data, size);
    
    /* set the value to sockParam */
    rc = iPodPlayerGetSocketInfo(&sockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    
    /* send command */
    rc = iPodPlayerIPCSend(sockParam, (U8*)pdata, mSize, 0, -1);
    IPOD_PLAYER_DEBUG_PRINT("iPodPlayerIPCSend rc=%d\n",rc);
    if(rc < 0)
    {
        free(pdata);
        return IPOD_PLAYER_ERROR;
    }
    
    free(pdata);
#endif
    return IPOD_PLAYER_OK;
}

/*!
 * \fn iPodPlayerSetGPSCurrentSystemTime(U32 devID, IPOD_PLAYER_GPS_TIME *gpsTime)
 * This function is used to set the current system time to Apple device in week time.<br>
 * Result of this API can be known by receiving the #IPOD_PLAYER_CB_SET_GPS_CURRENT_SYSTEM_TIME_RESULT
 * \param [in] devID Type is U32. This ID is used to decide the operating Apple device.<br>
 * \param [in] *gpsTime Type is #IPOD_PLAYER_GPS_TIME pointer This structure is set by week time.<br>
 * \retval #IPOD_PLAYER_OK Function success
 * \retval #IPOD_PLAYER_ERROR Function fail.
 */
S32 iPodPlayerSetGPSCurrentSystemTime(U32 devID, IPOD_PLAYER_GPS_TIME *gpsTime)
{
    S32 rc = IPOD_PLAYER_OK;
    
    /* For lint */
    devID = devID;
    gpsTime = gpsTime;
    return rc;
}
/*\}*/

#endif

S32 iPodPlayerSetDeviceDetection(U32 devID, IPOD_PLAYER_DEVICE_DETECTION_INFO *info)
{
    S32 rc = IPOD_PLAYER_OK;
    pid_t pid = getpid();
    S32 sendSockParam;
    static IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION param;
    U32 i = 0;
    
    if(info == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    if(info->devInfo != NULL){
        if(info->devInfo->macCount > IPOD_PLAYER_BT_MAC_COUNT_MAX)
        {
            return IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
    }
    
    /* set the value to sendSockParam */
    rc = iPodPlayerGetSocketInfo(&sendSockParam, (U8)IPOD_PLAYER_OPEN_SOCKET_CLIENT);
    if(IPOD_PLAYER_OK != rc)
    {
        rc = IPOD_PLAYER_ERROR;
    }
    
    /* Send the deinit command to the iPodPlayer Core */
    if(rc == IPOD_PLAYER_OK)
    {
        /* set the value to data_head */
        param.header.funcId = IPOD_FUNC_SET_DEVICE_DETECTION;
        param.header.appID = pid;

        param.header.devID = devID;
        memcpy(&param.info, info, sizeof(param.info));

        /* set accessory information data */
        memset(&param.accInfo, 0, sizeof(param.accInfo)); 

        if(info->accInfo != NULL)
        {
            if(info->accInfo->Name != NULL)
            {
                strncpy((char *)param.accInfo.Name, (char *)(info->accInfo->Name), IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->SerialNumber != NULL)
            {
                strncpy((char *)param.accInfo.SerialNumber, (char *)info->accInfo->SerialNumber, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->ModelNumber != NULL)
            {
                strncpy((char *)param.accInfo.ModelNumber, (char *)info->accInfo->ModelNumber, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->Manufacturer != NULL)
            {
                strncpy((char *)param.accInfo.Manufacturer, (char *)info->accInfo->Manufacturer, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->VendorId != NULL)
            {
                strncpy((char *)param.accInfo.VendorId, (char *)info->accInfo->VendorId, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->ProductId != NULL)
            {
                strncpy((char *)param.accInfo.ProductId, (char *)info->accInfo->ProductId, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->BCDDevice != NULL)
            {
                strncpy((char *)param.accInfo.BCDDevice, (char *)info->accInfo->BCDDevice, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->ProductPlanUUID != NULL)
            {
                strncpy((char *)param.accInfo.ProductPlanUUID, (char *)info->accInfo->ProductPlanUUID, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }

            param.accInfo.Hardware_version = info->accInfo->Hardware_version;   
            param.accInfo.Software_version = info->accInfo->Software_version;

            if(info->accInfo->Hardware_version_iap2 != NULL)
            {
                strncpy((char *)param.accInfo.Hardware_version_iap2 ,(char *)info->accInfo->Hardware_version_iap2, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }
            if(info->accInfo->Software_version_iap2 != NULL)
            {
                strncpy((char *)param.accInfo.Software_version_iap2 ,(char *)info->accInfo->Software_version_iap2, IPOD_PLAYER_ACC_INFO_STRING_LEN_MAX);
            }

            /* flag for ios in the car(carPlay) */
            param.accInfo.SupportedIOSInTheCar = info->accInfo->SupportedIOSInTheCar;

            /* ios information */
            param.accInfo.SupportediOSAppCount = info->accInfo->SupportediOSAppCount;
            /* maximum check */
            if(param.accInfo.SupportediOSAppCount > IPODCORE_MAX_IOSAPPS_INFO_NUM)
            {
                param.accInfo.SupportediOSAppCount = IPODCORE_MAX_IOSAPPS_INFO_NUM;    
            }
            for(i = 0; i < param.accInfo.SupportediOSAppCount; i++)
            {
                param.iosInfo[i].EANativeTransport = info->accInfo->iOSAppInfo[i].EANativeTransport;
                param.iosInfo[i].MatchAction = info->accInfo->iOSAppInfo[i].MatchAction;
                param.iosInfo[i].iOSAppIdentifier = info->accInfo->iOSAppInfo[i].iOSAppIdentifier;
                if(info->accInfo->iOSAppInfo[i].iOSAppName != NULL)
                {
                    strncpy((char *)param.iosInfo[i].iOSAppName, (char *)info->accInfo->iOSAppInfo[i].iOSAppName, IPOD_PLAYER_STRING_LEN_MAX); 
                }
            }
            
            /* mfi message code for identification information */
            param.accInfo.MsgSentByAccSize = info->accInfo->MsgSentByAcc->msgnum;
            /* maximum check */
            if(param.accInfo.MsgSentByAccSize > IPOD_PLAYER_IDEN_MSGCODE_MAX)
            {
                param.accInfo.MsgSentByAccSize = IPOD_PLAYER_IDEN_MSGCODE_MAX;
            }
            for(i = 0; i < (U32)param.accInfo.MsgSentByAccSize / sizeof(U16); i++)
            {
                param.accInfo.MsgSentByAcc[i] = info->accInfo->MsgSentByAcc->msgcodes[i];
            }
            
            param.accInfo.MsgRecvFromDeviceSize = info->accInfo->MsgRecvFromDevice->msgnum;
            /* muximum check */
            if(param.accInfo.MsgRecvFromDeviceSize > IPOD_PLAYER_IDEN_MSGCODE_MAX)
            {
                param.accInfo.MsgRecvFromDeviceSize = IPOD_PLAYER_IDEN_MSGCODE_MAX;
            }
            for(i = 0; i < (U32)param.accInfo.MsgRecvFromDeviceSize / sizeof(U16); i++)
            {
                param.accInfo.MsgRecvFromDevice[i] = info->accInfo->MsgRecvFromDevice->msgcodes[i];
            }
        }
        else
        {
            param.accInfo.MsgRecvFromDeviceSize = IPOD_PLAYER_ACCINFO_NULL;
            param.accInfo.MsgSentByAccSize = IPOD_PLAYER_ACCINFO_NULL;
            param.accInfo.SupportediOSAppCount = 0;
        }

        /* set bluetooth device information data */
        if(info->devInfo != NULL)
        {
            memcpy(&param.devInfo, info->devInfo, sizeof(param.devInfo));
            param.info.devInfo = &param.devInfo;
            memcpy(&param.macAddr, info->devInfo->macAddr, sizeof(param.macAddr[0]) * info->devInfo->macCount);
            param.devInfo.macAddr = param.macAddr;
        }
        else
        {
            param.devInfo.macCount = 0;
        }

        /* set vehicle infomation data */
        if((info->vehicleInfo != NULL) && (info->vehicleInfo->displayName != NULL))
        {
            param.vehicleInfo.displayName_valid = TRUE;
            strncpy((char *)param.vehicleInfo.displayName, (char *)info->vehicleInfo->displayName, IPOD_PLAYER_STRING_LEN_MAX);
        }
        else
        {
            param.vehicleInfo.displayName_valid = FALSE;
        }
        
        /* send command */
        rc = iPodPlayerIPCSend(sendSockParam, (U8*)&param, sizeof(param), 0, -1);
        if(rc != sizeof(param))
        {
            rc = IPOD_PLAYER_ERROR;
        }
        else
        {
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}
