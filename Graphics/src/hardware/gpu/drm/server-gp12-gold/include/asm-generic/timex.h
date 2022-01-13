#ifndef __ASM_GENERIC_TIMEX_H
#define __ASM_GENERIC_TIMEX_H

#include <asm/tsc.h>

#if 0
/*
 * If you have a cycle counter, return the value here.
 */
typedef unsigned long cycles_t;
#ifndef get_cycles
static inline cycles_t get_cycles(void)
{
	return 0;
}
#endif

#endif
/*
 * Architectures are encouraged to implement read_current_timer
 * and define this in order to avoid the expensive delay loop
 * calibration during boot.
 */
#undef ARCH_HAS_READ_CURRENT_TIMER

#endif /* __ASM_GENERIC_TIMEX_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/timex.h $ $Rev: 836322 $")
#endif
