#ifndef __QNX_LINUX_PAGEMAP_H
#define __QNX_LINUX_PAGEMAP_H

#include <linux/mm.h>
#include <linux/fs.h>

static inline gfp_t mapping_gfp_mask(struct address_space * mapping)
{
	return ( gfp_t)mapping->flags & ((1 << 25) - 1);
}

#define page_cache_get(page)		get_page(page)
#define page_cache_release(page)	put_page(page)

static inline void activate_page(struct page *page)
{
	SetPageActive(page);
}

/*
 * Mark a page as having seen activity.
 *
 * inactive,unreferenced	->	inactive,referenced
 * inactive,referenced		->	active,unreferenced
 * active,unreferenced		->	active,referenced
 */
static inline void mark_page_accessed(struct page *page)
{
	if (!PageActive(page) && !PageUnevictable(page) &&
			PageReferenced(page) && PageLRU(page)) {
		activate_page(page);
		ClearPageReferenced(page);
	} else if (!PageReferenced(page)) {
		SetPageReferenced(page);
	}
}

static inline void mapping_set_gfp_mask(struct address_space *m, gfp_t mask)
{
	m->flags = (m->flags & ~(__force unsigned long)__GFP_BITS_MASK) |
				(__force unsigned long)mask;
}

static inline gfp_t mapping_gfp_constraint(struct address_space *mapping, gfp_t gfp_mask)
{
	return mapping_gfp_mask(mapping) & gfp_mask;
}

static inline int fault_in_pages_readable(const char __user *uaddr, int size)
{
	return 0;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/pagemap.h $ $Rev: 837534 $")
#endif
