/*
 * Mutexes: blocking mutual exclusion locks
 *
 * started by Ingo Molnar:
 *
 *  Copyright (C) 2004, 2005, 2006 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 * Some modifications Copyright (c) 2017 QNX Software Systems.
 *
 * This file contains the main data structure and API definitions.
 */
#ifndef __LINUX_MUTEX_H
#define __LINUX_MUTEX_H

#include <pthread.h>
#include <assert.h>
#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/errno.h>
#include <asm/processor.h>

extern __thread struct task_struct *current;
struct mutex {
	pthread_mutex_t mux;
	atomic_t owner;
	atomic_t flag;
};

#define rt_mutex mutex

#define mutex_get_owner(m) (struct task_struct *)atomic_read(&(m)->owner)

#define __MUTEX_INITIALIZER(mutexname)					  \
	{													  \
		.mux = PTHREAD_MUTEX_INITIALIZER,				  \
		.owner = ATOMIC_INIT(0),					  \
	}

#define DEFINE_MUTEX(mutexname)								\
	struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)
		
#define mutex_init(mutex) ({\
			int __rc = pthread_mutex_init(&((mutex)->mux), NULL);		\
			if(EOK==__rc){												\
				atomic_set(&(mutex)->owner, 0);							\
			}															\
			assert(__rc == EOK);										\
			__rc;														\
		})
#define mutex_destroy(m) ({										\
			int __rc = pthread_mutex_destroy(&((m)->mux));		\
			assert(__rc==EOK);									\
			__rc;												\
		})

struct task_struct;
int  mutex_trylock(struct mutex * m);
int  mutex_lock(struct mutex * m);
void mutex_unlock(struct mutex * m);
int mutex_is_locked(struct mutex * lock);
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

#endif /* __LINUX_MUTEX_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/mutex.h $ $Rev: 838597 $")
#endif
