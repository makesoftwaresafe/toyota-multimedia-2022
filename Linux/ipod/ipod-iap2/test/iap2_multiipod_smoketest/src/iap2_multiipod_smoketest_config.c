/* **********************  includes  ********************** */


#include "iap2_multiipod_smoketest.h"
#include "iap2_test_init.h"
#include <iap2_parameter_free.h>
#include "iap2_dlt_log.h"
/* **********************  defines  ********************** */



/* **********************  variables  ********************** */

U16 USBDeviceModeMsgSentByAcc[] =        {                  \
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
    IAP2_MSG_ID_POWER_SOURCE_UPDATE,                        \
};

U16 USBDeviceModeMsgRecvFromDevice[] =            {         \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                  \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                       \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,               \
    IAP2_MSG_ID_USB_DEVICE_MODE_AUDIO_INFORMATION,          \
    IAP2_MSG_ID_POWER_UPDATE,                               \
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
    IAP2_MSG_ID_POWER_SOURCE_UPDATE,                        \
};

U16 USBHostModeMsgRecvFromDevice[] = {                      \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                  \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                       \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,               \
    IAP2_MSG_ID_POWER_UPDATE,                               \
};

U16 BTMsgSentByAcc[] = {                                    \
    IAP2_MSG_ID_START_MEDIA_LIBRARY_INFORMATION,            \
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_INFORMATION,             \
    IAP2_MSG_ID_START_MEDIA_LIBRARY_UPDATES,                \
    IAP2_MSG_ID_STOP_MEDIA_LIBRARY_UPDATES,                 \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_CURRENT_SELECTION,       \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_ITEMS,                   \
    IAP2_MSG_ID_PLAY_MEDIA_LIBRARY_COLLECTION,              \
    IAP2_MSG_ID_START_NOW_PLAYING_UPDATES,                  \
    IAP2_MSG_ID_STOP_NOW_PLAYING_UPDATES,                   \
    IAP2_MSG_ID_START_HID,                                  \
    IAP2_MSG_ID_STOP_HID,                                   \
    IAP2_MSG_ID_ACCESSORY_HID_REPORT                        \
};

U16 BTMsgRecvFromDevice[] = {                                       \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                          \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                               \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,                       \
    IAP2_MSG_ID_DEVICE_HID_REPORT                                   \
};

LOCAL S32 iap2SetAccInfo(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iap2UserConfig);

