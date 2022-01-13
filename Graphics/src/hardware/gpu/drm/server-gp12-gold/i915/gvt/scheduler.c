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
 *    Zhi Wang <zhi.a.wang@intel.com>
 *
 * Contributors:
 *    Ping Gao <ping.a.gao@intel.com>
 *    Tina Zhang <tina.zhang@intel.com>
 *    Chanbin Du <changbin.du@intel.com>
 *    Min He <min.he@intel.com>
 *    Bing Niu <bing.niu@intel.com>
 *    Zhenyu Wang <zhenyuw@linux.intel.com>
 *
 */

#include <linux/kthread.h>

#include "i915_drv.h"
#include "gvt.h"

#define RING_CTX_OFF(x) \
	offsetof(struct execlist_ring_context, x)

static void set_context_pdp_root_pointer(
		struct execlist_ring_context *ring_context,
		u32 pdp[8])
{
	struct execlist_mmio_pair *pdp_pair = &ring_context->pdp3_UDW;
	int i;

	for (i = 0; i < 8; i++)
		pdp_pair[i].val = pdp[7 - i];
}

static bool enable_lazy_shadow_ctx = true;
static int populate_shadow_context(struct intel_vgpu_workload *workload)
{
	struct intel_vgpu *vgpu = workload->vgpu;
	struct intel_gvt *gvt = vgpu->gvt;
	int ring_id = workload->ring_id;
	struct i915_gem_context *shadow_ctx = workload->vgpu->shadow_ctx;
	struct drm_i915_gem_object *ctx_obj =
		shadow_ctx->engine[ring_id].state->obj;
	struct execlist_ring_context *shadow_ring_context;
	struct page *page;
	void *dst;
	unsigned long context_gpa, context_page_num;
	struct drm_i915_private *dev_priv = gvt->dev_priv;
	struct i915_ggtt *ggtt = &gvt->dev_priv->ggtt;
	dma_addr_t addr;
	gen8_pte_t __iomem *pte;
	int i;

	gvt_dbg_sched("ring id %d workload lrca %x\n", ring_id,
			workload->ctx_desc.lrca);

	context_page_num = intel_lr_context_size(
			gvt->dev_priv->engine[ring_id]);

	context_page_num = context_page_num >> PAGE_SHIFT;

	if (IS_BROADWELL(gvt->dev_priv) && ring_id == RCS)
		context_page_num = 19;

	i = 2;

#ifdef __QNXNTO__
	/*
	 * DENSO - take lazy shadow context in IOMMU
	 * simulate stop_machine == irq_disable at touching GGTT entries
	 * now enable_lazy_shadow_context not be false
	 */
	if(intel_iommu_gfx_mapped) {
		/* enable_lazy_shadow_ctx = false; */
	}
#define BKL_start()	local_irq_disable()
#define BKL_finish()	local_irq_enable()
#else
#ifdef CONFIG_INTEL_IOMMU
	/*
	 * In case IOMMU for graphics is turned on, we don't want to
	 * turn on lazy shadow context feature because it will touch
	 * GGTT entries which require a BKL and since this is a
	 * performance enhancement feature, we will end up negating
	 * the performance.
	 */
	if(intel_iommu_gfx_mapped) {
		enable_lazy_shadow_ctx = false;
	}
#endif
#define BKL_start()
#define BKL_finish()
#endif

	while (i < context_page_num) {
		context_gpa = intel_vgpu_gma_to_gpa(vgpu->gtt.ggtt_mm,
				(u32)((workload->ctx_desc.lrca + i) <<
				GTT_PAGE_SHIFT));
		if (context_gpa == INTEL_GVT_INVALID_ADDR) {
			gvt_vgpu_err("Invalid guest context descriptor\n");
			return -EINVAL;
		}

		if (!enable_lazy_shadow_ctx) {
			page = i915_gem_object_get_page(ctx_obj,
					LRC_PPHWSP_PN + i);
			dst = kmap(page);
			intel_gvt_hypervisor_read_gpa(vgpu, context_gpa, dst,
				GTT_PAGE_SIZE);
			kunmap(page);
		} else {
			unsigned long mfn;

			BKL_start();
			addr = i915_ggtt_offset(
					shadow_ctx->engine[ring_id].state) +
					(LRC_PPHWSP_PN + i) * PAGE_SIZE;
			pte = (gen8_pte_t __iomem *)ggtt->gsm +
					(addr >> PAGE_SHIFT);

			mfn = intel_gvt_hypervisor_gfn_to_mfn(vgpu,
					context_gpa >> 12);
			if (mfn == INTEL_GVT_INVALID_ADDR) {
				gvt_vgpu_err("fail to translate gfn during context shadow\n");
				return -ENXIO;
			}

			mfn <<= 12;
			mfn |= _PAGE_PRESENT | _PAGE_RW | PPAT_CACHED_INDEX;
			writeq(mfn, pte);
			BKL_finish();
		}

		i++;
	}

	I915_WRITE(GFX_FLSH_CNTL_GEN6, GFX_FLSH_CNTL_EN);
	POSTING_READ(GFX_FLSH_CNTL_GEN6);

	page = i915_gem_object_get_page(ctx_obj, LRC_STATE_PN);
	shadow_ring_context = kmap(page);

#define COPY_REG(name) \
	intel_gvt_hypervisor_read_gpa(vgpu, workload->ring_context_gpa \
		+ RING_CTX_OFF(name.val), &shadow_ring_context->name.val, 4)

	COPY_REG(ctx_ctrl);
	COPY_REG(ctx_timestamp);

	if (ring_id == RCS) {
		COPY_REG(bb_per_ctx_ptr);
		COPY_REG(rcs_indirect_ctx);
		COPY_REG(rcs_indirect_ctx_offset);
	}
#undef COPY_REG

	set_context_pdp_root_pointer(shadow_ring_context,
				     workload->shadow_mm->shadow_page_table);

	intel_gvt_hypervisor_read_gpa(vgpu,
			workload->ring_context_gpa +
			sizeof(*shadow_ring_context),
			(void *)shadow_ring_context +
			sizeof(*shadow_ring_context),
			GTT_PAGE_SIZE - sizeof(*shadow_ring_context));

	kunmap(page);
	return 0;
}


