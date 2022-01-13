
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
 */

/*
 * NOTE:
 * This file contains hypervisor specific interactions to
 * implement the concept of mediated pass-through framework.
 */

#include <i915_drv.h>
#include <i915_pvinfo.h>
#include <gvt/gvt.h>
#include "qvmgt.h"

#include <linux/qnx.h>

//TODO: switch to vdev_vgpu.h when ready
#include "vdev_vgpu_local.h"

#ifdef CONFIG_INTEL_IOMMU
int intel_iommu_gfx_mapped = 1;
#endif

static struct kobject *gvt_kobj = NULL;
static struct kobject *ctrl_kobj = NULL;
static DEFINE_MUTEX(gvt_sysfs_lock);
static struct gvt_qvmgt qvmgt_priv;
const static struct intel_gvt_ops *intel_gvt_ops;

static inline struct qvmgt_sysmem_blk *memblk_from_gfn(const struct qvmgt_hvm_dev *const info,
		unsigned long const gfn)
{
	unsigned long const gpa = gfn << PAGE_SHIFT;
	unsigned i;

	if (!info->sysmem_blk)
		return NULL;

	for (i = 0; i < info->sysmem_blk_num; ++i) {
		if (gpa >= info->sysmem_blk[i].location && \
				gpa < info->sysmem_blk[i].location + info->sysmem_blk[i].length) {
			return &info->sysmem_blk[i];
		}
	}
	return NULL;
}

static inline void *va_from_gpa(const struct qvmgt_hvm_dev *const info,
		unsigned long const gpa)
{
	const struct qvmgt_sysmem_blk *const memblk = memblk_from_gfn(info,
			gpa >> PAGE_SHIFT);
	unsigned long va;

	if (memblk == NULL) {
		gvt_err("VM%d: failed to find memblk for gfn=0x%lx!\n", info->vm_id,
				gpa >> PAGE_SHIFT);
		return NULL;
	}
	va = (unsigned long)memblk->vaddr + (gpa - memblk->location);
	//gvt_dbg_core("VM%d: gpa=0x%lx -> va=0x%lx\n", info->vm_id, gpa, va);

	return (void *)va;
}

static void gvt_qvm_vmem_deinit(struct intel_vgpu *const vgpu)
{
	struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)vgpu->handle;
	unsigned i;

	if (!info || !info->sysmem_blk)
		return;

	for (i = 0; i < info->sysmem_blk_num; ++i) {
		if (info->sysmem_blk[i].vaddr) {
			munmap (info->sysmem_blk[i].vaddr, info->sysmem_blk[i].length);
		}
	}
	free(info->sysmem_blk);
	info->sysmem_blk = NULL;
	info->sysmem_blk_num = 0;
}

static int gvt_qvm_vmem_init(struct intel_vgpu *const vgpu) {
	struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)vgpu->handle;
	struct qvgpu_sysmem_get_info op;
	struct qvgpu_sysmem_get_info_reply *oprep;
	unsigned long nr_high_bkt, j;
	unsigned i, k;
	int rc;

	op.type = QVGPU_SYSMEM_GET_INFO;
	op.num_mem_blks = 0;
	memset(&op.zero, 0, sizeof(op.zero));

	oprep = malloc(sizeof(*oprep));
	if (oprep == NULL) return ENOMEM;

	rc = (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), oprep, sizeof(*oprep));
	if (rc < EOK) {
		gvt_err("VM%d: can't obtain sysmem (1/2) information(%s)!\n",
				info->vm_id, strerror(-rc));
		rc = -rc;
		goto exit;
	}
	if (oprep->num_mem_blks < 1) {
		gvt_err("VM%d: system doesn't have any RAM!\n", info->vm_id);
		rc = EINVAL;
		goto exit;
	}
	gvt_dbg_core("VM%d: found %d RAM blocks\n", info->vm_id, oprep->num_mem_blks);

	size_t const oprep_size = sizeof(*oprep) + oprep->num_mem_blks * sizeof(oprep->mem_blk[0]);
	struct qvgpu_sysmem_get_info_reply	*const oprep_tmp = realloc(oprep, oprep_size);
	if (oprep_tmp == NULL) {
		rc = ENOMEM;
		goto exit;
	}
	oprep = oprep_tmp;
	op.num_mem_blks = oprep->num_mem_blks;
	rc = (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), oprep, oprep_size);
	if (rc < EOK) {
		gvt_err("VM%d: can't obtain sysmem (2/2) information(%s)!\n",
				info->vm_id, strerror(-rc));
		rc = -rc;
		goto exit;
	}
	info->sysmem_blk_num = oprep->num_mem_blks;
	info->sysmem_blk = calloc(info->sysmem_blk_num, sizeof(*info->sysmem_blk));
	if (info->sysmem_blk == NULL) {
		gvt_err("VM%d: can't allocate sysmem_blk(%s)!\n",
				info->vm_id, strerror(errno));
		info->sysmem_blk_num = 0;
		rc = ENOMEM;
		goto exit;
	}
	for (i = 0, k = 0; i < oprep->num_mem_blks; ++i) {
		// contiguous memblk support
		if (!info->sysmem_blk[k].length) {
			info->sysmem_blk[k].location = oprep->mem_blk[i].location;
			info->sysmem_blk[k].host_pgnum = oprep->mem_blk[i].host_pgnum;
		}
		info->sysmem_blk[k].length += oprep->mem_blk[i].length;
		if ((i+1 < oprep->num_mem_blks) &&
			(oprep->mem_blk[i+1].location == oprep->mem_blk[i].location + oprep->mem_blk[i].length) &&
			(oprep->mem_blk[i+1].host_pgnum == oprep->mem_blk[i].host_pgnum +
				oprep->mem_blk[i].length / __PAGESIZE)) {
			--info->sysmem_blk_num;
			continue;
		}
		if (info->sysmem_blk[k].location & ~PAGE_MASK) {
			gvt_err("VM%d: block[%d]: loc 0x%lx host 0x%lx length 0x%x isn't page aligned!\n",
					info->vm_id, k, info->sysmem_blk[k].location,
					(unsigned long)info->sysmem_blk[k].host_pgnum << PAGE_SHIFT,
					info->sysmem_blk[k].length);
			rc = EINVAL;
			goto exit;
		}
		info->sysmem_blk[k].vaddr =	mmap64(NULL, info->sysmem_blk[k].length,
				PROT_READ|PROT_WRITE,
				MAP_SHARED|MAP_PHYS, NOFD, (off_t)((uint64_t)info->sysmem_blk[k].host_pgnum << PAGE_SHIFT));
		if(info->sysmem_blk[k].vaddr == MAP_FAILED) {
			gvt_err("VM%d: can't map block[%d]: loc 0x%lx host 0x%lx length 0x%x(%s)!\n",
					info->vm_id, k, info->sysmem_blk[k].location,
					(unsigned long)info->sysmem_blk[k].host_pgnum << PAGE_SHIFT,
					info->sysmem_blk[k].length);
			info->sysmem_blk[k].vaddr = 0;
			rc = errno;
			goto err;
		}
		gvt_dbg_core("VM%d: ram[%d]: loc 0x%lx host 0x%lx length 0x%x vaddr 0x%p\n",
				info->vm_id, k, info->sysmem_blk[k].location,
				(unsigned long)info->sysmem_blk[k].host_pgnum << PAGE_SHIFT,
				info->sysmem_blk[k].length, info->sysmem_blk[k].vaddr);
		++k;
	}
	gvt_dbg_core("VM%d: mapped %d RAM blocks\n", info->vm_id, info->sysmem_blk_num);
	goto exit;

