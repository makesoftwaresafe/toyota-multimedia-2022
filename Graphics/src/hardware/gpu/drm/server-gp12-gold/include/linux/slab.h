#ifndef _QNX_LINUX_SLAB_H
#define _QNX_LINUX_SLAB_H

#include <assert.h>
#include <linux/list.h>
#include <linux/gfp.h>

#ifdef __QNXNTO__
#include <stdlib.h>
#include <string.h>
#include <linux/compiler.h>
#endif /* __QNXNTO__ */

#define ZERO_SIZE_PTR ((void *)16)
#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= (unsigned long)ZERO_SIZE_PTR)

/*
 * Flags to pass to kmem_cache_create().
 * The ones marked DEBUG are only valid if CONFIG_SLAB_DEBUG is set.
 */
#define SLAB_DEBUG_FREE		0x00000100UL	/* DEBUG: Perform (expensive) checks on free */
#define SLAB_RED_ZONE		0x00000400UL	/* DEBUG: Red zone objs in a cache */
#define SLAB_POISON		0x00000800UL	/* DEBUG: Poison objects */
#define SLAB_HWCACHE_ALIGN	0x00002000UL	/* Align objs on cache lines */
#define SLAB_CACHE_DMA		0x00004000UL	/* Use GFP_DMA memory */
#define SLAB_STORE_USER		0x00010000UL	/* DEBUG: Store the last owner for bug hunting */
#define SLAB_PANIC		0x00040000UL	/* Panic if kmem_cache_create() fails */
/*
 * SLAB_DESTROY_BY_RCU - **WARNING** READ THIS!
 *
 * This delays freeing the SLAB page by a grace period, it does _NOT_
 * delay object freeing. This means that if you do kmem_cache_free()
 * that memory location is free to be reused at any time. Thus it may
 * be possible to see another object there in the same RCU grace period.
 *
 * This feature only ensures the memory location backing the object
 * stays valid, the trick to using this is relying on an independent
 * object validation pass. Something like:
 *
 *  rcu_read_lock()
 * again:
 *  obj = lockless_lookup(key);
 *  if (obj) {
 *    if (!try_get_ref(obj)) // might fail for free objects
 *      goto again;
 *
 *    if (obj->key != key) { // not the object we expected
 *      put_ref(obj);
 *      goto again;
 *    }
 *  }
 *  rcu_read_unlock();
 *
 * This is useful if we need to approach a kernel structure obliquely,
 * from its address obtained without the usual locking. We can lock
 * the structure to stabilize it and check it's still at the given address,
 * only if we can be sure that the memory has not been meanwhile reused
 * for some other kind of object (which our subsystem's lock might corrupt).
 *
 * rcu_read_lock before reading the address, then rcu_read_unlock after
 * taking the spinlock within the structure expected at that address.
 */
#define SLAB_DESTROY_BY_RCU	0x00080000UL	/* Defer freeing slabs to RCU */
#define SLAB_MEM_SPREAD		0x00100000UL	/* Spread some memory over cpuset */
#define SLAB_TRACE		0x00200000UL	/* Trace allocations and frees */

void *kmalloc(size_t size, gfp_t flags);
void kfree(const void *addr);

static inline void *kzalloc(size_t size, gfp_t flags) {
	return kmalloc(size, flags | __GFP_ZERO);
}

static inline void *kcalloc(size_t nsize, size_t size, gfp_t flags) {
	return kmalloc(nsize * size, flags | __GFP_ZERO);
}

void *krealloc(void *p, size_t new_size, gfp_t flags);

/*
 * kmem_cache
 */
extern struct kmem_cache* kmem_cache_create(const char *name,
		size_t obj_size, size_t align,
		unsigned long flags, void (*ctor)(void*));
extern void kmem_cache_destroy(struct kmem_cache* slab);
extern void kmem_cache_free(struct kmem_cache* slab, void *p);
extern void* kmem_cache_alloc(struct kmem_cache *slab, gfp_t flags);

static inline int kmem_cache_shrink(struct kmem_cache *slab) {
	return 0;
}
static inline void* kmem_cache_zalloc(struct kmem_cache *slab, gfp_t flags)
{
	return kmem_cache_alloc(slab, flags | __GFP_ZERO);
}

/**
 * kmalloc_array - allocate memory for an array.
 * @n: number of elements.
 * @size: element size.
 * @flags: the type of memory to allocate (see kmalloc).
 */
static inline void *kmalloc_array(size_t n, size_t size, gfp_t flags)
{
	if (size != 0 && n > SIZE_MAX / size)
		return NULL;
	return kmalloc(n * size, flags);
}

/*
 * Please use this macro to create slab caches. Simply specify the
 * name of the structure and maybe some flags that are listed above.
 *
 * The alignment of the struct determines object alignment. If you
 * f.e. add ____cacheline_aligned_in_smp to the struct declaration
 * then the objects will be properly aligned in SMP configurations.
 */
#define KMEM_CACHE(__struct, __flags) kmem_cache_create(#__struct,	\
		sizeof(struct __struct), __alignof__(struct __struct),	\
		(__flags), NULL)

/* The following flags affect the page allocator grouping pages by mobility */
#define SLAB_RECLAIM_ACCOUNT	0x00020000UL		/* Objects are reclaimable */
#define SLAB_TEMPORARY		SLAB_RECLAIM_ACCOUNT	/* Objects are short-lived */

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/slab.h $ $Rev: 846985 $")
#endif
