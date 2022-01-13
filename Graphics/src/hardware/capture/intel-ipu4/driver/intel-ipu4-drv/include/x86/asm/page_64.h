/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /arch/x86/include/page_64.h with __QNXNTO__ indicating modifications.
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

#ifndef _ASM_X86_PAGE_64_H
#define _ASM_X86_PAGE_64_H

#include <asm/page_64_types.h>

#ifdef __QNXNTO__
#define PHYS_OFFSET         CONFIG_DRAM_BASE
#endif

#ifndef __ASSEMBLY__

/* duplicated to the one in bootmem.h */
extern unsigned long max_pfn;
extern unsigned long phys_base;

static inline unsigned long __phys_addr_nodebug(unsigned long x)
{
	unsigned long y = x - __START_KERNEL_map;

	/* use the carry flag to determine if x was < __START_KERNEL_map */
	x = y + ((x > y) ? phys_base : (__START_KERNEL_map - PAGE_OFFSET));

	return x;
}

#ifdef CONFIG_DEBUG_VIRTUAL
extern unsigned long __phys_addr(unsigned long);
extern unsigned long __phys_addr_symbol(unsigned long);
#else
#define __phys_addr(x)		__phys_addr_nodebug(x)
#define __phys_addr_symbol(x) \
	((unsigned long)(x) - __START_KERNEL_map + phys_base)
#endif

#ifndef __QNXNTO__
#define __phys_reloc_hide(x)	(x)
#endif

#ifdef CONFIG_FLATMEM
#define pfn_valid(pfn)          ((pfn) < max_pfn)
#endif

void clear_page(void *page);
void copy_page(void *to, void *from);

#endif	/* !__ASSEMBLY__ */

#ifdef CONFIG_X86_VSYSCALL_EMULATION
# define __HAVE_ARCH_GATE_AREA 1
#endif

#endif /* _ASM_X86_PAGE_64_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/page_64.h $ $Rev: 836043 $")
#endif
