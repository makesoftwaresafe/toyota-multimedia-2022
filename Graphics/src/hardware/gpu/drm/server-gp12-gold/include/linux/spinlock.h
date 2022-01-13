#ifndef _QNX_LINUX_SPINLOCK_H
#define _QNX_LINUX_SPINLOCK_H
#define __LINUX_SPINLOCK_H

#include <linux/spinlock_types.h>
#include <linux/rwlock.h>
#include <linux/atomic.h>
#include <linux/bug.h>
#include <linux/bottom_half.h>

#ifdef __QNXNTO__
#include <linux/qnx_irqflags.h>
#endif

#define raw_spin_lock_init spin_lock_init

#if !defined(CONFIG_TRACE_QNX_MUTEX)
	void _spin_lock_init(spinlock_t* spin);
	#define spin_lock_init(spin) _spin_lock_init(spin);
#else
	void _spin_lock_init(spinlock_t* spin, const char* s1, int s2, const char* s3);
	void __mutex_debug_dump();
	#define spin_lock_init(spin) _spin_lock_init(spin, __FUNCTION__, __LINE__, #spin)
#endif /* !CONFIG_TRACE_QNX_MUTEX */

void __spin_lock_destroy(spinlock_t* spin);
#define spin_destroy(spin) __spin_lock_destroy(spin)

#define simple_spin_lock(spin) ({									\
			int __ret;								\
			__ret = pthread_spin_lock(&(spin)->spinlock);					\
			BUG_ON(__ret);								\
			unlikely(__ret);							\
		})

#define simple_spin_trylock(spin) ({ \
			int __ret;								\
			__ret = pthread_spin_trylock(&(spin)->spinlock);					\
			unlikely(__ret == EOK ? 1: 0);						\
		})

#define simple_spin_unlock(spin) ({									\
			int __ret;								\
			__ret = pthread_spin_unlock(&(spin)->spinlock);					\
			BUG_ON(__ret);								\
			unlikely(__ret);							\
		})

#define spin_unlock_wait(spin) ({								\
			spin_lock(spin);							\
			spin_unlock(spin);							\
		})

#define spin_lock(l)	do {	\
	if (!test_bit(spinlock_options_noatomic, &(l)->options)) \
		preempt_disable();	\
	simple_spin_lock(l); \
} while(0)
#define spin_unlock(l)	do {	\
	simple_spin_unlock(l);	\
	if (!test_bit(spinlock_options_noatomic, &(l)->options)) \
		preempt_enable(); \
} while(0)
#define spin_trylock(l) ({	\
	bool da = !test_bit(spinlock_options_noatomic, &(l)->options); \
	if (da) preempt_disable();	\
	simple_spin_trylock(l) ? \
	1 : ({ if (da) preempt_enable(); 0; }); })

static inline int spin_is_locked(spinlock_t *lock)
{
	struct _sync * sync = (struct _sync *)lock;
	volatile unsigned * owner = &sync->__owner;

	if (*owner == 0) {
		return 0;
	}

	return 1;
}

#define PTHREAD_SPIN_INITIALIZER(lockname)  PTHREAD_MUTEX_INITIALIZER

#define raw_spin_lock_irqsave(l,f) do { \
	if (!test_bit(spinlock_options_noatomic, &(l)->options)) \
		local_irq_save(f);	\
	spin_lock(l); \
} while(0)
#define raw_spin_unlock_irqrestore(l, f) do { \
	spin_unlock(l);	\
	if (!test_bit(spinlock_options_noatomic, &(l)->options)) \
		local_irq_restore(f); \
} while(0)
#define spin_lock_irqsave(x, y)          raw_spin_lock_irqsave(x, y)
#define spin_unlock_irqrestore(x, y)     raw_spin_unlock_irqrestore(x, y)
#define spin_lock_irq(x) do { \
	if (!test_bit(spinlock_options_noatomic, &(x)->options)) \
		local_irq_disable(); \
	spin_lock((x)); \
} while(0)
#define spin_unlock_irq(x) do { \
	spin_unlock((x)); \
	if (!test_bit(spinlock_options_noatomic, &(x)->options)) \
		local_irq_enable(); \
} while(0)

#define raw_spin_lock(x)                 spin_lock((x))
#define raw_spin_unlock(x)               spin_unlock((x))
#define raw_spin_trylock(x)              spin_trylock((x))
#define spin_lock_bh(l) do { \
	if (!test_bit(spinlock_options_noatomic, &(l)->options)) \
		__local_bh_disable_ip(_THIS_IP_, SOFTIRQ_LOCK_OFFSET);	\
	simple_spin_lock((l)); \
} while(0)
#define spin_unlock_bh(l) do { \
	simple_spin_unlock((l)); \
	if (!test_bit(spinlock_options_noatomic, &(l)->options)) \
		__local_bh_enable_ip(_THIS_IP_, SOFTIRQ_LOCK_OFFSET); \
} while(0)

#define raw_spin_trylock_irq(l) ({ \
	bool da = !test_bit(spinlock_options_noatomic, &(l)->options); \
	if (da) local_irq_disable();	\
	raw_spin_trylock(l) ? \
	1 : ({ if (da) local_irq_enable(); 0; }); })
#define spin_trylock_irq(lock) raw_spin_trylock_irq(lock)

static inline int spin_lock_nested(spinlock_t *lock, int nest)
{
    if (nest != SINGLE_DEPTH_NESTING) {
        BUG();
    }
    spin_lock(lock);
}

static inline unsigned long _raw_spin_lock_irqsave_nested(raw_spinlock_t *lock, int subclass)
{
	spin_lock(lock);
}

#define raw_spin_lock_irqsave_nested(lock, flags, subclass)		\
	do {								\
		raw_spin_lock_irqsave(lock, flags);			\
	} while (0)

#define spin_lock_irqsave_nested(lock, flags, subclass)			\
do {									\
	raw_spin_lock_irqsave_nested(lock, flags, subclass);		\
} while (0)

#define assert_spin_locked(lock) \
	BUG_ON(spin_is_locked((lock)) == 0)

// use mutex initializer
#define __SPIN_LOCK_INITIALIZER(lockname) { \
	.spinlock = PTHREAD_SPIN_INITIALIZER(lockname), \
	.options = 0, \
}
#define __SPIN_LOCK_UNLOCKED(lockname)    (spinlock_t ) __SPIN_LOCK_INITIALIZER(lockname)

#define DEFINE_SPINLOCK(x)	spinlock_t x = __SPIN_LOCK_UNLOCKED(x)

#define __RAW_SPIN_LOCK_INITIALIZER(lockname)	__SPIN_LOCK_INITIALIZER(lockname)

#define __RAW_SPIN_LOCK_UNLOCKED(lockname)	\
	(raw_spinlock_t) __RAW_SPIN_LOCK_INITIALIZER(lockname)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/spinlock.h $ $Rev: 864420 $")
#endif
