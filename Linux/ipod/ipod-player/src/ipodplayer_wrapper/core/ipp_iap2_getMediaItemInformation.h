/****************************************************
 *  ipp_iap2_getMediaItemInformation.h                                         
 *  Created on: 2017/07/12 16:00:00                      
 *  Implementation of the Class ipp_iap2_getMediaItemInfo       
 *  Original author: madachi                     
 ****************************************************/

#ifndef IPP_IAP2_GET_MEDIA_ITEM_INFO 
#define IPP_IAP2_GET_MEDIA_ITEM_INFO

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iPodPlayerCoreDef.h"

#define IPP_INVALID_TRACKID             -2
#define IPP_MEDIALIB_UP_INPROGRESS      -1


S32 ippiAP2GetMediaItemInfo(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_IAP2_EXECUTE_PARAM *param);


#endif /* IPP_IAP2_GET_MEDIA_ITEM_INFO */
 
