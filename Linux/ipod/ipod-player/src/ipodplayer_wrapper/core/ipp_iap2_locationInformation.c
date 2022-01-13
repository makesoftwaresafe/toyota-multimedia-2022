/****************************************************
 *  ipp_iap2_locationInformation.c       
 *  Created on: 2016/11/29 17:45:00                 
 *  Implementation of the Class ipp_iap2_locationInformation  
 *  Original author: madachi                        
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_locationInformation.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreFunc.h"

S32 ippiAP2SetLocationInformation( IPOD_PLAYER_CORE_IPODCTRL_CFG       *iPodCtrlCfg,
                                   IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32     rc = IPOD_PLAYER_OK;
    S32     rc_iap2 = IAP2_OK;
    iAP2LocationInformationParameter locationPara;

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
        /* Send Location Information Message */
        locationPara.iAP2NMEASentence = (U8**)calloc(1, sizeof(U8*));
        if(locationPara.iAP2NMEASentence != NULL)
        {
            *(locationPara.iAP2NMEASentence) = calloc(1, param->contents->locationInfo.size + 1);
            if(*(locationPara.iAP2NMEASentence) != NULL)
            {
                memcpy(*(locationPara.iAP2NMEASentence), param->contents->locationInfo.NMEAdata, param->contents->locationInfo.size);
                locationPara.iAP2NMEASentence_count = 1;

                /* Send location information to iAP2 library */
                rc_iap2 = iAP2LocationInformation(iPodCtrlCfg->iap2Param.device, &locationPara);

                /* Convert error code of iPodPlayer from error code of iAP2 library */
                rc = ippiAP2RetConvertToiPP(rc_iap2);

                free(*(locationPara.iAP2NMEASentence));
            }
            else
            {
                IPOD_DLT_ERROR("Could not get memory resource for location information (size = %zu) ", param->contents->locationInfo.size + 1);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
            free(locationPara.iAP2NMEASentence);
        }
        else
        {
            IPOD_DLT_ERROR("Could not get memory resource for location information (size = %zu) ", sizeof(U8 *));
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }

    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
} 

