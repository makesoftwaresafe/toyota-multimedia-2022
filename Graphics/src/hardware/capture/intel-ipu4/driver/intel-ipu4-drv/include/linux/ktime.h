/*
 *  include/linux/ktime.h
 *
 *  ktime_t - nanosecond-resolution time format.
 *
 *   Copyright(C) 2005, Thomas Gleixner <tglx@linutronix.de>
 *   Copyright(C) 2005, Red Hat, Inc., Ingo Molnar
 *   Modified from Linux original from Yocto Linux kernel GP101 from
 *   /include/linux/ktime.h.
 *
 *  data type definitions, declarations, prototypes and macros.
 *
 *  Started by: Thomas Gleixner and Ingo Molnar
 *
 *  Credits:
 *
 *      Roman Zippel provided the ideas and primary code snippets of
 *      the ktime_t union and further simplifications of the original
 *      code.
 *
 *  For licencing details see kernel-base/COPYING
 */
#ifndef _LINUX_KTIME_H
#define _LINUX_KTIME_H

#ifdef __QNXNTO__
#include <time.h>
#include <sys/time.h>
#else
#include <linux/time.h>
#include <linux/jiffies.h>
#endif

#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

#define TIME_T_MAX	(time_t)((1UL << ((sizeof(time_t) << 3) - 1)) - 1)

/* linux ktime use nanoseconds, same as timespec. */
union ktime {
	s64	tv64;
	struct {
# ifdef __BIG_ENDIAN
	s32	sec, nsec;
# else
	s32	nsec, sec;
# endif
	} tv;
};


typedef union ktime ktime_t;		/* Kill this */

/**
 * ktime_set - Set a ktime_t variable from a seconds/nanoseconds value
 * @secs:   seconds to set
 * @nsecs:  nanoseconds to set
 *
 * Return: The ktime_t representation of the value.
 */
static inline ktime_t ktime_set(const long secs, const unsigned long nsecs)
{
	return (ktime_t) { .tv = { .sec = secs, .nsec = nsecs } };
}

static inline ktime_t
ktime_get(void){
	ktime_t kt;
	struct timespec t;
	if( clock_gettime(CLOCK_MONOTONIC, &t) == -1 ) {
		perror( "Monotonic clock gettime failed" );
	}
	kt.tv.sec = t.tv_sec;
	kt.tv.nsec = t.tv_nsec;
	return kt;
}

static inline ktime_t
ktime_get_raw(void){
	// Raw monotonic is the same as monotonic for QNX
	return ktime_get();
}

static inline struct timeval
ktime_to_timeval(ktime_t kt){
	struct timeval tval;
	tval.tv_sec = kt.tv.sec;
	tval.tv_usec = kt.tv.nsec/1000;
	return tval;
}

#define ktime_get_monotonic_offset ktime_get
#define ktime_sub(lhs, rhs) \
		({ (ktime_t){ .tv64 = (lhs).tv64 - (rhs).tv64 }; })

#define ktime_add(lhs, rhs) \
		({ (ktime_t){ .tv64 = (lhs).tv64 + (rhs).tv64 }; })

#define ktime_add_ns(kt, nsval) \
		({ (ktime_t){ .tv64 = (kt).tv64 + (nsval) }; })

#define ktime_sub_ns(kt, nsval) \
		({ (ktime_t){ .tv64 = (kt).tv64 - (nsval) }; })

#ifndef __QNXNTO__
#define ktime_to_ns(kt)			((kt).tv64)
#else
static inline s64 ktime_to_ns(const ktime_t kt)
{
	return (s64) kt.tv.sec * NSEC_PER_SEC + kt.tv.nsec;
}
#endif

#define CURRENT_TIME		(current_kernel_time())


static inline unsigned long
get_seconds(void){
	unsigned long secs = 0;
	struct timespec t;
	if( clock_gettime(CLOCK_MONOTONIC, &t) == -1 ) {
	      perror( "Monolitic clock gettime faied" );
	}else {
		secs = t.tv_sec;
	}
	return secs;
}

static inline u64 ktime_get_raw_ns(void)
{
	return ktime_to_ns(ktime_get_raw());
}

static inline struct timespec current_kernel_time(void)
{
	struct timespec t;
	if( clock_gettime(CLOCK_MONOTONIC, &t) == -1 ) {
		perror( "Monolitic clock gettime faied" );
	}
	return t;
}

#include <linux/timekeeping.h>

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/ktime.h $ $Rev: 849585 $")
#endif
