/****************************************************
 *  ipp_iap2_callback.c                                         
 *  Created on: 2014/01/16 17:54:39                      
 *  Implementation of the Class ipp_iap2_callback       
 *  Original author: mshibata                     
 ****************************************************/
#include "ipp_iap2_callback.h"
#include "ipp_iap2_database.h"
#include "ipp_iap2_observer.h"
#include "ipp_iap2_eventnotification.h"
#include "ipp_iap2_common.h"
#include "ipp_iap2_ctrlcfglist.h"
#include "iPodPlayerCoreFunc.h"
#include "ipp_iap2_devinit.h"


static void ippiAP2CallBackUpdateNowPlayingDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, iAP2NowPlayingUpdateParameter* nowPlayingUpdateParameter, U32 *playBackActiveMask);
static void ippiAP2CallBackUpdateMediaLiabraryDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, iAP2MediaLibraryUpdateParameter *mediaLiabraryUpdateParameter, IPOD_PLAYER_MEDIA_LIB_INFO *info, BOOL appleMusicRadio);
static void ippiAP2CallBackUpdatePowerUpdateMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, iAP2PowerUpdateParameter* powerupdateParameter);

static U64 g_EAPrecvCount = 0;
static U64 g_EAPrecvDataByteCount = 0;
extern U64 g_EAPsendCount;
extern U64 g_EAPsendDataByteCount;

S32 ippiAP2InitServiceCallbackRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 getHandle = -1;
    S32 rcmq = 0;
    struct mq_attr mqattr;

    /* Check parameter */
    if((iPodCtrlCfg == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    memset(&mqattr, 0, sizeof(mqattr));
    mqattr.mq_flags = 0;
    mqattr.mq_maxmsg = IPOD_PLAYER_SERVICECALLBACKS_MAXMSG;
    mqattr.mq_msgsize = IPOD_PLAYER_SERVICECALLBACKS_MSGSIZE;

    rcmq = mq_unlink(IPOD_PLAYER_SERVICECALLBACKS_MQNAME);
    IPOD_DLT_INFO("mq_unlink :rcmq=%d, errno=%d", rcmq, errno);
    iPodCtrlCfg->serviceCallbacksEventFD = mq_open(
                     IPOD_PLAYER_SERVICECALLBACKS_MQNAME,
                     O_RDWR | O_CREAT | O_EXCL,
                     S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH,
                     &mqattr
                     );
    if(iPodCtrlCfg->serviceCallbacksEventFD == -1)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
        return IPOD_PLAYER_ERROR;
    }

    /* Register the fd into IPC library */
    rc = iPodPlayerIPCCreateHandle(&getHandle, iPodCtrlCfg->serviceCallbacksEventFD);
    if(rc == IPOD_PLAYER_IPC_OK)
    {
        /* Register the fd into handle table */
        rc = iPodCoreSetHandle(iPodCtrlCfg->handle, &(iPodCtrlCfg->handleNum), iPodCtrlCfg->serviceCallbacksEventFD);
        if(rc != IPOD_PLAYER_OK)
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}

S32 ippiAP2DeinitServiceCallbackRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcmq = 0;
    struct mq_attr mqattr;
    U8 eventValue[] = IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE;
    U32 i = 0;

    /* Check parameter */
    if((iPodCtrlCfg == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Clear the handle from handle table */
    rc = iPodCoreClearHandle(iPodCtrlCfg->handle, &(iPodCtrlCfg->handleNum), iPodCtrlCfg->serviceCallbacksEventFD);
    if(rc == IPOD_PLAYER_OK)
    {
        /* Delete the fd from IPC library */
        rc = iPodPlayerIPCDeleteHandle(iPodCtrlCfg->serviceCallbacksEventFD);
        if(rc == IPOD_PLAYER_IPC_OK)
        {
            rcmq = mq_getattr(iPodCtrlCfg->serviceCallbacksEventFD, &mqattr);
            if(rcmq != -1)
            {
                mqattr.mq_flags |= O_NONBLOCK;
                rcmq = mq_setattr(iPodCtrlCfg->serviceCallbacksEventFD, &mqattr, NULL);
                if(rcmq != -1)
                {
                    for(i = 0; i < IPOD_PLAYER_SERVICECALLBACKS_MAXMSG; i++)
                    {
                        rcmq = mq_receive(iPodCtrlCfg->serviceCallbacksEventFD, (char *)eventValue, sizeof(eventValue), NULL);
                        if(rcmq != -1)
                        {
                            IPOD_DLT_INFO("mq_receive success :%s", eventValue);
                        }
                        else
                        {
                            IPOD_DLT_INFO("mq_receive failed :%d, errno=%d(%s)", rcmq, errno, strerror(errno));
                        }
                    }
                }
                else
                {
                    IPOD_DLT_ERROR("mq_setattr failed:%d, errno=%d(%s)", rcmq, errno, strerror(errno));
                }
            }
            else
            {
                IPOD_DLT_ERROR("mq_getattr failed :%d, errno=%d(%s)", rcmq, errno, strerror(errno));
            }
            rcmq = mq_close(iPodCtrlCfg->serviceCallbacksEventFD);
            if(rcmq != -1)
            {
                rcmq = mq_unlink(IPOD_PLAYER_SERVICECALLBACKS_MQNAME);
                if(rcmq != -1)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
                    rc = IPOD_PLAYER_ERROR;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
                rc = IPOD_PLAYER_ERROR;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rc);
            rc = IPOD_PLAYER_ERROR;
        }
    }

    return rc;
}

static void ippiAP2SendEvent(S32 eventFD, char *eventValue)
{
    S32 rce = 0;
    static U32 debugCount = 0;

    debugCount++;
    //IPOD_DLT_INFO("[DBG]send event value=%s, count=%u", eventValue, debugCount);
    rce = mq_send(eventFD, eventValue, IPOD_PLAYER_SERVICECALLBACKS_MSGSIZE, 0);
    if(rce != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, errno);
    }

    return;
}

S32 ippiAP2CBAuthenticationFailed(iAP2Device_t* thisDevice, iAP2AuthenticationFailedParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;

    IPOD_DLT_ERROR("CB");
    
    return  0;
}

S32 ippiAP2CBAuthenticationSucceeded(iAP2Device_t* thisDevice, iAP2AuthenticationSucceededParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBRequestAuthenticationCertificate(iAP2Device_t* thisDevice, iAP2RequestAuthenticationCertificateParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBRequestAuthenticationChallengeResponse(iAP2Device_t* thisDevice, iAP2RequestAuthenticationChallengeResponseParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBStartIdentification(iAP2Device_t* thisDevice, iAP2StartIdentificationParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBIdentificationAccepted(iAP2Device_t* thisDevice, iAP2IdentificationAcceptedParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBIdentificationRejected(iAP2Device_t* thisDevice, iAP2IdentificationRejectedParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_ERROR("CB");

    return  0;
}

S32 ippiAP2CBAssistiveTouchInformation(iAP2Device_t* thisDevice, iAP2AssistiveTouchInformationParameter* thisParameter, void* context)
{
    S32 rc = IPOD_PLAYER_ERROR;                                     /* for return code       */
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, thisDevice, thisParameter, context);
    
    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_CTL_ERROR;
    }

    IPOD_DLT_INFO("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }
    
    if((thisParameter->IsEnabled_count > 0) && (thisParameter->IsEnabled != NULL))
    {
        /* Update assistive touch status */
        rc = ippiAP2DBSetAssistiveStatus(iPodCtrlCfg->iap2Param.dbHandle, IPP_IAP2_ASSISTIVE_ID, (IPOD_PLAYER_DEVICE_EVENT_ASSISTIVE_STATUS)thisParameter->IsEnabled[0]);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_ASSISTIVE, 0);
        }
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_ASSISTIVETOUCH);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return IAP2_OK;
}

S32 ippiAP2CBBluetoothConnectionUpdate(iAP2Device_t* thisDevice, iAP2BluetoothConnectionUpdateParameter* thisParameter, void* context)
{
    S32 rc = IPOD_PLAYER_ERROR;                                     /* for return code       */
    IAP2_BLUETOOTH_INFO btInfo;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_CTL_ERROR;
    }

    /* Initialize parameter */
    memset(&btInfo, 0, sizeof(btInfo));

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }
    
    if((thisParameter->iAP2BluetoothTransportComponentIdentifier_count > 0) && (thisParameter->iAP2BluetoothTransportComponentIdentifier != NULL))
    {
        btInfo.id = *thisParameter->iAP2BluetoothTransportComponentIdentifier;
        
        if((thisParameter->iAP2BluetoothComponentProfiles_count > 0) && (thisParameter->iAP2BluetoothComponentProfiles != NULL))
        {
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothAdvancedAudioDistributionProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_A2DP;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothAudioVideoRemotecontrolProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_AVRCP;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothHandsFreeProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_HFP;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothHumanInterfaceDeviceProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_HID;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothiAP2LinkProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_AiP;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothPersonalAreaNetworkAccessPointProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_PANA;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothPersonalAreaNetworkClientProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_PANU;
            }
            if(thisParameter->iAP2BluetoothComponentProfiles->iAP2BluetoothPhoneBookAccessProfile_count > 0)
            {
                btInfo.profile |= IPOD_PLAYER_PROFILE_PBAP;
            }
        }
        IPOD_DLT_INFO("CB :ID=%u, profile=0x%04x", btInfo.id, btInfo.profile);
        rc = ippiAP2DBSetBluetoothStatus(iPodCtrlCfg->iap2Param.dbHandle, &btInfo);
        if(rc == IPOD_PLAYER_OK)
        {
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_BT, (1 << btInfo.id));
        }
    }
    else
    {
        IPOD_DLT_WARN("CB :No information");
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_BLUETOOTHCONNECTION);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return  IAP2_OK;
}

