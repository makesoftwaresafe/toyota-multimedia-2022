/*
* Copyright (c) 2017 QNX Software Systems.
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

#ifndef __QNX_LINUX_H
#define __QNX_LINUX_H

#include <linux/version.h>
#include <linux/kconfig.h>
#include <linux/compiler.h>
#include <linux/printk.h> 
#include <linux/types.h>
#include <linux/limits.h>
#include <linux/debug.h>
#include <linux/lockdep.h>
#include <linux/kernel.h>
#include <linux/kern_levels.h>
#include <linux/export.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/bug.h>
#include <linux/err.h>
#include <asm/barrier.h>
#include <linux/bitops.h>
#include <linux/find.h> 

#include <linux/sizes.h>
#include <linux/errno.h>

#include <linux/string.h>
#include <linux/stringify.h>
#include <linux/ioctl.h>
#include <linux/math64.h>

#include <linux/stat.h>

#include <linux/atomic.h>
#include <linux/spinlock.h> /* depend on atomic.h */
#include <linux/mutex.h>
#include <linux/srcu.h>
#include <linux/rwsem.h>

#include <linux/kref.h>
#include <linux/swab.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/plist.h>
#include <linux/klist.h>

#include <linux/firmware.h>

#include <linux/pci.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <asm/page.h>

#include <linux/slab.h>
#include <linux/log2.h>
#include <linux/capability.h>

#include <linux/seq_file.h>
#include <linux/scatterlist.h>  /* depend on page.h */
#include <linux/dma-buf.h>  /* depend on scatterlist.h */
#include <linux/dma-mapping.h>  /* depend on scatterlist.h */

#include <linux/wait.h>
#include <linux/io.h>
#include <linux/rwlock.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/kobject.h>
#include <linux/timer.h>
#include <linux/workqueue.h>

#include <linux/pm.h>
#include <linux/pm_qos.h> /* depend on workqueue.h */

#include <linux/seqlock.h>

#include <linux/uidgid.h>
#include <linux/pid.h>
#include <linux/sched.h>

#include <linux/iova.h>
#include <linux/iommu.h>

#include <linux/vmalloc.h>
#include <linux/qnx_misc.h>

#include <linux/ktime.h>

#include <linux/notifier.h>
#include <linux/clk-provider.h>
#include <linux/clk.h>
#include <linux/clkdev.h>

#include <linux/ns_common.h>
#include <linux/user_namespace.h>
#include <linux/dcache.h>
#include <linux/fs.h> /* depend on mm.h */
#include <linux/sysfs.h> /* depend on mm.h */
#include <linux/debugfs.h> /* depend on fs.h */
#include <linux/device.h> /* depend on fs.h */
#include <linux/i2c.h> /*depend on device.h */

#include <linux/timekeeping.h>
#include <linux/property.h>
#include <linux/pm_runtime.h>
#include <linux/of.h>

#endif //__QNX_LINUX_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/linux.h $ $Rev: 838597 $")
#endif
