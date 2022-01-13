/*
 * Copyright © 2008-2015 Intel Corporation
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
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

#include <linux/oom.h>
#include <linux/shmem_fs.h>
#include <linux/slab.h>
#include <linux/swap.h>
#include <linux/pci.h>
#include <linux/dma-buf.h>
#include <drm/drmP.h>
#include <drm/i915_drm.h>

#include "i915_drv.h"
#include "i915_trace.h"

static bool i915_gem_shrinker_lock(struct drm_device *dev, bool *unlock)
{
	switch (mutex_trylock_recursive(&dev->struct_mutex)) {
        case MUTEX_TRYLOCK_FAILED:
                return false;

        case MUTEX_TRYLOCK_SUCCESS:
                *unlock = true;
                return true;

        case MUTEX_TRYLOCK_RECURSIVE:
                *unlock = false;
                return true;
        }

        BUG();
}

static void i915_gem_shrinker_unlock(struct drm_device *dev, bool unlock)
{
	if (!unlock)
		return;

	mutex_unlock(&dev->struct_mutex);

	/* expedite the RCU grace period to free some request slabs */
	synchronize_rcu_expedited();
}

static bool any_vma_pinned(struct drm_i915_gem_object *obj)
{
	struct i915_vma *vma;

	list_for_each_entry(vma, &obj->vma_list, obj_link)
		if (i915_vma_is_pinned(vma))
			return true;

	return false;
}

static bool swap_available(void)
{
	return get_nr_swap_pages() > 0;
}

static bool can_release_pages(struct drm_i915_gem_object *obj)
{
	if (!obj->mm.pages)
		return false;

	/* Consider only shrinkable ojects. */
	if (!i915_gem_object_is_shrinkable(obj))
		return false;

	/* Only report true if by unbinding the object and putting its pages
	 * we can actually make forward progress towards freeing physical
	 * pages.
	 *
	 * If the pages are pinned for any other reason than being bound
	 * to the GPU, simply unbinding from the GPU is not going to succeed
	 * in releasing our pin count on the pages themselves.
	 */
	if (atomic_read(&obj->mm.pages_pin_count) > obj->bind_count)
		return false;

	if (any_vma_pinned(obj))
		return false;

	/* We can only return physical pages to the system if we can either
	 * discard the contents (because the user has marked them as being
	 * purgeable) or if we can move their contents out to swap.
	 */
	return swap_available() || obj->mm.madv == I915_MADV_DONTNEED;
}

static bool unsafe_drop_pages(struct drm_i915_gem_object *obj)
{
	if (i915_gem_object_unbind(obj) == 0)
		__i915_gem_object_put_pages(obj, I915_MM_SHRINKER);
	return !READ_ONCE(obj->mm.pages);
}

/**
 * i915_gem_shrink - Shrink buffer object caches
 * @dev_priv: i915 device
 * @target: amount of memory to make available, in pages
 * @flags: control flags for selecting cache types
 *
 * This function is the main interface to the shrinker. It will try to release
 * up to @target pages of main memory backing storage from buffer objects.
 * Selection of the specific caches can be done with @flags. This is e.g. useful
 * when purgeable objects should be removed from caches preferentially.
 *
 * Note that it's not guaranteed that released amount is actually available as
 * free system memory - the pages might still be in-used to due to other reasons
 * (like cpu mmaps) or the mm core has reused them before we could grab them.
 * Therefore code that needs to explicitly shrink buffer objects caches (e.g. to
 * avoid deadlocks in memory reclaim) must fall back to i915_gem_shrink_all().
 *
 * Also note that any kind of pinning (both per-vma address space pins and
 * backing storage pins at the buffer object level) result in the shrinker code
 * having to skip the object.
 *
 * Returns:
 * The number of pages of backing storage actually released.
 */
