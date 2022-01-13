/****************************************************
 *  ipp_iap2_setaudiomode.h                                         
 *  Created on: 2014/01/17 17:55:32                      
 *  Implementation of the Class ipp_iap2_setaudiomode       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_58D243D8_575A_4239_A299_7883B6C8ABDA__INCLUDED_)
#define EA_58D243D8_575A_4239_A299_7883B6C8ABDA__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

typedef enum
{
    IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_REQUEST_SELECT = 0,
    IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_AUDIO_START,
    IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_AUDIO_STOP,
    IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE_CHECK_RESULT,
} IPOD_PLAYER_IAP2_SETAUDIOMODE_STAGE;


S32 iPodCoreiAP2AudioDeinitCfg(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 iPodCoreiAP2StopAudio(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
S32 ippiAP2SetAudioMode(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /*!defined(EA_58D243D8_575A_4239_A299_7883B6C8ABDA__INCLUDED_)*/
 