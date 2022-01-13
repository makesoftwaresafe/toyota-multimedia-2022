/*
 * Copyright (C) 2010 Nokia Corporation.
 * Copyright (C) 2012, 2013 Intel Corporation.
 *
 * Authors: Arun Raghavan <arun.raghavan@collabora.co.uk>
 *          Krzesimir Nowak <krnowak@openismus.com>
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

#include <gst/pbutils/pbutils.h>
#include "gupnp-dlna-gst-metadata-extractor.h"
#include "gupnp-dlna-gst-information.h"
#include "gupnp-dlna-gst-utils.h"

G_DEFINE_TYPE (GUPnPDLNAGstMetadataExtractor,
               gupnp_dlna_gst_metadata_extractor,
               GUPNP_TYPE_DLNA_METADATA_EXTRACTOR)

struct _GUPnPDLNAGstMetadataExtractorPrivate {
        gpointer placeholder;
};

static gboolean
unref_discoverer_in_idle (GstDiscoverer *discoverer)
{
        if (discoverer)
                g_object_unref (discoverer);

        return FALSE;
}

static void
gupnp_dlna_discovered_cb (GUPnPDLNAMetadataExtractor *self,
                          GstDiscovererInfo *info,
                          GError *error,
                          gpointer user_data)
{
        GstDiscoverer *discoverer = GST_DISCOVERER (user_data);
        GUPnPDLNAInformation *gupnp_info = NULL;

        if (error)
                gupnp_info = GUPNP_DLNA_INFORMATION
                                  (gupnp_dlna_gst_information_new_empty_with_uri
                                        (gst_discoverer_info_get_uri (info)));
        else
                gupnp_info = gupnp_dlna_gst_utils_information_from_discoverer_info
                                        (info);
        gupnp_dlna_metadata_extractor_emit_done (self,
                                                 gupnp_info,
                                                 error);
        g_object_unref (gupnp_info);
        g_idle_add ((GSourceFunc) unref_discoverer_in_idle, discoverer);
}

static gboolean
backend_extract_async (GUPnPDLNAMetadataExtractor  *extractor,
                       const gchar                 *uri,
                       guint                        timeout,
                       GError                     **error)
{
        GError *gst_error = NULL;
        GstClockTime clock_time = GST_MSECOND * timeout;
        GstDiscoverer *discoverer = gst_discoverer_new (clock_time, &gst_error);

        if (gst_error) {
                g_propagate_error (error, gst_error);

                return FALSE;
        }

        g_signal_connect_swapped (discoverer,
                                  "discovered",
                                  G_CALLBACK (gupnp_dlna_discovered_cb),
                                  extractor);
        gst_discoverer_start (discoverer);

        return gst_discoverer_discover_uri_async (discoverer,
                                                  uri);
}

static GUPnPDLNAInformation *
backend_extract_sync (GUPnPDLNAMetadataExtractor  *extractor G_GNUC_UNUSED,
                      const gchar                 *uri,
                      guint                        timeout_in_ms,
                      GError                     **error)
{
        GError *gst_error = NULL;
        GstClockTime clock_time = GST_MSECOND * timeout_in_ms;
        GstDiscoverer *discoverer = gst_discoverer_new (clock_time, &gst_error);
        GstDiscovererInfo* info;
        GUPnPDLNAInformation *gupnp_info;

        if (gst_error) {
                g_propagate_error (error, gst_error);

                return NULL;
        }

        info = gst_discoverer_discover_uri (discoverer,
                                            uri,
                                            &gst_error);

        g_object_unref (discoverer);
        if (gst_error) {
                g_propagate_error (error, gst_error);

                return NULL;
        }

        gupnp_info = GUPNP_DLNA_INFORMATION
              (gupnp_dlna_gst_information_new_from_discoverer_info (uri, info));
        gst_discoverer_info_unref (info);

        return gupnp_info;
}

static void
gupnp_dlna_gst_metadata_extractor_class_init
                       (GUPnPDLNAGstMetadataExtractorClass *gst_extractor_class)
{
        GUPnPDLNAMetadataExtractorClass *extractor_class =
                      GUPNP_DLNA_METADATA_EXTRACTOR_CLASS (gst_extractor_class);

        extractor_class->extract_async = backend_extract_async;
        extractor_class->extract_sync = backend_extract_sync;

        g_type_class_add_private
                                (gst_extractor_class,
                                 sizeof (GUPnPDLNAGstMetadataExtractorPrivate));
}

static void
gupnp_dlna_gst_metadata_extractor_init (GUPnPDLNAGstMetadataExtractor *self)
{
        GUPnPDLNAGstMetadataExtractorPrivate *priv =
            G_TYPE_INSTANCE_GET_PRIVATE (self,
                                         GUPNP_TYPE_DLNA_GST_METADATA_EXTRACTOR,
                                         GUPnPDLNAGstMetadataExtractorPrivate);

        self->priv = priv;
}

GUPnPDLNAGstMetadataExtractor *
gupnp_dlna_gst_metadata_extractor_new (void)
{
        return GUPNP_DLNA_GST_METADATA_EXTRACTOR
                          (g_object_new (GUPNP_TYPE_DLNA_GST_METADATA_EXTRACTOR,
                                         NULL));
}
