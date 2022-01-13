/*
* Copyright (c) 2017 QNX Software Systems.
* Modified from Linux original from Yocto Linux kernel GP101 from
* /include/linux/list.h.
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#ifndef _LINUX_LIST_H
#define _LINUX_LIST_H

#include <linux/types.h>

struct list_head;

typedef struct list_head {
	struct list_head *next;
	struct list_head *prev;
} list_head_t;

typedef struct hlist_head {
	struct hlist_node *first;
} hlist_head_t;

typedef struct hlist_node {
	struct hlist_node *next;
	struct hlist_node **pprev;
} hlist_node_t;


#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LINUX_LIST_HEAD(name) \
	list_head_t name = LIST_HEAD_INIT(name)

#define LIST_HEAD LINUX_LIST_HEAD 

#define list_entry(ptr, type, member) \
	container_of(ptr, type, member)

#define hlist_entry(ptr, type, member) container_of(ptr,type,member)

#define hlist_for_each(pos, head) \
	for (pos = (head)->first; pos ; pos = pos->next)

#define hlist_entry_safe(ptr, type, member) \
	({ typeof(ptr) ____ptr = (ptr); \
	   ____ptr ? hlist_entry(____ptr, type, member) : NULL; \
	})

#define hlist_for_each_entry(pos, head, member)				\
	for (pos = hlist_entry_safe((head)->first, typeof(*(pos)), member);\
	     pos;							\
	     pos = hlist_entry_safe((pos)->member.next, typeof(*(pos)), member))

#define hlist_for_each_safe(pos, n, head) \
	for (pos = (head)->first; pos && ({ n = pos->next; 1; }); \
	     pos = n)

#define hlist_for_each_entry_safe(pos, n, head, member) 		\
	for (pos = hlist_entry_safe((head)->first, typeof(*pos), member);\
	     pos && ({ n = pos->member.next; 1; });			\
	     pos = hlist_entry_safe(n, typeof(*pos), member))

#define list_first_entry(ptr, type, member) \
	list_entry((ptr)->next, type, member)

#define list_last_entry(ptr, type, member) \
	list_entry((ptr)->prev, type, member)

#define list_first_entry_or_null(ptr, type, member) \
	(!list_empty(ptr) ? list_first_entry(ptr, type, member) : NULL)

#define list_next_entry(pos, member) \
	list_entry((pos)->member.next, typeof(*(pos)), member)

#define list_prev_entry(pos, member) \
	list_entry((pos)->member.prev, typeof(*(pos)), member)

#define list_for_each(pos, head) \
	for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, n, head)				   \
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)

#define list_for_each_entry(pos, head, member)				\
	for (pos = list_entry((head)->next, typeof(*pos), member);	\
	     &pos->member != (head); 	\
	     pos = list_entry(pos->member.next, typeof(*pos), member))

#define list_for_each_entry_reverse(pos, head, member)			\
	for (pos = list_last_entry(head, typeof(*pos), member);		\
	     &pos->member != (head); 					\
	     pos = list_prev_entry(pos, member))

#define list_for_each_entry_continue(pos, head, member) 		\
	for (pos = list_next_entry(pos, member);			\
	     &pos->member != (head);					\
	     pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, n, head, member)			\
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		n = list_entry(pos->member.next, typeof(*pos), member);	\
	     &pos->member != (head); 					\
	     pos = n, n = list_entry(n->member.next, typeof(*n), member))

static inline void INIT_LIST_HEAD(list_head_t *list)
{
	list->next = list;
	list->prev = list;
}

static inline void __list_add(list_head_t *new, list_head_t *prev, list_head_t *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void list_add(list_head_t *new, list_head_t *head)
{
	__list_add(new, head, head->next);
}
static inline void list_add_tail(list_head_t * new, list_head_t * head)
{
	__list_add(new, head->prev, head);
}
static inline void __list_del(list_head_t * prev, list_head_t * next)
{
	next->prev = prev;
	prev->next = next;
}
static inline void __list_del_entry(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
}

#define LIST_POISON1 0
#define LIST_POISON2 0
static inline void list_del(list_head_t *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = LIST_POISON1;
	entry->prev = LIST_POISON2;
}
static inline void list_replace(list_head_t *old,
				list_head_t *new)
{
	new->next = old->next;
	new->next->prev = new;
	new->prev = old->prev;
	new->prev->next = new;
}

static inline void list_replace_init(list_head_t *old,
					list_head_t *new)
{
	list_replace(old, new);
	INIT_LIST_HEAD(old);
}

static inline void list_del_init(list_head_t *entry)
{
	__list_del_entry(entry);
	INIT_LIST_HEAD(entry);
}
static inline void list_move(list_head_t *list, list_head_t *head)
{
	__list_del_entry(list);
	list_add(list, head);
}
static inline void list_move_tail(list_head_t *list,
				  list_head_t *head)
{
	__list_del_entry(list);
	list_add_tail(list, head);
}
static inline int list_is_last(const list_head_t *list,
				const list_head_t *head)
{
	return list->next == head;
}
static inline int list_empty(const list_head_t *head)
{
	return head->next == head;
}
static inline int list_empty_careful(const list_head_t *head)
{
	list_head_t *next = head->next;
	return (next == head) && (next == head->prev);
}
static inline void list_rotate_left(list_head_t *head)
{
	list_head_t *first;

	if (!list_empty(head)) {
		first = head->next;
		list_move_tail(first, head);
	}
}
static inline int list_is_singular(const list_head_t *head)
{
	return !list_empty(head) && (head->next == head->prev);
}
static inline void __list_cut_position(list_head_t *list,
		list_head_t *head, list_head_t *entry)
{
	list_head_t *new_first = entry->next;
	list->next = head->next;
	list->next->prev = list;
	list->prev = entry;
	entry->next = list;
	head->next = new_first;
	new_first->prev = head;
}
static inline void list_cut_position(list_head_t *list,
		list_head_t *head, list_head_t *entry)
{
	if (list_empty(head))
		return;
	if (list_is_singular(head) &&
		(head->next != entry && head != entry))
		return;
	if (entry == head)
		INIT_LIST_HEAD(list);
	else
		__list_cut_position(list, head, entry);
}

static inline void __list_splice(const list_head_t *list,
				 list_head_t *prev,
				 list_head_t *next)
{
	list_head_t *first = list->next;
	list_head_t *last = list->prev;

	first->prev = prev;
	prev->next = first;

	last->next = next;
	next->prev = last;
}

/**
 * list_splice - join two lists, this is designed for stacks
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(const struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head, head->next);
}

/**
 * list_splice_tail - join two lists, each list being a queue
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice_tail(struct list_head *list,
				struct list_head *head)
{
	if (!list_empty(list))
		__list_splice(list, head->prev, head);
}

/*
 * Double linked lists with a single pointer list head.
 * Mostly useful for hash tables where the two pointer list head is
 * too wasteful.
 * You lose the ability to access the tail in O(1).
 */