err:
	gvt_qvm_vmem_deinit(vgpu);
exit:
	free(oprep);
	return rc;
}

static int gvt_qvm_write_cfg_space(struct intel_vgpu *vgpu,
		uint64_t addr, unsigned int const bytes, void *const data)
{
	unsigned int port = QSL_PCI_EXTRACT_OFF(addr);

	assert(((bytes == 4) && ((port & 3) == 0)) ||
		((bytes == 2) && ((port & 1) == 0)) || (bytes == 1));

	return intel_gvt_ops->emulate_cfg_write(vgpu, port, data, bytes);
}

static int gvt_qvm_read_cfg_space(struct intel_vgpu *vgpu,
		uint64_t addr, unsigned int const bytes, void *const data)
{
	unsigned int port = QSL_PCI_EXTRACT_OFF(addr);

	assert(((bytes == 4) && ((port & 3) == 0)) ||
		((bytes == 2) && ((port & 1) == 0)) || (bytes == 1));

	return intel_gvt_ops->emulate_cfg_read(vgpu, port, data, bytes);
}

void *
gvt_emulation_thread(void *const data)
{
	struct intel_vgpu *const 	vgpu = (struct intel_vgpu *)data;
	struct qvmgt_hvm_dev *const info  = (struct qvmgt_hvm_dev *)vgpu->handle;
	uint8_t						ioreq_data[QVGPU_IOREQ_MAX_DATA_SIZE];
	char 						tname[_NTO_THREAD_NAME_MAX];
	struct task_struct 	this_task, *qnx_emu_task;
	struct qvgpu_ioreq_msg		ioreq;
	iov_t						iov[2];
	int							rcvid, rc;
	pthread_t tid = pthread_self();

	/* set thread name */
	snprintf (tname, _NTO_THREAD_NAME_MAX, "qvmgt_emulation:%d", info->vm_id);
	pthread_setname_np(0, tname);

	/* Setup emulation task context, task should have interrupted task */
	/* context, but we emulate interrupted kernel thread.              */
	qnx_emu_task = &this_task;
	memset(qnx_emu_task, 0, sizeof(*qnx_emu_task));

	qnx_emu_task->pid = -1;
	qnx_emu_task->spid.pid = -1;
	strncpy(qnx_emu_task->comm, tname, sizeof(qnx_emu_task->comm) - 1);
	qnx_emu_task->real_cred = &qnx_emu_task->cred_vault;
	qnx_emu_task->cred = &qnx_emu_task->cred_vault;
	rc = qnx_create_task_sched(qnx_emu_task);
	if (rc) {
		/* Error output is done inside qnx_create_task_sched() function */
		return NULL;
	}
	current = qnx_emu_task;
	qnx_emu_task->attachment.user_tid = pthread_self();
	qnx_emu_task->attachment.copy_to_user_memcpy = 1;

	SETIOV(&iov[0], &ioreq, sizeof(ioreq));
	SETIOV(&iov[1], &ioreq_data, sizeof(ioreq_data));

	for(;;) {
		rcvid = MsgReceivev_r(info->iosrv_chid, iov, 2, NULL);
		if (rcvid <= 0) {
			if (rcvid == 0) {
				struct _pulse *pulse = (struct _pulse *)&ioreq;
				if (pulse->code != _PULSE_CODE_DISCONNECT) {
					gvt_err("VM%d: received unexpected pulse (code: %d, scoid: 0x%x) - ignoring!\n",
							info->vm_id, pulse->code, pulse->scoid);
					continue;
				}
			}
			/*  IO emulation channel was destroyed in qvmgt_sysfs_del_instance - exit thread */
			if (rcvid == -ESRCH) {
				break;
			}
			if (rcvid < 0 && rcvid != -EFAULT) {
				gvt_err("VM%d: error waiting for a message: %s!\n",
						info->vm_id, strerror(-rcvid));
				continue;
			}
			/* Lost the connection with vdev-vgpu-gvtg - destroy vGPU instance */
			gvt_err("VM%d: rcvid=%d %s ioreq.type=%d ioreq.dir=%d\n",
					info->vm_id, rcvid,
					rcvid == -EFAULT ?
						"EFAULT:vm down" :
						strerror(-rcvid),
					ioreq.type, ioreq.dir);
			mutex_lock(&gvt_sysfs_lock);
			qvmgt_instance_destroy(vgpu);
			mutex_unlock(&gvt_sysfs_lock);
			break;
		}

		/*
		 * set prio everytime received from qvm
		 */
		pthread_setschedprio(tid,
				qnx_emu_task->sched_param.sched_priority);

		switch (ioreq.type) {
		case QVGPU_IOREQ_TYPE_PCI:
			if (ioreq.dir == QVGPU_IOREQ_DIR_READ) {
				rc = gvt_qvm_read_cfg_space(vgpu, ioreq.address, ioreq.length, &ioreq_data);
				//gvt_dbg_core("pci read: addr = 0x%x bytes = %u data = 0x%x\n", QSL_PCI_EXTRACT_OFF(ioreq.address), ioreq.length, *((unsigned *)&ioreq_data));
			} else {
				//gvt_dbg_core("pci write: addr = 0x%x bytes = %u data = 0x%x\n", QSL_PCI_EXTRACT_OFF(ioreq.address), ioreq.length, *((unsigned *)&ioreq_data));
				rc = gvt_qvm_write_cfg_space(vgpu, ioreq.address, ioreq.length, &ioreq_data);
			}
			break;
		case QVGPU_IOREQ_TYPE_MEM:
			if (ioreq.dir == QVGPU_IOREQ_DIR_READ) {
				rc = intel_gvt_ops->emulate_mmio_read(vgpu, ioreq.address,
						&ioreq_data, ioreq.length);
				//gvt_dbg_core("mmio read: addr = 0x%lx bytes = %u data = 0x%lx\n", ioreq.address, ioreq.length, *((unsigned long*)&ioreq_data));
			} else {
				//gvt_dbg_core("mmio write: addr = 0x%lx bytes = %u data = 0x%lx\n", ioreq.address, ioreq.length, *((unsigned long*)&ioreq_data));
				rc = intel_gvt_ops->emulate_mmio_write(vgpu, ioreq.address,
						&ioreq_data, ioreq.length);

			}
			break;
		default:
			gvt_err("Unknown %s ioreq type %u addr 0x%lx size %u\n",
				(ioreq.dir == QVGPU_IOREQ_DIR_READ)?"READ":"WRITE", ioreq.type,
				 ioreq.address, ioreq.length);
			rc = -EINVAL;
			break;
		}

		if (rc) {
			gvt_err ("ERROR during IO emulation: %s (ret=%d)\n", strerror(-rc), -rc);
			MsgError(rcvid, -rc);
		} else {
			if (ioreq.dir == QVGPU_IOREQ_DIR_READ) {
				MsgReply(rcvid, 0, &ioreq_data, ioreq.length);
			} else {
				MsgReply(rcvid, 0, NULL, 0);
			}
		}
	}
	qnx_destroy_task_sched(current);
	current = NULL;

	return NULL;
}

