#ifndef _ASM_X86_KMAP_TYPES_H
#define _ASM_X86_KMAP_TYPES_H

#if defined(CONFIG_X86_32) && defined(CONFIG_DEBUG_HIGHMEM)
#define  __WITH_KM_FENCE
#endif

#include <asm-generic/kmap_types.h>

#undef __WITH_KM_FENCE

#endif /* _ASM_X86_KMAP_TYPES_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/kmap_types.h $ $Rev: 836322 $")
#endif
