#include <linux/fs.h>
#include <linux/slab.h>
#include <drm/drmP.h>

loff_t generic_file_llseek(struct file *file, loff_t offset, int whence)
{
    return 0;
}

loff_t default_llseek(struct file *file, loff_t offset, int whence)
{
    return 0;
}

struct simple_attr {
	int (*get)(void *, u64 *);
	int (*set)(void *, u64);
	char get_buf[24];	/* enough to store a u64 and "\n\0" */
	char set_buf[24];
	void *data;
	const char *fmt;	/* format for read operation */
	struct mutex mutex;	/* protects access to these buffers */
};

int nonseekable_open(struct inode *inode, struct file *filp)
{
	filp->f_mode &= ~(FMODE_LSEEK | FMODE_PREAD | FMODE_PWRITE);
	return 0;
}

int simple_attr_open(struct inode *inode, struct file *file,
		     int (*get)(void *, u64 *), int (*set)(void *, u64),
		     const char *fmt)
{
	struct simple_attr *attr;

	attr = kmalloc(sizeof(*attr), GFP_KERNEL);
	if (!attr)
		return -ENOMEM;

	attr->get = get;
	attr->set = set;
	attr->data = inode->i_private;
	attr->fmt = fmt;
	mutex_init(&attr->mutex);

	file->private_data = attr;

	return nonseekable_open(inode, file);
}

ssize_t simple_attr_read(struct file *file, char *buf,
			 size_t len, loff_t *ppos)
{
	struct simple_attr *attr;
	size_t size;
	ssize_t ret;

	attr = file->private_data;

	if (!attr->get)
		return -EACCES;
	ret = mutex_lock_interruptible(&attr->mutex);
	if (ret)
		return ret;

	if (*ppos) {		/* continued read */
		size = strlen(attr->get_buf);
	} else {		/* first read */
		u64 val;
		ret = attr->get(attr->data, &val);
		if (ret)
			goto out;

		size = scnprintf(attr->get_buf, sizeof(attr->get_buf),
				 attr->fmt, (unsigned long long)val);
	}

	ret = simple_read_from_buffer(buf, len, ppos, attr->get_buf, size);
out:
	mutex_unlock(&attr->mutex);
	return ret;
}

ssize_t simple_attr_write(struct file *file, const char *buf,
			  size_t len, loff_t *ppos)
{
	struct simple_attr *attr;
	u64 val;
	size_t size;
	ssize_t ret;

	attr = file->private_data;
	if (!attr->set)
		return -EACCES;

	ret = mutex_lock_interruptible(&attr->mutex);
	if (ret)
		return ret;

	ret = -EFAULT;
	size = min(sizeof(attr->set_buf) - 1, len);
	memcpy(attr->set_buf, buf, size);

	attr->set_buf[size] = '\0';
	val = strtoll(attr->set_buf, NULL, 0);
	ret = attr->set(attr->data, val);
	if (ret == 0)
		ret = len; /* on success, claim we got the whole input */

	mutex_unlock(&attr->mutex);
	return ret;
}

int simple_attr_release(struct inode *inode, struct file *file)
{
	struct simple_attr *attr;

	attr = file->private_data;
	mutex_destroy(&attr->mutex);

	kfree(file->private_data);
	return 0;
}

/**
 * simple_read_from_buffer - copy data from the buffer to user space
 * @to: the user space buffer to read to
 * @count: the maximum number of bytes to read
 * @ppos: the current position in the buffer
 * @from: the buffer to read from
 * @available: the size of the buffer
 *
 * The simple_read_from_buffer() function reads up to @count bytes from the
 * buffer @from at offset @ppos into the user space address starting at @to.
 *
 * On success, the number of bytes read is returned and the offset @ppos is
 * advanced by this number, or negative value is returned on error.
 **/
ssize_t simple_read_from_buffer(void *to, size_t count, loff_t *ppos,
				const void *from, size_t available)
{
	loff_t pos = *ppos;

	if (pos < 0) {
		return -EINVAL;
	}
	if (pos >= available || !count) {
		return 0;
	}
	if (count > available - pos) {
		count = available - pos;
	}
	memcpy(to, from + pos, count);

	*ppos = pos + count;
	return count;
}

static inline int unsigned_offsets(struct file *file)
{
	return file->f_mode & FMODE_UNSIGNED_OFFSET;
}

loff_t vfs_setpos(struct file *file, loff_t offset, loff_t maxsize)
{
	if (offset < 0 && !unsigned_offsets(file))
		return -EINVAL;
	if (offset > maxsize)
		return -EINVAL;

	if (offset != file->f_pos) {
		file->f_pos = offset;
		file->f_version = 0;
	}
	return offset;
}

loff_t
generic_file_llseek_size(struct file *file, loff_t offset, int whence,
		loff_t maxsize, loff_t eof)
{
	switch (whence) {
	case SEEK_END:
		offset += eof;
		break;
	case SEEK_CUR:
		/*
		 * Here we special-case the lseek(fd, 0, SEEK_CUR)
		 * position-querying operation.  Avoid rewriting the "same"
		 * f_pos value back to the file because a concurrent read(),
		 * write() or lseek() might have altered it
		 */
		if (offset == 0)
			return file->f_pos;
		/*
		 * f_lock protects against read/modify/write race with other
		 * SEEK_CURs. Note that parallel writes and reads behave
		 * like SEEK_SET.
		 */
		spin_lock(&file->f_lock);
		offset = vfs_setpos(file, file->f_pos + offset, maxsize);
		spin_unlock(&file->f_lock);
		return offset;
	case SEEK_DATA:
		/*
		 * In the generic case the entire file is data, so as long as
		 * offset isn't at the end of the file then the offset is data.
		 */
		if (offset >= eof)
			return -ENXIO;
		break;
	case SEEK_HOLE:
		/*
		 * There is a virtual hole at the end of the file, so as long as
		 * offset isn't i_size or larger, return i_size.
		 */
		if (offset >= eof)
			return -ENXIO;
		offset = eof;
		break;
	}

	return vfs_setpos(file, offset, maxsize);
}

loff_t fixed_size_llseek(struct file *file, loff_t offset, int whence, loff_t size)
{
	switch (whence) {
	case SEEK_SET: case SEEK_CUR: case SEEK_END:
		return generic_file_llseek_size(file, offset, whence,
						size, size);
	default:
		return -EINVAL;
	}
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/fs.c $ $Rev: 836322 $")
#endif
