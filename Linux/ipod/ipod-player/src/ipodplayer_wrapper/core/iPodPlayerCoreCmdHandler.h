#ifndef IPOD_PLAYER_CMD_HANDLER_H
#define IPOD_PLAYER_CMD_HANDLER_H

S32 iPodCoreCreateiPodCtrlHandler(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION *detectInfo, U32 devID);
S32 iPodCoreDeleteiPodCtrlHandler(IPOD_PLAYER_CORE_CFG *iPodCfg, IPOD_PLAYER_PARAM_SET_DEVICE_DETECTION *detectInfo, U32 devID);

S32 iPodCoreiPodCtrlSetList(IPOD_PLAYER_CORE_WAIT_LIST *waitList, IPOD_PLAYER_FUNC_HEADER *header, IPOD_PLAYER_MESSAGE_DATA_CONTENTS *contents, U32 size);
S32 iPodCoreiPodCtrlDelList(IPOD_PLAYER_CORE_WAIT_LIST *waitList);
S32 iPodCoreSetHandle(S32 *handle, U32 *handleNum, S32 setHandle);
S32 iPodCoreClearHandle(S32 *handle, U32 *handleNum, S32 clearHandle);

void iPodCoreInitLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList);
IPOD_PLAYER_CORE_LONG_RECEIVE_INFO * iPodCoreGetLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd);
S32 iPodCoreSetLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd, IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *info);
S32 iPodCoreClearLongRecvInfo(IPOD_PLAYER_CORE_LONG_RECEIVE_INFO *recvInfoList, S32 fd);

S32 iPodCoreiPodCtrlInitThreadInfoStack();
void iPodCoreiPodCtrlDeInitThreadInfoStack();
#endif /* IPOD_PLAYER_CMD_HANDLER_H */