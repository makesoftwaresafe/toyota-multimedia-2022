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

#ifndef INTEL_IPU4_DMA_H
#define INTEL_IPU4_DMA_H

#ifndef __QNXNTO__
#include <linux/iova.h>
#endif /* __QNXNTO__ */

struct iommu_domain;

struct intel_ipu4_dma_mapping {
	struct iommu_domain *domain;
	struct iova_domain iovad;
	struct kref ref;
};

extern const struct dma_map_ops intel_ipu4_dma_ops;

#endif /* INTEL_IPU4_DMA_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/intel-ipu4-dma.h $ $Rev: 838597 $")
#endif
