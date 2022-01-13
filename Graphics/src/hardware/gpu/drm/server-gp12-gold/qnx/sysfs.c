#undef IOFUNC_ATTR_T
#undef IOFUNC_OCB_T
struct _sysfs_device;
struct _sysfs_ocb;
#define IOFUNC_ATTR_T   struct _sysfs_device
#define IOFUNC_OCB_T    struct _sysfs_ocb

#include <stdint.h>
#include <pthread.h>
#include <sys/usbdi.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/rsrcdbmgr.h>
#include <sys/resmgr.h>

#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/qnx_resmgr.h>
#include "drmP.h"

extern struct attr_list minors_list;   /**< Linked list of minors */
extern struct attr_list groups_list;   /**< Linked list of groups */
extern int last_inode;

DEFINE_MUTEX(sysfs_mutex);

/* Since sysfs mostly operates with singular numeric values we */
/* simply allocate a page for temporary buffer. */
#define SYSFS_MIN_BUFFER_SIZE (4096)

extern dispatch_t* dpp;

struct _sysfs_device;

typedef struct _sysfs_ocb {
	iofunc_ocb_t		hdr;
	struct _sysfs_device*	sysfs;
	int			filesize;
	int			datasize;
	char*			filedata;
} sysfs_ocb_t;

typedef struct _sysfs_device {
	iofunc_attr_t		hdr;
	int			resmgr_id;
	resmgr_io_funcs_t	io_funcs;
	resmgr_connect_funcs_t	connect_funcs;
	iofunc_funcs_t		ocb_funcs;
	iofunc_mount_t		io_mount;
	resmgr_attr_t		rattr;
	int			sysfs_attr_type;
	const struct attribute*	sysfs_attr;
	struct kobject*		sysfs_kobj;
	char			name[PATH_MAX];
} sysfs_device_t;

int claim_task_structure(resmgr_context_t *ctp, int inopen);
int unclaim_task_structure(resmgr_context_t *ctp, int inclose);

static sysfs_ocb_t* _sysfs_ocb_calloc(resmgr_context_t* ctp, sysfs_device_t* sysfs_dev)
{
	sysfs_ocb_t* ocb;
	int err;
	ssize_t readsize;
	loff_t readpos;

	ocb = calloc(1, sizeof(sysfs_ocb_t));
	if (ocb == NULL) {
		return NULL;
	}
	ocb->sysfs = sysfs_dev;

	ocb->filedata = calloc(1, SYSFS_MIN_BUFFER_SIZE);
	if (ocb->filedata != NULL) {
		ocb->datasize = SYSFS_MIN_BUFFER_SIZE;
	} else {
		free(ocb);
		return NULL;
	}

	claim_task_structure(ctp, 0);

	/* Since we handle sysfs directly in RM we can use memcpy() instead copy_to_user() */
	current->attachment.copy_to_user_memcpy = 1;

	/* Set filesize to zero in case if read()/show() is not available in fops */
	ocb->filesize = 0;

	switch (sysfs_dev->sysfs_attr_type) {
		case SYSFS_KOBJ_ATTR: {
				struct device_attribute* dev_attr = container_of(sysfs_dev->sysfs_attr, struct device_attribute, attr);

				if (dev_attr->show) {
					readsize = dev_attr->show(kobj_to_dev(sysfs_dev->sysfs_kobj), dev_attr, ocb->filedata);
					if (readsize >= 0) {
						ocb->filesize += readsize;
					}
				}
			}
			break;
		case SYSFS_KOBJ_BIN_ATTR: {
				struct bin_attribute* bin_attr = container_of(sysfs_dev->sysfs_attr, struct bin_attribute, attr);

				if (bin_attr->read) {
					do {
						readsize = bin_attr->read(NULL, sysfs_dev->sysfs_kobj, bin_attr,
								ocb->filedata + ocb->filesize, ocb->filesize, SYSFS_MIN_BUFFER_SIZE);
						if (readsize >= 0) {
							ocb->filesize += readsize;
							if (readsize == SYSFS_MIN_BUFFER_SIZE) {
								ocb->filedata = realloc(ocb->filedata, ocb->filesize + SYSFS_MIN_BUFFER_SIZE);
							} else {
								break;
							}
						} else {
							break;
						}
					} while(1);
				}
			}
			break;
		default:
			/* Invalid sysfs attribute type */
			free(ocb->filedata);
			free(ocb);
			unclaim_task_structure(ctp, 0);
			return NULL;
	}

	/* Update file size in filesystem representation */
	sysfs_dev->hdr.nbytes = ocb->filesize;

	unclaim_task_structure(ctp, 0);

	return ocb;
}

static void _sysfs_ocb_free(sysfs_ocb_t* ocb)
{
	free(ocb->filedata);
	free(ocb);
}

static int resmgr_open_sysfs(resmgr_context_t* ctp, io_open_t* msg, RESMGR_HANDLE_T* handle, void* extra)
{
	int status;

	status=iofunc_open_default(ctp, msg, (iofunc_attr_t*)handle, extra);
	if (status) {
		return status;
	}

	return EOK;
}

