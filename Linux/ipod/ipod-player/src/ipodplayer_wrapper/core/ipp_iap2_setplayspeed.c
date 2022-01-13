/****************************************************
 *  ipp_iap2_setplayspeed.c                          
 *  Created on: 2014/01/29 17:00:00                  
 *  Implementation of the Class ipp_iap2_setplayspeed
 *  Original author: madachi                         
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_hidcommon.h"
#include "ipp_iap2_setplayspeed.h"

S32 ippiAP2SetPlaySpeed(IPOD_PLAYER_CORE_IPODCTRL_CFG       *iPodCtrlCfg,
                        IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32                         rc = IPOD_PLAYER_ERROR;
    iAP2Device_t*               iap2Device = NULL;
    IPOD_PLAYER_PLAYING_SPEED   playspeed;

/* Group 1 start table */
    ReportTable_t   reports_tbl_sta1[] = {  IAP2_STOP_HID,
                                            IAP2_START_HID_GROUP1,
                                            IAP2_RELEASE_REPORT         | IAP2_RESTORE_FLAG,
                                            IAP2_REPORT_TBL_STOP};
/* Group 2 start table */
    ReportTable_t   reports_tbl_sta2[] = {  IAP2_STOP_HID,
                                            IAP2_START_HID_GROUP2,
                                            IAP2_REPORT_TBL_STOP};
/* Tracking Increment table */
    ReportTable_t   reports_tbl_inc[] = {   IAP2_TRACKING_INC_REPORT    | IAP2_NOT_SAVE_FLAG,
                                            IAP2_RELEASE_REPORT         | IAP2_NOT_SAVE_FLAG,
                                            IAP2_REPORT_TBL_STOP};
/* Tracking Decrement table */
    ReportTable_t   reports_tbl_dec[] = {   IAP2_TRACKING_DEC_REPORT    | IAP2_NOT_SAVE_FLAG,
                                            IAP2_RELEASE_REPORT         | IAP2_NOT_SAVE_FLAG,
                                            IAP2_REPORT_TBL_STOP};
/* Tracking Normal table */
    ReportTable_t   reports_tbl_nr[] = {    IAP2_TRACKING_NR_REPORT     | IAP2_NOT_SAVE_FLAG,
                                            IAP2_RELEASE_REPORT         | IAP2_NOT_SAVE_FLAG,
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
    iap2Device = iPodCtrlCfg->iap2Param.device;

    /* Restart HID */
    rc = ippiAP2SendHIDReports(iap2Device, reports_tbl_sta2);
    if(rc == IPOD_PLAYER_OK)
    {
        playspeed = param->contents->setPlaySpeed.speed;

    /* Set playspeed */
        if(playspeed == IPOD_PLAYER_PLAYING_SPEED_NORMAL)
        {
            rc = ippiAP2SendHIDReports(iap2Device, reports_tbl_nr);     /* Tracking Normal */
        }
        else if(playspeed == IPOD_PLAYER_PLAYING_SPEED_SLOW)
        {
            rc = ippiAP2SendHIDReports(iap2Device, reports_tbl_dec);    /* Tracking Decremant */
        }
        else if(playspeed == IPOD_PLAYER_PLAYING_SPEED_FAST)
        {
            rc = ippiAP2SendHIDReports(iap2Device, reports_tbl_inc);    /* Tracking Incremant */
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
        }
    /* Stop HID and release */
        if(rc == IPOD_PLAYER_OK)
        {
            rc = ippiAP2SendHIDReports(iap2Device, reports_tbl_sta1);
        }

    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

