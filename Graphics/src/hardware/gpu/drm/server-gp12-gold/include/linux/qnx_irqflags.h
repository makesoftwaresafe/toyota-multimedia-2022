#ifndef _QNX_IRQFLAGS_H_
#define _QNX_IRQFLAGS_H_
/*
 * include/linux/qnx_irqflags.h
 *
 * IRQ flags tracing:
 *
 * Basically, local cpu flags must be treat in preempt-disabled
 * in QNX arch.
 *  - Any thread can run anytime, and no way to control local
 *  cpu. Now we decide to realize local-cpu control using
 *  exclusive mutex at preempt disable declaration.
 *
 *  - local cpu flags, including irq disabled state is
 *  defined in preempt disable, exclusively other thread's
 *  local cpu access.
 *
 * Now preempt_disable() must use here, however preempt.h
 * is defined over irqflags.h in Linux source code hierarchy
 *
 * So we decide to re-define irqflags control over preempt.h
 *
 * copyright DENSO corporation, Jan-2019
 */

#include <linux/typecheck.h>
#include <linux/preempt.h>
#include <linux/bug.h>

extern atomic_t	__irq_disabled;

static inline int irqs_disabled() {
	return atomic_read(&__irq_disabled);
}

#ifdef QNX_IRQFLAGS_NOT_USED_NOW
static inline notrace unsigned long arch_local_save_flags(void)
{
	return irqs_disabled();
}
static inline int arch_irqs_disabled_flags(unsigned long flags)
{
	return !!flags;
}

#define local_save_flags(flags) do { \
	preempt_disable();	\
	flags = arch_local_save_flags();	\
	preempt_enable();	\
} while(0)
#endif

extern void arch_local_irq_enable(void);
extern void arch_local_irq_disable(void);

#define local_irq_enable()	do {	\
	int	__f;	\
	__f = atomic_add_return(-1, &__irq_disabled);	\
	if (!__f) arch_local_irq_enable();	\
	preempt_enable();	\
} while(0)
#define local_irq_disable()	({	\
	int	__f;	\
	preempt_disable();	\
	__f = atomic_add_return(1, &__irq_disabled) - 1;	\
	if (!__f) arch_local_irq_disable();	\
	__f; })

static inline unsigned long arch_local_irq_save(void)
{
	return local_irq_disable();
}
static inline void arch_local_irq_restore(unsigned long flags)
{
	local_irq_enable();
}

#define local_irq_save(flags) do {		\
	typecheck(unsigned long, flags);	\
	flags = arch_local_irq_save(); } while(0)
#define local_irq_restore(flags) do {	\
	arch_local_irq_restore(flags); } while(0)

#endif
