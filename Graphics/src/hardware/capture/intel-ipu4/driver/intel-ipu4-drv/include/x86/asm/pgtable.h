/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /arch/x86/include/asm/pgtable_types.h.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _QNX_LINUX_PGTABLE_H
#define _QNX_LINUX_PGTABLE_H

#include <linux/types.h>
#include <asm/page.h>

#define FIRST_USER_ADDRESS	0UL

//x86 only
#define _PAGE_BIT_PRESENT	0	/* is present */
#define _PAGE_BIT_RW		1	/* writeable */
#define _PAGE_BIT_USER		2	/* userspace addressable */
#define _PAGE_BIT_PWT		3	/* page write through */
#define _PAGE_BIT_PCD		4	/* page cache disabled */
#define _PAGE_BIT_ACCESSED	5	/* was accessed (raised by CPU) */
#define _PAGE_BIT_DIRTY		6	/* was written to (raised by CPU) */
#define _PAGE_BIT_PSE		7	/* 4 MB (or 2MB) page */
#define _PAGE_BIT_PAT		7	/* on 4KB pages */
#define _PAGE_BIT_GLOBAL	8	/* Global TLB entry PPro+ */
#define _PAGE_BIT_SOFTW1	9	/* available for programmer */
#define _PAGE_BIT_SOFTW2	10	/* " */
#define _PAGE_BIT_SOFTW3	11	/* " */
#define _PAGE_BIT_PAT_LARGE	12	/* On 2MB or 1GB pages */
#define _PAGE_BIT_SPECIAL	_PAGE_BIT_SOFTW1
#define _PAGE_BIT_CPA_TEST	_PAGE_BIT_SOFTW1
#define _PAGE_BIT_SPLITTING	_PAGE_BIT_SOFTW2 /* only valid on a PSE pmd */
#define _PAGE_BIT_HIDDEN	_PAGE_BIT_SOFTW3 /* hidden by kmemcheck */
#define _PAGE_BIT_SOFT_DIRTY	_PAGE_BIT_SOFTW3 /* software dirty tracking */
#define _PAGE_BIT_NX           63       /* No execute: only valid after cpuid check */

/* If _PAGE_BIT_PRESENT is clear, we use these: */
/* - if the user mapped it with PROT_NONE; pte_present gives true */
#define _PAGE_BIT_PROTNONE	_PAGE_BIT_GLOBAL

#define _PAGE_PRESENT	(_AT(pteval_t, 1) << _PAGE_BIT_PRESENT)
#define _PAGE_RW	(_AT(pteval_t, 1) << _PAGE_BIT_RW)
#define _PAGE_USER	(_AT(pteval_t, 1) << _PAGE_BIT_USER)
#define _PAGE_PWT	(_AT(pteval_t, 1) << _PAGE_BIT_PWT)
#define _PAGE_PCD	(_AT(pteval_t, 1) << _PAGE_BIT_PCD)
#define _PAGE_ACCESSED	(_AT(pteval_t, 1) << _PAGE_BIT_ACCESSED)
#define _PAGE_DIRTY	(_AT(pteval_t, 1) << _PAGE_BIT_DIRTY)
#define _PAGE_PSE	(_AT(pteval_t, 1) << _PAGE_BIT_PSE)
#define _PAGE_GLOBAL	(_AT(pteval_t, 1) << _PAGE_BIT_GLOBAL)
#define _PAGE_SOFTW1	(_AT(pteval_t, 1) << _PAGE_BIT_SOFTW1)
#define _PAGE_SOFTW2	(_AT(pteval_t, 1) << _PAGE_BIT_SOFTW2)
#define _PAGE_PAT	(_AT(pteval_t, 1) << _PAGE_BIT_PAT)
#define _PAGE_PAT_LARGE (_AT(pteval_t, 1) << _PAGE_BIT_PAT_LARGE)
#define _PAGE_SPECIAL	(_AT(pteval_t, 1) << _PAGE_BIT_SPECIAL)
#define _PAGE_CPA_TEST	(_AT(pteval_t, 1) << _PAGE_BIT_CPA_TEST)
#define _PAGE_SPLITTING	(_AT(pteval_t, 1) << _PAGE_BIT_SPLITTING)
#define __HAVE_ARCH_PTE_SPECIAL

/* - set: nonlinear file mapping, saved PTE; unset:swap */
#define _PAGE_BIT_FILE		_PAGE_BIT_DIRTY

