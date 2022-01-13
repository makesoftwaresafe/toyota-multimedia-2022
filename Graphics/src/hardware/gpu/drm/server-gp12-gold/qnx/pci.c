#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/device.h>

#include <sys/neutrino.h>

#include <pci/cap_pmi.h>
#include <pci/cap_pcie.h>
#include <pci/cap_msi.h>
#include <pci/cap_msix.h>

#include <drm/drmP.h>

#include "base.h"

/* For PCI or other memory-mapped resources */
unsigned long pci_mem_start = 0xaeedbabe;

const char *pci_power_names[] = {
	"error", "D0", "D1", "D2", "D3hot", "D3cold", "unknown",
};

struct pci_dynid {
    struct list_head node;
    struct pci_device_id id;
};

static const struct pci_device_id pci_device_id_any = {
	.vendor = PCI_ANY_ID,
	.device = PCI_ANY_ID,
	.subvendor = PCI_ANY_ID,
	.subdevice = PCI_ANY_ID,
};

LIST_HEAD(pci_root_buses);
static LIST_HEAD(pci_domain_busn_res_list);

struct pci_domain_busn_res {
	struct list_head list;
	struct resource res;
	int domain_nr;
};

struct pci_bus *pci_bus_get(struct pci_bus *bus)
{
	if (bus)
		get_device(&bus->dev);
	return bus;
}
EXPORT_SYMBOL(pci_bus_get);

void pci_bus_put(struct pci_bus *bus)
{
	if (bus)
		put_device(&bus->dev);
}
EXPORT_SYMBOL(pci_bus_put);

/**
 * pci_match_id - See if a pci device matches a given pci_id table
 * @ids: array of PCI device id structures to search in
 * @dev: the PCI device structure to match against.
 *
 * Used by a driver to check whether a PCI device present in the
 * system is in its list of supported devices.  Returns the matching
 * pci_device_id structure or %NULL if there is no match.
 *
 * Deprecated, don't use this as it will not catch any dynamic ids
 * that a driver might want to check for.
 */
const struct pci_device_id *pci_match_id(const struct pci_device_id *ids,
					 struct pci_dev *dev)
{
	if (ids) {
		while (ids->vendor || ids->subvendor || ids->class_mask) {
			if (pci_match_one_device(ids, dev))
				return ids;
			ids++;
		}
	}
	return NULL;
}

/**
 * pci_match_device - Tell if a PCI device structure has a matching PCI device id structure
 * @drv: the PCI driver to match against
 * @dev: the PCI device structure to match against
 *
 * Used by a driver to check whether a PCI device present in the
 * system is in its list of supported devices.  Returns the matching
 * pci_device_id structure or %NULL if there is no match.
 */
static const struct pci_device_id *pci_match_device(struct pci_driver *drv,
						    struct pci_dev *dev)
{
	struct pci_dynid *dynid;
	const struct pci_device_id *found_id = NULL;

	/* When driver_override is set, only bind to the matching driver */
	if (dev->driver_override && strcmp(dev->driver_override, drv->name))
		return NULL;

	/* Look at the dynamic ids first, before the static ones */
	spin_lock(&drv->dynids.lock);
	list_for_each_entry(dynid, &drv->dynids.list, node) {
		if (pci_match_one_device(&dynid->id, dev)) {
			found_id = &dynid->id;
			break;
		}
	}
	spin_unlock(&drv->dynids.lock);

	if (!found_id)
		found_id = pci_match_id(drv->id_table, dev);

	/* driver_override will always match, send a dummy id */
	if (!found_id && dev->driver_override)
		found_id = &pci_device_id_any;

	return found_id;
}

struct drv_dev_and_id {
	struct pci_driver *drv;
	struct pci_dev *dev;
	const struct pci_device_id *id;
};

static long local_pci_probe(void *_ddi)
{
	struct drv_dev_and_id *ddi = _ddi;
	struct pci_dev *pci_dev = ddi->dev;
	struct pci_driver *pci_drv = ddi->drv;
	struct device *dev = &pci_dev->dev;
	int rc;

	/*
	 * Unbound PCI devices are always put in D0, regardless of
	 * runtime PM status.  During probe, the device is set to
	 * active and the usage count is incremented.  If the driver
	 * supports runtime PM, it should call pm_runtime_put_noidle()
	 * in its probe routine and pm_runtime_get_noresume() in its
	 * remove routine.
	 */
	pm_runtime_get_sync(dev);
	pci_dev->driver = pci_drv;
	rc = pci_drv->probe(pci_dev, ddi->id);
	if (!rc)
		return rc;
	if (rc < 0) {
		pci_dev->driver = NULL;
		pm_runtime_put_sync(dev);
		return rc;
	}
	/*
	 * Probe function should return < 0 for failure, 0 for success
	 * Treat values > 0 as success, but warn.
	 */
	dev_warn(dev, "Driver probe function unexpectedly returned %d\n", rc);
	return 0;
}


static int pci_call_probe(struct pci_driver *drv, struct pci_dev *dev,
			  const struct pci_device_id *id)
{
	int error;
	struct drv_dev_and_id ddi = { drv, dev, id };

	error = local_pci_probe(&ddi);

	return error;
}

/**
 * __pci_device_probe - check if a driver wants to claim a specific PCI device
 * @drv: driver to call to check if it wants the PCI device
 * @pci_dev: PCI device being probed
 *
 * returns 0 on success, else error.
 * side-effect: pci_dev->driver is set to drv when drv claims pci_dev.
 */
static int __pci_device_probe(struct pci_driver *drv, struct pci_dev *pci_dev)
{
	const struct pci_device_id *id;
	int error = 0;

	if (!pci_dev->driver && drv->probe) {
		error = -ENODEV;

		id = pci_match_device(drv, pci_dev);
		if (id)
			error = pci_call_probe(drv, pci_dev, id);
		if (error >= 0)
			error = 0;
	}
	return error;
}

static int pci_device_probe(struct device *dev)
{
	int error = 0;
	struct pci_driver *drv;
	struct pci_dev *pci_dev;

	drv = to_pci_driver(dev->driver);
	pci_dev = to_pci_dev(dev);
	pci_dev_get(pci_dev);
	error = __pci_device_probe(drv, pci_dev);
	if (error)
		pci_dev_put(pci_dev);

	return error;
}

/**
 * pci_bus_match - Tell if a PCI device structure has a matching PCI device id structure
 * @dev: the PCI device structure to match against
 * @drv: the device driver to search for matching PCI device id structures
 *
 * Used by a driver to check whether a PCI device present in the
 * system is in its list of supported devices. Returns the matching
 * pci_device_id structure or %NULL if there is no match.
 */
static int pci_bus_match(struct device *dev, struct device_driver *drv)
{
	struct pci_dev *pci_dev = to_pci_dev(dev);
	struct pci_driver *pci_drv;
	const struct pci_device_id *found_id;

	if (!pci_dev->match_driver)
		return 0;

	pci_drv = to_pci_driver(drv);
	found_id = pci_match_device(pci_drv, pci_dev);
	if (found_id)
		return 1;

	return 0;
}

static int pci_device_remove(struct device *dev)
{
	struct pci_dev *pci_dev = to_pci_dev(dev);
	struct pci_driver *drv = pci_dev->driver;

	if (drv) {
		if (drv->remove) {
			pm_runtime_get_sync(dev);
			drv->remove(pci_dev);
			pm_runtime_put_noidle(dev);
		}
		pci_dev->driver = NULL;
	}

	/* Undo the runtime PM settings in local_pci_probe() */
	pm_runtime_put_sync(dev);

	/*
	 * If the device is still on, set the power state as "unknown",
	 * since it might change by the next time we load the driver.
	 */
	if (pci_dev->current_state == PCI_D0)
		pci_dev->current_state = PCI_UNKNOWN;

	/*
	 * We would love to complain here if pci_dev->is_enabled is set, that
	 * the driver should have called pci_disable_device(), but the
	 * unfortunate fact is there are too many odd BIOS and bridge setups
	 * that don't like drivers doing that all of the time.
	 * Oh well, we can dream of sane hardware when we sleep, no matter how
	 * horrible the crap we have to deal with is when we are awake...
	 */

	pci_dev_put(pci_dev);
	return 0;
}

struct bus_type pci_bus_type = {
	.name		= "pci",
	.match		= pci_bus_match,
	.uevent		= NULL,
	.probe		= pci_device_probe,
	.remove		= pci_device_remove,
	.shutdown	= NULL,
	.dev_groups	= NULL,
	.bus_groups	= NULL,
	.drv_groups	= NULL,
	.pm		= NULL,
};

#ifndef _NTO_TCTL_IO_PRIV
#define _NTO_TCTL_IO_PRIV _NTO_TCTL_IO
#endif

#define NOMMU_MAPPING_ERROR		0

void *dma_generic_alloc_coherent(struct device *dev, size_t size,
				 dma_addr_t *dma_addr, gfp_t flag,
				 unsigned long attrs)
{
	BUG();
	return NULL;
}

void dma_generic_free_coherent(struct device *dev, size_t size, void *vaddr,
			       dma_addr_t dma_addr, unsigned long attrs)
{
	BUG();
	return;
}

static int
check_addr(char *name, struct device *hwdev, dma_addr_t bus, size_t size)
{
	if (hwdev && !dma_capable(hwdev, bus, size)) {
		if (*hwdev->dma_mask >= DMA_BIT_MASK(32))
			printk(KERN_ERR
			    "nommu_%s: overflow %Lx+%zu of device mask %Lx\n",
				name, (long long)bus, size,
				(long long)*hwdev->dma_mask);
		return 0;
	}
	return 1;
}

static dma_addr_t nommu_map_page(struct device *dev, struct page *page,
				 unsigned long offset, size_t size,
				 enum dma_data_direction dir,
				 unsigned long attrs)
{
	dma_addr_t bus = phys_to_dma(dev, page_to_phys(page)) + offset;
	WARN_ON(size == 0);
	if (!check_addr("map_single", dev, bus, size))
		return NOMMU_MAPPING_ERROR;
	flush_write_buffers();
	return bus;
}

/* Map a set of buffers described by scatterlist in streaming
 * mode for DMA.  This is the scatter-gather version of the
 * above pci_map_single interface.  Here the scatter gather list
 * elements are each tagged with the appropriate dma address
 * and length.  They are obtained via sg_dma_{address,length}(SG).
 *
 * NOTE: An implementation may be able to use a smaller number of
 *       DMA address/length pairs than there are SG table elements.
 *       (for example via virtual mapping capabilities)
 *       The routine returns the number of addr/length pairs actually
 *       used, at most nents.
 *
 * Device ownership issues as mentioned above for pci_map_single are
 * the same here.
 */
static int nommu_map_sg(struct device *hwdev, struct scatterlist *sg,
			int nents, enum dma_data_direction dir,
			unsigned long attrs)
{
	struct scatterlist *s;
	int i;

	WARN_ON(nents == 0 || sg[0].length == 0);

	for_each_sg(sg, s, nents, i) {
		BUG_ON(!sg_page(s));
		s->dma_address = sg_phys(s);
		if (!check_addr("map_sg", hwdev, s->dma_address, s->length))
			return 0;
		s->dma_length = s->length;
	}
	flush_write_buffers();
	return nents;
}

static void nommu_sync_single_for_device(struct device *dev,
			dma_addr_t addr, size_t size,
			enum dma_data_direction dir)
{
	flush_write_buffers();
}

static void nommu_sync_sg_for_device(struct device *dev,
			struct scatterlist *sg, int nelems,
			enum dma_data_direction dir)
{
	flush_write_buffers();
}

static int nommu_mapping_error(struct device *dev, dma_addr_t dma_addr)
{
	return dma_addr == NOMMU_MAPPING_ERROR;
}

int x86_dma_supported(struct device *dev, u64 mask)
{
	return 1;
}

struct dma_map_ops nommu_dma_ops = {
	.alloc			= dma_generic_alloc_coherent,
	.free			= dma_generic_free_coherent,
	.map_sg			= nommu_map_sg,
	.map_page		= nommu_map_page,
	.sync_single_for_device = nommu_sync_single_for_device,
	.sync_sg_for_device	= nommu_sync_sg_for_device,
	.is_phys		= 1,
	.mapping_error		= nommu_mapping_error,
	.dma_supported		= x86_dma_supported,
};

struct dma_map_ops *dma_ops = &nommu_dma_ops;

int pcibus_class_init(void);

static int pci_read(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 *value)
{
	BUG();
	return 0;
}

static int pci_write(struct pci_bus *bus, unsigned int devfn, int where, int size, u32 value)
{
	BUG();
	return 0;
}

struct pci_ops pci_root_ops = {
	.read = pci_read,
	.write = pci_write,
};

int qnx_pci_init(void)
{
    pcibus_class_init();
    bus_register(&pci_bus_type);

    LIST_HEAD(resources);
    struct pci_bus *bus;

    bus = pci_scan_root_bus(NULL, 0, &pci_root_ops, NULL, &resources);
    if (!bus) {
        return 0;
    }

    pci_bus_add_devices(bus);

    return 0;
}

void qnx_pci_fini(void)
{
    return;
}

int pci_read_config_byte(const struct pci_dev *dev, int where, u8 *val)
{
    pci_err_t err = PCI_ERR_OK;
    pci_bdf_t bdf;
    pci_devhdl_t handle = NULL;
    pci_ccode_t ccode;

    bdf = PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn));

    if (where < 0x40) {
        int data_read = 1;

        if (dev->hdl == NULL) {
            handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
        } else {
            handle = dev->hdl;
        }
        if (handle == NULL) {
            /* If somebody owns this particular PCI device, pretend that it is not exist in system */
            if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error is %d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
            }
            return PCIBIOS_DEVICE_NOT_FOUND;
        }

        switch(where) {
            case PCI_CAPABILITY_LIST: /* 0x34 */
                 err = pci_device_cfg_rd34(handle, val);
                 break;
            case PCI_HEADER_TYPE: /* 0x0E */
                 err = pci_device_read_ccode(bdf, &ccode);
                 if (err == PCI_ERR_OK) {
                     if ((ccode >> 8) == PCI_CLASS_BRIDGE_PCI) {
                         /* Special case for PCI-to-PCI bridge */
                         *val = PCI_HEADER_TYPE_BRIDGE;
                     } else {
                         *val = PCI_HEADER_TYPE_NORMAL;
                     }
                 }
                 break;
            case PCI_INTERRUPT_LINE:
                 {
                     int_t ntempirqs = 1;
                     pci_irq_t irq = 0xFF;

                     err = pci_device_read_irq(handle, &ntempirqs, &irq);
                     if (err == PCI_ERR_OK) {
                         *val = irq & 0x000000FF;
                         break;
                     }
                     if (err == PCI_ERR_IRQ_NOT_AVAIL) {
                         /* It is not an error, generic IRQ is just not available for this device (SkyLake, for example). */
                         *val = 0xFF;
                     }
                 }
                 break;
            case PCI_INTERRUPT_PIN:
                 /* Set INTA# interrupt line by default */
                 *val = 0x01;
                 break;
            default:
                 dev_err(&dev->dev, "Attempt to read byte at forbidden PCI configuration space: %04X\n", where);
                 data_read = 0;
                 break;
        }

        if (dev->hdl == NULL) {
            pci_device_detach(handle);
        }
        if (data_read) {
            /* PCI errors match errno as is */
            return -err;
        }
    }

    return -pci_device_cfg_rd8(bdf, where, val);
}

int pci_read_config_word(const struct pci_dev *dev, int where, u16 *val)
{
    pci_err_t err = PCI_ERR_OK;
    pci_bdf_t bdf;
    pci_devhdl_t handle = NULL;

    bdf = PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn));

    if (where < 0x40) {
        int data_read = 1;

        if (dev->hdl == NULL) {
            handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
        } else {
            handle = dev->hdl;
        }
        if (handle == NULL) {
            /* If somebody owns this particular PCI device, pretend that it is not exist in system */
            if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error is %d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
            }
            return PCIBIOS_DEVICE_NOT_FOUND;
        }

        switch(where) {
            case PCI_SUBSYSTEM_VENDOR_ID: /* 0x2C */
                 {
                     pci_ssvid_t ssvid;

                     err = pci_device_read_ssvid(bdf, &ssvid);
                     if (err == PCI_ERR_OK) {
                         *val = ssvid & 0x0000FFFF;
                     } else {
                         *val = 0xFFFF;
                     }
                 }
                 return -err;
                 break;
            case PCI_SUBSYSTEM_ID: /* 0x2E */
                 {
                     pci_ssid_t ssid;
                     err = pci_device_read_ssid(bdf, &ssid);
                     if (err == PCI_ERR_OK) {
                         *val = ssid & 0x0000FFFF;
                     } else {
                         *val = 0xFFFF;
                     }
                 }
                 break;
            default:
                 dev_err(&dev->dev, "Attempt to read word at forbidden PCI configuration space: %04X\n", where);
                 data_read = 0;
                 break;
        }

        if (dev->hdl == NULL) {
            pci_device_detach(handle);
        }
        if (data_read) {
            /* PCI errors match errno as is */
            return -err;
        }
    }

    return -pci_device_cfg_rd16(bdf, where, val);
}

