#include <linux/qnx.h>
#include <linux/linux.h>
#include <drm/drmP.h>

/*
 * System-wide workqueues which are always present.
 *
 * system_wq is the one used by schedule[_delayed]_work[_on]().
 * Multi-CPU multi-threaded.  There are users which expect relatively
 * short queue flush time.  Don't queue works which can run for too
 * long.
 *
 * system_long_wq is similar to system_wq but may host long running
 * works.  Queue flushing might take relatively long.
 *
 * system_nrt_wq is non-reentrant and guarantees that any given work
 * item is never executed in parallel by multiple CPUs.  Queue
 * flushing might take relatively long.
 *
 * system_unbound_wq is unbound workqueue.  Workers are not bound to
 * any specific CPU, not concurrency managed, and all queued works are
 * executed immediately as long as max_active limit is not reached and
 * resources are available.
 *
 * system_freezable_wq is equivalent to system_wq except that it's
 * freezable.
 *
 * system_nrt_freezable_wq is equivalent to system_nrt_wq except that
 * it's freezable.
 */

struct workqueue_struct *system_wq = NULL;
struct workqueue_struct *system_highpri_wq = NULL;
struct workqueue_struct *system_unbound_wq = NULL;
struct workqueue_struct *system_long_wq = NULL;
/* This one is currently unimplemented because it is unused by i915/DRM currently */
struct workqueue_struct *system_nrt_wq = NULL; 
struct workqueue_struct *tasklet_wq = NULL;

static void *workqueue_thread(void *arg);

int init_wq_system()
{
	system_wq = create_singlethread_workqueue("system wq");
	if (system_wq == NULL) {
		qnx_error("Creating system wq failed\n");
		return 0;
	}

	system_highpri_wq = create_singlethread_workqueue("system highpri wq");
	if (system_highpri_wq == NULL) {
		qnx_error("Creating system highpri wq failed\n");
		destroy_workqueue(system_wq);
		system_wq = NULL;
		return 0;
	}

	system_unbound_wq = create_singlethread_workqueue("system unbound wq");
	if (system_unbound_wq == NULL) {
		qnx_error("Creating system unbound wq failed\n");
		destroy_workqueue(system_highpri_wq);
		destroy_workqueue(system_wq);
		system_highpri_wq = NULL;
		system_wq = NULL;
		return 0;
	}

	system_long_wq = create_singlethread_workqueue("system long wq");
	if (system_long_wq == NULL) {
		qnx_error("Creating system long wq failed\n");
		destroy_workqueue(system_unbound_wq);
		destroy_workqueue(system_highpri_wq);
		destroy_workqueue(system_wq);
		system_unbound_wq = NULL;
		system_highpri_wq = NULL;
		system_wq = NULL;
		return 0;
	}

	tasklet_wq = create_singlethread_workqueue("tasklet wq");
	if (tasklet_wq == NULL) {
		qnx_error("Creating tasklet wq failed\n");
		destroy_workqueue(system_long_wq);
		destroy_workqueue(system_unbound_wq);
		destroy_workqueue(system_highpri_wq);
		destroy_workqueue(system_wq);
		system_long_wq = NULL;
		system_unbound_wq = NULL;
		system_highpri_wq = NULL;
		system_wq = NULL;
		return 0;
	}

	return 1;
}

void destroy_wq_system()
{
	if (tasklet_wq) {
		destroy_workqueue(tasklet_wq);
		tasklet_wq = NULL;
	}
	if (system_long_wq) {
		destroy_workqueue(system_long_wq);
		system_long_wq = NULL;
	}
	if (system_unbound_wq) {
		destroy_workqueue(system_unbound_wq);
		system_unbound_wq = NULL;
	}
	if (system_highpri_wq) {
		destroy_workqueue(system_highpri_wq);
		system_highpri_wq = NULL;
	}
	if (system_wq) {
		destroy_workqueue(system_wq);
		system_wq = NULL;
	}
}

