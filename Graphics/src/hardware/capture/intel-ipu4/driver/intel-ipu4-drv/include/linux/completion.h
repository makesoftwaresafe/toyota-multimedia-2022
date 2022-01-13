#ifndef __LINUX_COMPLETION_H
#define __LINUX_COMPLETION_H

/*
 * (C) Copyright 2001 Linus Torvalds
 * Some modifications (__QNXNTO__) Copyright (c) 2017 QNX Software Systems.
 *
 * Atomic wait-for-completion handler data structures.
 * See kernel/sched/completion.c for details.
 */

#ifndef __QNXNTO__
#include <linux/wait.h>
#else
#include <pthread.h>
#include <linux/wait.h>
#endif

/*
 * struct completion - structure used to maintain state for a "completion"
 *
 * This is the opaque structure used to maintain the state for a "completion".
 * Completions currently use a FIFO to queue threads that have to wait for
 * the "completion" event.
 *
 * See also:  complete(), wait_for_completion() (and friends _timeout,
 * _interruptible, _interruptible_timeout, and _killable), init_completion(),
 * reinit_completion(), and macros DECLARE_COMPLETION(),
 * DECLARE_COMPLETION_ONSTACK().
 */
#ifndef __QNXNTO__
struct completion {
	unsigned int done;
	wait_queue_head_t wait;
};
#else
struct completion {
    unsigned long     done;
    pthread_cond_t    cond;
    pthread_mutex_t   mutex;
    wait_queue_head_t wait;
};
#endif

#define COMPLETION_INITIALIZER(work) \
	{ 0, __WAIT_QUEUE_HEAD_INITIALIZER((work).wait) }

#define COMPLETION_INITIALIZER_ONSTACK(work) \
	({ init_completion(&work); work; })

/**
 * DECLARE_COMPLETION - declare and initialize a completion structure
 * @work:  identifier for the completion structure
 *
 * This macro declares and initializes a completion structure. Generally used
 * for static declarations. You should use the _ONSTACK variant for automatic
 * variables.
 */
#define DECLARE_COMPLETION(work) \
	struct completion work = COMPLETION_INITIALIZER(work)

#define INIT_COMPLETION(x)	{x.done=0;pthread_cond_init (&x.cv, NULL); pthread_mutex_init (&x.mutex, NULL);}

/*
 * Lockdep needs to run a non-constant initializer for on-stack
 * completions - so we use the _ONSTACK() variant for those that
 * are on the kernel stack:
 */
/**
 * DECLARE_COMPLETION_ONSTACK - declare and initialize a completion structure
 * @work:  identifier for the completion structure
 *
 * This macro declares and initializes a completion structure on the kernel
 * stack.
 */
#ifndef __QNXNTO__
#ifdef CONFIG_LOCKDEP
# define DECLARE_COMPLETION_ONSTACK(work) \
	struct completion work = COMPLETION_INITIALIZER_ONSTACK(work)
#else
# define DECLARE_COMPLETION_ONSTACK(work) DECLARE_COMPLETION(work)
#endif
#endif

/**
 * init_completion - Initialize a dynamically allocated completion
 * @x:  pointer to completion structure that is to be initialized
 *
 * This inline function will initialize a dynamically created completion
 * structure.
 */
static inline void init_completion(struct completion *x)
{
#ifndef __QNXNTO__
    x->done = 0;
    init_waitqueue_head(&x->wait);
#else
    pthread_condattr_t attr;

    x->done = 0;
    pthread_condattr_init( &attr);
    pthread_condattr_setclock( &attr, CLOCK_MONOTONIC);
    pthread_cond_init(&x->cond, &attr);
    pthread_condattr_destroy( &attr);

    pthread_mutex_init (&x->mutex, NULL);
#endif
}

#ifdef __QNXNTO__
// QNX version of driver must call this function to cleanup
static inline void destroy_completion(struct completion *x)
{
    x->done = 0;
    pthread_cond_destroy(&x->cond);
    pthread_mutex_destroy(&x->mutex);
}
#endif
/**
 * reinit_completion - reinitialize a completion structure
 * @x:  pointer to completion structure that is to be reinitialized
 *
 * This inline function should be used to reinitialize a completion structure so it can
 * be reused. This is especially important after complete_all() is used.
 */
static inline void reinit_completion(struct completion *x)
{
	x->done = 0;
}

extern void wait_for_completion(struct completion *);
extern void wait_for_completion_io(struct completion *);
extern int wait_for_completion_interruptible(struct completion *x);
extern int wait_for_completion_killable(struct completion *x);
extern unsigned long wait_for_completion_timeout(struct completion *x,
						   unsigned long timeout);
extern unsigned long wait_for_completion_io_timeout(struct completion *x,
						    unsigned long timeout);
extern long wait_for_completion_interruptible_timeout(
	struct completion *x, unsigned long timeout);
extern long wait_for_completion_killable_timeout(
	struct completion *x, unsigned long timeout);
extern bool try_wait_for_completion(struct completion *x);
extern bool completion_done(struct completion *x);

extern void complete(struct completion *);
extern void complete_all(struct completion *);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/completion.h $ $Rev: 871034 $")
#endif
