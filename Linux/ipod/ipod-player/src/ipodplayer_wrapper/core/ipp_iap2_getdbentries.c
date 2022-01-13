/****************************************************
 *  ipp_iap2_getdbentries.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_getdbentries.h"
#include "ipp_iap2_database.h"
#include "iPodPlayerCoreFunc.h"


static S32 ippiAP2GetDBEntriesCheckProgress(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                                            IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 progress = 0;
    IPOD_PLAYER_IAP2_DB_CATLIST *catList = NULL;
    U8 revision[IPOD_PLAYER_IAP2_DB_MAX_REVISION_LEN] = {0};

    
    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Get the progress from database */
    rc = ippiAP2DBGetProgress(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, &progress);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Progress is max. It means that all database has been already retrieved from Apple devoce */
        if(progress == IPOD_PLAYER_IAP2_MAX_PROGRESS)
        {
            /* Get revision.If revision is NULL, getDBEntry is waiting until revision is stored. */
            ippiAP2DBGetRevision(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, revision);
            if(revision[0] != 0)
            {
                /* Get current selecting categories */
                rc = ippiAP2DBGetSelectingCategories(iPodCtrlCfg->iap2Param.dbHandle, &catList);
                if(rc == IPOD_PLAYER_OK)
                {
                    param->waitData->storeBuf = (U8 *)catList;
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERR_DB_RETRIVE_NOT_COMPLETE;
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_DB_RETRIVE_NOT_COMPLETE;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        param->waitData->contents.getDBEntries.curNum = param->waitData->contents.getDBEntries.start;
        /* All lists should be obtained */
        if(param->waitData->contents.getDBEntries.num == -1)
        {
            /* Get total count of current selecting list */
            rc = ippiAP2DBGetCategoryCount(iPodCtrlCfg->iap2Param.dbHandle, &iPodCtrlCfg->iap2Param.dbHandle->localDevice, IPOD_PLAYER_TRACK_TYPE_DATABASE, param->waitData->contents.getDBEntries.type, catList);
            if(rc >= 0)
            {
                param->waitData->contents.getDBEntries.num = rc;
                rc = IPOD_PLAYER_OK;
            }
        }
        else
        {
            param->waitData->contents.getDBEntries.num += param->waitData->contents.getDBEntries.start;
            rc = IPOD_PLAYER_OK;
        }
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        if(catList != NULL)
        {
            ippiAP2DBFreeSelectingCategories(catList);
            param->waitData->storeBuf = NULL;
        }
    }
    
    return rc;
}

static S32 ippiAP2GetDBEntriesGetList(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                                      IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param,
                                      IPOD_PLAYER_IAP2_DB_CATLIST *catList)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 start = 0;
    U32 i = 0;
    U32 num = 0;
    U32 listNum = 0;
    IPOD_CB_PARAM_NOTIFY_DB_ENTRIES entries;
    
    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Check remainning parametr */
    if(catList == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    start = param->waitData->contents.getDBEntries.curNum;
    num = param->waitData->contents.getDBEntries.num;
    
    memset(&entries, 0, sizeof(entries));
    /* Check the number of remaining list */
    if(start + iPodCtrlCfg->threadInfo->maxEntry <= num)
    {
        listNum = iPodCtrlCfg->threadInfo->maxEntry;
    }
    else
    {
        listNum = num - start;
    }
    
    /* Get lists from start until listNum */
    rc = ippiAP2DBGetMediaItem(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, param->waitData->contents.getDBEntries.type,
                               start, listNum, catList, entries.entryList);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Set track index */
        for(i = 0; i < IPOD_PLAYER_ENTRIES_ARRYA_MAX; i++)
        {
            entries.entryList[i].trackIndex = start + i;
        }
    }
    
    if(rc == IPOD_PLAYER_OK)
    {
        /* Set the callback parameter to structure  */
        entries.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_DB_ENTRIES;
        entries.header.devID = param->waitData->contents.getDBEntries.header.devID;
        entries.header.appID = param->waitData->contents.getDBEntries.header.appID;
        entries.type = param->waitData->contents.getDBEntries.type;
        entries.count = listNum;
        
        /* Send notification to Application */
        rc = iPodPlayerIPCSend(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&entries, sizeof(entries), 0, IPODCORE_TMOUT_FOREVER);
        if(rc == (S32)sizeof(entries))
        {
            rc = IPOD_PLAYER_OK;
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc, iPodCtrlCfg->threadInfo->cmdQueueInfoClient);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    /* Check if still continue  */
    if(rc == IPOD_PLAYER_OK)
    {
        /* Still not obtain lists until number that application requested */
        if(start + iPodCtrlCfg->threadInfo->maxEntry < num)
        {
            param->waitData->contents.getDBEntries.curNum = start + iPodCtrlCfg->threadInfo->maxEntry;
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
    }
    
    return rc;
}

S32 ippiAP2GetDBEntries(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                        IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IAP2_CTL_ERROR;
    IPOD_PLAYER_IAP2_DB_CATLIST *catList = NULL;
    U32 waitTime = 0;
    
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
    
    switch(param->waitData->stage)
    {
    case IPP_GETDBENTRIES_STAGE_CHECK_PROGRESS:
        /* Check wheter database has been already retrieved */
        rc = ippiAP2GetDBEntriesCheckProgress(iPodCtrlCfg, param);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Move to next stage */
            param->waitData->stage = 1;
            waitTime = IPOD_PLAYER_TIMER_IMMEDIATE;
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        else if(rc == IPOD_PLAYER_ERR_DB_RETRIVE_NOT_COMPLETE)
        {
            waitTime = IPOD_PLAYER_IAP2_TIMER_FOR_PROGRESS;
            rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
        }
        else
        {
            rc = IPOD_PLAYER_ERROR;
        }
        break;
        
    case IPP_GETDBENTRIES_STAGE_GET_LIST:
        if(param->waitData->storeBuf != NULL)
        {
            catList = (IPOD_PLAYER_IAP2_DB_CATLIST *)((void *)param->waitData->storeBuf);
            /* Get list */
            rc = ippiAP2GetDBEntriesGetList(iPodCtrlCfg, param, catList);
            if(rc == IPOD_PLAYER_ERR_REQUEST_CONTINUE)
            {
                waitTime = IPOD_PLAYER_TIMER_IMMEDIATE;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, iPodCtrlCfg);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
        break;
        
    case IPP_GETDBENTRIES_STAGE_CANCEL:
        rc = IPOD_PLAYER_ERR_API_CANCELED;
        break;
        
    default:
        rc = IPOD_PLAYER_ERROR;
        break;
    }
    
    /* Function will be finished due to function success or failed */
    if(rc != IPOD_PLAYER_ERR_REQUEST_CONTINUE)
    {
        if(param->waitData->storeBuf != NULL)
        {
            catList = (IPOD_PLAYER_IAP2_DB_CATLIST *)((void *)param->waitData->storeBuf);
            if(catList->categories != NULL)
            {
                free(catList->categories);
                catList->categories = NULL;
            }
            free(catList);
            param->waitData->storeBuf = NULL;
        }
    }
    else
    {
        /* Set self trigger */
        iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, waitTime, 0);
    }
    
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

