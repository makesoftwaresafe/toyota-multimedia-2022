/*
 * drivers/base/core.c - core driver model code (device registration, etc)
 *
 * Copyright (c) 2002-3 Patrick Mochel
 * Copyright (c) 2002-3 Open Source Development Labs
 * Copyright (c) 2006 Greg Kroah-Hartman <gregkh@suse.de>
 * Copyright (c) 2006 Novell, Inc.
 *
 * This file is released under the GPLv2
 *
 */

#include <linux/device.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/notifier.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/mutex.h>
#include <linux/sysfs.h>
#include <linux/kdev_t.h>

#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/qnx_resmgr.h>
#include <drm/drmP.h>

#include "base.h"

#define to_drm_minor(d) dev_get_drvdata(d)

extern struct attr_list minors_list;   /**< Linked list of minors */
extern struct attr_list groups_list;   /**< Linked list of groups */
int last_inode;

#define print_dev_t(buffer, dev) sprintf((buffer), "%u:%u\n", MAJOR(dev), MINOR(dev))

static ssize_t dev_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return print_dev_t(buf, dev->devt);
}

static DEVICE_ATTR_RO(dev);

/* /sys/devices/ */
struct kset *devices_kset = NULL;

int (*platform_notify)(struct device *dev) = NULL;
int (*platform_notify_remove)(struct device *dev) = NULL;
static struct kobject *dev_kobj;
struct kobject *sysfs_dev_char_kobj;
struct kobject *sysfs_dev_block_kobj;

struct kobject *virtual_device_parent(struct device *dev)
{
	static struct kobject *virtual_dir = NULL;

	if (!virtual_dir)
		virtual_dir = kobject_create_and_add("virtual",
						     &devices_kset->kobj);

	return virtual_dir;
}

static struct kobject *get_device_parent(struct device *dev,
					 struct device *parent)
{
	/* subsystems can specify a default root directory for their devices */
	if (!parent && dev->bus && dev->bus->dev_root)
		return &dev->bus->dev_root->kobj;

	if (parent)
		return &parent->kobj;

	return NULL;
}

/**
 * devices_kset_move_last - move the device to the end of devices_kset's list.
 * @dev: device to move
 */
void devices_kset_move_last(struct device *dev)
{
	if (!devices_kset)
		return;
	pr_debug("devices_kset: Moving %s to end of list\n", dev_name(dev));
	spin_lock(&devices_kset->list_lock);
	list_move_tail(&dev->kobj.entry, &devices_kset->list);
	spin_unlock(&devices_kset->list_lock);
}

#define to_dev_attr(_attr) container_of(_attr, struct device_attribute, attr)

static ssize_t dev_attr_show(struct kobject *kobj, struct attribute *attr,
			     char *buf)
{
	struct device_attribute *dev_attr = to_dev_attr(attr);
	struct device *dev = kobj_to_dev(kobj);
	ssize_t ret = -EIO;

	if (dev_attr->show)
		ret = dev_attr->show(dev, dev_attr, buf);
	if (ret >= (ssize_t)PAGE_SIZE) {
		pr_debug("dev_attr_show: %ld returned bad count\n",
				(unsigned long)dev_attr->show);
	}
	return ret;
}

static ssize_t dev_attr_store(struct kobject *kobj, struct attribute *attr,
			      const char *buf, size_t count)
{
	struct device_attribute *dev_attr = to_dev_attr(attr);
	struct device *dev = kobj_to_dev(kobj);
	ssize_t ret = -EIO;

	if (dev_attr->store)
		ret = dev_attr->store(dev, dev_attr, buf, count);
	return ret;
}

static const struct sysfs_ops dev_sysfs_ops = {
	.show	= dev_attr_show,
	.store	= dev_attr_store,
};

/**
 * device_release - free device structure.
 * @kobj: device's kobject.
 *
 * This is called once the reference count for the object
 * reaches 0. We forward the call to the device's release
 * method, which should handle actually freeing the structure.
 */
