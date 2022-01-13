/**
* Support for Intel Camera Imaging ISP subsystem.
* Copyright (c) 2010 - 2016, Intel Corporation.
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

#ifndef __STORAGE_CLASS_H
#define __STORAGE_CLASS_H

#define STORAGE_CLASS_EXTERN \
extern

#if defined(_MSC_VER)
#define STORAGE_CLASS_INLINE \
static __inline
#elif defined(__HIVECC)
#define STORAGE_CLASS_INLINE \
static inline
#else
#define STORAGE_CLASS_INLINE \
static inline
#endif

/* Register struct */
#ifndef __register
#if defined(__HIVECC) && !defined(PIPE_GENERATION)
#define __register register
#else
#define __register
#endif
#endif

/* Memory attribute */
#ifndef MEM
#ifdef PIPE_GENERATION
#elif defined(__HIVECC)
#include <hive/attributes.h>
#else
#define MEM(any_mem)
#endif
#endif

#endif /* __STORAGE_CLASS_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600/support/storage_class.h $ $Rev: 838597 $")
#endif