#define HLIST_HEAD_INIT { .first = NULL }
#define HLIST_HEAD(name) struct hlist_head name = {  .first = NULL }
#define INIT_HLIST_HEAD(ptr) ((ptr)->first = NULL)
static inline void INIT_HLIST_NODE(struct hlist_node *h)
{
	h->next = NULL;
	h->pprev = NULL;
}
static inline int hlist_unhashed(const struct hlist_node *h)
{
	return !h->pprev;
}
static inline int hlist_empty(const struct hlist_head *h)
{
	return !h->first;
}
static inline void __hlist_del(struct hlist_node *n)
{
	struct hlist_node *next = n->next;
	struct hlist_node **pprev = n->pprev;
	*pprev = next;
	if (next)
		next->pprev = pprev;
}

static inline void hlist_del_init(struct hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		__hlist_del(n);
		INIT_HLIST_NODE(n);
	}
}

static inline void hlist_del_init_rcu(struct hlist_node *n)
{
	if (!hlist_unhashed(n)) {
		__hlist_del(n);
		n->pprev = NULL;
	}
}

static inline void hlist_add_after(struct hlist_node *n,
					struct hlist_node *next)
{
	next->next = n->next;
	n->next = next;
	next->pprev = &n->next;

	if(next->next)
		next->next->pprev  = &next->next;
}
static inline void hlist_add_head(struct hlist_node *n, struct hlist_head *h)
{
	struct hlist_node *first = h->first;
	n->next = first;
	if (first)
		first->pprev = &n->next;
	h->first = n;
	n->pprev = &h->first;
}

static inline void hlist_add_behind(struct hlist_node *n,
				    struct hlist_node *prev)
{
	n->next = prev->next;
	prev->next = n;
	n->pprev = &prev->next;

	if (n->next)
		n->next->pprev = &n->next;
}

//TODO. 
#define hlist_for_each_entry_rcu hlist_for_each_entry
#define hlist_add_after_rcu hlist_add_after
#define hlist_add_head_rcu hlist_add_head
#define hlist_add_behind_rcu hlist_add_behind

void list_sort(void *priv, struct list_head *head,
                int (*cmp)(void *priv, struct list_head *a,
						   struct list_head *b));

#endif

#if defined(__QNXNTO__) && defined(__USESRCVERSION)
#include <sys/srcversion.h>
__SRCVERSION("$URL: http://svn.ott.qnx.com/product/graphics/branches/denso-jp-hv-ivi/hardware/capture/intel-ipu4/driver/intel-ipu4-drv/include/linux/list.h $ $Rev: 838597 $")
#endif
