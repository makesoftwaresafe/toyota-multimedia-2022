/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/jiffies.h with some modifications.
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

#ifndef _LINUX_JIFFIES_H
#define _LINUX_JIFFIES_H

#include <sys/syspage.h>
#include <linux/timer.h>

//#define HZ CLOCKS_PER_SEC
#define HZ 1000

#define USER_HZ HZ

#if HZ >= 12 && HZ < 24
# define SHIFT_HZ	4
#elif HZ >= 24 && HZ < 48
# define SHIFT_HZ	5
#elif HZ >= 48 && HZ < 96
# define SHIFT_HZ	6
#elif HZ >= 96 && HZ < 192
# define SHIFT_HZ	7
#elif HZ >= 192 && HZ < 384
# define SHIFT_HZ	8
#elif HZ >= 384 && HZ < 768
# define SHIFT_HZ	9
#elif HZ >= 768 && HZ < 1536
# define SHIFT_HZ	10
#elif HZ >= 1536 && HZ < 3072
# define SHIFT_HZ	11
#elif HZ >= 3072 && HZ < 6144
# define SHIFT_HZ	12
#elif HZ >= 6144 && HZ < 12288
# define SHIFT_HZ	13
#else
# error Invalid value of HZ.
#endif

/* Suppose we want to divide two numbers NOM and DEN: NOM/DEN, then we can
 * improve accuracy by shifting LSH bits, hence calculating:
 *     (NOM << LSH) / DEN
 * This however means trouble for large NOM, because (NOM << LSH) may no
 * longer fit in 32 bits. The following way of calculating this gives us
 * some slack, under the following conditions:
 *   - (NOM / DEN) fits in (32 - LSH) bits.
 *   - (NOM % DEN) fits in (32 - LSH) bits.
 */
#define SH_DIV(NOM,DEN,LSH) (   (((NOM) / (DEN)) << (LSH))              \
                             + ((((NOM) % (DEN)) << (LSH)) + (DEN) / 2) / (DEN))

/* TICK_NSEC is the time between ticks in nsec assuming SHIFTED_HZ */
#define TICK_NSEC ((NSEC_PER_SEC+HZ/2)/HZ)

/* TICK_USEC is the time between ticks in usec assuming fake USER_HZ */
#define TICK_USEC ((1000000UL + USER_HZ/2) / USER_HZ)

#define MAX_JIFFY_OFFSET ((~0UL >> 1)-1)

#if BITS_PER_LONG < 64
# define MAX_SEC_IN_JIFFIES \
	(long)((u64)((u64)MAX_JIFFY_OFFSET * TICK_NSEC) / NSEC_PER_SEC)
#else	/* take care of overflow on 64 bits machines */
# define MAX_SEC_IN_JIFFIES \
	(SH_DIV((MAX_JIFFY_OFFSET >> SEC_JIFFIE_SC) * TICK_NSEC, NSEC_PER_SEC, 1) - 1)

#endif


#define SEC_JIFFIE_SC (31 - SHIFT_HZ)
#if !((((NSEC_PER_SEC << 2) / TICK_NSEC) << (SEC_JIFFIE_SC - 2)) & 0x80000000)
#undef SEC_JIFFIE_SC
#define SEC_JIFFIE_SC (32 - SHIFT_HZ)
#endif

#define NSEC_JIFFIE_SC (SEC_JIFFIE_SC + 29)
#define USEC_JIFFIE_SC (SEC_JIFFIE_SC + 19)

#define SEC_CONVERSION ((unsigned long)((((u64)NSEC_PER_SEC << SEC_JIFFIE_SC) +\
                                TICK_NSEC -1) / (u64)TICK_NSEC))

#define NSEC_CONVERSION ((unsigned long)((((u64)1 << NSEC_JIFFIE_SC) +\
                                        TICK_NSEC -1) / (u64)TICK_NSEC))
#define USEC_CONVERSION  \
                    ((unsigned long)((((u64)NSEC_PER_USEC << USEC_JIFFIE_SC) +\
                                        TICK_NSEC -1) / (u64)TICK_NSEC))


static inline unsigned long round_jiffies_up_relative(unsigned long j)
{
	return j;
}
static inline unsigned long round_jiffies_up(unsigned long j)
{
	return j;
}

