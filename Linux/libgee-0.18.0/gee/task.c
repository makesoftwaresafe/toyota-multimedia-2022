/* task.c generated by valac 0.27.1.26-9b1a5, the Vala compiler
 * generated from task.vala, do not modify */

/* task.vala
 *
 * Copyright (C) 2013  Maciej Piechotka
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
 * 	Maciej Piechotka <uzytkownik2@gmail.com>
 */

#include <glib.h>
#include <glib-object.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>


#define GEE_TYPE_FUTURE (gee_future_get_type ())
#define GEE_FUTURE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_FUTURE, GeeFuture))
#define GEE_IS_FUTURE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_FUTURE))
#define GEE_FUTURE_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_FUTURE, GeeFutureIface))

typedef struct _GeeFuture GeeFuture;
typedef struct _GeeFutureIface GeeFutureIface;
typedef struct _GeeTaskData GeeTaskData;

#define GEE_TYPE_PROMISE (gee_promise_get_type ())
#define GEE_PROMISE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_PROMISE, GeePromise))
#define GEE_PROMISE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GEE_TYPE_PROMISE, GeePromiseClass))
#define GEE_IS_PROMISE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_PROMISE))
#define GEE_IS_PROMISE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GEE_TYPE_PROMISE))
#define GEE_PROMISE_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GEE_TYPE_PROMISE, GeePromiseClass))

typedef struct _GeePromise GeePromise;
typedef struct _GeePromiseClass GeePromiseClass;
#define _gee_promise_unref0(var) ((var == NULL) ? NULL : (var = (gee_promise_unref (var), NULL)))
#define _g_object_unref0(var) ((var == NULL) ? NULL : (var = (g_object_unref (var), NULL)))
#define _gee_task_data_free0(var) ((var == NULL) ? NULL : (var = (gee_task_data_free (var), NULL)))
typedef struct _GeeAsyncTaskData GeeAsyncTaskData;
#define _g_free0(var) (var = (g_free (var), NULL))
#define _g_thread_pool_free0(var) ((var == NULL) ? NULL : (var = (g_thread_pool_free (var, FALSE, TRUE), NULL)))
#define _g_error_free0(var) ((var == NULL) ? NULL : (var = (g_error_free (var), NULL)))

typedef gpointer (*GeeTask) (void* user_data);
typedef enum  {
	GEE_FUTURE_ERROR_ABANDON_PROMISE,
	GEE_FUTURE_ERROR_EXCEPTION
} GeeFutureError;
#define GEE_FUTURE_ERROR gee_future_error_quark ()
typedef gpointer (*GeeFutureMapFunc) (gconstpointer value, void* user_data);
typedef gconstpointer (*GeeFutureLightMapFunc) (gconstpointer value, void* user_data);
typedef gpointer (*GeeFutureZipFunc) (gconstpointer a, gconstpointer b, void* user_data);
typedef GeeFuture* (*GeeFutureFlatMapFunc) (gconstpointer value, void* user_data);
struct _GeeFutureIface {
	GTypeInterface parent_iface;
	GType (*get_g_type) (GeeFuture* self);
	GBoxedCopyFunc (*get_g_dup_func) (GeeFuture* self);
	GDestroyNotify (*get_g_destroy_func) (GeeFuture* self);
	gconstpointer (*wait) (GeeFuture* self, GError** error);
	gboolean (*wait_until) (GeeFuture* self, gint64 end_time, gconstpointer* value, GError** error);
	void (*wait_async) (GeeFuture* self, GAsyncReadyCallback _callback_, gpointer _user_data_);
	gconstpointer (*wait_finish) (GeeFuture* self, GAsyncResult* _res_, GError** error);
	GeeFuture* (*map) (GeeFuture* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFutureMapFunc func, void* func_target, GDestroyNotify func_target_destroy_notify);
	GeeFuture* (*light_map) (GeeFuture* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFutureLightMapFunc func, void* func_target);
	GeeFuture* (*zip) (GeeFuture* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GType b_type, GBoxedCopyFunc b_dup_func, GDestroyNotify b_destroy_func, GeeFutureZipFunc zip_func, void* zip_func_target, GeeFuture* second);
	GeeFuture* (*flat_map) (GeeFuture* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFutureFlatMapFunc func, void* func_target, GDestroyNotify func_target_destroy_notify);
	gconstpointer (*get_value) (GeeFuture* self);
	gboolean (*get_ready) (GeeFuture* self);
	GError* (*get_exception) (GeeFuture* self);
	GeeFuture* (*light_map_fixed) (GeeFuture* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFutureLightMapFunc func, void* func_target, GDestroyNotify func_target_destroy_notify);
};