static void shadow_context_descriptor_update(struct i915_gem_context *ctx,
		struct intel_engine_cs *engine)
{
	struct intel_context *ce = &ctx->engine[engine->id];
	u64 desc = 0;

	desc = ce->lrc_desc;

	/* Update bits 0-11 of the context descriptor which includes flags
	 * like GEN8_CTX_* cached in desc_template
	 */
	desc &= U64_MAX << 12;
	desc |= ctx->desc_template & ((1ULL << 12) - 1);
	desc |= engine->ctx_desc_template;

	ce->lrc_desc = desc;
}

static inline int get_ctx_perfmon_enable(struct intel_vgpu_workload *workload)
{
	unsigned long hws_gpa;
	struct intel_vgpu *vgpu = workload->vgpu;
	int enabled = 0;

	hws_gpa = intel_vgpu_gma_to_gpa(vgpu->gtt.ggtt_mm,
			(u32)((workload->ctx_desc.lrca << GTT_PAGE_SHIFT) +
			I915_GEM_HWS_PERFMON_ADDR));
	if (hws_gpa == INTEL_GVT_INVALID_ADDR) {
		gvt_vgpu_err("invalid guest context descriptor\n");
		return -EFAULT;
	}
	intel_gvt_hypervisor_read_gpa(vgpu, hws_gpa, &enabled,
			sizeof(enabled));
	return enabled;
}

static inline int set_ctx_perfmon_enable(struct intel_vgpu_workload *workload,
		int enable)
{
	unsigned long hws_gpa;
	struct intel_vgpu *vgpu = workload->vgpu;

	hws_gpa = intel_vgpu_gma_to_gpa(vgpu->gtt.ggtt_mm,
			(u32)((workload->ctx_desc.lrca << GTT_PAGE_SHIFT) +
			I915_GEM_HWS_PERFMON_ADDR));
	if (hws_gpa == INTEL_GVT_INVALID_ADDR) {
		gvt_vgpu_err("invalid guest context descriptor\n");
		return -EFAULT;
	}
	return intel_gvt_hypervisor_write_gpa(vgpu, hws_gpa, &enable,
			sizeof(enable));
}

void perfmon_send_config(u32 **pcs,
		struct drm_i915_perfmon_config *config);

static int gvt_program_perfmon(struct drm_i915_private *dev_priv,
		struct intel_vgpu_workload *workload,
		struct intel_ring *ring,
		struct drm_i915_gem_request *req,
		struct i915_gem_context *ctx)
{
	struct drm_i915_perfmon_config *config_oa, *config_gp;
	size_t size;
	int ret = 0;
	u32 *cs;
	int enabled = get_ctx_perfmon_enable(workload);

	if (enabled < 0)
		return enabled;

	if (!atomic_read(&dev_priv->perfmon.config.enable) &&
		(enabled == 0))
		return 0;

	ret = mutex_lock_interruptible(&dev_priv->perfmon.config.lock);
	if (ret)
		return ret;

