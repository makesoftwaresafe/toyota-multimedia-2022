#ifndef _LINUX_DMA_DIRECTION_H
#define _LINUX_DMA_DIRECTION_H
/*
 * These definitions mirror those in pci.h, so they can be used
 * interchangeably with their PCI_ counterparts.
 */
enum dma_data_direction {
	DMA_BIDIRECTIONAL = 0,
	DMA_TO_DEVICE = 1,
	DMA_FROM_DEVICE = 2,
	DMA_NONE = 3,
};
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/dma-direction.h $ $Rev: 838597 $")
#endif
