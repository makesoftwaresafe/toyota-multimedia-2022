/*
 * Copyright(c) 2011-2016 Intel Corporation. All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Authors:
 *    Ke Yu
 *    Kevin Tian <kevin.tian@intel.com>
 *    Dexuan Cui
 *
 * Contributors:
 *    Tina Zhang <tina.zhang@intel.com>
 *    Min He <min.he@intel.com>
 *    Niu Bing <bing.niu@intel.com>
 *    Zhi Wang <zhi.a.wang@intel.com>
 *
 */

#include "i915_drv.h"
#include "gvt.h"

/**
 * intel_vgpu_gpa_to_mmio_offset - translate a GPA to MMIO offset
 * @vgpu: a vGPU
 *
 * Returns:
 * Zero on success, negative error code if failed
 */
int intel_vgpu_gpa_to_mmio_offset(struct intel_vgpu *vgpu, u64 gpa)
{
	u64 gttmmio_gpa = *(u64 *)(vgpu_cfg_space(vgpu) + PCI_BASE_ADDRESS_0) &
			  ~GENMASK(3, 0);
	return gpa - gttmmio_gpa;
}

#define reg_is_mmio(gvt, reg)  \
	(reg >= 0 && reg < gvt->device_info.mmio_size)

#define reg_is_gtt(gvt, reg)   \
	(reg >= gvt->device_info.gtt_start_offset \
	 && reg < gvt->device_info.gtt_start_offset + gvt_ggtt_sz(gvt))


static inline void mmio_accounting_read(struct intel_vgpu *vgpu,
		unsigned long offset, cycles_t cycles)
{
	struct vgpu_mmio_accounting_reg_stat *stat;

	if (!vgpu->stat.mmio_accounting)
		return;

	stat = &vgpu->stat.mmio_accounting_reg_stats[offset >> 2];
	stat->r_count++;
	stat->r_cycles += cycles;
}

static inline void mmio_accounting_write(struct intel_vgpu *vgpu,
		unsigned long offset, cycles_t cycles)
{
	struct vgpu_mmio_accounting_reg_stat *stat;

	if (!vgpu->stat.mmio_accounting)
		return;

	stat = &vgpu->stat.mmio_accounting_reg_stats[offset >> 2];
	stat->w_count++;
	stat->w_cycles += cycles;
}


static void failsafe_emulate_mmio_rw(struct intel_vgpu *vgpu, uint64_t pa,
		void *p_data, unsigned int bytes, bool read)
{
	struct intel_gvt *gvt = NULL;
	void *pt = NULL;
	unsigned int offset = 0;

	if (!vgpu || !p_data)
		return;

	gvt = vgpu->gvt;
	offset = intel_vgpu_gpa_to_mmio_offset(vgpu, pa);
	if (reg_is_mmio(gvt, offset)) {
		if (read)
			intel_vgpu_default_mmio_read(vgpu, offset, p_data,
					bytes);
		else
			intel_vgpu_default_mmio_write(vgpu, offset, p_data,
					bytes);
	} else if (reg_is_gtt(gvt, offset) &&
			vgpu->gtt.ggtt_mm->virtual_page_table) {
		offset -= gvt->device_info.gtt_start_offset;
		pt = vgpu->gtt.ggtt_mm->virtual_page_table + offset;
		if (read)
			memcpy(p_data, pt, bytes);
		else
			memcpy(pt, p_data, bytes);

	} else if (atomic_read(&vgpu->gtt.n_write_protected_guest_page)) {
		struct intel_vgpu_guest_page *gp;

		/* Since we enter the failsafe mode early during guest boot,
		 * guest may not have chance to set up its ppgtt table, so
		 * there should not be any wp pages for guest. Keep the wp
		 * related code here in case we need to handle it in furture.
		 */
		gp = intel_vgpu_find_guest_page(vgpu, pa >> PAGE_SHIFT);
		if (gp) {
			/* remove write protection to prevent furture traps */
			intel_vgpu_clean_guest_page(vgpu, gp);
			if (read)
				intel_gvt_hypervisor_read_gpa(vgpu, pa,
						p_data, bytes);
			else
				intel_gvt_hypervisor_write_gpa(vgpu, pa,
						p_data, bytes);
		}
	}
}

