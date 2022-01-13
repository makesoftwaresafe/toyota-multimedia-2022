/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/bitops.h with __QNXNTO__ indicating modifications.
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

#ifndef _LINUX_BITOPS_H
#define _LINUX_BITOPS_H
#ifndef __QNXNTO__
#include <asm/types.h>
#else
#include <linux/compiler.h>
#include <linux/types.h>
#endif // __QNXNTO__

#ifdef	__KERNEL__
#define BIT(nr)			(1UL << (nr))
#define BIT_ULL(nr)		(1ULL << (nr))
#define BIT_MASK(nr)		(1UL << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BIT_ULL_MASK(nr)	(1ULL << ((nr) % BITS_PER_LONG_LONG))
#define BIT_ULL_WORD(nr)	((nr) / BITS_PER_LONG_LONG)
#define BITS_PER_BYTE		8
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#endif

/*
 * Create a contiguous bitmask starting at bit position @l and ending at
 * position @h. For example
 * GENMASK_ULL(39, 21) gives us the 64bit vector 0x000000ffffe00000.
 */
#define GENMASK(h, l) \
	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))

#define GENMASK_ULL(h, l) \
	(((~0ULL) << (l)) & (~0ULL >> (BITS_PER_LONG_LONG - 1 - (h))))

extern unsigned int __sw_hweight8(unsigned int w);
extern unsigned int __sw_hweight16(unsigned int w);
extern unsigned int __sw_hweight32(unsigned int w);
extern unsigned long __sw_hweight64(__u64 w);

/*
 * Include this here because some architectures need generic_ffs/fls in
 * scope
 */
#include <asm/bitops.h>

#define for_each_set_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size));		\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

/* same as for_each_set_bit() but use bit as value to start with */
#define for_each_set_bit_from(bit, addr, size) \
	for ((bit) = find_next_bit((addr), (size), (bit));	\
	     (bit) < (size);					\
	     (bit) = find_next_bit((addr), (size), (bit) + 1))

#define for_each_clear_bit(bit, addr, size) \
	for ((bit) = find_first_zero_bit((addr), (size));	\
	     (bit) < (size);					\
	     (bit) = find_next_zero_bit((addr), (size), (bit) + 1))

/* same as for_each_clear_bit() but use bit as value to start with */
#define for_each_clear_bit_from(bit, addr, size) \
	for ((bit) = find_next_zero_bit((addr), (size), (bit));	\
	     (bit) < (size);					\
	     (bit) = find_next_zero_bit((addr), (size), (bit) + 1))

#ifndef __QNXNTO__
static __inline__ int get_bitmask_order(unsigned int count)
#else
static inline int get_bitmask_order(unsigned int count)
#endif
{
	int order;

	order = fls(count);
	return order;	/* We could be slightly more clever with -1 here... */
}

#ifndef __QNXNTO__
static __inline__ int get_count_order(unsigned int count)
#else
static inline int get_count_order(unsigned int count)
#endif
{
	int order;

	order = fls(count) - 1;
	if (count & (count - 1))
		order++;
	return order;
}

static inline unsigned long hweight_long(unsigned long w)
{
	return sizeof(w) == 4 ? hweight32(w) : hweight64(w);
}

