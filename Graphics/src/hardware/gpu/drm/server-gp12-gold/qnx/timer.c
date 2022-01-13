#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <drm/drmP.h>

#define I915_TIMER_CODE  111

struct _qnx_timers_data {
	struct sched_param	param;
	pthread_t		tid;
	pthread_attr_t		attr;
	int			max_prio;
	int			chid;
	int			coid;
} qnx_timers_data = {
	.tid = -1,
	.max_prio = -1,
	.chid = -1,
	.coid = -1,
};

struct hrtimer_clock_base qnx_timer_monotonic_base =
{
	.index = HRTIMER_BASE_MONOTONIC,
	.clockid = CLOCK_MONOTONIC,
	.get_time = &ktime_get,
	.resolution = KTIME_MONOTONIC_RES,
};

#define QNX_TIMER_THREAD_NAME "kernel timer"
/* "current" for timers should have interrupted task description     */
/* we set it to interrupted kernel thread state for easy processing. */
static struct task_struct qnx_timer_task;

static void* timer_thread(void* arg)
{
	int err;
	int rcvid;
	struct _pulse msg;
	bool pending;

	(void)arg;

	pthread_setname_np(0, QNX_TIMER_THREAD_NAME);

	err = qnx_create_task_sched(&qnx_timer_task);
	if (err) {
		/* Error output is done inside qnx_create_task_sched() function */
		return NULL;
	}
	current = &qnx_timer_task;
	qnx_timer_task.attachment.user_tid = pthread_self();
	qnx_timer_task.attachment.copy_to_user_memcpy = 1;

	for (;;) {
		rcvid = MsgReceive(qnx_timers_data.chid, &msg, sizeof(msg), NULL);
		if (rcvid == 0) {
			if (msg.code == I915_TIMER_CODE) {
				struct timer_list * timer = msg.value.sival_ptr;
				if (timer) {
					if (timer->function) {
						timer->function(timer->data);
					}

					pthread_mutex_lock(&timer->lock);
					timer->pending = false;
					pthread_mutex_unlock(&timer->lock);
				}
			}
		} else if (rcvid == -1) {
			qnx_error("MsgReceive error is %d\n", errno);
		}
	}

	qnx_destroy_task_sched(current);
	current = NULL;

	return NULL;
}

/* Linux kernel API to initialize timers at boot-up time */
void init_timers()
{
	int status = EOK;
	int curr_sched;
	pthread_attr_t task_attr;
	struct sched_param param;

	status = qnx_taskattr_init(&task_attr, QNX_PRTY_KERNEL);
	if (status) {
		/*
		 * error message at qnx_taskattr_init
		 */
		goto error;
	}
	/*
	 * get task sched param to set pulse priority
	 */
	status = pthread_attr_getschedparam(&task_attr, &param);
	if (status) {
		goto error;
	}
	qnx_timers_data.max_prio = param.sched_priority;

	qnx_timers_data.chid = ChannelCreate(_NTO_CHF_DISCONNECT | _NTO_CHF_UNBLOCK | _NTO_CHF_PRIVATE);
	if (qnx_timers_data.chid == -1) {
		qnx_error("ChannelCreate() call failed");
		pthread_attr_destroy(&qnx_timers_data.attr);
		goto error;
	}

	qnx_timers_data.coid = ConnectAttach(ND_LOCAL_NODE, 0, qnx_timers_data.chid, _NTO_SIDE_CHANNEL, 0);
	if (qnx_timers_data.coid == -1) {
		qnx_error("ConnectAttach() call failed");
		ChannelDestroy(qnx_timers_data.chid);
		qnx_timers_data.chid = -1;
		pthread_attr_destroy(&qnx_timers_data.attr);
		goto error;
	}

	/* According to kernel docs: No access to user space is allowed. */
	/* Because there is no  process context, there is no path to the */
	/* user space associated with any particular process.            */
	/* in_interrupt(), in_atomic(), in_softirq() should return true  */
	/* for this context.                                             */
	memset(&qnx_timer_task, 0, sizeof(qnx_timer_task));
	qnx_timer_task.pid = -1;
	qnx_timer_task.spid.pid = -1;
	strncpy(qnx_timer_task.comm, QNX_TIMER_THREAD_NAME, sizeof(qnx_timer_task.comm));
	qnx_timer_task.real_cred = &qnx_timer_task.cred_vault;
	qnx_timer_task.cred = &qnx_timer_task.cred_vault;

	/* Finally create a timer thread */
	status = pthread_create(&qnx_timers_data.tid, &task_attr,
			timer_thread, NULL);
	if (status != EOK) {
		qnx_error("pthread_create() call failed");
		ConnectDetach(qnx_timers_data.coid);
		qnx_timers_data.coid = -1;
		ChannelDestroy(qnx_timers_data.chid);
		qnx_timers_data.chid = -1;
		pthread_attr_destroy(&qnx_timers_data.attr);
		goto error;
	}

error:
	return;
}

