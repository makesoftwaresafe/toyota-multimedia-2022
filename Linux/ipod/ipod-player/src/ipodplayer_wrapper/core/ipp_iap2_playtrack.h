/****************************************************
 *  ipp_iap2_playtrack.h                                         
 *  Created on: 2014/01/17 17:55:31                      
 *  Implementation of the Class ipp_iap2_playtrack       
 *  Original author: mshibata                     
 ****************************************************/

#if !defined(EA_5A0D6319_228D_4d56_BC9F_D283F7B8955F__INCLUDED_)
#define EA_5A0D6319_228D_4d56_BC9F_D283F7B8955F__INCLUDED_


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

S32 ippiAP2PlayMediaLibraryItem(iAP2Device_t* iap2Device, iAP2Blob *ItemPersistentID, U32 startIndex, U8 **uid);
S32 ippiAP2PlayTrack(IPOD_PLAYER_CORE_IPODCTRL_CFG * iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);

#endif /*!defined(EA_5A0D6319_228D_4d56_BC9F_D283F7B8955F__INCLUDED_)*/
 