int pci_read_config_dword(const struct pci_dev *dev, int where, u32 *val)
{
    uint32_t data32;
    uint16_t data16;
    uint8_t  data8;
    pci_bdf_t bdf;
    pci_err_t err;
    pci_devhdl_t handle = NULL;

    bdf = PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn));

    if (where < 0x40) {
        switch(where) {
            case PCI_VENDOR_ID: /* 0x00 */
                 err = pci_device_read_vid(bdf, &data16);
                 if (err == PCI_ERR_OK) {
                     data32 = data16;
                     err = pci_device_read_did(bdf, &data16);
                     if (err == PCI_ERR_OK) {
                         data32 |= ((uint32_t)data16) << 16;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_COMMAND: /* 0x04 */
                 err = pci_device_read_cmd(bdf, (pci_cmd_t*)&data16);
                 if (err == PCI_ERR_OK) {
                     data32 = data16;
                     err = pci_device_read_status(bdf, (pci_stat_t*)&data16);
                     if (err == PCI_ERR_OK) {
                         data32 |= ((uint32_t)data16) << 16;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_CLASS_REVISION: /* 0x08 */
                 err = pci_device_read_ccode(bdf, (pci_ccode_t*)&data32);
                 if (err == PCI_ERR_OK) {
                     data32 <<= 8;
                     err = pci_device_read_revid(bdf, (pci_revid_t*)&data8);
                     if (err == PCI_ERR_OK) {
                         data32 |= data8;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_CACHE_LINE_SIZE: /* 0x0C */
                 err = pci_device_read_clsize(bdf, (pci_clsize_t*)&data8);
                 if (err == PCI_ERR_OK) {
                     data32 = data8;
                 } else {
                     /* This field is hardwired to 0s. The IGD as a PCI-compliant master does */
                     /* not use the Memory Write and Invalidate command and, in general, does */
                     /* not perform operations based on cache line size.                      */
                     data32 = 0x00000000;
                 }
                 err = pci_device_read_latency(bdf, (pci_latency_t*)&data8);
                 if (err == PCI_ERR_OK) {
                     data32 |= ((uint32_t)data8) << 8;
                 } else {
                     /* Master Latency Timer Count Value (MLTCV): Hardwired to 0s on IGDs. */
                     data32 |= 0x00000000;
                 }

                 /* Header Code. */
                 {
                     pci_ccode_t ccode;
                     err = pci_device_read_ccode(bdf, &ccode);
                     if (err == PCI_ERR_OK) {
                         if ((ccode >> 8) == PCI_CLASS_BRIDGE_PCI) {
                             /* Special case for PCI-to-PCI bridge */
                             data32 |= (PCI_HEADER_TYPE_BRIDGE) << 16;
                         } else {
                             data32 |= (PCI_HEADER_TYPE_NORMAL) << 16;
                         }
                     }
                 }

                 /* BIST is always zero too */
                 data32 |= 0x00000000;
                 *val = data32;
                 return -EOK;
            case PCI_BASE_ADDRESS_0: /* 0x10 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     if (dev->hdl == NULL) {
                         handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error %d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR0 (and BAR1 if address is 64 bit) */
                     pcibar.bar_num = 0;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                            switch (pcibar.type) {
                                case pci_asType_e_NONE:
                                     /* If BAR0 is NONE just return zero address */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_IO:
                                     /* If BAR0 is IO then it is 16 bit, just copy it and mark as IO */
                                     *val = pcibar.addr | 0x00000001;
                                     break;
                                case pci_asType_e_MEM:
                                     /* Copy memory address add bits */
                                     *val = pcibar.addr;
                                     if (pcibar.attr & pci_asAttr_e_64BIT) {
                                         /* If BAR0 MEM is 64 bit, add 64 bit flag */
                                         *val |= 0x00000004;
                                     }
                                     if (pcibar.attr & pci_asAttr_e_PREFETCH) {
                                         /* If BAR0 MEM is prefetchable add corresponding flag */
                                         *val |= 0x00000008;
                                     }
                                     break;
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_BASE_ADDRESS_1: /* 0x14 */
                 {
                     pci_ba_t pcibar[2];
                     int_t nbars = 2;

                     if (dev->hdl == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                         }
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR1 and BAR0 in case if address is 64 bit */
                     pcibar[0].bar_num = 0;
                     pcibar[1].bar_num = 1;
                     err = pci_device_read_ba(handle, &nbars, &pcibar[0], pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if ((pcibar[1].type == pci_asType_e_MEM) || (pcibar[1].type == pci_asType_e_IO)) {
                            /* Ok, it is pure 32 bit address or IO for BAR1 */
                            *val = pcibar[1].addr;
                            if (pcibar[1].type == pci_asType_e_IO) {
                                /* Mark it as IO */
                                *val |= 0x00000001;
                            }
                            if (pcibar[1].type == pci_asType_e_MEM) {
                                 if (pcibar[1].attr & pci_asAttr_e_PREFETCH) {
                                     /* If BAR1 MEM is prefetchable add corresponding flag */
                                     *val |= 0x00000008;
                                 }
                            }
                        } else {
                            /* BAR1 failed or empty, analyze BAR0 content */
                            switch (pcibar[0].type) {
                                case pci_asType_e_NONE:
                                case pci_asType_e_IO:
                                     /* If BAR0 is IO or NONE, then BAR1 was reported right */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_MEM:
                                     if (pcibar[0].attr & (pci_asAttr_e_32BIT | pci_asAttr_e_16BIT)) {
                                         /* If BAR0 MEM is 16 or 32 bit, then BAR1 was reported right */
                                         *val = 0x00000000;
                                     }
                                     if (pcibar[0].attr & pci_asAttr_e_64BIT) {
                                         /* Store MSB part of 64 bit address */
                                         *val = pcibar[0].addr >> 32;
                                     }
                                     break;
                              }
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_BASE_ADDRESS_2: /* 0x18 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     if (dev->hdl == NULL) {
                         handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR2 (and BAR3 if address is 64 bit) */
                     pcibar.bar_num = 2;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                            switch (pcibar.type) {
                                case pci_asType_e_NONE:
                                     /* If BAR2 is NONE just return zero address */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_IO:
                                     /* If BAR2 is IO then it is 16 bit, just copy it and mark as IO */
                                     *val = pcibar.addr | 0x00000001;
                                     break;
                                case pci_asType_e_MEM:
                                     /* Copy memory address add bits */
                                     *val = pcibar.addr;
                                     if (pcibar.attr & pci_asAttr_e_64BIT) {
                                         /* If BAR2 MEM is 64 bit, add 64 bit flag */
                                         *val |= 0x00000004;
                                     }
                                     if (pcibar.attr & pci_asAttr_e_PREFETCH) {
                                         /* If BAR2 MEM is prefetchable add corresponding flag */
                                         *val |= 0x00000008;
                                     }
                                     break;
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_BASE_ADDRESS_3: /* 0x1C */
                 {
                     pci_ba_t pcibar[2];
                     int_t nbars = 2;

                     if (dev->hdl == NULL) {
                         handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR3 and BAR2 in case if address is 64 bit */
                     pcibar[0].bar_num = 2;
                     pcibar[1].bar_num = 3;
                     err = pci_device_read_ba(handle, &nbars, &pcibar[0], pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if ((pcibar[1].type == pci_asType_e_MEM) || (pcibar[1].type == pci_asType_e_IO)) {
                            /* Ok, it is pure 32 bit address or IO for BAR3 */
                            *val = pcibar[1].addr;
                            if (pcibar[1].type == pci_asType_e_IO) {
                                /* Mark it as IO */
                                *val |= 0x00000001;
                            }
                            if (pcibar[1].type == pci_asType_e_MEM) {
                                 if (pcibar[1].attr & pci_asAttr_e_PREFETCH) {
                                     /* If BAR3 MEM is prefetchable add corresponding flag */
                                     *val |= 0x00000008;
                                 }
                            }
                        } else {
                            /* BAR3 failed or empty, analyze BAR2 content */
                            switch (pcibar[0].type) {
                                case pci_asType_e_NONE:
                                case pci_asType_e_IO:
                                     /* If BAR2 is IO or NONE, then BAR3 was reported right */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_MEM:
                                     if (pcibar[0].attr & (pci_asAttr_e_32BIT | pci_asAttr_e_16BIT)) {
                                         /* If BAR2 MEM is 16 or 32 bit, then BAR3 was reported right */
                                         *val = 0x00000000;
                                     }
                                     if (pcibar[0].attr & pci_asAttr_e_64BIT) {
                                         /* Store MSB part of 64 bit address */
                                         *val = pcibar[0].addr >> 32;
                                     }
                                     break;
                              }
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_BASE_ADDRESS_4: /* 0x20 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     if (dev->hdl == NULL) {
                         handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR4 (and BAR5 if address is 64 bit) */
                     pcibar.bar_num = 4;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                            switch (pcibar.type) {
                                case pci_asType_e_NONE:
                                     /* If BAR4 is NONE just return zero address */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_IO:
                                     /* If BAR4 is IO then it is 16 bit, just copy it and mark as IO */
                                     *val = pcibar.addr | 0x00000001;
                                     break;
                                case pci_asType_e_MEM:
                                     /* Copy memory address add bits */
                                     *val = pcibar.addr;
                                     if (pcibar.attr & pci_asAttr_e_64BIT) {
                                         /* If BAR4 MEM is 64 bit, add 64 bit flag */
                                         *val |= 0x00000004;
                                     }
                                     if (pcibar.attr & pci_asAttr_e_PREFETCH) {
                                         /* If BAR4 MEM is prefetchable add corresponding flag */
                                         *val |= 0x00000008;
                                     }
                                     break;
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_BASE_ADDRESS_5: /* 0x24 */
                 {
                     pci_ba_t pcibar[2];
                     int_t nbars = 2;

                     if (dev->hdl == NULL) {
                         handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR5 and BAR4 in case if address is 64 bit */
                     pcibar[0].bar_num = 4;
                     pcibar[1].bar_num = 5;
                     err = pci_device_read_ba(handle, &nbars, &pcibar[0], pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if ((pcibar[1].type == pci_asType_e_MEM) || (pcibar[1].type == pci_asType_e_IO)) {
                            /* Ok, it is pure 32 bit address or IO for BAR5 */
                            *val = pcibar[1].addr;
                            if (pcibar[1].type == pci_asType_e_IO) {
                                /* Mark it as IO */
                                *val |= 0x00000001;
                            }
                            if (pcibar[1].type == pci_asType_e_MEM) {
                                 if (pcibar[1].attr & pci_asAttr_e_PREFETCH) {
                                     /* If BAR5 MEM is prefetchable add corresponding flag */
                                     *val |= 0x00000008;
                                 }
                            }
                        } else {
                            /* BAR5 failed or empty, analyze BAR4 content */
                            switch (pcibar[0].type) {
                                case pci_asType_e_NONE:
                                case pci_asType_e_IO:
                                     /* If BAR4 is IO or NONE, then BAR5 was reported right */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_MEM:
                                     if (pcibar[0].attr & (pci_asAttr_e_32BIT | pci_asAttr_e_16BIT)) {
                                         /* If BAR4 MEM is 16 or 32 bit, then BAR5 was reported right */
                                         *val = 0x00000000;
                                     }
                                     if (pcibar[0].attr & pci_asAttr_e_64BIT) {
                                         /* Store MSB part of 64 bit address */
                                         *val = pcibar[0].addr >> 32;
                                     }
                                     break;
                              }
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_CARDBUS_CIS: /* 0x28 - pretend there is no cardbus at all :) */
                 *val = 0x00000000;
                 return -EOK;
            case PCI_SUBSYSTEM_VENDOR_ID: /* 0x2C */
                 err = pci_device_read_ssvid(bdf, &data16);
                 if (err == PCI_ERR_OK) {
                     data32 = data16;
                     err = pci_device_read_ssid(bdf, &data16);
                     if (err == PCI_ERR_OK) {
                         data32 |= ((uint32_t)data16) << 16;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_ROM_ADDRESS: /* 0x30 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     if (dev->hdl == NULL) {
                         handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     } else {
                         handle = dev->hdl;
                     }
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* -1 as BAR number presents expansion ROM base address */
                     pcibar.bar_num = -1;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if (pcibar.attr & pci_asAttr_e_EXPANSION_ROM) {
                            *val = pcibar.addr;
                        } else {
                            *val = 0x00000000;
                        }
                     }

                     if (dev->hdl == NULL) {
                         pci_device_detach(handle);
                     }
                 }
                 return -err;
            case PCI_CAPABILITY_LIST: /* 0x34 */
                 err = -pci_read_config_byte(dev, where, &data8);
                 if (err == EOK) {
                     /* The rest bytes are reserved, make them zero */
                     data32 = data8;
                     *val = data32;
                 }
                 return -err;
            case 0x38: /* 0x38 - reserved zone */
                 *val = 0x00000000;
                 return -EOK;
            case PCI_INTERRUPT_LINE: /* 0x3C */
                 {
                     int_t ntempirqs = 1;
                     pci_irq_t irq;

                     err = pci_device_read_irq(dev->hdl, &ntempirqs, &irq);
                     if (err != PCI_ERR_OK) {
                         /* Set disconnected interrupt */
                         data32 = 0x000000FF;
                     } else {
                         data32 = irq & 0x000000FF;
                     }
                 }
                 /* Set INTA# interrupt line by default */
                 data32 |= 0x00000100;
                 /* The IGD does not burst as a PCI-compliant master (value of 0x00) */
                 data32 |= 0x00000000;
                 /* The IGD has no specific requirements for how often it needs to access the PCI bus (use value of 0x00). */
                 data32 |= 0x00000000;
                 *val = data32;
                 return -EOK;
            default:
                 dev_err(&dev->dev, "Attempt to read dword at forbidden PCI configuration space: %04X\n", where);
                 break;
        }
    }

    return -pci_device_cfg_rd32(bdf, where, val);
}

int pci_write_config_byte(const struct pci_dev *dev, int where, u8 val)
{
    pci_err_t err;

    if (where < 0x40) {
        dev_err(&dev->dev, "Attempt to write byte to forbidden PCI configuration space: %04X\n", where);
    }

    if (dev->hdl == NULL) {
        pci_devhdl_t handle;

        handle = pci_device_attach(PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn)), pci_attachFlags_MULTI_OWNER, &err);
        if (handle == NULL) {
            /* If somebody owns this particular PCI device, pretend that it is not exist in system */
            if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
            }
            return PCIBIOS_DEVICE_NOT_FOUND;
        }
        err = pci_device_cfg_wr8(handle, where, val, NULL);
        pci_device_detach(handle);
    } else {
        err = pci_device_cfg_wr8(dev->hdl, where, val, NULL);
    }

    return -err;
}

int pci_write_config_word(const struct pci_dev *dev, int where, u16 val)
{
    pci_err_t err;

    if (where < 0x40) {
        dev_err(&dev->dev, "Attempt to write word to forbidden PCI configuration space: %04X\n", where);
    }

    if (dev->hdl == NULL) {
        pci_devhdl_t handle;

        handle = pci_device_attach(PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn)), pci_attachFlags_MULTI_OWNER, &err);
        if (handle == NULL) {
            /* If somebody owns this particular PCI device, pretend that it is not exist in system */
            if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
            }
            return PCIBIOS_DEVICE_NOT_FOUND;
        }
        err = pci_device_cfg_wr16(handle, where, val, NULL);
        pci_device_detach(handle);
    } else {
        err = pci_device_cfg_wr16(dev->hdl, where, val, NULL);
    }

    return -err;
}

int pci_write_config_dword(const struct pci_dev *dev, int where, u32 val)
{
    pci_err_t err;

    if (where < 0x40) {
        dev_err(&dev->dev, "Attempt to write dword to forbidden PCI configuration space: %04X\n", where);
    }

    if (dev->hdl == NULL) {
        pci_devhdl_t handle;

        handle = pci_device_attach(PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn)), pci_attachFlags_MULTI_OWNER, &err);
        if (handle == NULL) {
            /* If somebody owns this particular PCI device, pretend that it is not exist in system */
            if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), err);
            }
            return PCIBIOS_DEVICE_NOT_FOUND;
        }
        err = pci_device_cfg_wr32(handle, where, val, NULL);
        pci_device_detach(handle);
    } else {
        err = pci_device_cfg_wr32(dev->hdl, where, val, NULL);
    }

    return -err;
}

int pci_bus_read_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 *val)
{
    pci_devhdl_t handle;
    pci_err_t err;

    if (where < 0x40) {
        switch(where) {
            case PCI_CAPABILITY_LIST: /* 0x34 */
                 handle = pci_device_attach(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), pci_attachFlags_MULTI_OWNER, &err);
                 if (handle == NULL) {
                     /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                     if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                         dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                     }
                     return PCIBIOS_DEVICE_NOT_FOUND;
                 }
                 err = pci_device_cfg_rd34(handle, val);
                 pci_device_detach(handle);
                 return -err;
            default:
                 dev_err(NULL, "Attempt to read byte at forbidden PCI configuration space (bus access): %04X\n", where);
                 break;
        }
    }
    return -pci_device_cfg_rd8(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), where, val);
}

int pci_bus_read_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 *val)
{
    if (where < 0x40) {
        dev_err(NULL, "Attempt to read word at forbidden PCI configuration space (bus access): %04X\n", where);
    }
    return -pci_device_cfg_rd16(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), where, val);
}

int pci_bus_read_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 *val)
{
    uint32_t data32;
    uint16_t data16;
    uint8_t  data8;
    pci_bdf_t bdf;
    pci_err_t err;
    pci_devhdl_t handle = NULL;

    bdf = PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn));

    if (where < 0x40) {
        switch(where) {
            case PCI_VENDOR_ID: /* 0x00 */
                 err = pci_device_read_vid(bdf, &data16);
                 if (err == PCI_ERR_OK) {
                     data32 = data16;
                     err = pci_device_read_did(bdf, &data16);
                     if (err == PCI_ERR_OK) {
                         data32 |= ((uint32_t)data16) << 16;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_COMMAND: /* 0x04 */
                 err = pci_device_read_cmd(bdf, (pci_cmd_t*)&data16);
                 if (err == PCI_ERR_OK) {
                     data32 = data16;
                     err = pci_device_read_status(bdf, (pci_stat_t*)&data16);
                     if (err == PCI_ERR_OK) {
                         data32 |= ((uint32_t)data16) << 16;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_CLASS_REVISION: /* 0x08 */
                 err = pci_device_read_ccode(bdf, (pci_ccode_t*)&data32);
                 if (err == PCI_ERR_OK) {
                     data32 <<= 8;
                     err = pci_device_read_revid(bdf, (pci_revid_t*)&data8);
                     if (err == PCI_ERR_OK) {
                         data32 |= data8;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_CACHE_LINE_SIZE: /* 0x0C */
                 err = pci_device_read_clsize(bdf, (pci_clsize_t*)&data8);
                 if (err == PCI_ERR_OK) {
                     data32 = data8;
                 } else {
                     /* This field is hardwired to 0s. The IGD as a PCI-compliant master does */
                     /* not use the Memory Write and Invalidate command and, in general, does */
                     /* not perform operations based on cache line size.                      */
                     data32 = 0x00000000;
                 }
                 err = pci_device_read_latency(bdf, (pci_latency_t*)&data8);
                 if (err == PCI_ERR_OK) {
                     data32 |= ((uint32_t)data8) << 8;
                 } else {
                     /* Master Latency Timer Count Value (MLTCV): Hardwired to 0s on IGDs. */
                     data32 |= 0x00000000;
                 }
                 /* Header Code for the IGD. This code has the value 00h, indicating a type 0 configuration space format. */
                 data32 |= 0x00000000;
                 /* BIST is always zero too */
                 data32 |= 0x00000000;
                 *val = data32;
                 return -EOK;
            case PCI_BASE_ADDRESS_0: /* 0x10 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR0 (and BAR1 if address is 64 bit) */
                     pcibar.bar_num = 0;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                            switch (pcibar.type) {
                                case pci_asType_e_NONE:
                                     /* If BAR0 is NONE just return zero address */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_IO:
                                     /* If BAR0 is IO then it is 16 bit, just copy it and mark as IO */
                                     *val = pcibar.addr | 0x00000001;
                                     break;
                                case pci_asType_e_MEM:
                                     /* Copy memory address add bits */
                                     *val = pcibar.addr;
                                     if (pcibar.attr & pci_asAttr_e_64BIT) {
                                         /* If BAR0 MEM is 64 bit, add 64 bit flag */
                                         *val |= 0x00000004;
                                     }
                                     if (pcibar.attr & pci_asAttr_e_PREFETCH) {
                                         /* If BAR0 MEM is prefetchable add corresponding flag */
                                         *val |= 0x00000008;
                                     }
                                     break;
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_BASE_ADDRESS_1: /* 0x14 */
                 {
                     pci_ba_t pcibar[2];
                     int_t nbars = 2;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR1 and BAR0 in case if address is 64 bit */
                     pcibar[0].bar_num = 0;
                     pcibar[1].bar_num = 1;
                     err = pci_device_read_ba(handle, &nbars, &pcibar[0], pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if ((pcibar[1].type == pci_asType_e_MEM) || (pcibar[1].type == pci_asType_e_IO)) {
                            /* Ok, it is pure 32 bit address or IO for BAR1 */
                            *val = pcibar[1].addr;
                            if (pcibar[1].type == pci_asType_e_IO) {
                                /* Mark it as IO */
                                *val |= 0x00000001;
                            }
                            if (pcibar[1].type == pci_asType_e_MEM) {
                                 if (pcibar[1].attr & pci_asAttr_e_PREFETCH) {
                                     /* If BAR1 MEM is prefetchable add corresponding flag */
                                     *val |= 0x00000008;
                                 }
                            }
                        } else {
                            /* BAR1 failed or empty, analyze BAR0 content */
                            switch (pcibar[0].type) {
                                case pci_asType_e_NONE:
                                case pci_asType_e_IO:
                                     /* If BAR0 is IO or NONE, then BAR1 was reported right */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_MEM:
                                     if (pcibar[0].attr & (pci_asAttr_e_32BIT | pci_asAttr_e_16BIT)) {
                                         /* If BAR0 MEM is 16 or 32 bit, then BAR1 was reported right */
                                         *val = 0x00000000;
                                     }
                                     if (pcibar[0].attr & pci_asAttr_e_64BIT) {
                                         /* Store MSB part of 64 bit address */
                                         *val = pcibar[0].addr >> 32;
                                     }
                                     break;
                              }
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_BASE_ADDRESS_2: /* 0x18 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR2 (and BAR3 if address is 64 bit) */
                     pcibar.bar_num = 2;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                            switch (pcibar.type) {
                                case pci_asType_e_NONE:
                                     /* If BAR2 is NONE just return zero address */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_IO:
                                     /* If BAR2 is IO then it is 16 bit, just copy it and mark as IO */
                                     *val = pcibar.addr | 0x00000001;
                                     break;
                                case pci_asType_e_MEM:
                                     /* Copy memory address add bits */
                                     *val = pcibar.addr;
                                     if (pcibar.attr & pci_asAttr_e_64BIT) {
                                         /* If BAR2 MEM is 64 bit, add 64 bit flag */
                                         *val |= 0x00000004;
                                     }
                                     if (pcibar.attr & pci_asAttr_e_PREFETCH) {
                                         /* If BAR2 MEM is prefetchable add corresponding flag */
                                         *val |= 0x00000008;
                                     }
                                     break;
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_BASE_ADDRESS_3: /* 0x1C */
                 {
                     pci_ba_t pcibar[2];
                     int_t nbars = 2;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR3 and BAR2 in case if address is 64 bit */
                     pcibar[0].bar_num = 2;
                     pcibar[1].bar_num = 3;
                     err = pci_device_read_ba(handle, &nbars, &pcibar[0], pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if ((pcibar[1].type == pci_asType_e_MEM) || (pcibar[1].type == pci_asType_e_IO)) {
                            /* Ok, it is pure 32 bit address or IO for BAR3 */
                            *val = pcibar[1].addr;
                            if (pcibar[1].type == pci_asType_e_IO) {
                                /* Mark it as IO */
                                *val |= 0x00000001;
                            }
                            if (pcibar[1].type == pci_asType_e_MEM) {
                                 if (pcibar[1].attr & pci_asAttr_e_PREFETCH) {
                                     /* If BAR3 MEM is prefetchable add corresponding flag */
                                     *val |= 0x00000008;
                                 }
                            }
                        } else {
                            /* BAR3 failed or empty, analyze BAR2 content */
                            switch (pcibar[0].type) {
                                case pci_asType_e_NONE:
                                case pci_asType_e_IO:
                                     /* If BAR2 is IO or NONE, then BAR3 was reported right */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_MEM:
                                     if (pcibar[0].attr & (pci_asAttr_e_32BIT | pci_asAttr_e_16BIT)) {
                                         /* If BAR2 MEM is 16 or 32 bit, then BAR3 was reported right */
                                         *val = 0x00000000;
                                     }
                                     if (pcibar[0].attr & pci_asAttr_e_64BIT) {
                                         /* Store MSB part of 64 bit address */
                                         *val = pcibar[0].addr >> 32;
                                     }
                                     break;
                              }
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_BASE_ADDRESS_4: /* 0x20 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR4 (and BAR5 if address is 64 bit) */
                     pcibar.bar_num = 4;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                            switch (pcibar.type) {
                                case pci_asType_e_NONE:
                                     /* If BAR4 is NONE just return zero address */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_IO:
                                     /* If BAR4 is IO then it is 16 bit, just copy it and mark as IO */
                                     *val = pcibar.addr | 0x00000001;
                                     break;
                                case pci_asType_e_MEM:
                                     /* Copy memory address add bits */
                                     *val = pcibar.addr;
                                     if (pcibar.attr & pci_asAttr_e_64BIT) {
                                         /* If BAR4 MEM is 64 bit, add 64 bit flag */
                                         *val |= 0x00000004;
                                     }
                                     if (pcibar.attr & pci_asAttr_e_PREFETCH) {
                                         /* If BAR4 MEM is prefetchable add corresponding flag */
                                         *val |= 0x00000008;
                                     }
                                     break;
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_BASE_ADDRESS_5: /* 0x24 */
                 {
                     pci_ba_t pcibar[2];
                     int_t nbars = 2;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* Retrieve BAR5 and BAR4 in case if address is 64 bit */
                     pcibar[0].bar_num = 4;
                     pcibar[1].bar_num = 5;
                     err = pci_device_read_ba(handle, &nbars, &pcibar[0], pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if ((pcibar[1].type == pci_asType_e_MEM) || (pcibar[1].type == pci_asType_e_IO)) {
                            /* Ok, it is pure 32 bit address or IO for BAR5 */
                            *val = pcibar[1].addr;
                            if (pcibar[1].type == pci_asType_e_IO) {
                                /* Mark it as IO */
                                *val |= 0x00000001;
                            }
                            if (pcibar[1].type == pci_asType_e_MEM) {
                                 if (pcibar[1].attr & pci_asAttr_e_PREFETCH) {
                                     /* If BAR5 MEM is prefetchable add corresponding flag */
                                     *val |= 0x00000008;
                                 }
                            }
                        } else {
                            /* BAR5 failed or empty, analyze BAR4 content */
                            switch (pcibar[0].type) {
                                case pci_asType_e_NONE:
                                case pci_asType_e_IO:
                                     /* If BAR4 is IO or NONE, then BAR5 was reported right */
                                     *val = 0x00000000;
                                     break;
                                case pci_asType_e_MEM:
                                     if (pcibar[0].attr & (pci_asAttr_e_32BIT | pci_asAttr_e_16BIT)) {
                                         /* If BAR4 MEM is 16 or 32 bit, then BAR5 was reported right */
                                         *val = 0x00000000;
                                     }
                                     if (pcibar[0].attr & pci_asAttr_e_64BIT) {
                                         /* Store MSB part of 64 bit address */
                                         *val = pcibar[0].addr >> 32;
                                     }
                                     break;
                              }
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_CARDBUS_CIS: /* 0x28 - pretend there is no cardbus at all :) */
                 *val = 0x00000000;
                 return -EOK;
            case PCI_SUBSYSTEM_VENDOR_ID: /* 0x2C */
                 err = pci_device_read_ssvid(bdf, &data16);
                 if (err == PCI_ERR_OK) {
                     data32 = data16;
                     err = pci_device_read_ssid(bdf, &data16);
                     if (err == PCI_ERR_OK) {
                         data32 |= ((uint32_t)data16) << 16;
                         *val = data32;
                     }
                 }
                 return -err;
            case PCI_ROM_ADDRESS: /* 0x30 */
                 {
                     pci_ba_t pcibar;
                     int_t nbars = 1;

                     handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &err);
                     if (handle == NULL) {
                         /* If somebody owns this particular PCI device, pretend that it is not exist in system */
                         if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
                             dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
                         }
                         return PCIBIOS_DEVICE_NOT_FOUND;
                     }

                     /* -1 as BAR number presents expansion ROM base address */
                     pcibar.bar_num = -1;
                     err = pci_device_read_ba(handle, &nbars, &pcibar, pci_reqType_e_MANDATORY);
                     if (err == PCI_ERR_OK) {
                        if (pcibar.attr & pci_asAttr_e_EXPANSION_ROM) {
                            *val = pcibar.addr;
                        } else {
                            *val = 0x00000000;
                        }
                     }

                     pci_device_detach(handle);
                 }
                 return -err;
            case PCI_CAPABILITY_LIST: /* 0x34 */
                 err = -pci_bus_read_config_byte(bus, devfn, where, &data8);
                 if (err == EOK) {
                     /* The rest bytes are reserved, make them zero */
                     data32 = data8;
                     *val = data32;
                 }
                 return -err;
            case 0x38: /* 0x38 - reserved zone */
                 *val = 0x00000000;
                 return -EOK;
            case PCI_INTERRUPT_LINE: /* 0x3C */
                 /* Set disconnected interrupt (All Gen6+ use MSI interrupts) */
                 data32 = 0x000000FF;
                 /* Set INTA# interrupt line by default */
                 data32 |= 0x00000100;
                 /* The IGD does not burst as a PCI-compliant master (value of 0x00) */
                 data32 |= 0x00000000;
                 /* The IGD has no specific requirements for how often it needs to access the PCI bus (use value of 0x00). */
                 data32 |= 0x00000000;
                 *val = data32;
                 return -EOK;
            default:
                 dev_err(NULL, "Attempt to read dword at forbidden PCI configuration space (bus access): %04X\n", where);
                 break;
        }
    }

    return -pci_device_cfg_rd32(bdf, where, val);
}

int pci_bus_write_config_byte(struct pci_bus *bus, unsigned int devfn, int where, u8 val)
{
    pci_devhdl_t handle;
    pci_err_t err;

    if (where < 0x40) {
        dev_err(NULL, "Attempt to write byte to forbidden PCI configuration space (bus access): %04X\n", where);
    }

    handle = pci_device_attach(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), pci_attachFlags_MULTI_OWNER, &err);
    if (handle == NULL) {
        /* If somebody owns this particular PCI device, pretend that it is not exist in system */
        if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
            dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
        }
        return PCIBIOS_DEVICE_NOT_FOUND;
    }
    err = pci_device_cfg_wr8(handle, where, val, NULL);
    pci_device_detach(handle);

    return -err;
}

int pci_bus_write_config_word(struct pci_bus *bus, unsigned int devfn, int where, u16 val)
{
    pci_devhdl_t handle;
    pci_err_t err;

    if (where < 0x40) {
        dev_err(NULL, "Attempt to write word to forbidden PCI configuration space (bus access): %04X\n", where);
    }

    handle = pci_device_attach(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), pci_attachFlags_MULTI_OWNER, &err);
    if (handle == NULL) {
        /* If somebody owns this particular PCI device, pretend that it is not exist in system */
        if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
            dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
        }
        return PCIBIOS_DEVICE_NOT_FOUND;
    }
    err = pci_device_cfg_wr16(handle, where, val, NULL);
    pci_device_detach(handle);

    return -err;
}

int pci_bus_write_config_dword(struct pci_bus *bus, unsigned int devfn, int where, u32 val)
{
    pci_devhdl_t handle;
    pci_err_t err;

    if (where < 0x40) {
        dev_err(NULL, "Attempt to write dword to forbidden PCI configuration space (bus access): %04X\n", where);
    }

    handle = pci_device_attach(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), pci_attachFlags_MULTI_OWNER, &err);
    if (handle == NULL) {
        /* If somebody owns this particular PCI device, pretend that it is not exist in system */
        if ((err != PCI_ERR_ATTACH_EXCLUSIVE) && (err != PCI_ERR_ATTACH_OWNED)) {
            dev_err(NULL, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn), err);
        }
        return PCIBIOS_DEVICE_NOT_FOUND;
    }
    err = pci_device_cfg_wr32(handle, where, val, NULL);
    pci_device_detach(handle);

    return -err;
}

static inline bool pcie_cap_has_rtctl(const struct pci_dev *dev)
{
    int type = pci_pcie_type(dev);

    return type == PCI_EXP_TYPE_ROOT_PORT || type == PCI_EXP_TYPE_RC_EC;
}

static inline int pcie_cap_version(const struct pci_dev *dev)
{
    return pcie_caps_reg(dev) & PCI_EXP_FLAGS_VERS;
}

void pcie_aspm_pm_state_change(struct pci_dev *pdev)
{
    /* This function is part of ASPM - implement it later */
}

void pcie_aspm_powersave_config_link(struct pci_dev *pdev)
{
    /* This function is part of ASPM - implement it later */
}

void pci_disable_device(struct pci_dev *dev)
{
    pci_cmd_t old_cmd, cmd;
    pci_err_t error = 0;

    error = pci_device_read_cmd(PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn)), &old_cmd);
    if (!error) {
        cmd = old_cmd;
        if (old_cmd & PCI_COMMAND_MASTER) {
            cmd = old_cmd & ~PCI_COMMAND_MASTER;
        }
        if (cmd != old_cmd) {
            error = pci_device_write_cmd(dev->hdl, cmd, NULL);
            if (error) {
                 DRM_DEBUG("Can't write PCI device command register!\n");
            }
        }
        dev->is_busmaster = 0;
    } else {
        DRM_DEBUG("Can't read PCI device command register!\n");
    }
}

void pci_set_drvdata(struct pci_dev *pdev, void *data)
{
    dev_set_drvdata(&pdev->dev, data);
}

static void __pci_set_master(struct pci_dev *dev, bool enable)
{
    pci_err_t error = 0;
    pci_cmd_t old_cmd, cmd;
    pci_devhdl_t handle = NULL;
    pci_bdf_t bdf;

    bdf = PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn));

    if (dev->hdl == NULL) {
        handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &error);
    } else {
        handle = dev->hdl;
    }
    if (handle == NULL) {
        dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), error);
        return;
    }

    error = pci_device_read_cmd(bdf, &old_cmd);
    if (!error) {
        if (enable) {
            cmd = old_cmd | PCI_COMMAND_MASTER;
        } else {
            cmd = old_cmd & ~PCI_COMMAND_MASTER;
        }
        if (cmd != old_cmd) {
            error = pci_device_write_cmd(handle, cmd, NULL);
            if (error) {
                 DRM_ERROR("Can't write PCI device command register! error is %d\n", error);
            } else {
                dev->is_busmaster = enable;
            }
        } else {
            dev->is_busmaster = enable;
        }
    } else {
        DRM_DEBUG("Can't read PCI device command register! error is %d\n", error);
    }

    if (dev->hdl == NULL) {
        pci_device_detach(handle);
    }

    return;
}

void pci_set_master(struct pci_dev *dev)
{
    __pci_set_master(dev, true);
}

int pci_enable_resources(struct pci_dev *dev, int mask)
{
    pci_err_t error = 0;
    pci_cmd_t cmd, old_cmd;
    int i;
    struct resource *r;

    error = pci_device_read_cmd(PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn)), &old_cmd);
    if (!error) {
        cmd = old_cmd;
        for (i = 0; i < PCI_NUM_RESOURCES; i++) {
            if (!(mask & (1 << i))) {
                continue;
            }

            r = &dev->resource[i];

            if (!(r->flags & (IORESOURCE_IO | IORESOURCE_MEM))) {
                continue;
            }
            if ((i == PCI_ROM_RESOURCE) && (!(r->flags & IORESOURCE_ROM_ENABLE))) {
                continue;
            }

            if (r->flags & IORESOURCE_IO) {
                cmd |= PCI_COMMAND_IO;
            }
            if (r->flags & IORESOURCE_MEM) {
                cmd |= PCI_COMMAND_MEMORY;
            }
        }
    } else {
        DRM_DEBUG("Can't read PCI device command register!\n");
        return -EIO;
    }

    if (cmd != old_cmd) {
        error = pci_device_write_cmd(dev->hdl, cmd, NULL);
        if (error) {
             DRM_DEBUG("Can't write PCI device command register!\n");
             return -EIO;
        }
    }

    return 0;
}

