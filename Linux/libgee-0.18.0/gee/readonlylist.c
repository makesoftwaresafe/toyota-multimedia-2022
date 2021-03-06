/* readonlylist.c generated by valac 0.27.1.26-9b1a5, the Vala compiler
 * generated from readonlylist.vala, do not modify */

/* readonlylist.vala
 *
 * Copyright (C) 2007-2008  Jürg Billeter
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

#define GEE_TYPE_READ_ONLY_COLLECTION (gee_read_only_collection_get_type ())
#define GEE_READ_ONLY_COLLECTION(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_READ_ONLY_COLLECTION, GeeReadOnlyCollection))
#define GEE_READ_ONLY_COLLECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_READ_ONLY_COLLECTION, GeeReadOnlyCollectionClass))
#define GEE_IS_READ_ONLY_COLLECTION(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_READ_ONLY_COLLECTION))
#define GEE_IS_READ_ONLY_COLLECTION_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_READ_ONLY_COLLECTION))
#define GEE_READ_ONLY_COLLECTION_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_READ_ONLY_COLLECTION, GeeReadOnlyCollectionClass))

typedef struct _GeeReadOnlyCollection GeeReadOnlyCollection;
typedef struct _GeeReadOnlyCollectionClass GeeReadOnlyCollectionClass;
typedef struct _GeeReadOnlyCollectionPrivate GeeReadOnlyCollectionPrivate;

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

#define GEE_TYPE_READ_ONLY_LIST (gee_read_only_list_get_type ())
#define GEE_READ_ONLY_LIST(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_READ_ONLY_LIST, GeeReadOnlyList))
#define GEE_READ_ONLY_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_READ_ONLY_LIST, GeeReadOnlyListClass))
#define GEE_IS_READ_ONLY_LIST(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_READ_ONLY_LIST))
#define GEE_IS_READ_ONLY_LIST_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_READ_ONLY_LIST))
#define GEE_READ_ONLY_LIST_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_READ_ONLY_LIST, GeeReadOnlyListClass))

typedef struct _GeeReadOnlyList GeeReadOnlyList;
typedef struct _GeeReadOnlyListClass GeeReadOnlyListClass;
typedef struct _GeeReadOnlyListPrivate GeeReadOnlyListPrivate;

#define GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR (gee_read_only_collection_iterator_get_type ())
#define GEE_READ_ONLY_COLLECTION_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR, GeeReadOnlyCollectionIterator))
#define GEE_READ_ONLY_COLLECTION_ITERATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR, GeeReadOnlyCollectionIteratorClass))
#define GEE_READ_ONLY_COLLECTION_IS_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR))
#define GEE_READ_ONLY_COLLECTION_IS_ITERATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR))
#define GEE_READ_ONLY_COLLECTION_ITERATOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR, GeeReadOnlyCollectionIteratorClass))

typedef struct _GeeReadOnlyCollectionIterator GeeReadOnlyCollectionIterator;
typedef struct _GeeReadOnlyCollectionIteratorClass GeeReadOnlyCollectionIteratorClass;

#define GEE_READ_ONLY_LIST_TYPE_ITERATOR (gee_read_only_list_iterator_get_type ())
#define GEE_READ_ONLY_LIST_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_READ_ONLY_LIST_TYPE_ITERATOR, GeeReadOnlyListIterator))
#define GEE_READ_ONLY_LIST_ITERATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_READ_ONLY_LIST_TYPE_ITERATOR, GeeReadOnlyListIteratorClass))
#define GEE_READ_ONLY_LIST_IS_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_READ_ONLY_LIST_TYPE_ITERATOR))
#define GEE_READ_ONLY_LIST_IS_ITERATOR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_READ_ONLY_LIST_TYPE_ITERATOR))
#define GEE_READ_ONLY_LIST_ITERATOR_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_READ_ONLY_LIST_TYPE_ITERATOR, GeeReadOnlyListIteratorClass))

typedef struct _GeeReadOnlyListIterator GeeReadOnlyListIterator;
typedef struct _GeeReadOnlyListIteratorClass GeeReadOnlyListIteratorClass;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
typedef struct _GeeReadOnlyCollectionIteratorPrivate GeeReadOnlyCollectionIteratorPrivate;
typedef struct _GeeReadOnlyListIteratorPrivate GeeReadOnlyListIteratorPrivate;

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

struct _GeeReadOnlyCollection {
	GObject parent_instance;
	GeeReadOnlyCollectionPrivate * priv;
	GeeCollection* _collection;
};

struct _GeeReadOnlyCollectionClass {
	GObjectClass parent_class;
	GeeCollection* (*get_read_only_view) (GeeReadOnlyCollection* self);
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

struct _GeeReadOnlyList {
	GeeReadOnlyCollection parent_instance;
	GeeReadOnlyListPrivate * priv;
};

struct _GeeReadOnlyListClass {
	GeeReadOnlyCollectionClass parent_class;
	GeeList* (*get_read_only_view) (GeeReadOnlyList* self);
};

struct _GeeReadOnlyListPrivate {
	GType g_type;
	GBoxedCopyFunc g_dup_func;
	GDestroyNotify g_destroy_func;
};

struct _GeeReadOnlyCollectionIterator {
	GObject parent_instance;
	GeeReadOnlyCollectionIteratorPrivate * priv;
	GeeIterator* _iter;
};

struct _GeeReadOnlyCollectionIteratorClass {
	GObjectClass parent_class;
};

struct _GeeReadOnlyListIterator {
	GeeReadOnlyCollectionIterator parent_instance;
	GeeReadOnlyListIteratorPrivate * priv;
};

struct _GeeReadOnlyListIteratorClass {
	GeeReadOnlyCollectionIteratorClass parent_class;
};

struct _GeeReadOnlyListIteratorPrivate {
	GType g_type;
	GBoxedCopyFunc g_dup_func;
	GDestroyNotify g_destroy_func;
};


static gpointer gee_read_only_list_parent_class = NULL;
static gpointer gee_read_only_list_iterator_parent_class = NULL;
static GeeListIteratorIface* gee_read_only_list_iterator_gee_list_iterator_parent_iface = NULL;
static GeeListIface* gee_read_only_list_gee_list_parent_iface = NULL;

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
G_GNUC_INTERNAL GType gee_read_only_collection_get_type (void) G_GNUC_CONST G_GNUC_UNUSED;
GType gee_list_iterator_get_type (void) G_GNUC_CONST;
GType gee_list_get_type (void) G_GNUC_CONST;
G_GNUC_INTERNAL GType gee_read_only_list_get_type (void) G_GNUC_CONST G_GNUC_UNUSED;
#define GEE_READ_ONLY_LIST_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEE_TYPE_READ_ONLY_LIST, GeeReadOnlyListPrivate))
enum  {
	GEE_READ_ONLY_LIST_DUMMY_PROPERTY,
	GEE_READ_ONLY_LIST_G_TYPE,
	GEE_READ_ONLY_LIST_G_DUP_FUNC,
	GEE_READ_ONLY_LIST_G_DESTROY_FUNC,
	GEE_READ_ONLY_LIST_READ_ONLY_VIEW
};
G_GNUC_INTERNAL GeeReadOnlyList* gee_read_only_list_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeList* list);
G_GNUC_INTERNAL GeeReadOnlyList* gee_read_only_list_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeList* list);
G_GNUC_INTERNAL GeeReadOnlyCollection* gee_read_only_collection_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeCollection* collection);
G_GNUC_INTERNAL GeeReadOnlyCollection* gee_read_only_collection_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeCollection* collection);
static GeeListIterator* gee_read_only_list_real_list_iterator (GeeList* base);
GeeListIterator* gee_list_list_iterator (GeeList* self);
G_GNUC_INTERNAL GeeReadOnlyListIterator* gee_read_only_list_iterator_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeListIterator* iterator);
G_GNUC_INTERNAL GeeReadOnlyListIterator* gee_read_only_list_iterator_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeListIterator* iterator);
GType gee_read_only_collection_iterator_get_type (void) G_GNUC_CONST;
GType gee_read_only_list_iterator_get_type (void) G_GNUC_CONST;
static gint gee_read_only_list_real_index_of (GeeList* base, gconstpointer item);
gint gee_list_index_of (GeeList* self, gconstpointer item);
static void gee_read_only_list_real_insert (GeeList* base, gint index, gconstpointer item);
static gpointer gee_read_only_list_real_remove_at (GeeList* base, gint index);
static gpointer gee_read_only_list_real_get (GeeList* base, gint index);
gpointer gee_list_get (GeeList* self, gint index);
static void gee_read_only_list_real_set (GeeList* base, gint index, gconstpointer o);
static GeeList* gee_read_only_list_real_slice (GeeList* base, gint start, gint stop);
GeeList* gee_list_slice (GeeList* self, gint start, gint stop);
static gpointer gee_read_only_list_real_first (GeeList* base);
gpointer gee_list_first (GeeList* self);
static gpointer gee_read_only_list_real_last (GeeList* base);
gpointer gee_list_last (GeeList* self);
static void gee_read_only_list_real_insert_all (GeeList* base, gint index, GeeCollection* collection);
static void gee_read_only_list_real_sort (GeeList* base, GCompareDataFunc compare, void* compare_target, GDestroyNotify compare_target_destroy_notify);
G_GNUC_INTERNAL GeeList* gee_read_only_list_get_read_only_view (GeeReadOnlyList* self);
#define GEE_READ_ONLY_LIST_ITERATOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GEE_READ_ONLY_LIST_TYPE_ITERATOR, GeeReadOnlyListIteratorPrivate))
enum  {
	GEE_READ_ONLY_LIST_ITERATOR_DUMMY_PROPERTY,
	GEE_READ_ONLY_LIST_ITERATOR_G_TYPE,
	GEE_READ_ONLY_LIST_ITERATOR_G_DUP_FUNC,
	GEE_READ_ONLY_LIST_ITERATOR_G_DESTROY_FUNC
};
G_GNUC_INTERNAL GeeReadOnlyCollectionIterator* gee_read_only_collection_iterator_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeIterator* iterator);
G_GNUC_INTERNAL GeeReadOnlyCollectionIterator* gee_read_only_collection_iterator_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeIterator* iterator);
static void gee_read_only_list_iterator_real_set (GeeListIterator* base, gconstpointer item);
static void gee_read_only_list_iterator_real_add (GeeListIterator* base, gconstpointer item);
static gint gee_read_only_list_iterator_real_index (GeeListIterator* base);
gint gee_list_iterator_index (GeeListIterator* self);
static void _vala_gee_read_only_list_iterator_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void _vala_gee_read_only_list_iterator_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);
static void _vala_gee_read_only_list_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec);
static void _vala_gee_read_only_list_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec);


/**
 * Constructs a read-only list that mirrors the content of the specified
 * list.
 *
 * @param list the list to decorate.
 */
