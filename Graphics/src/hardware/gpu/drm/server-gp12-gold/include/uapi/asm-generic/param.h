#ifndef _UAPI__ASM_GENERIC_PARAM_H
#define _UAPI__ASM_GENERIC_PARAM_H

#ifndef HZ
#define HZ CONFIG_HZ
#endif

#ifndef EXEC_PAGESIZE
#define EXEC_PAGESIZE	4096
#endif

#ifndef NOGROUP
#define NOGROUP		(-1)
#endif

#define MAXHOSTNAMELEN	64	/* max length of hostname */


#endif /* _UAPI__ASM_GENERIC_PARAM_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/uapi/asm-generic/param.h $ $Rev: 836322 $")
#endif
