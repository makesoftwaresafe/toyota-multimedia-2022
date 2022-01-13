#ifndef IPP_AUDIOCOMMON_H
#define IPP_AUDIOCOMMON_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "adit_typedef_linux.h"
#include "iap2_defines.h"
#include "iap2_commands.h"
#include "iap2_parameters.h"
#include "iap2_parameter_free.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerUtilityLog.h"
#include "iPodPlayerIPCLib.h"

S32 iPodCoreiPodCtrlAudioInitIPC(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
void iPodCoreiPodCtrlAudioDeinitIPC(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);


#endif