G_GNUC_INTERNAL GeeReadOnlyList* gee_read_only_list_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeList* list) {
	GeeReadOnlyList * self = NULL;
	GeeList* _tmp0_ = NULL;
	g_return_val_if_fail (list != NULL, NULL);
	_tmp0_ = list;
	self = (GeeReadOnlyList*) gee_read_only_collection_construct (object_type, g_type, (GBoxedCopyFunc) g_dup_func, g_destroy_func, (GeeCollection*) _tmp0_);
	self->priv->g_type = g_type;
	self->priv->g_dup_func = g_dup_func;
	self->priv->g_destroy_func = g_destroy_func;
	return self;
}


G_GNUC_INTERNAL GeeReadOnlyList* gee_read_only_list_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeList* list) {
	return gee_read_only_list_construct (GEE_TYPE_READ_ONLY_LIST, g_type, g_dup_func, g_destroy_func, list);
}


/**
 * {@inheritDoc}
 */
static GeeListIterator* gee_read_only_list_real_list_iterator (GeeList* base) {
	GeeReadOnlyList * self;
	GeeListIterator* result = NULL;
	GeeCollection* _tmp0_ = NULL;
	GeeListIterator* _tmp1_ = NULL;
	GeeListIterator* _tmp2_ = NULL;
	GeeReadOnlyListIterator* _tmp3_ = NULL;
	GeeListIterator* _tmp4_ = NULL;
	self = (GeeReadOnlyList*) base;
	_tmp0_ = ((GeeReadOnlyCollection*) self)->_collection;
	_tmp1_ = gee_list_list_iterator (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST, GeeList));
	_tmp2_ = _tmp1_;
	_tmp3_ = gee_read_only_list_iterator_new (self->priv->g_type, (GBoxedCopyFunc) self->priv->g_dup_func, self->priv->g_destroy_func, _tmp2_);
	_tmp4_ = (GeeListIterator*) _tmp3_;
	_g_object_unref0 (_tmp2_);
	result = _tmp4_;
	return result;
}


