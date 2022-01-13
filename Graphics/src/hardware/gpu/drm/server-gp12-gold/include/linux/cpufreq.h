#ifndef _QNX_LINUX_CPUFREQ_H
#define _QNX_LINUX_CPUFREQ_H

#include <sys/syspage.h>
#include <stdbool.h>

#define CPU_FREQ (SYSPAGE_ENTRY(qtime)->cycles_per_sec);
#define cpufreq_quick_get_max(...) CPU_FREQ
#define tsc_khz CPU_FREQ

struct cpufreq_cpuinfo {
	unsigned int		max_freq;
	unsigned int		min_freq;

	/* in 10^(-9) s = nanoseconds */
	unsigned int		transition_latency;
};

struct cpufreq_policy {
	/* CPUs sharing clock, require sw coordination */
	struct cpufreq_cpuinfo	cpuinfo;/* see above */
};


struct cpufreq_policy *cpufreq_cpu_get(unsigned int cpu);
void cpufreq_cpu_put(struct cpufreq_policy *policy);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/cpufreq.h $ $Rev: 836322 $")
#endif
