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

#include <gmodule.h>
#include "gupnp-dlna-metadata-backend.h"

#define GET_DEFAULT_EXTRACTOR_SYMBOL "gupnp_dlna_get_default_extractor"

struct {
        GModule *module;
        GUPnPDLNAMetadataExtractor * (* get_default_extractor) (void);
} metadata_backend;

static gboolean
load_metadata_backend (void)
{
        static gsize backend_chosen = 0;

        if (g_once_init_enter (&backend_chosen)) {
                gchar **environment = g_get_environ ();
                const gchar *backend =
                               g_environ_getenv (environment,
                                                 "GUPNP_DLNA_METADATA_BACKEND");
                const gchar *backend_dir =
                           g_environ_getenv (environment,
                                             "GUPNP_DLNA_METADATA_BACKEND_DIR");
                GModule *module;
                gchar *module_path;
                gpointer get_default_extractor = NULL;
                gsize loaded = 1;

                if (!backend)
                        backend = GUPNP_DLNA_DEFAULT_METADATA_BACKEND;
                if (!backend_dir)
                        backend_dir = GUPNP_DLNA_DEFAULT_METADATA_BACKEND_DIR;
                module_path = g_module_build_path (backend_dir, backend);
                module = g_module_open (module_path, G_MODULE_BIND_MASK);

                if (!module) {
                        g_warning ("Could not load open metadata backend '%s'.",
                                   module_path);

                        goto fail;
                }
                if (!g_module_symbol (module,
                                      GET_DEFAULT_EXTRACTOR_SYMBOL,
                                      &get_default_extractor)) {
                        g_warning ("Could not find '"
                                   GET_DEFAULT_EXTRACTOR_SYMBOL
                                   "' symbol in '%s'.",
                                   module_path);

                        goto fail;
                }
                if (!get_default_extractor) {
                        g_warning ("'"
                                   GET_DEFAULT_EXTRACTOR_SYMBOL
                                   "' symbol in '%s' is invalid.",
                                   module_path);

                        goto fail;
                }
                g_module_make_resident (module);
                metadata_backend.module = module;
                metadata_backend.get_default_extractor = get_default_extractor;
                module = NULL;
                loaded = 2;
        fail:
                g_free (module_path);
                if (module)
                        g_module_close (module);
                g_strfreev (environment);
                g_once_init_leave (&backend_chosen, loaded);
        }

        return (backend_chosen == 2);
}

GUPnPDLNAMetadataExtractor *
gupnp_dlna_metadata_backend_get_extractor (void)
{
        gboolean metadata_backend_loaded = load_metadata_backend ();

        g_return_val_if_fail (metadata_backend_loaded == TRUE, NULL);

        return metadata_backend.get_default_extractor ();
}
