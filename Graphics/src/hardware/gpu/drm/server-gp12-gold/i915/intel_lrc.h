/*
 * Copyright © 2014 Intel Corporation
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
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef _INTEL_LRC_H_
#define _INTEL_LRC_H_

#include "intel_ringbuffer.h"

#define GEN8_LR_CONTEXT_ALIGN I915_GTT_MIN_ALIGNMENT

/* Execlists regs */
#define RING_ELSP(engine)			_MMIO((engine)->mmio_base + 0x230)
#define RING_EXECLIST_STATUS_LO(engine)		_MMIO((engine)->mmio_base + 0x234)
#define RING_EXECLIST_STATUS_HI(engine)		_MMIO((engine)->mmio_base + 0x234 + 4)
#define RING_CONTEXT_CONTROL(engine)		_MMIO((engine)->mmio_base + 0x244)
#define	  CTX_CTRL_INHIBIT_SYN_CTX_SWITCH	(1 << 3)
#define	  CTX_CTRL_ENGINE_CTX_RESTORE_INHIBIT	(1 << 0)
#define   CTX_CTRL_RS_CTX_ENABLE                (1 << 1)
#define RING_CONTEXT_STATUS_BUF_BASE(engine)	_MMIO((engine)->mmio_base + 0x370)
#define RING_CONTEXT_STATUS_BUF_LO(engine, i)	_MMIO((engine)->mmio_base + 0x370 + (i) * 8)
#define RING_CONTEXT_STATUS_BUF_HI(engine, i)	_MMIO((engine)->mmio_base + 0x370 + (i) * 8 + 4)
#define RING_CONTEXT_STATUS_PTR(engine)		_MMIO((engine)->mmio_base + 0x3a0)

/* The docs specify that the write pointer wraps around after 5h, "After status
 * is written out to the last available status QW at offset 5h, this pointer
 * wraps to 0."
 *
 * Therefore, one must infer than even though there are 3 bits available, 6 and
 * 7 appear to be * reserved.
 */
#define GEN8_CSB_ENTRIES 6
#define GEN8_CSB_PTR_MASK 0x7
#define GEN8_CSB_READ_PTR_MASK (GEN8_CSB_PTR_MASK << 8)
#define GEN8_CSB_WRITE_PTR_MASK (GEN8_CSB_PTR_MASK << 0)
#define GEN8_CSB_WRITE_PTR(csb_status) \
	(((csb_status) & GEN8_CSB_WRITE_PTR_MASK) >> 0)
#define GEN8_CSB_READ_PTR(csb_status) \
	(((csb_status) & GEN8_CSB_READ_PTR_MASK) >> 8)

enum {
	INTEL_CONTEXT_SCHEDULE_IN = 0,
	INTEL_CONTEXT_SCHEDULE_OUT,
};

/* Logical Rings */
void intel_logical_ring_stop(struct intel_engine_cs *engine);
void intel_logical_ring_cleanup(struct intel_engine_cs *engine);
int logical_render_ring_init(struct intel_engine_cs *engine);
int logical_xcs_ring_init(struct intel_engine_cs *engine);

/* Logical Ring Contexts */

/* One extra page is added before LRC for GuC as shared data */
#define LRC_GUCSHR_PN	(0)
#define LRC_PPHWSP_PN	(LRC_GUCSHR_PN + 1)
#define LRC_STATE_PN	(LRC_PPHWSP_PN + 1)

struct drm_i915_private;
struct i915_gem_context;

uint32_t intel_lr_context_size(struct intel_engine_cs *engine);

void intel_lr_context_resume(struct drm_i915_private *dev_priv);
uint64_t intel_lr_context_descriptor(struct i915_gem_context *ctx,
				     struct intel_engine_cs *engine);

/* Execlists */
int intel_sanitize_enable_execlists(struct drm_i915_private *dev_priv,
				    int enable_execlists);
void intel_execlists_enable_submission(struct drm_i915_private *dev_priv);
bool intel_lr_preempt_postprocess(struct intel_engine_cs *engine);

/* forcepreempt */
#define FORCEPREEMPT_RESET_TIMEOUT	5
void intel_forcepreempt_read(struct drm_i915_private *dev_priv,
		unsigned long *mask, long *timeout, char **ctxname);
int intel_forcepreempt_write(struct drm_i915_private *dev_priv,
		unsigned long mask, long timeout, char *ctxname);
void intel_forcepreempt_start_reset(struct drm_i915_private *dev_priv,
		u32 engine_mask);
void intel_forcepreempt_finish_reset(struct intel_engine_cs *engine);

static inline int intel_forcepreempt_default_timeout(
		struct drm_i915_private *dev_priv)
{
	return i915.forcepreempt_timeout ?: -1;
}

int intel_forcepreempt_timeout(struct intel_engine_cs *engine,
		struct drm_i915_gem_request *active_request);

static inline bool is_forcepreempt_reset(struct intel_engine_cs *engine)
{
	return atomic_read(&engine->fpreset_timeout) !=
			intel_forcepreempt_default_timeout(engine->i915);
}


#endif /* _INTEL_LRC_H_ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/intel_lrc.h $ $Rev: 836322 $")
#endif