static void do_setup_timer_locked(struct timer_list *timer)
{
	int status;

	SIGEV_PULSE_PTR_INIT(&timer->event, qnx_timers_data.coid, qnx_timers_data.max_prio, I915_TIMER_CODE, timer);
	status = timer_create(CLOCK_MONOTONIC, &timer->event, &timer->tmid);
	if (status) {
		qnx_error("[%d] timer=%p timer_create() call failed, panic",
				pthread_self(), timer);
		*(int*)0 = 0;
	}
}
void do_setup_timer(struct timer_list* timer)
{
	pthread_mutex_lock(&timer->lock);
	do_setup_timer_locked(timer);
	pthread_mutex_unlock(&timer->lock);
}

int mod_timer_locked(struct timer_list *timer, unsigned long expires)
{
	struct itimerspec itime;
	struct itimerspec otime;
	unsigned long cur = jiffies;
	int status = 0;

	/* Linux API allows manual structure fill-up, so we have to detect this case */
	if (timer->tmid == -1) {
		if (!timer->function) {
			qnx_error("timer function was not set up");
			return 0;
		}
		do_setup_timer_locked(timer);
	}

	timer->expires = expires;
	if (expires < cur) {
		expires = 0;
	} else {
		expires -= cur;
	}

	expires = jiffies_to_msecs(expires);
	nsec2timespec(&itime.it_value, jiffies_to_nsec(expires));
	nsec2timespec(&itime.it_interval, 0ULL);

	if (itime.it_value.tv_nsec < 1 * NSEC_PER_MSEC) {
		/* Fire timer immediately, but 1ms at least */
		itime.it_value.tv_nsec = 1 * NSEC_PER_MSEC;
	}

	memset(&otime, 0x00, sizeof(otime));

	status = timer_settime(timer->tmid, 0, &itime, &otime);
	if (status != 0) {
		qnx_error("[%d] timer_settime() call failed!"
				" timer=%p tmid=%d expire=%ld cur=%ld",
				pthread_self(),
				timer, timer->tmid,
				timer->expires, cur);
		return 0;
	} else {
		timer->pending = true;
	}

	if ((otime.it_value.tv_sec == 0) && (otime.it_value.tv_nsec == 0)) {
		return 0;
	}

	return 1;
}

/**
 * mod_timer - modify a timer's timeout
 * @timer: the timer to be modified
 * @expires: new timeout in jiffies
 *
 * mod_timer() is a more efficient way to update the expire field of an
 * active timer (if the timer is inactive it will be activated)
 *
 * mod_timer(timer, expires) is equivalent to:
 *
 *     del_timer(timer); timer->expires = expires; add_timer(timer);
 *
 * Note that if there are multiple unserialized concurrent users of the
 * same timer, then mod_timer() is the only safe way to modify the timeout,
 * since add_timer() cannot modify an already running timer.
 *
 * The function returns whether it has modified a pending timer or not.
 * (ie. mod_timer() of an inactive timer returns 0, mod_timer() of an
 * active timer returns 1.)
 */
int mod_timer(struct timer_list *timer, unsigned long expires)
{
	int rc;

	pthread_mutex_lock(&timer->lock);
	rc = mod_timer_locked(timer, expires);
	pthread_mutex_unlock(&timer->lock);

	return rc;
}

int mod_timer_pinned(struct timer_list *timer, unsigned long expires)
{
	return mod_timer(timer, expires);
}

/**
 * del_timer_sync - deactivate a timer and wait for the handler to finish.
 * @timer: the timer to be deactivated
 *
 * This function only differs from del_timer() on SMP: besides deactivating
 * the timer it also makes sure the handler has finished executing on other
 * CPUs.
 *
 * Synchronization rules: Callers must prevent restarting of the timer,
 * otherwise this function is meaningless. It must not be called from
 * interrupt contexts unless the timer is an irqsafe one. The caller must
 * not hold locks which would prevent completion of the timer's
 * handler. The timer's handler must not call add_timer_on(). Upon exit the
 * timer is not queued and the handler is not running on any CPU.
 *
 * The function returns whether it has deactivated a pending timer or not.
 */
