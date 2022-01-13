/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/fs.h and /include/uapi/linux/fs.h; takes portion of these
* and made some QNX specific modifications (__QNXNTO__).
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _QNX_LINUX_FS_H
#define _QNX_LINUX_FS_H

#include <linux/types.h>
#include <linux/uidgid.h>
#include <linux/spinlock.h>
#include <linux/dcache.h>
#include <linux/user_namespace.h>
#include <linux/mutex.h>
#include <linux/lockdep.h>
#include <linux/list.h>
#include <linux/fcntl.h>
#include <linux/migrate_mode.h>
#include <linux/kdev_t.h>

/*
 * Use sequence counter to get consistent i_size on 32-bit processors.
 */
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
#include <linux/seqlock.h>
#define __NEED_I_SIZE_ORDERED
#define i_size_ordered_init(inode) seqcount_init(&inode->i_size_seqcount)
#else
#define i_size_ordered_init(inode) do { } while (0)
#endif

#ifndef __QNXNTO__
#define SEEK_SET	0	/* seek relative to beginning of file */
#define SEEK_CUR	1	/* seek relative to current file position */
#define SEEK_END	2	/* seek relative to end of file */
#endif

#define SEEK_DATA	3	/* seek to the next data */
#define SEEK_HOLE	4	/* seek to the next hole */
#define SEEK_MAX	SEEK_HOLE

/* file is open for reading */
#define FMODE_READ		((__force fmode_t)0x1)
/* file is open for writing */
#define FMODE_WRITE		((__force fmode_t)0x2)
/* file is seekable */
#define FMODE_LSEEK		((__force fmode_t)0x4)
/* file can be accessed using pread */
#define FMODE_PREAD		((__force fmode_t)0x8)
/* file can be accessed using pwrite */
#define FMODE_PWRITE		((__force fmode_t)0x10)
/* File is opened for execution with sys_execve / sys_uselib */
#define FMODE_EXEC		((__force fmode_t)0x20)
/* File is opened with O_NDELAY (only set for block devices) */
#define FMODE_NDELAY		((__force fmode_t)0x40)
/* File is opened with O_EXCL (only set for block devices) */
#define FMODE_EXCL		((__force fmode_t)0x80)
/* File is opened using open(.., 3, ..) and is writeable only for ioctls
   (specialy hack for floppy.c) */
#define FMODE_WRITE_IOCTL	((__force fmode_t)0x100)
/* 32bit hashes as llseek() offset (for directories) */
#define FMODE_32BITHASH         ((__force fmode_t)0x200)
/* 64bit hashes as llseek() offset (for directories) */
#define FMODE_64BITHASH         ((__force fmode_t)0x400)

#define FASYNC_MAGIC 0x4601



#define I_DIRTY_SYNC		(1 << 0)
#define I_DIRTY_DATASYNC	(1 << 1)
#define I_DIRTY_PAGES		(1 << 2)
#define __I_NEW			3
#define I_NEW			(1 << __I_NEW)
#define I_WILL_FREE		(1 << 4)
#define I_FREEING		(1 << 5)
#define I_CLEAR			(1 << 6)
#define __I_SYNC		7
#define I_SYNC			(1 << __I_SYNC)
#define I_REFERENCED		(1 << 8)
#define __I_DIO_WAKEUP		9
#define I_DIO_WAKEUP		(1 << I_DIO_WAKEUP)
#define I_LINKABLE		(1 << 10)
#define I_DIRTY_TIME		(1 << 11)
#define __I_DIRTY_TIME_EXPIRED	12
#define I_DIRTY_TIME_EXPIRED	(1 << __I_DIRTY_TIME_EXPIRED)

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)
#define I_DIRTY_ALL (I_DIRTY | I_DIRTY_TIME)


#define S_PRIVATE	512	/* Inode is fs-internal */

#define MS_KERNMOUNT	(1<<22) /* this is a kern_mount call */


/* Possible states of 'frozen' field */
enum {
	SB_UNFROZEN = 0,		/* FS is unfrozen */
	SB_FREEZE_WRITE	= 1,		/* Writes, dir ops, ioctls frozen */
	SB_FREEZE_PAGEFAULT = 2,	/* Page faults stopped as well */
	SB_FREEZE_FS = 3,		/* For internal FS use (e.g. to stop
					 * internal threads if needed) */
	SB_FREEZE_COMPLETE = 4,		/* ->freeze_fs finished successfully */
};