/*Set Accessory Identification Values*/
LOCAL S32 iap2SetAccInfo(iAP2InitParam_t* iAP2InitParam,iap2UserConfig_t iap2UserConfig)
{    S32 rc = IAP2_OK;
iAP2AccessoryInfo_t* accInfo = iAP2InitParam->p_iAP2AccessoryInfo;

U16 i;
/* currently, we support only one iOS App communication via EA native transport */
U32 SupportediOSAppCount = iap2UserConfig.SupportediOSAppCount;
U8  iOSAppName[2][STRING_MAX] = {IAP2_IOS_APP_NAME_DEMO, IAP2_IOS_APP_NAME_DEMO_2};

U8  iOSAppIdentifier = 1;
U16 HIDComponentCount = 1;
U8  HIDCompName[] = {"Media Playback Remote"};
U16 iap2HIDComponentIdentifier = IAP2_HID_COMP_IDENTIFIER;

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
       else if(iap2UserConfig.iAP2TransportType == iAP2BLUETOOTH)
       {
           printf("\n Commands - Bluetooth");
           accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(BTMsgSentByAcc) );
           if(accInfo->iAP2CommandsUsedByApplication != NULL)
           {
               memcpy(accInfo->iAP2CommandsUsedByApplication, BTMsgSentByAcc, sizeof(BTMsgSentByAcc) );
               accInfo->iAP2CommandsUsedByApplication_length = sizeof(BTMsgSentByAcc);
           }
           else
           {
               rc = IAP2_ERR_NO_MEM;
           }

           if(rc == IAP2_OK)
           {
               accInfo->iAP2CallbacksExpectedFromDevice = calloc(1, sizeof(BTMsgRecvFromDevice) );
               if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
               {
                   memcpy(accInfo->iAP2CallbacksExpectedFromDevice, BTMsgRecvFromDevice, sizeof(BTMsgRecvFromDevice) );
                   accInfo->iAP2CallbacksExpectedFromDevice_length = sizeof(BTMsgRecvFromDevice);
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

   if(rc == IAP2_OK)
   {
       accInfo->iAP2USBHIDComponent_count = HIDComponentCount;
       accInfo->iAP2USBHIDComponent = (iAP2iAP2HIDComponent*)calloc(accInfo->iAP2USBHIDComponent_count, sizeof(iAP2iAP2HIDComponent));
       if(accInfo->iAP2USBHIDComponent != NULL)
       {
           for(i=0; ((i<accInfo->iAP2USBHIDComponent_count) && (rc == IAP2_OK)); i++)
           {
               accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction = (iAP2HIDComponentFunction*)calloc(1, sizeof(iAP2HIDComponentFunction));
               if(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction == NULL)
               {
                   rc = IAP2_ERR_NO_MEM;
               }
               if(rc == IAP2_OK)
               {
                   *(accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction) = IAP2_HID_COMPONENT_PLAYBACK_REMOTE;
                   accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentFunction_count++;
                   rc = iap2AllocateandUpdateData(&accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier,
                                                  &iap2HIDComponentIdentifier,
                                                  &accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentIdentifier_count,
                                                  1, iAP2_uint16);
               }
               if(rc == IAP2_OK)
               {
                   U8* p_HIDCompName = HIDCompName;

                   rc = iap2AllocateandUpdateData(&accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName,
                                                  &p_HIDCompName,
                                                  &accInfo->iAP2USBHIDComponent[i].iAP2HIDComponentName_count,
                                                  1, iAP2_utf8);
               }
           }
       }
       else
       {
           rc = IAP2_ERR_NO_MEM;
       }
   }


#if IAP2_ENABLE_BLUETOOTH_COMPONENT_TEST
   if(rc == IAP2_OK)
   {
       U16 BT_TransCompIdentifier = IAP2_BT_TRANS_COMP_ID;
       U8  BT_TransCompName[2][256] = {IAP2_BT_MY_CAR, IAP2_BT_TRANS_COMP_NAME};

       /* more than one iAP2BluetoothTransportComponent possible */
       accInfo->iAP2BluetoothTransportComponent = (iAP2BluetoothTransportComponent*)calloc(IAP2_BT_MAC_ADDRESS_CNT, sizeof(iAP2BluetoothTransportComponent) );
       if(accInfo->iAP2BluetoothTransportComponent != NULL)
       {
           accInfo->iAP2BluetoothTransportComponent_count = 1;
           for(i=0; i<IAP2_BT_MAC_ADDRESS_CNT; i++)
           {
               rc = iap2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                              &iap2UserConfig.BTAdapterMAC,
                                              &accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress_count,
                                              IAP2_BT_MAC_LENGTH, iAP2_blob);
               if(rc == IAP2_OK)
               {
                   /* each iAP2BluetoothTransportComponent must have an unique identifier */
                   BT_TransCompIdentifier += i;
                   rc = iap2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier,
                                                  &BT_TransCompIdentifier,
                                                  &accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentIdentifier_count,
                                                  1, iAP2_uint16);
               }
               if(rc == IAP2_OK)
               {
                   U8* BTTransCompName = BT_TransCompName[i];
                   /* provide the name of the Bluetooth transport (e.g. My Car) */
                   rc = iap2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName,
                                                  &BTTransCompName,
                                                  &accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportComponentName_count,
                                                  1, iAP2_utf8);
               }
               if((rc == IAP2_OK) && (iap2UserConfig.iAP2TransportType == iAP2BLUETOOTH))
               {
                   /* Set if Bluetooth component supports iAP2 connection */
                   accInfo->iAP2BluetoothTransportComponent[i].iAP2TransportSupportsiAP2Connection_count++;
               }
           }
       }
       else
       {
           rc = IAP2_ERR_NO_MEM;
       }
   }
#endif

