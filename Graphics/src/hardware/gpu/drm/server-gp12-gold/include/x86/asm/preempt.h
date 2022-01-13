#ifndef __ASM_PREEMPT_H
#define __ASM_PREEMPT_H

#include <asm/rmwcc.h>
#include <asm/percpu.h>
#include <linux/thread_info.h>
#include <asm/atomic.h>

#include <pthread.h>

extern atomic_t __preempt_count;
extern int	preempt_owner;

/*
 * We use the PREEMPT_NEED_RESCHED bit as an inverted NEED_RESCHED such
 * that a decrement hitting 0 means we can and should reschedule.
 */
#define PREEMPT_ENABLED	(0 + PREEMPT_NEED_RESCHED)

/*
 * We mask the PREEMPT_NEED_RESCHED bit so as not to confuse all current users
 * that think a non-zero value indicates we cannot preempt.
 */
#ifndef __QNXNTO__
static __always_inline int preempt_count(void)
{
	return raw_cpu_read_4(__preempt_count) & ~PREEMPT_NEED_RESCHED;
}
#endif /* __QNXNTO__ */

static __always_inline void preempt_count_set(int pc)
{
#ifdef __QNXNTO__
	BUG();	/* not implemented yet */
#else
	raw_cpu_write_4(__preempt_count, pc);
#endif /* __QNXNTO__ */
}

/*
 * must be macros to avoid header recursion hell
 */
#define init_task_preempt_count(p) do { \
	task_thread_info(p)->saved_preempt_count = PREEMPT_DISABLED; \
} while (0)

#define init_idle_preempt_count(p, cpu) do { \
	task_thread_info(p)->saved_preempt_count = PREEMPT_ENABLED; \
	per_cpu(__preempt_count, (cpu)) = PREEMPT_ENABLED; \
} while (0)

/*
 * We fold the NEED_RESCHED bit into the preempt count such that
 * preempt_enable() can decrement and test for needing to reschedule with a
 * single instruction.
 *
 * We invert the actual bit, so that when the decrement hits 0 we know we both
 * need to resched (the bit is cleared) and can resched (no preempt count).
 */

static __always_inline void set_preempt_need_resched(void)
{
}

static __always_inline void clear_preempt_need_resched(void)
{
}

static __always_inline bool test_preempt_need_resched(void)
{
	return 0;
}

#ifdef  __QNXNTO__
/*
 * only support __preepmt_count_add / sub in QNX implementation now
 */

extern void __preempt_count_add_ip(int val, unsigned long ip);
extern void __preempt_count_sub_ip(int val, unsigned long ip);
#define __preempt_count_add(v)	__preempt_count_add_ip(v, _THIS_IP_)
#define __preempt_count_sub(v)	__preempt_count_sub_ip(v, _THIS_IP_)

/*
 * preempt debug control
 */
extern atomic_t __preempt_debug_disable;
static inline void preempt_debug_enable()
{
	atomic_add(-1, &__preempt_debug_disable);
}
static inline void preempt_debug_disable()
{
	atomic_add(1, &__preempt_debug_disable);
}
#else

/*
 * The various preempt_count add/sub methods
 */

static __always_inline void __preempt_count_add(int val)
{
	raw_cpu_add_4(__preempt_count, val);
}

static __always_inline void __preempt_count_sub(int val)
{
	raw_cpu_add_4(__preempt_count, -val);
}

/*
 * Because we keep PREEMPT_NEED_RESCHED set when we do _not_ need to reschedule
 * a decrement which hits zero means we have no preempt_count and should
 * reschedule.
 */
static __always_inline bool __preempt_count_dec_and_test(void)
{
	GEN_UNARY_RMWcc("decl", __preempt_count, __percpu_arg(0), "e");
}

/*
 * Returns true when we need to resched and can (barring IRQ state).
 */
static __always_inline bool should_resched(int preempt_offset)
{
	BUG();
	return unlikely(raw_cpu_read_4(__preempt_count) == preempt_offset);
}
#endif	/* __QNXNTO__ */

#ifdef CONFIG_PREEMPT
  extern asmlinkage void ___preempt_schedule(void);
# define __preempt_schedule() asm ("call ___preempt_schedule")
  extern asmlinkage void preempt_schedule(void);
  extern asmlinkage void ___preempt_schedule_notrace(void);
# define __preempt_schedule_notrace() asm ("call ___preempt_schedule_notrace")
  extern asmlinkage void preempt_schedule_notrace(void);
#endif

#endif /* __ASM_PREEMPT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/preempt.h $ $Rev: 848997 $")
#endif
