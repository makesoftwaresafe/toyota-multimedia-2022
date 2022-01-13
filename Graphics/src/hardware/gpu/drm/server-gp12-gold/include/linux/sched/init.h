#ifndef _LINUX_SCHED_INIT_H
#define _LINUX_SCHED_INIT_H

/*
 * Scheduler init related prototypes:
 */

extern void sched_init(void);
extern void sched_init_smp(void);

#endif /* _LINUX_SCHED_INIT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/sched/init.h $ $Rev: 839737 $")
#endif
