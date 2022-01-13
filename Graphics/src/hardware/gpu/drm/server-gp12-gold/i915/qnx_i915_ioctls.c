#include <linux/qnx.h>
#include <linux/linux.h>

#include <drm/drmP.h>
#include <drm/i915_drm.h>
#include "i915_drv.h"
#include "i915_trace.h"
#include "intel_drv.h"

void* ioremap_wc_peer(pid_t pid, off64_t off, size_t size);

int i915_gem_mmap_qnx(struct drm_file *file, struct drm_device *dev, uint32_t handle,
	uint64_t size, uint64_t *vir_addr)
{
	int ret = 0;
	struct drm_i915_private *dev_priv = dev->dev_private;
	struct drm_i915_gem_object *obj;
	struct i915_vma *vma;

	obj = to_intel_bo(drm_gem_object_lookup(file, handle));
	if (!obj || &obj->base == NULL) {
		ret = -ENOENT;
		goto err;
	}
	drm_gem_object_unreference_unlocked(&obj->base);

	/* Try to flush the object off the GPU first without holding the lock.
	 * Upon acquiring the lock, we will perform our sanity checks and then
	 * repeat the flush holding the lock in the normal manner to catch cases
	 * where we are gazumped.
	 */
	ret = i915_gem_object_wait(obj, I915_WAIT_INTERRUPTIBLE, MAX_SCHEDULE_TIMEOUT, NULL);
	if (ret) {
		goto err;
	}

	ret = i915_gem_object_pin_pages(obj);
	if (ret) {
		goto err;
	}

	intel_runtime_pm_get(dev_priv);

	ret = i915_mutex_lock_interruptible(dev);
	if (ret) {
		goto err_rpm;
	}

	/* Access to snoopable pages through the GTT is incoherent. */
	if (obj->cache_level != I915_CACHE_NONE && !HAS_LLC(dev_priv)) {
		ret = -EINVAL;
		goto err_unlock;
	}

	vma = i915_gem_object_ggtt_pin(obj, NULL, 0, 0, PIN_MAPPABLE | PIN_NONFAULT);
	if (IS_ERR(vma)) {
		ret = PTR_ERR(vma);
		qnx_error("i915_gem_obj_ggtt_pin failed, ret=%d", ret);
		goto err_unlock;
	}

	ret = i915_gem_object_set_to_gtt_domain(obj, true);
	if (ret) {
		qnx_error("i915_gem_object_set_to_gtt_domain failed, ret=%d", ret);
		goto err_unpin;
	}

	ret = i915_vma_get_fence(vma);
	if (ret) {
		goto err_unpin;
	}

	/* Mark as being mmapped into userspace for later revocation */
	assert_rpm_wakelock_held(dev_priv);
	if (list_empty(&obj->userfault_link)) {
		list_add(&obj->userfault_link, &dev_priv->mm.userfault_list);
	}

	*vir_addr = (uint64_t)(uintptr_t)ioremap_wc_peer(file->pid->pid, dev_priv->ggtt.mappable.base + vma->node.start, (size_t)size);
	if (*vir_addr == (uint64_t)(uintptr_t)NULL) {
		qnx_error("ioremap_wc_peer failed");
		ret = -ENOMEM;
		goto err_unpin;
	}

err_unpin:
	__i915_vma_unpin(vma);
err_unlock:
	mutex_unlock(&dev->struct_mutex);
err_rpm:
	intel_runtime_pm_put(dev_priv);
	i915_gem_object_unpin_pages(obj);
err:

	return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/qnx_i915_ioctls.c $ $Rev: 847774 $")
#endif
