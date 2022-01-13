#ifndef _ASM_X86_INAT_TYPES_H
#define _ASM_X86_INAT_TYPES_H
/*
 * x86 instruction attributes
 *
 * Written by Masami Hiramatsu <mhiramat@redhat.com>
 * Some modifications (__QNXNTO__) Copyright (c) 2017 QNX Software Systems (renamed from inat_types.h).
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

/* Instruction attributes */
typedef unsigned int insn_attr_t;
typedef unsigned char insn_byte_t;
typedef signed int insn_value_t;

#ifdef __QNXNTO__
// Copied from pgtable-2level_types.h
typedef unsigned long	pgprotval_t;
// Copied from pgtable_types.h
typedef struct pgprot { pgprotval_t pgprot; } pgprot_t;
#endif

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/types.h $ $Rev: 838597 $")
#endif