struct _GeeTaskData {
	GeeTask function;
	gpointer function_target;
	GeePromise* promise;
};

struct _GeeAsyncTaskData {
	int _state_;
	GObject* _source_object_;
	GAsyncResult* _res_;
	GSimpleAsyncResult* _async_result;
	GeeFuture* _tmp0_;
	GeeFuture* _tmp1_;
	GError * _inner_error_;
};


static GOnce gee_task_data_async_pool;
static GOnce gee_task_data_async_pool = G_ONCE_INIT;

GQuark gee_future_error_quark (void);
GType gee_future_get_type (void) G_GNUC_CONST;
GeeFuture* gee_task (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeTask task, void* task_target, GError** error);
G_GNUC_INTERNAL void gee_task_data_free (GeeTaskData* self);
G_GNUC_INTERNAL GeeTaskData* gee_task_data_new (void);
G_GNUC_INTERNAL GeeTaskData* gee_task_data_new (void);
gpointer gee_promise_ref (gpointer instance);
void gee_promise_unref (gpointer instance);
GParamSpec* gee_param_spec_promise (const gchar* name, const gchar* nick, const gchar* blurb, GType object_type, GParamFlags flags);
void gee_value_set_promise (GValue* value, gpointer v_object);
void gee_value_take_promise (GValue* value, gpointer v_object);
gpointer gee_value_get_promise (const GValue* value);
GType gee_promise_get_type (void) G_GNUC_CONST;
GeePromise* gee_promise_new (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func);
GeePromise* gee_promise_construct (GType object_type, GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func);
GeeFuture* gee_promise_get_future (GeePromise* self);
G_GNUC_INTERNAL GThreadPool* gee_task_data_get_async_pool (void);
static void gee_async_task_data_free (gpointer _data);
void gee_async_task (GAsyncReadyCallback _callback_, gpointer _user_data_);
void gee_async_task_finish (GAsyncResult* _res_, GError** error);
static gboolean gee_async_task_co (GeeAsyncTaskData* _data_);
static gpointer _gee_async_task_co_gee_task (gpointer self);
static void gee_task_data_instance_init (GeeTaskData * self);
G_GNUC_INTERNAL void gee_task_data_run (GeeTaskData* self);
void gee_promise_set_value (GeePromise* self, gpointer value);
static GThreadPool* __lambda53_ (void);
static void ___lambda54_ (GeeTaskData* tdata);
static void ____lambda54__gfunc (gpointer data, gpointer self);
static gpointer ___lambda53__gthread_func (gpointer self);


/**
 * Schedules a task to execute asynchroniously. Internally one
 * of threads from pool will execute the task.
 *
 * Note: There is limited number of threads unless environment variable
 *   ``GEE_NUM_THREADS`` is set to -1. It is not adviced to call I/O or
 *   block inside the taks. If necessary it is possible to create a new one
 *   by anyther call.
 *
 * @param task Task to be executed
 * @return Future value returned by task
 * @see async_task
 * @since 0.11.0
 */
static gpointer _g_object_ref0 (gpointer self) {
	return self ? g_object_ref (self) : NULL;
}


