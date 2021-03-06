/* testreadonlycollection.c generated by valac 0.26.0, the Vala compiler
 * generated from testreadonlycollection.vala, do not modify */

/* testreadonlycollection.vala
 *
 * Copyright (C) 2008  Jürg Billeter
 * Copyright (C) 2009  Didier Villevalois
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
 * 	Tomaž Vajngerl <quikee@gmail.com>
 * 	Julien Peeters <contact@julienpeeters.fr>
 */

#include <glib.h>
#include <glib-object.h>
#include <gee.h>
#include <stdlib.h>
#include <string.h>


#define GEE_TYPE_TEST_CASE (gee_test_case_get_type ())
#define GEE_TEST_CASE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_TEST_CASE, GeeTestCase))
#define GEE_TEST_CASE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_TEST_CASE, GeeTestCaseClass))
#define GEE_IS_TEST_CASE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_TEST_CASE))
#define GEE_IS_TEST_CASE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_TEST_CASE))
#define GEE_TEST_CASE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_TEST_CASE, GeeTestCaseClass))

typedef struct _GeeTestCase GeeTestCase;
typedef struct _GeeTestCaseClass GeeTestCaseClass;
typedef struct _GeeTestCasePrivate GeeTestCasePrivate;

#define TYPE_READ_ONLY_COLLECTION_TESTS (read_only_collection_tests_get_type ())
#define READ_ONLY_COLLECTION_TESTS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_READ_ONLY_COLLECTION_TESTS, ReadOnlyCollectionTests))
#define READ_ONLY_COLLECTION_TESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_READ_ONLY_COLLECTION_TESTS, ReadOnlyCollectionTestsClass))
#define IS_READ_ONLY_COLLECTION_TESTS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_READ_ONLY_COLLECTION_TESTS))
#define IS_READ_ONLY_COLLECTION_TESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_READ_ONLY_COLLECTION_TESTS))
#define READ_ONLY_COLLECTION_TESTS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_READ_ONLY_COLLECTION_TESTS, ReadOnlyCollectionTestsClass))

typedef struct _ReadOnlyCollectionTests ReadOnlyCollectionTests;
typedef struct _ReadOnlyCollectionTestsClass ReadOnlyCollectionTestsClass;
typedef struct _ReadOnlyCollectionTestsPrivate ReadOnlyCollectionTestsPrivate;
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _vala_assert(expr, msg) if G_LIKELY (expr) ; else g_assertion_message_expr (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, msg);

struct _GeeTestCase {
	GObject parent_instance;
	GeeTestCasePrivate * priv;
};

struct _GeeTestCaseClass {
	GObjectClass parent_class;
	void (*set_up) (GeeTestCase* self);
	void (*tear_down) (GeeTestCase* self);
};

struct _ReadOnlyCollectionTests {
	GeeTestCase parent_instance;
	ReadOnlyCollectionTestsPrivate * priv;
	GeeCollection* test_collection;
	GeeCollection* ro_collection;
};

struct _ReadOnlyCollectionTestsClass {
	GeeTestCaseClass parent_class;
	GeeCollection* (*get_ro_view) (ReadOnlyCollectionTests* self, GeeCollection* collection);
};

typedef void (*GeeTestCaseTestMethod) (void* user_data);

static gpointer read_only_collection_tests_parent_class = NULL;

GType gee_test_case_get_type (void) G_GNUC_CONST;
GType read_only_collection_tests_get_type (void) G_GNUC_CONST;
enum  {
	READ_ONLY_COLLECTION_TESTS_DUMMY_PROPERTY
};
ReadOnlyCollectionTests* read_only_collection_tests_new (void);
ReadOnlyCollectionTests* read_only_collection_tests_construct (GType object_type);
ReadOnlyCollectionTests* read_only_collection_tests_new_with_name (const gchar* name);
ReadOnlyCollectionTests* read_only_collection_tests_construct_with_name (GType object_type, const gchar* name);
GeeTestCase* gee_test_case_construct (GType object_type, const gchar* name);
void gee_test_case_add_test (GeeTestCase* self, const gchar* name, GeeTestCaseTestMethod test, void* test_target, GDestroyNotify test_target_destroy_notify);
void read_only_collection_tests_test_unique_read_only_view_instance (ReadOnlyCollectionTests* self);
static void _read_only_collection_tests_test_unique_read_only_view_instance_gee_test_case_test_method (gpointer self);
void read_only_collection_tests_test_immutable_iterator (ReadOnlyCollectionTests* self);
static void _read_only_collection_tests_test_immutable_iterator_gee_test_case_test_method (gpointer self);
void read_only_collection_tests_test_immutable (ReadOnlyCollectionTests* self);
static void _read_only_collection_tests_test_immutable_gee_test_case_test_method (gpointer self);
void read_only_collection_tests_test_accurate_view (ReadOnlyCollectionTests* self);
static void _read_only_collection_tests_test_accurate_view_gee_test_case_test_method (gpointer self);
static void read_only_collection_tests_real_set_up (GeeTestCase* base);
GeeCollection* read_only_collection_tests_get_ro_view (ReadOnlyCollectionTests* self, GeeCollection* collection);
static void read_only_collection_tests_real_tear_down (GeeTestCase* base);
static GeeCollection* read_only_collection_tests_real_get_ro_view (ReadOnlyCollectionTests* self, GeeCollection* collection);
static void read_only_collection_tests_finalize (GObject* obj);


