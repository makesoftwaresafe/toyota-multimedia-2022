/*
 * Copyright (C) 1994 Linus Torvalds
 *
 * Pentium III FXSR, SSE support
 * General FPU state handling cleanups
 *	Gareth Hughes <gareth@valinux.com>, May 2000
 * x86-64 work by Andi Kleen 2002
 */

#ifndef _ASM_X86_FPU_API_H
#define _ASM_X86_FPU_API_H

/*
 * Careful: __kernel_fpu_begin/end() must be called with preempt disabled
 * and they don't touch the preempt state on their own.
 * If you enable preemption after __kernel_fpu_begin(), preempt notifier
 * should call the __kernel_fpu_end() to prevent the kernel/user FPU
 * state from getting corrupted. KVM for example uses this model.
 *
 * All other cases use kernel_fpu_begin/end() which disable preemption
 * during kernel FPU usage.
 */
static inline void __kernel_fpu_begin(void)
{
}

static inline void __kernel_fpu_end(void)
{
}

static inline void kernel_fpu_begin(void)
{
	__kernel_fpu_begin();
}

static inline void kernel_fpu_end(void)
{
	__kernel_fpu_end();
}

extern bool irq_fpu_usable(void);

/*
 * Some instructions like VIA's padlock instructions generate a spurious
 * DNA fault but don't modify SSE registers. And these instructions
 * get used from interrupt context as well. To prevent these kernel instructions
 * in interrupt context interacting wrongly with other user/kernel fpu usage, we
 * should use them only in the context of irq_ts_save/restore()
 */
extern int  irq_ts_save(void);
extern void irq_ts_restore(int TS_state);

/*
 * Query the presence of one or more xfeatures. Works on any legacy CPU as well.
 *
 * If 'feature_name' is set then put a human-readable description of
 * the feature there as well - this can be used to print error (or success)
 * messages.
 */
extern int cpu_has_xfeatures(u64 xfeatures_mask, const char **feature_name);

#endif /* _ASM_X86_FPU_API_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/fpu/api.h $ $Rev: 836322 $")
#endif
