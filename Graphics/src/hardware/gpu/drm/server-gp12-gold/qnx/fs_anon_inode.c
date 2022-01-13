#undef IOFUNC_ATTR_T
#undef IOFUNC_OCB_T
struct _anon_inode_device;
struct _anon_inode_ocb;
#define IOFUNC_ATTR_T   struct _anon_inode_device
#define IOFUNC_OCB_T    struct _anon_inode_ocb

#include <stdint.h>
#include <devctl.h>
#include <pthread.h>
#include <sys/usbdi.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/rsrcdbmgr.h>
#include <sys/resmgr.h>

#include <linux/fs.h>
#include <linux/file.h>
#include <linux/idr.h>
#include <drm/drmP.h>

#undef LIST_HEAD

#include <sys/queue.h>
#include <pthread.h>

static SLIST_HEAD(fdfile_list, fdfile_entry) fdfile_list_head = SLIST_HEAD_INITIALIZER(fdfile_list_head);

struct fdfile_entry {
	SLIST_ENTRY(fdfile_entry) entries;
	struct file* filep;
	int fd;
};

static pthread_mutex_t fdfile_list_mutex = PTHREAD_MUTEX_INITIALIZER;

#define QNX_SPECIAL_HANDLE_START (0x514E5848)
#define QNX_SPECIAL_HANDLE_END   (0x514E5848 + 1024)
const char* qnx_special_name = "[i915_perf]";

/* Start inode buffer size for read/write operations, could grow in runtime */
#define QNX_ANON_INODE_BUFFER_SIZE 32768

static DEFINE_IDR(fd_idr);

extern dispatch_t* dpp;

struct _anon_inode_device;

typedef struct _anon_inode_ocb {
	iofunc_ocb_t               hdr;
	struct _anon_inode_device* anon_inode;
	void*                      buffer;
	int                        buffer_size;
} anon_inode_ocb_t;

typedef struct _anon_inode_device {
	iofunc_attr_t          hdr;
	int                    resmgr_id;
	int                    refcount;
	int                    fd;
	pthread_mutex_t        mutex;
	resmgr_io_funcs_t      io_funcs;
	resmgr_connect_funcs_t connect_funcs;
	iofunc_funcs_t         ocb_funcs;
	iofunc_mount_t         io_mount;
	resmgr_attr_t          rattr;
	struct file*           filep;
	char                   name[PATH_MAX];
	iofunc_notify_t        notify[3];
} anon_inode_device_t;

int claim_task_structure(resmgr_context_t *ctp, int inopen);
int unclaim_task_structure(resmgr_context_t *ctp, int inclose);

/*
 * Install a file pointer in the fd array.
 */
void fd_install(unsigned int fd, struct file *file)
{
	struct fdfile_entry* entry;

	if ((fd < QNX_SPECIAL_HANDLE_START) || (fd > QNX_SPECIAL_HANDLE_END)) {
		qnx_error("fd_install() is not implemented for such type of handles!\n");
		BUG();
		return;
	}

	entry = calloc(1, sizeof(*entry));
	if (!entry) {
		/* This function can't fail according to linux API */
		qnx_error("can't allocate a structure to hold struct file and fd\n");
		BUG();
		return;
	}

	entry->filep = file;
	entry->fd = fd;

	pthread_mutex_lock(&fdfile_list_mutex);
	SLIST_INSERT_HEAD(&fdfile_list_head, entry, entries);
	pthread_mutex_unlock(&fdfile_list_mutex);
}

void put_unused_fd(unsigned int fd)
{
	struct fdfile_entry* entry;

	/* We can "destroy" only our special handle which means void handle */
	if ((fd < QNX_SPECIAL_HANDLE_START) || (fd > QNX_SPECIAL_HANDLE_END)) {
		qnx_error("put_unused_fd() is not implemented for such type of handles!\n");
		BUG();
		return;
	}

	pthread_mutex_lock(&fdfile_list_mutex);
	idr_remove(&fd_idr, fd);
	SLIST_FOREACH(entry, &fdfile_list_head, entries) {
		if (entry->fd == fd) {
			SLIST_REMOVE(&fdfile_list_head, entry, fdfile_entry, entries);
			free(entry);
			break;
		}
	}
	pthread_mutex_unlock(&fdfile_list_mutex);
}

