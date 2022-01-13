#ifndef _LINUX_LIST_SORT_H
#define _LINUX_LIST_SORT_H

#include <linux/types.h>

struct list_head;

void list_sort(void *priv, struct list_head *head,
	       int (*cmp)(void *priv, struct list_head *a,
			  struct list_head *b));
#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/list_sort.h $ $Rev: 836322 $")
#endif