GeeFuture* gee_task (GType g_type, GBoxedCopyFunc g_dup_func, GDestroyNotify g_destroy_func, GeeTask task, void* task_target, GError** error) {
	GeeFuture* result = NULL;
	GeeTaskData* tdata = NULL;
	GeeTaskData* _tmp0_ = NULL;
	GeeTaskData* _tmp1_ = NULL;
	GeeTask _tmp2_ = NULL;
	void* _tmp2__target = NULL;
	GDestroyNotify _tmp2__target_destroy_notify = NULL;
	GeeTaskData* _tmp3_ = NULL;
	GeePromise* _tmp4_ = NULL;
	GeeFuture* _result_ = NULL;
	GeeTaskData* _tmp5_ = NULL;
	GeePromise* _tmp6_ = NULL;
	GeeFuture* _tmp7_ = NULL;
	GeeFuture* _tmp8_ = NULL;
	GeeFuture* _tmp9_ = NULL;
	GThreadPool* _tmp10_ = NULL;
	GeeTaskData* _tmp11_ = NULL;
	GError * _inner_error_ = NULL;
	_tmp0_ = gee_task_data_new ();
	tdata = _tmp0_;
	_tmp1_ = tdata;
	_tmp2_ = task;
	_tmp2__target = task_target;
	_tmp2__target_destroy_notify = NULL;
	_tmp1_->function = _tmp2_;
	_tmp1_->function_target = _tmp2__target;
	_tmp3_ = tdata;
	_tmp4_ = gee_promise_new (g_type, (GBoxedCopyFunc) g_dup_func, g_destroy_func);
	_gee_promise_unref0 (_tmp3_->promise);
	_tmp3_->promise = _tmp4_;
	_tmp5_ = tdata;
	_tmp6_ = _tmp5_->promise;
	_tmp7_ = gee_promise_get_future (_tmp6_);
	_tmp8_ = _tmp7_;
	_tmp9_ = _g_object_ref0 (_tmp8_);
	_result_ = _tmp9_;
	_tmp10_ = gee_task_data_get_async_pool ();
	_tmp11_ = tdata;
	tdata = NULL;
	g_thread_pool_push (_tmp10_, _tmp11_, &_inner_error_);
	if (G_UNLIKELY (_inner_error_ != NULL)) {
		if (_inner_error_->domain == G_THREAD_ERROR) {
			g_propagate_error (error, _inner_error_);
			_g_object_unref0 (_result_);
			_gee_task_data_free0 (tdata);
			return NULL;
		} else {
			_g_object_unref0 (_result_);
			_gee_task_data_free0 (tdata);
			g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
			g_clear_error (&_inner_error_);
			return NULL;
		}
	}
	result = _result_;
	_gee_task_data_free0 (tdata);
	return result;
}


static void gee_async_task_data_free (gpointer _data) {
	GeeAsyncTaskData* _data_;
	_data_ = _data;
	g_slice_free (GeeAsyncTaskData, _data_);
}


void gee_async_task (GAsyncReadyCallback _callback_, gpointer _user_data_) {
	GeeAsyncTaskData* _data_;
	_data_ = g_slice_new0 (GeeAsyncTaskData);
	_data_->_async_result = g_simple_async_result_new (NULL, _callback_, _user_data_, gee_async_task);
	g_simple_async_result_set_op_res_gpointer (_data_->_async_result, _data_, gee_async_task_data_free);
	gee_async_task_co (_data_);
}


void gee_async_task_finish (GAsyncResult* _res_, GError** error) {
	GeeAsyncTaskData* _data_;
	if (g_simple_async_result_propagate_error (G_SIMPLE_ASYNC_RESULT (_res_), error)) {
		return;
	}
	_data_ = g_simple_async_result_get_op_res_gpointer (G_SIMPLE_ASYNC_RESULT (_res_));
}


/**
 * Continues the execution asynchroniously in helper thread. Internally
 * one of threads from pool will execute the task.
 *
 * Note: There is limited number of threads unless environment variable
 *   ``GEE_NUM_THREADS`` is set to -1. It is not adviced to call I/O or
 *   block inside the taks. If necessary it is possible to create a new one
 *   by anyther call.
 *
 * @see task
 * @since 0.11.0
 */
