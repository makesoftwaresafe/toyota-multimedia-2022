/****************************************************
 *  ipp_iap2_playtrack.c                             
 *  Created on:         2014/03/07 21:39:34          
 *  Implementation of the Class ipp_iap2_playtrack   
 *  Original author: madachi                         
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_playtrack.h"
#include "ipp_iap2_database.h"

/*************************************************
    Sending iAP2PlayMediaLibraryItem to device
**************************************************/
S32 ippiAP2PlayMediaLibraryItem(    iAP2Device_t*   iap2Device,
                                    iAP2Blob        *ItemPersistentID,
                                    U32             startIndex,
                                    U8              **uid)
{
    S32 rc  = IAP2_OK;
    iAP2PlayMediaLibraryItemsParameter  iAP2PlayMediaLibPara;
    
    /* parameter check */
    if((iap2Device == NULL) || (uid == NULL) || (ItemPersistentID == NULL)){
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
    }else{
        memset(&iAP2PlayMediaLibPara, 0, sizeof(iAP2PlayMediaLibPara));
        
        /* create array of ordered uint64 MediaItemPersistentIdentifiers */
        /* set Persistent ID of Mediaitem to Persistent ID array */
        rc = ippiAP2AllocateandUpdateData(&iAP2PlayMediaLibPara.iAP2ItemsPersistentIdentifiers,
                                       ItemPersistentID->iAP2BlobData,
                                       &iAP2PlayMediaLibPara.iAP2ItemsPersistentIdentifiers_count,
                                       ItemPersistentID->iAP2BlobLength, iAP2_blob);

        if(rc == IAP2_OK){
            /* set index of first item to start playback */
            rc = ippiAP2AllocateandUpdateData(&iAP2PlayMediaLibPara.ItemsStartingIndex,
                                           &startIndex,
                                           &iAP2PlayMediaLibPara.ItemsStartingIndex_count,
                                           1, iAP2_uint32);
        }

        /* set unique identifier to start playback */
        if(rc == IAP2_OK){
            rc = ippiAP2AllocateandUpdateData(&iAP2PlayMediaLibPara.MediaLibraryUniqueIdentifier,
                                           &uid,
                                           &iAP2PlayMediaLibPara.MediaLibraryUniqueIdentifier_count,
                                           1, iAP2_utf8);
        }

        if(rc == IAP2_OK){
            /* start playback */
            rc = iAP2PlayMediaLibraryItems(iap2Device, &iAP2PlayMediaLibPara);
            IPOD_DLT_INFO("Play :startIndex=%u, uid=%s", startIndex, (U8*)uid);
        }

        /* Free API parameter buffer */
        iAP2FreeiAP2PlayMediaLibraryItemsParameter(&iAP2PlayMediaLibPara);

    }

    /* Conver to error code of iPodPlayer from error code of iAP2 library */
    rc = ippiAP2RetConvertToiPP(rc);

    return rc;
}

/***************************************************
  iPodPlayer playTrack
****************************************************/
S32 ippiAP2PlayTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG          *iPodCtrlCfg,
                     IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM    *param)
{
    S32                         rc = IPOD_PLAYER_ERROR;
    iAP2Device_t*               iap2Device = NULL;
    U64                         trackID = 0;
    iAP2Blob                    MItemPID;
    IPOD_PLAYER_IAP2_DB_IDLIST  idList;
    UniqueId_t                  UniqueId;
    U32                         count = 0;
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
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE,
                                                iPodCtrlCfg->deviceConnection.deviceStatus,
                                                iPodCtrlCfg->deviceConnection.authStatus,
                                                iPodCtrlCfg->iPodInfo->mode);
        return IPOD_PLAYER_ERR_INVALID_MODE;    /* leave function */
    }
    
    memset(&idList, 0, sizeof(idList));
    memset(&catList, 0, sizeof(catList));
    
    rc = ippiAP2DBGetSelectingCategoryCount(iPodCtrlCfg->iap2Param.dbHandle, &count);
    if(rc == IPOD_PLAYER_OK)
    {
        if(count > 0)
        {
            catList.categories = calloc(count, sizeof(*catList.categories));
            if(catList.categories != NULL)
            {
                catList.count = count;
                rc = ippiAP2DBGetSelectingCategoryList(iPodCtrlCfg->iap2Param.dbHandle, &catList);
            }
            else
            {
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
    }
    
    /* Set Category */
    if(rc == IPOD_PLAYER_OK)
    {
        rc = ippiAP2DBGetCategoryCount( iPodCtrlCfg->iap2Param.dbHandle,
                                        &iPodCtrlCfg->iap2Param.dbHandle->localDevice,
                                        param->waitData->contents.playTrack.type,
                                        IPOD_PLAYER_DB_TYPE_TRACK,
                                        &catList);
        if(rc >= 0)
        {
            trackID = param->waitData->contents.playTrack.trackID;  /* Set track id (list offset) */
            count = rc;                                             /* Number of category list */

            /* check the number of track */
            if(count > trackID)
            {
                rc = IPOD_PLAYER_OK;
            }
            else
            {
                rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            }
        }
    }
    
    /* Execute operation */
    if(rc == IPOD_PLAYER_OK)
    {
        /* Track information to Media Item information */
        idList.count = count;
        idList.mediaId = calloc(count, sizeof(U64));
        if(idList.mediaId != NULL)
        {
            rc = ippiAP2DBGetMediaItemID(   iPodCtrlCfg->iap2Param.dbHandle,            /* DB handle object */
                                            &iPodCtrlCfg->iap2Param.dbHandle->localDevice,
                                            param->waitData->contents.playTrack.type,   /* Track type */
                                            IPOD_PLAYER_DB_TYPE_TRACK,                  /* DB type  */
                                            &catList,                                   /* category list */
                                            &idList);                                   /* persistent id list */
            MItemPID.iAP2BlobData = (U8 *)(&(idList.mediaId[0]));
            MItemPID.iAP2BlobLength = idList.count * sizeof(U64);
        }
        else
        {
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }

    if(rc == IPOD_PLAYER_OK)
    {
        /* Get Media Library UniqueId */
        memset(&UniqueId, 0, sizeof(UniqueId));
        UniqueId.len = IPP_IAP2_UNIQUE_ID_MAX;
        rc = ippiAP2DBGetMediaLibraryID(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, &UniqueId);
    }

    if(rc == IPOD_PLAYER_OK)
    {
        /* Playtrack(PlayMediaLibraryItem) */
        iap2Device = iPodCtrlCfg->iap2Param.device;             /* iAP2 device object */
        rc = ippiAP2PlayMediaLibraryItem(iap2Device, &MItemPID, trackID, (U8 **)(&(UniqueId.id)));
    }

    /* parameters free */
    if(catList.categories != NULL)
    {
        if(idList.mediaId != NULL)
        {
            free(idList.mediaId);
        }
        free(catList.categories);
    }
 
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}


