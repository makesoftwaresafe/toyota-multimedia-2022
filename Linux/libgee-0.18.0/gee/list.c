/* list.c generated by valac 0.27.1.26-9b1a5, the Vala compiler
 * generated from list.vala, do not modify */

/* list.vala
 *
 * Copyright (C) 2007  Jürg Billeter
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
 *
 * Author:
 * 	Jürg Billeter <j@bitron.ch>
 */

#include <glib.h>
#include <glib-object.h>


#define GEE_TYPE_TRAVERSABLE (gee_traversable_get_type ())
#define GEE_TRAVERSABLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_TRAVERSABLE, GeeTraversable))
#define GEE_IS_TRAVERSABLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_TRAVERSABLE))
#define GEE_TRAVERSABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_TRAVERSABLE, GeeTraversableIface))

typedef struct _GeeTraversable GeeTraversable;
typedef struct _GeeTraversableIface GeeTraversableIface;

#define GEE_TRAVERSABLE_TYPE_STREAM (gee_traversable_stream_get_type ())

#define GEE_TYPE_LAZY (gee_lazy_get_type ())
#define GEE_LAZY(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_LAZY, GeeLazy))
#define GEE_LAZY_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_LAZY, GeeLazyClass))
#define GEE_IS_LAZY(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_LAZY))
#define GEE_IS_LAZY_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_LAZY))
#define GEE_LAZY_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_LAZY, GeeLazyClass))

typedef struct _GeeLazy GeeLazy;
typedef struct _GeeLazyClass GeeLazyClass;

#define GEE_TYPE_ITERATOR (gee_iterator_get_type ())
#define GEE_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_ITERATOR, GeeIterator))
#define GEE_IS_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_ITERATOR))
#define GEE_ITERATOR_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_ITERATOR, GeeIteratorIface))

typedef struct _GeeIterator GeeIterator;
typedef struct _GeeIteratorIface GeeIteratorIface;

#define GEE_TYPE_ITERABLE (gee_iterable_get_type ())
#define GEE_ITERABLE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_ITERABLE, GeeIterable))
#define GEE_IS_ITERABLE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_ITERABLE))
#define GEE_ITERABLE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_ITERABLE, GeeIterableIface))

typedef struct _GeeIterable GeeIterable;
typedef struct _GeeIterableIface GeeIterableIface;

#define GEE_TYPE_COLLECTION (gee_collection_get_type ())
#define GEE_COLLECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_COLLECTION, GeeCollection))
#define GEE_IS_COLLECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_COLLECTION))
#define GEE_COLLECTION_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_COLLECTION, GeeCollectionIface))

typedef struct _GeeCollection GeeCollection;
typedef struct _GeeCollectionIface GeeCollectionIface;

#define GEE_TYPE_LIST (gee_list_get_type ())
#define GEE_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_LIST, GeeList))
#define GEE_IS_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_LIST))
#define GEE_LIST_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_LIST, GeeListIface))

typedef struct _GeeList GeeList;
typedef struct _GeeListIface GeeListIface;

#define GEE_TYPE_LIST_ITERATOR (gee_list_iterator_get_type ())
#define GEE_LIST_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_LIST_ITERATOR, GeeListIterator))
#define GEE_IS_LIST_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_LIST_ITERATOR))
#define GEE_LIST_ITERATOR_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_LIST_ITERATOR, GeeListIteratorIface))

typedef struct _GeeListIterator GeeListIterator;
typedef struct _GeeListIteratorIface GeeListIteratorIface;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))

#define GEE_TYPE_ABSTRACT_COLLECTION (gee_abstract_collection_get_type ())
#define GEE_ABSTRACT_COLLECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_ABSTRACT_COLLECTION, GeeAbstractCollection))
#define GEE_ABSTRACT_COLLECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_ABSTRACT_COLLECTION, GeeAbstractCollectionClass))
#define GEE_IS_ABSTRACT_COLLECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_ABSTRACT_COLLECTION))
#define GEE_IS_ABSTRACT_COLLECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_ABSTRACT_COLLECTION))
#define GEE_ABSTRACT_COLLECTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_ABSTRACT_COLLECTION, GeeAbstractCollectionClass))