static int qvm_create_ioreq_server(struct intel_vgpu *vgpu, uint64_t pci_loc)
{
	struct qvmgt_hvm_dev *info  = (struct qvmgt_hvm_dev *)vgpu->handle;
	struct qvgpu_ioreq_server_create op;
	pthread_attr_t attr;
	int rc;

	rc = qnx_taskattr_init(&attr, QNX_PRTY_KERNEL);
	if (rc) {
		return -rc;
	}
	info->iosrv_chid = ChannelCreate_r(_NTO_CHF_DISCONNECT);
	if (info->iosrv_chid < 0) {
		return info->iosrv_chid;
	}

	rc = pthread_create(&info->iosrv_tid, &attr, gvt_emulation_thread, vgpu);
	if(rc != EOK) {
		ChannelDestroy_r(info->iosrv_chid);
		info->iosrv_chid = -1;
		return -rc;
	}

	op.type = QVGPU_IOREQ_SERVER_CREATE;
	op.pid = getpid();
	op.chid = info->iosrv_chid;
	op.pci_loc = pci_loc;
	memset(&op.zero, 0, sizeof(op.zero));

	return (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), NULL, 0);
}

struct intel_vgpu *qvmgt_instance_create(struct qvmgt_hvm_params *vp,
		struct intel_vgpu_type *vgpu_type)
{
	struct qvmgt_hvm_dev *info;
	struct intel_vgpu *vgpu;
	struct qvgpu_version version_op;
	struct qvgpu_version_reply	version;
	int rc;

