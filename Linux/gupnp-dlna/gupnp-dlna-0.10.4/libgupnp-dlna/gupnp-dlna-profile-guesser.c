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

#include "gupnp-dlna-profile-guesser.h"
#include "gupnp-dlna-profile-guesser-impl.h"
#include "gupnp-dlna-profile-loader.h"
#include "gupnp-dlna-metadata-extractor.h"
#include "gupnp-dlna-metadata-backend.h"

/**
 * SECTION:gupnp-dlna-profile-guesser
 * @short_description: Utility API for discovering DLNA profile for
 * given media.
 *
 * The API provides synchronous and asynchronous guessing of DLNA
 * profile. The asynchronous mode requires a running #GMainLoop in the
 * default #GMainContext.
 */
enum {
        DONE,
        SIGNAL_LAST
};

static guint signals[SIGNAL_LAST];

G_DEFINE_TYPE (GUPnPDLNAProfileGuesser,
               gupnp_dlna_profile_guesser,
               G_TYPE_OBJECT)

struct _GUPnPDLNAProfileGuesserPrivate {
        gboolean relaxed_mode;
        gboolean extended_mode;
};

enum {
        PROP_0,
        PROP_DLNA_RELAXED_MODE,
        PROP_DLNA_EXTENDED_MODE,
};

static GList *profiles_list[2][2];

static void
gupnp_dlna_profile_guesser_set_property (GObject      *object,
                                         guint         property_id,
                                         const GValue *value,
                                         GParamSpec   *pspec)
{
        GUPnPDLNAProfileGuesser *self = GUPNP_DLNA_PROFILE_GUESSER (object);
        GUPnPDLNAProfileGuesserPrivate *priv = self->priv;

        switch (property_id) {
        case PROP_DLNA_RELAXED_MODE:
                priv->relaxed_mode = g_value_get_boolean (value);
                break;

        case PROP_DLNA_EXTENDED_MODE:
                priv->extended_mode = g_value_get_boolean (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                                   property_id,
                                                   pspec);
                break;
        }
}

static void
gupnp_dlna_profile_guesser_get_property (GObject    *object,
                                         guint       property_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
        GUPnPDLNAProfileGuesser *self = GUPNP_DLNA_PROFILE_GUESSER (object);
        GUPnPDLNAProfileGuesserPrivate *priv = self->priv;

        switch (property_id) {
        case PROP_DLNA_RELAXED_MODE:
                g_value_set_boolean (value, priv->relaxed_mode);
                break;

        case PROP_DLNA_EXTENDED_MODE:
                g_value_set_boolean (value, priv->extended_mode);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object,
                                                   property_id,
                                                   pspec);
                break;
        }
}

static void
gupnp_dlna_profile_guesser_class_init
                                   (GUPnPDLNAProfileGuesserClass *guesser_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (guesser_class);
        GParamSpec *pspec;
        guint iter;

        object_class->get_property = gupnp_dlna_profile_guesser_get_property;
        object_class->set_property = gupnp_dlna_profile_guesser_set_property;

        /**
         * GUPnPDLNAProfileGuesser:relaxed-mode:
         *
         * Whether profile matching should not be strictly compliant
         * with the DLNA specification.
         */
        pspec = g_param_spec_boolean ("relaxed-mode",
                                      "Relaxed mode property",
                                      "Indicates that profile matching should"
                                      "be strictly compliant with the DLNA "
                                      "specification",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_DLNA_RELAXED_MODE,
                                         pspec);

        /**
         * GUPnPDLNAProfileGuesser:extended-mode:
         * @extended: setting true will enable extended profile support
         *
         * Whether profile matching should be done also against DLNA
         * profiles not being a part of DLNA specification.
         */
        pspec = g_param_spec_boolean ("extended-mode",
                                      "Extended mode property",
                                      "Indicates support for profiles that are "
                                      "not part of the DLNA specification",
                                      FALSE,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY);
        g_object_class_install_property (object_class,
                                         PROP_DLNA_EXTENDED_MODE,
                                         pspec);

        /**
         * GUPnPDLNAProfileGuesser::done:
         * @profile_guesser: The #GUPnPDLNAProfileGuesser.
         * @info: (transfer none): URI metadata as #GUPnPDLNAInformation.
         * @dlna: (allow-none) (transfer none): The results as #GUPnPDLNAProfile.
         * @error: (allow-none): Contains details of the error if discovery failed,
         * else is %NULL.
         *
         * Will be emitted when guessing DLNA profile for a URI has finished.
         */
        signals[DONE] =
                g_signal_new ("done",
                              G_TYPE_FROM_CLASS (guesser_class),
                              G_SIGNAL_RUN_LAST,
                              0,
                              NULL,
                              NULL,
                              g_cclosure_marshal_generic,
                              G_TYPE_NONE,
                              3,
                              GUPNP_TYPE_DLNA_INFORMATION,
                              GUPNP_TYPE_DLNA_PROFILE,
                              G_TYPE_ERROR);

        /* Load DLNA profiles from disk */
        for (iter = 0; iter < 4; ++iter) {
                gboolean relaxed = (iter > 1); /* F,F,T,T */
                gboolean extended = ((iter) % 2 != 0); /* F,T,F,T */
                guint rel_index = (relaxed ? 1 : 0);
                guint ext_index = (extended ? 1 : 0);
                GUPnPDLNAProfileLoader *loader = gupnp_dlna_profile_loader_new
                                        (relaxed,
                                         extended);

                profiles_list[rel_index][ext_index] =
                               gupnp_dlna_profile_loader_get_from_disk (loader);
                g_object_unref (loader);
        }

        g_type_class_add_private (guesser_class,
                                  sizeof (GUPnPDLNAProfileGuesserPrivate));
}

