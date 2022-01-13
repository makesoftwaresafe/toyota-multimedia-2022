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
 * SECTION:gupnp-dlna-container-information
 * @short_description: Base class representing container metadata
 * needed for DLNA profiles matching.
 * @see_also: #GUPnPDLNABoolValue, #GUPnPDLNAFractionValue,
 * #GUPnPDLNAIntValue, #GUPnPDLNAStringValue
 *
 * #GUPnPDLNAContainerInformation holds all container metadatas
 * important for matching profiles. Note that it does not mean all
 * data should be provided for every media file as in some cases it
 * does not make sense (e.g. MPEG version does not make sense for WMA
 * media files).
 *
 * For metadata attributes that do not exist in current media file an
 * unset value should be returned. For metadata attributes that do
 * exist a set value with proper underlying value should be
 * returned. In case metadata extractor has completely no clue how to
 * extract some metadata attribute at all, an unsupported value should
 * be returned. Note that unsupported values should be a temporary
 * mean before fixing the multimedia framework to be able to extract
 * such attribute.
 *
 * Note that gupnp_dlna_container_information_get_mime() should always
 * return a set value. Otherwise it is highly probably that the file
 * will not match against any DLNA profile.
 */

#include "gupnp-dlna-container-information.h"

G_DEFINE_ABSTRACT_TYPE (GUPnPDLNAContainerInformation,
                        gupnp_dlna_container_information,
                        G_TYPE_OBJECT)

struct _GUPnPDLNAContainerInformationPrivate {
        gpointer placeholder;
};

static void
gupnp_dlna_container_information_class_init
                                (GUPnPDLNAContainerInformationClass *info_class)
{
        info_class->get_mpeg_version = NULL;
        info_class->get_packet_size = NULL;
        info_class->get_profile = NULL;
        info_class->is_system_stream = NULL;
        info_class->get_variant = NULL;
        info_class->get_mime = NULL;

        g_type_class_add_private
                                (info_class,
                                 sizeof (GUPnPDLNAContainerInformationPrivate));
}

static void
gupnp_dlna_container_information_init (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (info,
                                         GUPNP_TYPE_DLNA_CONTAINER_INFORMATION,
                                         GUPnPDLNAContainerInformationPrivate);

        info->priv = priv;
}

/**
 * gupnp_dlna_container_information_get_mpeg_version: (skip)
 * @info: A #GUPnPDLNAContainerInformation object.
 *
 * Returns: An MPEG version.
 */
GUPnPDLNAIntValue
gupnp_dlna_container_information_get_mpeg_version
                                        (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_CONTAINER_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_CONTAINER_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                        (GUPNP_IS_DLNA_CONTAINER_INFORMATION_CLASS (info_class),
                         GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mpeg_version != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_mpeg_version (info);
}

/**
 * gupnp_dlna_container_information_get_packet_size: (skip)
 * @info: A #GUPnPDLNAContainerInformation object.
 *
 * Returns: A packet size.
 */
GUPnPDLNAIntValue
gupnp_dlna_container_information_get_packet_size
                                        (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_CONTAINER_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_CONTAINER_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                        (GUPNP_IS_DLNA_CONTAINER_INFORMATION_CLASS (info_class),
                         GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_packet_size != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_packet_size (info);
}

/**
 * gupnp_dlna_container_information_get_profile: (skip)
 * @info: A #GUPnPDLNAContainerInformation object.
 *
 * Returns: A profile.
 */
GUPnPDLNAStringValue
gupnp_dlna_container_information_get_profile
                                        (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_CONTAINER_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_CONTAINER_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                        (GUPNP_IS_DLNA_CONTAINER_INFORMATION_CLASS (info_class),
                         GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_profile != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_profile (info);
}

/**
 * gupnp_dlna_container_information_is_system_stream: (skip)
 * @info: A #GUPnPDLNAContainerInformation object.
 *
 * Returns: Whether it is system stream.
 */
GUPnPDLNABoolValue
gupnp_dlna_container_information_is_system_stream
                                        (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_CONTAINER_INFORMATION (info),
                              GUPNP_DLNA_BOOL_VALUE_UNSET);

        info_class = GUPNP_DLNA_CONTAINER_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                        (GUPNP_IS_DLNA_CONTAINER_INFORMATION_CLASS (info_class),
                         GUPNP_DLNA_BOOL_VALUE_UNSET);
        g_return_val_if_fail (info_class->is_system_stream != NULL,
                              GUPNP_DLNA_BOOL_VALUE_UNSET);

        return info_class->is_system_stream (info);
}

/**
 * gupnp_dlna_container_information_get_variant: (skip)
 * @info: A #GUPnPDLNAContainerInformation object.
 *
 * Returns: A variant.
 */
GUPnPDLNAStringValue
gupnp_dlna_container_information_get_variant
                                        (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_CONTAINER_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_CONTAINER_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                        (GUPNP_IS_DLNA_CONTAINER_INFORMATION_CLASS (info_class),
                         GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_variant != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_variant (info);
}

/**
 * gupnp_dlna_container_information_get_mime: (skip)
 * @info: A #GUPnPDLNAContainerInformation object.
 *
 * Returns: A MIME type.
 */
GUPnPDLNAStringValue
gupnp_dlna_container_information_get_mime (GUPnPDLNAContainerInformation *info)
{
        GUPnPDLNAContainerInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_CONTAINER_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_CONTAINER_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                        (GUPNP_IS_DLNA_CONTAINER_INFORMATION_CLASS (info_class),
                         GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mime != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_mime (info);
}
