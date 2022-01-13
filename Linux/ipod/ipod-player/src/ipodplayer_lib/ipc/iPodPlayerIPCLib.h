/*! \file iPodPlayerLib.h
 *
 * \version: 1.0
 *
 * \author: steranaka
 */
#ifndef IPOD_PLAYER_IPC_LIB_H
#define IPOD_PLAYER_IPC_LIB_H

#include "iPodPlayerIPCLocal.h"

/* ################################### iPod Player S/C Mode Change ########################################## */

#define IPOD_PLAYER_IPC_FLAG_NONBLOCK 0

#define IPOD_PLAYER_IPC_FLAG_BLOCK 1

#define IPOD_PLAYER_IPC_OK          0

#define IPOD_PLAYER_IPC_NO_ACCEPT   1

#define IPOD_PLAYER_IPC_ERROR       -1

#define IPOD_PLAYER_IPC_ERR_NOMEM   -2

#define IPOD_PLAYER_IPC_ERR_MORE_PACKET -3

#define IPOD_PLAYER_IPC_FAILED -1

#define IPOD_PLAYER_IPC_TMO -4

#define IPOD_PLAYER_IPC_ERR_AGAIN -5

#define IPOD_PLAYER_IPC_ERR_CLOSE -6

#define IPOD_PLAYER_IPC_ERR_NOINFO -7

#define IPOD_PLAYER_IPC_ERR_EXIST -8

/* ##################################################################################################### */

/* ################################### iPod Player Function Header ########################################## */
S32 iPodPlayerIPCInit(void);
S32 iPodPlayerIPCDeinit(void);

S32 iPodPlayerIPCOpen(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *info);
S32 iPodPlayerIPCReceive(S32 handle, U8 *message, U32 size, S32 flag, S32 waitHandle, S32 timeout);
S32 iPodPlayerIPCSend(S32 handle, U8 *message, U32 size, U32 flag, S32 timeout);
S32 iPodPlayerIPCClose(S32 handle);
S32 iPodPlayerIPCWait(S32 waitHandle, U32 handleNum, S32 *handles, U32 *resNum, S32 *resHandle, U8 *retType, S32 timeout, U32 flag);
S32 iPodPlayerIPCCreateHandle(S32 *handle, S32 fd);
S32 iPodPlayerIPCDeleteHandle(S32 fd);
S32 iPodPlayerIPCLongSend(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout);
S32 iPodPlayerIPCLongReceive(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, S32 waitHandle, U32 flag, S32 timeout);

S32 iPodPlayerDoSemOpen(sem_t **semId, U8 *name);
S32 iPodPlayerDoSemClose(sem_t **semId);
S32 iPodPlayerDoSemPost(sem_t *semId);
S32 iPodPlayerDoSemWait(sem_t *semId, U32 timeout_ms);
S32 iPodPlayerIPCIterate(S32 waitHandle, U32 handleNum, IPOD_PLAYER_IPC_HANDLE_INFO *inputInfo, U32 num, IPOD_PLAYER_IPC_HANDLE_INFO *outputInfo);
S32 iPodPlayerIPCGetFDs(S32 handle, S32 num, S32 *fds);

#endif /* IPOD_PLAYER_IPC_LIB_H */
