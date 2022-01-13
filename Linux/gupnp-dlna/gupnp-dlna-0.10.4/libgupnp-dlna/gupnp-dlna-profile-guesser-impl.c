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

#include <glib.h>

#include "gupnp-dlna-profile.h"
#include "gupnp-dlna-profile-guesser-impl.h"
#include "gupnp-dlna-information.h"
#include "gupnp-dlna-audio-information.h"
#include "gupnp-dlna-container-information.h"
#include "gupnp-dlna-image-information.h"
#include "gupnp-dlna-video-information.h"
#include "gupnp-dlna-utils.h"
#include "gupnp-dlna-info-set.h"

static gboolean
is_video_profile (GUPnPDLNAProfile *profile)
{
        GList *container_restrictions =
                 gupnp_dlna_profile_get_container_restrictions (profile);
        GList *video_restrictions =
                     gupnp_dlna_profile_get_video_restrictions (profile);

        return (container_restrictions != NULL && video_restrictions != NULL);
}

static gboolean
match_profile (GUPnPDLNAProfile *profile,
               GUPnPDLNAInfoSet *stream_info_set,
               GList            *profile_restrictions)
{
        const gchar *name = gupnp_dlna_profile_get_name (profile);
        GList *iter;
        gchar *stream_dump;
        gchar *restrictions_dump;

        /* Profiles with an empty name are used only for inheritance
         * and should not be matched against. */
        if (name == NULL || name[0] == '\0') {
                /* TODO: if this warning never happens, then remove
                 * profile parameter from this function.
                 */
                g_warning ("Profiles weren't cleaned up properly?");
                return FALSE;
        }

        stream_dump = gupnp_dlna_info_set_to_string (stream_info_set);
        restrictions_dump = gupnp_dlna_utils_restrictions_list_to_string
                                        (profile_restrictions);
        g_debug ("Stream: %s\nRestrictions: %s",
                 stream_dump,
                 restrictions_dump);
        g_free (stream_dump);
        g_free (restrictions_dump);

        for (iter = profile_restrictions; iter != NULL; iter = iter->next) {
                GUPnPDLNARestriction *restriction =
                                        GUPNP_DLNA_RESTRICTION (iter->data);

                if (restriction != NULL &&
                    gupnp_dlna_info_set_fits_restriction (stream_info_set,
                                                          restriction))
                        return TRUE;
        }

        return FALSE;
}

static void
add_bool (GUPnPDLNAInfoSet   *info_set,
          const gchar        *name,
          GUPnPDLNABoolValue  value,
          const gchar        *type)
{
        switch (value.state) {
        case GUPNP_DLNA_VALUE_STATE_SET:
                if (!gupnp_dlna_info_set_add_bool (info_set, name, value.value))
                        g_warning ("Failed to add '%s' bool value (%s) to %s "
                                   "info set.",
                                   name,
                                   (value.value ? "true" : "false"),
                                   type);

                break;
        case GUPNP_DLNA_VALUE_STATE_UNSET:
                break;
        case GUPNP_DLNA_VALUE_STATE_UNSUPPORTED:
                if (!gupnp_dlna_info_set_add_unsupported_bool (info_set, name))
                        g_warning ("Failed to add '%s' bool unsupported value"
                                   " to %s info set.",
                                   name,
                                   type);

                break;
        default:
                g_critical ("Wrong value state value (%d).", value.state);
        }
}

static void
add_fraction (GUPnPDLNAInfoSet       *info_set,
              const gchar            *name,
              GUPnPDLNAFractionValue  value,
              const gchar            *type)
{
        switch (value.state) {
        case GUPNP_DLNA_VALUE_STATE_SET:
                if (!gupnp_dlna_info_set_add_fraction (info_set,
                                                       name,
                                                       value.numerator,
                                                       value.denominator))
                        g_warning ("Failed to add '%s' fraction value (%d/%d)"
                                   " to %s info set.",
                                   name,
                                   value.numerator,
                                   value.denominator,
                                   type);

                break;
        case GUPNP_DLNA_VALUE_STATE_UNSET:
                break;
        case GUPNP_DLNA_VALUE_STATE_UNSUPPORTED:
                if (!gupnp_dlna_info_set_add_unsupported_fraction (info_set,
                                                                   name))
                        g_warning ("Failed to add '%s' fraction unsupported"
                                   " value to %s info set.",
                                   name,
                                   type);

                break;
        default:
                g_critical ("Wrong value state value (%d).", value.state);
        }
}