static gpointer _gee_async_task_co_gee_task (gpointer self) {
	gpointer result;
	result = (gpointer) ((gintptr) gee_async_task_co (self));
	return result;
}


static gboolean gee_async_task_co (GeeAsyncTaskData* _data_) {
	switch (_data_->_state_) {
		case 0:
		goto _state_0;
		default:
		g_assert_not_reached ();
	}
	_state_0:
	_data_->_tmp0_ = NULL;
	_data_->_tmp0_ = gee_task (G_TYPE_BOOLEAN, NULL, NULL, _gee_async_task_co_gee_task, _data_, &_data_->_inner_error_);
	_data_->_tmp1_ = NULL;
	_data_->_tmp1_ = _data_->_tmp0_;
	_g_object_unref0 (_data_->_tmp1_);
	if (G_UNLIKELY (_data_->_inner_error_ != NULL)) {
		if (_data_->_inner_error_->domain == G_THREAD_ERROR) {
			g_simple_async_result_set_from_error (_data_->_async_result, _data_->_inner_error_);
			g_error_free (_data_->_inner_error_);
			if (_data_->_state_ == 0) {
				g_simple_async_result_complete_in_idle (_data_->_async_result);
			} else {
				g_simple_async_result_complete (_data_->_async_result);
			}
			g_object_unref (_data_->_async_result);
			return FALSE;
		} else {
			g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _data_->_inner_error_->message, g_quark_to_string (_data_->_inner_error_->domain), _data_->_inner_error_->code);
			g_clear_error (&_data_->_inner_error_);
			return FALSE;
		}
	}
	if (_data_->_state_ == 0) {
		g_simple_async_result_complete_in_idle (_data_->_async_result);
	} else {
		g_simple_async_result_complete (_data_->_async_result);
	}
	g_object_unref (_data_->_async_result);
	return FALSE;
}


G_GNUC_INTERNAL void gee_task_data_run (GeeTaskData* self) {
	GeePromise* _tmp0_ = NULL;
	GeeTask _tmp1_ = NULL;
	void* _tmp1__target = NULL;
	gpointer _tmp2_ = NULL;
	g_return_if_fail (self != NULL);
	_tmp0_ = self->promise;
	_tmp1_ = self->function;
	_tmp1__target = self->function_target;
	_tmp2_ = _tmp1_ (_tmp1__target);
	gee_promise_set_value (_tmp0_, _tmp2_);
}


static gboolean int64_try_parse (const gchar* str, gint64* _result_) {
	gint64 _vala_result = 0LL;
	gboolean result = FALSE;
	gchar* endptr = NULL;
	const gchar* _tmp0_ = NULL;
	gchar* _tmp1_ = NULL;
	gint64 _tmp2_ = 0LL;
	gchar* _tmp3_ = NULL;
	const gchar* _tmp4_ = NULL;
	const gchar* _tmp5_ = NULL;
	gint _tmp6_ = 0;
	gint _tmp7_ = 0;
	g_return_val_if_fail (str != NULL, FALSE);
	_tmp0_ = str;
	_tmp2_ = g_ascii_strtoll (_tmp0_, &_tmp1_, (guint) 0);
	endptr = _tmp1_;
	_vala_result = _tmp2_;
	_tmp3_ = endptr;
	_tmp4_ = str;
	_tmp5_ = str;
	_tmp6_ = strlen (_tmp5_);
	_tmp7_ = _tmp6_;
	if (_tmp3_ == (((gchar*) _tmp4_) + _tmp7_)) {
		result = TRUE;
		if (_result_) {
			*_result_ = _vala_result;
		}
		return result;
	} else {
		result = FALSE;
		if (_result_) {
			*_result_ = _vala_result;
		}
		return result;
	}
	if (_result_) {
		*_result_ = _vala_result;
	}
}


static void ___lambda54_ (GeeTaskData* tdata) {
	GeeTaskData* _tmp0_ = NULL;
	g_return_if_fail (tdata != NULL);
	_tmp0_ = tdata;
	gee_task_data_run (_tmp0_);
	_gee_task_data_free0 (tdata);
}


