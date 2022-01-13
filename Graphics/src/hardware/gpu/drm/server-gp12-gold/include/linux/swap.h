#ifndef _LINUX_SWAP_H
#define _LINUX_SWAP_H

#include <asm/processor.h>

static inline long get_nr_swap_pages(void)
{
	return 0;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/swap.h $ $Rev: 836322 $")
#endif
