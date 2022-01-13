/*
 * Copyright (C) 2012 Intel Corporation.
 *
 * Authors: Jens Georg <jensg@openismus.com>
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
 * SECTION:gupnp-media-collection
 * @short_description: Media collection writer
 *
 * #GUPnPMediaCollection is a helper class for writing media collection files.
 **/

#include "gupnp-media-collection.h"
#include "gupnp-didl-lite-writer.h"
#include "gupnp-didl-lite-writer-private.h"
#include "gupnp-didl-lite-parser.h"
#include "gupnp-didl-lite-parser-private.h"

// DIDL_S allowed tags as per DLNA Guidelines 11.1
#define DIDL_S_FILTER "dc:title,dc:creator,upnp:class,upnp:album,res,item," \
                      "container,dlna:lifetime"

G_DEFINE_TYPE (GUPnPMediaCollection,
               gupnp_media_collection,
               G_TYPE_OBJECT);

struct _GUPnPMediaCollectionPrivate {
        GUPnPDIDLLiteWriter *writer;
        GUPnPDIDLLiteObject *container;
        GList               *items;
        gboolean             mutable;
        char                *data;
};

enum {
        PROP_0,
        PROP_AUTHOR,
        PROP_TITLE,
        PROP_MUTABLE,
        PROP_DATA,
};

static void
reparent_children (GUPnPMediaCollection *collection)
{
        GList *it;
        xmlNode *container_node;

        container_node = gupnp_didl_lite_object_get_xml_node
                                        (collection->priv->container);

        /* Reverse iterate the list to get the correct order in XML */
        it = g_list_last (collection->priv->items);
        while (it) {
                xmlNode *node;

                node = gupnp_didl_lite_object_get_xml_node
                                        (GUPNP_DIDL_LITE_OBJECT (it->data));
                xmlUnlinkNode (node);
                xmlAddChild (container_node, node);

                it = it->prev;
        }
}

static void
on_container_available (GUPnPMediaCollection   *self,
                        GUPnPDIDLLiteContainer *container,
                        G_GNUC_UNUSED gpointer  user_data)
{
        /* According to media format spec, there's only one container allowed;
         * We allow any number of containers, but only the last one wins. */
        if (self->priv->container != NULL)
                g_object_unref (self->priv->container);

        self->priv->container = g_object_ref (container);
}

static void
on_item_available (GUPnPMediaCollection   *self,
                   GUPnPDIDLLiteItem      *item,
                   G_GNUC_UNUSED gpointer  user_data)
{
        self->priv->items = g_list_prepend (self->priv->items,
                                            g_object_ref (item));
}

static void
parse_data (GUPnPMediaCollection *collection, const char *data)
{
        GUPnPDIDLLiteParser *parser;
        GError *error = NULL;
        gboolean result;

        parser = gupnp_didl_lite_parser_new ();
        g_signal_connect_swapped (G_OBJECT (parser),
                                  "container-available",
                                  G_CALLBACK (on_container_available),
                                  collection);
        g_signal_connect_swapped (G_OBJECT (parser),
                                  "item-available",
                                  G_CALLBACK (on_item_available),
                                  collection);

        result = gupnp_didl_lite_parser_parse_didl_recursive (parser,
                                                              data,
                                                              TRUE,
                                                              &error);
        if (!result) {
                GUPnPMediaCollectionPrivate *priv = collection->priv;

                g_warning ("Failed to parse DIDL-Lite: %s", error->message);
                g_error_free (error);
                if (priv->container) {
                        g_object_unref (priv->container);
                        priv->container = NULL;
                }
                if (priv->items) {
                        g_list_free_full (priv->items, g_object_unref);
                        priv->items = NULL;
                }
        }
}

static void
gupnp_media_collection_init (GUPnPMediaCollection *collection)
{
        collection->priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (collection,
                                         GUPNP_TYPE_MEDIA_COLLECTION,
                                         GUPnPMediaCollectionPrivate);
        /* Initialize as mutable and decide later on in constructed() if we
         * really are. */
        collection->priv->mutable = TRUE;
}