S32 ippiAP2CBDeviceAuthenticationCertificate(iAP2Device_t* thisDevice, iAP2DeviceAuthenticationCertificateParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");
    
    return  0;
}

S32 ippiAP2CBDeviceAuthenticationResponse(iAP2Device_t* thisDevice, iAP2DeviceAuthenticationResponseParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBDeviceInformationUpdate(iAP2Device_t* thisDevice, iAP2DeviceInformationUpdateParameter* thisParameter, void* context)
{
    S32 rc = IPOD_PLAYER_ERROR;                                         /* for return code  */
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, thisDevice, thisParameter, context);

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }
    
    if((thisParameter->iAP2DeviceName_count > 0) && (thisParameter->iAP2DeviceName != NULL))
    {
        /* Update device information */
        IPOD_DLT_INFO("CB :deviceName=%s", thisParameter->iAP2DeviceName[0]);
        rc = ippiAP2DBSetDeviceName(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, iPodCtrlCfg->threadInfo->nameInfo.deviceName, thisParameter->iAP2DeviceName[0]);
    }
    else
    {
        IPOD_DLT_WARN("CB :No information");
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_DEVICEINFORMATION);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return IAP2_OK;
}

S32 ippiAP2CBDeviceLanguageUpdate(iAP2Device_t* thisDevice, iAP2DeviceLanguageUpdateParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");
    
    return  0;
}

