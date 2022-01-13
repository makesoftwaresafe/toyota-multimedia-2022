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
#include "gupnp-dlna-gst-video-information.h"
#include "gupnp-dlna-gst-info-utils.h"

G_DEFINE_TYPE (GUPnPDLNAGstVideoInformation,
               gupnp_dlna_gst_video_information,
               GUPNP_TYPE_DLNA_VIDEO_INFORMATION)

struct _GUPnPDLNAGstVideoInformationPrivate {
        GstDiscovererInfo *info;
        GList *stream_list;
        GstDiscovererVideoInfo *video_info;
        GstCaps *caps;
};

enum
{
        PROP_0,

        PROP_INFO
};

static GstDiscovererVideoInfo *
get_video_info (GUPnPDLNAGstVideoInformation *gst_info)
{
        GUPnPDLNAGstVideoInformationPrivate *priv = gst_info->priv;

        if (!priv->video_info) {
                GList *iter;

                if (!priv->stream_list) {
                        priv->stream_list =
                               gst_discoverer_info_get_stream_list (priv->info);
                        if (!priv->stream_list)
                                return NULL;
                }

                for (iter = priv->stream_list; iter; iter = iter->next) {
                        GstDiscovererStreamInfo *stream =
                                        GST_DISCOVERER_STREAM_INFO (iter->data);
                        GType stream_type = G_TYPE_FROM_INSTANCE (stream);

                        if (stream_type == GST_TYPE_DISCOVERER_VIDEO_INFO) {
                                priv->video_info =
                                             GST_DISCOVERER_VIDEO_INFO (stream);

                                break;
                        }
                }
        }

        return priv->video_info;
}

static GstCaps *
get_caps (GUPnPDLNAGstVideoInformation *gst_info)
{
        GUPnPDLNAGstVideoInformationPrivate *priv = gst_info->priv;

        if (!priv->caps)
                priv->caps = gst_discoverer_stream_info_get_caps
                                        (GST_DISCOVERER_STREAM_INFO
                                                   (get_video_info (gst_info)));

        return priv->caps;
}

static GUPnPDLNAIntValue
get_int_value (GUPnPDLNAGstVideoInformation *gst_info,
               const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream =
                         GST_DISCOVERER_STREAM_INFO (get_video_info (gst_info));
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_int_value (caps, stream, info, name);
}

static GUPnPDLNAStringValue
get_string_value (GUPnPDLNAGstVideoInformation *gst_info,
                  const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream =
                         GST_DISCOVERER_STREAM_INFO (get_video_info (gst_info));
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_string_value (caps, stream, info, name);
}

static GUPnPDLNABoolValue
get_bool_value (GUPnPDLNAGstVideoInformation *gst_info,
                const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream =
                         GST_DISCOVERER_STREAM_INFO (get_video_info (gst_info));
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_bool_value (caps, stream, info, name);
}

static GUPnPDLNAIntValue
backend_get_bitrate (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);
        GstDiscovererVideoInfo *video_info = get_video_info (gst_info);
        guint data = gst_discoverer_video_info_get_bitrate (video_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAFractionValue
backend_get_framerate (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);
        GstDiscovererVideoInfo *video_info = get_video_info (gst_info);
        guint ndata = gst_discoverer_video_info_get_framerate_num (video_info);
        guint ddata = gst_discoverer_video_info_get_framerate_denom
                                        (video_info);
        GUPnPDLNAFractionValue value = GUPNP_DLNA_FRACTION_VALUE_UNSET;

        if (ndata > 0 && ndata <= G_MAXINT && ddata > 0 && ddata <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.numerator = (gint) ndata;
                value.denominator = (gint) ddata;
        }

        return value;
}

static GUPnPDLNAIntValue
backend_get_height (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);
        GstDiscovererVideoInfo *video_info = get_video_info (gst_info);
        guint data = gst_discoverer_video_info_get_height (video_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNABoolValue
backend_is_interlaced (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);
        GstDiscovererVideoInfo *video_info = get_video_info (gst_info);
        gboolean data = gst_discoverer_video_info_get_height (video_info);
        GUPnPDLNABoolValue value;

        value.state = GUPNP_DLNA_VALUE_STATE_SET;
        value.value = data;

        return value;
}

static GUPnPDLNAStringValue
backend_get_level (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);

        return get_string_value (gst_info, "level");
}

static GUPnPDLNAIntValue
backend_get_mpeg_version (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);

        return get_int_value (gst_info, "mpegversion");
}

static GUPnPDLNAFractionValue
backend_get_pixel_aspect_ratio (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);
        GstDiscovererVideoInfo *video_info = get_video_info (gst_info);
        guint ndata = gst_discoverer_video_info_get_par_num (video_info);
        guint ddata = gst_discoverer_video_info_get_par_denom (video_info);
        GUPnPDLNAFractionValue value = GUPNP_DLNA_FRACTION_VALUE_UNSET;

        if (ndata > 0 && ndata <= G_MAXINT && ddata > 0 && ddata <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.numerator = (gint) ndata;
                value.denominator = (gint) ddata;
        }

        return value;
}

