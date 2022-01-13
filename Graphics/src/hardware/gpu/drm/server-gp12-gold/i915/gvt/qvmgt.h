/*
 * $QNXLicenseC:
 * Copyright 2015, QNX Software Systems. All Rights Reserved.
 *
 * You must obtain a written license from and pay applicable license fees to QNX
 * Software Systems before you may reproduce, modify or distribute this software,
 * or any work that includes all or part of this software.   Free development
 * licenses are available for evaluation and non-commercial purposes.  For more
 * information visit http://licensing.qnx.com or email licensing@qnx.com.
 *
 * This file may contain contributions from others.  Please review this entire
 * file for other proprietary rights or license notices, as well as the QNX
 * Development Suite License Guide at http://licensing.qnx.com/license-guide/
 * for other information.
 * $
 */

/**
 * @file
 * Interfaces coupled to QVM process running vGPU virtual device driver
 *
 */

#ifndef INTEL_GVT_QVMGT_H
#define INTEL_GVT_QVMGT_H

//TODO: This isn't part of the qvm<->kernel module interface any more.
//TODO: Need to figure out a better way of sharing pci config data
//TODO: between the mediator and vdev.
#ifndef QSL_PCI_EXTRACT_OFF
#define QSL_PCI_EXTRACT_OFF(loc)	(((unsigned)((loc) >> 32)) & 0xffffu)
#endif

#ifndef QSL_PCI_MAKE_LOC
#define QSL_PCI_MAKE_LOC(bus,dev,func,off)	\
			((((uint64_t)(func))<<  0)		\
			|(((uint64_t)(dev)) <<  8)		\
			|(((uint64_t)(bus)) << 16)		\
			|(((uint64_t)(off)) << 32))
#endif

#define VGT_PVINFO_END (VGT_PVINFO_PAGE + VGT_PVINFO_SIZE)
#define MAX_VMSYSFS_NODE_NAME_LENGTH 16


/**
 * Information about one system memory block.
 */
struct qvmgt_sysmem_blk {
	_Uint64t location;		/* guest physical address */
	_Uint64t length;		/* length of system memory block */
	_Uint32t host_pgnum;	/* host paddr page number */
	void *vaddr;			/* virtual address */
};

/*
 * qvmgt_hvm_dev is a wrapper of a vGPU instance which is represented by the
 * intel_vgpu structure.
 */
struct qvmgt_hvm_dev {
	int vm_id;
	struct intel_vgpu *vgpu;
	int pvmmio_used;

	int cmd_coid;
	int	iosrv_chid;
	pthread_t iosrv_tid;

	unsigned sysmem_blk_num;
	struct qvmgt_sysmem_blk *sysmem_blk;

	struct kobject *kobj;
	struct attribute_group attr_group;
	char node_name[MAX_VMSYSFS_NODE_NAME_LENGTH];
};

struct qvmgt_hvm_params {
	int vm_id;
	int aperture_sz; /* in MB */
	int gm_sz;  /* in MB */
	int fence_sz;
	int cap;
	/*
	 * 0/1: config the vgt device as secondary/primary VGA,
	 * -1: means the ioemu doesn't supply a value
	 */
	int gvt_primary;
	pid_t qvm_pid;
	int qvm_chid;
};

/*
 * struct gvt_qvmgt should be a single instance to share global
 * information for QVMGT module.
 */
#define GVT_MAX_VGPU_INSTANCE 15
struct gvt_qvmgt {
	struct intel_gvt *gvt;
	struct intel_vgpu *vgpus[GVT_MAX_VGPU_INSTANCE];
};

struct intel_vgpu *qvmgt_instance_create(struct qvmgt_hvm_params *vp,
		struct intel_vgpu_type *type);
void qvmgt_instance_destroy(struct intel_vgpu *vgpu);
void restore_dom0_plane_regs(enum pipe pipe, enum plane_id plane);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/gvt/qvmgt.h $ $Rev: 853904 $")
#endif
