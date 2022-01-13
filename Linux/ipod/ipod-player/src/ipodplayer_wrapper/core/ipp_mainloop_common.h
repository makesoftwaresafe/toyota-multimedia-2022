#ifndef IPP_MAINLOOP_COMMON_H
#define IPP_MAINLOOP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "adit_typedef_linux.h"
#include "iPodPlayerCoreDef.h"
#include "iPodPlayerCoreCfg.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerUtilityLog.h"

/* Common function to operate fd */
S32 iPodCoreEpollCtl(S32 epollFd, S32 handle, S32 ctl, U32 event);
void iPodCoreAddFDs(S32 epollFd, U32 num, S32 *handles);
void iPodCoreDelFDs(S32 epollFd, U32 num, S32 *handles);
S32 iPodCorePollWait(S32 pollFd, U32 waitTime, U32 infoNum, IPOD_PLAYER_IPC_HANDLE_INFO *outputInfo);

/* Common function to operate queue */
S32 iPodCoreSetQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size);
S32 iPodCoreCheckQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList, IPOD_PLAYER_FUNC_ID funcId);
void iPodCoreSortQueue(U32 listNum, IPOD_PLAYER_CORE_WAIT_LIST *waitList);
S32 iPodCoreDelQueue(IPOD_PLAYER_CORE_WAIT_LIST *waitList);
S32  iPodCoreCheckResultAndUpdateStatus(S32 result, IPOD_PLAYER_CORE_WAIT_LIST *waitData);

/* Common function to operate audio streaming */
S32 iPodCoreSetSampleRate(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 sample);
S32 iPodCoreSetSampleRateResult(IPOD_PLAYER_CORE_IPODCTRL_CFG *iPodCtrlCfg, IPOD_PLAYER_CORE_WAIT_LIST *waitData, IPOD_PLAYER_FUNC_HEADER *header, const IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 *size);

/* Common function to operate large data */
void iPodCoreInitLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList);
IPOD_PLAYER_CORE_LONG_RECEIVE_INFO * iPodCoreGetLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd);
S32 iPodCoreSetLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd, IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *info);
S32 iPodCoreClearLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd);
void iPodCoreCheckAndRemoveLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *longRecvInfo, S32 result, IPOD_PLAYER_IPC_HANDLE_INFO *handleInfo);


void iPodCoreClearData(IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents);

#endif /* IPP_MAINLOOP_COMMON_H */

