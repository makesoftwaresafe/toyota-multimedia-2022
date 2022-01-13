#ifndef _ASM_X86_PAGE_32_DEFS_H
#define _ASM_X86_PAGE_32_DEFS_H

#include <linux/const.h>

#define THREAD_SIZE_ORDER	1
#define THREAD_SIZE		(PAGE_SIZE << THREAD_SIZE_ORDER)

#define DOUBLEFAULT_STACK 1
#define NMI_STACK 0
#define DEBUG_STACK 0
#define MCE_STACK 0
#define N_EXCEPTION_STACKS 1

#ifdef CONFIG_X86_PAE
/* 44=32+12, the limit we can fit into an unsigned long pfn */
#define __PHYSICAL_MASK_SHIFT	44
#define __VIRTUAL_MASK_SHIFT	32

#else  /* !CONFIG_X86_PAE */
#define __PHYSICAL_MASK_SHIFT	32
#define __VIRTUAL_MASK_SHIFT	32
#endif	/* CONFIG_X86_PAE */

/*
 * Kernel image size is limited to 512 MB (see in arch/x86/kernel/head_32.S)
 */
#define KERNEL_IMAGE_SIZE	(512 * 1024 * 1024)

#ifndef __ASSEMBLY__

/*
 * This much address space is reserved for vmalloc() and iomap()
 * as well as fixmap mappings.
 */
extern unsigned int __VMALLOC_RESERVE;
extern int sysctl_legacy_va_layout;

extern void find_low_pfn_range(void);
extern void setup_bootmem_allocator(void);

#endif	/* !__ASSEMBLY__ */

#endif /* _ASM_X86_PAGE_32_DEFS_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/page_32_types.h $ $Rev: 837534 $")
#endif
