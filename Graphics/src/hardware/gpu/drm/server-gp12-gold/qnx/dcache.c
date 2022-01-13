#include <linux/qnx.h>
#include <linux/linux.h>

static int prepend(char **buffer, int *buflen, char *str, int namelen)
{
	*buflen -= namelen;
	if (*buflen < 0)
		return -ENAMETOOLONG;
	*buffer -= namelen;
	memcpy(*buffer, str, namelen);
	return 0;
}

char *simple_dname(struct dentry *dentry, char *buffer, int buflen)
{
	char *end = buffer + buflen;
	/* these dentries are never renamed, so d_lock is not needed */
	if (prepend(&end, &buflen, " (deleted)", 11) ||
	    prepend(&end, &buflen, (char*)dentry->d_name.name, dentry->d_name.len) ||
	    prepend(&end, &buflen, "/", 1))  
		end = ERR_PTR(-ENAMETOOLONG);
	return end;
}

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/qnx/dcache.c $ $Rev: 836322 $")
#endif
