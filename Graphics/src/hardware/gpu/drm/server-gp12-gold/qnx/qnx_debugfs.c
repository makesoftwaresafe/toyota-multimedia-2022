#if defined(CONFIG_DEBUG_FS)

#undef IOFUNC_ATTR_T
#undef IOFUNC_OCB_T
struct _debugfs_device;
struct _debugfs_ocb;
#define IOFUNC_ATTR_T   struct _debugfs_device
#define IOFUNC_OCB_T    struct _debugfs_ocb

#include <stdint.h>
#include <pthread.h>
#include <sys/usbdi.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/rsrcdbmgr.h>
#include <sys/resmgr.h>

#include <linux/dcache.h>
#include <linux/debugfs.h>
#include <drm/drmP.h>

/* Small buffer will cause slowdown on directory read operations, large buffer wastes memory, */
/* typical buffer size 64Kb-256Kb is a good ratio between slowdown and memory consumption.    */
#define DEBUGFS_MIN_BUFFER_SIZE (128*1024)

extern int debugfs_enable;

extern dispatch_t* dpp;

struct _debugfs_device;

typedef struct _debugfs_ocb {
    iofunc_ocb_t            hdr;
    struct _debugfs_device* debugfs;
    int                     filesize;
    int                     datasize;
    char*                   filedata;
} debugfs_ocb_t;

typedef struct _debugfs_device {
    iofunc_attr_t          hdr;
    int                    resmgr_id;
    resmgr_io_funcs_t      io_funcs;
    resmgr_connect_funcs_t connect_funcs;
    iofunc_funcs_t         ocb_funcs;
    iofunc_mount_t         io_mount;
    resmgr_attr_t          rattr;
    struct dentry*         dentry;
    char                   name[PATH_MAX];
} debugfs_device_t;

int claim_task_structure(resmgr_context_t *ctp, int inopen);
int unclaim_task_structure(resmgr_context_t *ctp, int inclose);

debugfs_ocb_t* _debugfs_ocb_calloc(resmgr_context_t* ctp, debugfs_device_t* debugfs_dev)
{
    debugfs_ocb_t* ocb;
    struct file dfile;
    int err;
    ssize_t readsize;
    loff_t readpos;

    ocb = calloc(1, sizeof(debugfs_ocb_t));
    if (ocb == NULL) {
        return NULL;
    }
    ocb->debugfs = debugfs_dev;

    ocb->filedata = calloc(1, DEBUGFS_MIN_BUFFER_SIZE);
    if (ocb->filedata != NULL) {
        ocb->datasize = DEBUGFS_MIN_BUFFER_SIZE;
    } else {
        free(ocb);
        return NULL;
    }

    claim_task_structure(ctp, 0);

    /* Since we handle debugfs directly in RM we can use memcpy() instead copy_to_user() */
    current->attachment.copy_to_user_memcpy = 1;

    memset(&dfile, 0x00, sizeof(dfile));
    dfile.f_inode = ocb->debugfs->dentry->d_inode;
    err = ocb->debugfs->dentry->d_inode->i_fop->open(ocb->debugfs->dentry->d_inode, &dfile);
    if (err != 0) {
        unclaim_task_structure(ctp, 0);
        /* Check if it is a real error or file is not available on this platform */
        if ((err != -ENODEV) && (err != -EIO)) {
            DRM_ERROR("Internal open error %d for debugfs device object: %s\n", -err, ocb->debugfs->name);
        }
        free(ocb->filedata);
        free(ocb);
        return NULL;
    }
    ocb->filesize = 0;

    if (ocb->debugfs->dentry->d_inode->i_fop->read) {
        do {
            readpos = ocb->filesize;
            readsize = ocb->debugfs->dentry->d_inode->i_fop->read(&dfile, ocb->filedata + ocb->filesize, DEBUGFS_MIN_BUFFER_SIZE, &readpos);
            if (readsize < 0) {
                if (readsize == -ENODEV) {
                    snprintf(ocb->filedata, DEBUGFS_MIN_BUFFER_SIZE, "Information is not available for this graphics chipset\n");
                    ocb->filesize = strlen(ocb->filedata);
                }
                break;
            } else {
                ocb->filesize += readsize;
                if (readsize == DEBUGFS_MIN_BUFFER_SIZE) {
                    ocb->filedata = realloc(ocb->filedata, ocb->filesize + DEBUGFS_MIN_BUFFER_SIZE);
                } else {
                    break;
                }
            }
        } while(1);
    }

    if (ocb->debugfs->dentry->d_inode->i_fop->release) {
        /* in case of usage simple_open() relase could be zero */
        err = ocb->debugfs->dentry->d_inode->i_fop->release(ocb->debugfs->dentry->d_inode, &dfile);
        if (err != 0) {
            DRM_ERROR("Internal release failed for debugfs device object: %s\n", ocb->debugfs->name);
            free(ocb->filedata);
            free(ocb);
            unclaim_task_structure(ctp, 0);
            return NULL;
        }
    }

    /* Update file size in filesystem representation */
    debugfs_dev->hdr.nbytes = ocb->filesize;

    unclaim_task_structure(ctp, 0);

    return ocb;
}

