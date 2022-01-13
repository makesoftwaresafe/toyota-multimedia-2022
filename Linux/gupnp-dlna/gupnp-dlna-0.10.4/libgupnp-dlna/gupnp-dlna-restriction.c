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

#include "gupnp-dlna-restriction-private.h"
#include "gupnp-dlna-value-list-private.h"

struct _GUPnPDLNARestriction {
        gchar *mime;
        GHashTable *entries; /* <gchar *, GUPnPDLNAValueList *> */
};

G_DEFINE_BOXED_TYPE (GUPnPDLNARestriction,
                     gupnp_dlna_restriction,
                     gupnp_dlna_restriction_copy,
                     gupnp_dlna_restriction_free)

GUPnPDLNARestriction *
gupnp_dlna_restriction_new (const gchar *mime)
{
        GUPnPDLNARestriction *restriction = g_slice_new (GUPnPDLNARestriction);

        restriction->mime = g_strdup (mime);
        restriction->entries = g_hash_table_new_full
                           (g_str_hash,
                            g_str_equal,
                            g_free,
                            (GDestroyNotify) gupnp_dlna_value_list_free);

        return restriction;
}

/**
 * gupnp_dlna_restriction_copy:
 * @restriction: (transfer none): A restriction to copy.
 *
 * Returns: (transfer full): A copy of @restriction.
 */
GUPnPDLNARestriction *
gupnp_dlna_restriction_copy (GUPnPDLNARestriction *restriction)
{
        GUPnPDLNARestriction *dup;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_return_val_if_fail (restriction != NULL, NULL);

        dup = gupnp_dlna_restriction_new (restriction->mime);
        g_hash_table_iter_init (&iter, restriction->entries);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                GUPnPDLNAValueList *dup_entry;

                if (key == NULL || value == NULL)
                        continue;
                dup_entry = gupnp_dlna_value_list_copy (value);

                if (dup_entry == NULL)
                        continue;
                g_hash_table_insert (dup->entries, g_strdup (key), dup_entry);
        }

        return dup;
}

/**
 * gupnp_dlna_restriction_free:
 * @restriction: A restriction to free.
 *
 * Frees @restriction.
 */
void
gupnp_dlna_restriction_free (GUPnPDLNARestriction *restriction)
{
        if (restriction == NULL)
                return;
        g_free (restriction->mime);
        g_hash_table_unref (restriction->entries);
        g_slice_free (GUPnPDLNARestriction, restriction);
}

gboolean
gupnp_dlna_restriction_add_value_list (GUPnPDLNARestriction *restriction,
                                       const gchar          *name,
                                       GUPnPDLNAValueList   *list)
{
        g_return_val_if_fail (restriction != NULL, FALSE);
        g_return_val_if_fail (name != NULL, FALSE);
        g_return_val_if_fail (list != NULL, FALSE);

        if (gupnp_dlna_value_list_is_empty (list))
                return FALSE;
        if (g_hash_table_contains (restriction->entries, name))
                return FALSE;
        gupnp_dlna_value_list_sort_items (list);
        g_hash_table_insert (restriction->entries, g_strdup (name), list);

        return TRUE;
}

void
gupnp_dlna_restriction_merge (GUPnPDLNARestriction *restriction,
                              GUPnPDLNARestriction *merged)
{
        GHashTableIter iter;
        gpointer name_ptr;
        gpointer value_list_ptr;

        g_return_if_fail (restriction != NULL);
        g_return_if_fail (merged != NULL);

        if (restriction->mime == NULL) {
                restriction->mime = merged->mime;
                merged->mime = NULL;
        }

        g_hash_table_iter_init (&iter, merged->entries);
        while (g_hash_table_iter_next (&iter,
                                       &name_ptr,
                                       &value_list_ptr)) {
                if (!g_hash_table_contains (restriction->entries, name_ptr)) {
                        g_hash_table_iter_steal (&iter);
                        g_hash_table_insert (restriction->entries,
                                             name_ptr,
                                             value_list_ptr);
                }
        }
        gupnp_dlna_restriction_free (merged);
}

/**
 * gupnp_dlna_restriction_is_empty:
 * @restriction: (transfer none): A restriction.
 *
 * Returns: %TRUE if @restriction is empty and has no MIME type,
 * otherwise %FALSE.
 */
gboolean
gupnp_dlna_restriction_is_empty (GUPnPDLNARestriction *restriction)
{
        g_return_val_if_fail (restriction != NULL, TRUE);

        return (restriction->mime == NULL &&
                g_hash_table_size (restriction->entries) == 0);
}

/**
 * gupnp_dlna_restriction_to_string:
 * @restriction: (transfer none): A restriction.
 *
 * Creates a string representation of @restriction.
 *
 * Returns: (transfer full): A string representation of
 * @restriction. Free with g_free() when not needed.
 */
gchar *
gupnp_dlna_restriction_to_string (GUPnPDLNARestriction *restriction)
{
        GString *str;
        GHashTableIter iter;
        gpointer key;
        gpointer value;

        g_return_val_if_fail (restriction != NULL, NULL);

        if (gupnp_dlna_restriction_is_empty (restriction))
                return g_strdup ("EMPTY");

        str = g_string_new (restriction->mime ? restriction->mime : "(null)");
        g_hash_table_iter_init (&iter, restriction->entries);
        while (g_hash_table_iter_next (&iter, &key, &value)) {
                gchar *raw = gupnp_dlna_value_list_to_string (value);

                g_string_append_printf (str, ", %s=%s", (gchar *) key, raw);
                g_free (raw);
        }

        return g_string_free (str, FALSE);
}

/**
 * gupnp_dlna_restriction_get_mime:
 * @restriction: (transfer none): A restriction.
 *
 * Gets @restriction's MIME type.
 *
 * Returns: (transfer none): MIME type. Do not modify.
 */
const gchar *
gupnp_dlna_restriction_get_mime (GUPnPDLNARestriction *restriction)
{
        g_return_val_if_fail (restriction != NULL, NULL);

        return restriction->mime;
}

/**
 * gupnp_dlna_restriction_get_entries:
 * @restriction: (transfer none): A restriction.
 *
 * Gets @restriction's entries. It is a string to #GUPnPDLNAValueList
 * mapping.
 *
 * Returns: (transfer none) (element-type utf8 GUPnPDLNAValueList):
 * Entries. Do not modify.
 */
GHashTable *
gupnp_dlna_restriction_get_entries (GUPnPDLNARestriction *restriction)
{
        g_return_val_if_fail (restriction != NULL, NULL);

        return restriction->entries;
}
