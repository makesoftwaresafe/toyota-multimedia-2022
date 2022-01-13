/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /arch/x86/include/page.h.
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

#ifndef _QNX_LINUX_PAGE_H
#define _QNX_LINUX_PAGE_H

#include <linux/compiler.h>
#include <asm/page_types.h>
#include <linux/list.h>

#ifdef CONFIG_X86_64
#include <asm/page_64.h>
#else
#include <asm/page_32.h>
#endif	/* CONFIG_X86_64 */

#include <linux/atomic.h>

/* qnx host apis */
#include <errno.h>
#include <sys/mman.h>
#include <stdlib.h> /* malloc */

typedef unsigned long	pteval_t;
#define _AT(T,X)	((T)(X))

#define pgprot_val(x)	((x).pgprot)
#define __pgprot(x)	((pgprot_t) { (x) } )

struct page {
	unsigned long flags;
	struct address_space *mapping;
	struct {
		union {
			unsigned long index;		/* Our offset within mapping. */
			void *freelist;		/* slub/slob first free object */
			bool pfmemalloc;	/* If set by the page allocator */
		};

		union {
			/*
			 * Keep _count separate from slub cmpxchg_double data.
			 * As the rest of the double word is protected by
			 * slab_lock but _count is not.
			 */
			unsigned counters;

			struct {

				union {
					/*
					 * Count of ptes mapped in
					 * mms, to show when page is
					 * mapped & limit reverse map
					 * searches.
					 *
					 * Used also for tail pages
					 * refcounting instead of
					 * _count. Tail pages cannot
					 * be mapped and keeping the
					 * tail page _count zero at
					 * all times guarantees
					 * get_page_unless_zero() will
					 * never succeed on tail
					 * pages.
					 */
					atomic_t _mapcount;

					struct { /* SLUB */
						unsigned inuse:16;
						unsigned objects:15;
						unsigned frozen:1;
					};
					int units;	/* SLOB */
				};
				atomic_t _count;		/* Usage count, see below. */
			};
		};
	};

	/* Third double word block */
	union {
		struct list_head lru;	/* Pageout list, eg. active_list
					 * protected by zone->lru_lock !
					 */
		struct {		/* slub per cpu partial pages */
			struct page *next;	/* Next partial slab */
			short int pages;
			short int pobjects;
		};

		struct list_head list;	/* slobs list of pages */
		struct {		/* slab fields */
			struct kmem_cache *slab_cache;
			struct slab *slab_page;
		};
	};

	/* Remainder is not double word aligned */
	union {
		unsigned long private;		/* Mapping-private opaque data:
						 * usually used for buffer_heads
						 * if PagePrivate set; used for
						 * swp_entry_t if PageSwapCache;
						 * indicates order in the buddy
						 * system if PG_buddy is set.
						 */
		struct kmem_cache *slab;	/* SLUB: Pointer to slab */
		struct page *first_page;	/* Compound tail pages */
	};

	/*
	 * On machines where all RAM is mapped into kernel address space,
	 * we can simply calculate the virtual address. On machines with
	 * highmem some memory is mapped into kernel virtual memory
	 * dynamically, so we need a place to store that address.
	 * Note that this field could be 16 bits on x86 ... ;)
	 *
	 * Architectures with slow multiplication can define
	 * WANT_PAGE_VIRTUAL in asm/page.h
	 */
	void *virtual;			/* Kernel virtual address (NULL if
					   not kmapped, ie. highmem) */
}__aligned(2 * sizeof(unsigned long));

#define page_address(page) ((page)->virtual)

static inline void *kmap_atomic(struct page *page)
{
	if(page == NULL)
		return NULL;
	return page_address(page);
}

static inline void get_page(struct page *page)
{
	atomic_inc(&page->_count);
}

static inline void put_page(struct page *page)
{
	if(atomic_dec_and_test(&page->_count) < 0)
		atomic_set(&page->_count,0);
}

#define __phys_addr_nodebug(x)	((x) - PAGE_OFFSET)
#define __phys_addr(x)		__phys_addr_nodebug(x)
#define __phys_reloc_hide(x)	RELOC_HIDE((x), 0)


#define __pa(x)		__phys_addr((unsigned long)(x))
#define __pa_nodebug(x)	__phys_addr_nodebug((unsigned long)(x))


#define PHYS_PFN_OFFSET	(PHYS_OFFSET >> PAGE_SHIFT)

#define ARCH_PFN_OFFSET		PHYS_PFN_OFFSET
extern struct page *mem_map;
#define __pfn_to_page(pfn)	(mem_map + ((pfn) - ARCH_PFN_OFFSET))
#define __page_to_pfn(page)	((unsigned long)((page) - mem_map) + ARCH_PFN_OFFSET)
#define page_to_pfn __page_to_pfn
#define pfn_to_page __pfn_to_page

#define virt_to_page(kaddr)	pfn_to_page(__pa(kaddr) >> PAGE_SHIFT)

#define offset_in_page(p)       ((unsigned long)(p) & ~PAGE_MASK)


#define page_cache_get(page)		get_page(page)
#define page_cache_release(page)	put_page(page)


static inline void *kmap(struct page *page)
{
	if(page == NULL)
		return NULL;
	return page_address(page);

}


static inline struct page *alloc_page(gfp_t flags)
{
	struct page *page = malloc(sizeof(struct page));
	if(page == NULL)
		return NULL;
	memset(page,0,sizeof(struct page));
	page->virtual = mmap( 0, PAGE_SIZE, PROT_READ | PROT_WRITE | PROT_NOCACHE,
			MAP_PRIVATE | MAP_PHYS | MAP_ANON, NOFD,0);
	return page;
}
static inline void __free_page(struct page *page)
{
	if(page->virtual) {
		munmap(page->virtual, PAGE_SIZE);
		page->virtual = NULL;
	}
	free(page);

}

static inline struct page *alloc_pages(gfp_t flags, unsigned order)
{
	int i;
	unsigned count = 1 << order;
	struct page *pages = malloc(sizeof(struct page)*count);
	if(pages == NULL)
		return NULL;

	memset(pages,0,sizeof(struct page)*count);
	for(i=0; i<count; i++){
		struct page * page = pages+i;
		page->virtual = mmap( 0, PAGE_SIZE*count, PROT_READ | PROT_WRITE | PROT_NOCACHE,
			MAP_PRIVATE | MAP_PHYS | MAP_ANON, NOFD,0);
		if(!page->virtual){
			//failed to alloc mem.
			goto fail;
		}
	}
	return pages;
 fail:
	for(i=0; i<count; i++){
		struct page * page = pages+i;
		if(!page->virtual)
			break;
		else {
			munmap(page->virtual, PAGE_SIZE);
		}
	}
	free(pages);
	return NULL;
}
static inline void __free_pages(struct page *pages, unsigned order)
{
	int i;
	unsigned count = 1 << order;
	for(i=0; i<count; i++){
		struct page * page = pages+i;
		if(page->virtual) {
			munmap(page->virtual, PAGE_SIZE);
		}
	}
	free(pages);
}

static inline int set_pages_uc(struct page *page, int numpages)
{
	return 0;
}

static inline int set_pages_wb(struct page *page, int numpages)
{
	return 0;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/page.h $ $Rev: 838597 $")
#endif
