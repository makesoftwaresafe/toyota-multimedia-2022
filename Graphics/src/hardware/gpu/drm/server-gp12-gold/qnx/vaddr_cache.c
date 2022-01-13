#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/memmsg.h>
#include <sys/slog.h>
#include <sys/slogcodes.h>
#include <sys/tree.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <asm/uaccess.h>
#include <linux/debug.h>
#include <linux/vaddr_cache.h>
#include <linux/slab.h>

static pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;

struct map_entry {
	RB_ENTRY(map_entry) links;
	off64_t paddr;
	void *vaddr;
};

static RB_HEAD(p2v_map, map_entry) vaddr_cache = RB_INITIALIZER(&vaddr_cache);

static int
paddr_cmp( struct map_entry *e1, struct map_entry *e2 )
{
	if( e1->paddr < e2->paddr ) {
		return -1;
	} else if( e1->paddr > e2->paddr ) {
		return 1;
	} else {
		return 0;
	}
}

RB_GENERATE(p2v_map, map_entry, links, paddr_cmp);

const int mblock_size = __PAGESIZE * 1024;
static int mmap64_count = 0;

static inline void *
cache_find(off64_t off)
{
	off64_t pa = off & ~(mblock_size-1);
	struct map_entry key = {.paddr = pa};

	struct map_entry *ent = RB_FIND(p2v_map, &vaddr_cache, &key);

	return ent ? ((char*)ent->vaddr + (off-pa)) : NULL;
}

static int
vaddr_cache_add_pages(pid_t pid, off64_t paddr, int pages, void ** vaddrs)
{
	int rc = pthread_mutex_lock(&_mutex);
	if(rc){
		qnx_error("pthread_mutex_lock");
		return -1;
	}
	int i;

	for(i=0; i<pages;){
		if( (vaddrs[i] = cache_find(paddr) )) {
			/* find it */
			i++;
			/* move to next page */
			paddr += __PAGESIZE;
		} else {
			void * vaddr;
			off64_t pa = paddr & ~(mblock_size-1);
			do {
				vaddr = mmap64(0, mblock_size, PROT_READ | PROT_WRITE, MAP_PHYS | MAP_SHARED, NOFD, pa);
				if (vaddr != MAP_FAILED) {
					mmap64_count++;
					break;
				}
				qnx_error("mmap64 failed.");
			} while(errno == EAGAIN);

			if(vaddr == MAP_FAILED){
				qnx_error("mmap64 failed.");
				pthread_mutex_unlock(&_mutex);
				return -1;
			}
			struct map_entry *ent = calloc(1, sizeof(*ent));
			if (!ent) {
				qnx_error("calloc() of map_entry failed.");
				pthread_mutex_unlock(&_mutex);
				return -1;
			}
			ent->paddr = pa;
			ent->vaddr = vaddr;
			RB_INSERT(p2v_map, &vaddr_cache, ent);
		}
	}
	pthread_mutex_unlock(&_mutex);
	return 0;
}

static inline off64_t vaddr2paddr(pid_t pid, void* vaddr, int size, off64_t* paddr, size_t* contig)
{
	if (-1 == mem_offset64_peer(pid, (uintptr_t)vaddr, size, paddr, contig)) {
		return -1;
	}

	return 0;
}

int
vaddr_cache_get_pages(pid_t pid, void * pvaddr, size_t len,
				struct vaddr_list_t * result)
{
	off64_t off = (off64_t)(uintptr_t)(pvaddr) & (__PAGESIZE-1);
	int pages = (off + len +__PAGESIZE-1)/__PAGESIZE;
	int page_size = __PAGESIZE * pages;

	result->off = off;
	result->pages = pages;
	result->vaddrs = (void **)calloc(1, pages * sizeof(void *));
	if(!result->vaddrs){
		qnx_error("calloc failed");
		return -ENOMEM;
	}
	void ** vaddrs = result->vaddrs;
	char * page_vaddr = (char *)((uintptr_t) pvaddr & ~(__PAGESIZE-1));

