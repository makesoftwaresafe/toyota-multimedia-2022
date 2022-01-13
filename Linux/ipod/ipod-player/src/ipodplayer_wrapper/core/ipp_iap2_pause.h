/*****************************************************
 *  ipp_iap2_pause.h                                  
 *  Created on: 2014/01/17 17:55:31                   
 *  Implementation of the Class ipp_iap2_pause        
 *  Original author: madachi                          
 ****************************************************/

#if !defined(EA_624AF8C8_1AAE_4a94_92D2_405DF307D74B__INCLUDED_)
#define EA_624AF8C8_1AAE_4a94_92D2_405DF307D74B__INCLUDED_

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

S32 ippiAP2Pause(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /*!defined(EA_624AF8C8_1AAE_4a94_92D2_405DF307D74B__INCLUDED_)*/
 
