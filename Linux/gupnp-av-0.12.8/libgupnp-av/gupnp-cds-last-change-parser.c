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
 * SECTION:gupnp-cds-last-change-parser
 * @short_description: LastChange parser for the format used in
 * CDS:3
 *
 * #GUPnPCDSLastChangeParser parses XML strings from
 * CDS's LastChange state variable.
 *
 */

#include <libgupnp/gupnp-error.h>

#include "xml-util.h"
#include "gupnp-cds-last-change-parser.h"

/**
 * GUPnPCDSLastChangeEntry:
 *
 * Opaque struct which contains information about the event.
 **/
struct _GUPnPCDSLastChangeEntry {
        int ref_count;
        GUPnPCDSLastChangeEvent event;
        char *object_id;
        char *parent_id;
        char *class;
        guint32 update_id;
        gboolean is_subtree_update;
};

G_DEFINE_TYPE (GUPnPCDSLastChangeParser,
               gupnp_cds_last_change_parser,
               G_TYPE_OBJECT)

G_DEFINE_BOXED_TYPE (GUPnPCDSLastChangeEntry,
                     gupnp_cds_last_change_entry,
                     gupnp_cds_last_change_entry_ref,
                     gupnp_cds_last_change_entry_unref);

static void
gupnp_cds_last_change_parser_init (G_GNUC_UNUSED GUPnPCDSLastChangeParser *parser)
{
}

static void
gupnp_cds_last_change_parser_class_init (G_GNUC_UNUSED GUPnPCDSLastChangeParserClass *klass)
{
}

/**
 * gupnp_cds_last_change_parser_new:
 *
 * Create a new #GUPnPCDSLastChangeParser.
 *
 * This parser is able to parse LastChange as defined in the
 * ContentDirectory:3 specification.
 *
 * Returns:(transfer full): A new instance of #GUPnPCDSLastChangeParser.
 **/
GUPnPCDSLastChangeParser *
gupnp_cds_last_change_parser_new (void)
{
        return g_object_new (GUPNP_TYPE_CDS_LAST_CHANGE_PARSER,
                             NULL);
}

/**
 * gupnp_cds_last_change_parser_parse:
 * @parser: #GUPnPCDSLastChangeParser
 * @last_change: XML string to parse
 * @error: Return value for parser error or %NULL to ingore
 *
 * Parse a LastChange XML document in the flavor defined by the
 * ContentDirectory:3 specification.
 *
 * Returns: (element-type GUPnPCDSLastChangeEntry)(transfer full):
 * List of #GUPnPCDSLastChangeEntry<!-- -->s
 **/
GList *
gupnp_cds_last_change_parser_parse (GUPnPCDSLastChangeParser *parser,
                                    const char               *last_change,
                                    GError                  **error)

{
        xmlDoc *doc;
        xmlNode *state_event, *it;
        GList *result = NULL;
        GUPnPCDSLastChangeEntry *entry;

        g_return_val_if_fail (GUPNP_IS_CDS_LAST_CHANGE_PARSER (parser),
                              NULL);

        doc = xmlParseDoc ((const xmlChar *) last_change);
        if (doc == NULL) {
                g_set_error (error,
                             GUPNP_XML_ERROR,
                             GUPNP_XML_ERROR_PARSE,
                             "Could not parse LastChange XML");

                goto out;
        }

        state_event = xml_util_get_element ((xmlNode *) doc,
                                            "StateEvent",
                                            NULL);
        if (state_event == NULL) {
                g_set_error (error,
                             GUPNP_XML_ERROR,
                             GUPNP_XML_ERROR_PARSE,
                             "Missing StateEvent node");

                goto out;
        }

        for (it = state_event->children; it != NULL; it = it->next) {
                if (it->type == XML_TEXT_NODE)
                        continue;
                else if (g_ascii_strcasecmp ((const char *) it->name,
                                             "objAdd") == 0) {
                        const char *tmp;

                        entry = g_slice_new0 (GUPnPCDSLastChangeEntry);
                        entry->ref_count = 1;
                        entry->event = GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED;

                        tmp = xml_util_get_attribute_content (it, "objID");
                        entry->object_id = g_strdup (tmp);

                        tmp = xml_util_get_attribute_content (it,
                                                              "objParentID");
                        entry->parent_id = g_strdup (tmp);

                        tmp = xml_util_get_attribute_content (it, "objClass");
                        entry->class = g_strdup (tmp);

                        entry->update_id = (guint32) xml_util_get_uint_attribute
                                        (it,
                                         "updateID",
                                         0);
                        entry->is_subtree_update =
                                        xml_util_get_boolean_attribute
                                                (it,
                                                 "stUpdate");
                } else if (g_ascii_strcasecmp ((const char *) it->name,
                                               "objMod") == 0) {
                        const char *tmp;

                        entry = g_slice_new0 (GUPnPCDSLastChangeEntry);
                        entry->ref_count = 1;
                        entry->event = GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED;

                        tmp = xml_util_get_attribute_content (it, "objID");
                        entry->object_id = g_strdup (tmp);

                        entry->update_id = (guint32) xml_util_get_uint_attribute
                                        (it,
                                         "updateID",
                                         0);
                        entry->is_subtree_update =
                                        xml_util_get_boolean_attribute
                                                (it,
                                                 "stUpdate");
                } else if (g_ascii_strcasecmp ((const char *) it->name,
                                               "objDel") == 0) {
                        const char *tmp;

                        entry = g_slice_new0 (GUPnPCDSLastChangeEntry);
                        entry->ref_count = 1;
                        entry->event = GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED;

                        tmp = xml_util_get_attribute_content (it, "objID");
                        entry->object_id = g_strdup (tmp);

                        entry->update_id = (guint32) xml_util_get_uint_attribute
                                        (it,
                                         "updateID",
                                         0);
                        entry->is_subtree_update =
                                        xml_util_get_boolean_attribute
                                                (it,
                                                 "stUpdate");
                } else if (g_ascii_strcasecmp ((const char *) it->name,
                                               "stDone") == 0) {
                        const char *tmp;

                        entry = g_slice_new0 (GUPnPCDSLastChangeEntry);
                        entry->ref_count = 1;
                        entry->event = GUPNP_CDS_LAST_CHANGE_EVENT_ST_DONE;

                        tmp = xml_util_get_attribute_content (it, "objID");
                        entry->object_id = g_strdup (tmp);

                        entry->update_id = (guint32) xml_util_get_uint_attribute
                                        (it,
                                         "updateID",
                                         0);
                } else {
                        g_warning ("Skipping invalid LastChange entry: %s",
                                   (const char *) it->name);
                        continue;
                }

                result = g_list_prepend (result, entry);
        }

        result = g_list_reverse (result);
out:
        if (doc != NULL) {
                xmlFreeDoc (doc);
        }

        return result;

}

