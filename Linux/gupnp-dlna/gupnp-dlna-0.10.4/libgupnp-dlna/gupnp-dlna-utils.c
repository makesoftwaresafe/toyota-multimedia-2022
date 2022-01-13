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

#include "gupnp-dlna-utils.h"
#include "gupnp-dlna-restriction.h"

void
gupnp_dlna_utils_free_restrictions (GList *list)
{
        if (list == NULL)
                return;
        g_list_free_full (list, (GDestroyNotify) gupnp_dlna_restriction_free);
}

gchar *
gupnp_dlna_utils_restrictions_list_to_string (GList *list)
{
        GList *iter;
        GPtrArray *strings = g_ptr_array_new_with_free_func (g_free);
        gchar *final_string;

        for (iter = list; iter != NULL; iter = iter->next) {
                GUPnPDLNARestriction *restriction =
                                        GUPNP_DLNA_RESTRICTION (iter->data);

                if (restriction)
                        g_ptr_array_add (strings,
                                         gupnp_dlna_restriction_to_string
                                                            (restriction));
        }

        if (strings->len) {
                g_ptr_array_add (strings, NULL);
                final_string = g_strjoinv ("; ", (gchar **) strings->pdata);
        }
        else
                final_string = g_strdup ("EMPTY");
        g_ptr_array_unref (strings);

        return final_string;
}
