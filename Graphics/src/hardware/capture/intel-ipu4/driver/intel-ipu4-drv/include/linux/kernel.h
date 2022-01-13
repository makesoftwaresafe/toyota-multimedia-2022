/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/kernel.h with modifications.
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

#ifndef _QNX_LINUX_KERNEL_H
#define _QNX_LINUX_KERNEL_H

#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/bitops.h>
#include <linux/log2.h>
#include <asm/string.h>

#ifndef USHRT_MAX
#define USHRT_MAX       ((u16)(~0U))
#endif
#ifndef SHRT_MAX
#define SHRT_MAX        ((s16)(USHRT_MAX>>1))
#endif
#ifndef SHRT_MIN
#define SHRT_MIN        ((s16)(-SHRT_MAX - 1))
#endif
#ifndef INT_MAX
#define INT_MAX         ((int)(~0U>>1))
#endif
#ifndef INT_MIN
#define INT_MIN         (-INT_MAX - 1)
#endif
#ifndef UINT_MAX
#define UINT_MAX        (~0U)
#endif
#ifndef LONG_MAX
#define LONG_MAX        ((long)(~0UL>>1))
#endif
#ifndef LONG_MIN
#define LONG_MIN        (-LONG_MAX - 1)
#endif
#ifndef ULONG_MAX
#define ULONG_MAX       (~0UL)
#endif
#ifndef LLONG_MAX
#define LLONG_MAX       ((long long)(~0ULL>>1))
#endif
#ifndef LLONG_MIN
#define LLONG_MIN       (-LLONG_MAX - 1)
#endif
#ifndef ULLONG_MAX
#define ULLONG_MAX      (~0ULL)
#endif
#ifndef SIZE_MAX
#define SIZE_MAX        (~(size_t)0)
#endif


#define U64_MAX		((u64)~0ULL)


#define STACK_MAGIC     0xdeadbeef

#define REPEAT_BYTE(x)  ((~0ul / 0xff) * (x))

#define ALIGN(x, a)		__ALIGN_KERNEL((x), (a))
#define __ALIGN_MASK(x, mask)	__ALIGN_KERNEL_MASK((x), (mask))
#define PTR_ALIGN(p, a)		((typeof(p))ALIGN((unsigned long)(p), (a)))
#define IS_ALIGNED(x, a)		(((x) & ((typeof(x))(a) - 1)) == 0)

#define __ALIGN_KERNEL(x, a)		__ALIGN_KERNEL_MASK(x, (typeof(x))(a) - 1)
#define __ALIGN_KERNEL_MASK(x, mask)	(((x) + (mask)) & ~(mask))


#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]) + __must_be_array(arr))

/**
 * upper_32_bits - return bits 32-63 of a number
 * @n: the number we're accessing
 *
 * A basic shift-right of a 64- or 32-bit quantity.  Use this to suppress
 * the "right shift count >= width of type" warning when that quantity is
 * 32-bits.
 */
#define upper_32_bits(n) ((u32)(((n) >> 16) >> 16))

/**
 * lower_32_bits - return bits 0-31 of a number
 * @n: the number we're accessing
 */
#define lower_32_bits(n) ((u32)(n))

# define might_sleep() do { } while (0)

/*
 * This looks more complex than it should be. But we need to
 * get the type for the ~ right in round_down (it needs to be
 * as wide as the result!), and we want to evaluate the macro
 * arguments just once each.
 */
#define __round_mask(x, y) ((__typeof__(x))((y)-1))
#define round_up(x, y) ((((x)-1) | __round_mask(x, y))+1)
#define round_down(x, y) ((x) & ~__round_mask(x, y))

#define FIELD_SIZEOF(t, f) (sizeof(((t*)0)->f))
#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))
#define DIV_ROUND_UP_ULL(ll,d) \
	({ unsigned long long _tmp = (ll)+(d)-1; do_div(_tmp, d); _tmp; })

#if BITS_PER_LONG == 32
# define DIV_ROUND_UP_SECTOR_T(ll,d) DIV_ROUND_UP_ULL(ll, d)
#else
# define DIV_ROUND_UP_SECTOR_T(ll,d) DIV_ROUND_UP(ll,d)
#endif

/*
 * Divide positive or negative dividend by positive divisor and round
 * to closest integer. Result is undefined for negative divisors and
 * for negative dividends if the divisor variable type is unsigned.
 */
#define DIV_ROUND_CLOSEST(x, divisor)(			\
{							\
	typeof(x) __x = x;				\
	typeof(divisor) __d = divisor;			\
	(((typeof(x))-1) > 0 ||				\
	 ((typeof(divisor))-1) > 0 || (__x) > 0) ?	\
		(((__x) + ((__d) / 2)) / (__d)) :	\
		(((__x) - ((__d) / 2)) / (__d));	\
}							\
)

/*
 * Multiplies an integer by a fraction, while avoiding unnecessary
 * overflow or loss of precision.
 */
#define mult_frac(x, numer, denom)(			\
{							\
	typeof(x) quot = (x) / (denom);			\
	typeof(x) rem  = (x) % (denom);			\
	(quot * (numer)) + ((rem * (numer)) / (denom));	\
}							\
)

/* The `const' in roundup() prevents gcc-3.3 from calling __divdi3 */
#define roundup(x, y) (					\
{							\
	const typeof(y) __y = y;			\
	(((x) + (__y - 1)) / __y) * __y;		\
}							\
)
#define rounddown(x, y) (				\
{							\
	typeof(x) __x = (x);				\
	__x - (__x % (y));				\
}							\
)


#define swap(a, b) \
	do { typeof(a) __tmp = (a); (a) = (b); (b) = __tmp; } while (0)

#define abs64(x) ({				\
		s64 __x = (x);			\
		(__x < 0) ? -__x : __x;		\
	})

