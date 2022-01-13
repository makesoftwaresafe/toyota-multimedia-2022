/*
 * Support for Intel Camera Imaging ISP subsystem.
 * Copyright (c) 2010 - 2015, Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#ifndef _IA_CSE_PKG_DIR_H
#define _IA_CSE_PKG_DIR_H

/* Address of CSE_PKG_DIR in IMR (in secure mode only) */

#if defined(C_RUN) || defined(HRT_UNSCHED) || defined(HRT_SCHED)
#define IA_CSE_PKG_DIR_ADDRESS 0x1000
#else
#define IA_CSE_PKG_DIR_ADDRESS 0x40
#endif

#endif /* __IA_CSE_PKG_DIR_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600/cse/interface/ia_cse_pkg_dir.h $ $Rev: 838597 $")
#endif
