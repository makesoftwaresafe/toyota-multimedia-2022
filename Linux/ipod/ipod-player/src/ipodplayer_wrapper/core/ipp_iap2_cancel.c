/****************************************************
 *  ipp_iap2_getdbcount.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_cancel.h"
#include "ipp_iap2_getdbentries.h"
#include "iPodPlayerCoreFunc.h"

static S32 ippiAP2CancelGetIndexOfQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList, U32 last, U32 funcId)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 i = 0;
    
    /* Parameter check */
    if(waitList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Loop until last list */
    for(i = 0; i < last; i++)
    {
        /* Check target id equals with id of current list */
        if((U32)waitList[i].contents.paramTemp.header.funcId == funcId)
        {
            rc = i;
            break;
        }
    }
    
    return rc;
}

static S32 ippiAP2CancelChangeStage(IPOD_PLAYER_CORE_WAIT_LIST *waitList, IPOD_PLAYER_CANCEL_TYPE type)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 i = 0;
    U32 funcId = 0;
    U32 stage = 0;
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, waitList);
    
    /* Check Parameter */
    if(waitList == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, waitList);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    switch(type)
    {
    case IPOD_PLAYER_CANCEL_DB_ENTRY:
        funcId = IPOD_FUNC_GET_DB_ENTRIES;
        stage = IPP_GETDBENTRIES_STAGE_CANCEL;
        break;
        
    case IPOD_PLAYER_CANCEL_COVERART:
        funcId = IPOD_FUNC_GET_COVERART;
        /* Todo */
        stage = 2;
        break;
        
    default:
        break;
    }
    
    /* Check type is type which is able to cancel */
    if((funcId == 0) && (stage == 0))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the position of cancel in the queue */
    rc = ippiAP2CancelGetIndexOfQueue(waitList, IPODCORE_WAIT_LIST_MAX, IPOD_FUNC_CANCEL);
    if(rc >= IPOD_PLAYER_OK)
    {
        /* Check if request of listing has before cancel queue */
        rc = ippiAP2CancelGetIndexOfQueue(waitList, rc, funcId);
        if(rc >= IPOD_PLAYER_OK)
        {
            i = rc;
            /* Stage change to cancel */
            waitList[i].stage = stage;
            rc = IPOD_PLAYER_OK;
        }
    }
    
    return rc;
}

S32 ippiAP2Cancel(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                      IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IAP2_CTL_ERROR;
    
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
    
    rc = ippiAP2CancelChangeStage(iPodCtrlCfg->waitList, param->waitData->contents.cancel.type);
    if(rc == IPOD_PLAYER_OK)
    {
        rc = iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


