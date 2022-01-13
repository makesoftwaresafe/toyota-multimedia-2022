/****************************************************
 *  ipp_iap2_setaudiomode.c                          
 *  Created on: 2014/01/17 17:55:32                  
 *  Implementation of the Class ipp_iap2_setaudiomode
 *  Original author: madachi                         
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_audiocommon.h"
#include "ipp_iap2_setaudiomode.h"
#include "iPodPlayerCoreCfg.h"

static S32 iPodCoreiAP2AudioRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 size, U8 *data)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (data == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, data);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Send the request */
    rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, data, size, 0, IPODCORE_SEND_TMOUT);
    if(rc == (S32)size)
    {
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }
    
    return rc;
}


S32 iPodCoreiAP2AudioDeinitCfg(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
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
        
    rc = iPodCoreiAP2AudioRequest(iPodCtrlCfg, sizeof(paiClrCfg), (U8 *)&paiClrCfg);
    
    return rc;
}

static S32 iPodCoreiAP2AudioInitCfg(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
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
    
    rc = iPodCoreiAP2AudioRequest(iPodCtrlCfg, sizeof(paiCfg), (U8 *)&paiCfg);
    
    return rc;
}

/* Start audio streaming */
static S32 iPodCoreiAP2AudioStartStreaming(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
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
    paiStart.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    /* Set the default err rate */
    paiStart.rate = iPodCtrlCfg->iPodInfo->sampleRate;
    
    rc = iPodCoreiAP2AudioRequest(iPodCtrlCfg, sizeof(paiStart), (U8 *)&paiStart);
    
    return rc;
}

static S32 iPodCoreiAP2AudioStopStreaming(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
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
    
    rc = iPodCoreiAP2AudioRequest(iPodCtrlCfg, sizeof(paiStop), (U8 *)&paiStop);
    
    return rc;
}

S32 iPodCoreiAP2StartAudio(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
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
    rc = iPodCoreiAP2AudioStartStreaming(iPodCtrlCfg);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Audio streaming is started */
        iPodCtrlCfg->startAudio = 1;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

S32 iPodCoreiAP2StopAudio(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
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
        rc = iPodCoreiAP2AudioStopStreaming(iPodCtrlCfg);
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

S32 ippiAP2SetAudioMode(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                        IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IAP2_CTL_ERROR;
    
    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (param == NULL) || (param->waitData == NULL) || (param->contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, 
                                                                        iPodCtrlCfg, param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection)){
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                            iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;    /* leave function */
    }
    
    switch(param->waitData->stage)
    {
    case IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_REQUEST_SELECT:
        if(((param->waitData->contents.setAudioMode.setting.mode == IPOD_PLAYER_SOUND_MODE_ON) || 
            (param->waitData->contents.setAudioMode.setting.mode == IPOD_PLAYER_SOUND_MODE_OFF)) && 
           ((param->waitData->contents.setAudioMode.setting.adjust == IPOD_PLAYER_STATE_ADJUST_ENABLE) ||
            (param->waitData->contents.setAudioMode.setting.adjust == IPOD_PLAYER_STATE_ADJUST_DISABLE)))
        {
            /* Requested mode is different from current mode */
            if(iPodCtrlCfg->audioSetting.mode != param->waitData->contents.setAudioMode.setting.mode)
            {
                /* New mode is audio on */
                if(param->waitData->contents.setAudioMode.setting.mode == IPOD_PLAYER_SOUND_MODE_ON)
                {
                    /* Initialize the configuration */
                    rc = iPodCoreiAP2AudioInitCfg(iPodCtrlCfg);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        param->waitData->stage = IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_AUDIO_START;
                    }
                }
                /* New mode is audio off */
                else
                {
                    /* Stop audio streaming  */
                    rc = iPodCoreiAP2StopAudio(iPodCtrlCfg);
                    if(rc == IPOD_PLAYER_OK)
                    {
                        param->waitData->stage = IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_AUDIO_STOP;
                    }
                }
                
            }
            else
            {
                /* Audio mode was not changed */
                rc = IPOD_PLAYER_OK;
            }
            
            /* Requested adjust is different fomr current adjust */
            if(rc == IPOD_PLAYER_OK)
            {
                if(iPodCtrlCfg->audioSetting.adjust != param->waitData->contents.setAudioMode.setting.adjust)
                {
                    /* Set the new adjust */
                    iPodCtrlCfg->audioSetting.adjust = param->waitData->contents.setAudioMode.setting.adjust;
                    
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
            
            if((rc == IPOD_PLAYER_OK) && (param->waitData->stage != IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_REQUEST_SELECT))
            {
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, param->waitData->contents.setAudioMode.setting.mode, param->waitData->contents.setAudioMode.setting.adjust);
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
        break;
        
    case IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_AUDIO_START:
        if((param->contents->paiResult.header.funcId == IPOD_FUNC_PAI_RESULT) &&
           (param->contents->paiResult.cmdId == IPOD_FUNC_PAI_SETCFG))
        {
            rc = param->contents->paiResult.result;
            if(rc == IPOD_PLAYER_OK)
            {
                /* Start audio streaming */
                rc = iPodCoreiAP2StartAudio(iPodCtrlCfg);
                if(rc == IPOD_PLAYER_OK)
                {
                    param->waitData->stage = IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_CHECK_RESULT;
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        
        break;
        
    case IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_AUDIO_STOP:
        if((param->contents->paiResult.header.funcId == IPOD_FUNC_PAI_RESULT) &&
           (param->contents->paiResult.cmdId == IPOD_FUNC_PAI_STOP))
        {
            rc = param->contents->paiResult.result;
            if(rc == IPOD_PLAYER_OK)
            {
                /* Deinit the configuration */
                rc = iPodCoreiAP2AudioDeinitCfg(iPodCtrlCfg);
                if(rc == IPOD_PLAYER_OK)
                {
                    param->waitData->stage = IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_CHECK_RESULT;
                    rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        
        break;
        
    case IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_CHECK_RESULT:
        if(param->contents->paiResult.header.funcId == IPOD_FUNC_PAI_RESULT)
        {
            if((param->contents->paiResult.cmdId == IPOD_FUNC_PAI_START) ||
               (param->contents->paiResult.cmdId == IPOD_FUNC_PAI_CLRCFG))
            {
                rc = param->contents->paiResult.result;
                if(rc == IPOD_PLAYER_OK)
                {
                    iPodCtrlCfg->audioSetting.mode = param->waitData->contents.setAudioMode.setting.mode;
                }
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        break;
        
    default:
        rc = IPOD_PLAYER_ERROR;
        break;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;

} 