int pcibios_enable_device(struct pci_dev *dev, int mask)
{
    return pci_enable_resources(dev, mask);
}

static int do_pci_enable_device(struct pci_dev *dev, int bars)
{
    int err;

    err = pci_set_power_state(dev, PCI_D0);
    if (err < 0 && err != -EIO) {
        return err;
    }
    err = pcibios_enable_device(dev, bars);
    if (err < 0) {
        return err;
    } else {
        return 0;
    }
}

static int __pci_enable_device_flags(struct pci_dev *dev, resource_size_t flags)
{
    int pci = -1;
    int err;
    int i, bars = 0;

    /*
     * Power state could be unknown at this point, either due to a fresh
     * boot or a device removal call.  So get the current power state
     * so that things like MSI message writing will behave as expected
     * (e.g. if the device really is in D0 at enable time).
     */
    if (dev->pm_cap) {
        u16 pmcsr;
        pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
        dev->current_state = (pmcsr & PCI_PM_CTRL_STATE_MASK);
    }

    if (atomic_add_return(1, &dev->enable_cnt) > 1) {
        /* already enabled */
        return 0;
    }
    /* only skip sriov related */
    for (i = 0; i <= PCI_ROM_RESOURCE; i++) {
        if (dev->resource[i].flags & flags) {
            bars |= (1 << i);
        }
    }
    for (i = PCI_BRIDGE_RESOURCES; i < DEVICE_COUNT_RESOURCE; i++) {
        if (dev->resource[i].flags & flags) {
                bars |= (1 << i);
        }
    }

    err = do_pci_enable_device(dev, bars);
    if (err < 0) {
        atomic_dec(&dev->enable_cnt);
    }

    return err;
}