S32 ippiAP2CBStartExternalAccessoryProtocolSession(iAP2Device_t* thisDevice, iAP2StartExternalAccessoryProtocolSessionParameter* thisParameter, void* context)
{
    S32 rc = IPOD_PLAYER_ERROR;                                         /* for return code  */
    IAP2_IOSAPP_INFO iOSAppInfo;                                        /* for iOS app info */
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, thisDevice, thisParameter, context);

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_CTL_ERROR;
    }

    /* Initialize parameter */
    memset(&iOSAppInfo, 0, sizeof(iOSAppInfo));

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    /* Check open iOS application information */
    if((thisParameter->iAP2ExternalAccesoryProtocolIdentifier_count > 0) &&
        (thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier_count > 0))
    {
        iOSAppInfo.index = thisParameter->iAP2ExternalAccesoryProtocolIdentifier[0];
        iOSAppInfo.session = thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier[0];
        
        IPOD_DLT_INFO("CB :ID=%u, SID=%u, sendCount=%lld, sendDataByteCount=%lld, recvCount=%lld, recvDataByteCount=%lld", iOSAppInfo.index, iOSAppInfo.session, g_EAPsendCount, g_EAPsendDataByteCount, g_EAPrecvCount, g_EAPrecvDataByteCount);

        /* Notify iOS application information */
        if(((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyOpeniOSApplication != NULL)
        {
            rc = ((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyOpeniOSApplication(iPodCtrlCfg, &iOSAppInfo);
        }
    }
    else
    {
        IPOD_DLT_WARN("CB :No information :sendCount=%lld, sendDataByteCount=%lld, recvCount=%lld, recvDataByteCount=%lld", g_EAPsendCount, g_EAPsendDataByteCount, g_EAPrecvCount, g_EAPrecvDataByteCount);
    }
    
    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_STARTEAPSESSION);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return IAP2_OK;
}

S32 ippiAP2CBStopExternalAccessoryProtocolSession(iAP2Device_t* thisDevice, iAP2StopExternalAccessoryProtocolSessionParameter* thisParameter, void* context)
{
    S32 rc = IPOD_PLAYER_ERROR;                                         /* for return code  */
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, thisDevice, thisParameter, context);

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    /* Check close iOS application information */
    if(thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier_count > 0)
    {
        IPOD_DLT_INFO("CB :SID=%u, sendCount=%lld, sendDataByteCount=%lld, recvCount=%lld, recvDataByteCount=%lld", *thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier, g_EAPsendCount, g_EAPsendDataByteCount, g_EAPrecvCount, g_EAPrecvDataByteCount);

        /* Notify iOS application information */
        if(((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyCloseiOSApplication != NULL)
        {
            rc = ((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyCloseiOSApplication(iPodCtrlCfg, *thisParameter->iAP2ExternalAccessoryProtocolSessionIdentifier);
        }
    }
    else
    {
        IPOD_DLT_WARN("CB :No information :sendCount=%lld, sendDataByteCount=%lld, recvCount=%lld, recvDataByteCount=%lld", g_EAPsendCount, g_EAPsendDataByteCount, g_EAPrecvCount, g_EAPrecvDataByteCount);
    }
    
    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_STOPEAPSESSION);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return IAP2_OK;
}

S32 ippiAP2CBReceiveiOSApplicationData(iAP2Device_t *thisDevice, U8 iAP2iOSAppIdentifier, U8 *iAP2iOSAppDataRxd, U16 iAP2iOSAppDataLength, void *context)
{
    S32 rc = IPOD_PLAYER_ERROR;                                         /* for return code */
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_FUNCREARGS, IPOD_LOG_PLAYER_CORE, thisDevice, iAP2iOSAppIdentifier, iAP2iOSAppDataRxd, iAP2iOSAppDataLength, context);

    /* Check parameter */
    if((thisDevice == NULL) || (iAP2iOSAppDataRxd == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, iAP2iOSAppDataRxd, context);
        return IAP2_CTL_ERROR;
    }

    IPOD_DLT_VERBOSE("CB :ID=%u, Len=%u", iAP2iOSAppIdentifier, iAP2iOSAppDataLength);
    g_EAPrecvCount++;
    g_EAPrecvDataByteCount += iAP2iOSAppDataLength;

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    if(((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyReceiveFromiOSApplication != NULL)
    {
        rc = ((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyReceiveFromiOSApplication(iPodCtrlCfg, iAP2iOSAppIdentifier, iAP2iOSAppDataLength, iAP2iOSAppDataRxd);
    }
    
    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_RECEIVEIOSAPPDATA);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return IAP2_OK;
}

S32 ippiAP2CBDeviceHIDReport(iAP2Device_t* thisDevice, iAP2DeviceHIDReportParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBStartLocationInformation(iAP2Device_t* thisDevice, iAP2StartLocationInformationParameter* thisParameter, void* context)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    U32 locationMask = 0;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    if(thisParameter->iAP2GlobalPositioningSystemFixData_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send GlobalPositioningSystemFixData - NMEA GPGGA sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_GPGGA;
    }
    if(thisParameter->iAP2RecommendedMinimumSpecificGPSTransitData_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send RecommendedMinimumSpecificGPSTransitData - NMEA GPRMC sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_GPRMC;
    }
    if(thisParameter->iAP2GPSSatellitesInView_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send GPSSatellitesInView - NMEA GPGSV sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_GPGSV;
    }
    if(thisParameter->iAP2VehicleSpeedData_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send VehicleSpeedData - NMEA PASCD sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_PASCD;
    }
    if(thisParameter->iAP2VehicleGyroData_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send VehicleGyroData - NMEA PAGCD sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_PAGCD;
    }
    if(thisParameter->iAP2VehicleAccelerometerData_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send VehicleAccelerometerData - NMEA PAACD sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_PAACD;
    }
    if(thisParameter->iAP2VehicleHeadingData_count > 0)
    {
        //IPOD_DLT_INFO("[DBG]Received Request to send VehicleHeadingData - NMEA GPHDT sentences");
        locationMask = locationMask | IPOD_PLAYER_NMEA_MASK_GPHDT;
    }

    IPOD_DLT_INFO("CB :mask=0x%08x", locationMask);

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg->delayedEvent.locationQueue[iPodCtrlCfg->delayedEvent.locationIxw].locationMask = locationMask;
    iPodCtrlCfg->delayedEvent.locationQueue[iPodCtrlCfg->delayedEvent.locationIxw].status = IPOD_PLAYER_LOCATION_INFO_START;
    iPodCtrlCfg->delayedEvent.locationNow.locationMask = locationMask;
    iPodCtrlCfg->delayedEvent.locationNow.status = IPOD_PLAYER_LOCATION_INFO_START;
    
    iPodCtrlCfg->delayedEvent.locationIxw++;
    if(iPodCtrlCfg->delayedEvent.locationIxw > IPP_DELAYED_QUEUE_MAX)
    {
        iPodCtrlCfg->delayedEvent.locationIxw = IPP_DELAYED_QUEUE_MAX;
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_STARTLOCATION);

    return IAP2_OK;
}

S32 ippiAP2CBStopLocationInformation(iAP2Device_t* thisDevice, iAP2StopLocationInformationParameter* thisParameter, void* context)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg->delayedEvent.locationQueue[iPodCtrlCfg->delayedEvent.locationIxw].locationMask = 0;
    iPodCtrlCfg->delayedEvent.locationQueue[iPodCtrlCfg->delayedEvent.locationIxw].status = IPOD_PLAYER_LOCATION_INFO_STOP;
    iPodCtrlCfg->delayedEvent.locationNow.locationMask = 0;
    iPodCtrlCfg->delayedEvent.locationNow.status = IPOD_PLAYER_LOCATION_INFO_STOP;
    
    iPodCtrlCfg->delayedEvent.locationIxw++;
    if(iPodCtrlCfg->delayedEvent.locationIxw > IPP_DELAYED_QUEUE_MAX)
    {
        iPodCtrlCfg->delayedEvent.locationIxw = IPP_DELAYED_QUEUE_MAX;
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_STOPLOCATION);

    return IAP2_OK;
}

S32 ippiAP2CBStartVehicleStatusUpdates(iAP2Device_t* thisDevice, iAP2StartVehicleStatusUpdatesParameter* thisParameter, void* context)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    if(thisParameter->iAP2Range_count > 0)
    {
        iPodCtrlCfg->vehicleInfo.iAP2VehicleRange = TRUE;
    }
    if(thisParameter->iAP2OutsideTemperature_count > 0)
    {
        iPodCtrlCfg->vehicleInfo.iAP2VehicleOutsideTemperature = TRUE;
    }
    if(thisParameter->iAP2RangeWarning_count > 0)
    {
        iPodCtrlCfg->vehicleInfo.iAP2VehicleRangeWarning = TRUE;
    }

    iPodCtrlCfg->delayedEvent.vehicleQueue[iPodCtrlCfg->delayedEvent.vehicleIxw].status = IPOD_PLAYER_VEHICLE_STATUS_START;
    iPodCtrlCfg->delayedEvent.vehicleNow.status = IPOD_PLAYER_VEHICLE_STATUS_START;
    
    iPodCtrlCfg->delayedEvent.vehicleIxw++;
    if(iPodCtrlCfg->delayedEvent.vehicleIxw > IPP_DELAYED_QUEUE_MAX)
    {
        iPodCtrlCfg->delayedEvent.vehicleIxw = IPP_DELAYED_QUEUE_MAX;
    }
    
    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_STARTVEHICLESTARTUS);

    return IAP2_OK;
}

S32 ippiAP2CBStopVehicleStatusUpdates(iAP2Device_t* thisDevice, iAP2StopVehicleStatusUpdatesParameter* thisParameter, void* context)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;                  /* for thread info */
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg->delayedEvent.vehicleQueue[iPodCtrlCfg->delayedEvent.vehicleIxw].status = IPOD_PLAYER_VEHICLE_STATUS_STOP;
    iPodCtrlCfg->delayedEvent.vehicleNow.status = IPOD_PLAYER_VEHICLE_STATUS_STOP;
    
    iPodCtrlCfg->delayedEvent.vehicleIxw++;
    if(iPodCtrlCfg->delayedEvent.vehicleIxw > IPP_DELAYED_QUEUE_MAX)
    {
        iPodCtrlCfg->delayedEvent.vehicleIxw = IPP_DELAYED_QUEUE_MAX;
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_STOPVEHICLESTARTUS);

    return IAP2_OK;
}

S32 ippiAP2CBMediaLibraryInformation(iAP2Device_t* thisDevice, iAP2MediaLibraryInformationParameter* thisParameter, void* context)
{
    U32 i = 0;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;
    iAP2MediaLibraryInformationSubParameter *subParam = NULL;

    /* Paramter check */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    if(thisParameter->iAP2MediaLibraryInformationSubParameter == NULL)
    {
        IPOD_DLT_WARN("CB :iAP2MediaLibraryInformationSubParameter is NULL.");
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    for(i = 0; i < thisParameter->iAP2MediaLibraryInformationSubParameter_count; i++)
    {
        subParam = &thisParameter->iAP2MediaLibraryInformationSubParameter[i];
        if(*subParam->iAP2MediaLibraryType == IAP2_LIBRARY_TYPE_LOCAL_DEVICE)
        {
            IPOD_DLT_INFO("CB :Local Device:%s, %s", subParam->iAP2MediaLibraryName[0], subParam->iAP2MediaUniqueIdentifier[0]);
            rc = ippiAP2DBSetMediaLibraryInformation(&iPodCtrlCfg->iap2Param.dbHandle->localDevice, iPodCtrlCfg->threadInfo->nameInfo.deviceName, subParam);
            if(rc == IPOD_PLAYER_OK)
            {
                strncpy((char *)iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaUniqueIdentifier, (const char *)subParam->iAP2MediaUniqueIdentifier[0], sizeof(iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaUniqueIdentifier));
                iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaChange = TRUE;
            }
            else
            {
                IPOD_DLT_ERROR("CB :ippiAP2DBSetMediaLibraryInformation(LOCAL_DEVICE) failed :rc=%d, %s, %s", rc, subParam->iAP2MediaLibraryName[0], subParam->iAP2MediaUniqueIdentifier[0]);
            }
        }
        else if(*subParam->iAP2MediaLibraryType == IAP2_LIBRARY_TYPE_APPLE_MUSIC_RADIO)
        {
            IPOD_DLT_INFO("CB :Apple Music Radio:%s, %s", subParam->iAP2MediaLibraryName[0], subParam->iAP2MediaUniqueIdentifier[0]);
            rc = ippiAP2DBSetMediaLibraryInformation(&iPodCtrlCfg->iap2Param.dbHandle->appleMusicRadio, iPodCtrlCfg->threadInfo->nameInfo.deviceName, subParam);
            if(rc == IPOD_PLAYER_OK)
            {
                strncpy((char *)iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaUniqueIdentifier, (const char *)subParam->iAP2MediaUniqueIdentifier[0], sizeof(iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaUniqueIdentifier));
                iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaChange = TRUE;
            }
            else
            {
                IPOD_DLT_ERROR("CB :ippiAP2DBSetMediaLibraryInformation(APPLE_MUSIC_RADIO) failed :rc=%d, %s, %s", rc, subParam->iAP2MediaLibraryName[0], subParam->iAP2MediaUniqueIdentifier[0]);
            }
        }
        else
        {
            IPOD_DLT_WARN("CB :No information");
        }
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_MEDIALIBRARYINFO);

    return IAP2_OK;
}

S32 ippiAP2CBMediaLibraryUpdate(iAP2Device_t* thisDevice, iAP2MediaLibraryUpdateParameter* thisParameter, void* context)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    U32 i = 0;
    IPOD_PLAYER_MEDIA_LIB_INFO info;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;
    IPOD_PLAYER_IAP2_DB_MEDIA_LIBRARY_HANDLE *handle = NULL;

    /* Paramter check */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_VERBOSE("CB");

    memset(&info, 0, sizeof(info));

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }
    
    if(thisParameter->iAP2MediaLibraryUniqueIdentifier_count > 0)
    {
        if(strncmp((const char *)thisParameter->iAP2MediaLibraryUniqueIdentifier[0], (const char *)iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaUniqueIdentifier, sizeof(iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaUniqueIdentifier)) == 0)
        {
            /* Local Device media library */
            handle = &iPodCtrlCfg->iap2Param.dbHandle->localDevice;
            if(thisParameter->iAP2MediaLibraryReset_count > 0)
            {
                ippiAP2DBDeleteAllItems(handle);
                info.status |= IPOD_PLAYER_MEDIA_RESET;
                iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.mediaLibraryReset = TRUE;
            }

            if(thisParameter->iAP2MediaLibraryRevision_count > 0)
            {
                ippiAP2DBSetMediaLibraryRevision(handle, thisParameter->iAP2MediaLibraryUniqueIdentifier[0], NULL, *thisParameter->iAP2MediaLibraryUpdateProgress);
                IPOD_DLT_VERBOSE("CB :Local Device :%s, oldRevision=%s, newRevision=%s", thisParameter->iAP2MediaLibraryUniqueIdentifier[0], iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbRevision, thisParameter->iAP2MediaLibraryRevision[0]);
                strncpy((char *)iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbRevision, (const char *)thisParameter->iAP2MediaLibraryRevision[0], sizeof(iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbRevision));

                info.progress = *thisParameter->iAP2MediaLibraryUpdateProgress;
                info.status |= IPOD_PLAYER_MEDIA_UPDATE_PROGRESS;
                iPodCtrlCfg->iap2Param.dbStatusOfLocalDevice.dbUpdateProgress = TRUE;
            }

            if(thisParameter->iAP2MediaItem_count > 0)
            {
                ippiAP2SetMediaItemDB(handle, thisParameter->iAP2MediaItem_count, thisParameter->iAP2MediaItem);
                info.status |= IPOD_PLAYER_MEDIA_ITEM_UPDATE;
            }

            if(thisParameter->iAP2MediaItemDeletePersistentIdentifier_count > 0)
            {
                ippiAP2DBDeleteMediaItem(handle, thisParameter->iAP2MediaItemDeletePersistentIdentifier_count, thisParameter->iAP2MediaItemDeletePersistentIdentifier);
                info.status |= IPOD_PLAYER_MEDIA_ITEM_DELETE;
            }

            if(thisParameter->iAP2MediaPlayList_count > 0)
            {
                ippiAP2DBSetPlaylist(handle, thisParameter->iAP2MediaPlayList_count, thisParameter->iAP2MediaPlayList);
                for(i = 0; i < thisParameter->iAP2MediaPlayList_count; i++)
                {
                    if(thisParameter->iAP2MediaPlayList[i].iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier_count > 0)
                    {
                        ippiAP2FileXferInit(&iPodCtrlCfg->iap2Param.xferList, IPOD_PLAYER_XFER_TYPE_PLAYLIST_LOCAL, *thisParameter->iAP2MediaPlayList[i].iAP2MediaPlaylistPersistentIdentifier, *thisParameter->iAP2MediaPlayList[i].iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier);
                    }
                }
                info.status |= IPOD_PLAYER_MEDIA_PLAYLIST_UPDATE;
            }

            if(thisParameter->iAP2MediaLibraryIsHidingRemoteItems_count > 0)
            {
                ippiAP2DBSetIsHiding(handle, thisParameter->iAP2MediaLibraryUniqueIdentifier[0], *thisParameter->iAP2MediaLibraryIsHidingRemoteItems);
            }
    
            if(thisParameter->iAP2MediaPlaylistDeletePersistentIdentifier_count > 0)
            {
                ippiAP2DBDeletePlaylist(handle, thisParameter->iAP2MediaPlaylistDeletePersistentIdentifier_count, thisParameter->iAP2MediaPlaylistDeletePersistentIdentifier);
                info.status |= IPOD_PLAYER_MEDIA_PLAYLIST_DELETE;
            }

            ippiAP2CallBackUpdateMediaLiabraryDataMask(iPodCtrlCfg, thisParameter, &info, FALSE);
        }
        else if(strncmp((const char *)thisParameter->iAP2MediaLibraryUniqueIdentifier[0], (const char *)iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaUniqueIdentifier, sizeof(iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaUniqueIdentifier)) == 0)
        {
            /* Apple Music Radio media library */
            handle = &iPodCtrlCfg->iap2Param.dbHandle->appleMusicRadio;
            if(thisParameter->iAP2MediaLibraryReset_count > 0)
            {
                ippiAP2DBDeleteAllItems(handle);
                info.status |= IPOD_PLAYER_MEDIA_RESET;
                iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.mediaLibraryReset = TRUE;
            }

            if(thisParameter->iAP2MediaLibraryRevision_count > 0)
            {
                ippiAP2DBSetMediaLibraryRevision(handle, thisParameter->iAP2MediaLibraryUniqueIdentifier[0], NULL, *thisParameter->iAP2MediaLibraryUpdateProgress);
                IPOD_DLT_VERBOSE("CB :Apple Music Radio :%s, oldRevision=%s, newRevision=%s", thisParameter->iAP2MediaLibraryUniqueIdentifier[0], iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbRevision, thisParameter->iAP2MediaLibraryRevision[0]);
                strncpy((char *)iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbRevision, (const char *)thisParameter->iAP2MediaLibraryRevision[0], sizeof(iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbRevision));

                info.progress = *thisParameter->iAP2MediaLibraryUpdateProgress;
                info.status |= IPOD_PLAYER_MEDIA_UPDATE_PROGRESS;
                iPodCtrlCfg->iap2Param.dbStatusOfAppleMusicRadio.dbUpdateProgress = TRUE;
            }

            if(thisParameter->iAP2MediaItem_count > 0)
            {
                ippiAP2SetMediaItemDB(handle, thisParameter->iAP2MediaItem_count, thisParameter->iAP2MediaItem);
                info.status |= IPOD_PLAYER_MEDIA_ITEM_UPDATE;
            }

            if(thisParameter->iAP2MediaItemDeletePersistentIdentifier_count > 0)
            {
                ippiAP2DBDeleteMediaItem(handle, thisParameter->iAP2MediaItemDeletePersistentIdentifier_count, thisParameter->iAP2MediaItemDeletePersistentIdentifier);
                info.status |= IPOD_PLAYER_MEDIA_ITEM_DELETE;
            }

            if(thisParameter->iAP2MediaPlayList_count > 0)
            {
                ippiAP2DBSetPlaylist(handle, thisParameter->iAP2MediaPlayList_count, thisParameter->iAP2MediaPlayList);
                for(i = 0; i < thisParameter->iAP2MediaPlayList_count; i++)
                {
                    if(thisParameter->iAP2MediaPlayList[i].iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier_count > 0)
                    {
                        ippiAP2FileXferInit(&iPodCtrlCfg->iap2Param.xferList, IPOD_PLAYER_XFER_TYPE_PLAYLIST_AMR, *thisParameter->iAP2MediaPlayList[i].iAP2MediaPlaylistPersistentIdentifier, *thisParameter->iAP2MediaPlayList[i].iAP2MediaPlaylistContainedMediaItemsFileTransferIdentifier);
                    }
                }
                info.status |= IPOD_PLAYER_MEDIA_PLAYLIST_UPDATE;
            }

            if(thisParameter->iAP2MediaLibraryIsHidingRemoteItems_count > 0)
            {
                ippiAP2DBSetIsHiding(handle, thisParameter->iAP2MediaLibraryUniqueIdentifier[0], *thisParameter->iAP2MediaLibraryIsHidingRemoteItems);
            }
    
            if(thisParameter->iAP2MediaPlaylistDeletePersistentIdentifier_count > 0)
            {
                ippiAP2DBDeletePlaylist(handle, thisParameter->iAP2MediaPlaylistDeletePersistentIdentifier_count, thisParameter->iAP2MediaPlaylistDeletePersistentIdentifier);
                info.status |= IPOD_PLAYER_MEDIA_PLAYLIST_DELETE;
            }

            ippiAP2CallBackUpdateMediaLiabraryDataMask(iPodCtrlCfg, thisParameter, &info, TRUE);
        }
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_MEDIALIBRARYUPDATE);

    return IAP2_OK;
}

S32 ippiAP2CBNowPlayingUpdateParameter(iAP2Device_t* thisDevice, iAP2NowPlayingUpdateParameter* thisParameter, void* context)
{
    S32 rc = IAP2_OK;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Paramter check */
    if((thisDevice == NULL) ||
       (thisParameter == NULL) ||
       ((thisParameter->iAP2MediaItemAttributes_count > 0) && (thisParameter->iAP2MediaItemAttributes == NULL)) ||
       ((thisParameter->iAP2PlaybackAttributes_count > 0) && (thisParameter->iAP2PlaybackAttributes == NULL)) ||
       (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_VERBOSE("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg->iap2Param.playbackStatusActiveMask = 0;
    ippiAP2CallBackUpdateNowPlayingDataMask(iPodCtrlCfg, thisParameter, &(iPodCtrlCfg->iap2Param.playbackStatusActiveMask));
    /* store active mask to playbackStatusSetMask to use iPodPlayerGetPlaybackStatus. */
    iPodCtrlCfg->iap2Param.playbackStatusSetMask |= iPodCtrlCfg->iap2Param.playbackStatusActiveMask;
    
    if(thisParameter->iAP2MediaItemAttributes_count > 0)
    {
        if(thisParameter->iAP2MediaItemAttributes[0].iAP2MediaItemArtworkFileTransferIdentifier_count > 0)
        {
            ippiAP2FileXferInit(&iPodCtrlCfg->iap2Param.xferList, IPOD_PLAYER_XFER_TYPE_COVERART, 0, *thisParameter->iAP2MediaItemAttributes[0].iAP2MediaItemArtworkFileTransferIdentifier);
        }
        rc = ippiAP2SetNowPlayingDB(iPodCtrlCfg->iap2Param.dbHandle, &thisParameter->iAP2MediaItemAttributes[0]);
    }
    
    /* Check playback status */
    if(thisParameter->iAP2PlaybackAttributes_count > 0)
    {
        if(thisParameter->iAP2PlaybackAttributes[0].iAP2PlaybackQueueListTransferID_count > 0)
        {
            ippiAP2FileXferInit(&iPodCtrlCfg->iap2Param.xferList, IPOD_PLAYER_XFER_TYPE_QUEUELIST, 0, *thisParameter->iAP2PlaybackAttributes[0].iAP2PlaybackQueueListTransferID);
        }

        rc = ippiAP2SetiPodInfoDB(iPodCtrlCfg->iap2Param.dbHandle, iPodCtrlCfg->threadInfo->nameInfo.deviceName, &thisParameter->iAP2PlaybackAttributes[0]);
    }
    
    rc = iPodCoreObserverCheckIsUpdateDataMaskSet(iPodCtrlCfg, IPP_IAP2_DATA_MASK_PLAYBACK_STATUS, 0);
    if((rc == IPOD_PLAYER_OK) && (((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyPlayBackStatus != NULL))
    {
        rc = ((IAP2_EVENTNOTIFICATION_FUNC_TABLE *)iPodCtrlCfg->iap2Param.notifyFuncTable)->notifyPlayBackStatus(iPodCtrlCfg);
        iPodCoreObserverClearUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_PLAYBACK_STATUS, 0);
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_NOWPLAYING);

    return rc;
}

S32 ippiAP2CBPowerUpdate(iAP2Device_t* thisDevice, iAP2PowerUpdateParameter* thisParameter, void* context)
{
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Paramter check */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }
    
    ippiAP2CallBackUpdatePowerUpdateMask(iPodCtrlCfg, thisParameter);

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_POWER);

    return IAP2_OK;
}

S32 ippiAP2CBTelephonyCallStateInformation(iAP2Device_t* thisDevice, iAP2TelephonyCallStateInformationParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBTelephonyUpdate(iAP2Device_t* thisDevice, iAP2TelephonyUpdateParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

static S32 ippiAP2ConvertSamplerateToiPPRate(iAP2USBDeviceModeAudioSampleRate iap2Rate, U32 *rate)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    if(rate == NULL)
    {
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    switch(iap2Rate)
    {
    case IAP2_SAMPLERATE_8000HZ:
        *rate = 8000;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_11025HZ:
        *rate = 11025;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_12000HZ:
        *rate = 12000;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_16000HZ:
        *rate = 16000;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_22050HZ:
        *rate = 22050;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_24000HZ:
        *rate = 24000;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_32000HZ:
        *rate = 32000;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_44100HZ:
        *rate = 44100;
        rc = IPOD_PLAYER_OK;
        break;
        
    case IAP2_SAMPLERATE_48000HZ:
        *rate = 48000;
        rc = IPOD_PLAYER_OK;
        break;
    default:
        rc = IPOD_PLAYER_ERROR;
        break;
    }
    
    return rc;
    
}
S32 ippiAP2CBUSBDeviceModeAudioInformation(iAP2Device_t* thisDevice, iAP2USBDeviceModeAudioInformationParameter* thisParameter, void* context)
{
    S32 rc = IPOD_PLAYER_ERROR;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    U32 rate = 0;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Parameter check */
    if((thisDevice == NULL) || (thisParameter == NULL) || (context == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, 
                                                                        thisDevice, thisParameter, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB");

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    if((thisParameter->iAP2SampleRate_count > 0) && (thisParameter->iAP2SampleRate != NULL))
    {
        rc = ippiAP2ConvertSamplerateToiPPRate(*thisParameter->iAP2SampleRate, &rate);
        if(rc == IPOD_PLAYER_OK)
        {
            rc = ippiAP2DBSetSample(iPodCtrlCfg->iap2Param.dbHandle, rate);
            iPodCtrlCfg->iap2Param.sampleRateStatus = TRUE;
        }
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_DEVICEMODEAUDIO);

    IPOD_LOG_INFO_WRITE32(IPOD_LOG_TYPE_SEQUENCE, IPOD_LOG_PLAYER_CORE, rc);
    
    return  IAP2_OK;
}

S32 ippiAP2CBVoiceOverCursorUpdate(iAP2Device_t* thisDevice, iAP2VoiceOverCursorUpdateParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBVoiceOverUpdate(iAP2Device_t* thisDevice, iAP2VoiceOverUpdateParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
}

S32 ippiAP2CBWiFiInformation(iAP2Device_t* thisDevice, iAP2WiFiInformationParameter* thisParameter, void* context)
{
    /* For compiler warning */
    thisDevice = thisDevice;
    thisParameter = thisParameter;
    context = context;
    
    IPOD_DLT_INFO("CB");

    return  0;
} 

S32 ippiAP2CBFileTransferSetup(iAP2Device_t *thisDevice, iAP2FileTransferSession_t *thisSession, void *context)
{
    S32 rc = IAP2_CTL_ERROR;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *info = NULL;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisSession == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisSession, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB :ID=%u, Len=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen);

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2FileXferInit(&iPodCtrlCfg->iap2Param.xferList, IPOD_PLAYER_XFER_TYPE_UNKNOWN, 0, thisSession->iAP2FileTransferID);
    info = ippiAP2FileXferGetInfo(iPodCtrlCfg->iap2Param.xferList, thisSession->iAP2FileTransferID);
    if(info != NULL)
    {
        if(info->buf != NULL)
        {
            IPOD_DLT_WARN("CB :buf is not NULL :%p", info->buf);
            free(info->buf);
            info->buf = NULL;
        }
        if(thisSession->iAP2FileXferRxLen > 0)
        {
            info->buf = calloc(1, thisSession->iAP2FileXferRxLen);
            if(info->buf != NULL)
            {
                info->totalSize = thisSession->iAP2FileXferRxLen;
                info->curSize = 0;
                info->status = IPOD_PLAYER_XFER_STATUS_START;
                rc = IAP2_OK;
            }
            else
            {
                rc = IAP2_CTL_ERROR;
                IPOD_DLT_ERROR("CB :info->buff is null.");
            }
        }
        else
        {
            info->totalSize = 0;
            info->curSize = 0;
            info->status = IPOD_PLAYER_XFER_STATUS_START;
            rc = IAP2_OK;
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
        IPOD_DLT_ERROR("CB :info is null.");
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERSETUP);

    return rc;
}

S32 ippiAP2CBFileTransferDataReceive(iAP2Device_t *thisDevice, iAP2FileTransferSession_t *thisSession, void *context)
{
    S32 rc = IAP2_OK;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *info = NULL;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisSession == NULL) || ( context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisSession, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_VERBOSE("CB :ID=%u, Len=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen);

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    info = ippiAP2FileXferGetInfo(iPodCtrlCfg->iap2Param.xferList, thisSession->iAP2FileTransferID);
    if(info != NULL)
    {
        if(info->totalSize == 0)
        {
            rc = IAP2_OK;
        }
        else if((info->buf != NULL) && (info->curSize + thisSession->iAP2FileXferRxLen <= info->totalSize))
        {
            memcpy(&info->buf[info->curSize], thisSession->iAP2FileXferRxBuf, thisSession->iAP2FileXferRxLen);
            info->curSize += thisSession->iAP2FileXferRxLen;
            rc = IAP2_OK;
        }
        else
        {
            rc = IAP2_CTL_ERROR;
            IPOD_DLT_ERROR("CB :info->buf=%p, info->curSize=%llu, thisSession->iAP2FileXferRxLen=%llu, info->totalSize=%llu", info->buf, info->curSize, thisSession->iAP2FileXferRxLen, info->totalSize);
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
        IPOD_DLT_ERROR("CB :info is null :thisSession->iAP2FileXferRxLen=%llu", thisSession->iAP2FileXferRxLen);
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERDATARECEIVE);

    return rc;
}


S32 ippiAP2CBFileTransferSuccess(iAP2Device_t *thisDevice, iAP2FileTransferSession_t *thisSession, void *context)
{
    S32 rc = IAP2_OK;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *info = NULL;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisSession == NULL) || ( context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisSession, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    info = ippiAP2FileXferGetInfo(iPodCtrlCfg->iap2Param.xferList, thisSession->iAP2FileTransferID);
    if(info != NULL)
    {
        if(info->curSize == info->totalSize)
        {
            info->status = IPOD_PLAYER_XFER_STATUS_DONE;
            rc = IAP2_OK;
            IPOD_DLT_INFO("CB :ID=%u, Len=%llu, totalSize=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen, info->totalSize);
        }
        else
        {
            info->status = IPOD_PLAYER_XFER_STATUS_FAIL;
            rc = IAP2_CTL_ERROR;
            IPOD_DLT_ERROR("CB :ID=%u, Len=%llu, curSize=%llu, totalSize=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen, info->curSize, info->totalSize);
        }
    }
    else
    {
        rc = IAP2_CTL_ERROR;
        IPOD_DLT_ERROR("CB :info is null :ID=%u, Len=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen);
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    //IPOD_DLT_INFO("[DBG]rc=%d, info=%p", rc, info);
    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERSUCCESS);

    return rc;
}


S32 ippiAP2CBFileTransferFailure(iAP2Device_t *thisDevice, iAP2FileTransferSession_t *thisSession, void *context)
{
    S32 rc = IAP2_OK;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *info = NULL;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisSession == NULL) || ( context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisSession, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_ERROR("CB :ID=%u, Len=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen);

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    info = ippiAP2FileXferGetInfo(iPodCtrlCfg->iap2Param.xferList, thisSession->iAP2FileTransferID);
    if(info != NULL)
    {
        info->status = IPOD_PLAYER_XFER_STATUS_FAIL;
        rc = IAP2_OK;
    }
    else
    {
        rc = IAP2_CTL_ERROR;
        IPOD_DLT_ERROR("CB :info is null.");
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERFAILURE);

    return rc;
}

S32 ippiAP2CBFileTransferCancel(iAP2Device_t *thisDevice, iAP2FileTransferSession_t *thisSession, void *context)
{
    S32 rc = IAP2_OK;
    IPOD_PLAYER_CORE_IAP2_FILE_XFER_INFO *info = NULL;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Check parameter */
    if((thisDevice == NULL) || (thisSession == NULL) || ( context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, thisDevice, thisSession, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    IPOD_DLT_INFO("CB :ID=%u, Len=%llu", thisSession->iAP2FileTransferID, thisSession->iAP2FileXferRxLen);

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    info = ippiAP2FileXferGetInfo(iPodCtrlCfg->iap2Param.xferList, thisSession->iAP2FileTransferID);
    if(info != NULL)
    {
        info->status = IPOD_PLAYER_XFER_STATUS_CANCEL;
        rc = IAP2_OK;
    }
    else
    {
        rc = IAP2_CTL_ERROR;
        IPOD_DLT_ERROR("CB :info is null.");
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERCANCEL);

    return rc;
}

/* This function is a past implementation.            */
/* ippiAP2CBDeviceState is not called by iAP2Service. */
/* Instead ippiAP2ServiceDeviceStateCB is called.     */
S32 ippiAP2CBDeviceState(iAP2Device_t* iap2Device, iAP2DeviceState_t dState, void* context)
{
    S32 rc = IAP2_CTL_ERROR;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;

    /* Parameter check */
    if((iap2Device == NULL) || (context == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Device, context);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    iPodCtrlCfg = (IPOD_PLAYER_CORE_IPODCTRL_CFG *)context;

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    switch(dState)
    {
    case iAP2NotConnected :
        /* iAP2NotConnected event happens many so connection status check */
        if(iPodCtrlCfg->deviceConnection.deviceStatus != IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT)
        {
            iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT;
            iPodCtrlCfg->deviceStatusChange++;
        }
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_UNKNOWN;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_UNKNOWN;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
        IPOD_DLT_INFO("Device State: Not Connected");
        break;
        
    case iAP2TransportConnected :
        IPOD_DLT_INFO("Device State: Transport Connected");
        break;
        
    case iAP2LinkConnected:
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP2;
        iPodCtrlCfg->deviceStatusChange++;
        IPOD_DLT_INFO("Device State: Link Connected");
        break;
        
    case iAP2AuthenticationPassed :
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP2;
        iPodCtrlCfg->authIdenPass.authPass = TRUE;
        if(iPodCtrlCfg->authIdenPass.idenPass)
        { 
            iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_SUCCESS;
        }
        else
        {
            iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
        }
        iPodCtrlCfg->deviceStatusChange++;
        IPOD_DLT_INFO("Device State: Authentication Passed");
        break;
        
    case iAP2IdentificationPassed :
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP2;
        iPodCtrlCfg->authIdenPass.idenPass = TRUE;
        if(iPodCtrlCfg->authIdenPass.authPass)
        {
            iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_SUCCESS;
        }
        else
        {
            iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
        }
        iPodCtrlCfg->deviceStatusChange++;
        IPOD_DLT_INFO("Device State: Identification Passed");
        break;
        
    case iAP2DeviceReady:
        IPOD_DLT_INFO("Device State: Device Ready");
        break;
        
    case iAP2LinkiAP1DeviceDetected:
        IPOD_DLT_INFO("Device State: iAP1 Device Detected");
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_IAP1_RESTART;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP1;
        iPodCtrlCfg->deviceStatusChange++;
        break;
        
    default:
        break;
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, rcm);
        return IAP2_CTL_ERROR;
    }

    ippiAP2SendEvent(eventFD, IPP_IAP2_SERVICE_CALLBACK_EVENT_DEVICESTATE);

    rc = IAP2_OK;
    
    return rc;
}

static iAP2StackCallbacks_t *ippiAP2SetSTCallbacks(void)
{
    iAP2StackCallbacks_t *callbacks = NULL;
    
    callbacks = calloc(1, sizeof(*callbacks));
    if(callbacks != NULL)
    {
        callbacks->p_iAP2DeviceState_cb = &ippiAP2CBDeviceState;
    }
    
    return callbacks;
}

static void ippiAP2ClearCSCallbacks(iAP2SessionCallbacks_t *callbacks)
{
    if(callbacks != NULL)
    {
        free(callbacks);
    }
    
    return;
}

static void ippiAP2ClearFTCallbacks(iAP2FileTransferCallbacks_t *callbacks)
{
    if(callbacks != NULL)
    {
        free(callbacks);
    }
    
    return;
}
static void ippiAP2ClearEAPCallbacks(iAP2EAPSessionCallbacks_t *callbacks)
{
    if(callbacks != NULL)
    {
        free(callbacks);
    }
    
    return;
}
static void ippiAP2ClearSTCallbacks(iAP2StackCallbacks_t *callbacks)
{
    if(callbacks != NULL)
    {
        free(callbacks);
    }
    
    return;
}

static iAP2SessionCallbacks_t *ippiAP2SetCSCallbacks(void)
{
    iAP2SessionCallbacks_t *callbacks = NULL;
    
    callbacks = calloc(1, sizeof(*callbacks));
    if(callbacks != NULL)
    {
        callbacks->iAP2AuthenticationFailed_cb                    = ippiAP2CBAuthenticationFailed;
        callbacks->iAP2AuthenticationSucceeded_cb                 = ippiAP2CBAuthenticationSucceeded;
        callbacks->iAP2RequestAuthenticationCertificate_cb        = ippiAP2CBRequestAuthenticationCertificate;
        callbacks->iAP2RequestAuthenticationChallengeResponse_cb  = ippiAP2CBRequestAuthenticationChallengeResponse;
        callbacks->iAP2StartIdentification_cb                     = ippiAP2CBStartIdentification;
        callbacks->iAP2IdentificationAccepted_cb                  = ippiAP2CBIdentificationAccepted;
        callbacks->iAP2IdentificationRejected_cb                  = ippiAP2CBIdentificationRejected;
        callbacks->iAP2BluetoothConnectionUpdate_cb               = ippiAP2CBBluetoothConnectionUpdate;
        callbacks->iAP2DeviceAuthenticationCertificate_cb         = NULL;
        callbacks->iAP2DeviceAuthenticationResponse_cb            = NULL;
        callbacks->iAP2DeviceInformationUpdate_cb                 = ippiAP2CBDeviceInformationUpdate;
        callbacks->iAP2DeviceLanguageUpdate_cb                    = ippiAP2CBDeviceLanguageUpdate;
        callbacks->iAP2TelephonyCallStateInformation_cb           = NULL;
        callbacks->iAP2TelephonyUpdate_cb                         = NULL;
        callbacks->iAP2StartVehicleStatusUpdates_cb               = ippiAP2CBStartVehicleStatusUpdates;
        callbacks->iAP2StopVehicleStatusUpdates_cb                = ippiAP2CBStopVehicleStatusUpdates;
        callbacks->iAP2AssistiveTouchInformation_cb               = ippiAP2CBAssistiveTouchInformation;
        callbacks->iAP2StartExternalAccessoryProtocolSession_cb   = ippiAP2CBStartExternalAccessoryProtocolSession;
        callbacks->iAP2StopExternalAccessoryProtocolSession_cb    = ippiAP2CBStopExternalAccessoryProtocolSession;
        callbacks->iAP2DeviceHIDReport_cb                         = NULL;
        callbacks->iAP2StartLocationInformation_cb                = ippiAP2CBStartLocationInformation;
        callbacks->iAP2StopLocationInformation_cb                 = ippiAP2CBStopLocationInformation;
        callbacks->iAP2MediaLibraryInformation_cb                 = ippiAP2CBMediaLibraryInformation;
        callbacks->iAP2MediaLibraryUpdate_cb                      = ippiAP2CBMediaLibraryUpdate;
        callbacks->iAP2NowPlayingUpdateParameter_cb               = ippiAP2CBNowPlayingUpdateParameter;
        callbacks->iAP2PowerUpdate_cb                             = ippiAP2CBPowerUpdate;
        callbacks->iAP2USBDeviceModeAudioInformation_cb           = ippiAP2CBUSBDeviceModeAudioInformation;
        callbacks->iAP2VoiceOverUpdate_cb                         = NULL;
        callbacks->iAP2WiFiInformation_cb                         = NULL;
    }
    
    return callbacks;
}

static iAP2FileTransferCallbacks_t *ippiAP2SetFTCallbacks(void)
{
    iAP2FileTransferCallbacks_t *callbacks = NULL;
    
    callbacks = calloc(1, sizeof(*callbacks));
    if(callbacks != NULL)
    {
        callbacks->iAP2FileTransferSuccess_cb           = ippiAP2CBFileTransferSuccess;
        callbacks->iAP2FileTransferFailure_cb           = ippiAP2CBFileTransferFailure;
        callbacks->iAP2FileTransferCancel_cb            = ippiAP2CBFileTransferCancel;
        callbacks->iAP2FileTransferPause_cb             = NULL;
        callbacks->iAP2FileTransferResume_cb            = NULL;
        callbacks->iAP2FileTransferDataRcvd_cb          = ippiAP2CBFileTransferDataReceive;
        callbacks->iAP2FileTransferSetup_cb             = ippiAP2CBFileTransferSetup;
    }
    
    return callbacks;
}

static iAP2EAPSessionCallbacks_t *ippiAP2SetEAPCallbacks(void)
{
    iAP2EAPSessionCallbacks_t *callbacks = NULL;
    
    callbacks = calloc(1, sizeof(*callbacks));
    if(callbacks != NULL)
    {
        callbacks->iAP2iOSAppDataReceived_cb = ippiAP2CBReceiveiOSApplicationData;
    }
    
    return callbacks;
}

S32 ippiAP2SetCallbacks(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param)
{
    S32 rc = IPOD_PLAYER_ERROR;
    
    /* Parameter check */
    if(iap2Param == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    
    /* Set callback pointers for control session */
    iap2Param->initParam->p_iAP2CSCallbacks = ippiAP2SetCSCallbacks();
    if(iap2Param->initParam->p_iAP2CSCallbacks != NULL)
    {
        /* Set callback pointers for file trasfer session */
        iap2Param->initParam->p_iAP2FileTransferCallbacks = ippiAP2SetFTCallbacks();
        if(iap2Param->initParam->p_iAP2FileTransferCallbacks != NULL)
        {
            /* Set callback pointers for EAP session */
            iap2Param->initParam->p_iAP2EAPSessionCallbacks = ippiAP2SetEAPCallbacks();
            if(iap2Param->initParam->p_iAP2EAPSessionCallbacks != NULL)
            {
                /* Set callback pointers for device status */
                iap2Param->initParam->p_iAP2StackCallbacks = ippiAP2SetSTCallbacks();
                if(iap2Param->initParam->p_iAP2StackCallbacks != NULL)
                {
                    rc = IPOD_PLAYER_OK;
                }
                else
                {
                    IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                    rc = IPOD_PLAYER_ERR_NOMEM;
                }
            }
            else
            {
                IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
                rc = IPOD_PLAYER_ERR_NOMEM;
            }
        }
        else
        {
            IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
            rc = IPOD_PLAYER_ERR_NOMEM;
        }
    }
    else
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_NOMEM);
        rc = IPOD_PLAYER_ERR_NOMEM;
    }
    
    if(rc != IPOD_PLAYER_OK)
    {
        ippiAP2ClearCallbacks(iap2Param);
    }
    
    return rc;
}

void ippiAP2ClearCallbacks(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param)
{
    /* Parameter check */
    if((iap2Param == NULL) || (iap2Param->initParam == NULL))
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iap2Param);
        return;
    }
    
    if(iap2Param->initParam->p_iAP2StackCallbacks != NULL)
    {
        ippiAP2ClearSTCallbacks(iap2Param->initParam->p_iAP2StackCallbacks);
        iap2Param->initParam->p_iAP2StackCallbacks = NULL;
    }
    
    if(iap2Param->initParam->p_iAP2EAPSessionCallbacks != NULL)
    {
        ippiAP2ClearEAPCallbacks(iap2Param->initParam->p_iAP2EAPSessionCallbacks);
        iap2Param->initParam->p_iAP2EAPSessionCallbacks = NULL;
    }
    
    if(iap2Param->initParam->p_iAP2FileTransferCallbacks != NULL)
    {
        ippiAP2ClearFTCallbacks(iap2Param->initParam->p_iAP2FileTransferCallbacks);
        iap2Param->initParam->p_iAP2FileTransferCallbacks = NULL;
    }
    
    if(iap2Param->initParam->p_iAP2CSCallbacks != NULL)
    {
        ippiAP2ClearCSCallbacks(iap2Param->initParam->p_iAP2CSCallbacks);
        iap2Param->initParam->p_iAP2CSCallbacks = NULL;
    }
    
    return;
}


static void ippiAP2CallBackUpdateNowPlayingDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, iAP2NowPlayingUpdateParameter* nowPlayingUpdateParameter, U32 *playBackActiveMask)
{
    U32 retMask = 0;
    
    /* Check parameter */
    if((iPodCtrlCfg == NULL) || (nowPlayingUpdateParameter == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, nowPlayingUpdateParameter);
        return;
    }
    
    if(nowPlayingUpdateParameter->iAP2MediaItemAttributes_count > 0)
    {
        /* Check track name */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemTitle[0]=%s", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemTitle[0]);
            /* Set track name */
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACK_NAME;
        }
        /* Check album name */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemAlbumTitle_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemAlbumTitle[0]=%s", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemAlbumTitle[0]);
            /* Set album name */
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_ALBUM_NAME;
        }
        /* Check artist name */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemArtist_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemArtist[0]=%s", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemArtist[0]);
            /* Set artist name */
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_ARTIST_NAME;
        }
        /* Check genre */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemGenre_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemGenre[0]=%s", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemGenre[0]);
            /* Set genre */
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_GENRE;
        }
        /* Check composer */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemComposer_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemComposer[0]=%s", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemComposer[0]);
            /* Set composer */
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_COMPOSER;
        }
        /* Check total time */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemPlaybackDurationInMilliseconds_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemPlaybackDurationInMilliseconds[0]=%u", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemPlaybackDurationInMilliseconds[0]);
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_TRACKLENGTH;
        }

        /* Check chapter count */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemChapterCount_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemChapterCount[0]=%d", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemChapterCount[0]);
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CHAPTER_COUNT;
        }
        
        /* Check Artwork fileTransfer ID */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemArtworkFileTransferIdentifier[0]=%d", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemArtworkFileTransferIdentifier[0]);
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_COVERART;
        }

        /* Check track ID */
        if(nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemPersistentIdentifier_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2MediaItemPersistentIdentifier[0]=%llu", nowPlayingUpdateParameter->iAP2MediaItemAttributes->iAP2MediaItemPersistentIdentifier[0]);
            retMask |= IPOD_PLAYER_TRACK_INFO_MASK_CAPABILITY;
        }

        //IPOD_DLT_INFO("[DBG]retMask=%x(%u)", retMask, retMask);
        if(retMask > 0)
        {
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_TRACK_INFO, retMask, 0);
        }
    }
    
    /* Check playback status */
    if(nowPlayingUpdateParameter->iAP2PlaybackAttributes_count > 0)
    {
        retMask = 0;
        
        /* Check play status */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackStatus_count > 0)
        {
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_STATUS;
        }
        /* Check play time */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackElapsedTimeInMilliseconds_count > 0)
        {
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_TIME;
        }
        /* Check shuffle status */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackShuffleMode_count > 0)
        {
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_SHUFFLE;
        }
        /* Check shuffle status */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackRepeatMode_count > 0)
        {
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_REPEAT;
        }
        /* Check Queue Index */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackQueueIndex_count > 0)
        {
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_TRACK_INDEX;
        }
        /* Check Queue Chapter Index */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackQueueChapterIndex_count > 0)
        {
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_CHAPTER_INDEX;
        }
        /* Check play app */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackAppName_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2PlaybackAppName[0]=%s", nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackAppName[0]);
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_APP_NAME;
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_IOSAPP_FULL, 0);
        }
        /* Check App Bundle ID */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackAppBundleID_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2PlaybackAppBundleID[0]=%s", nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackAppBundleID[0]);
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_APP_BUNDLE_ID;
        }
        /* Check Playback Media Library Unique Identifier */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PBMediaLibraryUniqueIdentifier_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2PBMediaLibraryUniqueIdentifier[0]=%s", nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PBMediaLibraryUniqueIdentifier[0]);
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_MEDIA_LIBRARY_UID;
        }
        /* Check Apple Music Radio station name */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PBAppleMusicRadioStationName_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2PBAppleMusicRadioStationName[0]=%s", nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PBAppleMusicRadioStationName[0]);
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_AMR_STATION_NAME;
        }
        /* Check Queue Count */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackQueueCount_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2PlaybackQueueCount[0]=%u", nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackQueueCount[0]);
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_QUEUE_COUNT;
        }
        /* Check Queue List Avail */
        if(nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackQueueListAvail_count > 0)
        {
            //IPOD_DLT_INFO("[DBG]iAP2PlaybackQueueListAvail[0]=%d", nowPlayingUpdateParameter->iAP2PlaybackAttributes->iAP2PlaybackQueueListAvail[0]);
            retMask |= IPP_IAP2_DATA_MASK_PLAYBACK_STATUS;
            *playBackActiveMask |= IPOD_PLAYER_PLAY_ACT_QUEUE_LIST_AVAIL;
        }

        if(retMask > 0)
        {
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_PLAYBACK_STATUS, 0, 0);
        }
    }
    
}

