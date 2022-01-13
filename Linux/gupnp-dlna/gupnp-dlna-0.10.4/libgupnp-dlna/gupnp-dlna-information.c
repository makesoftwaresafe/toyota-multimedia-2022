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

/**
 * SECTION:gupnp-dlna-information
 * @short_description: Base class for storing various types of
 * metadata informations.
 *
 * Subclasses of #GUPnPDLNAInformation should override all virtual
 * functions provided by this class. The overrides should return a
 * subclasses of specific information base classes.
 *
 * When instantiating a subclass of #GUPnPDLNAInformation make sure
 * that "uri" with a URI to media file is passed to g_object_new().
 */

#include "gupnp-dlna-information.h"

G_DEFINE_ABSTRACT_TYPE (GUPnPDLNAInformation,
                        gupnp_dlna_information,
                        G_TYPE_OBJECT)

struct _GUPnPDLNAInformationPrivate {
        gchar* uri;
        gboolean got_audio_info;
        gboolean got_container_info;
        gboolean got_image_info;
        gboolean got_video_info;
        GUPnPDLNAAudioInformation *audio_info;
        GUPnPDLNAContainerInformation *container_info;
        GUPnPDLNAImageInformation *image_info;
        GUPnPDLNAVideoInformation *video_info;
};

enum {
        PROP_0,

        PROP_URI,
        PROP_AUDIO_INFO,
        PROP_CONTAINER_INFO,
        PROP_IMAGE_INFO,
        PROP_VIDEO_INFO
};

static const gchar *
gupnp_dlna_information_get_profile_name_default (GUPnPDLNAInformation *info G_GNUC_UNUSED)
{
        return NULL;
}

static void
gupnp_dlna_information_dispose (GObject *object)
{
        GUPnPDLNAInformation *info = GUPNP_DLNA_INFORMATION (object);
        GUPnPDLNAInformationPrivate *priv = info->priv;

        g_clear_object (&priv->audio_info);
        g_clear_object (&priv->container_info);
        g_clear_object (&priv->image_info);
        g_clear_object (&priv->video_info);
        G_OBJECT_CLASS (gupnp_dlna_information_parent_class)->dispose (object);
}

static void
gupnp_dlna_information_finalize (GObject *object)
{
        GUPnPDLNAInformation *info = GUPNP_DLNA_INFORMATION (object);

        g_free (info->priv->uri);
        G_OBJECT_CLASS (gupnp_dlna_information_parent_class)->finalize (object);
}

static void
gupnp_dlna_information_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        GUPnPDLNAInformation *info = GUPNP_DLNA_INFORMATION (object);
        GUPnPDLNAInformationPrivate *priv = info->priv;

        switch (property_id) {
        case PROP_URI:
                g_free (priv->uri);
                priv->uri = g_value_dup_string (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_dlna_information_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
        GUPnPDLNAInformation *info = GUPNP_DLNA_INFORMATION (object);
        GUPnPDLNAInformationPrivate *priv = info->priv;

        switch (property_id) {
        case PROP_URI:
                g_value_set_string (value, priv->uri);

                break;
        case PROP_AUDIO_INFO:
                g_value_set_object
                          (value,
                           gupnp_dlna_information_get_audio_information (info));

                break;
        case PROP_CONTAINER_INFO:
                g_value_set_object
                      (value,
                       gupnp_dlna_information_get_container_information (info));

                break;
        case PROP_IMAGE_INFO:
                g_value_set_object
                          (value,
                           gupnp_dlna_information_get_image_information (info));

                break;
        case PROP_VIDEO_INFO:
                g_value_set_object
                          (value,
                           gupnp_dlna_information_get_video_information (info));

                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);

                break;
        }
}

