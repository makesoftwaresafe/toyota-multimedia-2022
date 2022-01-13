/****************************************************
 *  ipp_iap2_getdbentries.h                                         
 *  Created on: 2014/01/17 17:55:31                      
 *  Implementation of the Class ipp_iap2_play       
 *  Original author: mshibata                     
 ****************************************************/

#ifndef IPP_IAP2_GETDBENTREIS_H
#define IPP_IAP2_GETDBENTREIS_H

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
#include "iPodPlayerIPCLib.h"

typedef enum
{
    IPP_GETDBENTRIES_STAGE_CHECK_PROGRESS = 0x00,
    IPP_GETDBENTRIES_STAGE_GET_LIST,
    IPP_GETDBENTRIES_STAGE_CANCEL
}IPOD_PLAYER_IAP2_GETDBENTRIES_STAGE;
S32 ippiAP2GetDBEntries(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /* IPP_IAP2_GETDBENTREIS_H */

