#ifndef _QNX_LINUX_JIFFIES_H
#define _QNX_LINUX_JIFFIES_H

#include <sys/syspage.h>
#include <linux/timer.h>

#undef HZ
#define HZ CONFIG_HZ

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

/* TICK_NSEC is the time between ticks in nsec assuming SHIFTED_HZ */
#define TICK_NSEC ((NSEC_PER_SEC+HZ/2)/HZ)

/* TICK_USEC is the time between ticks in usec assuming fake USER_HZ */
#define TICK_USEC ((1000000UL + USER_HZ/2) / USER_HZ)

#define MAX_JIFFY_OFFSET ((~0UL >> 1)-1)

#if BITS_PER_LONG < 64
# define MAX_SEC_IN_JIFFIES \
	(long)((u64)((u64)MAX_JIFFY_OFFSET * TICK_NSEC) / NSEC_PER_SEC)
#else	/* take care of overflow on 64 bits machines */
    /* Suppose we want to divide two numbers NOM and DEN: NOM/DEN, then we can
     * improve accuracy by shifting LSH bits, hence calculating:
     *     (NOM << LSH) / DEN
     * This however means trouble for large NOM, because (NOM << LSH) may no
     * longer fit in 32 bits. The following way of calculating this gives us
     * some slack, under the following conditions:
     *   - (NOM / DEN) fits in (32 - LSH) bits.
     *   - (NOM % DEN) fits in (32 - LSH) bits.
     */
    #define SH_DIV(NOM,DEN,LSH) (   (((NOM) / (DEN)) << (LSH)) + ((((NOM) % (DEN)) << (LSH)) + (DEN) / 2) / (DEN))
    #define MAX_SEC_IN_JIFFIES (SH_DIV((MAX_JIFFY_OFFSET >> SEC_JIFFIE_SC) * TICK_NSEC, NSEC_PER_SEC, 1) - 1)
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

extern unsigned int jiffies_to_msecs(const unsigned long j);
extern unsigned int jiffies_to_usecs(const unsigned long j);
unsigned long msecs_to_jiffies(const unsigned int m);

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

extern unsigned long timespec_to_jiffies(const struct timespec *value);

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

extern unsigned long usecs_to_jiffies(const unsigned int u);
extern u64 nsecs_to_jiffies64(u64 n);
unsigned long nsecs_to_jiffies(u64 n);

#define typecheck(type,x) \
({	type __dummy; \
	typeof(x) __dummy2; \
	(void)(&__dummy == &__dummy2); \
	1; \
})

#define time_after(a,b)		\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)(b) - (long)(a) < 0))

#define time_before(a,b)	time_after(b,a)

#define time_after_eq(a,b)	\
	(typecheck(unsigned long, a) && \
	 typecheck(unsigned long, b) && \
	 ((long)(a) - (long)(b) >= 0))

#define time_before_eq(a,b)	time_after_eq(b,a)

#define time_in_range(a,b,c) \
	(time_after_eq(a,b) && \
	 time_before_eq(a,c))

/* time_is_before_jiffies(a) return true if a is before jiffies */
#define time_is_before_jiffies(a) time_after(jiffies, a)

/* time_is_after_jiffies(a) return true if a is after jiffies */
#define time_is_after_jiffies(a) time_before(jiffies, a)

/* time_is_before_eq_jiffies(a) return true if a is before or equal to jiffies*/
#define time_is_before_eq_jiffies(a) time_after_eq(jiffies, a)

/* time_is_after_eq_jiffies(a) return true if a is after or equal to jiffies*/
#define time_is_after_eq_jiffies(a) time_before_eq(jiffies, a)

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/jiffies.h $ $Rev: 836322 $")
#endif
