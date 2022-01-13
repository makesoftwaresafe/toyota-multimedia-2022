#ifndef _ASM_X86_MATH_EMU_H
#define _ASM_X86_MATH_EMU_H

#include <asm/ptrace.h>

/* This structure matches the layout of the data saved to the stack
   following a device-not-present interrupt, part of it saved
   automatically by the 80386/80486.
   */
struct math_emu_info {
	long ___orig_eip;
	struct pt_regs *regs;
};
#endif /* _ASM_X86_MATH_EMU_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/math_emu.h $ $Rev: 836322 $")
#endif
