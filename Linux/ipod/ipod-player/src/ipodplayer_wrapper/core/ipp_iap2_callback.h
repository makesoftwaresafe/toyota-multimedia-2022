/****************************************************
 *  ipp_iap2_callback.h                                         
 *  Created on: 2014/01/16 17:54:39                      
 *  Implementation of the Class ipp_iap2_callback       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_1780A95B_5676_4389_A889_CCD2D39DFFB4__INCLUDED_)
#define EA_1780A95B_5676_4389_A889_CCD2D39DFFB4__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/eventfd.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_parameters.h"
#include "iap2_service_init.h"
#include "iap2_callbacks.h"
#include "iPodPlayerCoreDef.h"

#define IPOD_PLAYER_SERVICECALLBACKS_MQNAME           "/mq_ipodplayer"
#define IPOD_PLAYER_SERVICECALLBACKS_MSGSIZE          (3 + 1)
#define IPOD_PLAYER_SERVICECALLBACKS_MAXMSG           1

#define IPP_IAP2_SERVICE_CALLBACK_EVENT_NONE                     "000"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICECONNECTEDDEVICE1  "101"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICECONNECTEDDEVICE2  "102"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_SERVICEDEVICESTATE       "103"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_ASSISTIVETOUCH           "201"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_BLUETOOTHCONNECTION      "202"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_DEVICEINFORMATION        "203"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_STARTEAPSESSION          "204"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_STOPEAPSESSION           "205"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_RECEIVEIOSAPPDATA        "206"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_STARTLOCATION            "207"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_STOPLOCATION             "208"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_STARTVEHICLESTARTUS      "209"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_STOPVEHICLESTARTUS       "210"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_MEDIALIBRARYINFO         "211"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_MEDIALIBRARYUPDATE       "212"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_NOWPLAYING               "213"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_POWER                    "214"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_DEVICEMODEAUDIO          "215"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERSETUP        "216"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERDATARECEIVE  "217"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERSUCCESS      "218"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERFAILURE      "219"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_FILETRANSFERCANCEL       "220"
#define IPP_IAP2_SERVICE_CALLBACK_EVENT_DEVICESTATE              "221"

S32 ippiAP2CBAuthenticationFailed(iAP2Device_t* thisDevice, iAP2AuthenticationFailedParameter* thisParameter, void* context);
S32 ippiAP2CBAuthenticationSucceeded(iAP2Device_t* thisDevice, iAP2AuthenticationSucceededParameter* thisParameter, void* context);
S32 ippiAP2CBRequestAuthenticationCertificate(iAP2Device_t* thisDevice, iAP2RequestAuthenticationCertificateParameter* thisParameter, void* context);
S32 ippiAP2CBRequestAuthenticationChallengeResponse(iAP2Device_t* thisDevice, iAP2RequestAuthenticationChallengeResponseParameter* thisParameter, void* context);
S32 ippiAP2CBStartIdentification(iAP2Device_t* thisDevice, iAP2StartIdentificationParameter* thisParameter, void* context);
S32 ippiAP2CBIdentificationAccepted(iAP2Device_t* thisDevice, iAP2IdentificationAcceptedParameter* thisParameter, void* context);
S32 ippiAP2CBIdentificationRejected(iAP2Device_t* thisDevice, iAP2IdentificationRejectedParameter* thisParameter, void* context);
S32 ippiAP2CBAssistiveTouchInformation(iAP2Device_t* thisDevice, iAP2AssistiveTouchInformationParameter* thisParameter, void* context);
S32 ippiAP2CBBluetoothConnectionUpdate(iAP2Device_t* thisDevice, iAP2BluetoothConnectionUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBDeviceAuthenticationCertificate(iAP2Device_t* thisDevice, iAP2DeviceAuthenticationCertificateParameter* thisParameter, void* context);
S32 ippiAP2CBDeviceAuthenticationResponse(iAP2Device_t* thisDevice, iAP2DeviceAuthenticationResponseParameter* thisParameter, void* context);
S32 ippiAP2CBDeviceInformationUpdate(iAP2Device_t* thisDevice, iAP2DeviceInformationUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBDeviceLanguageUpdate(iAP2Device_t* thisDevice, iAP2DeviceLanguageUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBStartExternalAccessoryProtocolSession(iAP2Device_t* thisDevice, iAP2StartExternalAccessoryProtocolSessionParameter* thisParameter, void* context);
S32 ippiAP2CBStopExternalAccessoryProtocolSession(iAP2Device_t* thisDevice, iAP2StopExternalAccessoryProtocolSessionParameter* thisParameter, void* context);
S32 ippiAP2CBDeviceHIDReport(iAP2Device_t* thisDevice, iAP2DeviceHIDReportParameter* thisParameter, void* context);
S32 ippiAP2CBStartLocationInformation(iAP2Device_t* thisDevice, iAP2StartLocationInformationParameter* thisParameter, void* context);
S32 ippiAP2CBStopLocationInformation(iAP2Device_t* thisDevice, iAP2StopLocationInformationParameter* thisParameter, void* context);
S32 ippiAP2CBMediaLibraryInformation(iAP2Device_t* thisDevice, iAP2MediaLibraryInformationParameter* thisParameter, void* context);
S32 ippiAP2CBMediaLibraryUpdate(iAP2Device_t* thisDevice, iAP2MediaLibraryUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBNowPlayingUpdateParameter(iAP2Device_t* thisDevice, iAP2NowPlayingUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBPowerUpdate(iAP2Device_t* thisDevice, iAP2PowerUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBTelephonyCallStateInformation(iAP2Device_t* thisDevice, iAP2TelephonyCallStateInformationParameter* thisParameter, void* context);
S32 ippiAP2CBTelephonyUpdate(iAP2Device_t* thisDevice, iAP2TelephonyUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBUSBDeviceModeAudioInformation(iAP2Device_t* thisDevice, iAP2USBDeviceModeAudioInformationParameter* thisParameter, void* context);
S32 ippiAP2CBStartVehicleStatusUpdates(iAP2Device_t* thisDevice, iAP2StartVehicleStatusUpdatesParameter* thisParameter, void* context);
S32 ippiAP2CBStopVehicleStatusUpdates(iAP2Device_t* thisDevice, iAP2StopVehicleStatusUpdatesParameter* thisParameter, void* context);
S32 ippiAP2CBVoiceOverCursorUpdate(iAP2Device_t* thisDevice, iAP2VoiceOverCursorUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBVoiceOverUpdate(iAP2Device_t* thisDevice, iAP2VoiceOverUpdateParameter* thisParameter, void* context);
S32 ippiAP2CBWiFiInformation(iAP2Device_t* thisDevice, iAP2WiFiInformationParameter* thisParameter, void* context);

S32 ippiAP2SetCallbacks(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param);
void ippiAP2ClearCallbacks(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param);

S32 ippiAP2InitServiceCallbackRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2DeinitServiceCallbackRequest(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
int32_t ippiAP2ServiceDeviceConnectedCB(iAP2Service_t* service, iAP2ServiceDeviceList_t* msg);
int32_t ippiAP2ServiceDeviceDisconnectedCB(iAP2Service_t* service, uint32_t deviceId);
int32_t ippiAP2ServiceDeviceStateCB(iAP2Service_t* service, uint32_t deviceId, iAP2ServiceDeviceState_t* msg);
int32_t ippiAP2ServiceConnectDeviceRespCB(iAP2Service_t* service, uint32_t deviceId, iAP2ServiceConnectDeviceResp_t* msg);
S32 ippiAP2ServiceSetCallbacks(iAP2ServiceCallbacks_t* serviceCallbacks);
S32 ippiAP2InitServiceCallbacksMutex(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2DeinitServiceCallbacksMutex(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2ServiceCallbacksMutexLock(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2ServiceCallbacksMutexUnlock(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);

#endif /*!defined(EA_1780A95B_5676_4389_A889_CCD2D39DFFB4__INCLUDED_)*/
 
