#include <linux/qnx.h>
#include <linux/linux.h>

void page_attach(struct page * page, void * vaddr);
void page_attach_ext(struct page* page, void* vaddr, off64_t paddr);

/*
 * Create scatter-list for the already allocated DMA buffer.
 */
int dma_common_get_sgtable(struct device *dev, struct sg_table *sgt,
		 void *cpu_addr, dma_addr_t handle, size_t size)
{
	/* MG_TODO: one pointer allocation??? :-/ */
	struct page *page = kmalloc(sizeof(struct page*), 0);
	int ret = 0;

	qnx_error("dma_common_get_sgtable(): non-chunked mode is not implemented!");
	BUG();
	abort();

	if (!page) {
		return -ENOMEM;
	}

	ret = sg_alloc_table(sgt, 1, GFP_KERNEL);
	if (unlikely(ret))
		return ret;

	/* MG_TODO: This code is invalid!!! */
	/* we don't support multi-chunk pages, because it require of re-porting of many */
	/* kernel functions, so historically all our objects are set of single pages.   */
	BUG();

	page_attach(page, cpu_addr);
	sg_set_page(sgt->sgl, page, PAGE_ALIGN(size), 0);

	return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/dma-mapping.c $ $Rev: 874574 $")
#endif
