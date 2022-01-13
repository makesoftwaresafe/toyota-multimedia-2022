/****************************************************
 *  ipp_iap2_getdbcount.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_getdbcount.h"
#include "ipp_iap2_database.h"

S32 ippiAP2GetDBCount(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                      IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 count = 0;
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
    
    /* Get count of current selecting categories*/
    rc = ippiAP2DBGetSelectingCategoryCount(iPodCtrlCfg->iap2Param.dbHandle, &count);
    if(rc == IPOD_PLAYER_OK)
    {
        /* One or more categories are selected now */
        if(count > 0)
        {
            /* Allocate for categories */
            catList.categories = calloc(count, sizeof(*catList.categories));
            if(catList.categories != NULL)
            {
                catList.count = count;
                /* Get lists of category*/
                rc = ippiAP2DBGetSelectingCategoryList(iPodCtrlCfg->iap2Param.dbHandle, &catList);
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        else
        {
            /* Need not to be allocated because of no selecting */
            rc = IPOD_PLAYER_OK;
        }
    }
    
    /* Set Category */
    if(rc == IPOD_PLAYER_OK)
    {
        /* Get count of indicated category */
        rc = ippiAP2DBGetCategoryCount(iPodCtrlCfg->iap2Param.dbHandle, &iPodCtrlCfg->iap2Param.dbHandle->localDevice, IPOD_PLAYER_TRACK_TYPE_DATABASE, param->waitData->contents.getDBCount.type, &catList);
        if(rc >= 0)
        {
            param->waitData->contents.getDBCountResult.num = rc;
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }
    
    /* Memory for categories has been allocated */
    if(catList.categories != NULL)
    {
        free(catList.categories);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

