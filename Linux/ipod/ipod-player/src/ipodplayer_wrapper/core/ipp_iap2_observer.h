#ifndef IPP_IAP2_OBSERVER_H
#define IPP_IAP2_OBSERVER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreCfg.h"
#include "ipp_iap2_common.h"

#define IPOD_PLAYER_MEDIA_ITEM_REVISION_LEN_MAX 256
#define IPP_IAP2_FILE_ID_MAX 256

S32 iPodCoreObserverSetTrackInfoNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, TrackInfoMask_t *dataMask);
S32 iPodCoreObserverSetDeviceEventNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask);
U32 iPodCoreObserverGetDeviceEventNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);
void iPodCoreObserverSetUpdateDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask, U32 subMask, U32 extMask);
void iPodCoreObserverClearUpdateDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask, U32 subMask);
S32 iPodCoreObserverCheckIsUpdateDataMaskSet(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, U32 dataMask, U32 subMask);
S32 iPodCoreObserverCheckIsNotifyDataMaskSet(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE type, U32 dataMask);
U32 iPodCoreObserverGetNeedToNotifyDataMask(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPP_IAP2_NOTIFY_TYPE type);
S32 iPodCoreObserverGetFileXferLocalDevicePlaylistCount(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list);
S32 iPodCoreObserverGetFileXferAppleMusicRadioPlaylistCount(IPOD_PLAYER_CORE_IAP2_FILE_XFER_LIST **list);

S32 iPodCoreObserver(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg);

#endif /* IPP_IAP2_OBSERVER_H */