static void
gupnp_dlna_profile_guesser_init (GUPnPDLNAProfileGuesser *self)
{
        GUPnPDLNAProfileGuesserPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (self,
                                         GUPNP_TYPE_DLNA_PROFILE_GUESSER,
                                         GUPnPDLNAProfileGuesserPrivate);

        priv->relaxed_mode = FALSE;
        priv->extended_mode = FALSE;
        self->priv = priv;
}

/**
 * gupnp_dlna_profile_guesser_new:
 * @relaxed_mode: %TRUE to enable relaxed mode support. %FALSE otherwise.
 * @extended_mode: %TRUE to enable extended mode support. %FALSE otherwise.
 *
 * Creates a new guesser that will use specific DLNA profiles for
 * matching - i.e. which profiles will be used depends on
 * @relaxed_mode and @extended_mode.
 *
 * Returns: A new #GUPnPDLNAProfileGuesser object.
 */
GUPnPDLNAProfileGuesser *
gupnp_dlna_profile_guesser_new (gboolean relaxed_mode,
                                gboolean extended_mode)
{
        return GUPNP_DLNA_PROFILE_GUESSER (g_object_new
                                           (GUPNP_TYPE_DLNA_PROFILE_GUESSER,
                                            "relaxed-mode", relaxed_mode,
                                            "extended-mode", extended_mode,
                                            NULL));
}

static gboolean
unref_extractor_in_idle (GUPnPDLNAMetadataExtractor *extractor)
{
        if (extractor)
                g_object_unref (extractor);

        return FALSE;
}

static void
gupnp_dlna_discovered_cb (GUPnPDLNAProfileGuesser *guesser,
                          GUPnPDLNAInformation    *info,
                          GError                  *error,
                          gpointer                 user_data)
{
        GUPnPDLNAProfile *profile = NULL;
        GUPnPDLNAMetadataExtractor *extractor =
                                      GUPNP_DLNA_METADATA_EXTRACTOR (user_data);

        if (!error) {
                profile = gupnp_dlna_profile_guesser_guess_profile_from_info
                                        (guesser,
                                         info);
        }
        g_signal_emit (guesser, signals[DONE], 0, info, profile, error);

        g_idle_add ((GSourceFunc) unref_extractor_in_idle, extractor);
}

/**
 * gupnp_dlna_profile_guesser_guess_profile_async:
 * @guesser: #GUPnPDLNAProfileGuesser object to use for guessing.
 * @uri: URI of media.
 * @timeout_in_ms: Timeout of guessing in miliseconds.
 * @error: #GError object or %NULL.
 *
 * Asynchronously guesses DLNA profile for given @uri. When guessing
 * is done, ::done signal is emitted on @guesser.
 *
 * Returns: %TRUE if @uri was successfully queued, %FALSE otherwise.
 */
