/*
 * Intel AGPGART routines.
 */

#include <linux/qnx.h>
#include <linux/linux.h>

#include <linux/agp.h>
#include <asm/cacheflush.h>
#include <linux/intel-agp.h>
#include <linux/vmalloc.h>

#include <drm/intel-gtt.h>

int intel_agp_enabled = 1;
EXPORT_SYMBOL(intel_agp_enabled);
struct agp_bridge_data *agp_bridge;
LIST_HEAD(agp_bridges);
int agp_off = 0;
__u32 *agp_gatt_table;
int agp_memory_reserved;

struct agp_bridge_data *agp_alloc_bridge(void)
{
	struct agp_bridge_data *bridge;

	bridge = kzalloc(sizeof(*bridge), GFP_KERNEL);
	if (!bridge)
		return NULL;

	atomic_set(&bridge->agp_in_use, 0);
	atomic_set(&bridge->current_memory_agp, 0);

	if (list_empty(&agp_bridges))
		agp_bridge = bridge;

	return bridge;
}

void agp_put_bridge(struct agp_bridge_data *bridge)
{
        kfree(bridge);

        if (list_empty(&agp_bridges))
                agp_bridge = NULL;
}

#define AGPGART_VERSION_MAJOR 0
#define AGPGART_VERSION_MINOR 103
static const struct agp_version agp_current_version =
{
	.major = AGPGART_VERSION_MAJOR,
	.minor = AGPGART_VERSION_MINOR,
};

static const struct { int mem, agp; } maxes_table[] = {
	{0, 0},
	{32, 4},
	{64, 28},
	{128, 96},
	{256, 204},
	{512, 440},
	{1024, 942},
	{2048, 1920},
	{4096, 3932}
};

static int agp_find_max(void)
{
	long memory, index, result;
#ifndef __QNXNTO__
#if PAGE_SHIFT < 20
	memory = totalram_pages >> (20 - PAGE_SHIFT);
#else
	memory = totalram_pages << (PAGE_SHIFT - 20);
#endif
#else
	BUG();
	qnx_error("this function should have never been called");
	abort();
#endif /* __QNXNTO__ */
	index = 1;

	while ((memory > maxes_table[index].mem) && (index < 8))
		index++;

	result = maxes_table[index - 1].agp +
	   ( (memory - maxes_table[index - 1].mem)  *
	     (maxes_table[index].agp - maxes_table[index - 1].agp)) /
	   (maxes_table[index].mem - maxes_table[index - 1].mem);

	result = result << (20 - PAGE_SHIFT);
	return result;
}


static int agp_backend_initialize(struct agp_bridge_data *bridge)
{
	int size_value, rc, got_gatt=0, got_keylist=0;

	bridge->max_memory_agp = agp_find_max();
	bridge->version = &agp_current_version;

	if (bridge->driver->needs_scratch_page) {
		struct page *page = bridge->driver->agp_alloc_page(bridge);

		if (!page) {
			dev_err(&bridge->dev->dev,
				"can't get memory for scratch page\n");
			return -ENOMEM;
		}

		bridge->scratch_page_page = page;
		bridge->scratch_page_dma = page_to_phys(page);

		bridge->scratch_page = bridge->driver->mask_memory(bridge,
						   bridge->scratch_page_dma, 0);
	}

	size_value = bridge->driver->fetch_size();
	if (size_value == 0) {
		dev_err(&bridge->dev->dev, "can't determine aperture size\n");
		rc = -EINVAL;
		goto err_out;
	}
	if (bridge->driver->create_gatt_table(bridge)) {
		dev_err(&bridge->dev->dev,
			"can't get memory for graphics translation table\n");
		rc = -ENOMEM;
		goto err_out;
	}
	got_gatt = 1;

	bridge->key_list = vzalloc(PAGE_SIZE * 4);
	if (bridge->key_list == NULL) {
		dev_err(&bridge->dev->dev,
			"can't allocate memory for key lists\n");
		rc = -ENOMEM;
		goto err_out;
	}
	got_keylist = 1;

	/* FIXME vmalloc'd memory not guaranteed contiguous */

	if (bridge->driver->configure()) {
		dev_err(&bridge->dev->dev, "error configuring host chipset\n");
		rc = -EINVAL;
		goto err_out;
	}
	INIT_LIST_HEAD(&bridge->mapped_list);
	spin_lock_init(&bridge->mapped_lock);

	return 0;

err_out:
	if (bridge->driver->needs_scratch_page) {
		struct page *page = bridge->scratch_page_page;

		bridge->driver->agp_destroy_page(page, AGP_PAGE_DESTROY_UNMAP);
		bridge->driver->agp_destroy_page(page, AGP_PAGE_DESTROY_FREE);
	}
	if (got_gatt)
		bridge->driver->free_gatt_table(bridge);
	if (got_keylist) {
		vfree(bridge->key_list);
		bridge->key_list = NULL;
	}
	return rc;
}

static void agp_backend_cleanup(struct agp_bridge_data *bridge)
{
	if (bridge->driver->cleanup)
		bridge->driver->cleanup();
	if (bridge->driver->free_gatt_table)
		bridge->driver->free_gatt_table(bridge);

	vfree(bridge->key_list);
	bridge->key_list = NULL;

	if (bridge->driver->agp_destroy_page &&
	    bridge->driver->needs_scratch_page) {
		struct page *page = bridge->scratch_page_page;

		bridge->driver->agp_destroy_page(page, AGP_PAGE_DESTROY_UNMAP);
		bridge->driver->agp_destroy_page(page, AGP_PAGE_DESTROY_FREE);
	}
#ifdef __QNXNTO__
	spin_destroy(&bridge->mapped_lock);
#endif
}

void agp_remove_bridge(struct agp_bridge_data *bridge)
{
	agp_backend_cleanup(bridge);
	list_del(&bridge->list);
}

void agp_free_key(int key)
{
	if (key < 0)
		return;

	if (key < MAXKEY)
		clear_bit(key, agp_bridge->key_list);
}

static int agp_get_key(void)
{
	int bit;

	bit = find_first_zero_bit(agp_bridge->key_list, MAXKEY);
	if (bit < MAXKEY) {
		set_bit(bit, agp_bridge->key_list);
		return bit;
	}
	return -1;
}

void global_cache_flush(void)
{

}
void agp_alloc_page_array(size_t size, struct agp_memory *mem)
{
	mem->pages = NULL;

	if (size <= 2*PAGE_SIZE)
		mem->pages = kmalloc(size, GFP_KERNEL | __GFP_NOWARN);
	if (mem->pages == NULL) {
		mem->pages = vmalloc(size);
	}
}

void agp_free_page_array(struct agp_memory *mem)
{
	if (is_vmalloc_addr(mem->pages)) {
		vfree(mem->pages);
	} else {
		kfree(mem->pages);
	}
}

struct agp_memory *agp_create_memory(int scratch_pages)
{
	struct agp_memory *new;

	new = kzalloc(sizeof(struct agp_memory), GFP_KERNEL);
	if (new == NULL)
		return NULL;

	new->key = agp_get_key();

	if (new->key < 0) {
		kfree(new);
		return NULL;
	}

	agp_alloc_page_array(PAGE_SIZE * scratch_pages, new);

	if (new->pages == NULL) {
		agp_free_key(new->key);
		kfree(new);
		return NULL;
	}
	new->num_scratch_pages = scratch_pages;
	new->type = AGP_NORMAL_MEMORY;
	return new;
}