/**
 * {@inheritDoc}
 */
static gint gee_read_only_list_real_index_of (GeeList* base, gconstpointer item) {
	GeeReadOnlyList * self;
	gint result = 0;
	GeeCollection* _tmp0_ = NULL;
	gconstpointer _tmp1_ = NULL;
	gint _tmp2_ = 0;
	self = (GeeReadOnlyList*) base;
	_tmp0_ = ((GeeReadOnlyCollection*) self)->_collection;
	_tmp1_ = item;
	_tmp2_ = gee_list_index_of (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST, GeeList), _tmp1_);
	result = _tmp2_;
	return result;
}


/**
 * Unimplemented method (read only list).
 */
static void gee_read_only_list_real_insert (GeeList* base, gint index, gconstpointer item) {
	GeeReadOnlyList * self;
	self = (GeeReadOnlyList*) base;
	g_assert_not_reached ();
}


/**
 * Unimplemented method (read only list).
 */
static gpointer gee_read_only_list_real_remove_at (GeeList* base, gint index) {
	GeeReadOnlyList * self;
	gpointer result = NULL;
	self = (GeeReadOnlyList*) base;
	g_assert_not_reached ();
	return result;
}


/**
 * {@inheritDoc}
 */
static gpointer gee_read_only_list_real_get (GeeList* base, gint index) {
	GeeReadOnlyList * self;
	gpointer result = NULL;
	GeeCollection* _tmp0_ = NULL;
	gint _tmp1_ = 0;
	gpointer _tmp2_ = NULL;
	self = (GeeReadOnlyList*) base;
	_tmp0_ = ((GeeReadOnlyCollection*) self)->_collection;
	_tmp1_ = index;
	_tmp2_ = gee_list_get (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST, GeeList), _tmp1_);
	result = _tmp2_;
	return result;
}


