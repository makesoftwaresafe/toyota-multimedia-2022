#ifndef IPOD_PLAYER_LOCAL_H
#define IPOD_PLAYER_LOCAL_H

#include <adit_typedef.h>
#include "sys_time_adit.h"
#include "pthread_adit.h"
#include "iPodPlayerDef.h"
#include "iPodPlayerIPCMessage.h"
#include "iPodPlayerCB.h"
#include "iPodPlayerDef_in.h"
#include <errno.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

S32 iPodPlayerCallback(IPOD_PLAYER_FUNC_HEADER *rdata, U8 dataNum, U8 **data, U32 *size, IPOD_PLAYER_REGISTER_CB_TABLE *cbTable);

#endif
