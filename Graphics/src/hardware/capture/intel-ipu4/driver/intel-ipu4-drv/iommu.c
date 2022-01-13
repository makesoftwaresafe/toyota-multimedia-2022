#include <linux/qnx.h>
#include <linux/linux.h>

/* is x a power of 2? */
#define is_power_of_2(x)	((x) != 0 && (((x) & ((x) - 1)) == 0))

static struct mutex iommu_group_mutex;
static int iommu_group_ID = 0;

struct iommu_group {
	struct list_head devices;
	struct mutex mutex;
	void *iommu_data;
	void (*iommu_data_release)(void *iommu_data);
	int id;
	int devices_ref_cnt;
};

struct iommu_device {
	struct list_head list;
	struct device *dev;
};

struct list {
	struct list *next, *prev;
};

/**
 * iommu_group_alloc - Allocate a new group
 *
 * This function is called by an iommu driver to allocate a new iommu
 * group.  The iommu group represents the minimum granularity of the iommu.
 * Upon successful return, the caller holds a reference to the supplied
 * group in order to hold the group until devices are added.  Use
 * iommu_group_put() to release this extra reference count, allowing the
 * group to be automatically reclaimed once it has no devices or external
 * references.
 */
struct iommu_group *iommu_group_alloc(void)
{
	struct iommu_group *group;

	group = kzalloc(sizeof(*group), GFP_KERNEL);
	if (!group)
		return ERR_PTR(-ENOMEM);

	mutex_init(&group->mutex);
	INIT_LIST_HEAD(&group->devices);

	mutex_lock(&iommu_group_mutex);
	group->id = iommu_group_ID++;
#ifdef __QNXNTO__
    // On linux case ref_count set to 1 when alloc group because they expects to keep group alive till iommu driver is active regardless attached to iommu group devices
    // Linux kernel destroys iommu group when destroy iommu driver
    // On qnx case ref_count set to 0 because we are going to keep group alive till it has attached device. When all devices are detached then group is destroyed.
    // if ref_count set to 1 we have memory leak on qnx case
	group->devices_ref_cnt = 0;
#else
	group->devices_ref_cnt = 1;
#endif
	mutex_unlock(&iommu_group_mutex);

	return group;
}

static void iommu_group_release(struct iommu_group *group)
{
	if (group->iommu_data_release)
		group->iommu_data_release(group->iommu_data);

#ifdef __QNXNTO__
	mutex_destroy(&group->mutex);
#endif
	kfree(group);
}

/**
 * iommu_group_get - Return the group for a device and increment reference
 * @dev: get the group that this device belongs to
 *
 * This function is called by iommu drivers and users to get the group
 * for the specified device.  If found, the group is returned and the group
 * reference in incremented, else NULL.
 */
struct iommu_group *iommu_group_get(struct device *dev)
{
	struct iommu_group *group = dev->iommu_group;

	if (group) {
		mutex_lock(&iommu_group_mutex);
		group->devices_ref_cnt++;
		mutex_unlock(&iommu_group_mutex);
	}

	return group;
}

/**
 * iommu_group_put - Decrement group reference
 * @group: the group to use
 *
 * This function is called by iommu drivers and users to release the
 * iommu group.  Once the reference count is zero, the group is released.
 */
