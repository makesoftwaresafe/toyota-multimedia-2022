#include <linux/qnx.h>
#include <linux/linux.h>

#include "base.h"

#define to_class_attr(_attr) container_of(_attr, struct class_attribute, attr)

static ssize_t class_attr_show(struct kobject *kobj, struct attribute *attr,
			       char *buf)
{
	struct class_attribute *class_attr = to_class_attr(attr);
	struct subsys_private *cp = to_subsys_private(kobj);
	ssize_t ret = -EIO;

	if (class_attr->show)
		ret = class_attr->show(cp->class, class_attr, buf);
	return ret;
}

static ssize_t class_attr_store(struct kobject *kobj, struct attribute *attr,
				const char *buf, size_t count)
{
	struct class_attribute *class_attr = to_class_attr(attr);
	struct subsys_private *cp = to_subsys_private(kobj);
	ssize_t ret = -EIO;

	if (class_attr->store)
		ret = class_attr->store(cp->class, class_attr, buf, count);
	return ret;
}

static void class_release(struct kobject *kobj)
{
	struct subsys_private *cp = to_subsys_private(kobj);
	struct class *class = cp->class;

	pr_debug("class '%s': release.\n", class->name);

	if (class->class_release)
		class->class_release(class);
	else
		pr_debug("class '%s' does not have a release() function, "
			 "be careful\n", class->name);

	kfree(cp);
}

static const struct kobj_ns_type_operations *class_child_ns_type(struct kobject *kobj)
{
	struct subsys_private *cp = to_subsys_private(kobj);
	struct class *class = cp->class;

	return class->ns_type;
}

static const struct sysfs_ops class_sysfs_ops = {
	.show	   = class_attr_show,
	.store	   = class_attr_store,
};

static struct kobj_type class_ktype = {
	.sysfs_ops	= &class_sysfs_ops,
	.release	= class_release,
	.child_ns_type	= class_child_ns_type,
};

static struct kset *class_kset;

static struct class *class_get(struct class *cls)
{
	if (cls)
		kset_get(&cls->p->subsys);
	return cls;
}

static void class_put(struct class *cls)
{
	if (cls)
		kset_put(&cls->p->subsys);
}

int class_create_file_ns(struct class *cls, const struct class_attribute *attr, const void *ns)
{
	 /* MG_TODO. add qnx support */
	return 0;
}

void class_remove_file_ns(struct class *class, const struct class_attribute *attr, const void *ns)
{
//	if (class)
//		sysfs_remove_file(&class->p->subsys.kobj, &attr->attr);
}

/**
 * class_destroy - destroys a struct class structure
 * @cls: pointer to the struct class that is to be destroyed
 *
 * Note, the pointer to be destroyed must have been created with a call
 * to class_create().
 */
void class_destroy(struct class *cls)
{
	if ((cls == NULL) || (IS_ERR(cls)))
		return;
	//TODO: class_unregister(cls);
}

ssize_t show_class_attr_string(struct class *class,
                               struct class_attribute *attr, char *buf)
{
	struct class_attribute_string *cs;
	cs = container_of(attr, struct class_attribute_string, attr);
	return snprintf(buf, PAGE_SIZE, "%s\n", cs->str);
}

void class_create_release(struct class *cls)
{
	kfree(cls);
}

static void klist_class_dev_get(struct klist_node *n)
{
	struct device *dev = container_of(n, struct device, knode_class);

	get_device(dev);
}

static void klist_class_dev_put(struct klist_node *n)
{
	struct device *dev = container_of(n, struct device, knode_class);

	put_device(dev);
}

int __class_register(struct class *cls, struct lock_class_key *key)
{
	struct subsys_private *cp;
	int error;

	pr_debug("device class '%s': registering\n", cls->name);

	cp = kzalloc(sizeof(*cp), GFP_KERNEL);
	if (!cp)
		return -ENOMEM;
	klist_init(&cp->klist_devices, klist_class_dev_get, klist_class_dev_put);
	INIT_LIST_HEAD(&cp->interfaces);
	kset_init(&cp->glue_dirs);
	mutex_init(&cp->mutex);
	error = kobject_set_name(&cp->subsys.kobj, "%s", cls->name);
	if (error) {
		kfree(cp);
		return error;
	}

	/* set the default /sys/dev directory for devices of this class */
	if (!cls->dev_kobj)
		cls->dev_kobj = sysfs_dev_char_kobj;

#if defined(CONFIG_BLOCK)
	/* let the block class directory show up in the root of sysfs */
	if (!sysfs_deprecated || cls != &block_class)
		cp->subsys.kobj.kset = class_kset;
#else
	cp->subsys.kobj.kset = class_kset;
#endif
	cp->subsys.kobj.ktype = &class_ktype;
	cp->class = cls;
	cls->p = cp;

	error = kset_register(&cp->subsys);
	if (error) {
		kfree(cp);
		return error;
	}

	return error;
}
EXPORT_SYMBOL_GPL(__class_register);

/**
 * class_create - create a struct class structure
 * @owner: pointer to the module that is to "own" this struct class
 * @name: pointer to a string for the name of this class.
 * @key: the lock_class_key for this class; used by mutex lock debugging
 *
 * This is used to create a struct class pointer that can then be used
 * in calls to device_create().
 *
 * Returns &struct class pointer on success, or ERR_PTR() on error.
 *
 * Note, the pointer created here is to be destroyed when finished by
 * making a call to class_destroy().
 */
struct class * __must_check __class_create(struct module *owner, const char *name,
    struct lock_class_key *key)
{
        struct class *cls;
        int retval;

        cls = kzalloc(sizeof(*cls), GFP_KERNEL);
        if (!cls) {
                retval = -ENOMEM;
                goto error;
        }

        cls->name = name;
        cls->owner = owner;
        cls->class_release = class_create_release;

        retval = __class_register(cls, key);
        if (retval)
                goto error;

        return cls;

error:
        kfree(cls);
        return ERR_PTR(retval);
}

int classes_init(void)
{
	class_kset = kset_create_and_add("class", NULL, NULL);
	if (!class_kset)
		return -ENOMEM;
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/class.c $ $Rev: 853159 $")
#endif
