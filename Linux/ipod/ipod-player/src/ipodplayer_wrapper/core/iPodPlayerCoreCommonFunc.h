#ifndef _IPOD_PLAYER_COMMON_FUNC_H
#define _IPOD_PLAYER_COMMON_FUNC_H

typedef struct
{
    mqd_t id;
    sem_t *semOutId;
    sem_t *semInId;
    U8 *queueInBuf;
    U8 *queueOutBuf;
    U64 maxBufSize;
    U64 maxQueueNum;
} IPOD_PLAYER_CORE_QUEUE_INFO;
S32 iPodCoreCommonCreateEpoll(S32 *epollFd, U8 maxNum);
S32 iPodCoreCommonAddEpoll(S32 epollFd, S32 addFd);
S32 iPodCoreCommonWaitEpoll(S32 epollFd, S32 *checkFd, U8 fdNum, U8 *checkNum, S32 timeout);
S32 iPodCoreCommonCreateQueue(IPOD_PLAYER_CORE_QUEUE_INFO *info, U8 *prefix, U8 *identify, U64 maxBufSize, U64 maxQueueNum);
S32 iPodCoreCommonDeleteQueue(IPOD_PLAYER_CORE_QUEUE_INFO *info);
S32 iPodCoreCommonWaitQueue(IPOD_PLAYER_CORE_QUEUE_INFO *info, U8 *data, U32 size);
S32 iPodCoreCommonSendQueue(IPOD_PLAYER_CORE_QUEUE_INFO *info, U8 *data, U32 size, U8 prior);

S32 iPodCoreCommonCreateSem(sem_t **semId);
S32 iPodCoreCommonWaitSem(sem_t *semId);
S32 iPodCoreCommonPostSem(sem_t *semId);
S32 iPodCoreCommonDeleateSem(sem_t **semId);

#endif /* IPOD_PLAYER_COMMON_FUNC_H */
