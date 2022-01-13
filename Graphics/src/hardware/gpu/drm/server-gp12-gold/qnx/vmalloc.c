#include <stdio.h>
#include <pthread.h>
#include <sys/mman.h>
#include <asm/pgtable_types.h>
#include <linux/mm.h>
#include <linux/mm_types.h>

#include <fcntl.h>

#include "mmap_trace.h"

void* vmap(struct page** pages, unsigned int count, unsigned long flags, pgprot_t prot)
{
	int it, jt;
	char* mem;
	void* res;

	switch(pgprot_val(prot) & _PAGE_CACHE_MASK) {
		case _PAGE_PCD: /* All caches are disabled. Intel often calls this mode as "WT eLLC" or WC */
			if (pages[0]->virtual_map_mode == QNX_PAGE_MAP_WC) {
				return page_address(pages[0]);
			}
			if (pages[0]->virtual_alt_map_mode == QNX_PAGE_MAP_WC) {
				return pages[0]->virtual_alt;
			}

			if (pages[0]->virtual_alt_map_mode == QNX_PAGE_MAP_UNKNOWN) {
				int fd;
				int rc;

				/* Ok, now remap WB or UC memory as WC */
				mem = mmap64(NULL, count * __PAGESIZE, PROT_NONE, MAP_SHARED, NOFD, 0);
				if (mem != MAP_FAILED) {
					int pages_count;

					it = 0;
					while (it < count) {
						pages_count = 1;
						for (jt = it; jt < count - 1; jt++) {
							if (pages[jt]->offset + __PAGESIZE == pages[jt + 1]->offset) {
								pages_count++;
							} else {
								break;
							}
						}

						fd = shm_open(SHM_ANON, O_RDWR | O_CREAT, 0666);
						if (fd == -1) {
							qnx_error("shm_open() failed in vmap(): %s", strerror(errno));
						} else {
							rc = shm_ctl(fd, SHMCTL_PHYS | SHMCTL_LAZYWRITE, pages[it]->offset, __PAGESIZE * pages_count);
							if (rc != 0) {
								qnx_error("shm_ctl() failed in vmap(): %s", strerror(errno));
								close(fd);
								return NULL;
							}
							res = mmap64(mem + it * __PAGESIZE, __PAGESIZE * pages_count, PROT_READ | PROT_WRITE | PROT_NOCACHE,
								MAP_FIXED | MAP_SHARED, fd, 0);
							if (res == MAP_FAILED) {
								qnx_error("shm_ctl() failed in vmap(): %s", strerror(errno));
								close(fd);
								return NULL;
							}
							for (jt = it; jt < it + pages_count; jt++) {
								pages[jt]->virtual_alt = mem + jt * __PAGESIZE;
								pages[jt]->virtual_alt_map_mode = QNX_PAGE_MAP_WC;
							}
							close(fd);
						}

						it += pages_count;
					}
					mmap_trace_add_range(mem, count * __PAGESIZE);
				} else {
					qnx_error("mmap64() balloon allocation failed in vmap()!");
					return NULL;
				}
				return pages[0]->virtual_alt;
			} else {
				BUG();
				fprintf(stderr, "*** ERROR ***: _PAGE_PCD page protection is requested (WT eLLC or WC), but all slots are busy with %d:%d, count is %d\n",
					pages[0]->virtual_map_mode, pages[0]->virtual_alt_map_mode, count);
				return NULL;
			}

			BUG();
			fprintf(stderr, "*** ERROR ***: _PAGE_PCD page protection is requested (WT eLLC or WC, current modes are %d:%d, count is %d)\n",
				pages[0]->virtual_map_mode, pages[0]->virtual_alt_map_mode, count);
			return NULL;
		case _PAGE_PAT: /* Special mode is requested. Intel often calls this mode as "WB LLCeLLC" */
			/* MG_TODO: Should be the same as (0) full cached mode. Need to check CPU manual */
			BUG();
			fprintf(stderr, "*** ERROR ***: _PAGE_PAT page protection is requested (WB LLCeLLC)\n");
			return NULL;
		case _PAGE_PWT | _PAGE_PCD: /* Special mode is requested. Intel often calls this mode as "uncached" */
			if (pages[0]->virtual_map_mode == QNX_PAGE_MAP_UC) {
				return page_address(pages[0]);
			}
			if (pages[0]->virtual_alt_map_mode == QNX_PAGE_MAP_UC) {
				return pages[0]->virtual_alt;
			}

			BUG();
			fprintf(stderr, "*** ERROR ***: _PAGE_PWT | _PAGE_PCD page protection is requested (uncached)\n");
			return NULL;
		case 0: /* Full cache mapping is requested. Intel often calls this mode as "cached" */
			if (pages[0]->virtual_map_mode == QNX_PAGE_MAP_WB) {
				/* We already have mapped this region as WB */
				return page_address(pages[0]);
			}
			if (pages[0]->virtual_alt_map_mode == QNX_PAGE_MAP_WB) {
				/* We already have mapped this region as WB */
				return pages[0]->virtual_alt;
			}

			if (pages[0]->virtual_alt_map_mode == QNX_PAGE_MAP_UNKNOWN) {
				/* Ok, now remap WC or UC memory as WB */
				mem = mmap64(NULL, count * __PAGESIZE, PROT_NONE, MAP_SHARED, NOFD, 0);
				if (mem != MAP_FAILED) {
					int pages_count;

					it = 0;
					while (it < count) {
						pages_count = 1;
						for (jt = it; jt < count - 1; jt++) {
							pages[jt]->virtual_alt = mem + jt * __PAGESIZE;
							pages[jt]->virtual_alt_map_mode = QNX_PAGE_MAP_WB;
							if (pages[jt]->offset + __PAGESIZE == pages[jt + 1]->offset) {
								pages_count++;
							} else {
								break;
							}
						}
						/* Fill last page of a block */
						pages[jt]->virtual_alt = mem + jt * __PAGESIZE;
						pages[jt]->virtual_alt_map_mode = QNX_PAGE_MAP_WB;

						res = mmap64(pages[it]->virtual_alt, __PAGESIZE * pages_count, PROT_READ | PROT_WRITE,
							MAP_FIXED | MAP_PHYS | MAP_SHARED, NOFD, pages[it]->offset);
						if (res == MAP_FAILED) {
							qnx_error("placement mmap64() in vmap() call failed!");
							/* Clean failed alternative mapping on current set of pages */
							for (jt = it; jt < it + pages_count; jt++) {
								pages[jt]->virtual_alt = NULL;
								pages[jt]->virtual_alt_map_mode = QNX_PAGE_MAP_UNKNOWN;
							}
							/* Clean failed alternative mapping on all pages before current */
							for (jt = 0; jt < count; jt++) {
								if (pages[jt]->virtual_alt) {
									munmap(pages[jt]->virtual_alt, __PAGESIZE);
									pages[jt]->virtual_alt = NULL;
									pages[jt]->virtual_alt_map_mode = QNX_PAGE_MAP_UNKNOWN;
								}
							}
							return NULL;
						}
						it += pages_count;
					}

					mmap_trace_add_range(mem, count * __PAGESIZE);
					return pages[0]->virtual_alt;
				} else {
					qnx_error("mmap64() balloon allocation failed in vmap()!");
					return NULL;
				}
			} else {
				BUG();
				fprintf(stderr, "*** ERROR ***: _PAGE_PWT | _PAGE_PCD page protection is requested (uncached), but all slots are busy with %d:%d, count is %d\n",
					pages[0]->virtual_map_mode, pages[0]->virtual_alt_map_mode, count);
				return NULL;
			}
			return NULL;
		default: /* Unknown mode is requested. */
			BUG();
			fprintf(stderr, "*** ERROR ***: CACHE_MASK=%016llX page protection is requested\n",
				(unsigned long long)pgprot_val(prot) & _PAGE_CACHE_MASK);
			return NULL;
	}

	return page_address(pages[0]);
}

void vunmap(const void *addr)
{
	/* Do nothing, page will be unmapped on destroy */
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/vmalloc.c $ $Rev: 874574 $")
#endif