void iommu_group_put(struct iommu_group *group)
{
	if (group) {
		mutex_lock(&iommu_group_mutex);
		group->devices_ref_cnt--;
#ifdef __QNXNTO__
		// Protect reading of devices_ref_cnt
		int devices_ref_cnt = group->devices_ref_cnt;
		mutex_unlock(&iommu_group_mutex);
		if (devices_ref_cnt == 0) {
#else
		mutex_unlock(&iommu_group_mutex);
		if (group->devices_ref_cnt == 0) {
#endif
			iommu_group_release(group);
		}
	}
}

/**
 * iommu_group_add_device - add a device to an iommu group
 * @group: the group into which to add the device (reference should be held)
 * @dev: the device
 *
 * This function is called by an iommu driver to add a device into a
 * group.  Adding a device increments the group reference count.
 */
int iommu_group_add_device(struct iommu_group *group, struct device *dev)
{
	struct iommu_device *device;

	device = kzalloc(sizeof(*device), GFP_KERNEL);
	if (!device)
		return -ENOMEM;

	device->dev = dev;
	INIT_LIST_HEAD(&device->list);

	dev->iommu_group = group;
	iommu_group_get(dev);

	mutex_lock(&group->mutex);
	list_add_tail(&device->list, &group->devices);
	mutex_unlock(&group->mutex);

	return 0;
}

/**
 * iommu_group_remove_device - remove a device from it's current group
 * @dev: device to be removed
 *
 * This function is called by an iommu driver to remove the device from
 * it's current group.  This decrements the iommu group reference count.
 */
void iommu_group_remove_device(struct device *dev)
{
	struct iommu_group *group = dev->iommu_group;
	struct iommu_device *tmp_device, *device = NULL;

	mutex_lock(&group->mutex);
	list_for_each_entry(tmp_device, &group->devices, list) {
		if (tmp_device->dev == dev) {
			device = tmp_device;
			list_del(&device->list);
			break;
		}
	}
	mutex_unlock(&group->mutex);

	if (!device)
		return;

	kfree(device);
	dev->iommu_group = NULL;
	iommu_group_put(group);
}


struct iommu_domain *iommu_domain_alloc(struct bus_type *bus)
{
	struct iommu_domain *domain;

	if (bus == NULL || bus->iommu_ops == NULL)
		return NULL;

	domain = bus->iommu_ops->domain_alloc(IOMMU_DOMAIN_UNMANAGED);
	if (!domain)
		return NULL;

	domain->ops  = bus->iommu_ops;
	domain->type = IOMMU_DOMAIN_UNMANAGED;

	return domain;
}

void iommu_domain_free(struct iommu_domain *domain)
{
	domain->ops->domain_free(domain);
}

int iommu_attach_device(struct iommu_domain *domain, struct device *dev)
{
	int ret;
	if (unlikely(domain->ops->attach_dev == NULL))
		return -ENODEV;

	ret = domain->ops->attach_dev(domain, dev);
	return ret;
}

phys_addr_t iommu_iova_to_phys(struct iommu_domain *domain, dma_addr_t iova)
{
	if (unlikely(domain->ops->iova_to_phys == NULL))
		return 0;

	return domain->ops->iova_to_phys(domain, iova);
}

static size_t iommu_pgsize(struct iommu_domain *domain,
			   unsigned long addr_merge, size_t size)
{
	unsigned int pgsize_idx;
	size_t pgsize;

	/* Max page size that still fits into 'size' */
	pgsize_idx = __fls(size);

	/* need to consider alignment requirements ? */
	if (likely(addr_merge)) {
		/* Max page size allowed by address */
		unsigned int align_pgsize_idx = __ffs(addr_merge);
		pgsize_idx = min(pgsize_idx, align_pgsize_idx);
	}

	/* build a mask of acceptable page sizes */
	pgsize = (1UL << (pgsize_idx + 1)) - 1;

	/* throw away page sizes not supported by the hardware */
	pgsize &= domain->ops->pgsize_bitmap;

	/* make sure we're still sane */
	BUG_ON(!pgsize);

	/* pick the biggest page */
	pgsize_idx = __fls(pgsize);
	pgsize = 1UL << pgsize_idx;

	return pgsize;
}

int iommu_map(struct iommu_domain *domain, unsigned long iova,
	      phys_addr_t paddr, size_t size, int prot)
{
	unsigned long orig_iova = iova;
	unsigned int min_pagesz;
	size_t orig_size = size;
	int ret = 0;

	if (unlikely(domain->ops->map == NULL ||
		     domain->ops->pgsize_bitmap == 0UL))
		return -ENODEV;

	if (unlikely(!(domain->type & __IOMMU_DOMAIN_PAGING)))
		return -EINVAL;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->ops->pgsize_bitmap);

	/*
	 * both the virtual address and the physical one, as well as
	 * the size of the mapping, must be aligned (at least) to the
	 * size of the smallest page supported by the hardware
	 */
	if (!IS_ALIGNED(iova | paddr | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx pa %pa size 0x%zx min_pagesz 0x%x\n",
		       iova, &paddr, size, min_pagesz);
		return -EINVAL;
	}

	pr_debug("map: iova 0x%lx pa %pa size 0x%zx\n", iova, &paddr, size);

	while (size) {
		size_t pgsize = iommu_pgsize(domain, iova | paddr, size);

		pr_debug("mapping: iova 0x%lx pa %pa pgsize 0x%zx\n",
			 iova, &paddr, pgsize);

		ret = domain->ops->map(domain, iova, paddr, pgsize, prot);
		if (ret)
			break;

		iova += pgsize;
		paddr += pgsize;
		size -= pgsize;
	}

	/* unroll mapping in case something went wrong */
	if (ret)
		iommu_unmap(domain, orig_iova, orig_size - size);

	return ret;
}

