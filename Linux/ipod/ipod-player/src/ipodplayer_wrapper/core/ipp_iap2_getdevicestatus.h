/****************************************************
 *  ipp_iap2_getdevicestatus.h                                         
 *  Created on: 2014/01/17 17:55:29                      
 *  Implementation of the Class ipp_iap2_getdevicestatus       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_95127BF1_3C48_4a7f_8442_3EADDFED118D__INCLUDED_)
#define EA_95127BF1_3C48_4a7f_8442_3EADDFED118D__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2GetDeviceStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /*!defined(EA_95127BF1_3C48_4a7f_8442_3EADDFED118D__INCLUDED_)*/
 