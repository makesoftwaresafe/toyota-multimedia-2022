#ifndef __QNX_ACPI_H__
#define __QNX_ACPI_H__

#include <linux/acpi/video.h>
#include <linux/device.h>
#include <linux/property.h>
#include <stdbool.h>

#ifdef CONFIG_ACPI
#else

#define ACPI_COMPANION(dev)		(NULL)
#define ACPI_COMPANION_SET(dev, adev)	do { } while (0)
#define ACPI_HANDLE(dev)		(NULL)
#define ACPI_DEVICE_CLASS(_cls, _msk)	.cls = (0), .cls_msk = (0),

static inline bool is_acpi_node(struct fwnode_handle *fwnode)
{
	return false;
}

static inline struct acpi_device *to_acpi_node(struct fwnode_handle *fwnode)
{
	return NULL;
}

static inline bool acpi_driver_match_device(struct device *dev,
					    const struct device_driver *drv)
{
	return false;
}

static inline int acpi_device_uevent_modalias(struct device *dev,
				struct kobj_uevent_env *env)
{
	return -ENODEV;
}

static inline int acpi_dev_gpio_irq_get(struct acpi_device *adev, int index)
{
	return -ENXIO;
}

static inline int acpi_device_modalias(struct device *dev,
				char *buf, int size)
{
	return -ENODEV;
}

static inline const char *acpi_dev_name(struct acpi_device *adev)
{
	return NULL;
}

#endif /* CONFIG_ACPI */


#endif /* __QNX_ACPI_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/acpi.h $ $Rev: 836322 $")
#endif
