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

#include "gupnp-dlna-gst-info-utils.h"

void
gupnp_dlna_gst_discoverer_info_unref (gpointer info)
{
        gst_discoverer_info_unref (info);
}

void
gupnp_dlna_gst_discoverer_stream_info_unref (gpointer stream)
{
        gst_discoverer_stream_info_unref (stream);
}

static GUPnPDLNAStringValue
get_string_value_from_structure (const GstStructure *st,
                                 const gchar *name)
{
        GUPnPDLNAStringValue value = GUPNP_DLNA_STRING_VALUE_UNSET;

        if (st != NULL) {
                const gchar* data = gst_structure_get_string (st, name);

                if (data != NULL) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.value = g_strdup (data);
                }
        }

        return value;
}

static GUPnPDLNAStringValue
get_string_value_from_tag_list (const GstTagList *tags,
                                const gchar *name)
{
        GUPnPDLNAStringValue value = GUPNP_DLNA_STRING_VALUE_UNSET;

        if (tags != NULL) {
                const GValue *g_value = gst_tag_list_get_value_index (tags,
                                                                      name,
                                                                      0);

                if (g_value != NULL && G_VALUE_HOLDS_STRING (g_value)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.value = g_value_dup_string (g_value);
                }
        }

        return value;
}

static GUPnPDLNAIntValue
get_int_value_from_structure (const GstStructure *st,
                              const gchar *name)
{
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (st != NULL) {
                gint idata;
                guint udata;

                if (gst_structure_get_int (st, name, &idata)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.value = idata;
                } else if (gst_structure_get_uint (st, name, &udata)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.value = (gint) udata;
                }
        }

        return value;
}

static GUPnPDLNAIntValue
get_int_value_from_tag_list (const GstTagList *tags,
                             const gchar *name)
{
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (tags != NULL) {
                const GValue *g_value = gst_tag_list_get_value_index (tags,
                                                                      name,
                                                                      0);

                if (g_value != NULL) {
                        if (G_VALUE_HOLDS_INT (g_value)) {
                                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                                value.value = g_value_get_int (g_value);
                        } else if (G_VALUE_HOLDS_UINT (g_value)) {
                                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                                value.value = g_value_get_uint (g_value);
                        }
                }
        }

        return value;
}

static GUPnPDLNABoolValue
get_bool_value_from_structure (const GstStructure *st,
                               const gchar *name)
{
        GUPnPDLNABoolValue value = GUPNP_DLNA_BOOL_VALUE_UNSET;

        if (st != NULL) {
                gboolean data;

                if (gst_structure_get_boolean (st, name, &data)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.value = data;
                }
        }

        return value;
}

static GUPnPDLNABoolValue
get_bool_value_from_tag_list (const GstTagList *tags,
                              const gchar *name)
{
        GUPnPDLNABoolValue value = GUPNP_DLNA_BOOL_VALUE_UNSET;

        if (tags != NULL) {
                const GValue *g_value = gst_tag_list_get_value_index (tags,
                                                                      name,
                                                                      0);

                if (g_value != NULL && G_VALUE_HOLDS_BOOLEAN (g_value)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.value = g_value_get_boolean (g_value);
                }
        }

        return value;
}

static GUPnPDLNAFractionValue
get_fraction_value_from_structure (const GstStructure *st,
                                   const gchar *name)
{
        GUPnPDLNAFractionValue value = GUPNP_DLNA_FRACTION_VALUE_UNSET;

        if (st != NULL) {
                gint ndata;
                gint ddata;

                if (gst_structure_get_fraction (st, name, &ndata, &ddata)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.numerator = ndata;
                        value.denominator = ddata;
                }
        }

        return value;
}

static GUPnPDLNAFractionValue
get_fraction_value_from_tag_list (const GstTagList *tags,
                                  const gchar *name)
{
        GUPnPDLNAFractionValue value = GUPNP_DLNA_FRACTION_VALUE_UNSET;

        if (tags != NULL) {
                const GValue *g_value = gst_tag_list_get_value_index (tags,
                                                                      name,
                                                                      0);

                if (g_value != NULL && GST_VALUE_HOLDS_FRACTION (g_value)) {
                        value.state = GUPNP_DLNA_VALUE_STATE_SET;
                        value.numerator =
                                     gst_value_get_fraction_numerator (g_value);
                        value.denominator =
                                   gst_value_get_fraction_denominator (g_value);
                }
        }

        return value;
}