void pci_iounmap(struct pci_dev *dev, void __iomem * addr)
{
    iounmap(addr);
}

int pci_enable_device(struct pci_dev *dev)
{
    return __pci_enable_device_flags(dev, IORESOURCE_MEM | IORESOURCE_IO);
}

/**
 * pci_iomap - create a virtual mapping cookie for a PCI BAR
 * @dev: PCI device that owns the BAR
 * @bar: BAR number
 * @maxlen: length of the memory to map
 *
 * Using this function you will get a __iomem address to your device BAR.
 * You can access it using ioread*() and iowrite*(). These functions hide
 * the details if this is a MMIO or PIO address space and will just do what
 * you expect from them in the correct way.
 *
 * @maxlen specifies the maximum length to map. If you want to get access to
 * the complete BAR without checking for its length first, pass %0 here.
 * */
void *pci_iomap(struct pci_dev *dev, int bar, unsigned long maxlen)
{
	return pci_iomap_range(dev, bar, 0, maxlen);
}

void* pci_iomap_range(struct pci_dev *dev, int bar, unsigned long offset, unsigned long maxlen)
{
	resource_size_t start = pci_resource_start(dev, bar);
	resource_size_t len = pci_resource_len(dev, bar);
	unsigned long flags = pci_resource_flags(dev, bar);

	if (len <= offset || !start)
		return NULL;
	len -= offset;
	start += offset;
	if (maxlen && len > maxlen)
		len = maxlen;

	if (flags & IORESOURCE_IO) {
		return NULL;
	}
	if (flags & IORESOURCE_MEM) {
		return (void *)ioremap(start, len);
	}

	return NULL;
}
EXPORT_SYMBOL(pci_iomap_range);

dma_addr_t pci_map_page(struct pci_dev *hwdev, struct page *page,
	unsigned long offset, size_t size, int direction)
{
	return page_to_phys(page)+offset;

}

void pci_unmap_page(struct pci_dev *hwdev, dma_addr_t dma_address,
	size_t size, int direction)
{
}

int
pci_dma_mapping_error(struct pci_dev *pdev, dma_addr_t dma_addr)
{
	return 0;
}

#define PCI_EXP_SAVE_REGS	7


struct pci_cap_saved_state *pci_find_saved_cap(
	struct pci_dev *pci_dev, char cap)
{
	struct pci_cap_saved_state *tmp;

	hlist_for_each_entry(tmp, &pci_dev->saved_cap_space, next) {
		if (tmp->cap.cap_nr == cap)
			return tmp;
	}
	return NULL;
}
/**
 * pci_pcie_cap2 - query for devices' PCI_CAP_ID_EXP v2 capability structure
 * @dev: PCI device to check
 *
 * Like pci_pcie_cap() but also checks that the PCIe capability version is
 * >= 2.  Note that v1 capability structures could be sparse in that not
 * all register fields were required.  v2 requires the entire structure to
 * be present size wise, while still allowing for non-implemented registers
 * to exist but they must be hardwired to 0.
 *
 * Due to the differences in the versions of capability structures, one
 * must be careful not to try and access non-existant registers that may
 * exist in early versions - v1 - of Express devices.
 *
 * Returns the offset of the PCIe capability structure as long as the
 * capability version is >= 2; otherwise 0 is returned.
 */
#if 0
static int pci_pcie_cap2(struct pci_dev *dev)
{
	u16 flags;
	int pos;
    int pci;

	pos = pci_pcie_cap(dev);
	if (pos) {
		pci_read_config_word(dev, pos + PCI_EXP_FLAGS, &flags);
		if ((flags & PCI_EXP_FLAGS_VERS) < 2)
			pos = 0;
	}

	return pos;
}
#endif /* 0 */

static int pci_save_pcie_state(struct pci_dev *dev)
{
	int pos, i = 0;
	struct pci_cap_saved_state *save_state;
	u16 *cap;

	pos = pci_pcie_cap(dev);
	if (!pos)
		return 0;

	save_state = pci_find_saved_cap(dev, PCI_CAP_ID_EXP);
	if (!save_state) {
		dev_err(&dev->dev, "buffer not found in %s\n", __func__);
		return -ENOMEM;
	}
	cap = (u16 *)&save_state->cap.data[0];
	pcie_capability_read_word(dev, PCI_EXP_DEVCTL, &cap[i++]);
	pcie_capability_read_word(dev, PCI_EXP_LNKCTL, &cap[i++]);
	pcie_capability_read_word(dev, PCI_EXP_SLTCTL, &cap[i++]);
	pcie_capability_read_word(dev, PCI_EXP_RTCTL,  &cap[i++]);
	pcie_capability_read_word(dev, PCI_EXP_DEVCTL2, &cap[i++]);
	pcie_capability_read_word(dev, PCI_EXP_LNKCTL2, &cap[i++]);
	pcie_capability_read_word(dev, PCI_EXP_SLTCTL2, &cap[i++]);
	return 0;
}

static int __pci_bus_find_cap_start(struct pci_bus* bus, unsigned int devfn, u8 hdr_type)
{
	u16 status = 0;
	int pci = 0;

	if (pci_device_read_status(PCI_IS_ARI(devfn) ? PCI_BDF_ARI(bus->number, devfn) : PCI_BDF(bus->number, _PCI_BDF_DEV(devfn), _PCI_BDF_FUNC(devfn)), &status) != PCI_ERR_OK) {
		return 0;
	}

	if (!(status & PCI_STATUS_CAP_LIST)) {
		return 0;
	}

	switch (hdr_type) {
	case PCI_HEADER_TYPE_NORMAL:
	case PCI_HEADER_TYPE_BRIDGE:
		return PCI_CAPABILITY_LIST;
	case PCI_HEADER_TYPE_CARDBUS:
		return PCI_CB_CAPABILITY_LIST;
	default:
		return 0;
	}

	return 0;
}
#define PCI_FIND_CAP_TTL	48

static int __pci_find_next_cap_ttl(struct pci_bus* bus, unsigned int devfn, u8 pos, int cap, int *ttl)
{
	u8 id;
	int pci = 0;

	while ((*ttl)--) {
		pci_bus_read_config_byte(bus, devfn, pos, &pos);
		if (pos < 0x40)
			break;
		pos &= ~3;
		pci_bus_read_config_byte(bus, devfn, pos + PCI_CAP_LIST_ID,
					 &id);
		if (id == 0xff)
			break;
		if (id == cap) {
			return pos;
		}
		pos += PCI_CAP_LIST_NEXT;
	}
	return 0;
}

static int __pci_find_next_cap(struct pci_bus* bus, unsigned int devfn,
			       u8 pos, int cap)
{
	int ttl = PCI_FIND_CAP_TTL;

	return __pci_find_next_cap_ttl(bus, devfn, pos, cap, &ttl);
}

int pci_find_capability(struct pci_dev *dev, int cap)
{
	int pos;

	pos = __pci_bus_find_cap_start(dev->bus, dev->devfn, dev->hdr_type);
	if (pos)
		pos = __pci_find_next_cap(dev->bus, dev->devfn, pos, cap);

	return pos;
}

#if 0
static void pci_restore_pcie_state(struct pci_dev *dev)
{
	int i = 0;
	struct pci_cap_saved_state *save_state;
	u16 *cap;

	save_state = pci_find_saved_cap(dev, PCI_CAP_ID_EXP);
	if (!save_state)
		return;

	cap = (u16 *)&save_state->cap.data[0];
	pcie_capability_write_word(dev, PCI_EXP_DEVCTL, cap[i++]);
	pcie_capability_write_word(dev, PCI_EXP_LNKCTL, cap[i++]);
	pcie_capability_write_word(dev, PCI_EXP_SLTCTL, cap[i++]);
	pcie_capability_write_word(dev, PCI_EXP_RTCTL, cap[i++]);
	pcie_capability_write_word(dev, PCI_EXP_DEVCTL2, cap[i++]);
	pcie_capability_write_word(dev, PCI_EXP_LNKCTL2, cap[i++]);
	pcie_capability_write_word(dev, PCI_EXP_SLTCTL2, cap[i++]);
}
#endif /* 0 */

static int pci_save_pcix_state(struct pci_dev *dev)
{
	int pos;
	struct pci_cap_saved_state *save_state;
    int pci = 0;

	pos = pci_find_capability(dev, PCI_CAP_ID_PCIX);
	if (pos <= 0)
		return 0;

	save_state = pci_find_saved_cap(dev, PCI_CAP_ID_PCIX);
	if (!save_state) {
		dev_err(&dev->dev, "buffer not found in %s\n", __func__);
		return -ENOMEM;
	}

	pci_read_config_word(dev, pos + PCI_X_CMD,
			     (u16 *)save_state->cap.data);

	return 0;
}

/**
 * pci_save_state - save the PCI configuration space of a device before suspending
 * @dev: - PCI device that we're dealing with
 */
int
pci_save_state(struct pci_dev *dev)
{
	int i;
	int pci = 0;
	/* XXX: 100% dword access ok here? */
	for (i = 0; i < 16; i++)
		pci_read_config_dword(dev, i * 4, &dev->saved_config_space[i]);
	dev->state_saved = true;

	if ((i = pci_save_pcie_state(dev)) != 0)
		return i;
	if ((i = pci_save_pcix_state(dev)) != 0)
		return i;
	return 0;
}

static inline void pci_dev_d3_sleep(struct pci_dev *dev)
{
	msleep(dev->d3_delay);
}

#define PCI_PM_D2_DELAY		200
#define PCI_PM_D3_WAIT		10
#define PCI_PM_D3COLD_WAIT	100
#define PCI_PM_BUS_WAIT		50

void pci_update_current_state(struct pci_dev *dev, pci_power_t state)
{
	int pci = 0;
	if (dev->pm_cap) {
		u16 pmcsr;

		/*
		 * Configuration space is not accessible for device in
		 * D3cold, so just keep or set D3cold for safety
		 */
		if (dev->current_state == PCI_D3cold)
			return;
		if (state == PCI_D3cold) {
			dev->current_state = PCI_D3cold;
			return;
		}
		pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
		dev->current_state = (pmcsr & PCI_PM_CTRL_STATE_MASK);
	} else {
		dev->current_state = state;
	}
}

static int pci_platform_power_transition(struct pci_dev *dev, pci_power_t state)
{
	int error = 0;

	if (dev->is_managed) {
		pci_update_current_state(dev, state);
		/* Fall back to PCI_D0 if native PM is not supported */
		if (!dev->pm_cap)
			dev->current_state = PCI_D0;
	} else {
		error = -ENODEV;
		/* Fall back to PCI_D0 if native PM is not supported */
		if (!dev->pm_cap)
			dev->current_state = PCI_D0;
	}

	return error;
}

static int pci_raw_set_power_state(struct pci_dev *dev, pci_power_t state)
{
	u16 pmcsr;

	/* Check if we're already there */
	if (dev->current_state == state)
		return 0;

	if (!dev->pm_cap)
		return -EIO;

	if (state < PCI_D0 || state > PCI_D3hot)
		return -EINVAL;

	if (state != PCI_D0 && dev->current_state <= PCI_D3cold
	    && dev->current_state > state) {
		return -EINVAL;
	}

	if ((state == PCI_D1 && !dev->d1_support)
	   || (state == PCI_D2 && !dev->d2_support))
		return -EIO;

	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);

	switch (dev->current_state) {
	case PCI_D0:
	case PCI_D1:
	case PCI_D2:
		pmcsr &= ~PCI_PM_CTRL_STATE_MASK;
		pmcsr |= state;
		break;
	case PCI_D3hot:
	case PCI_D3cold:
	case PCI_UNKNOWN: /* Boot-up */
	default:
		pmcsr = 0;
		break;
	}

	/* enter specified state */
	pci_write_config_word(dev, dev->pm_cap + PCI_PM_CTRL, pmcsr);

	/* Mandatory power management transition delays */
	/* see PCI PM 1.1 5.6.1 table 18 */
	if (state == PCI_D3hot || dev->current_state == PCI_D3hot)
		pci_dev_d3_sleep(dev);
	else if (state == PCI_D2 || dev->current_state == PCI_D2)
		udelay(PCI_PM_D2_DELAY);

	pci_read_config_word(dev, dev->pm_cap + PCI_PM_CTRL, &pmcsr);
	dev->current_state = (pmcsr & PCI_PM_CTRL_STATE_MASK);

	if (dev->bus)
		pcie_aspm_pm_state_change(dev);

	return 0;
}

static void __pci_start_power_transition(struct pci_dev *dev, pci_power_t state)
{
	if (state == PCI_D0) {
		pci_platform_power_transition(dev, PCI_D0);
		if (dev->runtime_d3cold) {
			msleep(dev->d3cold_delay);
		}
	}
}

static inline int pci_no_d1d2(struct pci_dev *dev)
{
	return 0;
}

int __pci_complete_power_transition(struct pci_dev *dev, pci_power_t state)
{
	int ret;

	if (state <= PCI_D0)
		return -EINVAL;
	ret = pci_platform_power_transition(dev, state);
	/* Power off the bridge may power off the whole hierarchy */

	return ret;
}

int pci_set_power_state(struct pci_dev *dev, pci_power_t state)
{
	int error;

	/* bound the state we're entering */
	if (state > PCI_D3cold)
		state = PCI_D3cold;
	else if (state < PCI_D0)
		state = PCI_D0;
	else if ((state == PCI_D1 || state == PCI_D2) && pci_no_d1d2(dev))
		/*
		 * If the device or the parent bridge do not support PCI PM,
		 * ignore the request if we're doing anything other than putting
		 * it into D0 (which would only happen on boot).
		 */
		return 0;

	/* Check if we're already there */
	if (dev->current_state == state)
		return 0;

	__pci_start_power_transition(dev, state);

	/* This device is quirked not to be put into D3, so
	   don't put it in D3 */
	if (state >= PCI_D3hot && (dev->dev_flags & PCI_DEV_FLAGS_NO_D3))
		return 0;

	/*
	 * To put device in D3cold, we put device into D3hot in native
	 * way, then put device into D3cold with platform ops
	 */
	error = pci_raw_set_power_state(dev, state > PCI_D3hot ?
					PCI_D3hot : state);

	if (!__pci_complete_power_transition(dev, state))
		error = 0;
	/*
	 * When aspm_policy is "powersave" this call ensures
	 * that ASPM is configured.
	 */
	if (!error && dev->bus)
		pcie_aspm_powersave_config_link(dev);

	return error;
	}

struct pci_dev* pci_dev_get(struct pci_dev *dev)
{
    if (dev) {
        get_device(&dev->dev);
    }

    return dev;
}


/**
 * pci_dev_put - release a use of the pci device structure
 * @dev: device that's been disconnected
 *
 * Must be called when a user of a device is finished with it.  When the last
 * user of the device calls this function, the memory of the device is freed.
 */
void pci_dev_put(struct pci_dev* dev)
{
    if (dev) {
        put_device(&dev->dev);
    }
}

struct pci_dev *pci_get_domain_bus_and_slot(int domain, unsigned int bus,
					    unsigned int devfn)
{
	struct pci_dev *dev = NULL;

	for_each_pci_dev(dev) {
		if (dev->bus->number == bus && dev->devfn == devfn) {
			return dev;
		}
	}
	return NULL;
}

int no_pci_devices(void)
{
    return 0;
}

static int match_pci_dev_by_id(struct device *dev, void *data)
{
	struct pci_dev *pdev = to_pci_dev(dev);
	struct pci_device_id *id = data;

	if (pci_match_one_device(id, pdev))
		return 1;
	return 0;
}

