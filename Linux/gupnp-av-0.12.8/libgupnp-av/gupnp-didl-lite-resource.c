/*
 * Copyright (C) 2009 Nokia Corporation.
 * Copyright (C) 2007, 2008 OpenedHand Ltd.
 * Copyright (C) 2012 Intel Corporation
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshan.ali@nokia.com>
 *                                <zeeshanak@gnome.org>
 *          Jorn Baayen <jorn@openedhand.com>
 *          Krzesimir Nowak <krnowak@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:gupnp-didl-lite-resource
 * @short_description: DIDL-Lite Resource
 *
 * #GUPnPDIDLLiteResource respresent a DIDL-Lite resource (res) element.
 */

#include <string.h>

#include "gupnp-didl-lite-resource.h"
#include "gupnp-didl-lite-resource-private.h"
#include "xml-util.h"
#include "time-utils.h"
#include "xsd-data.h"

G_DEFINE_TYPE (GUPnPDIDLLiteResource,
               gupnp_didl_lite_resource,
               G_TYPE_OBJECT);

struct _GUPnPDIDLLiteResourcePrivate {
        xmlNode     *xml_node;
        GUPnPXMLDoc *xml_doc;
        xmlNs       *dlna_ns;
        xmlNs       *pv_ns;

        GUPnPProtocolInfo *protocol_info;
};

enum {
        PROP_0,
        PROP_XML_NODE,
        PROP_XML_DOC,
        PROP_DLNA_NAMESPACE,
        PROP_PV_NAMESPACE,

        PROP_URI,
        PROP_IMPORT_URI,
        PROP_PROTOCOL_INFO,

        PROP_SIZE,
        PROP_SIZE64,
        PROP_CLEAR_TEXT_SIZE,
        PROP_DURATION,
        PROP_BITRATE,
        PROP_SAMPLE_FREQ,
        PROP_BITS_PER_SAMPLE,
        PROP_PROTECTION,

        PROP_AUDIO_CHANNELS,

        PROP_WIDTH,
        PROP_HEIGHT,
        PROP_COLOR_DEPTH,

        PROP_UPDATE_COUNT,
        PROP_TRACK_TOTAL,

        PROP_SUBTITLE_FILE_TYPE,
        PROP_SUBTITLE_FILE_URI
};

static void
get_resolution_info (GUPnPDIDLLiteResource *resource,
                     int                   *width,
                     int                   *height)
{
        const char *resolution;
        char **tokens;

        resolution = xml_util_get_attribute_content (resource->priv->xml_node,
                                                     "resolution");
        if (resolution == NULL)
                return;

        tokens = g_strsplit (resolution, "x", -1);
        if (tokens == NULL || tokens[0] == NULL || tokens[1] == NULL) {
                g_warning ("Failed to resolution string '%s'\n", resolution);

                goto return_point;
        }

        if (width)
                *width = atoi (tokens[0]);
        if (height)
                *height = atoi (tokens[1]);

return_point:
        g_strfreev (tokens);
}

static void
on_protocol_info_changed (GUPnPProtocolInfo        *info,
                          G_GNUC_UNUSED GParamSpec *pspec,
                          gpointer                  user_data)
{
        GUPnPDIDLLiteResource *resource = GUPNP_DIDL_LITE_RESOURCE (user_data);

        gupnp_didl_lite_resource_set_protocol_info (resource, info);
}

static void
gupnp_didl_lite_resource_init (GUPnPDIDLLiteResource *resource)
{
        resource->priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (resource,
                                         GUPNP_TYPE_DIDL_LITE_RESOURCE,
                                         GUPnPDIDLLiteResourcePrivate);
}

