#ifndef _QNX_LINUX_SEQ_FILE_H
#define _QNX_LINUX_SEQ_FILE_H

#include <linux/mutex.h>

#ifdef __QNXNTO__
#include <linux/list.h>
#include <linux/cred.h>
#endif /* __QNXNTO__ */

struct seq_operations;
struct file;
struct path;
struct inode;
struct dentry;

struct seq_file {
	char *buf;
	size_t size;
	size_t from;
	size_t count;
	size_t pad_until;
	loff_t index;
	loff_t read_pos;
	u64 version;
	struct mutex lock;
	const struct seq_operations *op;
	int poll_event;
	void *private;
};

struct seq_operations {
	void * (*start) (struct seq_file *m, loff_t *pos);
	void (*stop) (struct seq_file *m, void *v);
	void * (*next) (struct seq_file *m, void *v, loff_t *pos);
	int (*show) (struct seq_file *m, void *v);
};

#define SEQ_SKIP 1

/**
 * seq_get_buf - get buffer to write arbitrary data to
 * @m: the seq_file handle
 * @bufp: the beginning of the buffer is stored here
 *
 * Return the number of bytes available in the buffer, or zero if
 * there's no space.
 */
static inline size_t seq_get_buf(struct seq_file *m, char **bufp)
{
	BUG_ON(m->count > m->size);
	if (m->count < m->size)
		*bufp = m->buf + m->count;
	else
		*bufp = NULL;

	return m->size - m->count;
}

/**
 * seq_commit - commit data to the buffer
 * @m: the seq_file handle
 * @num: the number of bytes to commit
 *
 * Commit @num bytes of data written to a buffer previously acquired
 * by seq_buf_get.  To signal an error condition, or that the data
 * didn't fit in the available space, pass a negative @num value.
 */
static inline void seq_commit(struct seq_file *m, int num)
{
	if (num < 0) {
		m->count = m->size;
	} else {
		BUG_ON(m->count + num > m->size);
		m->count += num;
	}
}

char *mangle_path(char *s, char *p, char *esc);
int seq_open(struct file *, const struct seq_operations *);
ssize_t seq_read(struct file *, char __user *, size_t, loff_t *);
loff_t seq_lseek(struct file *, loff_t, int);
int seq_release(struct inode *, struct file *);
int seq_escape(struct seq_file *, const char *, const char *);
void seq_putc(struct seq_file *m, char c);
void seq_puts(struct seq_file *m, const char *s);
int seq_write(struct seq_file *seq, const void *data, size_t len);

void seq_vprintf(struct seq_file *m, const char *fmt, va_list args) __attribute__ ((format (printf,2,0)));
void seq_printf(struct seq_file *m, const char *fmt, ...) __attribute__ ((format (printf,2,3)));;

int seq_path(struct seq_file *, struct path *, char *);
int seq_dentry(struct seq_file *, struct dentry *, char *);
int seq_path_root(struct seq_file *m, struct path *path, struct path *root,
		  char *esc);
int seq_bitmap(struct seq_file *m, const unsigned long *bits,
				   unsigned int nr_bits);
/*static inline int seq_cpumask(struct seq_file *m, const struct cpumask *mask)
{
	return seq_bitmap(m, cpumask_bits(mask), nr_cpu_ids);
}

static inline int seq_nodemask(struct seq_file *m, nodemask_t *mask)
{
	return seq_bitmap(m, mask->bits, MAX_NUMNODES);
}

int seq_bitmap_list(struct seq_file *m, const unsigned long *bits,
		unsigned int nr_bits);

static inline int seq_cpumask_list(struct seq_file *m,
				   const struct cpumask *mask)
{
	return seq_bitmap_list(m, cpumask_bits(mask), nr_cpu_ids);
}

static inline int seq_nodemask_list(struct seq_file *m, nodemask_t *mask)
{
	return seq_bitmap_list(m, mask->bits, MAX_NUMNODES);
}*/

int single_open(struct file *, int (*)(struct seq_file *, void *), void *);
int single_release(struct inode *, struct file *);
void *__seq_open_private(struct file *, const struct seq_operations *, int);
int seq_open_private(struct file *, const struct seq_operations *, int);
int seq_release_private(struct inode *, struct file *);

#define SEQ_START_TOKEN ((void *)1)

/*
 * Helpers for iteration over list_head-s in seq_files
 */

extern struct list_head *seq_list_start(struct list_head *head,
		loff_t pos);
extern struct list_head *seq_list_start_head(struct list_head *head,
		loff_t pos);
extern struct list_head *seq_list_next(void *v, struct list_head *head,
		loff_t *ppos);

/*
 * Helpers for iteration over hlist_head-s in seq_files
 */

extern struct hlist_node *seq_hlist_start(struct hlist_head *head,
					  loff_t pos);
extern struct hlist_node *seq_hlist_start_head(struct hlist_head *head,
					       loff_t pos);
extern struct hlist_node *seq_hlist_next(void *v, struct hlist_head *head,
					 loff_t *ppos);

extern struct hlist_node *seq_hlist_start_rcu(struct hlist_head *head,
					      loff_t pos);
extern struct hlist_node *seq_hlist_start_head_rcu(struct hlist_head *head,
						   loff_t pos);
extern struct hlist_node *seq_hlist_next_rcu(void *v,
						   struct hlist_head *head,
						   loff_t *ppos);

static inline struct user_namespace *seq_user_ns(struct seq_file *seq)
{
#ifdef CONFIG_USER_NS
	return seq->user_ns;
#else
	//extern struct user_namespace init_user_ns;
	//return &init_user_ns;
	return 0;
#endif
}


#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/seq_file.h $ $Rev: 836322 $")
#endif
