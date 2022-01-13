#ifndef _DRM_RESMGR_H_
#define _DRM_RESMGR_H_

#include <uapi/drm/drm.h>

#include <sys/iofunc.h>

typedef struct _drm_ocb {
	iofunc_ocb_t hdr;
	void* ocb_ext_data;
	void* filedata;
} drm_ocb_t;

/*
 *  Define our device attributes structure.
*/
typedef struct _drm_device {
	iofunc_attr_t attr;
	void* device_ext_data;
} drm_device_t;

struct attr_list {
	struct list_head			list;	/**< Linked list of attrs */
	int 				count;
};

typedef struct _drm_ocb_ext_data {
	struct file filp;
	int filp_inited;
} drm_ocb_ext_data_t;

typedef struct _drm_device_ext_data {
	struct attr_list	*nodes;
	union {
		struct device_attribute dattr;
		struct bin_attribute battr;
	};
	char *buf;
	pid_t pid;
	struct inode inode;
	struct device *dev;
	struct sysfs_dirent *dir_sd;
	struct sysfs_dirent *sd;
	iofunc_attr_t* iofunc_attr;
} drm_device_ext_data_t;

/**
 * DRM nodes list struct
 */
struct drm_node_list {
	struct list_head	head;		/**< list head */
    const char			*name;
	int					count;
	drm_device_t		device;
	struct attr_list	attrs_list;	/**< list of attributes */
};

extern int __init qnx_drm_core_init();
extern void qnx_drm_core_exit();
extern int __init qnx_drm_kms_helper_init();
extern void qnx_drm_kms_helper_exit();
extern int __init qnx_mipi_dsi_bus_init();
extern void qnx_mipi_dsi_bus_exit();

extern int __init fbmem_init(void);
extern void fbmem_exit(void);

#define SYSFS_TYPE_MASK                 0x00ff
#define SYSFS_DIR                       0x0001
#define SYSFS_KOBJ_ATTR                 0x0002
#define SYSFS_KOBJ_BIN_ATTR             0x0004
/* identify any namespace tag on sysfs_dirents */
#define SYSFS_NS_TYPE_MASK              0xf00
#define SYSFS_NS_TYPE_SHIFT             8
#define SYSFS_FLAG_REMOVED              0x02000

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

struct sysfs_addrm_cxt {
        struct sysfs_dirent     *parent_sd;
        struct sysfs_dirent     *removed;
};

static inline unsigned int sysfs_type(struct sysfs_dirent *sd)
{
    return sd->s_flags & SYSFS_TYPE_MASK;
}

#endif /* _DRM_RESMGR_H_ */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/qnx_resmgr.h $ $Rev: 864420 $")
#endif
