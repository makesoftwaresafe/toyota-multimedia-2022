/*! \file iPodPlayer.h
 *
 * \version: 1.0
 *
 * \author: mshibata
 */
#ifndef IPOD_PLAYER_IPC_MESSAGE_H
#define IPOD_PLAYER_IPC_MESSAGE_H
#include <semaphore.h>

/* ################################### iPod Player Define Header ########################################## */
#define IPOD_PLAYER_SOCKET_NAME      "/tmp/IPPSock_"

#define IPOD_PLAYER_SEND_RETRY_NUM  10
#define IPOD_PLAYER_RECV_RETRY_NUM  10
#define IPOD_PLAYER_MESSAGE_SEND_RETRY_NUM  3
#define IPOD_PLAYER_MESSAGE_RECV_RETRY_NUM  3
#define IPOD_PLAYER_TOTAL_SEQ_NO   100
#define IPOD_PLAYER_IPC_MAX_EPOLL_NUM 128

#define IPOD_PLAYER_CONNECTION_NUM      1

#define IPOD_PLAYER_SOCKET_NAME_LEN 104

#define IPOD_PLAYER_SOCKET_IDENTIFY "IPP"

#define IPOD_PLAYER_IPC_MSEC 1000
#define IPOD_PLAYER_IPC_NSEC 1000000
#define IPOD_PLAYER_IPC_SEC  1000000000
#define IPOD_PLAYER_IPC_SEMWAIT_TMO 60000
#define IPOD_PLAYER_IPC_CONNECT_WAIT 1
#define IPOD_PLAYER_IPC_SEND_RETRY_WAIT 100000000
#define IPOD_PLAYER_IPC_SEND_RETRY_MAX 10
#define IPOD_PLAYER_WAIT_IN  0x01
#define IPOD_PLAYER_WAIT_OUT 0x02
#define IPOD_PLAYER_IPC_RECV_WAIT_NUM 1
#define IPOD_PLAYER_IPC_TEMP_MAX_LEN 256
#define IPOD_PLAYER_IPC_INTR_RETRY_WAIT 10000000
#define IPOD_PLAYER_IPC_MQUEUE_RETRY_MAX 5
#define IPOD_PLAYER_IPC_DATA_SEM_WAIT 10000
#define IPOD_PLAYER_IPC_ERPOLL_RETRY_MAX 10

/* ################################### iPod Player Function Header ########################################## */

typedef struct{
    U8 identify[4];         /* Identify data */
    S32 totalSeqNo;         /* max sequence no */
    S32 seqNo;              /* sequence no */
    S32 totalMessageSize;   /* total message size */
    S32 messageSize;        /* message size  */
}IPOD_PLAYER_MESSAGE_HEADER;


typedef struct
{
    /* Semaphore ID */
    S32 *semId;
    /* For socket descriptor storage and acquisition */
    S32 sd;
    /* socket descripter for communicate with client */
    S32 csd;
} IPOD_PLAYER_PARAM_SOCKET_INFO;

