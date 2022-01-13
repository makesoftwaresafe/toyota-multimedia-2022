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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */
#include "gupnp-dlna-gst-audio-information.h"
#include "gupnp-dlna-gst-info-utils.h"

G_DEFINE_TYPE (GUPnPDLNAGstAudioInformation,
               gupnp_dlna_gst_audio_information,
               GUPNP_TYPE_DLNA_AUDIO_INFORMATION)

struct _GUPnPDLNAGstAudioInformationPrivate {
        GstDiscovererInfo *info;
        GList *stream_list;
        GstDiscovererAudioInfo *audio_info;
        GstCaps *caps;
};

enum
{
        PROP_0,

        PROP_INFO
};

static GstDiscovererAudioInfo *
get_audio_info (GUPnPDLNAGstAudioInformation *gst_info)
{
        GUPnPDLNAGstAudioInformationPrivate *priv = gst_info->priv;

        if (!priv->audio_info) {
                GList *iter;
                gboolean adts_hack = FALSE;
                int stream_count = 0;

                if (!priv->stream_list) {
                        priv->stream_list =
                               gst_discoverer_info_get_stream_list (priv->info);
                        if (!priv->stream_list)
                                return NULL;
                }

                /* For ADTS files we get two audio streams and the important
                 * information is only on the "outer" stream which is the
                 * second stream in the stream list. If we only have audio
                 * streams, we skip the first audio stream we see
                 *
                 * Works around
                 * https://bugzilla.gnome.org/show_bug.cgi?id=699212
                 */
                iter = gst_discoverer_info_get_audio_streams (priv->info);
                stream_count = g_list_length (priv->stream_list);

                adts_hack = (stream_count == g_list_length (iter)) &&
                            stream_count == 2;
                gst_discoverer_stream_info_list_free (iter);

                for (iter = priv->stream_list; iter; iter = iter->next) {
                        GstDiscovererStreamInfo *stream =
                                        GST_DISCOVERER_STREAM_INFO (iter->data);
                        GType stream_type = G_TYPE_FROM_INSTANCE (stream);

                        /* copy caps from other stream */
                        if (adts_hack && !iter->next) {
                                priv->caps = gst_discoverer_stream_info_get_caps (stream);

                                continue;
                        }

                        if (stream_type == GST_TYPE_DISCOVERER_AUDIO_INFO) {
                                priv->audio_info =
                                             GST_DISCOVERER_AUDIO_INFO (stream);

                                if (!adts_hack)
                                        break;
                        }
                }
        }

        return priv->audio_info;
}

static GstCaps *
get_caps (GUPnPDLNAGstAudioInformation *gst_info)
{
        GUPnPDLNAGstAudioInformationPrivate *priv = gst_info->priv;
        GstDiscovererStreamInfo *info = GST_DISCOVERER_STREAM_INFO (get_audio_info (gst_info));

        if (!priv->caps)
                priv->caps = gst_discoverer_stream_info_get_caps (info);

        return priv->caps;
}

static GUPnPDLNAIntValue
get_int_value (GUPnPDLNAGstAudioInformation *gst_info,
               const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream =
                         GST_DISCOVERER_STREAM_INFO (get_audio_info (gst_info));
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_int_value (caps, stream, info, name);
}

static GUPnPDLNAStringValue
get_string_value (GUPnPDLNAGstAudioInformation *gst_info,
                  const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream =
                         GST_DISCOVERER_STREAM_INFO (get_audio_info (gst_info));
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_string_value (caps, stream, info, name);
}

static GUPnPDLNAIntValue
backend_get_bitrate (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);
        GstDiscovererAudioInfo *audio_info = get_audio_info (gst_info);
        guint data = gst_discoverer_audio_info_get_bitrate (audio_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        } else {
                value = get_int_value (gst_info, "bitrate");
        }

        return value;
}

