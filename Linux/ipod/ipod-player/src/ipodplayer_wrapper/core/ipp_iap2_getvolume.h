/****************************************************
 *  ipp_iap2_getvolume.h                                         
 *  Created on: 2014/01/17 17:55:30                      
 *  Implementation of the Class ipp_iap2_getvolume       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_695E5798_5BB5_4984_AD6B_9DA7B3EAA88F__INCLUDED_)
#define EA_695E5798_5BB5_4984_AD6B_9DA7B3EAA88F__INCLUDED_


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
    IPP_IAP2_GET_VOLUME_STAGE_SEND_REQUEST = 0x00,
    IPP_IAP2_GET_VOLUME_STAGE_CHECK_RESULT
} IPP_IAP2_GET_VOLUME_STAGE;

S32 ippiAP2GetVolume(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /*!defined(EA_695E5798_5BB5_4984_AD6B_9DA7B3EAA88F__INCLUDED_)*/
 
