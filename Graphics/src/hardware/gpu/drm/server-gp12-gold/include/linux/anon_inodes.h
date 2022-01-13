/*
 *  include/linux/anon_inodes.h
 *
 *  Copyright (C) 2007  Davide Libenzi <davidel@xmailserver.org>
 *
 */

#ifndef _LINUX_ANON_INODES_H
#define _LINUX_ANON_INODES_H

#include <linux/bug.h>

struct file_operations;

struct file *anon_inode_getfile(const char *name,
				const struct file_operations *fops,
				void *priv, int flags);
int anon_inode_getfd(const char *name, const struct file_operations *fops,
		     void *priv, int flags);

#endif /* _LINUX_ANON_INODES_H */


#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/anon_inodes.h $ $Rev: 841929 $")
#endif