typedef struct
{
    U8 isReady;
    S32 id;
    U8 type;
    S32 pid;
    S32 cid[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    U8 namedSem;
    sem_t *semId;
    U64 maxBufSize;
    U64 maxPacketSize;
    U8 sockFilePath[IPOD_PLAYER_IPC_TEMP_MAX_LEN];
} IPOD_PLAYER_PARAM_DESC_INFO;

typedef struct
{
    IPOD_PLAYER_PARAM_DESC_INFO info[IPOD_PLAYER_IPC_MAX_EPOLL_NUM];
    sem_t *semId;
} IPOD_PLAYER_PARAM_IPC_INFO;

typedef enum
{
    /* Socket server mode  */
    IPOD_PLAYER_OPEN_SOCKET_SERVER = 0x00,
    
    /* Socket client mode  */
    IPOD_PLAYER_OPEN_SOCKET_CLIENT,
    
    /* Queue server mode */
    IPOD_PLAYER_OPEN_QUEUE_SERVER,
    
    /* Queue client mode */
    IPOD_PLAYER_OPEN_QUEUE_CLIENT,
    
    /* Socket accepted */
    IPOD_PLAYER_OPEN_SOCKET_ACCEPT,
    
    /* Wait mode */
    IPOD_PLAYER_OPEN_WAIT_HANDLE,
    
    /* Socket server mode for long data */
    IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG,
    
    /* Socket client mode for long data */
    IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG,
    
    /* Queue server mode for long data */
    IPOD_PLAYER_OPEN_QUEUE_SERVER_LONG,
    
    /* Queue client mode for long data */
    IPOD_PLAYER_OPEN_QUEUE_CLIENT_LONG,
    
    /* Socket accpeted for long data */
    IPOD_PLAYER_OPEN_SOCKET_ACCEPT_LONG,
    
    /* User selected mode */
    IPOD_PLAYER_OPEN_USER_SELECT,
    
    IPOD_PLAYER_OPEN_TYPE_MAX
    
} IPOD_PLAYER_IPC_OPEN_TYPE;


typedef struct
{
    IPOD_PLAYER_IPC_OPEN_TYPE type;
    U8 *prefix;
    U8 *identify;
    U8 connectionNum;
    U8 *semName;
    U64 maxBufSize;
    U64 maxPacketSize;
} IPOD_PLAYER_IPC_OPEN_INFO;

typedef struct
{
    S32 handle;
    U8 type;
    U32 event;
} IPOD_PLAYER_IPC_HANDLE_INFO;

void iPodPlayerIPCFree(U8 *buf);
S32 iPodPlayerIPCInfoInit(void);
S32 iPodPlayerIPCInfoDeinit(void);
IPOD_PLAYER_PARAM_DESC_INFO * iPodPlayerIPCRefIPCInfo(S32 handle);
IPOD_PLAYER_PARAM_DESC_INFO * iPodPlayerIPCRefAllIPCInfo(S32 *num);
S32 iPodPlayerIPCUnrefIPCInfo(IPOD_PLAYER_PARAM_DESC_INFO *info);
void iPodPlayerIPCClearIPCInfo(IPOD_PLAYER_PARAM_DESC_INFO *info);

/* server side */
S32 iPodPlayerDoCreateServer(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo, U8 *uname, U8 connectionNum);
S32 iPodPlayerDoAcceptRecv(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo);
S32 iPodPlayerDoCloseServer(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo);
S32 iPodPlayerDoRecvSocket(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo, U8 *message, U32 size, S32 flag);
/* client side */
S32 iPodPlayerDoCreateClient(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo, U8 *uname, U8 connectionNum, U8 *semName);
S32 iPodPlayerDoCloseClient(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo);
S32 iPodPlayerDoSendSocket(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo, U8 *message, U32 size, U32 flag);
S32 iPodPlayerDoEnd(void);

S32 iPodPlayerIPCDoSemOpen(sem_t **semId, U8 *name, U8 *namedSem);
S32 iPodPlayerIPCDoSemClose(sem_t **semId, U8 namedSem);
S32 iPodPlayerIPCDoSemPost(sem_t *semId);
S32 iPodPlayerIPCDoSemWait(sem_t *semId, U32 timeout_ms);
S32 iPodPlayerIPCOpenSocketClient(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo);
S32 iPodPlayerIPCOpenQueueClient(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo);
S32 iPodPlayerIPCDoCloseSocketClient(S32 handle);
S32 iPodPlayerIPCDoCloseQueueClient(S32 handle);
S32 iPodPlayerIPCDoSendSocket(S32 handle, U8 *message, U32 size, U32 flag, S32 timeout);
S32 iPodPlayerIPCDoSendQueue(S32 handle, U8 *message, U32 size, U32 flag, S32 timeout);


S32 iPodPlayerIPCOpenEpoll(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo);
S32 iPodPlayerIPCCloseEpoll(S32 handle);
S32 iPodPlayerIPCModEpoll(S32 waitHandle, U32 handleNum, S32 *handle, U32 flag);
S32 iPodPlayerIPCWaitEpoll(S32 waitHandle, U32 handleNum, U32 *resNum, S32 *resHandle, S32 *resEvent, S32 timeout);
S32 iPodPlayerIPCEventCheck(S32 waitHandle, U32 *handleNum, S32 *handle, S32 *resEvent, U8 *retType);
S32 iPodPlayerIPCCheckHandle(S32 waitHandle, IPOD_PLAYER_IPC_HANDLE_INFO *info);

S32 iPodPlayerIPCGetIPCfds(S32 num, S32 *fds, S32 handle);
S32 iPodPlayerIPCDelEventCheck(U32 *handleNum, S32 *handle);
S32 iPodPlayerIPCOpenSocketServer(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo);
S32 iPodPlayerIPCOpenQueueServer(S32 *handle, IPOD_PLAYER_IPC_OPEN_INFO *openInfo);
S32 iPodPlayerIPCDoAcceptRecv(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo);
S32 iPodPlayerIPCDoAcceptClose(IPOD_PLAYER_PARAM_SOCKET_INFO *socketInfo);
S32 iPodPlayerIPCDoCloseSocketServer(S32 handle);
S32 iPodPlayerIPCDoCloseQueueServer(S32 handle);
S32 iPodPlayerIPCDoRecvSocket(S32 fd, U8 *message, U32 size, S32 flag, S32 timeout);
S32 iPodPlayerIPCDoRecvQueue(S32 handle, U8 *message, U32 size, S32 flag, S32 timeout);
S32 iPodPlayerIPCDoLongSendSocket(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout);
S32 iPodPlayerIPCDoLongRecvSocket(S32 handle, U8 dataNum, U8 **message, U32 *size, U32 *asize, U32 flag, S32 timeout);

S32 iPodPlayerIPCDoGetNextSocketMsgSize(S32 fd, U32 *size, U32 flag, S32 timeout);


#endif /* IPOD_PLAYER_IPC_MESSAGE_H */
