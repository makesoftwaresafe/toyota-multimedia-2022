/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/uapi/linux/sysinfo.h with __QNXNTO__ indicating modifications.
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

#ifndef _LINUX_SYSINFO_H
#define _LINUX_SYSINFO_H

#include <linux/types.h>
#ifdef __QNXNTO__
#include <asm-generic/posix_types.h>
#endif

#define SI_LOAD_SHIFT	16
struct sysinfo {
	__kernel_long_t uptime;		/* Seconds since boot */
	__kernel_ulong_t loads[3];	/* 1, 5, and 15 minute load averages */
	__kernel_ulong_t totalram;	/* Total usable main memory size */
	__kernel_ulong_t freeram;	/* Available memory size */
	__kernel_ulong_t sharedram;	/* Amount of shared memory */
	__kernel_ulong_t bufferram;	/* Memory used by buffers */
	__kernel_ulong_t totalswap;	/* Total swap space size */
	__kernel_ulong_t freeswap;	/* swap space still available */
	__u16 procs;		   	/* Number of current processes */
	__u16 pad;		   	/* Explicit padding for m68k */
	__kernel_ulong_t totalhigh;	/* Total high memory size */
	__kernel_ulong_t freehigh;	/* Available high memory size */
	__u32 mem_unit;			/* Memory unit size in bytes */
	char _f[20-2*sizeof(__kernel_ulong_t)-sizeof(__u32)];	/* Padding: libc5 uses this.. */
};

#endif /* _LINUX_SYSINFO_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/uapi/linux/sysinfo.h $ $Rev: 838597 $")
#endif