/**
 * intel_vgpu_emulate_mmio_read - emulate MMIO read
 * @vgpu: a vGPU
 * @pa: guest physical address
 * @p_data: data return buffer
 * @bytes: access data length
 *
 * Returns:
 * Zero on success, negative error code if failed
 */
int intel_vgpu_emulate_mmio_read_locked(struct intel_vgpu *vgpu, uint64_t pa,
		void *p_data, unsigned int bytes)
{
	struct intel_gvt *gvt = vgpu->gvt;
	struct intel_gvt_mmio_info *mmio;
	unsigned int offset = 0;
	int ret = -EINVAL;
	cycles_t t0, t1;

	t0 = i915_get_cycles();

	if (vgpu->failsafe) {
		failsafe_emulate_mmio_rw(vgpu, pa, p_data, bytes, true);
		return 0;
	}

	if (atomic_read(&vgpu->gtt.n_write_protected_guest_page)) {
		struct intel_vgpu_guest_page *gp;

		gp = intel_vgpu_find_guest_page(vgpu, pa >> PAGE_SHIFT);
		if (gp) {
			ret = intel_gvt_hypervisor_read_gpa(vgpu, pa,
					p_data, bytes);
			if (ret) {
				gvt_vgpu_err("guest page read error %d, "
					"gfn 0x%lx, pa 0x%llx, var 0x%x, len %d\n",
					ret, gp->gfn, pa, *(u32 *)p_data,
					bytes);
			}
			return ret;
		}
	}

	offset = intel_vgpu_gpa_to_mmio_offset(vgpu, pa);

	if (WARN_ON(bytes > 8))
		goto err;

	if (reg_is_gtt(gvt, offset)) {
		if (WARN_ON(!IS_ALIGNED(offset, 4) && !IS_ALIGNED(offset, 8)))
			goto err;
		if (WARN_ON(bytes != 4 && bytes != 8))
			goto err;
		if (WARN_ON(!reg_is_gtt(gvt, offset + bytes - 1)))
			goto err;

		ret = intel_vgpu_emulate_gtt_mmio_read(vgpu, offset,
				p_data, bytes);
		if (ret)
			goto err;
		return ret;
	}

	if (WARN_ON_ONCE(!reg_is_mmio(gvt, offset))) {
		ret = intel_gvt_hypervisor_read_gpa(vgpu, pa, p_data, bytes);
		return ret;
	}

	if (WARN_ON(!reg_is_mmio(gvt, offset + bytes - 1)))
		goto err;

	if (!intel_gvt_mmio_is_unalign(gvt, offset)) {
		if (WARN_ON(!IS_ALIGNED(offset, bytes)))
			goto err;
	}

	mmio = intel_gvt_find_mmio_info(gvt, rounddown(offset, 4));
	if (mmio) {
		if (!intel_gvt_mmio_is_unalign(gvt, mmio->offset)) {
			if (WARN_ON(offset + bytes > mmio->offset + mmio->size))
				goto err;
			if (WARN_ON(mmio->offset != offset))
				goto err;
		}
		ret = mmio->read(vgpu, offset, p_data, bytes);
	} else {
		ret = intel_vgpu_default_mmio_read(vgpu, offset, p_data, bytes);

		if (!vgpu->mmio.disable_warn_untrack) {
			gvt_vgpu_err("read untracked MMIO %x(%dB) val %x\n",
				offset, bytes, *(u32 *)p_data);

			if (offset == 0x206c) {
				gvt_vgpu_err("------------------------------------------\n");
				gvt_vgpu_err("likely triggers a gfx reset\n");
				gvt_vgpu_err("------------------------------------------\n");
				vgpu->mmio.disable_warn_untrack = true;
			}
		}
	}

	if (ret)
		goto err;

	intel_gvt_mmio_set_accessed(gvt, offset);
	t1 = i915_get_cycles();
	vgpu->stat.mmio_rcnt++;
	vgpu->stat.mmio_rcycles += t1 - t0;
	mmio_accounting_read(vgpu, offset, t1 - t0);
	return 0;
err:
	gvt_vgpu_err("fail to emulate MMIO read %08x len %d\n",
			offset, bytes);
	return ret;
}

