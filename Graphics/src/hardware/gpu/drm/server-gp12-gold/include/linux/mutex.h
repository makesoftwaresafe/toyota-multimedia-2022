#ifndef _QNX_LINUX_MUTEX_H
#define _QNX_LINUX_MUTEX_H

#include <pthread.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/errno.h>
#include <asm/processor.h>

struct mutex {
	pthread_mutex_t mux;
	atomic_long_t owner;
	atomic_long_t flag;
	bool destroyed;
};

#define mutex_get_owner(m) (struct task_struct *)(uintptr_t)atomic_long_read(&(m)->owner)

#define __MUTEX_INITIALIZER(mutexname)							\
	{										\
		.mux = PTHREAD_MUTEX_INITIALIZER,					\
		.owner = ATOMIC_LONG_INIT(0),						\
		.flag = ATOMIC_LONG_INIT(0),						\
		.destroyed = 0,								\
	}

#define DEFINE_MUTEX(mutexname)								\
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

struct task_struct;

#if !defined(CONFIG_TRACE_QNX_MUTEX)
	void mutex_init(struct mutex* m);
#else
	void __mutex_init(struct mutex* m, const char* s1, int s2, const char* s3);
	#define mutex_init(__mutex) __mutex_init(__mutex, __FUNCTION__, __LINE__, #__mutex)
	void __mutex_debug_dump();
#endif /* !CONFIG_TRACE_QNX_MUTEX */

void mutex_destroy(struct mutex* m);
int mutex_trylock(struct mutex* m);
int mutex_lock(struct mutex* m);
void mutex_unlock(struct mutex* m);
int mutex_is_locked(struct mutex* lock);
bool mutex_is_locked_by(struct mutex *mutex, struct task_struct *task);

#define mutex_lock_interruptible(mutex) mutex_lock(mutex)
#define mutex_lock_killable(mutex) mutex_lock(mutex)

#define mutex_lock_nested(lock, subclass) mutex_lock(lock)
#define mutex_lock_interruptible_nested(lock, subclass) mutex_lock_interruptible(lock)
#define mutex_lock_killable_nested(lock, subclass) mutex_lock_killable(lock)
#define mutex_lock_nest_lock(lock, nest_lock) mutex_lock(lock)

#define rt_mutex_lock(mutex) mutex_lock(mutex)
#define rt_mutex_trylock(mutex) mutex_trylock(mutex)
#define rt_mutex_unlock(mutex) mutex_unlock(mutex)

/*
 * These values are chosen such that FAIL and SUCCESS match the
 * values of the regular mutex_trylock().
 */
enum mutex_trylock_recursive_enum {
	MUTEX_TRYLOCK_FAILED    = 0,
	MUTEX_TRYLOCK_SUCCESS   = 1,
	MUTEX_TRYLOCK_RECURSIVE,
};

enum mutex_trylock_recursive_enum mutex_trylock_recursive(struct mutex *lock);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/mutex.h $ $Rev: 864420 $")
#endif
