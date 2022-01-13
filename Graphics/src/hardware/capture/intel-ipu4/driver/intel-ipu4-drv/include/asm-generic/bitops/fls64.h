/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Yocto Linux original taken from GP101 release; __QNXNTO__
* marks modifications to original from /asm-generic/bitops/fls64.h
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

#ifndef _ASM_GENERIC_BITOPS_FLS64_H_
#define _ASM_GENERIC_BITOPS_FLS64_H_

#include <asm/types.h>
#ifdef __QNXNTO__
#include <linux/types.h>
#endif
/**
 * fls64 - find last set bit in a 64-bit word
 * @x: the word to search
 *
 * This is defined in a similar way as the libc and compiler builtin
 * ffsll, but returns the position of the most significant set bit.
 *
 * fls64(value) returns 0 if value is 0 or the position of the last
 * set bit if value is nonzero. The last (most significant) bit is
 * at position 64.
 */

#ifdef __QNXNTO__
#ifndef __always_inline
#define __always_inline inline
#endif
#endif // __QNXNTO__

#if BITS_PER_LONG == 32
static __always_inline int fls64(__u64 x)
{
	__u32 h = x >> 32;
	if (h)
		return fls(h) + 32;
	return fls(x);
}
#elif BITS_PER_LONG == 64
static __always_inline int fls64(__u64 x)
{
	if (x == 0)
		return 0;
	return __fls(x) + 1;
}
#else
#error BITS_PER_LONG not 32 or 64
#endif

#endif /* _ASM_GENERIC_BITOPS_FLS64_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/asm-generic/bitops/fls64.h $ $Rev: 838597 $")
#endif