/*
 * pci_get_dev_by_id - begin or continue searching for a PCI device by id
 * @id: pointer to struct pci_device_id to match for the device
 * @from: Previous PCI device found in search, or %NULL for new search.
 *
 * Iterates through the list of known PCI devices.  If a PCI device is found
 * with a matching id a pointer to its device structure is returned, and the
 * reference count to the device is incremented.  Otherwise, %NULL is returned.
 * A new search is initiated by passing %NULL as the @from argument.  Otherwise
 * if @from is not %NULL, searches continue from next device on the global
 * list.  The reference count for @from is always decremented if it is not
 * %NULL.
 *
 * This is an internal function for use by the other search functions in
 * this file.
 */
static struct pci_dev *pci_get_dev_by_id(const struct pci_device_id *id,
					 struct pci_dev *from)
{
	struct device *dev;
	struct device *dev_start = NULL;
	struct pci_dev *pdev = NULL;

	if (from)
		dev_start = &from->dev;
	dev = bus_find_device(&pci_bus_type, dev_start, (void *)id,
			      match_pci_dev_by_id);
	if (dev)
		pdev = to_pci_dev(dev);
	pci_dev_put(from);
	return pdev;
}

/**
 * pci_get_subsys - begin or continue searching for a PCI device by vendor/subvendor/device/subdevice id
 * @vendor: PCI vendor id to match, or %PCI_ANY_ID to match all vendor ids
 * @device: PCI device id to match, or %PCI_ANY_ID to match all device ids
 * @ss_vendor: PCI subsystem vendor id to match, or %PCI_ANY_ID to match all vendor ids
 * @ss_device: PCI subsystem device id to match, or %PCI_ANY_ID to match all device ids
 * @from: Previous PCI device found in search, or %NULL for new search.
 *
 * Iterates through the list of known PCI devices.  If a PCI device is found
 * with a matching @vendor, @device, @ss_vendor and @ss_device, a pointer to its
 * device structure is returned, and the reference count to the device is
 * incremented.  Otherwise, %NULL is returned.  A new search is initiated by
 * passing %NULL as the @from argument.  Otherwise if @from is not %NULL,
 * searches continue from next device on the global list.
 * The reference count for @from is always decremented if it is not %NULL.
 */
struct pci_dev *pci_get_subsys(unsigned int vendor, unsigned int device,
			       unsigned int ss_vendor, unsigned int ss_device,
			       struct pci_dev *from)
{
	struct pci_device_id id = {
		.vendor = vendor,
		.device = device,
		.subvendor = ss_vendor,
		.subdevice = ss_device,
	};

	return pci_get_dev_by_id(&id, from);
}
EXPORT_SYMBOL(pci_get_subsys);

struct pci_dev* pci_get_device(unsigned int vendor, unsigned int device, struct pci_dev *from)
{
    return pci_get_subsys(vendor, device, PCI_ANY_ID, PCI_ANY_ID, from);
}

/**
 * pci_get_class - begin or continue searching for a PCI device by class
 * @class: search for a PCI device with this class designation
 * @from: Previous PCI device found in search, or %NULL for new search.
 *
 * Iterates through the list of known PCI devices.  If a PCI device is
 * found with a matching @class, the reference count to the device is
 * incremented and a pointer to its device structure is returned.
 * Otherwise, %NULL is returned.
 * A new search is initiated by passing %NULL as the @from argument.
 * Otherwise if @from is not %NULL, searches continue from next device
 * on the global list.  The reference count for @from is always decremented
 * if it is not %NULL.
 */
struct pci_dev *pci_get_class(unsigned int class, struct pci_dev *from)
{
	struct pci_device_id id = {
		.vendor = PCI_ANY_ID,
		.device = PCI_ANY_ID,
		.subvendor = PCI_ANY_ID,
		.subdevice = PCI_ANY_ID,
		.class_mask = PCI_ANY_ID,
		.class = class,
	};

	return pci_get_dev_by_id(&id, from);
}
EXPORT_SYMBOL(pci_get_class);

/**
 * pci_get_rom_size - obtain the actual size of the ROM image
 * @pdev: target PCI device
 * @rom: kernel virtual pointer to image of ROM
 * @size: size of PCI window
 *  return: size of actual ROM image
 *
 * Determine the actual length of the ROM image.
 * The PCI window size could be much larger than the
 * actual image size.
 */
size_t pci_get_rom_size(struct pci_dev *pdev, void  *rom, size_t size)
{
	void *image;
	int last_image;

	image = rom;
	do {
		void  *pds;
		/* Standard PCI ROMs start out with these bytes 55 AA */
		if (readb(image) != 0x55) {
			break;
		}
		if (readb(image + 1) != 0xAA)
			break;
		/* get the PCI data structure and check its signature */
		pds = image + readw(image + 24);
		if (readb(pds) != 'P')
			break;
		if (readb(pds + 1) != 'C')
			break;
		if (readb(pds + 2) != 'I')
			break;
		if (readb(pds + 3) != 'R')
			break;
		last_image = readb(pds + 21) & 0x80;
		/* this length is reliable */
		image += readw(pds + 16) * 512;
	} while (!last_image);

	/* never return a size larger than the PCI resource window */
	/* there are known ROMs that get the size wrong */
	return min((size_t)(image - rom), size);
}

int pci_assign_resource(struct pci_dev *dev, int resno)
{
    char *buf;
    pci_ba_t bar[7];
    pci_ba_t pcibar;
    int_t nbars = ARRAY_SIZE(bar);
    pci_err_t pci_err;
    pci_devhdl_t handle = NULL;
    int it;

    if (dev->hdl == NULL) {
        handle = pci_device_attach(PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn)), pci_attachFlags_MULTI_OWNER, &pci_err);
    } else {
        handle = dev->hdl;
    }
    if (handle == NULL) {
        dev_err(&dev->dev, "%s(): Unable to locate adapter %04X:%04X at %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->vendor, dev->device, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), pci_err);
        return PCIBIOS_DEVICE_NOT_FOUND;
    }

    /* Set resource IDs */
    for (it = 0; it < 6; it++) {
        bar[it].bar_num = it;
    }
    /* Last is PCI ROM resource */
    bar[it].bar_num = -1;

    pci_err = pci_device_read_ba(handle, &nbars, bar, pci_reqType_e_UNSPECIFIED);
    if (pci_err) {
        DRM_DEBUG("Can't read PCI bars for device %02X:%02X:%02X, err = %d!\n", dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), pci_err);
        if (dev->hdl == NULL) {
            pci_device_detach(handle);
        }
        return -EIO;
    }

    for (it = 0; it < nbars; it++) {
        if ((bar[it].bar_num == resno) || ((resno == PCI_ROM_RESOURCE) && (bar[it].attr & pci_asAttr_e_EXPANSION_ROM))) {
            pci_err = pci_device_map_as(handle, &bar[it], &pcibar);
            if (pci_err) {
                DRM_DEBUG("Failed to get PCI memory translations!\n");
            }
            dev->resource[resno].start = pcibar.addr;
            dev->resource[resno].flags = 0;
            if (pcibar.type == pci_asType_e_IO) {
                dev->resource[resno].flags |= IORESOURCE_IO;
            } else if (pcibar.type == pci_asType_e_MEM) {
                dev->resource[resno].flags |= IORESOURCE_MEM;
            }

            if(dev->resource[resno].start != 0) {
                dev->resource[resno].end = dev->resource[resno].start + pcibar.size - 1;
                dev->resource[resno].parent = NULL;
                dev->resource[resno].sibling = NULL;
                dev->resource[resno].child = NULL;
            } else {
                dev->resource[resno].start = 0;
                dev->resource[resno].end = 0;
                dev->resource[resno].parent = NULL;
                dev->resource[resno].sibling = NULL;
                dev->resource[resno].child = NULL;
            }
            break;
        }
    }
    if (it == nbars) {
        dev->resource[resno].start = 0;
        dev->resource[resno].end = 0;
        dev->resource[resno].parent = NULL;
        dev->resource[resno].sibling = NULL;
        dev->resource[resno].child = NULL;
    }

    if (dev->hdl == NULL) {
        pci_device_detach(handle);
    }

    return EOK;
}
/**
 * pci_map_rom - map a PCI ROM to kernel space
 * @pdev: pointer to pci device struct
 * @size: pointer to receive size of pci window over ROM
 *
 * Return: kernel virtual pointer to image of ROM
 *
 * Map a PCI ROM into kernel space. If ROM is boot video ROM,
 * the shadow BIOS copy will be returned instead of the
 * actual ROM.
 */
void *pci_map_rom(struct pci_dev *pdev, size_t *size)
{
	struct resource *res = &pdev->resource[PCI_ROM_RESOURCE];
	loff_t start;
	void *rom;

	do {
		if (res->flags & IORESOURCE_ROM_SHADOW) {
			/* primary video rom always starts here */
			start = (loff_t)0xC0000;
			*size = 0x20000; /* cover C000:0 through E000:0 */
		} else {
			if (res->flags &
				(IORESOURCE_ROM_COPY | IORESOURCE_ROM_BIOS_COPY)) {
				*size = pci_resource_len(pdev, PCI_ROM_RESOURCE);
				return (void *)(unsigned long)
					pci_resource_start(pdev, PCI_ROM_RESOURCE);
			} else {
				/* assign the ROM an address if it doesn't have one */
				if (res->parent == NULL && pci_assign_resource(pdev,PCI_ROM_RESOURCE)) {
					start = (loff_t)0xC0000;
					*size = 0x20000; /* cover C000:0 through E000:0 */
					break;
				}
				start = pci_resource_start(pdev, PCI_ROM_RESOURCE);
				*size = pci_resource_len(pdev, PCI_ROM_RESOURCE);
				if (*size == 0) {
					start = (loff_t)0xC0000;
					*size = 0x20000; /* cover C000:0 through E000:0 */
					break;
				}
			}
		}
	} while(0);

	rom = mmap_device_memory(0, *size, PROT_READ, 0, start);
	if (!rom) {
		/* restore enable if ioremap fails */
		if (!(res->flags & (IORESOURCE_ROM_ENABLE |
				    IORESOURCE_ROM_SHADOW |
				    IORESOURCE_ROM_COPY)))
			pci_disable_rom(pdev);
		return NULL;
	}

	/*
	 * Try to find the true size of the ROM since sometimes the PCI window
	 * size is much larger than the actual size of the ROM.
	 * True size is important if the ROM is going to be copied.
	 */
	*size = pci_get_rom_size(pdev, rom, *size);
	return rom;
}

/**
 * pci_unmap_rom - unmap the ROM from kernel space
 * @pdev: pointer to pci device struct
 * @rom: virtual address of the previous mapping
 *
 * Remove a mapping of a previously mapped ROM
 */
void pci_unmap_rom(struct pci_dev *pdev, void *rom)
{
	size_t size = pci_resource_len(pdev, PCI_ROM_RESOURCE);
	munmap_device_memory(rom, size);
}

/**
 * pci_enable_rom - enable ROM decoding for a PCI device
 * @pdev: PCI device to enable
 *
 * Enable ROM decoding on @dev.  This involves simply turning on the last
 * bit of the PCI ROM BAR.  Note that some cards may share address decoders
 * between the ROM and other resources, so enabling it may disable access
 * to MMIO registers or other card memory.
 */
int pci_enable_rom(struct pci_dev *pdev)
{
	// DO nothing in QNX we already have enable ROM at the pci_attach_device PCI_INIT_ROM
	struct resource *res = pdev->resource + PCI_ROM_RESOURCE;
	res->flags |= (res->flags & IORESOURCE_ROM_ENABLE);
	return 0;
}

/**
 * pci_disable_rom - disable ROM decoding for a PCI device
 * @pdev: PCI device to disable
 *
 * Disable ROM decoding on a PCI device by turning off the last bit in the
 * ROM BAR.
 */
void pci_disable_rom(struct pci_dev *pdev)
{
	struct resource *res = pdev->resource + PCI_ROM_RESOURCE;
	res->flags &= ~IORESOURCE_ROM_ENABLE;;
	// DO nothing in QNX we already have enable ROM at the pci_attach_device PCI_INIT_ROM
}

static void pci_free_dynids(struct pci_driver *drv)
{
	struct pci_dynid *dynid, *n;

	spin_lock(&drv->dynids.lock);
	list_for_each_entry_safe(dynid, n, &drv->dynids.list, node) {
		list_del(&dynid->node);
		kfree(dynid);
	}
	spin_unlock(&drv->dynids.lock);
}

/**
 * __pci_register_driver - register a new pci driver
 * @drv: the driver structure to register
 *
 * Adds the driver structure to the list of registered drivers.
 * Returns a negative value on error, otherwise 0.
 * If no error occurred, the driver remains registered even if
 * no device was claimed during registration.
 */
int __pci_register_driver(struct pci_driver *drv,  struct module *owner, const char *mod_name)
{
	/* initialize common driver fields */
	drv->driver.name = drv->name;
	drv->driver.bus = &pci_bus_type;
	drv->driver.owner = owner;
	drv->driver.mod_name = mod_name;

	spin_lock_init(&drv->dynids.lock);
	INIT_LIST_HEAD(&drv->dynids.list);

	/* register with core */
	return driver_register(&drv->driver);
}

/**
 * pci_unregister_driver - unregister a pci driver
 * @drv: the driver structure to unregister
 *
 * Deletes the driver structure from the list of registered PCI drivers,
 * gives it a chance to clean up by calling its remove() function for
 * each device it was responsible for, and marks those devices as
 * driverless.
 */

void pci_unregister_driver(struct pci_driver *drv)
{
	driver_unregister(&drv->driver);
	pci_free_dynids(drv);
	spin_destroy(&drv->dynids.lock);
}

int pci_map_sg(struct pci_dev *hwdev, struct scatterlist *sg, int nents, int direction)
{
	return dma_map_sg(hwdev == NULL ? NULL : &hwdev->dev, sg, nents, (enum dma_data_direction)direction);
}

void pci_unmap_sg(struct pci_dev *hwdev, struct scatterlist *sg, int nents, int direction)
{
	dma_unmap_sg(hwdev == NULL ? NULL : &hwdev->dev, sg, nents, (enum dma_data_direction)direction);
}

static inline bool pcie_cap_has_lnkctl(const struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return type == PCI_EXP_TYPE_ENDPOINT ||
	       type == PCI_EXP_TYPE_LEG_END ||
	       type == PCI_EXP_TYPE_ROOT_PORT ||
	       type == PCI_EXP_TYPE_UPSTREAM ||
	       type == PCI_EXP_TYPE_DOWNSTREAM ||
	       type == PCI_EXP_TYPE_PCI_BRIDGE ||
	       type == PCI_EXP_TYPE_PCIE_BRIDGE;
}

static inline bool pcie_cap_has_sltctl(const struct pci_dev *dev)
{
	int type = pci_pcie_type(dev);

	return (type == PCI_EXP_TYPE_ROOT_PORT ||
		type == PCI_EXP_TYPE_DOWNSTREAM) &&
	       pcie_caps_reg(dev) & PCI_EXP_FLAGS_SLOT;
}

static bool pcie_capability_reg_implemented(struct pci_dev *dev, int pos)
{
	if (!pci_is_pcie(dev))
		return false;

	switch (pos) {
	case PCI_EXP_FLAGS:
		return true;
	case PCI_EXP_DEVCAP:
	case PCI_EXP_DEVCTL:
	case PCI_EXP_DEVSTA:
		return true;
	case PCI_EXP_LNKCAP:
	case PCI_EXP_LNKCTL:
	case PCI_EXP_LNKSTA:
		return pcie_cap_has_lnkctl(dev);
	case PCI_EXP_SLTCAP:
	case PCI_EXP_SLTCTL:
	case PCI_EXP_SLTSTA:
		return pcie_cap_has_sltctl(dev);
	case PCI_EXP_RTCTL:
	case PCI_EXP_RTCAP:
	case PCI_EXP_RTSTA:
		return pcie_cap_has_rtctl(dev);
	case PCI_EXP_DEVCAP2:
	case PCI_EXP_DEVCTL2:
	case PCI_EXP_LNKCAP2:
	case PCI_EXP_LNKCTL2:
	case PCI_EXP_LNKSTA2:
		return pcie_cap_version(dev) > 1;
	default:
		return false;
	}
}



/*
 * Note that these accessor functions are only for the "PCI Express
 * Capability" (see PCIe spec r3.0, sec 7.8).  They do not apply to the
 * other "PCI Express Extended Capabilities" (AER, VC, ACS, MFVC, etc.)
 */
int pcie_capability_read_word(struct pci_dev *dev, int pos, u16 *val)
{
	int ret;

	*val = 0;
	if (pos & 1)
		return -EINVAL;

	if (pcie_capability_reg_implemented(dev, pos)) {
		ret = pci_read_config_word(dev, pci_pcie_cap(dev) + pos, val);
		/*
		 * Reset *val to 0 if pci_read_config_word() fails, it may
		 * have been written as 0xFFFF if hardware error happens
		 * during pci_read_config_word().
		 */
		if (ret)
			*val = 0;
		return ret;
	}

	/*
	 * For Functions that do not implement the Slot Capabilities,
	 * Slot Status, and Slot Control registers, these spaces must
	 * be hardwired to 0b, with the exception of the Presence Detect
	 * State bit in the Slot Status register of Downstream Ports,
	 * which must be hardwired to 1b.  (PCIe Base Spec 3.0, sec 7.8)
	 */
	if (pci_is_pcie(dev) && pos == PCI_EXP_SLTSTA &&
		 pci_pcie_type(dev) == PCI_EXP_TYPE_DOWNSTREAM) {
		*val = PCI_EXP_SLTSTA_PDS;
	}

	return 0;
}

