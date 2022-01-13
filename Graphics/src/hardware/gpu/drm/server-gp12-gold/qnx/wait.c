#include <linux/qnx.h>
#include <linux/linux.h>

static int try_to_wake_up(struct task_struct *p, unsigned int state, int wake_flags)
{
	int	ret;

	pthread_mutex_lock(&p->sched_mutex);
	set_bit(QNX_SCHED_SIGNALLED, &p->sched_flags);
	pthread_cond_broadcast(&p->sched_cond);

	ret = p->task_state & state;
	pthread_mutex_unlock(&p->sched_mutex);

	/*
	 * if try_to_wake_up always return 1,
	 * hang_check will be called every 1 jiffies = 1ms
	 * even target task yet started
	 */
	return ret;
}

int default_wake_function(wait_queue_t *wait, unsigned mode, int wake_flags, void *key)
{
	return try_to_wake_up(wait->private, mode, wake_flags);
}

int woken_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	/*
	 * Although this function is called under waitqueue lock, LOCK
	 * doesn't imply write barrier and the users expects write
	 * barrier semantics on wakeup functions.  The following
	 * smp_wmb() is equivalent to smp_wmb() in try_to_wake_up()
	 * and is paired with smp_store_mb() in wait_woken().
	 */
	smp_wmb(); /* C */
	wait->flags |= WQ_FLAG_WOKEN;

	return default_wake_function(wait, mode, sync, key);
}

int wake_bit_function(wait_queue_t *wait, unsigned mode, int sync, void *arg)
{
	struct wait_bit_key *key = arg;
	struct wait_bit_queue *wait_bit
		= container_of(wait, struct wait_bit_queue, wait);

	if (wait_bit->key.flags != key->flags ||
			wait_bit->key.bit_nr != key->bit_nr ||
			test_bit(key->bit_nr, key->flags))
		return 0;
	else
		return autoremove_wake_function(wait, mode, sync, key);
}

void deinit_waitqueue_head(wait_queue_head_t *q)
{
	q->initialized = 0;
	spin_destroy(&q->lock);
}

void add_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	spin_lock(&q->lock);
	__add_wait_queue(q, wait);
	spin_unlock(&q->lock);
}

void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *wait)
{
	spin_lock(&q->lock);
	__remove_wait_queue(q, wait);
	spin_unlock(&q->lock);
}

void __wake_up_common(wait_queue_head_t *q, unsigned int mode,
			int nr_exclusive, int wake_flags, void *key)
{
	wait_queue_t *curr, *next;

	list_for_each_entry_safe(curr, next, &q->task_list, task_list) {
		unsigned flags = curr->flags;

		if (curr->func(curr, mode, wake_flags, key) &&
			(flags & WQ_FLAG_EXCLUSIVE) && !--nr_exclusive)
			break;
	}
}

void __wake_up(wait_queue_head_t *q, unsigned int mode, int nr, void *key) {
	spin_lock(&q->lock);
	__wake_up_common(q, mode, nr, 0, key);
	spin_unlock(&q->lock);
}

void __wake_up_locked_key(wait_queue_head_t *q, unsigned int mode, void *key) {
	__wake_up_common(q, mode, 0, 0, key);
}

int autoremove_wake_function(wait_queue_t *wait, unsigned mode, int sync, void *key)
{
	int ret = default_wake_function(wait, mode, sync, key);

	if (ret)
		list_del_init(&wait->task_list);
	return ret;
}

/*
 * Note: we use "set_current_state()" _after_ the wait-queue add,
 * because we need a memory barrier there on SMP, so that any
 * wake-function that tests for the wait-queue being active
 * will be guaranteed to see waitqueue addition _or_ subsequent
 * tests in this thread will see the wakeup having taken place.
 *
 * The spin_unlock() itself is semi-permeable and only protects
 * one way (it only protects stuff inside the critical region and
 * stops them from bleeding out - it would still allow subsequent
 * loads to move into the critical region).
 */
void prepare_to_wait(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	unsigned long flags = 0;

	if (!q->initialized) {
		return;
	}

	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	if (list_empty(&wait->task_list))
		__add_wait_queue(q, wait);
	set_current_state(state);
	spin_unlock_irqrestore(&q->lock, flags);
}

/*
 * finish_wait - clean up after waiting in a queue
 * @q: waitqueue waited on
 * @wait: wait descriptor
 *
 * Sets current thread back to running state and removes
 * the wait descriptor from the given waitqueue if still
 * queued.
 */
void finish_wait(wait_queue_head_t *q, wait_queue_t *wait)
{
	unsigned long flags = 0;

	if (!q->initialized) {
		return;
	}

	__set_current_state(TASK_RUNNING);
	/*
	 * We can check for list emptiness outside the lock
	 * IFF:
	 *  - we use the "careful" check that verifies both
	 *    the next and prev pointers, so that there cannot
	 *    be any half-pending updates in progress on other
	 *    CPU's that we haven't seen yet (and that might
	 *    still change the stack area.
	 * and
	 *  - all other users take the lock (ie we can only
	 *    have _one_ other CPU that looks at or modifies
	 *    the list).
	 */
	if (!list_empty_careful(&wait->task_list)) {
		spin_lock_irqsave(&q->lock, flags);
		list_del_init(&wait->task_list);
		spin_unlock_irqrestore(&q->lock, flags);
	}
}

/**
 * wake_up_process - Wake up a specific process
 * @p: The process to be woken up.
 *
 * Attempt to wake up the nominated process and move it to the set of runnable
 * processes.
 *
 * Return: 1 if the process was woken up, 0 if it was already running.
 *
 * It may be assumed that this function implies a write memory barrier before
 * changing the task state if and only if any tasks are woken up.
 */