typedef struct _GeeAbstractCollection GeeAbstractCollection;
typedef struct _GeeAbstractCollectionClass GeeAbstractCollectionClass;

#define GEE_TYPE_ABSTRACT_LIST (gee_abstract_list_get_type ())
#define GEE_ABSTRACT_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_ABSTRACT_LIST, GeeAbstractList))
#define GEE_ABSTRACT_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_ABSTRACT_LIST, GeeAbstractListClass))
#define GEE_IS_ABSTRACT_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_ABSTRACT_LIST))
#define GEE_IS_ABSTRACT_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_ABSTRACT_LIST))
#define GEE_ABSTRACT_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_ABSTRACT_LIST, GeeAbstractListClass))

typedef struct _GeeAbstractList GeeAbstractList;
typedef struct _GeeAbstractListClass GeeAbstractListClass;

#define GEE_TYPE_ABSTRACT_BIDIR_LIST (gee_abstract_bidir_list_get_type ())
#define GEE_ABSTRACT_BIDIR_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_ABSTRACT_BIDIR_LIST, GeeAbstractBidirList))
#define GEE_ABSTRACT_BIDIR_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_ABSTRACT_BIDIR_LIST, GeeAbstractBidirListClass))
#define GEE_IS_ABSTRACT_BIDIR_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_ABSTRACT_BIDIR_LIST))
#define GEE_IS_ABSTRACT_BIDIR_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_ABSTRACT_BIDIR_LIST))
#define GEE_ABSTRACT_BIDIR_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_ABSTRACT_BIDIR_LIST, GeeAbstractBidirListClass))

typedef struct _GeeAbstractBidirList GeeAbstractBidirList;
typedef struct _GeeAbstractBidirListClass GeeAbstractBidirListClass;

#define GEE_TYPE_LINKED_LIST (gee_linked_list_get_type ())
#define GEE_LINKED_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_LINKED_LIST, GeeLinkedList))
#define GEE_LINKED_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_LINKED_LIST, GeeLinkedListClass))
#define GEE_IS_LINKED_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_LINKED_LIST))
#define GEE_IS_LINKED_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_LINKED_LIST))
#define GEE_LINKED_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_LINKED_LIST, GeeLinkedListClass))

typedef struct _GeeLinkedList GeeLinkedList;
typedef struct _GeeLinkedListClass GeeLinkedListClass;

#define GEE_TYPE_BIDIR_LIST (gee_bidir_list_get_type ())
#define GEE_BIDIR_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_BIDIR_LIST, GeeBidirList))
#define GEE_IS_BIDIR_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_BIDIR_LIST))
#define GEE_BIDIR_LIST_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_BIDIR_LIST, GeeBidirListIface))

typedef struct _GeeBidirList GeeBidirList;
typedef struct _GeeBidirListIface GeeBidirListIface;

#define GEE_TYPE_BIDIR_ITERATOR (gee_bidir_iterator_get_type ())
#define GEE_BIDIR_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_BIDIR_ITERATOR, GeeBidirIterator))
#define GEE_IS_BIDIR_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_BIDIR_ITERATOR))
#define GEE_BIDIR_ITERATOR_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_BIDIR_ITERATOR, GeeBidirIteratorIface))

typedef struct _GeeBidirIterator GeeBidirIterator;
typedef struct _GeeBidirIteratorIface GeeBidirIteratorIface;

#define GEE_TYPE_BIDIR_LIST_ITERATOR (gee_bidir_list_iterator_get_type ())
#define GEE_BIDIR_LIST_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_BIDIR_LIST_ITERATOR, GeeBidirListIterator))
#define GEE_IS_BIDIR_LIST_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_BIDIR_LIST_ITERATOR))
#define GEE_BIDIR_LIST_ITERATOR_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_BIDIR_LIST_ITERATOR, GeeBidirListIteratorIface))