int pcie_capability_read_dword(struct pci_dev *dev, int pos, u32 *val)
{
	int ret;

	*val = 0;
	if (pos & 3)
		return -EINVAL;

	if (pcie_capability_reg_implemented(dev, pos)) {
		ret = pci_read_config_dword(dev, pci_pcie_cap(dev) + pos, val);
		/*
		 * Reset *val to 0 if pci_read_config_dword() fails, it may
		 * have been written as 0xFFFFFFFF if hardware error happens
		 * during pci_read_config_dword().
		 */
		if (ret)
			*val = 0;
		return ret;
	}

	if (pci_is_pcie(dev) && pos == PCI_EXP_SLTCTL &&
		 pci_pcie_type(dev) == PCI_EXP_TYPE_DOWNSTREAM) {
		*val = PCI_EXP_SLTSTA_PDS;
	}

	return 0;
}


int pcie_capability_write_word(struct pci_dev *dev, int pos, u16 val)
{
	if (pos & 1)
		return -EINVAL;

	if (!pcie_capability_reg_implemented(dev, pos))
		return 0;

	return pci_write_config_word(dev, pci_pcie_cap(dev) + pos, val);
}
EXPORT_SYMBOL(pcie_capability_write_word);

int pcie_capability_write_dword(struct pci_dev *dev, int pos, u32 val)
{
	if (pos & 3)
		return -EINVAL;

	if (!pcie_capability_reg_implemented(dev, pos))
		return 0;

	return pci_write_config_dword(dev, pci_pcie_cap(dev) + pos, val);
}
EXPORT_SYMBOL(pcie_capability_write_dword);

int pcie_capability_clear_and_set_word(struct pci_dev *dev, int pos,
				       u16 clear, u16 set)
{
	int ret;
	u16 val;

	ret = pcie_capability_read_word(dev, pos, &val);
	if (!ret) {
		val &= ~clear;
		val |= set;
		ret = pcie_capability_write_word(dev, pos, val);
	}

	return ret;
}
EXPORT_SYMBOL(pcie_capability_clear_and_set_word);

int pcie_capability_clear_and_set_dword(struct pci_dev *dev, int pos,
					u32 clear, u32 set)
{
	int ret;
	u32 val;

	ret = pcie_capability_read_dword(dev, pos, &val);
	if (!ret) {
		val &= ~clear;
		val |= set;
		ret = pcie_capability_write_dword(dev, pos, val);
	}

	return ret;
}
EXPORT_SYMBOL(pcie_capability_clear_and_set_dword);

int pci_enable_msi_range(struct pci_dev *dev, int minvec, int maxvec)
{
    int_t msi_cap_idx;
    pci_cap_t msi = NULL;
    pci_err_t error = PCI_ERR_OK;
    pci_bdf_t bdf;
    int_t     ntempirqs = 1;
    pci_irq_t tempirq;
    pci_devhdl_t handle = NULL;

    bdf = PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn));

    if (dev->hdl == NULL) {
        handle = pci_device_attach(bdf, pci_attachFlags_MULTI_OWNER, &error);
    } else {
        handle = dev->hdl;
    }
    if (handle == NULL) {
        dev_err(&dev->dev, "%s(): Can't attach to requested PCI device %02X:%02X:%02X, error=%d\n", __FUNCTION__, dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn), error);
        return PCIBIOS_DEVICE_NOT_FOUND;
    }

    msi_cap_idx = pci_device_find_capid(bdf, CAPID_MSI);

    if (msi_cap_idx >= 0) {
        error = pci_device_read_cap(bdf, &msi, msi_cap_idx);
        if (error == PCI_ERR_OK) {
            error = cap_msi_set_nirq(handle, msi, maxvec - minvec + 1);
            if (error == PCI_ERR_OK) {
                error = pci_device_cfg_cap_enable(handle, pci_reqType_e_ADVISORY, msi);
                if (error) {
                    DRM_ERROR("Call to pci_device_cfg_cap_enable() for MSI has been failed! error is %d\n", error);
                }
            } else {
                DRM_ERROR("Call to cap_msi_set_nirq() has been failed! error is %d\n", error);
            }

            if (error != PCI_ERR_OK) {
                free(msi);
                if (dev->hdl == NULL) {
                    pci_device_detach(handle);
                }
                return -EIO;
            }
        } else {
            DRM_ERROR("Call to pci_device_read_cap() for MSI has been failed! error is %d\n", error);
            if (dev->hdl == NULL) {
                pci_device_detach(handle);
            }
            return -EIO;
        }
    } else {
        DRM_ERROR("Call to pci_device_find_capid() for MSI has been failed! error is %d\n", error);
        if (dev->hdl == NULL) {
            pci_device_detach(handle);
        }
        return -EIO;
    }

    /* Read new IRQ assignment, based on MSI, usually it is >256 */
    error = pci_device_read_irq(handle, &ntempirqs, &tempirq);
    if (error == 0) {
        dev->irq = tempirq;
    } else {
        DRM_ERROR("Can't read MSI IRQ from PCI device! error is %d\n", error);
    }

    if (dev->hdl == NULL) {
        pci_device_detach(handle);
    }

    return EOK;
}

void pci_disable_msi(struct pci_dev *dev)
{
    int_t msi_cap_idx;
    pci_cap_t msi = NULL;
    pci_err_t error;
    pci_bdf_t bdf;

    bdf = PCI_IS_ARI(dev->devfn) ? PCI_BDF_ARI(dev->bus->number, dev->devfn) : PCI_BDF(dev->bus->number, _PCI_BDF_DEV(dev->devfn), _PCI_BDF_FUNC(dev->devfn));
    msi_cap_idx = pci_device_find_capid(bdf, CAPID_MSI);

    if (msi_cap_idx >= 0) {
        error = pci_device_read_cap(bdf, &msi, msi_cap_idx);
        if (error == PCI_ERR_OK) {
            pci_device_cfg_cap_disable(dev->hdl, pci_reqType_e_MANDATORY, msi);
            free(msi);
        }
    }
}

struct resource *pci_bus_resource_n(const struct pci_bus *bus, int n)
{
	struct pci_bus_resource *bus_res;

	if (n < PCI_BRIDGE_RESOURCE_NUM)
		return bus->resource[n];

	n -= PCI_BRIDGE_RESOURCE_NUM;
	list_for_each_entry(bus_res, &bus->resources, list) {
		if (n-- == 0)
			return bus_res->res;
	}
	return NULL;
}
EXPORT_SYMBOL_GPL(pci_bus_resource_n);

static bool region_contains(struct pci_bus_region *region1,
			    struct pci_bus_region *region2)
{
	return region1->start <= region2->start && region1->end >= region2->end;
}

void pcibios_bus_to_resource(struct pci_bus *bus, struct resource *res,
			     struct pci_bus_region *region)
{
	struct pci_host_bridge *bridge = pci_find_host_bridge(bus);
	struct resource_entry *window;
	resource_size_t offset = 0;

	resource_list_for_each_entry(window, &bridge->windows) {
		struct pci_bus_region bus_region;

		if (resource_type(res) != resource_type(window->res))
			continue;

		bus_region.start = window->res->start - window->offset;
		bus_region.end = window->res->end - window->offset;

		if (region_contains(&bus_region, region)) {
			offset = window->offset;
			break;
		}
	}

	res->start = region->start + offset;
	res->end = region->end + offset;
}
EXPORT_SYMBOL(pcibios_bus_to_resource);

static struct pci_bus_region pci_32_bit = {0, 0xffffffffULL};
#ifdef CONFIG_PCI_BUS_ADDR_T_64BIT
static struct pci_bus_region pci_64_bit = {0, (pci_bus_addr_t) 0xffffffffffffffffULL};
static struct pci_bus_region pci_high = {(pci_bus_addr_t) 0x100000000ULL, (pci_bus_addr_t) 0xffffffffffffffffULL};
#endif

/*
 * @res contains CPU addresses.  Clip it so the corresponding bus addresses
 * on @bus are entirely within @region.  This is used to control the bus
 * addresses of resources we allocate, e.g., we may need a resource that
 * can be mapped by a 32-bit BAR.
 */
static void pci_clip_resource_to_region(struct pci_bus *bus,
					struct resource *res,
					struct pci_bus_region *region)
{
	struct pci_bus_region r;

	pcibios_resource_to_bus(bus, &r, res);
	if (r.start < region->start)
		r.start = region->start;
	if (r.end > region->end)
		r.end = region->end;

	if (r.end < r.start)
		res->end = res->start - 1;
	else
		pcibios_bus_to_resource(bus, res, &r);
}

static int pci_bus_alloc_from_region(struct pci_bus *bus, struct resource *res,
		resource_size_t size, resource_size_t align,
		resource_size_t min, unsigned long type_mask,
		resource_size_t (*alignf)(void *,
					  const struct resource *,
					  resource_size_t,
					  resource_size_t),
		void *alignf_data,
		struct pci_bus_region *region)
{
	int i, ret;
	struct resource *r, avail;
	resource_size_t max;

	type_mask |= IORESOURCE_TYPE_BITS;

	pci_bus_for_each_resource(bus, r, i) {
		resource_size_t min_used = min;

		if (!r)
			continue;

		/* type_mask must match */
		if ((res->flags ^ r->flags) & type_mask)
			continue;

		/* We cannot allocate a non-prefetching resource
		   from a pre-fetching area */
		if ((r->flags & IORESOURCE_PREFETCH) &&
		    !(res->flags & IORESOURCE_PREFETCH))
			continue;

		avail = *r;
		pci_clip_resource_to_region(bus, &avail, region);

		/*
		 * "min" is typically PCIBIOS_MIN_IO or PCIBIOS_MIN_MEM to
		 * protect badly documented motherboard resources, but if
		 * this is an already-configured bridge window, its start
		 * overrides "min".
		 */
		if (avail.start)
			min_used = avail.start;

		max = avail.end;

		/* Ok, try it out.. */
		ret = allocate_resource(r, res, size, min_used, max,
					align, alignf, alignf_data);
		if (ret == 0)
			return 0;
	}
	return -ENOMEM;
}

/*
 * pcibios align resources() is called every time generic PCI code
 * wants to generate a new address. The process of looking for
 * an available address, each candidate is first "aligned" and
 * then checked if the resource is available until a match is found.
 *
 * Since we are just checking candidates, don't use any fields other
 * than res->start.
 */
resource_size_t pcibios_align_resource(void *data, const struct resource *res,
					resource_size_t size, resource_size_t alignment)
{
	resource_size_t mask, align, start = res->start;

	/* If it's not IO, then it's gotta be MEM */
	align = (res->flags & IORESOURCE_IO) ? PCIBIOS_MIN_IO : PCIBIOS_MIN_MEM;

	/* Align to largest of MIN or input size */
	mask = max(alignment, align) - 1;
	start += mask;
	start &= ~mask;

	return start;
}

static struct pci_bus *find_pci_root_bus(struct pci_bus *bus)
{
	while (bus->parent)
		bus = bus->parent;

	return bus;
}

struct pci_host_bridge *pci_find_host_bridge(struct pci_bus *bus)
{
	struct pci_bus *root_bus = find_pci_root_bus(bus);

	return to_pci_host_bridge(root_bus->bridge);
}

void pcibios_resource_to_bus(struct pci_bus *bus, struct pci_bus_region *region,
                             struct resource *res)
{
	struct pci_host_bridge *bridge = pci_find_host_bridge(bus);
	struct resource_entry *window;
	resource_size_t offset = 0;

	resource_list_for_each_entry(window, &bridge->windows) {
		if (resource_contains(window->res, res)) {
			offset = window->offset;
			break;
		}
	}

	region->start = res->start - offset;
	region->end = res->end - offset;
}

/**
 * pci_dev_present - Returns 1 if device matching the device list is present, 0 if not.
 * @ids: A pointer to a null terminated list of struct pci_device_id structures
 * that describe the type of PCI device the caller is trying to find.
 *
 * Obvious fact: You do not have a reference to any device that might be found
 * by this function, so if that device is removed from the system right after
 * this function is finished, the value will be stale.  Use this function to
 * find devices that are usually built into a system, or for a general hint as
 * to if another device happens to be present at this specific moment in time.
 */
int pci_dev_present(const struct pci_device_id *ids)
{
	struct pci_dev *found = NULL;

	while (ids->vendor || ids->subvendor || ids->class_mask) {
		found = pci_get_dev_by_id(ids, NULL);
		if (found) {
			pci_dev_put(found);
			return 1;
		}
		ids++;
	}
	return 0;
}
EXPORT_SYMBOL(pci_dev_present);

/**
 * pci_bus_alloc_resource - allocate a resource from a parent bus
 * @bus: PCI bus
 * @res: resource to allocate
 * @size: size of resource to allocate
 * @align: alignment of resource to allocate
 * @min: minimum /proc/iomem address to allocate
 * @type_mask: IORESOURCE_* type flags
 * @alignf: resource alignment function
 * @alignf_data: data argument for resource alignment function
 *
 * Given the PCI bus a device resides on, the size, minimum address,
 * alignment and type, try to find an acceptable resource allocation
 * for a specific device resource.
 */
int pci_bus_alloc_resource(struct pci_bus *bus, struct resource *res,
		resource_size_t size, resource_size_t align,
		resource_size_t min, unsigned long type_mask,
		resource_size_t (*alignf)(void *,
					  const struct resource *,
					  resource_size_t,
					  resource_size_t),
		void *alignf_data)
{
#ifdef CONFIG_PCI_BUS_ADDR_T_64BIT
	int rc;

	if (res->flags & IORESOURCE_MEM_64) {
		rc = pci_bus_alloc_from_region(bus, res, size, align, min,
					       type_mask, alignf, alignf_data,
					       &pci_high);
		if (rc == 0)
			return 0;

		return pci_bus_alloc_from_region(bus, res, size, align, min,
						 type_mask, alignf, alignf_data,
						 &pci_64_bit);
	}
#endif

	return pci_bus_alloc_from_region(bus, res, size, align, min,
					 type_mask, alignf, alignf_data,
					 &pci_32_bit);
}
EXPORT_SYMBOL(pci_bus_alloc_resource);

/**
 * pci_release_dev - free a pci device structure when all users of it are finished.
 * @dev: device that's been disconnected
 *
 * Will be called only by the device core when all users of this pci device are
 * done.
 */
static void pci_release_dev(struct device *dev)
{
	struct pci_dev *pci_dev;

	pci_dev = to_pci_dev(dev);
	pci_release_of_node(pci_dev);
	pci_bus_put(pci_dev->bus);
	kfree(pci_dev->driver_override);
	kfree(pci_dev);
}

/**
 * pci_dma_configure - Setup DMA configuration
 * @dev: ptr to pci_dev struct of the PCI device
 *
 * Function to update PCI devices's DMA configuration using the same
 * info from the OF node or ACPI node of host bridge's parent (if any).
 */
static void pci_dma_configure(struct pci_dev *dev)
{
	/* No ACPI backend for now */
}

int pci_set_dma_max_seg_size(struct pci_dev *dev, unsigned int size)
{
	return dma_set_max_seg_size(&dev->dev, size);
}

int pci_set_dma_seg_boundary(struct pci_dev *dev, unsigned long mask)
{
	return dma_set_seg_boundary(&dev->dev, mask);
}

/* Enhanced Allocation Initialization */
void pci_ea_init(struct pci_dev *dev)
{
	/* Not sure we need it for video device, it is bridge capability */
}

static inline void pci_msi_set_enable(struct pci_dev *dev, int enable)
{
	/* QNX's libpci took care about this already, we should not program it directly */
}

static inline void pci_msix_clear_and_set_ctrl(struct pci_dev *dev, u16 clear, u16 set)
{
	/* QNX's libpci took care about this already, we should not program it directly */
}

void pci_msi_setup_pci_dev(struct pci_dev *dev)
{
	/*
	 * Disable the MSI hardware to avoid screaming interrupts
	 * during boot.  This is the power on reset default so
	 * usually this should be a noop.
	 */
	dev->msi_cap = pci_find_capability(dev, PCI_CAP_ID_MSI);
	if (dev->msi_cap)
		pci_msi_set_enable(dev, 0);

	dev->msix_cap = pci_find_capability(dev, PCI_CAP_ID_MSIX);
	if (dev->msix_cap)
		pci_msix_clear_and_set_ctrl(dev, PCI_MSIX_FLAGS_ENABLE, 0);
}

void pci_allocate_cap_save_buffers(struct pci_dev *dev)
{
	dev->pcie_cap = pci_find_capability(dev, PCI_CAP_ID_PM);
}

void pci_pm_init(struct pci_dev *dev)
{
	dev->pm_cap = pci_find_capability(dev, PCI_CAP_ID_PM);
}

static void pci_init_capabilities(struct pci_dev *dev)
{
	/* Enhanced Allocation */
	pci_ea_init(dev);

	/* Setup MSI caps & disable MSI/MSI-X interrupts */
	pci_msi_setup_pci_dev(dev);

	/* Buffers for saving PCIe and PCI-X capabilities */
	pci_allocate_cap_save_buffers(dev);

	/* Power Management */
	pci_pm_init(dev);

	/* Vital Product Data */
	/* pci_vpd_init(dev); */

	/* Alternative Routing-ID Forwarding */
	/* pci_configure_ari(dev); */

	/* Single Root I/O Virtualization */
	/* pci_iov_init(dev); */

	/* Address Translation Services */
	/* pci_ats_init(dev); */

	/* Enable ACS P2P upstream forwarding */
	/* pci_enable_acs(dev); */

	/* Precision Time Measurement */
	/* pci_ptm_init(dev); */

	/* Advanced Error Reporting */
	/* pci_aer_init(dev); */
}

