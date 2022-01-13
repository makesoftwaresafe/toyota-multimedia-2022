/*
 * $QNXLicenseC:
 * Copyright 2017, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable
 * license fees to QNX Software Systems before you may reproduce,
 * modify or distribute this software, or any work that includes
 * all or part of this software.   Free development licenses are
 * available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email
 * licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review
 * this entire file for other proprietary rights or license notices,
 * as well as the QNX Development Suite License Guide at
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

#ifndef _I915_DRM_QNX_H_
#define _I915_DRM_QNX_H_

#if defined(__QNXNTO__)

struct drm_i915_gem_vgtbuffer {
	__u32 vmid;
	__u32 plane_id;
#define I915_VGT_PLANE_PRIMARY 1
#define I915_VGT_PLANE_SPRITE 2
#define I915_VGT_PLANE_CURSOR 3
	__u32 pipe_id;
	__u32 phys_pipe_id;
	__u8  enabled;
	__u8  tiled;
	__u32 bpp;
	__u32 hw_format;
	__u32 drm_format;
	__u32 start;
	__u32 x_pos;
	__u32 y_pos;
	__u32 x_offset;
	__u32 y_offset;
	__u32 size;
	__u32 width;
	__u32 height;
	__u32 stride;
	__u64 user_ptr;
	__u32 user_size;
	__u32 flags;
#define I915_VGTBUFFER_READ_ONLY (1<<0)
#define I915_VGTBUFFER_QUERY_ONLY (1<<1)
#define I915_VGTBUFFER_CHECK_CAPABILITY (1<<2)
#define I915_VGTBUFFER_UNSYNCHRONIZED 0x80000000
	/**
	 * Returned handle for the object.
	 *
	 * Object handles are nonzero.
	 */
	__u32 handle;
};
#endif /* __QNXNTO__ */

#endif /* _I915_DRM_QNX_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/uapi/drm/i915_drm_qnx.h $ $Rev: 836322 $")
#endif
