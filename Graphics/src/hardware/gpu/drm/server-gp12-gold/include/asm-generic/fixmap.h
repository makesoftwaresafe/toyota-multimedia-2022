/*
 * fixmap.h: compile-time virtual memory allocation
 *
 * This file is subject to the terms and conditions of the GNU General Public
 * License.  See the file "COPYING" in the main directory of this archive
 * for more details.
 *
 * Copyright (C) 1998 Ingo Molnar
 *
 * Support of BIGMEM added by Gerhard Wichert, Siemens AG, July 1999
 * x86_32 and x86_64 integration by Gustavo F. Padovan, February 2009
 * Break out common bits to asm-generic by Mark Salter, November 2013
 */

#ifndef __ASM_GENERIC_FIXMAP_H
#define __ASM_GENERIC_FIXMAP_H

#include <linux/bug.h>

#ifndef __ASSEMBLY__
/*
 * Provide some reasonable defaults for page flags.
 * Not all architectures use all of these different types and some
 * architectures use different names.
 */
#ifndef FIXMAP_PAGE_NORMAL
#define FIXMAP_PAGE_NORMAL PAGE_KERNEL
#endif
#if !defined(FIXMAP_PAGE_RO) && defined(PAGE_KERNEL_RO)
#define FIXMAP_PAGE_RO PAGE_KERNEL_RO
#endif
#ifndef FIXMAP_PAGE_NOCACHE
#define FIXMAP_PAGE_NOCACHE PAGE_KERNEL_NOCACHE
#endif
#ifndef FIXMAP_PAGE_IO
#define FIXMAP_PAGE_IO PAGE_KERNEL_IO
#endif
#ifndef FIXMAP_PAGE_CLEAR
#define FIXMAP_PAGE_CLEAR __pgprot(0)
#endif

#ifndef set_fixmap
#define set_fixmap(idx, phys)				\
	__set_fixmap(idx, phys, FIXMAP_PAGE_NORMAL)
#endif

#ifndef clear_fixmap
#define clear_fixmap(idx)			\
	__set_fixmap(idx, 0, FIXMAP_PAGE_CLEAR)
#endif

/* Return a pointer with offset calculated */
#define __set_fixmap_offset(idx, phys, flags)		      \
({							      \
	unsigned long addr;				      \
	__set_fixmap(idx, phys, flags);			      \
	addr = fix_to_virt(idx) + ((phys) & (PAGE_SIZE - 1)); \
	addr;						      \
})

#define set_fixmap_offset(idx, phys) \
	__set_fixmap_offset(idx, phys, FIXMAP_PAGE_NORMAL)

/*
 * Some hardware wants to get fixmapped without caching.
 */
#define set_fixmap_nocache(idx, phys) \
	__set_fixmap(idx, phys, FIXMAP_PAGE_NOCACHE)

#define set_fixmap_offset_nocache(idx, phys) \
	__set_fixmap_offset(idx, phys, FIXMAP_PAGE_NOCACHE)

/*
 * Some fixmaps are for IO
 */
#define set_fixmap_io(idx, phys) \
	__set_fixmap(idx, phys, FIXMAP_PAGE_IO)

#define set_fixmap_offset_io(idx, phys) \
	__set_fixmap_offset(idx, phys, FIXMAP_PAGE_IO)

#endif /* __ASSEMBLY__ */
#endif /* __ASM_GENERIC_FIXMAP_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/fixmap.h $ $Rev: 840419 $")
#endif
