#ifndef __QNX_LINUX_H
#define __QNX_LINUX_H

#include <linux/kconfig.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/const.h>
#include <linux/limits.h>
#include <linux/ctype.h>
#include <linux/cache.h>
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
#include <linux/tracepoint.h>
#include <linux/find.h> 

#include <linux/string.h>
#include <asm/string.h>
#include <linux/stringify.h>
#include <linux/ioctl.h>

#include <linux/math64.h>

#include <linux/swab.h>
#include <linux/bottom_half.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/plist.h>
#include <linux/klist.h>
#include <linux/hashtable.h>

#include <linux/stat.h>

#include <linux/atomic.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/rcupdate.h>
#include <linux/srcu.h>
#include <linux/rwsem.h>

#include <linux/hash.h>

#include <linux/wait.h>
#include <linux/idr.h>
#include <linux/io.h>
#include <linux/rwlock.h>
#include <linux/completion.h>
#include <linux/mm.h>
#include <linux/kref.h>
#include <linux/kobject.h>
#include <linux/timer.h>
#include <linux/jiffies.h> /* depend on timer.h */
#include <linux/workqueue.h>

#include <linux/notifier.h>
#include <linux/pm.h>
#include <linux/pm_qos.h> /* depend on workqueue.h */

#include <linux/agp_backend.h>

#include <linux/irqreturn.h>
#include <linux/cpufreq.h>  /* replace for other cpu here */

#include <asm/page.h>
#include <asm/pgtable.h>

#include <linux/slab.h>

#include <linux/seq_file.h>
#include <linux/scatterlist.h>  /* depend on page.h */

#include <linux/log2.h>
#include <asm-generic/getorder.h>
#include <linux/capability.h>

#include <linux/uidgid.h>
#include <linux/pid.h>
#include <linux/sched.h>
#include <linux/sched/init.h>

#include <linux/uaccess.h> /* need current */
#include <linux/vmalloc.h>
#include <linux/compat.h>

#include <linux/processor.h>

#include <linux/seqlock.h>

#include <linux/ww_mutex.h>

#include <linux/writeback.h>
#include <linux/ns_common.h>
#include <linux/proc_ns.h>
#include <linux/user_namespace.h>
#include <linux/dcache.h>
#include <linux/fs.h> /* depend on mm.h */
#include <linux/sysfs.h> /* depend on mm.h */
#include <linux/debugfs.h> /* depend on fs.h */
#include <linux/printk.h> 
#include <linux/device.h> /* depend on fs.h */
#include <linux/mount.h> 
#include <linux/pagemap.h> /*depend on fs.h */
#include <linux/page-flags.h> /*depend on fs.h */

#include <linux/dcache.h>

#include <linux/shmem_fs.h>

#include <linux/i2c.h> /*depend on device.h */

#include <linux/fb.h>

#include <linux/hdmi.h>
#include <asm/device.h>
#include <linux/platform_device.h>
#include <linux/firmware.h>

#include <linux/kfifo.h>

#include <linux/scatterlist.h>
#include <linux/dma-buf.h>  /* depend on scatterlist.h */
#include <linux/dma-mapping.h>  /* depend on scatterlist.h */

#include <linux/pci.h>
#include <linux/irqreturn.h>
#include <linux/interrupt.h>
#include <linux/vgaarb.h>

#include <linux/preempt.h>
#include <linux/ktime.h>

#include <linux/usb.h>

#include <linux/i2c-algo-bit.h>

#include <linux/vga_switcheroo.h>
#include <linux/shrinker.h>

#include <linux/acpi/video.h>
#include <linux/acpi/actypes.h>

#include <linux/dmi.h>
#include <linux/backlight.h>

#include <linux/rbtree_augmented.h>
#include <linux/rbtree.h>

#include <linux/qnx_misc.h>

#include <linux/async.h>

#include <linux/dma-fence.h>
#include <linux/timekeeping.h>
#include <linux/of_device.h>
#include <linux/pm_runtime.h>
#include <linux/property.h>
#include <linux/of.h>
#include <linux/of_graph.h>
#include <linux/reboot.h>

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/linux.h $ $Rev: 836935 $")
#endif