static void
gupnp_didl_lite_resource_set_property (GObject      *object,
                                       guint         property_id,
                                       const GValue *value,
                                       GParamSpec   *pspec)
{
        GUPnPDIDLLiteResource *resource;

        resource = GUPNP_DIDL_LITE_RESOURCE (object);

        switch (property_id) {
        case PROP_XML_NODE:
                resource->priv->xml_node = g_value_get_pointer (value);
                break;
        case PROP_XML_DOC:
                resource->priv->xml_doc = g_value_dup_object (value);
                break;
        case PROP_URI:
                gupnp_didl_lite_resource_set_uri (resource,
                                                  g_value_get_string (value));
                break;
        case PROP_IMPORT_URI:
                gupnp_didl_lite_resource_set_import_uri
                                (resource,
                                 g_value_get_string (value));
                break;
        case PROP_PROTOCOL_INFO:
                gupnp_didl_lite_resource_set_protocol_info
                                (resource,
                                 g_value_get_object (value));
                break;
        case PROP_SIZE:
                gupnp_didl_lite_resource_set_size (resource,
                                                   g_value_get_long (value));
                break;
        case PROP_SIZE64:
                gupnp_didl_lite_resource_set_size64 (resource,
                                                     g_value_get_int64 (value));
                break;
        case PROP_DLNA_NAMESPACE:
                resource->priv->dlna_ns = g_value_get_pointer (value);
                break;
        case PROP_PV_NAMESPACE:
                resource->priv->pv_ns = g_value_get_pointer (value);
                break;
        case PROP_CLEAR_TEXT_SIZE:
                gupnp_didl_lite_resource_set_cleartext_size (resource,
                                                     g_value_get_int64 (value));
                break;
        case PROP_DURATION:
                gupnp_didl_lite_resource_set_duration
                                (resource,
                                 g_value_get_long (value));
                break;
        case PROP_BITRATE:
                gupnp_didl_lite_resource_set_bitrate (resource,
                                                      g_value_get_int (value));
                break;
        case PROP_SAMPLE_FREQ:
                gupnp_didl_lite_resource_set_sample_freq
                                (resource,
                                 g_value_get_int (value));
                break;
        case PROP_BITS_PER_SAMPLE:
                gupnp_didl_lite_resource_set_bits_per_sample
                                (resource,
                                 g_value_get_int (value));
                break;
        case PROP_PROTECTION:
                gupnp_didl_lite_resource_set_protection
                                (resource,
                                 g_value_get_string (value));
                break;
        case PROP_AUDIO_CHANNELS:
                gupnp_didl_lite_resource_set_audio_channels
                                (resource,
                                 g_value_get_int (value));
                break;
        case PROP_WIDTH:
                gupnp_didl_lite_resource_set_width (resource,
                                                    g_value_get_int (value));
                break;
        case PROP_HEIGHT:
                gupnp_didl_lite_resource_set_height (resource,
                                                     g_value_get_int (value));
                break;
        case PROP_COLOR_DEPTH:
                gupnp_didl_lite_resource_set_color_depth
                                (resource,
                                 g_value_get_int (value));
                break;
        case PROP_UPDATE_COUNT:
                gupnp_didl_lite_resource_set_update_count
                                        (resource,
                                         g_value_get_uint (value));
                break;
        case PROP_TRACK_TOTAL:
                gupnp_didl_lite_resource_set_track_total
                                        (resource,
                                         g_value_get_uint (value));
                break;
        case PROP_SUBTITLE_FILE_TYPE:
                gupnp_didl_lite_resource_set_subtitle_file_type
                                        (resource,
                                         g_value_get_string (value));
                break;
        case PROP_SUBTITLE_FILE_URI:
                gupnp_didl_lite_resource_set_subtitle_file_uri
                                        (resource,
                                         g_value_get_string (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_didl_lite_resource_get_property (GObject    *object,
                                       guint       property_id,
                                       GValue     *value,
                                       GParamSpec *pspec)
{
        GUPnPDIDLLiteResource *resource;

        resource = GUPNP_DIDL_LITE_RESOURCE (object);

        switch (property_id) {
        case PROP_XML_NODE:
                g_value_set_pointer
                        (value,
                         gupnp_didl_lite_resource_get_xml_node (resource));
                break;
        case PROP_URI:
                g_value_set_string
                        (value,
                         gupnp_didl_lite_resource_get_uri (resource));
                break;
        case PROP_IMPORT_URI:
                g_value_set_string
                        (value,
                         gupnp_didl_lite_resource_get_import_uri (resource));
                break;
        case PROP_PROTOCOL_INFO:
                g_value_set_object
                        (value,
                         gupnp_didl_lite_resource_get_protocol_info (resource));
                break;
        case PROP_SIZE:
                g_value_set_long (value,
                                  gupnp_didl_lite_resource_get_size (resource));
                break;
        case PROP_SIZE64:
                g_value_set_int64 (value,
                                   gupnp_didl_lite_resource_get_size64 (resource));
                break;
        case PROP_DLNA_NAMESPACE:
                g_value_set_pointer
                        (value,
                         gupnp_didl_lite_resource_get_dlna_namespace (resource));
                break;
        case PROP_PV_NAMESPACE:
                g_value_set_pointer
                        (value,
                         gupnp_didl_lite_resource_get_pv_namespace (resource));
                break;
        case PROP_CLEAR_TEXT_SIZE:
                g_value_set_int64
                         (value,
                          gupnp_didl_lite_resource_get_cleartext_size (resource));
                break;
        case PROP_DURATION:
                g_value_set_long
                        (value,
                         gupnp_didl_lite_resource_get_duration (resource));
                break;
        case PROP_BITRATE:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_resource_get_bitrate (resource));
                break;
        case PROP_BITS_PER_SAMPLE:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_resource_get_bits_per_sample
                                                                (resource));
                break;
        case PROP_SAMPLE_FREQ:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_resource_get_sample_freq (resource));
                break;
        case PROP_PROTECTION:
                g_value_set_string
                        (value,
                         gupnp_didl_lite_resource_get_protection (resource));
                break;
        case PROP_AUDIO_CHANNELS:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_resource_get_audio_channels
                                                                (resource));
                break;
        case PROP_WIDTH:
                g_value_set_int (value,
                                 gupnp_didl_lite_resource_get_width (resource));
                break;
        case PROP_HEIGHT:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_resource_get_height (resource));
                break;
        case PROP_COLOR_DEPTH:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_resource_get_color_depth (resource));
                break;
        case PROP_UPDATE_COUNT:
                g_value_set_uint
                         (value,
                          gupnp_didl_lite_resource_get_update_count (resource));
                break;
        case PROP_TRACK_TOTAL:
                g_value_set_uint
                         (value,
                          gupnp_didl_lite_resource_get_track_total (resource));
                break;
        case PROP_SUBTITLE_FILE_TYPE:
                g_value_set_string
                         (value,
                          gupnp_didl_lite_resource_get_subtitle_file_type
                                                (resource));
                break;
        case PROP_SUBTITLE_FILE_URI:
                g_value_set_string
                         (value,
                          gupnp_didl_lite_resource_get_subtitle_file_uri
                                                (resource));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_didl_lite_resource_dispose (GObject *object)
{
        GObjectClass                 *object_class;
        GUPnPDIDLLiteResourcePrivate *priv;

        priv = GUPNP_DIDL_LITE_RESOURCE (object)->priv;

        if (priv->xml_doc) {
                g_object_unref (priv->xml_doc);
                priv->xml_doc = NULL;
        }

        if (priv->protocol_info != NULL) {
                g_object_unref (priv->protocol_info);
                priv->protocol_info = NULL;
        }

        object_class = G_OBJECT_CLASS (gupnp_didl_lite_resource_parent_class);
        object_class->dispose (object);
}

