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

#include <glib.h>

#include "gupnp-dlna-info-set.h"
#include "gupnp-dlna-info-value.h"
#include "gupnp-dlna-value-list-private.h"

struct _GUPnPDLNAInfoSet {
        gchar *mime;
        GHashTable *entries; /* <gchar *, GUPnPDLNAInfoValue *> */
};

GUPnPDLNAInfoSet *
gupnp_dlna_info_set_new (const gchar *mime)
{
        GUPnPDLNAInfoSet *info_set;

        g_return_val_if_fail (mime != NULL, NULL);

        info_set = g_slice_new (GUPnPDLNAInfoSet);
        info_set->mime = g_strdup (mime);
        info_set->entries = g_hash_table_new_full
                           (g_str_hash,
                            g_str_equal,
                            g_free,
                            (GDestroyNotify) gupnp_dlna_info_value_free);

        return info_set;
}

void
gupnp_dlna_info_set_free (GUPnPDLNAInfoSet *info_set)
{
        if (info_set == NULL)
                return;
        g_free (info_set->mime);
        g_hash_table_unref (info_set->entries);
        g_slice_free (GUPnPDLNAInfoSet, info_set);
}

static gboolean
insert_value (GUPnPDLNAInfoSet   *info_set,
              const gchar        *name,
              GUPnPDLNAInfoValue *value)
{
        if (value == NULL) {
                g_debug ("Info set: value '%s' is NULL.", name);

                return FALSE;
        }

        if (g_hash_table_contains (info_set->entries, name)) {
                g_debug ("Info set: value '%s' already exists.", name);
                gupnp_dlna_info_value_free (value);

                return FALSE;
        }
        g_hash_table_insert (info_set->entries, g_strdup (name), value);

        return TRUE;
}

gboolean
gupnp_dlna_info_set_add_bool (GUPnPDLNAInfoSet *info_set,
                              const gchar      *name,
                              gboolean          value)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_bool (value));
}

gboolean
gupnp_dlna_info_set_add_unsupported_bool (GUPnPDLNAInfoSet *info_set,
                                          const gchar      *name)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_unsupported_bool ());
}

gboolean
gupnp_dlna_info_set_add_fraction (GUPnPDLNAInfoSet *info_set,
                                  const gchar      *name,
                                  gint              numerator,
                                  gint              denominator)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_fraction (numerator,
                                                                 denominator));
}

gboolean
gupnp_dlna_info_set_add_unsupported_fraction (GUPnPDLNAInfoSet *info_set,
                                              const gchar      *name)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_unsupported_fraction ());
}

gboolean
gupnp_dlna_info_set_add_int (GUPnPDLNAInfoSet *info_set,
                             const gchar      *name,
                             gint              value)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_int (value));
}

gboolean
gupnp_dlna_info_set_add_unsupported_int (GUPnPDLNAInfoSet *info_set,
                                         const gchar      *name)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_unsupported_int ());
}

gboolean
gupnp_dlna_info_set_add_string (GUPnPDLNAInfoSet *info_set,
                                const gchar      *name,
                                const gchar      *value)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);
        g_return_val_if_fail (value != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_string (value));
}

gboolean
gupnp_dlna_info_set_add_unsupported_string (GUPnPDLNAInfoSet *info_set,
                                            const gchar      *name)
{
        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);

        return insert_value (info_set,
                             name,
                             gupnp_dlna_info_value_new_unsupported_string ());
}

gboolean
gupnp_dlna_info_set_fits_restriction (GUPnPDLNAInfoSet     *info_set,
                                      GUPnPDLNARestriction *restriction)
{
        GHashTableIter iter;
        gpointer key;
        gpointer value;
        gboolean unsupported_match;

        g_return_val_if_fail (info_set != NULL, FALSE);
        g_return_val_if_fail (restriction != NULL, FALSE);

        if (g_strcmp0 (info_set->mime,
                       gupnp_dlna_restriction_get_mime (restriction)))
                return FALSE;

        unsupported_match = FALSE;
        g_hash_table_iter_init (&iter,
                                gupnp_dlna_restriction_get_entries
                                        (restriction));
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                GUPnPDLNAInfoValue *info_value;
                GUPnPDLNAValueList *value_list;
                gboolean unsupported;

                if (!g_hash_table_lookup_extended (info_set->entries,
                                                   key,
                                                   NULL,
                                                   (gpointer *) &info_value))
                        return FALSE;
                value_list = (GUPnPDLNAValueList *) value;
                if (!gupnp_dlna_value_list_is_superset (value_list,
                                                        info_value,
                                                        &unsupported))
                        return FALSE;
                else if (unsupported)
                        unsupported_match = TRUE;
        }

        if (unsupported_match)
                g_warning ("Info set matched restriction, but it has an "
                           "unsupported value.");

        return TRUE;
}

static gboolean
gupnp_dlna_info_set_is_empty (GUPnPDLNAInfoSet *info_set)
{
        g_return_val_if_fail (info_set != NULL, TRUE);

        return (info_set->mime == NULL &&
                g_hash_table_size (info_set->entries) == 0);
}

gchar *
gupnp_dlna_info_set_to_string (GUPnPDLNAInfoSet *info_set)
{
        GString *str;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_return_val_if_fail (info_set != NULL, NULL);

        if (gupnp_dlna_info_set_is_empty (info_set))
                return g_strdup ("EMPTY");

        str = g_string_new (info_set->mime ? info_set->mime : "(null)");
        g_hash_table_iter_init (&iter, info_set->entries);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                gchar *raw = gupnp_dlna_info_value_to_string (value);

                g_string_append_printf (str, ", %s=%s", (gchar *) key, raw);
                g_free (raw);
        }

        return g_string_free (str, FALSE);
}

const gchar *
gupnp_dlna_info_set_get_mime (GUPnPDLNAInfoSet *info_set)
{
        g_return_val_if_fail (info_set != NULL, NULL);

        return info_set->mime;
}