#define QNX_WORKQUEUE_SCHED_PARAM
workqueue_struct_t *init_workqueue(const char *name, int threads)
{
	int status;
	pthread_condattr_t cond_attr;
	workqueue_struct_t *wq = NULL;
	pthread_attr_t task_attr;

	wq = (workqueue_struct_t*)kzalloc(sizeof(workqueue_struct_t), GFP_KERNEL);
	if (wq == NULL){
		qnx_error("kzalloc failed.");
		return NULL;
	}

	status = qnx_taskattr_init(&task_attr,
#ifdef QNX_WORKQUEUE_SCHED_PARAM
			QNX_PRTY_KERNEL
#else
			QNX_PRTY_DEFAULT
#endif
			);
	if (status != 0)
		goto free_wq;

	status = pthread_mutex_init(&wq->mutex, NULL);
	if (status != 0) {
		goto free_attr;
	}
	status = pthread_condattr_init(&cond_attr);
	if (status != 0) {
		goto free_mutex;
	}
	status = pthread_condattr_setclock(&cond_attr, CLOCK_MONOTONIC);
	if(status != 0){
		goto free_cv_attr;
	}
	status = pthread_cond_init(&wq->cv, &cond_attr);
	if (status != 0) {
		goto free_cv_attr;
	}

	wq->quit = 0;
	wq->first = NULL;
	wq->last = NULL;
	strncpy(wq->name, name, sizeof(wq->name) - 1);
	wq->valid = WORKQ_VALID;

	if (0 != pthread_create(&wq->id, &task_attr, workqueue_thread, (void*)wq)) {
		goto free_cv;
	}

	pthread_condattr_destroy(&cond_attr);
	return wq;
 free_cv:
	pthread_cond_destroy(&wq->cv);
 free_cv_attr:
	pthread_condattr_destroy(&cond_attr);
 free_mutex:
	pthread_mutex_destroy (&wq->mutex);
 free_attr:
	pthread_attr_destroy(&task_attr);
 free_wq:
	kfree(wq);
	return NULL;
}

void destroy_workqueue(workqueue_struct_t *wq)
{
	/* MG_TODO: check if it is possible to destroy workqueue with enqueued tasks */

	assert (wq->valid == WORKQ_VALID);
	/* notify thread to quit */
	pthread_mutex_lock(&wq->mutex);
	wq->quit = 1;
	pthread_cond_broadcast(&wq->cv);
	pthread_mutex_unlock(&wq->mutex);

	pthread_join(wq->id, NULL);

	/* destroy wq */
	pthread_mutex_destroy(&wq->mutex);
	pthread_cond_destroy(&wq->cv);
	kfree(wq);

	return;
}

static bool push_work(workqueue_struct_t *wq, work_struct_t *item)
{
	assert(wq->valid == WORKQ_VALID);
	work_struct_t* w = wq->first;

	/* skip add it if item is already in queue */
	while(w) {
		if (w == item) {
			return false;
		}
		w = w->next;
	}

	/* not in queue. add it */
	item->next = 0;
	if (wq->first == NULL){
		assert(wq->last == NULL);
		wq->first = item;
		wq->last = item;
	} else {
		assert(wq->last && wq->last->next == 0);
		wq->last->next = item;
		wq->last = item;
	}
	assert(wq->first && wq->last && wq->last->next == 0);
	assert((wq->first && wq->last) || (!wq->first && !wq->last));

	return true;
}

static work_struct_t* pop_work(workqueue_struct_t *wq)
{
	work_struct_t * w = wq->first;

	if (w) {
		if (wq->last == w) {
			wq->last = NULL;
			assert(w->next == 0);
		}
		wq->first = w->next;
		clear_bit(WORK_STRUCT_PENDING_BIT, &w->work_data_bits);
	} else {
		assert(wq->first == 0 && wq->last == 0);
	}
	assert( (wq->first && wq->last) || (!wq->first && !wq->last));

	return w;
}

static bool check_work(workqueue_struct_t *wq)
{
	bool ret = false;

	pthread_mutex_lock(&wq->mutex);
	if (wq->first) {
		ret = true;
	}
	pthread_mutex_unlock(&wq->mutex);

	return ret;
}

