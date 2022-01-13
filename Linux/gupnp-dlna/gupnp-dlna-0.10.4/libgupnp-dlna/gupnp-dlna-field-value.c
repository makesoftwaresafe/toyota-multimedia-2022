/*
 * Copyright (C) 2012 Intel Corporation.
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

#include "gupnp-dlna-field-value.h"

GUPnPDLNAFieldValue *
gupnp_dlna_field_value_new_single (gchar *single)
{
        GUPnPDLNAFieldValue *value = g_slice_new (GUPnPDLNAFieldValue);

        value->type = GUPNP_DLNA_FIELD_VALUE_TYPE_SINGLE;
        value->value.single = g_strdup (single);

        return value;
}

GUPnPDLNAFieldValue *
gupnp_dlna_field_value_new_range (gchar *min,
                                  gchar *max)
{
        GUPnPDLNAFieldValue *value = g_slice_new (GUPnPDLNAFieldValue);

        value->type = GUPNP_DLNA_FIELD_VALUE_TYPE_RANGE;
        value->value.range.min = g_strdup (min);
        value->value.range.max = g_strdup (max);

        return value;
}

void
gupnp_dlna_field_value_free (GUPnPDLNAFieldValue *value)
{
        if (!value)
                return;

        switch (value->type) {
        case GUPNP_DLNA_FIELD_VALUE_TYPE_SINGLE:
                g_free (value->value.single);

                break;
        case GUPNP_DLNA_FIELD_VALUE_TYPE_RANGE:
                g_free (value->value.range.min);
                g_free (value->value.range.max);

                break;
        default:
                g_warning ("Unknown field value type.");
        }

        g_slice_free (GUPnPDLNAFieldValue, value);
}