ReadOnlyCollectionTests* read_only_collection_tests_construct (GType object_type) {
	ReadOnlyCollectionTests * self = NULL;
	self = (ReadOnlyCollectionTests*) read_only_collection_tests_construct_with_name (object_type, "ReadOnlyCollection");
	return self;
}


ReadOnlyCollectionTests* read_only_collection_tests_new (void) {
	return read_only_collection_tests_construct (TYPE_READ_ONLY_COLLECTION_TESTS);
}


static void _read_only_collection_tests_test_unique_read_only_view_instance_gee_test_case_test_method (gpointer self) {
	read_only_collection_tests_test_unique_read_only_view_instance ((ReadOnlyCollectionTests*) self);
}


static void _read_only_collection_tests_test_immutable_iterator_gee_test_case_test_method (gpointer self) {
	read_only_collection_tests_test_immutable_iterator ((ReadOnlyCollectionTests*) self);
}


static void _read_only_collection_tests_test_immutable_gee_test_case_test_method (gpointer self) {
	read_only_collection_tests_test_immutable ((ReadOnlyCollectionTests*) self);
}


static void _read_only_collection_tests_test_accurate_view_gee_test_case_test_method (gpointer self) {
	read_only_collection_tests_test_accurate_view ((ReadOnlyCollectionTests*) self);
}