static void
gupnp_didl_lite_resource_class_init (GUPnPDIDLLiteResourceClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = gupnp_didl_lite_resource_set_property;
        object_class->get_property = gupnp_didl_lite_resource_get_property;
        object_class->dispose = gupnp_didl_lite_resource_dispose;

        g_type_class_add_private (klass, sizeof (GUPnPDIDLLiteResourcePrivate));

        /**
         * GUPnPDIDLLiteResource:xml-node:
         *
         * The pointer to res node in XML document.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_XML_NODE,
                 g_param_spec_pointer ("xml-node",
                                       "XMLNode",
                                       "The pointer to res node in XML"
                                       " document.",
                                       G_PARAM_READWRITE |
                                       G_PARAM_CONSTRUCT_ONLY |
                                       G_PARAM_STATIC_NAME |
                                       G_PARAM_STATIC_NICK |
                                       G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:xml-doc:
         *
         * The reference to XML document containing this object.
         *
         * Internal property.
         *
         * Stability: Private
         **/
        g_object_class_install_property
                (object_class,
                 PROP_XML_DOC,
                 g_param_spec_object ("xml-doc",
                                      "XMLDoc",
                                      "The reference to XML document"
                                      " containing this object.",
                                      GUPNP_TYPE_XML_DOC,
                                      G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:uri:
         *
         * The URI associated with this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_URI,
                 g_param_spec_string ("uri",
                                      "URI",
                                      "The URI associated with this resource",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:import-uri:
         *
         * The Import URI associated with this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_IMPORT_URI,
                 g_param_spec_string ("import-uri",
                                      "ImportURI",
                                      "The import URI associated with this"
                                      " resource",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:protocol-info:
         *
         * The protocol info associated with this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_PROTOCOL_INFO,
                 g_param_spec_object ("protocol-info",
                                      "ProtocolInfo",
                                      "The protocol info associated with this"
                                      " resource",
                                      GUPNP_TYPE_PROTOCOL_INFO,
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:size:
         *
         * The size (in bytes) of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_SIZE,
                 g_param_spec_long ("size",
                                    "Size",
                                    "The size (in bytes) of this resource.",
                                    -1,
                                    G_MAXLONG,
                                    -1,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NAME |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:size64:
         *
         * The size (in bytes) of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_SIZE64,
                 g_param_spec_int64 ("size64",
                                     "Size64",
                                     "The size (in bytes) of this resource.",
                                     -1,
                                     G_MAXINT64,
                                     -1,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_NAME |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:clearTextsize:
         *
         * The size (in bytes) of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_CLEAR_TEXT_SIZE,
                 g_param_spec_int64 ("cleartext-size",
                                    "ClearTextSize",
                                    "The clear text size (in bytes) of this resource.",
                                    -1,
                                    G_MAXLONG,
                                    -1,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NAME |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:dlna-namespace:
         *
         * Pointer to the DLNA metadata namespace registered with the
         * resource object.
         *
         **/
        g_object_class_install_property
                (object_class,
                 PROP_DLNA_NAMESPACE,
                 g_param_spec_pointer ("dlna-namespace",
                                       "XML namespace",
                                       "Pointer to the DLNA metadata namespace "
                                       "registered with the resource.",
                                       G_PARAM_READWRITE |
                                       G_PARAM_CONSTRUCT_ONLY |
                                       G_PARAM_STATIC_NAME |
                                       G_PARAM_STATIC_NICK |
                                       G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:pv-namespace:
         *
         * Pointer to the PV metadata namespace registered with the
         * resource object.
         *
         **/
        g_object_class_install_property
                (object_class,
                 PROP_PV_NAMESPACE,
                 g_param_spec_pointer ("pv-namespace",
                                       "XML namespace",
                                       "Pointer to the PV metadata namespace "
                                       "registered with the resource.",
                                       G_PARAM_READWRITE |
                                       G_PARAM_CONSTRUCT_ONLY |
                                       G_PARAM_STATIC_STRINGS));

        /**
         * GUPnPDIDLLiteResource:duration:
         *
         * The duration (in seconds) of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_DURATION,
                 g_param_spec_long ("duration",
                                    "Duration",
                                    "The duration (in seconds) of this"
                                    " resource.",
                                    -1,
                                    G_MAXLONG,
                                    -1,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_NAME |
                                    G_PARAM_STATIC_NICK |
                                    G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:bitrate:
         *
         * The bitrate of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_BITRATE,
                 g_param_spec_int ("bitrate",
                                   "Bitrate",
                                   "The bitrate of this resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:sample-freq:
         *
         * The sample frequency of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_SAMPLE_FREQ,
                 g_param_spec_int ("sample-freq",
                                   "SampleFrequency",
                                   "The sample frequency of this resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:bits-per-sample:
         *
         * The sample size of this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_BITS_PER_SAMPLE,
                 g_param_spec_int ("bits-per-sample",
                                   "BitsPerSample",
                                   "The sample size of this resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:protection:
         *
         * The protection system used for this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_PROTECTION,
                 g_param_spec_string ("protection",
                                      "Protection",
                                      "The protection system used by this"
                                      " resource.",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:audio-channels:
         *
         * The number of audio channels in this resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_AUDIO_CHANNELS,
                 g_param_spec_int ("audio-channels",
                                   "AudioChannels",
                                   "The number of audio channels in this"
                                   " resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:width:
         *
         * The width of this image/video resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_WIDTH,
                 g_param_spec_int ("width",
                                   "Width",
                                   "The width of this image/video resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:height:
         *
         * The height of this image/video resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_HEIGHT,
                 g_param_spec_int ("height",
                                   "Height",
                                   "The height of this image/video resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:color-depth:
         *
         * The color-depth of this image/video resource.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_COLOR_DEPTH,
                 g_param_spec_int ("color-depth",
                                   "ColorDepth",
                                   "The color-depth of this image/video"
                                   " resource.",
                                   -1,
                                   G_MAXINT,
                                   -1,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteResource:update_count:
         *
         * The update count of this resource.
         **/
        g_object_class_install_property
                       (object_class,
                        PROP_UPDATE_COUNT,
                        g_param_spec_uint ("update-count",
                                           "UpdateCount",
                                           "The update count of this resource.",
                                           0,
                                           G_MAXUINT,
                                           0,
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_NAME |
                                           G_PARAM_STATIC_NICK |
                                           G_PARAM_STATIC_BLURB));
        /**
         * GUPnPDIDLLiteResource:track-total:
         *
         * Number of tracks in a DIDL_S or DIDL_V resource.
         **/
        g_object_class_install_property
                       (object_class,
                        PROP_TRACK_TOTAL,
                        g_param_spec_uint ("track-total",
                                           "TrackTotal",
                                           "The number of tracks of this "
                                           "resource.",
                                           0,
                                           G_MAXUINT,
                                           0,
                                           G_PARAM_READWRITE |
                                           G_PARAM_STATIC_STRINGS));

        /**
         * GUPnPDIDLLiteResource:subtitle-file-type:
         *
         * Type of external subtitle file. Usually SRT or SMI.
         **/
        g_object_class_install_property
                        (object_class,
                         PROP_SUBTITLE_FILE_TYPE,
                         g_param_spec_string ("subtitle-file-type",
                                              "Subtitle file type",
                                              "Type of the external subtitle "
                                              "file",
                                              NULL,
                                              G_PARAM_READWRITE |
                                              G_PARAM_STATIC_STRINGS));

        /**
         * GUPnPDIDLLiteResource:subtitle-file-uri:
         *
         * Uri to external subtitle file.
         **/
        g_object_class_install_property
                        (object_class,
                         PROP_SUBTITLE_FILE_TYPE,
                         g_param_spec_string ("subtitle-file-uri",
                                              "Subtitle file uri",
                                              "Uri of the external subtitle "
                                              "file",
                                              NULL,
                                              G_PARAM_READWRITE |
                                              G_PARAM_STATIC_STRINGS));
}

/**
 * gupnp_didl_lite_resource_new_from_xml:
 * @xml_node: The pointer to 'res' node in XML document
 * @xml_doc: The reference to XML document containing this resource
 *
 * Creates a new #GUPnPDIDLLiteResource for the @xml_node.
 *
 * Return value: A new #GUPnPDIDLLiteResource object. Unref after usage.
 **/
GUPnPDIDLLiteResource *
gupnp_didl_lite_resource_new_from_xml (xmlNode     *xml_node,
                                       GUPnPXMLDoc *xml_doc,
                                       xmlNs       *dlna_ns,
                                       xmlNs       *pv_ns)
{
        return g_object_new (GUPNP_TYPE_DIDL_LITE_RESOURCE,
                             "xml-node", xml_node,
                             "xml-doc", xml_doc,
                             "dlna-namespace", dlna_ns,
                             "pv-namespace", pv_ns,
                             NULL);
}

/**
 * gupnp_didl_lite_resource_get_xml_node:
 * @resource: The #GUPnPDIDLLiteResource
 *
 * Get the pointer to res node in XML document.
 *
 * Returns: (transfer none): The pointer to res node in XML document.
 **/
xmlNode *
gupnp_didl_lite_resource_get_xml_node (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return resource->priv->xml_node;
}

/**
 * gupnp_didl_lite_resource_get_dlna_namespace:
 * @resource: The #GUPnPDIDLLiteObject
 *
 * Get the pointer to the DLNA metadata namespace registered with the XML
 * document containing this object.
 *
 * Returns: (transfer none): The pointer to DLNA namespace in XML document.
 **/
xmlNsPtr
gupnp_didl_lite_resource_get_dlna_namespace (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return resource->priv->dlna_ns;
}

/**
 * gupnp_didl_lite_resource_get_pv_namespace:
 * @resource: The #GUPnPDIDLLiteObject
 *
 * Get the pointer to the DLNA metadata namespace registered with the XML
 * document containing this object.
 *
 * Returns: (transfer none): The pointer to DLNA namespace in XML document.
 **/
xmlNsPtr
gupnp_didl_lite_resource_get_pv_namespace (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return resource->priv->pv_ns;
}


/**
 * gupnp_didl_lite_resource_get_uri:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the URI associated with the @resource.
 *
 * Return value: The of URI the @resource or %NULL.
 **/
const char *
gupnp_didl_lite_resource_get_uri (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        if (G_UNLIKELY (resource->priv->xml_node->children == NULL))
                return NULL;

        return (const char *) resource->priv->xml_node->children->content;
}

/**
 * gupnp_didl_lite_resource_get_import_uri:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the import URI associated with the @resource.
 *
 * Return value: The import URI or %NULL.
 **/
const char *
gupnp_didl_lite_resource_get_import_uri (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return xml_util_get_attribute_content (resource->priv->xml_node,
                                               "importUri");
}

/**
 * gupnp_didl_lite_resource_get_protocol_info:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the protocol info associated with the @resource.
 *
 * Returns: (transfer none): The protocol info associated with the @resource or %NULL. The
 * returned object must not be unrefed.
 **/
GUPnPProtocolInfo *
gupnp_didl_lite_resource_get_protocol_info (GUPnPDIDLLiteResource *resource)
{
        GUPnPProtocolInfo *info;
        const char *protocol_info;
        GError *error;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        if (resource->priv->protocol_info != NULL)
                return resource->priv->protocol_info;

        protocol_info = xml_util_get_attribute_content
                                        (resource->priv->xml_node,
                                         "protocolInfo");
        if (protocol_info == NULL)
                return NULL;

        error = NULL;
        info = gupnp_protocol_info_new_from_string (protocol_info, &error);
        if (info == NULL) {
                g_warning ("Error parsing protocolInfo '%s': %s",
                           protocol_info,
                           error->message);

                g_error_free (error);
        }

        resource->priv->protocol_info = info;

        return info;
}

/**
 * gupnp_didl_lite_resource_get_size:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the size (in bytes) of the @resource.
 *
 * Return value: The size (in bytes) of the @resource or -1.
 **/
glong
gupnp_didl_lite_resource_get_size (GUPnPDIDLLiteResource *resource)
{
    return (glong) gupnp_didl_lite_resource_get_size64 (resource);
}

/**
 * gupnp_didl_lite_resource_get_size64:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the size (in bytes) of the @resource.
 *
 * Return value: The size (in bytes) of the @resource or -1.
 **/
gint64
gupnp_didl_lite_resource_get_size64 (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_int64_attribute (resource->priv->xml_node,
                                             "size",
                                             -1);
}

/**
 * gupnp_didl_lite_resource_get_cleartext_size:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the size (in bytes) of the @resource.
 *
 * Return value: The size (in bytes) of the @resource or -1.
 **/
gint64
gupnp_didl_lite_resource_get_cleartext_size (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_int64_attribute (resource->priv->xml_node,
                                             "cleartextSize",
                                             -1);
}

/**
 * gupnp_didl_lite_resource_get_duration:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the duration (in seconds) of the @resource.
 *
 * Return value: The duration (in seconds) of the @resource or -1.
 **/
glong
gupnp_didl_lite_resource_get_duration (GUPnPDIDLLiteResource *resource)
{
        const char *duration_str;
        long duration;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        duration_str = xml_util_get_attribute_content (resource->priv->xml_node,
                                                       "duration");
        duration = seconds_from_time (duration_str);

        return duration;
}

/**
 * gupnp_didl_lite_resource_get_bitrate:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the bitrate (in bytes per second) of the @resource.
 *
 * Return value: The bitrate (in bytes per second) of the @resource or -1.
 **/
int
gupnp_didl_lite_resource_get_bitrate (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_long_attribute (resource->priv->xml_node,
                                            "bitrate",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_get_sample_freq:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the sample frequency of the @resource.
 *
 * Return value: The sample frequency of the @resource or -1.
 **/
int
gupnp_didl_lite_resource_get_sample_freq (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_long_attribute (resource->priv->xml_node,
                                            "sampleFrequency",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_get_bits_per_sample:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the sample size of the @resource.
 *
 * Return value: The number of bits per sample of the @resource or -1.
 **/
int
gupnp_didl_lite_resource_get_bits_per_sample (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_long_attribute (resource->priv->xml_node,
                                            "bitsPerSample",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_get_protection:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the protection system used by the @resource.
 *
 * Return value: The protection system in use by the @resource or %NULL.
 **/
const char *
gupnp_didl_lite_resource_get_protection (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return xml_util_get_attribute_content (resource->priv->xml_node,
                                               "protection");
}

/**
 * gupnp_didl_lite_resource_get_audio_channels:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the number of audio channels in the @resource.
 *
 * Return value: The number of audio channels in the @resource or -1.
 **/
int
gupnp_didl_lite_resource_get_audio_channels (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_long_attribute (resource->priv->xml_node,
                                            "nrAudioChannels",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_get_width:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the width of this image/video resource.
 *
 * Return value: The width of this image/video resource or -1.
 **/
int
gupnp_didl_lite_resource_get_width (GUPnPDIDLLiteResource *resource)
{
        int width = -1;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        get_resolution_info (resource, &width, NULL);

        return width;
}

/**
 * gupnp_didl_lite_resource_get_height:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the height of this image/video resource.
 *
 * Return value: The height of the @resource or -1.
 **/
int
gupnp_didl_lite_resource_get_height (GUPnPDIDLLiteResource *resource)
{
        int height = -1;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        get_resolution_info (resource, NULL, &height);

        return height;
}

/**
 * gupnp_didl_lite_resource_get_color_depth:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the color-depth of this image/video resource.
 *
 * Return value: The color depth of the @resource or -1.
 **/
int
gupnp_didl_lite_resource_get_color_depth (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), -1);

        return xml_util_get_long_attribute (resource->priv->xml_node,
                                            "colorDepth",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_get_update_count:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the update count of this resource.
 *
 * Return value: The update count of the @resource.
 **/
guint
gupnp_didl_lite_resource_get_update_count (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), 0);

        return xml_util_get_uint_attribute (resource->priv->xml_node,
                                            "updateCount",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_get_track_total:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Get the total track count of this resource.
 *
 * Return value: The total track count of the @resource.
 **/
guint
gupnp_didl_lite_resource_get_track_total (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), 0);

        return xml_util_get_uint_attribute (resource->priv->xml_node,
                                            "trackTotal",
                                            -1);
}

/**
 * gupnp_didl_lite_resource_update_count_is_set:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Check whether the update count property of this resource is set.
 *
 * Return value: %TRUE if set, otherwise %FALSE.
 **/
gboolean
gupnp_didl_lite_resource_update_count_is_set (GUPnPDIDLLiteResource *resource)
{
        const char *content;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), FALSE);

        content = xml_util_get_attribute_content (resource->priv->xml_node,
                                                  "updateCount");
        return content != NULL;
}

/**
 * gupnp_didl_lite_resource_track_total_is_set:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Check whether the total track count property of this resource is set.
 *
 * Return value: %TRUE if set, otherwise %FALSE.
 **/
gboolean
gupnp_didl_lite_resource_track_total_is_set (GUPnPDIDLLiteResource *resource)
{
        const char *content;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), FALSE);

        content = xml_util_get_attribute_content (resource->priv->xml_node,
                                                  "trackTotal");
        return content != NULL;
}


/**
 * gupnp_didl_lite_resource_set_uri:
 * @resource: A #GUPnPDIDLLiteResource
 * @uri: The URI as string
 *
 * Set the URI associated with the @resource.
 **/
void
gupnp_didl_lite_resource_set_uri (GUPnPDIDLLiteResource *resource,
                                  const char            *uri)
{
        xmlChar *escaped;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));
        g_return_if_fail (uri != NULL);

        escaped = xmlEncodeSpecialChars (resource->priv->xml_doc->doc,
                                         (const unsigned char *) uri);
        xmlNodeSetContent (resource->priv->xml_node, escaped);
        xmlFree (escaped);

        g_object_notify (G_OBJECT (resource), "uri");
}

/**
 * gupnp_didl_lite_resource_set_import_uri:
 * @resource: A #GUPnPDIDLLiteResource
 * @import_uri: The URI as string
 *
 * Set the import URI associated with the @resource.
 **/
void
gupnp_didl_lite_resource_set_import_uri (GUPnPDIDLLiteResource *resource,
                                         const char            *import_uri)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        xmlSetProp (resource->priv->xml_node,
                    (unsigned char *) "importUri",
                    (unsigned char *) import_uri);

        g_object_notify (G_OBJECT (resource), "import-uri");
}

/**
 * gupnp_didl_lite_resource_set_protocol_info:
 * @resource: A #GUPnPDIDLLiteResource
 * @info: The protocol string
 *
 * Set the protocol info associated with the @resource.
 **/
void
gupnp_didl_lite_resource_set_protocol_info (GUPnPDIDLLiteResource *resource,
                                            GUPnPProtocolInfo     *info)
{
        char *str;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));
        g_return_if_fail (GUPNP_IS_PROTOCOL_INFO (info));

        str = gupnp_protocol_info_to_string (info);
        xmlSetProp (resource->priv->xml_node,
                    (unsigned char *) "protocolInfo",
                    (unsigned char *) str);
        g_free (str);

        /* Get a ref first in case it's the same object that we already have */
        g_object_ref (info);
        if (resource->priv->protocol_info != NULL)
                g_object_unref (resource->priv->protocol_info);
        resource->priv->protocol_info = info;

        /* We need to listen to changes to properties so we update the
         * corresponding xml property.
         */
        g_signal_handlers_disconnect_by_func (info,
                                              on_protocol_info_changed,
                                              resource);
        g_signal_connect (info,
                          "notify",
                          G_CALLBACK (on_protocol_info_changed),
                          resource);

        g_object_notify (G_OBJECT (resource), "protocol-info");
}

/**
 * gupnp_didl_lite_resource_set_size:
 * @resource: A #GUPnPDIDLLiteResource
 * @size: The size (in bytes)
 *
 * Set the size (in bytes) of the @resource. Passing a negative number will
 * unset this property.
 **/
void
gupnp_didl_lite_resource_set_size (GUPnPDIDLLiteResource *resource,
                                   glong                  size)
{
        gupnp_didl_lite_resource_set_size64 (resource, size);
}

/**
 * gupnp_didl_lite_resource_set_size64:
 * @resource: A #GUPnPDIDLLiteResource
 * @size: The size (in bytes)
 *
 * Set the size (in bytes) of the @resource. Passing a negative number will
 * unset this property.
 **/
void
gupnp_didl_lite_resource_set_size64 (GUPnPDIDLLiteResource *resource,
                                     gint64                 size)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (size < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "size");
        else {
                char *str;

                str = g_strdup_printf ("%" G_GINT64_FORMAT, size);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "size",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "size64");
        g_object_notify (G_OBJECT (resource), "size");
}

/**
 * gupnp_didl_lite_resource_set_cleartext_size:
 * @resource: A #GUPnPDIDLLiteResource
 * @cleartext_size: The size (in bytes)
 *
 * Set the size (in bytes) of the @resource. Passing a negative number will
 * unset this property.
 **/
void
gupnp_didl_lite_resource_set_cleartext_size
                                        (GUPnPDIDLLiteResource *resource,
                                         gint64                 cleartext_size)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (cleartext_size < 0)
                xmlUnsetNsProp (resource->priv->xml_node,
                                resource->priv->dlna_ns,
                                (unsigned char *) "cleartextSize");
        else {
                char *str;
                str = g_strdup_printf ("%" G_GINT64_FORMAT, cleartext_size);
                xml_util_get_ns (resource->priv->xml_doc->doc,
                                 GUPNP_XML_NAMESPACE_DLNA,
                                 &(resource->priv->dlna_ns));

                xmlSetNsProp (resource->priv->xml_node,
                              resource->priv->dlna_ns,
                              (unsigned char *) "cleartextSize",
                              (unsigned char *) str);

                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "cleartext-size");
}

/**
 * gupnp_didl_lite_resource_set_duration:
 * @resource: A #GUPnPDIDLLiteResource
 * @duration: The duration (in seconds)
 *
 * Set the duration (in seconds) of the @resource. Passing a negative number
 * will unset this property.
 **/
void
gupnp_didl_lite_resource_set_duration (GUPnPDIDLLiteResource *resource,
                                       glong                  duration)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (duration < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "duration");
        else {
                char *str;

                str = seconds_to_time (duration);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "duration",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "duration");
}

/**
 * gupnp_didl_lite_resource_set_bitrate:
 * @resource: A #GUPnPDIDLLiteResource
 * @bitrate: The bitrate
 *
 * Set the bitrate (in bytes per second) of the @resource. Passing a negative
 * number will unset this property.
 **/
void
gupnp_didl_lite_resource_set_bitrate (GUPnPDIDLLiteResource *resource,
                                      int                    bitrate)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (bitrate < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "bitrate");
        else {
                char *str;

                str = g_strdup_printf ("%d", bitrate);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "bitrate",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "bitrate");
}

/**
 * gupnp_didl_lite_resource_set_sample_freq:
 * @resource: A #GUPnPDIDLLiteResource
 * @sample_freq: The sample frequency
 *
 * Set the sample frequency of the @resource. Passing a negative number will
 * unset this property.
 **/
void
gupnp_didl_lite_resource_set_sample_freq (GUPnPDIDLLiteResource *resource,
                                          int                    sample_freq)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (sample_freq < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "sampleFrequency");
        else {
                char *str;

                str = g_strdup_printf ("%d", sample_freq);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "sampleFrequency",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "sample-freq");
}

/**
 * gupnp_didl_lite_resource_set_bits_per_sample:
 * @resource: A #GUPnPDIDLLiteResource
 * @sample_size: The number of bits per sample
 *
 * Set the sample size of the @resource. Passing a negative number will unset
 * this property.
 **/
void
gupnp_didl_lite_resource_set_bits_per_sample
                                        (GUPnPDIDLLiteResource *resource,
                                         int                    sample_size)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (sample_size < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "bitsPerSample");
        else {
                char *str;

                str = g_strdup_printf ("%d", sample_size);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "bitsPerSample",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "bits-per-sample");
}

/**
 * gupnp_didl_lite_resource_set_protection:
 * @resource: A #GUPnPDIDLLiteResource
 * @protection: The protection system identifier as string
 *
 * Set the protection system used by the @resource. Passing a negative number
 * will unset this property.
 **/
void
gupnp_didl_lite_resource_set_protection (GUPnPDIDLLiteResource *resource,
                                         const char            *protection)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        xmlSetProp (resource->priv->xml_node,
                    (unsigned char *) "protection",
                    (unsigned char *) protection);

        g_object_notify (G_OBJECT (resource), "protection");
}

