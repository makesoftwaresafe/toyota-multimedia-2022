#include <linux/types.h>
#include <linux/pid.h>
#include <linux/sched.h>

struct task_struct* qnx_find_task(pid_t pid, _Int32t tid);

struct task_struct *pid_task(struct pid *pid, enum pid_type type)
{
	struct task_struct *result = NULL;

	if (type == PIDTYPE_PID) {
		result = qnx_find_task(pid->pid, pid->tid);
	} else {
		BUG();
	}

	return result;
}

struct task_struct *get_pid_task(struct pid *pid, enum pid_type type)
{
	struct task_struct *result;

	rcu_read_lock();

	result = pid_task(pid, type);
	if (result) {
		get_task_struct(result);
	}

	rcu_read_unlock();

	return result;
}

struct pid *get_task_pid(struct task_struct *task, enum pid_type type)
{
	if (type == PIDTYPE_PID) {
		return &task->spid;
	} else {
		BUG();
	}

	return NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/pid.c $ $Rev: 836935 $")
#endif
