#include <pthread.h>
#include <sys/tree.h>
#include <sys/mman.h>
#include <linux/gfp.h>
#include <linux/slab.h>

#include "mmap_trace.h"

static pthread_mutex_t kmalloc_mutex = PTHREAD_MUTEX_INITIALIZER;

struct map_entry {
	RB_ENTRY(map_entry) links;
	void* vaddr;
	size_t size;
};

static RB_HEAD(kmalloc_map, map_entry) kmalloc_cache = RB_INITIALIZER(&kmalloc_cache);

static int vaddr_cmp(struct map_entry* e1, struct map_entry* e2)
{
	if (e1->vaddr < e2->vaddr) {
		return -1;
	} else if (e1->vaddr > e2->vaddr) {
		return 1;
	} else {
		return 0;
	}
}

RB_GENERATE(kmalloc_map, map_entry, links, vaddr_cmp);

int kmalloc_trace_dump()
{
    struct map_entry* ent;
    int count = 0;
    long size = 0;

    int rc = pthread_mutex_lock(&kmalloc_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock");
        return 0;
    }

    RB_FOREACH(ent, kmalloc_map, &kmalloc_cache) {
        count++;
        size += ent->size;
    }

    pthread_mutex_unlock(&kmalloc_mutex);

    fprintf(stderr, "<kmalloc_trace>: %d allocations (%ld bytes) in the tree\n", count, size);

    return 1;
}

static struct kmem_cache *map_entry_cache;
static struct map_entry *alloc_map_entry()
{
	pthread_mutex_lock(&kmalloc_mutex);
	if (!map_entry_cache) {
		map_entry_cache = kmem_cache_create("kmalloc-map-entry",
				sizeof(struct map_entry),
				0, 0, NULL);
	}
	pthread_mutex_unlock(&kmalloc_mutex);
	return kmem_cache_alloc(map_entry_cache, GFP_KERNEL | __GFP_ZERO);
}

static void free_map_entry(struct map_entry *me)
{
	kmem_cache_free(map_entry_cache, me);
}

/* From linux kernel comments: kmalloc allocates physically contiguous      */
/* memory, memory which pages are laid consecutively in physical RAM.       */
/* Also linux kernel has a special pool for block size less than page, this */
/* pool just works as usual calloc()/malloc().                              */

struct mem_header {
    s16 size;
    u16 caller;
    u16 id;
    u16 offset;
};

static inline struct mem_header *get_mem_header(void *addr)
{
    if ((uintptr_t)addr & (__PAGESIZE - 1)) {
        struct mem_header   *h;

        h = addr;
        h--;
        if (h->size == -1) {
            h--;
        }
        return h;
    }
    return NULL;
}

#define KM_POOL_SIZE	32
#define KM_POOL_MIN	2
#define KM_POOL_LIMIT	(128 * 1024)
#define KM_ALIGN	8

static struct km_pool {
	size_t size;
	void *pool;
	int count;
	int access;
} km_pool[KM_POOL_SIZE];

