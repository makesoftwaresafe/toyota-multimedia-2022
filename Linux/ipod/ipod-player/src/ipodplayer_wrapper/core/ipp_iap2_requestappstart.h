/*******************************************************
 *  ipp_iap2_requestappstart.h                          
 *  Created on: 2014/02/12 17:55:33                     
 *  Implementation of the Class ipp_iap2_requestappstart
 *  Original author: madachi                            
 *******************************************************/

#ifndef __ipp_iap2_requestappstart_h__
#define __ipp_iap2_requestappstart_h__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2RequestAppLaunch(iAP2Device_t* iap2Device, const U8* appname);
S32 ippiAP2RequestAppStart( IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /* ifndef __ipp_iap2_requestappstart_h__ */
 