void put_unused_file(struct file* filep)
{
	struct fdfile_entry* entry;

	pthread_mutex_lock(&fdfile_list_mutex);
	SLIST_FOREACH(entry, &fdfile_list_head, entries) {
		if (entry->filep == filep) {
			idr_remove(&fd_idr, entry->fd);
			SLIST_REMOVE(&fdfile_list_head, entry, fdfile_entry, entries);
			free(entry);
			break;
		}
	}
	pthread_mutex_unlock(&fdfile_list_mutex);
}

int get_unused_fd_flags(unsigned flags)
{
	int fd;

	pthread_mutex_lock(&fdfile_list_mutex);
	fd = idr_alloc(&fd_idr, NULL, QNX_SPECIAL_HANDLE_START, QNX_SPECIAL_HANDLE_END, GFP_KERNEL);
	pthread_mutex_unlock(&fdfile_list_mutex);

	/* Do not check if fd is correct here, just return an "FD", upper level should care about errors */
	return fd;
}

static struct file *__fget(unsigned int fd, fmode_t mask)
{
	struct fdfile_entry* entry;

	/* We can "destroy" only our special handles */
	if ((fd < QNX_SPECIAL_HANDLE_START) || (fd > QNX_SPECIAL_HANDLE_END)) {
		qnx_error("__fget() is not implemented for such type of handles!\n");
		BUG();
		return NULL;
	}

	pthread_mutex_lock(&fdfile_list_mutex);
	SLIST_FOREACH(entry, &fdfile_list_head, entries) {
		if (entry->fd == fd) {
			struct file* filep = entry->filep;
			pthread_mutex_unlock(&fdfile_list_mutex);
			return filep;
		}
	}
	pthread_mutex_unlock(&fdfile_list_mutex);

	return NULL;
}

struct file *fget(unsigned int fd)
{
	return __fget(fd, FMODE_PATH);
}

struct file *fget_raw(unsigned int fd)
{
	return __fget(fd, 0);
}

anon_inode_ocb_t* _anon_inode_ocb_calloc(resmgr_context_t* ctp, anon_inode_device_t* anon_inode_dev)
{
	anon_inode_ocb_t* ocb;
	struct file* filep;
	int err;
	ssize_t readsize;
	loff_t readpos;

	ocb = calloc(1, sizeof(anon_inode_ocb_t));
	if (ocb == NULL) {
		return NULL;
	}
	ocb->buffer = malloc(QNX_ANON_INODE_BUFFER_SIZE);
	if (ocb->buffer == NULL) {
		free(ocb);
		return NULL;
	}
	ocb->buffer_size = QNX_ANON_INODE_BUFFER_SIZE;

	ocb->anon_inode = anon_inode_dev;

	/* Update file size in filesystem representation, always zero, since it is a special file */
	anon_inode_dev->hdr.nbytes = 0;

	/* Initialize notify queues */
	IOFUNC_NOTIFY_INIT(ocb->anon_inode->notify);

	pthread_mutex_lock(&anon_inode_dev->mutex);
	anon_inode_dev->refcount++;
	pthread_mutex_unlock(&anon_inode_dev->mutex);

	return ocb;
}

void _anon_inode_ocb_free(anon_inode_ocb_t* ocb)
{
	pthread_mutex_lock(&ocb->anon_inode->mutex);
	ocb->anon_inode->refcount--;

	/* Here are some differences comparing to linux kernel:         */
	/* Linux: it gets an FD, on last FD close it does auto-destroy  */
	/* QNX: we have a "virtual file", we have to open it manually   */
	/*      and then on last FD close it does auto-destroy as well. */
	if (ocb->anon_inode->refcount == 0) {
		/* Here we have to destroy RM on client's close() call */
		resmgr_detach(dpp, ocb->anon_inode->resmgr_id, _RESMGR_DETACH_ALL);
		ocb->anon_inode->resmgr_id = -1;
		put_unused_fd(ocb->anon_inode->fd);
		pthread_mutex_destroy(&ocb->anon_inode->mutex);
		free(ocb->anon_inode);
	} else {
		pthread_mutex_unlock(&ocb->anon_inode->mutex);
	}

	free(ocb);
}

int resmgr_open_anon_inode(resmgr_context_t* ctp, io_open_t* msg, RESMGR_HANDLE_T* handle, void* extra)
{
	int status;

	status=iofunc_open_default(ctp, msg, (iofunc_attr_t*)handle, extra);
	if (status) {
		return status;
	}

	return EOK;
}

