/****************************************************
 *  ipp_iap2_play.c
 *  Created on: 2014/01/22 15:42:00
 *  Implementation of the Class ipp_iap2_play
 *  Original author: madachi
 ****************************************************/
#include "ipp_iap2_common.h"
#include "ipp_iap2_devinit.h"
#include "ipp_iap2_observer.h"
#include "ipp_iap2_eventnotification.h"
#include "pthread_adit.h"

/* PRQA: Lint Message 456: Mutex lock is seperated by argument of this function. */
S32 ippiAP2DelayedEventMutex(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_DELAYED_MUTEX_FUNC func)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;
    
    if(func == IPP_DELAYED_MUTEX_INIT)
    {
        rcm = pthread_mutex_init(&iPodCtrlCfg->delayedEvent.delayedMutex, NULL);
        if(rcm != 0)
        {
            IPOD_DLT_ERROR("Couldn't get mutex resource rc = %d error = %d", rcm, errno);
            rc = IPOD_PLAYER_ERROR;
        }
    }

    if(func == IPP_DELAYED_MUTEX_LOCK)
    {
        rcm = pthread_mutex_lock(&iPodCtrlCfg->delayedEvent.delayedMutex);
        if(rcm != 0)
        {
            IPOD_DLT_ERROR("Couldn't lock mutex rc = %d error = %d", rcm, errno);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    
    if(func == IPP_DELAYED_MUTEX_UNLOCK)    /*lint !e456 */
    {
        rcm = pthread_mutex_unlock(&iPodCtrlCfg->delayedEvent.delayedMutex);
        if(rcm != 0)
        {
            IPOD_DLT_ERROR("Couldn't unlock mutex rc = %d error = %d", rcm, errno);
            rc = IPOD_PLAYER_ERROR;
        }
    }

/* PRQA: Lint Message 454: Mutex unlock is done by other function. */
/* PRQA: Lint Message 456: Mutex lock is seperated by argument of this function. */

    return rc;      /*lint !e456 !e454 */
}

/* location information event */
static S32 locationEvent(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_LOCATION_INFO locationInfo)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_NOTIFY_LOCATION_INFO_STATUS notifyLocation;

    memset(&notifyLocation, 0, sizeof(notifyLocation));

    notifyLocation.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_LOCATION_INFO_STATUS;
    notifyLocation.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    notifyLocation.status = locationInfo.status;
    notifyLocation.locationMask = locationInfo.locationMask;
    
    /* Notify to iPodPlayerCore */
    rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyLocation, sizeof(notifyLocation));

    return rc;
}
/* vehicle status information event */
static S32 vehicleInfoEvent(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_VEHICLE_STATUS vehicleStatus)
{
    S32 rc = IPOD_PLAYER_OK;
    IPOD_PLAYER_PARAM_NOTIFY_VEHICLE_STATUS notifyVehicleStatus;

    memset(&notifyVehicleStatus, 0, sizeof(notifyVehicleStatus));
    notifyVehicleStatus.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_VEHICLE_STATUS;
    notifyVehicleStatus.header.devID = iPodCtrlCfg->threadInfo->appDevID;
    notifyVehicleStatus.status = vehicleStatus.status;
    
    /* Notify to iPodPlayerCore */
    rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&notifyVehicleStatus, sizeof(notifyVehicleStatus));

    return rc;
}

/* iOS App Info event */
static S32 iOSAppInfoEvent(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_IOS_APP iosApp)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_PARAM_NOTIFY_OPEN_APP openApp;
    IPOD_PLAYER_PARAM_NOTIFY_CLOSE_APP closeApp;

    if(iosApp.appInfo.status == IPODCORE_IOSAPP_OPEN)
    {
        memset(&openApp, 0, sizeof(openApp));
        openApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_OPEN_APP;
        openApp.header.devID = iPodCtrlCfg->threadInfo->appDevID;

        /* Notify the Application of opened application */
        openApp.index = iosApp.devIx;
        openApp.appID = iosApp.appInfo.appID;
        openApp.session = iosApp.appInfo.sessionID;

        /* Send the request that informs the iPodCore of the opened iOS application to iPodCore*/
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&openApp, sizeof(openApp));
    }
    
    if(iosApp.appInfo.status == IPODCORE_IOSAPP_CLOSE)
    {
        memset(&closeApp, 0, sizeof(closeApp));
        closeApp.header.funcId = (IPOD_PLAYER_FUNC_ID)IPOD_FUNC_NOTIFY_CLOSE_APP;
        closeApp.header.devID = iPodCtrlCfg->threadInfo->appDevID;

        /* Notify the Application of closed application */
        closeApp.appID = iosApp.appInfo.appID;
        closeApp.session = iosApp.appInfo.sessionID;

        /* Send the request that informs the iPodCore of the closed iOS application to iPodCore*/
        rc = ippiAP2EventNotificationSendNotifyMessage(iPodCtrlCfg->threadInfo->cmdQueueInfoClient, (U8 *)&closeApp, sizeof(closeApp));
    }

    return rc;
}

