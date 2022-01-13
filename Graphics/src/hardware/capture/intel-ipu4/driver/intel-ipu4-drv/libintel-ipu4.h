/*
 * Copyright (c) 2014--2016 Intel Corporation.
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

#ifndef LIBINTEL_IPU4_H
#define LIBINTEL_IPU4_H

#include <ia_css_isysapi.h>
#include <ia_css_input_buffer_cpu.h>
#include <ia_css_output_buffer_cpu.h>
#include <ia_css_shared_buffer_cpu.h>

#ifndef __QNXNTO__
void csslib_dump_isys_stream_cfg(struct device *dev,
		struct ipu_fw_isys_stream_cfg_data *stream_cfg);
void csslib_dump_isys_frame_buff_set(struct device *dev,
		struct ipu_fw_isys_frame_buff_set *buf,
		unsigned int outputs);
#else
// Definitions above have no associated implementation + uses old struct that no longer exist
// Need to make these visible to do our initialization
int libipu4_library_init(void);
void libipu4_library_exit(void);
#endif // __QNXNTO__
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/libintel-ipu4.h $ $Rev: 838597 $")
#endif