int resmgr_close_anon_inode(resmgr_context_t* ctp, void* reserved, anon_inode_ocb_t* ocb)
{
	int status;

	/* Remove notify queues at exit and unblock all waiting clients */
	iofunc_notify_trigger_strict(ctp, ocb->anon_inode->notify, INT_MAX, IOFUNC_NOTIFY_INPUT);
	iofunc_notify_trigger_strict(ctp, ocb->anon_inode->notify, INT_MAX, IOFUNC_NOTIFY_OUTPUT);
	iofunc_notify_trigger_strict(ctp, ocb->anon_inode->notify, INT_MAX, IOFUNC_NOTIFY_OBAND);
	iofunc_notify_remove(ctp, ocb->anon_inode->notify);

	if (ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->release) {
		claim_task_structure(ctp, 0);
		/* Since we handle anonymous inodes directly in RM we can use memcpy() instead copy_to_user() */
		current->attachment.copy_to_user_memcpy = 1;

		ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->release(NULL, ocb->anon_inode->filep);

		unclaim_task_structure(ctp, 0);
	}

	/* This function must be last called function, because it destroys OCB */
	status=iofunc_close_ocb_default(ctp, reserved, (iofunc_ocb_t*)ocb);
	if (status) {
		return status;
	}

	return EOK;
}

int resmgr_read_anon_inode(resmgr_context_t* ctp, io_read_t* msg, anon_inode_ocb_t* ocb)
{
	int result;
	int nparts = 0;
	int nonblock = 0;
	ssize_t nbytes;

	result = iofunc_read_verify(ctp, msg, (iofunc_ocb_t*)ocb, &nonblock);
	if (result != EOK) {
		_IO_SET_READ_NBYTES(ctp, -1);
		return result;
	}
	if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE) {
		_IO_SET_READ_NBYTES(ctp, -1);
		return ENOSYS;
	}

	if (nonblock) {
		ocb->anon_inode->filep->f_flags |= O_NONBLOCK;
	}

	if (msg->i.nbytes) {
		if ((msg->i.nbytes > ocb->buffer_size) || (ocb->buffer == NULL)) {
			free(ocb->buffer);
			ocb->buffer = malloc(msg->i.nbytes);
			if (ocb->buffer == NULL) {
				ocb->buffer_size = 0;
				_IO_SET_READ_NBYTES(ctp, -1);
				return ENOSPC;
			}
		}

		if (ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->read) {

			claim_task_structure(ctp, 0);
			/* Since we handle anonymous inodes directly in RM we can use memcpy() instead copy_to_user() */
			current->attachment.copy_to_user_memcpy = 1;

			nbytes = ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->read(ocb->anon_inode->filep, ocb->buffer, msg->i.nbytes, NULL);

			unclaim_task_structure(ctp, 0);

			if (nbytes >= 0) {
				SETIOV(ctp->iov, ocb->buffer, nbytes);
				_IO_SET_READ_NBYTES(ctp, nbytes);
				ocb->hdr.offset = 0;
				nparts = 1;
			} else {
				_IO_SET_WRITE_NBYTES(ctp, -1);
				/* nbytes has negative errno after linux's fops call */
				return _RESMGR_ERRNO(-nbytes);
			}
		} else {
			_IO_SET_WRITE_NBYTES(ctp, -1);
			return _RESMGR_ERRNO(ENOTSUP);
		}
	} else {
		_IO_SET_READ_NBYTES(ctp, 0);
	}

	/* Update file access time */
	ocb->hdr.attr->hdr.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;

	return (_RESMGR_NPARTS(nparts));
}

int resmgr_write_anon_inode(resmgr_context_t* ctp, io_write_t* msg, anon_inode_ocb_t* ocb)
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

	_IO_SET_WRITE_NBYTES(ctp, -1);

	return _RESMGR_ERRNO(ENOTSUP);
}

