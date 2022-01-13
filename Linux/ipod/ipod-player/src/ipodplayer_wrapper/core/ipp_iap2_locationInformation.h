/*******************************************************
 *  ipp_iap2_locationInformation.h                          
 *  Created on: 2016/11/29 18:35:00                     
 *  Implementation of the Class ipp_iap2_locationInformation
 *  Original author: madachi                            
 *******************************************************/

#ifndef __ipp_iap2_locationInformation_h__
#define __ipp_iap2_locationInformation_h__


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2SetLocationInformation( IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /* ifndef __ipp_iap2_locationInformation_h__ */
 