int wake_up_process(struct task_struct *p)
{
	WARN_ON(task_is_stopped_or_traced(p));
	return try_to_wake_up(p, TASK_NORMAL, 0);
}

int wake_up_state(struct task_struct *p, unsigned int state)
{
	return try_to_wake_up(p, state, 0);
}

long prepare_to_wait_event(wait_queue_head_t *q, wait_queue_t *wait, int state)
{
	unsigned long flags;

	if (signal_pending_state(state, current))
		return -ERESTARTSYS;

	wait->private = current;
	wait->func = autoremove_wake_function;

	spin_lock_irqsave(&q->lock, flags);
	if (list_empty(&wait->task_list)) {
		if (wait->flags & WQ_FLAG_EXCLUSIVE)
			__add_wait_queue_tail(q, wait);
		else
			__add_wait_queue(q, wait);
	}
	set_current_state(state);
	spin_unlock_irqrestore(&q->lock, flags);

	return 0;
}

void __wake_up_locked(wait_queue_head_t *q, unsigned int mode, int nr)
{
	__wake_up_common(q, mode, nr, 0, NULL);
}

static inline wait_queue_head_t *atomic_t_waitqueue(atomic_t *p)
{
	return NULL;
}

void __wake_up_bit(wait_queue_head_t *wq, void *word, int bit)
{
	struct wait_bit_key key = __WAIT_BIT_KEY_INITIALIZER(word, bit);
	if (waitqueue_active(wq))
		__wake_up(wq, TASK_NORMAL, 1, &key);
}

void wake_up_atomic_t(atomic_t *p)
{
	__wake_up_bit(atomic_t_waitqueue(p), p, 0);
}

/**
 * wake_up_bit - wake up a waiter on a bit
 * @word: the word being waited on, a kernel virtual address
 * @bit: the bit of the word being waited on
 *
 * There is a standard hashed waitqueue table for generic use. This
 * is the part of the hashtable's accessor API that wakes up waiters
 * on a bit. For instance, if one were to have waiters on a bitflag,
 * one would call wake_up_bit() after clearing the bit.
 *
 * In order for this to function properly, as it uses waitqueue_active()
 * internally, some kind of memory barrier must be done prior to calling
 * this. Typically, this will be smp_mb__after_atomic(), but in some
 * cases where bitflags are manipulated non-atomically under a lock, one
 * may need to use a less regular barrier, such fs/inode.c's smp_mb(),
 * because spin_unlock() does not guarantee a memory barrier.
 */
void wake_up_bit(void *word, int bit)
{
	__wake_up_bit(bit_waitqueue(word, bit), word, bit);
}
EXPORT_SYMBOL(wake_up_bit);

int __sched out_of_line_wait_on_bit(void *word, int bit,
				    wait_bit_action_f *action, unsigned mode)
{
	wait_queue_head_t *wq_head = bit_waitqueue(word, bit);
	DEFINE_WAIT_BIT(wq_entry, word, bit);

	return __wait_on_bit(wq_head, &wq_entry, action, mode);
}

int __sched out_of_line_wait_on_bit_timeout(
	void *word, int bit, wait_bit_action_f *action,
	unsigned mode, unsigned long timeout)
{
	wait_queue_head_t *wq = bit_waitqueue(word, bit);
	DEFINE_WAIT_BIT(wait, word, bit);

	wait.key.timeout = jiffies + timeout;
	return __wait_on_bit(wq, &wait, action, mode);
}
EXPORT_SYMBOL_GPL(out_of_line_wait_on_bit_timeout);

long wait_woken(wait_queue_t *wait, unsigned mode, long timeout)
{
	set_current_state(mode);
	if (!(wait->flags & WQ_FLAG_WOKEN) && !is_kthread_should_stop()) {
		timeout = schedule_timeout(timeout);
	}
	__set_current_state(TASK_RUNNING);

	/*
	 * The below implies an smp_mb(), it too pairs with the smp_wmb() from
	 * woken_wake_function() such that we must either observe the wait
	 * condition being true _OR_ WQ_FLAG_WOKEN such that we will not miss
	 * an event.
	 */
	smp_store_mb(wait->flags, wait->flags & ~WQ_FLAG_WOKEN);

	return timeout;
}
EXPORT_SYMBOL(wait_woken);

int bit_wait_timeout(struct wait_bit_key *word, int mode)
{
	unsigned long now = jiffies;

	if (time_after_eq(now, word->timeout)) {
		return -EAGAIN;
	}

	schedule_timeout(word->timeout - now);

	if (signal_pending_state(mode, current)) {
		return -EINTR;
	}

	return 0;
}

__sched int bit_wait(struct wait_bit_key *word, int mode)
{
	schedule();
	return 0;
}
EXPORT_SYMBOL(bit_wait);

/*
 * To allow interruptible waiting and asynchronous (i.e. nonblocking)
 * waiting, the actions of __wait_on_bit() and __wait_on_bit_lock() are
 * permitted return codes. Nonzero return codes halt waiting and return.
 */
int __wait_on_bit(wait_queue_head_t *wq, struct wait_bit_queue *q,
	wait_bit_action_f *action, unsigned mode)
{
	int ret = 0;

	do {
		prepare_to_wait(wq, &q->wait, mode);
		if (test_bit(q->key.bit_nr, q->key.flags))
			ret = (*action)(&q->key, mode);
	} while (test_bit(q->key.bit_nr, q->key.flags) && !ret);

	finish_wait(wq, &q->wait);

	return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/wait.c $ $Rev: 864420 $")
#endif