int del_timer_sync(struct timer_list *timer)
{
	int status;
	int retcode = 1;

	pthread_mutex_lock(&timer->lock);

	if (timer->tmid >= 0) {
		struct itimerspec value = {
			.it_value    = {.tv_sec = 0, .tv_nsec = 0},
			.it_interval = {.tv_sec = 0, .tv_nsec = 0},
		};
		struct itimerspec ovalue = value;

		/* Disarm timer first */
		status = timer_settime(timer->tmid, 0, &value, &ovalue);
		if (status != 0) {
			qnx_error("Can't disarm timer");
			pthread_mutex_unlock(&timer->lock);
			return 0;
		}

		timer->pending = false;
		if ((ovalue.it_value.tv_sec == 0) && (ovalue.it_value.tv_nsec == 0)) {
			/* Timer has expired before this call */
			retcode = 0;
		}

		/* Destroy it */
		status = timer_delete(timer->tmid);
		if (status != 0) {
			qnx_error("Can't delete timer");
			pthread_mutex_unlock(&timer->lock);
			return 0;
		}
		timer->tmid = -1;
	}

	pthread_mutex_unlock(&timer->lock);
	pthread_mutex_destroy(&timer->lock);

	return retcode;
}

/**
 * del_timer - deactivate a timer.
 * @timer: the timer to be deactivated
 *
 * del_timer() deactivates a timer - this works on both active and inactive
 * timers.
 *
 * The function returns whether it has deactivated a pending timer or not.
 * (ie. del_timer() of an inactive timer returns 0, del_timer() of an
 * active timer returns 1.)
 */
int del_timer(struct timer_list * timer)
{
	return del_timer_sync(timer);
}

signed long __sched schedule_timeout_uninterruptible(signed long timeout)
{
	__set_current_state(TASK_UNINTERRUPTIBLE);
	return schedule_timeout(timeout);
}


static void do_init_timer(struct timer_list *timer, unsigned int flags,
			  const char *name, struct lock_class_key *key)
{
	memset(timer, 0, sizeof(*timer));
	timer->flags = flags;
	timer->tmid = -1;

	if (pthread_mutex_init(&timer->lock, NULL)) {
		qnx_error("[%d] timer=%p mutex-init failed, should panic",
				pthread_self(), timer);
		*(int*)0 = 0;	/* PANIC */
	}
}

/**
 * init_timer_key - initialize a timer
 * @timer: the timer to be initialized
 * @flags: timer flags
 * @name: name of the timer
 * @key: lockdep class key of the fake lock used for tracking timer
 *       sync lock dependencies
 *
 * init_timer_key() must be done to a timer prior calling *any* of the
 * other timer functions.
 */
void init_timer_key(struct timer_list *timer, unsigned int flags,
		const char *name, struct lock_class_key *key)
{
	do_init_timer(timer, flags, name, key);
}

void add_timer(struct timer_list *timer)
{
	unsigned long cur = jiffies;
	unsigned long expires;

	if (timer->expires < cur) {
		expires = 0;
	} else {
		expires -= cur;
	}

	BUG_ON(timer_pending(timer));
	mod_timer(timer, expires);
}

int hrtimer_mod_locked(struct hrtimer *timer, ktime_t tim);

static void hrtimer_fn(unsigned long data)
{
	struct hrtimer *timer = (struct hrtimer *)data;

	pthread_mutex_lock(&timer->std_timer.lock);

	if (timer->state == HRTIMER_STATE_INACTIVE) {
		pthread_mutex_unlock(&timer->std_timer.lock);
		return;
	}

	timer->state = HRTIMER_STATE_CALLBACK;
	pthread_mutex_unlock(&timer->std_timer.lock);

	if (timer->function) {
		enum hrtimer_restart restart = timer->function(timer);

		pthread_mutex_lock(&timer->std_timer.lock);

		if ((restart == HRTIMER_RESTART) && (timer->state != HRTIMER_STATE_INACTIVE)) {
			if (hrtimer_mod_locked(timer, timer->node.expires) != 0) {
				qnx_error("restart timer_settime fails: %s", strerror(errno));
				pthread_mutex_unlock(&timer->std_timer.lock);
				return;
			}

			timer->state = HRTIMER_STATE_ENQUEUED;
		} else {
			timer->state = HRTIMER_STATE_INACTIVE;
		}
	} else {
		pthread_mutex_lock(&timer->std_timer.lock);
		timer->state = HRTIMER_STATE_INACTIVE;
	}

	pthread_mutex_unlock(&timer->std_timer.lock);
}