static void
gupnp_media_collection_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
        GUPnPMediaCollection *collection;

        collection = GUPNP_MEDIA_COLLECTION (object);

        switch (property_id) {
        case PROP_AUTHOR:
                gupnp_media_collection_set_author (collection,
                                                   g_value_get_string (value));
                break;
        case PROP_TITLE:
                gupnp_media_collection_set_title (collection,
                                                  g_value_get_string (value));
                break;
        case PROP_DATA:
                collection->priv->data = g_value_dup_string (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_media_collection_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
        GUPnPMediaCollection *collection;

        collection = GUPNP_MEDIA_COLLECTION (object);

        switch (property_id) {
        case PROP_AUTHOR:
                g_value_set_string
                        (value, gupnp_media_collection_get_author (collection));
                break;
        case PROP_TITLE:
                g_value_set_string
                        (value, gupnp_media_collection_get_title (collection));
                break;
        case PROP_MUTABLE:
                g_value_set_boolean
                        (value, gupnp_media_collection_get_mutable (collection));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_media_collection_constructed (GObject *object)
{
        GUPnPMediaCollection *collection;
        GObjectClass         *object_class;

        collection = GUPNP_MEDIA_COLLECTION (object);

        /* Check if we have some data. If there's data, we assume that the
         * user meant to parse a playlist. We ignore title and author then. */
        if (collection->priv->data != NULL) {
                if (collection->priv->container != NULL) {
                        g_object_unref (collection->priv->container);
                        collection->priv->container = NULL;
                }

                if (collection->priv->writer != NULL) {
                        g_object_unref (collection->priv->writer);
                        collection->priv->writer = NULL;
                }

                parse_data (collection, collection->priv->data);
                collection->priv->mutable = FALSE;
        } else if (collection->priv->writer == NULL) {
                collection->priv->writer =
                                        gupnp_didl_lite_writer_new (NULL);
                collection->priv->mutable = TRUE;
        }

        /* Chain up */
        object_class = G_OBJECT_CLASS (gupnp_media_collection_parent_class);
        if (object_class->constructed != NULL)
                object_class->constructed (object);
}

static void
gupnp_media_collection_dispose (GObject *object)
{
        GUPnPMediaCollection *collection;
        GObjectClass         *object_class;

        collection = GUPNP_MEDIA_COLLECTION (object);

        if (collection->priv->writer) {
                g_object_unref (collection->priv->writer);
                collection->priv->writer = NULL;
        }

        if (collection->priv->items) {
                g_list_free_full (collection->priv->items, g_object_unref);
                collection->priv->items = NULL;
        }

        if (collection->priv->container) {
                g_object_unref (collection->priv->container);
                collection->priv->container = NULL;
        }

        g_free (collection->priv->data);
        collection->priv->data = NULL;

        object_class = G_OBJECT_CLASS (gupnp_media_collection_parent_class);
        object_class->dispose (object);
}

static void
gupnp_media_collection_class_init (GUPnPMediaCollectionClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->set_property = gupnp_media_collection_set_property;
        object_class->get_property = gupnp_media_collection_get_property;
        object_class->constructed = gupnp_media_collection_constructed;
        object_class->dispose = gupnp_media_collection_dispose;

        g_type_class_add_private (klass, sizeof (GUPnPMediaCollectionPrivate));

        /**
         * GUPnPMediaCollection:author:
         *
         * The author of this media collection.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_AUTHOR,
                 g_param_spec_string ("author",
                                      "Author",
                                      "The author of this collection",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS));

        /**
         * GUPnPMediaCollection:title:
         *
         * The title of this media collection.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_AUTHOR,
                 g_param_spec_string ("title",
                                      "Title",
                                      "The title of this collection",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT |
                                      G_PARAM_STATIC_STRINGS));

        /**
         * GUPnPMediaCollection:mutable:
         *
         * Whether this media collation is modifyable or not.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_MUTABLE,
                 g_param_spec_boolean ("mutable",
                                       "Mutable",
                                       "The mutability of this collection",
                                       FALSE,
                                       G_PARAM_READABLE |
                                       G_PARAM_STATIC_STRINGS));

        /**
         * GUPnPMediaCollection:data:
         *
         * Block of data to parse a collection from. If data is set upon
         * construction it will override the other properties and create a
         * unmutable collection parsed from data.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_DATA,
                 g_param_spec_string ("data",
                                      "Data",
                                      "Data to construct the playlist from",
                                      NULL,
                                      G_PARAM_WRITABLE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_STRINGS));
}

/**
 * gupnp_media_collection_new:
 *
 * Create a new writable media collection.
 *
 * Returns: (transfer full): A new #GUPnPMediaCollection.
 **/
GUPnPMediaCollection *
gupnp_media_collection_new (void)
{
        return g_object_new (GUPNP_TYPE_MEDIA_COLLECTION, NULL);
}

/**
 * gupnp_media_collection_new_from_string:
 * @data: XML string.
 *
 * Parse a new #GUPnPMediaCollection from a block of XML data.
 *
 * Returns: (transfer full): A new #GUPnPMediaCollection.
 **/
GUPnPMediaCollection *
gupnp_media_collection_new_from_string (const char *data)
{
        return g_object_new (GUPNP_TYPE_MEDIA_COLLECTION,
                             "data", data,
                             NULL);
}

/**
 * gupnp_media_collection_set_title:
 * @collection: #GUPnPMediaCollection
 * @title: New Title of this collection;
 *
 * Set the title of a #GUPnPMediaCollection.
 **/
void
gupnp_media_collection_set_title  (GUPnPMediaCollection *collection,
                                   const char           *title)
{
        GUPnPDIDLLiteContainer *container;

        g_return_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection));
        g_return_if_fail (collection->priv->mutable);

        if (title == NULL)
                return;

        if (collection->priv->container != NULL) {
                gupnp_didl_lite_object_set_title (collection->priv->container,
                                                  title);

                return;
        }

        if (collection->priv->writer == NULL)
                collection->priv->writer = gupnp_didl_lite_writer_new (NULL);

        container = gupnp_didl_lite_writer_add_container
                                                (collection->priv->writer);
        collection->priv->container = GUPNP_DIDL_LITE_OBJECT (container);

        reparent_children (collection);

        gupnp_didl_lite_object_set_title (collection->priv->container,
                                          title);
}

/**
 * gupnp_media_collection_get_title:
 * @collection: #GUPnPMediaCollection
 *
 * Returns: The title of this media collection or %NULL if not set.
 **/
const char *
gupnp_media_collection_get_title  (GUPnPMediaCollection *collection)
{
        g_return_val_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection), NULL);

        if (collection->priv->container == NULL)
                return NULL;

        return gupnp_didl_lite_object_get_title (collection->priv->container);
}