static inline unsigned long jiffies_to_msec(unsigned long j) {
	return j * 1000L/HZ;
}

static inline unsigned long jiffies_to_msecs(unsigned long j) {
	return j * 1000L/HZ;
}

static inline unsigned long msecs_to_jiffies(unsigned long j) {
//return j * 1000000L;
  return j * HZ/1000L;
}

static inline _uint64
jiffies_to_usecs(unsigned long j) {
	_uint64 msec = j * 1000LL/HZ;
	return msec * 1000LL;
}

static inline _uint64 /*unsigned long */
jiffies_to_nsec(unsigned long j) {
	_uint64 msec = j * 1000LL/HZ;
	return msec * 1000000LL;
}
static inline unsigned long nsec_to_jiffies(u64 nsec) {
	return ((((u64)nsec * NSEC_CONVERSION) >>
		 (NSEC_JIFFIE_SC - SEC_JIFFIE_SC))) >> SEC_JIFFIE_SC;
}
static inline uint64_t
jiffies2cycles(uint64_t jiffies){
	uint64_t cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	return jiffies * cps/HZ;
}
static inline uint64_t
cycles2jiffies(uint64_t cycles){
	uint64_t cps = SYSPAGE_ENTRY(qtime)->cycles_per_sec;
	return HZ * cycles/cps;
}


extern unsigned long _jiffies;

static inline unsigned long 
timespec_to_jiffies(const struct timespec * value){
#if 0
	uint64_t t1 = value->tv_nsec;
	t1 = (t1*HZ)/1000000000L;
	unsigned long now = value->tv_sec * HZ + (unsigned long)t1;
	return now;
#else
	unsigned long sec = value->tv_sec;
	long nsec = value->tv_nsec + TICK_NSEC - 1;

	if (sec >= MAX_SEC_IN_JIFFIES){
		sec = MAX_SEC_IN_JIFFIES;
		nsec = 0;
	}
	return (((u64)sec * SEC_CONVERSION) +
		(((u64)nsec * NSEC_CONVERSION) >>
		 (NSEC_JIFFIE_SC - SEC_JIFFIE_SC))) >> SEC_JIFFIE_SC;
#endif
}

static inline unsigned long get_jiffies(void) {
	struct timespec tp;
	unsigned long now = 0;
	if (0 == clock_gettime(CLOCK_MONOTONIC, &tp)){
		now = timespec_to_jiffies(&tp);
	}
	_jiffies = now;
	return now;
}

#define jiffies get_jiffies()
//FIXME
static inline uint64_t get_jiffies_64(void) {
	return get_jiffies();
}

static inline unsigned long usecs_to_jiffies(const unsigned int u)
{
#if HZ <= USEC_PER_SEC && !(USEC_PER_SEC % HZ)
	return (u + (USEC_PER_SEC / HZ) - 1) / (USEC_PER_SEC / HZ);
#elif HZ > USEC_PER_SEC && !(HZ % USEC_PER_SEC)
	return u * (HZ / USEC_PER_SEC);
#else
	return (USEC_TO_HZ_MUL32 * u + USEC_TO_HZ_ADJ32)
		>> USEC_TO_HZ_SHR32;
#endif
}


static inline u64 nsecs_to_jiffies64(u64 n)
{
#if (NSEC_PER_SEC % HZ) == 0
	/* Common case, HZ = 100, 128, 200, 250, 256, 500, 512, 1000 etc. */
	return div_u64(n, NSEC_PER_SEC / HZ);
#elif (HZ % 512) == 0
	/* overflow after 292 years if HZ = 1024 */
	return div_u64(n * HZ / 512, NSEC_PER_SEC / 512);
#else
	/*
	 * Generic case - optimized for cases where HZ is a multiple of 3.
	 * overflow after 64.99 years, exact for HZ = 60, 72, 90, 120 etc.
	 */
	return div_u64(n * 9, (9ull * NSEC_PER_SEC + HZ / 2) / HZ);
#endif
}


#define time_in_range(a,b,c) \
	(time_after_eq(a,b) && \
	 time_before_eq(a,c))

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/jiffies.h $ $Rev: 838597 $")
#endif
