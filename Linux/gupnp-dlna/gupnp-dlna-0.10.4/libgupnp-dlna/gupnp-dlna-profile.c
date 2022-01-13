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

#include "gupnp-dlna-profile.h"
#include "gupnp-dlna-profile-private.h"

/**
 * SECTION:gupnp-dlna-profile
 * @short_description: Object representing a DLNA profile
 *
 * The #GUPnPDLNAProfileGuesser object provides a few APIs that return
 * #GUPnPDLNAProfile objects. These represent a single DLNA
 * profile. Each #GUPnPDLNAProfile has a name (the name of the DLNA
 * profile) and the corresponding MIME type.
 *
 * Public representation of the various audio/container/image/video
 * restrictions specified for that DLNA profile is to be added.
 */
G_DEFINE_TYPE (GUPnPDLNAProfile, gupnp_dlna_profile, G_TYPE_OBJECT)

struct _GUPnPDLNAProfilePrivate {
        gchar    *name;
        gchar    *mime;
        gboolean  extended;
        GList    *audio_restrictions;
        GList    *container_restrictions;
        GList    *image_restrictions;
        GList    *video_restrictions;
};

enum {
        PROP_0,
        PROP_DLNA_NAME,
        PROP_DLNA_MIME,
        PROP_DLNA_EXTENDED,
        PROP_AUDIO_RESTRICTIONS,
        PROP_CONTAINER_RESTRICTIONS,
        PROP_IMAGE_RESTRICTIONS,
        PROP_VIDEO_RESTRICTIONS
};

static void
gupnp_dlna_profile_get_property (GObject    *object,
                                 guint       property_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
        GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (object);
        GUPnPDLNAProfilePrivate *priv = profile->priv;

        switch (property_id) {
        case PROP_DLNA_NAME:
                g_value_set_string (value, priv->name);

                break;
        case PROP_DLNA_MIME:
                g_value_set_string (value, priv->mime);

                break;
        case PROP_DLNA_EXTENDED:
                g_value_set_boolean (value, priv->extended);

                break;
        case PROP_AUDIO_RESTRICTIONS:
                g_value_set_pointer (value, priv->audio_restrictions);

                break;
        case PROP_CONTAINER_RESTRICTIONS:
                g_value_set_pointer (value, priv->container_restrictions);

                break;
        case PROP_IMAGE_RESTRICTIONS:
                g_value_set_pointer (value, priv->image_restrictions);

                break;
        case PROP_VIDEO_RESTRICTIONS:
                g_value_set_pointer (value, priv->video_restrictions);

                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                                   property_id,
                                                   pspec);
                break;
        }
}

static void
free_restrictions (GList *list)
{
        if (list == NULL)
                return;
        g_list_free_full (list,
                          (GDestroyNotify) gupnp_dlna_restriction_free);
}

static void
gupnp_dlna_profile_set_property (GObject      *object,
                                 guint         property_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
        GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (object);
        GUPnPDLNAProfilePrivate *priv = profile->priv;

        switch (property_id) {
        case PROP_DLNA_NAME:
                g_free (priv->name);
                priv->name = g_value_dup_string (value);

                break;
        case PROP_DLNA_MIME:
                g_free (priv->mime);
                priv->mime = g_value_dup_string (value);

                break;
        case PROP_DLNA_EXTENDED:
                priv->extended = g_value_get_boolean (value);

                break;
        case PROP_AUDIO_RESTRICTIONS:
                free_restrictions (priv->audio_restrictions);
                priv->audio_restrictions = g_value_get_pointer (value);

                break;
        case PROP_CONTAINER_RESTRICTIONS:
                free_restrictions (priv->container_restrictions);
                priv->container_restrictions =
                        g_value_get_pointer (value);

                break;
        case PROP_IMAGE_RESTRICTIONS:
                free_restrictions (priv->image_restrictions);
                priv->image_restrictions = g_value_get_pointer (value);

                break;
        case PROP_VIDEO_RESTRICTIONS:
                free_restrictions (priv->video_restrictions);
                priv->video_restrictions = g_value_get_pointer (value);

                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_dlna_profile_finalize (GObject *object)
{
        GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (object);
        GUPnPDLNAProfilePrivate *priv = profile->priv;

        g_free (priv->name);
        g_free (priv->mime);
        free_restrictions (priv->audio_restrictions);
        free_restrictions (priv->container_restrictions);
        free_restrictions (priv->image_restrictions);
        free_restrictions (priv->video_restrictions);

        G_OBJECT_CLASS (gupnp_dlna_profile_parent_class)->finalize (object);
}

static void
gupnp_dlna_profile_class_init (GUPnPDLNAProfileClass *klass)
{
        GObjectClass *object_class = G_OBJECT_CLASS (klass);
        GParamSpec *pspec;

        object_class->get_property = gupnp_dlna_profile_get_property;
        object_class->set_property = gupnp_dlna_profile_set_property;
        object_class->finalize = gupnp_dlna_profile_finalize;

        /**
         * GUPnPDLNAProfile:name:
         *
         * Name of the DLNA profile.
         */
        pspec = g_param_spec_string ("name",
                                     "DLNA profile name",
                                     "The name of the DLNA profile",
                                     NULL,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_DLNA_NAME, pspec);

        /**
         * GUPnPDLNAProfile:mime:
         *
         * MIME type of the DLNA profile.
         */
        pspec = g_param_spec_string ("mime",
                                     "DLNA profile MIME type",
                                     "The MIME type of the DLNA profile",
                                     NULL,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class, PROP_DLNA_MIME, pspec);

        /**
         * GUPnPDLNAProfile:extended:
         *
         * Whether the DLNA profile is not a part of DLNA specification.
         */
        pspec = g_param_spec_boolean ("extended",
                                      "Extended mode property",
                                      "Indicates that this profile is not "
                                      "part of the DLNA specification",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_DLNA_EXTENDED,
                                         pspec);
        pspec = g_param_spec_pointer ("audio-restrictions",
                                      "Audio restrictions",
                                      "Audio restrictions for the DLNA Profile",
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_AUDIO_RESTRICTIONS,
                                         pspec);
        pspec = g_param_spec_pointer ("container-restrictions",
                                      "Container restrictions",
                                      "Container restrictions for the DLNA "
                                      "Profile",
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_CONTAINER_RESTRICTIONS,
                                         pspec);
        pspec = g_param_spec_pointer ("image-restrictions",
                                      "Image restrictions",
                                      "Image restrictions for the DLNA Profile",
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_IMAGE_RESTRICTIONS,
                                         pspec);
        pspec = g_param_spec_pointer ("video-restrictions",
                                      "Video restrictions",
                                      "Video restrictions for the DLNA Profile",
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_VIDEO_RESTRICTIONS,
                                         pspec);

        g_type_class_add_private (klass, sizeof (GUPnPDLNAProfilePrivate));
}

static void
gupnp_dlna_profile_init (GUPnPDLNAProfile *profile)
{
        GUPnPDLNAProfilePrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE (profile,
                                                  GUPNP_TYPE_DLNA_PROFILE,
                                                  GUPnPDLNAProfilePrivate);

        priv->name = NULL;
        priv->mime = NULL;
        priv->extended = FALSE;
        priv->audio_restrictions = NULL;
        priv->container_restrictions = NULL;
        priv->image_restrictions = NULL;
        priv->video_restrictions = NULL;
        profile->priv = priv;
}

/**
 * gupnp_dlna_profile_get_name:
 * @profile: The #GUPnPDLNAProfile object.
 *
 * Returns: The name of the DLNA profile represented by @profile.
 */
const gchar *
gupnp_dlna_profile_get_name (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), NULL);

        return profile->priv->name;
}

