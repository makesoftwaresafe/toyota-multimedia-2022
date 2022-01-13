/*
 * $QNXLicenseC:
 * Copyright 2013, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#ifndef _QNX_LINUX_SCHED_H
#define _QNX_LINUX_SCHED_H

#include <linux/capability.h>
#include <linux/pid.h>

#include <sys/iofunc.h>
#include <asm/processor.h>
#include <linux/rculist.h>
#include <linux/cred.h>

#include <sys/syspage.h>

#include <assert.h>
#include <linux/sched/task.h>	/* QNXNTO */

/*
 * Per process flags
 */
#define PF_EXITING	0x00000004	/* getting shut down */
#define PF_EXITPIDONE	0x00000008	/* pi exit done on shut down */
#define PF_VCPU		0x00000010	/* I'm a virtual CPU */
#define PF_WQ_WORKER	0x00000020	/* I'm a workqueue worker */
#define PF_FORKNOEXEC	0x00000040	/* forked but didn't exec */
#define PF_MCE_PROCESS  0x00000080      /* process policy on mce errors */
#define PF_SUPERPRIV	0x00000100	/* used super-user privileges */
#define PF_DUMPCORE	0x00000200	/* dumped core */
#define PF_SIGNALED	0x00000400	/* killed by a signal */
#define PF_MEMALLOC	0x00000800	/* Allocating memory */
#define PF_NPROC_EXCEEDED 0x00001000	/* set_user noticed that RLIMIT_NPROC was exceeded */
#define PF_USED_MATH	0x00002000	/* if unset the fpu must be initialized before use */
#define PF_USED_ASYNC	0x00004000	/* used async_schedule*(), used by module init */
#define PF_NOFREEZE	0x00008000	/* this thread should not be frozen */
#define PF_FROZEN	0x00010000	/* frozen for system suspend */
#define PF_FSTRANS	0x00020000	/* inside a filesystem transaction */
#define PF_KSWAPD	0x00040000	/* I am kswapd */
#define PF_MEMALLOC_NOIO 0x00080000	/* Allocating memory without IO involved */
#define PF_LESS_THROTTLE 0x00100000	/* Throttle me less: I clean memory */
#define PF_KTHREAD	0x00200000	/* I am a kernel thread */
#define PF_RANDOMIZE	0x00400000	/* randomize virtual address space */
#define PF_SWAPWRITE	0x00800000	/* Allowed to write to swap */
#define PF_NO_SETAFFINITY 0x04000000	/* Userland is not allowed to meddle with cpus_allowed */
#define PF_MCE_EARLY    0x08000000      /* Early kill for mce process policy */
#define PF_MUTEX_TESTER	0x20000000	/* Thread belongs to the rt mutex tester */
#define PF_FREEZER_SKIP	0x40000000	/* Freezer should not count it as freezable */
#define PF_SUSPEND_TASK 0x80000000      /* this thread called freeze_processes and should not be frozen */

/*
 * cloning flags:
 */
#define CSIGNAL		0x000000ff	/* signal mask to be sent at exit */
#define CLONE_VM	0x00000100	/* set if VM shared between processes */
#define CLONE_FS	0x00000200	/* set if fs info shared between processes */
#define CLONE_FILES	0x00000400	/* set if open files shared between processes */
#define CLONE_SIGHAND	0x00000800	/* set if signal handlers and blocked signals shared */
#define CLONE_PTRACE	0x00002000	/* set if we want to let tracing continue on the child too */
#define CLONE_VFORK	0x00004000	/* set if the parent wants the child to wake it up on mm_release */
#define CLONE_PARENT	0x00008000	/* set if we want to have the same parent as the cloner */
#define CLONE_THREAD	0x00010000	/* Same thread group? */
#define CLONE_NEWNS	0x00020000	/* New mount namespace group */
#define CLONE_SYSVSEM	0x00040000	/* share system V SEM_UNDO semantics */
#define CLONE_SETTLS	0x00080000	/* create a new TLS for the child */
#define CLONE_PARENT_SETTID	0x00100000	/* set the TID in the parent */
#define CLONE_CHILD_CLEARTID	0x00200000	/* clear the TID in the child */
#define CLONE_DETACHED		0x00400000	/* Unused, ignored */
#define CLONE_UNTRACED		0x00800000	/* set if the tracing process can't force CLONE_PTRACE on this clone */
#define CLONE_CHILD_SETTID	0x01000000	/* set the TID in the child */
/* 0x02000000 was previously the unused CLONE_STOPPED (Start in stopped state)
   and is now available for re-use. */
#define CLONE_NEWUTS		0x04000000	/* New utsname namespace */
#define CLONE_NEWIPC		0x08000000	/* New ipc namespace */
#define CLONE_NEWUSER		0x10000000	/* New user namespace */
#define CLONE_NEWPID		0x20000000	/* New pid namespace */
#define CLONE_NEWNET		0x40000000	/* New network namespace */
#define CLONE_IO		0x80000000	/* Clone io context */

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
#define TASK_TRACED		(TASK_WAKEKILL | __TASK_TRACED)

/* Task command name length: */
#define TASK_COMM_LEN 16