int hrtimer_mod_locked(struct hrtimer *timer, ktime_t tim)
{
	struct itimerspec itime;
#if defined(__KERNEL411__)
	unsigned long expires = tim.tv64;
#else
	unsigned long expires = tim.tv.sec * 1000000000UL + tim.tv.nsec;
#endif /* __KERNEL411__ */

	/* Linux API allows manual structure fill-up, so we have to detect this case */
	if (timer->std_timer.tmid == -1) {
		if (!timer->function) {
			qnx_error("hrtimer function was not set up");
			return -1;
		}
		do_setup_timer_locked(&timer->std_timer);
	}

	nsec2timespec(&itime.it_value, expires);
	nsec2timespec(&itime.it_interval, 0ULL);
	timer->node.expires = ktime_set(itime.it_value.tv_sec, itime.it_value.tv_nsec);
	timer->_softexpires = ktime_set(itime.it_value.tv_sec, itime.it_value.tv_nsec);

	return timer_settime(timer->std_timer.tmid, TIMER_ABSTIME, &itime, NULL);
}

/**
 * hrtimer_init - initialize a timer to the given clock
 * @timer:	the timer to be initialized
 * @clock_id:	the clock to be used
 * @mode:       The modes which are relevant for intitialization:
 *              HRTIMER_MODE_ABS, HRTIMER_MODE_REL, HRTIMER_MODE_ABS_SOFT,
 *              HRTIMER_MODE_REL_SOFT
 *
 *              The PINNED variants of the above can be handed in,
 *              but the PINNED bit is ignored as pinning happens
 *              when the hrtimer is started
 */
void hrtimer_init(struct hrtimer *timer, clockid_t which_clock, enum hrtimer_mode mode)
{
	/*
	 * POSIX magic: Relative CLOCK_REALTIME timers are not affected by
	 * clock modifications, so they needs to become CLOCK_MONOTONIC to
	 * ensure POSIX compliance.
	 */
	if (which_clock == CLOCK_REALTIME && mode & HRTIMER_MODE_REL) {
		which_clock = CLOCK_MONOTONIC;
	}

	if (which_clock != CLOCK_MONOTONIC) {
		qnx_error("clock is not CLOCK_MONOTONIC! Aborting ...");
		return;
	}

	if (timer == NULL) {
		qnx_error("timer is NULL! Aborting ...");
		return;
	}

	/* We don't care about soft irq context and cannot pin timer to a particular CPU, so ignore these flags */
	mode &= ~(HRTIMER_MODE_PINNED | HRTIMER_MODE_SOFT);
	if ((mode != HRTIMER_MODE_ABS) && (mode != HRTIMER_MODE_REL)) {
		qnx_error("timer mode is %d (not HRTIMER_MODE_ABS | HRTIMER_MODE_REL!) Aborting ...", mode);
		return;
	}

	memset(timer, 0, sizeof(*timer));
	timer->base = &qnx_timer_monotonic_base;

	setup_timer(&timer->std_timer, hrtimer_fn, (unsigned long) timer);
}

/**
 * hrtimer_start_range_ns - (re)start an hrtimer on the current CPU
 * @timer:	the timer to be added
 * @tim:	expiry time
 * @delta_ns:	"slack" range for the timer
 * @mode:	expiry mode: absolute (HRTIMER_MODE_ABS) or
 *		relative (HRTIMER_MODE_REL)
 */
int hrtimer_start_range_ns(struct hrtimer *timer, ktime_t tim, unsigned long range_ns, const enum hrtimer_mode mode)
{
	enum hrtimer_mode local_mode = mode;

	if (timer == NULL) {
		return -1;
	}

	local_mode &= ~(HRTIMER_MODE_PINNED | HRTIMER_MODE_SOFT);
	if ((local_mode != HRTIMER_MODE_ABS) && (local_mode != HRTIMER_MODE_REL)) {
		qnx_error("timer mode is %d (not HRTIMER_MODE_ABS | HRTIMER_MODE_REL!) Aborting ...", mode);
		return -1;
	}
	if (qnx_timers_data.chid <= 0) {
		return -1;
	}

	pthread_mutex_lock(&timer->std_timer.lock);

	/* Timer was not initialized */
	if (timer->std_timer.tmid <= 0) {
		pthread_mutex_unlock(&timer->std_timer.lock);
		return -1;
	}

	if (timer->state == HRTIMER_STATE_ENQUEUED) {
		pthread_mutex_unlock(&timer->std_timer.lock);
		return 0;
	}

	if (local_mode == HRTIMER_MODE_REL) {
		tim = ktime_add(ktime_get(), tim);
	}

	if (hrtimer_mod_locked(timer, tim) != 0) {
		qnx_error("start timer_settime fails: %s", strerror(errno));
		pthread_mutex_unlock(&timer->std_timer.lock);
		return -1;
	}

	timer->state = HRTIMER_STATE_ENQUEUED;

	pthread_mutex_unlock(&timer->std_timer.lock);

	return 0;
}

