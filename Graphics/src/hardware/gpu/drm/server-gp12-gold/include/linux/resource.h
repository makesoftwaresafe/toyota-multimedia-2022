#ifndef _LINUX_RESOURCE_H
#define _LINUX_RESOURCE_H

#include <uapi/linux/resource.h>


struct task_struct;
struct rlimit;

//int getrusage(struct task_struct *p, int who, struct rusage __user *ru);
int do_prlimit(struct task_struct *tsk, unsigned int resource,
		struct rlimit *new_rlim, struct rlimit *old_rlim);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/resource.h $ $Rev: 836322 $")
#endif