/* Update notification data mask */
static void ippiAP2CallBackUpdateMediaLiabraryDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, iAP2MediaLibraryUpdateParameter *mediaLiabraryUpdateParameter, IPOD_PLAYER_MEDIA_LIB_INFO *info, BOOL appleMusicRadio)
{
    
    /* Check parameter */
    if((iPodCtrlCfg == NULL) || (mediaLiabraryUpdateParameter == NULL) || (info == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, iPodCtrlCfg, mediaLiabraryUpdateParameter, info);
        return;
    }
    
    if((mediaLiabraryUpdateParameter->iAP2MediaLibraryUniqueIdentifier_count > 0) && (mediaLiabraryUpdateParameter->iAP2MediaLibraryRevision_count > 0))
    {
        if(!appleMusicRadio)
        {
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_DATABASE, 0);
            iPodCtrlCfg->iap2Param.iap2MediaParam->infoOfLocalDevice = *info;
        }
        else
        {
            iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_DATABASE_AMR, 0);
            iPodCtrlCfg->iap2Param.iap2MediaParam->infoOfAppleMusicRadio = *info;
        }
    }
}

static void ippiAP2CallBackUpdatePowerUpdateMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, iAP2PowerUpdateParameter* powerupdateParameter)
{
    U32 statusMask = 0;
    
    /* Check parameter */
    if((iPodCtrlCfg == NULL) || (powerupdateParameter == NULL))
    {
        /* Immediately leave the function with error if Parameter is invalid */
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, powerupdateParameter);
        return;
    }
    
    /* Check power update callback */
    if (powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory_count != 0)
    {
        /* Maximum Current Drawn */
        statusMask |= IPOD_PLAYER_CHARGING_CURRENT_MASK;
        iPodCtrlCfg->iap2Param.powerNotify.maxCurrentDrawn = *(powerupdateParameter->iAP2MaximumCurrentDrawnFromAccessory);
    }
    if (powerupdateParameter->iAP2AccessoryPowerMode_count != 0)
    {
        /* Accessory Power Mode */
        statusMask |= IPOD_PLAYER_POWER_MODE_MASK;
        iPodCtrlCfg->iap2Param.powerNotify.powerMode = (IPOD_PLAYER_ACC_POWER_MODES)(*(powerupdateParameter->iAP2AccessoryPowerMode));
    }
    if (powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent_count != 0)
    {
        /* Device Battery will charge if power is present */
        statusMask |= IPOD_PLAYER_CHARGING_BUTTERY_MASK;
        iPodCtrlCfg->iap2Param.powerNotify.chargeButtery = *(powerupdateParameter->iAP2DeviceBatteryWillChargeIfPowerIsPresent);
    }
    if (powerupdateParameter->iAP2IsExternalChargerConnected_count != 0)
    {
        /* External Charger Connected */
        statusMask |= IPOD_PLAYER_EXT_CHARGER_CONNECT_MASK;
    }
    if (powerupdateParameter->iAP2BatteryChargingState_count != 0)
    {
        /* Battery charging state */
        statusMask |= IPOD_PLAYER_CHARGE_STATE_MASK;
        iPodCtrlCfg->iap2Param.powerNotify.butteryState = (IPOD_PLAYER_BATTERY_CHARGING_STATE)(*(powerupdateParameter->iAP2BatteryChargingState));
    }
    if (powerupdateParameter->iAP2BatteryChargeLevel_count != 0)
    {
        /* Battery Charge Level */
        statusMask |= IPOD_PLAYER_CHARGE_LEVEL_MASK;
        iPodCtrlCfg->iap2Param.powerNotify.chargeLevel = *(powerupdateParameter->iAP2BatteryChargeLevel);
    }
    
    if(statusMask != 0)
    {
        iPodCtrlCfg->iap2Param.powerUpdateEventStatus |= statusMask;
        iPodCoreObserverSetUpdateDataMask(iPodCtrlCfg, IPP_IAP2_DATA_MASK_DEVICE_EVENT, IPOD_PLAYER_EVENT_MASK_POWER, 0);
    }
}