/**
 * gupnp_didl_lite_resource_set_audio_channels:
 * @resource: A #GUPnPDIDLLiteResource
 * @n_channels: The number of channels
 *
 * Set the number of audio channels in the @resource. Passing a negative number
 * will unset this property.
 **/
void
gupnp_didl_lite_resource_set_audio_channels (GUPnPDIDLLiteResource *resource,
                                             int                    n_channels)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (n_channels < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "nrAudioChannels");
        else {
                char *str;

                str = g_strdup_printf ("%d", n_channels);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "nrAudioChannels",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "audio-channels");
}

/**
 * gupnp_didl_lite_resource_set_width:
 * @resource: A #GUPnPDIDLLiteResource
 * @width: The width
 *
 * Set the width of this image/video resource. Setting both width and height to
 * a negative number will unset the resolution property.
 **/
void
gupnp_didl_lite_resource_set_width (GUPnPDIDLLiteResource *resource,
                                     int                   width)
{
        char *resolution;
        int height = -1;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        get_resolution_info (resource, NULL, &height);

        if (width < 0 && height < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "resolution");
        else {
                resolution = g_strdup_printf ("%dx%d", width, height);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "resolution",
                            (unsigned char *) resolution);
                g_free (resolution);
        }

        g_object_notify (G_OBJECT (resource), "width");
}

