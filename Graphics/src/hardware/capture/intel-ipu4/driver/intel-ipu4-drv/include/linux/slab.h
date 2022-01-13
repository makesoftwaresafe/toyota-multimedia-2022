/*
 * Written by Mark Hemment, 1996 (markhe@nextd.demon.co.uk).
 *
 * (C) SGI 2006, Christoph Lameter
 *  Cleaned up and restructured to ease the addition of alternative
 *  implementations of SLAB allocators.
 * (C) Linux Foundation 2008-2013
 * Some modifications Copyright (c) 2017 QNX Software Systems.
 *      Unified interface for all slab allocators
 */

#ifndef _LINUX_SLAB_H
#define _LINUX_SLAB_H

#include <assert.h>
#include <linux/list.h>

#ifdef __QNXNTO__
#include <stdlib.h>
#include <string.h>
#include <linux/compiler.h>
#endif /* __QNXNTO__ */

#define ZERO_SIZE_PTR ((void *)16)
#define ZERO_OR_NULL_PTR(x) ((unsigned long)(x) <= (unsigned long)ZERO_SIZE_PTR)

/*TODO, put it to page*.h */
#define SetPageReserved(page)    (void)(page)
#define ClearPageReserved(page)  (void)(page)


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



static inline void *kmalloc(size_t size, gfp_t flags) {
	void *mem = malloc(size);
	if (flags & __GFP_ZERO) {
		memset(mem, 0, size);
	}
    return mem;
}

static inline void *kzalloc(size_t size, gfp_t flags) {
	return kmalloc(size, flags | __GFP_ZERO);
}

static inline void *kcalloc(size_t nsize, size_t size, gfp_t flags) {
	return calloc(nsize, size);
}

static inline void kfree(void *addr) {
     free(addr);
}

static inline void *krealloc(void *p, size_t new_size, gfp_t flags)
{
	void *ret;

	if (unlikely(!p))
		return kmalloc(new_size, flags);

	if (unlikely(!new_size)) {
		kfree(p);
		return NULL;
	}

	ret = realloc(p, new_size); 
	return ret;
}

struct kmem_cache {
	const char * name;
	unsigned int obj_size;
	unsigned int size;	/* The aligned/padded/added on size  */
	unsigned int align;	/* Alignment as calculated */
	unsigned long flags;	/* Active flags on the slab */
	int refcount;		/* Use counter */
	void (*ctor)(void *);	/* Called on object slot creation */
	struct list_head list;	/* List of all slab caches on the system */

};


static inline int
kmem_cache_shrink(struct kmem_cache *slab){
	//TODO.
	assert (0 && "kmem_cache_shrink no imp yet!");
	return 0;
}
static inline void
kmem_cache_free(struct kmem_cache * slab, void *p){
	(void)slab;
	assert(p);
	free(p);
}

static inline void *
kmem_cache_zalloc(struct kmem_cache *slab, gfp_t flags)
{
	(void)flags;
	void * ptr = calloc(1, slab->obj_size);
	if(ptr && slab->ctor){
		slab->ctor(ptr);
	}
	return ptr;
}


static inline struct kmem_cache *
kmem_cache_create(const char *name, size_t obj_size, size_t align,
				  unsigned long flags,
				  void (*ctor)(void *)){
	struct kmem_cache * slab;
	slab = calloc(1, sizeof(*slab));
	if(!slab){
		//TODO.
		return 0;
	}
	*slab = (struct kmem_cache){
		.name = name,
		.obj_size = obj_size,
		.size = 0,
		.align = align,
		.flags = flags,
		.ctor = ctor,
		.refcount = 1,
	};
	return slab;
}

static inline void
kmem_cache_destroy(struct kmem_cache *slab){
	assert(slab);
	free(slab);
}

static inline void*
kmem_cache_alloc(struct kmem_cache *cachep, gfp_t flags)
{
	assert(0 && "kmem_cache_alloc Stub is called!\n");
	return NULL;
}

#endif  /* _LINUX_SLAB_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/slab.h $ $Rev: 838597 $")
#endif