typedef struct _GeeBidirListIterator GeeBidirListIterator;
typedef struct _GeeBidirListIteratorIface GeeBidirListIteratorIface;

typedef gboolean (*GeeForallFunc) (gpointer g, void* user_data);
typedef enum  {
	GEE_TRAVERSABLE_STREAM_YIELD,
	GEE_TRAVERSABLE_STREAM_CONTINUE,
	GEE_TRAVERSABLE_STREAM_END,
	GEE_TRAVERSABLE_STREAM_WAIT
} GeeTraversableStream;

typedef GeeTraversableStream (*GeeStreamFunc) (GeeTraversableStream state, GeeLazy* g, GeeLazy** lazy, void* user_data);
struct _GeeIteratorIface {
	GTypeInterface parent_iface;
	gboolean (*next) (GeeIterator* self);
	gboolean (*has_next) (GeeIterator* self);
	gpointer (*get) (GeeIterator* self);
	void (*remove) (GeeIterator* self);
	gboolean (*get_valid) (GeeIterator* self);
	gboolean (*get_read_only) (GeeIterator* self);
};

typedef gpointer (*GeeFoldFunc) (gpointer g, gpointer a, void* user_data);
typedef gpointer (*GeeMapFunc) (gpointer g, void* user_data);
typedef gboolean (*GeePredicate) (gconstpointer g, void* user_data);
typedef GeeIterator* (*GeeFlatMapFunc) (gpointer g, void* user_data);
struct _GeeTraversableIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeTraversable* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeTraversable* self);
	GDestroyNotify (*get_g_destroy_func) (GeeTraversable* self);
	gboolean (*foreach) (GeeTraversable* self, GeeForallFunc f, void* f_target);
	GeeIterator* (*stream) (GeeTraversable* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeStreamFunc f, void* f_target, GDestroyNotify f_target_destroy_notify);
	gpointer (*fold) (GeeTraversable* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldFunc f, void* f_target, gpointer seed);
	GeeIterator* (*map) (GeeTraversable* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeMapFunc f, void* f_target);
	GeeIterator* (*scan) (GeeTraversable* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldFunc f, void* f_target, gpointer seed);
	GeeIterator* (*filter) (GeeTraversable* self, GeePredicate pred, void* pred_target, GDestroyNotify pred_target_destroy_notify);
	GeeIterator* (*chop) (GeeTraversable* self, gint offset, gint length);
	GType (*get_element_type) (GeeTraversable* self);
	GeeIterator* (*flat_map) (GeeTraversable* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFlatMapFunc f, void* f_target, GDestroyNotify f_target_destroy_notify);
	GeeIterator** (*tee) (GeeTraversable* self, guint forks, int* result_length1);
};

struct _GeeIterableIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeIterable* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeIterable* self);
	GDestroyNotify (*get_g_destroy_func) (GeeIterable* self);
	GeeIterator* (*iterator) (GeeIterable* self);
};

struct _GeeCollectionIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeCollection* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeCollection* self);
	GDestroyNotify (*get_g_destroy_func) (GeeCollection* self);
	gboolean (*contains) (GeeCollection* self, gconstpointer item);
	gboolean (*add) (GeeCollection* self, gconstpointer item);
	gboolean (*remove) (GeeCollection* self, gconstpointer item);
	void (*clear) (GeeCollection* self);
	gboolean (*add_all) (GeeCollection* self, GeeCollection* collection);
	gboolean (*contains_all) (GeeCollection* self, GeeCollection* collection);
	gboolean (*remove_all) (GeeCollection* self, GeeCollection* collection);
	gboolean (*retain_all) (GeeCollection* self, GeeCollection* collection);
	gpointer* (*to_array) (GeeCollection* self, int* result_length1);
	gint (*get_size) (GeeCollection* self);
	gboolean (*get_is_empty) (GeeCollection* self);
	gboolean (*get_read_only) (GeeCollection* self);
	GeeCollection* (*get_read_only_view) (GeeCollection* self);
	gboolean (*add_all_array) (GeeCollection* self, gpointer* array, int array_length1);
	gboolean (*contains_all_array) (GeeCollection* self, gpointer* array, int array_length1);
	gboolean (*remove_all_array) (GeeCollection* self, gpointer* array, int array_length1);
	gboolean (*add_all_iterator) (GeeCollection* self, GeeIterator* iter);
	gboolean (*contains_all_iterator) (GeeCollection* self, GeeIterator* iter);
	gboolean (*remove_all_iterator) (GeeCollection* self, GeeIterator* iter);
};