#define SB_FREEZE_LEVELS (SB_FREEZE_COMPLETE - 1)

	
struct file;
struct vfsmount;
struct inode;
struct seq_file;

typedef int (*filldir_t)(void *, const char *, int, loff_t, u64, unsigned);
typedef struct files_struct *fl_owner_t;

struct fown_struct {
};


struct page;
struct iov_iter;
struct kiocb;
struct swap_info_struct;
struct writeback_control;
struct address_space;
struct address_space_operations {
	int (*writepage)(struct page *page, struct writeback_control *wbc);
	int (*readpage)(struct file *, struct page *);

	/* Write back some dirty pages from this mapping. */
	int (*writepages)(struct address_space *, struct writeback_control *);

	/* Set a page dirty.  Return true if this dirtied it */
	int (*set_page_dirty)(struct page *page);

	int (*readpages)(struct file *filp, struct address_space *mapping,
			struct list_head *pages, unsigned nr_pages);

	int (*write_begin)(struct file *, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned flags,
				struct page **pagep, void **fsdata);
	int (*write_end)(struct file *, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned copied,
				struct page *page, void *fsdata);

	/* Unfortunately this kludge is needed for FIBMAP. Don't use it */
	sector_t (*bmap)(struct address_space *, sector_t);
	void (*invalidatepage) (struct page *, unsigned int, unsigned int);
	int (*releasepage) (struct page *, gfp_t);
	void (*freepage)(struct page *);
	ssize_t (*direct_IO)(int, struct kiocb *, struct iov_iter *iter, loff_t offset);
	/*
	 * migrate the contents of a page to the specified target. If
	 * migrate_mode is MIGRATE_ASYNC, it must not block.
	 */
	int (*migratepage) (struct address_space *,
			struct page *, struct page *, enum migrate_mode);
	int (*launder_page) (struct page *);
	int (*is_partially_uptodate) (struct page *, unsigned long,
					unsigned long);
	void (*is_dirty_writeback) (struct page *, bool *, bool *);
	int (*error_remove_page)(struct address_space *, struct page *);

	/* swapfile support */
	int (*swap_activate)(struct swap_info_struct *sis, struct file *file,
				sector_t *span);
	void (*swap_deactivate)(struct file *file);
};

struct address_space {
	struct inode		*host;		/* owner: inode, block_device */
	atomic_t        i_mmap_writable;/* count VM_SHARED mappings */
	void            *private_data;  /* ditto */
	pgoff_t         writeback_index;/* writeback starts here */
	struct address_space	*assoc_mapping;	/* ditto */
	unsigned long		nrpages;	/* number of total pages */
	struct sg_table *sg;
	unsigned long		flags;		/* error bits/gfp mask */
	const struct address_space_operations *a_ops;	/* methods */
} __attribute__((aligned(sizeof(long))));


struct fasync_struct {
	spinlock_t		fa_lock;
	int			magic;
	int			fa_fd;
	struct fasync_struct	*fa_next; /* singly linked list */
	struct file		*fa_file;
	struct rcu_head		fa_rcu;
};


/*
 * This is the Inode Attributes structure, used for notify_change().  It
 * uses the above definitions as flags, to know which values have changed.
 * Also, in this manner, a Filesystem can look at only the values it cares
 * about.  Basically, these are the attributes that the VFS layer can
 * request to change from the FS layer.
 *
 * Derek Atkins <warlord@MIT.EDU> 94-10-20
 */
struct iattr {
        unsigned int    ia_valid;
        umode_t         ia_mode;
        uid_t           ia_uid;
        gid_t           ia_gid;
        off_t           ia_size;
        struct timespec ia_atime;
        struct timespec ia_mtime;
        struct timespec ia_ctime;

        /*
         * Not an attribute, but an auxiliary info for filesystems wanting to
         * implement an ftruncate() like method.  NOTE: filesystem should
         * check for (ia_valid & ATTR_FILE), and not for (ia_file != NULL).
         */
        struct file     *ia_file;
};



