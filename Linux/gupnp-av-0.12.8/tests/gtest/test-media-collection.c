/*
 * Copyright (C) 2012 Intel Corporation.
 * Copyright (C) 2013 Jens Georg <mail@jensge.org>
 *
 * Author: Jens Georg <jensg@openismus.com>
 *         Jens Georg <mail@jensge.org>
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

#include <string.h>

#include <libgupnp-av/gupnp-media-collection.h>

/* Flat DIDL_S playlist */
#define TEST_PARSE_COLLECTION_1 \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
"<DIDL-Lite" \
"    xmlns:dc=\"http://purl.org/dc/elements/1.1/\"" \
"    xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\"" \
"    xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\"" \
"    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" \
"    xsi:schemaLocation=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" \
"        http://www.upnp.org/schemas/av/didl-lite-v2-20060531.xsd" \
"      urn:schemas-upnp-org:metadata-1-0/upnp/" \
"        http://www.upnp.org/schemas/av/upnp-v2-20060531.xsd\">" \
"    <item id=\"\" parentID=\"0\" restricted=\"0\">" \
"        <dc:title>Song1</dc:title>" \
"        <upnp:class>object.item.audioItem</upnp:class>" \
"        <res protocolInfo=\"*:*:*:*\" />" \
"        <upnp:artist></upnp:artist>" \
"    </item>" \
"    <item id=\"\" parentID=\"0\" restricted=\"0\">" \
"        <dc:title>Song2</dc:title>" \
"        <upnp:class>object.item.audioItem</upnp:class>" \
"        <res protocolInfo=\"*:*:*:*\" />" \
"    </item>" \
"    <item id=\"\" parentID=\"0\" restricted=\"0\">" \
"        <dc:title>Song3</dc:title>" \
"        <upnp:class>object.item.audioItem</upnp:class>" \
"        <res protocolInfo=\"*:*:*:*\" />" \
"    </item>" \
"</DIDL-Lite>"

/* Nested DIDL_S playlist */
#define TEST_PARSE_COLLECTION_2 \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>" \
"<DIDL-Lite" \
"    xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\"" \
"    xmlns:dc=\"http://purl.org/dc/elements/1.1/\"" \
"    xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\"" \
"    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" \
"    xsi:schemaLocation=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/" \
"        http://www.upnp.org/schemas/av/didl-lite-v2-20060531.xsd" \
"      urn:schemas-upnp-org:metadata-1-0/upnp/" \
"        http://www.upnp.org/schemas/av/upnp-v2-20060531.xsd\">" \
"    <container id=\"1\" parentID=\"0\" restricted=\"0\">" \
"        <dc:title>NonFlatCollection</dc:title>" \
"        <dc:creator>NonFlatCollection Author</dc:creator>" \
"        <item id=\"\" parentID=\"1\" restricted=\"0\">" \
"            <dc:title>Song1</dc:title>" \
"            <upnp:class>object.item.audioItem</upnp:class>" \
"            <res protocolInfo=\"*:*:*:*\" />" \
"            <upnp:artist></upnp:artist>" \
"        </item>" \
"        <item id=\"\" parentID=\"1\" restricted=\"0\">" \
"            <dc:title>Song2</dc:title>" \
"            <upnp:class>object.item.audioItem</upnp:class>" \
"            <res protocolInfo=\"*:*:*:*\" />" \
"        </item>" \
"        <item id=\"\" parentID=\"1\" restricted=\"0\">" \
"            <dc:title>Song3</dc:title>" \
"            <upnp:class>object.item.audioItem</upnp:class>" \
"        <res protocolInfo=\"*:*:*:*\" />" \
"    </item>" \
"    </container>" \
"</DIDL-Lite>"