static void device_release(struct kobject *kobj)
{
	struct device *dev = kobj_to_dev(kobj);
	struct device_private *p = dev->p;

	/*
	 * Some platform devices are driven without driver attached
	 * and managed resources may have been acquired.  Make sure
	 * all resources are released.
	 *
	 * Drivers still can add resources into device after device
	 * is deleted but alive, so release devres here to avoid
	 * possible memory leak.
	 */
	devres_release_all(dev);

	if (dev->release)
		dev->release(dev);
	else if (dev->type && dev->type->release)
		dev->type->release(dev);
	else if (dev->class && dev->class->dev_release)
		dev->class->dev_release(dev);
	else
		WARN(1, KERN_ERR "Device '%s' does not have a release() "
			"function, it is broken and must be fixed.\n",
			dev_name(dev));
	kfree(p);
}

static const void *device_namespace(struct kobject *kobj)
{
	struct device *dev = kobj_to_dev(kobj);
	const void *ns = NULL;

	if (dev->class && dev->class->ns_type)
		ns = dev->class->namespace(dev);

	return ns;
}

static struct kobj_type device_ktype = {
	.release	= device_release,
	.sysfs_ops	= &dev_sysfs_ops,
	.namespace	= device_namespace,
};


int dev_uevent_filter(struct kset *kset, struct kobject *kobj)
{
	struct kobj_type *ktype = get_ktype(kobj);

	if (ktype == &device_ktype) {
		struct device *dev = kobj_to_dev(kobj);
		if (dev->bus)
			return 1;
		if (dev->class)
			return 1;
	}
	return 0;
}

static const char *dev_uevent_name(struct kset *kset, struct kobject *kobj)
{
	struct device *dev = kobj_to_dev(kobj);

	if (dev->bus)
		return dev->bus->name;
	if (dev->class)
		return dev->class->name;
	return NULL;
}

static int dev_uevent(struct kset *kset, struct kobject *kobj,
		      struct kobj_uevent_env *env)
{
	int retval = EOK;

	return retval;
}

static const struct kset_uevent_ops device_uevent_ops = {
	.filter =	dev_uevent_filter,
	.name =		dev_uevent_name,
	.uevent =	dev_uevent,
};

int devices_init(void)
{
	devices_kset = kset_create_and_add("devices", &device_uevent_ops, NULL);
	if (!devices_kset)
		return -ENOMEM;
	dev_kobj = kobject_create_and_add("dev", NULL);
	if (!dev_kobj)
		goto dev_kobj_err;
	sysfs_dev_block_kobj = kobject_create_and_add("block", dev_kobj);
	if (!sysfs_dev_block_kobj)
		goto block_kobj_err;
	sysfs_dev_char_kobj = kobject_create_and_add("char", dev_kobj);
	if (!sysfs_dev_char_kobj)
		goto char_kobj_err;

	return 0;

 char_kobj_err:
	kobject_put(sysfs_dev_block_kobj);
 block_kobj_err:
	kobject_put(dev_kobj);
 dev_kobj_err:
	kset_unregister(devices_kset);
	return -ENOMEM;
}

int device_add_groups(struct device *dev, const struct attribute_group **groups)
{
	return sysfs_create_groups(&dev->kobj, groups);
}

void device_remove_groups(struct device *dev,
			  const struct attribute_group **groups)
{
	sysfs_remove_groups(&dev->kobj, groups);
}

void device_initialize(struct device *dev)
{
    dev->kobj.kset = devices_kset;
    kobject_init(&dev->kobj, &device_ktype);
    INIT_LIST_HEAD(&dev->dma_pools);
    mutex_init(&dev->mutex);
    lockdep_set_novalidate_class(&dev->mutex);
    spin_lock_init(&dev->devres_lock);
    INIT_LIST_HEAD(&dev->devres_head);
    set_dev_node(dev, -1);
}

