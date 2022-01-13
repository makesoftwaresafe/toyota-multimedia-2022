/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/bottom_half.h with __QNXNTO__ indicating modifications.
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

#ifndef _LINUX_BH_H
#define _LINUX_BH_H

#include <linux/preempt.h>
#include <linux/preempt_mask.h>

#ifdef CONFIG_TRACE_IRQFLAGS
extern void __local_bh_disable_ip(unsigned long ip, unsigned int cnt);
#else
static __always_inline void __local_bh_disable_ip(unsigned long ip, unsigned int cnt)
{
	preempt_count_add(cnt);
	barrier();
}
#endif

static inline void local_bh_disable(void)
{
#ifndef __QNXNTO__
	__local_bh_disable_ip(_THIS_IP_, SOFTIRQ_DISABLE_OFFSET);
#endif
}

extern void _local_bh_enable(void);
extern void __local_bh_enable_ip(unsigned long ip, unsigned int cnt);

static inline void local_bh_enable_ip(unsigned long ip)
{
	__local_bh_enable_ip(ip, SOFTIRQ_DISABLE_OFFSET);
}

static inline void local_bh_enable(void)
{
#ifndef __QNXNTO__
	__local_bh_enable_ip(_THIS_IP_, SOFTIRQ_DISABLE_OFFSET);
#endif
}

#endif /* _LINUX_BH_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/bottom_half.h $ $Rev: 836043 $")
#endif