	if (!intel_gvt_ops || !qvmgt_priv.gvt)
		return NULL;

	vgpu = intel_gvt_ops->vgpu_create(qvmgt_priv.gvt, vgpu_type);
	if (IS_ERR(vgpu)) {
		gvt_err("%s[%d]:vgpu create failure ret=%ld\n",
				__func__, pthread_self(),
				PTR_ERR(vgpu));
		return NULL;
	}

	info = kzalloc(sizeof(struct qvmgt_hvm_dev), GFP_KERNEL);
	if (info == NULL)
		goto err;

	info->vm_id = vp->vm_id;
	info->vgpu = vgpu;
	vgpu->handle = (unsigned long)info;
	info->cmd_coid = ConnectAttach_r(ND_LOCAL_NODE, vp->qvm_pid, vp->qvm_chid, _NTO_SIDE_CHANNEL, 0);
	if (info->cmd_coid < 0) {
		gvt_err("Can not create connection to QVM VGT device!\n");
		goto err;
	}

	version_op.type = QVGPU_VERSION;

	rc = (int)MsgSend_r(info->cmd_coid, &version_op, sizeof(version_op), &version, sizeof(version));
	if(rc < 0) {
		gvt_err("vGPU VDEV version obtaining failed: %d %s\n",-rc, strerror(-rc));
		goto err;
	}

	if(version.version.major != QVGPU_VERSION_MAJOR) {
		gvt_err("vGPU VDEV version mismatch. Expected %u, got %u\n", QVGPU_VERSION_MAJOR, version.version.major);
		goto err;
	}

	rc = qvm_create_ioreq_server(vgpu, QSL_PCI_MAKE_LOC(vgpu->gvt->dev_priv->drm.pdev->bus->number,
			PCI_LNX_DEV(vgpu->gvt->dev_priv->drm.pdev->devfn), PCI_LNX_FUNC(vgpu->gvt->dev_priv->drm.pdev->devfn), 0));
	if (rc < 0) {
		gvt_err("vGPU VDEV creating IO server failed: %s\n", strerror(-rc));
		goto err;
	}

	rc = gvt_qvm_vmem_init(vgpu);
	if (rc) {
		gvt_err("vmem init failed: %s!\n", strerror(rc));
		goto err;
	}

	qvmgt_priv.vgpus[vgpu->id - 1] = vgpu;
	printk(KERN_INFO "%s[%d] success vgpu-id=%d\n",
			__func__, pthread_self(), vgpu->id);

	return vgpu;
err:
	qvmgt_instance_destroy(vgpu);
	return NULL;
}

static void disable_domu_plane(int pipe, int plane)
{
	struct drm_i915_private *const dev_priv = qvmgt_priv.gvt->dev_priv;

	I915_WRITE(PLANE_CTL(pipe, plane), 0);
	I915_WRITE(PLANE_SURF(pipe, plane), 0);
	POSTING_READ(PLANE_SURF(pipe, plane));
}

void qvmgt_instance_destroy(struct intel_vgpu *vgpu)
{
	const struct intel_gvt *const gvt = qvmgt_priv.gvt;
	struct qvmgt_hvm_dev *info;
	int pipe, plane;

	if (vgpu) {
		printk(KERN_ERR "##%s[%d]##\n", __func__, pthread_self());
		struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)vgpu->handle;

		for_each_pipe(gvt->dev_priv, pipe) {
			for_each_universal_plane(gvt->dev_priv, pipe, plane) {
				if (gvt->pipe_info[pipe].plane_owner[plane] == vgpu->id) {
					disable_domu_plane(pipe, plane);
				}
			}
		}
		intel_gvt_ops->vgpu_deactivate(vgpu);
		qvmgt_priv.vgpus[vgpu->id - 1] = NULL;
		if (info) {
			gvt_qvm_vmem_deinit (vgpu);

			ChannelDestroy_r(info->iosrv_chid);
			info->iosrv_chid = -1;

			if (info->iosrv_tid == pthread_self()) {
				pthread_detach(info->iosrv_tid);
			} else {
				pthread_join(info->iosrv_tid, NULL);
			}

			if (info->cmd_coid) {
				ConnectDetach_r(info->cmd_coid);
				info->cmd_coid = -1;
			}

			kfree(info);
			vgpu->handle = 0;
		}
		intel_gvt_ops->vgpu_destroy(vgpu);
	}
}

static ssize_t show_plane_owner(struct device *kdev, struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "Planes:\nPipe A: %d %d %d %d\n"
				"Pipe B: %d %d %d %d\nPipe C: %d %d %d\n",
		qvmgt_priv.gvt->pipe_info[PIPE_A].plane_owner[PLANE_PRIMARY],
		qvmgt_priv.gvt->pipe_info[PIPE_A].plane_owner[PLANE_SPRITE0],
		qvmgt_priv.gvt->pipe_info[PIPE_A].plane_owner[PLANE_SPRITE1],
		qvmgt_priv.gvt->pipe_info[PIPE_A].plane_owner[PLANE_SPRITE2],
		qvmgt_priv.gvt->pipe_info[PIPE_B].plane_owner[PLANE_PRIMARY],
		qvmgt_priv.gvt->pipe_info[PIPE_B].plane_owner[PLANE_SPRITE0],
		qvmgt_priv.gvt->pipe_info[PIPE_B].plane_owner[PLANE_SPRITE1],
		qvmgt_priv.gvt->pipe_info[PIPE_B].plane_owner[PLANE_SPRITE2],
		qvmgt_priv.gvt->pipe_info[PIPE_C].plane_owner[PLANE_PRIMARY],
		qvmgt_priv.gvt->pipe_info[PIPE_C].plane_owner[PLANE_SPRITE0],
		qvmgt_priv.gvt->pipe_info[PIPE_C].plane_owner[PLANE_SPRITE1]);
}

