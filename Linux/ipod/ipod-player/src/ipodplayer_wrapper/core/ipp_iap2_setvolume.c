/****************************************************
 *  ipp_iap2_setvolume.c                                         
 *  Created on: 2014/01/17 17:55:34                      
 *  Implementation of the Class ipp_iap2_setvolume       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_setvolume.h"

S32 ippiAP2SetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_PAI_SET_VOLUME setVolume;
    U32 sendSize = 0;
    U32 checkId = 0;
    
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
    
    memset(&setVolume, 0, sizeof(setVolume));
    
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
        case IPP_IAP2_SET_VOLUME_STAGE_SEND_REQUEST:
            IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_WAIT);
            setVolume.header.funcId = IPOD_FUNC_PAI_SETVOL;
            setVolume.header.appID = param->waitData->contents.paramTemp.header.appID;
            setVolume.header.devID = iPodCtrlCfg->threadInfo->appDevID;
            setVolume.volume = param->waitData->contents.setVolume.volume;
            sendSize = sizeof(setVolume);
            
            if(setVolume.volume <= IPODCORE_VOLUME_MAX)
            {
                rc = iPodPlayerIPCSend(iPodCtrlCfg->sckAudio, (U8 *)&setVolume, sendSize, 0, IPODCORE_TMOUT_FOREVER);
                if(rc == (S32)sendSize)
                {
                    param->waitData->stage = IPP_IAP2_SET_VOLUME_STAGE_CHECK_RESULT;
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
            
        case IPP_IAP2_SET_VOLUME_STAGE_CHECK_RESULT:
            checkId = (U32)param->contents->paiGetVolume.header.funcId;;
            if(checkId == (U32)(IPOD_PLAYER_NOTIFY_FUNC_MASK | IPOD_FUNC_SET_VOLUME))
            {
                IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING, checkId);
                rc = param->waitData->contents.paiSetVolume.result;
            }
            else
            {
                rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
            }
            break;
            
        default:
            rc = IPOD_PLAYER_ERROR;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, param->waitData->stage);
            break;
            
    }
    
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

