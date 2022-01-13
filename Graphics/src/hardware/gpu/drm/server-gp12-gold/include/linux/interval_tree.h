#ifndef _LINUX_INTERVAL_TREE_H
#define _LINUX_INTERVAL_TREE_H

#include <linux/rbtree.h>

struct interval_tree_node {
	struct rb_node rb;
	unsigned long start;	/* Start of interval */
	unsigned long last;	/* Last location _in_ interval */
	unsigned long __subtree_last;
};

extern void
interval_tree_insert(struct interval_tree_node *node, struct rb_root *root);

extern void
interval_tree_remove(struct interval_tree_node *node, struct rb_root *root);

extern struct interval_tree_node *
interval_tree_iter_first(struct rb_root *root,
			 unsigned long start, unsigned long last);

extern struct interval_tree_node *
interval_tree_iter_next(struct interval_tree_node *node,
			unsigned long start, unsigned long last);

#endif	/* _LINUX_INTERVAL_TREE_H */

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/gpu/drm/server-gp12-gold/include/linux/interval_tree.h $ $Rev: 839737 $")
#endif
