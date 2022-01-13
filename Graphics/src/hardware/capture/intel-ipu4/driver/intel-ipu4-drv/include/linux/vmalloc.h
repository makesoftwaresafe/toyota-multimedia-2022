/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/vmalloc.h.
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

#ifndef _LINUX_VMALLOC_H
#define _LINUX_VMALLOC_H

#include <linux/types.h>
#include <asm/pgtable.h>
#include <linux/slab.h>
#include <linux/io.h>

struct vm_struct {
	struct vm_struct	*next;
	void			*addr;
	unsigned long		size;
	unsigned long		flags;
	struct page		**pages;
	unsigned int		nr_pages;
	phys_addr_t		phys_addr;
};

#define kmalloc_track_caller  kmalloc
#define kmap_atomic_prot(page, prot)	kmap_atomic(page)

#define kmap_atomic_pfn(pfn)	kmap_atomic(pfn_to_page(pfn))
#define kmap_atomic_to_page(ptr)	virt_to_page(ptr)

#define kmap_flush_unused()	do {} while(0)


static inline void * vzalloc(unsigned long size) {
	void *buf;
	buf = malloc(size);
	memset(buf,0,size);
    return buf;
}

static inline void * vmalloc(unsigned long size) {
    return vzalloc(size);
}

#ifndef __QNXNTO__
void *vmalloc_user(unsigned long size);
void *vmalloc_node(unsigned long size, int node);
void *vzalloc_node(unsigned long size, int node);
void *vmalloc_exec(unsigned long size);
void *vmalloc_32(unsigned long size);
void *vmalloc_32_user(unsigned long size);
#endif
static inline void vfree(void *addr)
{
     free(addr);
}

static inline void *__vmalloc(unsigned long size, gfp_t flags , pgprot_t proto) {
    return kmalloc((size_t)size, flags);
}

static inline bool is_vmalloc_addr(void *ptr) {
	return true;
}

static inline void *vmalloc_32(unsigned long size)
{
	return __vmalloc(size, GFP_KERNEL, PAGE_KERNEL);
}

static inline void *vmalloc_user(unsigned long size) {
    return vzalloc(size);
}

#define __io_virt(x) ((void __force *) (x))

static inline void
_memset_io(volatile void  *dst, int c, size_t n)
{
	volatile void  *d = dst;

	while (n--) {
		writeb(c, d);
		d++;
	}
}

#define memset_io(a, b, c)	_memset_io((a), (b), (c))
static inline void
_memcpy_fromio(void *dst, const volatile void *src, size_t n)
{
	char *d = dst;

	while (n--) {
		char tmp = readb(src);
		*d++ = tmp;
		src++;
	}
}
#define memcpy_fromio(a, b, c)	_memcpy_fromio((a), (b), (c))
static inline void
_memcpy_toio(volatile void *dst, const void *src, size_t n)
{
	const char *s = src;
	volatile void *d = dst;

	while (n--) {
		char tmp = *s++;
		writeb(tmp, d);
		d++;
	}
}
#define memcpy_toio(a, b, c)	_memcpy_toio((a), (b), (c))




/* k*map*/
static inline void kunmap(struct page *page)
{
	// FIXME: unmap page
}
static inline void __kunmap_atomic(void *addr)
{
	// FIXME: unmap page
	//pagefault_enable();
}
static inline void kunmap_atomic(void *addr)
{
	// FIXME: unmap page
	//pagefault_enable();
}

static inline void *kmemdup(const void *src, size_t len, gfp_t gfp)
{
	void *p;

	p = kmalloc_track_caller(len, gfp);
	if (p)
		memcpy(p, src, len);
	return p;
}

void *vmap(struct page **pages, unsigned int count, unsigned long flags, pgprot_t prot);
void vunmap(void *addr);

struct page *vmalloc_to_page(const void *addr);

//TODO
static inline void  vmalloc_sync_all(void)
{
}

void vm_stubs_init(void);
void vm_stubs_destroy(void);

static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;
	return kmalloc(n * size, flags);
}

#endif //_QNX_LINUX_VMALLOC_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/vmalloc.h $ $Rev: 838597 $")
#endif
