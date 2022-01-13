#ifndef _UAPI_ASM_X86_MSR_H
#define _UAPI_ASM_X86_MSR_H

#ifndef __ASSEMBLY__

#include <linux/types.h>
#include <linux/ioctl.h>

#define X86_IOC_RDMSR_REGS	_IOWR('c', 0xA0, __u32[8])
#define X86_IOC_WRMSR_REGS	_IOWR('c', 0xA1, __u32[8])

#endif /* __ASSEMBLY__ */
#endif /* _UAPI_ASM_X86_MSR_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/msr.h $ $Rev: 836322 $")
#endif
