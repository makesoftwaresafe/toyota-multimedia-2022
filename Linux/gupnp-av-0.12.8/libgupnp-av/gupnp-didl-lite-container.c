/*
 * Copyright (C) 2009 Nokia Corporation.
 * Copyright (C) 2012 Intel Corporation
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *                                <zeeshan.ali@nokia.com>
 *          Krzesimir Nowak <krnowak@openismus.com>
 *          Christophe Guiraud <christophe.guiraud@intel.com>
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
 * SECTION:gupnp-didl-lite-container
 * @short_description: DIDL-Lite Container
 *
 * #GUPnPDIDLLiteContainer respresents a DIDL-Lite container element.
 */

#include <string.h>

#include "gupnp-didl-lite-container.h"
#include "gupnp-didl-lite-object-private.h"
#include "gupnp-didl-lite-createclass.h"
#include "gupnp-didl-lite-createclass-private.h"
#include "xml-util.h"


G_DEFINE_TYPE (GUPnPDIDLLiteContainer,
               gupnp_didl_lite_container,
               GUPNP_TYPE_DIDL_LITE_OBJECT);

enum {
        PROP_0,
        PROP_SEARCHABLE,
        PROP_CHILD_COUNT,
        PROP_STORAGE_USED,
        PROP_CONTAINER_UPDATE_ID,
        PROP_TOTAL_DELETED_CHILD_COUNT
};

static void
gupnp_didl_lite_container_init (G_GNUC_UNUSED GUPnPDIDLLiteContainer *container)
{
        /* Nothing to initialize, yay! */
}

