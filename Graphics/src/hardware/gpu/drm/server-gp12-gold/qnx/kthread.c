#include <linux/kthread.h>
#include <linux/qnx.h>
#include <linux/slab.h>

#include <linux/vmalloc.h>

int qnx_create_task_sched(struct task_struct* task);
void qnx_destroy_task_sched(struct task_struct* task);

#define KTHREADS_IN_QNX_MAX 16

static pthread_mutex_t kmutex = PTHREAD_MUTEX_INITIALIZER;

enum KTHREAD_BITS {
	KTHREAD_IS_PER_CPU = 1,
	KTHREAD_SHOULD_STOP = 2,
	KTHREAD_SHOULD_PARK = 4,
	KTHREAD_IS_PARKED = 8,
};

struct task_struct* kthreads[KTHREADS_IN_QNX_MAX] = {NULL, };

/* The main purpose of this function is to lock mutex and then unlock to avoid race conditions. */
/* Typical example is pthread_create() - it stores tid of created thread much later, allowing   */
/* new thread to execute if new thread priority is above normal. So mutex acquiring is required */
/* to slow down execution till task structure will not be fully initialized.                    */
void* kthread_thread(void *arg)
{
	struct task_struct* task = (struct task_struct*)arg;
	int rc;

//#define DEBUG_PARK
#ifdef DEBUG_PARK
	delay(100);
#endif
	current = task;
	task->attachment.user_tid = pthread_self();
	task->attachment.copy_to_user_memcpy = 1;

	if (qnx_create_task_sched(task)) {
		qnx_error("%s[%d] %s qnx_create_task_sched error",
				__func__, pthread_self(),
				task->comm);
		return NULL;
	}

	kthread_parkme();
	task->kthread_threadfn(task->kthread_data);

	current = NULL;

	return NULL;
}

__printf(4, 5) struct task_struct *kthread_create_on_node(int (*threadfn)(void *data),
	void *data, int node, const char namefmt[], ...)
{
	pthread_attr_t	task_attr;
	struct task_struct *task;
	char name[128];
	int rc = 0;
	va_list args;
	int slot;

	if (node != -1) {
		BUG();
	}

	va_start(args, namefmt);
	kvsnprintf(name, sizeof(name) - 1, namefmt, args);
	va_end(args);

	task = kzalloc(sizeof(*task), GFP_KERNEL);
	if (task == NULL) {
		qnx_error("%s - ENOMEM", __func__);
		return NULL;
	}

	if (qnx_taskattr_init(&task_attr, QNX_PRTY_KERNEL)) {
		goto err_task;
	}

	/* Initialize kernel thread task structue. It is detached from userspace. */
	/* Any userspace access will produce SIGSEGV (kernel panic in Linux).     */
	strncpy(task->comm, name, sizeof(task->comm) - 1);
	task->pid = -1;
	task->spid.pid = -1;
	task->real_cred = &task->cred_vault;
	task->cred = &task->cred_vault;
	task->kthread_flags = 0;
	task->kthread_threadfn = threadfn;
	task->kthread_data = data;

	init_completion(&task->kthread_parked);

	rc = pthread_mutex_lock(&kmutex);
	if (rc){
		pr_err("%s(): pthread_mutex_lock() failed with code %d\n", __FUNCTION__, rc);
		goto err_task_attr;
	}

	for (slot = 0; slot < KTHREADS_IN_QNX_MAX; slot++) {
		if (kthreads[slot] == NULL) {
			kthreads[slot] = task;
			task->kthread_slot = &kthreads[slot];
			break;
		}
	}
	if (slot == KTHREADS_IN_QNX_MAX) {
		pr_err("%s(): Can't create kthread. Maximum amount of kernel threads was exceeded\n", __FUNCTION__);
		goto err_mutex_lock;
	}

	rc = pthread_create(&task->kthread_tid, &task_attr, kthread_thread, (void*)task);
	if (rc != 0) {
		pr_err("%s(): Can't create kthread, rc=%d\n", __FUNCTION__, rc);
		goto err_slot;
	}

	/* Set kthread name */
	pthread_setname_np(task->kthread_tid, name);

	pthread_mutex_unlock(&kmutex);

#ifdef DEBUG_PARK
	do {
		static int debug = 1;
		int i;

		if (!debug) break;
		debug--;
		printk(KERN_INFO "%s:%s(%d) start DEBUG PARK %#x\n",
				__func__, current->comm, pthread_self(),
				preempt_count());
		for (i = 0; preempt_count(); i++) {
			preempt_enable();
		}
		kthread_park(task);
		kthread_unpark(task);
		for (; i; i--) {
			preempt_disable();
		}
		printk(KERN_INFO "%s:%s(%d) finish DEBUG PARK %#x\n",
				__func__, current->comm, pthread_self(),
				preempt_count());
	} while(0);
#endif
	return task;

err_slot:
	kthreads[slot] = NULL;
err_mutex_lock:
	pthread_mutex_unlock(&kmutex);
err_task_attr:
	pthread_attr_destroy(&task_attr);
err_task:
	kfree(task);
	return NULL;
}

