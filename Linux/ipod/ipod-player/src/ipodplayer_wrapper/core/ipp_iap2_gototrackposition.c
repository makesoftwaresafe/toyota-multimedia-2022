/****************************************************
 *  ipp_iap2_gototrackposition.c                             
 *  Created on:         2017/02/02 12:00:00          
 *  Implementation of the Class ippiAP2GotoTrackPosition   
 *  Original author: madachi                         
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_gototrackposition.h"
#include "ipp_iap2_database.h"

/***************************************************
  iPodPlayer playTrack
****************************************************/
S32 ippiAP2GotoTrackPosition(IPOD_PLAYER_CORE_IPODCTRL_CFG          *iPodCtrlCfg,
                             IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM    *param)
{
    S32                         ret = IPOD_PLAYER_OK;
    S32                         rc = IAP2_OK;
    iAP2Device_t*               iap2Device = NULL;
    iAP2SetNowPlayingInformationParameter setNowPlayingInformationPara;
    U32 ElapsedTime = param->waitData->contents.gotoTrackPosition.times;
    IPOD_PLAYER_PLAYBACK_STATUS status;
    IPOD_PLAYER_TRACK_INFO info;


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
    
    /* get media item from playingItem DB */
    rc = ippiAP2DBGetNowPlayingUpdateTrackInfo(iPodCtrlCfg->iap2Param.dbHandle, IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH, &info);
    if(rc == IPOD_PLAYER_OK)
    {
        /* get playback queue index from iPodInfo DB */
        status.playbackStatusActiveMask = IPOD_PLAYER_PLAY_ACT_TRACK_INDEX; /* track index is active */
        ret = ippiAP2DBGetPlaybackStatus(iPodCtrlCfg->iap2Param.dbHandle, sizeof(status), (U8 *)&status);
        if(ret == IPOD_PLAYER_OK)
        {
            /* check parameter for moving time. */
            if(info.length >= ElapsedTime)
            {
                /* Execute operation */
                memset(&setNowPlayingInformationPara, 0, sizeof(setNowPlayingInformationPara));

                rc = ippiAP2AllocateandUpdateData(&setNowPlayingInformationPara.iAP2ElapsedTime,
                                               &ElapsedTime,
                                               &setNowPlayingInformationPara.iAP2ElapsedTime_count,
                                               1,
                                               iAP2_uint32);
             
                if(rc == IAP2_OK)
                {
                    iap2Device = iPodCtrlCfg->iap2Param.device;             /* iAP2 device object */

                    /* send SetNowPlayingInformation */
                    rc = iAP2SetNowPlayingInformation(iap2Device, &setNowPlayingInformationPara);
                    if(rc != IAP2_OK){
                        IPOD_DLT_ERROR("SetNowPlayingInformation execution error rc = %d", rc); 
                    }
                }

                /* Free API parameter buffer */
                iAP2FreeiAP2SetNowPlayingInformationParameter(&setNowPlayingInformationPara);

                /* Conver to error code of iPodPlayer from error code of iAP2 library */
                ret = ippiAP2RetConvertToiPP(rc);
            }
            else
            {
                IPOD_DLT_ERROR("Invalid Parameter = %d ms", ElapsedTime); 
                ret = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            }
        }
        else
        {
            IPOD_DLT_ERROR("access error to internal data base (handle = %p)", iPodCtrlCfg->iap2Param.dbHandle); 
        }
    }
    else
    {
        IPOD_DLT_ERROR("access error to internal data base (handle = %p)", iPodCtrlCfg->iap2Param.dbHandle); 
    }
 
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return ret;
}