static void
add_int (GUPnPDLNAInfoSet   *info_set,
          const gchar       *name,
          GUPnPDLNAIntValue  value,
          const gchar       *type)
{
        switch (value.state) {
        case GUPNP_DLNA_VALUE_STATE_SET:
                if (!gupnp_dlna_info_set_add_int (info_set, name, value.value))
                        g_warning ("Failed to add '%s' int value (%d) to %s "
                                   "info set.",
                                   name,
                                   value.value,
                                   type);

                break;
        case GUPNP_DLNA_VALUE_STATE_UNSET:
                break;
        case GUPNP_DLNA_VALUE_STATE_UNSUPPORTED:
                if (!gupnp_dlna_info_set_add_unsupported_int (info_set, name))
                        g_warning ("Failed to add '%s' int unsupported value"
                                   " to %s info set.",
                                   name,
                                   type);

                break;
        default:
                g_critical ("Wrong value state value (%d).", value.state);
        }
}

static void
add_string (GUPnPDLNAInfoSet     *info_set,
            const gchar          *name,
            GUPnPDLNAStringValue  value,
            const gchar          *type)
{
        switch (value.state) {
        case GUPNP_DLNA_VALUE_STATE_SET:
                if (!gupnp_dlna_info_set_add_string (info_set,
                                                     name,
                                                     value.value))
                        g_warning ("Failed to add '%s' int value (%s) to %s "
                                   "info set.",
                                   name,
                                   value.value,
                                   type);
                g_free (value.value);

                break;
        case GUPNP_DLNA_VALUE_STATE_UNSET:
                break;
        case GUPNP_DLNA_VALUE_STATE_UNSUPPORTED:
                if (!gupnp_dlna_info_set_add_unsupported_string (info_set,
                                                                 name))
                        g_warning ("Failed to add '%s' string unsupported value"
                                   " to %s info set.",
                                   name,
                                   type);

                break;
        default:
                g_critical ("Wrong value state value (%d).", value.state);
        }
}

static GUPnPDLNAInfoSet *
create_info_set (GUPnPDLNAStringValue  value,
                 const gchar          *type)
{
        gchar *mime;
        gchar *lctype;
        GUPnPDLNAInfoSet *info_set;

        if (value.state == GUPNP_DLNA_VALUE_STATE_SET) {
                mime = value.value;
                lctype = g_ascii_strdown (type, -1);
        } else {
                g_warning ("%s information holds no mime type, expect it"
                           "to match to no DLNA profile.",
                           type);
                mime = g_ascii_strdown (type, -1);
                lctype = g_strdup (mime);
        }

        info_set = gupnp_dlna_info_set_new (mime);
        if (info_set == NULL)
                g_warning ("Failed to create %s info set with mime '%s'.",
                           lctype,
                           mime);
        g_free (mime);
        g_free (lctype);

        return info_set;
}

static GUPnPDLNAInfoSet *
info_set_from_container_information (GUPnPDLNAContainerInformation *info)
{
        static const gchar *const type = "container";
        GUPnPDLNAInfoSet *info_set = create_info_set
                              (gupnp_dlna_container_information_get_mime (info),
                               "Container");

        if (info_set == NULL)
                return NULL;

        add_int (info_set,
                 "mpegversion",
                 gupnp_dlna_container_information_get_mpeg_version (info),
                 type);

        add_int (info_set,
                 "packetsize",
                 gupnp_dlna_container_information_get_packet_size (info),
                 type);

        add_string (info_set,
                    "profile",
                    gupnp_dlna_container_information_get_profile (info),
                    type);

        add_bool (info_set,
                  "systemstream",
                  gupnp_dlna_container_information_is_system_stream (info),
                  type);

        add_string (info_set,
                    "variant",
                    gupnp_dlna_container_information_get_variant (info),
                    type);

        return info_set;
}

static gboolean
check_container_profile (GUPnPDLNAInformation *info,
                         GUPnPDLNAProfile     *profile)
{
        GUPnPDLNAContainerInformation *container_info =
                        gupnp_dlna_information_get_container_information (info);
        gboolean matched = FALSE;
        GList *profile_restrictions =
                 gupnp_dlna_profile_get_container_restrictions (profile);

        if (profile_restrictions != NULL && container_info != NULL) {
                GUPnPDLNAInfoSet *stream_info_set =
                           info_set_from_container_information (container_info);

                if (match_profile (profile,
                                   stream_info_set,
                                   profile_restrictions))
                        matched = TRUE;
                else
                        g_debug ("Container did not match.");
                gupnp_dlna_info_set_free (stream_info_set);
        } else if (profile_restrictions == NULL && container_info == NULL)
                matched = TRUE;

        return matched;
}