#define TEST_CREATE_FLAT \
"<DIDL-Lite " \
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" " \
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" " \
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">" \
    "<item restricted=\"1\">" \
      "<dc:title>Song1</dc:title>" \
      "<upnp:class>object.item.audioItem</upnp:class>" \
      "<res>http://example.com/song1.mp3</res>" \
    "</item>" \
    "<item restricted=\"1\">" \
      "<dc:title>Song2</dc:title>" \
      "<upnp:class>object.item.audioItem</upnp:class>" \
      "<res>http://example.com/song2.mp3</res>" \
    "</item>" \
    "<item restricted=\"1\">" \
      "<dc:title>Song3</dc:title>" \
      "<upnp:class>object.item.audioItem</upnp:class>" \
      "<res>http://example.com/song3.mp3</res>" \
    "</item>" \
"</DIDL-Lite>"

#define TEST_CREATE_FULL \
"<DIDL-Lite " \
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" " \
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" " \
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">" \
    "<container>" \
      "<dc:title>TestCollection1</dc:title>" \
      "<dc:creator>TestCollection1Author</dc:creator>" \
      "<item restricted=\"1\">" \
        "<dc:title>Song1</dc:title>" \
        "<upnp:class>object.item.audioItem</upnp:class>" \
        "<res>http://example.com/song1.mp3</res>" \
      "</item>" \
      "<item restricted=\"1\">" \
        "<dc:title>Song2</dc:title>" \
        "<upnp:class>object.item.audioItem</upnp:class>" \
        "<res>http://example.com/song2.mp3</res>" \
      "</item>" \
      "<item restricted=\"1\">" \
        "<dc:title>Song3</dc:title>" \
        "<upnp:class>object.item.audioItem</upnp:class>" \
        "<res>http://example.com/song3.mp3</res>" \
      "</item>" \
    "</container>" \
"</DIDL-Lite>"

#define TEST_CREATE_FULL_REPARENT \
"<DIDL-Lite " \
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" " \
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\" " \
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\">" \
    "<container>" \
      "<item restricted=\"1\">" \
        "<dc:title>Song1</dc:title>" \
        "<upnp:class>object.item.audioItem</upnp:class>" \
        "<res>http://example.com/song1.mp3</res>" \
      "</item>" \
      "<item restricted=\"1\">" \
        "<dc:title>Song2</dc:title>" \
        "<upnp:class>object.item.audioItem</upnp:class>" \
        "<res>http://example.com/song2.mp3</res>" \
      "</item>" \
      "<item restricted=\"1\">" \
        "<dc:title>Song3</dc:title>" \
        "<upnp:class>object.item.audioItem</upnp:class>" \
        "<res>http://example.com/song3.mp3</res>" \
      "</item>" \
      "<dc:title>TestCollection1</dc:title>" \
      "<dc:creator>TestCollection1Author</dc:creator>" \
    "</container>" \
"</DIDL-Lite>"

#define TEST_PARSE_NO_XML "This is just some random text"

static void
test_didl_collection_construction (void)
{
        GUPnPMediaCollection *collection;

        /* Check that a collection created via auxillary function is mutable */
        collection = gupnp_media_collection_new ();
        g_assert (gupnp_media_collection_get_mutable (collection));
        g_object_unref (collection);

        /* Check that a collection created via auxillary parse function is
         * not mutable */
        collection = gupnp_media_collection_new_from_string
                                        (TEST_PARSE_COLLECTION_1);
        g_assert (!gupnp_media_collection_get_mutable (collection));
        g_object_unref (collection);

        /* Check that creating a collection via g_object_new and title/author
         * properties set is mutable */
        collection = g_object_new (GUPNP_TYPE_MEDIA_COLLECTION,
                                   "title", "test01collection",
                                   "author", "test01author",
                                   NULL);
        g_assert (gupnp_media_collection_get_mutable (collection));
        g_object_unref (collection);

        /* Check that creating a collection via g_object_new and data is not
         * mutable */
        collection = g_object_new (GUPNP_TYPE_MEDIA_COLLECTION,
                                   "data", TEST_PARSE_COLLECTION_1,
                                   NULL);
        g_assert (!gupnp_media_collection_get_mutable (collection));
        g_object_unref (collection);

        /* Check that creating a collection with all properties set will result
         * in a collection that ignored "title" and "author" properties.
         */
        collection = g_object_new (GUPNP_TYPE_MEDIA_COLLECTION,
                                   "title", "test01collection",
                                   "author", "test01author",
                                   "data", TEST_PARSE_COLLECTION_1,
                                   NULL);
        g_assert (!gupnp_media_collection_get_mutable (collection));
        g_assert_cmpstr (gupnp_media_collection_get_author (collection), ==,
                         NULL);
        g_assert_cmpstr (gupnp_media_collection_get_title (collection), ==,
                         NULL);
        g_assert_cmpstr (gupnp_media_collection_get_string (collection), ==,
                         TEST_PARSE_COLLECTION_1);
        g_object_unref (collection);
}

