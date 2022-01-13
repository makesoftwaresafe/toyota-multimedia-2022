/****************************************************
 *  ipp_iap2_gettracktotalcount.c                                         
 *  Created on: 2017/07/12 15:44:30                      
 *  Implementation of the Class ippiAP2GetMediaItemInfo       
 *  Original author: madachi                     
 ****************************************************/

#include "ipp_iap2_getMediaItemInformation.h"
#include "ipp_iap2_database.h"

S32 ippiAP2GetMediaItemInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    S32 mediaType = IPP_INVALID_TRACKID;
    U32 progress = 0;

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
    
    /* Get the progress from database */
    rc = ippiAP2DBGetProgress(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, &progress);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Progress is max. It means that all database has been already retrieved from Apple devoce */
        if(progress == IPOD_PLAYER_IAP2_MAX_PROGRESS)
        {
            rc = ippiAP2DBGetMediaType(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, param->waitData->contents.getMediaItemInfo.trackID, &mediaType);
        }
        else
        {
            IPOD_DLT_WARN("MediaLibraryUpdate isn't finishing(Media Item) progress = %u", progress);
            mediaType = IPP_MEDIALIB_UP_INPROGRESS;
        }
    }
    else
    {
        IPOD_DLT_ERROR("Could not get progress of MediaItemUpdate. rc = %d", rc);
    }

    if(mediaType == IPP_INVALID_TRACKID)
    {
        rc = IPOD_PLAYER_ERR_INVALID_ID; 
    }

    if(rc == IPOD_PLAYER_OK)
    {
        param->waitData->contents.getMediaItemMediaItemInfoResult.trackID = param->waitData->contents.getMediaItemInfo.trackID;
        param->waitData->contents.getMediaItemMediaItemInfoResult.mediaType = mediaType;
    }
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

