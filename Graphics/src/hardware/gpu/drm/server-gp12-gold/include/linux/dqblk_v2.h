/*
 *  Definitions for vfsv0 quota format
 */

#ifndef _LINUX_DQBLK_V2_H
#define _LINUX_DQBLK_V2_H

#include <linux/dqblk_qtree.h>

/* Numbers of blocks needed for updates */
#define V2_INIT_ALLOC QTREE_INIT_ALLOC
#define V2_INIT_REWRITE QTREE_INIT_REWRITE
#define V2_DEL_ALLOC QTREE_DEL_ALLOC
#define V2_DEL_REWRITE QTREE_DEL_REWRITE

#endif /* _LINUX_DQBLK_V2_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/dqblk_v2.h $ $Rev: 836322 $")
#endif
