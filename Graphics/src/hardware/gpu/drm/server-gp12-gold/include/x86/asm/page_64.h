#ifndef _ASM_X86_PAGE_64_H
#define _ASM_X86_PAGE_64_H

#include <uapi/linux/const.h>
#include <asm/page_64_types.h>

#ifndef __ASSEMBLY__

/* duplicated to the one in bootmem.h */
extern unsigned long max_pfn;

#define __phys_reloc_hide(x)	(x)

#ifdef CONFIG_FLATMEM
#define pfn_valid(pfn)          ((pfn) < max_pfn)
#endif

#include <string.h>

static inline void clear_page(void *page)
{
	memset(page, 0, PAGE_SIZE);
}
void copy_page(void *to, void *from);

#endif	/* !__ASSEMBLY__ */

#ifdef CONFIG_X86_VSYSCALL_EMULATION
# define __HAVE_ARCH_GATE_AREA 1
#endif

#endif /* _ASM_X86_PAGE_64_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/page_64.h $ $Rev: 837534 $")
#endif