	while(pages >0){
		size_t contig;
		off64_t paddr;
		/* get physical addr */
		if(-1 == vaddr2paddr(pid, page_vaddr, page_size, &paddr, &contig)){
			free(result->vaddrs);
			result->vaddrs=NULL;
			return -1;
		}
		assert((contig % __PAGESIZE) == 0);
		/* map all of it */
		int npages = contig / __PAGESIZE;
		if(-1 == vaddr_cache_add_pages(pid, paddr, npages, vaddrs)){
			free(result->vaddrs);
			result->vaddrs = NULL;
			return -1;
		}

		page_vaddr += npages * __PAGESIZE;
		page_size -= npages * __PAGESIZE;
		vaddrs += npages;
		pages -= npages;
	}
	return 0;
}

/*
 * DENSO
 * uaddr cache
 */
#include <linux/mutex.h>
#include <linux/idr.h>

atomic_t uaddr_cache_validseq;
static struct kmem_cache *uaddr_entry_cache;

/*
 * uaddr_map -- set of uaddr per pid
 */
struct uaddr_entry {
	RB_ENTRY(uaddr_entry) links;
	off64_t upage_addr;
	void *vpage_addr;
	int uaddr_validseq;
};

typedef RB_HEAD(rb_uaddr, uaddr_entry)	uaddr_cache;

struct uaddr_map {
	uaddr_cache	cache;
	struct mutex	lock;

	pid_t pid;
	int uaddr_map_id;
	atomic_t refcount;
};

/*
 * uaddr_idr -- find uaddr_map by uaddr_map_id
 */
static DEFINE_MUTEX(uaddr_idr_lock);
static DEFINE_IDR(uaddr_idr);

static int uaddr_cmp(struct uaddr_entry *e1, struct uaddr_entry *e2)
{
	return e1->upage_addr - e2->upage_addr;
}

RB_GENERATE(rb_uaddr, uaddr_entry, links, uaddr_cmp);

//#define DEBUG_UADDR_UT
#ifdef DEBUG_UADDR_UT
#define uaddr_printf(fmt, ...) do { \
	printf("DEBUG_UADDR %s(%d) " fmt, \
		__func__, pthread_self(), ##__VA_ARGS__); \
} while(0)
#else
#define uaddr_printf(...)
#endif

static struct uaddr_map *uaddr_get_map(int id, pid_t pid, bool iscreate)
{
	struct uaddr_map *um;

	mutex_lock(&uaddr_idr_lock);

	if (!uaddr_entry_cache) {
		uaddr_entry_cache = KMEM_CACHE(uaddr_entry, 0);
		if (!uaddr_entry_cache) {
			mutex_unlock(&uaddr_idr_lock);
			qnx_error("KMEM_CACHE");
			return NULL;
		}
	}

	/*
	 * find by id
	 */
	if (id > 0) {
		um = idr_find(&uaddr_idr, id);
		if (um->pid == pid) {
			goto retp;
		}
	}
	if (!pid || id < 0) {
		um = NULL;
		goto retp;
	}
	/*
	 * find by pid
	 */
	idr_for_each_entry(&uaddr_idr, um, id) {
		if (um->pid == pid) {
			goto retp;
		}
	}
	if (!iscreate) {
		um = NULL;
		goto retp;
	}
	mutex_unlock(&uaddr_idr_lock);

	/*
	 * create new um
	 */
	um = kmalloc(sizeof(*um), GFP_KERNEL);
	if (!um) return NULL;

	RB_INIT(&um->cache);
	mutex_init(&um->lock);

	um->pid = pid;
	atomic_set(&um->refcount, 0);

	mutex_lock(&uaddr_idr_lock);
	id = idr_alloc(&uaddr_idr, um, 1, 0, GFP_KERNEL);
	if (id > 0) {
		um->uaddr_map_id = id;
	} else {
		mutex_destroy(&um->lock);
		kfree(um);
		um = NULL;
	}
	uaddr_printf("allocate um=%#lx id=%d pid=%d\n",
			(unsigned long)um, um->uaddr_map_id, pid);
retp:
	if (um) {
		atomic_add(1, &um->refcount);
	}
	mutex_unlock(&uaddr_idr_lock);
	return um;
}

int uaddr_get(int id, pid_t pid)
{
	struct uaddr_map *um;

	um = uaddr_get_map(id, pid, 1);	/* find and create */
	if (um) {
		return um->uaddr_map_id;
	} else {
		return -1;
	}
}

static struct uaddr_entry *new_ue()
{
	struct uaddr_entry *ue;