static void
gupnp_didl_lite_container_get_property (GObject    *object,
                                        guint       property_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
        GUPnPDIDLLiteContainer *container;

        container = GUPNP_DIDL_LITE_CONTAINER (object);

        switch (property_id) {
        case PROP_SEARCHABLE:
                g_value_set_boolean
                        (value,
                         gupnp_didl_lite_container_get_searchable (container));
                break;
        case PROP_CHILD_COUNT:
                g_value_set_int
                        (value,
                         gupnp_didl_lite_container_get_child_count (container));
                break;
        case PROP_STORAGE_USED:
                g_value_set_long
                        (value,
                         gupnp_didl_lite_container_get_storage_used (container));
                break;
        case PROP_CONTAINER_UPDATE_ID:
                g_value_set_uint
                              (value,
                               gupnp_didl_lite_container_get_container_update_id
                                        (container));
                break;
        case PROP_TOTAL_DELETED_CHILD_COUNT:
                g_value_set_uint
                        (value,
                         gupnp_didl_lite_container_get_total_deleted_child_count
                                        (container));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_didl_lite_container_set_property (GObject      *object,
                                        guint         property_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)

{
        GUPnPDIDLLiteContainer *container;

        container = GUPNP_DIDL_LITE_CONTAINER (object);

        switch (property_id) {
        case PROP_SEARCHABLE:
                gupnp_didl_lite_container_set_searchable
                                        (container,
                                         g_value_get_boolean (value));
                break;
        case PROP_CHILD_COUNT:
                gupnp_didl_lite_container_set_child_count
                                        (container,
                                         g_value_get_int (value));
                break;
        case PROP_STORAGE_USED:
                gupnp_didl_lite_container_set_storage_used
                                        (container,
                                         g_value_get_int64 (value));
                break;
        case PROP_CONTAINER_UPDATE_ID:
                gupnp_didl_lite_container_set_container_update_id
                                        (container,
                                         g_value_get_uint (value));
                break;
        case PROP_TOTAL_DELETED_CHILD_COUNT:
                gupnp_didl_lite_container_set_total_deleted_child_count
                                        (container,
                                         g_value_get_uint (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_didl_lite_container_class_init (GUPnPDIDLLiteContainerClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gupnp_didl_lite_container_get_property;
        object_class->set_property = gupnp_didl_lite_container_set_property;

        /**
         * GUPnPDIDLLiteContainer:searchable:
         *
         * Whether this container is searchable.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_SEARCHABLE,
                 g_param_spec_boolean ("searchable",
                                       "Searchable",
                                       "Whether this container is searchable.",
                                       FALSE,
                                       G_PARAM_READWRITE |
                                       G_PARAM_STATIC_NAME |
                                       G_PARAM_STATIC_NICK |
                                       G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteContainer:child-count:
         *
         * The child count of this container.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_CHILD_COUNT,
                 g_param_spec_int ("child-count",
                                   "ChildCount",
                                   "The child count of this container.",
                                   0,
                                   G_MAXINT,
                                   0,
                                   G_PARAM_READWRITE |
                                   G_PARAM_STATIC_NAME |
                                   G_PARAM_STATIC_NICK |
                                   G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteContainer:storage-used:
         *
         * The number of bytes used by all child items of this container.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_STORAGE_USED,
                 g_param_spec_int64 ("storage-used",
                                     "Storage Used",
                                     "The Number of bytes used by all child "
                                     "items of this container.",
                                     -1,
                                     G_MAXINT64,
                                     -1,
                                     G_PARAM_READWRITE |
                                     G_PARAM_STATIC_NAME |
                                     G_PARAM_STATIC_NICK |
                                     G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteContainer:container-update-id:
         *
         * Update ID of this container.
         **/
        g_object_class_install_property
                             (object_class,
                              PROP_CONTAINER_UPDATE_ID,
                              g_param_spec_uint ("container-update-id",
                                                 "ContainerUpdateID",
                                                 "Update ID of this container.",
                                                 0,
                                                 G_MAXUINT,
                                                 0,
                                                 G_PARAM_READWRITE |
                                                 G_PARAM_STATIC_NAME |
                                                 G_PARAM_STATIC_NICK |
                                                 G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteContainer:total-deleted-child-count:
         *
         * Total deleted child count of this container.
         **/
        g_object_class_install_property
             (object_class,
              PROP_TOTAL_DELETED_CHILD_COUNT,
              g_param_spec_uint ("total-deleted-child-count",
                                 "TotalDeletedChildCOunt",
                                 "Total deleted child count of this container.",
                                 0,
                                 G_MAXUINT,
                                 0,
                                 G_PARAM_READWRITE |
                                 G_PARAM_STATIC_NAME |
                                 G_PARAM_STATIC_NICK |
                                 G_PARAM_STATIC_BLURB));
}

/**
 * gupnp_didl_lite_container_get_searchable:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Checks whether @container is searchable.
 *
 * Return value: #TRUE if @container is searchable.
 **/
gboolean
gupnp_didl_lite_container_get_searchable (GUPnPDIDLLiteContainer *container)
{
        xmlNode *xml_node;

        g_return_val_if_fail (container != NULL, FALSE);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), FALSE);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        return xml_util_get_boolean_attribute (xml_node, "searchable");
}

/**
 * gupnp_didl_lite_container_get_child_count:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Get the child count of the @container.  If the child count is unknown, -1 is
 * returned.
 *
 * Return value: The child count of the @container, or -1 if it is unknown.
 **/
gint
gupnp_didl_lite_container_get_child_count (GUPnPDIDLLiteContainer *container)
{
        xmlNode *xml_node;

        g_return_val_if_fail (container != NULL, 0);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), 0);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        return xml_util_get_int_attribute (xml_node, "childCount", -1);
}

/**
 * gupnp_didl_lite_container_get_container_update_id:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Get the container update ID of the @container.
 *
 * Return value: The container update ID of the @container.
 **/
guint
gupnp_didl_lite_container_get_container_update_id
                                        (GUPnPDIDLLiteContainer *container)
{
        xmlNode *xml_node;

        g_return_val_if_fail (container != NULL, 0);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), 0);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        return xml_util_get_uint_child_element (xml_node,
                                                "containerUpdateID",
                                                0);
}

/**
 * gupnp_didl_lite_container_container_update_id_is_set:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Get whether the container update ID of the @container is set.
 *
 * Return value: %TRUE if update ID is set, otherwise %FALSE
 **/
gboolean
gupnp_didl_lite_container_container_update_id_is_set
                                        (GUPnPDIDLLiteContainer *container)
{
        const char *content;
        xmlNode *xml_node;

        g_return_val_if_fail (container != NULL, FALSE);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), FALSE);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                        (GUPNP_DIDL_LITE_OBJECT (container));
        content = xml_util_get_child_element_content (xml_node,
                                                      "containerUpdateID");
        return content != NULL;
}

