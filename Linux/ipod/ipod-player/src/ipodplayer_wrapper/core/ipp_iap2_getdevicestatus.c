/****************************************************
 *  ipp_iap2_getdevicestatus.c                                         
 *  Created on: 2014/01/17 17:55:29                      
 *  Implementation of the Class ipp_iap2_getdevicestatus       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_getdevicestatus.h"
#include "ipp_iap2_observer.h"

S32 ippiAP2GetDeviceStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 reqMask = 0;
    U32 resMask = 0;
    U32 eventMask = 0;
    
    
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
    
    /* Set the mask to local variable */
    reqMask = param->contents->getDeviceStatus.deviceStatusMask;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, reqMask);
    
    /* iPod event notifiation bit was set */
    if(IPP_IAP2_IS_MASK_SET(reqMask, IPOD_PLAYER_IPOD_STATUS_INFO_MASK_NOTIFY_EVENT))
    {
        /* Get event notification mask */
        eventMask = iPodCoreObserverGetDeviceEventNotifyDataMask(iPodCtrlCfg);
        param->waitData->contents.getDeviceStatusResult.info.notifyEvent = eventMask;
        resMask |= IPOD_PLAYER_IPOD_STATUS_INFO_MASK_NOTIFY_EVENT;
    }
    
    /* Requested status is set */
    if(resMask > 0)
    {
        param->waitData->contents.getDeviceStatusResult.info.deviceStatusMask = resMask;
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