static int remove_work(workqueue_struct_t *wq, work_struct_t *wk)
{
	work_struct_t * w = wq->first;
	if (w == NULL || !test_bit(WORK_STRUCT_PENDING_BIT,
					&wk->work_data_bits) ) {
		/* wq already popped the wk, but handler function is blocking in locking mutex */
		return -1;
	}
	if (w == wk) {
		pop_work(wq);
		return 0;
	}
	assert(wq->first && wq->first != wk);

	/* find the one before wk in queue */
	while(w && w->next != wk) {
		w = w->next;
	}

	/* wk is not in queue */
	if (!w) {
		return -1;
	}

	/* remove wk from queue */
	w->next = wk->next;
	if(wq->last == wk){
		assert(wk->next == 0);
		wq->last = w;
	}

	clear_bit(WORK_STRUCT_PENDING_BIT, &wk->work_data_bits);

	assert( (wq->first && wq->last) || (!wq->first && !wq->last));
	return 0;
}

static bool __queue_work(workqueue_struct_t *wq, work_struct_t *w)
{
	bool queue_res;

	if (!test_bit(WORK_STRUCT_PENDING_BIT,
				&w->work_data_bits)) {
		/*
		 * delayed_work, timer was canceled,
		 * but timer func still called
		 */
		return false;
	}
	w->wq = wq;
	queue_res = push_work(wq, w);
	pthread_cond_broadcast(&wq->cv);

	return queue_res;
}

/**
 * queue_work - queue work on a workqueue
 * @wq: workqueue to use
 * @work: work to queue
 *
 * Returns %false if @work was already on a queue, %true otherwise.
 *
 * We queue the work to the CPU on which it was submitted, but if the CPU dies
 * it can be processed by another CPU.
 */
bool queue_work(workqueue_struct_t *wq, work_struct_t *w)
{
	bool queue_res;

	/* Check if user tries to push a work without bound workqueue */
	if (wq == NULL) {
		return true;
	}

	w->debug.start_time = local_clock();
	w->debug.caller = caller_main_offset();
	w->debug.tid = pthread_self();

	pthread_mutex_lock(&wq->mutex);
	if (!test_and_set_bit(WORK_STRUCT_PENDING_BIT,
				&w->work_data_bits)) {
		queue_res = __queue_work(wq, w);
	} else {
		queue_res = false;
	}
	pthread_mutex_unlock(&wq->mutex);

	return queue_res;
}

void resume_tasklet_workqueue()
{
	pthread_mutex_lock(&tasklet_wq->mutex);
	pthread_cond_broadcast(&tasklet_wq->cv);
	pthread_mutex_unlock(&tasklet_wq->mutex);
}

/**
 * cancel_work_sync - cancel a work and wait for it to finish
 * @work: the work to cancel
 *
 * Cancel @work and wait for its execution to finish.  This function
 * can be used even if the work re-queues itself or migrates to
 * another workqueue.  On return from this function, @work is
 * guaranteed to be not pending or executing on any CPU.
 *
 * cancel_work_sync(&delayed_work->work) must not be used for
 * delayed_work's.  Use cancel_delayed_work_sync() instead.
 *
 * The caller must ensure that the workqueue on which @work was last
 * queued can't be destroyed before this function returns.
 *
 * Return:
 * %true if @work was pending, %false otherwise.
 */
bool cancel_work_sync(struct work_struct *work)
{
	workqueue_struct_t* wq = work->wq;
	int rc;

	if (!wq) {
		return false;
	}

	pthread_mutex_lock(&wq->mutex);
	rc = remove_work(wq, work);
	pthread_mutex_unlock(&wq->mutex);
	if (rc == 0) {
		/* Work was in a queue and was deleted successfully */
		return true;
	}

	/* Make sure if work is executing right now, we have wait for it */
	flush_work(work);
	return false;
}

static void flusher_func(struct work_struct *work)
{
	/* We don't need any code here, function can't be NULL for work queue */
}

void flush_workqueue(workqueue_struct_t *wq)
{
	work_struct_t wk;

	INIT_WORK_ONSTACK(&wk, flusher_func);

	queue_work(wq, &wk);
	flush_work(&wk);
}

/**
 * queue_delayed_work - queue work after delay
 * @wq: workqueue to use
 * @dwork: work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * Return: %false if @work was already on a queue, %true otherwise. If
 * @delay is zero and @dwork is idle, it will be scheduled for immediate
 * execution.
 */