static int resmgr_close_sysfs(resmgr_context_t* ctp, void* reserved, sysfs_ocb_t* ocb)
{
	int status;

	/* This function must be called last, because it destroys OCB */
	status=iofunc_close_ocb_default(ctp, reserved, (iofunc_ocb_t*)ocb);
	if (status) {
		return status;
	}

	return EOK;
}

static int resmgr_read_sysfs(resmgr_context_t* ctp, io_read_t* msg, sysfs_ocb_t* ocb)
{
	int result;
	int nparts = 0;
	int nonblock = 0;
	int nbytes;
	int nleft;

	result = iofunc_read_verify(ctp, msg, (iofunc_ocb_t*)ocb, &nonblock);
	if (result != EOK) {
		_IO_SET_READ_NBYTES(ctp, -1);
		return result;
	}
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		_IO_SET_READ_NBYTES(ctp, -1);
		return ENOSYS;
	}

	nleft = ocb->hdr.attr->hdr.nbytes - ocb->hdr.offset;
	nbytes = min(msg->i.nbytes, nleft);

	if (nbytes > 0) {
		SETIOV(ctp->iov, ocb->filedata + ocb->hdr.offset, nbytes);
		_IO_SET_READ_NBYTES(ctp, nbytes);
		ocb->hdr.offset += nbytes;
		nparts = 1;
	} else {
		_IO_SET_READ_NBYTES(ctp, 0);
	}

	/* Update file access time */
	ocb->hdr.attr->hdr.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;

	return (_RESMGR_NPARTS(nparts));
}

static int resmgr_write_sysfs(resmgr_context_t* ctp, io_write_t* msg, sysfs_ocb_t* ocb)
{
	int result;
	int nonblock = 0;
	char* data;
	int err;
	struct file dfile;

	result = iofunc_write_verify(ctp, msg, (iofunc_ocb_t*)ocb, &nonblock);
	if (result != EOK) {
		_IO_SET_WRITE_NBYTES(ctp, -1);
		return result;
	}
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		_IO_SET_WRITE_NBYTES(ctp, -1);
		return ENOSYS;
	}
	if (msg->i.nbytes == 0) {
		return _RESMGR_ERRNO(EOK);
	}

	if (msg->i.nbytes > 0) {
		ssize_t writesize = -1;
		int datasize;

		data = sizeof(msg->i) + (char *)&msg->i;
		if (msg->i.nbytes < SYSFS_MIN_BUFFER_SIZE) {
			datasize = msg->i.nbytes;
		} else {
			datasize = SYSFS_MIN_BUFFER_SIZE - 1;
		}

		claim_task_structure(ctp, 0);

		/*
		 * Since we handle sysfs directly in RM
		 * we can use memcpy() instead copy_to_user()
		 */
		current->attachment.copy_to_user_memcpy = 1;

		switch (ocb->sysfs->sysfs_attr_type) {
			case SYSFS_KOBJ_ATTR: {
				struct device_attribute* dev_attr =
					container_of(
						ocb->sysfs->sysfs_attr,
						struct device_attribute,
						attr);

				if (!dev_attr->store) {
					writesize = -ENOTSUP;
					break;
				}

				/* Emulate linux behavior */
				memcpy(ocb->filedata, data, datasize);
				ocb->filedata[datasize] = 0x00;

				writesize = dev_attr->store(
					kobj_to_dev(
						ocb->sysfs->
						sysfs_kobj),
					dev_attr,
					ocb->filedata,
					datasize);
				break;
			}
			case SYSFS_KOBJ_BIN_ATTR: {
				struct bin_attribute* bin_attr =
					container_of(
						ocb->sysfs->sysfs_attr,
						struct bin_attribute,
						attr);

				/*
				 * We do not support writes by offset,
				 * since sysfs works as write
				 * all-at-once or like a trigger
				 * on write
				 */
				if (!bin_attr->write) {
					writesize = -ENOTSUP;
					break;
				}

				/* Emulate linux behavior */
				memcpy(ocb->filedata, data, datasize);
				ocb->filedata[datasize] = 0x00;
				writesize = bin_attr->write(
					NULL,
					ocb->sysfs->sysfs_kobj,
					bin_attr,
					ocb->filedata, 0,
					datasize);
				break;
			}
			default:
				writesize = -EIO;
				/* Invalid sysfs attribute type */
		}

		unclaim_task_structure(ctp, 0);
		_IO_SET_WRITE_NBYTES(ctp,
				writesize >=0 ? writesize : -1);
		if (writesize < 0) {
			return _RESMGR_ERRNO(-writesize);
		}

	}

	/* Update file access time */
	ocb->hdr.attr->hdr.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;

	return _RESMGR_ERRNO(EOK);
}

#define RESMGR_MAX_SYSFS_ENTRIES 64

struct _sysfs_device* sysfs_entries[RESMGR_MAX_SYSFS_ENTRIES] = { NULL, };

