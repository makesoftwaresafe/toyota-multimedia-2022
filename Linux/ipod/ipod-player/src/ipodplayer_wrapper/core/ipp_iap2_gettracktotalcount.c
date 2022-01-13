/****************************************************
 *  ipp_iap2_gettracktotalcount.c                                         
 *  Created on: 2014/01/17 17:55:30                      
 *  Implementation of the Class ipp_iap2_gettracktotalcount       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_gettracktotalcount.h"
#include "ipp_iap2_database.h"

S32 ippiAP2GetTrackTotalCount(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 count = 0;
    
    
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
    
    if(param->contents->getTrackTotalCount.type == IPOD_PLAYER_TRACK_TYPE_PLAYBACK)
    {
        /* Playback type */
        rc = ippiAP2DBGetNowPlayingCount(iPodCtrlCfg->iap2Param.dbHandle, &count);
        //IPOD_DLT_INFO("[DBG]rc=%d, count=%u", rc, count);
    }
    else if(param->contents->getTrackTotalCount.type == IPOD_PLAYER_TRACK_TYPE_DATABASE)
    {
        /* DB type */
        /* Check whether the repeat status is set */
        rc = ippiAP2DBGetTrackTotalCount(iPodCtrlCfg->iap2Param.dbHandle, &iPodCtrlCfg->iap2Param.dbHandle->localDevice, param->contents->getTrackTotalCount.type, &count, NULL);
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, param->contents->getTrackTotalCount.type);
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    if(rc == IPOD_PLAYER_OK)
    {
        param->waitData->contents.getTrackTotalCountResult.count = count;
        param->waitData->contents.getTrackTotalCountResult.type = param->contents->getTrackTotalCount.type;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