static GUPnPDLNAInfoSet *
info_set_from_audio_information (GUPnPDLNAAudioInformation *info)
{
        static const gchar *const type = "audio";
        GUPnPDLNAInfoSet *info_set = create_info_set
                                  (gupnp_dlna_audio_information_get_mime (info),
                                   "Audio");

        if (info_set == NULL)
                return NULL;

        add_int (info_set,
                 "bitrate",
                 gupnp_dlna_audio_information_get_bitrate (info),
                 type);

        add_int (info_set,
                 "channels",
                 gupnp_dlna_audio_information_get_channels (info),
                 type);

        add_int (info_set,
                 "depth",
                 gupnp_dlna_audio_information_get_depth (info),
                 type);

        add_int (info_set,
                 "layer",
                 gupnp_dlna_audio_information_get_layer (info),
                 type);

        add_string (info_set,
                    "level",
                    gupnp_dlna_audio_information_get_level (info),
                    type);

        add_int (info_set,
                 "mpegaudioversion",
                 gupnp_dlna_audio_information_get_mpeg_audio_version (info),
                 type);

        add_int (info_set,
                 "mpegversion",
                 gupnp_dlna_audio_information_get_mpeg_version (info),
                 type);

        add_string (info_set,
                    "profile",
                    gupnp_dlna_audio_information_get_profile (info),
                    type);

        add_int (info_set,
                 "rate",
                 gupnp_dlna_audio_information_get_rate (info),
                 type);

        add_string (info_set,
                    "stream-format",
                    gupnp_dlna_audio_information_get_stream_format (info),
                    type);

        add_int (info_set,
                 "wmaversion",
                 gupnp_dlna_audio_information_get_wma_version (info),
                 type);

        return info_set;
}

static gboolean
check_audio_profile (GUPnPDLNAInformation *info,
                     GUPnPDLNAProfile     *profile)
{
        GUPnPDLNAInfoSet *info_set;
        gboolean matched;
        GUPnPDLNAAudioInformation *audio_info;
        GList *restrictions;

        if (is_video_profile (profile))
                return FALSE;

        matched = FALSE;
        audio_info = gupnp_dlna_information_get_audio_information (info);
        info_set = info_set_from_audio_information (audio_info);
        restrictions = gupnp_dlna_profile_get_audio_restrictions (profile);
        if (match_profile (profile, info_set, restrictions))
                matched = TRUE;
        else
                g_debug ("Audio did not match.");
        gupnp_dlna_info_set_free (info_set);

        return matched;
}

static GUPnPDLNAInfoSet *
info_set_from_video_information (GUPnPDLNAVideoInformation *info)
{
        static const gchar *const type = "video";
        GUPnPDLNAInfoSet *info_set = create_info_set
                                  (gupnp_dlna_video_information_get_mime (info),
                                   "Video");

        if (info_set == NULL)
                return NULL;

        add_int (info_set,
                 "bitrate",
                 gupnp_dlna_video_information_get_bitrate (info),
                 type);

        add_fraction (info_set,
                      "framerate",
                      gupnp_dlna_video_information_get_framerate (info),
                      type);

        add_int (info_set,
                 "height",
                 gupnp_dlna_video_information_get_height (info),
                 type);

        add_bool (info_set,
                  "interlaced",
                  gupnp_dlna_video_information_is_interlaced (info),
                  type);

        add_string (info_set,
                    "level",
                    gupnp_dlna_video_information_get_level (info),
                    type);

        add_int (info_set,
                 "mpegversion",
                 gupnp_dlna_video_information_get_mpeg_version (info),
                 type);

        add_fraction
                    (info_set,
                     "pixel-aspect-ratio",
                     gupnp_dlna_video_information_get_pixel_aspect_ratio (info),
                     type);

        add_string (info_set,
                    "profile",
                    gupnp_dlna_video_information_get_profile (info),
                    type);

        add_bool (info_set,
                  "systemstream",
                  gupnp_dlna_video_information_is_system_stream (info),
                  type);

        add_int (info_set,
                 "width",
                 gupnp_dlna_video_information_get_width (info),
                 type);

        return info_set;
}

