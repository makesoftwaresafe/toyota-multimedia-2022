#ifndef __QNX_LINUX_POLL_H
#define __QNX_LINUX_POLL_H

#include <linux/compiler.h>
#include <linux/ktime.h>
#include <linux/wait.h>
#include <linux/string.h>
#include <linux/fs.h>
#include <linux/sysctl.h>
#include <asm/uaccess.h>
#include <uapi/linux/poll.h>

static inline unsigned long poll_requested_events(const poll_table *p)
{
	return p ? p->_key : ~0UL;
}

#endif /* __QNX_LINUX_POLL_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/poll.h $ $Rev: 841929 $")
#endif
