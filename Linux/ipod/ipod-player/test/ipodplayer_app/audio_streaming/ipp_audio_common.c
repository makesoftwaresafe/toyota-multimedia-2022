#include <sys/eventfd.h>

#include "iPodPlayerIPCLib.h"
#include "iPodPlayerUtilityLog.h"
#include "iPodPlayerUtilityConfiguration.h"
#include "ipp_audio_common.h"

static volatile sig_atomic_t g_eventHandle = -1;

/**
 * void ipp_audio_signal(S32 para)
 *
 * signal handler for SIGTERM and SIGINT
 *
 */
static void ipp_audio_signal(S32 para)
{
    para = para;
    U64 data = 1;
    int rc = IPP_AUDIO_OK;
    
    /* send end event */
    rc = write(g_eventHandle, &data, sizeof(data));
    if(rc < 0)
    {
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_INFORMATION, IPOD_LOG_PLAYER_AUDIO, rc);
    }

    return;
}

/**
 * int iPodAudioSetCfg(void)
 *
 * Set the configuration by other process.
 *
 */
void iPodAudioSetCfg(IPOD_AUDIO_PLAYINFO *pinfo, IPOD_PLAYER_PARAM_PAI_CFG_TABLE *cfgTable, U32 devID)
{
    int rc = IPP_AUDIO_ERROR;
    IPOD_PLAYER_IPC_OPEN_INFO info;
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    U8 temp[IPP_AUDIO_INFO_IDENTIFY_MAX] = {0};
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Parameter check */
    if((pinfo == NULL) || (cfgTable == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, pinfo, cfgTable);
        return;
    }
    
    /* Initialize the structure */
    memset(&info, 0, sizeof(info));
    memset(&paiResult, 0, sizeof(paiResult));
    
    /* Set handle */
    pinfo->id = devID;
    
    info.type = IPOD_PLAYER_OPEN_SOCKET_CLIENT;
    info.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
    snprintf((char *)temp, sizeof(temp), "%s_%d", IPP_AUDIO_INFO_IDENTIFY, devID);
    info.identify = temp;
    info.connectionNum = 1;
    info.semName = NULL;
    info.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
    info.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
    /* Open ipc for client */
    rc = iPodPlayerIPCOpen(&pinfo->audioSckClient, &info);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        rc = IPP_AUDIO_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, rc);
        rc = IPP_AUDIO_ERR_IPC;
    }
    
    /* Set configuration */
    memcpy(&pinfo->cfgTable, cfgTable, sizeof(pinfo->cfgTable));
    
    /* 0: Command, 1-4: Result of this function with 4 bytes */
    paiResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_PAI_RESULT;
    paiResult.cmdId = IPOD_FUNC_PAI_SETCFG;
    paiResult.result = rc;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiResult, sizeof(paiResult), 0, -1);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
    
    return;
}

void iPodAudioClearCfg(IPOD_AUDIO_PLAYINFO *pinfo, S32 waitHandle)
{
    U32 resNum = 0;
    S32 resHandle = 0;
    U8 retType = 0;
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    
    /* Output buffer pointer check */
    if(pinfo == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID);
        return;
    }
    
    /* Initialize the structure */
    memset(&paiResult, 0, sizeof(paiResult));
    
    /* 0: Command, 1-4: Result of this function with 4 bytes */
    paiResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_PAI_RESULT;
    paiResult.cmdId = IPOD_FUNC_PAI_CLRCFG;
    paiResult.result = IPP_AUDIO_OK;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiResult, sizeof(paiResult), 0, -1);
    iPodPlayerIPCWait(waitHandle, 1, &pinfo->audioSckClient, &resNum, &resHandle, &retType, IPOD_AUDIO_WAIT_SEND, IPOD_PLAYER_WAIT_IN);
    iPodPlayerIPCClose(pinfo->audioSckClient);
    
    memset(&pinfo->cfgTable, 0, sizeof(pinfo->cfgTable));
    pinfo->audioSckClient = -1;
    pinfo->id = -1;
    
    return;
}

/**
 * void iPodAudioStop(void)
 *
 * Stop Audio.
 *
 */
void iPodAudioStop(IPOD_AUDIO_PLAYINFO *pinfo, IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Parameter check */
    if((pinfo == NULL) || (table == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, pinfo, table);
        return;
    }
    
    if(table->stop != NULL)
    {
        table->stop(pinfo->audioInfo);
        pinfo->audioInfo = NULL;
    }
    
    /* 0:command, 1-4: Result of this function with 4bytes. */
    paiResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_PAI_RESULT;
    paiResult.cmdId = IPOD_FUNC_PAI_STOP;
    paiResult.result = IPP_AUDIO_OK;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiResult, sizeof(paiResult), 0, -1);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
    
    return;
}

