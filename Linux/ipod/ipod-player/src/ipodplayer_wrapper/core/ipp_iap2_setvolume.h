/****************************************************
 *  ipp_iap2_setvolume.h                                         
 *  Created on: 2014/01/17 17:55:33                      
 *  Implementation of the Class ipp_iap2_setvolume       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_EDD6E2CE_ECD3_4714_8393_5EA063DC005B__INCLUDED_)
#define EA_EDD6E2CE_ECD3_4714_8393_5EA063DC005B__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerIPCLib.h"

typedef enum
{
    IPP_IAP2_SET_VOLUME_STAGE_SEND_REQUEST = 0x00,
    IPP_IAP2_SET_VOLUME_STAGE_CHECK_RESULT
} IPP_IAP2_SET_VOLUME_STAGE;

S32 ippiAP2SetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /*!defined(EA_EDD6E2CE_ECD3_4714_8393_5EA063DC005B__INCLUDED_)*/
 
