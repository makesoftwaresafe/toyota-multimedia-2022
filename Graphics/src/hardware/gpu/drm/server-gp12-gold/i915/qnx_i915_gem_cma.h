struct file* shmem_file_setup_cma(const char *name, loff_t size, unsigned long flags);

static int
qnx_drm_gem_cma_object_init(struct drm_device *dev,
						struct drm_gem_object *obj, size_t size)
{
	BUG_ON((size & (PAGE_SIZE - 1)) != 0);

	obj->dev = dev;
	obj->filp = shmem_file_setup_cma("drm mm object", size, VM_NORESERVE);

	if (IS_ERR(obj->filp))
		return PTR_ERR(obj->filp);

	kref_init(&obj->refcount);
	obj->handle_count = 0;
	obj->size = size;

	return 0;
}

static struct drm_i915_gem_object *
qnx_i915_gem_cma_alloc_object(struct drm_device *dev,
						  size_t size)
{
	struct drm_i915_private *dev_priv = to_i915(dev);
	struct drm_i915_gem_object *obj;
	struct address_space *mapping;
	gfp_t mask;

	obj = i915_gem_object_alloc(dev_priv);
	if (obj == NULL)
		return NULL;

	if (qnx_drm_gem_cma_object_init(dev, &obj->base, size) != 0) {
		i915_gem_object_free(obj);
		return NULL;
	}

	mask = GFP_HIGHUSER | __GFP_RECLAIMABLE;
	if (IS_I965GM(dev_priv) || IS_I965G(dev_priv)) {
		/* 965gm cannot relocate objects above 4GiB. */
		mask &= ~__GFP_HIGHMEM;
		mask |= __GFP_DMA32;
	}

	mapping = file_inode(obj->base.filp)->i_mapping;
	mapping_set_gfp_mask(mapping, mask);

	i915_gem_object_init(obj, &i915_gem_object_ops);

	obj->base.write_domain = I915_GEM_DOMAIN_CPU;
	obj->base.read_domains = I915_GEM_DOMAIN_CPU;

	if (HAS_LLC(dev_priv)) {
		/* On some devices, we can have the GPU use the LLC (the CPU
		 * cache) for about a 10% performance improvement
		 * compared to uncached.  Graphics requests other than
		 * display scanout are coherent with the CPU in
		 * accessing this cache.  This means in this mode we
		 * don't need to clflush on the CPU side, and on the
		 * GPU side we only need to flush internal caches to
		 * get data visible to the CPU.
		 *
		 * However, we maintain the display planes as UC, and so
		 * need to rebind when first used as such.
		 */
		obj->cache_level = I915_CACHE_LLC;
	} else
		obj->cache_level = I915_CACHE_NONE;

	trace_i915_gem_object_create(obj);

	return obj;
}

int
qnx_i915_gem_cma_create(struct drm_file *file,
						struct drm_device *dev,
						uint64_t size,
						uint32_t *handle_p)
{
	struct drm_i915_gem_object *obj;
	int ret;
	u32 handle;

	size = roundup(size, PAGE_SIZE);
	if (size == 0)
		return -EINVAL;

	/* Allocate the new object */
	obj = qnx_i915_gem_cma_alloc_object(dev, (size_t)size);
	if (obj == NULL)
		return -ENOMEM;

	ret = drm_gem_handle_create(file, &obj->base, &handle);
	/* drop reference from allocate - handle holds it now */
	drm_gem_object_unreference_unlocked(&obj->base);
	if (ret)
		return ret;

	*handle_p = handle;
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/i915/qnx_i915_gem_cma.h $ $Rev: 836322 $")
#endif