static ssize_t qvmgt_sysfs_instance_manage(struct device *kdev,
	struct device_attribute *attr, const char *buf, size_t count);

static DEVICE_ATTR(create_vgt_instance, 0220, NULL, qvmgt_sysfs_instance_manage);
static DEVICE_ATTR(plane_owner_show, 0440, show_plane_owner, NULL);

static struct attribute *qvmgt_ctrl_attrs[] = {
	&dev_attr_create_vgt_instance.attr,
	&dev_attr_plane_owner_show.attr,
	NULL,
};

static struct attribute_group control_attr_group = {
	.name = "control",
	.attrs = qvmgt_ctrl_attrs,
};

static struct attribute *gvt_attrs[] = {
	NULL,
};

static struct attribute_group gvt_attr_group = {
	.name = "vgt",
	.attrs = gvt_attrs
};

static int qvmgt_sysfs_add_instance(struct qvmgt_hvm_params *vp)
{
	struct intel_vgpu_type type = qvmgt_priv.gvt->types[1];
	struct intel_vgpu *vgpu;

	if (vp->vm_id <= 0 || vp->vm_id > GVT_MAX_VGPU) {
		gvt_err("vm_id=%d is out of range!\n", vp->vm_id);
		return -EINVAL;
	}
	type.req_vgpu_id = vp->vm_id;

	/*
	 * Intel's comment:
	 * We want to respect user's input of vGPU resource requirement.
	 * While doing this, still keep using vgpu_type program interface
	 * to create vgpu instance. Fortunately, this won't break gvt's
	 * instance management logic.
	 */
	type.low_gm_size = MB_TO_BYTES(vp->aperture_sz);
	type.high_gm_size = MB_TO_BYTES(vp->gm_sz);
	type.fence = vp->fence_sz;

	mutex_lock(&gvt_sysfs_lock);
	vgpu = qvmgt_instance_create(vp, &type);
	mutex_unlock(&gvt_sysfs_lock);
	if (vgpu == NULL) {
		gvt_err("qvmgt_sysfs_add_instance failed!\n");
		return -EINVAL;
	}
	return 0;
}

static int qvmgt_sysfs_del_instance(struct qvmgt_hvm_params *vp)
{
	struct intel_vgpu *vgpu;
	struct qvmgt_hvm_dev *info = NULL;

	if (-vp->vm_id <= 0 || -vp->vm_id > GVT_MAX_VGPU) {
		gvt_err("vm_id=%d is out of range!\n", -vp->vm_id);
		return -EINVAL;
	}

	vgpu = qvmgt_priv.vgpus[-vp->vm_id - 1];
	if (!vgpu) {
		gvt_err("vm_id=%d doesn't exist!\n", -vp->vm_id);
		return -EINVAL;
	}

	mutex_lock(&gvt_sysfs_lock);
	qvmgt_instance_destroy(vgpu);
	mutex_unlock(&gvt_sysfs_lock);

	return 0;
}

static ssize_t qvmgt_sysfs_instance_manage(struct device *kdev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct qvmgt_hvm_params vp;
	int param_cnt;
	char param_str[90];
	int ret;
	int high_gm_sz;
	int low_gm_sz;

	/*
	 * TODO: Looks like Intel changed the parameters passing scheme entirely
	 *       and now we only need to pass type of vGPU device.
	 *       Modify vdev-vgpu-gvtg and change it here when ready.
	 */

	(void)sscanf(buf, "%89s", param_str);
	param_cnt = sscanf(param_str, "%d,%d,%d,%d,%d,%d,%d", &vp.vm_id, &vp.qvm_pid,
			&vp.qvm_chid, &low_gm_sz, &high_gm_sz, &vp.fence_sz, &vp.gvt_primary);
	vp.aperture_sz = low_gm_sz;
	vp.gm_sz = high_gm_sz + low_gm_sz;

	if (param_cnt == 1) {
		if (vp.vm_id >= 0)
			return -EINVAL;
	} else if (param_cnt == 6 || param_cnt == 7) {
		if (!(vp.vm_id > 0 && vp.aperture_sz > 0 &&
			vp.aperture_sz <= vp.gm_sz && vp.fence_sz > 0))
			return -EINVAL;

		if (param_cnt == 7) {
			/* -1/0/1 means: not-specified, non-primary, primary */
			if (vp.gvt_primary < -1 || vp.gvt_primary > 1)
				return -EINVAL;
		} else {
			vp.gvt_primary = -1; /* no valid value specified. */
		}
	} else
		return -EINVAL;

	/*
	 * TODO: add this parameter to vdev_vgpu_gvtg
	 */
	vp.cap = 0; /* default 0 means no upper cap. */

	if (vp.vm_id > 0) {
		gvt_dbg_core("before calling vgt_add_state_sysfs(): %d,%d,%d,%d,%d,%d,%d,%d\n",
				vp.vm_id, vp.qvm_pid, vp.qvm_chid, vp.aperture_sz, vp.gm_sz,
				vp.fence_sz, vp.gvt_primary, vp.cap);
	}

	ret = (vp.vm_id > 0) ? qvmgt_sysfs_add_instance(&vp) :
		qvmgt_sysfs_del_instance(&vp);

	return ret < 0 ? ret : count;
}

