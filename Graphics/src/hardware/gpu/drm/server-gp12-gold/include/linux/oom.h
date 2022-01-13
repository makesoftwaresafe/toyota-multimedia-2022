#ifndef __INCLUDE_LINUX_OOM_H
#define __INCLUDE_LINUX_OOM_H

#include <linux/sched.h>
#include <linux/types.h>

struct notifier_block;

extern int register_oom_notifier(struct notifier_block *nb);
extern int unregister_oom_notifier(struct notifier_block *nb);

#endif /* _INCLUDE_LINUX_OOM_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/oom.h $ $Rev: 836322 $")
#endif