struct path {
	struct vfsmount *mnt;
	struct dentry *dentry;
};
struct kiocb;
struct file_lock;
struct pipe_inode_info;
struct poll_table_struct;
struct vm_area_struct;
struct file_operations {
	struct module *owner;
	loff_t (*llseek) (struct file *, loff_t, int);
	ssize_t (*read) (struct file *, char __user *, size_t, loff_t *);
	ssize_t (*write) (struct file *, const char __user *, size_t, loff_t *);
	ssize_t (*aio_read) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	ssize_t (*aio_write) (struct kiocb *, const struct iovec *, unsigned long, loff_t);
	int (*readdir) (struct file *, void *, filldir_t);
	unsigned int (*poll) (struct file *, struct poll_table_struct *);
	int (*ioctl) (struct inode *, struct file *, unsigned int, unsigned long);
	long (*unlocked_ioctl) (struct file *, unsigned int, unsigned long);
	long (*compat_ioctl) (struct file *, unsigned int, unsigned long);
	int (*mmap) (struct file *, struct vm_area_struct *);
	int (*open) (struct inode *, struct file *);
	int (*flush) (struct file *, fl_owner_t tid);
	int (*release) (struct inode *, struct file *);
	int (*fsync) (struct file *, int datasync);
	int (*aio_fsync) (struct kiocb *, int datasync);
	int (*fasync) (int, struct file *, int);
	int (*lock) (struct file *, int, struct file_lock *);
	ssize_t (*sendpage) (struct file *, struct page *, int, size_t, loff_t *, int);
	unsigned long (*get_unmapped_area)(struct file *, unsigned long, unsigned long, unsigned long, unsigned long);
	int (*check_flags)(int);
	int (*flock) (struct file *, int, struct file_lock *);
	ssize_t (*splice_write)(struct pipe_inode_info *, struct file *, loff_t *, size_t, unsigned int);
	ssize_t (*splice_read)(struct file *, loff_t *, struct pipe_inode_info *, size_t, unsigned int);
	int (*setlease)(struct file *, long, struct file_lock **);
};

struct nameidata;
struct kstat;
struct fiemap_extent_info;
struct cred;
struct inode_operations {
    struct dentry * (*lookup) (struct inode *,struct dentry *, unsigned int);
    void * (*follow_link) (struct dentry *, struct nameidata *);
    int (*permission) (struct inode *, int);
    struct posix_acl * (*get_acl)(struct inode *, int);

    int (*readlink) (struct dentry *, char __user *,int);
    void (*put_link) (struct dentry *, struct nameidata *, void *);

    int (*create) (struct inode *,struct dentry *, umode_t, bool);
    int (*link) (struct dentry *,struct inode *,struct dentry *);
    int (*unlink) (struct inode *,struct dentry *);
    int (*symlink) (struct inode *,struct dentry *,const char *);
    int (*mkdir) (struct inode *,struct dentry *,umode_t);
    int (*rmdir) (struct inode *,struct dentry *);
    int (*mknod) (struct inode *,struct dentry *,umode_t,dev_t);
    int (*rename) (struct inode *, struct dentry *,
            struct inode *, struct dentry *);
    int (*rename2) (struct inode *, struct dentry *,
            struct inode *, struct dentry *, unsigned int);
    int (*setattr) (struct dentry *, struct iattr *);
    int (*getattr) (struct vfsmount *mnt, struct dentry *, struct kstat *);
    int (*setxattr) (struct dentry *, const char *,const void *,size_t,int);
    ssize_t (*getxattr) (struct dentry *, const char *, void *, size_t);
    ssize_t (*listxattr) (struct dentry *, char *, size_t);
    int (*removexattr) (struct dentry *, const char *);
    int (*fiemap)(struct inode *, struct fiemap_extent_info *, u64 start,
              u64 len);
    int (*update_time)(struct inode *, struct timespec *, int);
    int (*atomic_open)(struct inode *, struct dentry *,
               struct file *, unsigned open_flag,
               umode_t create_mode, int *opened);
    int (*tmpfile) (struct inode *, struct dentry *, umode_t);
    int (*set_acl)(struct inode *, struct posix_acl *, int);

    /* WARNING: probably going away soon, do not use! */
    int (*dentry_open)(struct dentry *, struct file *, const struct cred *);
};

struct file_ra_state {
	unsigned int size;		/* # of readahead pages */
	unsigned int async_size;	/* do asynchronous readahead when
					   there are only # of pages ahead */

	unsigned int ra_pages;		/* Maximum readahead window */
	unsigned int mmap_miss;		/* Cache miss stat for mmap accesses */
	loff_t prev_pos;		/* Cache last read() position */
};

struct inode {
	int idx;