/**
 * gupnp_didl_lite_container_get_total_deleted_child_count:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Get the total deleted child count of the @container.
 *
 * Return value: The total deleted child count of the @container.
 **/
guint
gupnp_didl_lite_container_get_total_deleted_child_count
                                        (GUPnPDIDLLiteContainer *container)
{
        xmlNode *xml_node;

        g_return_val_if_fail (container != NULL, 0);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), 0);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        return xml_util_get_uint_child_element (xml_node,
                                                "totalDeletedChildCount",
                                                -1);
}

/**
 * gupnp_didl_lite_container_total_deleted_child_count_is_set:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Get whether the total deleted child conut of the @container is set.
 *
 * Return value: %TRUE if property is set, otherwise %FALSE
 **/
gboolean
gupnp_didl_lite_container_total_deleted_child_count_is_set
                                        (GUPnPDIDLLiteContainer *container)
{
        const char *content;
        xmlNode *xml_node;

        g_return_val_if_fail (container != NULL, FALSE);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), FALSE);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                        (GUPNP_DIDL_LITE_OBJECT (container));
        content = xml_util_get_child_element_content (xml_node,
                                                      "totalDeletedChildCount");
        return content != NULL;
}
/**
 * gupnp_didl_lite_container_get_create_classes:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Gets the list of create classes of the @container.
 *
 * Returns: (element-type utf8) (transfer full): The list of create classes
 * belonging to @container, or %NULL.
 * #g_list_free the returned list after usage and #g_free each string in it.
 **/
GList *
gupnp_didl_lite_container_get_create_classes (GUPnPDIDLLiteContainer *container)
{
        GList *classes = NULL;
        GList *ret = NULL;
        GList *l;

        g_return_val_if_fail (container != NULL, NULL);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), NULL);

        classes = gupnp_didl_lite_object_get_properties (
                        GUPNP_DIDL_LITE_OBJECT (container),
                        "createClass");

        for (l = classes; l; l = l->next) {
                char *create_class;
                xmlNode *node;

                node = (xmlNode *) l->data;

                if (node->children != NULL) {
                    create_class = g_strdup ((const char *) node->children->content);

                    ret = g_list_append (ret, create_class);
                }
        }

        g_list_free (classes);

        return ret;
}

/**
 * gupnp_didl_lite_container_get_create_classes_full:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Gets the list of create classes of the @container.
 *
 * Returns: (element-type GUPnPDIDLLiteCreateClass*) (transfer full): The list
 * of create classes belonging to @container, or %NULL.
 * #g_list_free the returned list after usage and unref each object in it.
 **/
GList *
gupnp_didl_lite_container_get_create_classes_full (
                                            GUPnPDIDLLiteContainer *container)
{
        GList *cc_list = NULL;
        GList *ret = NULL;
        GList *l;

        g_return_val_if_fail (container != NULL, NULL);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), NULL);

        cc_list = gupnp_didl_lite_object_get_properties (
                                           GUPNP_DIDL_LITE_OBJECT (container),
                                           "createClass");

        for (l = cc_list; l; l = l->next) {
                GUPnPDIDLLiteCreateClass *cc;
                xmlNode *cc_node;
                GUPnPXMLDoc *cc_doc;

                cc_node = (xmlNode *) l->data;
                if (!cc_node->children)
                        continue;

                cc_doc = gupnp_didl_lite_object_get_gupnp_xml_doc (
                                           GUPNP_DIDL_LITE_OBJECT (container));

                cc = gupnp_didl_lite_create_class_new_from_xml (cc_node, cc_doc);

                ret = g_list_append (ret, cc);
        }

        g_list_free (cc_list);

        return ret;
}