int qvmgt_sysfs_init(struct intel_gvt *gvt)
{
	int ret;

	gvt_kobj = kobject_create_and_add("vgt", &gvt->dev_priv->drm.primary->kdev->kobj);
	if (gvt_kobj == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ctrl_kobj = kobject_create_and_add("control", gvt_kobj);
	if (gvt_kobj == NULL) {
		ret = -ENOMEM;
		goto err;
	}

	ret = sysfs_merge_group(gvt_kobj, &gvt_attr_group);
	if (ret) {
		goto err;
	}

	ret = sysfs_merge_group(ctrl_kobj, &control_attr_group);
	if (ret) {
		goto err;
	}

	return 0;

err:
	if (gvt_kobj) {
		kobject_put(gvt_kobj);
	}
	if (ctrl_kobj) {
		kobject_put(ctrl_kobj);
	}
	return ret;
}

void qvmgt_sysfs_del(struct intel_gvt *gvt)
{
	sysfs_unmerge_group(ctrl_kobj, &control_attr_group);
	sysfs_unmerge_group(gvt_kobj, &gvt_attr_group);

	kobject_put(ctrl_kobj);
	kobject_put(gvt_kobj);
}

static int qvmgt_host_init(struct device *dev, void *gvt, const void *ops)
{
	int ret = -EFAULT;

	if (!gvt || !ops)
		return -EINVAL;

	qvmgt_priv.gvt = (struct intel_gvt *)gvt;
	intel_gvt_ops = (const struct intel_gvt_ops *)ops;

	ret = qvmgt_sysfs_init(qvmgt_priv.gvt);
	if (ret) {
		qvmgt_priv.gvt = NULL;
		intel_gvt_ops = NULL;
	}

	return ret;
}

static void qvmgt_host_exit(struct device *dev, void *gvt)
{
	/* TODO: destroy all existing vGPU instances here */
	qvmgt_sysfs_del(gvt);
	qvmgt_priv.gvt = NULL;
	intel_gvt_ops = NULL;
}

static int qvmgt_attach_vgpu(void *vgpu, unsigned long *handle)
{
	/* nothing to do here */
	return 0;
}

static void qvmgt_detach_vgpu(unsigned long handle)
{
	/* nothing to do here */
}

static int qvmgt_inject_msi(const unsigned long handle, const u32 addr_lo, const u16 data)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	struct qvgpu_msi_inject op;
	int rc;

	if (!info)
		return -EINVAL;

	op.type = QVGPU_MSI_INJECT;
	op.addr = addr_lo;
	op.data = data;

	rc = (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), NULL, 0);
	if (rc < EOK ) {
		gvt_err("VM%d: QVM failed to inject msi interrupt: %s!\n",
				info->vm_id, strerror(-rc));
		return rc;
	}
	return 0;
}

static unsigned long qvmgt_virt_to_mfn(void *const addr)
{
	off64_t paddr;
	if (mem_offset64(addr, NOFD, PAGE_SIZE, &paddr, NULL) == -1 ) {
		gvt_err("failed to obtain paddr for va=0x%p (%s)!\n", addr, strerror(errno));
		return INTEL_GVT_INVALID_ADDR;
	}
	return (unsigned long)(paddr >> PAGE_SHIFT);
}

static unsigned long qvmgt_gfn_to_mfn(unsigned long const handle, unsigned long const gfn)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	struct qvmgt_sysmem_blk *memblk;
	unsigned long mfn;

	if (!info)
		return INTEL_GVT_INVALID_ADDR;

	memblk = memblk_from_gfn(info, gfn);
	if (memblk == NULL) {
		gvt_err("VM%d: failed to find memblk for gfn=0x%lx!\n", info->vm_id, gfn);
		return INTEL_GVT_INVALID_ADDR;
	}
	mfn = memblk->host_pgnum + (gfn - (memblk->location >> PAGE_SHIFT));
	//gvt_dbg_core("VM%d: gfn=0x%lx -> mfn=0x%lx\n", info->vm_id, gfn, mfn);

	return mfn;
}

static int qvmgt_set_wp_page(unsigned long const handle, u64 const gfn)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	const unsigned long mfn = qvmgt_gfn_to_mfn(handle, gfn);
	struct qvgpu_sysmem_set op;
	int rc;

	if (!info || mfn == INTEL_GVT_INVALID_ADDR)
		return -EINVAL;

	op.type = QVGPU_SYSMEM_SET;
	op.flags = QVGPU_SYSMEM_PASS_RD|QVGPU_SYSMEM_TRAP_WR|QVGPU_SYSMEM_MODIFY;
	op.host_location = (_Uint64t)((_Uint64t)mfn << PAGE_SHIFT);
	op.guest_location = (_Uint64t)(gfn << PAGE_SHIFT);
	op.length = PAGE_SIZE;
	memset(&op.zero, 0, sizeof(op.zero));

	rc = (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), NULL, 0);
	if (rc < EOK ) {
		gvt_err("VM%d: QVM failed to set write protection: %s!\n",
				info->vm_id, strerror(-rc));
		return rc;
	}

	return 0;
}

