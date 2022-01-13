/****************************************************
 *  ipp_iap2_getplaybackstatus.h                                         
 *  Created on: 2014/01/17 17:55:29                      
 *  Implementation of the Class ipp_iap2_getplaybackstatus       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_8BE9579B_9319_406e_9330_AF18C81AC131__INCLUDED_)
#define EA_8BE9579B_9319_406e_9330_AF18C81AC131__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2GetPlaybackStatus(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /*!defined(EA_8BE9579B_9319_406e_9330_AF18C81AC131__INCLUDED_)*/
 