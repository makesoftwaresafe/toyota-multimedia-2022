/*
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

#ifndef __IA_CSS_PSYS_TERMINAL_MANIFEST_HSYS_USER_H
#define __IA_CSS_PSYS_TERMINAL_MANIFEST_HSYS_USER_H

/*! \file */

/** @file ia_css_psys_terminal.hsys.user.h
 *
 * Define the methods on the termianl manifest object: Hsys user interface
 */

#include <ia_css_psys_manifest_types.h>

/*! Print the terminal manifest object to file/stream

 @param	manifest[in]			terminal manifest object
 @param	fid[out]				file/stream handle

 @return < 0 on error
 */
extern int ia_css_terminal_manifest_print(
	const ia_css_terminal_manifest_t	*manifest,
	void					*fid);

#endif /* __IA_CSS_PSYS_TERMINAL_MANIFEST_HSYS_USER_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600psys/lib/psysapi/static/interface/ia_css_psys_terminal_manifest.hsys.user.h $ $Rev: 834264 $")
#endif
