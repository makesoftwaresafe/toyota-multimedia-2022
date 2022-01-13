/*
 * Copyright (c) 2013--2016 Intel Corporation.
 * Some modifications (__QNXNTO__) Copyright (c) 2017 QNX Software Systems.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License version
 * 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <asm/cacheflush.h>

#ifndef __QNXNTO__
#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/gfp.h>
#include <linux/highmem.h>
#include <linux/iommu.h>
#include <linux/iova.h>
#include <linux/module.h>
#include <linux/scatterlist.h>
#include <linux/vmalloc.h>
#else
#include <linux/qnx.h>
#include <linux/linux.h>
#include "qnxwrapper.h"
#endif /* __QNXNTO__ */

#include "intel-ipu4-dma.h"
#include "intel-ipu4-mmu.h"

#ifdef __QNXNTO__
typedef struct qnx_vm_area_node{
    struct qnx_vm_area_node*        next;
    struct page**                   pages;
    void*                           vaddr;
} qnx_vm_area_node_t;

static qnx_vm_area_node_t*          qnx_vm_areas;
#endif
/* Begin of things adapted from arch/arm/mm/dma-mapping.c */
static void __dma_clear_buffer(struct page *page, size_t size,
			       struct dma_attrs *attrs)
{
	/*
	 * Ensure that the allocated pages are zeroed, and that any data
	 * lurking in the kernel direct-mapped region is invalidated.
	 */
	if (PageHighMem(page)) {
		while (size > 0) {
			void *ptr = kmap_atomic(page);

			memset(ptr, 0, PAGE_SIZE);
			if (!dma_get_attr(DMA_ATTR_SKIP_CPU_SYNC, attrs))
				clflush_cache_range(ptr, PAGE_SIZE);
			kunmap_atomic(ptr);
			page++;
			size -= PAGE_SIZE;
		}
	} else {
		void *ptr = page_address(page);

		memset(ptr, 0, size);
		if (!dma_get_attr(DMA_ATTR_SKIP_CPU_SYNC, attrs))
			clflush_cache_range(ptr, size);
	}
}

static struct page **__iommu_alloc_buffer(struct device *dev, size_t size,
					  gfp_t gfp, struct dma_attrs *attrs)
{
	struct page **pages;
	int count = size >> PAGE_SHIFT;
	int array_size = count * sizeof(struct page *);
	int i = 0;

	if (array_size <= PAGE_SIZE)
		pages = kzalloc(array_size, GFP_KERNEL);
	else
		pages = vzalloc(array_size);
	if (!pages)
		return NULL;

#ifdef __QNXNTO__
    pages[0] = malloc(sizeof(struct page)*count);
    if (pages[0] == NULL) {
        goto error;
    }
    pages[0]->virtual = mmap( 0, PAGE_SIZE*count, PROT_READ | PROT_WRITE | PROT_NOCACHE,
                              MAP_PRIVATE | MAP_PHYS | MAP_ANON, NOFD, 0);
    if (pages[0]->virtual == NULL) {
        goto error;
    }

    __dma_clear_buffer(pages[0], PAGE_SIZE*count, attrs);
    for (i = 1; i < count; i++) {
        pages[i] = pages[0] + i;
        pages[i]->virtual = pages[0]->virtual + PAGE_SIZE*i;
    }
#else
	gfp |= __GFP_NOWARN;

	while (count) {
		int j, order = __fls(count);

		pages[i] = alloc_pages(gfp, order);
		while (!pages[i] && order)
			pages[i] = alloc_pages(gfp, --order);
		if (!pages[i])
			goto error;

		if (order) {
			split_page(pages[i], order);
			j = 1 << order;
			while (--j)
				pages[i + j] = pages[i] + j;
		}

		__dma_clear_buffer(pages[i], PAGE_SIZE << order, attrs);
		i += 1 << order;
		count -= 1 << order;
	}
#endif
	return pages;
error:
#ifndef __QNXNTO__
	while (i--)
		if (pages[i])
			__free_pages(pages[i], 0);
#else
    if (pages[0]) {
        if(pages[0]->virtual) {
            munmap(pages[0]->virtual, PAGE_SIZE*count);
        }
        free(pages[0]);
    }
#endif
	if (array_size <= PAGE_SIZE)
		kfree(pages);
	else
		vfree(pages);
	return NULL;
}

