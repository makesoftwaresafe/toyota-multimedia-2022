/* **********************  includes  ********************** */

#include "iap2_stresstest.h"
#include <iap2_parameter_free.h>
#include "iap2_dlt_log.h"

/* **********************  defines  ********************** */



/* **********************  variables  ********************** */

U16 USBDeviceModeMsgSentByAcc[] = {                         \
    IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION,            \
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_INFORMATION,             \
    IAP2_MSG_ID_START_MEDIA_LIBRARY_UPDATES,                \
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_UPDATES,                 \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_CURRENT_SELECTION,       \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_ITEMS,                   \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_COLLECTION,              \
    IAP2_MSG_ID_START_NOW_PLAYING_UPDATES,                  \
    IAP2_MSG_ID_STOP_NOW_PLAYING_UPDATES,                   \
    IAP2_MSG_ID_START_USB_DEVICE_MODE_AUDIO,                \
    IAP2_MSG_ID_STOP_USB_DEVICE_MODE_AUDIO,                 \
    IAP2_MSG_ID_START_POWER_UPDATES,                        \
    IAP2_MSG_ID_STOP_POWER_UPDATES,                         \
    IAP2_MSG_ID_POWER_SOURCE_UPDATE
};

U16 USBDeviceModeMsgRecvFromDevice[] = {                    \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                  \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                       \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,               \
    IAP2_MSG_ID_USB_DEVICE_MODE_AUDIO_INFORMATION,          \
    IAP2_MSG_ID_POWER_UPDATE
};

U16 USBHostModeMsgSentByAcc[] = {                           \
    IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION,            \
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_INFORMATION,             \
    IAP2_MSG_ID_START_MEDIA_LIBRARY_UPDATES,                \
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_UPDATES,                 \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_CURRENT_SELECTION,       \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_ITEMS,                   \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_COLLECTION,              \
    IAP2_MSG_ID_START_NOW_PLAYING_UPDATES,                  \
    IAP2_MSG_ID_STOP_NOW_PLAYING_UPDATES,                   \
    IAP2_MSG_ID_START_POWER_UPDATES,                        \
    IAP2_MSG_ID_STOP_POWER_UPDATES,                         \
    IAP2_MSG_ID_POWER_SOURCE_UPDATE
};

U16 USBHostModeMsgRecvFromDevice[] = {                      \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                  \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                       \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,               \
    IAP2_MSG_ID_POWER_UPDATE
};
/* **********************  locals  ********************** */


LOCAL S32 iap2SetAccInfo(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iap2UserConfig);
/* **********************  functions  ********************** */