/**
 * gupnp_didl_lite_container_get_search_classes:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Gets the list of search classes of the @container.
 *
 * Return value: (element-type utf8) (transfer full): The list of search classes
 * belonging to @container, or %NULL. #g_list_free the returned list after usage
 * and #g_free each string in it.
 **/
GList *
gupnp_didl_lite_container_get_search_classes (GUPnPDIDLLiteContainer *container)
{
        GList *classes = NULL;
        GList *ret = NULL;
        GList *l;

        g_return_val_if_fail (container != NULL, NULL);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), NULL);

        classes = gupnp_didl_lite_object_get_properties (
                        GUPNP_DIDL_LITE_OBJECT (container),
                        "searchClass");

        for (l = classes; l; l = l->next) {
                char *search_class;
                xmlNode *node;

                node = (xmlNode *) l->data;
                if (node->children != NULL) {
                    search_class = g_strdup ((const char *) node->children->content);

                    ret = g_list_append (ret, search_class);
                }
        }

        g_list_free (classes);

        return ret;
}

/**
 * gupnp_didl_lite_container_get_storage_used:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Get the number of bytes used by all child items of the @container.
 * If storage used is unknown, -1 is returned.
 *
 * Return value: The number of bytes used by all children of the @container,
 * or -1 if it is unknown.
 **/
gint64
gupnp_didl_lite_container_get_storage_used (GUPnPDIDLLiteContainer *container)
{
        GList *storage = NULL;
        xmlNode *xml_node;
        const char *str;

        g_return_val_if_fail (container != NULL, 0);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container), 0);

        storage = gupnp_didl_lite_object_get_properties (
                                        GUPNP_DIDL_LITE_OBJECT (container),
                                        "storageUsed");
        if (storage == NULL)
                return -1;

        /* only return value from first node */
        xml_node = (xmlNode *) storage->data;

        g_list_free (storage);

        str = (const char *) xml_node->content;

        return g_ascii_strtoll (str, NULL, 10);
}
/**
 * gupnp_didl_lite_container_set_searchable:
 * @container: #GUPnPDIDLLiteContainer
 * @searchable: The searchibility
 *
 * (Un)set the searchibility of @container.
 **/
void
gupnp_didl_lite_container_set_searchable (GUPnPDIDLLiteContainer *container,
                                          gboolean                searchable)
{
        xmlNode *xml_node;
        const char *str;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        if (searchable)
                str = "1";
        else
                str = "0";
        xmlSetProp (xml_node,
                    (unsigned char *) "searchable",
                    (unsigned char *) str);

        g_object_notify (G_OBJECT (container), "searchable");
}

/**
 * gupnp_didl_lite_container_set_child_count:
 * @container: #GUPnPDIDLLiteContainer
 * @child_count: The child count
 *
 * Set the child count of the @container.
 **/
void
gupnp_didl_lite_container_set_child_count (GUPnPDIDLLiteContainer *container,
                                           gint                    child_count)
{
        xmlNode *xml_node;
        char *str;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        str = g_strdup_printf ("%d", child_count);
        xmlSetProp (xml_node,
                    (unsigned char *) "childCount",
                    (unsigned char *) str);
        g_free (str);

        g_object_notify (G_OBJECT (container), "child-count");
}

/**
 * gupnp_didl_lite_container_set_container_update_id:
 * @container: #GUPnPDIDLLiteContainer
 * @update_id: The container update ID
 *
 * Set the container update ID of the @container.
 **/