	if (!atomic_read(&dev_priv->perfmon.config.enable)) {
		if (enabled == 1) {
			/* write 0 to OA_CTX_CONTROL to stop counters */
			cs = intel_ring_begin(req, 4);
			if (!IS_ERR(cs)) {
				*cs++ = MI_NOOP;
				*cs++ = MI_LOAD_REGISTER_IMM(1);
				*cs++ = GEN8_OA_CTX_CONTROL;
				*cs++ = 0;
				intel_ring_advance(req, cs);
			}
			set_ctx_perfmon_enable(workload, 0);

		}
		goto unlock;
	} else if (enabled == 1)
		goto unlock;

	/* check for pending OA config */
	config_oa = i915_perfmon_get_config(dev_priv, ctx,
				       &dev_priv->perfmon.config.oa,
				       &ctx->perfmon.config.oa.pending,
				       ctx->perfmon.config.oa.submitted_id);

	/* check for pending PERFMON config */
	config_gp = i915_perfmon_get_config(dev_priv, ctx,
				       &dev_priv->perfmon.config.gp,
				       &ctx->perfmon.config.gp.pending,
				       ctx->perfmon.config.gp.submitted_id);

	size = (config_oa ? config_oa->size : 0) +
		(config_gp ? config_gp->size : 0);

	if (size == 0)
		goto unlock;

	cs = intel_ring_begin(req, 4 * size);
	if (IS_ERR(cs))
		goto unlock;

	/* submit pending OA config */
	if (config_oa) {
		perfmon_send_config(&cs, config_oa);
		i915_perfmon_update_workaround_bb(dev_priv, config_oa);
	}

	/* submit pending general purpose perfmon counters config */
	if (config_gp)
		perfmon_send_config(&cs, config_gp);

	intel_ring_advance(req, cs);
	set_ctx_perfmon_enable(workload, 1);

unlock:
	mutex_unlock(&dev_priv->perfmon.config.lock);
	return ret;
}

/*
 * intel_gvt_scan_and_shadow_workload - audit the workload by scanning and
 * shadow it as well, include ringbuffer,wa_ctx and ctx.
 * @workload: an abstract entity for each execlist submission.
 *
 * This function is called before the workload submitting to i915, to make
 * sure the content of the workload is valid.
 */
int intel_gvt_scan_and_shadow_workload(struct intel_vgpu_workload *workload)
{
	int ring_id = workload->ring_id;
	struct i915_gem_context *shadow_ctx = workload->vgpu->shadow_ctx;
	struct drm_i915_private *dev_priv = workload->vgpu->gvt->dev_priv;
	struct drm_i915_gem_request *rq;
	struct intel_vgpu *vgpu = workload->vgpu;
	int ret;

	if (workload->shadowed)
		return 0;

	shadow_ctx->desc_template &= ~(0x3 << GEN8_CTX_ADDRESSING_MODE_SHIFT);
	shadow_ctx->desc_template |= workload->ctx_desc.addressing_mode <<
				    GEN8_CTX_ADDRESSING_MODE_SHIFT;

	if (!test_and_set_bit(ring_id, vgpu->shadow_ctx_desc_updated))
		shadow_context_descriptor_update(shadow_ctx,
					dev_priv->engine[ring_id]);

	rq = i915_gem_request_alloc(dev_priv->engine[ring_id], shadow_ctx);
	if (IS_ERR(rq)) {
		gvt_vgpu_err("fail to allocate gem request\n");
		ret = PTR_ERR(rq);
		goto out;
	}

	gvt_dbg_sched("ring id %d get i915 gem request %p\n", ring_id, rq);

	rq->vgpu_id = vgpu->id;
	workload->req = i915_gem_request_get(rq);
	if (vgpu)
		workload->req->perf.vgpu = vgpu;

	if (IS_GEN9(dev_priv) && ring_id == RCS)
		gvt_program_perfmon(dev_priv, workload, workload->req->ring,
				     workload->req, shadow_ctx);

	ret = intel_gvt_scan_and_shadow_ringbuffer(workload);
	if (ret)
		goto out;

	if ((workload->ring_id == RCS) &&
	    (workload->wa_ctx.indirect_ctx.size != 0)
	    && gvt_shadow_wa_ctx) {
		ret = intel_gvt_scan_and_shadow_wa_ctx(&workload->wa_ctx);
		if (ret)
			goto out;
	}

	ret = populate_shadow_context(workload);
	if (ret)
		goto out;

	workload->shadowed = true;

out:
	return ret;
}