	ue = kmem_cache_alloc(uaddr_entry_cache, GFP_KERNEL);
	if (ue) {
		ue->uaddr_validseq = atomic_read(&uaddr_cache_validseq);
	}
	return ue;
}

static void remove_ue(struct uaddr_map *um, struct uaddr_entry *ue)
{
	RB_REMOVE(rb_uaddr, &um->cache, ue);
	kmem_cache_free(uaddr_entry_cache, ue);
}

static struct uaddr_entry *find_ue(struct uaddr_map *um, off_t upage_addr)
{
	struct uaddr_entry *ue, key = {
		.upage_addr = upage_addr,
	};
	int c;
#ifdef DEBUG_CACHE_HIT
	static int debug;
	static int hit, miss;
#endif

	ue = RB_FIND(rb_uaddr, &um->cache, &key);
	if (!ue || ue->uaddr_validseq == atomic_read(&uaddr_cache_validseq)) {
#ifdef DEBUG_CACHE_HIT
		ue ? hit++ : miss++;
#endif
		return ue;
	}
	/*
	 * generate changed
	 */
	c = 0;
	while ((ue = RB_MIN(rb_uaddr, &um->cache))) {
		remove_ue(um, ue);
		c++;
	}
#ifdef DEBUG_CACHE_HIT
	if (++debug & 128) {
		debug = 0;
		printf("%s pid=%d invalidate uaddr-cache %d "
				"hit=%d/%d = %d%%\n",
				__func__, um->pid, c, hit, hit+miss+1,
				hit * 100 / (hit+miss+1) );
	}
	hit = miss = 0;
#endif
	return NULL;
}

static int uaddr_put_map(struct uaddr_map *um)
{
	struct uaddr_entry *ue;

	if (!um) return 0;

	if (atomic_add_return(-1, &um->refcount)) {
		return um->uaddr_map_id;
	}

	mutex_lock(&um->lock);
	while ((ue = RB_MIN(rb_uaddr, &um->cache))) {
		uaddr_printf("(id=%d pid=%d) "
				"remove entry %#lx:upage_addr=%#lx\n",
				um->uaddr_map_id, um->pid,
				(unsigned long)ue,
				(unsigned long)ue->upage_addr);
		remove_ue(um, ue);
	}
	mutex_unlock(&um->lock);

	mutex_lock(&uaddr_idr_lock);
	idr_remove(&uaddr_idr, um->uaddr_map_id);
	mutex_unlock(&uaddr_idr_lock);

	uaddr_printf("(%d pid=%d) destroy map=%#lx\n",
			um->uaddr_map_id, um->pid, (unsigned long)um);
	kfree(um);
	return 0;
}

int uaddr_put(int id)
{
	struct uaddr_map *um;
	int ret;

	um = uaddr_get_map(id, 0, 0);
	if (!um) {
		return id;
	}
	ret = uaddr_put_map(um);	/* decrement refcount for get um */
	if (ret > 0) {
		ret = uaddr_put_map(um);
	} else {
		qnx_error("refcount conflict um=%#lx", (unsigned long)um);
		ret = 0;	/* reset id for reusable */
	}
	return ret;
}