/**
 * void iPodAudioStart(void)
 *
 * Start ALSA.
 *
 */
void iPodAudioStart(IPOD_AUDIO_PLAYINFO *pinfo, S32 rate, IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    S32 rc = IPP_AUDIO_ERROR;
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Parameter check */
    if((pinfo == NULL) || (table == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, pinfo, table);
        return;
    }
    
    /* Initialize the structure */
    memset(&paiResult, 0, sizeof(paiResult));
    
    if(table->start != NULL)
    {
        rc = table->start(&pinfo->audioInfo, (U8 *)pinfo->cfgTable.src_name, (U8 *)pinfo->cfgTable.sink_name, rate);
    }
    
    /* 0: command, 1-4: Result of this function with 4 bytes. */
    paiResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_PAI_RESULT;
    paiResult.cmdId = IPOD_FUNC_PAI_START;
    paiResult.result = rc;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiResult, sizeof(paiResult), 0, -1);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
}

/**
 * void iPodAudioSetSamplerate(void)
 *
 * set sample rate.
 *
 */
void iPodAudioSetSamplerate(IPOD_AUDIO_PLAYINFO *pinfo, U32 rate, IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    int rc = IPP_AUDIO_ERROR;
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Paramter check */
    if((pinfo == NULL) || (table == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, pinfo, table);
        return;
    }
    
    /* Initialize the structure */
    memset(&paiResult, 0, sizeof(paiResult));
    
    if(table->setSample != NULL)
    {
        rc = table->setSample(pinfo->audioInfo, rate);
    }
    
    paiResult.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_PAI_RESULT;
    paiResult.cmdId = IPOD_FUNC_PAI_SETSR;
    paiResult.result = rc;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiResult, sizeof(paiResult), 0, -1);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
}

void iPodAudioSetVolume(IPOD_AUDIO_PLAYINFO *pinfo, U8 volume, U32 appID, IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    int rc = IPP_AUDIO_ERROR;
    IPOD_PLAYER_PARAM_PAI_RESULT paiResult;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Parameter check */
    if((pinfo == NULL) || (table == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, pinfo, table);
        return;
    }
    
    /* Initialize the structure */
    memset(&paiResult, 0, sizeof(paiResult));
    
    if(table->setVolume != NULL)
    {
        rc = table->setVolume(pinfo->audioInfo, volume);
    }
    
    paiResult.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_SET_VOLUME);
    paiResult.result = rc;
    paiResult.header.appID = appID;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiResult, sizeof(paiResult), 0, -1);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
}

void iPodAudioGetVolume(IPOD_AUDIO_PLAYINFO *pinfo, U32 appID, IPOD_AUDIO_COM_FUNC_TABLE *table)
{
    int rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_PAI_GET_VOLUME paiGetVolume;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    /* Parameter check */
    if((pinfo == NULL) || (table == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, pinfo, table);
        return;
    }
    
    /* Initialize the structure */
    memset(&paiGetVolume, 0, sizeof(paiGetVolume));
    
    if(table->getVolume != NULL)
    {
        rc = table->getVolume(pinfo->audioInfo, &paiGetVolume.volume);
    }
    
    paiGetVolume.header.funcId = (IPOD_PLAYER_FUNC_ID)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_VOLUME);
    paiGetVolume.header.appID = appID;
    paiGetVolume.result = rc;
    
    iPodPlayerIPCSend(pinfo->audioSckClient, (U8 *)&paiGetVolume, sizeof(paiGetVolume), 0, -1);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
}

IPOD_AUDIO_PLAYINFO *iPodAudioGetDeviceInfo(IPOD_AUDIO_INFO *info, S32 id)
{
    IPOD_AUDIO_PLAYINFO *pinfo = NULL;
    U32 i = 0;
    S32 empty = -1;
    
    /* Parameter check */
    if((info == NULL) || (info->pinfo == NULL) || (id == -1))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, info, id);
        return NULL;
    }
    
    /* Loop until maximum number of device */
    for(i = 0; (i < info->devMax) && (pinfo == NULL); i++)
    {
        /* Check id */
        if(info->pinfo[i].id == id)
        {
            /* Found the device */
            pinfo = &info->pinfo[i];
        }
        else if((info->pinfo[i].id == -1) && (empty == -1))
        {
            empty = i;
        }
    }
    
    if((pinfo == NULL) && (empty != -1))
    {
        /* New device is registered */
        pinfo = &info->pinfo[empty];
    }
    
    return pinfo;
}

