#ifndef _GENERIC_STATFS_H
#define _GENERIC_STATFS_H

#include <uapi/asm-generic/statfs.h>

typedef __kernel_fsid_t	fsid_t;
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/statfs.h $ $Rev: 836322 $")
#endif