size_t iommu_unmap(struct iommu_domain *domain, unsigned long iova, size_t size)
{
	size_t unmapped_page, unmapped = 0;
	unsigned int min_pagesz;

	if (unlikely(domain->ops->unmap == NULL ||
		     domain->ops->pgsize_bitmap == 0UL))
		return -ENODEV;

	if (unlikely(!(domain->type & __IOMMU_DOMAIN_PAGING)))
		return -EINVAL;

	/* find out the minimum page size supported */
	min_pagesz = 1 << __ffs(domain->ops->pgsize_bitmap);

	/*
	 * The virtual address, as well as the size of the mapping, must be
	 * aligned (at least) to the size of the smallest page supported
	 * by the hardware
	 */
	if (!IS_ALIGNED(iova | size, min_pagesz)) {
		pr_err("unaligned: iova 0x%lx size 0x%zx min_pagesz 0x%x\n",
		       iova, size, min_pagesz);
		return -EINVAL;
	}

	pr_debug("unmap this: iova 0x%lx size 0x%zx\n", iova, size);

	/*
	 * Keep iterating until we either unmap 'size' bytes (or more)
	 * or we hit an area that isn't mapped.
	 */
	while (unmapped < size) {
		size_t pgsize = iommu_pgsize(domain, iova, size - unmapped);

		unmapped_page = domain->ops->unmap(domain, iova, pgsize);
		if (!unmapped_page)
			break;

		pr_debug("unmapped: iova 0x%lx size 0x%zx\n",
			 iova, unmapped_page);

		iova += unmapped_page;
		unmapped += unmapped_page;
	}

	return unmapped;
}

/**
 * iommu_group_set_iommudata - set iommu_data for a group
 * @group: the group
 * @iommu_data: new data
 * @release: release function for iommu_data
 *
 * iommu drivers can store data in the group for use when doing iommu
 * operations.  This function provides a way to set the data after
 * the group has been allocated.  Caller should hold a group reference.
 */
void iommu_group_set_iommudata(struct iommu_group *group, void *iommu_data,
			       void (*release)(void *iommu_data))
{
	group->iommu_data = iommu_data;
	group->iommu_data_release = release;
}

/**
 * iommu_group_get_iommudata - retrieve iommu_data registered for a group
 * @group: the group
 *
 * iommu drivers can store data in the group for use when doing iommu
 * operations.  This function provides a way to retrieve it.  Caller
 * should hold a group reference.
 */
void *iommu_group_get_iommudata(struct iommu_group *group)
{
	return group->iommu_data;
}

