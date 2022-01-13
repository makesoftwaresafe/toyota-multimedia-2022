#ifndef _LINUX_BH_H
#define _LINUX_BH_H

#include <linux/preempt.h>

#ifdef __QNXNTO__
static __always_inline void __local_bh_disable_ip(unsigned long ip, unsigned int cnt)
{
	__preempt_count_add_ip(cnt, ip);
}
static __always_inline void __local_bh_enable_ip(unsigned long ip, unsigned int cnt)
{
	__preempt_count_sub_ip(cnt, ip);
}
#else

#ifdef CONFIG_TRACE_IRQFLAGS
extern void __local_bh_disable_ip(unsigned long ip, unsigned int cnt);
#else
static __always_inline void __local_bh_disable_ip(unsigned long ip, unsigned int cnt)
{
	preempt_count_add(cnt);
	barrier();
}
#endif

#endif	/* __QNXNTO__ */

static inline void local_bh_disable(void)
{
	__local_bh_disable_ip(_THIS_IP_, SOFTIRQ_DISABLE_OFFSET);
}

extern void _local_bh_enable(void);
extern void __local_bh_enable_ip(unsigned long ip, unsigned int cnt);

static inline void local_bh_enable_ip(unsigned long ip)
{
	__local_bh_enable_ip(ip, SOFTIRQ_DISABLE_OFFSET);
}

static inline void local_bh_enable(void)
{
	__local_bh_enable_ip(_THIS_IP_, SOFTIRQ_DISABLE_OFFSET);
}

#endif /* _LINUX_BH_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/bottom_half.h $ $Rev: 873525 $")
#endif
