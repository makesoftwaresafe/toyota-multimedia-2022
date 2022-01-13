/*
 * Copyright (c) 2016 Intel Corporation.
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

/*
 * Put ipu5 isys development time tricks and hacks to this file
 */

#ifndef __INTEL_IPU5_ISYS_DEVEL_H__
#define __INTEL_IPU5_ISYS_DEVEL_H__

struct intel_ipu4_isys;

#if IS_ENABLED(CONFIG_VIDEO_INTEL_IPU5)

int intel_ipu5_isys_load_pkg_dir(struct intel_ipu4_isys *isys);
void intel_ipu5_pkg_dir_configure_spc(struct intel_ipu4_device *isp,
			const struct intel_ipu4_hw_variants *hw_variant,
			int pkg_dir_idx, void __iomem *base,
			u64 *pkg_dir,
			dma_addr_t pkg_dir_dma_addr);

#else

#ifndef __QNXNTO__
static int intel_ipu5_isys_load_pkg_dir(struct intel_ipu4_isys *isys)
{
	return 0;
}
void intel_ipu5_pkg_dir_configure_spc(struct intel_ipu4_device *isp,
				const struct intel_ipu4_hw_variants *hw_variant,
				int pkg_dir_idx, void __iomem *base,
				u64 *pkg_dir,
				dma_addr_t pkg_dir_dma_addr)
{
}
#else
static inline int intel_ipu5_isys_load_pkg_dir(struct intel_ipu4_isys *isys)
{
	return 0;
}

static inline void intel_ipu5_pkg_dir_configure_spc(struct intel_ipu4_device *isp,
                const struct intel_ipu4_hw_variants *hw_variant,
                int pkg_dir_idx, void __iomem *base,
                u64 *pkg_dir,
                dma_addr_t pkg_dir_dma_addr)
{
}
#endif /* __QNXNTO__ */

#endif
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/intel-ipu5-devel.h $ $Rev: 836043 $")
#endif