GUPnPDLNAIntValue
gupnp_dlna_gst_get_int_value (GstCaps* caps,
                              GstDiscovererStreamInfo *stream,
                              GstDiscovererInfo *info,
                              const gchar *name)
{
        GUPnPDLNAIntValue value = GUPNP_DLNA_INT_VALUE_UNSET;

        if (caps != NULL) {
                guint caps_size = gst_caps_get_size (caps);
                guint iter;

                for (iter = 0; iter < caps_size; ++iter) {
                        const GstStructure *st = gst_caps_get_structure (caps,
                                                                         iter);

                        value = get_int_value_from_structure (st, name);
                        if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                                return value;
                }
        }

        if (stream != NULL) {
                const GstStructure *st =
                                   gst_discoverer_stream_info_get_misc (stream);

                value = get_int_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (info != NULL) {
                const GstStructure *st = gst_discoverer_info_get_misc (info);

                value = get_int_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (stream != NULL) {
                const GstTagList *tags =
                                   gst_discoverer_stream_info_get_tags (stream);

                value = get_int_value_from_tag_list (tags, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        return value;
}

GUPnPDLNAStringValue
gupnp_dlna_gst_get_string_value (GstCaps* caps,
                                 GstDiscovererStreamInfo *stream,
                                 GstDiscovererInfo *info,
                                 const gchar *name)
{
        GUPnPDLNAStringValue value = GUPNP_DLNA_STRING_VALUE_UNSET;

        if (caps != NULL) {
                guint caps_size = gst_caps_get_size (caps);
                guint iter;

                for (iter = 0; iter < caps_size; ++iter) {
                        const GstStructure *st = gst_caps_get_structure (caps,
                                                                         iter);

                        value = get_string_value_from_structure (st, name);
                        if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                                return value;
                }
        }

        if (stream != NULL) {
                const GstStructure *st =
                                   gst_discoverer_stream_info_get_misc (stream);

                value = get_string_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (info != NULL) {
                const GstStructure *st = gst_discoverer_info_get_misc (info);

                value = get_string_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (stream != NULL) {
                const GstTagList *tags =
                                   gst_discoverer_stream_info_get_tags (stream);

                value = get_string_value_from_tag_list (tags, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        return value;
}

GUPnPDLNABoolValue
gupnp_dlna_gst_get_bool_value (GstCaps* caps,
                               GstDiscovererStreamInfo *stream,
                               GstDiscovererInfo *info,
                               const gchar *name)
{
        GUPnPDLNABoolValue value = GUPNP_DLNA_BOOL_VALUE_UNSET;

        if (caps != NULL) {
                guint caps_size = gst_caps_get_size (caps);
                guint iter;

                for (iter = 0; iter < caps_size; ++iter) {
                        const GstStructure *st = gst_caps_get_structure (caps,
                                                                         iter);

                        value = get_bool_value_from_structure (st, name);
                        if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                                return value;
                }
        }

        if (stream != NULL) {
                const GstStructure *st =
                                   gst_discoverer_stream_info_get_misc (stream);

                value = get_bool_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (info != NULL) {
                const GstStructure *st = gst_discoverer_info_get_misc (info);

                value = get_bool_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (stream != NULL) {
                const GstTagList *tags =
                                   gst_discoverer_stream_info_get_tags (stream);

                value = get_bool_value_from_tag_list (tags, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        return value;
}

GUPnPDLNAFractionValue
gupnp_dlna_gst_get_fraction_value (GstCaps* caps,
                                   GstDiscovererStreamInfo *stream,
                                   GstDiscovererInfo *info,
                                   const gchar *name)
{
        GUPnPDLNAFractionValue value = GUPNP_DLNA_FRACTION_VALUE_UNSET;

        if (caps != NULL) {
                guint caps_size = gst_caps_get_size (caps);
                guint iter;

                for (iter = 0; iter < caps_size; ++iter) {
                        const GstStructure *st = gst_caps_get_structure (caps,
                                                                         iter);

                        value = get_fraction_value_from_structure (st, name);
                        if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                                return value;
                }
        }

        if (stream != NULL) {
                const GstStructure *st =
                                   gst_discoverer_stream_info_get_misc (stream);

                value = get_fraction_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (info != NULL) {
                const GstStructure *st = gst_discoverer_info_get_misc (info);

                value = get_fraction_value_from_structure (st, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        if (stream != NULL) {
                const GstTagList *tags =
                                   gst_discoverer_stream_info_get_tags (stream);

                value = get_fraction_value_from_tag_list (tags, name);
                if (value.state == GUPNP_DLNA_VALUE_STATE_SET)
                        return value;
        }

        return value;
}

GUPnPDLNAStringValue
gupnp_dlna_gst_get_mime (GstCaps* caps)
{
        GUPnPDLNAStringValue value = GUPNP_DLNA_STRING_VALUE_UNSET;
        guint count = gst_caps_get_size (caps);
        guint iter;

        for (iter = 0; iter < count; ++iter) {
                const GstStructure *st = gst_caps_get_structure (caps, iter);

                if (st != NULL) {
                        const gchar *name = gst_structure_get_name (st);

                        /* just in case */
                        if (name != NULL && name[0] != '\0') {
                                value.state = GUPNP_DLNA_VALUE_STATE_SET;
                                value.value = g_strdup (name);

                                break;
                        }
                }
        }

        return value;
}