int32_t ippiAP2ServiceDeviceConnectedCB(iAP2Service_t* service, iAP2ServiceDeviceList_t* msg)
{
    (void)service;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    S32 eventFD = 0;
    U8 connectedEvent[] = IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE;

    if(msg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, msg);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    if(msg->count == 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, msg->count);
        return IAP2_CTL_ERROR;
    }

    iPodCtrlCfg = ippiAP2SerchBySerialNumFromCtrlCfgList((U8*)msg->list[0].serial); 
    if(iPodCtrlCfg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, iPodCtrlCfg);
        return IAP2_CTL_ERROR;
    }

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm);
        return IAP2_CTL_ERROR;
    }

    IPOD_DLT_INFO("SCB :ID=%u, state=%d, serial=%s, transport=%d, eap=%d, eaNative=%d, carplay=%d", msg->list[0].id, msg->list[0].deviceState, msg->list[0].serial, msg->list[0].transport, msg->list[0].eapSupported, msg->list[0].eaNativeSupported, msg->list[0].carplaySupported);

    iPodCtrlCfg->iap2Param.deviceId = msg->list[0].id;

    if(msg->list[0].deviceState == iAP2TransportConnected)
    {
        if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
           (iPodCtrlCfg->deviceConnection.iapType == IPOD_PLAYER_PROTOCOL_TYPE_IAP2) &&
           (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_RUNNING))
        {
            IPOD_DLT_WARN("SCB :Already transport connected.");
        }
        strncpy((char *)connectedEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICECONNECTEDDEVICE1, sizeof(connectedEvent));
    }
    else if(msg->list[0].deviceState == iAP2DeviceReady)
    {
        if((iPodCtrlCfg->deviceConnection.deviceStatus == IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT) &&
           (iPodCtrlCfg->deviceConnection.iapType == IPOD_PLAYER_PROTOCOL_TYPE_IAP2) &&
           (iPodCtrlCfg->deviceConnection.authStatus == IPOD_PLAYER_AUTHENTICATION_SUCCESS))
        {
            IPOD_DLT_WARN("SCB :Already device ready.");
        }
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP2;
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_SUCCESS;
        iPodCtrlCfg->deviceStatusChange++;
        strncpy((char *)connectedEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICECONNECTEDDEVICE2, sizeof(connectedEvent));
    }
    else
    {
        IPOD_DLT_WARN("SCB :Unusual state :state=%d", msg->list[0].deviceState);
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm);
        return IAP2_CTL_ERROR;
    }

    if(strncmp((const char *)connectedEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE, sizeof(connectedEvent)) != 0)
    {
        ippiAP2SendEvent(eventFD, (char *)connectedEvent);
    }

    return IAP2_OK;
}

