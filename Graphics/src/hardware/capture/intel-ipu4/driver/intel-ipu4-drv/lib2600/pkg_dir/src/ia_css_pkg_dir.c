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

#ifdef _IA_CSS_PKG_DIR_INLINE_

#include "storage_class.h"

STORAGE_CLASS_INLINE int __ia_css_pkg_dir_avoid_warning_on_empty_file(void)
{
	return 0;
}

#else
#include "ia_css_pkg_dir_impl.h"

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600/pkg_dir/src/ia_css_pkg_dir.c $ $Rev: 838597 $")
#endif