void* kmalloc(size_t size, gfp_t flags)
{
    void* mem;
    struct map_entry* ent;
    int rc;

    if (size < __PAGESIZE) {
        struct mem_header   *h;
	void *p;
	size_t allocsz;
	unsigned long offset;

	allocsz = size + sizeof(*h) * 2 + KM_ALIGN;

        p = malloc(allocsz);
        if (!p) {
            return NULL;
        }
	offset = (unsigned long)p & (KM_ALIGN - 1);
	if (offset) {
		offset = KM_ALIGN - offset;
		p += offset;
		printk(KERN_INFO "##%s missing align offset=%ld\n",
				__func__, offset);
	}
	h = p;
        h->size = size;
        h->caller = caller_main_offset();
        h->id = 0xbeef;
	h->offset = (u16)offset;

        mem = h + 1;
        if (!( (uintptr_t)mem & (__PAGESIZE - 1))) {
            /*
             * unfortunately mem is page boundary
             */
            h++;
            h->size = -1;
            h->caller = 0xffff;
            h->id = 0xfeed;

            mem = h + 1;
        }

        if (flags & __GFP_ZERO) {
            memset(mem, 0, size);
        }
        return mem;
    } else do {
        /*
	 * try to get from pool
	 */
	int i;

	mem = NULL;
	pthread_mutex_lock(&kmalloc_mutex);
	for (i = 0; i < ARRAY_SIZE(km_pool); i++) {
		struct km_pool *pp = km_pool + i;

		if (!pp->size) {
			break;
		}
		if (size == pp->size && pp->count) {
			mem = pp->pool;
			pp->pool = *(void**)mem;
			pp->count--;
			break;
		}
	}
	pthread_mutex_unlock(&kmalloc_mutex);
	if (mem) break;

        size = ALIGN(size, __PAGESIZE);
        mem = mmap64(NULL, size, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE, NOFD, 0);
        if (mem == MAP_FAILED) {
            return NULL;
        }

        mmap_trace_add_range(mem, size);
    } while(0);

    ent = alloc_map_entry();
    if (!ent) {
        qnx_error("%s:alloc map_entry failed", __func__);
	mmap_trace_del_range(mem, size);
	rc = munmap(mem, size);
	if (rc) {
	    qnx_error("%s:munmap call failed", __func__);
	}
	return NULL;
    }

    rc = pthread_mutex_lock(&kmalloc_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock, error: %d", rc);
        if (rc == EDEADLK) {
            abort();
        }
        return NULL;
    }

    ent->vaddr = mem;
    ent->size = size;

    RB_INSERT(kmalloc_map, &kmalloc_cache, ent);

    pthread_mutex_unlock(&kmalloc_mutex);

    if (flags & __GFP_ZERO) {
        memset(mem, 0, size);
    }
    return mem;
}

/* From linux kernel comments: vmalloc allocates memory which is contiguous */
/* in kernel virtual memory space (that means pages allocated that way are  */
/* not contiguous in RAM, but the kernel sees them as one block).           */

void* vmalloc(unsigned long size)
{
    return kmalloc(size, ___GFP_MEMALLOC);
}

static bool kfree_cached(void *addr, size_t size)
{
	int	i;
	struct km_pool *pp;

	pp = NULL;
	for (i = 0; i < ARRAY_SIZE(km_pool); i++) {
		struct km_pool *t = km_pool + i;

		if (!t->size || t->size == size) {
			pp = t;
			break;
		}
	}
	if (!pp) return false;
	if (pp->count >= KM_POOL_MIN &&
		pp->size * (pp->count + 1) >= KM_POOL_LIMIT) {
		trace_logf(_NTO_TRACE_USERFIRST,
				"%s size=%#lx count=%d",
				__func__, pp->size, pp->count + 1);
		return false;
	}

	pp->size = size;
	if (!pp->access++) return false; /* not allow first access */
	pp->count++;
	*(void**)addr = pp->pool;
	pp->pool = addr;

	return true;
}

void kfree(const void* caddr)
{
    void *addr = (void*)caddr;
    struct map_entry key = {.vaddr = addr};
    struct map_entry* ent;
    size_t size = 0;
    struct mem_header   *mh;
    bool cached;

    if (addr == NULL) {
        return;
    }

    mh = get_mem_header(addr);
    if (mh) {
        void *p = mh;

        p -= mh->offset;
        free(p);
        return;
    }

    int rc = pthread_mutex_lock(&kmalloc_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock");
        return;
    }

    ent = RB_FIND(kmalloc_map, &kmalloc_cache, &key);

    if (ent == NULL) {
        qnx_error("Attempt to call kfree() for non-kmalloc'ed object: %p!", addr);
        pthread_mutex_unlock(&kmalloc_mutex);
        abort();
        return;
    }

    RB_REMOVE(kmalloc_map, &kmalloc_cache, ent);
    size = ent->size;
    cached = kfree_cached(addr, size);

    pthread_mutex_unlock(&kmalloc_mutex);
    free_map_entry(ent);

    if (cached) {
	    return;
    }

    if (size < __PAGESIZE) {
        printk(KERN_ERR "## %s conflict! addr=%p##\n", __func__, addr);
    } else {
        mmap_trace_del_range(addr, size);
        rc = munmap(addr, size);
        if (rc) {
            qnx_error("munmap() call failed!");
        }
    }
}