int intel_vgpu_emulate_mmio_read(struct intel_vgpu *vgpu, uint64_t pa,
		void *p_data, unsigned int bytes)
{
	int ret;
	cycles_t t0, t1;

	t0 = i915_get_cycles();
	mutex_lock(&vgpu->gvt->lock);
	t1 = i915_get_cycles();
	vgpu->stat.mmio_rl_cycles += t1 - t0;

	ret = intel_vgpu_emulate_mmio_read_locked(vgpu, pa, p_data, bytes);

	t0 = i915_get_cycles();
	mutex_unlock(&vgpu->gvt->lock);
	t1 = i915_get_cycles();
	vgpu->stat.mmio_rl_cycles += t1 - t0;
	vgpu->stat.mmio_rl_cnt++;

	return ret;
}

/**
 * intel_vgpu_emulate_mmio_write - emulate MMIO write
 * @vgpu: a vGPU
 * @pa: guest physical address
 * @p_data: write data buffer
 * @bytes: access data length
 *
 * Returns:
 * Zero on success, negative error code if failed
 */
int intel_vgpu_emulate_mmio_write_locked(struct intel_vgpu *vgpu, uint64_t pa,
		void *p_data, unsigned int bytes)
{
	struct intel_gvt *gvt = vgpu->gvt;
	struct intel_gvt_mmio_info *mmio;
	unsigned int offset = 0;
	u32 old_vreg = 0, old_sreg = 0;
	int ret = -EINVAL;
	cycles_t t0, t1;

	t0 = i915_get_cycles();

	if (vgpu->failsafe) {
		failsafe_emulate_mmio_rw(vgpu, pa, p_data, bytes, false);
		return 0;
	}

	if (atomic_read(&vgpu->gtt.n_write_protected_guest_page)) {
		struct intel_vgpu_guest_page *gp;

		gp = intel_vgpu_find_guest_page(vgpu, pa >> PAGE_SHIFT);
		if (gp) {
			ret = gp->handler(gp, pa, p_data, bytes);
			if (ret) {
				gvt_err("guest page write error %d, "
					"gfn 0x%lx, pa 0x%llx, "
					"var 0x%x, len %d\n",
					ret, gp->gfn, pa,
					*(u32 *)p_data, bytes);
			}
			t1 = i915_get_cycles();
			vgpu->stat.wp_cycles += t1 - t0;
			vgpu->stat.wp_cnt++;
			return ret;
		}
	}

	offset = intel_vgpu_gpa_to_mmio_offset(vgpu, pa);

	if (WARN_ON(bytes > 8))
		goto err;

	if (reg_is_gtt(gvt, offset)) {
		if (WARN_ON(!IS_ALIGNED(offset, 4) && !IS_ALIGNED(offset, 8)))
			goto err;
		if (WARN_ON(bytes != 4 && bytes != 8))
			goto err;
		if (WARN_ON(!reg_is_gtt(gvt, offset + bytes - 1)))
			goto err;

		ret = intel_vgpu_emulate_gtt_mmio_write(vgpu, offset,
				p_data, bytes);
		if (ret)
			goto err;
		return ret;
	}

	if (WARN_ON_ONCE(!reg_is_mmio(gvt, offset))) {
		ret = intel_gvt_hypervisor_write_gpa(vgpu, pa, p_data, bytes);
		return ret;
	}

	mmio = intel_gvt_find_mmio_info(gvt, rounddown(offset, 4));
	if (!mmio && !vgpu->mmio.disable_warn_untrack)
		gvt_dbg_mmio("vgpu%d: write untracked MMIO %x len %d val %x\n",
				vgpu->id, offset, bytes, *(u32 *)p_data);

	if (!intel_gvt_mmio_is_unalign(gvt, offset)) {
		if (WARN_ON(!IS_ALIGNED(offset, bytes)))
			goto err;
	}

	if (mmio) {
		u64 ro_mask = mmio->ro_mask;

		if (!intel_gvt_mmio_is_unalign(gvt, mmio->offset)) {
			if (WARN_ON(offset + bytes > mmio->offset + mmio->size))
				goto err;
			if (WARN_ON(mmio->offset != offset))
				goto err;
		}

		if (intel_gvt_mmio_has_mode_mask(gvt, mmio->offset)) {
			old_vreg = vgpu_vreg(vgpu, offset);
			old_sreg = vgpu_sreg(vgpu, offset);
		}

		if (!ro_mask) {
			ret = mmio->write(vgpu, offset, p_data, bytes);
		} else {
			/* Protect RO bits like HW */
			u64 data = 0;

			/* all register bits are RO. */
			if (ro_mask == ~(u64)0) {
				gvt_vgpu_err("try to write RO reg %x\n",
					offset);
				ret = 0;
				goto out;
			}
			/* keep the RO bits in the virtual register */
			memcpy(&data, p_data, bytes);
			data &= ~mmio->ro_mask;
			data |= vgpu_vreg(vgpu, offset) & mmio->ro_mask;
			ret = mmio->write(vgpu, offset, &data, bytes);
		}

		/* higher 16bits of mode ctl regs are mask bits for change */
		if (intel_gvt_mmio_has_mode_mask(gvt, mmio->offset)) {
			u32 mask = vgpu_vreg(vgpu, offset) >> 16;

			vgpu_vreg(vgpu, offset) = (old_vreg & ~mask)
				| (vgpu_vreg(vgpu, offset) & mask);
			vgpu_sreg(vgpu, offset) = (old_sreg & ~mask)
				| (vgpu_sreg(vgpu, offset) & mask);
		}
	} else
		ret = intel_vgpu_default_mmio_write(vgpu, offset, p_data,
				bytes);
	if (ret)
		goto err;

	if (vgpu->entire_nonctxmmio_checked
		&& intel_gvt_mmio_is_non_context(vgpu->gvt, offset)
		&& vgpu_vreg(vgpu, offset)
			!= *(u32 *)(vgpu->gvt->mmio.mmio_host_cache + offset)) {
		gvt_err("vgpu%d unexpected non-context MMIO change at 0x%x:0x%x,0x%x\n",
			vgpu->id, offset, vgpu_vreg(vgpu, offset),
			*(u32 *)(vgpu->gvt->mmio.mmio_host_cache + offset));
	}

out:
	intel_gvt_mmio_set_accessed(gvt, offset);
	t1 = i915_get_cycles();
	vgpu->stat.mmio_wcnt++;
	vgpu->stat.mmio_wcycles += t1 - t0;
	mmio_accounting_write(vgpu, offset, t1 - t0);
	return 0;
err:
	gvt_vgpu_err("fail to emulate MMIO write %08x len %d\n", offset,
		     bytes);
	return ret;
}