static void gen8_shadow_pid_cid(struct intel_vgpu_workload *workload)
{
	int ring_id = workload->ring_id;
	struct drm_i915_private *dev_priv = workload->vgpu->gvt->dev_priv;
	struct intel_engine_cs *engine = dev_priv->engine[ring_id];
	u32 *cs;

	/* Copy the PID and CID from the guest's HWS page to the host's one */
	cs = intel_ring_begin(workload->req, 16);
	*cs++ = MI_LOAD_REGISTER_MEM_GEN8 | MI_SRM_LRM_GLOBAL_GTT;
	*cs++ = i915_mmio_reg_offset(NOPID);
	*cs++ = (workload->ctx_desc.lrca << GTT_PAGE_SHIFT) + I915_GEM_HWS_PID_ADDR;
	*cs++ = 0;
	*cs++ = MI_STORE_REGISTER_MEM_GEN8 | MI_SRM_LRM_GLOBAL_GTT;
	*cs++ = i915_mmio_reg_offset(NOPID);
	*cs++ = engine->status_page.ggtt_offset + I915_GEM_HWS_PID_ADDR +
		(workload->vgpu->id << MI_STORE_DWORD_INDEX_SHIFT);
	*cs++ = 0;
	*cs++ = MI_LOAD_REGISTER_MEM_GEN8 | MI_SRM_LRM_GLOBAL_GTT;
	*cs++ = i915_mmio_reg_offset(NOPID);
	*cs++ = (workload->ctx_desc.lrca << GTT_PAGE_SHIFT) + I915_GEM_HWS_CID_ADDR;
	*cs++ = 0;
	*cs++ = MI_STORE_REGISTER_MEM_GEN8 | MI_SRM_LRM_GLOBAL_GTT;
	*cs++ = i915_mmio_reg_offset(NOPID);
	*cs++ = engine->status_page.ggtt_offset + I915_GEM_HWS_CID_ADDR +
		(workload->vgpu->id << MI_STORE_DWORD_INDEX_SHIFT);
	*cs++ = 0;
	intel_ring_advance(workload->req, cs);
}

static int dispatch_workload(struct intel_vgpu_workload *workload)
{
	int ring_id = workload->ring_id;
	struct i915_gem_context *shadow_ctx = workload->vgpu->shadow_ctx;
	struct drm_i915_private *dev_priv = workload->vgpu->gvt->dev_priv;
	struct intel_engine_cs *engine = dev_priv->engine[ring_id];
	struct intel_vgpu *vgpu = workload->vgpu;
	struct vgpu_statistics *vgpu_stat = &vgpu->stat;
	int ret = 0;
	cycles_t t0, t1;

	gvt_dbg_sched("ring id %d prepare to dispatch workload %p\n",
		ring_id, workload);

	mutex_lock(&dev_priv->drm.struct_mutex);

	t0 = i915_get_cycles();
	ret = intel_gvt_scan_and_shadow_workload(workload);
	t1 = i915_get_cycles();

	if (i915.enable_conformance_check
			&& intel_gvt_vgpu_conformance_check(vgpu, ring_id))
		gvt_err("vgpu%d unconformance guest detected\n", vgpu->id);

	vgpu_stat->scan_shadow_wl_cycles[ring_id] += t1 - t0;
	if (ret)
		goto out;

	gen8_shadow_pid_cid(workload);

	if (workload->prepare) {
		mutex_unlock(&dev_priv->drm.struct_mutex);
		mutex_lock(&vgpu->gvt->lock);
		mutex_lock(&dev_priv->drm.struct_mutex);
		ret = workload->prepare(workload);
		mutex_unlock(&vgpu->gvt->lock);
		if (ret)
			goto out;
	}

	/* pin shadow context by gvt even the shadow context will be pinned
	 * when i915 alloc request. That is because gvt will update the guest
	 * context from shadow context when workload is completed, and at that
	 * moment, i915 may already unpined the shadow context to make the
	 * shadow_ctx pages invalid. So gvt need to pin itself. After update
	 * the guest context, gvt can unpin the shadow_ctx safely.
	 */
	ret = engine->context_pin(engine, shadow_ctx);
	if (ret) {
		gvt_vgpu_err("fail to pin shadow context\n");
		goto out;
	}

	workload->guilty_count = atomic_read(&workload->req->ctx->guilty_count);

	gvt_dbg_sched("ring id %d submit workload to i915 %p\n",
			ring_id, workload->req);

	i915_add_request_no_flush(workload->req);
	workload->dispatched = true;

out:
	if (ret)
		workload->status = ret;
	mutex_unlock(&dev_priv->drm.struct_mutex);
	return ret;
}

static struct intel_vgpu_workload *pick_next_workload(
		struct intel_gvt *gvt, int ring_id)
{
	struct intel_gvt_workload_scheduler *scheduler = &gvt->scheduler;
	struct intel_vgpu_workload *workload = NULL;
	struct gvt_statistics *gvt_stat = &gvt->stat;