static int __iommu_free_buffer(struct device *dev, struct page **pages,
			       size_t size, struct dma_attrs *attrs)
{
	int count = size >> PAGE_SHIFT;
	int array_size = count * sizeof(struct page *);
	int i;

#ifdef __QNXNTO__
    if (pages[0]->virtual) {
        __dma_clear_buffer(pages[0], PAGE_SIZE*count, attrs);
        munmap(pages[0]->virtual, PAGE_SIZE*count);
        for (i = 0; i < count; i++) {
            qnxw_remove_addr_from_table(pages[i]->virtual);
        }
        free(pages[0]);
    }
#else
	for (i = 0; i < count; i++) {
		if (pages[i]) {
			__dma_clear_buffer(pages[i], PAGE_SIZE, attrs);
			__free_pages(pages[i], 0);
		}
	}
#endif

	if (array_size <= PAGE_SIZE)
		kfree(pages);
	else
		vfree(pages);
	return 0;
}

/* End of things adapted from arch/arm/mm/dma-mapping.c */

static void intel_ipu4_dma_sync_single_for_cpu(
	struct device *dev, dma_addr_t dma_handle, size_t size,
	enum dma_data_direction dir)
{
	struct device *aiommu = to_intel_ipu4_bus_device(dev)->iommu;
	struct intel_ipu4_mmu *mmu = dev_get_drvdata(aiommu);

	clflush_cache_range(
		phys_to_virt(iommu_iova_to_phys(
				     mmu->dmap->domain, dma_handle)), size);
}

static void intel_ipu4_dma_sync_sg_for_cpu(
	struct device *dev, struct scatterlist *sglist, int nents,
	enum dma_data_direction dir)
{
	struct device *aiommu = to_intel_ipu4_bus_device(dev)->iommu;
	struct intel_ipu4_mmu *mmu = dev_get_drvdata(aiommu);
	struct scatterlist *sg;
	int i;

	for_each_sg(sglist, sg, nents, i) {
		clflush_cache_range(
			phys_to_virt(iommu_iova_to_phys(
					     mmu->dmap->domain,
					     sg_dma_address(sg))),
			sg->length);
	}
}

static void *intel_ipu4_dma_alloc(struct device *dev, size_t size,
			       dma_addr_t *dma_handle, gfp_t gfp,
			       struct dma_attrs *attrs)
{
	struct device *aiommu = to_intel_ipu4_bus_device(dev)->iommu;
	struct intel_ipu4_mmu *mmu = dev_get_drvdata(aiommu);
	struct page **pages;
	struct iova *iova;
#ifndef __QNXNTO__
	struct vm_struct *area;
#else
    qnx_vm_area_node_t* area;
    qnx_vm_area_node_t* prev_area = NULL;
    qnx_vm_area_node_t* area_node = NULL;
#endif
	int i;
	int rval;

	size = PAGE_ALIGN(size);

	iova = alloc_iova(&mmu->dmap->iovad, size >> PAGE_SHIFT,
			  dma_get_mask(dev) >> PAGE_SHIFT, 0);
	if (!iova)
		return NULL;

	pages = __iommu_alloc_buffer(dev, size, gfp, attrs);
	if (!pages)
		goto out_free_iova;

	for (i = 0; iova->pfn_lo + i <= iova->pfn_hi; i++) {
		rval = iommu_map(mmu->dmap->domain,
				 (iova->pfn_lo + i) << PAGE_SHIFT,
				 page_to_phys(pages[i]), PAGE_SIZE, 0);
		if (rval)
			goto out_unmap;
	}

#ifdef __QNXNTO__
    // add pages to qnx vm area
    area_node = (qnx_vm_area_node_t*) calloc(1, sizeof(qnx_vm_area_node_t));
    if (area_node == NULL) {
        dev_err(dev, "Failed to alloc area_node \n");
        goto out_unmap;
    }

