/****************************************************
 *  ipp_iap2_init.h                                         
 *  Created on: 2014/01/16 17:54:41                      
 *  Implementation of the Class ipp_iap2_init       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_38DA4DD7_DD21_404f_953C_6212E488C644__INCLUDED_)
#define EA_38DA4DD7_DD21_404f_953C_6212E488C644__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_service_init.h"
#include "ipp_iap2_common.h"
#include "iPodPlayerCoreDef.h"


#define IPOD_PLAYER_CORE_HID_COMPONENT_NAME "Media Playback Remote"
#define IPOD_PLAYER_CORE_IAP2_ACC_INFO_EP_INIT "/dev/ffs/ep0"


S32 ippiAP2GetPollFDs(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param);
S32 ippiAP2Init(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param);
S32 ippiAP2Deinit(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param, U8 *serial);
S32 ippiAP2InitParam(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param, U8 devType, U8 * deviceName, void * callbackContext);
void ippiAP2DeinitParam(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param);
S32 ippiAP2HandleEvent(IPOD_PLAYER_CORE_IAP2_PARAM * iap2Param, S32 fd, S16 event);
BOOL ippiAP2CheckIdentificationTableFromDevice(U16 id, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo);
BOOL ippiAP2CheckIdentificationTableByAcc(U16 id, IPOD_PLAYER_CORE_THREAD_INFO *threadInfo);

S32 ippiAP2ServiceInitDeviceConnection(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param);
S32 ippiAP2ServiceDeviceDiscovered(IPOD_PLAYER_CORE_IAP2_PARAM *iap2Param);

#endif /*!defined(EA_38DA4DD7_DD21_404f_953C_6212E488C644__INCLUDED_)*/
 
