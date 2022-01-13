/****************************************************
 *  ipp_iap2_play.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_database.h"
#include "ipp_iap2_hidcommon.h"
#include "ipp_iap2_play.h"

/***********************************************************************
    Sending iAP2PlayMediaLibCurrentSelection to device
************************************************************************/
S32 ippiAP2PlayMediaLibCurrentSelection(    iAP2Device_t*   iap2Device,
                                            U8              **uid)
{
    S32 rc  = IAP2_OK;
    iAP2PlayMediaLibraryCurrentSelectionParameter  iAP2PlayMediaLibCurrentSelectionPara;
    
    /* parameter check */
    if((iap2Device == NULL) || (uid == NULL)){
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER);
    }else{
        memset(&iAP2PlayMediaLibCurrentSelectionPara, 0, sizeof(iAP2PlayMediaLibCurrentSelectionPara));
        
        /* set unique identifier to start playback */
        rc = ippiAP2AllocateandUpdateData(&iAP2PlayMediaLibCurrentSelectionPara.MediaLibraryUniqueIdentifier,
                                       &uid,
                                       &iAP2PlayMediaLibCurrentSelectionPara.MediaLibraryUniqueIdentifier_count,
                                       1, iAP2_utf8);

        if(rc == IAP2_OK){
            /* Playback Media library Current Selection */
            rc = iAP2PlayMediaLibraryCurrentSelection(iap2Device, &iAP2PlayMediaLibCurrentSelectionPara);
            IPOD_DLT_INFO("Play :uid=%s ", (U8*)uid);
        }

        /* Free API parameter buffer */
        iAP2FreeiAP2PlayMediaLibraryCurrentSelectionParameter(&iAP2PlayMediaLibCurrentSelectionPara);

    }

    /* Conver to error code of iPodPlayer from error code of iAP2 library */
    rc = ippiAP2RetConvertToiPP(rc);

    return rc;
}

S32 ippiAP2Play(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    iAP2Device_t* iap2Device = NULL;
    UniqueId_t UniqueId;
    ReportTable_t   reports_tbl[] = {   IAP2_PLAY_REPORT,
                                        IAP2_RELEASE_REPORT,
                                        IAP2_REPORT_TBL_STOP};

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

    /* Execute operation */
    /* iAP2 device object */
    iap2Device = (iAP2Device_t*)iPodCtrlCfg->iap2Param.device;

    /* Set Play Current Selection Flag */
    param->waitData->contents.playResult.playCurSel = param->waitData->contents.play.playCurSel;
    if(param->waitData->contents.play.playCurSel)
    {
        /* Current selection playing */
        memset(&UniqueId, 0, sizeof(UniqueId));
        UniqueId.len = IPP_IAP2_UNIQUE_ID_MAX;
        /* Get Media Library UniqueId */
        rc = ippiAP2DBGetMediaLibraryID(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, &UniqueId);
        if(rc == IPOD_PLAYER_OK)
        {
            /* Current Selection Playtrack (PlayMediaLibraryCurrentSelection) */
            rc = ippiAP2PlayMediaLibCurrentSelection(iPodCtrlCfg->iap2Param.device, (U8 **)(&(UniqueId.id)));
        }
    }
    else
    {
        /* Normal playing */
        rc = ippiAP2SendHIDReports(iap2Device, reports_tbl);
        IPOD_DLT_INFO("Play");
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

