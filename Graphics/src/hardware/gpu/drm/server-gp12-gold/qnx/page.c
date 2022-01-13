#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/mutex.h>
#include <linux/list.h>

#include "mmap_trace.h"

off64_t _vaddr_offset_qnx(void* vaddr, size_t size, size_t* contiguous_block);

off64_t _page_offset_qnx(struct page* page)
{
	off64_t paddr = 0;

	if (mem_offset64(page_address(page), NOFD, PAGE_SIZE, &paddr, NULL) == -1) {
		qnx_error("mem_offset64");
	}

	return paddr;
}

struct page* alloc_pages(gfp_t flags, unsigned order)
{
	return alloc_pages_qnx(flags, 1 << order);
}

struct page* alloc_pages_qnx(gfp_t flags, unsigned count)
{
	struct page* pages;
	void* vaddr;
	int wc = 0;
	int fd;
	int rc;
	int i, j;
	bool below_1mb = true;

	pages = kzalloc(sizeof(struct page) * count, GFP_KERNEL);
	if (pages == NULL) {
		qnx_error ("kzalloc failed");
		return NULL;
	}

	/* Check if driver request our QNX specific WC mapping by default */
	if (flags & __GFP_WC) {
		wc = SHMCTL_LAZYWRITE;
	}

	/* MG_TODO: Should we handle and respect DMA32 flag to allocate memory */
	/* in lower memory, below 4Gb? */
	if ((flags & GFP_DMA32) == GFP_DMA32) {
		/* BUG(); */
	}

	do {
		/* Allocate contiguous memory in virtual space */
		fd = shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0666);
		if (fd == -1) {
			qnx_error("shm_open() failed: %s", strerror(errno));
			kfree(pages);
			return NULL;
		}

		rc = shm_ctl(fd, SHMCTL_ANON | SHMCTL_PHYS | wc, 0, PAGE_SIZE * count);
		if (rc != 0) {
			qnx_error("shm_ctl() failed: %s", strerror(errno));
			close(fd);
			kfree(pages);
			return NULL;
		}

		vaddr = mmap64(NULL, PAGE_SIZE * count, PROT_READ | PROT_WRITE | (wc ? PROT_NOCACHE : 0), MAP_SHARED, fd, 0);
		if (vaddr == MAP_FAILED) {
			qnx_error("mmap64() failed: %s", strerror(errno));
			close(fd);
			kfree(pages);
			return NULL;
		}
		close(fd);

		for (i = 0; i < count; i++) {
			size_t contguous_block_size = 1;
			off64_t paddr;

			paddr = _vaddr_offset_qnx(vaddr + i * PAGE_SIZE, (count - i) * PAGE_SIZE, &contguous_block_size);
			if (paddr == 0) {
				/* Message was output at _vaddr_offset_qnx() function already */
				munmap(vaddr, PAGE_SIZE * count);
				kfree(pages);
				return NULL;
			}

			/* This is a special check for the memory below 1Mb of physical RAM, we should not   */
			/* accept it at any case because it seems that GPUs with embedded VGA-compatible     */
			/* display controllers treat this memory in a very specific way. For example, I did  */
			/* allocations in this memory region and it always lead to immediate GPU hangup if   */
			/* this memory was used as ring buffer. Under Linux this memory under special kernel */
			/* control and can't be easily allocated, only by using special API (for legacy).    */
			below_1mb = (paddr < 0x0000000000100000ULL);
			if (below_1mb) {
				break;
			}

			for (j = i; (j < i + contguous_block_size / PAGE_SIZE) && (j < count); j++) {
				set_page_address(&pages[j], (void*)((char*)vaddr + PAGE_SIZE * j));
				pages[j].virtual_map_mode = wc ? QNX_PAGE_MAP_WC : QNX_PAGE_MAP_WB;
				pages[j].virtual_alt = NULL;
				pages[j].virtual_alt_map_mode = QNX_PAGE_MAP_UNKNOWN;
				pages[j].offset = paddr;
				paddr += PAGE_SIZE;
			}
			i += contguous_block_size / PAGE_SIZE - 1;
		}
	} while(below_1mb);

	/* Add a trace to this memory allocation */
	mmap_trace_add_range(vaddr, PAGE_SIZE * count);

	return pages;
}

