#ifndef _ASM_X86_PAGE_H
#define _ASM_X86_PAGE_H

#include <linux/types.h>

#ifdef __KERNEL__

#include <asm/page_types.h>

#ifdef CONFIG_X86_64
#include <asm/page_64.h>
#else
#include <asm/page_32.h>
#endif	/* CONFIG_X86_64 */

#ifndef __ASSEMBLY__

struct page;

#include <linux/range.h>
extern struct range pfn_mapped[];
extern int nr_pfn_mapped;

static inline void clear_user_page(void *page, unsigned long vaddr,
				   struct page *pg)
{
	clear_page(page);
}

static inline void copy_user_page(void *to, void *from, unsigned long vaddr,
				  struct page *topage)
{
	copy_page(to, from);
}

#define __alloc_zeroed_user_highpage(movableflags, vma, vaddr) \
	alloc_page_vma(GFP_HIGHUSER | __GFP_ZERO | movableflags, vma, vaddr)
#define __HAVE_ARCH_ALLOC_ZEROED_USER_HIGHPAGE

static inline bool __virt_addr_valid(unsigned long x)
{
	if (x != 0) {
		return true;
	}

	return false;
}

/*
 * virt_to_page(kaddr) returns a valid pointer if and only if
 * virt_addr_valid(kaddr) returns true.
 */
extern bool __virt_addr_valid(unsigned long kaddr);
#define virt_addr_valid(kaddr)	__virt_addr_valid((unsigned long) (kaddr))

#endif	/* __ASSEMBLY__ */

#include <asm-generic/memory_model.h>
#include <asm-generic/getorder.h>

#define HAVE_ARCH_HUGETLB_UNMAPPED_AREA

#endif	/* __KERNEL__ */
#endif /* _ASM_X86_PAGE_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/page.h $ $Rev: 837534 $")
#endif
