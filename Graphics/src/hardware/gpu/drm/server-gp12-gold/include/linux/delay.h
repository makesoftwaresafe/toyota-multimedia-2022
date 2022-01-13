#ifndef _LINUX_DELAY_H
#define _LINUX_DELAY_H

/*
 * Copyright (C) 1993 Linus Torvalds
 *
 * Delay routines, using a pre-computed "loops_per_jiffy" value.
 */

#include <linux/kernel.h>

extern unsigned long loops_per_jiffy;

#include <asm/delay.h>

#ifdef __QNXNTO__
#include <unistd.h>
#endif /* __QNXNTO__ */

/*
 * Using udelay() for intervals greater than a few milliseconds can
 * risk overflow for high loops_per_jiffy (high bogomips) machines. The
 * mdelay() provides a wrapper to prevent this.  For delays greater
 * than MAX_UDELAY_MS milliseconds, the wrapper is used.  Architecture
 * specific values can be defined in asm-???/delay.h as an override.
 * The 2nd mdelay() definition ensures GCC will optimize away the 
 * while loop for the common cases where n <= MAX_UDELAY_MS  --  Paul G.
 */

#ifndef MAX_UDELAY_MS
#define MAX_UDELAY_MS	5
#endif

extern unsigned long lpj_fine;
void calibrate_delay(void);
unsigned long msleep_interruptible(unsigned int msecs);

/* milliseconds */
static inline void usleep_range(unsigned long min, unsigned long max)
{
	if (min >= 1000) {
		usleep((_Uint32t)min);
	} else {
		const struct timespec ts = {
			.tv_sec = 0,
			.tv_nsec = 1000 * min};
		nanosleep(&ts, NULL);
	}
}

static void inline msleep(unsigned int ms){
	usleep_range(ms * 1000, ms * 1000);
}
static void inline mdelay(unsigned long ms) {
	delay(ms);
}

static inline void ssleep(unsigned int seconds)
{
	msleep(seconds * 1000);
}

#endif /* defined(_LINUX_DELAY_H) */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/delay.h $ $Rev: 836322 $")
#endif
