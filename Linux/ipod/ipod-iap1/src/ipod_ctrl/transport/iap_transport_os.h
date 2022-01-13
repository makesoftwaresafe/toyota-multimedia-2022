#ifndef IAP_TRANSPORT_OS_H
#define IAP_TRANSPORT_OS_H

#include <semaphore.h>

#ifdef __cplusplus
extern "C" {
#endif

#define IPOD_FLG_ONE_WAY 1
#define IPOD_FLG_TWO_WAY 2

#define IPOD_TASK_ID pthread_t
#define IPOD_SEM_ID sem_t *
#define IPOD_MSEC 1000
#define IPOD_NSEC 1000000
#define IPOD_SEM_DEFAULT 1
#define IPOD_MSG_MAX_DEFULT 10

typedef struct _IPOD_FLG_ID
{
    S32 readerSend;
    S32 readerWait;
} IPOD_FLG_ID;


#define IPOD_TIME_NSEC_1000000 1000000

void iPodOSSleep(U32 sleep_ms);
S32 iPodOSDeleteTask(IPOD_TASK_ID task_id);
void iPodOSExitTask(IPOD_TASK_ID task_id);
S32 iPodOSJoinTask(IPOD_TASK_ID task_id);
IPOD_TASK_ID iPodOSCreateTask(void* task_addr, const U8* task_name, S32 prio, S32 stack_size, S32 cpu_id, void* exinf);
S32 iPodOSVerifyTaskID(IPOD_TASK_ID task_id);
void iPodOSClearTaskID(IPOD_TASK_ID* task_id);
S32 iPodOSCreateSemaphore(const U8* semaphore_name, IPOD_SEM_ID *sem_Id);
S32 iPodOSDeleteSemaphore(IPOD_SEM_ID semaphore_id, const U8 *semaphore_name);
S32 iPodOSWaitSemaphore(IPOD_SEM_ID semaphore_id, S32 timeout_ms);
S32 iPodOSSignalSemaphore(IPOD_SEM_ID semaphore_id);
S32 iPodOSTestSemaphore(IPOD_SEM_ID semaphore_id);
S32 iPodOSCreateFlag(const U8* flag_name, IPOD_FLG_ID *flg_id, U8 direction);
S32 iPodOSDeleteFlag(IPOD_FLG_ID flag_id, const U8 *flag_name, U8 direction);
S32 iPodOSSetFlag(IPOD_FLG_ID flag_id, U32 bit_mask, U8 direction);
S32 iPodOSWaitFlag(IPOD_FLG_ID flag_id, U32 bit_mask, U32 options, U32* flg, S32 timeout_ms, U8 direction);
S32 iPodOSClrFlag(IPOD_FLG_ID flag_id, U32 clrptn);
S32 iPodOSGetTsp(IPOD_TASK_ID tskId, void* pk_tskspc);
S32 iPodOSSetTsp(IPOD_TASK_ID tskId, void* pk_tskspc);
S32 iPodOSGetRid(IPOD_TASK_ID tskId);
S32 iPodOSSetRid(IPOD_TASK_ID tskId, S32 resid);
void iPodOSCancelPoint();
#ifdef __cplusplus
}
#endif

#endif /* IAP_TRANSPORT_OS_H */