int resmgr_devctl_anon_inode(resmgr_context_t* ctp, io_devctl_t* msg, anon_inode_ocb_t* ocb)
{
	int status;
	long ret = -ENOTSUP;

	if ((status = iofunc_devctl_default(ctp, msg, &ocb->hdr)) !=  _RESMGR_DEFAULT)
	{
		return status;
	}

	if (ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->unlocked_ioctl) {
		claim_task_structure(ctp, 0);
		/* Since we handle anonymous inodes directly in RM we can use memcpy() instead copy_to_user() even in ioctl() */
		current->attachment.copy_to_user_memcpy = 1;

		iofunc_attr_unlock(&ocb->hdr.attr->hdr);
		ret = ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->unlocked_ioctl(ocb->anon_inode->filep, msg->i.dcmd, (uintptr_t)_DEVCTL_DATA(msg->i));
		iofunc_attr_lock(&ocb->hdr.attr->hdr);

		unclaim_task_structure(ctp, 0);
	}

	msg->o.zero = 0;
	msg->o.zero2 = 0;
	msg->o.nbytes = 0;
	msg->o.ret_val = ret;
	_RESMGR_STATUS(ctp, -ret);

	return (_RESMGR_PTR(ctp, &msg->o, sizeof(msg->o)));
}

int resmgr_notify_anon_inode(resmgr_context_t* ctp, io_notify_t* msg, anon_inode_ocb_t* ocb)
{
	int trigger = 0;
	unsigned int mask = 0;

	if (ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->poll) {
		claim_task_structure(ctp, 0);
		/* Since we handle anonymous inodes directly in RM we can use memcpy() instead copy_to_user() */
		current->attachment.copy_to_user_memcpy = 1;

		mask = ocb->anon_inode->filep->f_path.dentry->d_inode->i_fop->poll(ocb->anon_inode->filep, NULL);
		if (mask & POLLIN) {
			trigger |= _NOTIFY_COND_INPUT | _NOTIFY_COND_OBAND;
		}

		unclaim_task_structure(ctp, 0);
	}

	return iofunc_notify(ctp, msg, ocb->anon_inode->notify, trigger, NULL, NULL);
}

int resmgr_register_anon_inode(struct file* file, const char* name, int fd)
{
	struct _anon_inode_device* anon_inode;
	int status;

	anon_inode = calloc(1, sizeof(*anon_inode));
	if (anon_inode == NULL) {
		return -1;
	}

	anon_inode->filep = file;
	strncpy(anon_inode->name, name, sizeof(anon_inode->name));

	memset(&anon_inode->ocb_funcs, 0x00, sizeof(anon_inode->ocb_funcs));
	anon_inode->ocb_funcs.nfuncs = _IOFUNC_NFUNCS;
	anon_inode->ocb_funcs.ocb_calloc = _anon_inode_ocb_calloc;
	anon_inode->ocb_funcs.ocb_free = _anon_inode_ocb_free;
	memset(&anon_inode->io_mount, 0x00, sizeof(anon_inode->io_mount));
	anon_inode->io_mount.funcs = &anon_inode->ocb_funcs;

	memset(&anon_inode->rattr, 0, sizeof(anon_inode->rattr));
	iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &anon_inode->connect_funcs, _RESMGR_IO_NFUNCS, &anon_inode->io_funcs);
	iofunc_attr_init(&anon_inode->hdr, _S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, NULL, NULL);

	anon_inode->connect_funcs.open = resmgr_open_anon_inode;
	anon_inode->io_funcs.read = resmgr_read_anon_inode;
	anon_inode->io_funcs.write = resmgr_write_anon_inode;
	anon_inode->io_funcs.devctl = resmgr_devctl_anon_inode;
	anon_inode->io_funcs.notify = resmgr_notify_anon_inode;
	anon_inode->io_funcs.close_ocb = resmgr_close_anon_inode;

	anon_inode->hdr.mount = &anon_inode->io_mount;

	anon_inode->resmgr_id = resmgr_attach(dpp, &anon_inode->rattr, name, _FTYPE_ANY, 0, &anon_inode->connect_funcs,
		&anon_inode->io_funcs, (RESMGR_HANDLE_T*)&anon_inode->hdr);
	if (anon_inode->resmgr_id == -1) {
		free(anon_inode);
		qnx_error("resmgr_attach() has been failed for anon_inode device\n");
		return -1;
	}

	anon_inode->refcount = 0;

	status = pthread_mutex_init(&anon_inode->mutex, NULL);
	if (status != EOK){
		resmgr_detach(dpp, anon_inode->resmgr_id, _RESMGR_DETACH_ALL);
		free(anon_inode);
		qnx_error("pthread_mutex_init() call failed\n");
		return -1;
	}

	anon_inode->fd = fd;

	resmgr_devino(anon_inode->resmgr_id, &anon_inode->io_mount.dev, &anon_inode->hdr.inode);

	return 0;
}

