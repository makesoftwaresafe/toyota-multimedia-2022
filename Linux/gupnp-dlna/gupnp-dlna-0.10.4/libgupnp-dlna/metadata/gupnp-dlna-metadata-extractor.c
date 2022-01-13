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

/**
 * SECTION:gupnp-dlna-metadata-extractor
 * @short_description: Base class for extracting metadata for given
 * media.
 *
 * #GUPnPDLNAMetadataExtractor is used by #GUPnPDLNAProfileGuesser to
 * get needed metadata for profile matching. This class itself does
 * not implement any metadata extraction features. For that it is
 * expected to provide a plugin which:
 * - Provides a #GUPnPDLNAMetadataExtractor subclass implementation.
 * - Exports a <function>gupnp_dlna_get_default_extractor</function>() which
 * takes no parameters and returns a #GUPnPDLNAMetadataExtractor.
 *
 * #GModule is used for loading the plugin, so #GModule's features can
 * be used also (like exporting <function>g_module_check_init</function>() to
 * initialize the multimedia framework used for extracting metadata).
 *
 * Default plugin directory and default plugin file is set during
 * configuration stage, but it can be overriden during runtime by
 * setting respectively <envar>GUPNP_DLNA_METADATA_BACKEND</envar> and
 * <envar>GUPNP_DLNA_METADATA_BACKEND_DIR</envar> environment variables before
 * the plugin is loaded (i.e. before #GUPnPDLNAProfileGuesser is used for
 * guessing profile).
 *
 * The <envar>GUPNP_DLNA_METADATA_BACKEND</envar> environment variable should
 * hold a name like <userinput>"gstreamer"</userinput>, so
 * <filename>libgstreamer.so</filename> will be loaded. For determining a
 * plugin filename g_module_build_path() is used.
 *
 * If subclassing #GUPnPDLNAMetadataExtractor then also
 * #GUPnPDLNAInformation, #GUPnPDLNAAudioInformation,
 * #GUPnPDLNAContainerInformation, #GUPnPDLNAImageInformation and
 * #GUPnPDLNAVideoInformation have to be subclassed as well.
 */

#include "gupnp-dlna-metadata-extractor.h"