int agp_add_bridge(struct agp_bridge_data *bridge)
{
	int error;

	if (agp_off) {
		error = -ENODEV;
		goto err_put_bridge;
	}

	if (!bridge->dev) {
		printk (KERN_DEBUG PFX "Erk, registering with no pci_dev!\n");
		error = -EINVAL;
		goto err_put_bridge;
	}

	error = agp_backend_initialize(bridge);
	if (error) {
		goto err_out;
	}

	if (list_empty(&agp_bridges)) {
	//	error = agp_frontend_initialize();
		if (error) {
			goto frontend_err;
		}
	}

	list_add(&bridge->list, &agp_bridges);
	return 0;

frontend_err:
	agp_backend_cleanup(bridge);
err_out:
err_put_bridge:
	agp_put_bridge(bridge);
	return error;
}


static int intel_fetch_size(void)
{
	int i;
	u16 temp;
	struct aper_size_info_16 *values;

	pci_read_config_word(agp_bridge->dev, INTEL_APSIZE, &temp);
	values = A_SIZE_16(agp_bridge->driver->aperture_sizes);

	for (i = 0; i < agp_bridge->driver->num_aperture_sizes; i++) {
		if (temp == values[i].size_value) {
			agp_bridge->previous_size = agp_bridge->current_size = (void *) (values + i);
			agp_bridge->aperture_size_idx = i;
			return values[i].size;
		}
	}

	return 0;
}

static int __intel_8xx_fetch_size(u8 temp)
{
	int i;
	struct aper_size_info_8 *values;

	values = A_SIZE_8(agp_bridge->driver->aperture_sizes);

	for (i = 0; i < agp_bridge->driver->num_aperture_sizes; i++) {
		if (temp == values[i].size_value) {
			agp_bridge->previous_size =
				agp_bridge->current_size = (void *) (values + i);
			agp_bridge->aperture_size_idx = i;
			return values[i].size;
		}
	}
	return 0;
}

static int intel_8xx_fetch_size(void)
{
	u8 temp;

	pci_read_config_byte(agp_bridge->dev, INTEL_APSIZE, &temp);
	return __intel_8xx_fetch_size(temp);
}

static int intel_815_fetch_size(void)
{
	u8 temp;

	/* Intel 815 chipsets have a _weird_ APSIZE register with only
	 * one non-reserved bit, so mask the others out ... */
	pci_read_config_byte(agp_bridge->dev, INTEL_APSIZE, &temp);
	temp &= (1 << 3);

	return __intel_8xx_fetch_size(temp);
}

static void intel_tlbflush(struct agp_memory *mem)
{
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x2200);
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x2280);
}


static void intel_8xx_tlbflush(struct agp_memory *mem)
{
	u32 temp;
	pci_read_config_dword(agp_bridge->dev, INTEL_AGPCTRL, &temp);
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, temp & ~(1 << 7));
	pci_read_config_dword(agp_bridge->dev, INTEL_AGPCTRL, &temp);
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, temp | (1 << 7));
}


static void intel_cleanup(void)
{
	u16 temp;
	struct aper_size_info_16 *previous_size;

	previous_size = A_SIZE_16(agp_bridge->previous_size);
	pci_read_config_word(agp_bridge->dev, INTEL_NBXCFG, &temp);
	pci_write_config_word(agp_bridge->dev, INTEL_NBXCFG, temp & ~(1 << 9));
	pci_write_config_word(agp_bridge->dev, INTEL_APSIZE, previous_size->size_value);
}


static void intel_8xx_cleanup(void)
{
	u16 temp;
	struct aper_size_info_8 *previous_size;

	previous_size = A_SIZE_8(agp_bridge->previous_size);
	pci_read_config_word(agp_bridge->dev, INTEL_NBXCFG, &temp);
	pci_write_config_word(agp_bridge->dev, INTEL_NBXCFG, temp & ~(1 << 9));
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, previous_size->size_value);
}


static int intel_configure(void)
{
	u32 temp;
	u16 temp2;
	struct aper_size_info_16 *current_size;

	current_size = A_SIZE_16(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_word(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x2280);

	/* paccfg/nbxcfg */
	pci_read_config_word(agp_bridge->dev, INTEL_NBXCFG, &temp2);
	pci_write_config_word(agp_bridge->dev, INTEL_NBXCFG,
			(temp2 & ~(1 << 10)) | (1 << 9));
	/* clear any possible error conditions */
	pci_write_config_byte(agp_bridge->dev, INTEL_ERRSTS + 1, 7);
	return 0;
}

static int intel_815_configure(void)
{
	u32 temp, addr;
	u8 temp2;
	struct aper_size_info_8 *current_size;

	/* attbase - aperture base */
	/* the Intel 815 chipset spec. says that bits 29-31 in the
	* ATTBASE register are reserved -> try not to write them */
	if (agp_bridge->gatt_bus_addr & INTEL_815_ATTBASE_MASK) {
		return -EINVAL;
	}

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE,
			current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	pci_read_config_dword(agp_bridge->dev, INTEL_ATTBASE, &addr);
	addr &= INTEL_815_ATTBASE_MASK;
	addr |= agp_bridge->gatt_bus_addr;
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* apcont */
	pci_read_config_byte(agp_bridge->dev, INTEL_815_APCONT, &temp2);
	pci_write_config_byte(agp_bridge->dev, INTEL_815_APCONT, temp2 | (1 << 1));

	/* clear any possible error conditions */
	/* Oddness : this chipset seems to have no ERRSTS register ! */
	return 0;
}

static void intel_820_tlbflush(struct agp_memory *mem)
{
	return;
}

static void intel_820_cleanup(void)
{
	u8 temp;
	struct aper_size_info_8 *previous_size;

	previous_size = A_SIZE_8(agp_bridge->previous_size);
	pci_read_config_byte(agp_bridge->dev, INTEL_I820_RDCR, &temp);
	pci_write_config_byte(agp_bridge->dev, INTEL_I820_RDCR,
			temp & ~(1 << 1));
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE,
			previous_size->size_value);
}


static int intel_820_configure(void)
{
	u32 temp;
	u8 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* global enable aperture access */
	/* This flag is not accessed through MCHCFG register as in */
	/* i850 chipset. */
	pci_read_config_byte(agp_bridge->dev, INTEL_I820_RDCR, &temp2);
	pci_write_config_byte(agp_bridge->dev, INTEL_I820_RDCR, temp2 | (1 << 1));
	/* clear any possible AGP-related error conditions */
	pci_write_config_word(agp_bridge->dev, INTEL_I820_ERRSTS, 0x001c);
	return 0;
}

static int intel_840_configure(void)
{
	u32 temp;
	u16 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* mcgcfg */
	pci_read_config_word(agp_bridge->dev, INTEL_I840_MCHCFG, &temp2);
	pci_write_config_word(agp_bridge->dev, INTEL_I840_MCHCFG, temp2 | (1 << 9));
	/* clear any possible error conditions */
	pci_write_config_word(agp_bridge->dev, INTEL_I840_ERRSTS, 0xc000);
	return 0;
}

static int intel_845_configure(void)
{
	u32 temp;
	u8 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	if (agp_bridge->apbase_config != 0) {
		pci_write_config_dword(agp_bridge->dev, AGP_APBASE,
				       agp_bridge->apbase_config);
	} else {
		/* address to map to */
		pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
		agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);
		agp_bridge->apbase_config = temp;
	}

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* agpm */
	pci_read_config_byte(agp_bridge->dev, INTEL_I845_AGPM, &temp2);
	pci_write_config_byte(agp_bridge->dev, INTEL_I845_AGPM, temp2 | (1 << 1));
	/* clear any possible error conditions */
	pci_write_config_word(agp_bridge->dev, INTEL_I845_ERRSTS, 0x001c);
	return 0;
}