	umode_t			i_mode;
	unsigned short		i_opflags;
	kuid_t			i_uid;
	kgid_t			i_gid;
	unsigned int		i_flags;

#ifdef CONFIG_FS_POSIX_ACL
	struct posix_acl	*i_acl;
	struct posix_acl	*i_default_acl;
#endif

	const struct inode_operations	*i_op;
	struct super_block	*i_sb;
	struct address_space   *i_mapping;

#ifdef CONFIG_SECURITY
	void			*i_security;
#endif

	/* Stat data, not accessed from path walking */
	unsigned long		i_ino;
	/*
	 * Filesystems may only read i_nlink directly.  They shall use the
	 * following functions for modification:
	 *
	 *    (set|clear|inc|drop)_nlink
	 *    inode_(inc|dec)_link_count
	 */
	union {
		const unsigned int i_nlink;
		unsigned int __i_nlink;
	};
	dev_t			i_rdev;
	loff_t			i_size;
	struct timespec		i_atime;
	struct timespec		i_mtime;
	struct timespec		i_ctime;
	spinlock_t		i_lock;	/* i_blocks, i_bytes, maybe i_size */
	unsigned short          i_bytes;
	unsigned int		i_blkbits;
	blkcnt_t		i_blocks;

#ifdef __NEED_I_SIZE_ORDERED
	seqcount_t		i_size_seqcount;
#endif

	/* Misc */
	unsigned long		i_state;
	struct mutex		i_mutex;

	unsigned long		dirtied_when;	/* jiffies of first dirtying */
	unsigned long		dirtied_time_when;

	struct hlist_node	i_hash;
	struct list_head	i_wb_list;	/* backing dev IO list */
	struct list_head	i_lru;		/* inode LRU list */
	struct list_head	i_sb_list;
	union {
		struct hlist_head	i_dentry;
		struct rcu_head		i_rcu;
	};
	u64			i_version;
	atomic_t		i_count;
	atomic_t		i_dio_count;
	atomic_t		i_writecount;
#ifdef CONFIG_IMA
	atomic_t		i_readcount; /* struct files open RO */
#endif
	const struct file_operations	*i_fop;	/* former ->i_op->default_file_ops */
	struct file_lock_context	*i_flctx;
	struct address_space	i_data;
	struct list_head	i_devices;
	union {
		struct pipe_inode_info	*i_pipe;
		struct block_device	*i_bdev;
		struct cdev		*i_cdev;
	};

	__u32			i_generation;

#ifdef CONFIG_FSNOTIFY
	__u32			i_fsnotify_mask; /* all events this inode cares about */
	struct hlist_head	i_fsnotify_marks;
#endif

	void			*i_private; /* fs or device private pointer */
};

struct file {
	/*
	 * fu_list becomes invalid after file_free is called and queued via
	 * fu_rcuhead for RCU freeing
	 */
	union {
		struct list_head	fu_list;
	//	struct rcu_head 	fu_rcuhead;
	} f_u;
	int file_descr;
	struct path		f_path;
#define f_dentry	f_path.dentry
#define f_vfsmnt	f_path.mnt
	struct inode		*f_inode;	/* cached value */
	const struct file_operations	*f_op;

	/*
	 * Protects f_ep_links, f_flags, f_pos vs i_size in lseek SEEK_CUR.
	 * Must not be taken from IRQ context.
	 */
	pthread_spinlock_t		f_lock;
#ifdef CONFIG_SMP
	int			f_sb_list_cpu;
#endif
	atomic_t			f_count;
	unsigned int 		f_flags;
	mode_t			f_mode;
	loff_t			f_pos;
	struct fown_struct	f_owner;
	const struct cred	*f_cred;
	struct file_ra_state	f_ra;

	u64			f_version;
#ifdef CONFIG_SECURITY
	void			*f_security;
#endif
	/* needed for tty driver, and maybe others */
	void			*private_data;

#ifdef CONFIG_EPOLL
	/* Used by fs/eventpoll.c to link all the hooks to this file */
	struct list_head	f_ep_links;
	struct list_head	f_tfile_llink;
#endif /* #ifdef CONFIG_EPOLL */
	struct address_space	*f_mapping;
#ifdef CONFIG_DEBUG_WRITECOUNT
	unsigned long f_mnt_write_state;
#endif

#ifdef __QNXNTO__
	void	*drm_attr;
#endif /* __QNXNTO__ */

};

