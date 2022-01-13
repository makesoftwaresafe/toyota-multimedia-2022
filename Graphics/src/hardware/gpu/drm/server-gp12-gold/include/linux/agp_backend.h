#ifndef _QNX_LINUX_AGP_BACKEND_H
#define _QNX_LINUX_AGP_BACKEND_H

#include <linux/types.h>
#include <linux/list.h>

enum chipset_type {
	NOT_SUPPORTED,
	SUPPORTED,
};

struct agp_version {
	u16 major;
	u16 minor;
};

struct agp_kern_info {
	struct agp_version version;
	struct pci_dev *device;
	enum chipset_type chipset;
	unsigned long mode;
	unsigned long aper_base;
	size_t aper_size;
	int max_memory;		/* In pages */
	int current_memory;
	bool cant_use_aperture;
	unsigned long page_mask;
	const struct vm_operations_struct *vm_ops;
};

/*
 * The agp_memory structure has information about the block of agp memory
 * allocated.  A caller may manipulate the next and prev pointers to link
 * each allocated item into a list.  These pointers are ignored by the backend.
 * Everything else should never be written to, but the caller may read any of
 * the items to determine the status of this block of agp memory.
 */

struct agp_bridge_data;

struct agp_memory {
	struct agp_memory *next;
	struct agp_memory *prev;
	struct agp_bridge_data *bridge;
	struct page **pages;
	size_t page_count;
	int key;
	int num_scratch_pages;
	off_t pg_start;
	u32 type;
	u32 physical;
	bool is_bound;
	bool is_flushed;
	/* list of agp_memory mapped to the aperture */
	struct list_head mapped_list;
	/* DMA-mapped addresses */
	struct scatterlist *sg_list;
	int num_sg;
};

#define AGP_NORMAL_MEMORY 0

#define AGP_USER_TYPES (1 << 16)
#define AGP_USER_MEMORY (AGP_USER_TYPES)
#define AGP_USER_CACHED_MEMORY (AGP_USER_TYPES + 1)

extern struct agp_bridge_data *agp_bridge;
extern struct list_head agp_bridges;

extern struct agp_bridge_data *(*agp_find_bridge)(struct pci_dev *);

extern void agp_free_memory(struct agp_memory *);
extern struct agp_memory *agp_allocate_memory(struct agp_bridge_data *, size_t, u32);
extern int agp_copy_info(struct agp_bridge_data *, struct agp_kern_info *);
extern int agp_bind_memory(struct agp_memory *, off_t);
extern int agp_unbind_memory(struct agp_memory *);
extern void agp_enable(struct agp_bridge_data *, u32);
extern struct agp_bridge_data *agp_backend_acquire(struct pci_dev *);
extern void agp_backend_release(struct agp_bridge_data *);

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/agp_backend.h $ $Rev: 836322 $")
#endif