/**
 * anon_inode_getfile - creates a new file instance by hooking it up to an
 *                      anonymous inode, and a dentry that describe the "class"
 *                      of the file
 *
 * @name:    [in]    name of the "class" of the new file
 * @fops:    [in]    file operations for the new file
 * @priv:    [in]    private data for the new file (will be file's private_data)
 * @flags:   [in]    flags
 *
 * Creates a new file by hooking it on a single inode. This is useful for files
 * that do not need to have a full-fledged inode in order to operate correctly.
 * All the files created with anon_inode_getfile() will share a single inode,
 * hence saving memory and avoiding code duplication for the file/inode/dentry
 * setup.  Returns the newly created file* or an error pointer.
 */
struct file *anon_inode_getfile(const char *name,
				const struct file_operations *fops,
				void *priv, int flags)
{
	struct file* anonfile;
	char target_name[PATH_MAX];

	anonfile = calloc(1, sizeof(*anonfile));
	if (!anonfile) {
		return NULL;
	}

	anonfile->f_path.dentry = calloc(1, sizeof(*anonfile->f_path.dentry));
	if (!anonfile->f_path.dentry) {
		free(anonfile);
		return NULL;
	}

	anonfile->f_path.dentry->d_inode = calloc(1, sizeof(*anonfile->f_path.dentry->d_inode));
	if (!anonfile->f_path.dentry->d_inode) {
		free(anonfile->f_path.dentry);
		free(anonfile);
		return NULL;
	}

	anonfile->f_path.dentry->d_name.name = calloc(1, strlen(target_name) + 1);
	if (!anonfile->f_path.dentry->d_name.name) {
		free(anonfile->f_path.dentry->d_inode);
		free(anonfile->f_path.dentry);
		free(anonfile);
		return NULL;
	}

	strncpy((void*)anonfile->f_path.dentry->d_name.name, (void*)target_name, strlen(target_name));
	strncpy((char*)anonfile->f_path.dentry->d_iname, target_name, sizeof(anonfile->f_path.dentry->d_iname) - 1);

	anonfile->f_path.dentry->d_parent = NULL;
	anonfile->f_path.dentry->d_inode->i_private = priv;
	anonfile->private_data = priv;
	anonfile->f_path.dentry->d_inode->i_fop = fops;

	return anonfile;
}

/**
 * anon_inode_getfd - creates a new file instance by hooking it up to an
 *                    anonymous inode, and a dentry that describe the "class"
 *                    of the file
 *
 * @name:    [in]    name of the "class" of the new file
 * @fops:    [in]    file operations for the new file
 * @priv:    [in]    private data for the new file (will be file's private_data)
 * @flags:   [in]    flags
 *
 * Creates a new file by hooking it on a single inode. This is useful for files
 * that do not need to have a full-fledged inode in order to operate correctly.
 * All the files created with anon_inode_getfd() will share a single inode,
 * hence saving memory and avoiding code duplication for the file/inode/dentry
 * setup.  Returns new descriptor or an error code.
 */
int anon_inode_getfd(const char *name, const struct file_operations *fops,
		     void *priv, int flags)
{
	int error, fd;
	struct file *file;
	char fs_name[PATH_MAX];
	char fs_name_fd[PATH_MAX];

	if (strlen(name) >= strlen(qnx_special_name)) {
		/* Do a comparison for name only, do not use metrics ID located in the name */
		if (memcmp(name, qnx_special_name, strlen(qnx_special_name)) != 0) {
			qnx_error("Can't handle this anon inode name\n");
			return -1;
		}
	}

	snprintf(fs_name, sizeof(fs_name) - 1, "/sys/class/drm/card0/metrics/%s", name);

	error = get_unused_fd_flags(flags);
	if (error < 0) {
		return error;
        }
	fd = error;

	snprintf(fs_name_fd, sizeof(fs_name_fd) - 1, "-%d", fd);
	strncat(fs_name, fs_name_fd, sizeof(fs_name) - 1);

	file = anon_inode_getfile(fs_name, fops, priv, flags);
	if (IS_ERR(file)) {
		error = PTR_ERR(file);
		goto err_put_unused_fd;
	}
	fd_install(fd, file);

	/* Create a fs entry for fs_name and attach resmgr to it */
	DRM_DEBUG_DRIVER("Registering anon_inode filename: %s\n", fs_name);
	resmgr_register_anon_inode(file, fs_name, fd);

	return fd;

err_put_unused_fd:
	put_unused_fd(fd);

	return error;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/fs_anon_inode.c $ $Rev: 858776 $")
#endif