	mutex_lock(&gvt->sched_lock);

	/*
	 * no current vgpu / will be scheduled out / no workload
	 * bail out
	 */
	if (!scheduler->current_vgpu[ring_id]) {
		gvt_dbg_sched("ring id %d stop - no current vgpu\n", ring_id);
		goto out;
	}

	if (scheduler->need_reschedule[ring_id]) {
		gvt_dbg_sched("ring id %d stop - will reschedule\n", ring_id);
		goto out;
	}

	if (list_empty(workload_q_head(scheduler->current_vgpu[ring_id], ring_id))) {
		gvt_dbg_sched("ring id %d stop - no available workload\n",
				ring_id);
		goto out;
	}

	/*
	 * still have current workload, maybe the workload disptacher
	 * fail to submit it for some reason, resubmit it.
	 */
	if (scheduler->current_workload[ring_id]) {
		workload = scheduler->current_workload[ring_id];
		gvt_dbg_sched("ring id %d still have current workload %p\n",
				ring_id, workload);
		goto out;
	}

	/*
	 * pick a workload as current workload
	 * once current workload is set, schedule policy routines
	 * will wait the current workload is finished when trying to
	 * schedule out a vgpu.
	 */
	scheduler->current_workload[ring_id] = container_of(
			workload_q_head(scheduler->current_vgpu[ring_id], ring_id)->next,
			struct intel_vgpu_workload, list);

	workload = scheduler->current_workload[ring_id];

	gvt_dbg_sched("ring id %d pick new workload %p\n", ring_id, workload);

	atomic_inc(&workload->vgpu->running_workload_num);
out:
	mutex_unlock(&gvt->sched_lock);
	if (workload)
		gvt_stat->pick_hit_cnt[ring_id]++;
	else
		gvt_stat->pick_miss_cnt[ring_id]++;
	return workload;
}

static void update_guest_context(struct intel_vgpu_workload *workload)
{
	struct intel_vgpu *vgpu = workload->vgpu;
	struct intel_gvt *gvt = vgpu->gvt;
	int ring_id = workload->ring_id;
	struct i915_gem_context *shadow_ctx = workload->vgpu->shadow_ctx;
	struct drm_i915_gem_object *ctx_obj =
		shadow_ctx->engine[ring_id].state->obj;
	struct execlist_ring_context *shadow_ring_context;
	struct page *page;
	void *src;
	unsigned long context_gpa, context_page_num;
	int i;

	gvt_dbg_sched("ring id %d workload lrca %x\n", ring_id,
			workload->ctx_desc.lrca);
	if (!enable_lazy_shadow_ctx) {
		context_page_num = intel_lr_context_size(
			gvt->dev_priv->engine[ring_id]);

		context_page_num = context_page_num >> PAGE_SHIFT;

		if (IS_BROADWELL(gvt->dev_priv) && ring_id == RCS)
			context_page_num = 19;

		i = 2;

		while (i < context_page_num) {
			context_gpa = intel_vgpu_gma_to_gpa(vgpu->gtt.ggtt_mm,
				(u32)((workload->ctx_desc.lrca + i) <<
					GTT_PAGE_SHIFT));
			if (context_gpa == INTEL_GVT_INVALID_ADDR) {
				gvt_vgpu_err("invalid guest context descriptor\n");
				return;
			}

			page = i915_gem_object_get_page(ctx_obj,
					LRC_PPHWSP_PN + i);
			src = kmap(page);
			intel_gvt_hypervisor_write_gpa(vgpu, context_gpa, src,
					GTT_PAGE_SIZE);
			kunmap(page);
			i++;
		}
	}
	intel_gvt_hypervisor_write_gpa(vgpu, workload->ring_context_gpa +
		RING_CTX_OFF(ring_header.val), &workload->rb_tail, 4);

	page = i915_gem_object_get_page(ctx_obj, LRC_STATE_PN);
	shadow_ring_context = kmap(page);

#define COPY_REG(name) \
	intel_gvt_hypervisor_write_gpa(vgpu, workload->ring_context_gpa + \
		RING_CTX_OFF(name.val), &shadow_ring_context->name.val, 4)

	COPY_REG(ctx_ctrl);
	COPY_REG(ctx_timestamp);

#undef COPY_REG

	intel_gvt_hypervisor_write_gpa(vgpu,
			workload->ring_context_gpa +
			sizeof(*shadow_ring_context),
			(void *)shadow_ring_context +
			sizeof(*shadow_ring_context),
			GTT_PAGE_SIZE - sizeof(*shadow_ring_context));

	kunmap(page);
}

