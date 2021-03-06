/* mapiterator.c generated by valac 0.27.1.26-9b1a5, the Vala compiler
 * generated from mapiterator.vala, do not modify */

/* mapiterator.vala
 *
 * Copyright (C) 2009  Didier Villevalois
 * Copyright (C) 2011  Maciej Piechotka
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
 * 	Didier 'Ptitjes Villevalois <ptitjes@free.fr>
 */

#include <glib.h>
#include <glib-object.h>


#define GEE_TYPE_MAP_ITERATOR (gee_map_iterator_get_type ())
#define GEE_MAP_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GEE_TYPE_MAP_ITERATOR, GeeMapIterator))
#define GEE_IS_MAP_ITERATOR(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GEE_TYPE_MAP_ITERATOR))
#define GEE_MAP_ITERATOR_GET_INTERFACE(obj) (G_TYPE_INSTANCE_GET_INTERFACE ((obj), GEE_TYPE_MAP_ITERATOR, GeeMapIteratorIface))

typedef struct _GeeMapIterator GeeMapIterator;
typedef struct _GeeMapIteratorIface GeeMapIteratorIface;
#define _a_destroy_func0(var) (((var == NULL) || (a_destroy_func == NULL)) ? NULL : (var = (a_destroy_func (var), NULL)))

typedef gpointer (*GeeFoldMapFunc) (gconstpointer k, gconstpointer v, gpointer a, void* user_data);
typedef gboolean (*GeeForallMapFunc) (gconstpointer k, gconstpointer v, void* user_data);
struct _GeeMapIteratorIface {
	GTypeInterface parent_iface;
	GType (*get_k_type) (GeeMapIterator* self);
	GBoxedCopyFunc (*get_k_dup_func) (GeeMapIterator* self);
	GDestroyNotify (*get_k_destroy_func) (GeeMapIterator* self);
	GType (*get_v_type) (GeeMapIterator* self);
	GBoxedCopyFunc (*get_v_dup_func) (GeeMapIterator* self);
	GDestroyNotify (*get_v_destroy_func) (GeeMapIterator* self);
	gboolean (*next) (GeeMapIterator* self);
	gboolean (*has_next) (GeeMapIterator* self);
	gpointer (*get_key) (GeeMapIterator* self);
	gpointer (*get_value) (GeeMapIterator* self);
	void (*set_value) (GeeMapIterator* self, gconstpointer value);
	void (*unset) (GeeMapIterator* self);
	gpointer (*fold) (GeeMapIterator* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldMapFunc f, void* f_target, gpointer seed);
	gboolean (*foreach) (GeeMapIterator* self, GeeForallMapFunc f, void* f_target);
	gboolean (*get_valid) (GeeMapIterator* self);
	gboolean (*get_mutable) (GeeMapIterator* self);
	gboolean (*get_read_only) (GeeMapIterator* self);
};



GType gee_map_iterator_get_type (void) G_GNUC_CONST;
gboolean gee_map_iterator_next (GeeMapIterator* self);
gboolean gee_map_iterator_has_next (GeeMapIterator* self);
gpointer gee_map_iterator_get_key (GeeMapIterator* self);
gpointer gee_map_iterator_get_value (GeeMapIterator* self);
void gee_map_iterator_set_value (GeeMapIterator* self, gconstpointer value);
void gee_map_iterator_unset (GeeMapIterator* self);
gpointer gee_map_iterator_fold (GeeMapIterator* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldMapFunc f, void* f_target, gpointer seed);
static gpointer gee_map_iterator_real_fold (GeeMapIterator* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldMapFunc f, void* f_target, gpointer seed);
gboolean gee_map_iterator_get_valid (GeeMapIterator* self);
gboolean gee_map_iterator_foreach (GeeMapIterator* self, GeeForallMapFunc f, void* f_target);
static gboolean gee_map_iterator_real_foreach (GeeMapIterator* self, GeeForallMapFunc f, void* f_target);
gboolean gee_map_iterator_get_mutable (GeeMapIterator* self);
gboolean gee_map_iterator_get_read_only (GeeMapIterator* self);