struct pid;
struct drm_file;

#define QNX_SCHED_SIGNALLED 0x00000001

struct task_struct {
	pid_t pid;
	struct pid spid;
	volatile long task_state;
	int usage_count;

	char comm[TASK_COMM_LEN];

	const struct cred __rcu *real_cred; /* Objective and real subjective task credentials (COW): */
	const struct cred __rcu *cred;      /* Effective (overridable) subjective task credentials (COW): */
	struct cred cred_vault;
	int prio;
	int flags;

	/* Linux kernel scheduler emulation under QNX per task */
	unsigned long sched_flags;
	pthread_mutex_t sched_mutex;
	pthread_cond_t sched_cond;
	bool signal;

	struct sched_param sched_param;
	int sched_policy;
	int sched_max_priority, sched_high_priority;
	int sched_requestor_priority;

	/*
	 * scheduling preemption disable control
	 */
	struct {
		int		count;
		int		qnx_irq_thread;
		pthread_t	tid;
		int		prio;

		/*
		 * debug info -- DEBUG_PREEMPT
		 */
		u64		start_time;
		int		owner;
	} preempt;

	/* For kernel threads (kthread) */
	unsigned long kthread_flags;
	int (*kthread_threadfn)(void *data);
	void* kthread_data;
	pthread_t kthread_tid;

	struct completion kthread_parked;
	struct task_struct **kthread_slot;

	struct {
		/* This tid is mostly used for debug purposes and never */
		/* in the i915/DRM code or in the linux emulation code. */
		pthread_t	user_tid;
		struct task_struct	*opened;
		bool		copy_to_user_memcpy;
		int	uaddr_cache_id;
	} attachment;
};

extern __thread struct task_struct *current;

#define __set_current_state(state_value)		\
	do { current->task_state = state_value; } while (0)
#define set_current_state(state_value)			\
	do { current->task_state = state_value; } while(0)

#ifdef __QNXNTO__
/*
 * preempt_count defined here, from asm/preempt.h
 */
static __always_inline int preempt_count(void)
{
	return current->preempt.count ? atomic_read(&__preempt_count) : 0;
}

#define hardirq_count()	(preempt_count() & HARDIRQ_MASK)
#define softirq_count()	(preempt_count() & SOFTIRQ_MASK)
#define irq_count()	(preempt_count() & (HARDIRQ_MASK | SOFTIRQ_MASK \
				 | NMI_MASK))

#define in_irq()		(hardirq_count())
#define in_softirq()		(softirq_count())
#define in_interrupt()		(irq_count())
#define in_serving_softirq()	(softirq_count() & SOFTIRQ_OFFSET)
#endif

static inline bool capable(int cap)
{
	if (cap == CAP_SYS_ADMIN) {
		return current->real_cred->euid.val == 0;
	}

	return 0;
}

static inline pid_t task_pid_nr(struct task_struct *tsk)
{
	return tsk->pid;
}

static inline struct pid* task_pid(struct task_struct *tsk)
{
	return &tsk->spid;
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
#define io_schedule_timeout schedule_timeout
signed long schedule_timeout_uninterruptible(signed long timeout);
void schedule(void);

static inline signed long
schedule_timeout_killable(signed long timeout)
{
	//__set_current_state(TASK_KILLABLE);
	return schedule_timeout(timeout);
}

struct task_struct;
static inline int signal_pending(struct task_struct *p)
{
	return p->signal;
}

#define __sched


#define	MAX_SCHEDULE_TIMEOUT	LONG_MAX

#define cond_resched sched_yield

static inline void io_schedule(void)
{
	schedule_timeout(MAX_SCHEDULE_TIMEOUT);
}

#define task_is_traced(task)		(0)
#define task_is_stopped(task)		(0)
#define task_is_stopped_or_traced(task)	(0)

static inline int __fatal_signal_pending(struct task_struct *p)
{
	return signal_pending(p);
}

static inline int fatal_signal_pending(struct task_struct *p)
{
	return signal_pending(p);
}

static inline void __sched yield(void)
{
	sched_yield();
}

static inline bool need_resched()
{
	return false;
}

extern int wake_up_process(struct task_struct *tsk);
extern int wake_up_state(struct task_struct *tsk, unsigned int state);

int send_sig(int sig, struct task_struct *p, int priv);
void block_all_signals(int (*notifier)(void *priv), void *priv, sigset_t *mask);
void unblock_all_signals(void);

static inline int signal_pending_state(long state, struct task_struct *p)
{
	(void)state;

	return signal_pending(p);
}

static inline unsigned long long sched_clock(void)
{
	return SYSPAGE_ENTRY(qtime)->nsec;
}

static inline struct pid *task_tgid(struct task_struct *task)
{
	return &task->spid;
}

static inline u64 local_clock(void)
{
	return SYSPAGE_ENTRY(qtime)->nsec;
}

static inline pid_t task_pid_vnr(struct task_struct *task)
{
	return task->pid;
}

int sched_setscheduler_nocheck(struct task_struct* task, int policy, const struct sched_param* param);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/sched.h $ $Rev: 858776 $")
#endif
