#include <linux/qnx.h>
#include <linux/dma-buf.h>
#include <linux/linux.h>
#include <inttypes.h>

#include "mmap_trace.h"

void drm_clflush_virt_range(void *addr, unsigned long length);

void dma_sync_single_for_device(struct device *dev, dma_addr_t handle,
				size_t size, enum dma_data_direction dir)
{
	void* ptr = (void*)(uintptr_t)handle;

	switch (dir) {
		case DMA_BIDIRECTIONAL:
		case DMA_TO_DEVICE:
		case DMA_FROM_DEVICE:
			fprintf(stderr, "dma_sync_single_for_device() is not supported under QNX!\n");
			BUG();
			break;
		default:
			break;
	}
}

void dma_sync_single_for_device_virtual(struct device *dev, void* addr, size_t size, enum dma_data_direction dir)
{
	switch (dir) {
		case DMA_BIDIRECTIONAL:
		case DMA_TO_DEVICE:
		case DMA_FROM_DEVICE:
			drm_clflush_virt_range(addr, size);
			break;
		default:
			break;
	}
}

extern const struct file_operations dma_buf_fops;
int is_dma_buf_file(struct file *file)
{
        return file->f_op == &dma_buf_fops;
}

static int dma_buf_release(struct inode *inode, struct file *file)
{
        struct dma_buf *dmabuf;
        if (!is_dma_buf_file(file))
               return -EINVAL;

        dmabuf = file->private_data;

        dmabuf->ops->release(dmabuf);
        kfree(dmabuf);
        return 0;
}

static int dma_buf_mmap_internal(struct file *file, struct vm_area_struct *vma)
{
       struct dma_buf *dmabuf;

       if (!is_dma_buf_file(file))
                return -EINVAL;

       dmabuf = file->private_data;
       /* check for overflowing the buffer's size */
       if (vma->vm_pgoff + ((vma->vm_end - vma->vm_start) >> PAGE_SHIFT) >
            dmabuf->size >> PAGE_SHIFT)
                return -EINVAL;

        return dmabuf->ops->mmap(dmabuf, vma);
}

const struct file_operations dma_buf_fops = {
	.release        = dma_buf_release,
	.mmap           = dma_buf_mmap_internal,
};

int dma_map_sg(struct device *dev, struct scatterlist *sglist, int nents, enum dma_data_direction direction)
{
	struct scatterlist *sg;
	struct page* pg;
	int i;

	pg = sg_page(sglist);
	if (page_address(pg)) {
		if (pg->virtual_map_mode == QNX_PAGE_MAP_WB) {
			/* Flush cache only if this memory was mapped as WB */
			dma_sync_single_for_device_virtual(dev, page_address(pg), sglist->length * nents, direction);
		}
	} else {
		/* Primary mapping must be always exist */
		return 0;
	}

	/* Alternative mapping is optional */
	if (pg->virtual_alt && pg->virtual_alt_map_mode == QNX_PAGE_MAP_WB) {
		/* Flush cache only if this memory was mapped as WB */
		dma_sync_single_for_device_virtual(dev, pg->virtual_alt, sglist->length * nents, direction);
	}

	if (!sglist->dma_address) {
		for_each_sg(sglist, sg, nents, i) {
			sg->dma_address = sg_phys(sg);
			sg->dma_length = sg->length;
		}
	}

	return nents;
}

dma_addr_t dma_map_page(struct device *dev, struct page *page,
	size_t offset, size_t size, enum dma_data_direction dir)
{
	dma_addr_t handle = page_to_phys(page) + offset;
	void* vaddr;

	vaddr = page_address(page) + offset;

	dma_sync_single_for_device_virtual(dev, vaddr, size, dir);
	return handle;
}

void dma_unmap_page(struct device *dev, dma_addr_t addr,
	size_t size, enum dma_data_direction dir)
{
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/dma.c $ $Rev: 874574 $")
#endif