unsigned long
i915_gem_shrink(struct drm_i915_private *dev_priv,
		unsigned long target, unsigned flags)
{
	const struct {
		struct list_head *list;
		unsigned int bit;
	} phases[] = {
		{ &dev_priv->mm.unbound_list, I915_SHRINK_UNBOUND },
		{ &dev_priv->mm.bound_list, I915_SHRINK_BOUND },
		{ NULL, 0 },
	}, *phase;
	unsigned long count = 0;
	bool unlock;

	if (!i915_gem_shrinker_lock(&dev_priv->drm, &unlock))
		return 0;

	trace_i915_gem_shrink(dev_priv, target, flags);
	i915_gem_retire_requests(dev_priv);

	/*
	 * Unbinding of objects will require HW access; Let us not wake the
	 * device just to recover a little memory. If absolutely necessary,
	 * we will force the wake during oom-notifier.
	 */
	if ((flags & I915_SHRINK_BOUND) &&
	    !intel_runtime_pm_get_if_in_use(dev_priv))
		flags &= ~I915_SHRINK_BOUND;

	/*
	 * As we may completely rewrite the (un)bound list whilst unbinding
	 * (due to retiring requests) we have to strictly process only
	 * one element of the list at the time, and recheck the list
	 * on every iteration.
	 *
	 * In particular, we must hold a reference whilst removing the
	 * object as we may end up waiting for and/or retiring the objects.
	 * This might release the final reference (held by the active list)
	 * and result in the object being freed from under us. This is
	 * similar to the precautions the eviction code must take whilst
	 * removing objects.
	 *
	 * Also note that although these lists do not hold a reference to
	 * the object we can safely grab one here: The final object
	 * unreferencing and the bound_list are both protected by the
	 * dev->struct_mutex and so we won't ever be able to observe an
	 * object on the bound_list with a reference count equals 0.
	 */
	for (phase = phases; phase->list; phase++) {
		struct list_head still_in_list;
		struct drm_i915_gem_object *obj;

		if ((flags & phase->bit) == 0)
			continue;

		INIT_LIST_HEAD(&still_in_list);
		while (count < target &&
		       (obj = list_first_entry_or_null(phase->list,
						       typeof(*obj),
						       global_link))) {
			list_move_tail(&obj->global_link, &still_in_list);
			if (!obj->mm.pages) {
				list_del_init(&obj->global_link);
				continue;
			}

			if (flags & I915_SHRINK_PURGEABLE &&
			    obj->mm.madv != I915_MADV_DONTNEED)
				continue;

			if (!(flags & I915_SHRINK_ACTIVE) &&
			    (i915_gem_object_is_active(obj) ||
			     obj->framebuffer_references))
				continue;

			if (!can_release_pages(obj))
				continue;

			if (unsafe_drop_pages(obj)) {
				/* May arrive from get_pages on another bo */
				mutex_lock_nested(&obj->mm.lock,
						  I915_MM_SHRINKER);
				if (!obj->mm.pages) {
					__i915_gem_object_invalidate(obj);
					list_del_init(&obj->global_link);
					count += obj->base.size >> PAGE_SHIFT;
				}
				mutex_unlock(&obj->mm.lock);
			}
		}
		list_splice_tail(&still_in_list, phase->list);
	}

	if (flags & I915_SHRINK_BOUND)
		intel_runtime_pm_put(dev_priv);

	i915_gem_retire_requests(dev_priv);

	i915_gem_shrinker_unlock(&dev_priv->drm, unlock);

	return count;
}

/**
 * i915_gem_shrink_all - Shrink buffer object caches completely
 * @dev_priv: i915 device
 *
 * This is a simple wraper around i915_gem_shrink() to aggressively shrink all
 * caches completely. It also first waits for and retires all outstanding
 * requests to also be able to release backing storage for active objects.
 *
 * This should only be used in code to intentionally quiescent the gpu or as a
 * last-ditch effort when memory seems to have run out.
 *
 * Returns:
 * The number of pages of backing storage actually released.
 */
unsigned long i915_gem_shrink_all(struct drm_i915_private *dev_priv)
{
	unsigned long freed;

	freed = i915_gem_shrink(dev_priv, -1UL,
				I915_SHRINK_BOUND |
				I915_SHRINK_UNBOUND |
				I915_SHRINK_ACTIVE);
	synchronize_rcu(); /* wait for our earlier RCU delayed slab frees */

	return freed;
}

static unsigned long
i915_gem_shrinker_count(struct shrinker *shrinker, struct shrink_control *sc)
{
	struct drm_i915_private *dev_priv =
		container_of(shrinker, struct drm_i915_private, mm.shrinker);
	struct drm_device *dev = &dev_priv->drm;
	struct drm_i915_gem_object *obj;
	unsigned long count;
	bool unlock;

	if (!i915_gem_shrinker_lock(dev, &unlock))
		return 0;

	i915_gem_retire_requests(dev_priv);

	count = 0;
	list_for_each_entry(obj, &dev_priv->mm.unbound_list, global_link)
		if (can_release_pages(obj))
			count += obj->base.size >> PAGE_SHIFT;

	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_link) {
		if (!i915_gem_object_is_active(obj) && can_release_pages(obj))
			count += obj->base.size >> PAGE_SHIFT;
	}

	i915_gem_shrinker_unlock(dev, unlock);

	return count;
}

static unsigned long
i915_gem_shrinker_scan(struct shrinker *shrinker, struct shrink_control *sc)
{
	struct drm_i915_private *dev_priv =
		container_of(shrinker, struct drm_i915_private, mm.shrinker);
	struct drm_device *dev = &dev_priv->drm;
	unsigned long freed;
	bool unlock;

	if (!i915_gem_shrinker_lock(dev, &unlock))
		return SHRINK_STOP;

	freed = i915_gem_shrink(dev_priv,
				sc->nr_to_scan,
				I915_SHRINK_BOUND |
				I915_SHRINK_UNBOUND |
				I915_SHRINK_PURGEABLE);
	if (freed < sc->nr_to_scan)
		freed += i915_gem_shrink(dev_priv,
					 sc->nr_to_scan - freed,
					 I915_SHRINK_BOUND |
					 I915_SHRINK_UNBOUND);

	i915_gem_shrinker_unlock(dev, unlock);

	return freed;
}

