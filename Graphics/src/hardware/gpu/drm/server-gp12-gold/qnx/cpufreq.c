#include <linux/cpufreq.h>

static struct cpufreq_policy __policy = {
	.cpuinfo = {
		.max_freq = -1, 
		.min_freq = -1,
	},
};

struct cpufreq_policy *
cpufreq_cpu_get(unsigned int cpu)
{
	(void)cpu;
	if(__policy.cpuinfo.max_freq == -1){
		__policy.cpuinfo.max_freq = CPU_FREQ; /*TODO. QNX */
		__policy.cpuinfo.min_freq = CPU_FREQ; /*TODO. QNX */
	}
	return &__policy;
}

void cpufreq_cpu_put(struct cpufreq_policy *policy)
{
	(void)policy;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/cpufreq.c $ $Rev: 836322 $")
#endif
