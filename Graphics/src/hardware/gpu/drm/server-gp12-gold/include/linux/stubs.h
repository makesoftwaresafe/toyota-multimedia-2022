#ifndef _QNX_LINUX_STUBS_H_
#define _QNX_LINUX_STUBS_H_

#define acquire_console_sem()
#define release_console_sem()

void get_dma_buf(struct dma_buf *dmabuf);
int dma_buf_fd(struct dma_buf *dmabuf, int flags);
struct dma_buf *dma_buf_get(int fd);
void dma_buf_unmap_attachment(struct dma_buf_attachment *attach,
                                struct sg_table *sg_table,
                                enum dma_data_direction direction);
void dma_buf_detach(struct dma_buf *dmabuf, struct dma_buf_attachment *attach);

void vm_stubs_init(void);
void vm_stubs_destroy(void);

extern void schedule(void);

/* Support for virtually mapped pages */
struct page *vmalloc_to_page(const void *addr);

// timer support

/* call with NULL to unregister */
int vga_client_register(struct pci_dev *pdev, void *cookie,
                        void (*irq_set_state)(void *cookie, bool state),
                        unsigned int (*set_vga_decode)(void *cookie, bool decode));

struct platform_device *platform_device_register_simple(
                const char *name, int id,
                const struct resource *res, unsigned int num);

void class_destroy(struct class *cls);

int device_create_file(struct device *dev,
                       const struct device_attribute *attr);

void device_remove_file(struct device *dev,
                        const struct device_attribute *attr);

int device_create_bin_file(struct device *dev,
                           const struct bin_attribute *attr);

void device_remove_bin_file(struct device *dev,
                            const struct bin_attribute *attr);

int device_for_each_child(struct device *parent, void *data,
			  int (*fn)(struct device *dev, void *data));

int device_register(struct device *dev);

void device_unregister(struct device *dev);

void device_initialize(struct device *dev);

int dev_set_name(struct device *dev, const char *fmt, ...);

struct device *device_create(struct class *class, struct device *parent,
                             dev_t devt, void *drvdata, const char *fmt, ...);

void device_destroy(struct class *class, dev_t devt);

void kobject_put(struct kobject *kobj);

int fasync_helper(int fd, struct file * filp, int on, struct fasync_struct **fapp);

int sysfs_create_files(struct kobject *kobj, const struct attribute **ptr);
void sysfs_remove_files(struct kobject * kobj, const struct attribute **ptr);

#endif /* _STUBS_H_ */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/stubs.h $ $Rev: 848997 $")
#endif
