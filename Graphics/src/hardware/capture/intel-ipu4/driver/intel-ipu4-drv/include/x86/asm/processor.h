/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /arch/x86/include/asm/processor.h.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _ASM_X86_PROCESSOR_H
#define _ASM_X86_PROCESSOR_H

#include <linux/types.h>
#include <linux/cpumask.h>

/* REP NOP (PAUSE) is a good thing to insert into busy-wait loops. */
static inline void rep_nop(void)
{
	asm volatile("rep; nop" ::: "memory");
}

static inline void cpu_relax(void)
{
	rep_nop();
}

#define cpu_relax_lowlatency() cpu_relax()


struct cpuinfo_x86 {
	__u8			x86;		/* CPU family */
	__u8			x86_vendor;	/* CPU vendor */
	__u8			x86_model;
	__u8			x86_mask;

	u16			x86_clflush_size;
};

typedef struct {
	unsigned long		seg;
} mm_segment_t;


extern struct cpuinfo_x86	boot_cpu_data;

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/x86/asm/processor.h $ $Rev: 838597 $")
#endif
