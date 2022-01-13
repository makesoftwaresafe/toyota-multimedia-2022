#ifndef _LINUX_HIGHMEM_H
#define _LINUX_HIGHMEM_H

#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/bug.h>
#include <linux/mm.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>

struct page;

inline static void flush_kernel_dcache_page(struct page *page)
{
}

void flush_kernel_vmap_range(void *vaddr, int size);
void invalidate_kernel_vmap_range(void *vaddr, int size);

struct page *kmap_to_page(void *vaddr);

void *kmap_atomic(struct page *page);
void *kmap(struct page *page);
void kunmap_atomic(void *addr);
void kunmap(struct page *page);

#endif /* _LINUX_HIGHMEM_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/highmem.h $ $Rev: 837534 $")
#endif