struct _GeeListIteratorIface {
	GTypeInterface parent_iface;
	void (*set) (GeeListIterator* self, gconstpointer item);
	void (*add) (GeeListIterator* self, gconstpointer item);
	gint (*index) (GeeListIterator* self);
};

struct _GeeListIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeList* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeList* self);
	GDestroyNotify (*get_g_destroy_func) (GeeList* self);
	GeeListIterator* (*list_iterator) (GeeList* self);
	gpointer (*get) (GeeList* self, gint index);
	void (*set) (GeeList* self, gint index, gconstpointer item);
	gint (*index_of) (GeeList* self, gconstpointer item);
	void (*insert) (GeeList* self, gint index, gconstpointer item);
	gpointer (*remove_at) (GeeList* self, gint index);
	GeeList* (*slice) (GeeList* self, gint start, gint stop);
	gpointer (*first) (GeeList* self);
	gpointer (*last) (GeeList* self);
	void (*insert_all) (GeeList* self, gint index, GeeCollection* collection);
	void (*sort) (GeeList* self, GCompareDataFunc compare_func, void* compare_func_target, GDestroyNotify compare_func_target_destroy_notify);
	GeeList* (*get_read_only_view) (GeeList* self);
};

typedef gboolean (*GeeEqualDataFunc) (gconstpointer a, gconstpointer b, void* user_data);
struct _GeeBidirIteratorIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeBidirIterator* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeBidirIterator* self);
	GDestroyNotify (*get_g_destroy_func) (GeeBidirIterator* self);
	gboolean (*previous) (GeeBidirIterator* self);
	gboolean (*has_previous) (GeeBidirIterator* self);
	gboolean (*first) (GeeBidirIterator* self);
	gboolean (*last) (GeeBidirIterator* self);
};

struct _GeeBidirListIteratorIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeBidirListIterator* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeBidirListIterator* self);
	GDestroyNotify (*get_g_destroy_func) (GeeBidirListIterator* self);
	void (*insert) (GeeBidirListIterator* self, gconstpointer item);
};

struct _GeeBidirListIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeBidirList* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeBidirList* self);
	GDestroyNotify (*get_g_destroy_func) (GeeBidirList* self);
	GeeBidirListIterator* (*bidir_list_iterator) (GeeBidirList* self);
	GeeBidirList* (*get_read_only_view) (GeeBidirList* self);
};