/**
 * Unimplemented method (read only list).
 */
static void gee_read_only_list_real_set (GeeList* base, gint index, gconstpointer o) {
	GeeReadOnlyList * self;
	self = (GeeReadOnlyList*) base;
	g_assert_not_reached ();
}


/**
 * {@inheritDoc}
 */
static GeeList* gee_read_only_list_real_slice (GeeList* base, gint start, gint stop) {
	GeeReadOnlyList * self;
	GeeList* result = NULL;
	GeeCollection* _tmp0_ = NULL;
	gint _tmp1_ = 0;
	gint _tmp2_ = 0;
	GeeList* _tmp3_ = NULL;
	self = (GeeReadOnlyList*) base;
	_tmp0_ = ((GeeReadOnlyCollection*) self)->_collection;
	_tmp1_ = start;
	_tmp2_ = stop;
	_tmp3_ = gee_list_slice (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST, GeeList), _tmp1_, _tmp2_);
	result = _tmp3_;
	return result;
}


/**
 * {@inheritDoc}
 */
static gpointer gee_read_only_list_real_first (GeeList* base) {
	GeeReadOnlyList * self;
	gpointer result = NULL;
	GeeCollection* _tmp0_ = NULL;
	gpointer _tmp1_ = NULL;
	self = (GeeReadOnlyList*) base;
	_tmp0_ = ((GeeReadOnlyCollection*) self)->_collection;
	_tmp1_ = gee_list_first (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST, GeeList));
	result = _tmp1_;
	return result;
}


/**
 * {@inheritDoc}
 */
static gpointer gee_read_only_list_real_last (GeeList* base) {
	GeeReadOnlyList * self;
	gpointer result = NULL;
	GeeCollection* _tmp0_ = NULL;
	gpointer _tmp1_ = NULL;
	self = (GeeReadOnlyList*) base;
	_tmp0_ = ((GeeReadOnlyCollection*) self)->_collection;
	_tmp1_ = gee_list_last (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST, GeeList));
	result = _tmp1_;
	return result;
}


/**
 * Unimplemented method (read only list).
 */
static void gee_read_only_list_real_insert_all (GeeList* base, gint index, GeeCollection* collection) {
	GeeReadOnlyList * self;
	self = (GeeReadOnlyList*) base;
	g_return_if_fail (collection != NULL);
	g_assert_not_reached ();
}


/**
 * {@inheritDoc}
 */
