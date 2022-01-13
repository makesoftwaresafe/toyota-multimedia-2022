#ifndef _PVCLOCK_GTOD_H
#define _PVCLOCK_GTOD_H

#include <linux/notifier.h>

/*
 * The pvclock gtod notifier is called when the system time is updated
 * and is used to keep guest time synchronized with host time.
 *
 * The 'action' parameter in the notifier function is false (0), or
 * true (non-zero) if system time was stepped.
 */
extern int pvclock_gtod_register_notifier(struct notifier_block *nb);
extern int pvclock_gtod_unregister_notifier(struct notifier_block *nb);

#endif /* _PVCLOCK_GTOD_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/pvclock_gtod.h $ $Rev: 836322 $")
#endif