static int resmgr_register_sysfs(const char* name, struct kobject *kobj, const struct attribute* attr, int type)
{
	struct _sysfs_device* sysfs;
	int it;

	mutex_lock(&sysfs_mutex);
	for (it = 0; it < RESMGR_MAX_SYSFS_ENTRIES; it++) {
		if (sysfs_entries[it] != NULL) {
			if (strncmp(sysfs_entries[it]->name, name, sizeof(sysfs_entries[it]->name) - 1) == 0) {
				sysfs = sysfs_entries[it];
				sysfs_entries[it] = NULL;
				break;
			}
		}
	}

	if (it == RESMGR_MAX_SYSFS_ENTRIES) {
		for (it = 0; it < RESMGR_MAX_SYSFS_ENTRIES; it++) {
			if (sysfs_entries[it] == NULL) {
				sysfs_entries[it] = (void*)(uintptr_t)0xDEADBEAF;
				break;
			}
		}
	} else {
		/* Do not allow register duplicates, this is exact linux sysfs kernel API behavior */
		mutex_unlock(&sysfs_mutex);
		return -1;
	}
	mutex_unlock(&sysfs_mutex);
	if (it == RESMGR_MAX_SYSFS_ENTRIES) {
		return -1;
	}

	sysfs = calloc(1, sizeof(*sysfs));
	if (sysfs == NULL) {
		return -1;
	}
	sysfs_entries[it] = sysfs;

	strncpy(sysfs->name, name, sizeof(sysfs->name) - 1);
	sysfs->sysfs_attr_type = type;
	sysfs->sysfs_attr = attr;

	sysfs->sysfs_kobj = kobj;

	memset(&sysfs->ocb_funcs, 0x00, sizeof(sysfs->ocb_funcs));
	sysfs->ocb_funcs.nfuncs = _IOFUNC_NFUNCS;
	sysfs->ocb_funcs.ocb_calloc = _sysfs_ocb_calloc;
	sysfs->ocb_funcs.ocb_free = _sysfs_ocb_free;
	memset(&sysfs->io_mount, 0x00, sizeof(sysfs->io_mount));
	sysfs->io_mount.funcs = &sysfs->ocb_funcs;

	memset(&sysfs->rattr, 0, sizeof(sysfs->rattr));
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &sysfs->connect_funcs, _RESMGR_IO_NFUNCS, &sysfs->io_funcs);
	iofunc_attr_init(&sysfs->hdr, _S_IFREG | attr->mode, NULL, NULL);

	sysfs->connect_funcs.open = resmgr_open_sysfs;
	sysfs->io_funcs.read = resmgr_read_sysfs;
	sysfs->io_funcs.write = resmgr_write_sysfs;
	sysfs->io_funcs.close_ocb = resmgr_close_sysfs;

	sysfs->hdr.mount = &sysfs->io_mount;

	sysfs->resmgr_id = resmgr_attach(dpp, &sysfs->rattr, name, _FTYPE_ANY, 0, &sysfs->connect_funcs,
		&sysfs->io_funcs, (RESMGR_HANDLE_T*)&sysfs->hdr);
	if (sysfs->resmgr_id == -1) {
		free(sysfs);
		DRM_ERROR("resmgr_attach() has been failed for SYSFS device: %s", name);
		return -1;
	}
	resmgr_devino(sysfs->resmgr_id, &sysfs->io_mount.dev, &sysfs->hdr.inode);

	return 0;
}

static int resmgr_unregister_sysfs(const char* name, struct kobject *kobj, const struct attribute* attr, int type)
{
	struct _sysfs_device* sysfs = NULL;
	int it;

	mutex_lock(&sysfs_mutex);
	for (it = 0; it < RESMGR_MAX_SYSFS_ENTRIES; it++) {
		if (sysfs_entries[it] != NULL) {
			if (strncmp(sysfs_entries[it]->name, name, sizeof(sysfs_entries[it]->name) - 1) == 0) {
				sysfs = sysfs_entries[it];
				sysfs_entries[it] = NULL;
				break;
			}
		}
	}
	mutex_unlock(&sysfs_mutex);
	if (it == RESMGR_MAX_SYSFS_ENTRIES) {
		return -1;
	}

	resmgr_detach(dpp, sysfs->resmgr_id, _RESMGR_DETACH_ALL);

	return 0;
}

int dev_uevent_filter(struct kset *kset, struct kobject *kobj);

static void sysfs_set_path(struct kobject* kobj, char* path, int size, const char* grp_name)
{
	path[0] = 0;

	/* Perform sanity checks because there could be some detached kernel objects */
	if (kobj->name && kobj->parent) {
		struct device* device;

		device = NULL;
		if (dev_uevent_filter(NULL, kobj)) {
			device = kobj_to_dev(kobj);
		}

		/* MG_TODO: Make a recursive traverse or at least change to one single snprintf() */

		if (device && device->class && device->class->name) {
			/* "class" could be extracted from parent class, but it is a decent amount of work */
			strncpy(path, "/sys/class/", size - 1);
			strncat(path, device->class->name, size - 1);
			strncat(path, "/", size - 1);
			strncat(path, kobj->name, size - 1);
			strncat(path, "/", size - 1);
		} else {
			device = NULL;
			if (kobj->parent && dev_uevent_filter(NULL, kobj->parent)) {
				device = kobj_to_dev(kobj->parent);
			}

			if (device && device->class && device->class->name) {
				/* "class" could be extracted from parent class, but it is a decent amount of work */
				strncpy(path, "/sys/class/", size - 1);
				strncat(path, device->class->name, size - 1);
				strncat(path, "/", size - 1);
				strncat(path, kobj->parent->name, size - 1);
				strncat(path, "/", size - 1);
				strncat(path, kobj->name, size - 1);
				strncat(path, "/", size - 1);
			} else {
				device = NULL;
				if (kobj->parent->parent && dev_uevent_filter(NULL, kobj->parent->parent)) {
					device = kobj_to_dev(kobj->parent->parent);
				}

				if (device && device->class && device->class->name) {
					strncpy(path, "/sys/class/", size - 1);
					strncat(path, device->class->name, size - 1);
					strncat(path, "/", size - 1);
					strncat(path, kobj->parent->parent->name, size - 1);
					strncat(path, "/", size - 1);
					strncat(path, kobj->parent->name, size - 1);
					strncat(path, "/", size - 1);
					strncat(path, kobj->name, size - 1);
					strncat(path, "/", size - 1);
				}
			}
		}
		if (grp_name != NULL) {
			strncat(path, grp_name, size - 1);
			strncat(path, "/", size - 1);
		}
	}
}

