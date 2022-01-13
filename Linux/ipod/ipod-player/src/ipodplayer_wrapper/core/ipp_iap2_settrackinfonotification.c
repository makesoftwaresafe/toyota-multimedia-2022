/*******************************************************************
 *  ipp_iap2_settrackinfonotification.c                             
 *  Created on: 2014/01/17 17:55:33                                 
 *  Implementation of the Class ipp_iap2_settrackinfonotification   
 *  Original author: mshibata                                       
 *******************************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_settrackinfonotification.h"
#include "ipp_iap2_observer.h"

S32 ippiAP2SetTrackInfoNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                                    IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    TrackInfoMask_t trackInfoMask;

    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);

    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;   /* leave function */
    }

    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                                iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;    /* leave function */
    }
    
    if(IPP_IAP2_SUPPORTED_TRACK_INFO_MASK & param->waitData->contents.setTrackInfoNotification.trackInfoMask)
    {
        memset(&trackInfoMask, 0, sizeof(trackInfoMask));
        /* Execute operation */
        /* Sets the mask pattern of track information that send to application */
        trackInfoMask.mask = param->waitData->contents.setTrackInfoNotification.trackInfoMask;
        trackInfoMask.formatId = param->waitData->contents.setTrackInfoNotification.formatId;
        
        rc = iPodCoreObserverSetTrackInfoNotifyDataMask(iPodCtrlCfg, &trackInfoMask);
    }
    else if(IPP_IAP2_IS_MASK_SET((IPP_IAP2_TRACK_INFO_MASK & (~IPP_IAP2_SUPPORTED_TRACK_INFO_MASK)), param->waitData->contents.setTrackInfoNotification.trackInfoMask))
    {
        rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);

    return rc;
} 