void pci_device_add(struct pci_dev *dev, struct pci_bus *bus)
{
	int ret;

	device_initialize(&dev->dev);
	dev->dev.release = pci_release_dev;

	set_dev_node(&dev->dev, pcibus_to_node(bus));
	dev->dev.dma_mask = &dev->dma_mask;
	dev->dev.dma_parms = &dev->dma_parms;
	dev->dev.coherent_dma_mask = 0xffffffffull;
	pci_dma_configure(dev);

	pci_set_dma_max_seg_size(dev, 65536);
	pci_set_dma_seg_boundary(dev, 0xffffffff);

	/* Clear the state_saved flag. */
	dev->state_saved = false;

	/* Initialize various capabilities */
	pci_init_capabilities(dev);

	/*
	 * Add the device to our list of discovered devices
	 * and the bus list for fixup functions, etc.
	 */
	list_add_tail(&dev->bus_list, &bus->devices);

	/* Notifier could use PCI capabilities */
	dev->match_driver = false;
	ret = device_add(&dev->dev);
	WARN_ON(ret < 0);
}

/**
 * pci_get_slot - locate PCI device for a given PCI slot
 * @bus: PCI bus on which desired PCI device resides
 * @devfn: encodes number of PCI slot in which the desired PCI
 * device resides and the logical device number within that slot
 * in case of multi-function devices.
 *
 * Given a PCI bus and slot/function number, the desired PCI device
 * is located in the list of PCI devices.
 * If the device is found, its reference count is increased and this
 * function returns a pointer to its data structure.  The caller must
 * decrement the reference count by calling pci_dev_put().
 * If no device is found, %NULL is returned.
 */
struct pci_dev *pci_get_slot(struct pci_bus *bus, unsigned int devfn)
{
	struct pci_dev *dev;

	list_for_each_entry(dev, &bus->devices, bus_list) {
		if (dev->devfn == devfn)
			goto out;
	}

	dev = NULL;
 out:
	pci_dev_get(dev);
	return dev;
}
EXPORT_SYMBOL(pci_get_slot);

#define PCI_COMMAND_DECODE_ENABLE	(PCI_COMMAND_MEMORY | PCI_COMMAND_IO)

enum pci_bar_type {
	pci_bar_unknown,	/* Standard PCI BAR probe */
	pci_bar_io,		/* An io port BAR */
	pci_bar_mem32,		/* A 32-bit memory BAR */
	pci_bar_mem64,		/* A 64-bit memory BAR */
};

/**
 * pci_read_base - read a PCI BAR
 * @dev: the PCI device
 * @type: type of the BAR
 * @res: resource buffer to be filled in
 * @pos: BAR position in the config space
 *
 * Returns 1 if the BAR is 64-bit, or 0 if 32-bit.
 */
int __pci_read_base(struct pci_dev *dev, enum pci_bar_type type,
		    struct resource *res, unsigned int pos)
{
	res->name = pci_name(dev);

	pci_assign_resource(dev, pos);

	return (res->flags & IORESOURCE_MEM_64) ? 1 : 0;
}

static void pci_read_bases(struct pci_dev *dev, unsigned int howmany, int rom)
{
	unsigned int pos;

	if (dev->non_compliant_bars)
		return;

	for (pos = 0; pos < howmany; pos++) {
		struct resource *res = &dev->resource[pos];
		pos += __pci_read_base(dev, pci_bar_unknown, res, pos);
	}

	if (rom) {
		struct resource *res = &dev->resource[PCI_ROM_RESOURCE];
		dev->rom_base_reg = rom;
		res->flags = IORESOURCE_MEM | IORESOURCE_PREFETCH |
				IORESOURCE_READONLY | IORESOURCE_CACHEABLE |
				IORESOURCE_SIZEALIGN;
		__pci_read_base(dev, pci_bar_mem32, res, PCI_ROM_RESOURCE);
	}
}

#define LEGACY_IO_RESOURCE (IORESOURCE_IO | IORESOURCE_PCI_FIXED)

#define PCI_CFG_SPACE_SIZE	256
#define PCI_CFG_SPACE_EXP_SIZE	4096

/**
 * pci_cfg_space_size - get the configuration space size of the PCI device.
 * @dev: PCI device
 *
 * Regular PCI devices have 256 bytes, but PCI-X 2 and PCI Express devices
 * have 4096 bytes.  Even if the device is capable, that doesn't mean we can
 * access it.  Maybe we don't have a way to generate extended config space
 * accesses, or the device is behind a reverse Express bridge.  So we try
 * reading the dword at 0x100 which must either be 0 or a valid extended
 * capability header.
 */
static int pci_cfg_space_size_ext(struct pci_dev *dev)
{
	u32 status;
	int pos = PCI_CFG_SPACE_SIZE;

	if (pci_read_config_dword(dev, pos, &status) != PCIBIOS_SUCCESSFUL)
		goto fail;
	if (status == 0xffffffff)
		goto fail;

	return PCI_CFG_SPACE_EXP_SIZE;

 fail:
	return PCI_CFG_SPACE_SIZE;
}

int pci_cfg_space_size(struct pci_dev *dev)
{
	int pos;
	u32 status;
	u16 class;

	class = dev->class >> 8;
	if (class == PCI_CLASS_BRIDGE_HOST)
		return pci_cfg_space_size_ext(dev);

	if (!pci_is_pcie(dev)) {
		pos = pci_find_capability(dev, PCI_CAP_ID_PCIX);
		if (!pos)
			goto fail;

		pci_read_config_dword(dev, pos + PCI_X_STATUS, &status);
		if (!(status & (PCI_X_STATUS_266MHZ | PCI_X_STATUS_533MHZ)))
			goto fail;
	}

	return pci_cfg_space_size_ext(dev);

 fail:
	return PCI_CFG_SPACE_SIZE;
}

static void pci_read_irq(struct pci_dev *dev)
{
	unsigned char irq;

	pci_read_config_byte(dev, PCI_INTERRUPT_PIN, &irq);
	dev->pin = irq;
	if (irq)
		pci_read_config_byte(dev, PCI_INTERRUPT_LINE, &irq);
	dev->irq = irq;
}

/*
 * Returns true if the PCI bus is root (behind host-PCI bridge),
 * false otherwise
 *
 * Some code assumes that "bus->self == NULL" means that bus is a root bus.
 * This is incorrect because "virtual" buses added for SR-IOV (via
 * virtfn_add_bus()) have "bus->self == NULL" but are not root buses.
 */
bool pci_is_root_bus(struct pci_bus *pbus)
{
	return !(pbus->parent);
}

struct pci_dev *pci_upstream_bridge(struct pci_dev *dev)
{
	dev = pci_physfn(dev);
	if (pci_is_root_bus(dev->bus))
		return NULL;

	return dev->bus->self;
}

void set_pcie_port_type(struct pci_dev *pdev)
{
	int pos;
	u16 reg16;
	int type;
	struct pci_dev *parent;

	pos = pci_find_capability(pdev, PCI_CAP_ID_EXP);
	if (!pos)
		return;
	pdev->pcie_cap = pos;
	pci_read_config_word(pdev, pos + PCI_EXP_FLAGS, &reg16);
	pdev->pcie_flags_reg = reg16;
	pci_read_config_word(pdev, pos + PCI_EXP_DEVCAP, &reg16);
	pdev->pcie_mpss = reg16 & PCI_EXP_DEVCAP_PAYLOAD;

	/*
	 * A Root Port is always the upstream end of a Link.  No PCIe
	 * component has two Links.  Two Links are connected by a Switch
	 * that has a Port on each Link and internal logic to connect the
	 * two Ports.
	 */
	type = pci_pcie_type(pdev);
	if (type == PCI_EXP_TYPE_ROOT_PORT)
		pdev->has_secondary_link = 1;
	else if (type == PCI_EXP_TYPE_UPSTREAM ||
		 type == PCI_EXP_TYPE_DOWNSTREAM) {
		parent = pci_upstream_bridge(pdev);
		if (!parent->has_secondary_link)
			pdev->has_secondary_link = 1;
	}
}

/**
 * pci_setup_device - fill in class and map information of a device
 * @dev: the device structure to fill
 *
 * Initialize the device structure with information about the device's
 * vendor,class,memory and IO-space addresses,IRQ lines etc.
 * Called at initialisation of the PCI subsystem and by CardBus services.
 * Returns 0 on success and negative if unknown type of device (not normal,
 * bridge or CardBus).
 */
int pci_setup_device(struct pci_dev *dev)
{
	u32 class;
	u16 cmd;
	u8 hdr_type;
	struct pci_slot *slot;
	int pos = 0;
	struct pci_bus_region region;
	struct resource *res;

	if (pci_read_config_byte(dev, PCI_HEADER_TYPE, &hdr_type))
		return -EIO;

	dev->sysdata = dev->bus->sysdata;
	dev->dev.parent = dev->bus->bridge;
	dev->dev.bus = &pci_bus_type;
	dev->hdr_type = hdr_type & 0x7f;
	dev->multifunction = !!(hdr_type & 0x80);
	dev->error_state = pci_channel_io_normal;
	set_pcie_port_type(dev);

	list_for_each_entry(slot, &dev->bus->slots, list)
		if (PCI_LNX_SLOT(dev->devfn) == slot->number)
			dev->slot = slot;

	/* Assume 32-bit PCI; let 64-bit PCI cards (which are far rarer)
	   set this higher, assuming the system even supports it.  */
	dev->dma_mask = 0xffffffff;

	dev_set_name(&dev->dev, "%04x:%02x:%02x.%d", pci_domain_nr(dev->bus),
		     dev->bus->number, PCI_LNX_SLOT(dev->devfn),
		     PCI_FUNC(dev->devfn));

	pci_read_config_dword(dev, PCI_CLASS_REVISION, &class);
	dev->revision = class & 0xff;
	dev->class = class >> 8;		    /* upper 3 bytes */

	dev_printk(KERN_DEBUG, &dev->dev, "[%04x:%04x] type %02x class %#08x\n",
		   dev->vendor, dev->device, dev->hdr_type, dev->class);

	/* need to have dev->class ready */
	dev->cfg_size = pci_cfg_space_size(dev);

	/* "Unknown power state" */
	dev->current_state = PCI_UNKNOWN;

	/* Early fixups, before probing the BARs */
	pci_fixup_device(pci_fixup_early, dev);
	/* device class may be changed after fixup */
	class = dev->class >> 8;

	if (dev->non_compliant_bars) {
		pci_read_config_word(dev, PCI_COMMAND, &cmd);
		if (cmd & (PCI_COMMAND_IO | PCI_COMMAND_MEMORY)) {
			dev_info(&dev->dev, "device has non-compliant BARs; disabling IO/MEM decoding\n");
			cmd &= ~PCI_COMMAND_IO;
			cmd &= ~PCI_COMMAND_MEMORY;
			pci_write_config_word(dev, PCI_COMMAND, cmd);
		}
	}

	switch (dev->hdr_type) {		    /* header type */
	case PCI_HEADER_TYPE_NORMAL:		    /* standard header */
		if (class == PCI_CLASS_BRIDGE_PCI)
			goto bad;
		pci_read_irq(dev);
		pci_read_bases(dev, 6, PCI_ROM_ADDRESS);
		pci_read_config_word(dev, PCI_SUBSYSTEM_VENDOR_ID, &dev->subsystem_vendor);
		pci_read_config_word(dev, PCI_SUBSYSTEM_ID, &dev->subsystem_device);

		/*
		 * Do the ugly legacy mode stuff here rather than broken chip
		 * quirk code. Legacy mode ATA controllers have fixed
		 * addresses. These are not always echoed in BAR0-3, and
		 * BAR0-3 in a few cases contain junk!
		 */
		if (class == PCI_CLASS_STORAGE_IDE) {
			u8 progif;
			pci_read_config_byte(dev, PCI_CLASS_PROG, &progif);
			if ((progif & 1) == 0) {
				region.start = 0x1F0;
				region.end = 0x1F7;
				res = &dev->resource[0];
				res->flags = LEGACY_IO_RESOURCE;
				pcibios_bus_to_resource(dev->bus, res, &region);
				dev_info(&dev->dev, "legacy IDE quirk: reg 0x10: %pR\n",
					 res);
				region.start = 0x3F6;
				region.end = 0x3F6;
				res = &dev->resource[1];
				res->flags = LEGACY_IO_RESOURCE;
				pcibios_bus_to_resource(dev->bus, res, &region);
				dev_info(&dev->dev, "legacy IDE quirk: reg 0x14: %pR\n",
					 res);
			}
			if ((progif & 4) == 0) {
				region.start = 0x170;
				region.end = 0x177;
				res = &dev->resource[2];
				res->flags = LEGACY_IO_RESOURCE;
				pcibios_bus_to_resource(dev->bus, res, &region);
				dev_info(&dev->dev, "legacy IDE quirk: reg 0x18: %pR\n",
					 res);
				region.start = 0x376;
				region.end = 0x376;
				res = &dev->resource[3];
				res->flags = LEGACY_IO_RESOURCE;
				pcibios_bus_to_resource(dev->bus, res, &region);
				dev_info(&dev->dev, "legacy IDE quirk: reg 0x1c: %pR\n",
					 res);
			}
		}
		break;

	case PCI_HEADER_TYPE_BRIDGE:		    /* bridge header */
		if (class != PCI_CLASS_BRIDGE_PCI)
			goto bad;
		/* The PCI-to-PCI bridge spec requires that subtractive
		   decoding (i.e. transparent) bridge must have programming
		   interface code of 0x01. */
		pci_read_irq(dev);
		dev->transparent = ((dev->class & 0xff) == 1);
		pci_read_bases(dev, 2, PCI_ROM_ADDRESS1);
		pos = pci_find_capability(dev, PCI_CAP_ID_SSVID);
		if (pos) {
			pci_read_config_word(dev, pos + PCI_SSVID_VENDOR_ID, &dev->subsystem_vendor);
			pci_read_config_word(dev, pos + PCI_SSVID_DEVICE_ID, &dev->subsystem_device);
		}
		break;

	case PCI_HEADER_TYPE_CARDBUS:		    /* CardBus bridge header */
		if (class != PCI_CLASS_BRIDGE_CARDBUS)
			goto bad;
		pci_read_irq(dev);
		pci_read_bases(dev, 1, 0);
		pci_read_config_word(dev, PCI_CB_SUBSYSTEM_VENDOR_ID, &dev->subsystem_vendor);
		pci_read_config_word(dev, PCI_CB_SUBSYSTEM_ID, &dev->subsystem_device);
		break;

	default:				    /* unknown header */
		dev_err(&dev->dev, "unknown header type %02x, ignoring device\n",
			dev->hdr_type);
		return -EIO;

	bad:
		dev_err(&dev->dev, "ignoring class %#08x (doesn't match header type %02x)\n",
			dev->class, dev->hdr_type);
		dev->class = PCI_CLASS_NOT_DEFINED;
	}

	/* We found a fine healthy device, go go go... */
	return 0;
}

bool pci_bus_read_dev_vendor_id(struct pci_bus *bus, int devfn, u32 *l,
				int crs_timeout)
{
	int delay = 1;

	if (pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, l))
		return false;

	/* some broken boards return 0 or ~0 if a slot is empty: */
	if (*l == 0xffffffff || *l == 0x00000000 ||
	    *l == 0x0000ffff || *l == 0xffff0000)
		return false;

	/*
	 * Configuration Request Retry Status.  Some root ports return the
	 * actual device ID instead of the synthetic ID (0xFFFF) required
	 * by the PCIe spec.  Ignore the device ID and only check for
	 * (vendor id == 1).
	 */
	while ((*l & 0xffff) == 0x0001) {
		if (!crs_timeout)
			return false;

		msleep(delay);
		delay *= 2;
		if (pci_bus_read_config_dword(bus, devfn, PCI_VENDOR_ID, l))
			return false;
		/* Card hasn't responded in 60 seconds?  Must be stuck. */
		if (delay > crs_timeout) {
			printk(KERN_WARNING "pci %04x:%02x:%02x.%d: not responding\n",
			       pci_domain_nr(bus), bus->number, PCI_LNX_SLOT(devfn),
			       PCI_FUNC(devfn));
			return false;
		}
	}

	return true;
}
EXPORT_SYMBOL(pci_bus_read_dev_vendor_id);

struct device_type pci_dev_type = { NULL };

struct pci_dev *pci_alloc_dev(struct pci_bus *bus)
{
	struct pci_dev *dev;

	dev = kzalloc(sizeof(struct pci_dev), GFP_KERNEL);
	if (!dev)
		return NULL;

	INIT_LIST_HEAD(&dev->bus_list);
	dev->dev.type = &pci_dev_type;
	dev->bus = pci_bus_get(bus);

	return dev;
}
EXPORT_SYMBOL(pci_alloc_dev);

/*
 * Read the config data for a PCI device, sanity-check it
 * and fill in the dev structure...
 */
static struct pci_dev *pci_scan_device(struct pci_bus *bus, int devfn)
{
	struct pci_dev *dev;
	u32 l;

	if (!pci_bus_read_dev_vendor_id(bus, devfn, &l, 60*1000))
		return NULL;

	dev = pci_alloc_dev(bus);
	if (!dev)
		return NULL;

	dev->devfn = devfn;
	dev->vendor = l & 0xffff;
	dev->device = (l >> 16) & 0xffff;

	pci_set_of_node(dev);

	if (pci_setup_device(dev)) {
		pci_bus_put(dev->bus);
		kfree(dev);
		return NULL;
	}

	return dev;
}

struct pci_dev *pci_scan_single_device(struct pci_bus *bus, int devfn)
{
	struct pci_dev *dev;

	dev = pci_get_slot(bus, devfn);
	if (dev) {
		pci_dev_put(dev);
		return dev;
	}

	dev = pci_scan_device(bus, devfn);
	if (!dev)
		return NULL;

	pci_device_add(dev, bus);

	return dev;
}
EXPORT_SYMBOL(pci_scan_single_device);

int pci_flags = 0;

int pci_has_flag(int flag)
{
	return pci_flags & flag;
}

static int only_one_child(struct pci_bus *bus)
{
	struct pci_dev *parent = bus->self;

	if (!parent || !pci_is_pcie(parent))
		return 0;
	if (pci_pcie_type(parent) == PCI_EXP_TYPE_ROOT_PORT)
		return 1;
	if (pci_pcie_type(parent) == PCI_EXP_TYPE_DOWNSTREAM &&
	    !pci_has_flag(PCI_SCAN_ALL_PCIE_DEVS))
		return 1;
	return 0;
}