int intel_vgpu_emulate_mmio_write(struct intel_vgpu *vgpu, uint64_t pa,
		void *p_data, unsigned int bytes)
{
	int ret;
	cycles_t t0, t1;

	t0 = i915_get_cycles();
	mutex_lock(&vgpu->gvt->lock);
	t1 = i915_get_cycles();
	vgpu->stat.mmio_wl_cycles += t1 - t0;

	ret = intel_vgpu_emulate_mmio_write_locked(vgpu, pa, p_data, bytes);

	t0 = i915_get_cycles();
	mutex_unlock(&vgpu->gvt->lock);
	t1 = i915_get_cycles();
	vgpu->stat.mmio_wl_cycles += t1 - t0;
	vgpu->stat.mmio_wl_cnt++;
	return ret;
}

/**
 * intel_vgpu_reset_mmio - reset virtual MMIO space
 * @vgpu: a vGPU
 *
 */
void intel_vgpu_reset_mmio(struct intel_vgpu *vgpu, bool dmlr)
{
	struct intel_gvt *gvt = vgpu->gvt;
	const struct intel_gvt_device_info *info = &gvt->device_info;
	void  *mmio = gvt->firmware.mmio;

	if (dmlr) {
		memcpy(vgpu->mmio.vreg, mmio, info->mmio_size);
		memcpy(vgpu->mmio.sreg, mmio, info->mmio_size);

		vgpu_vreg(vgpu, GEN6_GT_THREAD_STATUS_REG) = 0;

		/* set the bit 0:2(Core C-State ) to C0 */
		vgpu_vreg(vgpu, GEN6_GT_CORE_STATUS) = 0;

		vgpu->mmio.disable_warn_untrack = false;
	} else {
#define GVT_GEN8_MMIO_RESET_OFFSET		(0x44200)
		/* only reset the engine related, so starting with 0x44200
		 * interrupt include DE,display mmio related will not be
		 * touched
		 */
		memcpy(vgpu->mmio.vreg, mmio, GVT_GEN8_MMIO_RESET_OFFSET);
		memcpy(vgpu->mmio.sreg, mmio, GVT_GEN8_MMIO_RESET_OFFSET);
	}

	/* below vreg init value are got from handler.c,
	 * which won't change during vgpu life cycle
	 */
	vgpu_vreg(vgpu, 0xe651c) = 1 << 17;
	vgpu_vreg(vgpu, 0xe661c) = 1 << 17;
	vgpu_vreg(vgpu, 0xe671c) = 1 << 17;
	vgpu_vreg(vgpu, 0xe681c) = 1 << 17;
	vgpu_vreg(vgpu, 0xe6c04) = 3;
	vgpu_vreg(vgpu, 0xe6e1c) = 0x2f << 16;

	/* Non-context MMIOs need entire check again if mmio/vgpu reset */
	vgpu->entire_nonctxmmio_checked = false;
}