GType gee_traversable_stream_get_type (void) G_GNUC_CONST;
gpointer gee_lazy_ref (gpointer instance);
void gee_lazy_unref (gpointer instance);
GParamSpec* gee_param_spec_lazy (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void gee_value_set_lazy (GValue* value, gpointer v_object);
void gee_value_take_lazy (GValue* value, gpointer v_object);
gpointer gee_value_get_lazy (const GValue* value);
GType gee_lazy_get_type (void) G_GNUC_CONST;
GType gee_iterator_get_type (void) G_GNUC_CONST;
GType gee_traversable_get_type (void) G_GNUC_CONST;
GType gee_iterable_get_type (void) G_GNUC_CONST;
GType gee_collection_get_type (void) G_GNUC_CONST;
GType gee_list_iterator_get_type (void) G_GNUC_CONST;
GType gee_list_get_type (void) G_GNUC_CONST;
GeeListIterator* gee_list_list_iterator (GeeList* self);
gpointer gee_list_get (GeeList* self, gint index);
void gee_list_set (GeeList* self, gint index, gconstpointer item);
gint gee_list_index_of (GeeList* self, gconstpointer item);
void gee_list_insert (GeeList* self, gint index, gconstpointer item);
gpointer gee_list_remove_at (GeeList* self, gint index);
GeeList* gee_list_slice (GeeList* self, gint start, gint stop);
gpointer gee_list_first (GeeList* self);
static gpointer gee_list_real_first (GeeList* self);
gpointer gee_list_last (GeeList* self);
static gpointer gee_list_real_last (GeeList* self);
gint gee_collection_get_size (GeeCollection* self);
void gee_list_insert_all (GeeList* self, gint index, GeeCollection* collection);
static void gee_list_real_insert_all (GeeList* self, gint index, GeeCollection* collection);
GeeIterator* gee_iterable_iterator (GeeIterable* self);
gboolean gee_iterator_next (GeeIterator* self);
gpointer gee_iterator_get (GeeIterator* self);
void gee_list_sort (GeeList* self, GCompareDataFunc compare_func, void* compare_func_target, GDestroyNotify compare_func_target_destroy_notify);
static void gee_list_real_sort (GeeList* self, GCompareDataFunc compare_func, void* compare_func_target, GDestroyNotify compare_func_target_destroy_notify);
GCompareDataFunc gee_functions_get_compare_func_for (GType t, void** result_target, GDestroyNotify* result_target_destroy_notify);
G_GNUC_INTERNAL void gee_tim_sort_sort (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeList* list, GCompareDataFunc compare, void* compare_target);
GeeList* gee_list_empty (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func);
GeeLinkedList* gee_linked_list_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeEqualDataFunc equal_func, void* equal_func_target, GDestroyNotify equal_func_target_destroy_notify);
GeeLinkedList* gee_linked_list_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeEqualDataFunc equal_func, void* equal_func_target, GDestroyNotify equal_func_target_destroy_notify);
GType gee_abstract_collection_get_type (void) G_GNUC_CONST;
GType gee_abstract_list_get_type (void) G_GNUC_CONST;
GType gee_abstract_bidir_list_get_type (void) G_GNUC_CONST;
GType gee_linked_list_get_type (void) G_GNUC_CONST;
GType gee_bidir_iterator_get_type (void) G_GNUC_CONST;
GType gee_bidir_list_iterator_get_type (void) G_GNUC_CONST;
GType gee_bidir_list_get_type (void) G_GNUC_CONST;
GeeBidirList* gee_abstract_bidir_list_get_read_only_view (GeeAbstractBidirList* self);
GeeList* gee_list_get_read_only_view (GeeList* self);


/**
 * Returns a ListIterator that can be used for iteration over this list.
 *
 * @return a ListIterator that can be used for iteration over this list
 */
GeeListIterator* gee_list_list_iterator (GeeList* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->list_iterator (self);
}


/**
 * Returns the item at the specified index in this list.
 *
 * @param index zero-based index of the item to be returned
 *
 * @return      the item at the specified index in the list
 */
gpointer gee_list_get (GeeList* self, gint index) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->get (self, index);
}


/**
 * Sets the item at the specified index in this list.
 *
 * @param index zero-based index of the item to be set
 */
void gee_list_set (GeeList* self, gint index, gconstpointer item) {
	g_return_if_fail (self != NULL);
	GEE_LIST_GET_INTERFACE (self)->set (self, index, item);
}


/**
 * Returns the index of the first occurence of the specified item in
 * this list.
 *
 * @return the index of the first occurence of the specified item, or
 *         -1 if the item could not be found
 */
gint gee_list_index_of (GeeList* self, gconstpointer item) {
	g_return_val_if_fail (self != NULL, 0);
	return GEE_LIST_GET_INTERFACE (self)->index_of (self, item);
}


/**
 * Inserts an item into this list at the specified position.
 *
 * @param index zero-based index at which item is inserted
 * @param item  item to insert into the list
 */
void gee_list_insert (GeeList* self, gint index, gconstpointer item) {
	g_return_if_fail (self != NULL);
	GEE_LIST_GET_INTERFACE (self)->insert (self, index, item);
}