static GUPnPDLNAStringValue
backend_get_profile (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);

        return get_string_value (gst_info, "profile");
}

static GUPnPDLNABoolValue
backend_is_system_stream (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);

        return get_bool_value (gst_info, "systemstream");
}

static GUPnPDLNAIntValue
backend_get_width (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);
        GstDiscovererVideoInfo *video_info = get_video_info (gst_info);
        guint data = gst_discoverer_video_info_get_width (video_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAStringValue
backend_get_mime (GUPnPDLNAVideoInformation *self)
{
        GUPnPDLNAGstVideoInformation* gst_info =
                                        GUPNP_DLNA_GST_VIDEO_INFORMATION (self);

        return gupnp_dlna_gst_get_mime (get_caps (gst_info));
}

static void
gupnp_dlna_gst_video_information_dispose (GObject *object)
{
        GUPnPDLNAGstVideoInformation *info =
                                      GUPNP_DLNA_GST_VIDEO_INFORMATION (object);
        GUPnPDLNAGstVideoInformationPrivate *priv = info->priv;

        g_clear_pointer (&priv->info, gupnp_dlna_gst_discoverer_info_unref);
        g_clear_pointer (&priv->stream_list,
                         gst_discoverer_stream_info_list_free);
        /* priv->video_info is freed with stream list above */
        priv->video_info = NULL;
        g_clear_pointer (&priv->caps, gst_caps_unref);
        G_OBJECT_CLASS
              (gupnp_dlna_gst_video_information_parent_class)->dispose (object);
}

static void
gupnp_dlna_gst_video_information_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
        GUPnPDLNAGstVideoInformation *info =
                                      GUPNP_DLNA_GST_VIDEO_INFORMATION (object);
        GUPnPDLNAGstVideoInformationPrivate *priv = info->priv;

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
gupnp_dlna_gst_video_information_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
        GUPnPDLNAGstVideoInformation *info =
                                      GUPNP_DLNA_GST_VIDEO_INFORMATION (object);
        GUPnPDLNAGstVideoInformationPrivate *priv = info->priv;

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
gupnp_dlna_gst_video_information_class_init
                             (GUPnPDLNAGstVideoInformationClass *gst_info_class)
{
        GUPnPDLNAVideoInformationClass *info_class =
                            GUPNP_DLNA_VIDEO_INFORMATION_CLASS (gst_info_class);
        GObjectClass *object_class = G_OBJECT_CLASS (gst_info_class);
        GParamSpec *pspec;

        object_class->get_property =
                                  gupnp_dlna_gst_video_information_get_property;
        object_class->set_property =
                                  gupnp_dlna_gst_video_information_set_property;
        object_class->dispose = gupnp_dlna_gst_video_information_dispose;

        info_class->get_bitrate = backend_get_bitrate;
        info_class->get_framerate = backend_get_framerate;
        info_class->get_height = backend_get_height;
        info_class->is_interlaced = backend_is_interlaced;
        info_class->get_level = backend_get_level;
        info_class->get_mpeg_version = backend_get_mpeg_version;
        info_class->get_pixel_aspect_ratio = backend_get_pixel_aspect_ratio;
        info_class->get_profile = backend_get_profile;
        info_class->is_system_stream = backend_is_system_stream;
        info_class->get_width = backend_get_width;
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
                                  sizeof (GUPnPDLNAGstVideoInformationPrivate));
}

static void
gupnp_dlna_gst_video_information_init (GUPnPDLNAGstVideoInformation *self)
{
        GUPnPDLNAGstVideoInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (self,
                                         GUPNP_TYPE_DLNA_GST_VIDEO_INFORMATION,
                                         GUPnPDLNAGstVideoInformationPrivate);

        priv->info = NULL;
        priv->stream_list = NULL;
        priv->video_info = NULL;
        priv->caps = NULL;
        self->priv = priv;
}

GUPnPDLNAGstVideoInformation *
gupnp_dlna_gst_video_information_new_from_discoverer_info
                                        (GstDiscovererInfo *info)
{
        GList* video_list;
        GUPnPDLNAGstVideoInformation *video_info = NULL;

        g_return_val_if_fail (GST_IS_DISCOVERER_INFO (info), NULL);

        video_list = gst_discoverer_info_get_video_streams (info);

        if (video_list) {
                if (!gst_discoverer_video_info_is_image
                                 (GST_DISCOVERER_VIDEO_INFO (video_list->data)))
                        video_info = GUPNP_DLNA_GST_VIDEO_INFORMATION
                                        (g_object_new
                                         (GUPNP_TYPE_DLNA_GST_VIDEO_INFORMATION,
                                          "info", info,
                                          NULL));
                gst_discoverer_stream_info_list_free (video_list);
        }

        return video_info;
}
