/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/sched.h.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _LINUX_SCHED_H
#define _LINUX_SCHED_H

#include <linux/sched/prio.h>

#include <linux/capability.h>
#include <linux/pid.h>
#ifndef __QNXNTO__
#include <linux/mm_types.h>
#endif

#include <sys/iofunc.h>
#include <asm/processor.h>

#include <assert.h>

/*
 * Task state bitmask. NOTE! These bits are also
 * encoded in fs/proc/array.c: get_task_state().
 *
 * We have two separate sets of flags: task->state
 * is about runnability, while task->exit_state are
 * about the task exiting. Confusing, but this way
 * modifying one set can't modify the other one by
 * mistake.
 */
#define TASK_RUNNING 0
#define TASK_INTERRUPTIBLE 1
#define TASK_UNINTERRUPTIBLE 2
#define __TASK_STOPPED		4
#define __TASK_TRACED		8
#define TASK_DEAD		64
#define TASK_WAKEKILL		128
#define TASK_WAKING		256
#define TASK_PARKED		512
#define TASK_NOLOAD		1024
#define TASK_STATE_MAX		2048

#define TASK_KILLABLE		(TASK_WAKEKILL | TASK_UNINTERRUPTIBLE)
#define TASK_STOPPED		(TASK_WAKEKILL | __TASK_STOPPED)

/* Convenience macros for the sake of wake_up */ //TR
#define TASK_NORMAL		(TASK_INTERRUPTIBLE | TASK_UNINTERRUPTIBLE)

#define TASK_TRACED		(TASK_WAKEKILL | __TASK_TRACED)

#define TASK_COMM_LEN 16

struct pid;
struct _resmgr_context;
struct drm_file;
struct task_struct {
	pid_t pid;
	volatile long task_state;  /*TODO. FIXME. rename task_state to state */
	struct _resmgr_context *resmgr_context;
	// flags
	unsigned copy_to_user_memcpy : 1;

	char comm[TASK_COMM_LEN]; 
	struct drm_file * file;

	struct mm_struct *mm, *active_mm;

};

extern __thread struct task_struct *current;

#define __set_current_state(VAL) do{ \
	current->task_state = (VAL); }while(0)
#define set_current_state(VAL) \
	set_mb(current->task_state, (VAL))

static inline uid_t
current_euid(void){
	struct _client_info cinfo;
	int err = ConnectClientInfo_r(current->resmgr_context->info.scoid,
			&cinfo, NGROUPS_MAX);
	return err ? (uid_t)-1 : cinfo.cred.euid;
}

static inline bool
capable(int cap){
	// Treat root (euid 0) as the only user with any capabilities.
	(void)cap;
	return current_euid() == 0;
}

static inline pid_t
task_pid_nr(struct task_struct *tsk)
{
	return tsk->resmgr_context->info.pid;
}

static inline struct pid*
task_pid(struct task_struct *tsk)
{
	return (struct pid*)&tsk->resmgr_context->info.pid;
}


/**
  * schedule_timeout - sleep until timeout
  * @timeout: timeout value in jiffies
  *
  * Make the current task sleep until @timeout jiffies have
  * elapsed. The routine will return immediately unless
  * the current task state has been set (see set_current_state()).
  *
  * You can set the task state as follows -
  *
  * %TASK_UNINTERRUPTIBLE - at least @timeout jiffies are guaranteed to
  * pass before the routine returns. The routine will return 0
  *
  * %TASK_INTERRUPTIBLE - the routine may return early if a signal is
  * delivered to the current task. In this case the remaining time
  * in jiffies will be returned, or 0 if the timer expired in time
  *
  * The current task state is guaranteed to be TASK_RUNNING when this
  * routine returns.
  *
  * Specifying a @timeout value of %MAX_SCHEDULE_TIMEOUT will schedule
  * the CPU away without a bound on the timeout. In this case the return
  * value will be %MAX_SCHEDULE_TIMEOUT.
  *
  * In all cases the return value is guaranteed to be non-negative.
  */
signed long schedule_timeout(signed long timeout);
signed long schedule_timeout_uninterruptible(signed long timeout);
void schedule(void);

struct task_struct;
int signal_pending(struct task_struct *p);

#ifndef __QNXNTO__
#define __sched		__attribute__((__section__(".sched.text")))
#else
#define __sched
#endif

#define	MAX_SCHEDULE_TIMEOUT	LONG_MAX

#define cond_resched sched_yield

static inline void io_schedule(void)
{
	sched_yield();
}


#define task_is_traced(task)	((task->task_state & __TASK_TRACED) != 0)
#define task_is_stopped(task)	((task->task_state & __TASK_STOPPED) != 0)
#define task_is_stopped_or_traced(task)	\
			((task->task_state & (__TASK_STOPPED | __TASK_TRACED)) != 0)


static inline int __fatal_signal_pending(struct task_struct *p)
{
#ifdef __QNXNTO__
	return 0;
#else
	unlikely(sigismember(&p->pending.signal, SIGKILL));
#endif
}

static inline int fatal_signal_pending(struct task_struct *p)
{
	return signal_pending(p) && __fatal_signal_pending(p);
}


static inline void __sched yield(void)
{
	sched_yield();
}

static __always_inline bool need_resched(void)
{
#ifndef __QNXNTO__
	return unlikely(tif_need_resched());
#else
	return false;
#endif
}

static inline struct task_struct *pid_task(struct pid *pid, enum pid_type type)
{
	assert(0 && "pid_task no imp yet!");
	return NULL;
}

extern int wake_up_process(struct task_struct *tsk);

int send_sig(int sig, struct task_struct *p, int priv);
int signal_pending(struct task_struct *p);
void block_all_signals(int (*notifier)(void *priv), void *priv, sigset_t *mask);
void unblock_all_signals(void);

static inline int signal_pending_state(long state, struct task_struct *p)
{
	if (!(state & (TASK_INTERRUPTIBLE | TASK_WAKEKILL)))
		return 0;
	if (!signal_pending(p))
		return 0;

	return (state & TASK_INTERRUPTIBLE) || __fatal_signal_pending(p);
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/sched.h $ $Rev: 838597 $")
#endif