struct kstatfs {
	long f_type;
	long f_bsize;
	u64 f_blocks;
	u64 f_bfree;
	u64 f_bavail;
	u64 f_files;
	u64 f_ffree;
	__kernel_fsid_t f_fsid;
	long f_namelen;
	long f_frsize;
	long f_flags;
	long f_spare[4];
};

struct super_block {
	struct list_head	s_list;		/* Keep this first */
	dev_t			s_dev;		/* search index; _not_ kdev_t */
	unsigned char		s_blocksize_bits;
	unsigned long		s_blocksize;
	loff_t			s_maxbytes;	/* Max file size */
	struct file_system_type	*s_type;
	const struct super_operations	*s_op;
	unsigned long		s_magic;
};

struct writeback_control;
struct shrink_control;
struct super_operations {
   	struct inode *(*alloc_inode)(struct super_block *sb);
	void (*destroy_inode)(struct inode *);

   	void (*dirty_inode) (struct inode *, int flags);
	int (*write_inode) (struct inode *, struct writeback_control *wbc);
	int (*drop_inode) (struct inode *);
	void (*evict_inode) (struct inode *);
	void (*put_super) (struct super_block *);
	int (*sync_fs)(struct super_block *sb, int wait);
	int (*freeze_super) (struct super_block *);
	int (*freeze_fs) (struct super_block *);
	int (*thaw_super) (struct super_block *);
	int (*unfreeze_fs) (struct super_block *);
	int (*statfs) (struct dentry *, struct kstatfs *);
	int (*remount_fs) (struct super_block *, int *, char *);
	void (*umount_begin) (struct super_block *);

	int (*show_options)(struct seq_file *, struct dentry *);
	int (*show_devname)(struct seq_file *, struct dentry *);
	int (*show_path)(struct seq_file *, struct dentry *);
	int (*show_stats)(struct seq_file *, struct dentry *);
#ifdef CONFIG_QUOTA
	ssize_t (*quota_read)(struct super_block *, int, char *, size_t, loff_t);
	ssize_t (*quota_write)(struct super_block *, int, const char *, size_t, loff_t);
	struct dquot **(*get_dquots)(struct inode *);
#endif
	int (*bdev_try_to_free_page)(struct super_block*, struct page*, gfp_t);
	long (*nr_cached_objects)(struct super_block *,
				  struct shrink_control *);
	long (*free_cached_objects)(struct super_block *,
				    struct shrink_control *);
};


struct file_system_type {
	const char *name;
	int fs_flags;
#define FS_REQUIRES_DEV		1 
#define FS_BINARY_MOUNTDATA	2
#define FS_HAS_SUBTYPE		4
#define FS_USERNS_MOUNT		8	/* Can be mounted by userns root */
#define FS_USERNS_DEV_MOUNT	16 /* A userns mount does not imply MNT_NODEV */
#define FS_RENAME_DOES_D_MOVE	32768	/* FS will handle d_move() during rename() internally. */
	struct dentry *(*mount) (struct file_system_type *, int,
		       const char *, void *);
	void (*kill_sb) (struct super_block *);
	struct module *owner;
	struct file_system_type * next;
	struct hlist_head fs_supers;

	struct lock_class_key s_lock_key;
	struct lock_class_key s_umount_key;
	struct lock_class_key s_vfs_rename_key;
	struct lock_class_key s_writers_key[SB_FREEZE_LEVELS];

	struct lock_class_key i_lock_key;
	struct lock_class_key i_mutex_key;
	struct lock_class_key i_mutex_dir_key;
};

struct dentry *mount_pseudo(struct file_system_type *fs_type, char *name,
		const struct super_operations *ops,
		const struct dentry_operations *dops, unsigned long magic);
int simple_pin_fs(struct file_system_type *type, struct vfsmount **mount, int *count);
void simple_release_fs(struct vfsmount **mount, int *count);
void kill_anon_super(struct super_block *sb);

/* Alas, no aliases. Too much hassle with bringing module.h everywhere */
#define fops_get(fops) \
	(((fops) && (fops)->owner) ? (fops) : NULL)
#define fops_put(fops) \
        do { if (fops); } while(0)


static inline loff_t
noop_llseek(struct file *file, loff_t offset, int origin)
{
//	DRM_DEBUG("Stub is called!\n");
	return 0;//file->f_pos;
}

static inline int register_chrdev(unsigned int major, const char *name,
				  const struct file_operations *fops)
{
	//assert(0 && "register_chrdev no impt yet");
   	return 0; //__register_chrdev(major, 0, 256, name, fops);
}