S32 ippiAP2DelayedNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    U32 i = 0;
    IPP_IOS_APP iOSApp;

    /* check authentication success with connection status. */
    if(iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS)
    {
        /* notification for location information */
        if(iPodCtrlCfg->delayedEvent.locationIxw > 0)
        {
            for(i = 0; i < iPodCtrlCfg->delayedEvent.locationIxw; i++)
            {
                /* event location information */
                rc = locationEvent(iPodCtrlCfg, iPodCtrlCfg->delayedEvent.locationQueue[i]);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Couldn't send delayed location information event rc = %d", rc);
                }
            }
            iPodCtrlCfg->delayedEvent.locationIxw = 0;
        }
        
        /* notification for vehicle status */
        if(iPodCtrlCfg->delayedEvent.vehicleIxw > 0)
        {
            for(i = 0; i < iPodCtrlCfg->delayedEvent.vehicleIxw; i++)
            {
                /* event vihcle status */
                rc = vehicleInfoEvent(iPodCtrlCfg, iPodCtrlCfg->delayedEvent.vehicleQueue[i]);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Couldn't send delayed vehicle status event rc = %d", rc);
                }
            }
            iPodCtrlCfg->delayedEvent.vehicleIxw = 0;
        }

        /* notification for iOS communication  */
        if(iPodCtrlCfg->delayedEvent.iosIxw > 0)
        {
            for(i = 0; i < iPodCtrlCfg->delayedEvent.iosIxw; i++)
            {
                /* event IOS APP */
                rc = iOSAppInfoEvent(iPodCtrlCfg, iPodCtrlCfg->delayedEvent.iOSAppQueue[i]);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Couldn't send delayed iOS application event rc = %d", rc);
                }
            }
            iPodCtrlCfg->delayedEvent.iosIxw = 0;
        }
        
        if(iPodCtrlCfg->delayedEvent.lastEventNotify)
        {
            if(iPodCtrlCfg->delayedEvent.locationNow.status == IPOD_PLAYER_LOCATION_INFO_START)
            {
                rc = locationEvent(iPodCtrlCfg, iPodCtrlCfg->delayedEvent.locationNow);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Couldn't send delayed location information event rc = %d", rc);
                }
            }

            if(iPodCtrlCfg->delayedEvent.vehicleNow.status == IPOD_PLAYER_VEHICLE_STATUS_START)
            {
                rc = vehicleInfoEvent(iPodCtrlCfg, iPodCtrlCfg->delayedEvent.vehicleNow);
                if(rc != IPOD_PLAYER_OK)
                {
                    IPOD_DLT_ERROR("Couldn't send delayed vehicle status event rc = %d", rc);
                }
            }

            for(i = 0; i < IPODCORE_MAX_IOSAPPS_INFO_NUM; i++)
            {
                if(iPodCtrlCfg->iOSAppID[i].status == IPODCORE_IOSAPP_OPEN)
                {
                    memset(&iOSApp, 0, sizeof(iOSApp));
                    iOSApp.devIx = i;
                    iOSApp.appInfo = iPodCtrlCfg->iOSAppID[i];
                    rc = iOSAppInfoEvent(iPodCtrlCfg, iOSApp);
                    if(rc != IPOD_PLAYER_OK)
                    {
                        IPOD_DLT_ERROR("Couldn't send delayed iOS application event rc = %d", rc);
                    }
                }
            }
            iPodCtrlCfg->delayedEvent.lastEventNotify = FALSE;
        }
    }

    return rc;
}

S32 ippiAP2DevInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg,
                   IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param)
{
    S32 rc = IAP2_CTL_ERROR;

    /* Log for function start */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCSTART, IPOD_LOG_PLAYER_CORE);
    /* Log for function parameter */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, iPodCtrlCfg, param);
    
    /* Parameter check */
    if(!ippiAP2CheckNullParameter(iPodCtrlCfg, param)){
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;   /* leave function */
    }
    
    iPodCtrlCfg->deviceStatusChange = 1;
    
    if(iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS)
    {
        /* set playback mask */
        iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_PLAYBACK_STATUS, 0, 0);
        
        /* set delayed event to check delayed event in initilaize. */
        iPodCtrlCfg->delayedEvent.lastEventNotify = TRUE;
    }

    /* Retunr error is always no reply to Application */
    rc = IPOD_PLAYER_ERR_NO_REPLY;
    
    /* Log for function end */
    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCRETURN, IPOD_LOG_PLAYER_CORE, rc);
    
    return rc;
}

