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
#include "gupnp-dlna-gst-container-information.h"
#include "gupnp-dlna-gst-info-utils.h"

G_DEFINE_TYPE (GUPnPDLNAGstContainerInformation,
               gupnp_dlna_gst_container_information,
               GUPNP_TYPE_DLNA_CONTAINER_INFORMATION)

struct _GUPnPDLNAGstContainerInformationPrivate {
        GstDiscovererInfo *info;
        GstDiscovererStreamInfo *container_info;
        GstCaps *caps;
};

enum
{
        PROP_0,

        PROP_INFO
};

static GstDiscovererStreamInfo *
get_container_info (GUPnPDLNAGstContainerInformation *gst_info)
{
        GUPnPDLNAGstContainerInformationPrivate *priv = gst_info->priv;

        if (!priv->container_info) {
                priv->container_info =
                               gst_discoverer_info_get_stream_info (priv->info);
        }

        return priv->container_info;
}

static GstCaps *
get_caps (GUPnPDLNAGstContainerInformation *gst_info)
{
        GUPnPDLNAGstContainerInformationPrivate *priv = gst_info->priv;

        if (!priv->caps) {
                priv->caps = gst_discoverer_stream_info_get_caps
                                        (get_container_info (gst_info));
        }

        return priv->caps;
}

static GUPnPDLNAIntValue
get_int_value (GUPnPDLNAGstContainerInformation *gst_info,
               const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream = get_container_info (gst_info);
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_int_value (caps, stream, info, name);
}

static GUPnPDLNAStringValue
get_string_value (GUPnPDLNAGstContainerInformation *gst_info,
                  const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream = get_container_info (gst_info);
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_string_value (caps, stream, info, name);
}

static GUPnPDLNABoolValue
get_bool_value (GUPnPDLNAGstContainerInformation *gst_info,
                const gchar *name)
{
        GstCaps *caps = get_caps (gst_info);
        GstDiscovererStreamInfo *stream = get_container_info (gst_info);
        GstDiscovererInfo *info = gst_info->priv->info;

        return gupnp_dlna_gst_get_bool_value (caps, stream, info, name);
}

static GUPnPDLNAIntValue
backend_get_mpeg_version (GUPnPDLNAContainerInformation *self)
{
        GUPnPDLNAGstContainerInformation* gst_info =
                                    GUPNP_DLNA_GST_CONTAINER_INFORMATION (self);

        return get_int_value (gst_info, "mpegversion");
}

static GUPnPDLNAIntValue
backend_get_packet_size (GUPnPDLNAContainerInformation *self)
{
        GUPnPDLNAGstContainerInformation* gst_info =
                                    GUPNP_DLNA_GST_CONTAINER_INFORMATION (self);

        return get_int_value (gst_info, "packetsize");
}

static GUPnPDLNAStringValue
backend_get_profile (GUPnPDLNAContainerInformation *self)
{
        GUPnPDLNAGstContainerInformation* gst_info =
                                    GUPNP_DLNA_GST_CONTAINER_INFORMATION (self);

        return get_string_value (gst_info, "profile");
}

static GUPnPDLNABoolValue
backend_is_system_stream (GUPnPDLNAContainerInformation *self)
{
        GUPnPDLNAGstContainerInformation* gst_info =
                                    GUPNP_DLNA_GST_CONTAINER_INFORMATION (self);

        return get_bool_value (gst_info, "systemstream");
}

static GUPnPDLNAStringValue
backend_get_variant (GUPnPDLNAContainerInformation *self)
{
        GUPnPDLNAGstContainerInformation* gst_info =
                                    GUPNP_DLNA_GST_CONTAINER_INFORMATION (self);

        return get_string_value (gst_info, "variant");
}

static GUPnPDLNAStringValue
backend_get_mime (GUPnPDLNAContainerInformation *self)
{
        GUPnPDLNAGstContainerInformation* gst_info =
                                    GUPNP_DLNA_GST_CONTAINER_INFORMATION (self);

        return gupnp_dlna_gst_get_mime (get_caps (gst_info));
}

