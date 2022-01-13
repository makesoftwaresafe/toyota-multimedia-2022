#ifndef __ASM_PGALLOC_H
#define __ASM_PGALLOC_H

static inline int phys_wc_to_mtrr_index(int handle)
{
	(void)handle;
	return -1;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/pgalloc.h $ $Rev: 836322 $")
#endif