static void
test_didl_collection_parse_flat (void)
{
        GUPnPMediaCollection *collection;
        GList *items, *it;

        collection = gupnp_media_collection_new_from_string
                                        (TEST_PARSE_COLLECTION_1);
        g_assert_cmpstr (gupnp_media_collection_get_author (collection), ==,
                         NULL);
        g_assert_cmpstr (gupnp_media_collection_get_title (collection), ==,
                         NULL);
        g_assert_cmpstr (gupnp_media_collection_get_string (collection), ==,
                         TEST_PARSE_COLLECTION_1);

        it = items = gupnp_media_collection_get_items (collection);
        g_assert_cmpint (g_list_length (items), ==, 3);
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song1");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song2");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song3");
        g_list_free_full (items, (GDestroyNotify) g_object_unref);
}

static void
test_didl_collection_parse_full (void)
{
        GUPnPMediaCollection *collection;
        GList *items, *it;

        collection = gupnp_media_collection_new_from_string
                                        (TEST_PARSE_COLLECTION_2);
        g_assert_cmpstr (gupnp_media_collection_get_author (collection), ==,
                         "NonFlatCollection Author");
        g_assert_cmpstr (gupnp_media_collection_get_title (collection), ==,
                         "NonFlatCollection");
        g_assert_cmpstr (gupnp_media_collection_get_string (collection), ==,
                         TEST_PARSE_COLLECTION_2);

        it = items = gupnp_media_collection_get_items (collection);
        g_assert_cmpint (g_list_length (items), ==, 3);
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song1");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song2");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song3");
        g_list_free_full (items, (GDestroyNotify) g_object_unref);
}

#define ERROR_MESSAGE "Failed to parse DIDL-Lite: No 'DIDL-Lite' node in the DIDL-Lite XML:"

static gboolean
ignore_xml_parse_error (G_GNUC_UNUSED const gchar   *log_domain,
                        G_GNUC_UNUSED GLogLevelFlags log_level,
                        const gchar                 *message,
                        G_GNUC_UNUSED gpointer       user_data)
{
        if (strncmp (message,
                     ERROR_MESSAGE,
                     g_utf8_strlen (ERROR_MESSAGE, -1) - 1) == 0) {
                return FALSE;
        }

        return TRUE;
}

static void
test_didl_collection_parse_invalid (void)
{
        GUPnPMediaCollection *collection;
        GList *items;

        g_test_log_set_fatal_handler (ignore_xml_parse_error, NULL);
        collection = gupnp_media_collection_new_from_string (TEST_PARSE_NO_XML);
        items = gupnp_media_collection_get_items (collection);

        g_assert (items == NULL);
        g_object_unref (collection);
}

static void
test_didl_collection_create_flat (void)
{
        GUPnPMediaCollection *collection;
        GUPnPDIDLLiteItem *item;
        GList *items, *it;
        GUPnPDIDLLiteResource *res;

        collection = gupnp_media_collection_new ();
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song1");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song1.mp3");
        g_object_unref (res);
        g_object_unref (item);
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song2");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song2.mp3");
        g_object_unref (res);
        g_object_unref (item);
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song3");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song3.mp3");
        g_object_unref (res);
        g_object_unref (item);
        it = items = gupnp_media_collection_get_items (collection);
        g_assert_cmpint (g_list_length (items), ==, 3);
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song1");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song2");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song3");
        g_list_free_full (items, (GDestroyNotify) g_object_unref);
        g_assert_cmpstr (gupnp_media_collection_get_string (collection), ==,
                         TEST_CREATE_FLAT);
}