void
gupnp_didl_lite_container_set_container_update_id
                                        (GUPnPDIDLLiteContainer *container,
                                         guint                   update_id)
{
        xmlNode *xml_node;
        xmlNsPtr upnp_ns;
        GUPnPXMLDoc *xml_doc;
        char *str;
        GUPnPDIDLLiteObject *self_as_object;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        self_as_object = GUPNP_DIDL_LITE_OBJECT (container);
        xml_node = gupnp_didl_lite_object_get_xml_node (self_as_object);
        xml_doc = gupnp_didl_lite_object_get_gupnp_xml_doc (self_as_object);
        upnp_ns = gupnp_didl_lite_object_get_upnp_namespace (self_as_object);

        str = g_strdup_printf ("%u", update_id);
        xml_util_set_child (xml_node,
                            GUPNP_XML_NAMESPACE_UPNP,
                            &upnp_ns,
                            xml_doc->doc,
                            "containerUpdateID",
                            str);
        g_free (str);

        g_object_notify (G_OBJECT (container), "container-update-id");
}

/**
 * gupnp_didl_lite_container_unset_container_update_id:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Unset the container update ID property of the @container.
 **/
void
gupnp_didl_lite_container_unset_container_update_id
                                        (GUPnPDIDLLiteContainer *container)
{
        xmlNode *xml_node;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                        (GUPNP_DIDL_LITE_OBJECT (container));
        xml_util_unset_child (xml_node, "containerUpdateID");

        g_object_notify (G_OBJECT (container), "container-update-id");
}

/**
 * gupnp_didl_lite_container_set_total_deleted_child_count:
 * @container: #GUPnPDIDLLiteContainer
 * @count: The container update ID
 *
 * Set the container update ID of the @container.
 **/
void
gupnp_didl_lite_container_set_total_deleted_child_count
                                        (GUPnPDIDLLiteContainer *container,
                                         guint                   count)
{
        xmlNode *xml_node;
        xmlNsPtr upnp_ns;
        GUPnPXMLDoc *xml_doc;
        char *str;
        GUPnPDIDLLiteObject *self_as_object;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        self_as_object = GUPNP_DIDL_LITE_OBJECT (container);
        xml_node = gupnp_didl_lite_object_get_xml_node (self_as_object);
        xml_doc = gupnp_didl_lite_object_get_gupnp_xml_doc (self_as_object);
        upnp_ns = gupnp_didl_lite_object_get_upnp_namespace (self_as_object);

        str = g_strdup_printf ("%u", count);
        xml_util_set_child (xml_node,
                            GUPNP_XML_NAMESPACE_UPNP,
                            &upnp_ns,
                            xml_doc->doc,
                            "totalDeletedChildCount",
                            str);
        g_free (str);

        g_object_notify (G_OBJECT (container), "total-deleted-child-count");
}

/**
 * gupnp_didl_lite_container_unset_total_deleted_child_count:
 * @container: #GUPnPDIDLLiteContainer
 *
 * Unset the total deleted child count property of the @container.
 **/
void
gupnp_didl_lite_container_unset_total_deleted_child_count
                                        (GUPnPDIDLLiteContainer *container)
{
        xmlNode *xml_node;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                        (GUPNP_DIDL_LITE_OBJECT (container));
        xml_util_unset_child (xml_node, "totalDeletedChildCount");

        g_object_notify (G_OBJECT (container), "total-deleted-child-count");
}

/**
 * gupnp_didl_lite_container_add_create_class:
 * @container: #GUPnPDIDLLiteContainer
 * @create_class: The createClass to add.
 *
 * Add a new create class to the @container. includeDerived defaults to "0".
 **/
void
gupnp_didl_lite_container_add_create_class (
                GUPnPDIDLLiteContainer *container,
                const char             *create_class)
{
        gupnp_didl_lite_container_add_create_class_full (container,
                                                         create_class,
                                                         FALSE);
}