/**
 * gupnp_didl_lite_resource_set_height:
 * @resource: A #GUPnPDIDLLiteResource
 * @height: The height
 *
 * Set the height of this image/video resource. Setting both width and height to
 * a negative number will unset the resolution property.
 **/
void
gupnp_didl_lite_resource_set_height (GUPnPDIDLLiteResource *resource,
                                     int                    height)
{
        int width = -1;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        get_resolution_info (resource, &width, NULL);

        if (width < 0 && height < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "resolution");
        else {
                char *resolution;

                resolution = g_strdup_printf ("%dx%d", width, height);
                xmlSetProp (resource->priv->xml_node,
                                (unsigned char *) "resolution",
                                (unsigned char *) resolution);
                g_free (resolution);
        }

        g_object_notify (G_OBJECT (resource), "height");
}

/**
 * gupnp_didl_lite_resource_set_color_depth:
 * @resource: A #GUPnPDIDLLiteResource
 * @color_depth: The color-depth
 *
 * Set the color-depth of this image/video resource. Passing a negative number
 * will unset this property.
 **/
void
gupnp_didl_lite_resource_set_color_depth (GUPnPDIDLLiteResource *resource,
                                          int                    color_depth)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (color_depth < 0)
                xmlUnsetProp (resource->priv->xml_node,
                              (unsigned char *) "colorDepth");
        else {
                char *str;

                str = g_strdup_printf ("%d", color_depth);
                xmlSetProp (resource->priv->xml_node,
                            (unsigned char *) "colorDepth",
                            (unsigned char *) str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (resource), "color-depth");
}

