/****************************************************
 *  ipp_iap2_ctrlcfglist.h                           
 *  Created on: 2017/05/24 10:00:00                  
 *  Implementation of the Class ipp_iap2_ctrlcfglist 
 *  Original author: mando                           
 ****************************************************/

#ifndef __ipp_iap2_ctrlcfglist_h__
#define __ipp_iap2_ctrlcfglist_h__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h> 
#include "iPodPlayerCoreDef.h"

void ippiAP2PrintCtrlCfgInfo();
S32 ippiAP2InitCtrlCfgList();
S32 ippiAP2EnqueueToCtrlCfgList(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2DequeueFromCtrlCfgList(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
IPOD_PLAYER_CORE_IPODCTRL_CFG *ippiAP2SerchBySerialNumFromCtrlCfgList(U8 *serialNum);

#endif /*__ipp_iap2_ctrlcfglist_h__*/ 