/**
 * gupnp_dlna_profile_get_mime:
 * @profile: The #GUPnPDLNAProfile object.
 *
 * Returns: The DLNA MIME type of the DLNA profile represented by @profile.
 */
const gchar *
gupnp_dlna_profile_get_mime (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), NULL);

        return profile->priv->mime;
}

/**
 * gupnp_dlna_profile_get_extended:
 * @profile: The #GUPnPDLNAProfile object.
 *
 * Returns: %TRUE if @profile is extended one and %FALSE otherwise.
 */
gboolean
gupnp_dlna_profile_get_extended (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), FALSE);

        return profile->priv->extended;
}

/**
 * gupnp_dlna_profile_get_audio_restrictions:
 * @profile: (transfer none): A profile.
 *
 * Gets a list of audio restrictions.
 *
 * Returns: (transfer none) (element-type GUPnPDLNARestriction): Audio
 * restrictions. Do not modify.
 */
GList *
gupnp_dlna_profile_get_audio_restrictions (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), NULL);

        return profile->priv->audio_restrictions;
}

/**
 * gupnp_dlna_profile_get_container_restrictions:
 * @profile: (transfer none): A profile.
 *
 * Gets a list of container restrictions.
 *
 * Returns: (transfer none) (element-type GUPnPDLNARestriction):
 * Container restrictions. Do not modify.
 */
GList *
gupnp_dlna_profile_get_container_restrictions (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), NULL);

        return profile->priv->container_restrictions;
}

/**
 * gupnp_dlna_profile_get_image_restrictions:
 * @profile: (transfer none): A profile.
 *
 * Gets a list of image restrictions.
 *
 * Returns: (transfer none) (element-type GUPnPDLNARestriction): Image
 * restrictions. Do not modify.
 */
GList *
gupnp_dlna_profile_get_image_restrictions (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), NULL);

        return profile->priv->image_restrictions;
}

/**
 * gupnp_dlna_profile_get_video_restrictions:
 * @profile: (transfer none): A profile.
 *
 * Gets a list of video restrictions.
 *
 * Returns: (transfer none) (element-type GUPnPDLNARestriction): Video
 * restrictions. Do not modify.
 */
GList *
gupnp_dlna_profile_get_video_restrictions (GUPnPDLNAProfile *profile)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE (profile), NULL);

        return profile->priv->video_restrictions;
}

GUPnPDLNAProfile *
gupnp_dlna_profile_new (const gchar *name,
                        const gchar *mime,
                        GList       *audio_restrictions,
                        GList       *container_restrictions,
                        GList       *image_restrictions,
                        GList       *video_restrictions,
                        gboolean     extended)
{
        return GUPNP_DLNA_PROFILE
                             (g_object_new
                              (GUPNP_TYPE_DLNA_PROFILE,
                               "name", name,
                               "mime", mime,
                               "audio-restrictions", audio_restrictions,
                               "container-restrictions", container_restrictions,
                               "image-restrictions", image_restrictions,
                               "video-restrictions", video_restrictions,
                               "extended", extended,
                               NULL));
}