bool kthread_should_stop(void)
{
	return test_bit(KTHREAD_SHOULD_STOP, &current->kthread_flags);
}

/**
 * kthread_should_park - should this kthread park now?
 *
 * When someone calls kthread_park() on your kthread, it will be woken
 * and this will return true.  You should then do the necessary
 * cleanup and call kthread_parkme()
 *
 * Similar to kthread_should_stop(), but this keeps the thread alive
 * and in a park position. kthread_unpark() "restarts" the thread and
 * calls the thread function again.
 */
bool kthread_should_park(void)
{
	return test_bit(KTHREAD_SHOULD_PARK, &current->kthread_flags);
}

void kthread_unpark(struct task_struct *k)
{
	if (!k->kthread_slot)
		return;

	clear_bit(KTHREAD_SHOULD_PARK, &k->kthread_flags);
	if (test_and_clear_bit(KTHREAD_IS_PARKED, &k->kthread_flags)) {
		wake_up_state(k, TASK_PARKED);
	}
#ifdef DEBUG_PARK
	printk(KERN_INFO "%s %s(%d)\n", __func__, k->comm, pthread_self());
#endif
}

/**
 * kthread_park - park a thread created by kthread_create().
 * @k: thread created by kthread_create().
 *
 * Sets kthread_should_park() for @k to return true, wakes it, and
 * waits for it to return. This can also be called after kthread_create()
 * instead of calling wake_up_process(): the thread will park without
 * calling threadfn().
 *
 * Returns 0 if the thread is parked, -ENOSYS if the thread exited.
 * If called by the kthread itself just the park bit is set.
 */
int kthread_park(struct task_struct *k)
{
	if (!k->kthread_slot)
		return -ENOSYS;
	if (!test_bit(KTHREAD_IS_PARKED, &k->kthread_flags)) {
		set_bit(KTHREAD_SHOULD_PARK, &k->kthread_flags);
		if (k != current) {
			wake_up_process(k);
#ifdef DEBUG_PARK
			printk(KERN_INFO
				"%s:%s(%d) wait task=%s parked\n",
				__func__, current->comm, pthread_self(),
				k->comm);
#endif
			wait_for_completion(&k->kthread_parked);
#ifdef DEBUG_PARK
			printk(KERN_INFO
				"%s:%s(%d) wait park complete task=%s\n",
				__func__, current->comm, pthread_self(),
				k->comm);
#endif
		}
	}
	return 0;
}

void kthread_parkme(void)
{
	struct task_struct *self = current;

#ifdef DEBUG_PARK
	printk(KERN_INFO "%s %s(%d) start flags=%#lx\n",
			__func__, self->comm, pthread_self(),
			self->kthread_flags);
#endif
	__set_current_state(TASK_PARKED);
	while (test_bit(KTHREAD_SHOULD_PARK, &self->kthread_flags)) {
		if (!test_and_set_bit(KTHREAD_IS_PARKED,
					&self->kthread_flags)) {
#ifdef DEBUG_PARK
			printk(KERN_INFO "%s %s(%d) parked flags=%#lx\n",
				__func__, self->comm, pthread_self(),
				self->kthread_flags);
#endif
			complete(&self->kthread_parked);
		}
		schedule();
		__set_current_state(TASK_PARKED);
	}
	clear_bit(KTHREAD_IS_PARKED, &self->kthread_flags);
	__set_current_state(TASK_RUNNING);
#ifdef DEBUG_PARK
	printk(KERN_INFO "%s %s(%d) finish, running\n",
			__func__, self->comm, pthread_self());
#endif
}

int kthread_stop(struct task_struct *k)
{
	int rc;

	rc = pthread_mutex_lock(&kmutex);
	if (!k->kthread_slot || *k->kthread_slot != k) {
		pr_err("%s(): Can't find kthread %p tid %d:%s\n",
				__FUNCTION__, k,
				k ? k->kthread_tid : 0,
				k ? k->comm : 0);
		rc = -1;
	} else {
		*k->kthread_slot = NULL;
	}
	pthread_mutex_unlock(&kmutex);

	set_bit(KTHREAD_SHOULD_STOP, &k->kthread_flags);
	kthread_unpark(k);
	wake_up_process(k);

	pthread_join(k->kthread_tid, NULL);
	qnx_destroy_task_sched(k);
	kfree(k);

	return 0;
}

/*
 * QNX comment
 * This function is similar to kthread_should_stop() but do not report errors, only true or false
 *
 * DN comment
 * is_kthread_should_stop : based on original code
 *    current thread is PF_KTHREAD && kthread_should_stop
 */
bool is_kthread_should_stop()
{
	return current->kthread_slot && kthread_should_stop();
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/kthread.c $ $Rev: 856018 $")
#endif