int uaddr_to_vaddr(int id, pid_t pid, struct uaddr *uaddr)
{
	struct uaddr_map *um;
	struct uaddr_entry *ue, ue_entity;

	off64_t upage_addr;
	size_t page_offset;
	int ret;

	um = uaddr_get_map(id, pid, 1);
	if (!um) {
		return -ENOMEM;
	}

	upage_addr = (off64_t)(uintptr_t)uaddr->uaddr;
	page_offset = upage_addr & (__PAGESIZE-1);
	upage_addr -= page_offset;

	mutex_lock(&um->lock);
	ue = find_ue(um, upage_addr);
	if (ue) {
		ue_entity = *ue;
		ue = &ue_entity;
			/*
			 * finded ue might be remove at unlock
			 */
	}
	mutex_unlock(&um->lock);

	if (!ue) {
		off64_t pa;
		size_t mapsize, contig;
		void *vaddr;

		/*
		 * map user space into current space
		 */
		mapsize = ALIGN(page_offset + uaddr->size, __PAGESIZE);
		ret = mem_offset64_peer(pid, upage_addr, mapsize,
				&pa, &contig);
		if (ret == -1) {
			qnx_error("mem_offset64_peer failed "
				"pid=%d upage_addr=%#lx mapsize=%#lx",
				pid, upage_addr, mapsize);
			ret = -EFAULT;
			goto retp;
		}
		/*
		 * save map info into cache
		 */
		while (contig) {
			struct uaddr_entry *tue;
			static int debug=0;

			ret = vaddr_cache_add_pages(pid, pa, 1, &vaddr);
			if (ret) {
				qnx_error("vaddr_cache_add_pages failed "
					"pid=%d pa=%#lx contig=%#lx ret=%d",
					pid, pa, contig, ret);
				ret = -EFAULT;
				goto retp;
			}

			tue = new_ue();
			if (!tue) {
				break;
			}

			tue->upage_addr = upage_addr;
			tue->vpage_addr = vaddr;

			if (!ue) {
				ue_entity = *tue;
				ue = &ue_entity;
			}

			mutex_lock(&um->lock);
			if (RB_INSERT(rb_uaddr, &um->cache, tue)) {
				/*
				 * insert error : already inserted
				 */
				kmem_cache_free(uaddr_entry_cache, tue);
				tue = NULL;
				contig = 0;	/* not continue any more */
			}
			mutex_unlock(&um->lock);

			if (debug && tue) {
				/*
				 * *tue might be removed,
				 * but it is for simple debug
				 */
				debug--;
				uaddr_printf("alloc ue=%#lx:"
						"uaddr=%#lx vaddr=%#lx "
						"mapsize=%#lx\n",
						(unsigned long)tue,
						tue->upage_addr,
						(unsigned long)
							tue->vpage_addr,
						tue->mapsize);
			}
			pa += __PAGESIZE;
			upage_addr += __PAGESIZE;
			vaddr += __PAGESIZE;
			if (contig >= __PAGESIZE) {
				contig -= __PAGESIZE;
			} else {
				contig = 0;
			}
		}
	}
	if (!ue) {
		ret = -ENOMEM;
		goto retp;
	}

	uaddr->vaddr = ue->vpage_addr + page_offset;
	uaddr->size = min(__PAGESIZE - page_offset, uaddr->size);
	ret = 0;
retp:
	if (um) {
		uaddr_put_map(um);
	}
	return ret;
}

void uaddr_forget(int id, pid_t pid, struct uaddr *uaddr)
{
	struct uaddr_map *um;

	off64_t upage_addr;
	size_t page_offset, remain;
	int ret;

	um = uaddr_get_map(id, pid, 1);
	if (!um) {
		return;
	}

	upage_addr = (off64_t)(uintptr_t)uaddr->uaddr;
	page_offset = upage_addr & (__PAGESIZE-1);
	upage_addr -= page_offset;

	remain = ALIGN(page_offset + uaddr->size, __PAGESIZE);
	while (remain) {
		struct uaddr_entry *ue;

		mutex_lock(&um->lock);
		ue = find_ue(um, upage_addr);
		if (ue) {
			remove_ue(um, ue);
		}
		mutex_unlock(&um->lock);

		upage_addr += __PAGESIZE;
		if (remain >= __PAGESIZE) {
			remain -= __PAGESIZE;
		} else {
			remain = 0;
		}
	}
	uaddr_put_map(um);
}

void uaddr_test(const char *comm, pid_t pid)
{
	struct uaddr_map *um;
	struct uaddr_entry *ue;
	int id, i;

	uaddr_printf("(%s pid=%d) start\n", comm, pid);

	id = uaddr_get(0, pid);
	uaddr_printf("(pid=%d) get-1=%d\n", pid, id);
	um = uaddr_get_map(id, pid, 0);
	uaddr_printf("(pid=%d) get-2=%p\n", pid, um);
	for (i = 0; i < 3; i++) {
		ue = new_ue();
		ue->upage_addr = (i + 1);

		RB_INSERT(rb_uaddr, &um->cache, ue);
		uaddr_printf("(pid=%d) insert %d:%#lx\n",
				pid, i, (unsigned long)ue);
	}
	for (i = 0; i < 3; i++) {
		ue = find_ue(um, i+1 );
		uaddr_printf("(pid=%d) find %d:%#lx\n",
				pid, i, (unsigned long)ue);
	}
	uaddr_put_map(um);
	uaddr_put(id);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/vaddr_cache.c $ $Rev: 847774 $")
#endif

