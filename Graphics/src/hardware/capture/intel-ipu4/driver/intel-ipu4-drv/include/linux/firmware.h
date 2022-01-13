#ifndef _LINUX_FIRMWARE_H
#define _LINUX_FIRMWARE_H

#include <linux/types.h>
#include <linux/compiler.h>
#include <linux/gfp.h>

#define FW_ACTION_NOHOTPLUG 0
#define FW_ACTION_HOTPLUG 1

struct firmware {
	size_t size;
	const u8 *data;
	struct page **pages;

	/* firmware loader private fields */
	void *priv;
};

struct module;
struct device;

struct builtin_fw {
	char *name;
	void *data;
	unsigned long size;
};

/* We have to play tricks here much like stringify() to get the
   __COUNTER__ macro to be expanded as we want it */
#define __fw_concat1(x, y) x##y
#define __fw_concat(x, y) __fw_concat1(x, y)

#define DECLARE_BUILTIN_FIRMWARE(name, blob)				     \
	DECLARE_BUILTIN_FIRMWARE_SIZE(name, &(blob), sizeof(blob))

#define DECLARE_BUILTIN_FIRMWARE_SIZE(name, blob, size)			     \
	static const struct builtin_fw __fw_concat(__builtin_fw,__COUNTER__) \
	__used __section(.builtin_fw) = { name, blob, size }

#if defined(CONFIG_FW_LOADER) || (defined(CONFIG_FW_LOADER_MODULE) && defined(MODULE))
int request_firmware(const struct firmware **fw, const char *name,
		     struct device *device);
int request_firmware_nowait(
	struct module *module, bool uevent,
	const char *name, struct device *device, gfp_t gfp, void *context,
	void (*cont)(const struct firmware *fw, void *context));
int request_firmware_direct(const struct firmware **fw, const char *name,
			    struct device *device);

void release_firmware(const struct firmware *fw);
#else
static inline int request_firmware(const struct firmware **fw,
				   const char *name,
				   struct device *device)
{
	return -EINVAL;
}
static inline int request_firmware_nowait(
	struct module *module, bool uevent,
	const char *name, struct device *device, gfp_t gfp, void *context,
	void (*cont)(const struct firmware *fw, void *context))
{
	return -EINVAL;
}

static inline void release_firmware(const struct firmware *fw)
{
}

static inline int request_firmware_direct(const struct firmware **fw,
					  const char *name,
					  struct device *device)
{
	return -EINVAL;
}

#endif
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/firmware.h $ $Rev: 838597 $")
#endif
