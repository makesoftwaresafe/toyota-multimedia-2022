/****************************************************
 *  ipp_iap2_devinit.h                                         
 *  Created on: 2014/01/17 17:55:31                      
 *  Implementation of the Class ipp_iap2_play       
 *  Original author: mshibata                     
 ****************************************************/

#ifndef IPP_IAP2_DEVINIT_H
#define IPP_IAP2_DEVINIT_H

#include <iap2_service_init.h>
#include <adit_dlt.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2DevInit(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);
S32 ippiAP2DelayedNotification(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2DelayedEventMutex(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_DELAYED_MUTEX_FUNC func);
void iPodCoreObserverClearDelayedEvent(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
void iPodCoreObserverSetDelayedEvent(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);


#endif /* IPP_IAP2_DEVINIT_H */
 
