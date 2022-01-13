#ifndef _QNX_LINUX_AGP_X86_H
#define _QNX_LINUX_AGP_X86_H

#include <linux/io-mapping.h>
#include <asm/cacheflush.h>

/*
 * Functions to keep the agpgart mappings coherent with the MMU. The
 * GART gives the CPU a physical alias of pages in memory. The alias
 * region is mapped uncacheable. Make sure there are no conflicting
 * mappings with different cachability attributes for the same
 * page. This avoids data corruption on some CPUs.
 */

#define map_page_into_agp(page) set_pages_uc(page, 1)
#define unmap_page_from_agp(page) set_pages_wb(page, 1)

/* /\* */
/*  * Could use CLFLUSH here if the cpu supports it. But then it would */
/*  * need to be called for each cacheline of the whole page so it may */
/*  * not be worth it. Would need a page for it. */
/*  *\/ */
/* static inline void native_wbinvd(void) */
/* { */
/* 	//TODO. replace it  */
/* 	asm volatile("wbinvd": : :"memory"); */
/* } */

#define flush_agp_cache() native_wbinvd()

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/agp_x86.h $ $Rev: 845340 $")
#endif
