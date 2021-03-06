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

#ifndef __IA_CSS_INPUT_BUFFER_CPU_H
#define __IA_CSS_INPUT_BUFFER_CPU_H

#include "vied/shared_memory_map.h"
#include "ia_css_input_buffer.h"

ia_css_input_buffer
ia_css_input_buffer_alloc(
	vied_subsystem_t sid,
	vied_memory_t mid,
	unsigned int size);

void
ia_css_input_buffer_free(
	vied_subsystem_t sid,
	vied_memory_t mid,
	ia_css_input_buffer b);

ia_css_input_buffer_cpu_address
ia_css_input_buffer_cpu_map(ia_css_input_buffer b);

ia_css_input_buffer_cpu_address
ia_css_input_buffer_cpu_unmap(ia_css_input_buffer b);

ia_css_input_buffer_css_address
ia_css_input_buffer_css_map(vied_memory_t mid, ia_css_input_buffer b);

ia_css_input_buffer_css_address
ia_css_input_buffer_css_unmap(ia_css_input_buffer b);


#endif /* __IA_CSS_INPUT_BUFFER_CPU_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600/buffer/interface/ia_css_input_buffer_cpu.h $ $Rev: 838597 $")
#endif