static int qvmgt_unset_wp_page(unsigned long const handle, u64 const gfn)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	const unsigned long mfn = qvmgt_gfn_to_mfn(handle, gfn);
	struct qvgpu_sysmem_set op;
	int rc;

	if (!info || mfn == INTEL_GVT_INVALID_ADDR)
		return -EINVAL;

	op.type = QVGPU_SYSMEM_SET;
	op.flags = QVGPU_SYSMEM_PASS_RD|QVGPU_SYSMEM_PASS_WR|QVGPU_SYSMEM_MODIFY;
	op.host_location = (_Uint64t)((_Uint64t)mfn << PAGE_SHIFT);
	op.guest_location = (_Uint64t)(gfn << PAGE_SHIFT);
	op.length = PAGE_SIZE;
	memset(&op.zero, 0, sizeof(op.zero));

	rc = (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), NULL, 0);
	if (rc < EOK ) {
		gvt_err("VM%d: QVM failed to set write protection: %s!\n",
				info->vm_id, strerror(-rc));
		return rc;
	}

	return 0;
}

static int qvmgt_read_gpa(unsigned long const handle, unsigned long const gpa,
		void *const buf, unsigned long const len)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	void *va;

	if (!info)
		return -EINVAL;

	va = va_from_gpa(info, gpa);
	if (!va) {
		gvt_err("VM%d: failed to obtain vaddr for gpa=0x%lx!\n", info->vm_id, gpa);
		return -EFAULT;
	}
	memcpy(buf, va, len);
	return 0;
}

static int qvmgt_write_gpa(unsigned long const handle, unsigned long const gpa,
		void *const buf, unsigned long const len)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	void *va;

	if (!info)
		return -EINVAL;

	va = va_from_gpa(info, gpa);
	if (!va) {
		gvt_err("VM%d: failed to obtain vaddr for gpa=0x%lx!\n", info->vm_id, gpa);
		return -EFAULT;
	}
	memcpy(va, buf, len);
	return 0;
}

static inline int set_sys_mem(const struct qvmgt_hvm_dev *const info,
		unsigned long const hpa, unsigned long const gpa, unsigned const length,
		unsigned const flags)
{
	struct qvgpu_sysmem_set op;

	op.type = QVGPU_SYSMEM_SET;
	op.host_location = hpa;
	op.guest_location = gpa;
	op.length = length;
	op.flags = flags;
	memset(&op.zero, 0, sizeof(op.zero));

	return (int)MsgSend_r(info->cmd_coid, &op, sizeof(op), NULL, 0);
}

static int qvmgt_map_gfn_to_mfn(unsigned long const handle, unsigned long const gfn,
		unsigned long const mfn, unsigned int const nr, bool const map)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	unsigned flags;
	int rc;

	if (!info)
		return -EINVAL;

	gvt_dbg_core("VM%d: %s: gpfn = 0x%lx mfn = 0x%lx num = %d\n",
			info->vm_id, map?"map":"unmap", gfn, mfn, nr);

	flags = (map) ? QVGPU_SYSMEM_PASS_RD|QVGPU_SYSMEM_PASS_WR : QVGPU_SYSMEM_NONE;
	rc = set_sys_mem(info, mfn << PAGE_SHIFT, gfn << PAGE_SHIFT, nr * PAGE_SIZE, flags);
	if (rc < EOK ) {
		gvt_err("VM%d: QVM memory mapping failed: %s!\n",
				info->vm_id, strerror(-rc));
	}
	return rc;
}

static int qvmgt_set_trap_area(unsigned long const handle, u64 const start,
		u64 const end, bool const map)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	unsigned flags;
	int rc;

	if (!info)
		return -EINVAL;

	gvt_dbg_core("VM%d: %s: start = 0x%lx end = 0x%lx\n", info->vm_id,
			map?"trap":"untrap", start, end);

	flags = map ? (QVGPU_SYSMEM_TRAP_RD|QVGPU_SYSMEM_TRAP_WR) : QVGPU_SYSMEM_NONE;
	rc = set_sys_mem(info, 0, start, end - start + 1, flags);
	if (rc < EOK ) {
		gvt_err("VM%d: QVM failed to set trap: %s!\n",
			info->vm_id, strerror(-rc));
		return rc;
	}

	return 0;
}