/*
 * From seqno passed to context switch should be very quick, so setting 100ms
 * as the max wait timeout.
 */
#define MAX_WAIT_IN_US 10000

/*
 * The high DW of RING_EXECLIST_STATUS reflects the lrca of current running
 * context. If it's 0 or the lrca of another context, it means that gvt context
 * is switched out and safe to update to guest's context.
 */
static void wait_context_switch(struct intel_vgpu_workload *workload)
{
	struct drm_i915_private *dev_priv = workload->vgpu->gvt->dev_priv;
	struct intel_engine_cs *engine  = dev_priv->engine[workload->ring_id];
	i915_reg_t reg_hi = RING_EXECLIST_STATUS_HI(engine);
	u64 lrc_desc = intel_lr_context_descriptor(workload->vgpu->shadow_ctx,
			engine);
	unsigned fw =
		intel_uncore_forcewake_for_reg(dev_priv, reg_hi, FW_REG_READ);
	int ret = 0;
	static long max_wait_in_us = MAX_WAIT_IN_US;
	u64	startat = local_clock();

	lrc_desc >>= 12;
	lrc_desc &= 0xfffff;

	intel_uncore_forcewake_get(dev_priv, fw);

#define CS_DONE  ((I915_READ_FW(reg_hi) & 0xfffff) != lrc_desc)
	if (!CS_DONE)
		ret = _wait_for(CS_DONE, max_wait_in_us, 10);
#undef CS_DONE
	intel_uncore_forcewake_put(dev_priv, fw);

	/*
	 * This is a workaround by reading from the CTX_STATUS_PTR register,
	 * in order to avoid a system hang issue. So far we don't know why it
	 * can avoid the hang, but we're suspecting it's related to some context
	 * cache flush.
	 */
	POSTING_READ(RING_CONTEXT_STATUS_PTR(engine));
	if (ret) {
		printk(KERN_INFO "## %s[%d] :"
			"GVT context not completed after %ldms (%ld)!"
			"ret=%d\n",
				__func__, pthread_self(),
				max_wait_in_us / 1000,
				(unsigned long)((local_clock() - startat)
					/ NSEC_PER_MSEC),
				ret);
	//	if (max_wait_in_us < 100 * NSEC_PER_USEC) {
	//		max_wait_in_us += MAX_WAIT_IN_US;
	//	}

	}
}

static void complete_current_workload(struct intel_gvt *gvt, int ring_id)
{
	struct intel_gvt_workload_scheduler *scheduler = &gvt->scheduler;
	struct intel_vgpu_workload *workload;
	struct intel_vgpu *vgpu;
	int event;

	mutex_lock(&gvt->sched_lock);

	workload = scheduler->current_workload[ring_id];
	vgpu = workload->vgpu;

	/* For the workload w/ request, needs to wait for the context
	 * switch to make sure request is completed.
	 * For the workload w/o request, directly complete the workload.
	 */
	if (workload->req) {
		struct drm_i915_private *dev_priv =
			workload->vgpu->gvt->dev_priv;
		struct intel_engine_cs *engine =
			dev_priv->engine[workload->ring_id];
		wait_event(workload->shadow_ctx_status_wq,
			   !atomic_read(&workload->shadow_ctx_active));

		i915_gem_request_put(fetch_and_zero(&workload->req));

		if (!workload->status && !vgpu->resetting) {
			wait_context_switch(workload);
			update_guest_context(workload);

			mutex_lock(&gvt->lock);
			for_each_set_bit(event, workload->pending_events,
					 INTEL_GVT_EVENT_MAX)
				intel_vgpu_trigger_virtual_event(vgpu, event);
			mutex_unlock(&gvt->lock);
		}
		mutex_lock(&dev_priv->drm.struct_mutex);
		/* unpin shadow ctx as the shadow_ctx update is done */
		engine->context_unpin(engine, workload->vgpu->shadow_ctx);
		mutex_unlock(&dev_priv->drm.struct_mutex);
	}

	gvt_dbg_sched("ring id %d complete workload %p status %d\n",
			ring_id, workload, workload->status);

	scheduler->current_workload[ring_id] = NULL;

	mutex_lock(&gvt->lock);
	list_del_init(&workload->list);
	if (workload->status == -EIO)
		intel_vgpu_reset_execlist(vgpu, 1 << ring_id);

	workload->complete(workload);

	atomic_dec(&vgpu->running_workload_num);
	wake_up(&scheduler->workload_complete_wq);

	if (gvt->scheduler.need_reschedule[ring_id]) {
		set_bit(ring_id + INTEL_GVT_REQUEST_EVENT_SCHED_RING,
					(void *)&gvt->service_request);
		intel_gvt_request_service(gvt, INTEL_GVT_REQUEST_EVENT_SCHED);
	}

	mutex_unlock(&gvt->lock);
	mutex_unlock(&gvt->sched_lock);
}

