#include <linux/qnx.h>
#include <linux/linux.h>

#include <drm/drmP.h>

#define SYSCTL_WRITES_LEGACY	-1
#define SYSCTL_WRITES_WARN	 0
#define SYSCTL_WRITES_STRICT	 1

static int sysctl_writes_strict = SYSCTL_WRITES_STRICT;

static int do_proc_dointvec_conv(bool *negp, unsigned long *lvalp,
				 int *valp,
				 int write, void *data)
{
	if (write) {
		if (*negp) {
			if (*lvalp > (unsigned long) INT_MAX + 1)
				return -EINVAL;
			*valp = -*lvalp;
		} else {
			if (*lvalp > (unsigned long) INT_MAX)
				return -EINVAL;
			*valp = *lvalp;
		}
	} else {
		int val = *valp;
		if (val < 0) {
			*negp = true;
			*lvalp = -(unsigned long)val;
		} else {
			*negp = false;
			*lvalp = (unsigned long)val;
		}
	}
	return 0;
}

static void warn_sysctl_write(struct ctl_table *table)
{
	pr_warn_once("%s wrote to %s when file position was not 0!\n"
		"This will not be supported in the future. To silence this\n"
		"warning, set kernel.sysctl_writes_strict = -1\n",
		current->comm, table->procname);
}

static size_t proc_skip_spaces(char **buf)
{
	size_t ret;
	char *tmp = skip_spaces(*buf);
	ret = tmp - *buf;
	*buf = tmp;
	return ret;
}

#define TMPBUFLEN 22
/**
 * proc_get_long - reads an ASCII formatted integer from a user buffer
 *
 * @buf: a kernel buffer
 * @size: size of the kernel buffer
 * @val: this is where the number will be stored
 * @neg: set to %TRUE if number is negative
 * @perm_tr: a vector which contains the allowed trailers
 * @perm_tr_len: size of the perm_tr vector
 * @tr: pointer to store the trailer character
 *
 * In case of success %0 is returned and @buf and @size are updated with
 * the amount of bytes read. If @tr is non-NULL and a trailing
 * character exists (size is non-zero after returning from this
 * function), @tr is updated with the trailing character.
 */
static int proc_get_long(char **buf, size_t *size,
			  unsigned long *val, bool *neg,
			  const char *perm_tr, unsigned perm_tr_len, char *tr)
{
	int len;
	char *p, tmp[TMPBUFLEN];

	if (!*size)
		return -EINVAL;

	len = *size;
	if (len > TMPBUFLEN - 1)
		len = TMPBUFLEN - 1;

	memcpy(tmp, *buf, len);

	tmp[len] = 0;
	p = tmp;
	if (*p == '-' && *size > 1) {
		*neg = true;
		p++;
	} else
		*neg = false;
	if (!isdigit(*p))
		return -EINVAL;

	*val = simple_strtoul(p, &p, 0);

	len = p - tmp;

	/* We don't know if the next char is whitespace thus we may accept
	 * invalid integers (e.g. 1234...a) or two integers instead of one
	 * (e.g. 123...1). So lets not allow such large numbers. */
	if (len == TMPBUFLEN - 1)
		return -EINVAL;

	if (len < *size && perm_tr_len && !memchr(perm_tr, *p, perm_tr_len))
		return -EINVAL;

	if (tr && (len < *size))
		*tr = *p;

	*buf += len;
	*size -= len;

	return 0;
}

/**
 * proc_put_long - converts an integer to a decimal ASCII formatted string
 *
 * @buf: the user buffer
 * @size: the size of the user buffer
 * @val: the integer to be converted
 * @neg: sign of the number, %TRUE for negative
 *
 * In case of success %0 is returned and @buf and @size are updated with
 * the amount of bytes written.
 */
static int proc_put_long(void __user **buf, size_t *size, unsigned long val,
			  bool neg)
{
	int len;
	char tmp[TMPBUFLEN], *p = tmp;

	sprintf(p, "%s%lu", neg ? "-" : "", val);
	len = strlen(tmp);
	if (len > *size)
		len = *size;
	if (copy_to_user(*buf, tmp, len))
		return -EFAULT;
	*size -= len;
	*buf += len;
	return 0;
}
#undef TMPBUFLEN

static int proc_put_char(void __user **buf, size_t *size, char c)
{
	if (*size) {
		char __user **buffer = (char __user **)buf;
		if (put_user(c, *buffer))
			return -EFAULT;
		(*size)--, (*buffer)++;
		*buf = *buffer;
	}
	return 0;
}

static const char proc_wspace_sep[] = { ' ', '\t', '\n' };

