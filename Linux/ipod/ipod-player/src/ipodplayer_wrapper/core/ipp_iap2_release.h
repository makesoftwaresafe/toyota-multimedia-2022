/*****************************************************
 *  ipp_iap2_release.h                                  
 *  Created on: 2017/11/28 21:36:00                   
 *  Implementation of the Class ipp_iap2_release        
 *  Original author: madachi                          
 ****************************************************/

#if !defined(IPP_IAP2_RELEASE_H)
#define IPP_IAP2_RELEASE_H

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
#include "iPodPlayerUtilityLog.h"

S32 ippiAP2Release(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /*!defined(IPP_IAP2_RELEASE_H)*/
 