static void
gupnp_dlna_gst_container_information_dispose (GObject *object)
{
        GUPnPDLNAGstContainerInformation *info =
                                  GUPNP_DLNA_GST_CONTAINER_INFORMATION (object);
        GUPnPDLNAGstContainerInformationPrivate *priv = info->priv;
        GObjectClass *parent_class =
             G_OBJECT_CLASS (gupnp_dlna_gst_container_information_parent_class);

        g_clear_pointer (&priv->info, gupnp_dlna_gst_discoverer_info_unref);
        g_clear_pointer (&priv->container_info,
                         gupnp_dlna_gst_discoverer_stream_info_unref);
        g_clear_pointer (&priv->caps, gst_caps_unref);
        parent_class->dispose (object);
}

static void
gupnp_dlna_gst_container_information_set_property (GObject      *object,
                                                   guint         property_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec)
{
        GUPnPDLNAGstContainerInformation *info =
                                  GUPNP_DLNA_GST_CONTAINER_INFORMATION (object);
        GUPnPDLNAGstContainerInformationPrivate *priv = info->priv;

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
gupnp_dlna_gst_container_information_get_property (GObject    *object,
                                                   guint       property_id,
                                                   GValue     *value,
                                                   GParamSpec *pspec)
{
        GUPnPDLNAGstContainerInformation *info =
                                  GUPNP_DLNA_GST_CONTAINER_INFORMATION (object);
        GUPnPDLNAGstContainerInformationPrivate *priv = info->priv;

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
gupnp_dlna_gst_container_information_class_init
                         (GUPnPDLNAGstContainerInformationClass *gst_info_class)
{
        GUPnPDLNAContainerInformationClass *info_class =
                        GUPNP_DLNA_CONTAINER_INFORMATION_CLASS (gst_info_class);
        GObjectClass *object_class = G_OBJECT_CLASS (gst_info_class);
        GParamSpec *pspec;

        object_class->get_property =
                              gupnp_dlna_gst_container_information_get_property;
        object_class->set_property =
                              gupnp_dlna_gst_container_information_set_property;
        object_class->dispose = gupnp_dlna_gst_container_information_dispose;

        info_class->get_mpeg_version = backend_get_mpeg_version;
        info_class->get_packet_size = backend_get_packet_size;
        info_class->get_profile = backend_get_profile;
        info_class->is_system_stream = backend_is_system_stream;
        info_class->get_variant = backend_get_variant;
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

        g_type_class_add_private
                             (gst_info_class,
                              sizeof (GUPnPDLNAGstContainerInformationPrivate));
}

static void
gupnp_dlna_gst_container_information_init
                                        (GUPnPDLNAGstContainerInformation *self)
{
        GUPnPDLNAGstContainerInformationPrivate *priv =
                G_TYPE_INSTANCE_GET_PRIVATE
                                     (self,
                                      GUPNP_TYPE_DLNA_GST_CONTAINER_INFORMATION,
                                      GUPnPDLNAGstContainerInformationPrivate);

        priv->info = NULL;
        priv->container_info = NULL;
        priv->caps = NULL;
        self->priv = priv;
}

GUPnPDLNAGstContainerInformation *
gupnp_dlna_gst_container_information_new_from_discoverer_info
                                        (GstDiscovererInfo *info)
{
        GUPnPDLNAGstContainerInformation *container_info = NULL;
        GstDiscovererStreamInfo *stream_info;

        g_return_val_if_fail (GST_IS_DISCOVERER_INFO (info), NULL);

        stream_info = gst_discoverer_info_get_stream_info (info);

        if (stream_info) {
                GType type = G_TYPE_FROM_INSTANCE (stream_info);

                if (type == GST_TYPE_DISCOVERER_CONTAINER_INFO)
                        container_info = GUPNP_DLNA_GST_CONTAINER_INFORMATION
                                (g_object_new
                                     (GUPNP_TYPE_DLNA_GST_CONTAINER_INFORMATION,
                                      "info", info,
                                      NULL));
                gst_discoverer_stream_info_unref (stream_info);
        }

        return container_info;
}
