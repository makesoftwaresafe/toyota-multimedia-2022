/*
 * Copyright (C) 2012, 2013 Intel Corporation.
 *
 * Authors: Krzesimir Nowak <krnowak@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "gupnp-dlna-value.h"
#include "gupnp-dlna-value-union.h"

struct _GUPnPDLNAValueVTable {
        gboolean
        (* is_superset) (GUPnPDLNAValue     *base,
                         GUPnPDLNAInfoValue *info);

        GUPnPDLNAValue *
        (* copy) (GUPnPDLNAValue     *base,
                  GUPnPDLNAValueType *type);

        void
        (* free) (GUPnPDLNAValue     *base,
                  GUPnPDLNAValueType *type);

        gchar *
        (* to_string) (GUPnPDLNAValue     *base,
                       GUPnPDLNAValueType *type);

        GUPnPDLNAValueUnion *
        (* get_sort_value) (GUPnPDLNAValue *base);

        gboolean
        (* to_g_value) (GUPnPDLNAValue     *base,
                        GUPnPDLNAValueType *type,
                        GValue             *g_value);
};

/* single */
typedef struct _GUPnPDLNAValueSingle GUPnPDLNAValueSingle;

struct _GUPnPDLNAValueSingle {
        GUPnPDLNAValue      base;
        GUPnPDLNAValueUnion value;
};

static gboolean
single_is_superset (GUPnPDLNAValue     *base,
                    GUPnPDLNAInfoValue *info);

static GUPnPDLNAValue *
single_copy (GUPnPDLNAValue     *base,
             GUPnPDLNAValueType *type);

static void
single_free (GUPnPDLNAValue     *base,
             GUPnPDLNAValueType *type);

static gchar *
single_to_string (GUPnPDLNAValue     *base,
                  GUPnPDLNAValueType *type);

static GUPnPDLNAValueUnion *
single_get_sort_value (GUPnPDLNAValue *base);

static gboolean
single_to_g_value (GUPnPDLNAValue     *base,
                   GUPnPDLNAValueType *type,
                   GValue             *g_value);

static GUPnPDLNAValueVTable single_vtable = {
        single_is_superset,
        single_copy,
        single_free,
        single_to_string,
        single_get_sort_value,
        single_to_g_value
};

static gboolean
single_is_superset (GUPnPDLNAValue     *base,
                    GUPnPDLNAInfoValue *info)
{
        GUPnPDLNAValueSingle *value = (GUPnPDLNAValueSingle *) base;
        GUPnPDLNAValueType *info_type = gupnp_dlna_info_value_get_type (info);
        GUPnPDLNAValueUnion *info_value =
                                        gupnp_dlna_info_value_get_value (info);

        return gupnp_dlna_value_type_is_equal (info_type,
                                               &value->value,
                                               info_value);
}

static GUPnPDLNAValue *
single_copy (GUPnPDLNAValue     *base,
             GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueSingle *value = (GUPnPDLNAValueSingle *) base;
        GUPnPDLNAValueSingle *dup = g_slice_new (GUPnPDLNAValueSingle);

        dup->base.vtable = &single_vtable;
        if (!gupnp_dlna_value_type_copy (type, &value->value, &dup->value)) {
                g_slice_free (GUPnPDLNAValueSingle, dup);
                dup = NULL;
        }

        return (GUPnPDLNAValue *) dup;
}

static void
single_free (GUPnPDLNAValue     *base,
             GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueSingle *value = (GUPnPDLNAValueSingle *) base;

        gupnp_dlna_value_type_clean (type, &value->value);
        g_slice_free (GUPnPDLNAValueSingle, value);
}

static gchar *
single_to_string (GUPnPDLNAValue     *base,
                  GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueSingle *value = (GUPnPDLNAValueSingle *) base;

        return gupnp_dlna_value_type_to_string (type, &value->value);
}

static GUPnPDLNAValueUnion *
single_get_sort_value (GUPnPDLNAValue *base)
{
        GUPnPDLNAValueSingle *value = (GUPnPDLNAValueSingle *) base;

        return &value->value;
}

static gboolean
single_to_g_value (GUPnPDLNAValue     *base,
                   GUPnPDLNAValueType *type,
                   GValue             *g_value)
{
        GUPnPDLNAValueSingle *value = (GUPnPDLNAValueSingle *) base;

        gupnp_dlna_value_type_to_g_value (type, &value->value, g_value);

        return TRUE;
}

/* range */
typedef struct _GUPnPDLNAValueRange GUPnPDLNAValueRange;

struct _GUPnPDLNAValueRange {
        GUPnPDLNAValue base;
        GUPnPDLNAValueUnion min;
        GUPnPDLNAValueUnion max;
};

