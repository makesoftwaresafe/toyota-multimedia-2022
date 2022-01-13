/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /arch/x86/include/page_32.h with __QNXNTO__ indicating modifications.
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

#ifndef _ASM_X86_PAGE_32_H
#define _ASM_X86_PAGE_32_H

#include <asm/page_32_types.h>

#ifdef __QNXNTO__
#define PHYS_OFFSET 		CONFIG_DRAM_BASE
#endif

#ifndef __ASSEMBLY__

#define __phys_addr_nodebug(x)	((x) - PAGE_OFFSET)
#ifdef CONFIG_DEBUG_VIRTUAL
extern unsigned long __phys_addr(unsigned long);
#else
#define __phys_addr(x)		__phys_addr_nodebug(x)
#endif
#define __phys_addr_symbol(x)	__phys_addr(x)
#define __phys_reloc_hide(x)	RELOC_HIDE((x), 0)

#ifdef CONFIG_FLATMEM
#define pfn_valid(pfn)		((pfn) < max_mapnr)
#endif /* CONFIG_FLATMEM */

#ifdef CONFIG_X86_USE_3DNOW
#include <asm/mmx.h>

static inline void clear_page(void *page)
{
	mmx_clear_page(page);
}

static inline void copy_page(void *to, void *from)
{
	mmx_copy_page(to, from);
}
#else  /* !CONFIG_X86_USE_3DNOW */
#ifdef __QNXNTO__
#include <string.h>
#else
#include <linux/string.h>
#endif

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
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/page_32.h $ $Rev: 838597 $")
#endif
