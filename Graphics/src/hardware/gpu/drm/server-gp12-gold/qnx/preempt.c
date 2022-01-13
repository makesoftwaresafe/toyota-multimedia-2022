/*
 * preempt control
 * copyright DENSO corporation
 */
#include <linux/interrupt.h>
#include <linux/sched.h>
#include <linux/qnx.h>

#include <asm/preempt.h>

atomic_t __preempt_count;
atomic_t __irq_disabled;
int	preempt_owner, preempt_owner_irq;
struct task_struct *preempt_owner_task;
static pthread_mutex_t	preempt_mutex	= PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t	preempt_cond	= PTHREAD_COND_INITIALIZER;

atomic_t __preempt_debug_disable;

// #define DEBUG_PREEMPT
#define DEBUG_PREEMPT2	/* light debug */

#ifdef  DEBUG_PREEMPT
#define PREEMPT_TRACE_SIZE	32
static struct preempt_trace {
	int	caller, this, count, tid;
} preempt_trace[PREEMPT_TRACE_SIZE], preempt_trace_irq[PREEMPT_TRACE_SIZE];
static int	preempt_trace_pos, preempt_trace_pos_irq;

/*
 * The various preempt_count add/sub methods
 */

static void preempt_trace_func(int caller, int this)
{
	int	i, pos, tid, count, *ppos;
	static pthread_mutex_t	preempt_trace_mutex =
		PTHREAD_MUTEX_INITIALIZER;
	struct preempt_trace	*tp;

	if (current->preempt.qnx_irq_thread) {
		tp = preempt_trace_irq;
		ppos = &preempt_trace_pos_irq;
	} else {
		tp = preempt_trace;
		ppos = &preempt_trace_pos;
	}
	/*
	 * trace preempt
	 */
	tid = pthread_self();
	count = atomic_read(&__preempt_count);

	pthread_mutex_lock(&preempt_trace_mutex);
	pos = *ppos;
	for (i = 1; i < PREEMPT_TRACE_SIZE; i++) {
		int	temppos;

		temppos = (pos - i + PREEMPT_TRACE_SIZE) &
			(PREEMPT_TRACE_SIZE - 1);
		if (!tp[temppos].caller) {
			/*
			 * no trace here
			 */
			break;
		}
		if (tp[temppos].count == count) {
			/*
			 * found same count, now remove previous
			 */
			if (tp[temppos].caller == caller &&
				tp[temppos].tid == tid) {
				pos = temppos;
			}
			break;
		}
	}
	tp[pos].count = count;
	tp[pos].caller = caller;
	tp[pos].this = this;
	tp[pos].tid = tid;
	*ppos = (pos + 1) &
		(PREEMPT_TRACE_SIZE - 1);
	pthread_mutex_unlock(&preempt_trace_mutex);
}
#else
#define preempt_trace_func(caller, this)
#endif

void preempt_dummy_prevent_optimize()
{
	if (!caller_main_offset()) {
#ifdef DEBUG_PREEMPT
		printk(KERN_INFO "dummy, to avoid compiler optimize "
				"%d\n", preempt_trace[0].caller);
		printk(KERN_INFO "dummy, to avoid compiler optimize "
				"%d\n", preempt_trace_irq[0].caller);
#endif
		printk(KERN_INFO "dummy, to avoid compiler optimize "
				"owner=%#x task:%s\n",
				preempt_owner,
				preempt_owner_task->comm);
	}
}

static inline void set_preempt_disable_prio(struct task_struct *task)
{
	pthread_t	tid;
	int		ret;

	WARN(task->preempt.count,
			"task->preempt.count=%#x count=%#x owner=%#x\n",
			task->preempt.count,
			atomic_read(&__preempt_count),
			task->preempt.owner);

	tid = pthread_self();
	task->preempt.tid = tid;
	task->preempt.prio = -1;

	if (task->sched_param.sched_priority ==
			task->sched_max_priority) {
		return;
	}

	ret = pthread_setschedprio(tid, task->sched_max_priority);
	if (!ret) {
		task->preempt.prio = task->sched_param.sched_priority;
	}
	WARN(ret, "pthread_setschedprio(%d)=%d",
			task->sched_max_priority, ret);
}

static inline int do_preempt_count_add(int val,
		struct task_struct *task, unsigned long ip)
{
	int	ret;

	BUG_ON(!val);

	ret = atomic_add_return(val, &__preempt_count);
	task->preempt.count += val;

	if (task->preempt.count == val) do {
		/*
		 * initial preempt disabled
		 */
#ifdef DEBUG_PREEMPT2
		task->preempt.start_time = local_clock();
#endif
		task->preempt.owner = main_offset(ip);
	} while(0);
	if (!task->preempt.count) {
#ifdef DEBUG_PREEMPT2
#define MAX_INTV	(5 * NSEC_PER_MSEC)
		static u64	max_intv;
		unsigned long long	intv;

		intv = local_clock() - task->preempt.start_time;
		if (!atomic_read(&__preempt_debug_disable) &&
				intv > max_intv) {
			max_intv =  intv <= MAX_INTV ? intv : MAX_INTV;
			printk(KERN_INFO "%s %s[%d] preempt enabled, "
				"owner=%#x intv=%lld\n",
				__func__, task->comm, task->preempt.tid,
				task->preempt.owner,
				intv);
		}
#endif
		/*
		 * final preempt enabled
		 */
		if (task->preempt.prio >= 0) {
			int	ret;
			ret = pthread_setschedprio(task->preempt.tid,
				task->preempt.prio);
			WARN(ret, "prio=%d", task->preempt.prio);
		}
	}
	return ret;
}

void __preempt_count_add_ip(int val, unsigned long ip)
{
	struct task_struct	*task = current;

	BUG_ON(!task || val <= 0);	/* val must positive value */
	if (task->preempt.count) {
		/*
		 * already preempt owner
		 */
		do_preempt_count_add(val, task, ip);
		preempt_trace_func(main_offset(ip),
				main_offset(_THIS_IP_));

		return;
	}

	/*
	 * initial preempt, at least this task
	 * prepare preempt_disable before mutex lock
	 */
	set_preempt_disable_prio(task);

	pthread_mutex_lock(&preempt_mutex);
	for(;;) {
		if (task->preempt.qnx_irq_thread && !irqs_disabled()) {
			/*
			 * IRQ can interrupt current preempt-owner task
			 */
			break;
		}
		if (!atomic_read(&__preempt_count)) {
			/*
			 * wait for preempt enabled
			 */
			break;
		}
		pthread_cond_wait(&preempt_cond, &preempt_mutex);
	}
	do_preempt_count_add(val, task, ip);
	pthread_mutex_unlock(&preempt_mutex);

	if (current->preempt.qnx_irq_thread) {
		preempt_owner_irq = main_offset(ip);
	} else {
		preempt_owner = main_offset(ip);
		preempt_owner_task = task;
	}
	preempt_trace_func(caller_main_offset(),
			main_offset(_THIS_IP_));
}

void __preempt_count_sub_ip(int val, unsigned long ip)
{
	if (!do_preempt_count_add(-val, current, ip)) {
		pthread_cond_broadcast(&preempt_cond);
	}
	preempt_trace_func(main_offset(ip),
			main_offset(_THIS_IP_));
}