/**
 * Removes the item at the specified index of this list.
 *
 * @param index zero-based index of the item to be removed
 *
 * @return      the removed element
 */
gpointer gee_list_remove_at (GeeList* self, gint index) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->remove_at (self, index);
}


/**
 * Returns a slice of this list.
 *
 * @param start zero-based index of the begin of the slice
 * @param stop  zero-based index after the end of the slice
 *
 * @return A list containing a slice of this list
 */
GeeList* gee_list_slice (GeeList* self, gint start, gint stop) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->slice (self, start, stop);
}


/**
 * Returns the first item of the list. Fails if the list is empty.
 *
 * @return      first item in the list
 */
static gpointer gee_list_real_first (GeeList* self) {
	gpointer result = NULL;
	gpointer _tmp0_ = NULL;
	_tmp0_ = gee_list_get (self, 0);
	result = _tmp0_;
	return result;
}


gpointer gee_list_first (GeeList* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->first (self);
}


/**
 * Returns the last item of the list. Fails if the list is empty.
 *
 * @return      last item in the list
 */
static gpointer gee_list_real_last (GeeList* self) {
	gpointer result = NULL;
	gint _tmp0_ = 0;
	gint _tmp1_ = 0;
	gpointer _tmp2_ = NULL;
	_tmp0_ = gee_collection_get_size ((GeeCollection*) self);
	_tmp1_ = _tmp0_;
	_tmp2_ = gee_list_get (self, _tmp1_ - 1);
	result = _tmp2_;
	return result;
}


gpointer gee_list_last (GeeList* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->last (self);
}


/**
 * Inserts items into this list for the input collection at the
 * specified position.
 *
 * @param index zero-based index of the items to be inserted
 * @param collection collection of items to be inserted
 */
static void gee_list_real_insert_all (GeeList* self, gint index, GeeCollection* collection) {
	g_return_if_fail (collection != NULL);
	{
		GeeIterator* _item_it = NULL;
		GeeCollection* _tmp0_ = NULL;
		GeeIterator* _tmp1_ = NULL;
		_tmp0_ = collection;
		_tmp1_ = gee_iterable_iterator ((GeeIterable*) _tmp0_);
		_item_it = _tmp1_;
		while (TRUE) {
			GeeIterator* _tmp2_ = NULL;
			gboolean _tmp3_ = FALSE;
			gpointer item = NULL;
			GeeIterator* _tmp4_ = NULL;
			gpointer _tmp5_ = NULL;
			gint _tmp6_ = 0;
			gconstpointer _tmp7_ = NULL;
			gint _tmp8_ = 0;
			_tmp2_ = _item_it;
			_tmp3_ = gee_iterator_next (_tmp2_);
			if (!_tmp3_) {
				break;
			}
			_tmp4_ = _item_it;
			_tmp5_ = gee_iterator_get (_tmp4_);
			item = _tmp5_;
			_tmp6_ = index;
			_tmp7_ = item;
			gee_list_insert (self, _tmp6_, _tmp7_);
			_tmp8_ = index;
			index = _tmp8_ + 1;
			((item == NULL) || (GEE_LIST_GET_INTERFACE (self)->get_g_destroy_func (self) == NULL)) ? NULL : (item = (GEE_LIST_GET_INTERFACE (self)->get_g_destroy_func (self) (item), NULL));
		}
		_g_object_unref0 (_item_it);
	}
}


void gee_list_insert_all (GeeList* self, gint index, GeeCollection* collection) {
	g_return_if_fail (self != NULL);
	GEE_LIST_GET_INTERFACE (self)->insert_all (self, index, collection);
}


/**
 * Sorts items by comparing with the specified compare function.
 *
 * @param compare_func compare function to use to compare items
 */