int32_t ippiAP2ServiceDeviceDisconnectedCB(iAP2Service_t* service, uint32_t deviceId)
{
    (void)service;
    (void)deviceId;

    IPOD_DLT_INFO("SCB :ID=%u", deviceId);

    return IAP2_OK;
}

int32_t ippiAP2ServiceDeviceStateCB(iAP2Service_t* service, uint32_t deviceId, iAP2ServiceDeviceState_t* msg)
{
    (void)service;
    (void)deviceId;
    IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg = NULL;
    S32 rcm = IPOD_PLAYER_OK;
    U8 stateEvent[] = IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE;
    S32 eventFD = 0;

    if(msg == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, msg);
        return IAP2_INVALID_INPUT_PARAMETER;
    }

    iPodCtrlCfg = ippiAP2SerchBySerialNumFromCtrlCfgList((U8*)msg->serial); 
    if(iPodCtrlCfg == NULL)
    {
        IPOD_DLT_WARN("SCB :No matching device :serial=%s, state=%d", msg->serial, msg->state);
        return IAP2_CTL_ERROR;
    }

    rcm = ippiAP2ServiceCallbacksMutexLock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm);
        return IAP2_CTL_ERROR;
    }

    switch(msg->state)
    {
    case iAP2NotConnected:
        IPOD_DLT_INFO("SCB: ID=%u, Not Connected(%d), serial=%s", deviceId, msg->state, msg->serial);
        /* iAP2NotConnected event happens many so connection status check */
        if(iPodCtrlCfg->deviceConnection.deviceStatus != IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT)
        {
            iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_DISCONNECT;
            iPodCtrlCfg->deviceStatusChange++;
            strncpy((char *)stateEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICEDEVICESTATE, sizeof(stateEvent));
        }
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_UNKNOWN;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_UNKNOWN;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_UNKNOWN;
        break;

    case iAP2TransportConnected:
        IPOD_DLT_INFO("SCB :ID=%u, Transport Connected(%d), serial=%s", deviceId, msg->state, msg->serial);
        break;

    case iAP2LinkConnected:
        IPOD_DLT_INFO("SCB :ID=%u, Link Connected(%d), serial=%s", deviceId, msg->state, msg->serial);
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_RUNNING;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP2;
        iPodCtrlCfg->deviceStatusChange++;
        strncpy((char *)stateEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICEDEVICESTATE, sizeof(stateEvent));
        break;

    case iAP2AuthenticationPassed:
        IPOD_DLT_INFO("SCB :ID=%u, Authentication Passed(%d), serial=%s", deviceId, msg->state, msg->serial);
        iPodCtrlCfg->authIdenPass.authPass = TRUE;
        break;

    case iAP2IdentificationPassed:
        IPOD_DLT_INFO("SCB :ID=%u, Identification Passed(%d), serial=%s", deviceId, msg->state, msg->serial);
        iPodCtrlCfg->authIdenPass.idenPass = TRUE;
        break;

    case iAP2DeviceReady:
        IPOD_DLT_INFO("SCB :ID=%u, Device Ready(%d), serial=%s", deviceId, msg->state, msg->serial);
        break;

    case iAP2LinkiAP1DeviceDetected:
        IPOD_DLT_INFO("SCB :ID=%u, iAP1 Device Detected(%d), serial=%s", deviceId, msg->state, msg->serial);
        iPodCtrlCfg->deviceConnection.deviceStatus = IPOD_PLAYER_DEVICE_CONNECT_STATUS_CONNECT;
        iPodCtrlCfg->deviceConnection.authStatus = IPOD_PLAYER_AUTHENTICATION_IAP1_RESTART;
        iPodCtrlCfg->deviceConnection.powerStatus = IPOD_PLAYER_POWER_ON;
        iPodCtrlCfg->deviceConnection.iapType = IPOD_PLAYER_PROTOCOL_TYPE_IAP1;
        iPodCtrlCfg->deviceStatusChange++;
        strncpy((char *)stateEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICEDEVICESTATE, sizeof(stateEvent));
        break;

    case iAP2ComError:
        IPOD_DLT_WARN("SCB :ID=%u, Protocol Error(%d), serial=%s", deviceId, msg->state, msg->serial);
        break;

    default:
        IPOD_DLT_ERROR("SCB :ID=%u, Undefined state(%d), serial=%s", deviceId, msg->state, msg->serial);
        break;
    }

    eventFD = iPodCtrlCfg->serviceCallbacksEventFD;

    rcm = ippiAP2ServiceCallbacksMutexUnlock(iPodCtrlCfg);
    if(rcm != IPOD_PLAYER_OK)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm);
        return IAP2_CTL_ERROR;
    }

    if(strncmp((const char *)stateEvent, IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE, sizeof(stateEvent)) != 0)
    {
        ippiAP2SendEvent(eventFD, (char *)stateEvent);
    }

    return IAP2_OK;
}