static gboolean
range_is_superset (GUPnPDLNAValue     *base,
                   GUPnPDLNAInfoValue *info);

static GUPnPDLNAValue *
range_copy (GUPnPDLNAValue     *base,
            GUPnPDLNAValueType *type);

static void
range_free (GUPnPDLNAValue     *base,
            GUPnPDLNAValueType *type);

static gchar *
range_to_string (GUPnPDLNAValue     *base,
                 GUPnPDLNAValueType *type);

static GUPnPDLNAValueUnion *
range_get_sort_value (GUPnPDLNAValue *base);

static gboolean
range_to_g_value (GUPnPDLNAValue     *base,
                  GUPnPDLNAValueType *type,
                  GValue             *g_value);

static GUPnPDLNAValueVTable range_vtable = {
        range_is_superset,
        range_copy,
        range_free,
        range_to_string,
        range_get_sort_value,
        range_to_g_value
};

static gboolean
range_is_superset (GUPnPDLNAValue     *base,
                   GUPnPDLNAInfoValue *info)
{
        GUPnPDLNAValueRange *range = (GUPnPDLNAValueRange *) base;
        GUPnPDLNAValueType *info_type = gupnp_dlna_info_value_get_type (info);
        GUPnPDLNAValueUnion *info_value =
                                        gupnp_dlna_info_value_get_value (info);

        return gupnp_dlna_value_type_is_in_range (info_type,
                                                  &range->min,
                                                  &range->max,
                                                  info_value);
}

static GUPnPDLNAValue *
range_copy (GUPnPDLNAValue     *base,
            GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueRange *range = (GUPnPDLNAValueRange *) base;
        GUPnPDLNAValueRange *dup = g_slice_new (GUPnPDLNAValueRange);

        dup->base.vtable = &range_vtable;
        if (!gupnp_dlna_value_type_copy (type, &range->min, &dup->min)) {
                g_slice_free (GUPnPDLNAValueRange, dup);
                dup = NULL;
        }
        if (dup && !gupnp_dlna_value_type_copy (type,
                                                &range->max,
                                                &dup->max)) {
                gupnp_dlna_value_type_clean (type, &dup->min);
                g_slice_free (GUPnPDLNAValueRange, dup);
                dup = NULL;
        }

        return (GUPnPDLNAValue *) dup;
}

static void
range_free (GUPnPDLNAValue     *base,
            GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueRange *range = (GUPnPDLNAValueRange *) base;

        gupnp_dlna_value_type_clean (type, &range->min);
        gupnp_dlna_value_type_clean (type, &range->max);
        g_slice_free (GUPnPDLNAValueRange, range);
}

static gchar *
range_to_string (GUPnPDLNAValue     *base,
                 GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueRange *range = (GUPnPDLNAValueRange *) base;
        gchar *str;
        gchar *min = gupnp_dlna_value_type_to_string (type, &range->min);
        gchar *max = gupnp_dlna_value_type_to_string (type, &range->max);

        str = g_strdup_printf ("[ %s, %s ]", min, max);
        g_free (min);
        g_free (max);

        return str;
}

static GUPnPDLNAValueUnion *
range_get_sort_value (GUPnPDLNAValue *base)
{
        GUPnPDLNAValueRange *range = (GUPnPDLNAValueRange *) base;

        return &range->min;
}

static gboolean
range_to_g_value (GUPnPDLNAValue     *base,
                  GUPnPDLNAValueType *type,
                  GValue             *g_value)
{
        GValue from = G_VALUE_INIT;
        GValue to = G_VALUE_INIT;
        GUPnPDLNAValueRange *range = (GUPnPDLNAValueRange *) base;
        gboolean result;

        gupnp_dlna_value_type_to_g_value (type, &range->min, &from);
        gupnp_dlna_value_type_to_g_value (type, &range->max, &to);
        result = gupnp_dlna_value_type_flatten (type, g_value, &from, &to);
        g_value_unset (&from);
        g_value_unset (&to);

        return result;
}

/* API */

GUPnPDLNAValue *
gupnp_dlna_value_new_single (GUPnPDLNAValueType *type,
                             const gchar        *raw)
{
        GUPnPDLNAValueSingle *value;

        g_return_val_if_fail (type != NULL, NULL);
        g_return_val_if_fail (raw != NULL, NULL);

        value = g_slice_new (GUPnPDLNAValueSingle);
        value->base.vtable = &single_vtable;
        if (!gupnp_dlna_value_type_init (type, &value->value, raw)) {
                g_slice_free (GUPnPDLNAValueSingle, value);
                value = NULL;
        }

        return (GUPnPDLNAValue *) value;
}