static int intel_850_configure(void)
{
	u32 temp;
	u16 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* mcgcfg */
	pci_read_config_word(agp_bridge->dev, INTEL_I850_MCHCFG, &temp2);
	pci_write_config_word(agp_bridge->dev, INTEL_I850_MCHCFG, temp2 | (1 << 9));
	/* clear any possible AGP-related error conditions */
	pci_write_config_word(agp_bridge->dev, INTEL_I850_ERRSTS, 0x001c);
	return 0;
}

static int intel_860_configure(void)
{
	u32 temp;
	u16 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* mcgcfg */
	pci_read_config_word(agp_bridge->dev, INTEL_I860_MCHCFG, &temp2);
	pci_write_config_word(agp_bridge->dev, INTEL_I860_MCHCFG, temp2 | (1 << 9));
	/* clear any possible AGP-related error conditions */
	pci_write_config_word(agp_bridge->dev, INTEL_I860_ERRSTS, 0xf700);
	return 0;
}

static int intel_830mp_configure(void)
{
	u32 temp;
	u16 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* gmch */
	pci_read_config_word(agp_bridge->dev, INTEL_NBXCFG, &temp2);
	pci_write_config_word(agp_bridge->dev, INTEL_NBXCFG, temp2 | (1 << 9));
	/* clear any possible AGP-related error conditions */
	pci_write_config_word(agp_bridge->dev, INTEL_I830_ERRSTS, 0x1c);
	return 0;
}

static int intel_7505_configure(void)
{
	u32 temp;
	u16 temp2;
	struct aper_size_info_8 *current_size;

	current_size = A_SIZE_8(agp_bridge->current_size);

	/* aperture size */
	pci_write_config_byte(agp_bridge->dev, INTEL_APSIZE, current_size->size_value);

	/* address to map to */
	pci_read_config_dword(agp_bridge->dev, AGP_APBASE, &temp);
	agp_bridge->gart_bus_addr = (temp & PCI_BASE_ADDRESS_MEM_MASK);

	/* attbase - aperture base */
	pci_write_config_dword(agp_bridge->dev, INTEL_ATTBASE, agp_bridge->gatt_bus_addr);

	/* agpctrl */
	pci_write_config_dword(agp_bridge->dev, INTEL_AGPCTRL, 0x0000);

	/* mchcfg */
	pci_read_config_word(agp_bridge->dev, INTEL_I7505_MCHCFG, &temp2);
	pci_write_config_word(agp_bridge->dev, INTEL_I7505_MCHCFG, temp2 | (1 << 9));

	return 0;
}

//TODO: stubs
void get_agp_version(struct agp_bridge_data *bridge)
{
	u32 ncapid;

	/* Exit early if already set by errata workarounds. */
	if (bridge->major_version != 0)
		return;

	pci_read_config_dword(bridge->dev, bridge->capndx, &ncapid);
	bridge->major_version = (ncapid >> AGP_MAJOR_VERSION_SHIFT) & 0xf;
	bridge->minor_version = (ncapid >> AGP_MINOR_VERSION_SHIFT) & 0xf;
}
static void agp_v2_parse_one(u32 *requested_mode, u32 *bridge_agpstat, u32 *vga_agpstat)
{
	u32 tmp;

	if (*requested_mode & AGP2_RESERVED_MASK) {
		printk(KERN_INFO PFX "reserved bits set (%x) in mode 0x%x. Fixed.\n",
			*requested_mode & AGP2_RESERVED_MASK, *requested_mode);
		*requested_mode &= ~AGP2_RESERVED_MASK;
	}

	/*
	 * Some dumb bridges are programmed to disobey the AGP2 spec.
	 * This is likely a BIOS misprogramming rather than poweron default, or
	 * it would be a lot more common.
	 * https://bugs.freedesktop.org/show_bug.cgi?id=8816
	 * AGPv2 spec 6.1.9 states:
	 *   The RATE field indicates the data transfer rates supported by this
	 *   device. A.G.P. devices must report all that apply.
	 * Fix them up as best we can.
	 */
	switch (*bridge_agpstat & 7) {
	case 4:
		*bridge_agpstat |= (AGPSTAT2_2X | AGPSTAT2_1X);
		printk(KERN_INFO PFX "BIOS bug. AGP bridge claims to only support x4 rate. "
			"Fixing up support for x2 & x1\n");
		break;
	case 2:
		*bridge_agpstat |= AGPSTAT2_1X;
		printk(KERN_INFO PFX "BIOS bug. AGP bridge claims to only support x2 rate. "
			"Fixing up support for x1\n");
		break;
	default:
		break;
	}

	/* Check the speed bits make sense. Only one should be set. */
	tmp = *requested_mode & 7;
	switch (tmp) {
		case 0:
			printk(KERN_INFO PFX "%s tried to set rate=x0. Setting to x1 mode.\n", current->comm);
			*requested_mode |= AGPSTAT2_1X;
			break;
		case 1:
		case 2:
			break;
		case 3:
			*requested_mode &= ~(AGPSTAT2_1X);	/* rate=2 */
			break;
		case 4:
			break;
		case 5:
		case 6:
		case 7:
			*requested_mode &= ~(AGPSTAT2_1X|AGPSTAT2_2X); /* rate=4*/
			break;
	}

	/* disable SBA if it's not supported */
	if (!((*bridge_agpstat & AGPSTAT_SBA) && (*vga_agpstat & AGPSTAT_SBA) && (*requested_mode & AGPSTAT_SBA)))
		*bridge_agpstat &= ~AGPSTAT_SBA;

	/* Set rate */
	if (!((*bridge_agpstat & AGPSTAT2_4X) && (*vga_agpstat & AGPSTAT2_4X) && (*requested_mode & AGPSTAT2_4X)))
		*bridge_agpstat &= ~AGPSTAT2_4X;

	if (!((*bridge_agpstat & AGPSTAT2_2X) && (*vga_agpstat & AGPSTAT2_2X) && (*requested_mode & AGPSTAT2_2X)))
		*bridge_agpstat &= ~AGPSTAT2_2X;

	if (!((*bridge_agpstat & AGPSTAT2_1X) && (*vga_agpstat & AGPSTAT2_1X) && (*requested_mode & AGPSTAT2_1X)))
		*bridge_agpstat &= ~AGPSTAT2_1X;

	/* Now we know what mode it should be, clear out the unwanted bits. */
	if (*bridge_agpstat & AGPSTAT2_4X)
		*bridge_agpstat &= ~(AGPSTAT2_1X | AGPSTAT2_2X);	/* 4X */

	if (*bridge_agpstat & AGPSTAT2_2X)
		*bridge_agpstat &= ~(AGPSTAT2_1X | AGPSTAT2_4X);	/* 2X */

	if (*bridge_agpstat & AGPSTAT2_1X)
		*bridge_agpstat &= ~(AGPSTAT2_2X | AGPSTAT2_4X);	/* 1X */

	/* Apply any errata. */
	if (agp_bridge->flags & AGP_ERRATA_FASTWRITES)
		*bridge_agpstat &= ~AGPSTAT_FW;

	if (agp_bridge->flags & AGP_ERRATA_SBA)
		*bridge_agpstat &= ~AGPSTAT_SBA;

	if (agp_bridge->flags & AGP_ERRATA_1X) {
		*bridge_agpstat &= ~(AGPSTAT2_2X | AGPSTAT2_4X);
		*bridge_agpstat |= AGPSTAT2_1X;
	}

	/* If we've dropped down to 1X, disable fast writes. */
	if (*bridge_agpstat & AGPSTAT2_1X)
		*bridge_agpstat &= ~AGPSTAT_FW;
}