/**
 * intel_vgpu_init_mmio - init MMIO  space
 * @vgpu: a vGPU
 *
 * Returns:
 * Zero on success, negative error code if failed
 */
int intel_vgpu_init_mmio(struct intel_vgpu *vgpu)
{
	const struct intel_gvt_device_info *info = &vgpu->gvt->device_info;

	vgpu->mmio.sreg = vzalloc(info->mmio_size);
#ifndef __QNXNTO__
	vgpu->mmio.vreg = (void *)__get_free_pages(GFP_KERNEL,
			info->mmio_size_order);
	vgpu->mmio.shared_page = (struct gvt_shared_page *) __get_free_pages(
			GFP_KERNEL, 0);
#else
	vgpu->qnx_vreg_pages = alloc_pages_qnx(GFP_KERNEL,
			(1 << info->mmio_size_order) + 1);
	if (!vgpu->qnx_vreg_pages) {
		return -ENOMEM;
	}
	vgpu->mmio.shared_page = page_address(vgpu->qnx_vreg_pages);
	vgpu->mmio.vreg = page_address(vgpu->qnx_vreg_pages) +
		PAGE_SIZE;
#endif /* __QNXNTO__ */
	if (!vgpu->mmio.vreg || !vgpu->mmio.sreg || !vgpu->mmio.shared_page)
		return -ENOMEM;

	intel_vgpu_reset_mmio(vgpu, true);

	return 0;
}

/**
 * intel_vgpu_clean_mmio - clean MMIO space
 * @vgpu: a vGPU
 *
 */
void intel_vgpu_clean_mmio(struct intel_vgpu *vgpu)
{
	const struct intel_gvt_device_info *info = &vgpu->gvt->device_info;

	vfree(vgpu->mmio.sreg);
#ifndef __QNXNTO__
	free_pages((unsigned long) vgpu->mmio.vreg, info->mmio_size_order);
	free_pages((unsigned long) vgpu->mmio.shared_page, 0);
#else
	__free_page_qnx(vgpu->qnx_vreg_pages);
	vgpu->qnx_vreg_pages = NULL;
#endif /* __QNXNTO__ */
	vgpu->mmio.vreg = vgpu->mmio.sreg = vgpu->mmio.shared_page = NULL;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/gvt/mmio.c $ $Rev: 845437 $")
#endif
