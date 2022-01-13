/*
 * Copyright (C) 2012 Openismus GmbH
 *
 * Author: Jens Georg <jensg@openismus.com>
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

#include <libgupnp-av/gupnp-didl-lite-parser.h>
#include <libgupnp-av/gupnp-didl-lite-writer.h>

#define TEST_DIDL_BGO674319 \
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
"        <dc:title>New Song</dc:title>" \
"        <upnp:class>object.item.audioItem</upnp:class>" \
"        <res protocolInfo=\"*:*:*:*\" />" \
"        <upnp:artist></upnp:artist>" \
"    </item>" \
"</DIDL-Lite>"

#define TEST_DIDL_BGO705564 \
"<DIDL-Lite xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\" " \
           "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\" " \
           "xmlns:dlna=\"urn:schemas-dlna-org:metadata-1-0/\">" \
        "<item>" \
                "<upnp:albumArtURI dlna:profileID=\"JPEG_TN\">" \
                "http://example.com/album.jpg"\
                "</upnp:albumArtURI>"\
        "</item>" \
"</DIDL-Lite>"

static void
test_bgo674319_on_object_available (G_GNUC_UNUSED GUPnPDIDLLiteParser *parser,
                                    GUPnPDIDLLiteObject               *object,
                                    gpointer                           user_data)
{
        GUPnPDIDLLiteObject **out = (GUPnPDIDLLiteObject **) user_data;
        *out = g_object_ref (object);
}

static void
test_bgo674319 (void)
{
        GUPnPDIDLLiteParser *parser;
        GUPnPDIDLLiteObject *object = NULL;
        GList *artists;

        parser = gupnp_didl_lite_parser_new ();
        g_signal_connect (parser,
                          "object-available",
                          G_CALLBACK (test_bgo674319_on_object_available),
                          &object);
        g_assert (gupnp_didl_lite_parser_parse_didl (parser, TEST_DIDL_BGO674319, NULL));
        g_assert (object != NULL);

        artists = gupnp_didl_lite_object_get_artists (object);
        g_assert (artists == NULL);
}

static void
test_bgo687462 (void)
{
        GUPnPDIDLLiteWriter *writer;
        GUPnPDIDLLiteObject *object;

        writer = gupnp_didl_lite_writer_new (NULL);
        object = (GUPnPDIDLLiteObject *)
                                gupnp_didl_lite_writer_add_item (writer);
        gupnp_didl_lite_object_set_album (object, "Test");
        g_assert_cmpstr (gupnp_didl_lite_object_get_album (object), ==, "Test");

        gupnp_didl_lite_object_set_album_art (object, "AlbumArt");
        g_assert_cmpstr (gupnp_didl_lite_object_get_album_art (object), ==, "AlbumArt");

        gupnp_didl_lite_writer_filter (writer, "upnp:album");

        g_assert_cmpstr (gupnp_didl_lite_object_get_album (object), ==, "Test");
        g_assert (gupnp_didl_lite_object_get_album_art (object) == NULL);
}

static void
test_bgo705564 (void)
{
        GUPnPDIDLLiteWriter *writer;
        GUPnPDIDLLiteObject *object;
        char *xml;

        writer = gupnp_didl_lite_writer_new (NULL);
        object = (GUPnPDIDLLiteObject *)
                        gupnp_didl_lite_writer_add_item (writer);
        gupnp_didl_lite_object_set_album_art (object,
                                              "http://example.com/album.jpg");

        xml = gupnp_didl_lite_writer_get_string (writer);
        g_assert_cmpstr (xml, ==, TEST_DIDL_BGO705564);
        g_free (xml);
        g_object_unref (object);
        g_object_unref (writer);
}

static void
test_bgo753314 (void)
{
        GUPnPDIDLLiteObject *object;
        GUPnPDIDLLiteWriter *writer;
        GUPnPDIDLLiteResource *resource;
        GUPnPProtocolInfo *pi;
        GList *list = NULL;
        GUPnPDLNAFlags flags = GUPNP_DLNA_FLAGS_NONE;

        flags = GUPNP_DLNA_FLAGS_BYTE_BASED_SEEK |
                GUPNP_DLNA_FLAGS_STREAMING_TRANSFER_MODE |
                GUPNP_DLNA_FLAGS_BACKGROUND_TRANSFER_MODE;

        writer = gupnp_didl_lite_writer_new (NULL);
        object = (GUPnPDIDLLiteObject *)
                        gupnp_didl_lite_writer_add_item (writer);
        resource = gupnp_didl_lite_object_add_resource (object);
        pi = gupnp_protocol_info_new ();
        gupnp_protocol_info_set_protocol (pi, "http");
        gupnp_protocol_info_set_mime_type (pi, "video/mp4");
        gupnp_protocol_info_set_dlna_flags (pi, flags);
        gupnp_didl_lite_resource_set_protocol_info (resource, pi);
        g_object_unref (pi);
        g_object_unref (resource);

        list = gupnp_didl_lite_object_get_resources (object);
        g_assert_cmpint (g_list_length (list), ==, 1);

        resource = list->data;
        pi = gupnp_didl_lite_resource_get_protocol_info (resource);

        g_assert_cmpint (gupnp_protocol_info_get_dlna_flags (pi), ==, flags);
        g_object_unref (resource);
        g_object_unref (object);
        g_object_unref (writer);
}

int main (int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
        g_type_init ();
#endif
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/bugs/gnome/674319", test_bgo674319);
        g_test_add_func ("/bugs/gnome/687462", test_bgo687462);
        g_test_add_func ("/bugs/gnome/705564", test_bgo705564);
/*        g_test_add_func ("/bugs/gnome/753314", test_bgo753314); */

        g_test_run ();


        return 0;
}
