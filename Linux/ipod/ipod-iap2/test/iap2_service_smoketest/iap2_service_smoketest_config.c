/*
 * iap2_smoketest_config.c
 *
 *  Created on: Feb 20, 2014
 *      Author: dgirnus
 */

/* **********************  includes  ********************** */

#include "iap2_smoketest.h"
#include <iap2_parameter_free.h>
#include "iap2_dlt_log.h"

/* **********************  defines  ********************** */



/* **********************  variables  ********************** */

U16 USBMultiHostModeMsgSentByAcc[] = {                         \
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
    IAP2_MSG_ID_START_HID,                                  \
    IAP2_MSG_ID_STOP_HID,                                   \
    IAP2_MSG_ID_ACCESSORY_HID_REPORT,                       \
    IAP2_MSG_ID_VEHICLE_STATUS_UPDATE,                      \
    IAP2_MSG_ID_LOCATION_INFORMATION,                       \
    IAP2_MSG_ID_REQUEST_APP_LAUNCH,                         \
    IAP2_MSG_ID_START_COMMUNICATIONS_UPDATES,               \
    IAP2_MSG_ID_STOP_COMMUNICATIONS_UPDATES,                \
    IAP2_MSG_ID_START_CALL_STATE_UPDATES,                   \
    IAP2_MSG_ID_STOP_CALL_STATE_UPDATES,                    \
    IAP2_MSG_ID_START_LIST_UPDATES,                         \
    IAP2_MSG_ID_STOP_LIST_UPDATES,                          \
    IAP2_MSG_ID_INITIATE_CALL,                              \
    IAP2_MSG_ID_ACCEPT_CALL,                                \
    IAP2_MSG_ID_MERGE_CALLS,                                \
    IAP2_MSG_ID_SWAP_CALLS,                                 \
    IAP2_MSG_ID_END_CALL,                                   \
    IAP2_MSG_ID_SEND_DTMF,                                  \
    IAP2_MSG_ID_HOLD_STATUS_UPDATE,                         \
    IAP2_MSG_ID_MUTE_STATUS_UPDATE,                         \
    IAP2_MSG_ID_ACCESSORY_WI_FI_CONFIGURATION_INFORMATION,  \
    IAP2_MSG_ID_START_ROUTE_GUIDANCE_UPDATE,                \
    IAP2_MSG_ID_STOP_ROUTE_GUIDANCE_UPDATE
};

U16 USBMultiHostModeMsgRecvFromDevice[] = {                            \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                  \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                       \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,               \
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,                  \
    IAP2_MSG_ID_POWER_UPDATE,                               \
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,               \
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,                \
    IAP2_MSG_ID_START_LOCATION_INFORMATION,                 \
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,                  \
    IAP2_MSG_ID_DEVICE_HID_REPORT,                          \
    IAP2_MSG_ID_COMMUNICATIONS_UPDATE,                              \
    IAP2_MSG_ID_CALL_STATE_UPDATE,                                  \
    IAP2_MSG_ID_LIST_UPDATE,                                        \
    IAP2_MSG_ID_REQUEST_ACCESSORY_WI_FI_CONFIGURATION_INFORMATION,   \
    IAP2_MSG_ID_ROUTE_GUIDANCE_UPDATE,                      \
    IAP2_MSG_ID_ROUTE_GUIDANCE_MANEUVER_UPDATE
};

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
    IAP2_MSG_ID_POWER_SOURCE_UPDATE,                        \
    IAP2_MSG_ID_START_HID,                                  \
    IAP2_MSG_ID_STOP_HID,                                   \
    IAP2_MSG_ID_ACCESSORY_HID_REPORT,                       \
    IAP2_MSG_ID_VEHICLE_STATUS_UPDATE,                      \
    IAP2_MSG_ID_LOCATION_INFORMATION,                       \
    IAP2_MSG_ID_REQUEST_APP_LAUNCH,                         \
    IAP2_MSG_ID_START_COMMUNICATIONS_UPDATES,               \
    IAP2_MSG_ID_STOP_COMMUNICATIONS_UPDATES,                \
    IAP2_MSG_ID_START_CALL_STATE_UPDATES,                   \
    IAP2_MSG_ID_STOP_CALL_STATE_UPDATES,                    \
    IAP2_MSG_ID_START_LIST_UPDATES,                         \
    IAP2_MSG_ID_STOP_LIST_UPDATES,                          \
    IAP2_MSG_ID_INITIATE_CALL,                              \
    IAP2_MSG_ID_ACCEPT_CALL,                                \
    IAP2_MSG_ID_MERGE_CALLS,                                \
    IAP2_MSG_ID_SWAP_CALLS,                                 \
    IAP2_MSG_ID_END_CALL,                                   \
    IAP2_MSG_ID_SEND_DTMF,                                  \
    IAP2_MSG_ID_HOLD_STATUS_UPDATE,                         \
    IAP2_MSG_ID_MUTE_STATUS_UPDATE,                         \
    IAP2_MSG_ID_ACCESSORY_WI_FI_CONFIGURATION_INFORMATION,  \
    IAP2_MSG_ID_START_ROUTE_GUIDANCE_UPDATE,                \
    IAP2_MSG_ID_STOP_ROUTE_GUIDANCE_UPDATE
};