/**
 * gupnp_didl_lite_resource_set_update_count:
 * @resource: A #GUPnPDIDLLiteResource
 * @update_count: The update_count
 *
 * Set the update count of this resource.
 **/
void
gupnp_didl_lite_resource_set_update_count (GUPnPDIDLLiteResource *resource,
                                          guint                  update_count)
{
        char *str;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        str = g_strdup_printf ("%u", update_count);
        xmlSetProp (resource->priv->xml_node,
                    (unsigned char *) "updateCount",
                    (unsigned char *) str);
        g_free (str);

        g_object_notify (G_OBJECT (resource), "update-count");
}

/**
 * gupnp_didl_lite_resource_set_track_total:
 * @resource: A #GUPnPDIDLLiteResource
 * @track_total: The total number of tracks in this resource
 *
 * Set the total number of tracks in this resource.
 **/
void
gupnp_didl_lite_resource_set_track_total (GUPnPDIDLLiteResource *resource,
                                          guint                  track_total)
{
        char *str;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        xml_util_get_ns (resource->priv->xml_doc->doc,
                         GUPNP_XML_NAMESPACE_DLNA,
                         &(resource->priv->dlna_ns));

        str = g_strdup_printf ("%u", track_total);
        xmlSetNsProp (resource->priv->xml_node,
                      resource->priv->dlna_ns,
                      (unsigned char *) "trackTotal",
                      (unsigned char *) str);
        g_free (str);

        g_object_notify (G_OBJECT (resource), "track-total");
}


