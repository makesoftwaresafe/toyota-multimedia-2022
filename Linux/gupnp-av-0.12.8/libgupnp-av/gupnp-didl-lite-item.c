/*
 * Copyright (C) 2009 Nokia Corporation.
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshan.ali@nokia.com>
 *                                <zeeshanak@gnome.org>
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
 * SECTION:gupnp-didl-lite-item
 * @short_description: DIDL-Lite Item
 *
 * #GUPnPDIDLLiteItem respresents a DIDL-Lite item element.
 */

#include <string.h>

#include "gupnp-didl-lite-item.h"
#include "xml-util.h"
#include "time-utils.h"

G_DEFINE_TYPE (GUPnPDIDLLiteItem,
               gupnp_didl_lite_item,
               GUPNP_TYPE_DIDL_LITE_OBJECT);

enum {
        PROP_0,
        PROP_REF_ID,
        PROP_LIFETIME
};

static void
gupnp_didl_lite_item_init (G_GNUC_UNUSED GUPnPDIDLLiteItem *item)
{
        /* Nothing to initialize, yay! */
}

static void
gupnp_didl_lite_item_get_property (GObject    *object,
                                   guint       property_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
        GUPnPDIDLLiteItem *item;

        item = GUPNP_DIDL_LITE_ITEM (object);

        switch (property_id) {
        case PROP_REF_ID:
                g_value_set_string
                        (value,
                         gupnp_didl_lite_item_get_ref_id (item));
                break;
        case PROP_LIFETIME:
                g_value_set_long
                        (value,
                         gupnp_didl_lite_item_get_lifetime (item));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_didl_lite_item_set_property (GObject      *object,
                                   guint         property_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)

{
        GUPnPDIDLLiteItem *item;

        item = GUPNP_DIDL_LITE_ITEM (object);

        switch (property_id) {
        case PROP_REF_ID:
                gupnp_didl_lite_item_set_ref_id (item,
                                                 g_value_get_string (value));
                break;
        case PROP_LIFETIME:
                gupnp_didl_lite_item_set_lifetime (item,
                                                   g_value_get_long (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_didl_lite_item_class_init (GUPnPDIDLLiteItemClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gupnp_didl_lite_item_get_property;
        object_class->set_property = gupnp_didl_lite_item_set_property;

        /**
         * GUPnPDIDLLiteItem:ref-id:
         *
         * The ref ID of this item.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_REF_ID,
                 g_param_spec_string ("ref-id",
                                      "RefID",
                                      "The ref ID of this item.",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPDIDLLiteItem:lifetime:
         *
         * The lifetime in seconds of this DIDLLite item in a media collection.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_LIFETIME,
                 g_param_spec_long ("lifetime",
                                    "Lifetime",
                                    "The lifetime (in seconds) of this"
                                    " item.",
                                    -1,
                                    G_MAXLONG,
                                    -1,
                                    G_PARAM_READWRITE |
                                    G_PARAM_STATIC_STRINGS));
}

/**
 * gupnp_didl_lite_item_get_ref_id:
 * @item: #GUPnPDIDLLiteItem
 *
 * Get the ref ID of the @item.
 *
 * Return value: The ref ID of the @item, or %NULL.
 **/
const char *
gupnp_didl_lite_item_get_ref_id (GUPnPDIDLLiteItem *item)
{
        xmlNode *xml_node;

        g_return_val_if_fail (item != NULL, 0);
        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_ITEM (item), NULL);

        xml_node = gupnp_didl_lite_object_get_xml_node
                                        (GUPNP_DIDL_LITE_OBJECT (item));

        return xml_util_get_attribute_content (xml_node, "refID");
}

/**
 * gupnp_didl_lite_item_set_ref_id:
 * @item: #GUPnPDIDLLiteItem
 * @ref_id: The ref ID
 *
 * Set the ref ID of the @item.
 **/
void
gupnp_didl_lite_item_set_ref_id (GUPnPDIDLLiteItem *item,
                                 const char        *ref_id)
{
        xmlNode *xml_node;

        g_return_if_fail (item != NULL);
        g_return_if_fail (GUPNP_IS_DIDL_LITE_ITEM (item));

        xml_node = gupnp_didl_lite_object_get_xml_node
                                (GUPNP_DIDL_LITE_OBJECT (item));

        xmlSetProp (xml_node,
                    (unsigned char *) "refID",
                    (unsigned char *) ref_id);

        g_object_notify (G_OBJECT (item), "ref-id");
}

/**
 * gupnp_didl_lite_item_set_lifetime:
 * @item: #GUPnPDIDLLiteItem
 * @lifetime: The lifetime (in seconds) of this item in a media collection.
 **/
void
gupnp_didl_lite_item_set_lifetime (GUPnPDIDLLiteItem *item,
                                   glong              lifetime)
{
        xmlNode *node = NULL;
        xmlNs *ns = NULL;
        GUPnPXMLDoc *doc = NULL;
        GUPnPDIDLLiteObject *object = NULL;

        g_return_if_fail (GUPNP_IS_DIDL_LITE_ITEM (item));

        object = GUPNP_DIDL_LITE_OBJECT (item);
        node = gupnp_didl_lite_object_get_xml_node (object);
        ns = gupnp_didl_lite_object_get_dlna_namespace (object);
        g_object_get (G_OBJECT (object), "xml-doc", &doc, NULL);

        if (lifetime < 0)
                xml_util_unset_child (node, "lifetime");
        else {
                char *str;

                str = seconds_to_time (lifetime);
                xml_util_set_child (node,
                                    GUPNP_XML_NAMESPACE_DLNA,
                                    &ns,
                                    doc->doc,
                                    "lifetime",
                                    str);
                g_free (str);
        }

        g_object_notify (G_OBJECT (object), "lifetime");
}

/**
 * gupnp_didl_lite_item_get_lifetime:
 * @item: #GUPnPDIDLLiteItem
 *
 * Returns: -1 if unset or the lifetime (in seconds) of the current item.
 **/
glong
gupnp_didl_lite_item_get_lifetime (GUPnPDIDLLiteItem *item)
{
        xmlNode *node = NULL;
        const char *lifetime_str;
        long lifetime;
        GUPnPDIDLLiteObject *object = NULL;

        g_return_val_if_fail (GUPNP_IS_DIDL_LITE_ITEM (item), -1);

        object = GUPNP_DIDL_LITE_OBJECT (item);
        node = gupnp_didl_lite_object_get_xml_node (object);

        lifetime_str = xml_util_get_child_element_content (node, "lifetime");
        lifetime = seconds_from_time (lifetime_str);

        return lifetime;
}
