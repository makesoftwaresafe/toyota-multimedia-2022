#ifndef _XEN_PAGE_H
#define _XEN_PAGE_H

#include <asm/xen/page.h>

static inline unsigned long page_to_mfn(struct page *page)
{
	return pfn_to_mfn(page_to_pfn(page));
}

struct xen_memory_region {
	phys_addr_t start;
	phys_addr_t size;
};

#define XEN_EXTRA_MEM_MAX_REGIONS 128 /* == E820MAX */

extern __initdata
struct xen_memory_region xen_extra_mem[XEN_EXTRA_MEM_MAX_REGIONS];

extern unsigned long xen_released_pages;

#endif	/* _XEN_PAGE_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/xen/page.h $ $Rev: 836322 $")
#endif