    pthread_mutex_lock(&qnx_vm_area_mutex);
    area = qnx_vm_areas;
    while (area) {
        if (area->vaddr == pages[0]->virtual) {
            dev_err(dev, "FATAL error: got existing vaddr %p\n", pages[0]->virtual);
            pthread_mutex_unlock(&qnx_vm_area_mutex);
            goto out_free_area_node;
        }
        prev_area = area;
        area = area->next;
    }
    area_node->vaddr = pages[0]->virtual;
    area_node->pages = pages;
    if (prev_area) {
        prev_area->next = area_node;
    } else {
        qnx_vm_areas = area_node;
    }
    pthread_mutex_unlock(&qnx_vm_area_mutex);

    *dma_handle = iova->pfn_lo << PAGE_SHIFT;

    mmu->tlb_invalidate(mmu);

    return(pages[0]->virtual);

out_free_area_node:
    free(area_node);
#else
	area = __get_vm_area(size, 0, VMALLOC_START, VMALLOC_END);
	if (!area)
		goto out_unmap;

	area->pages = pages;

	if (map_vm_area(area, PAGE_KERNEL, pages))
		goto out_vunmap;

	*dma_handle = iova->pfn_lo << PAGE_SHIFT;

	mmu->tlb_invalidate(mmu);

	return area->addr;

out_vunmap:
	vunmap(area->addr);

#endif

out_unmap:
	__iommu_free_buffer(dev, pages, size, attrs);
	for (i--; i >= 0; i--) {
		iommu_unmap(mmu->dmap->domain, (iova->pfn_lo + i) << PAGE_SHIFT,
			    PAGE_SIZE);
	}
out_free_iova:
	__free_iova(&mmu->dmap->iovad, iova);

	return NULL;
}

static void intel_ipu4_dma_free(struct device *dev, size_t size, void *vaddr,
				dma_addr_t dma_handle, struct dma_attrs *attrs)
{
	struct device *aiommu = to_intel_ipu4_bus_device(dev)->iommu;
	struct intel_ipu4_mmu *mmu = dev_get_drvdata(aiommu);
#ifdef __QNXNTO__
    qnx_vm_area_node_t* area;
    qnx_vm_area_node_t* prev_area = NULL;

    pthread_mutex_lock(&qnx_vm_area_mutex);
    area = qnx_vm_areas;
    while (area) {
        if (area->vaddr == vaddr) {
            break;
        }
        prev_area = area;
        area = area->next;
    }
    pthread_mutex_unlock(&qnx_vm_area_mutex);
#else
	struct vm_struct *area = find_vm_area(vaddr);
#endif
	struct iova *iova = find_iova(&mmu->dmap->iovad,
				dma_handle >> PAGE_SHIFT);

	if (WARN_ON(!area))
		return;

	if (WARN_ON(!area->pages))
		return;

	BUG_ON(!iova);

	size = PAGE_ALIGN(size);

	iommu_unmap(mmu->dmap->domain, iova->pfn_lo << PAGE_SHIFT,
		(iova->pfn_hi - iova->pfn_lo + 1) << PAGE_SHIFT);

	__free_iova(&mmu->dmap->iovad, iova);

	__iommu_free_buffer(dev, area->pages, size, attrs);

#ifdef __QNXNTO__
    pthread_mutex_lock(&qnx_vm_area_mutex);
    if (prev_area) {
        prev_area->next = area->next;
    } else {
        qnx_vm_areas = area->next;
    }
    pthread_mutex_unlock(&qnx_vm_area_mutex);
    free(area);
#else
	vunmap(vaddr);
#endif
	mmu->tlb_invalidate(mmu);
}

static int intel_ipu4_dma_mmap(struct device *dev, struct vm_area_struct *vma,
			void *addr, dma_addr_t iova, size_t size,
			struct dma_attrs *attrs)
{
#ifndef __QNXNTO__
	struct vm_struct *area = find_vm_area(addr);
	size_t count = PAGE_ALIGN(size) >> PAGE_SHIFT;
	size_t i;

	if (!area)
		return -EFAULT;

	if (vma->vm_start & ~PAGE_MASK)
		return -EINVAL;

	if (size > area->size)
		return -EFAULT;