static int sysfs_create_qnxfs_file(struct kobject *kobj, const struct attribute* attr, int type, const char* grp_name)
{
	char filename[PATH_MAX];
	bool path_resolved = false;

	/* Create a real sysfs reflection of attributes */
	sysfs_set_path(kobj, filename, sizeof(filename), grp_name);

	if (strlen(filename) != 0) {
		path_resolved = true;
	}

	if (!path_resolved) {
		/* We resolve only small subset of devices which belong to class devices */
		/* Bus devices are completely ignored. */
		return 0;
	}

	/* Create target fs filename */
	strncat(filename, attr->name, sizeof(filename) - 1);

	DRM_DEBUG_DRIVER("Registering sysfs filename: %s\n", filename);

	resmgr_register_sysfs(filename, kobj, attr, type);

	return 0;
}

static int sysfs_remove_qnxfs_file(struct kobject* kobj, const struct attribute* attr, int type, const char* grp_name)
{
	char filename[PATH_MAX];
	bool path_resolved = false;

	/* Create a real sysfs reflection of attributes */
	sysfs_set_path(kobj, filename, sizeof(filename), grp_name);

	if (strlen(filename) != 0) {
		path_resolved = true;
	}

	if (!path_resolved) {
		/* We resolve only small subset of devices which belong to class devices */
		/* Bus devices are completely ignored. */
		return 0;
	}

	/* Create target fs filename */
	strncat(filename, attr->name, sizeof(filename) - 1);

	DRM_DEBUG_DRIVER("Unregistering sysfs filename: %s\n", filename);
	resmgr_unregister_sysfs(filename, kobj, attr, type);

	return 0;
}

int sysfs_add_file(struct sysfs_dirent *dir_sd, const struct attribute *attr, int type)
{
	struct drm_node_list	*list;
	struct drm_node_list	*group_list;
	struct device_attribute *dev_attr;
	struct bin_attribute 	*bin_attr;
	struct sysfs_dirent 	*sd = NULL;

	if (dir_sd == NULL) {
		return EOK;
	}

	group_list = dir_sd->list;

	drm_device_ext_data_t* parent_attr = (drm_device_ext_data_t *)group_list->device.device_ext_data;
	list = kzalloc(sizeof(*list), GFP_KERNEL);
	if (!list) {
		return -ENOMEM;
	}

	iofunc_attr_init(&list->device.attr, S_IFREG | attr->mode, 0, 0);
	list->device.attr.rdev = 0;

	group_list->count++;
	list->device.attr.inode = ++last_inode;
	list->device.attr.nbytes = strlen(attr->name);
	list->name = attr->name;

	drm_device_ext_data_t *attr_priv = kzalloc(sizeof(drm_device_ext_data_t), GFP_KERNEL);
	if (attr_priv) {
		attr_priv->dir_sd = dir_sd;
		sd = kzalloc (sizeof(struct sysfs_dirent), GFP_KERNEL);
		if (sd) {
			sd->s_name = attr->name;
			sd->s_mode = attr->mode;
			sd->s_flags = type;
			attr_priv->sd = sd;
		}

		switch (type){
		case SYSFS_KOBJ_BIN_ATTR:
			bin_attr = container_of(attr, struct bin_attribute, attr);
			attr_priv->battr.read = bin_attr->read;
			attr_priv->battr.write = bin_attr->write;
			break;
		case SYSFS_KOBJ_ATTR:
		default:
			dev_attr = container_of(attr, struct device_attribute, attr);
			attr_priv->dattr.show = dev_attr->show;
			attr_priv->dattr.store = dev_attr->store;
			break;
		}

		attr_priv->dev = parent_attr->dev;
		group_list->attrs_list.count++;
		attr_priv->iofunc_attr = &list->device.attr;
	} else {
		DRM_ERROR("Not enough memory!");
	}
	list->device.device_ext_data = (void *)attr_priv;

	list_add(&list->head, &group_list->attrs_list.list);

	parent_attr->iofunc_attr->nbytes++;

	return EOK;
}

/**
 *      sysfs_create_file - create an attribute file for an object.
 *      @kobj:  object we're creating for.
 *      @attr:  attribute descriptor.
 */
int sysfs_create_file_ns(struct kobject * kobj, const struct attribute * attr, const void* ns)
{
	(void)ns;
	int ret = 0;

	BUG_ON(!kobj || !attr);
	ret = sysfs_add_file((void*)kobj->sd, attr, SYSFS_KOBJ_ATTR);
	ret |= sysfs_create_qnxfs_file(kobj, attr, SYSFS_KOBJ_ATTR, NULL);

	return ret;
}

