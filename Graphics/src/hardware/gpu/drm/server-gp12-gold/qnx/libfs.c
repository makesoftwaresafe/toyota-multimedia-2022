#include <linux/qnx.h>
#include <linux/linux.h>
#include <linux/statfs.h>


static DEFINE_SPINLOCK(pin_fs_lock);

/*
 * Common helper for pseudo-filesystems (sockfs, pipefs, bdev - stuff that
 * will never be mountable)
 */
struct dentry *mount_pseudo(struct file_system_type *fs_type, char *name,
	const struct super_operations *ops,
	const struct dentry_operations *dops, unsigned long magic)
{
#ifdef __QNXNTO__
	return 0;
#else	
	struct super_block *s;
	struct dentry *dentry;
	struct inode *root;
	struct qstr d_name = QSTR_INIT(name, strlen(name));

	s = sget(fs_type, NULL, set_anon_super, MS_NOUSER, NULL);
	if (IS_ERR(s))
		return ERR_CAST(s);

	s->s_maxbytes = MAX_LFS_FILESIZE;
	s->s_blocksize = PAGE_SIZE;
	s->s_blocksize_bits = PAGE_SHIFT;
	s->s_magic = magic;
	s->s_op = ops ? ops : &simple_super_operations;
	s->s_time_gran = 1;
	root = new_inode(s);
	if (!root)
		goto Enomem;
	/*
	 * since this is the first inode, make it number 1. New inodes created
	 * after this must take care not to collide with it (by passing
	 * max_reserved of 1 to iunique).
	 */
	root->i_ino = 1;
	root->i_mode = S_IFDIR | S_IRUSR | S_IWUSR;
	root->i_atime = root->i_mtime = root->i_ctime = CURRENT_TIME;
	dentry = __d_alloc(s, &d_name);
	if (!dentry) {
		iput(root);
		goto Enomem;
	}
	d_instantiate(dentry, root);
	s->s_root = dentry;
	s->s_d_op = dops;
	s->s_flags |= MS_ACTIVE;
	return dget(s->s_root);

Enomem:
	deactivate_locked_super(s);
	return ERR_PTR(-ENOMEM);
#endif	
}


void mntput(struct vfsmount *mnt)
{
#if 0
	if (mnt) {
		struct mount *m = real_mount(mnt);
		/* avoid cacheline pingpong, hope gcc doesn't get "smart" */
		if (unlikely(m->mnt_expiry_mark))
			m->mnt_expiry_mark = 0;
		mntput_no_expire(m);
	}
#endif	
}

struct vfsmount *mntget(struct vfsmount *mnt)
{
#if 0	
	if (mnt)
		mnt_add_count(real_mount(mnt), 1);
#endif	
	return mnt;
}


#define PAGE_CACHE_SIZE		PAGE_SIZE

int simple_statfs(struct dentry *dentry, struct kstatfs *buf)
{
	buf->f_type = dentry->d_sb->s_magic;
	buf->f_bsize = PAGE_CACHE_SIZE;
	buf->f_namelen = NAME_MAX;
	return 0;
}

void simple_release_fs(struct vfsmount **mount, int *count)
{
	struct vfsmount *mnt;
	spin_lock(&pin_fs_lock);
	mnt = *mount;
	if (!--*count)
		*mount = NULL;
	spin_unlock(&pin_fs_lock);
	mntput(mnt);
}

void kill_anon_super(struct super_block *sb)
{
	BUG();
}

static struct vfsmount* vfs_kern_mount(struct file_system_type *type, int flags, const char *name, void *data)
{
	struct vfsmount * mount;
	mount = kmalloc(sizeof(*mount), GFP_KERNEL);
	if (!mount){
		return ERR_PTR(-ENOMEM);
	}
	return mount;
}

int simple_pin_fs(struct file_system_type *type, struct vfsmount **mount, int *count)
{
	struct vfsmount *mnt = NULL;
	spin_lock(&pin_fs_lock);
	if (unlikely(!*mount)) {
		spin_unlock(&pin_fs_lock);
		mnt = vfs_kern_mount(type, MS_KERNMOUNT, type->name, NULL);
		if (IS_ERR(mnt))
			return PTR_ERR(mnt);
		spin_lock(&pin_fs_lock);
		if (!*mount)
			*mount = mnt;
	}
	mntget(*mount);
	++*count;
	spin_unlock(&pin_fs_lock);
	mntput(mnt);
	return 0;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/libfs.c $ $Rev: 846345 $")
#endif
