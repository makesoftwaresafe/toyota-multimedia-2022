/****************************************************
 *  ipp_iap2_gototrackposition.h                             
 *  Created on:         2017/02/02 12:00:00          
 *  Implementation of the Class ippiAP2GotoTrackPosition   
 *  Original author: madachi                         
 ****************************************************/

#ifndef IPP_IAP2_GOTOTRACKPOSITION_H
#define IPP_IAP2_GOTOTRACKPOSITION_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2GotoTrackPosition(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /* ifndef IPP_IAP2_GOTOTRACKPOSITION_H */
 
