/****************************************************
 *  ipp_iap2_getdbentries.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_selectdbentry.h"
#include "ipp_iap2_database.h"

S32 ippiAP2SelectDBEntry(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                         IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IAP2_CTL_ERROR;
    U64 id = 0;
    
    IPOD_PLAYER_IAP2_DB_CATLIST *catList = NULL;
    
    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param)){
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;   /* leave function */
    }
    
    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection)){
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                                iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;    /* leave function */
    }
    
    /* Initialize the structure */
    memset(&catList, 0, sizeof(catList));
    
    /* GetID */
    rc = ippiAP2DBGetSelectingCategories(iPodCtrlCfg->iap2Param.dbHandle, &catList);
    if(rc == IPOD_PLAYER_OK)
    {
        if(param->waitData->contents.selectDBEntry.entry != -1)
        {
            rc = ippiAP2DBGetCategoryID(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, param->waitData->contents.selectDBEntry.type, param->waitData->contents.selectDBEntry.entry, &id, catList);
            if(rc == IPOD_PLAYER_OK)
            {
                rc = ippiAP2DBSetCategoryID(iPodCtrlCfg->iap2Param.dbHandle, param->waitData->contents.selectDBEntry.type, id);
            }
        }
        else
        {
            rc = ippiAP2DBDeleteSelect(iPodCtrlCfg->iap2Param.dbHandle, param->waitData->contents.selectDBEntry.type);
        }
    }
    
    ippiAP2DBFreeSelectingCategories(catList);
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

