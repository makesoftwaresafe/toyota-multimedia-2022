#include <linux/qnx.h>
#include <linux/linux.h>

#define LAST_INO_BATCH 1024

/*
 * Empty aops. Can be used for the cases where the user does not
 * define any of the address_space operations.
 */
const struct address_space_operations empty_aops = {
};

unsigned int get_next_ino(void)
{
	static atomic_t shared_last_ino;
	int next = atomic_add_return(1, &shared_last_ino);
	unsigned int res = next - LAST_INO_BATCH;
	return res;
}

static int no_open(struct inode *inode, struct file *file)
{
	return -ENXIO;
}

int security_inode_alloc(struct inode *inode)
{
	//inode->i_security = NULL;
	return 0; //security_ops->inode_alloc_security(inode);
}

/**
 * inode_init_always - perform inode structure intialisation
 * @sb: superblock inode belongs to
 * @inode: inode to initialise
 *
 * These are initializations that need to be done on every inode
 * allocation as the fields are not initialised by slab allocation.
 */
int inode_init_always(struct super_block *sb, struct inode *inode)
{
	static const struct inode_operations empty_iops;
	static const struct file_operations no_open_fops = {.open = no_open};
	struct address_space *const mapping = &inode->i_data;

	inode->i_sb = sb;
	//inode->i_blkbits = sb->s_blocksize_bits;
	inode->i_flags = 0;
	atomic_set(&inode->i_count, 1);
	inode->i_op = &empty_iops;
	inode->i_fop = &no_open_fops;
	inode->__i_nlink = 1;
	inode->i_opflags = 0;
	i_uid_write(inode, 0);
	i_gid_write(inode, 0);
	atomic_set(&inode->i_writecount, 0);
	inode->i_size = 0;
	inode->i_blocks = 0;
	inode->i_bytes = 0;
	inode->i_generation = 0;
	inode->i_pipe = NULL;
	inode->i_bdev = NULL;
	inode->i_cdev = NULL;
	inode->i_rdev = 0;
	inode->dirtied_when = 0;

	if (security_inode_alloc(inode))
		goto out;
//	spin_lock_init(&inode->i_lock);
//	lockdep_set_class(&inode->i_lock, &sb->s_type->i_lock_key);

//	mutex_init(&inode->i_mutex);
//	lockdep_set_class(&inode->i_mutex, &sb->s_type->i_mutex_key);

	atomic_set(&inode->i_dio_count, 0);

	mapping->a_ops = &empty_aops;
	mapping->host = inode;
	mapping->flags = 0;
	atomic_set(&mapping->i_mmap_writable, 0);
	mapping_set_gfp_mask(mapping, GFP_HIGHUSER_MOVABLE);
	mapping->private_data = NULL;
	mapping->writeback_index = 0;
	inode->i_private = NULL;
	inode->i_mapping = mapping;
	INIT_HLIST_HEAD(&inode->i_dentry);	/* buggered by rcu freeing */
#ifdef CONFIG_FS_POSIX_ACL
	inode->i_acl = inode->i_default_acl = ACL_NOT_CACHED;
#endif

#ifdef CONFIG_FSNOTIFY
	inode->i_fsnotify_mask = 0;
#endif
	inode->i_flctx = NULL;

	return 0;
out:
	return -ENOMEM;
}

static struct inode *alloc_inode(struct super_block *sb)
{
	struct inode *inode;
#ifdef __QNXNTO__
	/*TODO. fixme */
	inode = kzalloc(sizeof(*inode), GFP_KERNEL);
#else	
	if (sb->s_op->alloc_inode)
		inode = sb->s_op->alloc_inode(sb);
	else
		inode = kmem_cache_alloc(inode_cachep, GFP_KERNEL);
#endif
	
	if (!inode)
		return NULL;

	if (unlikely(inode_init_always(sb, inode))) {
#ifdef __QNXNTO__
		kfree(inode);
#else
		if (inode->i_sb->s_op->destroy_inode)
			inode->i_sb->s_op->destroy_inode(inode);
		else
			kmem_cache_free(inode_cachep, inode);
#endif
		return NULL;
	}
	return inode;
}

/**
 *	new_inode_pseudo 	- obtain an inode
 *	@sb: superblock
 *
 *	Allocates a new inode for given superblock.
 *	Inode wont be chained in superblock s_inodes list
 *	This means :
 *	- fs can't be unmount
 *	- quotas, fsnotify, writeback can't work
 */
struct inode *new_inode_pseudo(struct super_block *sb)
{
	struct inode *inode = alloc_inode(sb);

	if (inode) {
		spin_lock(&inode->i_lock);
		inode->i_state = 0;
		spin_unlock(&inode->i_lock);
		INIT_LIST_HEAD(&inode->i_sb_list);
	}
	return inode;
}

/*
 * nop .set_page_dirty method so that people can use .page_mkwrite on
 * anon inodes.
 */
static int anon_set_page_dirty(struct page *page)
{
	return 0;
};

/*
 * A single inode exists for all anon_inode files. Contrary to pipes,
 * anon_inode inodes have no associated per-instance data, so we need
 * only allocate one of them.
 */
struct inode *alloc_anon_inode(struct super_block *s)
{
	static const struct address_space_operations anon_aops = {
		.set_page_dirty = anon_set_page_dirty,
	};
	struct inode *inode = new_inode_pseudo(s);

	if (!inode)
		return ERR_PTR(-ENOMEM);

	inode->i_ino = get_next_ino();
	inode->i_mapping->a_ops = &anon_aops;

	/*
	 * Mark the inode dirty from the very beginning,
	 * that way it will never be moved to the dirty
	 * list because mark_inode_dirty() will think
	 * that it already _is_ on the dirty list.
	 */
	inode->i_state = I_DIRTY;
	inode->i_mode = S_IRUSR | S_IWUSR;
	inode->i_uid = current_fsuid();
	inode->i_gid = current_fsgid();
	inode->i_flags |= S_PRIVATE;
	inode->i_atime = inode->i_mtime = inode->i_ctime = CURRENT_TIME;
	return inode;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/inode.c $ $Rev: 843003 $")
#endif
