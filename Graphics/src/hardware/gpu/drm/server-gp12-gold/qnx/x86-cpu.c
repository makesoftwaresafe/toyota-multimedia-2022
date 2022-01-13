#include <linux/qnx.h>
#include <linux/linux.h>
#include <drm/drmP.h>

struct cpuinfo_x86	boot_cpu_data = { .x86_clflush_size = 64, .x86 = 6 };

void cpu_init()
{
	cpu_detect(&boot_cpu_data);
	DRM_DEBUG("cpu info: x86_clflush_size = %d, x86_cache_alignment=%d\n",
			   boot_cpu_data.x86_clflush_size, boot_cpu_data.x86_cache_alignment);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/x86-cpu.c $ $Rev: 836322 $")
#endif