int32_t ippiAP2ServiceConnectDeviceRespCB(iAP2Service_t* service, uint32_t deviceId, iAP2ServiceConnectDeviceResp_t* msg)
{
    (void)service;
    (void)deviceId;
    (void)msg;

    IPOD_DLT_INFO("SCB :ID=%u", deviceId);

    return IAP2_OK;
}

S32 ippiAP2ServiceSetCallbacks(iAP2ServiceCallbacks_t* serviceCallbacks)
{
    S32 rc = IPOD_PLAYER_OK;

    /* Parameter check */
    if(serviceCallbacks == NULL)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERR_INVALID_PARAMETER, serviceCallbacks);
        return IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }

    /* Set callback pointers */
    serviceCallbacks->p_iAP2ServiceDeviceConnected_cb     = &ippiAP2ServiceDeviceConnectedCB;
    serviceCallbacks->p_iAP2ServiceDeviceDisconnected_cb  = &ippiAP2ServiceDeviceDisconnectedCB;
    serviceCallbacks->p_iAP2ServiceDeviceState_cb         = &ippiAP2ServiceDeviceStateCB;
    serviceCallbacks->p_iAP2ServiceConnectDeviceResp_cb   = &ippiAP2ServiceConnectDeviceRespCB;

    g_EAPrecvCount = 0;
    g_EAPrecvDataByteCount = 0;
    g_EAPsendCount = 0;
    g_EAPsendDataByteCount = 0;

    return rc;
}

