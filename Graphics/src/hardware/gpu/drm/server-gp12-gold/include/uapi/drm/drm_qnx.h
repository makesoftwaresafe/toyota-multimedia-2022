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

#ifndef _DRM_QNX_H_
#define _DRM_QNX_H_

#if defined(__QNXNTO__)

/**
 *   QNX extensions
 */

struct drm_shm_qnx {
	char name[16];
	__u32 size;
};

/* same as struct drm_i915_gem_create */
struct drm_gem_cma_create_qnx {
	__u64 size;
	/**
	 * Returned handle for the object.
	 *
	 * Object handles are nonzero.
	 */
	__u32 handle;
	__u32 pad;
};

enum {
	DRM_ATTACH_LOCAL_QNX=0,
	DRM_ATTACH_VM_QNX,
};

enum {
	DRM_ATTACH_PADDR_QNX = 0,
	DRM_ATTACH_VADDR_QNX,
	DRM_ATTACH_GGTT_QNX,
	DRM_ATTACH_PPGTT_QNX,
};

struct drm_gem_gaddr_paddr {
	__u32 count;
	__u64 * addr;
	__u32 * size;
};

struct drm_gem_gaddr_vaddr {
	__u64 pid;
	__u64 addr;
};

struct drm_gem_gaddr_vm_ggtt {
	__u32 vm_id;
	__u64 addr;
};

struct drm_gem_gaddr_vm_ppgtt {
	__u64 pdp0;
	__u64 addr;
};

struct drm_gem_buffer_attach_qnx {
	/* must be filled by user */
	union {
		struct drm_gem_gaddr_paddr paddr;
		struct drm_gem_gaddr_vaddr vaddr;
		struct drm_gem_gaddr_vm_ggtt vm_ggtt;
		struct drm_gem_gaddr_vm_ppgtt vm_ppgtt;
		__u64 rsvd[4];
	};
	__u32 size;
	__u32 width;
	__u32 height;
	__u32 stride;
	__u32 tiling;
	__u32 plan_offsets[3];
	__u32 swizzle;
	/* filled by drm server */
	__u32 handle;
};

struct drm_gem_attach_qnx {
	struct {
		__u8 flag;
		__u8 type;
		__u8 rsvd[2];
	} vm;
	__u32 count;
	struct drm_gem_buffer_attach_qnx * bufs;
};

enum {
	DRM_GEM_VM_SHM_MAP = 0,
	DRM_GEM_VM_SHM_UNMAP,
};
struct drm_gem_vm_shm_qnx {
	__u32 flag;
	__u64 vaddr[2];
};

struct drm_gem_vm_shbufs_status_qnx {
	__u32 status; /*TODO. zero means nothing changed */
	__u32 rsvd;
};

enum {
	DRM_GEM_VM_SHBUFS_CONSUME_BEGIN = 0,
	DRM_GEM_VM_SHBUFS_CONSUME_END,
};

enum {
	DRM_GEM_VM_SHBUFS_CONSUME_STATUS_OK=0,
	DRM_GEM_VM_SHBUFS_CONSUME_STATUS_RETRY,
};

struct drm_gem_vm_shbufs_consume_qnx {
	__u32 flag;
	__u32 status;
	__u32 shm_idx;
	__u32 rsvd;
};

enum {
	DRM_GEM_VM_SHBUFS_STATUS = 0,
	DRM_GEM_VM_SHBUFS_CONSUME,
};

struct drm_gem_vm_shbufs_qnx {
	__u32 flag;
	union {
		struct drm_gem_vm_shbufs_status_qnx status;
		struct drm_gem_vm_shbufs_consume_qnx consume;
	};
};

#define DRM_IOCTL_BASE_QNX		'q'
#define DRM_IO_QNX(nr)			_IO(DRM_IOCTL_BASE_QNX, nr)
#define DRM_IOR_QNX(nr, type)		_IOR(DRM_IOCTL_BASE_QNX, nr, type)
#define DRM_IOW_QNX(nr, type)		_IOW(DRM_IOCTL_BASE_QNX, nr, type)
#define DRM_IOWR_QNX(nr, type)		_IOWR(DRM_IOCTL_BASE_QNX, nr, type)
#define DRM_COMMAND_BASE_QNX		0x0

/* IDs 0x00 and 0x01 are free now */
#define	DRM_IOCTL_SHM_QNX		DRM_IOWR_QNX(0x02, struct drm_shm_qnx)
#define	DRM_IOCTL_GEM_CMA_CREATE_QNX	DRM_IOWR_QNX(0x03, struct drm_gem_cma_create_qnx)
#define	DRM_IOCTL_GEM_ATTACH_QNX	DRM_IOWR_QNX(0x04, struct drm_gem_attach_qnx)
#define	DRM_IOCTL_GEM_VM_SHM_QNX	DRM_IOWR_QNX(0x05, struct drm_gem_vm_shm_qnx)
#define	DRM_IOCTL_GEM_VM_SHBUFS_QNX	DRM_IOWR_QNX(0x06, struct drm_gem_vm_shbufs_qnx)

/**
 *   end of QNX extensions
 */

#endif /* __QNXNTO__ */

#endif /* _DRM_QNX_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/uapi/drm/drm_qnx.h $ $Rev: 837047 $")
#endif
