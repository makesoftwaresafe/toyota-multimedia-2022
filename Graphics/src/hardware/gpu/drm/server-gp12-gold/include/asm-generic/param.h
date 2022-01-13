#ifndef __ASM_GENERIC_PARAM_H
#define __ASM_GENERIC_PARAM_H

#include <uapi/asm-generic/param.h>

# undef HZ
# define HZ		CONFIG_HZ		/* Internal kernel timer frequency */
# define USER_HZ	(CONFIG_HZ / 10)	/* some user interfaces are */
# undef CLOCKS_PER_SEC
# define CLOCKS_PER_SEC	(USER_HZ)       /* in "ticks" like times() */
#endif /* __ASM_GENERIC_PARAM_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/asm-generic/param.h $ $Rev: 836322 $")
#endif
