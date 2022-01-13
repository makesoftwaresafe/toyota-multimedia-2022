#ifndef __QNXNTO_QNX_H
#define __QNXNTO_QNX_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <mem.h>
#include <math.h>
#include <signal.h>
#include <pthread.h>
#include <ioctl.h>
#include <limits.h>
#include <inttypes.h>
#include <fcntl.h>
#include <dirent.h>

#include <hw/inout.h>

#include <sys/neutrino.h>
#include <sys/iofunc.h>
#include <sys/iomgr.h>
#include <sys/iomsg.h>
#include <sys/dispatch.h>
#include <sys/rsrcdbmgr.h>
#include <sys/procmgr.h>
#include <sys/slogcodes.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>

#include <linux/kconfig.h>
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/const.h>
#include <linux/limits.h>
#include <linux/ctype.h>

/*
 * special qnx related functions
 */
enum qnx_priority {
	QNX_PRTY_MAX = 0,
	QNX_PRTY_HIGH = -5,
	QNX_PRTY_KERNEL = -10,
	QNX_PRTY_DRM_MIN = QNX_PRTY_KERNEL,
	QNX_PRTY_DEFAULT = -9999,
	QNX_PRTY_USER_MAX = 20,
};

struct task_struct;
extern int qnx_get_priority(enum qnx_priority prio,
		struct task_struct *task);
extern int qnx_taskattr_init(pthread_attr_t *attr,
		enum qnx_priority prio);
extern int qnx_current_task_sched(struct task_struct *task);
extern int qnx_create_task_sched(struct task_struct *task);
extern void qnx_destroy_task_sched(struct task_struct *task);
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/qnx.h $ $Rev: 848328 $")
#endif
