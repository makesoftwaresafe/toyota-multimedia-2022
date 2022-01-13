/****************************************************
 *  ipp_iap2_cleardbselection.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_cleardbselection.h"
#include "ipp_iap2_database.h"

S32 ippiAP2ClearDBSelection(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                            IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    IPOD_PLAYER_IAP2_DB_CATLIST catList;
    
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
    
    /* Initialize the structure */
    memset(&catList, 0, sizeof(catList));
    
    /* Remove the selected categories from current selecting category */
    rc = ippiAP2DBClearSelectingCategory(iPodCtrlCfg->iap2Param.dbHandle, param->waitData->contents.clearSelection.type);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

