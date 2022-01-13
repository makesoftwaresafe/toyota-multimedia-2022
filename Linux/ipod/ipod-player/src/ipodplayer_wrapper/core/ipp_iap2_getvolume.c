/****************************************************
 *  ipp_iap2_getvolume.c                                         
 *  Created on: 2014/01/17 17:55:30                      
 *  Implementation of the Class ipp_iap2_getvolume       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_getvolume.h"

S32 ippiAP2GetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 funcId = 0;
    IPOD_PLAYER_PARAM_PAI_GET_VOLUME getVolume;
    
    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if((iPodCtrlCfg == NULL) || (param == NULL) || (param->waitData == NULL) || (param->contents == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Initialize parameter */
    memset(&getVolume, 0, sizeof(getVolume));
    
    
    /* Here checks the status whether this function can execute */
    if((!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection)) || (iPodCtrlCfg->audioSetting.mode != IPOD_PLAYER_SOUND_MODE_ON))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                                iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, param->waitData->stage);
    
    switch(param->waitData->stage)
    {
        /* Request to get the track information to iPod Ctrl at first */
        case IPP_IAP2_GET_VOLUME_STAGE_SEND_REQUEST:
            getVolume.header.funcId = IPOD_FUNC_PAI_GETVOL;
            getVolume.header.appID = param->contents->paramTemp.header.appID;
            getVolume.header.devID = iPodCtrlCfg->threadInfo->appDevID;
            rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, (U8 *)&getVolume, sizeof(getVolume), 0, IPODCORE_TMOUT_FOREVER);
            if(rc == (S32)sizeof(getVolume))
            {
                param->waitData->stage = IPP_IAP2_GET_VOLUME_STAGE_CHECK_RESULT;
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
            }
            break;
            
        /* Wait to receive the data from callback function */
        case IPP_IAP2_GET_VOLUME_STAGE_CHECK_RESULT:
            funcId = param->contents->paiGetVolume.header.funcId;
            if(funcId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_GET_VOLUME))
            {
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING, param->contents->paiGetVolume.header.funcId);
                rc = param->contents->paiGetVolume.result;
                param->waitData->contents.getVolumeResult.volume = param->contents->paiGetVolume.volume;
            }
            else
            {
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            }
            break;
            
        default:
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, param->waitData->status);
            break;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

