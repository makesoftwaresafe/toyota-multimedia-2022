/****************************************************
 *  ipp_iap2_gettrackinfo.h                                         
 *  Created on: 2014/01/17 17:55:30                      
 *  Implementation of the Class ipp_iap2_gettrackinfo       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_1B06510D_D737_499f_949F_131B8EBD2E0B__INCLUDED_)
#define EA_1B06510D_D737_499f_949F_131B8EBD2E0B__INCLUDED_


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
    IPP_IAP2_GET_TRACKINFO_STAGE_CHECK_PROGRESS = 0x00,
    IPP_IAP2_GET_TRACKINFO_STAGE_GET_TRACKINFO
} IPP_IAP2_GET_TRACKINFO_STAGE;

#define IPP_IAP2_GET_TRACKINFO_MAX              5


S32 ippiAP2GetTrackInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /*!defined(EA_1B06510D_D737_499f_949F_131B8EBD2E0B__INCLUDED_)*/
 
