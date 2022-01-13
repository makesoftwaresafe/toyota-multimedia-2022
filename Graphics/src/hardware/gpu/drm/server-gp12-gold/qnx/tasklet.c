#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <linux/qnx.h>

#include "i915_drv.h"	/* regacc_trace_simple() */

/* Tasklets --- multithreaded analogue of BHs.

   Main feature differing them of generic softirqs: tasklet
   is running only on one CPU simultaneously.

   Main feature differing them of BHs: different tasklets
   may be run simultaneously on different CPUs.

   Properties:
   * If tasklet_schedule() is called, then tasklet is guaranteed
     to be executed on some cpu at least once after this.
   * If the tasklet is already scheduled, but its execution is still not
     started, it will be executed only once.
   * If this tasklet is already running on another CPU (or schedule is called
     from tasklet itself), it is rescheduled for later.
   * Tasklet is strictly serialized wrt itself, but not
     wrt another tasklets. If client needs some intertask synchronization,
     he makes it with spinlocks.
 */

#define bitmask(m)	(1 << (m))

static void tasklet_work_func(struct work_struct *work)
{
	struct tasklet_struct *tasklet = container_of(work, struct tasklet_struct, tasklet_work);

	pthread_mutex_lock(&tasklet->lock);
	tasklet->state |= bitmask(TASKLET_STATE_RUN);
	pthread_mutex_unlock(&tasklet->lock);

	/*
	 * not only local_bh_disable, but also local_irq_disable
	 * because tasklet is not based on softirq archtecture
	 * in QNX implementation
	 */
	local_irq_disable();

	__local_bh_disable_ip((unsigned long)tasklet->func, SOFTIRQ_OFFSET);
	tasklet->func(tasklet->data);
	__local_bh_enable_ip((unsigned long)tasklet->func, SOFTIRQ_OFFSET);

	local_irq_enable();

	pthread_mutex_lock(&tasklet->lock);
	tasklet->state &=~ (bitmask(TASKLET_STATE_SCHED) +
			bitmask(TASKLET_STATE_RUN) );
	if (tasklet->waitcount) {
		pthread_cond_broadcast(&tasklet->cond);
	}
	pthread_mutex_unlock(&tasklet->lock);
}

static void tasklet_init_work(struct work_struct *work)
{
	struct task_struct *task = current;
	pthread_t	tid;
	int	ret;

	/*
	 * set tasklet workqueue thread priority
	 */
	tid = pthread_self();
	ret = pthread_setschedprio(tid, task->sched_max_priority);
	if (ret) {
		qnx_error("%s[%d] pthread_setschedprio(%d) = %d",
				__func__, tid,
				task->sched_max_priority, ret);
		return;
	}
	pthread_getschedparam(tid,
			&task->sched_policy,
			&task->sched_param); /* update current param */
}

void tasklet_init(struct tasklet_struct *t, void (*func)(unsigned long), unsigned long data)
{
	struct work_struct	init_work;

	/* Initialize tasklet structure */
	t->next = NULL;
	t->state = 0;
	t->count = 0;
	t->func = func;
	t->data = data;
	t->waitcount = 0;

	INIT_WORK(&t->tasklet_work, tasklet_work_func);
	pthread_mutex_init(&t->lock, NULL);
	pthread_cond_init(&t->cond, NULL);

	/*
	 * initialize tasklet workqueue thread
	 */
	INIT_WORK_ONSTACK(&init_work, tasklet_init_work);
	queue_work(tasklet_wq, &init_work);
	flush_work(&init_work);
	destroy_work_on_stack(&init_work);
}

void tasklet_unlock_wait(struct tasklet_struct *t)
{
	pthread_mutex_lock(&t->lock);
	while (t->state & (bitmask(TASKLET_STATE_SCHED) +
				bitmask(TASKLET_STATE_RUN)) ) {
		t->waitcount++;
		pthread_cond_wait(&t->cond, &t->lock);
		t->waitcount--;
	}
	pthread_mutex_unlock(&t->lock);
}

void tasklet_kill(struct tasklet_struct *t)
{
	/*
	 * not kill, but wait current scheduled tasklet
	 */
	tasklet_unlock_wait(t);
}

void __tasklet_schedule(struct tasklet_struct *t)
{
	int	disabled;

	pthread_mutex_lock(&t->lock);
	disabled = t->count;
	t->state |= bitmask(disabled ? TASKLET_STATE_PEND :
			TASKLET_STATE_SCHED);
	pthread_mutex_unlock(&t->lock);

	if (!disabled) {
		queue_work(tasklet_wq, &t->tasklet_work);
	}
}

void __tasklet_hi_schedule(struct tasklet_struct *t)
{
	regacc_trace_simple(_RET_IP_);
	__tasklet_schedule(t);
}

void tasklet_disable_nosync(struct tasklet_struct *t)
{
	pthread_mutex_lock(&t->lock);
	t->count++;
	pthread_mutex_unlock(&t->lock);
}

void tasklet_disable(struct tasklet_struct *t)
{
	tasklet_disable_nosync(t);
	tasklet_unlock_wait(t);
}

void tasklet_enable(struct tasklet_struct *t)
{
	int	doSched = 0;

	pthread_mutex_lock(&t->lock);
	if (!--t->count) {
		doSched = t->state & bitmask(TASKLET_STATE_PEND);
		if (doSched) {
			t->state ^= (bitmask(TASKLET_STATE_PEND) +
				bitmask(TASKLET_STATE_SCHED));
		}
	}
	pthread_mutex_unlock(&t->lock);
	if (doSched) {
		queue_work(tasklet_wq, &t->tasklet_work);
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/dev-drm/hardware/gpu/drm/server-gp12-gold/qnx/tasklet.c $ $Rev: 836935 $")
#endif
