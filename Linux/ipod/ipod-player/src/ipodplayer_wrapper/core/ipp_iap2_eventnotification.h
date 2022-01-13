/****************************************************
 *  ipp_iap2_eventnotification.h                                         
 *  Created on: 2014/01/17 17:55:28                      
 *  Implementation of the Class ipp_iap2_eventnotification       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(_IPP_IAP2_EVENTNOTIFICATION_H_)
#define _IPP_IAP2_EVENTNOTIFICATION_H_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerIPCLib.h"


typedef struct
{
    U8 index;
    U16 session;
} IAP2_IOSAPP_INFO;


typedef S32 (* IPP_IAP2_NOTIFY_PLAYBACK_STATUS) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
typedef S32 (* IPP_IAP2_NOTIFY_CONNECTION_STATUS) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
typedef S32 (* IPP_IAP2_NOTIFY_TRACK_INFO) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 infoMask);
typedef S32 (* IPP_IAP2_NOTIFY_PLAYBACK_CHANGE) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
typedef S32 (* IPP_IAP2_NOTIFY_DB_ENTRIES) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
typedef S32 (* IPP_IAP2_NOTIFY_COVERART_DATA) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataSize, U8 *data);
typedef S32 (* IPP_IAP2_NOTIFY_OPEN_APP) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IAP2_IOSAPP_INFO *iOSAppInfo);
typedef S32 (* IPP_IAP2_NOTIFY_CLOSE_APP) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U16 appSession);
typedef S32 (* IPP_IAP2_NOTIFY_RECEIVE_FROM_APP) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U8 appSession, U16 dataSize, U8 *appData);
typedef S32 (* IPP_IAP2_NOTIFY_DEVICE_EVENT) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 eventMask);
typedef S32 (* IPP_IAP2_NOTIFY_LOCATION_INFO_STATUS) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_LOCATION_INFO_STATUS status, U32 locationMask);
typedef S32 (* IPP_IAP2_NOTIFY_VEHICLE_STATUS) (IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_VEHICLE_STATUS status);


typedef struct
{
    IPP_IAP2_NOTIFY_CONNECTION_STATUS notifyConntectionStatus;
    IPP_IAP2_NOTIFY_PLAYBACK_STATUS notifyPlayBackStatus;
    IPP_IAP2_NOTIFY_TRACK_INFO notifyTrackInformation;
    IPP_IAP2_NOTIFY_PLAYBACK_CHANGE notifyPlayBackChange;
    IPP_IAP2_NOTIFY_DB_ENTRIES notifyDBEntries;
    IPP_IAP2_NOTIFY_COVERART_DATA notifyCoverArtData;
    IPP_IAP2_NOTIFY_OPEN_APP notifyOpeniOSApplication;
    IPP_IAP2_NOTIFY_CLOSE_APP notifyCloseiOSApplication;
    IPP_IAP2_NOTIFY_RECEIVE_FROM_APP notifyReceiveFromiOSApplication;
    IPP_IAP2_NOTIFY_DEVICE_EVENT notifyDeviceEvent;
    IPP_IAP2_NOTIFY_LOCATION_INFO_STATUS notifyLocationInfoStatus;
    IPP_IAP2_NOTIFY_VEHICLE_STATUS notifyVehicleStatus;
    
} IAP2_EVENTNOTIFICATION_FUNC_TABLE;



IAP2_EVENTNOTIFICATION_FUNC_TABLE *ippiAP2EventNotificationSetNotifyFunctions(void);
void ippiAP2EventNotificationClearNotifyFunctions(IAP2_EVENTNOTIFICATION_FUNC_TABLE *notifyFuncs);
S32 ippiAP2EventNotificationSendNotifyMessage(S32 handle, U8 *message, U32 size);


#endif /*!defined(_IPP_IAP2_EVENTNOTIFICATION_H_)*/
 