int sysfs_create_link(struct kobject *kobj, struct kobject *target, const char *name)
{
	return 0;
}
void sysfs_remove_link(struct kobject *kobj, const char *name)
{
}

/**
  *      sysfs_remove_file - remove an object attribute.
  *      @kobj:  object we're acting for.
  *      @attr:  attribute descriptor.
  *
  *      Hash the attribute name and kill the victim.
  */

void sysfs_remove_file_ns(struct kobject *kobj, const struct attribute * attr, const void* ns)
{
	(void)ns;

	BUG_ON(!kobj || !attr);
	sysfs_remove_qnxfs_file(kobj, attr, SYSFS_KOBJ_ATTR, NULL);

	return;
}

int sysfs_create_files(struct kobject *kobj, const struct attribute **ptr)
{
	int err = 0;
	int i;

	for (i = 0; ptr[i] && !err; i++) {
		err |= sysfs_create_file(kobj, ptr[i]);
		if (err) {
			break;
		}
	}
	if (err) {
		while (--i >= 0) {
			sysfs_remove_file(kobj, ptr[i]);
		}
	}

	return err;
}

void sysfs_remove_files(struct kobject * kobj, const struct attribute **ptr)
{
	int i;
	for (i = 0; ptr[i]; i++) {
		sysfs_remove_file(kobj, ptr[i]);
	}
}

/**
 *      sysfs_create_bin_file - create binary file for object.
 *      @kobj:  object.
 *      @attr:  attribute descriptor.
 */

int sysfs_create_bin_file(struct kobject *kobj, const struct bin_attribute *attr)
{
	int ret = 0;
	BUG_ON(!kobj || !attr);

	ret = sysfs_add_file((void*)kobj->sd, &attr->attr, SYSFS_KOBJ_BIN_ATTR);
	ret |= sysfs_create_qnxfs_file(kobj, &attr->attr, SYSFS_KOBJ_BIN_ATTR, NULL);

	return ret;
}

/**
  *      sysfs_remove_bin_file - remove binary file for object.
  *      @kobj:  object.
  *      @attr:  attribute descriptor.
  */
void sysfs_remove_bin_file(struct kobject *kobj, const struct bin_attribute *attr)
{
	BUG_ON(!kobj || !attr);

	sysfs_remove_qnxfs_file(kobj, &attr->attr, SYSFS_KOBJ_BIN_ATTR, NULL);

	return;
}

/**
 *      sysfs_addrm_start - prepare for sysfs_dirent add/remove
 *      @acxt: pointer to sysfs_addrm_cxt to be used
 *      @parent_sd: parent sysfs_dirent
 *
 *      This function is called when the caller is about to add or
 *      remove sysfs_dirent under @parent_sd.  This function acquires
 *      sysfs_mutex.  @acxt is used to keep and pass context to
 *      other addrm functions.
 *
 *      LOCKING:
 *      Kernel thread context (may sleep).  sysfs_mutex is locked on
 *      return.
 */
void sysfs_addrm_start(struct sysfs_addrm_cxt *acxt,
                        struct sysfs_dirent *parent_sd)
{
        memset(acxt, 0, sizeof(*acxt));
        acxt->parent_sd = parent_sd;

        mutex_lock((void*)&sysfs_mutex);
}

/**
 *      sysfs_addrm_finish - finish up sysfs_dirent add/remove
 *      @acxt: addrm context to finish up
 *
 *      Finish up sysfs_dirent add/remove.  Resources acquired by
 *      sysfs_addrm_start() are released and removed sysfs_dirents are
 *      cleaned up.
 *
 *      LOCKING:
 *      sysfs_mutex is released.
 */
void sysfs_addrm_finish(struct sysfs_addrm_cxt *acxt)
{
        /* release resources acquired by sysfs_addrm_start() */
        mutex_unlock((void*)&sysfs_mutex);
}

/*
 * Return any namespace tags on this dirent.
 * enum kobj_ns_type is defined in linux/kobject.h
 */
enum kobj_ns_type sysfs_ns_type(struct sysfs_dirent *sd)
{
        return (sd->s_flags & SYSFS_NS_TYPE_MASK) >> SYSFS_NS_TYPE_SHIFT;
}

/* Name hashing routines. Initial hash value */
/* Hash courtesy of the R5 hash in reiserfs modulo sign bits */
#define init_name_hash()                0

/**
  *      sysfs_name_hash
  *      @ns:   Namespace tag to hash
  *      @name: Null terminated string to hash
  *
  *      Returns 31 bit hash of ns + name (so it fits in an off_t )
  */
static unsigned int sysfs_name_hash(const void *ns, const char *name)
{
        unsigned long hash = init_name_hash();
        unsigned int len = strlen(name);
        while (len--)
                hash = partial_name_hash(*name++, hash);
        hash = ( end_name_hash(hash) ^ hash_ptr( (void *)ns, 31 ) );
        hash &= 0x7fffffffU;
        /* Reserve hash numbers 0, 1 and INT_MAX for magic directory entries */
        if (hash < 1)
                hash += 2;
        if (hash >= INT_MAX)
                hash = INT_MAX - 1;
        return hash;
}