void _debugfs_ocb_free(debugfs_ocb_t* ocb)
{
    free(ocb->filedata);
    free(ocb);
}

int resmgr_open_debugfs(resmgr_context_t* ctp, io_open_t* msg, RESMGR_HANDLE_T* handle, void* extra)
{
    int status;

    status=iofunc_open_default(ctp, msg, (iofunc_attr_t*)handle, extra);
    if (status) {
        return status;
    }

    return EOK;
}

int resmgr_close_debugfs(resmgr_context_t* ctp, void* reserved, debugfs_ocb_t* ocb)
{
    int status;

    status=iofunc_close_ocb_default(ctp, reserved, (iofunc_ocb_t*)ocb);
    if (status) {
        return status;
    }

    return EOK;
}

int resmgr_read_debugfs(resmgr_context_t* ctp, io_read_t* msg, debugfs_ocb_t* ocb)
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

int resmgr_write_debugfs(resmgr_context_t* ctp, io_write_t* msg, debugfs_ocb_t* ocb)
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
        ssize_t writesize;
        loff_t writepos;

        if (!ocb->debugfs->dentry->d_inode->i_fop->write) {
            _IO_SET_WRITE_NBYTES(ctp, -1);
            return _RESMGR_ERRNO(ENOTSUP);
        }
        data = sizeof(msg->i) + (char *)&msg->i;

        memset(&dfile, 0x00, sizeof(dfile));
        dfile.f_inode = ocb->debugfs->dentry->d_inode;

        claim_task_structure(ctp, 0);

        /* Since we handle debugfs directly in RM we can use memcpy() instead copy_to_user() */
        current->attachment.copy_to_user_memcpy = 1;

        err = ocb->debugfs->dentry->d_inode->i_fop->open(ocb->debugfs->dentry->d_inode, &dfile);
        if (err != 0) {
            unclaim_task_structure(ctp, 0);
            /* Check if it is a real error or file is not available on this platform */
            if ((err != -ENODEV) && (err != -EIO)) {
                DRM_ERROR("Internal open error %d for debugfs device object: %s\n", -err, ocb->debugfs->name);
            }
            _IO_SET_WRITE_NBYTES(ctp, -1);
            return _RESMGR_ERRNO(-err);
        }

        writepos = ocb->filesize;
        writesize = ocb->debugfs->dentry->d_inode->i_fop->write(&dfile, data, msg->i.nbytes, &writepos);
        if (writesize < 0) {
            unclaim_task_structure(ctp, 0);
            _IO_SET_WRITE_NBYTES(ctp, -1);
            return _RESMGR_ERRNO(-writesize);
        }
        if (ocb->debugfs->dentry->d_inode->i_fop->release) {
            /* in case of usage simple_open() relase could be zero */
            ocb->debugfs->dentry->d_inode->i_fop->release(ocb->debugfs->dentry->d_inode, &dfile);
        }

        unclaim_task_structure(ctp, 0);
        _IO_SET_WRITE_NBYTES(ctp, writesize);
    }

    /* Update file access time */
    ocb->hdr.attr->hdr.flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;

    return _RESMGR_ERRNO(EOK);
}

