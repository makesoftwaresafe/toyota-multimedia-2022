/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Yocto Linux original taken from GP101 release - __QNXNTO__
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

#ifndef __ASM_GENERIC_BITS_PER_LONG
#define __ASM_GENERIC_BITS_PER_LONG

#include <uapi/asm-generic/bitsperlong.h>

#ifndef __QNXNTO__
#ifdef CONFIG_64BIT
#define BITS_PER_LONG 64
#else
#define BITS_PER_LONG 32
#endif /* CONFIG_64BIT */
#endif /* __QNXNTO__ */

/*
 * FIXME: The check currently breaks x86-64 build, so it's
 * temporarily disabled. Please fix x86-64 and reenable
 */
#if 0 && BITS_PER_LONG != __BITS_PER_LONG
#error Inconsistent word size. Check asm/bitsperlong.h
#endif

#ifndef BITS_PER_LONG_LONG
#define BITS_PER_LONG_LONG 64
#endif

#endif /* __ASM_GENERIC_BITS_PER_LONG */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/asm-generic/bitsperlong.h $ $Rev: 838597 $")
#endif
