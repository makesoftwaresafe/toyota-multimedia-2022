/****************************************************
 *  ipp_iap2_getdeviceproperty.h                                         
 *  Created on: 2014/01/17 17:55:29                      
 *  Implementation of the Class ipp_iap2_getdeviceproperty       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_C32753C6_7E7F_4844_A9F2_56D6BD17F38A__INCLUDED_)
#define EA_C32753C6_7E7F_4844_A9F2_56D6BD17F38A__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2GetDeviceProperty(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /*!defined(EA_C32753C6_7E7F_4844_A9F2_56D6BD17F38A__INCLUDED_)*/
 