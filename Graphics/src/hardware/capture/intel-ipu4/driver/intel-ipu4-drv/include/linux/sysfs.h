/*
 * sysfs.h - definitions for the device driver filesystem
 *
 * Copyright (c) 2001,2002 Patrick Mochel
 * Copyright (c) 2004 Silicon Graphics, Inc.
 * Copyright (c) 2007 SUSE Linux Products GmbH
 * Copyright (c) 2007 Tejun Heo <teheo@suse.de>
 * Some modifications Copyright (c) 2017 QNX Software Systems.
 *
 * Please see Documentation/filesystems/sysfs.txt for more information.
 */

#ifndef _QNX_LINUX_SYSFS_H_
#define _QNX_LINUX_SYSFS_H_

#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/list.h>
#include <linux/fs.h>
#ifndef __QNXNTO__
#include <linux/device.h>
#endif

#define SYSFS_TYPE_MASK                 0x00ff
#define SYSFS_DIR                       0x0001
#define SYSFS_KOBJ_ATTR                 0x0002
#define SYSFS_KOBJ_BIN_ATTR             0x0004
/* identify any namespace tag on sysfs_dirents */
#define SYSFS_NS_TYPE_MASK              0xf00
#define SYSFS_NS_TYPE_SHIFT             8
#define SYSFS_FLAG_REMOVED              0x02000


/*
 * Namespace types which are used to tag kobjects and sysfs entries.
 * Network namespace will likely be the first.
 */
enum kobj_ns_type {
        KOBJ_NS_TYPE_NONE = 0,
        KOBJ_NS_TYPE_NET,
        KOBJ_NS_TYPES
};


struct attribute {
	const char		*name;
	umode_t			mode;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
	bool			ignore_lockdep:1;
	struct lock_class_key	*key;
	struct lock_class_key	skey;
#endif
};

struct kobject;
struct vm_area_struct;
struct bin_attribute {
	struct attribute	attr;
	size_t			size;
	void			*private;
	ssize_t (*read)(struct file *, struct kobject *, struct bin_attribute *,
			char *, loff_t, size_t);
	ssize_t (*write)(struct file *,struct kobject *, struct bin_attribute *,
			 char *, loff_t, size_t);
	int (*mmap)(struct file *, struct kobject *, struct bin_attribute *attr,
		    struct vm_area_struct *vma);
};


struct sysfs_dirent;
/* type-specific structures for sysfs_dirent->s_* union members */
struct sysfs_elem_dir {
        struct kobject          *kobj;

        unsigned long           subdirs;
        /* children rbtree starts here and goes through sd->s_rb */
        struct rb_root          children;
};

struct sysfs_elem_symlink {
        struct sysfs_dirent     *target_sd;
};

struct sysfs_elem_attr {
        struct attribute        *attr;
        struct sysfs_open_dirent *open;
};

struct sysfs_elem_bin_attr {
        struct bin_attribute    *bin_attr;
        struct hlist_head       buffers;
};

struct sysfs_inode_attrs {
        struct iattr    ia_iattr;
        void            *ia_secdata;
        u32             ia_secdata_len;
};

/*
 * Context structure to be used while adding/removing nodes.
 */
struct sysfs_addrm_cxt {
        struct sysfs_dirent     *parent_sd;
        struct sysfs_dirent     *removed;
};

/*
 * sysfs_dirent - the building block of sysfs hierarchy.  Each and
 * every sysfs node is represented by single sysfs_dirent.
 *
 * As long as s_count reference is held, the sysfs_dirent itself is
 * accessible.  Dereferencing s_elem or any other outer entity
 * requires s_active reference.
 */
struct sysfs_dirent {
        atomic_t                s_count;
        atomic_t                s_active;
#ifdef CONFIG_DEBUG_LOCK_ALLOC
        struct lockdep_map      dep_map;
#endif
        struct sysfs_dirent     *s_parent;
        struct sysfs_dirent     *s_sibling;
        const char              *s_name;

        struct rb_node          s_rb;

        union {
                struct completion       *completion;
                struct sysfs_dirent     *removed_list;
        } u;

        const void              *s_ns; /* namespace tag */
        unsigned int            s_hash; /* ns + name hash */
        union {
                struct sysfs_elem_dir           s_dir;
                struct sysfs_elem_symlink       s_symlink;
                struct sysfs_elem_attr          s_attr;
                struct sysfs_elem_bin_attr      s_bin_attr;
        };

        unsigned short          s_flags;
        umode_t                 s_mode;
        unsigned int            s_ino;
        struct sysfs_inode_attrs *s_iattr;
        void					*list;
};



#define __ATTR(_name, _mode, _show, _store) {				\
	.attr = {.name = __stringify(_name), .mode = _mode },		\
	.show	= _show,						\
	.store	= _store,						\
}

#define __ATTR_RO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IRUGO },	\
	.show	= _name##_show,						\
}

#define __ATTR_WO(_name) {						\
	.attr	= { .name = __stringify(_name), .mode = S_IWUSR },	\
	.store	= _name##_store,					\
}

#define __ATTR_RW(_name) __ATTR(_name, (S_IWUSR | S_IRUGO),		\
			 _name##_show, _name##_store)

#define __ATTR_NULL { .attr = { .name = NULL } }

// TR - added
#define __ATTRIBUTE_GROUPS(_name)				\
static const struct attribute_group *_name##_groups[] = {	\
	&_name##_group,						\
	NULL,							\
}

#define ATTRIBUTE_GROUPS(_name)					\
static const struct attribute_group _name##_group = {		\
	.attrs = _name##_attrs,					\
};								\
__ATTRIBUTE_GROUPS(_name)


#define __must_check 

int __must_check sysfs_create_file(struct kobject *kobj,
								   const struct attribute *attr);

void sysfs_remove_file(struct kobject *kobj, const struct attribute *attr);

int sysfs_create_link(struct kobject *kobj, struct kobject *target, const char *name);
void sysfs_remove_link(struct kobject *kobj, const char *name);

int sysfs_create_bin_file(struct kobject *kobj,
						  const struct bin_attribute *attr);
void sysfs_remove_bin_file(struct kobject *kobj,
						   const struct bin_attribute *attr);

int sysfs_create_files(struct kobject *kobj,
					   const struct attribute **attr);
void sysfs_remove_file(struct kobject *kobj,
					   const struct attribute *attr);
void sysfs_remove_files(struct kobject *kobj,
						const struct attribute **attr);

void sysfs_remove_dir(struct kobject *kobj);


static inline unsigned int sysfs_type(struct sysfs_dirent *sd)
{
    return sd->s_flags & SYSFS_TYPE_MASK;
}

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/sysfs.h $ $Rev: 838597 $")
#endif
