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
 */

#include "i915_drv.h"
#include "intel_gvt.h"
#include "gvt/gvt.h"

/**
 * DOC: Intel GVT-g host support
 *
 * Intel GVT-g is a graphics virtualization technology which shares the
 * GPU among multiple virtual machines on a time-sharing basis. Each
 * virtual machine is presented a virtual GPU (vGPU), which has equivalent
 * features as the underlying physical GPU (pGPU), so i915 driver can run
 * seamlessly in a virtual machine.
 *
 * To virtualize GPU resources GVT-g driver depends on hypervisor technology
 * e.g KVM/VFIO/mdev, Xen, etc. to provide resource access trapping capability
 * and be virtualized within GVT-g device module. More architectural design
 * doc is available on https://01.org/group/2230/documentation-list.
 */

static bool is_supported_device(struct drm_i915_private *dev_priv)
{
	if (IS_BROADWELL(dev_priv))
		return true;
	if (IS_SKYLAKE(dev_priv))
		return true;
	if (IS_BROXTON(dev_priv))
		return true;
	return false;
}

/**
 * intel_gvt_init - initialize GVT components
 * @dev_priv: drm i915 private data
 *
 * This function is called at the initialization stage to create a GVT device.
 *
 * Returns:
 * Zero on success, negative error code if failed.
 *
 */
int intel_gvt_init(struct drm_i915_private *dev_priv)
{
	int ret;

	if (!i915.enable_gvt) {
		DRM_DEBUG_DRIVER("GVT-g is disabled by kernel params\n");
		return 0;
	}

	if (intel_vgpu_active(dev_priv)) {
		DRM_DEBUG_DRIVER("GVT-g is disabled for guest\n");
		goto bail;
	}

	if (!is_supported_device(dev_priv)) {
		DRM_DEBUG_DRIVER("Unsupported device. GVT-g is disabled\n");
		goto bail;
	}

	if (!i915.enable_execlists) {
		DRM_INFO("GPU guest virtualisation [GVT-g] disabled due to disabled execlist submission [i915.enable_execlists module parameter]\n");
		goto bail;
	}

	/*
	 * We're not in host or fail to find a MPT module, disable GVT-g
	 */
	ret = intel_gvt_init_host();
	if (ret) {
		DRM_DEBUG_DRIVER("Not in host or MPT modules not found\n");
		goto bail;
	}

	ret = intel_gvt_init_device(dev_priv);
	if (ret) {
		DRM_DEBUG_DRIVER("Fail to init GVT device\n");
		goto bail;
	}

	return 0;

bail:
	i915.enable_gvt = 0;
	return 0;
}

/**
 * intel_gvt_cleanup - cleanup GVT components when i915 driver is unloading
 * @dev_priv: drm i915 private *
 *
 * This function is called at the i915 driver unloading stage, to shutdown
 * GVT components and release the related resources.
 */
void intel_gvt_cleanup(struct drm_i915_private *dev_priv)
{
	if (!intel_gvt_active(dev_priv))
		return;

	intel_gvt_clean_device(dev_priv);
}

void update_vgpu_idle_state(int engine_id,
			struct drm_i915_gem_request *rq,
			bool is_schedule_in)
{
	cycles_t cur = get_cycles();
	struct engine_idle_state *vgpu_eng_is;

	if (!rq || rq->vgpu_id == 0)
		return;

	if (rq->vgpu_id < GVT_STATE_GPUS &&
		engine_id < GVT_STATE_ENGINES) {
		vgpu_eng_is = &gvt_state.vm[rq->vgpu_id].engines[engine_id];

		if (is_schedule_in) {
			if (atomic_read(&vgpu_eng_is->is_in_idle)) {
				vgpu_eng_is->submit_count++;
				vgpu_eng_is->submit_unfinished++;
				atomic_set(&vgpu_eng_is->is_in_idle, false);
			}
			vgpu_eng_is->idle_total_time += cur -
				vgpu_eng_is->idle_start;
			vgpu_eng_is->idle_start = cur;
		} else {
			vgpu_eng_is->idle_start = cur;
			atomic_set(&vgpu_eng_is->is_in_idle, true);
			vgpu_eng_is->submit_unfinished--;
			WARN_ON_ONCE(vgpu_eng_is->submit_unfinished != 0);
		}
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/intel_gvt.c $ $Rev: 841111 $")
#endif
