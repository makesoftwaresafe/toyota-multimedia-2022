/****************************************************
 *  ipp_iap2_sendtoapp.c                            
 *  Created on: 2014/02/12 17:00:00                 
 *  Implementation of the Class ipp_iap2_sendtoapp  
 *  Original author: madachi                        
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_sendtoapp.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreFunc.h"

U64 g_EAPsendCount = 0;
U64 g_EAPsendDataByteCount = 0;

/*******************************************
* Check Application identifier for EAP
********************************************/
BOOL ippiAP2CheckAppID(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 appID)
{
    BOOL ret = FALSE;
    int i= 0;
    
    if(appID != 0)  /* appID must not be 0. */
    {
        for(i = 0; i < IPODCORE_MAX_IOSAPPS_INFO_NUM; i++)
        {
            if(iPodCtrlCfg->iOSAppID[i].appID == appID)
            {
                ret = TRUE;
                break;
            }
        }
    }

    return ret;
}

/*******************************************
* Send EAP Session Message
********************************************/
S32 ippiAP2SendEAPSessionMessage(   iAP2Device_t*   iap2Device,     /* device object */
                                    const U8*       iOSDataSend,    /* Pointer of iOS send data */
                                    U32             iOSDataLen,     /* length of iOS send data */
                                    U16             iOSAppId)       /* iOS App Id */
{
    S32 rc = IAP2_CTL_ERROR;

    if((iap2Device == NULL) || (iOSDataSend == NULL) || (iOSDataLen == 0))
    {
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    else
    {
        /* Send the EAP Session Message */
        rc = iAP2SendEAPSessionMessage(iap2Device, iOSDataSend, iOSDataLen, iOSAppId);
        if(rc != IAP2_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }
    }

    /* Conver to error code of iPodPlayer from error code of iAP2 library */
    rc = ippiAP2RetConvertToiPP(rc);

    return rc;
}

S32 ippiAP2SendToApp(   IPOD_PLAYER_CORE_IPODCTRL_CFG       *iPodCtrlCfg,
                        IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32                         rc = IPOD_PLAYER_ERROR;
    iAP2Device_t*               iap2Device = NULL;
    const U8*                   data = NULL;
    U32                         size = 0;
    U32                         handle = 0;
    U32                         sendSize = 0;
    IPOD_PLAYER_CORE_WAIT_LIST  *pwl = NULL;

    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);

    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;   /* leave function */
    }

    pwl = param->waitData;
    /* buffer check */
    if(pwl->storeBuf == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM, pwl->storeBuf);
        return IPOD_PLAYER_ERR_NOMEM;           /* leave function */
    }

    /* Here checks the status whether this function can execute */
    if(!ippiAP2CheckConnectionReady(&iPodCtrlCfg->deviceConnection))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg->deviceConnection.deviceStatus,
                                                iPodCtrlCfg->deviceConnection.authStatus, iPodCtrlCfg->iPodInfo->mode);
        rc = IPOD_PLAYER_ERR_INVALID_MODE;

    }
    else
    {
        /* Execute operation */
        /* iAP2 device object */
        iap2Device = iPodCtrlCfg->iap2Param.device;
        memcpy((S8 *)&handle,   (const S8 *)pwl->storeBuf, sizeof(handle));
        memcpy((S8 *)&size,     (const S8 *)&(pwl->storeBuf[sizeof(handle)]), sizeof(size));

        if(ippiAP2CheckAppID(iPodCtrlCfg, handle))
        {
            pwl->contents.sendToApp.handle = handle;
            pwl->contents.sendToApp.dataSize = size;
    /*        if(iPodCtrlCfg->iPodInfo->property.maxPayload <= (size - pwl->storeData)){ */
            if(IPP_SENDTOAPP_PAYLOAD_SIZE <= (size - pwl->storeData))
            {
    /*            sendSize = iPodCtrlCfg->iPodInfo->property.maxPayload; */
                sendSize = IPP_SENDTOAPP_PAYLOAD_SIZE;
            }
            else
            {
                sendSize = size - pwl->storeData;
            }

            if(pwl->storeBuf != NULL)
            {
                g_EAPsendCount++;
                g_EAPsendDataByteCount += sendSize;
                if((g_EAPsendCount & 0xff) == 0)
                {
                    IPOD_DLT_VERBOSE("sendCount=%llu, sendDataByteCount=%llu", g_EAPsendCount, g_EAPsendDataByteCount);
                }

                data = &(pwl->storeBuf[sizeof(handle) + sizeof(size) + sizeof(pwl->contents.sendToApp.data)]);

                /* send EAP session message */
                rc = ippiAP2SendEAPSessionMessage(iap2Device, &(data[pwl->storeData]), sendSize, pwl->contents.sendToApp.handle);
                if(rc == IPOD_PLAYER_OK)
                {
                    if(pwl->storeData + sendSize < pwl->contents.sendToApp.dataSize)
                    {
                        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, IPODCORE_QUEUE_STATUS_RUNNING);
                        pwl->storeData += sendSize;
                        rc = IPOD_PLAYER_ERR_REQUEST_CONTINUE;
                        iPodCoreiPodCtrlSetTimer(iPodCtrlCfg->iPodInfo->nextRequestFd, IPOD_PLAYER_TIMER_IMMEDIATE, 0);
                    }
                    else
                    {
                        /* Set the result command size */
                        param->size = sizeof(IPOD_CB_PARAM_SEND_TO_APP_RESULT);
                    }
                }
            }
            else
            {
                rc = IPOD_PLAYER_ERROR;
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            }
        }
        else
        {
            rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, handle);

        }
    }

    if(rc == IPOD_PLAYER_OK)
    {
        handle = pwl->contents.sendToApp.handle;
        pwl->contents.sendToAppResult.header.funcId = IPOD_FUNC_SEND_TO_APP;
        pwl->contents.sendToAppResult.handle = handle;
        pwl->contents.sendToAppResult.header.longData = 0;
        IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCDETAIL, IPOD_LOG_PLAYER_CORE, rc);
    }
    else
    {
        /* Set the result command size */
        param->size = sizeof(IPOD_CB_PARAM_SEND_TO_APP_RESULT);
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);

    return rc;
} 