static void gee_read_only_list_real_sort (GeeList* base, GCompareDataFunc compare, void* compare_target, GDestroyNotify compare_target_destroy_notify) {
	GeeReadOnlyList * self;
	self = (GeeReadOnlyList*) base;
	g_assert_not_reached ();
	(compare_target_destroy_notify == NULL) ? NULL : (compare_target_destroy_notify (compare_target), NULL);
	compare = NULL;
	compare_target = NULL;
	compare_target_destroy_notify = NULL;
}


G_GNUC_INTERNAL GeeList* gee_read_only_list_get_read_only_view (GeeReadOnlyList* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_READ_ONLY_LIST_GET_CLASS (self)->get_read_only_view (self);
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


static GeeList* gee_read_only_list_real_get_read_only_view (GeeReadOnlyList* base) {
	GeeList* result;
	GeeReadOnlyList* self;
	GeeList* _tmp0_ = NULL;
	self = base;
	_tmp0_ = _g_object_ref0 ((GeeList*) self);
	result = _tmp0_;
	return result;
}


G_GNUC_INTERNAL GeeReadOnlyListIterator* gee_read_only_list_iterator_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeListIterator* iterator) {
	GeeReadOnlyListIterator * self = NULL;
	GeeListIterator* _tmp0_ = NULL;
	g_return_val_if_fail (iterator != NULL, NULL);
	_tmp0_ = iterator;
	self = (GeeReadOnlyListIterator*) gee_read_only_collection_iterator_construct (object_type, g_type, (GBoxedCopyFunc) g_dup_func, g_destroy_func, (GeeIterator*) _tmp0_);
	self->priv->g_type = g_type;
	self->priv->g_dup_func = g_dup_func;
	self->priv->g_destroy_func = g_destroy_func;
	return self;
}


G_GNUC_INTERNAL GeeReadOnlyListIterator* gee_read_only_list_iterator_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeListIterator* iterator) {
	return gee_read_only_list_iterator_construct (GEE_READ_ONLY_LIST_TYPE_ITERATOR, g_type, g_dup_func, g_destroy_func, iterator);
}


static void gee_read_only_list_iterator_real_set (GeeListIterator* base, gconstpointer item) {
	GeeReadOnlyListIterator * self;
	self = (GeeReadOnlyListIterator*) base;
	g_assert_not_reached ();
}


static void gee_read_only_list_iterator_real_add (GeeListIterator* base, gconstpointer item) {
	GeeReadOnlyListIterator * self;
	self = (GeeReadOnlyListIterator*) base;
	g_assert_not_reached ();
}


static gint gee_read_only_list_iterator_real_index (GeeListIterator* base) {
	GeeReadOnlyListIterator * self;
	gint result = 0;
	GeeIterator* _tmp0_ = NULL;
	gint _tmp1_ = 0;
	self = (GeeReadOnlyListIterator*) base;
	_tmp0_ = ((GeeReadOnlyCollectionIterator*) self)->_iter;
	_tmp1_ = gee_list_iterator_index (G_TYPE_CHECK_INSTANCE_CAST (_tmp0_, GEE_TYPE_LIST_ITERATOR, GeeListIterator));
	result = _tmp1_;
	return result;
}


