/****************************************************
 *  ipp_iap2_fastforward.c                          
 *  Created on: 2014/01/17 17:55:28                 
 *  Implementation of the Class ipp_iap2_fastforward
 *  Original author: mshibata                       
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_hidcommon.h"
#include "ipp_iap2_fastforward.h"

S32 ippiAP2FastForward( IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                        IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    iAP2Device_t* iap2Device = NULL;
    ReportTable_t   reports_tbl[] = {   IAP2_NEXT_TR_REPORT,
                                        IAP2_REPORT_TBL_STOP};

    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                            iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;    /* leave function */
    }

    /* Execute operation */
    /* iAP2 device object */
    iap2Device = iPodCtrlCfg->iap2Param.device;
    /* Fast forward */
    rc = ippiAP2SendHIDReports(iap2Device, reports_tbl);

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

