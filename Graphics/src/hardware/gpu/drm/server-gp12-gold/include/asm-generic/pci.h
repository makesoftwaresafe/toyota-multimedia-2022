/*
 * linux/include/asm-generic/pci.h
 *
 *  Copyright (C) 2003 Russell King
 */
#ifndef _ASM_GENERIC_PCI_H
#define _ASM_GENERIC_PCI_H

#ifndef HAVE_ARCH_PCI_GET_LEGACY_IDE_IRQ
static inline int pci_get_legacy_ide_irq(struct pci_dev *dev, int channel)
{
	return channel ? 15 : 14;
}
#endif /* HAVE_ARCH_PCI_GET_LEGACY_IDE_IRQ */

/*
 * By default, assume that no iommu is in use and that the PCI
 * space is mapped to address physical 0.
 */
#ifndef PCI_DMA_BUS_IS_PHYS
#define PCI_DMA_BUS_IS_PHYS	(1)
#endif

#endif /* _ASM_GENERIC_PCI_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/pci.h $ $Rev: 836322 $")
#endif
