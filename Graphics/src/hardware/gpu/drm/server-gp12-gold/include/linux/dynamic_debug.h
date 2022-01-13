#ifndef _DYNAMIC_DEBUG_H
#define _DYNAMIC_DEBUG_H

#if defined(CONFIG_DYNAMIC_DEBUG)

#else

#include <linux/string.h>
#include <linux/errno.h>

#endif


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/dynamic_debug.h $ $Rev: 836322 $")
#endif
