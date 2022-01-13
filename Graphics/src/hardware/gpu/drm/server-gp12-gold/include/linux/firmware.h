#ifndef _QNX_LINUX_FIRMWARE_H
#define _QNX_LINUX_FIRMWARE_H

#include <linux/types.h>

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

struct firmware {
	size_t size;
	const u8 *data;
	struct page **pages;
};


int request_firmware(const struct firmware **firmware_p, const char *name, struct device *device);
void release_firmware(const struct firmware *fw);

int request_firmware_nowait(struct module *module, bool uevent,
	const char *name, struct device *device, gfp_t gfp, void *context,
	void (*cont)(const struct firmware *fw, void *context));

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/firmware.h $ $Rev: 836322 $")
#endif