/**
 * Advances to the next entry in the iteration.
 *
 * @return ``true`` if the iterator has a next entry
 */
gboolean gee_map_iterator_next (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->next (self);
}


/**
 * Checks whether there is a next entry in the iteration.
 *
 * @return ``true`` if the iterator has a next entry
 */
gboolean gee_map_iterator_has_next (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->has_next (self);
}


/**
 * Returns the current key in the iteration.
 *
 * @return the current key in the iteration
 */
gpointer gee_map_iterator_get_key (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_key (self);
}


/**
 * Returns the value associated with the current key in the iteration.
 *
 * @return the value for the current key
 */
gpointer gee_map_iterator_get_value (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_value (self);
}


/**
 * Sets the value associated with the current key in the iteration.
 *
 * @param value the new value for the current key
 */
void gee_map_iterator_set_value (GeeMapIterator* self, gconstpointer value) {
	g_return_if_fail (self != NULL);
	GEE_MAP_ITERATOR_GET_INTERFACE (self)->set_value (self, value);
}


/**
 * Unsets the current entry in the iteration. The cursor is set in an
 * in-between state. {@link get_key}, {@link get_value}, {@link set_value}
 * and {@link unset} will fail until the next move of the cursor (calling
 * {@link next}).
 */
void gee_map_iterator_unset (GeeMapIterator* self) {
	g_return_if_fail (self != NULL);
	GEE_MAP_ITERATOR_GET_INTERFACE (self)->unset (self);
}


/**
 * Standard aggragation function.
 *
 * It takes a function, seed and first element, returns the new seed and
 * progress to next element when the operation repeats.
 *
 * Operation moves the iterator to last element in iteration. If iterator
 * points at some element it will be included in iteration.
 */
static gpointer gee_map_iterator_real_fold (GeeMapIterator* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldMapFunc f, void* f_target, gpointer seed) {
	gpointer result = NULL;
	gboolean _tmp0_ = FALSE;
	gboolean _tmp1_ = FALSE;
	gpointer _tmp17_ = NULL;
	_tmp0_ = gee_map_iterator_get_valid (self);
	_tmp1_ = _tmp0_;
	if (_tmp1_) {
		GeeFoldMapFunc _tmp2_ = NULL;
		void* _tmp2__target = NULL;
		gpointer _tmp3_ = NULL;
		gpointer _tmp4_ = NULL;
		gpointer _tmp5_ = NULL;
		gpointer _tmp6_ = NULL;
		gpointer _tmp7_ = NULL;
		gpointer _tmp8_ = NULL;
		_tmp2_ = f;
		_tmp2__target = f_target;
		_tmp3_ = gee_map_iterator_get_key (self);
		_tmp4_ = _tmp3_;
		_tmp5_ = gee_map_iterator_get_value (self);
		_tmp6_ = _tmp5_;
		_tmp7_ = seed;
		seed = NULL;
		_tmp8_ = _tmp2_ (_tmp4_, _tmp6_, _tmp7_, _tmp2__target);
		_a_destroy_func0 (seed);
		seed = _tmp8_;
		((_tmp6_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) == NULL)) ? NULL : (_tmp6_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) (_tmp6_), NULL));
		((_tmp4_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) == NULL)) ? NULL : (_tmp4_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) (_tmp4_), NULL));
	}
	while (TRUE) {
		gboolean _tmp9_ = FALSE;
		GeeFoldMapFunc _tmp10_ = NULL;
		void* _tmp10__target = NULL;
		gpointer _tmp11_ = NULL;
		gpointer _tmp12_ = NULL;
		gpointer _tmp13_ = NULL;
		gpointer _tmp14_ = NULL;
		gpointer _tmp15_ = NULL;
		gpointer _tmp16_ = NULL;
		_tmp9_ = gee_map_iterator_next (self);
		if (!_tmp9_) {
			break;
		}
		_tmp10_ = f;
		_tmp10__target = f_target;
		_tmp11_ = gee_map_iterator_get_key (self);
		_tmp12_ = _tmp11_;
		_tmp13_ = gee_map_iterator_get_value (self);
		_tmp14_ = _tmp13_;
		_tmp15_ = seed;
		seed = NULL;
		_tmp16_ = _tmp10_ (_tmp12_, _tmp14_, _tmp15_, _tmp10__target);
		_a_destroy_func0 (seed);
		seed = _tmp16_;
		((_tmp14_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) == NULL)) ? NULL : (_tmp14_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) (_tmp14_), NULL));
		((_tmp12_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) == NULL)) ? NULL : (_tmp12_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) (_tmp12_), NULL));
	}
	_tmp17_ = seed;
	seed = NULL;
	result = _tmp17_;
	_a_destroy_func0 (seed);
	return result;
}