#define min_t(type, x, y) ({			\
	type __min1 = (x);			\
	type __min2 = (y);			\
	__min1 < __min2 ? __min1: __min2; })

#define max_t(type, x, y) ({			\
	type __max1 = (x);			\
	type __max2 = (y);			\
	__max1 > __max2 ? __max1: __max2; })

#define min3(x, y, z) ({			\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	typeof(z) _min3 = (z);			\
	(void) (&_min1 == &_min2);		\
	(void) (&_min1 == &_min3);		\
	_min1 < _min2 ? (_min1 < _min3 ? _min1 : _min3) : \
		(_min2 < _min3 ? _min2 : _min3); })

#define max3(x, y, z) ({			\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	typeof(z) _max3 = (z);			\
	(void) (&_max1 == &_max2);		\
	(void) (&_max1 == &_max3);		\
	_max1 > _max2 ? (_max1 > _max3 ? _max1 : _max3) : \
		(_max2 > _max3 ? _max2 : _max3); })

#define clamp_t(type, val, min, max) ({		\
	type __val = (val);			\
	type __min = (min);			\
	type __max = (max);			\
	__val = __val < __min ? __min: __val;	\
	__val > __max ? __max: __val; })

#define min_c(x, y) ({				\
	typeof(x) _min1 = (x);			\
	typeof(y) _min2 = (y);			\
	(void) (&_min1 == &_min2);		\
	_min1 < _min2 ? _min1 : _min2; })

#define max_c(x, y) ({				\
	typeof(x) _max1 = (x);			\
	typeof(y) _max2 = (y);			\
	(void) (&_max1 == &_max2);		\
	_max1 > _max2 ? _max1 : _max2; })

#define clamp(val, lo, hi) min_c((typeof(val))max_c(val, lo), hi)

/* Values used for system_state */
extern enum system_states {
	SYSTEM_BOOTING,
	SYSTEM_RUNNING,
	SYSTEM_HALT,
	SYSTEM_POWER_OFF,
	SYSTEM_RESTART,
} system_state;

//TODO. 
//#define HZ CLOCKS_PER_SEC

#define __must_check
char *kasprintf(gfp_t gfp, const char *fmt, ...);

static inline int __must_check kstrtou32(const char *s, unsigned int base, u32 *res)
{
    unsigned long ret = strtoul(s, 0, base);
	if(errno){
		return errno;
	}
	*res = (u32)ret;
	return 0;
}

void hex_dump_to_buffer(const void *buf, size_t len, int rowsize,
						int groupsize, char *linebuf, size_t linebuflen,
						bool ascii);

static inline int
vscnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	int i;
	ssize_t ssize = size;

	i = vsnprintf(buf, size, fmt, args);

	return (i >= ssize) ? (ssize - 1) : i;
}

static inline int scnprintf(char * buf, size_t size, const char * fmt, ...)
{
	va_list args;
	ssize_t ssize = size;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buf, size, fmt, args);
	va_end(args);

	return (i >= ssize) ? (ssize - 1) : i;
}

char *kvasprintf(gfp_t gfp, const char *fmt, va_list args);

#define dev_err(dev, format, ...) qnx_error(format, ## __VA_ARGS__)
#define dev_warn(dev, format, ...) qnx_warning(format, ## __VA_ARGS__)

extern int oops_in_progress;		/* If set, an oops, panic(), BUG() or die() is in progress */


#define DIV_ROUND_CLOSEST_ULL(x, divisor)(		\
{							\
	typeof(divisor) __d = divisor;			\
	unsigned long long _tmp = (x) + (__d) / 2;	\
	do_div(_tmp, __d);				\
	_tmp;						\
}							\
)



#if BITS_PER_LONG == 64

# define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })


#elif BITS_PER_LONG == 32

# define do_div(n,base) ({					\
	uint32_t __base = (base);				\
	uint32_t __rem;						\
	__rem = ((uint64_t)(n)) % __base;			\
	(n) = ((uint64_t)(n)) / __base;				\
	__rem;							\
 })

#endif

int kstrtouint(const char *s, unsigned int base, unsigned int *res);
int kstrtoint(const char *s, unsigned int base, int *res);

extern __attribute__ ((format (printf,2,3))) int ksprintf(char *buf, const char * fmt, ...);
extern __attribute__ ((format (printf,2,0))) int kvsprintf(char *buf, const char *, va_list);
extern __attribute__ ((format (printf,3,4))) int ksnprintf(char *buf, size_t size, const char *fmt, ...);
extern __attribute__ ((format (printf,3,0))) int kvsnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern __attribute__ ((format (printf,3,4))) int kscnprintf(char *buf, size_t size, const char *fmt, ...);
extern __attribute__ ((format (printf,3,0))) int kvscnprintf(char *buf, size_t size, const char *fmt, va_list args);
extern __attribute__ ((format (printf,2,3))) char *kasprintf(gfp_t gfp, const char *fmt, ...);
extern __attribute__ ((format (printf,2,0))) char *kvasprintf(gfp_t gfp, const char *fmt, va_list args);
extern __attribute__ ((format (printf,2,0))) const char *kvasprintf_const(gfp_t gfp, const char *fmt, va_list args);
extern __attribute__ ((format (scanf,2,3))) int ksscanf(const char *, const char *, ...);
extern __attribute__ ((format (scanf,2,0))) int kvsscanf(const char *, const char *, va_list);

extern const char hex_asc[];
#define hex_asc_lo(x)   hex_asc[((x) & 0x0f)]
#define hex_asc_hi(x)   hex_asc[((x) & 0xf0) >> 4]
extern int hex_to_bin(char ch);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/kernel.h $ $Rev: 838597 $")
#endif