#define _PAGE_PRESENT	(_AT(pteval_t, 1) << _PAGE_BIT_PRESENT)
#define _PAGE_RW	(_AT(pteval_t, 1) << _PAGE_BIT_RW)
#define _PAGE_USER      (_AT(pteval_t, 1) << _PAGE_BIT_USER)
#define _PAGE_PWT	(_AT(pteval_t, 1) << _PAGE_BIT_PWT)
#define _PAGE_PCD	(_AT(pteval_t, 1) << _PAGE_BIT_PCD)
#define _PAGE_ACCESSED	(_AT(pteval_t, 1) << _PAGE_BIT_ACCESSED)
#define _PAGE_DIRTY	(_AT(pteval_t, 1) << _PAGE_BIT_DIRTY)
#define _PAGE_PSE       (_AT(pteval_t, 1) << _PAGE_BIT_PSE)
#define _PAGE_GLOBAL	(_AT(pteval_t, 1) << _PAGE_BIT_GLOBAL)
#define _PAGE_NX	(_AT(pteval_t, 0))
#define _PAGE_PROTNONE  (_AT(pteval_t, 1) << _PAGE_BIT_PROTNONE)

#define _PAGE_CACHE_WC          (_PAGE_PWT)

#define __PAGE_KERNEL_EXEC	(_PAGE_PRESENT | _PAGE_RW | _PAGE_DIRTY | _PAGE_ACCESSED | _PAGE_GLOBAL)
#define __PAGE_KERNEL	(__PAGE_KERNEL_EXEC | _PAGE_NX)

#define PAGE_KERNEL   __pgprot(__PAGE_KERNEL)

#define PAGE_NONE      			__pgprot(_PAGE_PROTNONE | _PAGE_ACCESSED)
#define PAGE_SHARED  			__pgprot(_PAGE_PRESENT | _PAGE_RW | _PAGE_USER | \
                                 _PAGE_ACCESSED | _PAGE_NX)
#define PAGE_SHARED_EXEC        __pgprot(_PAGE_PRESENT | _PAGE_RW |     \
                                        _PAGE_USER | _PAGE_ACCESSED)
#define PAGE_COPY_NOEXEC        __pgprot(_PAGE_PRESENT | _PAGE_USER |   \
                                         _PAGE_ACCESSED | _PAGE_NX)
#define PAGE_COPY_EXEC          __pgprot(_PAGE_PRESENT | _PAGE_USER |   \
                                         _PAGE_ACCESSED)
#define PAGE_COPY               PAGE_COPY_NOEXEC
#define PAGE_READONLY           __pgprot(_PAGE_PRESENT | _PAGE_USER |   \
                                         _PAGE_ACCESSED | _PAGE_NX)
#define PAGE_READONLY_EXEC      __pgprot(_PAGE_PRESENT | _PAGE_USER |   \
                                         _PAGE_ACCESSED)

#define SHMLBA PAGE_SIZE	 /* attach addr a multiple of this */


/*         xwr */
#define __P000	PAGE_NONE
#define __P001	PAGE_READONLY
#define __P010	PAGE_COPY
#define __P011	PAGE_COPY
#define __P100	PAGE_READONLY_EXEC
#define __P101	PAGE_READONLY_EXEC
#define __P110	PAGE_COPY_EXEC
#define __P111	PAGE_COPY_EXEC

#define __S000	PAGE_NONE
#define __S001	PAGE_READONLY
#define __S010	PAGE_SHARED
#define __S011	PAGE_SHARED
#define __S100	PAGE_READONLY_EXEC
#define __S101	PAGE_READONLY_EXEC
#define __S110	PAGE_SHARED_EXEC
#define __S111	PAGE_SHARED_EXEC

#define pat_enabled  1

#ifndef arch_vm_get_page_prot
#define arch_vm_get_page_prot(vm_flags) __pgprot(0)//FIXME: ...
#endif

typedef struct page *pgtable_t;
#define pgprot_noncached(prot) (prot)

static inline pgprot_t 
pgprot_writecombine(pgprot_t prot)
{
	if (pat_enabled)
		return __pgprot(pgprot_val(prot) | _PAGE_CACHE_WC);
	else
		return pgprot_noncached(prot);
}

#define io_remap_pfn_range(vma, vaddr, pfn, size, prot)	\
	remap_pfn_range(vma, vaddr, pfn, size, prot)


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/pgtable.h $ $Rev: 838597 $")
#endif