/**
 * gupnp_didl_lite_resource_unset_update_count:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Unset the update count of this resource.
 **/
void
gupnp_didl_lite_resource_unset_update_count (GUPnPDIDLLiteResource *resource)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        xmlUnsetProp (resource->priv->xml_node,
                      (unsigned char *) "updateCount");

        g_object_notify (G_OBJECT (resource), "update-count");
}

/**
 * gupnp_didl_lite_resource_unset_track_total:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Unset the total track count of this resource.
 **/
void
gupnp_didl_lite_resource_unset_track_total (GUPnPDIDLLiteResource *resource)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        xmlUnsetNsProp (resource->priv->xml_node,
                        resource->priv->dlna_ns,
                        (unsigned char *) "trackTotal");

        g_object_notify (G_OBJECT (resource), "track-total");
}

/**
 * gupnp_didl_lite_resource_get_subtitle_file_uri:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Returns: The content of the subtitleFileUri property or %NULL when not set.
 *
 * Since: 0.12.4
 **/
const char *
gupnp_didl_lite_resource_get_subtitle_file_uri
                                        (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return xml_util_get_attribute_content (resource->priv->xml_node,
                                               "subtitleFileUri");
}

/**
 * gupnp_didl_lite_resource_get_subtitle_file_type:
 * @resource: A #GUPnPDIDLLiteResource
 *
 * Returns: The content of the subtitleFileType property or %NULL
 *
 * Since: 0.12.4
 **/
