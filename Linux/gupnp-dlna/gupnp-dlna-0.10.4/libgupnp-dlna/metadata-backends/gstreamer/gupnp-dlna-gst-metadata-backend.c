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

#include <glib.h>
#include <gmodule.h>
#include <gst/gst.h>
#include "gupnp-dlna-gst-metadata-extractor.h"

G_MODULE_EXPORT GUPnPDLNAMetadataExtractor *
gupnp_dlna_get_default_extractor (void)
{
        return GUPNP_DLNA_METADATA_EXTRACTOR
                                     (gupnp_dlna_gst_metadata_extractor_new ());
}

G_MODULE_EXPORT const gchar *
g_module_check_init (GModule *module G_GNUC_UNUSED)
{
        if (!gst_is_initialized ())
                gst_init (NULL, NULL);

        return NULL;
}
