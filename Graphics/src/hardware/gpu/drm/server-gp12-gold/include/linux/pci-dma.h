#ifndef _LINUX_PCI_DMA_H
#define _LINUX_PCI_DMA_H

#define DECLARE_PCI_UNMAP_ADDR(ADDR_NAME) DEFINE_DMA_UNMAP_ADDR(ADDR_NAME);
#define DECLARE_PCI_UNMAP_LEN(LEN_NAME)   DEFINE_DMA_UNMAP_LEN(LEN_NAME);
#define pci_unmap_addr             dma_unmap_addr
#define pci_unmap_addr_set         dma_unmap_addr_set
#define pci_unmap_len              dma_unmap_len
#define pci_unmap_len_set          dma_unmap_len_set

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/pci-dma.h $ $Rev: 836322 $")
#endif