void __free_pages(struct page *pages, unsigned order)
{
	int i;
	unsigned count = 1 << order;

	for(i = 0; i < count; i++) {
		__free_page_qnx(&pages[i]);
	}

	kfree(pages);
}

void __free_page_qnx(struct page* page)
{
	int rc;

	if (page->virtual_alt) {
		mmap_trace_del_range(page->virtual_alt, PAGE_SIZE);
		rc = munmap(page->virtual_alt, PAGE_SIZE);
		if (rc) {
			qnx_error("munmap() call failed!");
		}
	}
	page->virtual_alt = NULL;
	page->virtual_alt_map_mode = QNX_PAGE_MAP_UNKNOWN;
	if (page_address(page)) {
		mmap_trace_del_range(page_address(page), PAGE_SIZE);
		rc = munmap(page_address(page), PAGE_SIZE);
		if (rc) {
			qnx_error("munmap() call failed!");
		}
	}
	set_page_address(page, NULL);
	page->virtual_map_mode = QNX_PAGE_MAP_UNKNOWN;
	page->offset = 0;
}

dma_addr_t page_to_phys(struct page *page)
{
	if ((page_address(page) == NULL) || (page->offset == 0)) {
		/* It seems we have never "attached" this page (see page_attach()) */
		BUG();
		abort();
	}

	return page->offset;
}

void page_attach(struct page* page, void* vaddr)
{
	if (!vaddr) {
		BUG_ON(!vaddr);
		abort();
	}
	set_page_address(page, vaddr);
	page->virtual_alt = NULL;
	page->offset = _page_offset_qnx(page);
	if (!page->offset) {
		BUG_ON(!page->offset);
		abort();
	}
}

void page_attach_ext(struct page* page, void* vaddr, off64_t paddr)
{
	if (!vaddr) {
		BUG_ON(!vaddr);
		abort();
	}
	set_page_address(page, vaddr);
	page->virtual_alt = NULL;
	page->offset = paddr;
	if (!page->offset) {
		BUG_ON(!page->offset);
		abort();
	}
}

unsigned long get_zeroed_page(gfp_t gfp_mask)
{
	return (unsigned long)kzalloc(PAGE_SIZE, gfp_mask);
}

struct page* _get_zeroed_page(gfp_t gfp_mask)
{
	return alloc_pages(gfp_mask, 0);
}

uint16_t __cachemode2pte_tbl[_PAGE_CACHE_MODE_NUM] = {
	[_PAGE_CACHE_MODE_WB      ]	= 0         | 0        ,
	[_PAGE_CACHE_MODE_WC      ]	= 0         | _PAGE_PCD,
	[_PAGE_CACHE_MODE_UC_MINUS]	= 0         | _PAGE_PCD,
	[_PAGE_CACHE_MODE_UC      ]	= _PAGE_PWT | _PAGE_PCD,
	[_PAGE_CACHE_MODE_WT      ]	= 0         | _PAGE_PCD,
	[_PAGE_CACHE_MODE_WP      ]	= 0         | _PAGE_PCD,
};

uint8_t __pte2cachemode_tbl[8] = {
	[__pte2cm_idx( 0        | 0         | 0        )] = _PAGE_CACHE_MODE_WB,
	[__pte2cm_idx(_PAGE_PWT | 0         | 0        )] = _PAGE_CACHE_MODE_UC_MINUS,
	[__pte2cm_idx( 0        | _PAGE_PCD | 0        )] = _PAGE_CACHE_MODE_UC_MINUS,
	[__pte2cm_idx(_PAGE_PWT | _PAGE_PCD | 0        )] = _PAGE_CACHE_MODE_UC,
	[__pte2cm_idx( 0        | 0         | _PAGE_PAT)] = _PAGE_CACHE_MODE_WB,
	[__pte2cm_idx(_PAGE_PWT | 0         | _PAGE_PAT)] = _PAGE_CACHE_MODE_UC_MINUS,
	[__pte2cm_idx(0         | _PAGE_PCD | _PAGE_PAT)] = _PAGE_CACHE_MODE_UC_MINUS,
	[__pte2cm_idx(_PAGE_PWT | _PAGE_PCD | _PAGE_PAT)] = _PAGE_CACHE_MODE_UC,
};

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/page.c $ $Rev: 874574 $")
#endif