gboolean
gupnp_dlna_profile_guesser_guess_profile_async
                                       (GUPnPDLNAProfileGuesser  *guesser,
                                        const gchar              *uri,
                                        guint                     timeout_in_ms,
                                        GError                  **error)
{
        GUPnPDLNAMetadataExtractor *extractor;
        gboolean queued;
        GError *extractor_error;
        guint id;

        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), FALSE);
        g_return_val_if_fail (uri != NULL, FALSE);
        g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

        extractor = gupnp_dlna_metadata_backend_get_extractor ();
        g_return_val_if_fail (extractor != NULL, FALSE);

        extractor_error = NULL;
        id = g_signal_connect_swapped (extractor,
                                       "done",
                                       G_CALLBACK (gupnp_dlna_discovered_cb),
                                       guesser);
        queued = gupnp_dlna_metadata_extractor_extract_async (extractor,
                                                              uri,
                                                              timeout_in_ms,
                                                              &extractor_error);
        if (extractor_error) {
                g_propagate_error (error, extractor_error);
                g_signal_handler_disconnect (extractor, id);
                g_object_unref (extractor);
        }

        return queued;
}

/* Synchronous API */

/**
 * gupnp_dlna_profile_guesser_guess_profile_sync:
 * @guesser: #GUPnPDLNAProfileGuesser object to use for guessing.
 * @uri: URI of media.
 * @timeout_in_ms: Timeout of guessing in miliseconds.
 * @dlna_info: (allow-none) (transfer full) (out): A place where to
 * store DLNA information or %NULL.
 * @error: (allow-none): #GError object or %NULL.
 *
 * Synchronously guesses DLNA profile for given @uri.
 *
 * Returns: (transfer none): DLNA profile if any had matched, %NULL otherwise.
 */
GUPnPDLNAProfile *
gupnp_dlna_profile_guesser_guess_profile_sync
                                       (GUPnPDLNAProfileGuesser  *guesser,
                                        const gchar              *uri,
                                        guint                     timeout_in_ms,
                                        GUPnPDLNAInformation    **dlna_info,
                                        GError                  **error)
{
        GError *extraction_error;
        GUPnPDLNAMetadataExtractor *extractor;
        GUPnPDLNAInformation *info;
        GUPnPDLNAProfile *profile;

        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), NULL);
        g_return_val_if_fail (uri != NULL, NULL);
        g_return_val_if_fail (dlna_info == NULL || *dlna_info == NULL, NULL);
        g_return_val_if_fail (error == NULL || *error == NULL, NULL);

        extraction_error = NULL;
        extractor = gupnp_dlna_metadata_backend_get_extractor ();
        g_return_val_if_fail (extractor != NULL, NULL);

        info = gupnp_dlna_metadata_extractor_extract_sync (extractor,
                                                           uri,
                                                           timeout_in_ms,
                                                           &extraction_error);
        profile = NULL;

        if (extraction_error)
                g_propagate_error (error,
                                   extraction_error);
        else
                profile = gupnp_dlna_profile_guesser_guess_profile_from_info
                                        (guesser,
                                         info);

        if (info) {
                if (dlna_info)
                        *dlna_info = info;
                else
                        g_object_unref (info);
        }
        if (extractor)
                g_object_unref (extractor);

        return profile;
}

static gint
compare_profile_name (GUPnPDLNAProfile *profile, const char *name)
{
        return g_ascii_strcasecmp (gupnp_dlna_profile_get_name (profile),
                                   name);
}

/**
 * gupnp_dlna_profile_guesser_guess_profile_from_info:
 * @guesser: The #GUPnPDLNAProfileGuesser object.
 * @info: The #GUPnPDLNAInformation object.
 *
 * Guesses the profile which fits to passed @info.
 *
 * Returns: (transfer none): A #GUPnPDLNAProfile object on success,
 * %NULL otherwise.
 */
GUPnPDLNAProfile *
gupnp_dlna_profile_guesser_guess_profile_from_info
                                        (GUPnPDLNAProfileGuesser *guesser,
                                         GUPnPDLNAInformation    *info)
{
        GList *profiles;
        GUPnPDLNAVideoInformation *video_info;
        GUPnPDLNAAudioInformation *audio_info;
        GUPnPDLNAImageInformation *image_info;
        GUPnPDLNAProfile *profile;
        const gchar *profile_name;

        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), NULL);
        g_return_val_if_fail (GUPNP_IS_DLNA_INFORMATION (info), NULL);

        profiles = gupnp_dlna_profile_guesser_list_profiles (guesser);
        video_info = gupnp_dlna_information_get_video_information (info);
        audio_info = gupnp_dlna_information_get_audio_information (info);
        image_info = gupnp_dlna_information_get_image_information (info);
        profile_name = gupnp_dlna_information_get_profile_name (info);

        if (profile_name) {
                GList *it = NULL;
                it = g_list_find_custom (profiles,
                                         profile_name,
                                         (GCompareFunc) compare_profile_name);
                if (it != NULL)
                        return it->data;
                else
                        g_warning ("Profile '%s' provided by back-end not known to GUPnP-DLNA",
                                   profile_name);
        }

        if (image_info)
                profile = gupnp_dlna_profile_guesser_impl_guess_image_profile
                                        (info,
                                         profiles);
        else if (video_info)
                profile = gupnp_dlna_profile_guesser_impl_guess_video_profile
                                        (info,
                                         profiles);
        else if (audio_info)
                profile = gupnp_dlna_profile_guesser_impl_guess_audio_profile
                                        (info,
                                         profiles);
        else
                profile = NULL;

        return profile;
}