gpointer gee_map_iterator_fold (GeeMapIterator* self, GType a_type, GBoxedCopyFunc a_dup_func, GDestroyNotify a_destroy_func, GeeFoldMapFunc f, void* f_target, gpointer seed) {
	g_return_val_if_fail (self != NULL, NULL);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->fold (self, a_type, a_dup_func, a_destroy_func, f, f_target, seed);
}


/**
 * Apply function to each element returned by iterator. 
 *
 * Operation moves the iterator to last element in iteration. If iterator
 * points at some element it will be included in iteration.
 */
static gboolean gee_map_iterator_real_foreach (GeeMapIterator* self, GeeForallMapFunc f, void* f_target) {
	gboolean result = FALSE;
	gboolean _tmp0_ = FALSE;
	gboolean _tmp1_ = FALSE;
	_tmp0_ = gee_map_iterator_get_valid (self);
	_tmp1_ = _tmp0_;
	if (_tmp1_) {
		GeeForallMapFunc _tmp2_ = NULL;
		void* _tmp2__target = NULL;
		gpointer _tmp3_ = NULL;
		gpointer _tmp4_ = NULL;
		gpointer _tmp5_ = NULL;
		gpointer _tmp6_ = NULL;
		gboolean _tmp7_ = FALSE;
		gboolean _tmp8_ = FALSE;
		_tmp2_ = f;
		_tmp2__target = f_target;
		_tmp3_ = gee_map_iterator_get_key (self);
		_tmp4_ = _tmp3_;
		_tmp5_ = gee_map_iterator_get_value (self);
		_tmp6_ = _tmp5_;
		_tmp7_ = _tmp2_ (_tmp4_, _tmp6_, _tmp2__target);
		_tmp8_ = !_tmp7_;
		((_tmp6_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) == NULL)) ? NULL : (_tmp6_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) (_tmp6_), NULL));
		((_tmp4_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) == NULL)) ? NULL : (_tmp4_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) (_tmp4_), NULL));
		if (_tmp8_) {
			result = FALSE;
			return result;
		}
	}
	while (TRUE) {
		gboolean _tmp9_ = FALSE;
		GeeForallMapFunc _tmp10_ = NULL;
		void* _tmp10__target = NULL;
		gpointer _tmp11_ = NULL;
		gpointer _tmp12_ = NULL;
		gpointer _tmp13_ = NULL;
		gpointer _tmp14_ = NULL;
		gboolean _tmp15_ = FALSE;
		gboolean _tmp16_ = FALSE;
		_tmp9_ = gee_map_iterator_next (self);
		if (!_tmp9_) {
			break;
		}
		_tmp10_ = f;
		_tmp10__target = f_target;
		_tmp11_ = gee_map_iterator_get_key (self);
		_tmp12_ = _tmp11_;
		_tmp13_ = gee_map_iterator_get_value (self);
		_tmp14_ = _tmp13_;
		_tmp15_ = _tmp10_ (_tmp12_, _tmp14_, _tmp10__target);
		_tmp16_ = !_tmp15_;
		((_tmp14_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) == NULL)) ? NULL : (_tmp14_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_v_destroy_func (self) (_tmp14_), NULL));
		((_tmp12_ == NULL) || (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) == NULL)) ? NULL : (_tmp12_ = (GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_k_destroy_func (self) (_tmp12_), NULL));
		if (_tmp16_) {
			result = FALSE;
			return result;
		}
	}
	result = TRUE;
	return result;
}