/**
 * gupnp_media_collection_set_author:
 * @collection: #GUPnPMediaCollection
 * @author: New author of this media collection.
 *
 * Set the author of the media collection
 **/
void
gupnp_media_collection_set_author (GUPnPMediaCollection *collection,
                                   const char           *author)
{
        GUPnPDIDLLiteContainer *container;

        g_return_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection));
        g_return_if_fail (collection->priv->mutable);

        if (author == NULL)
                return;

        if (collection->priv->container != NULL) {
                gupnp_didl_lite_object_set_creator (collection->priv->container,
                                                    author);

                return;
        }

        if (collection->priv->writer == NULL)
                collection->priv->writer = gupnp_didl_lite_writer_new (NULL);

        container = gupnp_didl_lite_writer_add_container
                                                (collection->priv->writer);
        collection->priv->container = GUPNP_DIDL_LITE_OBJECT (container);

        reparent_children (collection);

        gupnp_didl_lite_object_set_creator (collection->priv->container,
                                            author);
}

/**
 * gupnp_media_collection_get_author:
 * @collection: #GUPnPMediaCollection
 *
 * Returns: The author of this media collection or %NULL if not set.
 **/
const char *
gupnp_media_collection_get_author (GUPnPMediaCollection *collection)
{
        g_return_val_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection), NULL);

        if (collection->priv->container == NULL)
                return NULL;

        return gupnp_didl_lite_object_get_creator (collection->priv->container);
}