int kfree_check(void* addr)
{
    struct map_entry key = {.vaddr = addr};
    struct map_entry* ent;

    if (addr == NULL) {
        return 0;
    }

    int rc = pthread_mutex_lock(&kmalloc_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock");
        return 0;
    }

    ent = RB_FIND(kmalloc_map, &kmalloc_cache, &key);
    if (ent == NULL) {
        pthread_mutex_unlock(&kmalloc_mutex);
        return 0;
    }

    pthread_mutex_unlock(&kmalloc_mutex);

    return 1;
}

void *krealloc(void *p, size_t new_size, gfp_t flags)
{
    struct map_entry key = {.vaddr = p};
    struct map_entry* ent;
    struct mem_header   *mh;
    size_t old_size = 0;
    void* new_p;

    if (!p) {
        return kmalloc(new_size, flags);
    }

    if (!new_size) {
        kfree(p);
        return NULL;
    }

    mh = get_mem_header(p);
    if (mh) {
        old_size = mh->size;
    } else {
        int rc = pthread_mutex_lock(&kmalloc_mutex);
        if (rc) {
            qnx_error("pthread_mutex_lock");
            return NULL;
        }

        ent = RB_FIND(kmalloc_map, &kmalloc_cache, &key);

        if (ent == NULL) {
            qnx_error("Attempt to call krealloc() for non-kmalloc'ed object: %p!", p);
            pthread_mutex_unlock(&kmalloc_mutex);
            abort();
            return NULL;
        }

        old_size = ent->size;
        new_size = (new_size + (__PAGESIZE - 1)) & ~(__PAGESIZE - 1);

        pthread_mutex_unlock(&kmalloc_mutex);
    }

    /* shrink/grow inside one page */
    if (old_size == new_size) {
        return p;
    }

    new_p = kmalloc(new_size, flags);
    if (!new_p) {
        return NULL;
    }

    memcpy(new_p, p, new_size > old_size ? old_size : new_size);
    kfree(p);

    return new_p;
}

/*
 * kmem_cache
 * This is different approach from kmalloc to port into QNX:
 * - kmem_cache_alloc must take care of alignment, especially,
 *   SLAB_HWCACHE_ALIGN is derived from boot parameter.
 * - kmalloc in QNX imprementation take 2 way of small size slab
 *   and over pagesize memory. each methods must keep allocated size
 *   information. QNX took searching RBtree to get allocated size
 *   corresponding slab address, but it spend more CPU.
 *   We take a way to get size area directly from address
 *
 *   But this methods can't keep alignment
 */

#define ARCH_SLAB_MINALIGN __alignof__(unsigned long long)

static inline
unsigned long calculate_alignment(unsigned long flags,
		unsigned long align, unsigned long size)
{
	/*
	 * If the user wants hardware cache aligned objects then follow that
	 * suggestion if the object is sufficiently large.
	 *
	 * The hardware cache alignment cannot override the specified
	 * alignment though. If that is greater then use it.
	 */
	if (flags & SLAB_HWCACHE_ALIGN) {
		unsigned long ralign = cache_line_size();
		while (size <= ralign / 2)
			ralign /= 2;
		align = max(align, ralign);
	}

	if (align < ARCH_SLAB_MINALIGN)
		align = ARCH_SLAB_MINALIGN;

	return ALIGN(align, sizeof(void *));
}

struct kmem_cache_element {
	struct kmem_cache_element *next;
};

struct kmem_cache {
	const char* name;
	size_t obj_size;
	size_t size;			/* The aligned/padded/added on size  */
	size_t align;			/* Alignment as calculated */
	unsigned long flags;			/* Active flags on the slab */
	void (*ctor)(void *);			/* Called on object slot creation */
	struct mutex lock;

	int cache_size;				/* Allocator cache, total amount of entries */
	struct kmem_cache_element *cache;
};

#define KMEM_ALLOC_MAGIC	0xa10ca10c
#define KMEM_FREE_MAGIC		0xfeeefeee

struct kmem_cache_tail {
	void *alloced;
	int alloc, free, magic;
};

