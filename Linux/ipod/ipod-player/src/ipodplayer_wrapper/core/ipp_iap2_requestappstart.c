/*******************************************************
 *  ipp_iap2_requestappstart.c                          
 *  Created on: 2014/02/12 17:00:00                     
 *  Implementation of the Class ipp_iap2_requestappstart
 *  Original author: madachi                            
 *******************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_requestappstart.h"

/*******************************************
* Request App Launch
********************************************/
S32 ippiAP2RequestAppLaunch(iAP2Device_t* iap2Device, const U8* appname)
{
    S32                                 rc = IAP2_CTL_ERROR;
    iAP2RequestAppLaunchParameter       ReqAppLaunchPara;


    memset(&ReqAppLaunchPara, 0, sizeof(ReqAppLaunchPara));

    if((iap2Device == NULL) || (appname == NULL))
    {
        rc = IAP2_BAD_PARAMETER;
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
    }
    else
    {
        rc = ippiAP2AllocateandUpdateData(  &ReqAppLaunchPara.iAP2AppBundleID,
                                            &appname,
                                            &ReqAppLaunchPara.iAP2AppBundleID_count,
                                            1, iAP2_utf8);
    }
    
    if(rc == IAP2_OK)
    {
        /* Request Application launch */
        rc = iAP2RequestAppLaunch(iap2Device, &ReqAppLaunchPara);
        if(rc != IAP2_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        }

        /* Free API parameter buffer */
        iAP2FreeiAP2RequestAppLaunchParameter(&ReqAppLaunchPara);
    }

    /* Conver to error code of iPodPlayer from error code of iAP2 library */
    rc = ippiAP2RetConvertToiPP(rc);

    return rc;
}

BOOL ippiap2CheckiAPName(IPOD_PLAYER_CORE_IPODCTRL_CFG *cfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    BOOL ret = FALSE;
    IPOD_PLAYER_ACC_IOS_INFO *iosInfo;
    char *appName;
    U32 i = 0;

    if((cfg != NULL) && (param != NULL))
    {
        iosInfo = cfg->threadInfo->iosInfo;
        appName = (char *)param->contents->requestAppStart.appName;
        
        for(i = 0; i < cfg->threadInfo->accInfo.SupportediOSAppCount; i++)
        {
            if(strncmp((char *)iosInfo[i].iOSAppName, appName, IPOD_PLAYER_IOSAPP_NAME_LEN_MAX) == 0)
            {
                ret = TRUE; /* iAP Name exist */
                break;
            }
        }
    }
    
    return ret;
}

S32 ippiAP2RequestAppStart( IPOD_PLAYER_CORE_IPODCTRL_CFG       *iPodCtrlCfg,
                            IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32             rc = IAP2_CTL_ERROR;
    iAP2Device_t*   iap2Device = NULL;
    const U8*       appName = NULL;

    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);

    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param))
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;   /* leave function */
    }

    if(!ippiap2CheckiAPName(iPodCtrlCfg, param))
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
    appName = (const U8*)param->contents->requestAppStart.appName;

    /* Send EAP Session Message */
    rc = ippiAP2RequestAppLaunch(iap2Device, appName);

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

