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

#ifndef __IA_CSS_TERMINAL_H
#define __IA_CSS_TERMINAL_H

#include "type_support.h"
#include "ia_css_terminal_types.h"
#include "ia_css_param_storage_class.h"

IA_CSS_PARAMETERS_STORAGE_CLASS_H
unsigned int ia_css_param_in_terminal_get_descriptor_size(
	const unsigned int nof_sections
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_param_section_desc_t *
ia_css_param_in_terminal_get_param_section_desc(
	const ia_css_param_terminal_t *param_terminal,
	const unsigned int section_index
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
unsigned int ia_css_param_out_terminal_get_descriptor_size(
	const unsigned int nof_sections,
	const unsigned int nof_fragments
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_param_section_desc_t *
ia_css_param_out_terminal_get_param_section_desc(
	const ia_css_param_terminal_t *param_terminal,
	const unsigned int section_index,
	const unsigned int nof_sections,
	const unsigned int fragment_index
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
int ia_css_param_terminal_create(
	ia_css_param_terminal_t *param_terminal,
	const uint16_t terminal_offset,
	const uint16_t terminal_size,
	const uint16_t is_input_terminal
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
unsigned int ia_css_spatial_param_terminal_get_descriptor_size(
	const unsigned int nof_frame_param_sections,
	const unsigned int nof_fragments
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_fragment_grid_desc_t *
ia_css_spatial_param_terminal_get_fragment_grid_desc(
	const ia_css_spatial_param_terminal_t *spatial_param_terminal,
	const unsigned int fragment_index
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_frame_grid_param_section_desc_t *
ia_css_spatial_param_terminal_get_frame_grid_param_section_desc(
	const ia_css_spatial_param_terminal_t *spatial_param_terminal,
	const unsigned int section_index
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
int ia_css_spatial_param_terminal_create(
	ia_css_spatial_param_terminal_t *spatial_param_terminal,
	const uint16_t terminal_offset,
	const uint16_t terminal_size,
	const uint16_t is_input_terminal,
	const unsigned int nof_fragments,
	const uint32_t kernel_id
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
unsigned int ia_css_sliced_param_terminal_get_descriptor_size(
	const unsigned int nof_slice_param_sections,
	const unsigned int nof_slices[],
	const unsigned int nof_fragments
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_fragment_slice_desc_t *
ia_css_sliced_param_terminal_get_fragment_slice_desc(
	const ia_css_sliced_param_terminal_t *sliced_param_terminal,
	const unsigned int fragment_index
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_slice_param_section_desc_t *
ia_css_sliced_param_terminal_get_slice_param_section_desc(
	const ia_css_sliced_param_terminal_t *sliced_param_terminal,
	const unsigned int fragment_index,
	const unsigned int slice_index,
	const unsigned int section_index,
	const unsigned int nof_slice_param_sections
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
int ia_css_sliced_param_terminal_create(
	ia_css_sliced_param_terminal_t *sliced_param_terminal,
	const uint16_t terminal_offset,
	const uint16_t terminal_size,
	const uint16_t is_input_terminal,
	const unsigned int nof_slice_param_sections,
	const unsigned int nof_slices[],
	const unsigned int nof_fragments,
	const uint32_t kernel_id
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
unsigned int ia_css_program_terminal_get_descriptor_size(
	const unsigned int nof_fragments,
	const unsigned int nof_fragment_param_sections,
	const unsigned int nof_kernel_fragment_sequencer_infos,
	const unsigned int nof_command_objs
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_fragment_param_section_desc_t *
ia_css_program_terminal_get_frgmnt_prm_sct_desc(
	const ia_css_program_terminal_t *program_terminal,
	const unsigned int fragment_index,
	const unsigned int section_index,
	const unsigned int nof_fragment_param_sections
);

/* Keep old function name before Windows/Android change name */
IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_fragment_param_section_desc_t *
ia_css_program_terminal_get_fragment_param_section_desc(
	const ia_css_program_terminal_t *program_terminal,
	const unsigned int fragment_index,
	const unsigned int section_index,
	const unsigned int nof_fragment_param_sections
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_kernel_fragment_sequencer_info_desc_t *
ia_css_program_terminal_get_kernel_frgmnt_seq_info_desc(
	const ia_css_program_terminal_t *program_terminal,
	const unsigned int fragment_index,
	const unsigned int info_index,
	const unsigned int nof_kernel_fragment_sequencer_infos
);

/* Keep old function name before Windows/Android change name */
IA_CSS_PARAMETERS_STORAGE_CLASS_H
ia_css_kernel_fragment_sequencer_info_desc_t *
ia_css_program_terminal_get_kernel_fragment_sequencer_info_desc(
	const ia_css_program_terminal_t *program_terminal,
	const unsigned int fragment_index,
	const unsigned int info_index,
	const unsigned int nof_kernel_fragment_sequencer_infos
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
int ia_css_program_terminal_create(
	ia_css_program_terminal_t *program_terminal,
	const uint16_t terminal_offset,
	const uint16_t terminal_size,
	const unsigned int nof_fragments,
	const unsigned int nof_kernel_fragment_sequencer_infos,
	const unsigned int nof_command_objs
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
int ia_css_program_terminal_get_command_base_offset(
	const ia_css_program_terminal_t *program_terminal,
	const unsigned int nof_fragments,
	const unsigned int nof_kernel_fragment_sequencer_infos,
	const unsigned int commands_slots_used,
	uint16_t *command_desc_offset
);

IA_CSS_PARAMETERS_STORAGE_CLASS_H
uint16_t *ia_css_program_terminal_get_line_count(
	const ia_css_kernel_fragment_sequencer_command_desc_t
	*kernel_fragment_sequencer_command_desc_base,
	const unsigned int set_count
);

#ifdef _INLINE_PARAMETERS_
#include "ia_css_terminal_impl.h"
#endif /* _INLINE_PARAMETERS_ */

#endif /* __IA_CSS_TERMINAL_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600psys/lib/vied_parameters/interface/ia_css_terminal.h $ $Rev: 834264 $")
#endif