#define to_sysfs_dirent(X) container_of((X), struct sysfs_dirent, s_rb);

static int sysfs_name_compare(unsigned int hash, const void *ns,
        const char *name, const struct sysfs_dirent *sd)
{
        if (hash != sd->s_hash)
                return hash - sd->s_hash;
        if (ns != sd->s_ns)
                return ns - sd->s_ns;
        return strcmp(name, sd->s_name);
}

/**
 *      sysfs_find_dirent - find sysfs_dirent with the given name
 *      @parent_sd: sysfs_dirent to search under
 *      @name: name to look for
 *
 *      Look for sysfs_dirent with name @name under @parent_sd.
 *
 *      LOCKING:
 *      mutex_lock(sysfs_mutex)
 *
 *      RETURNS:
 *      Pointer to sysfs_dirent if found, NULL if not.
 */
struct sysfs_dirent *sysfs_find_dirent(struct sysfs_dirent *parent_sd,
                                       const void *ns,
                                       const unsigned char *name)
{
        struct rb_node *node = parent_sd->s_dir.children.rb_node;
        unsigned int hash;

        if (!!sysfs_ns_type(parent_sd) != !!ns) {
                WARN(1, KERN_WARNING "sysfs: ns %s in '%s' for '%s'\n",
                        sysfs_ns_type(parent_sd)? "required": "invalid",
                        parent_sd->s_name, name);
                return NULL;
        }

        hash = sysfs_name_hash(ns, (const char *)name);
        while (node) {
                struct sysfs_dirent *sd;
                 int result;

                sd = to_sysfs_dirent(node);
                result = sysfs_name_compare(hash, ns, (const char *)name, sd);
                if (result < 0)
                         node = node->rb_left;
                else if (result > 0)
                         node = node->rb_right;
                else
                        return sd;
        }
        return NULL;
}

/**
 *      sysfs_remove_one - remove sysfs_dirent from parent
 *      @acxt: addrm context to use
 *      @sd: sysfs_dirent to be removed
 *
 *      Mark @sd removed and drop nlink of parent inode if @sd is a
 *      directory.  @sd is unlinked from the children list.
 *
 *      This function should be called between calls to
 *      sysfs_addrm_start() and sysfs_addrm_finish() and should be
 *      passed the same @acxt as passed to sysfs_addrm_start().
 *
 *      LOCKING:
 *      Determined by sysfs_addrm_start().
 */
void sysfs_remove_one(struct sysfs_addrm_cxt *acxt, struct sysfs_dirent *sd)
{
        struct sysfs_inode_attrs *ps_iattr;

        BUG_ON(sd->s_flags & SYSFS_FLAG_REMOVED);

//        sysfs_unlink_sibling(sd);
        //TODO: remove file

        /* Update timestamps on the parent */
        if (acxt->parent_sd) {
            ps_iattr = acxt->parent_sd->s_iattr;
            if (ps_iattr) {
                   struct iattr *ps_iattrs = &ps_iattr->ia_iattr;
                   struct timespec time;
                   clock_gettime(CLOCK_REALTIME, &time);
                   ps_iattrs->ia_ctime = ps_iattrs->ia_mtime = time;
            }
        }

        sd->s_flags |= SYSFS_FLAG_REMOVED;
        sd->u.removed_list = acxt->removed;
        acxt->removed = sd;
}

int sysfs_hash_and_remove(struct sysfs_dirent *dir_sd, const void *ns, const char *name)
{
        struct sysfs_addrm_cxt acxt;
        struct sysfs_dirent *sd;

        if (!dir_sd) {
                WARN(1, KERN_WARNING "sysfs: can not remove '%s', no directory\n",
                        name);
               return -ENOENT;
        }

        sysfs_addrm_start(&acxt, dir_sd);

        sd = sysfs_find_dirent(dir_sd, ns, (const unsigned char *)name);
        if (sd)
                sysfs_remove_one(&acxt, sd);

        sysfs_addrm_finish(&acxt);

        if (sd)
                return 0;
        else
                return -ENOENT;
}

/**
 *      sysfs_get_dirent - find and get sysfs_dirent with the given name
 *      @parent_sd: sysfs_dirent to search under
 *      @name: name to look for
 *
 *      Look for sysfs_dirent with name @name under @parent_sd and get
 *      it if found.
 *
 *      LOCKING:
 *      Kernel thread context (may sleep).  Grabs sysfs_mutex.
 *
 *      RETURNS:
 *      Pointer to sysfs_dirent if found, NULL if not.
 */
struct sysfs_dirent *sysfs_get_dirent(struct sysfs_dirent *parent_sd,
                                      const void *ns,
                                      const unsigned char *name)
{
        struct sysfs_dirent *sd;

        mutex_lock((void*)&sysfs_mutex);
        sd = sysfs_find_dirent(parent_sd, ns, name);
//        sysfs_get(sd);
        mutex_unlock((void*)&sysfs_mutex);

        return sd;
}

static void remove_dir(struct sysfs_dirent *sd)
{
        struct sysfs_addrm_cxt acxt;

        sysfs_addrm_start(&acxt, sd->s_parent);
        sysfs_remove_one(&acxt, sd);
        sysfs_addrm_finish(&acxt);
}

