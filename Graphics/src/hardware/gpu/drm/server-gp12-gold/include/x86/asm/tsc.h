/*
 * x86 TSC related functions
 */
#ifndef _ASM_X86_TSC_H
#define _ASM_X86_TSC_H

#ifdef __QNXNTO__
#include <sys/neutrino.h>
#include <inttypes.h>
#endif /* __QNXNTO__ */

#include <asm/processor.h>

#define NS_SCALE	10 /* 2^10, carefully chosen */
#define US_SCALE	32 /* 2^32, arbitralrily chosen */

/*
 * Standard way to access the cycle counter.
 */
typedef unsigned long long cycles_t;

extern unsigned int cpu_khz;
//extern unsigned int tsc_khz;

extern void disable_TSC(void);

static inline cycles_t get_cycles(void)
{
    return ClockCycles();
}

extern void tsc_init(void);
extern void mark_tsc_unstable(char *reason);
extern int unsynchronized_tsc(void);
extern int check_tsc_unstable(void);
extern int check_tsc_disabled(void);
extern unsigned long native_calibrate_tsc(void);
extern unsigned long long native_sched_clock_from_tsc(u64 tsc);

extern int tsc_clocksource_reliable;

/*
 * Boot-time check whether the TSCs are synchronized across
 * all CPUs/cores:
 */
extern void check_tsc_sync_source(int cpu);
extern void check_tsc_sync_target(void);

extern int notsc_setup(char *);
extern void tsc_save_sched_clock_state(void);
extern void tsc_restore_sched_clock_state(void);

/* MSR based TSC calibration for Intel Atom SoC platforms */
unsigned long try_msr_calibrate_tsc(void);

#endif /* _ASM_X86_TSC_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/x86/asm/tsc.h $ $Rev: 836322 $")
#endif
