/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/timer.h.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _QNX_LINUX_TIMER_H
#define _QNX_LINUX_TIMER_H

#include <linux/types.h>
#include <linux/sched.h> 
#include <linux/ktime.h> 
#include <linux/math64.h>
#include <linux/lockdep.h>

#include <sys/time.h>

#ifdef __QNXNTO__
#include <unistd.h>
#endif /* __QNXNTO__ */

struct timer_list {
	list_head_t         entry;
    pthread_t           tid;
    pthread_mutex_t     mutex;
    pthread_cond_t      cond;
    bool                initialized;
    bool                thread_active;
    // Time that timer expires in nanoseconds
	s64                 expires;
	void (*function)(unsigned long);
	unsigned long       data;
};

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

static inline bool timespec_valid(const struct timespec *ts)
{
	/* Dates before 1970 are bogus */
	if (ts->tv_sec < 0)
		return false;
	/* Can't have more nanoseconds then a second */
	if ((unsigned long)ts->tv_nsec >= NSEC_PER_SEC)
		return false;
	return true;
}

static inline s64 timespec_to_ns(const struct timespec *ts)
{
	return ((s64) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}


static inline void getrawmonotonic(struct timespec *ts)
{
	if( clock_gettime(CLOCK_MONOTONIC, ts) == -1 ) {
	      perror( "Monolitic clock gettime faied" );
    }
}


static inline void set_normalized_timespec(struct timespec *ts, time_t sec, s64 nsec)
{
	while (nsec >= NSEC_PER_SEC) {
		/*
		 * The following asm() prevents the compiler from
		 * optimising this loop into a modulo operation. See
		 * also __iter_div_u64_rem() in include/linux/time.h
		 */
		asm("" : "+rm"(nsec));
		nsec -= NSEC_PER_SEC;
		++sec;
	}
	while (nsec < 0) {
		asm("" : "+rm"(nsec));
		nsec += NSEC_PER_SEC;
		--sec;
	}
	ts->tv_sec = sec;
	ts->tv_nsec = nsec;
}

static inline struct timespec timespec_sub(struct timespec lhs,
						struct timespec rhs)
{
	struct timespec ts_delta;
	set_normalized_timespec(&ts_delta, lhs.tv_sec - rhs.tv_sec,
				lhs.tv_nsec - rhs.tv_nsec);
	return ts_delta;
}

#define typecheck(type,x) \
({	type __dummy; \
	typeof(x) __dummy2; \
	(void)(&__dummy == &__dummy2); \
	1; \
})

#define time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)(b) - (long)(a) < 0))

#define time_before(a,b)	time_after(b,a)

#define time_after_eq(a,b)	\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)(a) - (long)(b) >= 0))

#define time_before_eq(a,b)	time_after_eq(b,a)

static inline void getnstimeofday(struct timespec *ts)
{
	if( clock_gettime(CLOCK_REALTIME, ts) == -1 ) {
	      perror( "Monolitic clock gettime failed" );
    }
}

static inline void do_gettimeofday(struct timeval *tv)
{
	struct timespec now;

	getnstimeofday(&now);
//	tv->tv_sec = (time_t)(now.tv_sec);
//	tv->tv_usec = (suseconds_t)(now.tv_nsec/1000);
}

static inline s64 timeval_to_ns(const struct timeval *tv)
{
	return ((s64) tv->tv_sec * NSEC_PER_SEC) +
		tv->tv_usec * NSEC_PER_USEC;
}

static inline struct timespec ns_to_timespec(const s64 nsec)
{
	struct timespec ts;
	s32 rem;

	if (!nsec)
		return (struct timespec) {0, 0};

// Linker error when using div_s64_rem()
#ifdef __QNXNTO__
	rem = nsec % NSEC_PER_SEC;
	ts.tv_sec = nsec / NSEC_PER_SEC;
#else
	ts.tv_sec = div_s64_rem(nsec, NSEC_PER_SEC, &rem);
#endif
	if (unlikely(rem < 0)) {
		ts.tv_sec--;
		rem += NSEC_PER_SEC;
	}
	ts.tv_nsec = rem;

	return ts;
}

/* microseconds */
static void inline udelay(unsigned long j) {
	if(j >= 1000){
		usleep((_Uint32t)j);
	}else {
		struct timespec ts;
		ts.tv_sec  = 0;
		ts.tv_nsec = j * 1000;
		nanospin(&ts);
	}
}
static inline void usleep_range(unsigned long min, unsigned long max)
{
	udelay(min);
}

/* milliseconds */
static void inline msleep(unsigned int ms){
	delay(ms);
}
static void inline mdelay(unsigned long ms) {
	delay(ms);
}
/* nanoseconds */
static inline void ndelay(unsigned long x)
{
	udelay(DIV_ROUND_UP(x, 1000));
}

void init_timer_key(struct timer_list *timer, unsigned int flags,
		const char *name, struct lock_class_key *key);
struct lock_class_key;
static inline void init_timer_on_stack_key(struct timer_list *timer,
					   unsigned int flags, const char *name,
					   struct lock_class_key *key)
{
	init_timer_key(timer, flags, name, key);
}

#define __init_timer_on_stack(_timer, _flags)				\
	init_timer_on_stack_key((_timer), (_flags), NULL, NULL)

#define __setup_timer_on_stack(_timer, _fn, _data, _flags)		\
	do {								\
		__init_timer_on_stack((_timer), (_flags));		\
		(_timer)->function = (_fn);				\
		(_timer)->data = (_data);				\
	} while (0)

#define setup_timer(timer, func, data) \
	_setup_timer(timer, func, data, "timer_"#func)

void _setup_timer(struct timer_list * timer,
				void (*function)(unsigned long),
				  unsigned long data, const char * name);
#define setup_timer_on_stack(timer, fn, data)				\
	__setup_timer_on_stack((timer), (fn), (data), 0)

static inline void destroy_timer_on_stack(struct timer_list *timer){ }


int del_timer_sync(struct timer_list *timer);
int mod_timer(struct timer_list *timer, unsigned long expires);
int mod_timer_pinned(struct timer_list *timer, unsigned long expires);

long __sched schedule_timeout(long timeout);

static inline signed long __sched
schedule_timeout_killable(signed long timeout)
{
	//__set_current_state(TASK_KILLABLE);
	return schedule_timeout(timeout);
}

/**
 * timer_pending - is a timer pending?
 * @timer: the timer in question
 *
 * timer_pending will tell whether a given timer is currently pending,
 * or not. Callers must ensure serialization wrt. other operations done
 * to this timer, eg. interrupt contexts, or other CPUs on SMP.
 *
 * return value: 1 if the timer is pending, 0 if not.
 */
static inline int timer_pending(const struct timer_list * timer)
{
	struct timespec time;
	s64 nanoseconds;
	getnstimeofday(&time);
	nanoseconds = timespec_to_ns(&time);
	// Expire is pending only if it is in future
	if (timer->expires > nanoseconds) {
	    return 1;
	}
	return 0;
}

static inline void init_timer(struct timer_list *timer)
{
    int err;
    pthread_condattr_t attr;

    if (timer == NULL) {
        return;
    }
    timer->initialized = false;
    timer->thread_active = false;
    timer->expires = 0;
    err = pthread_mutex_init(&timer->mutex, NULL);
    if (err != EOK) {
        return;
    }
    err = pthread_condattr_init(&attr);
    if (err == EOK) {
        err = pthread_condattr_setclock(&attr, CLOCK_MONOTONIC);
        if (err == EOK) {
            err = pthread_cond_init(&timer->cond, &attr);
        }
    }
    pthread_condattr_destroy(&attr);
    if (err != EOK) {
        pthread_mutex_destroy(&timer->mutex);
        return;
    }
    timer->initialized = true;
 }

static inline void destroy_timer(struct timer_list *timer)
{
    if (timer == NULL) {
        return;
    }
    if (timer->thread_active) {
        timer->thread_active = false;
        pthread_join(timer->tid, NULL);
    }
    if (timer->initialized) {
        pthread_cond_destroy(&timer->cond);
        pthread_mutex_destroy(&timer->mutex);
    }
}

void* timer_wait_thread(void* arg);

static inline void add_timer(struct timer_list *timer)
{
    pthread_attr_t      thread_attr;
    struct sched_param  sched_param;
    int                 err;

    if (timer == NULL) {
        return;
    }
    pthread_mutex_lock(&timer->mutex);
    if (timer->thread_active == false) {
        // Start thread to wait on timer; nothing to do if already running
        pthread_attr_init(&thread_attr);
        sched_param.sched_priority = 29;
        pthread_attr_setinheritsched(&thread_attr, PTHREAD_EXPLICIT_SCHED);
        pthread_attr_setschedpolicy(&thread_attr, SCHED_RR);
        pthread_attr_setschedparam(&thread_attr, &sched_param);
        timer->thread_active = true;
        err = pthread_create(&timer->tid, &thread_attr, timer_wait_thread, (void*) timer);
        pthread_attr_destroy(&thread_attr);
        if (err != EOK) {
            timer->thread_active = false;
        }
    }
    pthread_mutex_unlock(&timer->mutex);
}

static inline int try_to_del_timer_sync(struct timer_list *timer)
{
    int err = EOK;
    if (timer == NULL) {
        return EINVAL;
    }
    pthread_mutex_lock(&timer->mutex);
    if (timer->thread_active) {
        timer->thread_active = false;
        err = pthread_join(timer->tid, NULL);
    }
    return err;
}

#define del_singleshot_timer_sync(t) del_timer_sync(t)


#endif //_QNX_LINUX_TIMER_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/timer.h $ $Rev: 838597 $")
#endif
