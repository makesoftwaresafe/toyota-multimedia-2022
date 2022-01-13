#ifndef _UAPI_ASM_X86_VSYSCALL_H
#define _UAPI_ASM_X86_VSYSCALL_H

enum vsyscall_num {
	__NR_vgettimeofday,
	__NR_vtime,
	__NR_vgetcpu,
};

#define VSYSCALL_ADDR (-10UL << 20)

#endif /* _UAPI_ASM_X86_VSYSCALL_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/vsyscall.h $ $Rev: 836322 $")
#endif
