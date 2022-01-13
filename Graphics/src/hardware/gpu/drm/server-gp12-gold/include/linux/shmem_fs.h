#ifndef _QNX_LINUX_SHMEM_FS_H
#define _QNX_LINUX_SHMEM_FS_H

#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/scatterlist.h>
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <sys/iomsg.h>

struct file * shmem_file_setup(const char *name, loff_t size, unsigned long flags);
void shmem_truncate_range(struct inode *inode, loff_t lstart, loff_t lend);

static inline struct page* shmem_read_mapping_page_gfp(struct address_space *mapping, unsigned long index, unsigned long gfp)
{
	struct page *page;

	page = sg_page(mapping->sg->sgl + index);

	return page;
}


static inline struct page *shmem_read_mapping_page(struct address_space *mapping, unsigned long index)
{
	return shmem_read_mapping_page_gfp(mapping, index, mapping_gfp_mask(mapping));
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/shmem_fs.h $ $Rev: 874574 $")
#endif