const char *
gupnp_didl_lite_resource_get_subtitle_file_type
                                        (GUPnPDIDLLiteResource *resource)
{
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource), NULL);

        return xml_util_get_attribute_content (resource->priv->xml_node,
                                               "subtitleFileType");
}

/**
 * gupnp_didl_lite_resource_set_subtitle_file_uri:
 * @resource: A #GUPnPDIDLLiteResource
 * @uri: (allow-none):  An URI to an external subtitle file or %NULL to remove.
 *
 * Set the URI of an external subtitle file to be used with this resource.
 * When @uri is %NULL the value is removed.
 *
 * Since: 0.12.4
 **/
void
gupnp_didl_lite_resource_set_subtitle_file_uri
                                        (GUPnPDIDLLiteResource *resource,
                                         const char            *uri)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (uri == NULL)
                xmlUnsetNsProp (resource->priv->xml_node,
                                resource->priv->pv_ns,
                                (unsigned char *) "subtitleFileUri");
        else {
                xml_util_get_ns (resource->priv->xml_doc->doc,
                                 GUPNP_XML_NAMESPACE_PV,
                                 &(resource->priv->pv_ns));
                xmlSetNsProp (resource->priv->xml_node,
                              resource->priv->pv_ns,
                              (unsigned char *) "subtitleFileUri",
                              (unsigned char *) uri);
        }

        g_object_notify (G_OBJECT (resource), "subtitle-file-uri");
}

/**
 * gupnp_didl_lite_resource_set_subtitle_file_type:
 * @resource: A #GUPnPDIDLLiteResource
 * @type: (allow-none): An URI to an external subtitle file
 *
 * Set the type of an external subtitle file, specified via
 * pv:subtitleFileUri using gupnp_didl_lite_resource_set_subtitle_file_uri().
 *
 * When @type is %NULL the value is removed.
 *
 * Since: 0.12.4
 **/
void
gupnp_didl_lite_resource_set_subtitle_file_type
                                        (GUPnPDIDLLiteResource *resource,
                                         const char            *type)
{
        g_return_if_fail (GUPNP_IS_DIDL_LITE_RESOURCE (resource));

        if (type == NULL)
                xmlUnsetNsProp (resource->priv->xml_node,
                                resource->priv->pv_ns,
                                (unsigned char *) "subtitleFileUri");
        else {
                xml_util_get_ns (resource->priv->xml_doc->doc,
                                 GUPNP_XML_NAMESPACE_PV,
                                 &(resource->priv->pv_ns));

                xmlSetNsProp (resource->priv->xml_node,
                              resource->priv->pv_ns,
                              (unsigned char *) "subtitleFileType",
                              (unsigned char *) type);
        }

        g_object_notify (G_OBJECT (resource), "subtitle-file-type");
}