/**
 * gupnp_cds_last_change_entry_ref:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Increase reference count of a #GUPnPCDSLastChangeEntry.
 *
 * Returns:(transfer full): The object passed in @entry.
 **/
GUPnPCDSLastChangeEntry *
gupnp_cds_last_change_entry_ref (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL, NULL);
        g_return_val_if_fail (entry->ref_count > 0, NULL);

        g_atomic_int_inc (&entry->ref_count);

        return entry;
}

/**
 * gupnp_cds_last_change_entry_unref:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Decrease reference count of a #GUPnPCDSLastChangeEntry. If the reference
 * count drops to 0, @entry is freed.
 **/
void
gupnp_cds_last_change_entry_unref (GUPnPCDSLastChangeEntry *entry)
{
        g_return_if_fail (entry != NULL);
        g_return_if_fail (entry->ref_count > 0);

        if (g_atomic_int_dec_and_test (&entry->ref_count)) {
                g_free (entry->class);
                g_free (entry->object_id);
                g_free (entry->parent_id);

                g_slice_free (GUPnPCDSLastChangeEntry, entry);
        }
}

/**
 * gupnp_cds_last_change_entry_get_event:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Get the type of the last change entry as defined in
 * #GUPnPCDSLastChangeEvent.
 *
 * Returns: An event from the #GUPnPCDSLastChangeEvent or
 * %GUPNP_CDS_LAST_CHANGE_EVENT_INVALID if the entry is not valid.
 **/
GUPnPCDSLastChangeEvent
gupnp_cds_last_change_entry_get_event (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL,
                              GUPNP_CDS_LAST_CHANGE_EVENT_INVALID);

        return entry->event;
}

/**
 * gupnp_cds_last_change_entry_get_object_id:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Get the ID of the object in this change entry.
 *
 * Returns: (transfer none): The id of the object of this entry.
 **/
const char *
gupnp_cds_last_change_entry_get_object_id (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL, NULL);

        return entry->object_id;
}

/**
 * gupnp_cds_last_change_entry_get_parent_id:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Get the parent object id of the object in this change entry. This is only
 * valid if gupnp_cds_last_change_entry_get_event() returns
 * %GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED.
 *
 * Returns: (transfer none): The id of the object's parent of this entry.
 **/
const char *
gupnp_cds_last_change_entry_get_parent_id (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL, NULL);

        return entry->parent_id;
}

/**
 * gupnp_cds_last_change_entry_get_class:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Get the class of the object in this change entry. This is only
 * valid if gupnp_cds_last_change_entry_get_event() returns
 * %GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED.
 *
 * Returns: (transfer none): The upnp class of the object of this entry.
 **/
const char *
gupnp_cds_last_change_entry_get_class (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL, NULL);

        return entry->class;
}

/**
 * gupnp_cds_last_change_entry_is_subtree_update:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Returns whether this entry is part of a subtree update.
 *
 * Returns: %TRUE, if the entry is part of a subtree update, %FALSE otherwise.
 **/
gboolean
gupnp_cds_last_change_entry_is_subtree_update (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL, FALSE);

        return entry->is_subtree_update;
}

/**
 * gupnp_cds_last_change_entry_get_update_id:
 * @entry: A #GUPnPCDSLastChangeEntry
 *
 * Get the update id of the last change entry.
 *
 * Returns: update id of the entry or 0 if the entry is not valid.
 **/
guint32
gupnp_cds_last_change_entry_get_update_id (GUPnPCDSLastChangeEntry *entry)
{
        g_return_val_if_fail (entry != NULL, 0);

        return entry->update_id;
}