bool queue_delayed_work(workqueue_struct_t *wq, struct delayed_work *dwork, unsigned long delay)
{
	bool rc;
	work_struct_t *w = &dwork->work;

	w->debug.start_time = local_clock();
	w->debug.caller = caller_main_offset();
	w->debug.tid = pthread_self();

	pthread_mutex_lock(&wq->mutex);

	if (test_and_set_bit(WORK_STRUCT_PENDING_BIT,
				&w->work_data_bits)) {
		rc = false;
	} else if (!delay) {
		rc = __queue_work(wq, w);
	} else {
		dwork->work.wq = wq;
		dwork->work.data = w;

		rc = !mod_timer(&dwork->timer, jiffies + delay);
		if (rc) {
			set_bit(WORK_STRUCT_TIMERED_BIT,
					&w->work_data_bits);
		} else {
			/*
			 * timer_settime failed, disarmed timer
			 */
			rc = __queue_work(wq, w);
		}
	}
	pthread_mutex_unlock(&wq->mutex);

	return rc;
}

/**
 * mod_delayed_work_on - modify delay of or queue a delayed work on specific CPU
 * @cpu: CPU number to execute work on
 * @wq: workqueue to use
 * @dwork: work to queue
 * @delay: number of jiffies to wait before queueing
 *
 * If @dwork is idle, equivalent to queue_delayed_work_on(); otherwise,
 * modify @dwork's timer so that it expires after @delay.  If @delay is
 * zero, @work is guaranteed to be scheduled immediately regardless of its
 * current state.
 *
 * Return: %false if @dwork was idle and queued, %true if @dwork was
 * pending and its timer was modified.
 *
 * This function is safe to call from any context including IRQ handler.
 * See try_to_grab_pending() for details.
 */
bool mod_delayed_work_on(int cpu, struct workqueue_struct *wq, struct delayed_work *dwork, unsigned long delay)
{
	bool rc = true;
	(void)cpu;

	pthread_mutex_lock(&wq->mutex);

	dwork->work.wq = wq;

	if (!dwork->timer.pending) {
		rc = false;
	}

	mod_timer(&dwork->timer, jiffies + delay);

	pthread_mutex_unlock(&wq->mutex);

	return rc;
}

static bool cancel_dwork(struct delayed_work *dwork, bool sync)
{
	workqueue_struct_t *wq = dwork->work.wq;
	int rc;

	if (wq == NULL) {
		/* Nothing to cancel, workqueue was not assigned! */
		return false;
	}

	/*
	 * clear pending bit
	 */
	clear_bit(WORK_STRUCT_PENDING_BIT,
			&dwork->work.work_data_bits);
	if (test_and_clear_bit(WORK_STRUCT_TIMERED_BIT,
				&dwork->work.work_data_bits)) {
		del_timer_sync(&dwork->timer);
	}

	/*
	 * try to remove_work, if timer expired
	 */
	pthread_mutex_lock(&wq->mutex);
	rc = remove_work(wq, &dwork->work);
	pthread_mutex_unlock(&wq->mutex);
	if (rc == 0) {
		return true;
	}
	/*
	 * work doesn't find workqueue
	 */
	if (sync) {
		return flush_work(&dwork->work);
	} else {
		return false;
	}
}

/**
 * cancel_delayed_work - cancel a delayed work
 * @dwork: delayed_work to cancel
 *
 * Kill off a pending delayed_work.
 *
 * Return: %true if @dwork was pending and canceled; %false if it wasn't
 * pending.
 *
 * Note:
 * The work callback function may still be running on return, unless
 * it returns %true and the work doesn't re-arm itself.  Explicitly flush or
 * use cancel_delayed_work_sync() to wait on it.
 *
 * This function is safe to call from any context including IRQ handler.
 */
bool cancel_delayed_work(struct delayed_work *dw)
{
	return cancel_dwork(dw, false);
}

/**
 * cancel_delayed_work_sync - cancel a delayed work and wait for it to finish
 * @dwork: the delayed work cancel
 *
 * This is cancel_work_sync() for delayed works.
 *
 * Return:
 * %true if @dwork was pending, %false otherwise.
 */