S32 ippiAP2InitServiceCallbacksMutex(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;

    rcm = pthread_mutex_init(&iPodCtrlCfg->serviceCallbacksMutex, NULL);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm, errno);
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}

S32 ippiAP2DeinitServiceCallbacksMutex(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;

    rcm = pthread_mutex_destroy(&iPodCtrlCfg->serviceCallbacksMutex);
    if((rcm != 0) && (rcm != EBUSY))
    {
        IPOD_DLT_ERROR("pthread_mutex_destroy failed :%d", rcm);
        rc = IPOD_PLAYER_ERR_INVALID_PARAMETER;
    }
    else if(rcm == EBUSY)
    {
        IPOD_DLT_WARN("pthread_mutex_destroy busy :%d", rcm);
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}

/* PRQA: Lint Message 454: Mutex unlock is done by other function. */
S32 ippiAP2ServiceCallbacksMutexLock(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;

    rcm = pthread_mutex_lock(&iPodCtrlCfg->serviceCallbacksMutex);
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm, errno);
        rc = IPOD_PLAYER_ERROR;
    }

/*lint -save -e454 */
     return rc;
}
/*lint -restore */

/* PRQA: Lint Message 455: Mutex lock is done by other function. */ 
S32 ippiAP2ServiceCallbacksMutexUnlock(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg)
{
    S32 rc = IPOD_PLAYER_OK;
    S32 rcm = 0;

    rcm = pthread_mutex_unlock(&iPodCtrlCfg->serviceCallbacksMutex);  /*lint !e455 */
    if(rcm != 0)
    {
        IPOD_LOG_ERR_WRITE32(IPOD_LOG_TYPE_ERROR, IPOD_LOG_PLAYER_CORE, IPOD_PLAYER_ERROR, rcm, errno);
        rc = IPOD_PLAYER_ERROR;
    }

    return rc;
}
