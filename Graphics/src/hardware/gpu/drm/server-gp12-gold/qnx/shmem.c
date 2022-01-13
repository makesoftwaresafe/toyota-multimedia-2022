#include <linux/qnx.h>
#include <linux/linux.h>
#include <drm/drmP.h>

#include "mmap_trace.h"

void page_attach(struct page * page, void * vaddr);
void page_attach_ext(struct page* page, void* vaddr, off64_t paddr);

off64_t _vaddr_offset_qnx(void* vaddr, size_t size, size_t* contiguous_block)
{
	off64_t paddr = 0;

	if (mem_offset64(vaddr, NOFD, size, &paddr, contiguous_block) == -1) {
		qnx_error("mem_offset64");
	}

	return paddr;
}

static struct file* _shmem_file_setup(const char *name, loff_t size, unsigned long flags, unsigned long shmctl_flags)
{
	int rv, fd = -1, page_count;
	struct file* filp = NULL;
	void* vaddr = NULL;
	struct sg_table* st = NULL;
	struct scatterlist* dst = NULL;
	struct page* page = NULL;
	struct page* page_iter = NULL;
	int err = EOK;
	unsigned long i, j;
	size_t contiguous_block;
	off64_t paddr;

	if (size == 0) {
		err = EINVAL;
		goto error;
	}

	filp = kzalloc(sizeof(struct file), GFP_KERNEL);
	if (filp == NULL) {
		qnx_error("kzalloc(struct file) failed\n");
		err = ENOMEM;
		goto error;
	}
	atomic_long_set(&filp->f_count, 1);

	/* Create a new memory object */
	fd = shm_open(SHM_ANON, O_RDWR | O_CREAT | O_EXCL, 0666);
	if (fd == -1) {
		err = errno;
		qnx_error("shm_open() failed: %s", strerror(err));
		goto error;
	}

	rv = shm_ctl(fd, SHMCTL_ANON | SHMCTL_LAZYWRITE | shmctl_flags, 0, (uint64_t)size);
	if (rv != 0) {
		err = errno;
		qnx_error("shm_ctl() failed: %s", strerror(err));
		goto error;
	}

