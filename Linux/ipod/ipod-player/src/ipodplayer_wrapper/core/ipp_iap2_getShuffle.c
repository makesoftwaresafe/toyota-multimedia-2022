/****************************************************
 *  ipp_iap2_getShuffle.c                                         
 *  Created on: 2014/01/17 17:55:30                      
 *  Implementation of the Class ipp_iap2_getShuffle       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_getShuffle.h"
#include "ipp_iap2_database.h"

S32 ippiAP2GetShuffle(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_SHUFFLE_STATUS shuffleMode;
    
    
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
    
    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                            iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;
    }
    
    
    /* Init current shuffle status to result */
    param->waitData->contents.getShuffleResult.status = IPOD_PLAYER_SHUFFLE_STATUS_UNKNOWN;
    
    rc = ippiAP2DBGetShuffle(iPodCtrlCfg->iap2Param.dbHandle, &shuffleMode);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Set the current shuffle status to result */
         param->waitData->contents.getShuffleResult.status = shuffleMode;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

