#ifndef __QNX_IO_MAPPING_H__
#define __QNX_IO_MAPPING_H__

#include <linux/pagemap.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
#include <asm/barrier.h>

/** io mapping */
struct io_mapping {
	resource_size_t base;
	unsigned long size;
	pgprot_t prot;
	void* vaddr;
	void* vaddr_wc;
	int fd_wc;
};

void io_mapping_unmap_atomic(void *vaddr);
void io_mapping_unmap(void *vaddr);
void* ioremap_wc(resource_size_t offset, unsigned long size);

static inline void
io_mapping_free(struct io_mapping *iomap)
{
	if (iomap->vaddr_wc) {
		io_mapping_unmap(iomap->vaddr_wc);
		iomap->vaddr_wc = NULL;
	}
	if (iomap->vaddr) {
		io_mapping_unmap(iomap->vaddr);
		iomap->vaddr = NULL;
	}
}

static inline void iomap_free(struct io_mapping *mapping)
{
	io_mapping_free(mapping);
}

static inline void io_mapping_fini(struct io_mapping *mapping)
{
	iomap_free(mapping);
}

static inline int iomap_create_wc(resource_size_t base, unsigned long size, pgprot_t *prot)
{
	*prot = pgprot_writecombine(PAGE_KERNEL_IO);

	return 0;
}

/*
 * For small address space machines, mapping large objects
 * into the kernel virtual space isn't practical. Where
 * available, use fixmap support to dynamically map pages
 * of the object at run time.
 */

static inline struct io_mapping *
io_mapping_init_wc(struct io_mapping *iomap,
		   resource_size_t base,
		   unsigned long size)
{
	pgprot_t prot;

	if (iomap_create_wc(base, size, &prot))
		return NULL;

	iomap->base = base;
	iomap->size = size;
	iomap->prot = prot;
	iomap->vaddr_wc = ioremap_wc(base, size);

	return iomap;
}

static inline void* io_mapping_map_wc(struct io_mapping* mapping, unsigned long offset, unsigned long size)
{
	resource_size_t phys_addr;

	BUG_ON(offset >= mapping->size);
	phys_addr = mapping->base + offset;

	return (void*)((char*)mapping->vaddr_wc + offset);
}

static inline void* io_mapping_map_atomic_wc(struct io_mapping* mapping, unsigned long offset)
{
	return io_mapping_map_wc(mapping, offset, PAGE_SIZE);
}

#endif /* __QNX_IO-MAPPING_H__ */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/io-mapping.h $ $Rev: 844871 $")
#endif