enum {
        DONE,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

G_DEFINE_ABSTRACT_TYPE (GUPnPDLNAMetadataExtractor,
                        gupnp_dlna_metadata_extractor,
                        G_TYPE_OBJECT)

struct _GUPnPDLNAMetadataExtractorPrivate {
        gpointer placeholder;
};

static void
gupnp_dlna_metadata_extractor_class_init
                              (GUPnPDLNAMetadataExtractorClass *extractor_class)
{
        extractor_class->extract_async = NULL;
        extractor_class->extract_sync = NULL;

        /**
         * GUPnPDLNAMetadataExtractor::done:
         * @extractor: The #GUPnPDLNAMetadataExtractor.
         * @info: (transfer none): The results as #GUPnPDLNAInformation.
         * @error: (allow-none) (transfer none): Contains details of
         * the error if discovery fails, otherwise is %NULL.
         *
         * Will be emitted when all information on a URI could be
         * discovered.
         */
        signals[DONE] =
                g_signal_new ("done",
                              G_TYPE_FROM_CLASS (extractor_class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL,
                              NULL,
                              g_cclosure_marshal_generic,
                              G_TYPE_NONE,
                              2,
                              GUPNP_TYPE_DLNA_INFORMATION,
                              G_TYPE_ERROR);

        g_type_class_add_private (extractor_class,
                                  sizeof (GUPnPDLNAMetadataExtractorPrivate));
}

static void
gupnp_dlna_metadata_extractor_init (GUPnPDLNAMetadataExtractor *self)
{
        GUPnPDLNAMetadataExtractorPrivate *priv =
                G_TYPE_INSTANCE_GET_PRIVATE (self,
                                             GUPNP_TYPE_DLNA_METADATA_EXTRACTOR,
                                             GUPnPDLNAMetadataExtractorPrivate);

        self->priv = priv;
}

/**
 * gupnp_dlna_metadata_extractor_extract_async:
 * @extractor: #GUPnPDLNAMetadataExtractor object to use for discovery
 * @uri: URI to gather metadata for
 * @timeout_in_ms: Timeout in miliseconds.
 * @error: A #GError.
 *
 * Queues @uri for metadata discovery. When discovery is completed,
 * the ::done signal is emitted on @extractor.
 *
 * Returns: %TRUE if @uri was successfully queued, %FALSE otherwise.
 */
gboolean
gupnp_dlna_metadata_extractor_extract_async
                                    (GUPnPDLNAMetadataExtractor  *extractor,
                                     const gchar                 *uri,
                                     guint                        timeout_in_ms,
                                     GError                     **error)
{
        GUPnPDLNAMetadataExtractorClass *extractor_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_METADATA_EXTRACTOR (extractor),
                              FALSE);
        g_return_val_if_fail (uri != NULL, FALSE);

        extractor_class = GUPNP_DLNA_METADATA_EXTRACTOR_GET_CLASS (extractor);

        g_return_val_if_fail (GUPNP_IS_DLNA_METADATA_EXTRACTOR_CLASS
                                        (extractor_class),
                              FALSE);
        g_return_val_if_fail (extractor_class->extract_async != NULL, FALSE);

        return extractor_class->extract_async (extractor,
                                               uri,
                                               timeout_in_ms,
                                               error);
}

/**
 * gupnp_dlna_metadata_extractor_extract_sync:
 * @extractor: #GUPnPDLNAMetadataExtractor object to use for discovery
 * @uri: URI to gather metadata for
 * @timeout_in_ms: Timeout in miliseconds.
 * @error: A #GError.
 *
 * Discovers synchronously metadata of given @uri.
 *
 * Returns: A #GUPnPDLNAInformation object if discovery succeeded,
 * otherwise %NULL.
 */
GUPnPDLNAInformation *
gupnp_dlna_metadata_extractor_extract_sync
                                    (GUPnPDLNAMetadataExtractor  *extractor,
                                     const gchar                 *uri,
                                     guint                        timeout_in_ms,
                                     GError                     **error)
{
        GUPnPDLNAMetadataExtractorClass *extractor_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_METADATA_EXTRACTOR (extractor),
                              NULL);
        g_return_val_if_fail (uri != NULL, NULL);

        extractor_class = GUPNP_DLNA_METADATA_EXTRACTOR_GET_CLASS (extractor);

        g_return_val_if_fail (GUPNP_IS_DLNA_METADATA_EXTRACTOR_CLASS
                                        (extractor_class),
                              NULL);
        g_return_val_if_fail (extractor_class->extract_async != NULL, NULL);

        return extractor_class->extract_sync (extractor,
                                              uri,
                                              timeout_in_ms,
                                              error);
}

/**
 * gupnp_dlna_metadata_extractor_emit_done:
 * @extractor: A #GUPnPDLNAMetadataExtractor object.
 * @info: (transfer none): A #GUPnPDLNAInformation about discovered URI.
 * @error: (allow-none) (transfer none): A #GError.
 *
 * Emits ::done signal. This function is intended to be used by
 * subclasses of #GUPnPDLNAMetadataExtractor. It is required to always
 * pass a meaningful @info, even in case of error. That way a receiver
 * of this signal can know which URI discovery failed by using
 * gupnp_dlna_information_get_uri().
 */
void
gupnp_dlna_metadata_extractor_emit_done (GUPnPDLNAMetadataExtractor *extractor,
                                         GUPnPDLNAInformation       *info,
                                         GError                     *error)
{
        g_return_if_fail (GUPNP_IS_DLNA_METADATA_EXTRACTOR (extractor));
        g_return_if_fail (GUPNP_IS_DLNA_INFORMATION (info));

        g_signal_emit (extractor, signals[DONE], 0, info, error);
}