static void agp_v3_parse_one(u32 *requested_mode, u32 *bridge_agpstat, u32 *vga_agpstat)
{
	u32 origbridge=*bridge_agpstat, origvga=*vga_agpstat;
	u32 tmp;

	if (*requested_mode & AGP3_RESERVED_MASK) {
		printk(KERN_INFO PFX "reserved bits set (%x) in mode 0x%x. Fixed.\n",
			*requested_mode & AGP3_RESERVED_MASK, *requested_mode);
		*requested_mode &= ~AGP3_RESERVED_MASK;
	}

	/* Check the speed bits make sense. */
	tmp = *requested_mode & 7;
	if (tmp == 0) {
		printk(KERN_INFO PFX "%s tried to set rate=x0. Setting to AGP3 x4 mode.\n", current->comm);
		*requested_mode |= AGPSTAT3_4X;
	}
	if (tmp >= 3) {
		printk(KERN_INFO PFX "%s tried to set rate=x%d. Setting to AGP3 x8 mode.\n", current->comm, tmp * 4);
		*requested_mode = (*requested_mode & ~7) | AGPSTAT3_8X;
	}

	/* ARQSZ - Set the value to the maximum one.
	 * Don't allow the mode register to override values. */
	*bridge_agpstat = ((*bridge_agpstat & ~AGPSTAT_ARQSZ) |
		max_t(u32,(*bridge_agpstat & AGPSTAT_ARQSZ),(*vga_agpstat & AGPSTAT_ARQSZ)));

	/* Calibration cycle.
	 * Don't allow the mode register to override values. */
	*bridge_agpstat = ((*bridge_agpstat & ~AGPSTAT_CAL_MASK) |
		min_t(u32,(*bridge_agpstat & AGPSTAT_CAL_MASK),(*vga_agpstat & AGPSTAT_CAL_MASK)));

	/* SBA *must* be supported for AGP v3 */
	*bridge_agpstat |= AGPSTAT_SBA;

	/*
	 * Set speed.
	 * Check for invalid speeds. This can happen when applications
	 * written before the AGP 3.0 standard pass AGP2.x modes to AGP3 hardware
	 */
	if (*requested_mode & AGPSTAT_MODE_3_0) {
		/*
		 * Caller hasn't a clue what it is doing. Bridge is in 3.0 mode,
		 * have been passed a 3.0 mode, but with 2.x speed bits set.
		 * AGP2.x 4x -> AGP3.0 4x.
		 */
		if (*requested_mode & AGPSTAT2_4X) {
			printk(KERN_INFO PFX "%s passes broken AGP3 flags (%x). Fixed.\n",
						current->comm, *requested_mode);
			*requested_mode &= ~AGPSTAT2_4X;
			*requested_mode |= AGPSTAT3_4X;
		}
	} else {
		/*
		 * The caller doesn't know what they are doing. We are in 3.0 mode,
		 * but have been passed an AGP 2.x mode.
		 * Convert AGP 1x,2x,4x -> AGP 3.0 4x.
		 */
		printk(KERN_INFO PFX "%s passes broken AGP2 flags (%x) in AGP3 mode. Fixed.\n",
					current->comm, *requested_mode);
		*requested_mode &= ~(AGPSTAT2_4X | AGPSTAT2_2X | AGPSTAT2_1X);
		*requested_mode |= AGPSTAT3_4X;
	}

	if (*requested_mode & AGPSTAT3_8X) {
		if (!(*bridge_agpstat & AGPSTAT3_8X)) {
			*bridge_agpstat &= ~(AGPSTAT3_8X | AGPSTAT3_RSVD);
			*bridge_agpstat |= AGPSTAT3_4X;
			printk(KERN_INFO PFX "%s requested AGPx8 but bridge not capable.\n", current->comm);
			return;
		}
		if (!(*vga_agpstat & AGPSTAT3_8X)) {
			*bridge_agpstat &= ~(AGPSTAT3_8X | AGPSTAT3_RSVD);
			*bridge_agpstat |= AGPSTAT3_4X;
			printk(KERN_INFO PFX "%s requested AGPx8 but graphic card not capable.\n", current->comm);
			return;
		}
		/* All set, bridge & device can do AGP x8*/
		*bridge_agpstat &= ~(AGPSTAT3_4X | AGPSTAT3_RSVD);
		goto done;

	} else if (*requested_mode & AGPSTAT3_4X) {
		*bridge_agpstat &= ~(AGPSTAT3_8X | AGPSTAT3_RSVD);
		*bridge_agpstat |= AGPSTAT3_4X;
		goto done;

	} else {

		/*
		 * If we didn't specify an AGP mode, we see if both
		 * the graphics card, and the bridge can do x8, and use if so.
		 * If not, we fall back to x4 mode.
		 */
		if ((*bridge_agpstat & AGPSTAT3_8X) && (*vga_agpstat & AGPSTAT3_8X)) {
			printk(KERN_INFO PFX "No AGP mode specified. Setting to highest mode "
				"supported by bridge & card (x8).\n");
			*bridge_agpstat &= ~(AGPSTAT3_4X | AGPSTAT3_RSVD);
			*vga_agpstat &= ~(AGPSTAT3_4X | AGPSTAT3_RSVD);
		} else {
			printk(KERN_INFO PFX "Fell back to AGPx4 mode because ");
			if (!(*bridge_agpstat & AGPSTAT3_8X)) {
				printk(KERN_INFO PFX "bridge couldn't do x8. bridge_agpstat:%x (orig=%x)\n",
					*bridge_agpstat, origbridge);
				*bridge_agpstat &= ~(AGPSTAT3_8X | AGPSTAT3_RSVD);
				*bridge_agpstat |= AGPSTAT3_4X;
			}
			if (!(*vga_agpstat & AGPSTAT3_8X)) {
				printk(KERN_INFO PFX "graphics card couldn't do x8. vga_agpstat:%x (orig=%x)\n",
					*vga_agpstat, origvga);
				*vga_agpstat &= ~(AGPSTAT3_8X | AGPSTAT3_RSVD);
				*vga_agpstat |= AGPSTAT3_4X;
			}
		}
	}

done:
	/* Apply any errata. */
	if (agp_bridge->flags & AGP_ERRATA_FASTWRITES)
		*bridge_agpstat &= ~AGPSTAT_FW;

	if (agp_bridge->flags & AGP_ERRATA_SBA)
		*bridge_agpstat &= ~AGPSTAT_SBA;

	if (agp_bridge->flags & AGP_ERRATA_1X) {
		*bridge_agpstat &= ~(AGPSTAT2_2X | AGPSTAT2_4X);
		*bridge_agpstat |= AGPSTAT2_1X;
	}
}


