/*
 * Copyright (c) 2015--2016 Intel Corporation.
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
 */

#ifndef LIBCSSPSYS2600_H
#define LIBCSSPSYS2600_H

#include <ia_css_psysapi.h>
#include <ia_css_psys_process_group.h>
#include <ia_css_psys_device.h>
#include <ia_css_psys_terminal.h>
#include <ia_css_psys_process.h>
#include <ia_css_psys_program_group_manifest.h>
#include <ia_css_psys_program_manifest.h>

extern struct ia_css_syscom_context *psys_syscom;
#ifdef __QNXNTO__
int libcsspsys2600_init(void);
void libcsspsys2600_exit(void);
#endif // __QNXNTO__
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600psys/libcsspsys2600.h $ $Rev: 834264 $")
#endif
