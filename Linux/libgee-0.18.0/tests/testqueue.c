/* testqueue.c generated by valac 0.26.0, the Vala compiler
 * generated from testqueue.vala, do not modify */

/* testqueue.vala
 *
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
 * 	Didier 'Ptitjes' Villevalois <ptitjes@free.fr>
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

#define TYPE_COLLECTION_TESTS (collection_tests_get_type ())
#define COLLECTION_TESTS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_COLLECTION_TESTS, CollectionTests))
#define COLLECTION_TESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_COLLECTION_TESTS, CollectionTestsClass))
#define IS_COLLECTION_TESTS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_COLLECTION_TESTS))
#define IS_COLLECTION_TESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_COLLECTION_TESTS))
#define COLLECTION_TESTS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_COLLECTION_TESTS, CollectionTestsClass))

typedef struct _CollectionTests CollectionTests;
typedef struct _CollectionTestsClass CollectionTestsClass;
typedef struct _CollectionTestsPrivate CollectionTestsPrivate;

#define TYPE_QUEUE_TESTS (queue_tests_get_type ())
#define QUEUE_TESTS(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_QUEUE_TESTS, QueueTests))
#define QUEUE_TESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), TYPE_QUEUE_TESTS, QueueTestsClass))
#define IS_QUEUE_TESTS(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_QUEUE_TESTS))
#define IS_QUEUE_TESTS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), TYPE_QUEUE_TESTS))
#define QUEUE_TESTS_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), TYPE_QUEUE_TESTS, QueueTestsClass))

typedef struct _QueueTests QueueTests;
typedef struct _QueueTestsClass QueueTestsClass;
typedef struct _QueueTestsPrivate QueueTestsPrivate;
#define _g_free0(var) (var = (g_free (var), NULL))
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

struct _CollectionTests {
	GeeTestCase parent_instance;
	CollectionTestsPrivate * priv;
	GeeCollection* test_collection;
};

struct _CollectionTestsClass {
	GeeTestCaseClass parent_class;
};

struct _QueueTests {
	CollectionTests parent_instance;
	QueueTestsPrivate * priv;
};

struct _QueueTestsClass {
	CollectionTestsClass parent_class;
};

typedef void (*GeeTestCaseTestMethod) (void* user_data);

static gpointer queue_tests_parent_class = NULL;

GType gee_test_case_get_type (void) G_GNUC_CONST;
GType collection_tests_get_type (void) G_GNUC_CONST;
GType queue_tests_get_type (void) G_GNUC_CONST;
enum  {
	QUEUE_TESTS_DUMMY_PROPERTY
};
QueueTests* queue_tests_construct (GType object_type, const gchar* name);
CollectionTests* collection_tests_construct (GType object_type, const gchar* name);
void gee_test_case_add_test (GeeTestCase* self, const gchar* name, GeeTestCaseTestMethod test, void* test_target, GDestroyNotify test_target_destroy_notify);
void queue_tests_test_capacity_bound (QueueTests* self);
static void _queue_tests_test_capacity_bound_gee_test_case_test_method (gpointer self);
void queue_tests_test_one_element_operation (QueueTests* self);
static void _queue_tests_test_one_element_operation_gee_test_case_test_method (gpointer self);
void queue_tests_test_gobject_properties (QueueTests* self);
static void _queue_tests_test_gobject_properties_gee_test_case_test_method (gpointer self);


static void _queue_tests_test_capacity_bound_gee_test_case_test_method (gpointer self) {
	queue_tests_test_capacity_bound ((QueueTests*) self);
}


static void _queue_tests_test_one_element_operation_gee_test_case_test_method (gpointer self) {
	queue_tests_test_one_element_operation ((QueueTests*) self);
}


static void _queue_tests_test_gobject_properties_gee_test_case_test_method (gpointer self) {
	queue_tests_test_gobject_properties ((QueueTests*) self);
}


QueueTests* queue_tests_construct (GType object_type, const gchar* name) {
	QueueTests * self = NULL;
	const gchar* _tmp0_ = NULL;
	g_return_val_if_fail (name != NULL, NULL);
	_tmp0_ = name;
	self = (QueueTests*) collection_tests_construct (object_type, _tmp0_);
	gee_test_case_add_test ((GeeTestCase*) self, "[Queue] capacity bound", _queue_tests_test_capacity_bound_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	gee_test_case_add_test ((GeeTestCase*) self, "[Queue] one element operation", _queue_tests_test_one_element_operation_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	gee_test_case_add_test ((GeeTestCase*) self, "[Queue] GObject properties", _queue_tests_test_gobject_properties_gee_test_case_test_method, g_object_ref (self), g_object_unref);
	return self;
}


static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


void queue_tests_test_capacity_bound (QueueTests* self) {
	GeeQueue* test_queue = NULL;
	GeeCollection* _tmp0_ = NULL;
	GeeQueue* _tmp1_ = NULL;
	GeeQueue* _tmp2_ = NULL;
	GeeQueue* _tmp3_ = NULL;
	gint _tmp4_ = 0;
	gint _tmp5_ = 0;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((CollectionTests*) self)->test_collection;
	_tmp1_ = _g_object_ref0 (G_TYPE_CHECK_INSTANCE_TYPE (_tmp0_, GEE_TYPE_QUEUE) ? ((GeeQueue*) _tmp0_) : NULL);
	test_queue = _tmp1_;
	_tmp2_ = test_queue;
	_vala_assert (_tmp2_ != NULL, "test_queue != null");
	_tmp3_ = test_queue;
	_tmp4_ = gee_queue_get_capacity (_tmp3_);
	_tmp5_ = _tmp4_;
	if (_tmp5_ == GEE_QUEUE_UNBOUNDED_CAPACITY) {
		GeeQueue* _tmp6_ = NULL;
		gint _tmp7_ = 0;
		gint _tmp8_ = 0;
		GeeQueue* _tmp9_ = NULL;
		gboolean _tmp10_ = FALSE;
		gboolean _tmp11_ = FALSE;
		_tmp6_ = test_queue;
		_tmp7_ = gee_queue_get_remaining_capacity (_tmp6_);
		_tmp8_ = _tmp7_;
		_vala_assert (_tmp8_ == GEE_QUEUE_UNBOUNDED_CAPACITY, "test_queue.remaining_capacity == Gee.Queue.UNBOUNDED_CAPACITY");
		_tmp9_ = test_queue;
		_tmp10_ = gee_queue_get_is_full (_tmp9_);
		_tmp11_ = _tmp10_;
		_vala_assert (!_tmp11_, "! test_queue.is_full");
	} else {
		GeeQueue* _tmp12_ = NULL;
		gboolean _tmp13_ = FALSE;
		gboolean _tmp14_ = FALSE;
		gint capacity = 0;
		GeeQueue* _tmp15_ = NULL;
		gint _tmp16_ = 0;
		gint _tmp17_ = 0;
		GeeQueue* _tmp32_ = NULL;
		gboolean _tmp33_ = FALSE;
		gboolean _tmp34_ = FALSE;
		GeeQueue* _tmp49_ = NULL;
		gboolean _tmp50_ = FALSE;
		gboolean _tmp51_ = FALSE;
		_tmp12_ = test_queue;
		_tmp13_ = gee_collection_get_is_empty ((GeeCollection*) _tmp12_);
		_tmp14_ = _tmp13_;
		_vala_assert (_tmp14_, "test_queue.is_empty");
		_tmp15_ = test_queue;
		_tmp16_ = gee_queue_get_capacity (_tmp15_);
		_tmp17_ = _tmp16_;
		capacity = _tmp17_;
		{
			gint i = 0;
			i = 1;
			{
				gboolean _tmp18_ = FALSE;
				_tmp18_ = TRUE;
				while (TRUE) {
					gint _tmp20_ = 0;
					gint _tmp21_ = 0;
					GeeQueue* _tmp22_ = NULL;
					gboolean _tmp23_ = FALSE;
					gboolean _tmp24_ = FALSE;
					GeeQueue* _tmp25_ = NULL;
					gboolean _tmp26_ = FALSE;
					GeeQueue* _tmp27_ = NULL;
					gint _tmp28_ = 0;
					gint _tmp29_ = 0;
					gint _tmp30_ = 0;
					gint _tmp31_ = 0;
					if (!_tmp18_) {
						gint _tmp19_ = 0;
						_tmp19_ = i;
						i = _tmp19_ + 1;
					}
					_tmp18_ = FALSE;
					_tmp20_ = i;
					_tmp21_ = capacity;
					if (!(_tmp20_ <= _tmp21_)) {
						break;
					}
					_tmp22_ = test_queue;
					_tmp23_ = gee_queue_get_is_full (_tmp22_);
					_tmp24_ = _tmp23_;
					_vala_assert (!_tmp24_, "! test_queue.is_full");
					_tmp25_ = test_queue;
					_tmp26_ = gee_queue_offer (_tmp25_, "one");
					_vala_assert (_tmp26_, "test_queue.offer (\"one\")");
					_tmp27_ = test_queue;
					_tmp28_ = gee_queue_get_remaining_capacity (_tmp27_);
					_tmp29_ = _tmp28_;
					_tmp30_ = capacity;
					_tmp31_ = i;
					_vala_assert (_tmp29_ == (_tmp30_ - _tmp31_), "test_queue.remaining_capacity == capacity - i");
				}
			}
		}
		_tmp32_ = test_queue;
		_tmp33_ = gee_queue_get_is_full (_tmp32_);
		_tmp34_ = _tmp33_;
		_vala_assert (_tmp34_, "test_queue.is_full");
		{
			gint i = 0;
			i = 1;
			{
				gboolean _tmp35_ = FALSE;
				_tmp35_ = TRUE;
				while (TRUE) {
					gint _tmp37_ = 0;
					gint _tmp38_ = 0;
					GeeQueue* _tmp39_ = NULL;
					gpointer _tmp40_ = NULL;
					gchar* _tmp41_ = NULL;
					GeeQueue* _tmp42_ = NULL;
					gboolean _tmp43_ = FALSE;
					gboolean _tmp44_ = FALSE;
					GeeQueue* _tmp45_ = NULL;
					gint _tmp46_ = 0;
					gint _tmp47_ = 0;
					gint _tmp48_ = 0;
					if (!_tmp35_) {
						gint _tmp36_ = 0;
						_tmp36_ = i;
						i = _tmp36_ + 1;
					}
					_tmp35_ = FALSE;
					_tmp37_ = i;
					_tmp38_ = capacity;
					if (!(_tmp37_ <= _tmp38_)) {
						break;
					}
					_tmp39_ = test_queue;
					_tmp40_ = gee_queue_poll (_tmp39_);
					_tmp41_ = (gchar*) _tmp40_;
					_vala_assert (g_strcmp0 (_tmp41_, "one") == 0, "test_queue.poll () == \"one\"");
					_g_free0 (_tmp41_);
					_tmp42_ = test_queue;
					_tmp43_ = gee_queue_get_is_full (_tmp42_);
					_tmp44_ = _tmp43_;
					_vala_assert (!_tmp44_, "! test_queue.is_full");
					_tmp45_ = test_queue;
					_tmp46_ = gee_queue_get_remaining_capacity (_tmp45_);
					_tmp47_ = _tmp46_;
					_tmp48_ = i;
					_vala_assert (_tmp47_ == _tmp48_, "test_queue.remaining_capacity == i");
				}
			}
		}
		_tmp49_ = test_queue;
		_tmp50_ = gee_collection_get_is_empty ((GeeCollection*) _tmp49_);
		_tmp51_ = _tmp50_;
		_vala_assert (_tmp51_, "test_queue.is_empty");
	}
	_g_object_unref0 (test_queue);
}


void queue_tests_test_one_element_operation (QueueTests* self) {
	GeeQueue* test_queue = NULL;
	GeeCollection* _tmp0_ = NULL;
	GeeQueue* _tmp1_ = NULL;
	GeeArrayList* recipient = NULL;
	GeeArrayList* _tmp2_ = NULL;
	gboolean _tmp3_ = FALSE;
	gpointer _tmp4_ = NULL;
	gchar* _tmp5_ = NULL;
	gint _tmp6_ = 0;
	gint _tmp7_ = 0;
	gboolean _tmp8_ = FALSE;
	gboolean _tmp9_ = FALSE;
	gpointer _tmp10_ = NULL;
	gchar* _tmp11_ = NULL;
	gint _tmp12_ = 0;
	gint _tmp13_ = 0;
	gboolean _tmp14_ = FALSE;
	gboolean _tmp15_ = FALSE;
	gpointer _tmp16_ = NULL;
	gchar* _tmp17_ = NULL;
	gpointer _tmp18_ = NULL;
	gchar* _tmp19_ = NULL;
	gboolean _tmp20_ = FALSE;
	gint _tmp21_ = 0;
	gint _tmp22_ = 0;
	gint _tmp23_ = 0;
	gboolean _tmp24_ = FALSE;
	gboolean _tmp25_ = FALSE;
	gint _tmp26_ = 0;
	gint _tmp27_ = 0;
	gpointer _tmp28_ = NULL;
	gchar* _tmp29_ = NULL;
	gint _tmp30_ = 0;
	gint _tmp31_ = 0;
	gint _tmp32_ = 0;
	gboolean _tmp33_ = FALSE;
	gboolean _tmp34_ = FALSE;
	gint _tmp35_ = 0;
	gint _tmp36_ = 0;
	gboolean _tmp37_ = FALSE;
	gint _tmp38_ = 0;
	gint _tmp39_ = 0;
	gint _tmp40_ = 0;
	gboolean _tmp41_ = FALSE;
	gboolean _tmp42_ = FALSE;
	gint _tmp43_ = 0;
	gint _tmp44_ = 0;
	gpointer _tmp45_ = NULL;
	gchar* _tmp46_ = NULL;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((CollectionTests*) self)->test_collection;
	_tmp1_ = _g_object_ref0 (G_TYPE_CHECK_INSTANCE_TYPE (_tmp0_, GEE_TYPE_QUEUE) ? ((GeeQueue*) _tmp0_) : NULL);
	test_queue = _tmp1_;
	_tmp2_ = gee_array_list_new (G_TYPE_STRING, (GBoxedCopyFunc) g_strdup, g_free, NULL, NULL, NULL);
	recipient = _tmp2_;
	_vala_assert (test_queue != NULL, "test_queue != null");
	_tmp3_ = gee_queue_offer (test_queue, "one");
	_vala_assert (_tmp3_, "test_queue.offer (\"one\")");
	_tmp4_ = gee_queue_peek (test_queue);
	_tmp5_ = (gchar*) _tmp4_;
	_vala_assert (g_strcmp0 (_tmp5_, "one") == 0, "test_queue.peek () == \"one\"");
	_g_free0 (_tmp5_);
	_tmp6_ = gee_collection_get_size ((GeeCollection*) test_queue);
	_tmp7_ = _tmp6_;
	_vala_assert (_tmp7_ == 1, "test_queue.size == 1");
	_tmp8_ = gee_collection_get_is_empty ((GeeCollection*) test_queue);
	_tmp9_ = _tmp8_;
	_vala_assert (!_tmp9_, "! test_queue.is_empty");
	_tmp10_ = gee_queue_poll (test_queue);
	_tmp11_ = (gchar*) _tmp10_;
	_vala_assert (g_strcmp0 (_tmp11_, "one") == 0, "test_queue.poll () == \"one\"");
	_g_free0 (_tmp11_);
	_tmp12_ = gee_collection_get_size ((GeeCollection*) test_queue);
	_tmp13_ = _tmp12_;
	_vala_assert (_tmp13_ == 0, "test_queue.size == 0");
	_tmp14_ = gee_collection_get_is_empty ((GeeCollection*) test_queue);
	_tmp15_ = _tmp14_;
	_vala_assert (_tmp15_, "test_queue.is_empty");
	_tmp16_ = gee_queue_peek (test_queue);
	_tmp17_ = (gchar*) _tmp16_;
	_vala_assert (_tmp17_ == NULL, "test_queue.peek () == null");
	_g_free0 (_tmp17_);
	_tmp18_ = gee_queue_poll (test_queue);
	_tmp19_ = (gchar*) _tmp18_;
	_vala_assert (_tmp19_ == NULL, "test_queue.poll () == null");
	_g_free0 (_tmp19_);
	gee_abstract_collection_clear ((GeeAbstractCollection*) recipient);
	_tmp20_ = gee_queue_offer (test_queue, "one");
	_vala_assert (_tmp20_, "test_queue.offer (\"one\")");
	_tmp21_ = gee_queue_drain (test_queue, (GeeCollection*) recipient, 1);
	_vala_assert (_tmp21_ == 1, "test_queue.drain (recipient, 1) == 1");
	_tmp22_ = gee_collection_get_size ((GeeCollection*) test_queue);
	_tmp23_ = _tmp22_;
	_vala_assert (_tmp23_ == 0, "test_queue.size == 0");
	_tmp24_ = gee_collection_get_is_empty ((GeeCollection*) test_queue);
	_tmp25_ = _tmp24_;
	_vala_assert (_tmp25_, "test_queue.is_empty");
	_tmp26_ = gee_abstract_collection_get_size ((GeeCollection*) recipient);
	_tmp27_ = _tmp26_;
	_vala_assert (_tmp27_ == 1, "recipient.size == 1");
	_tmp28_ = gee_abstract_list_get ((GeeAbstractList*) recipient, 0);
	_tmp29_ = (gchar*) _tmp28_;
	_vala_assert (g_strcmp0 (_tmp29_, "one") == 0, "recipient.get (0) == \"one\"");
	_g_free0 (_tmp29_);
	gee_abstract_collection_clear ((GeeAbstractCollection*) recipient);
	_tmp30_ = gee_queue_drain (test_queue, (GeeCollection*) recipient, 1);
	_vala_assert (_tmp30_ == 0, "test_queue.drain (recipient, 1) == 0");
	_tmp31_ = gee_collection_get_size ((GeeCollection*) test_queue);
	_tmp32_ = _tmp31_;
	_vala_assert (_tmp32_ == 0, "test_queue.size == 0");
	_tmp33_ = gee_collection_get_is_empty ((GeeCollection*) test_queue);
	_tmp34_ = _tmp33_;
	_vala_assert (_tmp34_, "test_queue.is_empty");
	_tmp35_ = gee_abstract_collection_get_size ((GeeCollection*) recipient);
	_tmp36_ = _tmp35_;
	_vala_assert (_tmp36_ == 0, "recipient.size == 0");
	gee_abstract_collection_clear ((GeeAbstractCollection*) recipient);
	_tmp37_ = gee_queue_offer (test_queue, "one");
	_vala_assert (_tmp37_, "test_queue.offer (\"one\")");
	_tmp38_ = gee_queue_drain (test_queue, (GeeCollection*) recipient, -1);
	_vala_assert (_tmp38_ == 1, "test_queue.drain (recipient) == 1");
	_tmp39_ = gee_collection_get_size ((GeeCollection*) test_queue);
	_tmp40_ = _tmp39_;
	_vala_assert (_tmp40_ == 0, "test_queue.size == 0");
	_tmp41_ = gee_collection_get_is_empty ((GeeCollection*) test_queue);
	_tmp42_ = _tmp41_;
	_vala_assert (_tmp42_, "test_queue.is_empty");
	_tmp43_ = gee_abstract_collection_get_size ((GeeCollection*) recipient);
	_tmp44_ = _tmp43_;
	_vala_assert (_tmp44_ == 1, "recipient.size == 1");
	_tmp45_ = gee_abstract_list_get ((GeeAbstractList*) recipient, 0);
	_tmp46_ = (gchar*) _tmp45_;
	_vala_assert (g_strcmp0 (_tmp46_, "one") == 0, "recipient.get (0) == \"one\"");
	_g_free0 (_tmp46_);
	_g_object_unref0 (recipient);
	_g_object_unref0 (test_queue);
}


void queue_tests_test_gobject_properties (QueueTests* self) {
	GeeQueue* test_queue = NULL;
	GeeCollection* _tmp0_ = NULL;
	GeeQueue* _tmp1_ = NULL;
	GValue value = {0};
	GValue _tmp2_ = {0};
	GValue _tmp3_ = {0};
	gint _tmp4_ = 0;
	gint _tmp5_ = 0;
	gint _tmp6_ = 0;
	GValue _tmp7_ = {0};
	GValue _tmp8_ = {0};
	gint _tmp9_ = 0;
	gint _tmp10_ = 0;
	gint _tmp11_ = 0;
	GValue _tmp12_ = {0};
	GValue _tmp13_ = {0};
	gboolean _tmp14_ = FALSE;
	gboolean _tmp15_ = FALSE;
	gboolean _tmp16_ = FALSE;
	g_return_if_fail (self != NULL);
	_tmp0_ = ((CollectionTests*) self)->test_collection;
	_tmp1_ = _g_object_ref0 (G_TYPE_CHECK_INSTANCE_TYPE (_tmp0_, GEE_TYPE_QUEUE) ? ((GeeQueue*) _tmp0_) : NULL);
	test_queue = _tmp1_;
	_vala_assert (test_queue != NULL, "test_queue != null");
	g_value_init (&_tmp2_, G_TYPE_INT);
	G_IS_VALUE (&value) ? (g_value_unset (&value), NULL) : NULL;
	value = _tmp2_;
	_tmp3_ = value;
	g_object_get_property ((GObject*) test_queue, "capacity", &value);
	_tmp4_ = g_value_get_int (&value);
	_tmp5_ = gee_queue_get_capacity (test_queue);
	_tmp6_ = _tmp5_;
	_vala_assert (_tmp4_ == _tmp6_, "value.get_int () == test_queue.capacity");
	g_value_unset (&value);
	g_value_init (&_tmp7_, G_TYPE_INT);
	G_IS_VALUE (&value) ? (g_value_unset (&value), NULL) : NULL;
	value = _tmp7_;
	_tmp8_ = value;
	g_object_get_property ((GObject*) test_queue, "remaining-capacity", &value);
	_tmp9_ = g_value_get_int (&value);
	_tmp10_ = gee_queue_get_remaining_capacity (test_queue);
	_tmp11_ = _tmp10_;
	_vala_assert (_tmp9_ == _tmp11_, "value.get_int () == test_queue.remaining_capacity");
	g_value_unset (&value);
	g_value_init (&_tmp12_, G_TYPE_BOOLEAN);
	G_IS_VALUE (&value) ? (g_value_unset (&value), NULL) : NULL;
	value = _tmp12_;
	_tmp13_ = value;
	g_object_get_property ((GObject*) test_queue, "is-full", &value);
	_tmp14_ = g_value_get_boolean (&value);
	_tmp15_ = gee_queue_get_is_full (test_queue);
	_tmp16_ = _tmp15_;
	_vala_assert (_tmp14_ == _tmp16_, "value.get_boolean () == test_queue.is_full");
	g_value_unset (&value);
	G_IS_VALUE (&value) ? (g_value_unset (&value), NULL) : NULL;
	_g_object_unref0 (test_queue);
}


static void queue_tests_class_init (QueueTestsClass * klass) {
	queue_tests_parent_class = g_type_class_peek_parent (klass);
}


static void queue_tests_instance_init (QueueTests * self) {
}


GType queue_tests_get_type (void) {
	static volatile gsize queue_tests_type_id__volatile = 0;
	if (g_once_init_enter (&queue_tests_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (QueueTestsClass), (GBaseInitFunc) NULL, (GBaseFinalizeFunc) NULL, (GClassInitFunc) queue_tests_class_init, (GClassFinalizeFunc) NULL, NULL, sizeof (QueueTests), 0, (GInstanceInitFunc) queue_tests_instance_init, NULL };
		GType queue_tests_type_id;
		queue_tests_type_id = g_type_register_static (TYPE_COLLECTION_TESTS, "QueueTests", &g_define_type_info, G_TYPE_FLAG_ABSTRACT);
		g_once_init_leave (&queue_tests_type_id__volatile, queue_tests_type_id);
	}
	return queue_tests_type_id__volatile;
}



