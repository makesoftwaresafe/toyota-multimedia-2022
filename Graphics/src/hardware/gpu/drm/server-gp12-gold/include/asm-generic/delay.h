#ifndef __ASM_GENERIC_DELAY_H
#define __ASM_GENERIC_DELAY_H

#ifdef __QNXNTO__
#include <unistd.h>
#endif /* __QNXNTO__ */

/* Undefined functions to get compile-time errors */
extern void __bad_udelay(void);
extern void __bad_ndelay(void);

extern void __udelay(unsigned long usecs);
extern void __ndelay(unsigned long nsecs);
extern void __const_udelay(unsigned long xloops);
extern void __delay(unsigned long loops);

/*
 * The weird n/20000 thing suppresses a "comparison is always false due to
 * limited range of data type" warning with non-const 8-bit arguments.
 */

/* 0x10c7 is 2**32 / 1000000 (rounded up) */
static void inline udelay(unsigned long j) {
	if(j >= 1000){
		usleep((_Uint32t)j);
	}else {
		struct timespec ts;
		ts.tv_sec  = 0;
		ts.tv_nsec = j * 1000;
		nanospin(&ts);
	}
}

/* 0x5 is 2**32 / 1000000000 (rounded up) */
static inline void ndelay(unsigned long x)
{
	struct timespec ts = {
		.tv_sec = 0,
		.tv_nsec = x,
	};
	nanospin(&ts);
}

#endif /* __ASM_GENERIC_DELAY_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/delay.h $ $Rev: 836322 $")
#endif
