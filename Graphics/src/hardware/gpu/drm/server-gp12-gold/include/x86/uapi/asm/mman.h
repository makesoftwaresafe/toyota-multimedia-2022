#ifndef _ASM_X86_MMAN_H
#define _ASM_X86_MMAN_H

#define MAP_32BIT	0x40		/* only give out 32bit addresses */

#define MAP_HUGE_2MB    (21 << MAP_HUGE_SHIFT)
#define MAP_HUGE_1GB    (30 << MAP_HUGE_SHIFT)

#include <asm-generic/mman.h>

#endif /* _ASM_X86_MMAN_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/uapi/asm/mman.h $ $Rev: 836322 $")
#endif