U16 USBDeviceModeMsgRecvFromDevice[] = {                            \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                          \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                               \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,                       \
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,                          \
    IAP2_MSG_ID_USB_DEVICE_MODE_AUDIO_INFORMATION,                  \
    IAP2_MSG_ID_POWER_UPDATE,                                       \
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,                       \
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,                        \
    IAP2_MSG_ID_START_LOCATION_INFORMATION,                         \
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,                          \
    IAP2_MSG_ID_DEVICE_HID_REPORT,                                  \
    IAP2_MSG_ID_COMMUNICATIONS_UPDATE,                              \
    IAP2_MSG_ID_CALL_STATE_UPDATE,                                  \
    IAP2_MSG_ID_LIST_UPDATE,                                        \
    IAP2_MSG_ID_WIRELESS_CAR_PLAY_UPDATE,                           \
    IAP2_MSG_ID_DEVICE_UUID_UPDATE,                                 \
    IAP2_MSG_ID_GPRMC_DATA_STATUS_VALUES_NOTIFICATION,              \
    IAP2_MSG_ID_REQUEST_ACCESSORY_WI_FI_CONFIGURATION_INFORMATION,  \
    IAP2_MSG_ID_ROUTE_GUIDANCE_UPDATE,                              \
    IAP2_MSG_ID_ROUTE_GUIDANCE_MANEUVER_UPDATE
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
    IAP2_MSG_ID_VEHICLE_STATUS_UPDATE,                      \
    IAP2_MSG_ID_LOCATION_INFORMATION,                       \
    IAP2_MSG_ID_START_HID,                                  \
    IAP2_MSG_ID_STOP_HID,                                   \
    IAP2_MSG_ID_ACCESSORY_HID_REPORT,                       \
    IAP2_MSG_ID_REQUEST_APP_LAUNCH,                         \
    IAP2_MSG_ID_START_ROUTE_GUIDANCE_UPDATE,                \
    IAP2_MSG_ID_STOP_ROUTE_GUIDANCE_UPDATE,                 \
    IAP2_MSG_ID_ACCESSORY_AUTHENTICATION_SERIAL_NUMBER    /* Added to avoid the ATS error
                                                             "Accessory did not identify the
                                                             AccessoryAuthenticationSerialNumber message"
                                                             Error seen after ATS_v5.16 */
};