	for (i = 0; i < count; i++)
		vm_insert_page(vma, vma->vm_start + (i << PAGE_SHIFT),
			area->pages[i]);
#endif
	return 0;
}

static void intel_ipu4_dma_unmap_sg(struct device *dev,
				struct scatterlist *sglist,
				int nents, enum dma_data_direction dir,
				struct dma_attrs *attrs)
{
	struct device *aiommu = to_intel_ipu4_bus_device(dev)->iommu;
	struct intel_ipu4_mmu *mmu = dev_get_drvdata(aiommu);
	struct iova *iova = find_iova(&mmu->dmap->iovad,
				sg_dma_address(sglist) >> PAGE_SHIFT);
#ifdef __QNXNTO__
	struct scatterlist *sg;
	struct page *page;
	int i;
#endif

	if (!nents)
		return;

	BUG_ON(!iova);

	if (!dma_get_attr(DMA_ATTR_SKIP_CPU_SYNC, attrs))
		intel_ipu4_dma_sync_sg_for_cpu(dev, sglist, nents,
					       DMA_BIDIRECTIONAL);

	iommu_unmap(mmu->dmap->domain, iova->pfn_lo << PAGE_SHIFT,
		    (iova->pfn_hi - iova->pfn_lo + 1) << PAGE_SHIFT);

#ifdef __QNXNTO__
	for_each_sg(sglist, sg, nents, i){
		page = sg_page(sg);
		qnxw_remove_addr_from_table(page->virtual);
	}
#endif
	mmu->tlb_invalidate(mmu);

	__free_iova(&mmu->dmap->iovad, iova);
}

static int intel_ipu4_dma_map_sg(struct device *dev, struct scatterlist *sglist,
			      int nents, enum dma_data_direction dir,
			      struct dma_attrs *attrs)
{
	struct device *aiommu = to_intel_ipu4_bus_device(dev)->iommu;
	struct intel_ipu4_mmu *mmu = dev_get_drvdata(aiommu);
	struct scatterlist *sg;
	struct iova *iova;
	size_t size = 0;
	uint32_t iova_addr;
	int i;

	for_each_sg(sglist, sg, nents, i)
		size += PAGE_ALIGN(sg->length) >> PAGE_SHIFT;

	dev_dbg(dev, "dmamap: mapping sg %d entries, %zu pages\n", nents, size);

	iova = alloc_iova(&mmu->dmap->iovad, size,
			  dma_get_mask(dev) >> PAGE_SHIFT, 0);
	if (!iova)
		return 0;

	dev_dbg(dev, "dmamap: iova low pfn %lu, high pfn %lu\n", iova->pfn_lo,
		iova->pfn_hi);

	iova_addr = iova->pfn_lo;

	for_each_sg(sglist, sg, nents, i) {
		int rval;

		dev_dbg(dev,
#ifdef __QNXNTO__
#ifdef QNX_ADDRESSING_64BITS
            "mapping entry %d: iova 0x%8.8x, physical 0x%16.16lx",
#else
            "mapping entry %d: iova 0x%8.8x, physical 0x%16.16x",
#endif
#else
			"dmamap details: mapping entry %d: iova 0x%8.8x, \
			physical 0x%16.16llx\n",
#endif
			i, iova_addr << PAGE_SHIFT, page_to_phys(sg_page(sg)));
		rval = iommu_map(mmu->dmap->domain, iova_addr << PAGE_SHIFT,
				 page_to_phys(sg_page(sg)),
				 PAGE_ALIGN(sg->length), 0);
		if (rval)
			goto out_fail;
		sg_dma_address(sg) = iova_addr << PAGE_SHIFT;
#ifdef CONFIG_NEED_SG_DMA_LENGTH
		sg_dma_len(sg) = sg->length;
#endif /* CONFIG_NEED_SG_DMA_LENGTH */

		iova_addr += PAGE_ALIGN(sg->length) >> PAGE_SHIFT;
	}

	if (!dma_get_attr(DMA_ATTR_SKIP_CPU_SYNC, attrs))
		intel_ipu4_dma_sync_sg_for_cpu(dev, sglist, nents,
					       DMA_BIDIRECTIONAL);

