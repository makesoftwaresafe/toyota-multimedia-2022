#include <stdlib.h>
#include <inttypes.h>
#include <sys/tree.h>

#include <linux/qnx.h>
#include <linux/debug.h>
#include <asm/uaccess.h>

#include "mmap_trace.h"

static pthread_mutex_t iomap_mutex = PTHREAD_MUTEX_INITIALIZER;

struct map_entry {
	RB_ENTRY(map_entry) links;
	void* vaddr;
	size_t size;
};

static RB_HEAD(iomap_map, map_entry) iomap_cache = RB_INITIALIZER(&iomap_cache);

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

RB_GENERATE(iomap_map, map_entry, links, vaddr_cmp);

/**
 * ioremap_wc	-	map memory into CPU space write combined
 * @phys_addr:	bus address of the memory
 * @size:	size of the resource to map
 *
 * This version of ioremap ensures that the memory is marked write combining.
 * Write combining allows faster writes to some hardware devices.
 *
 * Must be freed with iounmap.
 */
void* ioremap_wc(resource_size_t offset, unsigned long size)
{
	int fd;
	int rc;
	void* addr;
	struct map_entry* ent;

	fd = shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd == -1) {
		qnx_error("shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0666) failed");
		return NULL;
	}

	rc = shm_ctl(fd, SHMCTL_PHYS | SHMCTL_LAZYWRITE, (uint64_t)offset, (uint64_t)size);
	if (rc) {
		qnx_error("shm_ctl(fd, SHMCTL_PHYS | SHMCTL_LAZYWRITE, 0x%"PRIx64", %"PRId64") failed",
				(uint64_t)offset, (uint64_t)size);
		close(fd);
		return NULL;
	}

	addr = mmap64(0, (size_t)size, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		qnx_error("mmap64() failed");
		close(fd);
		return NULL;
	}
	mmap_trace_add_range(addr, size);

	close(fd);

	rc = pthread_mutex_lock(&iomap_mutex);
	if (rc) {
		qnx_error("pthread_mutex_lock, error: %d", rc);
		if (rc == EDEADLK) {
			abort();
		}
		mmap_trace_del_range(addr, size);
		rc = munmap(addr, size);
		if (rc) {
			qnx_error("munmap() call failed!");
		}
		return NULL;
	}

	ent = calloc(1, sizeof(*ent));
	if (!ent) {
		qnx_error("calloc(1, map_entry) in iomap() failed.");
		mmap_trace_del_range(addr, size);
		rc = munmap(addr, size);
		if (rc) {
			qnx_error("munmap() call failed!");
		}
		pthread_mutex_unlock(&iomap_mutex);
		return NULL;
	}

	ent->vaddr = addr;
	ent->size = size;

	RB_INSERT(iomap_map, &iomap_cache, ent);

	pthread_mutex_unlock(&iomap_mutex);

	return addr;
}

/**
 * ioremap_nocache     -   map bus memory into CPU space
 * @phys_addr:    bus address of the memory
 * @size:      size of the resource to map
 *
 * ioremap_nocache performs a platform specific sequence of operations to
 * make bus memory CPU accessible via the readb/readw/readl/writeb/
 * writew/writel functions and the other mmio helpers. The returned
 * address is not guaranteed to be usable directly as a virtual
 * address.
 *
 * This version of ioremap ensures that the memory is marked uncachable
 * on the CPU as well as honouring existing caching rules from things like
 * the PCI bus. Note that there are other caches and buffers on many
 * busses. In particular driver authors should read up on PCI writes
 *
 * It's useful if some control registers are in such an area and
 * write combining or read caching is not desirable:
 *
 * Must be freed with iounmap.
 */
