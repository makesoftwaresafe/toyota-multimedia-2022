#ifndef _QNX_LINUX_REBOOT_H
#define _QNX_LINUX_REBOOT_H

#include <linux/notifier.h>

#define SYS_DOWN	0x0001	/* Notify of system down */
#define SYS_RESTART	SYS_DOWN
#define SYS_HALT	0x0002	/* Notify of system halt */
#define SYS_POWER_OFF	0x0003	/* Notify of system power off */

extern int register_reboot_notifier(struct notifier_block *);
extern int unregister_reboot_notifier(struct notifier_block *);

#endif /* _QNX_LINUX_REBOOT_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/reboot.h $ $Rev: 836322 $")
#endif