/**
 * gupnp_dlna_profile_guesser_get_profile:
 * @guesser: The #GUPnPDLNAProfileGuesser object.
 * @name: The name of the DLNA profile to be retrieved.
 *
 * Lookups for #GUPnPDLNAProfile with given @name.
 *
 * Returns: (transfer none): A #GUPnPDLNAProfile object on success,
 * %NULL otherwise.
 */
GUPnPDLNAProfile *
gupnp_dlna_profile_guesser_get_profile (GUPnPDLNAProfileGuesser *guesser,
                                        const gchar             *name)
{
        // TODO: use a GHashTable for this.
        GList *iter;
        GUPnPDLNAProfileGuesserPrivate *priv;

        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), NULL);
        g_return_val_if_fail (name != NULL, NULL);

        priv = guesser->priv;

        for (iter = profiles_list[priv->relaxed_mode][priv->extended_mode];
             iter;
             iter = iter->next) {
                GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (iter->data);

                if (!g_strcmp0 (gupnp_dlna_profile_get_name (profile), name)) {
                        return profile;
                }
        }

        return NULL;
}

/**
 * gupnp_dlna_profile_guesser_list_profiles:
 * @guesser: The #GUPnPDLNAProfileGuesser whose profile list is required.
 *
 * Gets a list of the all DLNA profiles supported by @guesser.
 *
 * Returns: (transfer none) (element-type GUPnPDLNAProfile): A #GList
 * of #GUPnPDLNAProfile on success, %NULL otherwise.
 */
GList *
gupnp_dlna_profile_guesser_list_profiles (GUPnPDLNAProfileGuesser *guesser)
{
        GUPnPDLNAProfileGuesserPrivate *priv;

        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), NULL);

        priv = guesser->priv;

        return profiles_list[priv->relaxed_mode][priv->extended_mode];
}

/**
 * gupnp_dlna_profile_guesser_get_relaxed_mode:
 * @guesser: The #GUPnPDLNAProfileGuesser object.
 *
 * Returns: %TRUE if @guesser is in relaxed mode, %FALSE otherwise.
 */
gboolean
gupnp_dlna_profile_guesser_get_relaxed_mode (GUPnPDLNAProfileGuesser *guesser)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), FALSE);

        return guesser->priv->relaxed_mode;
}

/**
 * gupnp_dlna_profile_guesser_get_extended_mode:
 * @guesser: The #GUPnPDLNAProfileGuesser object.
 *
 * Returns: %TRUE if @guesser is in extended mode, %FALSE otherwise.
 */
gboolean
gupnp_dlna_profile_guesser_get_extended_mode (GUPnPDLNAProfileGuesser *guesser)
{
        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_GUESSER (guesser), FALSE);

        return guesser->priv->extended_mode;
}

/**
 * gupnp_dlna_profile_guesser_cleanup:
 *
 * Cleans up the DLNA profiles. Provided to remove Valgrind noise. Not
 * thread-safe. Do not call it if there is even a slightest chance
 * that profile guessing will be performed during process
 * lifetime. The profiles are not reloaded after cleanup.
 */
void
gupnp_dlna_profile_guesser_cleanup (void)
{
        guint iter;

        for (iter = 0; iter < 4; ++iter) {
                gboolean relaxed = (iter > 1); /* F,F,T,T */
                gboolean extended = ((iter) % 2 != 0); /* F,T,F,T */
                guint rel_index = (relaxed ? 1 : 0);
                guint ext_index = (extended ? 1 : 0);

                g_list_free_full (profiles_list[rel_index][ext_index],
                                  g_object_unref);
                profiles_list[rel_index][ext_index] = NULL;
        }
}