static gboolean
check_video_profile (GUPnPDLNAInformation *info,
                     GUPnPDLNAProfile     *profile)
{
        GUPnPDLNAVideoInformation *video_info =
                            gupnp_dlna_information_get_video_information (info);
        GUPnPDLNAAudioInformation *audio_info =
                            gupnp_dlna_information_get_audio_information (info);
        GUPnPDLNAInfoSet *info_set = NULL;
        GList *restrictions;
        gboolean result = FALSE;

        if (video_info == NULL || audio_info == NULL)
                goto out;

        restrictions = gupnp_dlna_profile_get_video_restrictions (profile);
        info_set = info_set_from_video_information (video_info);
        if (!match_profile (profile, info_set, restrictions)) {
                g_debug ("Video did not match");

                goto out;
        }
        gupnp_dlna_info_set_free (info_set);

        restrictions = gupnp_dlna_profile_get_audio_restrictions (profile);
        info_set = info_set_from_audio_information (audio_info);
        if (!match_profile (profile, info_set, restrictions)) {
                g_debug ("Audio did not match");

                goto out;
        }

        if (check_container_profile (info, profile))
                result = TRUE;
 out:
        gupnp_dlna_info_set_free (info_set);

        return result;
}

static GUPnPDLNAInfoSet *
info_set_from_image_information (GUPnPDLNAImageInformation *info)
{
        static const gchar *const type = "image";
        GUPnPDLNAInfoSet *info_set = create_info_set
                                  (gupnp_dlna_image_information_get_mime (info),
                                   "Image");

        if (info_set == NULL)
                return NULL;

        add_int (info_set,
                 "depth",
                 gupnp_dlna_image_information_get_depth (info),
                 type);

        add_int (info_set,
                 "height",
                 gupnp_dlna_image_information_get_height (info),
                 type);

        add_int (info_set,
                 "width",
                 gupnp_dlna_image_information_get_width (info),
                 type);

        return info_set;
}

GUPnPDLNAProfile *
gupnp_dlna_profile_guesser_impl_guess_image_profile
                                        (GUPnPDLNAInformation *info,
                                         GList                *profiles)
{
        GList *iter;
        GUPnPDLNAImageInformation *image_info =
                            gupnp_dlna_information_get_image_information (info);
        GUPnPDLNAInfoSet *info_set;
        GUPnPDLNAProfile *found_profile;

        if (!image_info)
                return NULL;

        info_set = info_set_from_image_information (image_info);
        found_profile = NULL;

        for (iter = profiles; iter; iter = iter->next) {
                GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (iter->data);
                GList *restrictions =
                            gupnp_dlna_profile_get_image_restrictions (profile);

                g_debug ("Matching image against profile: %s",
                         gupnp_dlna_profile_get_name (profile));

                if (match_profile (profile, info_set, restrictions)) {
                        found_profile = profile;

                        break;
                } else
                        g_debug ("Image did not match");
        }

        gupnp_dlna_info_set_free (info_set);

        return found_profile;
}

GUPnPDLNAProfile *
gupnp_dlna_profile_guesser_impl_guess_video_profile
                                        (GUPnPDLNAInformation *info,
                                         GList                *profiles)
{
        GUPnPDLNAProfile *found_profile = NULL;
        GList *iter;

        for (iter = profiles; iter; iter = iter->next) {
                GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (iter->data);

                g_debug ("Matching video against profile: %s",
                         gupnp_dlna_profile_get_name (profile));

                if (check_video_profile (info, profile)) {
                        found_profile = profile;

                        break;
                }
        }

        return found_profile;
}

GUPnPDLNAProfile *
gupnp_dlna_profile_guesser_impl_guess_audio_profile
                                        (GUPnPDLNAInformation *info,
                                         GList                *profiles)
{
        GList *iter;
        GUPnPDLNAProfile *found_profile = NULL;

        for (iter = profiles; iter != NULL; iter = iter->next) {
                GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (iter->data);

                g_debug ("Matching audio against profile: %s",
                         gupnp_dlna_profile_get_name (profile));

                if (check_audio_profile (info, profile) &&
                    check_container_profile (info, profile)) {
                        found_profile = profile;

                        break;
                }
        }

        return found_profile;
}
