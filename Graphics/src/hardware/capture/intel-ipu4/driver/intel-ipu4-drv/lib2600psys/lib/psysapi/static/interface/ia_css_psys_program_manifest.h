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

#ifndef __IA_CSS_PSYS_PROGRAM_MANIFEST_H
#define __IA_CSS_PSYS_PROGRAM_MANIFEST_H

/*! \file */

/** @file ia_css_psys_program_manifest.h
 *
 * Define the methods on the program manifest object that are not part of a
 * single interface
 */

#include <ia_css_psys_manifest_types.h>

#include <type_support.h>			/* uint8_t */

#include <ia_css_psys_program_manifest.sim.h>

#include <ia_css_psys_program_manifest.hsys.user.h>

#include <ia_css_kernel_bitmap.h>		/* ia_css_kernel_bitmap_t */

/*
 * Resources needs
 */
#include <ia_css_psys_program_manifest.hsys.kernel.h>

#define IA_CSS_PROGRAM_INVALID_DEPENDENCY	((uint8_t)(-1))

/*! Check if the program manifest object specifies a fixed cell allocation

 @param	manifest[in]			program manifest object

 @return has_fixed_cell, false on invalid argument
 */
extern bool ia_css_has_program_manifest_fixed_cell(
	const ia_css_program_manifest_t			*manifest);

/*! Get the stored size of the program manifest object

 @param	manifest[in]			program manifest object

 @return size, 0 on invalid argument
 */
extern size_t ia_css_program_manifest_get_size(
	const ia_css_program_manifest_t			*manifest);

/*! Get the program ID of the program manifest object

 @param	manifest[in]			program manifest object

 @return program ID, IA_CSS_PROGRAM_INVALID_ID on invalid argument
 */
extern ia_css_program_ID_t ia_css_program_manifest_get_program_ID(
	const ia_css_program_manifest_t			*manifest);

/*! Set the program ID of the program manifest object

 @param	manifest[in]			program manifest object

 @param program ID

 @return 0 on success, -1 on invalid manifest argument
 */
extern int ia_css_program_manifest_set_program_ID(
	ia_css_program_manifest_t			*manifest,
	ia_css_program_ID_t id);

/*! Get the (pointer to) the program group manifest parent of the program
 * manifest object

 @param	manifest[in]			program manifest object

 @return the pointer to the parent, NULL on invalid manifest argument
 */
extern ia_css_program_group_manifest_t *ia_css_program_manifest_get_parent(
	const ia_css_program_manifest_t			*manifest);

/*! Set the (pointer to) the program group manifest parent of the program
 * manifest object

 @param	manifest[in]			program manifest object
 @param	program_offset[in]		this program's offset from
					program_group_manifest's base address.

 @return < 0 on invalid manifest argument
 */
extern int ia_css_program_manifest_set_parent_offset(
	ia_css_program_manifest_t			*manifest,
	int32_t program_offset);

/*! Get the type of the program manifest object

 @param	manifest[in]			program manifest object

 @return program type, limit value (IA_CSS_N_PROGRAM_TYPES) on invalid manifest
	argument
*/
extern ia_css_program_type_t ia_css_program_manifest_get_type(
	const ia_css_program_manifest_t			*manifest);

/*! Set the type of the program manifest object

 @param	manifest[in]			program manifest object
 @param	program_type[in]		program type

 @return < 0 on invalid manifest argument
 */
extern int ia_css_program_manifest_set_type(
	ia_css_program_manifest_t			*manifest,
	const ia_css_program_type_t			program_type);

/*! Set the cell id of the program manifest object

 @param	manifest[in]			program manifest object
 @param	program_cell_id[in]		program cell id

 @return < 0 on invalid manifest argument
 */
extern int ia_css_program_manifest_set_cell_ID(
	ia_css_program_manifest_t			*manifest,
	const vied_nci_cell_ID_t			cell_id);

/*! Set the cell type of the program manifest object

 @param	manifest[in]			program manifest object
 @param	program_cell_type[in]		program cell type

 @return < 0 on invalid manifest argument
 */
extern int ia_css_program_manifest_set_cell_type_ID(
	ia_css_program_manifest_t			*manifest,
	const vied_nci_cell_type_ID_t			cell_type_id);

/*! Get the memory resource (size) specification for a memory
 that belongs to the cell where the program will be mapped

 @param	manifest[in]			program manifest object
 @param	mem_type_id[in]			mem type ID

 @return 0 when not applicable and/or invalid arguments
 */
extern vied_nci_resource_size_t ia_css_program_manifest_get_int_mem_size(
	const ia_css_program_manifest_t			*manifest,
	const vied_nci_mem_type_ID_t			mem_type_id);

/*! Set the memory resource (size) specification for a memory
 that belongs to the cell where the program will be mapped

 @param	manifest[in]			program manifest object
 @param	mem_type_id[in]			mem type id
 @param	int_mem_size[in]		internal memory size

 @return < 0 on invalid arguments
 */
extern int ia_css_program_manifest_set_int_mem_size(
	ia_css_program_manifest_t			*manifest,
	const vied_nci_mem_type_ID_t			mem_type_id,
	const vied_nci_resource_size_t			int_mem_size);

/*! Get the memory resource (size) specification for a memory
 that does not belong to the cell where the program will be mapped

 @param	manifest[in]			program manifest object
 @param	mem_type_id[in]			mem type ID

 @return 0 when not applicable and/or invalid arguments
 */
