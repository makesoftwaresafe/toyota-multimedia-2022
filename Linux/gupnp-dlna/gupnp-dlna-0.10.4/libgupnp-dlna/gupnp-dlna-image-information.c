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
 * SECTION:gupnp-dlna-image-information
 * @short_description: Base class representing image metadata needed
 * for DLNA profiles matching.
 * @see_also: #GUPnPDLNABoolValue, #GUPnPDLNAFractionValue,
 * #GUPnPDLNAIntValue, #GUPnPDLNAStringValue
 *
 * #GUPnPDLNAImageInformation holds all image metadatas important for
 * matching profiles. Note that it does not mean all data should be
 * provided for every image file as in some cases it does not make
 * sense.
 *
 * For metadata attributes that do not exist in current image file an
 * unset value should be returned. For metadata attributes that do
 * exist a set value with proper underlying value should be
 * returned. In case metadata extractor has completely no clue how to
 * extract some metadata attribute at all, an unsupported value should
 * be returned. Note that unsupported values should be a temporary
 * mean before fixing the multimedia framework to be able to extract
 * such attribute.
 *
 * Note that gupnp_dlna_image_information_get_mime() should always
 * return a set value. Otherwise it is highly probably that the file
 * will not match against any DLNA profile.
 */

#include "gupnp-dlna-image-information.h"

G_DEFINE_ABSTRACT_TYPE (GUPnPDLNAImageInformation,
                        gupnp_dlna_image_information,
                        G_TYPE_OBJECT)

struct _GUPnPDLNAImageInformationPrivate {
        gpointer placeholder;
};

static void
gupnp_dlna_image_information_class_init
                                    (GUPnPDLNAImageInformationClass *info_class)
{
        info_class->get_depth = NULL;
        info_class->get_height = NULL;
        info_class->get_width = NULL;
        info_class->get_mime = NULL;

        g_type_class_add_private (info_class,
                                  sizeof (GUPnPDLNAImageInformationPrivate));
}

static void
gupnp_dlna_image_information_init (GUPnPDLNAImageInformation *info)
{
        GUPnPDLNAImageInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (info,
                                         GUPNP_TYPE_DLNA_IMAGE_INFORMATION,
                                         GUPnPDLNAImageInformationPrivate);

        info->priv = priv;
}

/**
 * gupnp_dlna_image_information_get_depth: (skip)
 * @info: A #GUPnPDLNAImageInformation object.
 *
 * Returns: A depth of an image.
 */
GUPnPDLNAIntValue
gupnp_dlna_image_information_get_depth (GUPnPDLNAImageInformation *info)
{
        GUPnPDLNAImageInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_IMAGE_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_IMAGE_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_IMAGE_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_depth != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_depth (info);
}

/**
 * gupnp_dlna_image_information_get_height: (skip)
 * @info: A #GUPnPDLNAImageInformation object.
 *
 * Returns: A height of an image.
 */
GUPnPDLNAIntValue
gupnp_dlna_image_information_get_height (GUPnPDLNAImageInformation *info)
{
        GUPnPDLNAImageInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_IMAGE_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_IMAGE_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_IMAGE_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_height != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_height (info);
}

/**
 * gupnp_dlna_image_information_get_width: (skip)
 * @info: A #GUPnPDLNAImageInformation object.
 *
 * Returns: A width of an image.
 */
GUPnPDLNAIntValue
gupnp_dlna_image_information_get_width (GUPnPDLNAImageInformation *info)
{
        GUPnPDLNAImageInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_IMAGE_INFORMATION (info),
                              GUPNP_DLNA_INT_VALUE_UNSET);

        info_class = GUPNP_DLNA_IMAGE_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_IMAGE_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_INT_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_width != NULL,
                              GUPNP_DLNA_INT_VALUE_UNSET);

        return info_class->get_width (info);
}

/**
 * gupnp_dlna_image_information_get_mime: (skip)
 * @info: A #GUPnPDLNAImageInformation object.
 *
 * Returns: A MIME type of an image.
 */
GUPnPDLNAStringValue
gupnp_dlna_image_information_get_mime (GUPnPDLNAImageInformation *info)
{
        GUPnPDLNAImageInformationClass *info_class;

        g_return_val_if_fail (GUPNP_IS_DLNA_IMAGE_INFORMATION (info),
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        info_class = GUPNP_DLNA_IMAGE_INFORMATION_GET_CLASS (info);

        g_return_val_if_fail
                            (GUPNP_IS_DLNA_IMAGE_INFORMATION_CLASS (info_class),
                             GUPNP_DLNA_STRING_VALUE_UNSET);
        g_return_val_if_fail (info_class->get_mime != NULL,
                              GUPNP_DLNA_STRING_VALUE_UNSET);

        return info_class->get_mime (info);
}