u32 agp_collect_device_status(struct agp_bridge_data *bridge, u32 requested_mode, u32 bridge_agpstat)
{
	struct pci_dev *device = NULL;
	u32 vga_agpstat;
	u8 cap_ptr;

	for (;;) {
		device = pci_get_class(PCI_CLASS_DISPLAY_VGA << 8, device);
		if (!device) {
			printk(KERN_INFO PFX "Couldn't find an AGP VGA controller.\n");
			return 0;
		}
		cap_ptr = pci_find_capability(device, PCI_CAP_ID_AGP);
		if (cap_ptr)
			break;
	}

	/*
	 * Ok, here we have a AGP device. Disable impossible
	 * settings, and adjust the readqueue to the minimum.
	 */
	pci_read_config_dword(device, cap_ptr+PCI_AGP_STATUS, &vga_agpstat);

	/* adjust RQ depth */
	bridge_agpstat = ((bridge_agpstat & ~AGPSTAT_RQ_DEPTH) |
	     min_t(u32, (requested_mode & AGPSTAT_RQ_DEPTH),
		 min_t(u32, (bridge_agpstat & AGPSTAT_RQ_DEPTH), (vga_agpstat & AGPSTAT_RQ_DEPTH))));

	/* disable FW if it's not supported */
	if (!((bridge_agpstat & AGPSTAT_FW) &&
		 (vga_agpstat & AGPSTAT_FW) &&
		 (requested_mode & AGPSTAT_FW)))
		bridge_agpstat &= ~AGPSTAT_FW;

	/* Check to see if we are operating in 3.0 mode */
	if (agp_bridge->mode & AGPSTAT_MODE_3_0)
		agp_v3_parse_one(&requested_mode, &bridge_agpstat, &vga_agpstat);
	else
		agp_v2_parse_one(&requested_mode, &bridge_agpstat, &vga_agpstat);

	pci_dev_put(device);
	return bridge_agpstat;
}

void agp_generic_enable(struct agp_bridge_data *bridge, u32 requested_mode)
{
	u32 bridge_agpstat, temp;

	get_agp_version(agp_bridge);

	pci_read_config_dword(agp_bridge->dev,
		      agp_bridge->capndx + PCI_AGP_STATUS, &bridge_agpstat);

	bridge_agpstat = agp_collect_device_status(agp_bridge, requested_mode, bridge_agpstat);
	if (bridge_agpstat == 0)
		/* Something bad happened. FIXME: Return error code? */
		return;

	bridge_agpstat |= AGPSTAT_AGP_ENABLE;

	/* Do AGP version specific frobbing. */
	if (bridge->major_version >= 3) {
		if (bridge->mode & AGPSTAT_MODE_3_0) {
			/* If we have 3.5, we can do the isoch stuff. */
			if (bridge->minor_version >= 5)
				//agp_3_5_enable(bridge);
			agp_device_command(bridge_agpstat, true);
			return;
		} else {
		    /* Disable calibration cycle in RX91<1> when not in AGP3.0 mode of operation.*/
		    bridge_agpstat &= ~(7<<10) ;
		    pci_read_config_dword(bridge->dev,
					bridge->capndx+AGPCTRL, &temp);
		    temp |= (1<<9);
		    pci_write_config_dword(bridge->dev,
					bridge->capndx+AGPCTRL, temp);

		}
	}

	/* AGP v<3 */
	agp_device_command(bridge_agpstat, false);
}

void agp_device_command(u32 bridge_agpstat, bool agp_v3)
{
	struct pci_dev *device = NULL;
	int mode;

	mode = bridge_agpstat & 0x7;
	if (agp_v3)
		mode *= 4;

	for_each_pci_dev(device) {
		u8 agp = pci_find_capability(device, PCI_CAP_ID_AGP);
		if (!agp)
			continue;

		pci_write_config_dword(device, agp + PCI_AGP_COMMAND, bridge_agpstat);
	}
}

int agp_generic_create_gatt_table(struct agp_bridge_data *bridge)
{
	char *table;
	char *table_end;
	int size;
	int page_order;
	int num_entries;
	int i;
	void *temp;
	struct page *page;

	(void)size;
	/* The generic routines can't handle 2 level gatt's */
	if (bridge->driver->size_type == LVL2_APER_SIZE)
		return -EINVAL;

	table = NULL;
	i = bridge->aperture_size_idx;
	temp = bridge->current_size;
	size = page_order = num_entries = 0;

	if (bridge->driver->size_type != FIXED_APER_SIZE) {
		do {
			switch (bridge->driver->size_type) {
			case U8_APER_SIZE:
				size = A_SIZE_8(temp)->size;
				page_order =
				    A_SIZE_8(temp)->page_order;
				num_entries =
				    A_SIZE_8(temp)->num_entries;
				break;
			case U16_APER_SIZE:
				size = A_SIZE_16(temp)->size;
				page_order = A_SIZE_16(temp)->page_order;
				num_entries = A_SIZE_16(temp)->num_entries;
				break;
			case U32_APER_SIZE:
				size = A_SIZE_32(temp)->size;
				page_order = A_SIZE_32(temp)->page_order;
				num_entries = A_SIZE_32(temp)->num_entries;
				break;
				/* This case will never really happen. */
			case FIXED_APER_SIZE:
			case LVL2_APER_SIZE:
			default:
				size = page_order = num_entries = 0;
				break;
			}

#ifndef __QNXNTO__
			table = alloc_gatt_pages(page_order);
#else
			BUG();
			qnx_error("this function should have never been called");
			abort();
#endif /* __QNXNTO__ */

			if (table == NULL) {
				i++;
				switch (bridge->driver->size_type) {
				case U8_APER_SIZE:
					bridge->current_size = A_IDX8(bridge);
					break;
				case U16_APER_SIZE:
					bridge->current_size = A_IDX16(bridge);
					break;
				case U32_APER_SIZE:
					bridge->current_size = A_IDX32(bridge);
					break;
				/* These cases will never really happen. */
				case FIXED_APER_SIZE:
				case LVL2_APER_SIZE:
				default:
					break;
				}
				temp = bridge->current_size;
			} else {
				bridge->aperture_size_idx = i;
			}
		} while (!table && (i < bridge->driver->num_aperture_sizes));
	} else {
		size = ((struct aper_size_info_fixed *) temp)->size;
		page_order = ((struct aper_size_info_fixed *) temp)->page_order;
		num_entries = ((struct aper_size_info_fixed *) temp)->num_entries;
#ifndef __QNXNTO__
		table = alloc_gatt_pages(page_order);
#else
		BUG();
		qnx_error("this function should have never been called");
		abort();
#endif /* __QNXNTO__ */
	}

	if (table == NULL)
		return -ENOMEM;

	table_end = table + ((PAGE_SIZE * (1 << page_order)) - 1);

#ifndef __QNXNTO__
	for (page = virt_to_page(table); page <= virt_to_page(table_end); page++)
		SetPageReserved(page);
#else
	BUG();
	qnx_error("this function should have never been called");
	abort();
#endif /* __QNXNTO__ */

	bridge->gatt_table_real = (u32 *) table;
	agp_gatt_table = (void *)table;

	bridge->driver->cache_flush();
#ifdef CONFIG_X86
	if (set_memory_uc((unsigned long)table, 1 << page_order))
		printk(KERN_WARNING "Could not set GATT table memory to UC!\n");

	bridge->gatt_table = (u32 __iomem *)table;
#else
	bridge->gatt_table = ioremap(virt_to_phys(table),
					(PAGE_SIZE * (1 << page_order)));
	bridge->driver->cache_flush();
#endif

	if (bridge->gatt_table == NULL) {

#ifndef __QNXNTO__
		for (page = virt_to_page(table); page <= virt_to_page(table_end); page++)
			ClearPageReserved(page);

		free_gatt_pages(table, page_order);
#else
		BUG();
		qnx_error("this function should have never been called");
		abort();
#endif /* __QNXNTO__ */
		return -ENOMEM;
	}
	bridge->gatt_bus_addr = virt_to_phys(bridge->gatt_table_real);

	/* AK: bogus, should encode addresses > 4GB */
	for (i = 0; i < num_entries; i++) {
		writel(bridge->scratch_page, bridge->gatt_table+i);
		readl(bridge->gatt_table+i);	/* PCI Posting. */
	}

	return 0;
}

