#include <stdlib.h>

#ifdef CONFIG_DRM_DEBUG_MM

#include <pthread.h>
#include <sys/tree.h>
#include <sys/mman.h>

#include <linux/gfp.h>

static pthread_mutex_t mmap_trace_mutex = PTHREAD_MUTEX_INITIALIZER;

struct map_entry {
	RB_ENTRY(map_entry) links;
	void* vaddr;
};

static RB_HEAD(mmap_trace_map, map_entry) mmap_trace_cache = RB_INITIALIZER(&mmap_trace_cache);

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

RB_GENERATE(mmap_trace_map, map_entry, links, vaddr_cmp);

int mmap_trace_dump()
{
    struct map_entry* ent;
    int count = 0;

    int rc = pthread_mutex_lock(&mmap_trace_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock");
        return 0;
    }

    RB_FOREACH(ent, mmap_trace_map, &mmap_trace_cache) {
        count++;
    }

    pthread_mutex_unlock(&mmap_trace_mutex);

    fprintf(stderr, "<mmap_trace>: %d pages (%ld bytes) in the tree\n", count, (long)count * PAGE_SIZE);

    return 1;
}

int mmap_trace_check(void* addr)
{
    struct map_entry key = {.vaddr = addr};
    struct map_entry* ent;

    if (addr == NULL) {
        return 0;
    }

    int rc = pthread_mutex_lock(&mmap_trace_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock");
        return 0;
    }

    ent = RB_FIND(mmap_trace_map, &mmap_trace_cache, &key);
    if (ent == NULL) {
        pthread_mutex_unlock(&mmap_trace_mutex);
        return 0;
    }

    pthread_mutex_unlock(&mmap_trace_mutex);

    return 1;
}

void* mmap_trace_add(void* addr)
{
    void* mem;
    struct map_entry* ent;

    int rc = pthread_mutex_lock(&mmap_trace_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock, error: %d", rc);
        if (rc == EDEADLK) {
            abort();
        }
        return NULL;
    }

    ent = calloc(1, sizeof(*ent));
    if (!ent) {
        qnx_error("calloc(1, map_entry) in mmap_trace() failed.");
        pthread_mutex_unlock(&mmap_trace_mutex);
        return NULL;
    }

    ent->vaddr = addr;

    RB_INSERT(mmap_trace_map, &mmap_trace_cache, ent);

    pthread_mutex_unlock(&mmap_trace_mutex);

    return addr;
}

void* mmap_trace_add_range(void* addr, size_t size)
{
    int it;
    char* vaddr = (char*)addr;

    for (it = 0; it < (size + PAGE_SIZE - 1) / PAGE_SIZE; it++) {
        mmap_trace_add(vaddr);
        vaddr += PAGE_SIZE;
    }

    return addr;
}

void* mmap_trace_del(void* addr)
{
    struct map_entry key = {.vaddr = (void*)addr};
    struct map_entry* ent;

    if (addr == NULL) {
        return NULL;
    }

    int rc = pthread_mutex_lock(&mmap_trace_mutex);
    if (rc) {
        qnx_error("pthread_mutex_lock");
        return NULL;
    }

    ent = RB_FIND(mmap_trace_map, &mmap_trace_cache, &key);
    if (ent == NULL) {
        qnx_error("Attempt to call munmap() for non-mmap'ed object: %p!", addr);
        pthread_mutex_unlock(&mmap_trace_mutex);
        abort();
        return NULL;
    }

    RB_REMOVE(mmap_trace_map, &mmap_trace_cache, ent);

    pthread_mutex_unlock(&mmap_trace_mutex);

    free(ent);

    return addr;
}

void* mmap_trace_del_range(void* addr, size_t size)
{
    int it;
    char* vaddr = (char*)addr;

    for (it = 0; it < (size + PAGE_SIZE - 1) / PAGE_SIZE; it++) {
        mmap_trace_del(vaddr);
        vaddr += PAGE_SIZE;
    }

    return addr;
}

#else /* CONFIG_DRM_DEBUG_MM */

int mmap_trace_dump()
{
    return 1;
}

int mmap_trace_check(void* addr)
{
    return 1;
}

void* mmap_trace_add(void* addr)
{
    return addr;
}

void* mmap_trace_add_range(void* addr, size_t size)
{
    return addr;
}

void* mmap_trace_del(void* addr)
{
    return addr;
}

void* mmap_trace_del_range(void* addr, size_t size)
{
    return addr;
}

#endif /* CONFIG_DRM_DEBUG_MM */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/mmap_trace.c $ $Rev: 845340 $")
#endif