static void
test_didl_collection_create_full (void)
{
        GUPnPMediaCollection *collection;
        GUPnPDIDLLiteItem *item;
        GList *items, *it;
        GUPnPDIDLLiteResource *res;

        collection = gupnp_media_collection_new ();
        gupnp_media_collection_set_title (collection, "TestCollection1");
        gupnp_media_collection_set_author (collection, "TestCollection1Author");

        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song1");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song1.mp3");
        g_object_unref (res);
        g_object_unref (item);
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song2");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song2.mp3");
        g_object_unref (res);
        g_object_unref (item);
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song3");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song3.mp3");
        g_object_unref (res);
        g_object_unref (item);
        it = items = gupnp_media_collection_get_items (collection);
        g_assert_cmpint (g_list_length (items), ==, 3);
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song1");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song2");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song3");
        g_list_free_full (items, (GDestroyNotify) g_object_unref);
        g_assert_cmpstr (gupnp_media_collection_get_string (collection), ==,
                         TEST_CREATE_FULL);
}

static void
test_didl_collection_create_reparent (void)
{
        GUPnPMediaCollection *collection;
        GUPnPDIDLLiteItem *item;
        GList *items, *it;
        GUPnPDIDLLiteResource *res;

        collection = gupnp_media_collection_new ();

        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song1");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song1.mp3");
        g_object_unref (res);
        g_object_unref (item);
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song2");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song2.mp3");
        g_object_unref (res);
        g_object_unref (item);
        item = gupnp_media_collection_add_item (collection);
        gupnp_didl_lite_object_set_title (GUPNP_DIDL_LITE_OBJECT (item),
                                          "Song3");
        gupnp_didl_lite_object_set_upnp_class (GUPNP_DIDL_LITE_OBJECT (item),
                                               "object.item.audioItem");
        res = gupnp_didl_lite_object_add_resource (GUPNP_DIDL_LITE_OBJECT (item));
        gupnp_didl_lite_resource_set_uri (res, "http://example.com/song3.mp3");
        g_object_unref (res);
        g_object_unref (item);
        it = items = gupnp_media_collection_get_items (collection);

        /* Force reparenting of the items in the XML */
        gupnp_media_collection_set_title (collection, "TestCollection1");
        gupnp_media_collection_set_author (collection, "TestCollection1Author");

        g_assert_cmpint (g_list_length (items), ==, 3);
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song1");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song2");
        it = it->next;
        g_assert_cmpstr (gupnp_didl_lite_object_get_title (GUPNP_DIDL_LITE_OBJECT (it->data)), ==,
                         "Song3");
        g_list_free_full (items, (GDestroyNotify) g_object_unref);

        g_assert_cmpstr (gupnp_media_collection_get_string (collection), ==,
                         TEST_CREATE_FULL_REPARENT);
}

int main (int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
        g_type_init ();
#endif
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/didl/collection/construction",
                         test_didl_collection_construction);
        g_test_add_func ("/didl/collection/parse_flat",
                         test_didl_collection_parse_flat);
        g_test_add_func ("/didl/collection/parse_full",
                         test_didl_collection_parse_full);
        g_test_add_func ("/didl/collection/parse_invalid",
                         test_didl_collection_parse_invalid);

        g_test_add_func ("/didl/collection/create_flat",
                         test_didl_collection_create_flat);
        g_test_add_func ("/didl/collection/create_full",
                         test_didl_collection_create_full);
        g_test_add_func ("/didl/collection/create_reparent",
                         test_didl_collection_create_reparent);

        return g_test_run ();
}