int agp_generic_free_gatt_table(struct agp_bridge_data *bridge)
{
	int page_order;
	char *table, *table_end;
	void *temp;
	struct page *page;

	temp = bridge->current_size;

	switch (bridge->driver->size_type) {
	case U8_APER_SIZE:
		page_order = A_SIZE_8(temp)->page_order;
		break;
	case U16_APER_SIZE:
		page_order = A_SIZE_16(temp)->page_order;
		break;
	case U32_APER_SIZE:
		page_order = A_SIZE_32(temp)->page_order;
		break;
	case FIXED_APER_SIZE:
		page_order = A_SIZE_FIX(temp)->page_order;
		break;
	case LVL2_APER_SIZE:
		/* The generic routines can't deal with 2 level gatt's */
		return -EINVAL;
	default:
		page_order = 0;
		break;
	}

	/* Do not worry about freeing memory, because if this is
	 * called, then all agp memory is deallocated and removed
	 * from the table. */

#ifdef CONFIG_X86
	set_memory_wb((unsigned long)bridge->gatt_table, 1 << page_order);
#else
	iounmap(bridge->gatt_table,(PAGE_SIZE * (1 << page_order)));
#endif
	table = (char *) bridge->gatt_table_real;
	table_end = table + ((PAGE_SIZE * (1 << page_order)) - 1);

#ifndef __QNXNTO__
	for (page = virt_to_page(table); page <= virt_to_page(table_end); page++)
		ClearPageReserved(page);

	free_gatt_pages(bridge->gatt_table_real, page_order);
#else
	BUG();
	qnx_error("this function should have never been called");
	abort();
#endif /* __QNXNTO__ */

	agp_gatt_table = NULL;
	bridge->gatt_table = NULL;
	bridge->gatt_table_real = NULL;
	bridge->gatt_bus_addr = 0;

	return 0;
}


int agp_generic_insert_memory(struct agp_memory * mem, off_t pg_start, int type)
{
	int num_entries;
	size_t i;
	off_t j;
	void *temp;
	struct agp_bridge_data *bridge;
	int mask_type;

	bridge = mem->bridge;
	if (!bridge)
		return -EINVAL;

	if (mem->page_count == 0)
		return 0;

	temp = bridge->current_size;

	switch (bridge->driver->size_type) {
	case U8_APER_SIZE:
		num_entries = A_SIZE_8(temp)->num_entries;
		break;
	case U16_APER_SIZE:
		num_entries = A_SIZE_16(temp)->num_entries;
		break;
	case U32_APER_SIZE:
		num_entries = A_SIZE_32(temp)->num_entries;
		break;
	case FIXED_APER_SIZE:
		num_entries = A_SIZE_FIX(temp)->num_entries;
		break;
	case LVL2_APER_SIZE:
		/* The generic routines can't deal with 2 level gatt's */
		return -EINVAL;
	default:
		num_entries = 0;
		break;
	}

	num_entries -= agp_memory_reserved/PAGE_SIZE;
	if (num_entries < 0) num_entries = 0;

	if (type != mem->type)
		return -EINVAL;

	mask_type = bridge->driver->agp_type_to_mask_type(bridge, type);
	if (mask_type != 0) {
		/* The generic routines know nothing of memory types */
		return -EINVAL;
	}

	if (((pg_start + mem->page_count) > num_entries) ||
	    ((pg_start + mem->page_count) < pg_start))
		return -EINVAL;

	j = pg_start;

	while (j < (pg_start + mem->page_count)) {
		if (!PGE_EMPTY(bridge, readl(bridge->gatt_table+j)))
			return -EBUSY;
		j++;
	}

	if (!mem->is_flushed) {
		bridge->driver->cache_flush();
		mem->is_flushed = true;
	}

	for (i = 0, j = pg_start; i < mem->page_count; i++, j++) {
		writel(bridge->driver->mask_memory(bridge,
						   page_to_phys(mem->pages[i]),
						   mask_type),
		       bridge->gatt_table+j);
	}
	readl(bridge->gatt_table+j-1);	/* PCI Posting. */

	bridge->driver->tlb_flush(mem);
	return 0;
}

int agp_num_entries(void)
{
	int num_entries;
	void *temp;

	temp = agp_bridge->current_size;

	switch (agp_bridge->driver->size_type) {
	case U8_APER_SIZE:
		num_entries = A_SIZE_8(temp)->num_entries;
		break;
	case U16_APER_SIZE:
		num_entries = A_SIZE_16(temp)->num_entries;
		break;
	case U32_APER_SIZE:
		num_entries = A_SIZE_32(temp)->num_entries;
		break;
	case LVL2_APER_SIZE:
		num_entries = A_SIZE_LVL2(temp)->num_entries;
		break;
	case FIXED_APER_SIZE:
		num_entries = A_SIZE_FIX(temp)->num_entries;
		break;
	default:
		num_entries = 0;
		break;
	}

	num_entries -= agp_memory_reserved>>PAGE_SHIFT;
	if (num_entries<0)
		num_entries = 0;
	return num_entries;
}

int agp_generic_remove_memory(struct agp_memory *mem, off_t pg_start, int type)
{
	size_t i;
	struct agp_bridge_data *bridge;
	int mask_type, num_entries;

	bridge = mem->bridge;
	if (!bridge)
		return -EINVAL;

	if (mem->page_count == 0)
		return 0;

	if (type != mem->type)
		return -EINVAL;

	num_entries = agp_num_entries();
	if (((pg_start + mem->page_count) > num_entries) ||
	    ((pg_start + mem->page_count) < pg_start))
		return -EINVAL;

	mask_type = bridge->driver->agp_type_to_mask_type(bridge, type);
	if (mask_type != 0) {
		/* The generic routines know nothing of memory types */
		return -EINVAL;
	}

	/* AK: bogus, should encode addresses > 4GB */
	for (i = pg_start; i < (mem->page_count + pg_start); i++) {
		writel(bridge->scratch_page, bridge->gatt_table+i);
	}
	readl(bridge->gatt_table+i-1);	/* PCI Posting. */

	bridge->driver->tlb_flush(mem);
	return 0;
}
struct agp_memory *agp_generic_alloc_by_type(size_t page_count, int type)
{
	return NULL;
}

void agp_generic_free_by_type(struct agp_memory *curr)
{
	agp_free_page_array(curr);
	agp_free_key(curr->key);
	kfree(curr);
}

struct page *agp_generic_alloc_page(struct agp_bridge_data *bridge)
{
	struct page * page;

	page = alloc_page(GFP_KERNEL | GFP_DMA32 | __GFP_ZERO);
	if (page == NULL)
		return NULL;

	map_page_into_agp(page);

	get_page(page);
	atomic_inc(&agp_bridge->current_memory_agp);
	return page;
}

