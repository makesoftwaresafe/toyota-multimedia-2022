
#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/scatterlist.h>
#ifdef __QNXNTO__
#include "qnxwrapper.h"
#endif /* __QNXNTO__ */

/**
 * sg_next - return the next scatterlist entry in a list
 * @sg:		The current sg entry
 *
 * Description:
 *   Usually the next entry will be @sg@ + 1, but if this sg element is part
 *   of a chained scatterlist, it could jump to the start of a new
 *   scatterlist array.
 *
 **/
struct scatterlist *sg_next(struct scatterlist *sg)
{
#ifdef CONFIG_DEBUG_SG
	BUG_ON(sg->sg_magic != SG_MAGIC);
#endif
	if (sg_is_last(sg))
		return NULL;

	sg++;
	if (unlikely(sg_is_chain(sg)))
		sg = sg_chain_ptr(sg);

	return sg;
}

/**
 * sg_init_table - Initialize SG table
 * @sgl:	   The SG table
 * @nents:	   Number of entries in table
 *
 * Notes:
 *   If this is part of a chained sg table, sg_mark_end() should be
 *   used only on the last table part.
 *
 **/
void sg_init_table(struct scatterlist *sgl, unsigned int nents)
{
	memset(sgl, 0, sizeof(*sgl) * nents);
#ifdef CONFIG_DEBUG_SG
	{
		unsigned int i;
		for (i = 0; i < nents; i++)
			sgl[i].sg_magic = SG_MAGIC;
	}
#endif
	sg_mark_end(&sgl[nents - 1]);
}

/*
 * The default behaviour of sg_alloc_table() is to use these kmalloc/kfree
 * helpers.
 */
static struct scatterlist *sg_kmalloc(unsigned int nents, gfp_t gfp_mask)
{
#ifndef __QNXNTO__
	if (nents == SG_MAX_SINGLE_ALLOC) {
		/*
		 * Kmemleak doesn't track page allocations as they are not
		 * commonly used (in a raw form) for kernel data structures.
		 * As we chain together a list of pages and then a normal
		 * kmalloc (tracked by kmemleak), in order to for that last
		 * allocation not to become decoupled (and thus a
		 * false-positive) we need to inform kmemleak of all the
		 * intermediate allocations.
		 */
		void *ptr = (void *) __get_free_page(gfp_mask);
		kmemleak_alloc(ptr, PAGE_SIZE, 1, gfp_mask);
		return ptr;
	} else
#endif
		return kmalloc(nents * sizeof(struct scatterlist), gfp_mask);
}

static void sg_kfree(struct scatterlist *sg, unsigned int nents)
{
#ifndef __QNXNTO__
	if (nents == SG_MAX_SINGLE_ALLOC) {
		kmemleak_free(sg);
		free_page((unsigned long) sg);
	} else
#endif
		kfree(sg);
}

/**
 * __sg_free_table - Free a previously mapped sg table
 * @table:	The sg table header to use
 * @max_ents:	The maximum number of entries per single scatterlist
 * @skip_first_chunk: don't free the (preallocated) first scatterlist chunk
 * @free_fn:	Free function
 *
 *  Description:
 *    Free an sg table previously allocated and setup with
 *    __sg_alloc_table().  The @max_ents value must be identical to
 *    that previously used with __sg_alloc_table().
 *
 **/
void __sg_free_table(struct sg_table *table, unsigned int max_ents,
		     bool skip_first_chunk, sg_free_fn *free_fn)
{
	struct scatterlist *sgl, *next;

	if (unlikely(!table->sgl))
		return;

	sgl = table->sgl;
	while (table->orig_nents) {
		unsigned int alloc_size = table->orig_nents;
		unsigned int sg_size;

		/*
		 * If we have more than max_ents segments left,
		 * then assign 'next' to the sg table after the current one.
		 * sg_size is then one less than alloc size, since the last
		 * element is the chain pointer.
		 */
		if (alloc_size > max_ents) {
			next = sg_chain_ptr(&sgl[max_ents - 1]);
			alloc_size = max_ents;
			sg_size = alloc_size - 1;
		} else {
			sg_size = alloc_size;
			next = NULL;
		}

		table->orig_nents -= sg_size;
		if (skip_first_chunk)
			skip_first_chunk = false;
		else
			free_fn(sgl, alloc_size);
		sgl = next;
	}

	table->sgl = NULL;
}

/**
 * sg_free_table - Free a previously allocated sg table
 * @table:  The mapped sg table header
 *
 **/
void sg_free_table(struct sg_table *table)
{
    __sg_free_table(table, SG_MAX_SINGLE_ALLOC, false, sg_kfree);
}

/**
 * __sg_alloc_table - Allocate and initialize an sg table with given allocator
 * @table:	The sg table header to use
 * @nents:	Number of entries in sg list
 * @max_ents:	The maximum number of entries the allocator returns per call
 * @gfp_mask:	GFP allocation mask
 * @alloc_fn:	Allocator to use
 *
 * Description:
 *   This function returns a @table @nents long. The allocator is
 *   defined to return scatterlist chunks of maximum size @max_ents.
 *   Thus if @nents is bigger than @max_ents, the scatterlists will be
 *   chained in units of @max_ents.
 *
 * Notes:
 *   If this function returns non-0 (eg failure), the caller must call
 *   __sg_free_table() to cleanup any leftover allocations.
 *
 **/