static void gee_read_only_list_iterator_class_init (GeeReadOnlyListIteratorClass * klass) {
	gee_read_only_list_iterator_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GeeReadOnlyListIteratorPrivate));
	G_OBJECT_CLASS (klass)->get_property = _vala_gee_read_only_list_iterator_get_property;
	G_OBJECT_CLASS (klass)->set_property = _vala_gee_read_only_list_iterator_set_property;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_ITERATOR_G_TYPE, g_param_spec_gtype ("g-type", "type", "type", G_TYPE_NONE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_ITERATOR_G_DUP_FUNC, g_param_spec_pointer ("g-dup-func", "dup func", "dup func", G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_ITERATOR_G_DESTROY_FUNC, g_param_spec_pointer ("g-destroy-func", "destroy func", "destroy func", G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
}


static void gee_read_only_list_iterator_gee_list_iterator_interface_init (GeeListIteratorIface * iface) {
	gee_read_only_list_iterator_gee_list_iterator_parent_iface = g_type_interface_peek_parent (iface);
	iface->set = (void (*)(GeeListIterator*, gconstpointer)) gee_read_only_list_iterator_real_set;
	iface->add = (void (*)(GeeListIterator*, gconstpointer)) gee_read_only_list_iterator_real_add;
	iface->index = (gint (*)(GeeListIterator*)) gee_read_only_list_iterator_real_index;
}


static void gee_read_only_list_iterator_instance_init (GeeReadOnlyListIterator * self) {
	self->priv = GEE_READ_ONLY_LIST_ITERATOR_GET_PRIVATE (self);
}


GType gee_read_only_list_iterator_get_type (void) {
	static volatile gsize gee_read_only_list_iterator_type_id__volatile = 0;
	if (g_once_init_enter (&gee_read_only_list_iterator_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (GeeReadOnlyListIteratorClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gee_read_only_list_iterator_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GeeReadOnlyListIterator), 0, (GInstanceInitFunc) gee_read_only_list_iterator_instance_init, NULL };
		static const GInterfaceInfo gee_list_iterator_info = { (GInterfaceInitFunc) gee_read_only_list_iterator_gee_list_iterator_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		GType gee_read_only_list_iterator_type_id;
		gee_read_only_list_iterator_type_id = g_type_register_static (GEE_READ_ONLY_COLLECTION_TYPE_ITERATOR, "GeeReadOnlyListIterator", &g_define_type_info, 0);
		g_type_add_interface_static (gee_read_only_list_iterator_type_id, GEE_TYPE_LIST_ITERATOR, &gee_list_iterator_info);
		g_once_init_leave (&gee_read_only_list_iterator_type_id__volatile, gee_read_only_list_iterator_type_id);
	}
	return gee_read_only_list_iterator_type_id__volatile;
}


static void _vala_gee_read_only_list_iterator_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GeeReadOnlyListIterator * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, GEE_READ_ONLY_LIST_TYPE_ITERATOR, GeeReadOnlyListIterator);
	switch (property_id) {
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void _vala_gee_read_only_list_iterator_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	GeeReadOnlyListIterator * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, GEE_READ_ONLY_LIST_TYPE_ITERATOR, GeeReadOnlyListIterator);
	switch (property_id) {
		case GEE_READ_ONLY_LIST_ITERATOR_G_TYPE:
		self->priv->g_type = g_value_get_gtype (value);
		break;
		case GEE_READ_ONLY_LIST_ITERATOR_G_DUP_FUNC:
		self->priv->g_dup_func = g_value_get_pointer (value);
		break;
		case GEE_READ_ONLY_LIST_ITERATOR_G_DESTROY_FUNC:
		self->priv->g_destroy_func = g_value_get_pointer (value);
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void gee_read_only_list_class_init (GeeReadOnlyListClass * klass) {
	gee_read_only_list_parent_class = g_type_class_peek_parent (klass);
	g_type_class_add_private (klass, sizeof (GeeReadOnlyListPrivate));
	GEE_READ_ONLY_LIST_CLASS (klass)->get_read_only_view = gee_read_only_list_real_get_read_only_view;
	G_OBJECT_CLASS (klass)->get_property = _vala_gee_read_only_list_get_property;
	G_OBJECT_CLASS (klass)->set_property = _vala_gee_read_only_list_set_property;
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_G_TYPE, g_param_spec_gtype ("g-type", "type", "type", G_TYPE_NONE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_G_DUP_FUNC, g_param_spec_pointer ("g-dup-func", "dup func", "dup func", G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_G_DESTROY_FUNC, g_param_spec_pointer ("g-destroy-func", "destroy func", "destroy func", G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_WRITABLE | G_PARAM_CONSTRUCT_ONLY));
	/**
	 * {@inheritDoc}
	 */
	g_object_class_install_property (G_OBJECT_CLASS (klass), GEE_READ_ONLY_LIST_READ_ONLY_VIEW, g_param_spec_object ("read-only-view", "read-only-view", "read-only-view", GEE_TYPE_LIST, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
}


static GType gee_read_only_list_gee_list_get_g_type (GeeReadOnlyList* self) {
	return self->priv->g_type;
}


static GBoxedCopyFunc gee_read_only_list_gee_list_get_g_dup_func (GeeReadOnlyList* self) {
	return self->priv->g_dup_func;
}


static GDestroyNotify gee_read_only_list_gee_list_get_g_destroy_func (GeeReadOnlyList* self) {
	return self->priv->g_destroy_func;
}


static void gee_read_only_list_gee_list_interface_init (GeeListIface * iface) {
	gee_read_only_list_gee_list_parent_iface = g_type_interface_peek_parent (iface);
	iface->list_iterator = (GeeListIterator* (*)(GeeList*)) gee_read_only_list_real_list_iterator;
	iface->index_of = (gint (*)(GeeList*, gconstpointer)) gee_read_only_list_real_index_of;
	iface->insert = (void (*)(GeeList*, gint, gconstpointer)) gee_read_only_list_real_insert;
	iface->remove_at = (gpointer (*)(GeeList*, gint)) gee_read_only_list_real_remove_at;
	iface->get = (gpointer (*)(GeeList*, gint)) gee_read_only_list_real_get;
	iface->set = (void (*)(GeeList*, gint, gconstpointer)) gee_read_only_list_real_set;
	iface->slice = (GeeList* (*)(GeeList*, gint, gint)) gee_read_only_list_real_slice;
	iface->first = (gpointer (*)(GeeList*)) gee_read_only_list_real_first;
	iface->last = (gpointer (*)(GeeList*)) gee_read_only_list_real_last;
	iface->insert_all = (void (*)(GeeList*, gint, GeeCollection*)) gee_read_only_list_real_insert_all;
	iface->sort = (void (*)(GeeList*, GCompareDataFunc, void*, GDestroyNotify)) gee_read_only_list_real_sort;
	iface->get_g_type = (GType(*)(GeeList*)) gee_read_only_list_gee_list_get_g_type;
	iface->get_g_dup_func = (GBoxedCopyFunc(*)(GeeList*)) gee_read_only_list_gee_list_get_g_dup_func;
	iface->get_g_destroy_func = (GDestroyNotify(*)(GeeList*)) gee_read_only_list_gee_list_get_g_destroy_func;
	iface->get_read_only_view = (GeeList* (*) (GeeList *)) gee_read_only_list_get_read_only_view;
}


static void gee_read_only_list_instance_init (GeeReadOnlyList * self) {
	self->priv = GEE_READ_ONLY_LIST_GET_PRIVATE (self);
}


/**
 * Read-only view for {@link List} collections.
 *
 * This class decorates any class which implements the {@link List}
 * interface by making it read only. Any method which normally modify data will
 * throw an error.
 *
 * @see List
 */
G_GNUC_INTERNAL GType gee_read_only_list_get_type (void) {
	static volatile gsize gee_read_only_list_type_id__volatile = 0;
	if (g_once_init_enter (&gee_read_only_list_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (GeeReadOnlyListClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) gee_read_only_list_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (GeeReadOnlyList), 0, (GInstanceInitFunc) gee_read_only_list_instance_init, NULL };
		static const GInterfaceInfo gee_list_info = { (GInterfaceInitFunc) gee_read_only_list_gee_list_interface_init, (GInterfaceFinalizeFunc) NULL, NULL};
		GType gee_read_only_list_type_id;
		gee_read_only_list_type_id = g_type_register_static (GEE_TYPE_READ_ONLY_COLLECTION, "GeeReadOnlyList", &g_define_type_info, 0);
		g_type_add_interface_static (gee_read_only_list_type_id, GEE_TYPE_LIST, &gee_list_info);
		g_once_init_leave (&gee_read_only_list_type_id__volatile, gee_read_only_list_type_id);
	}
	return gee_read_only_list_type_id__volatile;
}


static void _vala_gee_read_only_list_get_property (GObject * object, guint property_id, GValue * value, GParamSpec * pspec) {
	GeeReadOnlyList * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, GEE_TYPE_READ_ONLY_LIST, GeeReadOnlyList);
	switch (property_id) {
		case GEE_READ_ONLY_LIST_READ_ONLY_VIEW:
		g_value_take_object (value, gee_read_only_list_get_read_only_view (self));
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}


static void _vala_gee_read_only_list_set_property (GObject * object, guint property_id, const GValue * value, GParamSpec * pspec) {
	GeeReadOnlyList * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (object, GEE_TYPE_READ_ONLY_LIST, GeeReadOnlyList);
	switch (property_id) {
		case GEE_READ_ONLY_LIST_G_TYPE:
		self->priv->g_type = g_value_get_gtype (value);
		break;
		case GEE_READ_ONLY_LIST_G_DUP_FUNC:
		self->priv->g_dup_func = g_value_get_pointer (value);
		break;
		case GEE_READ_ONLY_LIST_G_DESTROY_FUNC:
		self->priv->g_destroy_func = g_value_get_pointer (value);
		break;
		default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
		break;
	}
}