static void
gupnp_dlna_information_class_init (GUPnPDLNAInformationClass *info_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (info_class);
        GParamSpec *pspec;

        object_class->dispose = gupnp_dlna_information_dispose;
        object_class->finalize = gupnp_dlna_information_finalize;
        object_class->set_property = gupnp_dlna_information_set_property;
        object_class->get_property = gupnp_dlna_information_get_property;
        info_class->get_container_information = NULL;
        info_class->get_image_information = NULL;
        info_class->get_video_information = NULL;
        info_class->get_audio_information = NULL;
        info_class->get_profile_name = gupnp_dlna_information_get_profile_name_default;

        /**
         * GUPnPDLNAInformation:uri:
         *
         * URI of file which metadata this object stores.
         */
        pspec = g_param_spec_string ("uri",
                                     "uri",
                                     "URI of file which metadata this object "
                                     "stores",
                                     NULL,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_URI, pspec);

        /**
         * GUPnPDLNAInformation:audio-information:
         *
         * Audio information of a file.
         */
        pspec = g_param_spec_object ("audio-information",
                                     "Audio information",
                                     "Audio information of a file",
                                     GUPNP_TYPE_DLNA_AUDIO_INFORMATION,
                                     G_PARAM_READABLE);
        g_object_class_install_property (object_class, PROP_AUDIO_INFO, pspec);

        /**
         * GUPnPDLNAInformation:container-information:
         *
         * Container information of a file.
         */
        pspec = g_param_spec_object ("container-information",
                                     "Container information",
                                     "Container information of a file",
                                     GUPNP_TYPE_DLNA_CONTAINER_INFORMATION,
                                     G_PARAM_READABLE);
        g_object_class_install_property (object_class,
                                         PROP_CONTAINER_INFO,
                                         pspec);

        /**
         * GUPnPDLNAInformation:image-information:
         *
         * Image information of a file.
         */
        pspec = g_param_spec_object ("image-information",
                                     "Image information",
                                     "Image information of a file",
                                     GUPNP_TYPE_DLNA_IMAGE_INFORMATION,
                                     G_PARAM_READABLE);
        g_object_class_install_property (object_class, PROP_IMAGE_INFO, pspec);

        /**
         * GUPnPDLNAInformation:video-information:
         *
         * Video information of a file.
         */
        pspec = g_param_spec_object ("video-information",
                                     "Video information",
                                     "Video information of a file",
                                     GUPNP_TYPE_DLNA_VIDEO_INFORMATION,
                                     G_PARAM_READABLE);
        g_object_class_install_property (object_class, PROP_VIDEO_INFO, pspec);

        g_type_class_add_private (info_class,
                                  sizeof (GUPnPDLNAInformationPrivate));
}

static void
gupnp_dlna_information_init (GUPnPDLNAInformation *info)
{
        GUPnPDLNAInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (info,
                                         GUPNP_TYPE_DLNA_INFORMATION,
                                         GUPnPDLNAInformationPrivate);

        priv->uri = NULL;
        priv->got_audio_info = FALSE;
        priv->got_container_info = FALSE;
        priv->got_image_info = FALSE;
        priv->got_video_info = FALSE;
        priv->audio_info = NULL;
        priv->container_info = NULL;
        priv->image_info = NULL;
        priv->video_info = NULL;
        info->priv = priv;
}

/**
 * gupnp_dlna_information_get_audio_information:
 * @info: A #GUPnPDLNAInformation object.
 *
 * Get an audio information of media file if applicable (e.g. for
 * video and audio files).
 *
 * Returns: (transfer none): A #GUPnPDLNAAudioInformation object or %NULL.
 */
GUPnPDLNAAudioInformation *
gupnp_dlna_information_get_audio_information (GUPnPDLNAInformation *info)
{
        GUPnPDLNAInformationPrivate *priv;

        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        priv = info->priv;
        if (!priv->got_audio_info) {
                GUPnPDLNAInformationClass *info_class;

                info_class = GUPNP_DLNA_INFORMATION_GET_CLASS (info);

                g_return_val_if_fail
                                  (GUPNP_IS_DLNA_INFORMATION_CLASS (info_class),
                                   NULL);
                g_return_val_if_fail (info_class->get_audio_information != NULL,
                                      NULL);

                priv->audio_info = info_class->get_audio_information (info);
                priv->got_audio_info = TRUE;
        }

        return priv->audio_info;
}