int resmgr_register_debugfs(struct dentry* dentry, const char* name)
{
    struct _debugfs_device* debugfs;

    debugfs = calloc(1, sizeof(*debugfs));
    if (debugfs == NULL) {
        return -1;
    }

    debugfs->dentry = dentry;
    strncpy(debugfs->name, name, sizeof(debugfs->name));

    memset(&debugfs->ocb_funcs, 0x00, sizeof(debugfs->ocb_funcs));
    debugfs->ocb_funcs.nfuncs = _IOFUNC_NFUNCS;
    debugfs->ocb_funcs.ocb_calloc = _debugfs_ocb_calloc;
    debugfs->ocb_funcs.ocb_free = _debugfs_ocb_free;
    memset(&debugfs->io_mount, 0x00, sizeof(debugfs->io_mount));
    debugfs->io_mount.funcs = &debugfs->ocb_funcs;

    memset(&debugfs->rattr, 0, sizeof(debugfs->rattr));
    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &debugfs->connect_funcs,
                     _RESMGR_IO_NFUNCS, &debugfs->io_funcs);
    iofunc_attr_init(&debugfs->hdr, _S_IFREG | S_IRUSR | S_IRGRP | S_IROTH, NULL, NULL);

    debugfs->connect_funcs.open = resmgr_open_debugfs;
    debugfs->io_funcs.read = resmgr_read_debugfs;
    debugfs->io_funcs.write = resmgr_write_debugfs;
    debugfs->io_funcs.close_ocb = resmgr_close_debugfs;

    debugfs->hdr.mount = &debugfs->io_mount;

    debugfs->resmgr_id = resmgr_attach(dpp,
        &debugfs->rattr, name, _FTYPE_ANY, 0, &debugfs->connect_funcs,
        &debugfs->io_funcs, (RESMGR_HANDLE_T*)&debugfs->hdr);
    if (debugfs->resmgr_id == -1) {
        free(debugfs);
        DRM_ERROR("resmgr_attach() has been failed for DEBUGFS device: %s", name);
        return -1;
    }

    resmgr_devino(debugfs->resmgr_id, &debugfs->io_mount.dev, &debugfs->hdr.inode);

    return 0;
}

void debugfs_remove(struct dentry *dentry)
{
    if (dentry->d_inode != NULL) {
        free(dentry->d_inode);
        dentry->d_inode = NULL;
    }
    if (dentry != NULL) {
        free(dentry);
    }
}

struct dentry *debugfs_create_file(const char *name, umode_t mode,
   struct dentry *parent, void *data, const struct file_operations *fops)
{
    struct dentry* file = NULL;
    struct dentry* i = parent;
    char tempstr[PATH_MAX];
    char tempstr2[PATH_MAX];

    file = calloc(1, sizeof(*file));
    if (file == NULL) {
        return file;
    }

    file->d_name.name = calloc(1, strlen(name) + 1);
    strcpy((void*)file->d_name.name, (void*)name);
    strncpy((char*)file->d_iname, name, sizeof(file->d_iname));
    file->d_parent = parent;
    file->d_inode = calloc(1, sizeof(*file->d_inode));
    file->d_inode->i_private = data;
    file->d_inode->i_fop = fops;

    if (!debugfs_enable) {
        /* Just pretend we have registered debugfs entry */
        return file;
    }

    strncpy(tempstr, name, sizeof(tempstr));
    do {
        if (i) {
            strncpy(tempstr2, (char*)i->d_iname, sizeof(tempstr2));
            strcat(tempstr2, "/");
            strcat(tempstr2, tempstr);
            strncpy(tempstr, tempstr2, sizeof(tempstr));
            i = i->d_parent;
        }
    } while(i);

    strncpy(tempstr2, "/sys/kernel/debug/", sizeof(tempstr2));
    strcat(tempstr2, tempstr);
    strncpy(tempstr, tempstr2, sizeof(tempstr));

    DRM_DEBUG_DRIVER("Registering debugfs filename: %s\n", tempstr);
    resmgr_register_debugfs(file, tempstr);

    return file;
}

struct dentry *debugfs_create_dir(const char *name, struct dentry *parent)
{
    struct dentry* dir = NULL;

    dir = calloc(1, sizeof(*dir));
    if (dir == NULL) {
        return dir;
    }

    dir->d_name.name = (void*)name;
    strncpy((char*)dir->d_iname, name, sizeof(dir->d_iname));
    dir->d_parent = parent;

    return dir;
}

void debugfs_remove_recursive(struct dentry *dentry)
{
}

#endif /* CONFIG_DEBUG_FS */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/qnx_debugfs.c $ $Rev: 856018 $")
#endif