int agp_generic_alloc_pages(struct agp_bridge_data *bridge, struct agp_memory *mem, size_t num_pages)
{
	struct page * page;
	int i, ret = -ENOMEM;

	for (i = 0; i < num_pages; i++) {
		page = alloc_page(GFP_KERNEL | GFP_DMA32 | __GFP_ZERO);
		/* agp_free_memory() needs gart address */
		if (page == NULL)
			goto out;

#ifndef CONFIG_X86
		map_page_into_agp(page);
#endif
		get_page(page);
		atomic_inc(&agp_bridge->current_memory_agp);

		mem->pages[i] = page;
		mem->page_count++;
	}

#ifdef CONFIG_X86
	set_pages_array_uc(mem->pages, num_pages);
#endif
	ret = 0;
out:
	return ret;
}
void agp_generic_destroy_page(struct page *page, int flags)
{
	if (page == NULL)
		return;

	if (flags & AGP_PAGE_DESTROY_UNMAP)
		unmap_page_from_agp(page);

	if (flags & AGP_PAGE_DESTROY_FREE) {
		put_page(page);
		free(page);
		atomic_dec(&agp_bridge->current_memory_agp);
	}
}

void agp_generic_destroy_pages(struct agp_memory *mem)
{
	int i;
	struct page *page;

	if (!mem)
		return;

#ifdef CONFIG_X86
	set_pages_array_wb(mem->pages, mem->page_count);
#endif

	for (i = 0; i < mem->page_count; i++) {
		page = mem->pages[i];

#ifndef CONFIG_X86
		unmap_page_from_agp(page);
#endif
		put_page(page);
		free(page);
		atomic_dec(&agp_bridge->current_memory_agp);
		mem->pages[i] = NULL;
	}
}

unsigned long agp_generic_mask_memory(struct agp_bridge_data *bridge,
				      dma_addr_t addr, int type)
{
	/* memory type is ignored in the generic routine */
	if (bridge->driver->masks)
		return addr | bridge->driver->masks[0].mask;
	else
		return addr;
}

int agp_generic_type_to_mask_type(struct agp_bridge_data *bridge,
				  int type)
{
	if (type >= AGP_USER_TYPES)
		return 0;
	return type;
}


/* Setup function */
static const struct gatt_mask intel_generic_masks[] =
{
	{.mask = 0x00000017, .type = 0}
};

static const struct aper_size_info_8 intel_815_sizes[2] =
{
	{64, 16384, 4, 0},
	{32, 8192, 3, 8},
};

static const struct aper_size_info_8 intel_8xx_sizes[7] =
{
	{256, 65536, 6, 0},
	{128, 32768, 5, 32},
	{64, 16384, 4, 48},
	{32, 8192, 3, 56},
	{16, 4096, 2, 60},
	{8, 2048, 1, 62},
	{4, 1024, 0, 63}
};

static const struct aper_size_info_16 intel_generic_sizes[7] =
{
	{256, 65536, 6, 0},
	{128, 32768, 5, 32},
	{64, 16384, 4, 48},
	{32, 8192, 3, 56},
	{16, 4096, 2, 60},
	{8, 2048, 1, 62},
	{4, 1024, 0, 63}
};

static const struct aper_size_info_8 intel_830mp_sizes[4] =
{
	{256, 65536, 6, 0},
	{128, 32768, 5, 32},
	{64, 16384, 4, 48},
	{32, 8192, 3, 56}
};

static const struct agp_bridge_driver intel_generic_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_generic_sizes,
	.size_type		= U16_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_configure,
	.fetch_size		= intel_fetch_size,
	.cleanup		= intel_cleanup,
	.tlb_flush		= intel_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_815_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_815_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 2,
	.needs_scratch_page	= true,
	.configure		= intel_815_configure,
	.fetch_size		= intel_815_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type	= agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_820_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_8xx_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_820_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_820_cleanup,
	.tlb_flush		= intel_820_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_830mp_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_830mp_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 4,
	.needs_scratch_page	= true,
	.configure		= intel_830mp_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_840_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_8xx_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_840_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_845_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_8xx_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_845_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_850_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_8xx_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_850_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_860_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_8xx_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_860_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

static const struct agp_bridge_driver intel_7505_driver = {
	.owner			= THIS_MODULE,
	.aperture_sizes		= intel_8xx_sizes,
	.size_type		= U8_APER_SIZE,
	.num_aperture_sizes	= 7,
	.needs_scratch_page	= true,
	.configure		= intel_7505_configure,
	.fetch_size		= intel_8xx_fetch_size,
	.cleanup		= intel_8xx_cleanup,
	.tlb_flush		= intel_8xx_tlbflush,
	.mask_memory		= agp_generic_mask_memory,
	.masks			= intel_generic_masks,
	.agp_enable		= agp_generic_enable,
	.cache_flush		= global_cache_flush,
	.create_gatt_table	= agp_generic_create_gatt_table,
	.free_gatt_table	= agp_generic_free_gatt_table,
	.insert_memory		= agp_generic_insert_memory,
	.remove_memory		= agp_generic_remove_memory,
	.alloc_by_type		= agp_generic_alloc_by_type,
	.free_by_type		= agp_generic_free_by_type,
	.agp_alloc_page		= agp_generic_alloc_page,
	.agp_alloc_pages        = agp_generic_alloc_pages,
	.agp_destroy_page	= agp_generic_destroy_page,
	.agp_destroy_pages      = agp_generic_destroy_pages,
	.agp_type_to_mask_type  = agp_generic_type_to_mask_type,
};

/* Table to describe Intel GMCH and AGP/PCIE GART drivers.  At least one of
 * driver and gmch_driver must be non-null, and find_gmch will determine
 * which one should be used if a gmch_chip_id is present.
 */
static const struct intel_agp_driver_description {
	unsigned int chip_id;
	char *name;
	const struct agp_bridge_driver *driver;
} intel_agp_chipsets[] = {
	{ PCI_DEVICE_ID_INTEL_82443LX_0, "440LX", &intel_generic_driver },
	{ PCI_DEVICE_ID_INTEL_82443BX_0, "440BX", &intel_generic_driver },
	{ PCI_DEVICE_ID_INTEL_82443GX_0, "440GX", &intel_generic_driver },
	{ PCI_DEVICE_ID_INTEL_82815_MC, "i815", &intel_815_driver },
	{ PCI_DEVICE_ID_INTEL_82820_HB, "i820", &intel_820_driver },
	{ PCI_DEVICE_ID_INTEL_82820_UP_HB, "i820", &intel_820_driver },
	{ PCI_DEVICE_ID_INTEL_82830_HB, "830M", &intel_830mp_driver },
	{ PCI_DEVICE_ID_INTEL_82840_HB, "i840", &intel_840_driver },
	{ PCI_DEVICE_ID_INTEL_82845_HB, "i845", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_82845G_HB, "845G", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_82850_HB, "i850", &intel_850_driver },
	{ PCI_DEVICE_ID_INTEL_82854_HB, "854", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_82855PM_HB, "855PM", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_82855GM_HB, "855GM", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_82860_HB, "i860", &intel_860_driver },
	{ PCI_DEVICE_ID_INTEL_82865_HB, "865", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_82875_HB, "i875", &intel_845_driver },
	{ PCI_DEVICE_ID_INTEL_7505_0, "E7505", &intel_7505_driver },
	{ PCI_DEVICE_ID_INTEL_7205_0, "E7205", &intel_7505_driver },
	{ 0, NULL, NULL }
};