/**
 * gupnp_dlna_information_get_container_information:
 * @info: A #GUPnPDLNAInformation object.
 *
 * Get an container information of media file if applicable (e.g. for
 * video and audio files).
 *
 * Returns: (transfer none): A #GUPnPDLNAContainerInformation object or %NULL.
 */
GUPnPDLNAContainerInformation *
gupnp_dlna_information_get_container_information (GUPnPDLNAInformation *info)
{
        GUPnPDLNAInformationPrivate *priv;

        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        priv = info->priv;
        if (!priv->got_container_info) {
                GUPnPDLNAInformationClass *info_class;

                info_class = GUPNP_DLNA_INFORMATION_GET_CLASS (info);

                g_return_val_if_fail
                                  (GUPNP_IS_DLNA_INFORMATION_CLASS (info_class),
                                   NULL);
                g_return_val_if_fail
                                 (info_class->get_container_information != NULL,
                                  NULL);

                priv->container_info =
                                   info_class->get_container_information (info);
                priv->got_container_info = TRUE;
        }

        return priv->container_info;
}

/**
 * gupnp_dlna_information_get_image_information:
 * @info: A #GUPnPDLNAInformation object.
 *
 * Get an container information of media file if applicable (e.g. for
 * image files).
 *
 * Returns: (transfer none): A #GUPnPDLNAImageInformation object or %NULL.
 */
GUPnPDLNAImageInformation *
gupnp_dlna_information_get_image_information (GUPnPDLNAInformation *info)
{
        GUPnPDLNAInformationPrivate *priv;

        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        priv = info->priv;
        if (!priv->got_image_info) {
                GUPnPDLNAInformationClass *info_class;

                info_class = GUPNP_DLNA_INFORMATION_GET_CLASS (info);

                g_return_val_if_fail
                                  (GUPNP_IS_DLNA_INFORMATION_CLASS (info_class),
                                   NULL);
                g_return_val_if_fail (info_class->get_image_information != NULL,
                                      NULL);

                priv->image_info = info_class->get_image_information (info);
                priv->got_image_info = TRUE;
        }

        return priv->image_info;
}

/**
 * gupnp_dlna_information_get_video_information:
 * @info: A #GUPnPDLNAInformation object.
 *
 * Get an container information of media file if applicable (e.g. for
 * video files).
 *
 * Returns: (transfer none): A #GUPnPDLNAVideoInformation object or %NULL.
 */
GUPnPDLNAVideoInformation *
gupnp_dlna_information_get_video_information (GUPnPDLNAInformation *info)
{
        GUPnPDLNAInformationPrivate *priv;

        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        priv = info->priv;
        if (!priv->got_video_info) {
                GUPnPDLNAInformationClass *info_class;

                info_class = GUPNP_DLNA_INFORMATION_GET_CLASS (info);

                g_return_val_if_fail
                                  (GUPNP_IS_DLNA_INFORMATION_CLASS (info_class),
                                   NULL);
                g_return_val_if_fail (info_class->get_video_information != NULL,
                                      NULL);

                priv->video_info = info_class->get_video_information (info);
                priv->got_video_info = TRUE;
        }

        return priv->video_info;
}

/**
 * gupnp_dlna_information_get_profile_name:
 * @info: A #GUPnPDLNAInformation object.
 *
 * Returns: (transfer none): The name of a DLNA profile or %NULL.
 */
const gchar *
gupnp_dlna_information_get_profile_name (GUPnPDLNAInformation *info)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        return GUPNP_DLNA_INFORMATION_GET_CLASS (info)->get_profile_name (info);
}


/**
 * gupnp_dlna_information_get_uri:
 * @info: A #GUPnPDLNAInformation object.
 *
 * Returns: (transfer none): An URI of a file.
 */
const gchar *
gupnp_dlna_information_get_uri (GUPnPDLNAInformation *info)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        return info->priv->uri;
}