static int qvmgt_map_pvmmio(unsigned long handle, u64 start,
		u64 end, bool map)
{
	const struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	u64 start_trap = start;
	unsigned long mfn;
	unsigned flags;
	int rc, stage = 1;

	/*
	 * Map mmio vreg space to guest as r/o.
	 * Note: mmio.vreg is allocated in physically contiguous memory,
	 * therefore, we can map the whole mmio block in one operation.
	 */
	mfn = qvmgt_virt_to_mfn(info->vgpu->mmio.vreg);
	if (mfn == INTEL_GVT_INVALID_ADDR) {
		gvt_err("VM%d: failed to get paddr for mmio.vreg!\n", info->vm_id);
		return -EINVAL;
	}
	flags = (map) ? QVGPU_SYSMEM_PASS_RD|QVGPU_SYSMEM_TRAP_WR : QVGPU_SYSMEM_NONE;
	rc = set_sys_mem(info, mfn << PAGE_SHIFT, start, VGT_PVINFO_PAGE, flags);
	if (rc < EOK ) goto err;
	++stage;

	rc = set_sys_mem(info, (mfn << PAGE_SHIFT) + VGT_PVINFO_END,
			start + VGT_PVINFO_END,
			qvmgt_priv.gvt->device_info.mmio_size - VGT_PVINFO_END,
			flags);
	if (rc < EOK ) goto err;
	++stage;

	/* set trap for pvinfo page */
	flags = (map) ? QVGPU_SYSMEM_TRAP_RD|QVGPU_SYSMEM_TRAP_WR : QVGPU_SYSMEM_NONE;
	rc = set_sys_mem(info, 0, start + VGT_PVINFO_PAGE, VGT_PVINFO_SIZE, flags);
	if (rc < EOK ) goto err;
	start_trap += qvmgt_priv.gvt->device_info.mmio_size;
	++stage;

	/* map shared page to guest as r/w */
	mfn = qvmgt_virt_to_mfn(info->vgpu->mmio.shared_page);
	if (mfn == INTEL_GVT_INVALID_ADDR) {
		gvt_err("VM%d: failed to get paddr for mmio.shared_page!\n", info->vm_id);
		return -EINVAL;
	}
	flags = (map) ? QVGPU_SYSMEM_PASS_RD|QVGPU_SYSMEM_PASS_WR : QVGPU_SYSMEM_NONE;
	rc = set_sys_mem(info, mfn << PAGE_SHIFT, start_trap, PAGE_SIZE, flags);
	if (rc < EOK ) goto err;
	start_trap += PAGE_SIZE;
	++stage;

	flags = map ? (QVGPU_SYSMEM_TRAP_RD|QVGPU_SYSMEM_TRAP_WR) : QVGPU_SYSMEM_NONE;
	rc = set_sys_mem(info, 0, start_trap, end - start_trap + 1, flags);
	if (rc < EOK ) goto err;

	return 0;
err:
	gvt_err("VM%d: QVM failed to set trap (stage-%d): %s!\n",
		info->vm_id, stage, strerror(-rc));
	return rc;
}

static int qvmgt_set_pvmmio(unsigned long handle, u64 start,
		u64 end, bool map)
{
	struct qvmgt_hvm_dev *const info = (struct qvmgt_hvm_dev *)handle;
	int rc;

	if (!info)
		return -EINVAL;

	gvt_dbg_core("VM%d: %s pvmmio: start = 0x%lx end = 0x%lx\n", info->vm_id,
			map?"set":"unset", start, end);

	if (map) {
		rc = info->pvmmio_used ? qvmgt_map_pvmmio(handle, start, end, false) :
				qvmgt_set_trap_area(handle, start, end, false);
		if (rc)
			return rc;
	}

	rc = qvmgt_map_pvmmio(handle, start, end, map);
	if (rc)
		return rc;

	if (!info->pvmmio_used && map)
		info->pvmmio_used = 1;

	return 0;
}

/*
 * According to Intel pause/unpause_domain functions MUST be implemented
 * when VT-d is enabled. Keep it in mind and implement later.
 */
static int qvmgt_pause_domain(unsigned long handle)
{
#ifdef CONFIG_INTEL_IOMMU
/*
 * DENSO implementation - no need to unpause vm
 *	gvt_err("IMPORTANT: Implement me!\n");
 *	BUG();
 */
#endif
	return 0;
}

static int qvmgt_unpause_domain(unsigned long handle)
{
#ifdef CONFIG_INTEL_IOMMU
/*
 * DENSO implementation - no need to unpause vm
 *	gvt_err("IMPORTANT: Implement me!\n");
 *	BUG();
 */
#endif
	return 0;
}

//TODO: We might want to modify it to do some QNX specific task(s) later.
static int qvmgt_dom0_ready(void)
{
	char *env[] = {"GVT_DOM0_READY=1", NULL};
	if(!gvt_kobj) {
		return 0;
	}
	gvt_dbg_core("qvmgt: Host is ready to accept guests VM(s)\n");
	return kobject_uevent_env(gvt_kobj, KOBJ_ADD, env);
}

struct intel_gvt_mpt qvmgt_mpt = {
	.host_init = qvmgt_host_init,
	.host_exit = qvmgt_host_exit,
	.attach_vgpu = qvmgt_attach_vgpu,
	.detach_vgpu = qvmgt_detach_vgpu,
	.inject_msi = qvmgt_inject_msi,
	.from_virt_to_mfn = qvmgt_virt_to_mfn,
	.set_wp_page = qvmgt_set_wp_page,
	.unset_wp_page = qvmgt_unset_wp_page,
	.read_gpa = qvmgt_read_gpa,
	.write_gpa = qvmgt_write_gpa,
	.gfn_to_mfn = qvmgt_gfn_to_mfn,
	.map_gfn_to_mfn = qvmgt_map_gfn_to_mfn,
	.set_trap_area = qvmgt_set_trap_area,
	.set_pvmmio = qvmgt_set_pvmmio,
	.pause_domain = qvmgt_pause_domain,
	.unpause_domain= qvmgt_unpause_domain,
	.dom0_ready = qvmgt_dom0_ready,
};
EXPORT_SYMBOL_GPL(qvmgt_mpt);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/gvt/qvmgt.c $ $Rev: 856018 $")
#endif
