
#include "iPodPlayerLocal.h"
#include "iPodPlayerIPCLib.h"
#include "iPodPlayerData.h"

static S32 g_ipodSendSckHandle = -1; /* sendSocket */
static S32 g_ipodRecvSckHandle = -1; /* recvSocket */
static S32 g_ipodLongRecvSckHandle = -1;
static S32 g_ipodLongSendSckHandle = -1; /* sendSocket */
static S32 g_ipodWaitHandle = -1;
static S32 g_ipodSendQueueHandle = -1; /* sendSocket */
static S32 g_ipodRecvQueueHandle = -1; /* recvSocket */
static S32 g_iPodwaitQueueHandle = -1;
static S32 g_ipodSendQueueLocalHandle = -1; /* sendLocalQueue */
static S32 g_ipodRecvQueueLocalHandle = -1; /* recvLocalQueue */


void iPodPlayerInitSocketInfo(void)
{
    g_ipodSendSckHandle = -1;
    g_ipodRecvSckHandle = -1;
    g_ipodLongRecvSckHandle = 1;
    g_ipodLongSendSckHandle = -1;
    g_ipodWaitHandle = -1;
    g_ipodSendQueueHandle = -1;
    g_ipodRecvQueueHandle = -1;
    g_ipodSendQueueLocalHandle = -1;
    g_ipodRecvQueueLocalHandle = -1;
    return;
}

S32 iPodPlayerSetSocketInfo(S32 socketInfo, U8 mode)
{
    S32 rc = IPOD_PLAYER_OK;
    
    switch(mode)
    {
    case IPOD_PLAYER_OPEN_SOCKET_SERVER:
        /* server side */
        g_ipodRecvSckHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_SOCKET_CLIENT:
        /* client side */
        g_ipodSendSckHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG:
        /* server side */
        g_ipodLongRecvSckHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG:
        /* client side */
        g_ipodLongSendSckHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_WAIT_HANDLE:
        g_ipodWaitHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_SERVER:
        g_ipodRecvQueueHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_CLIENT:
        g_ipodSendQueueHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_SERVER_LOCAL:
        g_ipodRecvQueueLocalHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_CLIENT_LOCAL:
        g_ipodSendQueueLocalHandle = socketInfo;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_WAIT:
        g_iPodwaitQueueHandle = socketInfo;
        break;
        
    default:
        /* Parameter invalid */
        rc = IPOD_PLAYER_ERROR;
        break;
    }
    
    return rc;
}

S32 iPodPlayerGetSocketInfo(S32 *socketInfo, U8 mode)
{
    S32 rc = IPOD_PLAYER_OK;
    
    switch(mode)
    {
    case IPOD_PLAYER_OPEN_SOCKET_SERVER:
        /* server side */
        *socketInfo = g_ipodRecvSckHandle;
        break;
        
    case IPOD_PLAYER_OPEN_SOCKET_CLIENT:
        /* client side */
        *socketInfo = g_ipodSendSckHandle;
        break;
        
    case IPOD_PLAYER_OPEN_SOCKET_SERVER_LONG:
        /* server side */
        *socketInfo = g_ipodLongRecvSckHandle;
        break;
        
    case IPOD_PLAYER_OPEN_SOCKET_CLIENT_LONG:
        /* client side */
        *socketInfo = g_ipodLongSendSckHandle;
        break;
        
    case IPOD_PLAYER_OPEN_WAIT_HANDLE:
        *socketInfo = g_ipodWaitHandle;
        break;
    
    case IPOD_PLAYER_OPEN_QUEUE_SERVER:
        *socketInfo = g_ipodRecvQueueHandle;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_CLIENT:
        *socketInfo = g_ipodSendQueueHandle;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_SERVER_LOCAL:
        *socketInfo = g_ipodRecvQueueLocalHandle;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_CLIENT_LOCAL:
        *socketInfo = g_ipodSendQueueLocalHandle;
        break;
        
    case IPOD_PLAYER_OPEN_QUEUE_WAIT:
        *socketInfo = g_iPodwaitQueueHandle;
        break;
        
    default:
        /* Parameter invalid */
        rc = IPOD_PLAYER_ERROR;
        break;
    }
    
    return rc;
}

