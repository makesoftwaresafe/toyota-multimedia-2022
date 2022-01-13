/* include this file if the platform implements the dma_ DMA Mapping API
 * and wants to provide the pci_ DMA Mapping API in terms of it */

#ifndef _ASM_GENERIC_PCI_DMA_COMPAT_H
#define _ASM_GENERIC_PCI_DMA_COMPAT_H

#include <linux/dma-mapping.h>
#include <asm-generic/dma-mapping-common.h>

int dma_map_sg(struct device *dev, struct scatterlist *sglist, int nents,
	       enum dma_data_direction direction);

static inline void dma_unmap_sg(struct device *dev, struct scatterlist *sglist,
		  int nents, enum dma_data_direction dir)
{
}

static inline int
pci_dma_supported(struct pci_dev *hwdev, u64 mask)
{
	return dma_supported(hwdev == NULL ? NULL : &hwdev->dev, mask);
}

static inline void *
pci_alloc_consistent(struct pci_dev *hwdev, size_t size,
		     dma_addr_t *dma_handle)
{
	return dma_alloc_coherent(hwdev == NULL ? NULL : &hwdev->dev, size, dma_handle, GFP_ATOMIC);
}

static inline void *
pci_zalloc_consistent(struct pci_dev *hwdev, size_t size,
		      dma_addr_t *dma_handle)
{
	return dma_zalloc_coherent(hwdev == NULL ? NULL : &hwdev->dev,
				   size, dma_handle, GFP_ATOMIC);
}

static inline void
pci_free_consistent(struct pci_dev *hwdev, size_t size,
		    void *vaddr, dma_addr_t dma_handle)
{
	dma_free_coherent(hwdev == NULL ? NULL : &hwdev->dev, size, vaddr, dma_handle);
}

static inline dma_addr_t
pci_map_single(struct pci_dev *hwdev, void *ptr, size_t size, int direction)
{
//	return dma_map_single(hwdev == NULL ? NULL : &hwdev->dev, ptr, size, (enum dma_data_direction)direction);
	return 0;
}

static inline void
pci_unmap_single(struct pci_dev *hwdev, dma_addr_t dma_addr,
		 size_t size, int direction)
{
//	dma_unmap_single(hwdev == NULL ? NULL : &hwdev->dev, dma_addr, size, (enum dma_data_direction)direction);
	return;
}

dma_addr_t pci_map_page(struct pci_dev *hwdev, struct page *page, unsigned long offset, size_t size, int direction);
void pci_unmap_page(struct pci_dev *hwdev, dma_addr_t dma_address, size_t size, int direction);

int pci_map_sg(struct pci_dev *hwdev, struct scatterlist *sg, int nents, int direction);
void pci_unmap_sg(struct pci_dev *hwdev, struct scatterlist *sg, int nents, int direction);

static inline void
pci_dma_sync_single_for_cpu(struct pci_dev *hwdev, dma_addr_t dma_handle,
		    size_t size, int direction)
{
	dma_sync_single_for_cpu(hwdev == NULL ? NULL : &hwdev->dev, dma_handle, size, (enum dma_data_direction)direction);
}

static inline void
pci_dma_sync_single_for_device(struct pci_dev *hwdev, dma_addr_t dma_handle,
		    size_t size, int direction)
{
	dma_sync_single_for_device(hwdev == NULL ? NULL : &hwdev->dev, dma_handle, size, (enum dma_data_direction)direction);
}

static inline void
pci_dma_sync_sg_for_cpu(struct pci_dev *hwdev, struct scatterlist *sg,
		int nelems, int direction)
{
	dma_sync_sg_for_cpu(hwdev == NULL ? NULL : &hwdev->dev, sg, nelems, (enum dma_data_direction)direction);
}

static inline void
pci_dma_sync_sg_for_device(struct pci_dev *hwdev, struct scatterlist *sg,
		int nelems, int direction)
{
	dma_sync_sg_for_device(hwdev == NULL ? NULL : &hwdev->dev, sg, nelems, (enum dma_data_direction)direction);
}

int pci_dma_mapping_error(struct pci_dev *pdev, dma_addr_t dma_addr);

#ifdef CONFIG_PCI
static inline int pci_set_dma_mask(struct pci_dev *dev, u64 mask)
{
	return dma_set_mask(&dev->dev, mask);
}

static inline int pci_set_consistent_dma_mask(struct pci_dev *dev, u64 mask)
{
	return dma_set_coherent_mask(&dev->dev, mask);
}
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/pci-dma-compat.h $ $Rev: 837534 $")
#endif
