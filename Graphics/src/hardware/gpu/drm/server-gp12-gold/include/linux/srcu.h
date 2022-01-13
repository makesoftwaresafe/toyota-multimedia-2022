#ifndef _QNX_LINUX_SRCU_H
#define _QNX_LINUX_SRCU_H

struct srcu_struct_array {
        unsigned long c[2];
        unsigned long seq[2];
};

struct srcu_struct {
        unsigned completed;
        struct srcu_struct_array __percpu *per_cpu_ref;
        spinlock_t queue_lock; /* protect ->batch_queue, ->running */
        bool running;
        /* callbacks just queued */
//        struct rcu_batch batch_queue;
        /* callbacks try to do the first check_zero */
//        struct rcu_batch batch_check0;
        /* callbacks done with the first check_zero and the flip */
        //struct rcu_batch batch_check1;
//        struct rcu_batch batch_done;
//        struct delayed_work work;
};


#if 0 //TODO
#define init_srcu_struct(sp) \
({ \
	static struct lock_class_key __srcu_key; \
	\
	__init_srcu_struct((sp), #sp, &__srcu_key); \
})

#else
static inline int init_srcu_struct(struct srcu_struct *sp){
	return 0;
}

#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/srcu.h $ $Rev: 836322 $")
#endif
