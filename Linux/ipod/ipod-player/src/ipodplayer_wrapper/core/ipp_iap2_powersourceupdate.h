/****************************************************
 *  ipp_iap2_powersourceupdate.h                             
 *  Created on:         2017/02/14 12:00:00          
 *  Implementation of the Class ippiAP2PowerSourceUpdate   
 *  Original author: madachi                         
 ****************************************************/

#ifndef IPP_IAP2_POWERSOURCEUPDATE_H
#define IPP_IAP2_POWERSOURCEUPDATE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 powerSourceUpdate(iAP2Device_t *device, U16 current, U8 batteryCharge);
S32 ippiAP2PowerSourceUpdate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /* ifndef IPP_IAP2_POWERSOURCEUPDATE_H */
 
