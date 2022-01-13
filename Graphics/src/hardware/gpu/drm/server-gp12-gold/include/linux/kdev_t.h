#ifndef _LINUX_KDEV_T_H
#define _LINUX_KDEV_T_H

#include <uapi/linux/kdev_t.h>

#define MINORBITS       20
#define MINORMASK       ((1U << MINORBITS) - 1)

#define MAJOR(dev)      ((unsigned int) ((dev) >> MINORBITS))
#define MINOR(dev)      ((unsigned int) ((dev) & MINORMASK))
#define MKDEV(ma,mi)    (((ma) << MINORBITS) | (mi))

static inline u16 old_encode_dev(dev_t dev)
{
	return (MAJOR(dev) << 8) | MINOR(dev);
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/kdev_t.h $ $Rev: 836322 $")
#endif