U16 USBHostModeMsgRecvFromDevice[] = {                      \
    IAP2_MSG_ID_MEDIA_LIBRARY_INFORMATION,                  \
    IAP2_MSG_ID_MEDIA_LIBRARY_UPDATE,                       \
    IAP2_MSG_ID_NOW_PLAYING_UPDATE_PARAMETER,               \
    IAP2_MSG_ID_DEVICE_INFORMATION_UPDATE,                  \
    IAP2_MSG_ID_POWER_UPDATE,                               \
    IAP2_MSG_ID_START_VEHICLE_STATUS_UPDATES,               \
    IAP2_MSG_ID_STOP_VEHICLE_STATUS_UPDATES,                \
    IAP2_MSG_ID_START_LOCATION_INFORMATION,                 \
    IAP2_MSG_ID_STOP_LOCATION_INFORMATION,                  \
    IAP2_MSG_ID_DEVICE_HID_REPORT,                          \
    IAP2_MSG_ID_ROUTE_GUIDANCE_UPDATE,                      \
    IAP2_MSG_ID_ROUTE_GUIDANCE_MANEUVER_UPDATE
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
           else if(iap2UserConfig.iAP2TransportType == iAP2MULTIHOSTMODE)
           {
               accInfo->iAP2CommandsUsedByApplication = calloc(1, sizeof(USBMultiHostModeMsgSentByAcc) );
               if(accInfo->iAP2CommandsUsedByApplication != NULL)
               {
                   memcpy(accInfo->iAP2CommandsUsedByApplication, USBMultiHostModeMsgSentByAcc, sizeof(USBMultiHostModeMsgSentByAcc) );
                   accInfo->iAP2CommandsUsedByApplication_length = sizeof(USBMultiHostModeMsgSentByAcc);
               }
               else
               {
                   rc = IAP2_ERR_NO_MEM;
               }

               if(rc == IAP2_OK)
               {
                   accInfo->iAP2CallbacksExpectedFromDevice = calloc(1, sizeof(USBMultiHostModeMsgRecvFromDevice) );
                   if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
                   {
                       memcpy(accInfo->iAP2CallbacksExpectedFromDevice, USBMultiHostModeMsgRecvFromDevice, sizeof(USBMultiHostModeMsgRecvFromDevice) );
                       accInfo->iAP2CallbacksExpectedFromDevice_length = sizeof(USBMultiHostModeMsgRecvFromDevice);
                   }
                   else
                   {
                       rc = IAP2_ERR_NO_MEM;
                   }
               }
               printf("\n Multihost mode identification information");
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

       if( (rc == IAP2_OK) && (iap2UserConfig.iap2iOSintheCar == TRUE) )
       {
           U16 MessageReceivedFromDevice_iOSintheCar[] = { IAP2_MSG_ID_START_OOBBT_PAIRING,               \
                                                           IAP2_MSG_ID_STOP_OOBBT_PAIRING,                \
                                                           IAP2_MSG_ID_OOBBT_PAIRING_LINK_KEY_INFORMATION   };

           accInfo->iAP2CallbacksExpectedFromDevice = realloc(accInfo->iAP2CallbacksExpectedFromDevice, accInfo->iAP2CallbacksExpectedFromDevice_length + sizeof(MessageReceivedFromDevice_iOSintheCar) );
           if(accInfo->iAP2CallbacksExpectedFromDevice != NULL)
           {
               memcpy(&accInfo->iAP2CallbacksExpectedFromDevice[accInfo->iAP2CallbacksExpectedFromDevice_length / sizeof(U16) ], MessageReceivedFromDevice_iOSintheCar, sizeof(MessageReceivedFromDevice_iOSintheCar) );
               accInfo->iAP2CallbacksExpectedFromDevice_length += sizeof(MessageReceivedFromDevice_iOSintheCar);
           }
           else
           {
               rc = IAP2_ERR_NO_MEM;
           }
           if(rc == IAP2_OK)
           {
               U16 MessageSentByAcc_iOSintheCar[] = { IAP2_MSG_ID_OOBBT_PAIRING_ACCESSORY_INFORMATION, \
                                                      IAP2_MSG_ID_OOBBT_PAIRING_COMPLETION_INFORMATION   };

               accInfo->iAP2CommandsUsedByApplication = realloc(accInfo->iAP2CommandsUsedByApplication, accInfo->iAP2CommandsUsedByApplication_length + sizeof(MessageSentByAcc_iOSintheCar) );
               if(accInfo->iAP2CommandsUsedByApplication != NULL)
               {
                   memcpy(&accInfo->iAP2CommandsUsedByApplication[accInfo->iAP2CommandsUsedByApplication_length / sizeof(U16) ], MessageSentByAcc_iOSintheCar, sizeof(MessageSentByAcc_iOSintheCar) );
                   accInfo->iAP2CommandsUsedByApplication_length += sizeof(MessageSentByAcc_iOSintheCar);
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
                           accInfo->iAP2iOSAppInfo[i].iAP2ExternalAccessoryProtocolCarPlay = FALSE;
                           if(accInfo->iAP2iOSAppInfo[i].iAP2iOSAppName == NULL)
                           {
                                rc = IAP2_ERR_NO_MEM;
                                IAP2TESTDLTLOG(DLT_LOG_ERROR,"iAP2iOSAppInfo[%d].iAP2iOSAppName is NULL",i);
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
           U64 mac_address[] = {IAP2_BT_MAC_ADDRESS1, IAP2_BT_MAC_ADDRESS2};

           /* more than one iAP2BluetoothTransportComponent possible */
           accInfo->iAP2BluetoothTransportComponent = (iAP2BluetoothTransportComponent*)calloc(IAP2_BT_MAC_ADDRESS_CNT, sizeof(iAP2BluetoothTransportComponent) );
           if(accInfo->iAP2BluetoothTransportComponent != NULL)
           {
               accInfo->iAP2BluetoothTransportComponent_count = IAP2_BT_MAC_ADDRESS_CNT;
               for(i=0; i<IAP2_BT_MAC_ADDRESS_CNT; i++)
               {
                   rc = iap2AllocateandUpdateData(&accInfo->iAP2BluetoothTransportComponent[i].iAP2BluetoothTransportMediaAccessControlAddress,
                                                  &mac_address[i],
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

   #if IAP2_ENABLE_ROUTE_GUIDANCE_COMPONENT_TEST
        if(rc == IAP2_OK)
        {
            U16 RG_Display_Identifier = IAP2_ROUTE_GUIDANCE_DISPLAY_COMP_ID;
            U8  RG_Display_Name[1][256] = {IAP2_ROUTE_GUIDANCE_DISPLAY_COMP_NAME};
            U16 RG_Max_Current_Road_Name_Length[] = {256};
            U16 RG_Max_Destination_Road_Name_Length[] = {256};
            U16 RG_Max_After_Maneuver_Road_Name_Length[] = {256};
            U16 RG_Max_Maneuver_Description_Length[] = {512};
            U16 RG_Max_GuidanceManeuver_Storage_Capacity[] = {20};

            /* more than one iAP2RouteGuidanceDisplayComponent possible */
            accInfo->iAP2RouteGuidanceDisplayComponent = (iAP2RouteGuidanceDisplayComponent*)calloc(1, sizeof(iAP2RouteGuidanceDisplayComponent));
            if(accInfo->iAP2RouteGuidanceDisplayComponent != NULL)
            {
                accInfo->iAP2RouteGuidanceDisplayComponent_count = 1;
                for(i=0; i<1; i++)
                {
                    /* each iAP2BluetoothTransportComponent must have an unique identifier */
                    RG_Display_Identifier += i;
                    rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier,
                                                   &RG_Display_Identifier,
                                                   &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Identifier_count,
                                                   1, iAP2_uint16);
                    if(rc == IAP2_OK)
                    {
                        U8* RGDisplay_Name = RG_Display_Name[i];
                        /* provide the name of the Route Guidance Display Component */
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Name,
                                                       &RGDisplay_Name,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2Name_count,
                                                       1, iAP2_utf8);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength,
                                                       &RG_Max_Current_Road_Name_Length,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxCurrentRoadNameLength_count,
                                                       1, iAP2_uint16);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength,
                                                       &RG_Max_Destination_Road_Name_Length,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxDestinationRoadNameLength_count,
                                                       1, iAP2_uint16);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength,
                                                       &RG_Max_After_Maneuver_Road_Name_Length,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxAfterManeuverRoadNameLength_count,
                                                       1, iAP2_uint16);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength,
                                                       &RG_Max_Maneuver_Description_Length,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxManeuverDescriptionLength_count,
                                                       1, iAP2_uint16);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity,
                                                       &RG_Max_GuidanceManeuver_Storage_Capacity,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceStorageCapacity_count,
                                                       1, iAP2_uint16);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity,
                                                       &RG_Max_Guidance_Capacity,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxGuidanceManeuverStorageCapacity_count,
                                                       1, iAP2_uint16);
                    }
                    if(rc == IAP2_OK)
                    {
                        rc = iap2AllocateandUpdateData(&accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength,
                                                       &RG_Max_Guidance_Description_Length,
                                                       &accInfo->iAP2RouteGuidanceDisplayComponent[i].iAP2MaxLaneGuidanceDescriptionLength_count,
                                                       1, iAP2_uint16);
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
              U8  MapsDisplayName[]                 = {"MapsDisplayName"};
              accInfo->iAP2VehicleInformationComponent->iAP2MapsDisplayName = (U8*)strndup( (const char*)MapsDisplayName, strnlen((const char*)MapsDisplayName, STRING_MAX) );
              if(accInfo->iAP2VehicleInformationComponent->iAP2MapsDisplayName == NULL)
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
                iap2InitCSCallbacks(iAP2InitParam->p_iAP2CSCallbacks,
                                    iAP2InitParam->p_iAP2AccessoryConfig->iAP2EAPSupported,
                                    iAP2InitParam->p_iAP2AccessoryConfig->iAP2iOSintheCar);
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
            /* External Accessory Protocol session callbacks */
            if(NULL != iAP2InitParam->p_iAP2MultiEAPSessionCallbacks)
            {
                iap2InitMultiEAPSessionCallbacks(iAP2InitParam->p_iAP2MultiEAPSessionCallbacks);
            }
            /* init ipa2stack callback */
            if(NULL != iAP2InitParam->p_iAP2StackCallbacks)
            {
                iap2InitStackCallbacks(iAP2InitParam->p_iAP2StackCallbacks);
            }
            /* init EA native transport callbacks */
            if(NULL != iAP2InitParam->p_iAP2EANativeTransportCallbacks)
            {
                iap2InitEANativeTransportCallbacks(iAP2InitParam->p_iAP2EANativeTransportCallbacks);
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