/**
 * gupnp_didl_lite_container_add_create_class_full:
 * @container: #GUPnPDIDLLiteContainer
 * @create_class: The createClass to add.
 * @include_derived: Whether object with dervied classes may be created in
 * this container or not.
 *
 * Add a new create class to the @container.
 **/
void
gupnp_didl_lite_container_add_create_class_full (
                GUPnPDIDLLiteContainer *container,
                const char             *create_class,
                gboolean                include_derived)
{
        xmlNode *container_node, *new_node;
        xmlNs *namespace;
        const char *str;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        container_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));
        namespace = gupnp_didl_lite_object_get_upnp_namespace
                                (GUPNP_DIDL_LITE_OBJECT (container));

        new_node = xmlNewChild (container_node,
                                namespace,
                                (unsigned char *) "createClass",
                                (unsigned char *) create_class);
        if (include_derived)
                str = "1";
        else
                str = "0";

        xmlSetProp (new_node,
                    (unsigned char *) "includeDerived",
                    (unsigned char *) str);
}

/**
 * gupnp_didl_lite_container_add_search_class:
 * @container: #GUPnPDIDLLiteContainer
 * @search_class: The searchClass to add.
 *
 * Add a new search class to the @container.
 **/
void
gupnp_didl_lite_container_add_search_class (
                GUPnPDIDLLiteContainer *container,
                const char             *search_class)
{
        gupnp_didl_lite_container_add_search_class_full (container,
                                                         search_class,
                                                         TRUE);
}

/**
 * gupnp_didl_lite_container_add_search_class_full:
 * @container: #GUPnPDIDLLiteContainer
 * @search_class: The searchClass to add.
 * @include_derived: includeDerived attribute of the DIDL
 *
 * Add a new search class to the @container.
 **/
void
gupnp_didl_lite_container_add_search_class_full (
                GUPnPDIDLLiteContainer *container,
                const char             *search_class,
                gboolean                include_derived)
{
        xmlNode *xml_node;
        xmlNode *new_xml_node;
        xmlNs *namespace;
        const char *str;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));
        namespace = gupnp_didl_lite_object_get_upnp_namespace
                                (GUPNP_DIDL_LITE_OBJECT (container));

        new_xml_node = xmlNewChild (xml_node,
                                    namespace,
                                    (unsigned char *) "searchClass",
                                    (unsigned char *) search_class);

        if (include_derived)
                str = "1";
        else
                str = "0";

        xmlSetProp (new_xml_node,
                    (unsigned char*) "includeDerived",
                    (unsigned char*) str);
}

/**
 * gupnp_didl_lite_container_set_storage_used:
 * @container: #GUPnPDIDLLiteContainer
 * @storage_used: The number of bytes used by all child items of the
 *                @container or -1 if unknown.
 *
 * Set the number of bytes used by all child items of the @container.
 **/
void
gupnp_didl_lite_container_set_storage_used (
                GUPnPDIDLLiteContainer *container,
                gint64                  storage_used)
{
        GList *storage = NULL;
        xmlNode *xml_node;
        xmlNs *namespace;
        char *str;

        g_return_if_fail (container != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_CONTAINER (container));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (container));

        namespace = gupnp_didl_lite_object_get_upnp_namespace
                                (GUPNP_DIDL_LITE_OBJECT (container));

        str = g_strdup_printf ("%"G_GINT64_FORMAT, storage_used);

        storage = gupnp_didl_lite_object_get_properties (
                                        GUPNP_DIDL_LITE_OBJECT (container),
                                        "storageUsed");
        if (storage == NULL)
                xmlNewChild (xml_node,
                             namespace,
                             (unsigned char *) "storageUsed",
                             (unsigned char *) str);
        else
                xmlNodeSetContent ((xmlNode *) storage->data,
                                   (unsigned char *) str);

        g_free (str);

        g_object_notify (G_OBJECT (container), "storage-used");
}