static inline void unregister_chrdev(unsigned int major, const char *name)
{
	//assert(0 && "unregister_chrdev no impt yet");
	//__unregister_chrdev(major, 0, 256, name);
}

static inline unsigned iminor(const struct inode *inode)
{
	return MINOR(inode->i_rdev);
}
/*
 * get additional reference to inode; caller must already hold one.
 */
static inline void ihold(struct inode *inode)
{
	WARN_ON(atomic_inc_return(&inode->i_count) < 2);
}
static inline void iput_final(struct inode *inode)
{
	assert(0 && "iput_final no imp yet!");
}
static inline void iput(struct inode *inode)
{
#ifndef __QNXNTO__
	if (inode) {
		BUG_ON(inode->i_state & I_CLEAR);
		if (atomic_dec_and_lock(&inode->i_count, &inode->i_lock))
			iput_final(inode);
	}
#else
#endif
}

static inline struct inode *file_inode(struct file *f)
{
	return f->f_path.dentry->d_inode;
	//	return f->f_inode;
}


static inline loff_t i_size_read(const struct inode *inode)
{
#if BITS_PER_LONG==32 && defined(CONFIG_SMP)
	loff_t i_size;
	unsigned int seq;

	do {
		seq = read_seqcount_begin(&inode->i_size_seqcount);
		i_size = inode->i_size;
	} while (read_seqcount_retry(&inode->i_size_seqcount, seq));
	return i_size;
#elif BITS_PER_LONG==32 && defined(CONFIG_PREEMPT)
	loff_t i_size;

	preempt_disable();
	i_size = inode->i_size;
	preempt_enable();
	return i_size;
#else
	return inode->i_size;
#endif
}

#define replace_fops(f, fops) \
	do {	\
		struct file *__file = (f); \
		fops_put(__file->f_op); \
		BUG_ON(!(__file->f_op = (fops))); \
	} while(0)

static inline void i_uid_write(struct inode *inode, uid_t uid)
{
	inode->i_uid = make_kuid(&init_user_ns, uid);
}
static inline void i_gid_write(struct inode *inode, gid_t gid)
{
	inode->i_gid = make_kgid(&init_user_ns, gid);
}

extern int simple_statfs(struct dentry *, struct kstatfs *);
extern struct inode *alloc_anon_inode(struct super_block *);

extern ssize_t simple_read_from_buffer(void __user *to, size_t count,
			loff_t *ppos, const void *from, size_t available);
extern ssize_t simple_write_to_buffer(void *to, size_t available, loff_t *ppos,
		const void __user *from, size_t count);
extern loff_t default_llseek(struct file *file, loff_t offset, int whence);

static inline __printf(1, 2)
void __simple_attr_check_format(const char *fmt, ...)
{
	/* don't do anything, just let the compiler check the arguments; */
}

#define DEFINE_SIMPLE_ATTRIBUTE(__fops, __get, __set, __fmt)		\
static int __fops ## _open(struct inode *inode, struct file *file)	\
{									\
	__simple_attr_check_format(__fmt, 0ull);			\
	return simple_attr_open(inode, file, __get, __set, __fmt);	\
}									\
static const struct file_operations __fops = {				\
	.owner	 = THIS_MODULE,						\
	.open	 = __fops ## _open,					\
	.release = simple_attr_release,					\
	.read	 = simple_attr_read,					\
	.write	 = simple_attr_write,					\
	.llseek	 = generic_file_llseek,					\
}

int simple_attr_open(struct inode *inode, struct file *file,
		     int (*get)(void *, u64 *), int (*set)(void *, u64),
		     const char *fmt);
int simple_attr_release(struct inode *inode, struct file *file);
ssize_t simple_attr_read(struct file *file, char *buf,
			 size_t len, loff_t *ppos);
ssize_t simple_attr_write(struct file *file, const char *buf,
			  size_t len, loff_t *ppos);

extern loff_t generic_file_llseek(struct file *file, loff_t offset, int whence);
extern loff_t generic_file_llseek_size(struct file *file, loff_t offset,
		int whence, loff_t maxsize, loff_t eof);
extern loff_t fixed_size_llseek(struct file *file, loff_t offset,
		int whence, loff_t size);

extern loff_t no_llseek(struct file *file, loff_t offset, int whence);

#endif //_QNX_LINUX_FS_H

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/fs.h $ $Rev: 838597 $")
#endif
