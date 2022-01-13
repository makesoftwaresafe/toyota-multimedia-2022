#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/ktime.h>
#include <drm/drmP.h>

/* When userspace needs to send an in-fence to the driver it passes file descriptor */
/* of the Sync File to the kernel. The kernel can then retrieve the fences from it. */

struct dma_fence *sync_file_get_fence(int fd)
{
	fprintf(stderr, "sync_file_get_fence(): call is not implemented\n");
	return NULL;
}

/* When a driver needs to send an out-fence userspace it creates a sync_file. */

struct sync_file *sync_file_create(struct dma_fence *fence)
{
	fprintf(stderr, "sync_file_create(): call is not implemented\n");
	return NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/syncfile.c $ $Rev: 836322 $")
#endif
