/****************************************************
 *  ipp_iap2_getdeviceproperty.c                                         
 *  Created on: 2014/01/17 17:55:29                      
 *  Implementation of the Class ipp_iap2_getdeviceproperty       
 *  Original author: mshibata                     
 ****************************************************/

#include "ipp_iap2_getdeviceproperty.h"
#include "ipp_iap2_database.h"
#include "ipp_iap2_observer.h"
#include "ipp_iap2_init.h"

S32 ippiAP2GetDeviceProperty(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    U32 reqMask = 0;
    U32 supportedMask = 0;
    U32 resMask = 0;
    U32 eventMask = 0;
    U8 name[IPOD_PLAYER_IAP2_DB_STRING_MAX_LEN] = {0};
    
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
    
    /* Set the mask to local variable */
    reqMask = param->contents->getDeviceProperty.devicePropertyMask;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, reqMask);
    
    /* Device name bit was set */
    if(IPP_IAP2_IS_MASK_SET(reqMask, IPOD_PLAYER_DEVICE_MASK_NAME))
    {
        /* check identification information - MsgRecvFromDevice: DEVICE_INFORMATION_UPDATE */
        if(ippiAP2CheckIdentificationTableFromDevice(IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE, iPodCtrlCfg->threadInfo))
        {
            /* Get the device name from db */
            rc = ippiAP2DBGetDeviceName(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, (U8 *)name);
            if(rc == IPOD_PLAYER_OK)
            {
                resMask |= IPOD_PLAYER_DEVICE_MASK_NAME;
                strncpy((char *)&param->waitData->contents.getDevicePropertyResult.property.name, (const char *)name, sizeof(param->waitData->contents.getDevicePropertyResult.property.name));
                param->waitData->contents.getDevicePropertyResult.property.name[sizeof(param->waitData->contents.getDevicePropertyResult.property.name) - 1] = '\0';
            }
            else
            {
                memset(&param->waitData->contents.getDevicePropertyResult.property.name, 0, sizeof(param->waitData->contents.getDevicePropertyResult.property.name));
            }
        }
    }
    
    /* Device notification mask bit was set */
    if(IPP_IAP2_IS_MASK_SET(reqMask, IPOD_PLAYER_DEVICE_MASK_EVENT))
    {
        /* Get event notification mask */
        eventMask = iPodCoreObserverGetDeviceEventNotifyDataMask(iPodCtrlCfg);
        param->waitData->contents.getDevicePropertyResult.property.curEvent = eventMask;
        /* The device supported notification is not supported */
        param->waitData->contents.getDevicePropertyResult.property.supportEvent = IPP_IAP2_SUPPORTED_DEVICE_EVENT_MASK;
        resMask |= IPOD_PLAYER_DEVICE_MASK_EVENT;
    }
    
    /* Device notification mask bit was set */
    if(IPP_IAP2_IS_MASK_SET(reqMask, IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE))
    {
        /* Copy the supported feature from internal property */
        param->waitData->contents.getDevicePropertyResult.property.supportedFeatureMask = IPP_IAP2_SUPPORTED_FEATURE_MASK;
        resMask |= IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE;
    }
    
    /* Device coverart format bit was set */
    if(IPP_IAP2_IS_MASK_SET(reqMask, IPOD_PLAYER_DEVICE_MASK_FORMAT))
    {
        /* Copy the count of coverart format from internal property structure */
        param->waitData->contents.getDevicePropertyResult.property.formatCount= 1;
        /* Copy the coverart format from internal property structure */
        param->waitData->contents.getDevicePropertyResult.property.format[0].formatId = IPP_IAP2_COVERART_FORMAT;
        param->waitData->contents.getDevicePropertyResult.property.format[0].imageWidth = 0;
        param->waitData->contents.getDevicePropertyResult.property.format[0].imageHeight = 0;
        resMask |= IPOD_PLAYER_DEVICE_MASK_FORMAT;
    }

    /* Requested property is set */
    supportedMask = IPOD_PLAYER_DEVICE_MASK_NAME | IPOD_PLAYER_DEVICE_MASK_EVENT | IPOD_PLAYER_DEVICE_MASK_SUPPORTED_FEATURE | IPOD_PLAYER_DEVICE_MASK_FORMAT;
    //IPOD_DLT_INFO("[DBG]reqMask :0x%x, resMask :0x%x, supportedMask :0x%x", reqMask, resMask, supportedMask);
    if(resMask != 0)
    {
        param->waitData->contents.getDevicePropertyResult.property.devicePropertyMask = resMask;
        rc = IPOD_PLAYER_OK;
    }
    else if((reqMask & ~supportedMask) == 0)
    {
        param->waitData->contents.getDevicePropertyResult.property.devicePropertyMask = 0;
        rc = IPOD_PLAYER_OK;
    }
    else
    {
        IPOD_DLT_INFO("[ERROR]reqMask :0x%x, resMask :0x%x, supportedMask :0x%x", reqMask, resMask, supportedMask);
        rc = IPOD_PLAYER_ERR_NOT_APPLICABLE;
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);

    return rc;
} 

