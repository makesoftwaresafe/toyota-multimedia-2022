#ifndef __QNX_LINUX_NS_COMMON_H
#define __QNX_LINUX_NS_COMMON_H

struct proc_ns_operations;

struct ns_common {
	atomic_long_t stashed;
	const struct proc_ns_operations *ops;
	unsigned int inum;
};

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/ns_common.h $ $Rev: 836322 $")
#endif