bool iommu_present(struct bus_type *bus)
{
	return bus->iommu_ops != NULL;
}

/**
 * bus_set_iommu - set iommu-callbacks for the bus
 * @bus: bus.
 * @ops: the callbacks provided by the iommu-driver
 *
 * This function is called by an iommu driver to set the iommu methods
 * used for a particular bus. Drivers for devices on that bus can use
 * the iommu-api after these ops are registered.
 * This special function is needed because IOMMUs are usually devices on
 * the bus itself, so the iommu drivers are not initialized when the bus
 * is set up. With this function the iommu-driver can set the iommu-ops
 * afterwards.
 */
int bus_set_iommu(struct bus_type *bus, const struct iommu_ops *ops)
{
	if (bus->iommu_ops != NULL)
		return -EBUSY;

	bus->iommu_ops = ops;
	return 0;
}

static struct kmem_cache *iova_cache;
static unsigned int iova_cache_users;
static struct mutex iova_cache_mutex;


void
init_iova_domain(struct iova_domain *iovad, unsigned long granule,
	unsigned long start_pfn, unsigned long pfn_32bit)
{
	/*
	 * IOVA granularity will normally be equal to the smallest
	 * supported IOMMU page size; both *must* be capable of
	 * representing individual CPU pages exactly.
	 */
	BUG_ON((granule > PAGE_SIZE) || !is_power_of_2(granule));

	spin_lock_init(&iovad->iova_rbtree_lock);
	iovad->rbroot = RB_ROOT;
	iovad->cached32_node = NULL;
	iovad->granule = granule;
	iovad->start_pfn = start_pfn;
	iovad->dma_32bit_pfn = pfn_32bit;
}

void free_iova_mem(struct iova *iova)
{
	kmem_cache_free(iova_cache, iova);
}

int iova_cache_get(void)
{
	mutex_lock(&iova_cache_mutex);
	if (!iova_cache_users) {
		iova_cache = kmem_cache_create(
			"iommu_iova", sizeof(struct iova), 0,
			SLAB_HWCACHE_ALIGN, NULL);
		if (!iova_cache) {
			mutex_unlock(&iova_cache_mutex);
			printk(KERN_ERR "Couldn't create iova cache\n");
			return -ENOMEM;
		}
	}

	iova_cache_users++;
	mutex_unlock(&iova_cache_mutex);

	return 0;
}

void iova_cache_put(void)
{
	mutex_lock(&iova_cache_mutex);
	BUG_ON(!iova_cache_users);
	iova_cache_users--;
	if (!iova_cache_users)
		kmem_cache_destroy(iova_cache);
	mutex_unlock(&iova_cache_mutex);
}

/**
 * put_iova_domain - destroys the iova doamin
 * @iovad: - iova domain in question.
 * All the iova's in that domain are destroyed.
 */
void put_iova_domain(struct iova_domain *iovad)
{
	struct rb_node *node;
	unsigned long flags;

	spin_lock_irqsave(&iovad->iova_rbtree_lock, flags);
	node = rb_first(&iovad->rbroot);
	while (node) {
		struct iova *iova = container_of(node, struct iova, node);
		rb_erase(node, &iovad->rbroot);
		free_iova_mem(iova);
		node = rb_first(&iovad->rbroot);
	}
	spin_unlock_irqrestore(&iovad->iova_rbtree_lock, flags);
#ifdef __QNXNTO__
	spin_destroy(&iovad->iova_rbtree_lock);
#endif
}

static struct rb_node *
__get_cached_rbnode(struct iova_domain *iovad, unsigned long *limit_pfn)
{
	if ((*limit_pfn != iovad->dma_32bit_pfn) ||
		(iovad->cached32_node == NULL))
		return rb_last(&iovad->rbroot);
	else {
		struct rb_node *prev_node = rb_prev(iovad->cached32_node);
		struct iova *curr_iova =
			container_of(iovad->cached32_node, struct iova, node);
		*limit_pfn = curr_iova->pfn_lo - 1;
		return prev_node;
	}
}

