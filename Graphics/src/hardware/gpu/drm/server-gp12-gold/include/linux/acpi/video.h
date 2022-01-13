#ifndef _QNX_LINUX_ACPI_VIDEO_H
#define _QNX_LINUX_ACPI_VIDEO_H

#include <stdio.h>
#include <sys/mman.h>

static inline int acpi_video_register(void) { return 0; }
static inline void acpi_video_unregister(void) { return; }

#define acpi_lid_open() false

// ACPI
static inline void *acpi_os_ioremap(off64_t offset, unsigned long size)
{
	void *ptr;

	ptr = (void *)(mmap_device_memory(0, size, PROT_READ | PROT_WRITE | PROT_NOCACHE, 0, offset));
	if ( ptr == MAP_FAILED ) {
		fprintf(stderr, "mmap_device_memory for physical address failed\n"); fflush(stderr);
		return NULL;
	}

	return ptr;
}


static inline void acpi_os_unmap_iomem(void* virt, unsigned long size)
{
	munmap(virt, size);
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/acpi/video.h $ $Rev: 836322 $")
#endif