int __sg_alloc_table(struct sg_table *table, unsigned int nents,
		     unsigned int max_ents, struct scatterlist *first_chunk,
		     gfp_t gfp_mask, sg_alloc_fn *alloc_fn)
{
	struct scatterlist *sg, *prv;
	unsigned int left;

	memset(table, 0, sizeof(*table));

	if (nents == 0)
		return -EINVAL;
#ifndef CONFIG_ARCH_HAS_SG_CHAIN
	if (WARN_ON_ONCE(nents > max_ents))
		return -EINVAL;
#endif

	left = nents;
	prv = NULL;
	do {
		unsigned int sg_size, alloc_size = left;

		if (alloc_size > max_ents) {
			alloc_size = max_ents;
			sg_size = alloc_size - 1;
		} else
			sg_size = alloc_size;

		left -= sg_size;

		if (first_chunk) {
			sg = first_chunk;
			first_chunk = NULL;
		} else {
			sg = alloc_fn(alloc_size, gfp_mask);
		}
		if (unlikely(!sg)) {
			/*
			 * Adjust entry count to reflect that the last
			 * entry of the previous table won't be used for
			 * linkage.  Without this, sg_kfree() may get
			 * confused.
			 */
			if (prv)
				table->nents = ++table->orig_nents;

			return -ENOMEM;
		}

		sg_init_table(sg, alloc_size);
		table->nents = table->orig_nents += sg_size;

		/*
		 * If this is the first mapping, assign the sg table header.
		 * If this is not the first mapping, chain previous part.
		 */
		if (prv)
			sg_chain(prv, max_ents, sg);
		else
			table->sgl = sg;

		/*
		 * If no more entries after this one, mark the end
		 */
		if (!left)
			sg_mark_end(&sg[sg_size - 1]);

		prv = sg;
	} while (left);

	return 0;
}

/**
 * sg_alloc_table - Allocate and initialize an sg table
 * @table:	The sg table header to use
 * @nents:	Number of entries in sg list
 * @gfp_mask:	GFP allocation mask
 *
 *  Description:
 *    Allocate and initialize an sg table. If @nents@ is larger than
 *    SG_MAX_SINGLE_ALLOC a chained sg table will be setup.
 *
 **/
int sg_alloc_table(struct sg_table *table, unsigned int nents, gfp_t gfp_mask)
{
	int ret;

	ret = __sg_alloc_table(table, nents, SG_MAX_SINGLE_ALLOC,
			       NULL, gfp_mask, sg_kmalloc);
	if (unlikely(ret))
		__sg_free_table(table, SG_MAX_SINGLE_ALLOC, false, sg_kfree);

	return ret;
}

/**
 * sg_alloc_table_from_pages - Allocate and initialize an sg table from
 *			       an array of pages
 * @sgt:	The sg table header to use
 * @pages:	Pointer to an array of page pointers
 * @n_pages:	Number of pages in the pages array
 * @offset:     Offset from start of the first page to the start of a buffer
 * @size:       Number of valid bytes in the buffer (after offset)
 * @gfp_mask:	GFP allocation mask
 *
 *  Description:
 *    Allocate and initialize an sg table from a list of pages. Contiguous
 *    ranges of the pages are squashed into a single scatterlist node. A user
 *    may provide an offset at a start and a size of valid data in a buffer
 *    specified by the page array. The returned sg table is released by
 *    sg_free_table.
 *
 * Returns:
 *   0 on success, negative error on failure
 */
int sg_alloc_table_from_pages(struct sg_table *sgt,
	struct page **pages, unsigned int n_pages,
	unsigned long offset, unsigned long size,
	gfp_t gfp_mask)
{
	unsigned int chunks;
#ifndef __QNXNTO__
	unsigned int i;
	unsigned int cur_page;
    struct scatterlist *s;
#endif
	int ret;

	chunks = 1;
#ifdef __QNXNTO__
    // use only 1 chunk since we allocate one contiguous memory block for required pages
    ret = sg_alloc_table(sgt, chunks, gfp_mask);
    if (unlikely(ret))
        return ret;

    sg_set_page(sgt->sgl, pages[0], size, offset);
#else
	/* compute number of contiguous chunks */
	for (i = 1; i < n_pages; ++i)
		if (page_to_pfn(pages[i]) != page_to_pfn(pages[i - 1]) + 1)
			++chunks;

	ret = sg_alloc_table(sgt, chunks, gfp_mask);
	if (unlikely(ret))
		return ret;

	/* merging chunks and putting them into the scatterlist */
	cur_page = 0;
	for_each_sg(sgt->sgl, s, sgt->orig_nents, i) {
		unsigned long chunk_size;
		unsigned int j;

		/* look for the end of the current chunk */
		for (j = cur_page + 1; j < n_pages; ++j)
			if (page_to_pfn(pages[j]) !=
			    page_to_pfn(pages[j - 1]) + 1)
				break;

		chunk_size = ((j - cur_page) << PAGE_SHIFT) - offset;
		sg_set_page(s, pages[cur_page], min(size, chunk_size), offset);
		size -= chunk_size;
		offset = 0;
		cur_page = j;
	}
#endif
	return 0;
}

/**
 * sg_phys - Return physical address of an sg entry
 * @sg:	     SG entry
 *
 * Description:
 *   This calls page_to_phys() on the page in this sg entry, and adds the
 *   sg offset. The caller must know that it is legal to call page_to_phys()
 *   on the sg page.
 *
 **/
dma_addr_t sg_phys(struct scatterlist *sg)
{
	return page_to_phys(sg_page(sg)) + sg->offset;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/scatterlist.c $ $Rev: 838597 $")
#endif
