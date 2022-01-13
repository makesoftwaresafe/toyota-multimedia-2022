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

#include <string.h> /* for memset */

#include "gupnp-dlna-info-value.h"

/* private */
struct _GUPnPDLNAInfoValue {
        GUPnPDLNAValueType  *type;
        GUPnPDLNAValueUnion  value;
        gboolean             unsupported;
};

static GUPnPDLNAInfoValue *
value_new (GUPnPDLNAValueType *type,
           gchar *raw)
{
        GUPnPDLNAInfoValue *info_value = g_slice_new (GUPnPDLNAInfoValue);

        info_value->type = type;
        if (!gupnp_dlna_value_type_init (type, &info_value->value, raw)) {
                g_slice_free (GUPnPDLNAInfoValue, info_value);
                info_value = NULL;
        }
        else
                info_value->unsupported = FALSE;
        g_free (raw);

        return info_value;
}

static GUPnPDLNAInfoValue *
value_unsupported (GUPnPDLNAValueType *type)
{
        GUPnPDLNAInfoValue *info_value = g_slice_new (GUPnPDLNAInfoValue);

        info_value->type = type;
        memset (&info_value->value, 0, sizeof (GUPnPDLNAValueUnion));
        info_value->unsupported = TRUE;

        return info_value;
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_bool (gboolean value)
{
        return value_new (gupnp_dlna_value_type_bool (),
                          g_strdup (value ? "true" : "false"));
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_unsupported_bool (void)
{
        return value_unsupported (gupnp_dlna_value_type_bool ());
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_fraction (gint numerator,
                                    gint denominator)
{
        return value_new (gupnp_dlna_value_type_fraction (),
                          g_strdup_printf ("%d/%d",
                                           numerator,
                                           denominator));
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_unsupported_fraction (void)
{
        return value_unsupported (gupnp_dlna_value_type_fraction ());
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_int (gint value)
{
        return value_new (gupnp_dlna_value_type_int (),
                          g_strdup_printf ("%d", value));
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_unsupported_int (void)
{
        return value_unsupported (gupnp_dlna_value_type_int ());
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_string (const gchar *value)
{
        return value_new (gupnp_dlna_value_type_string (),
                          g_strdup (value));
}

GUPnPDLNAInfoValue *
gupnp_dlna_info_value_new_unsupported_string (void)
{
        return value_unsupported (gupnp_dlna_value_type_string ());
}

void
gupnp_dlna_info_value_free (GUPnPDLNAInfoValue *info_value)
{
        if (info_value == NULL)
                return;

        if (!info_value->unsupported)
                gupnp_dlna_value_type_clean (info_value->type,
                                             &info_value->value);
        g_slice_free (GUPnPDLNAInfoValue, info_value);
}

GUPnPDLNAValueType *
gupnp_dlna_info_value_get_type (GUPnPDLNAInfoValue *info_value)
{
        g_return_val_if_fail (info_value != NULL, NULL);

        return info_value->type;
}

GUPnPDLNAValueUnion *
gupnp_dlna_info_value_get_value (GUPnPDLNAInfoValue *info_value)
{
        g_return_val_if_fail (info_value != NULL, NULL);

        if (info_value->unsupported)
                return NULL;
        else
                return &info_value->value;
}

gchar *
gupnp_dlna_info_value_to_string (GUPnPDLNAInfoValue *info_value)
{
        const gchar *type;
        gchar *raw;
        gchar *str;

        g_return_val_if_fail (info_value != NULL, NULL);

        type = gupnp_dlna_value_type_name (info_value->type);
        if (info_value->unsupported)
                raw = g_strdup ("<UNSUPPORTED>");
        else
                raw = gupnp_dlna_value_type_to_string (info_value->type,
                                                       &info_value->value);
        str = g_strdup_printf ("(%s)%s", type, raw);
        g_free (raw);

        return str;
}

gboolean
gupnp_dlna_info_value_is_unsupported (GUPnPDLNAInfoValue *info_value)
{
        g_return_val_if_fail (info_value != NULL, FALSE);

        return info_value->unsupported;
}
