/*
 * Sleepable Read-Copy Update mechanism for mutual exclusion
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * Copyright (C) IBM Corporation, 2006
 * Copyright (C) Fujitsu, 2012
 * Some modifications Copyright (c) 2017 QNX Software Systems.
 *
 * Author: Paul McKenney <paulmck@us.ibm.com>
 *     Lai Jiangshan <laijs@cn.fujitsu.com>
 *
 * For detailed explanation of Read-Copy Update mechanism see -
 *      Documentation/RCU/ *.txt
 *
 */

#ifndef _LINUX_SRCU_H
#define _LINUX_SRCU_H

struct srcu_struct_array {
        unsigned long c[2];
        unsigned long seq[2];
};

struct srcu_struct {
        unsigned completed;
        struct srcu_struct_array __percpu *per_cpu_ref;
        spinlock_t queue_lock; /* protect ->batch_queue, ->running */
        bool running;
#ifndef __QNXNTO__
        /* callbacks just queued */
        struct rcu_batch batch_queue;
        /* callbacks try to do the first check_zero */
        struct rcu_batch batch_check0;
        /* callbacks done with the first check_zero and the flip */
        struct rcu_batch batch_check1;
        struct rcu_batch batch_done;
        struct delayed_work work;
#endif
};


#ifndef __QNXNTO__
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
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/srcu.h $ $Rev: 838597 $")
#endif