void* ioremap_nocache(resource_size_t offset, unsigned long size)
{
	int rc;
	void* addr;
	struct map_entry* ent;

	addr = mmap64(0, (size_t)size, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED | MAP_PHYS, NOFD, offset);
	if (addr == MAP_FAILED) {
		return NULL;
	}
	mmap_trace_add_range(addr, size);

	rc = pthread_mutex_lock(&iomap_mutex);
	if (rc) {
		qnx_error("pthread_mutex_lock, error: %d", rc);
		if (rc == EDEADLK) {
			abort();
		}
		mmap_trace_del_range(addr, size);
		rc = munmap(addr, size);
		if (rc) {
			qnx_error("munmap() call failed!");
		}
		return NULL;
	}

	ent = calloc(1, sizeof(*ent));
	if (!ent) {
		qnx_error("calloc(1, map_entry) in iomap() failed.");
		mmap_trace_del_range(addr, size);
		rc = munmap(addr, size);
		if (rc) {
			qnx_error("munmap() call failed!");
		}
		pthread_mutex_unlock(&iomap_mutex);
		return NULL;
	}

	ent->vaddr = addr;
	ent->size = size;

	RB_INSERT(iomap_map, &iomap_cache, ent);

	pthread_mutex_unlock(&iomap_mutex);

	return addr;
}

/**
 * ioremap     -   map bus memory into CPU space
 * @offset:    bus address of the memory
 * @size:      size of the resource to map
 *
 * ioremap performs a platform specific sequence of operations to
 * make bus memory CPU accessible via the readb/readw/readl/writeb/
 * writew/writel functions and the other mmio helpers. The returned
 * address is not guaranteed to be usable directly as a virtual
 * address.
 *
 * If the area you are trying to map is a PCI BAR you should have a
 * look at pci_iomap().
 */
void* ioremap(resource_size_t offset, unsigned long size)
{
	return ioremap_nocache(offset, size);
}

/**
 * iounmap - Free a IO remapping
 * @addr: virtual address from ioremap_*
 *
 * Caller must ensure there is only one unmapping for the same pointer.
 */
void iounmap(void* vaddr)
{
	struct map_entry key = {.vaddr = vaddr};
	struct map_entry* ent;
	size_t size = 0;
	int rc;

	rc = pthread_mutex_lock(&iomap_mutex);
	if (rc) {
		qnx_error("pthread_mutex_lock");
		return;
	}

	ent = RB_FIND(iomap_map, &iomap_cache, &key);

	if (ent == NULL) {
		qnx_error("Attempt to call iounmap() for non-iomap'ed object: %p!", vaddr);
		pthread_mutex_unlock(&iomap_mutex);
		abort();
		return;
	}

	size = ent->size;
	RB_REMOVE(iomap_map, &iomap_cache, ent);

	pthread_mutex_unlock(&iomap_mutex);

	free(ent);

	mmap_trace_del_range(vaddr, size);
	rc = munmap(vaddr, size);
	if (rc) {
		qnx_error("munmap() call failed!");
	}
}

void iounmap_atomic(void* vaddr)
{
	iounmap(vaddr);
}

void io_mapping_unmap(void *vaddr)
{
	/* Use pre-mapped addresses */
}

void io_mapping_unmap_atomic(void *vaddr)
{
	/* Use pre-mapped addresses */
}

/* Functions below are QNX extensions to linux kernel iomap API */

void* ioremap_wc_peer(pid_t pid, off64_t offset, size_t size)
{
	int fd;
	int rc;
	void* addr;

	fd = shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd == -1) {
		qnx_error("shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0666) failed");
		return NULL;
	}

	rc = shm_ctl(fd, SHMCTL_PHYS | SHMCTL_LAZYWRITE, (uint64_t)offset, (uint64_t)size);
	if (rc) {
		qnx_error("shm_ctl(fd, SHMCTL_PHYS | SHMCTL_LAZYWRITE, 0x%"PRIx64", %"PRId64") failed",
				(uint64_t)offset, (uint64_t)size);
		close(fd);
		return NULL;
	}

	addr = mmap64_peer(pid, 0, size, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, fd, 0);
	if (addr == MAP_FAILED) {
		qnx_error("mmap64_peer(%"PRId64", 0, %zd, PROT_READ | PROT_WRITE | PROT_NOCACHE,"
			" MAP_SHARED, %d, 0) failed", (uint64_t)pid, size, fd);
		close(fd);
		return NULL;
	}

	close(fd);
	return addr;
}

void iounremap_wc_peer(pid_t pid, void* vaddr, size_t size)
{
	int rc;

	rc = munmap64_peer(pid, vaddr, size);
	if (rc) {
		qnx_error("munmap64_peer() call failed!");
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/iomap.c $ $Rev: 845340 $")
#endif
