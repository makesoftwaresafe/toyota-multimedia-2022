/*
 *  kernel/sched/core.c
 *
 *  Core kernel scheduler code and related syscalls
 *
 */
#include <linux/sched.h>
#include <linux/sched/clock.h>
#include <uapi/linux/sched/types.h>
#include <linux/context_tracking.h>

#include <linux/mmu_context.h>
#include <linux/module.h>
#include <linux/prefetch.h>
#include <linux/profile.h>
#include <linux/hash.h>
#include <linux/qnx.h>

#define WAIT_TABLE_BITS 8
#define WAIT_TABLE_SIZE (1 << WAIT_TABLE_BITS)
static wait_queue_head_t bit_wait_table[WAIT_TABLE_SIZE] __cacheline_aligned;

wait_queue_head_t *bit_waitqueue(void *word, int bit)
{
	const int shift = BITS_PER_LONG == 32 ? 5 : 6;
	unsigned long val = (unsigned long)word << shift | bit;

	return bit_wait_table + hash_long(val, WAIT_TABLE_BITS);
}
EXPORT_SYMBOL(bit_waitqueue);

void __init sched_init(void)
{
	int i;

	for (i = 0; i < WAIT_TABLE_SIZE; i++)
		init_waitqueue_head(bit_wait_table + i);
}

int sched_setscheduler_nocheck(struct task_struct* task, int policy, const struct sched_param* param)
{
	struct sched_param prm;
	int rc;
	pthread_t	tid = pthread_self();

	if (param->sched_priority != 1) {
		WARN(1, "param.sched_priority is not realtime. Not implemented yet.");
		return -EINVAL;
	}

	if (task != current) {
		WARN(1, "Cannot set RT priority for non-current task. Not implemented yet.");
		return -ENOTSUP;
	}

	rc = pthread_setschedprio(tid, task->sched_high_priority);
	if (rc != EOK) {
		qnx_error("Can't set kthread priority\n");
	}
	pthread_getschedparam(tid, &task->sched_policy, &task->sched_param);
	task->prio = task->sched_param.sched_priority;

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/sched_core.c $ $Rev: 848997 $")
#endif
