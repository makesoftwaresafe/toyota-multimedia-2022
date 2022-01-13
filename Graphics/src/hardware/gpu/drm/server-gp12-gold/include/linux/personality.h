#ifndef _LINUX_PERSONALITY_H
#define _LINUX_PERSONALITY_H

#include <uapi/linux/personality.h>

/*
 * Return the base personality without flags.
 */
#define personality(pers)	(pers & PER_MASK)

/*
 * Change personality of the currently running process.
 */
#define set_personality(pers)	(current->personality = (pers))

#endif /* _LINUX_PERSONALITY_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/personality.h $ $Rev: 836322 $")
#endif
