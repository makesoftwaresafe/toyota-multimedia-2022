#ifndef _LINUX_STDDEF_H
#define _LINUX_STDDEF_H

#ifdef __QNXNTO__
#include <stddef.h>
#endif /* __QNXNTO__ */

#include <uapi/linux/stddef.h>

/**
 * offsetofend(TYPE, MEMBER)
 *
 * @TYPE: The type of the structure
 * @MEMBER: The member within the structure to get the end offset of
 */
#define offsetofend(TYPE, MEMBER) \
	(offsetof(TYPE, MEMBER)	+ sizeof(((TYPE *)0)->MEMBER))

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/stddef.h $ $Rev: 836322 $")
#endif
