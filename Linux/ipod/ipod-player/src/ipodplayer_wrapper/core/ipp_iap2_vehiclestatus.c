/****************************************************
 *  ipp_iap2_locationInformation.c       
 *  Created on: 2016/11/29 17:45:00                 
 *  Implementation of the Class ipp_iap2_locationInformation  
 *  Original author: madachi                        
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_vehiclestatus.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreFunc.h"

S32 ippiAP2SetVehicleStatus( IPOD_PLAYER_CORE_IPODCTRL_CFG       *iPodCtrlCfg,
                             IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32     rc = IPOD_PLAYER_OK;
    S32     rc_iap2 = IAP2_OK;
    iAP2VehicleStatusUpdateParameter VSUpdatePara;

 
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
        rc = IPOD_PLAYER_ERR_INVALID_MODE;    /* leave function */
    }
    
   if(rc == IPOD_PLAYER_OK)
    {
        /* Execute operation */
        /* Send Vehicle Status Message */
        memset(&VSUpdatePara, 0, sizeof(iAP2VehicleStatusUpdateParameter));
        if(iPodCtrlCfg->vehicleInfo.iAP2VehicleRange == TRUE)
        {
            /* range parameter */
            U16 range = param->contents->vehicleStatus.status.range;
            rc = ippiAP2AllocateandUpdateData(&(VSUpdatePara.iAP2Range), &range,
                                &(VSUpdatePara.iAP2Range_count),
                                1, iAP2_uint16);
        }
        if((iPodCtrlCfg->vehicleInfo.iAP2VehicleOutsideTemperature == TRUE) && (rc == IPOD_PLAYER_OK))
        {
            /* outside temperature parameter */
            S16 outsideTemperature = param->contents->vehicleStatus.status.outsideTemperature;
            rc = ippiAP2AllocateandUpdateData(&(VSUpdatePara.iAP2OutsideTemperature), &outsideTemperature,
                                &(VSUpdatePara.iAP2OutsideTemperature_count),
                                1, iAP2_int16);
        }
        if((iPodCtrlCfg->vehicleInfo.iAP2VehicleRangeWarning == TRUE) && (rc == IPOD_PLAYER_OK))
        {
            /* range warning */
            U8 rangeWarning = param->contents->vehicleStatus.status.rangeWarning;
            rc = ippiAP2AllocateandUpdateData(&(VSUpdatePara.iAP2RangeWarning), &rangeWarning,
                                &(VSUpdatePara.iAP2RangeWarning_count),
                                1, iAP2_uint8);
        }

        if(rc == IPOD_PLAYER_OK)
        {
            rc_iap2 = iAP2VehicleStatusUpdate(iPodCtrlCfg->iap2Param.device, &VSUpdatePara);

            /* Convert error code of iPodPlayer from error code of iAP2 library */
            rc = ippiAP2RetConvertToiPP(rc_iap2);
        }

        iAP2FreeiAP2VehicleStatusUpdateParameter(&VSUpdatePara);
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

