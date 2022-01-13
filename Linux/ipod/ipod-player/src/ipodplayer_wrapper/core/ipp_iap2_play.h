/****************************************************
 *  ipp_iap2_play.h                                         
 *  Created on: 2014/01/17 17:55:31                      
 *  Implementation of the Class ipp_iap2_play       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_02E17083_A116_4ebe_9EFF_A6084478652C__INCLUDED_)
#define EA_02E17083_A116_4ebe_9EFF_A6084478652C__INCLUDED_

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

S32 ippiAP2PlayMediaLibCurrentSelection(iAP2Device_t* iap2Device, U8 **uid);
S32 ippiAP2Play(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /*!defined(EA_02E17083_A116_4ebe_9EFF_A6084478652C__INCLUDED_)*/
 