static void
__cached_rbnode_insert_update(struct iova_domain *iovad,
	unsigned long limit_pfn, struct iova *new)
{
	if (limit_pfn != iovad->dma_32bit_pfn)
		return;
	iovad->cached32_node = &new->node;
}

static void
__cached_rbnode_delete_update(struct iova_domain *iovad, struct iova *free)
{
	struct iova *cached_iova;
	struct rb_node *curr;

	if (!iovad->cached32_node)
		return;
	curr = iovad->cached32_node;
	cached_iova = container_of(curr, struct iova, node);

	if (free->pfn_lo >= cached_iova->pfn_lo) {
		struct rb_node *node = rb_next(&free->node);
		struct iova *iova = container_of(node, struct iova, node);

		/* only cache if it's below 32bit pfn */
		if (node && iova->pfn_lo < iovad->dma_32bit_pfn)
			iovad->cached32_node = node;
		else
			iovad->cached32_node = NULL;
	}
}

/* Computes the padding size required, to make the
 * the start address naturally aligned on its size
 */
static int
iova_get_pad_size(int size, unsigned int limit_pfn)
{
	unsigned int pad_size = 0;
	unsigned int order = ilog2(size);

	if (order)
		pad_size = (limit_pfn + 1) % (1 << order);

	return pad_size;
}

static int __alloc_and_insert_iova_range(struct iova_domain *iovad,
		unsigned long size, unsigned long limit_pfn,
			struct iova *new, bool size_aligned)
{
	struct rb_node *prev, *curr = NULL;
	unsigned long flags;
	unsigned long saved_pfn;
	unsigned int pad_size = 0;

	/* Walk the tree backwards */
	spin_lock_irqsave(&iovad->iova_rbtree_lock, flags);
	saved_pfn = limit_pfn;
	curr = __get_cached_rbnode(iovad, &limit_pfn);
	prev = curr;
	while (curr) {
		struct iova *curr_iova = container_of(curr, struct iova, node);

		if (limit_pfn < curr_iova->pfn_lo)
			goto move_left;
		else if (limit_pfn < curr_iova->pfn_hi)
			goto adjust_limit_pfn;
		else {
			if (size_aligned)
				pad_size = iova_get_pad_size(size, limit_pfn);
			if ((curr_iova->pfn_hi + size + pad_size) <= limit_pfn)
				break;	/* found a free slot */
		}
adjust_limit_pfn:
		limit_pfn = curr_iova->pfn_lo - 1;
move_left:
		prev = curr;
		curr = rb_prev(curr);
	}

	if (!curr) {
		if (size_aligned)
			pad_size = iova_get_pad_size(size, limit_pfn);
		if ((iovad->start_pfn + size + pad_size) > limit_pfn) {
			spin_unlock_irqrestore(&iovad->iova_rbtree_lock, flags);
			return -ENOMEM;
		}
	}

	/* pfn_lo will point to size aligned address if size_aligned is set */
	new->pfn_lo = limit_pfn - (size + pad_size) + 1;
	new->pfn_hi = new->pfn_lo + size - 1;

	/* Insert the new_iova into domain rbtree by holding writer lock */
	/* Add new node and rebalance tree. */
	{
		struct rb_node **entry, *parent = NULL;

		/* If we have 'prev', it's a valid place to start the
		   insertion. Otherwise, start from the root. */
		if (prev)
			entry = &prev;
		else
			entry = &iovad->rbroot.rb_node;

		/* Figure out where to put new node */
		while (*entry) {
			struct iova *this = container_of(*entry,
							struct iova, node);
			parent = *entry;

			if (new->pfn_lo < this->pfn_lo)
				entry = &((*entry)->rb_left);
			else if (new->pfn_lo > this->pfn_lo)
				entry = &((*entry)->rb_right);
			else
				BUG(); /* this should not happen */
		}

		/* Add new node and rebalance tree. */
		rb_link_node(&new->node, parent, entry);
		rb_insert_color(&new->node, &iovad->rbroot);
	}
	__cached_rbnode_insert_update(iovad, saved_pfn, new);

