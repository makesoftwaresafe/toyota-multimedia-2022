/*
* Copyright (c) 2017 QNX Software Systems.
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

#ifndef _QNX_LINUX_SPINLOCK_H
#define _QNX_LINUX_SPINLOCK_H

#include <pthread.h>
#ifndef __QNXNTO__
#include <linux/spinlock_types.h>
#endif
#include <linux/rwlock.h>
#include <linux/atomic.h>
#include <linux/bug.h>
#include <linux/bottom_half.h>

typedef pthread_spinlock_t spinlock_t;

typedef pthread_spinlock_t raw_spinlock_t;

#define spin_lock_init(spin) do{ \
	int result_of_pthread_spin_init = \
		pthread_spin_init((spin), PTHREAD_PROCESS_PRIVATE /*PTHREAD_PROCESS_SHARED*/); \
	if(result_of_pthread_spin_init){ \
		qnx_error("pthread_spin_init failed err_code=%d", result_of_pthread_spin_init); \
	}\
	assert(result_of_pthread_spin_init == EOK); \
}while(0)

#define spin_destroy(spin) do { \
	pthread_spin_destroy((spin)); \
	}while(0)

#define spin_lock(spin) pthread_spin_lock((spin))
#define spin_trylock(spin) pthread_spin_trylock((spin))
#define spin_unlock(spin) pthread_spin_unlock((spin))

#define raw_spin_lock_irqsave(x,y)       (void)y; pthread_spin_lock((x))
#define raw_spin_unlock_irqrestore(x, y) (void)y; pthread_spin_unlock((x))
#define spin_lock_irqsave(x, y)          (void)y; pthread_spin_lock((x))
#define spin_lock_irqrestore(x, y)       (void)y; pthread_spin_lock((x))
#define spin_unlock_irqrestore(x, y)     (void)y; pthread_spin_unlock((x))
#define spin_lock_irq(x) pthread_spin_lock((x))
#define spin_unlock_irq(x) pthread_spin_unlock((x))

#define spin_lock_bh(spin) pthread_spin_lock((spin))
#define spin_unlock_bh(spin) pthread_spin_unlock((spin))


struct qnx_pthread_sync { 
	union {															
		unsigned	__count;	/* Count for recursive mutexs and semaphores */ 
		int			__fd;		/* File descriptor for a named-semaphore */	
		int			__clockid;	/* Clock for timed condvar wait */	
	} __u;															
	unsigned	__owner;		/* Thread id (valid for mutex only) */ 
};

static inline int spin_is_locked(spinlock_t *lock)
{
	struct qnx_pthread_sync * sync = (struct qnx_pthread_sync *)lock;
	volatile unsigned * count = &sync->__u.__count;
	if(*count == 0)
		return 0;
	return 1; //TODO
}


#define local_irq_save(x)
#define local_irq_restore(x)

#define assert_spin_locked(...)

#define atomic_dec_and_lock(atomic, lock) \
		__cond_lock(lock, _atomic_dec_and_lock(atomic, lock))

// use mutex initializer
#define PTHREAD_SPIN_INITIALIZER(lockname)  PTHREAD_MUTEX_INITIALIZER 
#define __SPIN_LOCK_INITIALIZER(lockname) PTHREAD_SPIN_INITIALIZER(lockname)
#define __SPIN_LOCK_UNLOCKED(lockname) \
	(spinlock_t ) __SPIN_LOCK_INITIALIZER(lockname)

#define DEFINE_SPINLOCK(x)	spinlock_t x = __SPIN_LOCK_UNLOCKED(x)

#define __RAW_SPIN_LOCK_UNLOCKED(lockname)	\
	(raw_spinlock_t) __RAW_SPIN_LOCK_INITIALIZER(lockname)

#define __RAW_SPIN_LOCK_INITIALIZER(lockname)	__SPIN_LOCK_INITIALIZER(lockname)


/*
 * This is an implementation of the notion of "decrement a
 * reference count, and return locked if it decremented to zero".
 *
 * NOTE NOTE NOTE! This is _not_ equivalent to
 *
 *	if (atomic_dec_and_test(&atomic)) {
 *		spin_lock(&lock);
 *		return 1;
 *	}
 *	return 0;
 *
 * because the spin-lock and the decrement must be
 * "atomic".
 */
static inline int _atomic_dec_and_lock(atomic_t *atomic, spinlock_t *lock)
{
	/* Subtract 1 from counter unless that drops it to 0 (ie. it was 1) */
	if (atomic_add_unless(atomic, -1, 1))
		return 0;

	/* Otherwise do it the slow way */
	spin_lock(lock);
	if (atomic_dec_and_test(atomic))
		return 1;
	spin_unlock(lock);
	return 0;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/spinlock.h $ $Rev: 838597 $")
#endif