static void __sysfs_remove_dir(struct sysfs_dirent *dir_sd)
{
        struct sysfs_addrm_cxt acxt;
        struct sysfs_dirent **pos;

        if (!dir_sd)
                return;

        DRM_DEBUG("sysfs %s: removing dir\n", dir_sd->s_name);
        sysfs_addrm_start(&acxt, dir_sd);
        pos = (void*)&dir_sd->s_dir.children;
        while (*pos) {
                struct sysfs_dirent *sd = *pos;

                if (sysfs_type(sd) != SYSFS_DIR)
                        sysfs_remove_one(&acxt, sd);
                else
                        pos = &(*pos)->s_sibling;
        }
        sysfs_addrm_finish(&acxt);

        remove_dir(dir_sd);
}

/**
 *      sysfs_remove_dir - remove an object's directory.
 *      @kobj:  object.
 *
 *      The only thing special about this is that we remove any files in
 *      the directory before we remove the directory, and we've inlined
 *      what used to be sysfs_rmdir() below, instead of calling separately.
 */

void sysfs_remove_dir(struct kobject * kobj)
{
        struct sysfs_dirent *sd = (void*)kobj->sd;

        kobj->sd = NULL;

        __sysfs_remove_dir(sd);
}

/**
 * sysfs_merge_group - merge files into a pre-existing attribute group.
 * @kobj:	The kobject containing the group.
 * @grp:	The files to create and the attribute group they belong to.
 *
 * This function returns an error if the group doesn't exist or any of the
 * files already exist in that group, in which case none of the new files
 * are created.
 */
int sysfs_merge_group(struct kobject *kobj, const struct attribute_group *grp)
{
	struct sysfs_dirent *sd = NULL; // FIXME: remove after search implementation
	struct drm_node_list *list;
	int error = 0;
	struct attribute *const *attr = NULL;
	struct bin_attribute *const *bin_attr;
	int i;

	list = kzalloc(sizeof(*list), GFP_KERNEL);
	if (!list) {
		return -EINVAL;
	}

	iofunc_attr_init(&list->device.attr, S_IFDIR | 0777, NULL, NULL);

	list->device.attr.inode = ++last_inode;
	list->name = grp->name;

	INIT_LIST_HEAD(&list->attrs_list.list);

	drm_device_ext_data_t *attr_priv = kzalloc (sizeof (drm_device_ext_data_t), GFP_KERNEL);

	if (attr_priv) {
		attr_priv->nodes = &list->attrs_list;
		attr_priv->iofunc_attr = &list->device.attr;
		attr_priv->dev = container_of(kobj, struct device, kobj);
		sd = kzalloc (sizeof(struct sysfs_dirent), GFP_KERNEL);
		if (sd){
			sd->s_name = grp->name;
			sd->s_mode = attr_priv->iofunc_attr->mode;
			sd->s_flags = SYSFS_KOBJ_ATTR;
		sd->list = list;
			attr_priv->sd = sd;
		}
	}

	list->device.device_ext_data = (void *)attr_priv;

	if (kobj->parent && kobj->parent->sd) {
		struct drm_node_list *group_list = ((struct sysfs_dirent *)(kobj->parent->sd))->list;
			list_add(&list->head, &group_list->attrs_list.list);
			group_list->attrs_list.count++;
	} else {
			list_add(&list->head, &minors_list.list);
			minors_list.count++;
	}

	kobj->sd = (void*)sd;

	if (grp->attrs) {
		for ((i = 0, attr = grp->attrs); *attr && !error; (++i, ++attr)) {
			error |= sysfs_add_file(sd, *attr, SYSFS_KOBJ_ATTR);
			error |= sysfs_create_qnxfs_file(kobj, *attr, SYSFS_KOBJ_ATTR, grp->name);
		}
		if (error) {
			while (--i >= 0) {
				sysfs_hash_and_remove(sd, NULL, (*--attr)->name);
				sysfs_create_qnxfs_file(kobj, *--attr, SYSFS_KOBJ_ATTR, grp->name);
			}
			return error;
		}
	}
	if (grp->bin_attrs) {
		for ((i = 0, bin_attr = grp->bin_attrs); *bin_attr && !error; (++i, ++bin_attr)) {
			error |= sysfs_create_qnxfs_file(kobj, &(*bin_attr)->attr, SYSFS_KOBJ_BIN_ATTR, grp->name);
		}
		if (error) {
			while (--i >= 0) {
				sysfs_remove_qnxfs_file(kobj, &(*--bin_attr)->attr, SYSFS_KOBJ_BIN_ATTR, grp->name);
			}
			return error;
		}
	}

	return error;
}

/**
 * sysfs_unmerge_group - remove files from a pre-existing attribute group.
 * @kobj:       The kobject containing the group.
 * @grp:        The files to remove and the attribute group they belong to.
 */