gboolean gee_map_iterator_foreach (GeeMapIterator* self, GeeForallMapFunc f, void* f_target) {
	g_return_val_if_fail (self != NULL, FALSE);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->foreach (self, f, f_target);
}


gboolean gee_map_iterator_get_valid (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_valid (self);
}


gboolean gee_map_iterator_get_mutable (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_mutable (self);
}


gboolean gee_map_iterator_get_read_only (GeeMapIterator* self) {
	g_return_val_if_fail (self != NULL, FALSE);
	return GEE_MAP_ITERATOR_GET_INTERFACE (self)->get_read_only (self);
}


static void gee_map_iterator_base_init (GeeMapIteratorIface * iface) {
	static gboolean initialized = FALSE;
	if (!initialized) {
		initialized = TRUE;
		/**
		 * Determines wheather the call to {@link get_key}, {@link get_value} and 
		 * {@link set_value} is legal. It is false at the beginning and after
		 * {@link unset} call and true otherwise.
		 */
		g_object_interface_install_property (iface, g_param_spec_boolean ("valid", "valid", "valid", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
		/**
		 * Determines wheather the call to {@link set_value} is legal assuming the
		 * iterator is valid. The value must not change in runtime hence the user
		 * of iterator may cache it.
		 */
		g_object_interface_install_property (iface, g_param_spec_boolean ("mutable", "mutable", "mutable", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
		/**
		 * Determines wheather the call to {@link unset} is legal assuming the
		 * iterator is valid. The value must not change in runtime hence the user
		 * of iterator may cache it.
		 */
		g_object_interface_install_property (iface, g_param_spec_boolean ("read-only", "read-only", "read-only", FALSE, G_PARAM_STATIC_NAME | G_PARAM_STATIC_NICK | G_PARAM_STATIC_BLURB | G_PARAM_READABLE));
		iface->fold = gee_map_iterator_real_fold;
		iface->foreach = gee_map_iterator_real_foreach;
	}
}


/**
 * An iterator over a map.
 *
 * Gee's iterators are "on-track" iterators. They always point to an item
 * except before the first call to {@link next}, or, when an
 * item has been removed, until the next call to {@link next}.
 *
 * Please note that when the iterator is out of track, neither {@link get_key},
 * {@link get_value}, {@link set_value} nor {@link unset} are defined and all
 * will fail. After the next call to {@link next}, they will
 * be defined again.
 */
GType gee_map_iterator_get_type (void) {
	static volatile gsize gee_map_iterator_type_id__volatile = 0;
	if (g_once_init_enter (&gee_map_iterator_type_id__volatile)) {
		static const GTypeInfo g_define_type_info = { sizeof (GeeMapIteratorIface), (GBaseInitFunc) gee_map_iterator_base_init, (GBaseFinalizeFunc) NULL, (GClassInitFunc) NULL, (GClassFinalizeFunc) NULL, NULL, 0, 0, (GInstanceInitFunc) NULL, NULL };
		GType gee_map_iterator_type_id;
		gee_map_iterator_type_id = g_type_register_static (G_TYPE_INTERFACE, "GeeMapIterator", &g_define_type_info, 0);
		g_type_interface_add_prerequisite (gee_map_iterator_type_id, G_TYPE_OBJECT);
		g_once_init_leave (&gee_map_iterator_type_id__volatile, gee_map_iterator_type_id);
	}
	return gee_map_iterator_type_id__volatile;
}