struct shrinker_lock_uninterruptible {
	bool was_interruptible;
	bool unlock;
};

static bool
i915_gem_shrinker_lock_uninterruptible(struct drm_i915_private *dev_priv,
				       struct shrinker_lock_uninterruptible *slu,
				       int timeout_ms)
{
	unsigned long timeout = jiffies + msecs_to_jiffies_timeout(timeout_ms);

	do {
		if (i915_gem_wait_for_idle(dev_priv, 0) == 0 &&
		    i915_gem_shrinker_lock(&dev_priv->drm, &slu->unlock))
			break;

		schedule_timeout_killable(1);
		if (fatal_signal_pending(current))
			return false;

		if (time_after(jiffies, timeout)) {
			pr_err("Unable to lock GPU to purge memory.\n");
			return false;
		}
	} while (1);

	slu->was_interruptible = dev_priv->mm.interruptible;
	dev_priv->mm.interruptible = false;
	return true;
}

static void
i915_gem_shrinker_unlock_uninterruptible(struct drm_i915_private *dev_priv,
					 struct shrinker_lock_uninterruptible *slu)
{
	dev_priv->mm.interruptible = slu->was_interruptible;
	i915_gem_shrinker_unlock(&dev_priv->drm, slu->unlock);
}

static int
i915_gem_shrinker_oom(struct notifier_block *nb, unsigned long event, void *ptr)
{
	struct drm_i915_private *dev_priv =
		container_of(nb, struct drm_i915_private, mm.oom_notifier);
	struct shrinker_lock_uninterruptible slu;
	struct drm_i915_gem_object *obj;
	unsigned long unevictable, bound, unbound, freed_pages;

	if (!i915_gem_shrinker_lock_uninterruptible(dev_priv, &slu, 5000))
		return NOTIFY_DONE;

	intel_runtime_pm_get(dev_priv);
	freed_pages = i915_gem_shrink_all(dev_priv);
	intel_runtime_pm_put(dev_priv);

	/* Because we may be allocating inside our own driver, we cannot
	 * assert that there are no objects with pinned pages that are not
	 * being pointed to by hardware.
	 */
	unbound = bound = unevictable = 0;
	list_for_each_entry(obj, &dev_priv->mm.unbound_list, global_link) {
		if (!obj->mm.pages)
			continue;

		if (!can_release_pages(obj))
			unevictable += obj->base.size >> PAGE_SHIFT;
		else
			unbound += obj->base.size >> PAGE_SHIFT;
	}
	list_for_each_entry(obj, &dev_priv->mm.bound_list, global_link) {
		if (!obj->mm.pages)
			continue;

		if (!can_release_pages(obj))
			unevictable += obj->base.size >> PAGE_SHIFT;
		else
			bound += obj->base.size >> PAGE_SHIFT;
	}

	i915_gem_shrinker_unlock_uninterruptible(dev_priv, &slu);

	if (freed_pages || unbound || bound)
		pr_info("Purging GPU memory, %lu pages freed, "
			"%lu pages still pinned.\n",
			freed_pages, unevictable);
	if (unbound || bound)
		pr_err("%lu and %lu pages still available in the "
		       "bound and unbound GPU page lists.\n",
		       bound, unbound);

	*(unsigned long *)ptr += freed_pages;
	return NOTIFY_DONE;
}

/**
 * i915_gem_shrinker_init - Initialize i915 shrinker
 * @dev_priv: i915 device
 *
 * This function registers and sets up the i915 shrinker and OOM handler.
 */
void i915_gem_shrinker_init(struct drm_i915_private *dev_priv)
{
	dev_priv->mm.shrinker.scan_objects = i915_gem_shrinker_scan;
	dev_priv->mm.shrinker.count_objects = i915_gem_shrinker_count;
	dev_priv->mm.shrinker.seeks = DEFAULT_SEEKS;
	WARN_ON(register_shrinker(&dev_priv->mm.shrinker));

	dev_priv->mm.oom_notifier.notifier_call = i915_gem_shrinker_oom;
	WARN_ON(register_oom_notifier(&dev_priv->mm.oom_notifier));
}

/**
 * i915_gem_shrinker_cleanup - Clean up i915 shrinker
 * @dev_priv: i915 device
 *
 * This function unregisters the i915 shrinker and OOM handler.
 */
void i915_gem_shrinker_cleanup(struct drm_i915_private *dev_priv)
{
	WARN_ON(unregister_oom_notifier(&dev_priv->mm.oom_notifier));
	unregister_shrinker(&dev_priv->mm.shrinker);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/i915_gem_shrinker.c $ $Rev: 836322 $")
#endif