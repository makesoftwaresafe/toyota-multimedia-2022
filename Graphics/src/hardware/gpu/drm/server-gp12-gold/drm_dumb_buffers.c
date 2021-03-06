/*
 * Copyright (c) 2006-2008 Intel Corporation
 * Copyright (c) 2007 Dave Airlie <airlied@linux.ie>
 * Copyright (c) 2008 Red Hat Inc.
 * Copyright (c) 2016 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <drm/drmP.h>

#include "drm_crtc_internal.h"

#ifdef __QNXNTO__
#include "i915_drv.h"
#endif /* __QNXNTO__ */

/**
 * DOC: overview
 *
 * The KMS API doesn't standardize backing storage object creation and leaves it
 * to driver-specific ioctls. Furthermore actually creating a buffer object even
 * for GEM-based drivers is done through a driver-specific ioctl - GEM only has
 * a common userspace interface for sharing and destroying objects. While not an
 * issue for full-fledged graphics stacks that include device-specific userspace
 * components (in libdrm for instance), this limit makes DRM-based early boot
 * graphics unnecessarily complex.
 *
 * Dumb objects partly alleviate the problem by providing a standard API to
 * create dumb buffers suitable for scanout, which can then be used to create
 * KMS frame buffers.
 *
 * To support dumb objects drivers must implement the &drm_driver.dumb_create,
 * &drm_driver.dumb_destroy and &drm_driver.dumb_map_offset operations. See
 * there for further details.
 *
 * Note that dumb objects may not be used for gpu acceleration, as has been
 * attempted on some ARM embedded platforms. Such drivers really must have
 * a hardware-specific ioctl to allocate suitable buffer objects.
 */

int drm_mode_create_dumb_ioctl(struct drm_device *dev,
			       void *data, struct drm_file *file_priv)
{
	struct drm_mode_create_dumb *args = data;
	u32 cpp, stride, size;

	if (!dev->driver->dumb_create)
		return -ENOSYS;
	if (!args->width || !args->height || !args->bpp)
		return -EINVAL;

	/* overflow checks for 32bit size calculations */
	/* NOTE: DIV_ROUND_UP() can overflow */
	cpp = DIV_ROUND_UP(args->bpp, 8);
	if (!cpp || cpp > 0xffffffffU / args->width)
		return -EINVAL;
	stride = cpp * args->width;
	if (args->height > 0xffffffffU / stride)
		return -EINVAL;

	/* test for wrap-around */
	size = args->height * stride;
	if (PAGE_ALIGN(size) == 0)
		return -EINVAL;

	/*
	 * handle, pitch and size are output parameters. Zero them out to
	 * prevent drivers from accidentally using uninitialized data. Since
	 * not all existing userspace is clearing these fields properly we
	 * cannot reject IOCTL with garbage in them.
	 */
	args->handle = 0;
	args->pitch = 0;
	args->size = 0;

	return dev->driver->dumb_create(file_priv, dev, args);
}

/**
 * drm_mode_mmap_dumb_ioctl - create an mmap offset for a dumb backing storage buffer
 * @dev: DRM device
 * @data: ioctl data
 * @file_priv: DRM file info
 *
 * Allocate an offset in the drm device node's address space to be able to
 * memory map a dumb buffer.
 *
 * Called by the user via ioctl.
 *
 * Returns:
 * Zero on success, negative errno on failure.
 */
int drm_mode_mmap_dumb_ioctl(struct drm_device *dev,
			     void *data, struct drm_file *file_priv)
{
	struct drm_mode_map_dumb *args = data;

	/* call driver ioctl to get mmap offset */
	if (!dev->driver->dumb_map_offset)
		return -ENOSYS;

#ifndef __QNXNTO__
	return dev->driver->dumb_map_offset(file_priv, dev, args->handle, &args->offset);
#else
	int ret;

	ret = dev->driver->dumb_map_offset(file_priv, dev, args->handle, &args->offset);

	if (ret == 0) {
		struct drm_i915_gem_object *obj;

		args->cvaddr = (__u64)(uintptr_t)MAP_FAILED;
		obj = i915_gem_object_lookup(file_priv, args->handle);
		if (obj) {
			/* Do a client mmaping */
			ret = i915_gem_mmap_qnx(file_priv, dev, args->handle, obj->base.size, &args->cvaddr);

			i915_gem_object_put(obj);
		}
	}

	return ret;
#endif /* __QNXNTO__ */
}

int drm_mode_destroy_dumb_ioctl(struct drm_device *dev,
				void *data, struct drm_file *file_priv)
{
	struct drm_mode_destroy_dumb *args = data;

	if (!dev->driver->dumb_destroy)
		return -ENOSYS;

	return dev->driver->dumb_destroy(file_priv, dev, args->handle);
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/drm_dumb_buffers.c $ $Rev: 837047 $")
#endif
