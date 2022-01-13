/*
 *	File with in-memory structures of old quota format
 */

#ifndef _LINUX_DQBLK_V1_H
#define _LINUX_DQBLK_V1_H

/* Numbers of blocks needed for updates */
#define V1_INIT_ALLOC 1
#define V1_INIT_REWRITE 1
#define V1_DEL_ALLOC 0
#define V1_DEL_REWRITE 2

#endif	/* _LINUX_DQBLK_V1_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/dqblk_v1.h $ $Rev: 836322 $")
#endif
