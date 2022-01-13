#ifndef _QNX_LINUX_WW_MUTEX_H
#define _QNX_LINUX_WW_MUTEX_H

#include <linux/atomic.h>
#include <linux/mutex.h>
#include <linux/lockdep.h>
#include <linux/list.h>

struct ww_mutex;
struct ww_class {
	atomic_long_t stamp;
	struct lock_class_key acquire_key;
	struct lock_class_key mutex_key;
	const char *acquire_name;
	const char *mutex_name;
};
struct ww_acquire_ctx {
	struct task_struct *task;
	unsigned long stamp;
	unsigned acquired;
	struct list_head locked_list;
};

struct ww_mutex {
	struct mutex base;
	struct ww_acquire_ctx *ctx;
	unsigned pending_stamp;
	struct list_head list;
};

#define __WW_CLASS_MUTEX_INITIALIZER(lockname, ww_class)

#define __WW_CLASS_INITIALIZER(ww_class)	{				\
		.stamp = ATOMIC_LONG_INIT(0),						\
		.acquire_name = #ww_class "_acquire",			\
			.mutex_name = #ww_class "_mutex" }

#define __WW_MUTEX_INITIALIZER(lockname, class)		  \
		{ .base = __MUTEX_INITIALIZER,				  \
		  .ctx = NULL, \
		  .pending_stamp = -1	  }

#define DEFINE_WW_CLASS(classname) \
	struct ww_class classname = __WW_CLASS_INITIALIZER(classname)

#define DEFINE_WW_MUTEX(mutexname, ww_class) \
	struct ww_mutex mutexname = __WW_MUTEX_INITIALIZER(mutexname, ww_class)

void ww_mutex_init(struct ww_mutex *lock, struct ww_class *ww_class);
void ww_mutex_destroy(struct ww_mutex *lock);
void ww_acquire_init(struct ww_acquire_ctx *ctx, struct ww_class *ww_class);
void ww_acquire_fini(struct ww_acquire_ctx *ctx);
void ww_acquire_done(struct ww_acquire_ctx *ctx);

int __ww_mutex_lock(struct ww_mutex *lock, struct ww_acquire_ctx *ctx);
void __ww_mutex_unlock(struct ww_mutex *lock);

static inline int
ww_mutex_lock(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
	if (ctx)
		return __ww_mutex_lock(lock, ctx);

	mutex_lock(&lock->base);
	return 0;
}

static inline void
ww_mutex_unlock(struct ww_mutex *lock)
{
	/*
	 * The unlocking fastpath is the 0->1 transition from 'locked'
	 * into 'unlocked' state:
	 */
	if (lock->ctx) {
		__ww_mutex_unlock(lock);
	} else {
		mutex_unlock(&lock->base);
	}
}

static inline int __must_check
ww_mutex_lock_interruptible(struct ww_mutex *lock,
		struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock(lock, ctx);
}

static inline void
ww_mutex_lock_slow(struct ww_mutex *lock, struct ww_acquire_ctx *ctx)
{
	int ret; /*TODO. optimization ?*/
	ret = ww_mutex_lock(lock, ctx);
	(void)ret;
}

static inline int __must_check
ww_mutex_lock_slow_interruptible(struct ww_mutex *lock,
		struct ww_acquire_ctx *ctx)
{
	return ww_mutex_lock_interruptible(lock, ctx);
}

static inline int __must_check
ww_mutex_trylock(struct ww_mutex *lock)
{
	return mutex_trylock(&lock->base);
}

static inline bool
ww_mutex_is_locked(struct ww_mutex *lock)
{
	return mutex_is_locked(&lock->base);
}


#endif	/* _QNX_LINUX_WW_MUTEX_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/ww_mutex.h $ $Rev: 836322 $")
#endif