	spin_unlock_irqrestore(&iovad->iova_rbtree_lock, flags);


	return 0;
}

struct iova *alloc_iova_mem(void)
{
    struct iova *iovaPtr = NULL;

    iovaPtr = malloc(sizeof(struct iova));
    if (iovaPtr) {
        memset(iovaPtr, 0, sizeof(struct iova));
    }
    return(iovaPtr);
}

/**
 * alloc_iova - allocates an iova
 * @iovad: - iova domain in question
 * @size: - size of page frames to allocate
 * @limit_pfn: - max limit address
 * @size_aligned: - set if size_aligned address range is required
 * This function allocates an iova in the range iovad->start_pfn to limit_pfn,
 * searching top-down from limit_pfn to iovad->start_pfn. If the size_aligned
 * flag is set then the allocated address iova->pfn_lo will be naturally
 * aligned on roundup_power_of_two(size).
 */
struct iova *
alloc_iova(struct iova_domain *iovad, unsigned long size,
	unsigned long limit_pfn,
	bool size_aligned)
{
	struct iova *new_iova;
	int ret;

	new_iova = alloc_iova_mem();
	if (!new_iova)
		return NULL;

	/* If size aligned is set then round the size to
	 * to next power of two.
	 */
	if (size_aligned)
		size = __roundup_pow_of_two(size);

	ret = __alloc_and_insert_iova_range(iovad, size, limit_pfn,
			new_iova, size_aligned);

	if (ret) {
		free_iova_mem(new_iova);
		return NULL;
	}

	return new_iova;
}

/**
 * __free_iova - frees the given iova
 * @iovad: iova domain in question.
 * @iova: iova in question.
 * Frees the given iova belonging to the giving domain
 */
void
__free_iova(struct iova_domain *iovad, struct iova *iova)
{
	unsigned long flags;

	spin_lock_irqsave(&iovad->iova_rbtree_lock, flags);
	__cached_rbnode_delete_update(iovad, iova);
	rb_erase(&iova->node, &iovad->rbroot);
	spin_unlock_irqrestore(&iovad->iova_rbtree_lock, flags);
	free_iova_mem(iova);
}

/**
 * find_iova - find's an iova for a given pfn
 * @iovad: - iova domain in question.
 * @pfn: - page frame number
 * This function finds and returns an iova belonging to the
 * given doamin which matches the given pfn.
 */
struct iova *find_iova(struct iova_domain *iovad, unsigned long pfn)
{
	unsigned long flags;
	struct rb_node *node;

	/* Take the lock so that no other thread is manipulating the rbtree */
	spin_lock_irqsave(&iovad->iova_rbtree_lock, flags);
	node = iovad->rbroot.rb_node;
	while (node) {
		struct iova *iova = container_of(node, struct iova, node);

		/* If pfn falls within iova's range, return iova */
		if ((pfn >= iova->pfn_lo) && (pfn <= iova->pfn_hi)) {
			spin_unlock_irqrestore(&iovad->iova_rbtree_lock, flags);
			/* We are not holding the lock while this iova
			 * is referenced by the caller as the same thread
			 * which called this function also calls __free_iova()
			 * and it is by design that only one thread can possibly
			 * reference a particular iova and hence no conflict.
			 */
			return iova;
		}

		if (pfn < iova->pfn_lo)
			node = node->rb_left;
		else if (pfn > iova->pfn_lo)
			node = node->rb_right;
	}

	spin_unlock_irqrestore(&iovad->iova_rbtree_lock, flags);
	return NULL;
}


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/iommu.c $ $Rev: 876786 $")
#endif