	mmu->tlb_invalidate(mmu);

	return nents;

out_fail:
	intel_ipu4_dma_unmap_sg(dev, sglist, i, dir, attrs);

	return 0;
}

/*
* Create scatter-list for the already allocated DMA buffer
*/
static int intel_ipu4_dma_get_sgtable(struct device *dev, struct sg_table *sgt,
				void *cpu_addr, dma_addr_t handle, size_t size,
				struct dma_attrs *attrs)
{
#ifndef __QNXNTO__
	struct vm_struct *area = find_vm_area(cpu_addr);
	int n_pages;
	int ret = 0;

	if (WARN_ON(!area->pages))
		return -ENOMEM;

	n_pages = PAGE_ALIGN(size) >> PAGE_SHIFT;

	ret = sg_alloc_table_from_pages(sgt, area->pages, n_pages, 0, size,
						GFP_KERNEL);
	if (ret)
		dev_dbg(dev, "IPU get sgt table fail\n");

	return ret;
#else
    int ret = 0;
    struct vm_struct qnx_area;
    struct vm_struct *area = &qnx_area;
    void *store_cpu_addr = cpu_addr;
    size_t store_size = size;
    off_t offset;
    size_t contig_len;
    int i, n_chunks = 0;
    struct scatterlist *sg;

    /* compute number of contiguous chunks */
    while (size > 0) {
        if (mem_offset(cpu_addr, NOFD, size, &offset, &contig_len) == -1) {
            qnx_error("Failed to get length of contiguous block");
            return -ENOMEM;
        }
        n_chunks++;
        size -= contig_len;
        cpu_addr += contig_len;
    }
    area->pages = kmalloc_array(n_chunks, sizeof(struct page*), GFP_KERNEL);
    if (area->pages == NULL) {
        qnx_error("Failed to allocate pages");
        return -ENOMEM;
    }

    ret = sg_alloc_table(sgt, n_chunks, GFP_KERNEL);
    if (ret) {
        qnx_error("Failed to allocate sg table");
        goto fail_to_alloc_sg_table;
    }

    cpu_addr = store_cpu_addr;
    size = store_size;
    sg = sgt->sgl;
    for (i = 0; i < n_chunks; i++, sg = sg_next(sg)) {
        area->pages[i] = vmalloc_to_page(cpu_addr);
        if(!area->pages[i]) {
            qnx_error("Failed to allocate page %d", i);
            i--;
            goto fail_to_alloc_page;
        }
        if (mem_offset(cpu_addr, NOFD, size, &offset, &contig_len) == -1) {
            qnx_error("Failed to get length of contiguous block");
            goto fail_to_alloc_page;
        }
        sg_set_page(sg, area->pages[i], contig_len, 0);
        size -= contig_len;
        cpu_addr += contig_len;
    }

    // Free only array of pointer - caller is responsible to free pages that
    // are now embedded in the sg table
    kfree(area->pages);
    return 0;

fail_to_alloc_page:
    for ( ; i >= 0; i--) {
        free(area->pages[i]);
    }
    sg_free_table(sgt);

fail_to_alloc_sg_table:
    kfree(area->pages);
    return -ENOMEM;
#endif
}

const struct dma_map_ops intel_ipu4_dma_ops = {
	.alloc = intel_ipu4_dma_alloc,
	.free = intel_ipu4_dma_free,
	.mmap = intel_ipu4_dma_mmap,
	.map_sg = intel_ipu4_dma_map_sg,
	.unmap_sg = intel_ipu4_dma_unmap_sg,
	.sync_single_for_cpu = intel_ipu4_dma_sync_single_for_cpu,
	.sync_single_for_device = intel_ipu4_dma_sync_single_for_cpu,
	.sync_sg_for_cpu = intel_ipu4_dma_sync_sg_for_cpu,
	.sync_sg_for_device = intel_ipu4_dma_sync_sg_for_cpu,
	.get_sgtable = intel_ipu4_dma_get_sgtable,
};
EXPORT_SYMBOL_GPL(intel_ipu4_dma_ops);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/intel-ipu4-dma.c $ $Rev: 838597 $")
#endif