extern vied_nci_resource_size_t ia_css_program_manifest_get_ext_mem_size(
	const ia_css_program_manifest_t			*manifest,
	const vied_nci_mem_type_ID_t			mem_type_id);

/*! Set the memory resource (size) specification for a memory
 that does not belong to the cell where the program will be mapped

 @param	manifest[in]			program manifest object
 @param	mem_type_id[in]			mem type id
 @param	ext_mem_size[in]		external memory size

 @return < 0 on invalid arguments
 */
extern int ia_css_program_manifest_set_ext_mem_size(
	ia_css_program_manifest_t			*manifest,
	const vied_nci_mem_type_ID_t			mem_type_id,
	const vied_nci_resource_size_t			ext_mem_size);

/*! Get a device channel resource (size) specification

 @param	manifest[in]			program manifest object
 @param	dev_chn_id[in]			device channel ID

 @return 0 when not applicable and/or invalid arguments
 */
extern vied_nci_resource_size_t ia_css_program_manifest_get_dev_chn_size(
	const ia_css_program_manifest_t			*manifest,
	const vied_nci_dev_chn_ID_t			dev_chn_id);

/*! Set a device channel resource (size) specification

 @param	manifest[in]			program manifest object
 @param	dev_chn_id[in]			device channel ID
 @param	dev_chn_size[in]		device channel size

 @return < 0 on invalid arguments
 */
extern int ia_css_program_manifest_set_dev_chn_size(
	ia_css_program_manifest_t			*manifest,
	const vied_nci_dev_chn_ID_t			dev_chn_id,
	const vied_nci_resource_size_t			dev_chn_size);

/*! Get the kernel composition of the program manifest object

 @param	manifest[in]			program manifest object

 @return bitmap, 0 on invalid arguments
 */
extern ia_css_kernel_bitmap_t ia_css_program_manifest_get_kernel_bitmap(
	const ia_css_program_manifest_t			*manifest);

/*! Set the kernel dependency of the program manifest object

 @param	manifest[in]			program manifest object
 @param	kernel_bitmap[in]		kernel composition bitmap

 @return < 0 on invalid arguments
 */
extern int ia_css_program_manifest_set_kernel_bitmap(
	ia_css_program_manifest_t			*manifest,
	const ia_css_kernel_bitmap_t			kernel_bitmap);

/*! Get the number of programs this programs depends on from the program group
 * manifest object

 @param	manifest[in]			program manifest object

 @return program dependency count
 */
extern uint8_t ia_css_program_manifest_get_program_dependency_count(
	const ia_css_program_manifest_t			*manifest);

/*! Get the index of the program which the programs at this index depends on
    from the program manifest object

 @param	manifest[in]			program manifest object

 @return program dependency,
	IA_CSS_PROGRAM_INVALID_DEPENDENCY on invalid arguments
	*/
extern uint8_t ia_css_program_manifest_get_program_dependency(
	const ia_css_program_manifest_t			*manifest,
	const unsigned int				index);

/*! Set the index of the program which the programs at this index depends on
    in the program manifest object

 @param	manifest[in]			program manifest object

 @return program dependency
 */
extern int ia_css_program_manifest_set_program_dependency(
	ia_css_program_manifest_t			*manifest,
	const uint8_t					program_dependency,
	const unsigned int				index);

/*! Get the number of terminals this programs depends on from the program group
 * manifest object

 @param	manifest[in]			program manifest object

 @return program dependency count
 */
extern uint8_t ia_css_program_manifest_get_terminal_dependency_count(
	const ia_css_program_manifest_t			*manifest);

/*! Get the index of the terminal which the programs at this index depends on
    from the program manifest object

 @param	manifest[in]			program manifest object

 @return terminal dependency, IA_CSS_PROGRAM_INVALID_DEPENDENCY on error
 */
uint8_t ia_css_program_manifest_get_terminal_dependency(
	const ia_css_program_manifest_t			*manifest,
	const unsigned int				index);

/*! Set the index of the terminal which the programs at this index depends on
    in the program manifest object

 @param	manifest[in]			program manifest object

 @return < 0 on invalid arguments
 */
extern int ia_css_program_manifest_set_terminal_dependency(
	ia_css_program_manifest_t			*manifest,
	const uint8_t					terminal_dependency,
	const unsigned int				index);

/*! Check if the program manifest object specifies a subnode program

 @param	manifest[in]			program manifest object

 @return is_subnode, false on invalid argument
 */
extern bool ia_css_is_program_manifest_subnode_program_type(
	const ia_css_program_manifest_t			*manifest);

/*! Check if the program manifest object specifies a supernode program

 @param	manifest[in]			program manifest object

 @return is_supernode, false on invalid argument
 */
extern bool ia_css_is_program_manifest_supernode_program_type(
	const ia_css_program_manifest_t			*manifest);
/*! Check if the program manifest object specifies a singular program

 @param	manifest[in]			program manifest object

 @return is_singular, false on invalid argument
 */
extern bool ia_css_is_program_manifest_singular_program_type(
	const ia_css_program_manifest_t			*manifest);

#endif /* __IA_CSS_PSYS_PROGRAM_MANIFEST_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/lib2600psys/lib/psysapi/static/interface/ia_css_psys_program_manifest.h $ $Rev: 834264 $")
#endif