	filp->file_descr = fd;
	size = (size + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
	page_count = size / PAGE_SIZE;
	filp->f_path.dentry = kzalloc(sizeof(struct dentry), GFP_KERNEL);
	if (filp->f_path.dentry == NULL) {
		qnx_error("kzalloc(struct dentry) failed");
		err = ENOMEM;
		goto error;
	}
	filp->f_path.dentry->d_inode = kzalloc(sizeof(struct inode), GFP_KERNEL);
	if (filp->f_path.dentry->d_inode == NULL) {
		qnx_error("kzalloc(struct inode) failed");
		err = ENOMEM;
		goto error;
	}
	filp->f_path.dentry->d_inode->i_mapping = kzalloc(sizeof(struct address_space), GFP_KERNEL);
	if (filp->f_path.dentry->d_inode->i_mapping == NULL) {
		qnx_error("kzalloc(struct address_space) failed");
		err = ENOMEM;
		goto error;
	}

	/* Share filemappings with inode mappings */
	filp->f_mapping = filp->f_path.dentry->d_inode->i_mapping;
	filp->f_path.dentry->d_inode->i_mapping->host = filp->f_path.dentry->d_inode;
	filp->f_path.dentry->d_inode->i_mapping->nrpages = page_count;
	filp->f_path.dentry->d_inode->i_size = size;

	vaddr = mmap64(NULL, size, PROT_READ | PROT_WRITE | PROT_NOCACHE, MAP_SHARED, fd, 0);
	if (vaddr == MAP_FAILED) {
		err = errno;
		qnx_error("mmap64() failed");
		goto error;
	}
	mmap_trace_add_range(vaddr, size);

	/* Copy sg so that we make an independent mapping */
	st = kzalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (st == NULL) {
		qnx_error("kmalloc(struct sg_table) failed");
		err = ENOMEM;
		goto error;
	}

	rv = sg_alloc_table(st, page_count, GFP_KERNEL);
	if (rv) {
		qnx_error("sg_alloc_table() failed: %s", strerror(-rv));
		err = -rv;
		goto error;
	}
	dst = st->sgl;

	page = kzalloc(sizeof(struct page) * page_count, GFP_KERNEL);
	if (page == NULL) {
		qnx_error("kzalloc(struct page) failed");
		err = ENOMEM;
		goto error;
	}

	page_iter = page;
	for (i = 0; i < page_count; i++) {
		paddr = _vaddr_offset_qnx(vaddr + i * PAGE_SIZE, (page_count - i) * PAGE_SIZE, &contiguous_block);
		if (paddr == 0) {
			/* Message was output at _vaddr_offset_qnx() function already */
			err = ENOMEM;
			goto error;
		}
		for (j = i; (j < i + contiguous_block / PAGE_SIZE) && (j < page_count); j++) {
			init_page_count(page_iter);
			page_attach_ext(page_iter, (char*)vaddr + j * PAGE_SIZE, paddr);
			sg_set_page(dst, page_iter, PAGE_SIZE, 0);
			page_iter->virtual_map_mode = QNX_PAGE_MAP_WC;
			dst++;
			page_iter++;
			paddr += PAGE_SIZE;
		}
		i += contiguous_block / PAGE_SIZE - 1;
	}

	filp->f_path.dentry->d_inode->i_mapping->sg = st;

	return filp;

error:
	kfree(page);
	if (st) {
		sg_free_table(st);
		st->sgl = NULL;
	}
	kfree(st);
	if (vaddr) {
		mmap_trace_del_range(vaddr, size);
		rv = munmap(vaddr, size);
		if (rv) {
			qnx_error("munmap() call failed!");
		}
	}
	if (filp) {
		if (filp->f_path.dentry) {
			if (filp->f_path.dentry->d_inode) {
				kfree(filp->f_path.dentry->d_inode->i_mapping);
				filp->f_path.dentry->d_inode->i_mapping = NULL;
			}
			kfree(filp->f_path.dentry->d_inode);
			filp->f_path.dentry->d_inode = NULL;
		}
		kfree(filp->f_path.dentry);
		filp->f_path.dentry = NULL;
		filp->file_descr = -1;
	}
	if (fd != -1) {
		close(fd);
	}
	kfree(filp);

	return ERR_PTR(-err);
}

struct file* shmem_file_setup(const char *name, loff_t size, unsigned long flags)
{
	return _shmem_file_setup(name, size, flags, 0);
}

struct file* shmem_file_setup_cma(const char *name, loff_t size, unsigned long flags)
{
	/* alloc physical contiguous memory */
	return _shmem_file_setup(name, size, flags, SHMCTL_PHYS);
}

static void _shmem_truncate(struct inode *inode)
{
	struct page* page = NULL;
	struct page* page_iter = NULL;
	int rv, i;

	BUG_ON(!inode);
	if (inode->i_mapping) {
		if (inode->i_mapping->sg != NULL) {
			page = sg_page(inode->i_mapping->sg->sgl);

			/* Alt mapping should be destroyed first, since it depends on main map       */
			/* otherwise memory will be freed to OS and be still mapped into our process */
			if (page->virtual_alt != NULL) {
				mmap_trace_del_range(page->virtual_alt, (size_t)inode->i_size);
				rv = munmap(page->virtual_alt, (size_t)inode->i_size);
				if (rv) {
					qnx_error("munmap() call failed for ->virtual_alt!");
				}
				page->virtual_alt = NULL;
			}
			if (page_address(page) != NULL) {
				mmap_trace_del_range(page_address(page), (size_t)inode->i_size);
				rv = munmap(page_address(page), (size_t)inode->i_size);
				if (rv) {
					qnx_error("munmap() call failed for ->virtual!");
				}
				set_page_address(page, NULL);
			}

			/* Cleanup pointers to make crash imminent in case of pages re-using */
			for (i = 0; i < inode->i_mapping->nrpages; i++) {
				page_iter = shmem_read_mapping_page(inode->i_mapping, i);
				memset(page_iter, 0x00, sizeof(*page_iter));
			}

			kfree(page);
			sg_free_table(inode->i_mapping->sg);
			kfree(inode->i_mapping->sg);
			inode->i_mapping->sg = NULL;
		}
		kfree(inode->i_mapping);
		inode->i_mapping = NULL;
	}
}

void shmem_truncate_range(struct inode *inode, loff_t lstart, loff_t lend)
{
	if (lstart == 0 && lend == (loff_t)-1) {
		_shmem_truncate(inode);
	} else {
		BUG();
	}
}

int pagecache_write_begin(struct file *file, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned flags,
				struct page **pagep, void **fsdata)
{
	struct scatterlist* sgl = mapping->sg->sgl;
	int skip_pages = pos / PAGE_SIZE;

	while (skip_pages) {
		skip_pages--;
		sgl = sg_next(sgl);
		if (sgl == NULL) {
			DRM_ERROR("Unexpected end of scatterlist\n");
			return -1;
		}
	}
	*pagep = sg_page(sgl);

	return 0;
}
EXPORT_SYMBOL(pagecache_write_begin);

int pagecache_write_end(struct file *file, struct address_space *mapping,
				loff_t pos, unsigned len, unsigned copied,
				struct page *page, void *fsdata)
{
	return 0;
}
EXPORT_SYMBOL(pagecache_write_end);

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/shmem.c $ $Rev: 874574 $")
#endif