GUPnPDLNAValue *
gupnp_dlna_value_new_ranged (GUPnPDLNAValueType *type,
                             const gchar        *min,
                             const gchar        *max)
{
        GUPnPDLNAValueRange *range;

        g_return_val_if_fail (type != NULL, NULL);
        g_return_val_if_fail (min != NULL, NULL);
        g_return_val_if_fail (max != NULL, NULL);

        range = g_slice_new (GUPnPDLNAValueRange);
        range->base.vtable = &range_vtable;
        if (!gupnp_dlna_value_type_init (type, &range->min, min)) {
                g_slice_free (GUPnPDLNAValueRange, range);
                range = NULL;
        }
        if (range && !gupnp_dlna_value_type_init (type, &range->max, max)) {
                gupnp_dlna_value_type_clean (type, &range->min);
                g_slice_free (GUPnPDLNAValueRange, range);
                range = NULL;
        }
        if (range && !gupnp_dlna_value_type_verify_range (type,
                                                          &range->min,
                                                          &range->max)) {
                gupnp_dlna_value_type_clean (type, &range->min);
                gupnp_dlna_value_type_clean (type, &range->max);
                g_slice_free (GUPnPDLNAValueRange, range);
                range = NULL;
        }

        return (GUPnPDLNAValue *) range;
}

gboolean
gupnp_dlna_value_is_superset (GUPnPDLNAValue     *base,
                              GUPnPDLNAInfoValue *single)
{
        g_return_val_if_fail (base != NULL, FALSE);
        g_return_val_if_fail (single != NULL, FALSE);
        g_return_val_if_fail (base->vtable != NULL, FALSE);
        g_return_val_if_fail (base->vtable->is_superset != NULL, FALSE);

        return base->vtable->is_superset (base, single);
}

GUPnPDLNAValue *
gupnp_dlna_value_copy (GUPnPDLNAValue     *base,
                       GUPnPDLNAValueType *type)
{
        g_return_val_if_fail (base != NULL, NULL);
        g_return_val_if_fail (type != NULL, NULL);
        g_return_val_if_fail (base->vtable != NULL, NULL);
        g_return_val_if_fail (base->vtable->copy != NULL, NULL);

        return base->vtable->copy (base, type);
}

void
gupnp_dlna_value_free (GUPnPDLNAValue     *base,
                       GUPnPDLNAValueType *type)
{
        if (base == NULL)
                return;

        g_return_if_fail (type != NULL);
        g_return_if_fail (base->vtable != NULL);
        g_return_if_fail (base->vtable->free != NULL);

        base->vtable->free (base, type);
}

gchar *
gupnp_dlna_value_to_string (GUPnPDLNAValue     *base,
                            GUPnPDLNAValueType *type)
{
        g_return_val_if_fail (base != NULL, NULL);
        g_return_val_if_fail (type != NULL, NULL);
        g_return_val_if_fail (base->vtable != NULL, NULL);
        g_return_val_if_fail (base->vtable->to_string != NULL, NULL);

        return base->vtable->to_string (base, type);
}

static GUPnPDLNAValueUnion *
gupnp_dlna_value_get_sort_value (GUPnPDLNAValue *base)
{
        g_return_val_if_fail (base->vtable != NULL, NULL);
        g_return_val_if_fail (base->vtable->get_sort_value != NULL, NULL);

        return base->vtable->get_sort_value (base);
}

gint
gupnp_dlna_value_compare (GUPnPDLNAValue     *base,
                          GUPnPDLNAValue     *other,
                          GUPnPDLNAValueType *type)
{
        GUPnPDLNAValueUnion *base_union;
        GUPnPDLNAValueUnion *other_union;

        g_return_val_if_fail (base != NULL, 0);
        g_return_val_if_fail (other != NULL, 0);
        g_return_val_if_fail (type != NULL, 0);

        base_union = gupnp_dlna_value_get_sort_value (base);
        other_union = gupnp_dlna_value_get_sort_value (other);

        return gupnp_dlna_value_type_compare (type, base_union, other_union);
}

GValue *
gupnp_dlna_value_to_g_value (GUPnPDLNAValue     *base,
                             GUPnPDLNAValueType *type)
{
        GValue *g_value;

        g_return_val_if_fail (base->vtable != NULL, NULL);
        g_return_val_if_fail (base->vtable->to_g_value != NULL, NULL);

        g_value = g_new0 (GValue, 1);

        if (!base->vtable->to_g_value (base, type, g_value)) {
                g_free (g_value);
                g_value = NULL;
        }

        return g_value;
}
