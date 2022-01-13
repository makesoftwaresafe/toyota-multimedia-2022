#ifndef _ASM_GENERIC_KMAP_TYPES_H
#define _ASM_GENERIC_KMAP_TYPES_H

#ifdef __WITH_KM_FENCE
# define KM_TYPE_NR 41
#else
# define KM_TYPE_NR 20
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/kmap_types.h $ $Rev: 836322 $")
#endif