static void inject_error_cs_irq(struct intel_vgpu *vgpu, int ring_id)
{
	enum intel_gvt_event_type events[] = {
		RCS_CMD_STREAMER_ERR,
		BCS_CMD_STREAMER_ERR,
		VCS_CMD_STREAMER_ERR,
		VCS2_CMD_STREAMER_ERR,
		VECS_CMD_STREAMER_ERR,
	};
	intel_vgpu_trigger_virtual_event(vgpu, events[ring_id]);
}

struct workload_thread_param {
	struct intel_gvt *gvt;
	int ring_id;
};

static int workload_thread(void *priv)
{
	struct workload_thread_param *p = (struct workload_thread_param *)priv;
	struct intel_gvt *gvt = p->gvt;
	int ring_id = p->ring_id;
	struct intel_gvt_workload_scheduler *scheduler = &gvt->scheduler;
	struct intel_vgpu_workload *workload = NULL;
	struct intel_vgpu *vgpu = NULL;
	struct vgpu_statistics *vgpu_stat;
	struct gvt_statistics *gvt_stat = &gvt->stat;
	int ret;
	long lret;
	cycles_t t[14];
	bool need_force_wake = IS_SKYLAKE(gvt->dev_priv) || IS_BROXTON(gvt->dev_priv);
	DEFINE_WAIT_FUNC(wait, woken_wake_function);

	kfree(p);

#ifdef __QNXNTO__
	if (i915.gvt_priority > 0 &&
		current->sched_max_priority > i915.gvt_priority) {

		struct task_struct *task = current;
		pthread_t tid = pthread_self();
		/*
		 * set priority, balance for graphics on QVM
		 * for more detail, see qnx_create_task_sched
		 */
		pthread_setschedprio(tid, i915.gvt_priority);
		pthread_getschedparam(tid,
				&task->sched_policy,
				&task->sched_param);	/* update param */
	}
#endif
	gvt_dbg_core("workload thread for ring %d started\n", ring_id);

	while (!kthread_should_stop()) {
		t[0] = i915_get_cycles();
		add_wait_queue(&scheduler->waitq[ring_id], &wait);
		do {
			cycles_t t0, t1;
			workload = pick_next_workload(gvt, ring_id);
			if (workload)
				break;
			t0 = i915_get_cycles();
			wait_woken(&wait, TASK_INTERRUPTIBLE,
				   MAX_SCHEDULE_TIMEOUT);
			t1 = i915_get_cycles();
			gvt_stat->wait_workload_cycles[ring_id] += t1 - t0;

		} while (!kthread_should_stop());
		remove_wait_queue(&scheduler->waitq[ring_id], &wait);

		if (!workload)
			break;

		gvt_dbg_sched("ring id %d next workload %p vgpu %d\n",
				workload->ring_id, workload,
				workload->vgpu->id);

		t[1] = i915_get_cycles();
		intel_runtime_pm_get(gvt->dev_priv);

		gvt_dbg_sched("ring id %d will dispatch workload %p\n",
				workload->ring_id, workload);

		if (need_force_wake)
			intel_uncore_forcewake_get(gvt->dev_priv,
					FORCEWAKE_ALL);

		t[3] = i915_get_cycles();
		mutex_lock(&gvt->sched_lock);
		t[4] = i915_get_cycles();
		ret = dispatch_workload(workload);
		t[5] = i915_get_cycles();
		mutex_unlock(&gvt->sched_lock);
		t[6] = i915_get_cycles();

		vgpu_stat = &workload->vgpu->stat;
		workload->perf.queue_out_time = t[1];
		vgpu_stat->workload_submit_cycles[ring_id] +=
						workload->perf.queue_in_time -
						workload->perf.submit_time;
		vgpu_stat->workload_queue_in_out_cycles[ring_id] +=
						workload->perf.queue_out_time -
						workload->perf.queue_in_time;

		vgpu_stat->pick_workload_cycles[ring_id] += t[1] - t[0];
		vgpu_stat->schedule_misc_cycles[ring_id] += t[3] - t[1];
		vgpu_stat->dispatch_lock_cycles[ring_id] += t[6] - t[3];
		vgpu_stat->dispatch_cycles[ring_id] += t[5] - t[4];

		if (ret) {
			vgpu = workload->vgpu;
			gvt_vgpu_err("fail to dispatch workload, skip\n");
			goto complete;
		}

		t[8] = i915_get_cycles();
		gvt_dbg_sched("ring id %d wait workload %p\n",
				workload->ring_id, workload);
		lret = i915_wait_request(workload->req, 0,
				MAX_SCHEDULE_TIMEOUT);

		t[9] = i915_get_cycles();
		vgpu_stat->wait_complete_cycles[ring_id] += t[9] - t[8];
		gvt_dbg_sched("i915_wait_request %p returns %ld\n",
				workload, lret);
		if (lret >= 0 && workload->status == -EINPROGRESS)
			workload->status = 0;

		/*
		 * increased guilty_count means that this request triggerred
		 * a GPU reset, so we need to notify the guest about the
		 * hang.
		 */
		if (workload->guilty_count <
				atomic_read(&workload->req->ctx->guilty_count)) {
			workload->status = -EIO;
			inject_error_cs_irq(workload->vgpu, ring_id);
		}

complete:
		gvt_dbg_sched("will complete workload %p, status: %d\n",
				workload, workload->status);

		t[10] = i915_get_cycles();
		complete_current_workload(gvt, ring_id);
		t[11] = i915_get_cycles();

		if (need_force_wake)
			intel_uncore_forcewake_put(gvt->dev_priv,
					FORCEWAKE_ALL);

		intel_runtime_pm_put(gvt->dev_priv);
		t[13] = i915_get_cycles();

		vgpu_stat->after_complete_cycles[ring_id] += t[11] - t[10];
		vgpu_stat->schedule_misc_cycles[ring_id] += t[13] - t[11];
	}
	return 0;
}

