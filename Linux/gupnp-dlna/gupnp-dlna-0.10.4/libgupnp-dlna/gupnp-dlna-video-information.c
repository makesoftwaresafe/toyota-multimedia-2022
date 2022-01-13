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
 * SECTION:gupnp-dlna-video-information
 * @short_description: Base class representing video metadata needed
 * for DLNA profiles matching.
 * @see_also: #GUPnPDLNABoolValue, #GUPnPDLNAFractionValue,
 * #GUPnPDLNAIntValue, #GUPnPDLNAStringValue
 *
 * #GUPnPDLNAVideoInformation holds all video metadatas important for
 * matching profiles. Note that it does not mean all data should be
 * provided for every video file as in some cases it does not make
 * sense (e.g. MPEG version does not make sense for Quicktime video
 * files).
 *
 * For metadata attributes that do not exist in current video file an
 * unset value should be returned. For metadata attributes that do
 * exist a set value with proper underlying value should be
 * returned. In case metadata extractor has completely no clue how to
 * extract some metadata attribute at all, an unsupported value should
 * be returned. Note that unsupported values should be a temporary
 * mean before fixing the multimedia framework to be able to extract
 * such attribute.
 *
 * Note that gupnp_dlna_video_information_get_mime() should always
 * return a set value. Otherwise it is highly probably that the file
 * will not match against any DLNA profile.
 */

#include "gupnp-dlna-video-information.h"

G_DEFINE_ABSTRACT_TYPE (GUPnPDLNAVideoInformation,
                        gupnp_dlna_video_information,
                        G_TYPE_OBJECT)

struct _GUPnPDLNAVideoInformationPrivate {
        gpointer placeholder;
};

static void
gupnp_dlna_video_information_class_init
                                    (GUPnPDLNAVideoInformationClass *info_class)
{
        info_class->get_bitrate = NULL;
        info_class->get_framerate = NULL;
        info_class->get_height = NULL;
        info_class->is_interlaced = NULL;
        info_class->get_level = NULL;
        info_class->get_mpeg_version = NULL;
        info_class->get_pixel_aspect_ratio = NULL;
        info_class->get_profile = NULL;
        info_class->is_system_stream = NULL;
        info_class->get_width = NULL;
        info_class->get_mime = NULL;

        g_type_class_add_private (info_class,
                                  sizeof (GUPnPDLNAVideoInformationPrivate));
}

static void
gupnp_dlna_video_information_init (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (info,
                                         GUPNP_TYPE_DLNA_VIDEO_INFORMATION,
                                         GUPnPDLNAVideoInformationPrivate);

        info->priv = priv;
}

/**
 * gupnp_dlna_video_information_get_bitrate: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A bitrate.
 */
GUPnPDLNAIntValue
gupnp_dlna_video_information_get_bitrate (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_bitrate != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_bitrate (info);
}

/**
 * gupnp_dlna_video_information_get_framerate: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A framerate.
 */
GUPnPDLNAFractionValue
gupnp_dlna_video_information_get_framerate (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_FRACTION_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_FRACTION_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_framerate != NULL,
                              GUPNP_DLNA_FRACTION_VALUE_UNSET);

        return info_class->get_framerate (info);
}

/**
 * gupnp_dlna_video_information_get_height: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A height.
 */
GUPnPDLNAIntValue
gupnp_dlna_video_information_get_height (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_height != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_height (info);
}

/**
 * gupnp_dlna_video_information_is_interlaced: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: Whether video is interlaced.
 */
GUPnPDLNABoolValue
gupnp_dlna_video_information_is_interlaced (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_BOOL_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_BOOL_VALUE_UNSET);
        g_return_val_if_fail (info_class->is_interlaced != NULL,
                              GUPNP_DLNA_BOOL_VALUE_UNSET);

        return info_class->is_interlaced (info);
}

/**
 * gupnp_dlna_video_information_get_level: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A level.
 */
GUPnPDLNAStringValue
gupnp_dlna_video_information_get_level (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_level != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_level (info);
}

/**
 * gupnp_dlna_video_information_get_mpeg_version: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: An MPEG version.
 */
GUPnPDLNAIntValue
gupnp_dlna_video_information_get_mpeg_version (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mpeg_version != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_mpeg_version (info);
}

/**
 * gupnp_dlna_video_information_get_pixel_aspect_ratio: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A pixel-aspect-ratio.
 */
GUPnPDLNAFractionValue
gupnp_dlna_video_information_get_pixel_aspect_ratio
                                        (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_FRACTION_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_FRACTION_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_pixel_aspect_ratio != NULL,
                              GUPNP_DLNA_FRACTION_VALUE_UNSET);

        return info_class->get_pixel_aspect_ratio (info);
}

/**
 * gupnp_dlna_video_information_get_profile: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A profile.
 */
GUPnPDLNAStringValue
gupnp_dlna_video_information_get_profile (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_profile != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_profile (info);
}

/**
 * gupnp_dlna_video_information_is_system_stream: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: Whether it is a system stream.
 */
GUPnPDLNABoolValue
gupnp_dlna_video_information_is_system_stream (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_BOOL_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_BOOL_VALUE_UNSET);
        g_return_val_if_fail (info_class->is_system_stream != NULL,
                              GUPNP_DLNA_BOOL_VALUE_UNSET);

        return info_class->is_system_stream (info);
}

/**
 * gupnp_dlna_video_information_get_width: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A width.
 */
GUPnPDLNAIntValue
gupnp_dlna_video_information_get_width (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_width != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_width (info);
}

/**
 * gupnp_dlna_video_information_get_mime: (skip)
 * @info: A #GUPnPDLNAVideoInformation object.
 *
 * Returns: A MIME type.
 */
GUPnPDLNAStringValue
gupnp_dlna_video_information_get_mime (GUPnPDLNAVideoInformation *info)
{
        GUPnPDLNAVideoInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_VIDEO_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_VIDEO_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_VIDEO_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mime != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_mime (info);
}