static void ____lambda54__gfunc (gpointer data, gpointer self) {
	___lambda54_ ((GeeTaskData*) data);
}


static GThreadPool* __lambda53_ (void) {
	GThreadPool* result = NULL;
	gint num_threads = 0;
	guint _tmp0_ = 0U;
	gchar* gee_num_threads_str = NULL;
	const gchar* _tmp1_ = NULL;
	gchar* _tmp2_ = NULL;
	const gchar* _tmp3_ = NULL;
	GError * _inner_error_ = NULL;
	_tmp0_ = g_get_num_processors ();
	num_threads = (gint) _tmp0_;
	_tmp1_ = g_getenv ("GEE_NUM_THREADS");
	_tmp2_ = g_strdup (_tmp1_);
	gee_num_threads_str = _tmp2_;
	_tmp3_ = gee_num_threads_str;
	if (_tmp3_ != NULL) {
		gint64 _result_ = 0LL;
		const gchar* _tmp4_ = NULL;
		gint64 _tmp5_ = 0LL;
		gboolean _tmp6_ = FALSE;
		_tmp4_ = gee_num_threads_str;
		_tmp6_ = int64_try_parse (_tmp4_, &_tmp5_);
		_result_ = _tmp5_;
		if (_tmp6_) {
			gint64 _tmp7_ = 0LL;
			_tmp7_ = _result_;
			num_threads = (gint) _tmp7_;
		}
	}
	{
		GThreadPool* _tmp8_ = NULL;
		gint _tmp9_ = 0;
		GThreadPool* _tmp10_ = NULL;
		GThreadPool* _tmp11_ = NULL;
		_tmp9_ = num_threads;
		_tmp10_ = g_thread_pool_new (____lambda54__gfunc, NULL, _tmp9_, FALSE, &_inner_error_);
		_tmp8_ = _tmp10_;
		if (G_UNLIKELY (_inner_error_ != NULL)) {
			if (_inner_error_->domain == G_THREAD_ERROR) {
				goto __catch4_g_thread_error;
			}
			_g_free0 (gee_num_threads_str);
			g_critical ("file %s: line %d: unexpected error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
			g_clear_error (&_inner_error_);
			return NULL;
		}
		_tmp11_ = _tmp8_;
		_tmp8_ = NULL;
		result = _tmp11_;
		_g_thread_pool_free0 (_tmp8_);
		_g_free0 (gee_num_threads_str);
		return result;
	}
	goto __finally4;
	__catch4_g_thread_error:
	{
		GError* err = NULL;
		err = _inner_error_;
		_inner_error_ = NULL;
		abort ();
		_g_error_free0 (err);
	}
	__finally4:
	_g_free0 (gee_num_threads_str);
	g_critical ("file %s: line %d: uncaught error: %s (%s, %d)", __FILE__, __LINE__, _inner_error_->message, g_quark_to_string (_inner_error_->domain), _inner_error_->code);
	g_clear_error (&_inner_error_);
	return NULL;
}


static gpointer ___lambda53__gthread_func (gpointer self) {
	gpointer result;
	result = __lambda53_ ();
	return result;
}


G_GNUC_INTERNAL GThreadPool* gee_task_data_get_async_pool (void) {
	GThreadPool* result = NULL;
	gconstpointer _tmp0_ = NULL;
	_tmp0_ = g_once (&gee_task_data_async_pool, ___lambda53__gthread_func, NULL);
	result = (GThreadPool*) _tmp0_;
	return result;
}


G_GNUC_INTERNAL GeeTaskData* gee_task_data_new (void) {
	GeeTaskData* self;
	self = g_slice_new0 (GeeTaskData);
	gee_task_data_instance_init (self);
	return self;
}


static void gee_task_data_instance_init (GeeTaskData * self) {
}


G_GNUC_INTERNAL void gee_task_data_free (GeeTaskData* self) {
	_gee_promise_unref0 (self->promise);
	g_slice_free (GeeTaskData, self);
}