bool cancel_delayed_work_sync(struct delayed_work *dw)
{
	return cancel_dwork(dw, true);
}

/**
 * flush_work - wait for a work to finish executing the last queueing instance
 * @work: the work to flush
 *
 * Wait until @work has finished execution.  @work is guaranteed to be idle
 * on return if it hasn't been requeued since flush started.
 *
 * Return:
 * %true if flush_work() waited for the work to finish execution,
 * %false if it was already idle.
 */
bool flush_work(struct work_struct *work)
{
	struct workqueue_struct* wq = work->wq;
	work_struct_t* w;
	int work_pending;

	/* Check if the work was scheduled for execution */
	if (wq == NULL) {
		return false;
	}

	pthread_mutex_lock(&wq->mutex);

	work_pending = 0;
	w = wq->first;
	while(w) {
		if (w == work) {
			work_pending = 1;
			break;
		}
		w = w->next;
	}

	if ((work_pending) || (wq->current == work)) {
		do {
			pthread_cond_wait(&wq->cv, &wq->mutex);

			work_pending = 0;
			w = wq->first;
			while(w) {
				if (w == work) {
					work_pending = 1;
					break;
				}
				w = w->next;
			}

			if ((!work_pending) && (wq->current != work)) {
				break;
			}
		} while(1);
	} else {
		pthread_mutex_unlock(&wq->mutex);
		return false;
	}

	pthread_mutex_unlock(&wq->mutex);
	return true;
}

/**
 * flush_delayed_work - wait for a dwork to finish executing the last queueing
 * @dwork: the delayed work to flush
 *
 * Delayed timer is cancelled and the pending work is queued for
 * immediate execution.  Like flush_work(), this function only
 * considers the last queueing instance of @dwork.
 *
 * Return:
 * %true if flush_work() waited for the work to finish execution,
 * %false if it was already idle.
 */
bool flush_delayed_work(struct delayed_work* dwork)
{
	struct workqueue_struct* wq = dwork->work.wq;

	/* Delayed work was not scheduled for execution, nothing to flush */
	if (wq == NULL) {
		return false;
	}
	if (del_timer_sync(&dwork->timer)) {
		pthread_mutex_lock(&wq->mutex);
		__queue_work(wq, &dwork->work);
		pthread_mutex_unlock(&wq->mutex);
	}

	return flush_work(&dwork->work);
}