static int agp_intel_probe(struct pci_dev *pdev,
				     const struct pci_device_id *ent)
{
	struct agp_bridge_data *bridge;
	u8 cap_ptr = 0;
	struct resource *r;
	int i, err;

	cap_ptr = pci_find_capability(pdev, PCI_CAP_ID_AGP);

	bridge = agp_alloc_bridge();
	if (!bridge)
		return -ENOMEM;

	bridge->capndx = cap_ptr;

	if (intel_gmch_probe(pdev, NULL, bridge))
		goto found_gmch;

	for (i = 0; intel_agp_chipsets[i].name != NULL; i++) {
		/* In case that multiple models of gfx chip may
		   stand on same host bridge type, this can be
		   sure we detect the right IGD. */
		if (pdev->device == intel_agp_chipsets[i].chip_id) {
			bridge->driver = intel_agp_chipsets[i].driver;
			break;
		}
	}

	if (!bridge->driver) {
		if (cap_ptr)
		agp_put_bridge(bridge);
		return -ENODEV;
	}

	bridge->dev = pdev;
	bridge->dev_private_data = NULL;

	/*
	* The following fixes the case where the BIOS has "forgotten" to
	* provide an address range for the GART.
	* 20030610 - hamish@zot.org
	* This happens before pci_enable_device() intentionally;
	* calling pci_enable_device() before assigning the resource
	* will result in the GART being disabled on machines with such
	* BIOSs (the GART ends up with a BAR starting at 0, which
	* conflicts a lot of other devices).
	*/
	r = &pdev->resource[0];
	if (!r->start && r->end) {
		if (pci_assign_resource(pdev, 0)) {
			dev_err(&pdev->dev, "can't assign resource 0\n");
			agp_put_bridge(bridge);
			return -ENODEV;
		}
	}

	/*
	* If the device has not been properly setup, the following will catch
	* the problem and should stop the system from crashing.
	* 20030610 - hamish@zot.org
	*/
	if (pci_enable_device(pdev)) {
		dev_err(&pdev->dev, "can't enable PCI device\n");
		agp_put_bridge(bridge);
		return -ENODEV;
	}

	/* Fill in the mode register */
	if (cap_ptr) {
		pci_read_config_dword(pdev,
				bridge->capndx+PCI_AGP_STATUS,
				&bridge->mode);
	}

found_gmch:
	// TODO: commented
//	pci_set_drvdata(pdev, bridge);
	err = agp_add_bridge(bridge);
	if (!err)
		intel_agp_enabled = 1;
	return err;
}

static void agp_intel_remove(struct pci_dev *pdev)
{
	struct agp_bridge_data *bridge = pci_get_drvdata(pdev);

	agp_remove_bridge(bridge);

	intel_gmch_remove();

	agp_put_bridge(bridge);
}

#ifdef CONFIG_PM
static int agp_intel_resume(struct pci_dev *pdev)
{
	struct agp_bridge_data *bridge = pci_get_drvdata(pdev);

	bridge->driver->configure();

	return 0;
}
#endif

static struct pci_device_id agp_intel_pci_table[] = {
#define ID(x)						\
	{						\
	.class		= (PCI_CLASS_BRIDGE_HOST << 8),	\
	.class_mask	= ~0,				\
	.vendor		= PCI_VENDOR_ID_INTEL,		\
	.device		= x,				\
	.subvendor	= PCI_ANY_ID,			\
	.subdevice	= PCI_ANY_ID,			\
	}
	ID(PCI_DEVICE_ID_INTEL_82441), /* for HAS2 support */
	ID(PCI_DEVICE_ID_INTEL_82443LX_0),
	ID(PCI_DEVICE_ID_INTEL_82443BX_0),
	ID(PCI_DEVICE_ID_INTEL_82443GX_0),
	ID(PCI_DEVICE_ID_INTEL_82810_MC1),
	ID(PCI_DEVICE_ID_INTEL_82810_MC3),
	ID(PCI_DEVICE_ID_INTEL_82810E_MC),
	ID(PCI_DEVICE_ID_INTEL_82815_MC),
	ID(PCI_DEVICE_ID_INTEL_82820_HB),
	ID(PCI_DEVICE_ID_INTEL_82820_UP_HB),
	ID(PCI_DEVICE_ID_INTEL_82830_HB),
	ID(PCI_DEVICE_ID_INTEL_82840_HB),
	ID(PCI_DEVICE_ID_INTEL_82845_HB),
	ID(PCI_DEVICE_ID_INTEL_82845G_HB),
	ID(PCI_DEVICE_ID_INTEL_82850_HB),
	ID(PCI_DEVICE_ID_INTEL_82854_HB),
	ID(PCI_DEVICE_ID_INTEL_82855PM_HB),
	ID(PCI_DEVICE_ID_INTEL_82855GM_HB),
	ID(PCI_DEVICE_ID_INTEL_82860_HB),
	ID(PCI_DEVICE_ID_INTEL_82865_HB),
	ID(PCI_DEVICE_ID_INTEL_82875_HB),
	ID(PCI_DEVICE_ID_INTEL_7505_0),
	ID(PCI_DEVICE_ID_INTEL_7205_0),
	ID(PCI_DEVICE_ID_INTEL_E7221_HB),
	ID(PCI_DEVICE_ID_INTEL_82915G_HB),
	ID(PCI_DEVICE_ID_INTEL_82915GM_HB),
	ID(PCI_DEVICE_ID_INTEL_82945G_HB),
	ID(PCI_DEVICE_ID_INTEL_82945GM_HB),
	ID(PCI_DEVICE_ID_INTEL_82945GME_HB),
	ID(PCI_DEVICE_ID_INTEL_PINEVIEW_M_HB),
	ID(PCI_DEVICE_ID_INTEL_PINEVIEW_HB),
	ID(PCI_DEVICE_ID_INTEL_82946GZ_HB),
	ID(PCI_DEVICE_ID_INTEL_82G35_HB),
	ID(PCI_DEVICE_ID_INTEL_82965Q_HB),
	ID(PCI_DEVICE_ID_INTEL_82965G_HB),
	ID(PCI_DEVICE_ID_INTEL_82965GM_HB),
	ID(PCI_DEVICE_ID_INTEL_82965GME_HB),
	ID(PCI_DEVICE_ID_INTEL_G33_HB),
	ID(PCI_DEVICE_ID_INTEL_Q35_HB),
	ID(PCI_DEVICE_ID_INTEL_Q33_HB),
	ID(PCI_DEVICE_ID_INTEL_GM45_HB),
	ID(PCI_DEVICE_ID_INTEL_EAGLELAKE_HB),
	ID(PCI_DEVICE_ID_INTEL_Q45_HB),
	ID(PCI_DEVICE_ID_INTEL_G45_HB),
	ID(PCI_DEVICE_ID_INTEL_G41_HB),
	ID(PCI_DEVICE_ID_INTEL_B43_HB),
	ID(PCI_DEVICE_ID_INTEL_B43_1_HB),
	ID(PCI_DEVICE_ID_INTEL_IRONLAKE_D_HB),
	ID(PCI_DEVICE_ID_INTEL_IRONLAKE_D2_HB),
	ID(PCI_DEVICE_ID_INTEL_IRONLAKE_M_HB),
	ID(PCI_DEVICE_ID_INTEL_IRONLAKE_MA_HB),
	ID(PCI_DEVICE_ID_INTEL_IRONLAKE_MC2_HB),
	{ }
};


static struct pci_driver agp_intel_pci_driver = {
	.name		= "agpgart-intel",
	.id_table	= agp_intel_pci_table,
	.probe		= agp_intel_probe,
	.remove		= agp_intel_remove,
#ifdef CONFIG_PM
	.resume		= agp_intel_resume,
#endif
};

int agp_intel_init(void)
{
	if (agp_off)
		return -EINVAL;
	return pci_register_driver(&agp_intel_pci_driver);
}

void  agp_intel_cleanup(void)
{
	pci_unregister_driver(&agp_intel_pci_driver);
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/intel-agp.c $ $Rev: 845340 $")
#endif
