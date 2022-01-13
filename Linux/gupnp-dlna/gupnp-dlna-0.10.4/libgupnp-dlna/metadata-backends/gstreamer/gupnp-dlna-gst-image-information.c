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
#include "gupnp-dlna-gst-image-information.h"
#include "gupnp-dlna-gst-info-utils.h"

G_DEFINE_TYPE (GUPnPDLNAGstImageInformation,
               gupnp_dlna_gst_image_information,
               GUPNP_TYPE_DLNA_IMAGE_INFORMATION)

struct _GUPnPDLNAGstImageInformationPrivate {
        GstDiscovererInfo *info;
        GList *stream_list;
        GstDiscovererVideoInfo *image_info;
        GstCaps *caps;
};

enum
{
        PROP_0,

        PROP_INFO
};

static GstDiscovererVideoInfo *
get_image_info (GUPnPDLNAGstImageInformation *gst_info)
{
        GUPnPDLNAGstImageInformationPrivate *priv = gst_info->priv;

        if (!priv->image_info) {
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
                                priv->image_info =
                                             GST_DISCOVERER_VIDEO_INFO (stream);

                                break;
                        }
                }
        }

        return priv->image_info;
}

static GstCaps *
get_caps (GUPnPDLNAGstImageInformation *gst_info)
{
        GUPnPDLNAGstImageInformationPrivate *priv = gst_info->priv;

        if (!priv->caps)
                priv->caps = gst_discoverer_stream_info_get_caps
                                        (GST_DISCOVERER_STREAM_INFO
                                                   (get_image_info (gst_info)));

        return priv->caps;
}

static GUPnPDLNAIntValue
backend_get_depth (GUPnPDLNAImageInformation *self)
{
        GUPnPDLNAGstImageInformation *gst_info =
                                        GUPNP_DLNA_GST_IMAGE_INFORMATION (self);
        GstDiscovererVideoInfo *image_info = get_image_info (gst_info);
        guint data = gst_discoverer_video_info_get_depth (image_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAIntValue
backend_get_height (GUPnPDLNAImageInformation *self)
{
        GUPnPDLNAGstImageInformation* gst_info =
                                        GUPNP_DLNA_GST_IMAGE_INFORMATION (self);
        GstDiscovererVideoInfo *image_info = get_image_info (gst_info);
        guint data = gst_discoverer_video_info_get_height (image_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAIntValue
backend_get_width (GUPnPDLNAImageInformation *self)
{
        GUPnPDLNAGstImageInformation* gst_info =
                                        GUPNP_DLNA_GST_IMAGE_INFORMATION (self);
        GstDiscovererVideoInfo *image_info = get_image_info (gst_info);
        guint data = gst_discoverer_video_info_get_width (image_info);
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (data > 0 && data <= G_MAXINT) {
                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                value.value = (gint) data;
        }

        return value;
}

static GUPnPDLNAStringValue
backend_get_mime (GUPnPDLNAImageInformation *self)
{
        GUPnPDLNAGstImageInformation* gst_info =
                                        GUPNP_DLNA_GST_IMAGE_INFORMATION (self);

        return gupnp_dlna_gst_get_mime (get_caps (gst_info));
}

static void
gupnp_dlna_gst_image_information_dispose (GObject *object)
{
        GUPnPDLNAGstImageInformation *info =
                                      GUPNP_DLNA_GST_IMAGE_INFORMATION (object);
        GUPnPDLNAGstImageInformationPrivate *priv = info->priv;
        GObjectClass *parent_class =
                 G_OBJECT_CLASS (gupnp_dlna_gst_image_information_parent_class);
        g_clear_pointer (&priv->info, gupnp_dlna_gst_discoverer_info_unref);
        g_clear_pointer (&priv->stream_list,
                         gst_discoverer_stream_info_list_free);
        /* priv->image_info is freed with stream list above */
        priv->image_info = NULL;
        g_clear_pointer (&priv->caps, gst_caps_unref);
        parent_class->dispose (object);
}

static void
gupnp_dlna_gst_image_information_set_property (GObject      *object,
                                               guint         property_id,
                                               const GValue *value,
                                               GParamSpec   *pspec)
{
        GUPnPDLNAGstImageInformation *info =
                                      GUPNP_DLNA_GST_IMAGE_INFORMATION (object);
        GUPnPDLNAGstImageInformationPrivate *priv = info->priv;

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
gupnp_dlna_gst_image_information_get_property (GObject    *object,
                                               guint       property_id,
                                               GValue     *value,
                                               GParamSpec *pspec)
{
        GUPnPDLNAGstImageInformation *info =
                                      GUPNP_DLNA_GST_IMAGE_INFORMATION (object);
        GUPnPDLNAGstImageInformationPrivate *priv = info->priv;

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
gupnp_dlna_gst_image_information_class_init
                             (GUPnPDLNAGstImageInformationClass *gst_info_class)
{
        GUPnPDLNAImageInformationClass *info_class =
                            GUPNP_DLNA_IMAGE_INFORMATION_CLASS (gst_info_class);
        GObjectClass *object_class = G_OBJECT_CLASS (gst_info_class);
        GParamSpec *pspec;

        object_class->get_property =
                                  gupnp_dlna_gst_image_information_get_property;
        object_class->set_property =
                                  gupnp_dlna_gst_image_information_set_property;
        object_class->dispose = gupnp_dlna_gst_image_information_dispose;

        info_class->get_depth = backend_get_depth;
        info_class->get_height = backend_get_height;
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
                                  sizeof (GUPnPDLNAGstImageInformationPrivate));
}

static void
gupnp_dlna_gst_image_information_init (GUPnPDLNAGstImageInformation *self)
{
        GUPnPDLNAGstImageInformationPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (self,
                                         GUPNP_TYPE_DLNA_GST_IMAGE_INFORMATION,
                                         GUPnPDLNAGstImageInformationPrivate);

        priv->info = NULL;
        priv->stream_list = NULL;
        priv->image_info = NULL;
        priv->caps = NULL;
        self->priv = priv;
}

GUPnPDLNAGstImageInformation *
gupnp_dlna_gst_image_information_new_from_discoverer_info
                                        (GstDiscovererInfo *info)
{
        GList* image_list;
        GUPnPDLNAGstImageInformation *image_info = NULL;

        g_return_val_if_fail (GST_IS_DISCOVERER_INFO (info), NULL);

        image_list = gst_discoverer_info_get_video_streams (info);

        if (image_list) {
                if ((image_list->next == NULL) &&
                    gst_discoverer_video_info_is_image
                                 (GST_DISCOVERER_VIDEO_INFO (image_list->data)))
                        image_info = GUPNP_DLNA_GST_IMAGE_INFORMATION
                                (g_object_new
                                         (GUPNP_TYPE_DLNA_GST_IMAGE_INFORMATION,
                                          "info", info,
                                          NULL));
                gst_discoverer_stream_info_list_free (image_list);
        }

        return image_info;
}
