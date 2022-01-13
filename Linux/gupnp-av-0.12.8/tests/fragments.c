/*
 * Copyright (C) 2012 Intel Corporation
 *
 * Authors: Krzesimir Nowak <krnowak@openismus.com>
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

#include <glib-object.h>
#include <libgupnp-av/gupnp-didl-lite-object.h>
#include <libgupnp-av/gupnp-didl-lite-writer.h>
#include <libgupnp-av/gupnp-didl-lite-item.h>

/* creates an item described by:
static const gchar * const didllite =
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<DIDL-Lite\n"
        "xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
        "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\"\n"
        "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\"\n"
        "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
        "xsi:schemaLocation=\"\n"
        "urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\n"
        "http://www.upnp.org/schemas/av/didl-lite.xsd\n"
        "urn:schemas-upnp-org:metadata-1-0/upnp/\n"
        "http://www.upnp.org/schemas/av/upnp.xsd\">\n"
        "<item id=\"$id\" parentID=\"$parent_id\" restricted=\"0\">\n"
        "<dc:title>Try a little tenderness</dc:title>\n"
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>\n"
        "<res protocolInfo=\"http-get:*:audio/mpeg:*\" size=\"3558000\">\n"
        "http://168.192.1.1/audio197.mp3\n"
        "</res>\n"
        "<upnp:artist>Unknown</upnp:artist>\n"
        "</item>\n"
        "</DIDL-Lite>\n";
*/
static GUPnPDIDLLiteObject *
get_item (GUPnPDIDLLiteWriter *writer, guint id, guint parent_id)
{
        GUPnPDIDLLiteItem *item = gupnp_didl_lite_writer_add_item (writer);
        GUPnPDIDLLiteObject *object = GUPNP_DIDL_LITE_OBJECT (item);
        GUPnPDIDLLiteContributor *artist;
        GUPnPDIDLLiteResource *resource;
        GUPnPProtocolInfo *info;
        gchar *str_id = g_strdup_printf ("%u", id);

        gupnp_didl_lite_object_set_id (object, str_id);
        g_free (str_id);
        str_id = g_strdup_printf ("%u", parent_id);
        gupnp_didl_lite_object_set_parent_id (object, str_id);
        g_free (str_id);
        gupnp_didl_lite_object_set_restricted (object, FALSE);
        gupnp_didl_lite_object_set_title (object, "Try a little tenderness");
        gupnp_didl_lite_object_set_upnp_class
                                        (object,
                                         "object.item.audioItem.musicTrack");
        artist = gupnp_didl_lite_object_add_artist (object);
        gupnp_didl_lite_contributor_set_name (artist, "Unknown");
        g_object_unref (artist);
        resource = gupnp_didl_lite_object_add_resource (object);
        info = gupnp_protocol_info_new ();
        gupnp_protocol_info_set_protocol (info, "http-get");
        gupnp_protocol_info_set_network (info, "*");
        gupnp_protocol_info_set_mime_type (info, "audio/mpeg");
        gupnp_didl_lite_resource_set_protocol_info (resource, info);
        g_object_unref (info);
        gupnp_didl_lite_resource_set_size (resource, 3558000);
        gupnp_didl_lite_resource_set_uri (resource,
                                          "http://168.192.1.1/audio197.mp3");
        g_object_unref (resource);

        return object;
}

static const gchar *current_fragments[] = {
        /* 1 */
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>",
        /* 2 */
        "",
        /* 3 */
        "<upnp:artist>Unknown</upnp:artist>",
        /* 4 */
        "<dc:title>Try a little tenderness</dc:title>"
};

static const gchar *new_fragments[] = {
        /* 1 */
        "<upnp:class>object.item.audioItem.musicTrack</upnp:class>"
        "<upnp:genre>Obscure</upnp:genre>",
        /* 2 */
        "<upnp:genre>Even more obscure</upnp:genre>",
        /* 3 */
        "",
        /* 4 */
        "<dc:title>Cthulhu fhtagn</dc:title>"
};

static void
debug_dump (GUPnPDIDLLiteObject *object)
{
        xmlChar *dump = NULL;
        xmlNodePtr node = gupnp_didl_lite_object_get_xml_node (object);
        xmlDocPtr doc = node->doc;

        xmlDocDumpMemory (doc, &dump, NULL);
        g_debug ("Obj dump:\n%s", dump);
        xmlFree (dump);
}

int main (void)
{
        GUPnPDIDLLiteObject *temp_object;
        GUPnPDIDLLiteObject *object;
        GUPnPDIDLLiteFragmentResult result;
        GUPnPDIDLLiteWriter *writer;
        int retval = 1;
        const gchar *value;
        GList* artists;
        GUPnPDIDLLiteContributor *artist;

#if !GLIB_CHECK_VERSION (2, 35, 0)
        g_type_init ();
#endif
        g_setenv ("GUPNP_AV_DATADIR", ABS_TOP_SRCDIR G_DIR_SEPARATOR_S "data", FALSE);

        writer = gupnp_didl_lite_writer_new (NULL);
        temp_object = get_item (writer, 3, 2);
        object = get_item (writer, 18, 13);
        debug_dump (object);
        result = gupnp_didl_lite_object_apply_fragments (object,
                                                         (char **) current_fragments,
                                                         G_N_ELEMENTS (current_fragments),
                                                         (char **) new_fragments,
                                                         G_N_ELEMENTS (new_fragments));
        debug_dump (object);
        if (result != GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK) {
                g_warning ("Applying fragments failed.");
                goto out;
        }

        value = gupnp_didl_lite_object_get_title (object);

        if (g_strcmp0 (value, "Cthulhu fhtagn")) {
                g_warning ("Title is '%s', should be 'Cthulhu fhtagn'.", value);
                goto out;
        }

        artists = gupnp_didl_lite_object_get_artists (object);

        if (artists) {
                g_warning ("Should be no artists.");
                g_list_free_full (artists, g_object_unref);
                goto out;
        }

        value = gupnp_didl_lite_object_get_title (temp_object);

        if (g_strcmp0 (value, "Try a little tenderness")) {
                g_warning ("Title is '%s', should be 'Try a little tenderness'.", value);
                goto out;
        }

        artists = gupnp_didl_lite_object_get_artists (temp_object);

        if (!artists) {
                g_warning ("Should be one artist, there are none.");
                goto out;
        }
        if (artists->next) {
                g_list_free_full (artists, g_object_unref);
                g_warning ("Should be one artist, there are more.");
                goto out;
        }
        artist = g_object_ref (artists->data);
        g_list_free_full (artists, g_object_unref);
        value = gupnp_didl_lite_contributor_get_name (artist);
        if (g_strcmp0 (value, "Unknown")) {
                g_object_unref (artist);
                g_warning ("Artist is '%s', but should be 'Unknown'.", value);
                goto out;
        }
        g_object_unref (artist);

        retval = 0;
 out:
        g_object_unref (object);
        g_object_unref (temp_object);
        g_object_unref (writer);
        return retval;
}
