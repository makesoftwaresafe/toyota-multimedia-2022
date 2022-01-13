#include <linux/qnx.h>
#include <linux/linux.h>

struct devres_node {
	struct list_head		entry;
	dr_release_t			release;
#ifdef CONFIG_DEBUG_DEVRES
	const char			*name;
	size_t				size;
#endif
};

struct devres {
	struct devres_node		node;
	/* -- 3 pointers */
	unsigned long long		data[];	/* guarantee ull alignment */
};

#ifdef CONFIG_DEBUG_DEVRES
static int log_devres = 0;
module_param_named(log, log_devres, int, S_IRUGO | S_IWUSR);

static void set_node_dbginfo(struct devres_node *node, const char *name,
			     size_t size)
{
	node->name = name;
	node->size = size;
}

static void devres_log(struct device *dev, struct devres_node *node,
		       const char *op)
{
	if (unlikely(log_devres))
		dev_err(dev, "DEVRES %3s %p %s (%lu bytes)\n",
			op, node, node->name, (unsigned long)node->size);
}
#else /* CONFIG_DEBUG_DEVRES */
#define set_node_dbginfo(node, n, s)	do {} while (0)
#define devres_log(dev, node, op)	do {} while (0)
#endif /* CONFIG_DEBUG_DEVRES */

static void add_dr(struct device *dev, struct devres_node *node)
{
	devres_log(dev, node, "ADD");
	BUG_ON(!list_empty(&node->entry));
	list_add_tail(&node->entry, &dev->devres_head);
}

static __always_inline struct devres * alloc_dr(dr_release_t release,
						size_t size, gfp_t gfp)
{
	size_t tot_size = sizeof(struct devres) + size;
	struct devres *dr;

	dr = kmalloc(tot_size, gfp);
	if (unlikely(!dr))
		return NULL;

	memset(dr, 0, offsetof(struct devres, data));

	INIT_LIST_HEAD(&dr->node.entry);
	dr->node.release = release;
	return dr;
}

static void device_create_release(struct device *dev)
{
	pr_debug("device: '%s': %s\n", dev_name(dev), __func__);
	kfree(dev);
}

int kobject_set_name_vargs(struct kobject *kobj, const char *fmt, va_list vargs);

static struct device *
device_create_groups_vargs(struct class *class, struct device *parent,
			   dev_t devt, void *drvdata,
			   const struct attribute_group **groups,
			   const char *fmt, va_list args)
{
	struct device *dev = NULL;
	int retval = -ENODEV;

	if (class == NULL || IS_ERR(class))
		goto error;

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev) {
		retval = -ENOMEM;
		goto error;
	}

	device_initialize(dev);
	dev->devt = devt;
	dev->class = class;
	dev->parent = parent;
	dev->groups = groups;
	dev->release = device_create_release;
	dev_set_drvdata(dev, drvdata);

	retval = kobject_set_name_vargs(&dev->kobj, fmt, args);
	if (retval)
		goto error;

	retval = device_add(dev);
	if (retval)
		goto error;

	return dev;

error:
	put_device(dev);
	return ERR_PTR(retval);
}

struct device *device_create_with_groups(struct class *class,
					 struct device *parent, dev_t devt,
					 void *drvdata,
					 const struct attribute_group **groups,
					 const char *fmt, ...)
{
	va_list vargs;
	struct device *dev;

	va_start(vargs, fmt);
	dev = device_create_groups_vargs(class, parent, devt, drvdata, groups,
					 fmt, vargs);
	va_end(vargs);
	return dev;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/devices.c $ $Rev: 836322 $")
#endif