struct kmem_cache* kmem_cache_create(const char *name, size_t obj_size, size_t align,
	unsigned long flags, void (*ctor)(void*))
{
	struct kmem_cache* slab;
	int it;

	slab = kmalloc(sizeof(*slab), GFP_KERNEL);
	if (!slab) {
		qnx_error("Can't allocate memory for slab cache");
		return NULL;
	}

	*slab = (struct kmem_cache) {
		.name = name,
		.obj_size = obj_size,
		.flags = flags,
		.ctor = ctor,
		.cache = NULL,
		.cache_size = 0,
	};
	slab->size = ALIGN(max(obj_size,
				sizeof(struct kmem_cache_element)),
			__alignof__(struct kmem_cache_tail));
	slab->align = calculate_alignment(flags, align,
			slab->size + sizeof(struct kmem_cache_tail));

	mutex_init(&slab->lock);

#ifdef DEBUG_KMEM_CACHE
	printk(KERN_INFO "DEBUG %s caller=%#x objsize=%ld size=%ld "
			"align=%ld\n", __func__, caller_main_offset(),
			obj_size, slab->size, slab->align);
#endif

	return slab;
}

void kmem_cache_destroy(struct kmem_cache* slab)
{
	mutex_lock(&slab->lock);
	while (slab->cache) {
		void *p;
		struct kmem_cache_tail *tail;

		p = slab->cache;
		slab->cache = slab->cache->next;

		p += slab->size;
		tail = p;
		free(tail->alloced);
	}
	mutex_unlock(&slab->lock);
	mutex_destroy(&slab->lock);
	kfree(slab);
}

void kmem_cache_free(struct kmem_cache* slab, void *p)
{
	struct kmem_cache_element *e = p;

#ifdef DEBUG_KMEM_CACHE
	trace_logf(_NTO_TRACE_USERFIRST, "%s caller=%#x slab=%#lx p=%#lx",
			__func__, caller_main_offset(), (unsigned long)slab,
			(unsigned long)p);
#endif
	if (!p)
		return;
	else {
		struct kmem_cache_tail *tail;

		tail = p + slab->size;
		WARN_ON_ONCE(tail->magic != KMEM_ALLOC_MAGIC);
		tail->free = caller_main_offset();
		tail->magic = KMEM_FREE_MAGIC;
	}
	mutex_lock(&slab->lock);
	e->next = slab->cache;
	slab->cache = e;
	mutex_unlock(&slab->lock);
}

void *kmem_cache_alloc(struct kmem_cache *slab, gfp_t flags)
{
	struct kmem_cache_element *e;
	struct kmem_cache_tail *tail;
	void *p, *ret;
	size_t sz;

	mutex_lock(&slab->lock);
	e = slab->cache;
	if (e) {
		slab->cache = e->next;
	}
	mutex_unlock(&slab->lock);
	if (e) {
		ret = e;
		goto retp;
	}
	/*
	 * allocate new kmem
	 */
	sz = slab->size + sizeof(*tail) + slab->align - 1;
	p = malloc(sz);
	if (!p) {
		return NULL;	/* alloc failed */
	}
	ret = PTR_ALIGN(p, slab->align);
	tail = ret + slab->size;
	tail->alloced = p;
#ifdef DEBUG_KMEM_CACHE
	if (p != ret) do {
		static int debug=1;
		if (!debug) break;
		debug--;
		printk(KERN_INFO "DEBUG %s size=%ld:%ld align=%ld "
				"alloced=0x%p returned=0x%p tail=0x%p\n",
				__func__, slab->size, sz, slab->align,
				p, ret, tail);
	} while(0);
	trace_logf(_NTO_TRACE_USERFIRST, "%s caller=%#x slab=%#lx "
			"addr=%#lx:%#lx tail=%#lx size=%ld:%ld align=%ld",
			__func__, caller_main_offset(), (unsigned long)slab,
			(unsigned long)p, (unsigned long)ret,
			(unsigned long)tail, slab->size, sz, slab->align);
#endif

retp:
	if (flags & __GFP_ZERO) {
		memset(ret, 0, slab->obj_size);
	}
	tail = ret + slab->size;
	tail->alloc = caller_main_offset();
	tail->magic = KMEM_ALLOC_MAGIC;

	if (slab->ctor) {
		(slab->ctor)(ret);
	}
	return ret;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/kmalloc.c $ $Rev: 851843 $")
#endif
