#ifndef _QNX_I915_IOCTLS_H
#define _QNX_I915_IOCTLS_H 1

int i915_gem_mmap_qnx(struct drm_file *file, struct drm_device *dev, uint32_t handle,
	uint64_t size, uint64_t *vir_addr);

#endif /* _QNX_I915_IOCTLS_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/qnx_i915_ioctls.h $ $Rev: 837534 $")
#endif
