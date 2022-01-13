/*
 * Runtime locking correctness validator
 *
 *  Copyright (C) 2006,2007 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *  Copyright (C) 2007 Red Hat, Inc., Peter Zijlstra <pzijlstr@redhat.com>
 *  Some modifications Copyright (c) 2017 QNX Software Systems.
 *
 * see Documentation/locking/lockdep-design.txt for more details.
 */
#ifndef __LINUX_LOCKDEP_H
#define __LINUX_LOCKDEP_H

#define NR_LOCKDEP_CACHING_CLASSES 2
#define MAX_LOCKDEP_SUBCLASSES 8UL

#ifndef CALLER_ADDR0
#define CALLER_ADDR0 ((unsigned long)__builtin_return_address(0))
#endif

#ifndef _RET_IP_
#define _RET_IP_ CALLER_ADDR0
#endif

#ifndef _THIS_IP_
#define _THIS_IP_ ({ __label__ __here; __here: (unsigned long)&&__here; })
#endif


#define STATIC_LOCKDEP_MAP_INIT(_name, _key) \
	{ .name = (_name), .key = (void *)(_key), }


struct lockdep_subclass_key {
	char __one_byte;
};

struct lock_class_key {
	struct lockdep_subclass_key subkeys[MAX_LOCKDEP_SUBCLASSES];
};

struct lockdep_map {
	struct lock_class_key	*key;
	struct lock_class	*class_cache[NR_LOCKDEP_CACHING_CLASSES];
	const char		*name;
#ifdef CONFIG_LOCK_STAT
	int			cpu;
	unsigned long		ip;
#endif
};

/*TODO. fixme. add real imp for debugging */
static inline void lockdep_off(void)
{
}

static inline void lockdep_on(void)
{
}

# define lock_acquire(l, s, t, r, c, n, i)	do { } while (0)
# define lock_release(l, n, i)			do { } while (0)
# define lock_set_class(l, n, k, s, i)		do { } while (0)
# define lock_set_subclass(l, s, i)		do { } while (0)
# define lockdep_set_current_reclaim_state(g)	do { } while (0)
# define lockdep_clear_current_reclaim_state()	do { } while (0)
# define lockdep_trace_alloc(g)			do { } while (0)
# define lockdep_init()				do { } while (0)
# define lockdep_info()				do { } while (0)
# define lockdep_init_map(lock, name, key, sub) \
		do { (void)(name); (void)(key); } while (0)
# define lockdep_set_class(lock, key)		do { (void)(key); } while (0)
# define lockdep_set_class_and_name(lock, key, name) \
		do { (void)(key); (void)(name); } while (0)
#define lockdep_set_class_and_subclass(lock, key, sub) \
		do { (void)(key); } while (0)
#define lockdep_set_subclass(lock, sub)		do { } while (0)

#define lockdep_set_novalidate_class(lock) do { } while (0)
#define might_lock(lock) do { } while (0)
#define might_lock_read(lock) do { } while (0)

#define lockdep_assert_held(l)			do { (void)(l); } while (0)

#endif /* __LINUX_LOCKDEP_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/lockdep.h $ $Rev: 838597 $")
#endif