#if IAP2_ENABLE_VEHICLE_INFORMATION_COMPONENT_TEST
   if(rc == IAP2_OK)
   {
       accInfo->iAP2VehicleInformationComponent = (iAP2VehicleInformationComponent_t*)calloc(1, sizeof(iAP2VehicleInformationComponent_t) );
       if(accInfo->iAP2VehicleInformationComponent == NULL)
       {
           rc = IAP2_ERR_NO_MEM;
       }
       else
       {
           U8  VehicleDisplayName[]                 = {"DisplayName"};
           accInfo->iAP2VehicleInformationComponent->iAP2DisplayName = (U8*)strndup( (const char*)VehicleDisplayName, strnlen((const char*)VehicleDisplayName, STRING_MAX) );
           if(accInfo->iAP2VehicleInformationComponent->iAP2DisplayName == NULL)
           {
               rc = IAP2_ERR_NO_MEM;
           }
       }
       if(rc == IAP2_OK)
       {
           /* Defining multiple engine types */
           accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count = 2;
           accInfo->iAP2VehicleInformationComponent->iAP2EngineType = (iAP2EngineTypes*)calloc(accInfo->iAP2VehicleInformationComponent->iAP2EngineType_count, sizeof(iAP2EngineTypes) );
           if(accInfo->iAP2VehicleInformationComponent->iAP2EngineType == NULL)
           {
               rc = IAP2_ERR_NO_MEM;
           }
       }
       if(rc == IAP2_OK)
       {
           accInfo->iAP2VehicleInformationComponent->iAP2EngineType[0] = IAP2_DIESEL;
           accInfo->iAP2VehicleInformationComponent->iAP2EngineType[1] = IAP2_CNG;
       }
   }
#endif

#if IAP2_ENABLE_VEHICLE_STATUS_COMPONENT_TEST
   if(rc == IAP2_OK)
   {
       accInfo->iAP2VehicleStatusComponent = (iAP2VehicleStatusComponent_t*)calloc(1, sizeof(iAP2VehicleStatusComponent_t) );
       if(accInfo->iAP2VehicleStatusComponent == NULL)
       {
           rc = IAP2_ERR_NO_MEM;
       }
       else
       {
           accInfo->iAP2VehicleStatusComponent->iAP2InsideTemperature  = TRUE;
           accInfo->iAP2VehicleStatusComponent->iAP2NightMode          = FALSE;
           accInfo->iAP2VehicleStatusComponent->iAP2OutsideTemperature = TRUE;
           accInfo->iAP2VehicleStatusComponent->iAP2RangeWarning       = TRUE;
           accInfo->iAP2VehicleStatusComponent->iAP2Range              = TRUE;
       }
   }
#endif

#if IAP2_ENABLE_LOCATION_INFORMATION_COMPONENT_TEST
   if(rc == IAP2_OK)
   {
       accInfo->iAP2LocationInformationComponent = (iAP2LocationInformationComponent_t*)calloc(1, sizeof(iAP2LocationInformationComponent_t) );
       if(accInfo->iAP2LocationInformationComponent == NULL)
       {
           rc = IAP2_ERR_NO_MEM;
       }
       else
       {
           accInfo->iAP2LocationInformationComponent->iAP2GlobalPositioningSystemFixData           = TRUE;
           accInfo->iAP2LocationInformationComponent->iAP2RecommendedMinimumSpecificGPSTransitData = TRUE;
           accInfo->iAP2LocationInformationComponent->iAP2VehicleAccelerometerData                 = TRUE;
           accInfo->iAP2LocationInformationComponent->iAP2VehicleGyroData                          = TRUE;
           accInfo->iAP2LocationInformationComponent->iAP2VehicleSpeedData                         = TRUE;
       }
   }
#endif

   return rc;
}

S32 iap2SetInitialParameter(iAP2InitParam_t* iAP2InitParam, iap2UserConfig_t iap2UserConfig)
{

    S32 rc = IAP2_OK ;

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
                iap2InitCSCallbacks(iAP2InitParam->p_iAP2CSCallbacks, iAP2InitParam->p_iAP2AccessoryConfig->iAP2EAPSupported);
            }
            /* File transfer session callbacks */
            if(NULL != iAP2InitParam->p_iAP2FileTransferCallbacks)
            {
                iap2InitFileTransferCallbacks(iAP2InitParam->p_iAP2FileTransferCallbacks);
            }
            /* External Accessory Protocol session callbacks */
            if(NULL != iAP2InitParam->p_iAP2EAPSessionCallbacks)
            {
                iap2InitEAPSessionCallbacks(iAP2InitParam->p_iAP2EAPSessionCallbacks);
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
        printf("iAP2InitParam is NULL");
    }

    return rc;
}