/**
 * pci_find_next_ext_capability - Find an extended capability
 * @dev: PCI device to query
 * @start: address at which to start looking (0 to start at beginning of list)
 * @cap: capability code
 *
 * Returns the address of the next matching extended capability structure
 * within the device's PCI configuration space or 0 if the device does
 * not support it.  Some capabilities can occur several times, e.g., the
 * vendor-specific capability, and this provides a way to find them all.
 */
int pci_find_next_ext_capability(struct pci_dev *dev, int start, int cap)
{
	u32 header;
	int ttl;
	int pos = PCI_CFG_SPACE_SIZE;

	/* minimum 8 bytes per capability */
	ttl = (PCI_CFG_SPACE_EXP_SIZE - PCI_CFG_SPACE_SIZE) / 8;

	if (dev->cfg_size <= PCI_CFG_SPACE_SIZE)
		return 0;

	if (start)
		pos = start;

	if (pci_read_config_dword(dev, pos, &header) != PCIBIOS_SUCCESSFUL)
		return 0;

	/*
	 * If we have no capabilities, this is indicated by cap ID,
	 * cap version and next pointer all being 0.
	 */
	if (header == 0)
		return 0;

	while (ttl-- > 0) {
		if (PCI_EXT_CAP_ID(header) == cap && pos != start)
			return pos;

		pos = PCI_EXT_CAP_NEXT(header);
		if (pos < PCI_CFG_SPACE_SIZE)
			break;

		if (pci_read_config_dword(dev, pos, &header) != PCIBIOS_SUCCESSFUL)
			break;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(pci_find_next_ext_capability);

/**
 * pci_find_ext_capability - Find an extended capability
 * @dev: PCI device to query
 * @cap: capability code
 *
 * Returns the address of the requested extended capability structure
 * within the device's PCI configuration space or 0 if the device does
 * not support it.  Possible values for @cap:
 *
 *  %PCI_EXT_CAP_ID_ERR		Advanced Error Reporting
 *  %PCI_EXT_CAP_ID_VC		Virtual Channel
 *  %PCI_EXT_CAP_ID_DSN		Device Serial Number
 *  %PCI_EXT_CAP_ID_PWR		Power Budgeting
 */
int pci_find_ext_capability(struct pci_dev *dev, int cap)
{
	return pci_find_next_ext_capability(dev, 0, cap);
}
EXPORT_SYMBOL_GPL(pci_find_ext_capability);

static unsigned next_fn(struct pci_bus *bus, struct pci_dev *dev, unsigned fn)
{
	int pos;
	u16 cap = 0;
	unsigned next_fn;

	if (pci_ari_enabled(bus)) {
		if (!dev)
			return 0;
		pos = pci_find_ext_capability(dev, PCI_EXT_CAP_ID_ARI);
		if (!pos)
			return 0;

		pci_read_config_word(dev, pos + PCI_ARI_CAP, &cap);
		next_fn = PCI_ARI_CAP_NFN(cap);
		if (next_fn <= fn)
			return 0;	/* protect against malformed list */

		return next_fn;
	}

	/* dev may be NULL for non-contiguous multifunction devices */
	if (!dev || dev->multifunction)
		return (fn + 1) % 8;

	return 0;
}

/**
 * pci_scan_slot - scan a PCI slot on a bus for devices.
 * @bus: PCI bus to scan
 * @devfn: slot number to scan (must have zero function.)
 *
 * Scan a PCI slot on the specified PCI bus for devices, adding
 * discovered devices to the @bus->devices list.  New devices
 * will not have is_added set.
 *
 * Returns the number of new devices found.
 */
int pci_scan_slot(struct pci_bus *bus, int devfn)
{
	unsigned fn, nr = 0;
	struct pci_dev *dev;

	if (only_one_child(bus) && (devfn > 0))
		return 0; /* Already scanned the entire slot */

	dev = pci_scan_single_device(bus, devfn);
	if (!dev)
		return 0;
	if (!dev->is_added)
		nr++;

	for (fn = next_fn(bus, dev, 0); fn > 0; fn = next_fn(bus, dev, fn)) {
		dev = pci_scan_single_device(bus, devfn + fn);
		if (dev) {
			if (!dev->is_added)
				nr++;
			dev->multifunction = 1;
		}
	}

	return nr;
}
EXPORT_SYMBOL(pci_scan_slot);

unsigned int pci_scan_child_bus(struct pci_bus *bus)
{
	unsigned int devfn, pass, max = bus->busn_res.start;
	struct pci_dev *dev;

	dev_dbg(&bus->dev, "scanning bus\n");

	/* Go find them, Rover! */
	for (devfn = 0; devfn < 0x100; devfn += 8) {
		if (bus->number == 0) {
			switch (PCI_LNX_SLOT(devfn)) {
				case 0: /* Host Bridge, always fixed position B0:D0:F0 (according to PCI spec) */
				case 2: /* Video controller, always fixed position B0:D2:F0 (according to x86 arch model) */
					pci_scan_slot(bus, devfn);
					break;
				default:
					/* Do nothing, skip undesired PCI devices */
					break;
			}
		}
	}

	/*
	 * After performing arch-dependent fixup of the bus, look behind
	 * all PCI-to-PCI bridges on this bus.
	 */
	if (!bus->is_added) {
		bus->is_added = 1;
	}

	/*
	 * We've scanned the bus and so we know all about what's on
	 * the other side of any bridges that may be on this bus plus
	 * any devices.
	 *
	 * Return how far we've got finding sub-buses.
	 */
	dev_dbg(&bus->dev, "bus scan returning with max=%02x\n", max);
	return max;
}
EXPORT_SYMBOL_GPL(pci_scan_child_bus);

static struct pci_bus *pci_alloc_bus(struct pci_bus *parent)
{
	struct pci_bus *b;

	b = kzalloc(sizeof(*b), GFP_KERNEL);
	if (!b)
		return NULL;

	INIT_LIST_HEAD(&b->node);
	INIT_LIST_HEAD(&b->children);
	INIT_LIST_HEAD(&b->devices);
	INIT_LIST_HEAD(&b->slots);
	INIT_LIST_HEAD(&b->resources);
	b->max_bus_speed = PCI_SPEED_UNKNOWN;
	b->cur_bus_speed = PCI_SPEED_UNKNOWN;
#ifdef CONFIG_PCI_DOMAINS_GENERIC
	if (parent)
		b->domain_nr = parent->domain_nr;
#endif
	return b;
}

/*
 * PCI Bus Class
 */

static struct attribute *pcibus_attrs[] = {
	NULL,
	NULL,
	NULL,
	NULL,
};

static const struct attribute_group pcibus_group = {
	.attrs = pcibus_attrs,
};

const struct attribute_group *pcibus_groups[] = {
	&pcibus_group,
	NULL,
};

void pci_bus_remove_resources(struct pci_bus *bus)
{
	int i;
	struct pci_bus_resource *bus_res, *tmp;

	for (i = 0; i < PCI_BRIDGE_RESOURCE_NUM; i++)
		bus->resource[i] = NULL;

	list_for_each_entry_safe(bus_res, tmp, &bus->resources, list) {
		list_del(&bus_res->list);
		kfree(bus_res);
	}
}

static void release_pcibus_dev(struct device *dev)
{
	struct pci_bus *pci_bus = to_pci_bus(dev);

	put_device(pci_bus->bridge);
	pci_bus_remove_resources(pci_bus);
	pci_release_bus_of_node(pci_bus);
	kfree(pci_bus);
}

static struct class pcibus_class = {
	.name		= "pci_bus",
	.dev_release	= &release_pcibus_dev,
	.dev_groups	= pcibus_groups,
};

int pcibus_class_init(void)
{
	return class_register(&pcibus_class);
}

void pci_free_resource_list(struct list_head *resources)
{
	resource_list_free(resources);
}
EXPORT_SYMBOL(pci_free_resource_list);

static void pci_release_host_bridge_dev(struct device *dev)
{
	struct pci_host_bridge *bridge = to_pci_host_bridge(dev);

	if (bridge->release_fn)
		bridge->release_fn(bridge);

	pci_free_resource_list(&bridge->windows);

	kfree(bridge);
}

static struct pci_host_bridge *pci_alloc_host_bridge(struct pci_bus *b)
{
	struct pci_host_bridge *bridge;

	bridge = kzalloc(sizeof(*bridge), GFP_KERNEL);
	if (!bridge)
		return NULL;

	INIT_LIST_HEAD(&bridge->windows);
	bridge->bus = b;
	return bridge;
}

static struct resource *get_pci_domain_busn_res(int domain_nr)
{
	struct pci_domain_busn_res *r;

	list_for_each_entry(r, &pci_domain_busn_res_list, list)
		if (r->domain_nr == domain_nr)
			return &r->res;

	r = kzalloc(sizeof(*r), GFP_KERNEL);
	if (!r)
		return NULL;

	r->domain_nr = domain_nr;
	r->res.start = 0;
	r->res.end = 0xff;
	r->res.flags = IORESOURCE_BUS | IORESOURCE_PCI_FIXED;

	list_add_tail(&r->list, &pci_domain_busn_res_list);

	return &r->res;
}
int pci_bus_insert_busn_res(struct pci_bus *b, int bus, int bus_max)
{
	struct resource *res = &b->busn_res;
	struct resource *parent_res;

	res->start = bus;
	res->end = bus_max;
	res->flags = IORESOURCE_BUS;

	if (!pci_is_root_bus(b))
		parent_res = &b->parent->busn_res;
	else {
		parent_res = get_pci_domain_busn_res(pci_domain_nr(b));
		res->flags |= IORESOURCE_PCI_FIXED;
	}

	return 1;
}

void pci_bus_add_resource(struct pci_bus *bus, struct resource *res,
			  unsigned int flags)
{
	struct pci_bus_resource *bus_res;

	bus_res = kzalloc(sizeof(struct pci_bus_resource), GFP_KERNEL);
	if (!bus_res) {
		dev_err(&bus->dev, "can't add %pR resource\n", res);
		return;
	}

	bus_res->res = res;
	bus_res->flags = flags;
	list_add_tail(&bus_res->list, &bus->resources);
}

static struct pci_bus *pci_do_find_bus(struct pci_bus *bus, unsigned char busnr)
{
	struct pci_bus *child;
	struct pci_bus *tmp;

	if (bus->number == busnr)
		return bus;

	list_for_each_entry(tmp, &bus->children, node) {
		child = pci_do_find_bus(tmp, busnr);
		if (child)
			return child;
	}
	return NULL;
}

/*
 * pci_find_next_bus - begin or continue searching for a PCI bus
 * @from: Previous PCI bus found, or %NULL for new search.
 *
 * Iterates through the list of known PCI buses.  A new search is
 * initiated by passing %NULL as the @from argument.  Otherwise if
 * @from is not %NULL, searches continue from next device on the
 * global list.
 */
struct pci_bus *pci_find_next_bus(const struct pci_bus *from)
{
	struct list_head *n;
	struct pci_bus *b = NULL;

	n = from ? from->node.next : pci_root_buses.next;
	if (n != &pci_root_buses)
		b = list_entry(n, struct pci_bus, node);
	return b;
}
EXPORT_SYMBOL(pci_find_next_bus);

/**
 * pci_find_bus - locate PCI bus from a given domain and bus number
 * @domain: number of PCI domain to search
 * @busnr: number of desired PCI bus
 *
 * Given a PCI bus number and domain number, the desired PCI bus is located
 * in the global list of PCI buses.  If the bus is found, a pointer to its
 * data structure is returned.  If no bus is found, %NULL is returned.
 */
struct pci_bus *pci_find_bus(int domain, int busnr)
{
	struct pci_bus *bus = NULL;
	struct pci_bus *tmp_bus;

	while ((bus = pci_find_next_bus(bus)) != NULL)  {
		if (pci_domain_nr(bus) != domain)
			continue;
		tmp_bus = pci_do_find_bus(bus, busnr);
		if (tmp_bus)
			return tmp_bus;
	}
	return NULL;
}
EXPORT_SYMBOL(pci_find_bus);

struct pci_bus *pci_create_root_bus(struct device *parent, int bus,
		struct pci_ops *ops, void *sysdata, struct list_head *resources)
{
	int error;
	struct pci_host_bridge *bridge;
	struct pci_bus *b, *b2;
	struct resource_entry *window, *n;
	struct resource *res;
	resource_size_t offset;
	char bus_addr[64];
	char *fmt;

	b = pci_alloc_bus(NULL);
	if (!b)
		return NULL;

	b->sysdata = sysdata;
	b->ops = ops;
	b->number = b->busn_res.start = bus;
	pci_bus_assign_domain_nr(b, parent);
	b2 = pci_find_bus(pci_domain_nr(b), bus);
	if (b2) {
		/* If we already got to this bus through a different bridge, ignore it */
		dev_dbg(&b2->dev, "bus already known\n");
		goto err_out;
	}

	bridge = pci_alloc_host_bridge(b);
	if (!bridge)
		goto err_out;

	bridge->dev.parent = parent;
	bridge->dev.release = pci_release_host_bridge_dev;
	dev_set_name(&bridge->dev, "pci%04x:%02x", pci_domain_nr(b), bus);

	error = device_register(&bridge->dev);
	if (error) {
		put_device(&bridge->dev);
		goto err_out;
	}
	b->bridge = get_device(&bridge->dev);
	device_enable_async_suspend(b->bridge);
	pci_set_bus_of_node(b);

	if (!parent)
		set_dev_node(b->bridge, pcibus_to_node(b));

	b->dev.class = &pcibus_class;
	b->dev.parent = b->bridge;
	dev_set_name(&b->dev, "%04x:%02x", pci_domain_nr(b), bus);
	error = device_register(&b->dev);
	if (error)
		goto class_dev_reg_err;

	if (parent)
		dev_info(parent, "PCI host bridge to bus %s\n", dev_name(&b->dev));
	else
		printk(KERN_INFO "PCI host bridge to bus %s\n", dev_name(&b->dev));

	/* Add initial resources to the bus */
	resource_list_for_each_entry_safe(window, n, resources) {
		list_move_tail(&window->node, &bridge->windows);
		res = window->res;
		offset = window->offset;
		if (res->flags & IORESOURCE_BUS)
			pci_bus_insert_busn_res(b, bus, res->end);
		else
			pci_bus_add_resource(b, res, 0);
		if (offset) {
			if (resource_type(res) == IORESOURCE_IO)
				fmt = " (bus address [%#06llx-%#06llx])";
			else
				fmt = " (bus address [%#010llx-%#010llx])";
			snprintf(bus_addr, sizeof(bus_addr), fmt,
				 (unsigned long long) (res->start - offset),
				 (unsigned long long) (res->end - offset));
		} else
			bus_addr[0] = '\0';
		dev_info(&b->dev, "root bus resource %pR%s\n", res, bus_addr);
	}

	list_add_tail(&b->node, &pci_root_buses);

	return b;

class_dev_reg_err:
	put_device(&bridge->dev);
	device_unregister(&bridge->dev);
err_out:
	kfree(b);
	return NULL;
}
EXPORT_SYMBOL_GPL(pci_create_root_bus);

int pci_bus_update_busn_res_end(struct pci_bus *b, int bus_max)
{
	struct resource *res = &b->busn_res;
	struct resource old_res = *res;
	resource_size_t size;
	int ret;

	if (res->start > bus_max)
		return -EINVAL;

	size = bus_max - res->start + 1;
	ret = adjust_resource(res, res->start, size);
	dev_printk(KERN_DEBUG, &b->dev,
			"busn_res: %pR end %s updated to %02x\n",
			&old_res, ret ? "can not be" : "is", bus_max);

	if (!ret && !res->parent)
		pci_bus_insert_busn_res(b, res->start, res->end);

	return ret;
}

struct pci_bus *pci_scan_root_bus(struct device *parent, int bus,
		struct pci_ops *ops, void *sysdata, struct list_head *resources)
{
	struct resource_entry *window;
	bool found = false;
	struct pci_bus *b;
	int max;

	b = pci_create_root_bus(parent, bus, ops, sysdata, resources);
	if (!b)
		return NULL;

	pci_bus_insert_busn_res(b, bus, 255);

	max = pci_scan_child_bus(b);

	if (!found) {
		pci_bus_update_busn_res_end(b, max);
	}

	return b;
}
EXPORT_SYMBOL(pci_scan_root_bus);

/**
 * pci_bus_add_device - start driver for a single device
 * @dev: device to add
 *
 * This adds add sysfs entries and start device drivers
 */
void pci_bus_add_device(struct pci_dev *dev)
{
	int retval;

	/*
	 * Can not put in pci_device_add yet because resources
	 * are not assigned yet for some devices.
	 */
	dev->match_driver = true;
	retval = device_attach(&dev->dev);
	WARN_ON(retval < 0);

	dev->is_added = 1;
}
EXPORT_SYMBOL_GPL(pci_bus_add_device);

/**
 * pci_bus_add_devices - start driver for PCI devices
 * @bus: bus to check for new devices
 *
 * Start driver for PCI devices and add some sysfs entries.
 */
void pci_bus_add_devices(const struct pci_bus *bus)
{
	struct pci_dev *dev;
	struct pci_bus *child;

	list_for_each_entry(dev, &bus->devices, bus_list) {
		/* Skip already-added devices */
		if (dev->is_added)
			continue;
		pci_bus_add_device(dev);
	}

	list_for_each_entry(dev, &bus->devices, bus_list) {
		BUG_ON(!dev->is_added);
		child = dev->subordinate;
		if (child)
			pci_bus_add_devices(child);
	}
}
EXPORT_SYMBOL(pci_bus_add_devices);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/pci.c $ $Rev: 845340 $")
#endif
