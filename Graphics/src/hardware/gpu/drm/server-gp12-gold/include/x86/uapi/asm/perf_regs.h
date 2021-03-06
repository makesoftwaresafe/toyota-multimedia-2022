#ifndef _ASM_X86_PERF_REGS_H
#define _ASM_X86_PERF_REGS_H

enum perf_event_x86_regs {
	PERF_REG_X86_AX,
	PERF_REG_X86_BX,
	PERF_REG_X86_CX,
	PERF_REG_X86_DX,
	PERF_REG_X86_SI,
	PERF_REG_X86_DI,
	PERF_REG_X86_BP,
	PERF_REG_X86_SP,
	PERF_REG_X86_IP,
	PERF_REG_X86_FLAGS,
	PERF_REG_X86_CS,
	PERF_REG_X86_SS,
	PERF_REG_X86_DS,
	PERF_REG_X86_ES,
	PERF_REG_X86_FS,
	PERF_REG_X86_GS,
	PERF_REG_X86_R8,
	PERF_REG_X86_R9,
	PERF_REG_X86_R10,
	PERF_REG_X86_R11,
	PERF_REG_X86_R12,
	PERF_REG_X86_R13,
	PERF_REG_X86_R14,
	PERF_REG_X86_R15,

	PERF_REG_X86_32_MAX = PERF_REG_X86_GS + 1,
	PERF_REG_X86_64_MAX = PERF_REG_X86_R15 + 1,
};
#endif /* _ASM_X86_PERF_REGS_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/perf_regs.h $ $Rev: 836322 $")
#endif
