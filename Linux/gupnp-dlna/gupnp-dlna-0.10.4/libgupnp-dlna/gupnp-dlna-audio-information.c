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

/**
 * SECTION:gupnp-dlna-audio-information
 * @short_description: Base class representing audio metadata needed
 * for DLNA profiles matching.
 * @see_also: #GUPnPDLNABoolValue, #GUPnPDLNAFractionValue,
 * #GUPnPDLNAIntValue, #GUPnPDLNAStringValue
 *
 * #GUPnPDLNAAudioInformation holds all audio metadatas important for
 * matching profiles. Note that it does not mean all data should be
 * provided for every audio file as in some cases it does not make
 * sense (e.g. WMA version does not make sense for MPEG audio files).
 *
 * For metadata attributes that do not exist in current audio file an
 * unset value should be returned. For metadata attributes that do
 * exist a set value with proper underlying value should be
 * returned. In case metadata extractor has completely no clue how to
 * extract some metadata attribute at all, an unsupported value should
 * be returned. Note that unsupported values should be a temporary
 * mean before fixing the multimedia framework to be able to extract
 * such attribute.
 *
 * Note that gupnp_dlna_audio_information_get_mime() should always
 * return a set value. Otherwise it is highly probably that the file
 * will not match against any DLNA profile.
 */

#include "gupnp-dlna-audio-information.h"

G_DEFINE_ABSTRACT_TYPE (GUPnPDLNAAudioInformation,
                        gupnp_dlna_audio_information,
                        G_TYPE_OBJECT)

struct _GUPnPDLNAAudioInformationPrivate {
        gpointer placeholder;
};

static void
gupnp_dlna_audio_information_class_init
                                    (GUPnPDLNAAudioInformationClass *info_class)
{
        info_class->get_bitrate = NULL;
        info_class->get_channels = NULL;
        info_class->get_depth = NULL;
        info_class->get_layer = NULL;
        info_class->get_level = NULL;
        info_class->get_mpeg_audio_version = NULL;
        info_class->get_mpeg_version = NULL;
        info_class->get_profile = NULL;
        info_class->get_rate = NULL;
        info_class->get_stream_format = NULL;
        info_class->get_wma_version = NULL;
        info_class->get_mime = NULL;

        g_type_class_add_private (info_class,
                                  sizeof (GUPnPDLNAAudioInformationPrivate));
}

static void
gupnp_dlna_audio_information_init (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (info,
                                         GUPNP_TYPE_DLNA_AUDIO_INFORMATION,
                                         GUPnPDLNAAudioInformationPrivate);

        info->priv = priv;
}

/**
 * gupnp_dlna_audio_information_get_bitrate: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A bitrate.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_bitrate (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_bitrate != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_bitrate (info);
}

/**
 * gupnp_dlna_audio_information_get_channels: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A number of channels.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_channels (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_channels != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_channels (info);
}

/**
 * gupnp_dlna_audio_information_get_depth: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A depth.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_depth (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_depth != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_depth (info);
}

/**
 * gupnp_dlna_audio_information_get_layer: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A layer.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_layer (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_layer != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_layer (info);
}

/**
 * gupnp_dlna_audio_information_get_level: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A level.
 */
GUPnPDLNAStringValue
gupnp_dlna_audio_information_get_level (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_level != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_level (info);
}

/**
 * gupnp_dlna_audio_information_get_mpeg_audio_version: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: An MPEG audio version.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_mpeg_audio_version
                                        (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mpeg_audio_version != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_mpeg_audio_version (info);
}

/**
 * gupnp_dlna_audio_information_get_mpeg_version: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: An MPEG version.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_mpeg_version (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mpeg_version != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_mpeg_version (info);
}

/**
 * gupnp_dlna_audio_information_get_profile: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A profile.
 */
GUPnPDLNAStringValue
gupnp_dlna_audio_information_get_profile (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_profile != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_profile (info);
}

/**
 * gupnp_dlna_audio_information_get_rate: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A sample rate.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_rate (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_rate != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_rate (info);
}

/**
 * gupnp_dlna_audio_information_get_stream_format: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A stream format.
 */
GUPnPDLNAStringValue
gupnp_dlna_audio_information_get_stream_format (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_stream_format != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_stream_format (info);
}

/**
 * gupnp_dlna_audio_information_get_wma_version: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A WMA version.
 */
GUPnPDLNAIntValue
gupnp_dlna_audio_information_get_wma_version (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_wma_version != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_wma_version (info);
}

/**
 * gupnp_dlna_audio_information_get_mime: (skip)
 * @info: A #GUPnPDLNAAudioInformation object.
 *
 * Returns: A MIME type.
 */
GUPnPDLNAStringValue
gupnp_dlna_audio_information_get_mime (GUPnPDLNAAudioInformation *info)
{
        GUPnPDLNAAudioInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_AUDIO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_AUDIO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_AUDIO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mime != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_mime (info);
}