/*Set Accessory Identification Values*/
LOCAL S32 iap2SetAccInfo(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iap2UserConfig)
{
    S32 rc = IAP2_OK;
    iAP2AccessoryInfo_t* accInfo = iAP2InitParam->p_iAP2AccessoryInfo;

    U16 i;
    /* currently, we support only one iOS App communication via EA native transport */
    U32 SupportediOSAppCount = iap2UserConfig.SupportediOSAppCount;
    U8  iOSAppName[2][STRING_MAX] = {IAP2_IOS_APP_NAME_DEMO, IAP2_IOS_APP_NAME_DEMO_2};
    U8  iOSAppIdentifier = 1;

    rc = iap2InitAccInfo(accInfo, iap2UserConfig);

    if(rc == IAP2_OK)
    {
        if(iap2UserConfig.iAP2TransportType == iAP2USBHOSTMODE)
        {
            accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(USBHostModeMsgSentByAcc) );
            if(accInfo->iAP2CommandsUsedByApplication != NULL)
            {
                memcpy(accInfo->iAP2CommandsUsedByApplication, USBHostModeMsgSentByAcc, sizeof(USBHostModeMsgSentByAcc) );
                accInfo->iAP2CommandsUsedByApplication_length = sizeof(USBHostModeMsgSentByAcc);
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }

            if(rc == IAP2_OK)
            {
                accInfo->iAP2CallbacksExpectedFromDevice = calloc(1, sizeof(USBHostModeMsgRecvFromDevice) );
                if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
                {
                    memcpy(accInfo->iAP2CallbacksExpectedFromDevice, USBHostModeMsgRecvFromDevice, sizeof(USBHostModeMsgRecvFromDevice) );
                    accInfo->iAP2CallbacksExpectedFromDevice_length = sizeof(USBHostModeMsgRecvFromDevice);
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
        }
        else
         {
            accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(USBDeviceModeMsgSentByAcc) );
            if(accInfo->iAP2CommandsUsedByApplication != NULL)
            {
                memcpy(accInfo->iAP2CommandsUsedByApplication, USBDeviceModeMsgSentByAcc, sizeof(USBDeviceModeMsgSentByAcc) );
                accInfo->iAP2CommandsUsedByApplication_length = sizeof(USBDeviceModeMsgSentByAcc);
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }

            if(rc == IAP2_OK)
            {
                accInfo->iAP2CallbacksExpectedFromDevice = calloc(1, sizeof(USBDeviceModeMsgRecvFromDevice) );
                if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
                {
                    memcpy(accInfo->iAP2CallbacksExpectedFromDevice, USBDeviceModeMsgRecvFromDevice, sizeof(USBDeviceModeMsgRecvFromDevice) );
                    accInfo->iAP2CallbacksExpectedFromDevice_length = sizeof(USBDeviceModeMsgRecvFromDevice);
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
        }
    }

    if( (rc == IAP2_OK) && (iap2GetiOS8testing() == TRUE) )
    {
        U16 MessageReceivedFromDevice_iOS8[] = { IAP2_MSG_ID_DEVICE_TIME_UPDATE };

        accInfo->iAP2CallbacksExpectedFromDevice = realloc(accInfo->iAP2CallbacksExpectedFromDevice, accInfo->iAP2CallbacksExpectedFromDevice_length + sizeof(MessageReceivedFromDevice_iOS8) );
        if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
        {
            memcpy(&accInfo->iAP2CallbacksExpectedFromDevice[accInfo->iAP2CallbacksExpectedFromDevice_length / sizeof(U16) ], MessageReceivedFromDevice_iOS8, sizeof(MessageReceivedFromDevice_iOS8) );
            accInfo->iAP2CallbacksExpectedFromDevice_length += sizeof(MessageReceivedFromDevice_iOS8);
        }
        else
        {
            rc = IAP2_ERR_NO_MEM;
        }
        if(rc == IAP2_OK)
        {
            U16 MessageSentByAcc_iOS8[] = { IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_SPECIAL,  \
                                            IAP2_MSG_ID_SET_NOW_PLAYING_INFORMATION  };

            accInfo->iAP2CommandsUsedByApplication = realloc(accInfo->iAP2CommandsUsedByApplication, accInfo->iAP2CommandsUsedByApplication_length + sizeof(MessageSentByAcc_iOS8) );
           if(accInfo->iAP2CommandsUsedByApplication != NULL)
            {
                memcpy(&accInfo->iAP2CommandsUsedByApplication[accInfo->iAP2CommandsUsedByApplication_length / sizeof(U16) ], MessageSentByAcc_iOS8, sizeof(MessageSentByAcc_iOS8) );
                accInfo->iAP2CommandsUsedByApplication_length += sizeof(MessageSentByAcc_iOS8);
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
        }
    }
    if(rc == IAP2_OK)
    {
        accInfo->iAP2MaximumCurrentDrawnFromDevice = 0;

        accInfo->iAP2SupportsiOSintheCar = iAP2InitParam->p_iAP2AccessoryConfig->iAP2iOSintheCar;

        if ((TRUE == iap2GetEAtesting()) && (TRUE == iAP2InitParam->p_iAP2AccessoryConfig->iAP2EAPSupported))
        {
            printf("Enable EAP Support\n");

            U16 MessageReceivedFromDevice_EAP[] = { IAP2_MSG_ID_START_EXTERNAL_ACCESSORY_PROTOCOL_SESSION,  \
                                                    IAP2_MSG_ID_STOP_EXTERNAL_ACCESSORY_PROTOCOL_SESSION  };

            accInfo->iAP2CallbacksExpectedFromDevice = realloc(accInfo->iAP2CallbacksExpectedFromDevice, accInfo->iAP2CallbacksExpectedFromDevice_length + sizeof(MessageReceivedFromDevice_EAP) );
            if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
            {
                memcpy(&accInfo->iAP2CallbacksExpectedFromDevice[accInfo->iAP2CallbacksExpectedFromDevice_length / sizeof(U16) ], MessageReceivedFromDevice_EAP, sizeof(MessageReceivedFromDevice_EAP) );
                accInfo->iAP2CallbacksExpectedFromDevice_length += sizeof(MessageReceivedFromDevice_EAP);
            }
            else
            {
                rc = IAP2_ERR_NO_MEM;
            }
            if(rc == IAP2_OK)
            {
                U16 MessageSentByAcc_EAP[] = { IAP2_MSG_ID_STATUS_EXTERNAL_ACCESSORY_PROTOCOL_SESSION };

                accInfo->iAP2CommandsUsedByApplication = realloc(accInfo->iAP2CommandsUsedByApplication, accInfo->iAP2CommandsUsedByApplication_length + sizeof(MessageSentByAcc_EAP) );
                if(accInfo->iAP2CommandsUsedByApplication != NULL)
                {
                    memcpy(&accInfo->iAP2CommandsUsedByApplication[accInfo->iAP2CommandsUsedByApplication_length / sizeof(U16) ], MessageSentByAcc_EAP, sizeof(MessageSentByAcc_EAP) );
                    accInfo->iAP2CommandsUsedByApplication_length += sizeof(MessageSentByAcc_EAP);
                }
                else
                {
                    rc = IAP2_ERR_NO_MEM;
                }
            }
        }
        if( (TRUE == iap2GetEAtesting())
            && ((TRUE == iAP2InitParam->p_iAP2AccessoryConfig->iAP2EAPSupported) || (TRUE == iap2UserConfig.iap2EANativeTransport)) )
        {
            accInfo->iAP2SupportediOSAppCount = SupportediOSAppCount;
            if(accInfo->iAP2SupportediOSAppCount > 0)
            {
                accInfo->iAP2iOSAppInfo = calloc(accInfo->iAP2SupportediOSAppCount, sizeof(iAP2iOSAppInfo_t) );
                if(accInfo->iAP2iOSAppInfo != NULL)
                {
                    for(i=0;( (i<accInfo->iAP2SupportediOSAppCount) && (rc == IAP2_OK) );i++)
                    {
                        accInfo->iAP2iOSAppInfo[i].iAP2iOSAppIdentifier = iOSAppIdentifier+i;
                        accInfo->iAP2iOSAppInfo[i].iAP2EAPMatchAction   = IAP2_NO_APP_MATCH;
                        accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName = (U8*)strndup( (const char*)&iOSAppName[i][0], strnlen((const char*)&iOSAppName[i][0], STRING_MAX) );
                        if(accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName == NULL)
                        {
                            rc = IAP2_ERR_NO_MEM;
                            IAP2TESTDLTLOG(DLT_LOG_ERROR, "iAP2iOSAppInfo[%d].iAP2iOSAppName is NULL",i);
                        }

                        /* currently, we support only one iOS App communication via EA native transport */
                        if((i == 0) && (iap2UserConfig.iap2EANativeTransport == TRUE))
                        {
                            /* the first specified iOS App communicates via EA native transport */
                            accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport = TRUE;
                        }
                        else if((i > 0) && (iap2UserConfig.iap2EAPSupported != TRUE))
                        {
                            accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport = TRUE;
                        }
                        else
                        {
                            accInfo->iAP2iOSAppInfo[i].iAP2EANativeTransport = FALSE;
                        }
                   }
                } else{
                    rc = IAP2_ERR_NO_MEM;
                }
            } else{
                printf("No iOS App configured.\n");
            }
        }
    }

    return rc;
}

S32 iap2SetInitialParameter(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iap2UserConfig)
{

    S32 rc = IAP2_OK ;

    /* Initializing the iap2HdlComThreadPollMqEvent_CB function pointer */
    iap2HdlComThreadPollMqEvent = &iap2HdlComThreadPollMqEvent_CB;

    if(NULL != iAP2InitParam)
    {
        rc = iap2ConfigAcc(iAP2InitParam,iap2UserConfig);

        if(rc == IAP2_OK && NULL != iAP2InitParam->p_iAP2AccessoryInfo)
        {
            rc = iap2SetAccInfo(iAP2InitParam, iap2UserConfig);
        }
        if (rc == IAP2_OK)
        {
            /* TBD: set useful values */
            if(NULL != iAP2InitParam->p_iAP2CSCallbacks)
            {
                iap2InitCSCallbacks(iAP2InitParam->p_iAP2CSCallbacks);
            }
            /* File transfer session callbacks */
            if(NULL != iAP2InitParam->p_iAP2FileTransferCallbacks)
            {
                iap2InitFileTransferCallbacks(iAP2InitParam->p_iAP2FileTransferCallbacks);
            }
            /* init ipa2stack callback */
            if(NULL != iAP2InitParam->p_iAP2StackCallbacks)
            {
                iap2InitStackCallbacks(iAP2InitParam->p_iAP2StackCallbacks);
            }
        }
        else
          {
            printf("Failed to set Accessory Info");
          }
    }
    else
    {
        rc = IAP2_BAD_PARAMETER ;
    }

    return rc;
}