/**
 * rol64 - rotate a 64-bit value left
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u64 rol64(__u64 word, unsigned int shift)
#else
static inline uint64_t rol64(uint64_t word, unsigned int shift)
#endif
{
	return (word << shift) | (word >> (64 - shift));
}

/**
 * ror64 - rotate a 64-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u64 ror64(__u64 word, unsigned int shift)
#else
static inline uint64_t ror64(uint64_t word, unsigned int shift)
#endif
{
	return (word >> shift) | (word << (64 - shift));
}

/**
 * rol32 - rotate a 32-bit value left
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u32 rol32(__u32 word, unsigned int shift)
#else
static inline uint32_t rol32(uint32_t word, unsigned int shift)
#endif
{
	return (word << shift) | (word >> (32 - shift));
}

/**
 * ror32 - rotate a 32-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u32 ror32(__u32 word, unsigned int shift)
#else
static inline uint32_t ror32(uint32_t word, unsigned int shift)
#endif
{
	return (word >> shift) | (word << (32 - shift));
}

/**
 * rol16 - rotate a 16-bit value left
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u16 rol16(__u16 word, unsigned int shift)
#else
static inline uint16_t rol16(uint16_t word, unsigned int shift)
#endif
{
	return (word << shift) | (word >> (16 - shift));
}

/**
 * ror16 - rotate a 16-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u16 ror16(__u16 word, unsigned int shift)
#else
static inline uint16_t ror16(uint16_t word, unsigned int shift)
#endif
{
	return (word >> shift) | (word << (16 - shift));
}

/**
 * rol8 - rotate an 8-bit value left
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u8 rol8(__u8 word, unsigned int shift)
#else
static inline uint8_t rol8(uint8_t word, unsigned int shift)
#endif
{
	return (word << shift) | (word >> (8 - shift));
}

/**
 * ror8 - rotate an 8-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
#ifndef __QNXNTO__
static inline __u8 ror8(__u8 word, unsigned int shift)
#else
static inline uint8_t ror8(uint8_t word, unsigned int shift)
#endif
{
	return (word >> shift) | (word << (8 - shift));
}

/**
 * sign_extend32 - sign extend a 32-bit value using specified bit as sign-bit
 * @value: value to sign extend
 * @index: 0 based bit index (0<=index<32) to sign bit
 */
#ifndef __QNXNTO__
static inline __s32 sign_extend32(__u32 value, int index)
#else
static inline int32_t sign_extend32(uint32_t value, int index)
#endif
{
#ifdef __QNXNTO__
	uint8_t shift = 31 - index;
	return (int32_t)(value << shift) >> shift;
#else
	__u8 shift = 31 - index;
	return (__s32)(value << shift) >> shift;
#endif
}

/**
 * sign_extend64 - sign extend a 64-bit value using specified bit as sign-bit
 * @value: value to sign extend
 * @index: 0 based bit index (0<=index<64) to sign bit
 */
#ifndef __QNXNTO__
static inline __s64 sign_extend64(__u64 value, int index)
{
	__u8 shift = 63 - index;
	return (__s64)(value << shift) >> shift;
}
#endif

static inline unsigned fls_long(unsigned long l)
{
	if (sizeof(l) == 4)
		return fls(l);
	return fls64(l);
}

/**
 * __ffs64 - find first set bit in a 64 bit word
 * @word: The 64 bit word
 *
 * On 64 bit arches this is a synomyn for __ffs
 * The result is not defined if no bits are set, so check that @word
 * is non-zero before calling this.
 */
#ifndef __QNXNTO__
static inline unsigned long __ffs64(u64 word)
{
#if BITS_PER_LONG == 32
	if (((u32)word) == 0UL)
		return __ffs((u32)(word >> 32)) + 32;
#elif BITS_PER_LONG != 64
#error BITS_PER_LONG not 32 or 64
#endif
	return __ffs((unsigned long)word);
}
#else
static inline unsigned long __ffs64(uint64_t word)
{
#if BITS_PER_LONG == 32
	if (((uint32_t)word) == 0UL)
		return __ffs((uint32_t)(word >> 32)) + 32;
#elif BITS_PER_LONG != 64
#error BITS_PER_LONG not 32 or 64
#endif
	return __ffs((unsigned long)word);
}
#endif //__QNXNTO__

#ifdef __KERNEL__

#ifndef set_mask_bits
#define set_mask_bits(ptr, _mask, _bits)	\
({								\
	const typeof(*ptr) mask = (_mask), bits = (_bits);	\
	typeof(*ptr) old, new;					\
								\
	do {							\
		old = ACCESS_ONCE(*ptr);			\
		new = (old & ~mask) | bits;			\
	} while (cmpxchg(ptr, old, new) != old);		\
								\
	new;							\
})
#endif

#ifndef find_last_bit
/**
 * find_last_bit - find the last set bit in a memory region
 * @addr: The address to start the search at
 * @size: The number of bits to search
 *
 * Returns the bit number of the last set bit, or size.
 */
extern unsigned long find_last_bit(const unsigned long *addr,
				   unsigned long size);
#endif

#endif /* __KERNEL__ */
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/bitops.h $ $Rev: 838597 $")
#endif