S32 iPodAudioMain(IPOD_AUDIO_INFO *info)
{
    int rc = IPOD_PLAYER_ERROR;
    IPOD_AUDIO_PLAYINFO *pinfo = NULL;
    U32 size = 0;
    IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents = NULL;
    S32 handles[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    U32 handleNum = 1;
    S32 resHandle[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    U32 resNum = 0;
    U8 i = 0;
    U8 retType[IPOD_PLAYER_IPC_MAX_EPOLL_NUM] = {0};
    U64 value = 0;
    U32 endFlag = 0;
    
    /* Clear and Set the connected socket */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    if(info == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_AUDIO, IPP_AUDIO_ERR_INVALID, info);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    size = sizeof(IPOD_PLAYER_MESSAGE_DATA_CONTENTS);
    
    handles[0] = info->audioSckServer;
    
    /* monitor event handle */
    if(g_eventHandle >= 0)
    {
        handles[handleNum] = g_eventHandle;
        handleNum += 1;
    }
    
    while(endFlag == 0)
    {
        memset(info->pSckIn, 0, sizeof(*info->pSckIn));
        
        /* Wait the status changing  */
        rc = iPodPlayerIPCWait(info->audioSckWait, handleNum, handles, &resNum, resHandle, retType, -1, IPOD_PLAYER_WAIT_IN);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            for(i = 0; i < resNum; i++)
            {
                /* event occured */
                if(resHandle[i] == g_eventHandle)
                {
                    /* read event */
                    rc = read(g_eventHandle, &value, sizeof(value));
                    if(rc < 0)
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_INFORMATION, IPOD_LOG_PLAYER_AUDIO, rc);
                    }
                    else
                    {
                        rc = IPOD_PLAYER_IPC_OK;
                    }
                    endFlag = 1;
                }
                else
                {
                    rc = iPodPlayerIPCReceive(resHandle[i], (U8 *)info->pSckIn, size, 0, info->audioSckWait, -1);
                    if(rc > IPOD_PLAYER_OK)
                    {
                        contents = info->pSckIn;
                        pinfo = iPodAudioGetDeviceInfo(info, contents->paramTemp.header.devID);
                        switch(contents->paramTemp.header.funcId)
                        {
                        case IPOD_FUNC_PAI_START:
                            iPodAudioStart(pinfo, contents->paiStart.rate, &info->table);
                            break;
                            
                        case IPOD_FUNC_PAI_STOP:
                            iPodAudioStop(pinfo, &info->table);
                            break;
                            
                        case IPOD_FUNC_PAI_SETCFG:
                            iPodAudioSetCfg(pinfo, &contents->paiCfg.table, contents->paiCfg.header.devID);
                            break;
                            
                        case IPOD_FUNC_PAI_CLRCFG:
                            iPodAudioClearCfg(pinfo, info->audioSckWait);
                            break;
                            
                        case IPOD_FUNC_PAI_SETSR:
                            iPodAudioSetSamplerate(pinfo, contents->paiSetSample.sampleRate, &info->table);
                            break;
                            
                        case IPOD_FUNC_PAI_SETVOL:
                            iPodAudioSetVolume(pinfo, contents->paiSetVolume.volume, contents->paiSetVolume.header.appID, &info->table);
                            break;
                            
                        case IPOD_FUNC_PAI_GETVOL:
                            iPodAudioGetVolume(pinfo, contents->paiGetVolume.header.appID, &info->table);
                            break;
                            
                        default:
                            break;
                        }
                    }
                }
            }
        }
        
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO);
    
    return IPOD_PLAYER_OK;
}

void iPodAudioDeinit(IPOD_AUDIO_INFO *info)
{
    if(info == NULL)
    {
        return;
    }
    
    iPodAudioComDeinit(&info->table);
    
    if(g_eventHandle >= 0)
    {
        iPodPlayerIPCDeleteHandle(g_eventHandle);
        close(g_eventHandle);
        g_eventHandle = -1;
    }
    
    if(info->audioSckServer >= 0)
    {
        iPodPlayerIPCClose(info->audioSckServer);
        info->audioSckServer = -1;
    }
    
    if(info->audioSckWait >= 0)
    {
        iPodPlayerIPCClose(info->audioSckWait);
        info->audioSckWait = -1;
    }
    
    if(info->pinfo != NULL)
    {
        free(info->pinfo);
        info->pinfo = NULL;
    }
    
    if(info->pSckIn != NULL)
    {
        free(info->pSckIn);
        info->pSckIn = NULL;
    }
    
    free(info);
    iPodPlayerIPCDeinit();
    
    return;
}

