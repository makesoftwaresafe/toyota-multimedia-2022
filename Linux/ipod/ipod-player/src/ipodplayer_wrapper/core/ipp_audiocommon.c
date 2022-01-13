#include "ipp_audiocommon.h"
#include "ipp_audio_common.h"
#include "iPodPlayerCoreFunc.h"

S32 iPodCoreiPodCtrlAudioInitIPC(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_IPC_OPEN_INFO info;
    U8 temp[IPOD_PLAYER_STRING_LEN_MAX] = {0};
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (iPodCtrlCfg->threadInfo == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    
    /* Prepare to open the socket client for communication with audio server */
    info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
    info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
    info.identify = (U8 *)IPP_AUDIO_SOCK_PATH;
    info.connectionNum = IPODCORE_AUDIO_SERVER_CONNECTION_NUM;
    info.semName = NULL;
    info.maxBufSize = iPodCtrlCfg->threadInfo->maxMsgSize;
    info.maxPacketSize = iPodCtrlCfg->threadInfo->maxMsgSize;
    /* Connect socket to audio Server */
    rc = iPodPlayerIPCOpen(&iPodCtrlCfg->sckAudio, &info);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Prepare to open the socket server for communication with audio server */
        info.type = IPOD_PLAYER_OPEN_SOCKET_SERVER;
        snprintf((char *)temp, sizeof(temp), "%s_%d", IPODCORE_AUDIO_SERVER_IDENTIFY, iPodCtrlCfg->threadInfo->appDevID);
        info.identify = temp;
        rc = iPodPlayerIPCOpen(&iPodCtrlCfg->sckAudioServer, &info);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            /* Register the handle to the handl table */
            rc = iPodCoreSetHandle(iPodCtrlCfg->handle, &iPodCtrlCfg->handleNum, iPodCtrlCfg->sckAudioServer);
        }
        else
        {
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc, info.type);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc, info.type);
        rc = IPOD_PLAYER_ERROR;
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        /* Server was created */
        if(iPodCtrlCfg->sckAudioServer >= 0)
        {
            iPodPlayerIPCClose(iPodCtrlCfg->sckAudioServer);
            iPodCtrlCfg->sckAudioServer = -1;
        }
        
        /* Client was created */
        if(iPodCtrlCfg->sckAudio >= 0)
        {
            iPodPlayerIPCClose(iPodCtrlCfg->sckAudio);
            iPodCtrlCfg->sckAudio = -1;
        }
    }
    
    return rc;
}
    
void iPodCoreiPodCtrlAudioDeinitIPC(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return;
    }
    
    /* Socket server was opened */
    if(iPodCtrlCfg->sckAudioServer >= 0)
    {
        iPodCoreClearHandle(iPodCtrlCfg->handle, &iPodCtrlCfg->handleNum, iPodCtrlCfg->sckAudioServer);
        rc = iPodPlayerIPCClose(iPodCtrlCfg->sckAudioServer);
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        iPodCtrlCfg->sckAudioServer = -1;
    }
    
    /* Socket client was opened */
    if(iPodCtrlCfg->sckAudio >= 0)
    {
        rc = iPodPlayerIPCClose(iPodCtrlCfg->sckAudio);
        if(rc != IPOD_PLAYER_IPC_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
        iPodCtrlCfg->sckAudio = -1;
    }
    
    return;
}