static void* workqueue_thread(void* arg)
{
	struct timespec timeout;
	workqueue_struct_t *wq = (workqueue_struct_t *)arg;
	struct task_struct this_task, *qnx_workqueue_task;
	work_struct_t *we = NULL;
	int status;

	if (wq->name[0]) {
		pthread_setname_np(0, wq->name);
	}

	/* According to kernel docs: Tasklets run in software interrupt   */
	/* context with the result that all tasklet code must be atomic.  */
	/* Instead, workqueue functions run in the context of a special   */
	/* kernel process; as a result, they have more flexibility. In    */
	/* particular, workqueue functions can sleep.                     */
	/* Since we emulate tasklets through workqueue, tasklet workqueue */
	/* queue will require special handling for in_softirq(),          */
	/* in_interrupt(), in_atomic() calls. Workqueues have these       */
	/* states set to false.                                           */
	qnx_workqueue_task = &this_task;
	memset(qnx_workqueue_task, 0, sizeof(*qnx_workqueue_task));

	qnx_workqueue_task->pid = -1;
	qnx_workqueue_task->spid.pid = -1;
	strncpy(qnx_workqueue_task->comm, wq->name, sizeof(qnx_workqueue_task->comm) - 1);
	qnx_workqueue_task->real_cred = &qnx_workqueue_task->cred_vault;
	qnx_workqueue_task->cred = &qnx_workqueue_task->cred_vault;
	status = qnx_create_task_sched(qnx_workqueue_task);
	if (status) {
		/* Error output is done inside qnx_create_task_sched() function */
		return NULL;
	}
	current = qnx_workqueue_task;
	qnx_workqueue_task->attachment.user_tid = pthread_self();
	qnx_workqueue_task->attachment.copy_to_user_memcpy = 1;

	status = pthread_mutex_lock(&wq->mutex);
	if (status != 0) {
		qnx_error("Can't lock mutex in workqueue_thread!\n");
		return NULL;
	}

	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &timeout);
		timeout.tv_sec += WORKQUEUE_WORKER_EXIT_TIMEOUT;

		while ((we = pop_work(wq)) == NULL && !wq->quit) {
			status = pthread_cond_timedwait(&wq->cv, &wq->mutex, &timeout);
			if (status == ETIMEDOUT) {
				break;
			} else if (status != 0) {
				pthread_mutex_unlock(&wq->mutex);
				return NULL;
			}
		}
		if (wq->quit) {
			break;
		}
		if (we) {
			struct work_struct	temp = *we;
			unsigned long long	elapse;
			static unsigned long long	max_elapse
				= 5 * NSEC_PER_MSEC;

			wq->current = we;
			status = pthread_mutex_unlock(&wq->mutex);
			assert(EOK == status);

			if (we->func) {
				we->func(we->data);
			}

			elapse = local_clock() - temp.debug.start_time;
			if (elapse > max_elapse) {
				max_elapse = elapse;
				printk(KERN_INFO "DEBUG:%s %s "
					"elapse=%lldms fn=%#x "
					"caller=%#x[%d]\n",
					__func__, current->comm,
					elapse / NSEC_PER_MSEC,
					main_offset(temp.func),
					temp.debug.caller,
					temp.debug.tid);
			}
			status = pthread_mutex_lock(&wq->mutex);
			if (status != 0) {
				qnx_error("Can't lock mutex in workqueue_thread!\n");
				return NULL;
			}
			wq->current = NULL;
			we = NULL;

			pthread_cond_broadcast(&wq->cv);
		}
	}

	pthread_mutex_unlock(&wq->mutex);

	qnx_destroy_task_sched(current);
	current = NULL;

	return NULL;
}

/**
 * work_busy - test whether a work is currently pending or running
 * @work: the work to be tested
 *
 * Test whether @work is currently pending or running.  There is no
 * synchronization around this function and the test result is
 * unreliable and only useful as advisory hints or for debugging.
 *
 * Return:
 * OR'd bitmask of WORK_BUSY_* bits.
 */
unsigned int work_busy(struct work_struct *work)
{
	work_struct_t* w;
	int work_pending = 0;

	if (work->wq) {
		pthread_mutex_lock(&work->wq->mutex);

		w = work->wq->first;
		work_pending = test_bit(WORK_STRUCT_PENDING_BIT,
					&work->work_data_bits);
		w = NULL;
		while(w) {
			if (w == work) {
				work_pending = 1;
				break;
			}
			w = w->next;
		}
		pthread_mutex_unlock(&work->wq->mutex);
	}

	if (!work_pending) {
		return 0;
	}

	return WORK_BUSY_PENDING;
}

void delayed_work_timer_fn(unsigned long __data)
{
	struct delayed_work *dwork = (struct delayed_work *)__data;
	struct workqueue_struct* wq = dwork->work.wq;

	/* should have been called from irqsafe timer with irq already off */
	pthread_mutex_lock(&wq->mutex);
	clear_bit(WORK_STRUCT_TIMERED_BIT, &dwork->work.work_data_bits);
	__queue_work(wq, &dwork->work);
	pthread_mutex_unlock(&wq->mutex);
}

/**
 * drain_workqueue - drain a workqueue
 * @wq: workqueue to drain
 *
 * Wait until the workqueue becomes empty.  While draining is in progress,
 * only chain queueing is allowed.  IOW, only currently pending or running
 * work items on @wq can queue further work items on it.  @wq is flushed
 * repeatedly until it becomes empty.  The number of flushing is determined
 * by the depth of chaining and should be relatively short.  Whine if it
 * takes too long.
 */
void drain_workqueue(workqueue_struct_t *wq)
{
	int iteration = 0;

	while (check_work(wq)) {
		flush_workqueue(wq);
		iteration++;
		if (iteration == 10) {
			/* Print only once at iteration 10 */
			qnx_error("drain_workqueue() has potential lockup\n");
		}
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/workqueue.c $ $Rev: 873525 $")
#endif