IPOD_AUDIO_INFO *iPodAudioInit(void)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_IPC_OPEN_INFO openInfo;
    IPOD_AUDIO_INFO *info = NULL;
    IPOD_UTIL_CFG_HANDLE handle = NULL;                 /* for cfg handle */
    struct sigaction sig;
    U32 i = 0;
    S32 event_handl = -1;
    S32 getHandle = -1;
    
    /* Initialize the structure */
    memset(&openInfo, 0, sizeof(openInfo));
    memset(&sig, 0, sizeof(sig));
    
    sig.sa_handler = SIG_IGN;
    sig.sa_flags = SA_RESTART;
    sigaction(SIGPIPE, &sig, NULL);
    
    sig.sa_handler = ipp_audio_signal;
    sigaction(SIGTERM, &sig, NULL);
    sig.sa_handler = ipp_audio_signal;
    sigaction(SIGINT, &sig, NULL);
    
    rc = iPodPlayerIPCInit();
    if(rc != IPOD_PLAYER_IPC_OK)
    {
        return NULL;
    }
    
    info = calloc(1, sizeof(*info));
    if(info == NULL)
    {
        iPodPlayerIPCDeinit();
        return NULL;
    }
    
    /* Allocate the memory */
    info->pSckIn = calloc(1, sizeof(*info->pSckIn));
    if(info->pSckIn != NULL)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize cfg function */
        handle = iPodUtilInitCf((U8 *)IPOD_AUDIO_CFG_FILE_NAME, (U8 *)IPOD_AUDIO_CFG_ROOT_TAG);
        if(handle != NULL)
        {
            rc = iPodUtilGetNumCfn(handle, (const U8 *)IPOD_AUDIO_CFG_DEVICE_MAX_NUM, 1, (S32 *)&info->devMax);
            if(rc > 0)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
            iPodUtilDeInitCf(handle);
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        info->pinfo = calloc(info->devMax, sizeof(*info->pinfo));
        if(info->pinfo != NULL)
        {
            for(i = 0; i < info->devMax; i++)
            {
                info->pinfo[i].id = -1;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        openInfo.type = IPOD_PLAYER_OPEN_WAIT_HANDLE;
        openInfo.prefix = NULL;
        openInfo.identify = NULL;
        openInfo.connectionNum = 1;
        openInfo.semName = NULL;
        openInfo.maxBufSize = 0;
        rc = iPodPlayerIPCOpen(&info->audioSckWait, &openInfo);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            openInfo.type = IPOD_PLAYER_OPEN_SOCKET_SERVER;
            openInfo.prefix = (U8 *)IPOD_PLAYER_SOCKET_NAME;
            openInfo.identify = (U8 *)IPP_AUDIO_SOCK_PATH;
            openInfo.maxBufSize = IPOD_PLAYER_DATA_SIZE_MAX;
            openInfo.maxPacketSize = IPOD_PLAYER_DATA_SIZE_MAX;
            openInfo.connectionNum = info->devMax;
            /* Create socket as AF_UNIX*/
            rc = iPodPlayerIPCOpen(&info->audioSckServer, &openInfo);
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
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* initialize event */
        event_handl = eventfd(0, EFD_NONBLOCK);
        if(event_handl >= 0)
        {
            /* register the fd into IPC library */
            rc = iPodPlayerIPCCreateHandle(&getHandle, event_handl);
            if(rc == IPOD_PLAYER_IPC_OK)
            {
                /* set global parameter */
                g_eventHandle = event_handl;
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                /* failed to register fd */
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            /* failed to init event */
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodAudioComInit(&info->table);
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        iPodAudioDeinit(info);
        info = NULL;
    }
    
    return info;
}

int main()
{
    IPOD_AUDIO_INFO *info = NULL;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_AUDIO);
    
    info = iPodAudioInit();
    if(info != NULL)
    {
        iPodAudioMain(info);
        
        iPodAudioDeinit(info);
        info = NULL;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_AUDIO, 0);
    
    return 0;
}