static GUPnPDLNAIntValue
backend_get_channels (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);
        GstDiscovererAudioInfo *audio_info = get_audio_info (gst_info);
        guint data = gst_discoverer_audio_info_get_channels (audio_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAIntValue
backend_get_depth (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);
        GstDiscovererAudioInfo *audio_info = get_audio_info (gst_info);
        guint data = gst_discoverer_audio_info_get_depth (audio_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAIntValue
backend_get_layer (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return get_int_value (gst_info, "layer");
}

static GUPnPDLNAStringValue
backend_get_level (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);
        return get_string_value (gst_info, "level");
}

static GUPnPDLNAIntValue
backend_get_mpeg_audio_version (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return get_int_value (gst_info, "mpegaudioversion");
}

static GUPnPDLNAIntValue
backend_get_mpeg_version (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return get_int_value (gst_info, "mpegversion");
}

static GUPnPDLNAStringValue
backend_get_profile (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return get_string_value (gst_info, "profile");
}

static GUPnPDLNAIntValue
backend_get_rate (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);
        GstDiscovererAudioInfo *audio_info = get_audio_info (gst_info);
        guint data = gst_discoverer_audio_info_get_sample_rate (audio_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAStringValue
backend_get_stream_format (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return get_string_value (gst_info, "stream-format");
}

static GUPnPDLNAIntValue
backend_get_wma_version (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return get_int_value (gst_info, "wmaversion");
}

static GUPnPDLNAStringValue
backend_get_mime (GUPnPDLNAAudioInformation *self)
{
        GUPnPDLNAGstAudioInformation* gst_info =
                                        GUPNP_DLNA_GST_AUDIO_INFORMATION (self);

        return gupnp_dlna_gst_get_mime (get_caps (gst_info));
}

static void
gupnp_dlna_gst_audio_information_dispose (GObject *object)
{
        GUPnPDLNAGstAudioInformation *info =
                                      GUPNP_DLNA_GST_AUDIO_INFORMATION (object);
        GUPnPDLNAGstAudioInformationPrivate *priv = info->priv;
        GObjectClass *parent_class =
                 G_OBJECT_CLASS (gupnp_dlna_gst_audio_information_parent_class);

        g_clear_pointer (&priv->info, gupnp_dlna_gst_discoverer_info_unref);
        g_clear_pointer (&priv->stream_list,
                         gst_discoverer_stream_info_list_free);
        /* priv->audio_info is freed with stream list above */
        priv->audio_info = NULL;
        g_clear_pointer (&priv->caps, gst_caps_unref);
        parent_class->dispose (object);
}

static void
gupnp_dlna_gst_audio_information_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
        GUPnPDLNAGstAudioInformation *info =
                                      GUPNP_DLNA_GST_AUDIO_INFORMATION (object);
        GUPnPDLNAGstAudioInformationPrivate *priv = info->priv;

        switch (property_id) {
        case PROP_INFO:
                g_clear_pointer (&priv->info,
                                 gupnp_dlna_gst_discoverer_info_unref);
                priv->info =
                        GST_DISCOVERER_INFO (g_value_dup_object (value));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_dlna_gst_audio_information_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
        GUPnPDLNAGstAudioInformation *info =
                                      GUPNP_DLNA_GST_AUDIO_INFORMATION (object);
        GUPnPDLNAGstAudioInformationPrivate *priv = info->priv;

        switch (property_id) {
        case PROP_INFO:
                g_value_set_object (value, G_OBJECT (priv->info));
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_dlna_gst_audio_information_class_init
                             (GUPnPDLNAGstAudioInformationClass *gst_info_class)
{
        GUPnPDLNAAudioInformationClass *info_class =
                            GUPNP_DLNA_AUDIO_INFORMATION_CLASS (gst_info_class);
        GObjectClass *object_class = G_OBJECT_CLASS (gst_info_class);
        GParamSpec *pspec;

        object_class->get_property =
                                  gupnp_dlna_gst_audio_information_get_property;
        object_class->set_property =
                                  gupnp_dlna_gst_audio_information_set_property;
        object_class->dispose = gupnp_dlna_gst_audio_information_dispose;

        info_class->get_bitrate = backend_get_bitrate;
        info_class->get_channels = backend_get_channels;
        info_class->get_depth = backend_get_depth;
        info_class->get_layer = backend_get_layer;
        info_class->get_level = backend_get_level;
        info_class->get_mpeg_audio_version = backend_get_mpeg_audio_version;
        info_class->get_mpeg_version = backend_get_mpeg_version;
        info_class->get_profile = backend_get_profile;
        info_class->get_rate = backend_get_rate;
        info_class->get_stream_format = backend_get_stream_format;
        info_class->get_wma_version = backend_get_wma_version;
        info_class->get_mime = backend_get_mime;

        pspec = g_param_spec_object ("info",
                                     "info",
                                     "discoverer info",
                                     GST_TYPE_DISCOVERER_INFO,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT);
        g_object_class_install_property (object_class,
                                         PROP_INFO,
                                         pspec);

        g_type_class_add_private (gst_info_class,
                                  sizeof (GUPnPDLNAGstAudioInformationPrivate));
}

static void
gupnp_dlna_gst_audio_information_init (GUPnPDLNAGstAudioInformation *self)
{
        GUPnPDLNAGstAudioInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (self,
                                         GUPNP_TYPE_DLNA_GST_AUDIO_INFORMATION,
                                         GUPnPDLNAGstAudioInformationPrivate);

        priv->info = NULL;
        priv->stream_list = NULL;
        priv->audio_info = NULL;
        priv->caps = NULL;
        self->priv = priv;
}

GUPnPDLNAGstAudioInformation *
gupnp_dlna_gst_audio_information_new_from_discoverer_info (GstDiscovererInfo *info)
{
        GList* audio_list;
        GUPnPDLNAGstAudioInformation *audio_info = NULL;

        g_return_val_if_fail (GST_IS_DISCOVERER_INFO (info), NULL);

        audio_list = gst_discoverer_info_get_audio_streams (info);

        if (audio_list) {
                audio_info = GUPNP_DLNA_GST_AUDIO_INFORMATION (g_object_new
                                         (GUPNP_TYPE_DLNA_GST_AUDIO_INFORMATION,
                                          "info", info,
                                          NULL));
                gst_discoverer_stream_info_list_free (audio_list);
        }

        return audio_info;
}