int device_register_qnx(struct device *dev)
{
	struct drm_node_list *list;
	struct sysfs_dirent *sd = NULL; // FIXME: remove after search implementation

	list = kzalloc(sizeof(*list), GFP_KERNEL);
	if (!list) {
		return -EINVAL;
	}

	if ((dev) && (dev->type) && (dev->type->name) && (strcmp(dev->type->name, "drm_minor") == 0)) {
		iofunc_attr_init(&list->device.attr, S_IFCHR | 0666, 0, 0);
		list->device.attr.rdev = rsrcdbmgr_devno_attach("dri", ((struct drm_minor*)to_drm_minor(dev))->index, 0);
	} else {
		iofunc_attr_init(&list->device.attr, S_IFDIR | 0777, 0, 0);
		list->device.attr.rdev = 0;
	}

	list->device.attr.inode = ++last_inode;
	list->name = dev->kobj.name;

	INIT_LIST_HEAD(&list->attrs_list.list);
	INIT_LIST_HEAD(&list->head);

	drm_device_ext_data_t *attr_priv = kmalloc(sizeof(drm_device_ext_data_t), GFP_KERNEL);

	if (attr_priv) {
		attr_priv->dev = dev;
		attr_priv->nodes = &list->attrs_list;
		attr_priv->iofunc_attr = &list->device.attr;
		if (MAJOR(dev->devt) == DRM_MAJOR) {
			attr_priv->inode.i_rdev = dev->devt;
		} else {
			attr_priv->inode.i_rdev = 0;
		}
		sd = kmalloc(sizeof(struct sysfs_dirent), GFP_KERNEL);
		if (sd) {
			sd->s_name = dev_name(dev);
			sd->s_mode = attr_priv->iofunc_attr->mode;
			sd->s_flags = SYSFS_KOBJ_ATTR;
			sd->list = list;
			attr_priv->sd = sd;
		} else {
			DRM_ERROR("Not enough memory!");
			return -ENOMEM;
		}
	}
	else {
		DRM_ERROR("Not enough memory!");
		return -ENOMEM;
	}

	list->device.device_ext_data = (void *)attr_priv;

	list_add(&list->head, &minors_list.list);
	minors_list.count++;

	dev->kobj.sd = (void*)sd;
	return 0;
}

static int device_add_attrs(struct device *dev)
{
	int error;

	if (dev->groups) {
		error = device_add_groups(dev, dev->groups);
		if (error)
			goto err_remove_type_groups;
	}

	return 0;

err_remove_type_groups:
	return error;
}

static void device_remove_attrs(struct device *dev)
{
	if (dev->groups) {
		device_remove_groups(dev, dev->groups);
	}
}

/**
 * device_add - add device to device hierarchy.
 * @dev: device.
 *
 * This is part 2 of device_register(), though may be called
 * separately _iff_ device_initialize() has been called separately.
 *
 * This adds @dev to the kobject hierarchy via kobject_add(), adds it
 * to the global and sibling lists for the device, then
 * adds it to the other relevant subsystems of the driver model.
 *
 * Do not call this routine or device_register() more than once for
 * any device structure.  The driver model core is not designed to work
 * with devices that get unregistered and then spring back to life.
 * (Among other things, it's very hard to guarantee that all references
 * to the previous incarnation of @dev have been dropped.)  Allocate
 * and register a fresh new struct device instead.
 *
 * NOTE: _Never_ directly free @dev after calling this function, even
 * if it returned an error! Always use put_device() to give up your
 * reference instead.
 */
int device_add(struct device *dev)
{
	struct device *parent = NULL;
	struct kobject *kobj;
	struct class_interface *class_intf;
	int error = -EINVAL;

	device_register_qnx(dev);

	dev = get_device(dev);
	if (!dev)
		goto done;

	if (!dev->p) {
		error = device_private_init(dev);
		if (error)
			goto done;
	}

	/*
	 * for statically allocated devices, which should all be converted
	 * some day, we need to initialize the name. We prevent reading back
	 * the name, and force the use of dev_name()
	 */
	if (dev->init_name) {
		dev_set_name(dev, "%s", dev->init_name);
		dev->init_name = NULL;
	}

	/* subsystems can specify simple device enumeration */
	if (!dev_name(dev) && dev->bus && dev->bus->dev_name)
		dev_set_name(dev, "%s%u", dev->bus->dev_name, dev->id);

	if (!dev_name(dev)) {
		error = -EINVAL;
		goto name_error;
	}

	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);

	parent = get_device(dev->parent);
	kobj = get_device_parent(dev, parent);
	if (kobj)
		dev->kobj.parent = kobj;

	/* use parent numa_node */
	if (parent)
		set_dev_node(dev, dev_to_node(parent));

	/* first, register with generic layer. */
	/* we require the name to be set before, and pass NULL */
	error = kobject_add(&dev->kobj, dev->kobj.parent, NULL);
	if (error)
		goto Error;

	/* notify platform of device entry */
	if (platform_notify)
		platform_notify(dev);

	error = device_add_attrs(dev);
	if (error)
		goto AttrsError;
	error = bus_add_device(dev);
	if (error)
		goto BusError;

	if (MAJOR(dev->devt)) {
		error = device_create_file(dev, &dev_attr_dev);
		if (error)
			goto DevAttrError;

		devtmpfs_create_node(dev);
	}

	/* Notify clients of device addition.  This call must come
	 * after dpm_sysfs_add() and before kobject_uevent().
	 */
	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_ADD_DEVICE, dev);

	kobject_uevent(&dev->kobj, KOBJ_ADD);
	bus_probe_device(dev);
	if (parent)
		klist_add_tail(&dev->p->knode_parent,
			       &parent->p->klist_children);

	if (dev->class) {
		mutex_lock(&dev->class->p->mutex);
		/* tie the class to the device */
		klist_add_tail(&dev->knode_class,
			       &dev->class->p->klist_devices);

		/* notify any interfaces that the device is here */
		list_for_each_entry(class_intf,
				    &dev->class->p->interfaces, node)
			if (class_intf->add_dev)
				class_intf->add_dev(dev, class_intf);
		mutex_unlock(&dev->class->p->mutex);
	}
