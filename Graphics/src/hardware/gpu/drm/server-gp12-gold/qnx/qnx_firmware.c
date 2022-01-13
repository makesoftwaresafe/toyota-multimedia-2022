#include <linux/types.h>

#include <linux/slab.h>
#include <linux/device.h>
#include <linux/module.h>
#include <linux/firmware.h>

#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

char firmware_path[PATH_MAX] = "/lib/firmware";

int request_firmware(const struct firmware **firmware_p, const char *name, struct device *device)
{
	int fd;
	char path[PATH_MAX];
	struct firmware* fw;
	int temp;

	*firmware_p = NULL;

	*firmware_p = NULL;
	fw = kmalloc(sizeof(struct firmware), GFP_KERNEL);
	if (fw == NULL) {
		return -ENOMEM;
	}

	snprintf(path, sizeof(path), "%s/%s", firmware_path, name);
	fd = open(path, O_RDONLY);
	if (fd == -1) {
		kfree(fw);
		return -EINVAL;
	}
	fw->size = lseek(fd, 0, SEEK_END);
	lseek(fd, 0, SEEK_SET);

	if (fw->size == -1) {
		kfree(fw);
		close(fd);
		return -EINVAL;
	}

	fw->data = kzalloc(fw->size, GFP_KERNEL);
	if (fw->data == NULL) {
		kfree(fw);
		close(fd);
		return -ENOMEM;
	}

	temp = read(fd, (void*)fw->data, fw->size);
	if (fw->size != temp) {
		kfree((void*)fw->data);
		kfree(fw);
		close(fd);

		return -EINVAL;
	}
	close(fd);

	*firmware_p = fw;
	return EOK;
}

void release_firmware(const struct firmware *fw)
{
	if (fw != NULL) {
		kfree((void*)fw->data);
	}
	kfree((void*)fw);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/qnx_firmware.c $ $Rev: 845340 $")
#endif
