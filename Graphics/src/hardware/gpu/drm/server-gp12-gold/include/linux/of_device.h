#ifndef _QNX_LINUX_OF_DEVICE_H
#define _QNX_LINUX_OF_DEVICE_H

#include <linux/of.h>

struct device;
struct device_driver;

static inline int of_driver_match_device(struct device *dev,
					 struct device_driver *drv)
{
	return 0;
}

static inline void of_device_uevent(struct device *dev,
			struct kobj_uevent_env *env) { }

static inline int of_device_get_modalias(struct device *dev,
				   char *str, ssize_t len)
{
	return -ENODEV;
}

static inline int of_device_uevent_modalias(struct device *dev,
				   struct kobj_uevent_env *env)
{
	return -ENODEV;
}

static inline void of_device_node_put(struct device *dev) { }

static inline const struct of_device_id *__of_match_device(
		const struct of_device_id *matches, const struct device *dev)
{
	return NULL;
}
#define of_match_device(matches, dev)	\
	__of_match_device(of_match_ptr(matches), (dev))

static inline struct device_node *of_cpu_device_node_get(int cpu)
{
	return NULL;
}

#endif /* _QNX_LINUX_OF_DEVICE_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/of_device.h $ $Rev: 836322 $")
#endif
