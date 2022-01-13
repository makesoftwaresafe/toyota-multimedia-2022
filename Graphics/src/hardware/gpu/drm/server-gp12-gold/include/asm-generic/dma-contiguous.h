#ifndef _ASM_GENERIC_DMA_CONTIGUOUS_H
#define _ASM_GENERIC_DMA_CONTIGUOUS_H

#include <linux/types.h>

static inline void
dma_contiguous_early_fixup(phys_addr_t base, unsigned long size) { }

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/dma-contiguous.h $ $Rev: 836322 $")
#endif