/**
 * hrtimer_cancel - cancel a timer and wait for the handler to finish.
 * @timer:	the timer to be cancelled
 *
 * Returns:
 *  0 when the timer was not active
 *  1 when the timer was active
 */
int hrtimer_cancel(struct hrtimer *timer)
{
	if (timer == NULL) {
		return 0;
	}
	if (qnx_timers_data.chid <= 0) {
		return 0;
	}

	pthread_mutex_lock(&timer->std_timer.lock);

	/* Timer was not initialized */
	if (timer->std_timer.tmid <= 0) {
		pthread_mutex_unlock(&timer->std_timer.lock);
		return 0;
	}

	if (timer->state == HRTIMER_STATE_INACTIVE) {
		pthread_mutex_unlock(&timer->std_timer.lock);
		return 0;
	}

	if (hrtimer_mod_locked(timer, ktime_set(0, 0)) != 0) {
		qnx_error("cancel timer_settime fails: %s", strerror(errno));
		pthread_mutex_unlock(&timer->std_timer.lock);
		return 0;
	}

	/* Signal callback handler that we would like to deactivate timer */
	timer->state = HRTIMER_STATE_INACTIVE;

	/* timer_pending() access should be done with mutex held on */
	while (timer_pending(&timer->std_timer)) {
		/* Allow timer thread to do it's job */
		pthread_mutex_unlock(&timer->std_timer.lock);
		/* Lock  timer again for timer_pending() call */
		pthread_mutex_lock(&timer->std_timer.lock);
	}

	/* Set final state to deactivated */
	timer->state = HRTIMER_STATE_INACTIVE;

	pthread_mutex_unlock(&timer->std_timer.lock);

	return 1;
}

/**
 * hrtimer_start - (re)start an hrtimer on the current CPU
 * @timer:	the timer to be added
 * @tim:	expiry time
 * @mode:	expiry mode: absolute (HRTIMER_MODE_ABS) or
 *		relative (HRTIMER_MODE_REL)
 *
 * Returns:
 *  0 on success
 *  1 when the timer was active
 */
int hrtimer_start(struct hrtimer *timer, ktime_t tim, const enum hrtimer_mode mode)
{
	return hrtimer_start_range_ns(timer, tim, 0, mode);
}
EXPORT_SYMBOL_GPL(hrtimer_start);

#if defined(__KERNEL411__)
/**
 * hrtimer_forward - forward the timer expiry
 * @timer:	hrtimer to forward
 * @now:	forward past this time
 * @interval:	the interval to forward
 *
 * Forward the timer expiry so it will expire in the future.
 * Returns the number of overruns.
 *
 * Can be safely called from the callback function of @timer. If
 * called from other contexts @timer must neither be enqueued nor
 * running the callback and the caller needs to take care of
 * serialization.
 *
 * Note: This only updates the timer expiry value and does not requeue
 * the timer.
 */
u64 hrtimer_forward(struct hrtimer *timer, ktime_t now, ktime_t interval)
{
	u64 orun = 1;
	ktime_t delta;

	delta = ktime_sub(now, hrtimer_get_expires(timer));

	if (delta.tv64 < 0)
		return 0;

	if (interval.tv64 < timer->base->resolution.tv64)
		interval.tv64 = timer->base->resolution.tv64;

	if (unlikely(delta.tv64 >= interval.tv64)) {
		s64 incr = ktime_to_ns(interval);

		orun = ktime_divns(delta, incr);
		hrtimer_add_expires_ns(timer, incr * orun);
		if (hrtimer_get_expires_tv64(timer) > now.tv64)
			return orun;
		/*
		 * This (and the ktime_add() below) is the
		 * correction for exact:
		 */
		orun++;
	}
	hrtimer_add_expires(timer, interval);

	return orun;
}
EXPORT_SYMBOL_GPL(hrtimer_forward);
#endif /* __KERNEL411__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/timer.c $ $Rev: 857676 $")
#endif
