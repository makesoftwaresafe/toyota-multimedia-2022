#ifndef _ASM_X86_PAGE_32_H
#define _ASM_X86_PAGE_32_H

#include <asm/page_32_types.h>

#ifndef __ASSEMBLY__

#ifdef CONFIG_HUGETLB_PAGE
#define HAVE_ARCH_HUGETLB_UNMAPPED_AREA
#endif

#ifdef CONFIG_FLATMEM
#define pfn_valid(pfn)		((pfn) < max_mapnr)
#endif /* CONFIG_FLATMEM */

#ifdef CONFIG_X86_USE_3DNOW
#include <asm/mmx.h>

inline void clear_page(void *page);
void copy_page(void *to, void *from);

#else  /* !CONFIG_X86_USE_3DNOW */
#include <string.h>

static inline void clear_page(void *page)
{
	memset(page, 0, PAGE_SIZE);
}

static inline void copy_page(void *to, void *from)
{
	memcpy(to, from, PAGE_SIZE);
}
#endif	/* CONFIG_X86_3DNOW */
#endif	/* !__ASSEMBLY__ */

#endif /* _ASM_X86_PAGE_32_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/page_32.h $ $Rev: 837534 $")
#endif