void sysfs_unmerge_group(struct kobject *kobj, const struct attribute_group *grp)
{
	struct sysfs_dirent *dir_sd = NULL;
	struct attribute *const *attr;
	struct bin_attribute *const *bin_attr;
	int i;

	dir_sd = sysfs_get_dirent((void*)kobj->sd, NULL, (const unsigned char *)grp->name);
	if (dir_sd) {
	for (attr = grp->attrs; *attr; ++attr)
		sysfs_hash_and_remove(dir_sd, NULL, (*attr)->name);
	}

	if (grp->attrs) {
		for ((i = 0, attr = grp->attrs); *attr; (++i, ++attr)) {
			sysfs_remove_qnxfs_file(kobj, *attr, SYSFS_KOBJ_ATTR, grp->name);
		}
	}
	if (grp->bin_attrs) {
		for ((i = 0, bin_attr = grp->bin_attrs); *bin_attr; (++i, ++bin_attr)) {
			sysfs_remove_qnxfs_file(kobj, &(*bin_attr)->attr, SYSFS_KOBJ_BIN_ATTR, grp->name);
		}
	}
}

/**
 * device_create_file - create sysfs attribute file for device.
 * @dev: device.
 * @attr: device attribute descriptor.
 */
int device_create_file(struct device *dev,
                       const struct device_attribute *attr)
{
	int error = 0;

	if (dev) {
		error = sysfs_create_file(&dev->kobj, &attr->attr);
	}

	return error;
}

/**
 * device_remove_file - remove sysfs attribute file.
 * @dev: device.
 * @attr: device attribute descriptor.
 */
void device_remove_file(struct device *dev,
                        const struct device_attribute *attr)
{
	if (dev) {
		sysfs_remove_file(&dev->kobj, &attr->attr);
	}
}


/**
 * device_create_bin_file - create sysfs binary attribute file for device.
 * @dev: device.
 * @attr: device binary attribute descriptor.
 */
int device_create_bin_file(struct device *dev, const struct bin_attribute *attr)
{
	int error = -EINVAL;

	if (dev) {
		error = sysfs_create_bin_file(&dev->kobj, attr);
	}

	return error;
}

/**
 * device_remove_bin_file - remove sysfs binary attribute file
 * @dev: device.
 * @attr: device binary attribute descriptor.
 */
void device_remove_bin_file(struct device *dev, const struct bin_attribute *attr)
{
	if (dev) {
		sysfs_remove_bin_file(&dev->kobj, attr);
	}
}

int sysfs_create_dir_ns(struct kobject *kobj, const void *ns)
{
	return EOK;
}

/**
 * sysfs_create_groups - given a directory kobject, create a bunch of attribute groups
 * @kobj:	The kobject to create the group on
 * @groups:	The attribute groups to create, NULL terminated
 *
 * This function creates a bunch of attribute groups.  If an error occurs when
 * creating a group, all previously created groups will be removed, unwinding
 * everything back to the original state when this function was called.
 * It will explicitly warn and error if any of the attribute files being
 * created already exist.
 *
 * Returns 0 on success or error code from sysfs_create_group on failure.
 */
int sysfs_create_groups(struct kobject *kobj, const struct attribute_group** groups)
{
	int error = 0;
	int i;

	if (!groups)
		return 0;

	for (i = 0; groups[i]; i++) {
		error = sysfs_create_group(kobj, groups[i]);
		if (error) {
			while (--i >= 0)
				sysfs_remove_group(kobj, groups[i]);
			break;
		}
	}
	return error;
}

void sysfs_remove_groups(struct kobject *kobj, const struct attribute_group** groups)
{
	int i;

	if (!groups) {
		return;
	}

	for (i = 0; groups[i]; i++) {
		sysfs_remove_group(kobj, groups[i]);
	}

	return;
}

int sysfs_move_dir_ns(struct kobject *kobj, struct kobject *new_parent_kobj, const void *new_ns)
{
	return EOK;
}

int sysfs_rename_dir_ns(struct kobject *kobj, const char *new_name, const void *new_ns)
{
	return EOK;
}

int sysfs_create_group(struct kobject *kobj, const struct attribute_group *grp)
{
	struct attribute *const *attr;
	struct bin_attribute *const *bin_attr;
	int error = 0;
	int i;

	if (grp->attrs) {
		for ((i = 0, attr = grp->attrs); *attr && !error; (++i, ++attr)) {
			sysfs_create_qnxfs_file(kobj, *attr, SYSFS_KOBJ_ATTR, grp->name);
		}
	}
	if (grp->bin_attrs) {
		for ((i = 0, bin_attr = grp->bin_attrs); *bin_attr && !error; (++i, ++bin_attr)) {
			sysfs_create_qnxfs_file(kobj, &(*bin_attr)->attr, SYSFS_KOBJ_BIN_ATTR, grp->name);
		}
	}

	return EOK;
}

void sysfs_remove_group(struct kobject *kobj, const struct attribute_group *grp)
{
	struct attribute *const *attr;
	struct bin_attribute *const *bin_attr;
	int i;

	if (grp->attrs) {
		for ((i = 0, attr = grp->attrs); *attr; (++i, ++attr)) {
			sysfs_remove_qnxfs_file(kobj, *attr, SYSFS_KOBJ_ATTR, grp->name);
		}
	}
	if (grp->bin_attrs) {
		for ((i = 0, bin_attr = grp->bin_attrs); *bin_attr; (++i, ++bin_attr)) {
			sysfs_remove_qnxfs_file(kobj, &(*bin_attr)->attr, SYSFS_KOBJ_BIN_ATTR, grp->name);
		}
	}

	return;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/sysfs.c $ $Rev: 858776 $")
#endif
