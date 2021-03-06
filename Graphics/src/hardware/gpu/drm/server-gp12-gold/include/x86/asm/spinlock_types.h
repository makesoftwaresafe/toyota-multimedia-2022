#ifndef _ASM_X86_SPINLOCK_TYPES_H
#define _ASM_X86_SPINLOCK_TYPES_H

#include <linux/types.h>

#ifdef CONFIG_PARAVIRT_SPINLOCKS
#define __TICKET_LOCK_INC	2
#define TICKET_SLOWPATH_FLAG	((__ticket_t)1)
#else
#define __TICKET_LOCK_INC	1
#define TICKET_SLOWPATH_FLAG	((__ticket_t)0)
#endif

#if (CONFIG_NR_CPUS < (256 / __TICKET_LOCK_INC))
typedef u8  __ticket_t;
typedef u16 __ticketpair_t;
#else
typedef u16 __ticket_t;
typedef u32 __ticketpair_t;
#endif

#define TICKET_LOCK_INC	((__ticket_t)__TICKET_LOCK_INC)

#define TICKET_SHIFT	(sizeof(__ticket_t) * 8)

#ifdef CONFIG_QUEUED_SPINLOCKS
#include <asm-generic/qspinlock_types.h>
#else
typedef struct arch_spinlock {
	union {
		__ticketpair_t head_tail;
		struct __raw_tickets {
			__ticket_t head, tail;
		} tickets;
	};
} arch_spinlock_t;

#define __ARCH_SPIN_LOCK_UNLOCKED	{ { 0 } }
#endif /* CONFIG_QUEUED_SPINLOCKS */

#include <asm-generic/qrwlock_types.h>

#endif /* _ASM_X86_SPINLOCK_TYPES_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/spinlock_types.h $ $Rev: 836322 $")
#endif