ReadOnlyCollectionTests* read_only_collection_tests_construct_with_name (GType object_type, const gchar* name) {
	ReadOnlyCollectionTests * self = NULL;
	const gchar* _tmp0_ = NULL;
	g_return_val_if_fail (name != NULL, NULL);
	_tmp0_ = name;
	self = (ReadOnlyCollectionTests*) gee_test_case_construct (object_type, _tmp0_);
	gee_test_case_add_test ((GeeTestCase*) self, "[ReadOnlyCollection] unique read-only view instance", _read_only_collection_tests_test_unique_read_only_view_instance_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	gee_test_case_add_test ((GeeTestCase*) self, "[ReadOnlyCollection] immutable iterator", _read_only_collection_tests_test_immutable_iterator_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	gee_test_case_add_test ((GeeTestCase*) self, "[ReadOnlyCollection] immutable", _read_only_collection_tests_test_immutable_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	gee_test_case_add_test ((GeeTestCase*) self, "[ReadOnlyCollection] accurate view", _read_only_collection_tests_test_accurate_view_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	return self;
}


ReadOnlyCollectionTests* read_only_collection_tests_new_with_name (const gchar* name) {
	return read_only_collection_tests_construct_with_name (TYPE_READ_ONLY_COLLECTION_TESTS, name);
}


static void read_only_collection_tests_real_set_up (GeeTestCase* base) {
	ReadOnlyCollectionTests * self;
	GeeHashMultiSet* _tmp0_ = NULL;
	GeeCollection* _tmp1_ = NULL;
	GeeCollection* _tmp2_ = NULL;
	self = (ReadOnlyCollectionTests*) base;
	_tmp0_ = gee_hash_multi_set_new_fixed (G_TYPE_STRING, (GBoxedCopyFunc) g_strdup, g_free, NULL, NULL, NULL, NULL, NULL, NULL);
	_g_object_unref0 (self->test_collection);
	self->test_collection = (GeeCollection*) _tmp0_;
	_tmp1_ = self->test_collection;
	_tmp2_ = read_only_collection_tests_get_ro_view (self, _tmp1_);
	_g_object_unref0 (self->ro_collection);
	self->ro_collection = _tmp2_;
}


static void read_only_collection_tests_real_tear_down (GeeTestCase* base) {
	ReadOnlyCollectionTests * self;
	self = (ReadOnlyCollectionTests*) base;
	_g_object_unref0 (self->test_collection);
	self->test_collection = NULL;
	_g_object_unref0 (self->ro_collection);
	self->ro_collection = NULL;
}


static GeeCollection* read_only_collection_tests_real_get_ro_view (ReadOnlyCollectionTests* self, GeeCollection* collection) {
	GeeCollection* result = NULL;
	GeeCollection* _tmp0_ = NULL;
	GeeCollection* _tmp1_ = NULL;
	GeeCollection* _tmp2_ = NULL;
	g_return_val_if_fail (collection != NULL, NULL);
	_tmp0_ = collection;
	_tmp1_ = gee_collection_get_read_only_view (_tmp0_);
	_tmp2_ = _tmp1_;
	result = _tmp2_;
	return result;
}


GeeCollection* read_only_collection_tests_get_ro_view (ReadOnlyCollectionTests* self, GeeCollection* collection) {
	g_return_val_if_fail (self != NULL, NULL);
	return READ_ONLY_COLLECTION_TESTS_GET_CLASS (self)->get_ro_view (self, collection);
}


void read_only_collection_tests_test_unique_read_only_view_instance (ReadOnlyCollectionTests* self) {
	GeeCollection* another_ro_collection = NULL;
	GeeCollection* _tmp0_ = NULL;
	GeeCollection* _tmp1_ = NULL;
	GeeCollection* _tmp2_ = NULL;
	GeeCollection* _tmp3_ = NULL;
	GeeCollection* _tmp4_ = NULL;
	GObject* _tmp5_ = NULL;
	GeeCollection* _tmp6_ = NULL;
	gconstpointer _tmp7_ = NULL;
	GeeCollection* _tmp8_ = NULL;
	GeeCollection* _tmp9_ = NULL;
	GeeCollection* _tmp10_ = NULL;
	gconstpointer _tmp11_ = NULL;
	GeeCollection* _tmp12_ = NULL;
	GeeCollection* _tmp13_ = NULL;
	GeeCollection* _tmp14_ = NULL;
	GeeCollection* _tmp15_ = NULL;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->test_collection;
	_tmp1_ = read_only_collection_tests_get_ro_view (self, _tmp0_);
	another_ro_collection = _tmp1_;
	_tmp2_ = self->ro_collection;
	_tmp3_ = another_ro_collection;
	_vala_assert (_tmp2_ == _tmp3_, "ro_collection == another_ro_collection");
	_tmp4_ = self->ro_collection;
	_tmp5_ = g_object_new (G_TYPE_OBJECT, NULL);
	g_object_set_data_full ((GObject*) _tmp4_, "marker", _tmp5_, g_object_unref);
	_tmp6_ = another_ro_collection;
	_tmp7_ = g_object_get_data ((GObject*) _tmp6_, "marker");
	_vala_assert (((GObject*) _tmp7_) != NULL, "another_ro_collection.get_data<Object> (\"marker\") != null");
	_g_object_unref0 (another_ro_collection);
	another_ro_collection = NULL;
	_g_object_unref0 (self->ro_collection);
	self->ro_collection = NULL;
	_tmp8_ = self->test_collection;
	_tmp9_ = read_only_collection_tests_get_ro_view (self, _tmp8_);
	_g_object_unref0 (another_ro_collection);
	another_ro_collection = _tmp9_;
	_tmp10_ = another_ro_collection;
	_tmp11_ = g_object_get_data ((GObject*) _tmp10_, "marker");
	_vala_assert (((GObject*) _tmp11_) == NULL, "another_ro_collection.get_data<Object> (\"marker\") == null");
	_tmp12_ = another_ro_collection;
	_tmp13_ = another_ro_collection;
	_tmp14_ = read_only_collection_tests_get_ro_view (self, _tmp13_);
	_tmp15_ = _tmp14_;
	_vala_assert (_tmp12_ == _tmp15_, "another_ro_collection == get_ro_view (another_ro_collection)");
	_g_object_unref0 (_tmp15_);
	_g_object_unref0 (another_ro_collection);
}


void read_only_collection_tests_test_immutable_iterator (ReadOnlyCollectionTests* self) {
	GeeCollection* _tmp0_ = NULL;
	gboolean _tmp1_ = FALSE;
	GeeCollection* _tmp2_ = NULL;
	gboolean _tmp3_ = FALSE;
	GeeCollection* _tmp4_ = NULL;
	gint _tmp5_ = 0;
	gint _tmp6_ = 0;
	GeeCollection* _tmp7_ = NULL;
	gboolean _tmp8_ = FALSE;
	GeeCollection* _tmp9_ = NULL;
	gboolean _tmp10_ = FALSE;
	GeeIterator* iterator = NULL;
	GeeCollection* _tmp11_ = NULL;
	GeeIterator* _tmp12_ = NULL;
	gboolean one_found = FALSE;
	gboolean two_found = FALSE;
	gboolean _tmp25_ = FALSE;
	gboolean _tmp26_ = FALSE;
	GeeIterator* _tmp27_ = NULL;
	gboolean _tmp28_ = FALSE;
	GeeIterator* _tmp29_ = NULL;
	gboolean _tmp30_ = FALSE;
	GeeCollection* _tmp31_ = NULL;
	GeeIterator* _tmp32_ = NULL;
	GeeIterator* _tmp33_ = NULL;
	gboolean _tmp34_ = FALSE;
	GeeIterator* _tmp35_ = NULL;
	gboolean _tmp36_ = FALSE;
	GeeIterator* _tmp37_ = NULL;
	gboolean _tmp38_ = FALSE;
	gboolean _tmp39_ = FALSE;
	gboolean _tmp40_ = FALSE;
	GeeCollection* _tmp42_ = NULL;
	gint _tmp43_ = 0;
	gint _tmp44_ = 0;
	GeeCollection* _tmp45_ = NULL;
	gboolean _tmp46_ = FALSE;
	GeeCollection* _tmp47_ = NULL;
	gboolean _tmp48_ = FALSE;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->test_collection;
	_tmp1_ = gee_collection_add (_tmp0_, "one");
	_vala_assert (_tmp1_, "test_collection.add (\"one\")");
	_tmp2_ = self->test_collection;
	_tmp3_ = gee_collection_add (_tmp2_, "two");
	_vala_assert (_tmp3_, "test_collection.add (\"two\")");
	_tmp4_ = self->ro_collection;
	_tmp5_ = gee_collection_get_size (_tmp4_);
	_tmp6_ = _tmp5_;
	_vala_assert (_tmp6_ == 2, "ro_collection.size == 2");
	_tmp7_ = self->ro_collection;
	_tmp8_ = gee_collection_contains (_tmp7_, "one");
	_vala_assert (_tmp8_, "ro_collection.contains (\"one\")");
	_tmp9_ = self->ro_collection;
	_tmp10_ = gee_collection_contains (_tmp9_, "two");
	_vala_assert (_tmp10_, "ro_collection.contains (\"two\")");
	_tmp11_ = self->ro_collection;
	_tmp12_ = gee_iterable_iterator ((GeeIterable*) _tmp11_);
	iterator = _tmp12_;
	one_found = FALSE;
	two_found = FALSE;
	while (TRUE) {
		GeeIterator* _tmp13_ = NULL;
		gboolean _tmp14_ = FALSE;
		GeeIterator* _tmp15_ = NULL;
		gboolean _tmp16_ = FALSE;
		gboolean _tmp17_ = FALSE;
		GeeIterator* _tmp18_ = NULL;
		gpointer _tmp19_ = NULL;
		gchar* _tmp20_ = NULL;
		GQuark _tmp22_ = 0U;
		static GQuark _tmp21_label0 = 0;
		static GQuark _tmp21_label1 = 0;
		_tmp13_ = iterator;
		_tmp14_ = gee_iterator_next (_tmp13_);
		if (!_tmp14_) {
			break;
		}
		_tmp15_ = iterator;
		_tmp16_ = gee_iterator_get_valid (_tmp15_);
		_tmp17_ = _tmp16_;
		_vala_assert (_tmp17_, "iterator.valid");
		_tmp18_ = iterator;
		_tmp19_ = gee_iterator_get (_tmp18_);
		_tmp20_ = (gchar*) _tmp19_;
		_tmp22_ = (NULL == _tmp20_) ? 0 : g_quark_from_string (_tmp20_);
		g_free (_tmp20_);
		if (_tmp22_ == ((0 != _tmp21_label0) ? _tmp21_label0 : (_tmp21_label0 = g_quark_from_static_string ("one")))) {
			switch (0) {
				default:
				{
					gboolean _tmp23_ = FALSE;
					_tmp23_ = one_found;
					_vala_assert (!_tmp23_, "! one_found");
					one_found = TRUE;
					break;
				}
			}
		} else if (_tmp22_ == ((0 != _tmp21_label1) ? _tmp21_label1 : (_tmp21_label1 = g_quark_from_static_string ("two")))) {
			switch (0) {
				default:
				{
					gboolean _tmp24_ = FALSE;
					_tmp24_ = two_found;
					_vala_assert (!_tmp24_, "! two_found");
					two_found = TRUE;
					break;
				}
			}
		} else {
			switch (0) {
				default:
				{
					g_assert_not_reached ();
				}
			}
		}
	}
	_tmp25_ = one_found;
	_vala_assert (_tmp25_, "one_found");
	_tmp26_ = two_found;
	_vala_assert (_tmp26_, "two_found");
	_tmp27_ = iterator;
	_tmp28_ = gee_iterator_has_next (_tmp27_);
	_vala_assert (!_tmp28_, "! iterator.has_next ()");
	_tmp29_ = iterator;
	_tmp30_ = gee_iterator_next (_tmp29_);
	_vala_assert (!_tmp30_, "! iterator.next ()");
	_tmp31_ = self->ro_collection;
	_tmp32_ = gee_iterable_iterator ((GeeIterable*) _tmp31_);
	_g_object_unref0 (iterator);
	iterator = _tmp32_;
	_tmp33_ = iterator;
	_tmp34_ = gee_iterator_has_next (_tmp33_);
	_vala_assert (_tmp34_, "iterator.has_next ()");
	_tmp35_ = iterator;
	_tmp36_ = gee_iterator_next (_tmp35_);
	_vala_assert (_tmp36_, "iterator.next ()");
	_tmp37_ = iterator;
	_tmp38_ = gee_iterator_get_read_only (_tmp37_);
	_tmp39_ = _tmp38_;
	_vala_assert (_tmp39_, "iterator.read_only");
	_tmp40_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp40_) {
		GeeIterator* _tmp41_ = NULL;
		_tmp41_ = iterator;
		gee_iterator_remove (_tmp41_);
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp42_ = self->ro_collection;
	_tmp43_ = gee_collection_get_size (_tmp42_);
	_tmp44_ = _tmp43_;
	_vala_assert (_tmp44_ == 2, "ro_collection.size == 2");
	_tmp45_ = self->ro_collection;
	_tmp46_ = gee_collection_contains (_tmp45_, "one");
	_vala_assert (_tmp46_, "ro_collection.contains (\"one\")");
	_tmp47_ = self->ro_collection;
	_tmp48_ = gee_collection_contains (_tmp47_, "two");
	_vala_assert (_tmp48_, "ro_collection.contains (\"two\")");
	_g_object_unref0 (iterator);
}


void read_only_collection_tests_test_immutable (ReadOnlyCollectionTests* self) {
	GeeCollection* _tmp0_ = NULL;
	gboolean _tmp1_ = FALSE;
	GeeCollection* _tmp2_ = NULL;
	gint _tmp3_ = 0;
	gint _tmp4_ = 0;
	GeeCollection* _tmp5_ = NULL;
	gboolean _tmp6_ = FALSE;
	GeeCollection* dummy = NULL;
	GeeArrayList* _tmp7_ = NULL;
	GeeCollection* _tmp8_ = NULL;
	gboolean _tmp9_ = FALSE;
	GeeCollection* _tmp10_ = NULL;
	gboolean _tmp11_ = FALSE;
	gboolean _tmp12_ = FALSE;
	GeeCollection* _tmp15_ = NULL;
	gint _tmp16_ = 0;
	gint _tmp17_ = 0;
	GeeCollection* _tmp18_ = NULL;
	gboolean _tmp19_ = FALSE;
	gboolean _tmp20_ = FALSE;
	GeeCollection* _tmp22_ = NULL;
	gint _tmp23_ = 0;
	gint _tmp24_ = 0;
	GeeCollection* _tmp25_ = NULL;
	gboolean _tmp26_ = FALSE;
	gboolean _tmp27_ = FALSE;
	GeeCollection* _tmp30_ = NULL;
	gint _tmp31_ = 0;
	gint _tmp32_ = 0;
	GeeCollection* _tmp33_ = NULL;
	gboolean _tmp34_ = FALSE;
	gboolean _tmp35_ = FALSE;
	GeeCollection* _tmp39_ = NULL;
	gint _tmp40_ = 0;
	gint _tmp41_ = 0;
	GeeCollection* _tmp42_ = NULL;
	gboolean _tmp43_ = FALSE;
	gboolean _tmp44_ = FALSE;
	GeeCollection* _tmp48_ = NULL;
	gint _tmp49_ = 0;
	gint _tmp50_ = 0;
	GeeCollection* _tmp51_ = NULL;
	gboolean _tmp52_ = FALSE;
	GeeCollection* _tmp53_ = NULL;
	gboolean _tmp54_ = FALSE;
	gboolean _tmp55_ = FALSE;
	GeeCollection* _tmp59_ = NULL;
	gint _tmp60_ = 0;
	gint _tmp61_ = 0;
	GeeCollection* _tmp62_ = NULL;
	gboolean _tmp63_ = FALSE;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->test_collection;
	_tmp1_ = gee_collection_add (_tmp0_, "one");
	_vala_assert (_tmp1_, "test_collection.add (\"one\")");
	_tmp2_ = self->ro_collection;
	_tmp3_ = gee_collection_get_size (_tmp2_);
	_tmp4_ = _tmp3_;
	_vala_assert (_tmp4_ == 1, "ro_collection.size == 1");
	_tmp5_ = self->ro_collection;
	_tmp6_ = gee_collection_contains (_tmp5_, "one");
	_vala_assert (_tmp6_, "ro_collection.contains (\"one\")");
	_tmp7_ = gee_array_list_new (G_TYPE_STRING, (GBoxedCopyFunc) g_strdup, g_free, NULL, NULL, NULL);
	dummy = (GeeCollection*) _tmp7_;
	_tmp8_ = dummy;
	_tmp9_ = gee_collection_add (_tmp8_, "one");
	_vala_assert (_tmp9_, "dummy.add (\"one\")");
	_tmp10_ = dummy;
	_tmp11_ = gee_collection_add (_tmp10_, "two");
	_vala_assert (_tmp11_, "dummy.add (\"two\")");
	_tmp12_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp12_) {
		GeeCollection* _tmp13_ = NULL;
		gboolean _tmp14_ = FALSE;
		_tmp13_ = self->ro_collection;
		_tmp14_ = gee_collection_add (_tmp13_, "two");
		_vala_assert (_tmp14_, "ro_collection.add (\"two\")");
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp15_ = self->ro_collection;
	_tmp16_ = gee_collection_get_size (_tmp15_);
	_tmp17_ = _tmp16_;
	_vala_assert (_tmp17_ == 1, "ro_collection.size == 1");
	_tmp18_ = self->ro_collection;
	_tmp19_ = gee_collection_contains (_tmp18_, "one");
	_vala_assert (_tmp19_, "ro_collection.contains (\"one\")");
	_tmp20_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp20_) {
		GeeCollection* _tmp21_ = NULL;
		_tmp21_ = self->ro_collection;
		gee_collection_clear (_tmp21_);
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp22_ = self->ro_collection;
	_tmp23_ = gee_collection_get_size (_tmp22_);
	_tmp24_ = _tmp23_;
	_vala_assert (_tmp24_ == 1, "ro_collection.size == 1");
	_tmp25_ = self->ro_collection;
	_tmp26_ = gee_collection_contains (_tmp25_, "one");
	_vala_assert (_tmp26_, "ro_collection.contains (\"one\")");
	_tmp27_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp27_) {
		GeeCollection* _tmp28_ = NULL;
		gboolean _tmp29_ = FALSE;
		_tmp28_ = self->ro_collection;
		_tmp29_ = gee_collection_remove (_tmp28_, "one");
		_vala_assert (_tmp29_, "ro_collection.remove (\"one\")");
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp30_ = self->ro_collection;
	_tmp31_ = gee_collection_get_size (_tmp30_);
	_tmp32_ = _tmp31_;
	_vala_assert (_tmp32_ == 1, "ro_collection.size == 1");
	_tmp33_ = self->ro_collection;
	_tmp34_ = gee_collection_contains (_tmp33_, "one");
	_vala_assert (_tmp34_, "ro_collection.contains (\"one\")");
	_tmp35_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp35_) {
		GeeCollection* _tmp36_ = NULL;
		GeeCollection* _tmp37_ = NULL;
		gboolean _tmp38_ = FALSE;
		_tmp36_ = self->ro_collection;
		_tmp37_ = dummy;
		_tmp38_ = gee_collection_add_all (_tmp36_, _tmp37_);
		_vala_assert (_tmp38_, "ro_collection.add_all (dummy)");
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp39_ = self->ro_collection;
	_tmp40_ = gee_collection_get_size (_tmp39_);
	_tmp41_ = _tmp40_;
	_vala_assert (_tmp41_ == 1, "ro_collection.size == 1");
	_tmp42_ = self->ro_collection;
	_tmp43_ = gee_collection_contains (_tmp42_, "one");
	_vala_assert (_tmp43_, "ro_collection.contains (\"one\")");
	_tmp44_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp44_) {
		GeeCollection* _tmp45_ = NULL;
		GeeCollection* _tmp46_ = NULL;
		gboolean _tmp47_ = FALSE;
		_tmp45_ = self->ro_collection;
		_tmp46_ = dummy;
		_tmp47_ = gee_collection_remove_all (_tmp45_, _tmp46_);
		_vala_assert (_tmp47_, "ro_collection.remove_all (dummy)");
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp48_ = self->ro_collection;
	_tmp49_ = gee_collection_get_size (_tmp48_);
	_tmp50_ = _tmp49_;
	_vala_assert (_tmp50_ == 1, "ro_collection.size == 1");
	_tmp51_ = self->ro_collection;
	_tmp52_ = gee_collection_contains (_tmp51_, "one");
	_vala_assert (_tmp52_, "ro_collection.contains (\"one\")");
	_tmp53_ = dummy;
	_tmp54_ = gee_collection_remove (_tmp53_, "one");
	_vala_assert (_tmp54_, "dummy.remove (\"one\")");
	_tmp55_ = g_test_trap_fork ((guint64) 0, G_TEST_TRAP_SILENCE_STDOUT | G_TEST_TRAP_SILENCE_STDERR);
	if (_tmp55_) {
		GeeCollection* _tmp56_ = NULL;
		GeeCollection* _tmp57_ = NULL;
		gboolean _tmp58_ = FALSE;
		_tmp56_ = self->ro_collection;
		_tmp57_ = dummy;
		_tmp58_ = gee_collection_retain_all (_tmp56_, _tmp57_);
		_vala_assert (_tmp58_, "ro_collection.retain_all (dummy)");
		exit (0);
	}
	g_test_trap_assert_failed ();
	_tmp59_ = self->ro_collection;
	_tmp60_ = gee_collection_get_size (_tmp59_);
	_tmp61_ = _tmp60_;
	_vala_assert (_tmp61_ == 1, "ro_collection.size == 1");
	_tmp62_ = self->ro_collection;
	_tmp63_ = gee_collection_contains (_tmp62_, "one");
	_vala_assert (_tmp63_, "ro_collection.contains (\"one\")");
	_g_object_unref0 (dummy);
}


void read_only_collection_tests_test_accurate_view (ReadOnlyCollectionTests* self) {
	GeeCollection* dummy = NULL;
	GeeArrayList* _tmp0_ = NULL;
	gboolean _tmp1_ = FALSE;
	gboolean _tmp2_ = FALSE;
	GeeCollection* _tmp3_ = NULL;
	GType _tmp4_ = 0UL;
	GType _tmp5_ = 0UL;
	GeeCollection* _tmp6_ = NULL;
	gint _tmp7_ = 0;
	gint _tmp8_ = 0;
	GeeCollection* _tmp9_ = NULL;
	gboolean _tmp10_ = FALSE;
	gboolean _tmp11_ = FALSE;
	GeeCollection* _tmp12_ = NULL;
	gboolean _tmp13_ = FALSE;
	GeeCollection* _tmp14_ = NULL;
	gboolean _tmp15_ = FALSE;
	GeeCollection* _tmp16_ = NULL;
	gint _tmp17_ = 0;
	gint _tmp18_ = 0;
	GeeCollection* _tmp19_ = NULL;
	gboolean _tmp20_ = FALSE;
	gboolean _tmp21_ = FALSE;
	GeeCollection* _tmp22_ = NULL;
	gboolean _tmp23_ = FALSE;
	GeeCollection* _tmp24_ = NULL;
	gboolean _tmp25_ = FALSE;
	GeeCollection* _tmp26_ = NULL;
	gint _tmp27_ = 0;
	gint _tmp28_ = 0;
	GeeCollection* _tmp29_ = NULL;
	gboolean _tmp30_ = FALSE;
	gboolean _tmp31_ = FALSE;
	GeeCollection* _tmp32_ = NULL;
	gboolean _tmp33_ = FALSE;
	GeeCollection* _tmp34_ = NULL;
	gboolean _tmp35_ = FALSE;
	GeeCollection* _tmp36_ = NULL;
	gboolean _tmp37_ = FALSE;
	GeeCollection* _tmp38_ = NULL;
	gboolean _tmp39_ = FALSE;
	GeeCollection* _tmp40_ = NULL;
	gint _tmp41_ = 0;
	gint _tmp42_ = 0;
	GeeCollection* _tmp43_ = NULL;
	gboolean _tmp44_ = FALSE;
	gboolean _tmp45_ = FALSE;
	GeeCollection* _tmp46_ = NULL;
	gboolean _tmp47_ = FALSE;
	GeeCollection* _tmp48_ = NULL;
	gboolean _tmp49_ = FALSE;
	GeeCollection* _tmp50_ = NULL;
	gboolean _tmp51_ = FALSE;
	GeeCollection* _tmp52_ = NULL;
	GeeCollection* _tmp53_ = NULL;
	gint _tmp54_ = 0;
	gint _tmp55_ = 0;
	GeeCollection* _tmp56_ = NULL;
	gboolean _tmp57_ = FALSE;
	gboolean _tmp58_ = FALSE;
	GeeCollection* _tmp59_ = NULL;
	gboolean _tmp60_ = FALSE;
	GeeCollection* _tmp61_ = NULL;
	gboolean _tmp62_ = FALSE;
	g_return_if_fail (self != NULL);
	_tmp0_ = gee_array_list_new (G_TYPE_STRING, (GBoxedCopyFunc) g_strdup, g_free, NULL, NULL, NULL);
	dummy = (GeeCollection*) _tmp0_;
	_tmp1_ = gee_collection_add (dummy, "one");
	_vala_assert (_tmp1_, "dummy.add (\"one\")");
	_tmp2_ = gee_collection_add (dummy, "two");
	_vala_assert (_tmp2_, "dummy.add (\"two\")");
	_tmp3_ = self->ro_collection;
	_tmp4_ = gee_traversable_get_element_type ((GeeTraversable*) _tmp3_);
	_tmp5_ = _tmp4_;
	_vala_assert (_tmp5_ == G_TYPE_STRING, "ro_collection.element_type == typeof (string)");
	_tmp6_ = self->ro_collection;
	_tmp7_ = gee_collection_get_size (_tmp6_);
	_tmp8_ = _tmp7_;
	_vala_assert (_tmp8_ == 0, "ro_collection.size == 0");
	_tmp9_ = self->ro_collection;
	_tmp10_ = gee_collection_get_is_empty (_tmp9_);
	_tmp11_ = _tmp10_;
	_vala_assert (_tmp11_, "ro_collection.is_empty");
	_tmp12_ = self->ro_collection;
	_tmp13_ = gee_collection_contains (_tmp12_, "one");
	_vala_assert (!_tmp13_, "! ro_collection.contains (\"one\")");
	_tmp14_ = self->test_collection;
	_tmp15_ = gee_collection_add (_tmp14_, "one");
	_vala_assert (_tmp15_, "test_collection.add (\"one\")");
	_tmp16_ = self->ro_collection;
	_tmp17_ = gee_collection_get_size (_tmp16_);
	_tmp18_ = _tmp17_;
	_vala_assert (_tmp18_ == 1, "ro_collection.size == 1");
	_tmp19_ = self->ro_collection;
	_tmp20_ = gee_collection_get_is_empty (_tmp19_);
	_tmp21_ = _tmp20_;
	_vala_assert (!_tmp21_, "! ro_collection.is_empty");
	_tmp22_ = self->ro_collection;
	_tmp23_ = gee_collection_contains (_tmp22_, "one");
	_vala_assert (_tmp23_, "ro_collection.contains (\"one\")");
	_tmp24_ = self->test_collection;
	_tmp25_ = gee_collection_add (_tmp24_, "two");
	_vala_assert (_tmp25_, "test_collection.add (\"two\")");
	_tmp26_ = self->ro_collection;
	_tmp27_ = gee_collection_get_size (_tmp26_);
	_tmp28_ = _tmp27_;
	_vala_assert (_tmp28_ == 2, "ro_collection.size == 2");
	_tmp29_ = self->ro_collection;
	_tmp30_ = gee_collection_get_is_empty (_tmp29_);
	_tmp31_ = _tmp30_;
	_vala_assert (!_tmp31_, "! ro_collection.is_empty");
	_tmp32_ = self->ro_collection;
	_tmp33_ = gee_collection_contains (_tmp32_, "one");
	_vala_assert (_tmp33_, "ro_collection.contains (\"one\")");
	_tmp34_ = self->ro_collection;
	_tmp35_ = gee_collection_contains (_tmp34_, "two");
	_vala_assert (_tmp35_, "ro_collection.contains (\"two\")");
	_tmp36_ = self->ro_collection;
	_tmp37_ = gee_collection_contains_all (_tmp36_, dummy);
	_vala_assert (_tmp37_, "ro_collection.contains_all (dummy)");
	_tmp38_ = self->test_collection;
	_tmp39_ = gee_collection_remove (_tmp38_, "one");
	_vala_assert (_tmp39_, "test_collection.remove (\"one\")");
	_tmp40_ = self->ro_collection;
	_tmp41_ = gee_collection_get_size (_tmp40_);
	_tmp42_ = _tmp41_;
	_vala_assert (_tmp42_ == 1, "ro_collection.size == 1");
	_tmp43_ = self->ro_collection;
	_tmp44_ = gee_collection_get_is_empty (_tmp43_);
	_tmp45_ = _tmp44_;
	_vala_assert (!_tmp45_, "! ro_collection.is_empty");
	_tmp46_ = self->ro_collection;
	_tmp47_ = gee_collection_contains (_tmp46_, "one");
	_vala_assert (!_tmp47_, "! ro_collection.contains (\"one\")");
	_tmp48_ = self->ro_collection;
	_tmp49_ = gee_collection_contains (_tmp48_, "two");
	_vala_assert (_tmp49_, "ro_collection.contains (\"two\")");
	_tmp50_ = self->ro_collection;
	_tmp51_ = gee_collection_contains_all (_tmp50_, dummy);
	_vala_assert (!_tmp51_, "! ro_collection.contains_all (dummy)");
	_tmp52_ = self->test_collection;
	gee_collection_clear (_tmp52_);
	_tmp53_ = self->ro_collection;
	_tmp54_ = gee_collection_get_size (_tmp53_);
	_tmp55_ = _tmp54_;
	_vala_assert (_tmp55_ == 0, "ro_collection.size == 0");
	_tmp56_ = self->ro_collection;
	_tmp57_ = gee_collection_get_is_empty (_tmp56_);
	_tmp58_ = _tmp57_;
	_vala_assert (_tmp58_, "ro_collection.is_empty");
	_tmp59_ = self->ro_collection;
	_tmp60_ = gee_collection_contains (_tmp59_, "one");
	_vala_assert (!_tmp60_, "! ro_collection.contains (\"one\")");
	_tmp61_ = self->ro_collection;
	_tmp62_ = gee_collection_contains (_tmp61_, "two");
	_vala_assert (!_tmp62_, "! ro_collection.contains (\"two\")");
	_g_object_unref0 (dummy);
}


static void read_only_collection_tests_class_init (ReadOnlyCollectionTestsClass * klass) {
	read_only_collection_tests_parent_class = g_type_class_peek_parent (klass);
	((GeeTestCaseClass *) klass)->set_up = read_only_collection_tests_real_set_up;
	((GeeTestCaseClass *) klass)->tear_down = read_only_collection_tests_real_tear_down;
	((ReadOnlyCollectionTestsClass *) klass)->get_ro_view = read_only_collection_tests_real_get_ro_view;
	G_OBJECT_CLASS (klass)->finalize = read_only_collection_tests_finalize;
}


static void read_only_collection_tests_instance_init (ReadOnlyCollectionTests * self) {
}


static void read_only_collection_tests_finalize (GObject* obj) {
	ReadOnlyCollectionTests * self;
	self = G_TYPE_CHECK_INSTANCE_CAST (obj, TYPE_READ_ONLY_COLLECTION_TESTS, ReadOnlyCollectionTests);
	_g_object_unref0 (self->test_collection);
	_g_object_unref0 (self->ro_collection);
	G_OBJECT_CLASS (read_only_collection_tests_parent_class)->finalize (obj);
}


GType read_only_collection_tests_get_type (void) {
	static volatile gsize read_only_collection_tests_type_id__volatile = 0;
	if (g_once_init_enter (&read_only_collection_tests_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (ReadOnlyCollectionTestsClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) read_only_collection_tests_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (ReadOnlyCollectionTests), 0, (GInstanceInitFunc) read_only_collection_tests_instance_init, NULL };
		GType read_only_collection_tests_type_id;
		read_only_collection_tests_type_id = g_type_register_static (GEE_TYPE_TEST_CASE, "ReadOnlyCollectionTests", &g_define_type_info, 0);
		g_once_init_leave (&read_only_collection_tests_type_id__volatile, read_only_collection_tests_type_id);
	}
	return read_only_collection_tests_type_id__volatile;
}



