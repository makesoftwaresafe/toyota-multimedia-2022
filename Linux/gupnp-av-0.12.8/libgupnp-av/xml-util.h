/*
 * Copyright (C) 2006, 2007 OpenedHand Ltd.
 * Copyright (C) 2007 Zeeshan Ali.
 * Copyright (C) 2012 Intel Corporation
 *
 * Author: Jorn Baayen <jorn@openedhand.com>
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 * Author: Krzesimir Nowak <krnowak@openismus.com>
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

#ifndef __XML_UTIL_H__
#define __XML_UTIL_H__

#include <glib.h>
#include <libxml/tree.h>
#include <stdarg.h>

typedef enum _GUPnPXMLNamespace
{
        GUPNP_XML_NAMESPACE_DIDL_LITE,
        GUPNP_XML_NAMESPACE_DC,
        GUPNP_XML_NAMESPACE_DLNA,
        GUPNP_XML_NAMESPACE_PV,
        GUPNP_XML_NAMESPACE_UPNP,
        GUPNP_XML_NAMESPACE_COUNT
} GUPnPXMLNamespace;

G_BEGIN_DECLS

/* Misc utilities for inspecting xmlNodes */
G_GNUC_INTERNAL xmlNode *
xml_util_get_element                    (xmlNode    *node,
                                         ...) G_GNUC_NULL_TERMINATED;

G_GNUC_INTERNAL GList *
xml_util_get_child_elements_by_name     (xmlNode *node,
                                         const char *name);

G_GNUC_INTERNAL const char *
xml_util_get_child_element_content      (xmlNode    *node,
                                         const char *child_name);

G_GNUC_INTERNAL guint
xml_util_get_uint_child_element         (xmlNode    *node,
                                         const char *child_name,
                                         guint       default_value);

G_GNUC_INTERNAL guint64
xml_util_get_uint64_child_element       (xmlNode    *node,
                                         const char *child_name,
                                         guint64     default_value);

G_GNUC_INTERNAL const char *
xml_util_get_attribute_content          (xmlNode    *node,
                                         const char *attribute_name);

G_GNUC_INTERNAL gboolean
xml_util_get_boolean_attribute          (xmlNode    *node,
                                         const char *attribute_name);

G_GNUC_INTERNAL guint
xml_util_get_uint_attribute             (xmlNode    *node,
                                         const char *attribute_name,
                                         guint       default_value);

G_GNUC_INTERNAL gint
xml_util_get_int_attribute              (xmlNode    *node,
                                         const char *attribute_name,
                                         gint        default_value);

G_GNUC_INTERNAL glong
xml_util_get_long_attribute             (xmlNode    *node,
                                         const char *attribute_name,
                                         glong       default_value);

G_GNUC_INTERNAL gint64
xml_util_get_int64_attribute            (xmlNode    *node,
                                         const char *attribute_name,
                                         gint64      default_value);

G_GNUC_INTERNAL xmlNode *
xml_util_set_child                      (xmlNode          *parent_node,
                                         GUPnPXMLNamespace ns,
                                         xmlNsPtr         *namespace,
                                         xmlDoc           *doc,
                                         const char       *name,
                                         const char       *value);

G_GNUC_INTERNAL void
xml_util_unset_child                    (xmlNode    *parent_node,
                                         const char *name);

G_GNUC_INTERNAL gboolean
xml_util_verify_attribute_is_boolean    (xmlNode    *node,
                                         const char *attribute_name);

G_GNUC_INTERNAL char *
xml_util_get_child_string               (xmlNode    *parent_node,
                                         xmlDoc     *doc,
                                         const char *name);

G_GNUC_INTERNAL gboolean
xml_util_node_deep_equal                (xmlNode *first,
                                         xmlNode *second);

G_GNUC_INTERNAL xmlNode *
xml_util_find_node                      (xmlNode *haystack,
                                         xmlNode *needle);

G_GNUC_INTERNAL xmlNode *
xml_util_copy_node                      (xmlNode *node);

G_GNUC_INTERNAL GHashTable *
xml_util_get_attributes_map             (xmlNode *node);

G_GNUC_INTERNAL xmlNsPtr
xml_util_create_namespace               (xmlNodePtr root,
                                         GUPnPXMLNamespace ns);

G_GNUC_INTERNAL xmlNsPtr
xml_util_lookup_namespace               (xmlDocPtr doc,
                                         GUPnPXMLNamespace ns);

G_GNUC_INTERNAL xmlNsPtr
xml_util_get_ns                         (xmlDocPtr doc,
                                         GUPnPXMLNamespace ns,
                                         xmlNsPtr *ns_out);

G_END_DECLS

#endif /* __XML_UTIL_H__ */