done:
	put_device(dev);
	return error;

DevAttrError:
	bus_remove_device(dev);
BusError:
	device_remove_attrs(dev);
AttrsError:
	kobject_uevent(&dev->kobj, KOBJ_REMOVE);
	kobject_del(&dev->kobj);
Error:
	put_device(parent);
name_error:
	kfree(dev->p);
	dev->p = NULL;
	goto done;
}
EXPORT_SYMBOL_GPL(device_add);

/**
 * device_del - delete device from system.
 * @dev: device.
 *
 * This is the first part of the device unregistration
 * sequence. This removes the device from the lists we control
 * from here, has it removed from the other driver model
 * subsystems it was added to in device_add(), and removes it
 * from the kobject hierarchy.
 *
 * NOTE: this should be called manually _iff_ device_add() was
 * also called manually.
 */
void device_del(struct device *dev)
{
	struct device *parent = dev->parent;
	struct kobject *glue_dir = NULL;
	struct class_interface *class_intf;

	/* Notify clients of device removal.  This call must come
	 * before dpm_sysfs_remove().
	 */
	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_DEL_DEVICE, dev);

	if (parent)
		klist_del(&dev->p->knode_parent);
	if (MAJOR(dev->devt)) {
		devtmpfs_delete_node(dev);
		device_remove_file(dev, &dev_attr_dev);
	}
	if (dev->class) {
		mutex_lock(&dev->class->p->mutex);
		/* notify any interfaces that the device is now gone */
		list_for_each_entry(class_intf,
				    &dev->class->p->interfaces, node)
			if (class_intf->remove_dev)
				class_intf->remove_dev(dev, class_intf);
		/* remove the device from the class list */
		klist_del(&dev->knode_class);
		mutex_unlock(&dev->class->p->mutex);
	}
	device_remove_attrs(dev);
	bus_remove_device(dev);
	driver_deferred_probe_del(dev);

	/* Notify the platform of the removal, in case they
	 * need to do anything...
	 */
	if (platform_notify_remove)
		platform_notify_remove(dev);
	if (dev->bus)
		blocking_notifier_call_chain(&dev->bus->p->bus_notifier,
					     BUS_NOTIFY_REMOVED_DEVICE, dev);
	kobject_uevent(&dev->kobj, KOBJ_REMOVE);
	kobject_del(&dev->kobj);
	put_device(parent);
}
EXPORT_SYMBOL_GPL(device_del);

/**
 * device_unregister - unregister device from system.
 * @dev: device going away.
 *
 * We do this in two parts, like we do device_register(). First,
 * we remove it from all the subsystems with device_del(), then
 * we decrement the reference count via put_device(). If that
 * is the final reference count, the device will be cleaned up
 * via device_release() above. Otherwise, the structure will
 * stick around until the final reference to the device is dropped.
 */
void device_unregister(struct device *dev)
{
	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
	device_del(dev);
	put_device(dev);
}
EXPORT_SYMBOL_GPL(device_unregister);

/**
 * dev_set_name - set a device name
 * @dev: device
 * @fmt: format string for the device's name
 */
int dev_set_name(struct device *dev, const char *fmt, ...)
{
	va_list vargs;

	va_start(vargs, fmt);
	kobject_set_name_vargs(&dev->kobj, fmt, vargs);
	va_end(vargs);
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/core.c $ $Rev: 853904 $")
#endif