static void gee_list_real_sort (GeeList* self, GCompareDataFunc compare_func, void* compare_func_target, GDestroyNotify compare_func_target_destroy_notify) {
	GCompareDataFunc _tmp0_ = NULL;
	void* _tmp0__target = NULL;
	GCompareDataFunc _tmp4_ = NULL;
	void* _tmp4__target = NULL;
	_tmp0_ = compare_func;
	_tmp0__target = compare_func_target;
	if (_tmp0_ == NULL) {
		void* _tmp1_ = NULL;
		GDestroyNotify _tmp2_ = NULL;
		GCompareDataFunc _tmp3_ = NULL;
		_tmp3_ = gee_functions_get_compare_func_for (GEE_LIST_GET_INTERFACE (self)->get_g_type (self), &_tmp1_, &_tmp2_);
		(compare_func_target_destroy_notify == NULL) ? NULL : (compare_func_target_destroy_notify (compare_func_target), NULL);
		compare_func = NULL;
		compare_func_target = NULL;
		compare_func_target_destroy_notify = NULL;
		compare_func = _tmp3_;
		compare_func_target = _tmp1_;
		compare_func_target_destroy_notify = _tmp2_;
	}
	_tmp4_ = compare_func;
	_tmp4__target = compare_func_target;
	gee_tim_sort_sort (GEE_LIST_GET_INTERFACE (self)->get_g_type (self), (GBoxedCopyFunc) GEE_LIST_GET_INTERFACE (self)->get_g_dup_func (self), GEE_LIST_GET_INTERFACE (self)->get_g_destroy_func (self), self, _tmp4_, _tmp4__target);
	(compare_func_target_destroy_notify == NULL) ? NULL : (compare_func_target_destroy_notify (compare_func_target), NULL);
	compare_func = NULL;
	compare_func_target = NULL;
	compare_func_target_destroy_notify = NULL;
}


void gee_list_sort (GeeList* self, GCompareDataFunc compare_func, void* compare_func_target, GDestroyNotify compare_func_target_destroy_notify) {
	g_return_if_fail (self != NULL);
	GEE_LIST_GET_INTERFACE (self)->sort (self, compare_func, compare_func_target, compare_func_target_destroy_notify);
}


/**
 * Returns an immutable empty list.
 *
 * @return an immutable empty list
 */
GeeList* gee_list_empty (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func) {
	GeeList* result = NULL;
	GeeLinkedList* _tmp0_ = NULL;
	GeeLinkedList* _tmp1_ = NULL;
	GeeBidirList* _tmp2_ = NULL;
	GeeBidirList* _tmp3_ = NULL;
	GeeList* _tmp4_ = NULL;
	_tmp0_ = gee_linked_list_new (g_type, (GBoxedCopyFunc) g_dup_func, g_destroy_func, NULL, NULL, NULL);
	_tmp1_ = _tmp0_;
	_tmp2_ = gee_abstract_bidir_list_get_read_only_view ((GeeAbstractBidirList*) _tmp1_);
	_tmp3_ = _tmp2_;
	_tmp4_ = (GeeList*) _tmp3_;
	_g_object_unref0 (_tmp1_);
	result = _tmp4_;
	return result;
}


GeeList* gee_list_get_read_only_view (GeeList* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_LIST_GET_INTERFACE (self)->get_read_only_view (self);
}


static void gee_list_base_init (GeeListIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
		/**
		 * The read-only view of this list.
		 */
		g_object_interface_install_property (iface, g_param_spec_object ("read-only-view", "read-only-view", "read-only-view", GEE_TYPE_LIST, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
		iface->first = gee_list_real_first;
		iface->last = gee_list_real_last;
		iface->insert_all = gee_list_real_insert_all;
		iface->sort = gee_list_real_sort;
	}
}


/**
 * An ordered collection.
 */
GType gee_list_get_type (void) {
	static volatile gsize gee_list_type_id__volatile = 0;
	if (g_once_init_enter (&gee_list_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (GeeListIface), (GBaseInitFunc) gee_list_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		GType gee_list_type_id;
		gee_list_type_id = g_type_register_static (G_TYPE_INTERFACE, "GeeList", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gee_list_type_id, GEE_TYPE_COLLECTION);
		g_once_init_leave (&gee_list_type_id__volatile, gee_list_type_id);
	}
	return gee_list_type_id__volatile;
}