static int __do_proc_dointvec(void *tbl_data, struct ctl_table *table,
		  int write, void __user *buffer,
		  size_t *lenp, loff_t *ppos,
		  int (*conv)(bool *negp, unsigned long *lvalp, int *valp,
			      int write, void *data),
		  void *data)
{
	int *i, vleft, first = 1, err = 0;
	size_t left;
	char *kbuf = NULL, *p = NULL;
	
	if (!tbl_data || !table->maxlen || !*lenp || (*ppos && !write)) {
		*lenp = 0;
		return 0;
	}
	
	i = (int *) tbl_data;
	vleft = table->maxlen / sizeof(*i);
	left = *lenp;

	if (!conv)
		conv = do_proc_dointvec_conv;

	if (write) {
		if (*ppos) {
			switch (sysctl_writes_strict) {
			case SYSCTL_WRITES_STRICT:
				goto out;
			case SYSCTL_WRITES_WARN:
				warn_sysctl_write(table);
				break;
			default:
				break;
			}
		}

		if (left > PAGE_SIZE - 1)
			left = PAGE_SIZE - 1;
		p = kbuf = memdup_user_nul(buffer, left);
		if (IS_ERR(kbuf))
			return PTR_ERR(kbuf);
	}

	for (; left && vleft--; i++, first=0) {
		unsigned long lval;
		bool neg;

		if (write) {
			left -= proc_skip_spaces(&p);

			if (!left)
				break;
			err = proc_get_long(&p, &left, &lval, &neg,
					     proc_wspace_sep,
					     sizeof(proc_wspace_sep), NULL);
			if (err)
				break;
			if (conv(&neg, &lval, i, 1, data)) {
				err = -EINVAL;
				break;
			}
		} else {
			if (conv(&neg, &lval, i, 0, data)) {
				err = -EINVAL;
				break;
			}
			if (!first)
				err = proc_put_char(&buffer, &left, '\t');
			if (err)
				break;
			err = proc_put_long(&buffer, &left, lval, neg);
			if (err)
				break;
		}
	}

	if (!write && !first && left && !err)
		err = proc_put_char(&buffer, &left, '\n');
	if (write && !err && left)
		left -= proc_skip_spaces(&p);
	if (write) {
		kfree(kbuf);
		if (first)
			return err ? : -EINVAL;
	}
	*lenp -= left;
out:
	*ppos += *lenp;
	return err;
}

static int do_proc_dointvec(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos,
		  int (*conv)(bool *negp, unsigned long *lvalp, int *valp,
			      int write, void *data),
		  void *data)
{
	return __do_proc_dointvec(table->data, table, write,
			buffer, lenp, ppos, conv, data);
}

struct do_proc_dointvec_minmax_conv_param {
	int *min;
	int *max;
};

static int do_proc_dointvec_minmax_conv(bool *negp, unsigned long *lvalp,
					int *valp,
					int write, void *data)
{
	struct do_proc_dointvec_minmax_conv_param *param = data;
	if (write) {
		int val = *negp ? -*lvalp : *lvalp;
		if ((param->min && *param->min > val) ||
		    (param->max && *param->max < val))
			return -EINVAL;
		*valp = val;
	} else {
		int val = *valp;
		if (val < 0) {
			*negp = true;
			*lvalp = -(unsigned long)val;
		} else {
			*negp = false;
			*lvalp = (unsigned long)val;
		}
	}
	return 0;
}

/**
 * proc_dointvec_minmax - read a vector of integers with min/max values
 * @table: the sysctl table
 * @write: %TRUE if this is a write to the sysctl file
 * @buffer: the user buffer
 * @lenp: the size of the user buffer
 * @ppos: file position
 *
 * Reads/writes up to table->maxlen/sizeof(unsigned int) integer
 * values from/to the user buffer, treated as an ASCII string.
 *
 * This routine will ensure the values are within the range specified by
 * table->extra1 (min) and table->extra2 (max).
 *
 * Returns 0 on success.
 */
int proc_dointvec_minmax(struct ctl_table *table, int write,
		  void __user *buffer, size_t *lenp, loff_t *ppos)
{
	struct do_proc_dointvec_minmax_conv_param param = {
		.min = (int *) table->extra1,
		.max = (int *) table->extra2,
	};
	return do_proc_dointvec(table, write, buffer, lenp, ppos,
				do_proc_dointvec_minmax_conv, &param);
}

struct ctl_table_header *register_sysctl_table(struct ctl_table * table)
{
	return NULL;
}

struct ctl_table_header *register_sysctl_paths(const struct ctl_path *path, struct ctl_table *table)
{
	return NULL;
}

void unregister_sysctl_table(struct ctl_table_header * table)
{
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/qnx_sysctl.c $ $Rev: 853904 $")
#endif