void intel_gvt_wait_vgpu_idle(struct intel_vgpu *vgpu)
{
	struct intel_gvt *gvt = vgpu->gvt;
	struct intel_gvt_workload_scheduler *scheduler = &gvt->scheduler;

	if (atomic_read(&vgpu->running_workload_num)) {
		gvt_dbg_sched("wait vgpu idle\n");

		wait_event(scheduler->workload_complete_wq,
				!atomic_read(&vgpu->running_workload_num));
	}
}

void intel_gvt_clean_workload_scheduler(struct intel_gvt *gvt)
{
	struct intel_gvt_workload_scheduler *scheduler = &gvt->scheduler;
	struct intel_engine_cs *engine;
	enum intel_engine_id i;

	gvt_dbg_core("clean workload scheduler\n");

	for_each_engine(engine, gvt->dev_priv, i) {
		kthread_stop(scheduler->thread[i]);
	}
}

int intel_gvt_init_workload_scheduler(struct intel_gvt *gvt)
{
	struct intel_gvt_workload_scheduler *scheduler = &gvt->scheduler;
	struct workload_thread_param *param = NULL;
	struct intel_engine_cs *engine;
	enum intel_engine_id i;
	int ret;

	gvt_dbg_core("init workload scheduler\n");

	init_waitqueue_head(&scheduler->workload_complete_wq);

	for_each_engine(engine, gvt->dev_priv, i) {
		init_waitqueue_head(&scheduler->waitq[i]);

		param = kzalloc(sizeof(*param), GFP_KERNEL);
		if (!param) {
			ret = -ENOMEM;
			goto err;
		}

		param->gvt = gvt;
		param->ring_id = i;

		scheduler->thread[i] = kthread_run(workload_thread, param,
			"gvt workload %d", i);
		if (IS_ERR(scheduler->thread[i])) {
			gvt_err("fail to create workload thread\n");
			ret = PTR_ERR(scheduler->thread[i]);
			goto err;
		}

	}
	return 0;
err:
	intel_gvt_clean_workload_scheduler(gvt);
	kfree(param);
	param = NULL;
	return ret;
}

void intel_vgpu_clean_gvt_context(struct intel_vgpu *vgpu)
{
	i915_gem_context_put_unlocked(vgpu->shadow_ctx);
}

int intel_vgpu_init_gvt_context(struct intel_vgpu *vgpu)
{
	atomic_set(&vgpu->running_workload_num, 0);

	vgpu->shadow_ctx = i915_gem_context_create_gvt(
			&vgpu->gvt->dev_priv->drm);
	if (IS_ERR(vgpu->shadow_ctx))
		return PTR_ERR(vgpu->shadow_ctx);

	if (!vgpu->shadow_ctx->name) {
		vgpu->shadow_ctx->name = kasprintf(GFP_KERNEL, "Shadow Context %d", vgpu->id);
	}

	vgpu->shadow_ctx->engine[RCS].initialised = true;

	bitmap_zero(vgpu->shadow_ctx_desc_updated, I915_NUM_ENGINES);

	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/gvt/scheduler.c $ $Rev: 852681 $")
#endif