/**
 * gupnp_media_collection_add_item:
 * @collection: #GUPnPMediaCollection
 *
 * Return value: (transfer full): A new #GUPnPDIDLLiteItem object. Unref after
 * use.
 **/
GUPnPDIDLLiteItem *
gupnp_media_collection_add_item (GUPnPMediaCollection *collection)
{
        GUPnPDIDLLiteItem *item = NULL;

        g_return_val_if_fail (collection != NULL, NULL);
        g_return_val_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection), NULL);
        g_return_val_if_fail (collection->priv->mutable, NULL);

        if (collection->priv->container != NULL)
                item = gupnp_didl_lite_writer_add_container_child_item
                                        (collection->priv->writer,
                                         GUPNP_DIDL_LITE_CONTAINER
                                                (collection->priv->container));
        else
                item = gupnp_didl_lite_writer_add_item
                                        (collection->priv->writer);

        /* Keep a reference of the object in case we need to do reparenting */
        collection->priv->items = g_list_prepend (collection->priv->items,
                                                  g_object_ref (item));

        /* Mandatory in DLNA for object. Not specified if mandatory for
         * DIDL_S, but to avoid problems with clients reusing their normal
         * DIDL-Lite parser, we set it here if the application doesn't.
         */
        gupnp_didl_lite_object_set_restricted (GUPNP_DIDL_LITE_OBJECT (item),
                                               TRUE);

        return item;
}

/**
 * gupnp_media_collection_get_string:
 * @collection: #GUPnPMediaCollection
 *
 * Return value: (transfer full): XML string representing this media
 * collection. g_free() after use. If the colleciton is not mutable, returns a
 * copy of the original string.
 **/
char *
gupnp_media_collection_get_string (GUPnPMediaCollection *collection)
{
        g_return_val_if_fail (collection != NULL, NULL);
        g_return_val_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection), NULL);

        if (collection->priv->data)
                return g_strdup (collection->priv->data);

        gupnp_didl_lite_writer_filter_tags (collection->priv->writer,
                                            DIDL_S_FILTER);

        return gupnp_didl_lite_writer_get_string (collection->priv->writer);
}

/**
 * gupnp_media_collection_get_items:
 * @collection: #GUPnPMediaCollection
 *
 * Return value: (transfer full)(element-type GUPnPDIDLLiteItem): A #GList
 * containing the elemens of this collection, in proper order. Unref all items
 * and free the list after use.
 **/
GList *
gupnp_media_collection_get_items (GUPnPMediaCollection *collection)
{
        GList *tmp = NULL, *iter;

        g_return_val_if_fail (collection != NULL, NULL);
        g_return_val_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection), NULL);

        for (iter = collection->priv->items; iter != NULL; iter = iter->next) {
                tmp = g_list_prepend (tmp, g_object_ref (iter->data));
        }

        return tmp;
}

/**
 * gupnp_media_collection_get_mutable:
 * @collection: #GUPnPMediaCollection
 *
 * Return value: #TRUE if the collections is modifiable, #FALSE otherwise.
 **/
gboolean
gupnp_media_collection_get_mutable (GUPnPMediaCollection *collection)
{
        g_return_val_if_fail (collection != NULL, FALSE);
        g_return_val_if_fail (GUPNP_IS_MEDIA_COLLECTION (collection), FALSE);

        return collection->priv->mutable;
}
