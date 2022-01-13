/****************************************************
 *  ipp_iap2_powersourceupdate.c                             
 *  Created on:         2017/02/14 12:00:00          
 *  Implementation of the Class ippiAP2GotoTrackPosition   
 *  Original author: madachi                         
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_powersourceupdate.h"

/***************************************************
  iPodPlayer Power Source Updates
****************************************************/
S32 powerSourceUpdate(iAP2Device_t *device, U16 current, U8 batteryCharge)
{
    S32 ret = IPOD_PLAYER_OK;
    S32 rc = IAP2_OK;
    iAP2PowerSourceUpdateParameter powerSourceUpdatePara;
    
    if(device != NULL)
    {
        memset(&powerSourceUpdatePara, 0, sizeof(powerSourceUpdatePara));

        ret = ippiAP2AllocateandUpdateData(&powerSourceUpdatePara.iAP2AvailableCurrentForDevice,
                                            &current,
                                            &powerSourceUpdatePara.iAP2AvailableCurrentForDevice_count,
                                            1, iAP2_uint16);
        if(ret == IPOD_PLAYER_OK)
        {
            ret = ippiAP2AllocateandUpdateData(&powerSourceUpdatePara.iAP2DeviceBatteryShouldChargeIfPowerIsPresent,
                                            &batteryCharge,
                                            &powerSourceUpdatePara.iAP2DeviceBatteryShouldChargeIfPowerIsPresent_count,
                                            1, iAP2_uint8);
        }

        if(ret == IPOD_PLAYER_OK)
        {
            rc = iAP2PowerSourceUpdate(device, &powerSourceUpdatePara);
            if(rc != IAP2_OK){
                IPOD_DLT_ERROR("Power source update error rc = %d", rc); 
            }

            /* Convert error code of iPodPlayer from error code of iAP2 library */
            ret = ippiAP2RetConvertToiPP(rc);
        }

        iAP2FreeiAP2PowerSourceUpdateParameter(&powerSourceUpdatePara);
    }
    else
    {
        ret = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
 
    return ret;
}

S32 ippiAP2PowerSourceUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG          *iPodCtrlCfg,
                             IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM    *param)
{
    S32 ret = IPOD_PLAYER_OK;

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

    /* execute Power Source updates */
    ret = powerSourceUpdate(iPodCtrlCfg->iap2Param.device, 
                            param->waitData->contents.powerSourceUpdate.powermA,
                            param->waitData->contents.powerSourceUpdate.chargeButtery);
    if(ret == IPOD_PLAYER_OK)
    {
        iPodCtrlCfg->iap2Param.powerInfo.powermA = param->waitData->contents.powerSourceUpdate.powermA;
        iPodCtrlCfg->iap2Param.powerInfo.chargeButtery = param->waitData->contents.powerSourceUpdate.chargeButtery;
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, ret);
    
    return ret;
}
