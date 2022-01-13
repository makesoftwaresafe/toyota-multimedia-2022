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

#include <stdarg.h>
#include <libxml/parserInternals.h>

#include "fragment-util.h"
#include "xml-util.h"

typedef struct {
        gchar *node_name;
        gchar *attribute_name;
} NodeDiff;

static NodeDiff *
node_diff_new (const xmlChar *node_name,
               const xmlChar *attribute_name)
{
        NodeDiff *diff = g_slice_new (NodeDiff);

        diff->node_name = g_strdup ((gchar *) node_name);
        diff->attribute_name = g_strdup ((gchar *) attribute_name);

        return diff;
}

static void
node_diff_free (NodeDiff *diff)
{
        if (diff != NULL) {
                g_free (diff->node_name);
                g_free (diff->attribute_name);
                g_slice_free (NodeDiff, diff);
        }
}

static GList *
get_toplevel_changes (xmlNodePtr current_node,
                      xmlNodePtr new_node)
{
        xmlAttrPtr attribute;
        GHashTable *current_attributes = xml_util_get_attributes_map
                                        (current_node);
        GList *changes = NULL;
        const xmlChar *name = new_node->name;

        /* compare attributes */
        for (attribute = new_node->properties;
             attribute != NULL;
             attribute = attribute->next) {
                const xmlChar *value = NULL;
                const xmlChar *key = attribute->name;
                gboolean differs = FALSE;

                if (g_hash_table_lookup_extended (current_attributes,
                                                  key,
                                                  NULL,
                                                  (gpointer *) &value)) {
                        if (xmlStrcmp (value, attribute->children->content))
                                differs = TRUE;
                        g_hash_table_remove (current_attributes, key);
                } else
                        differs = TRUE;
                if (differs)
                        changes = g_list_prepend (changes,
                                                  node_diff_new (name,
                                                                 key));
        }

        if (g_hash_table_size (current_attributes) > 0) {
                GHashTableIter iter;
                xmlChar *key = NULL;

                g_hash_table_iter_init (&iter, current_attributes);
                while (g_hash_table_iter_next (&iter,
                                               (gpointer *) &key,
                                               NULL))
                        changes = g_list_prepend (changes, node_diff_new (name,
                                                                          key));
        }

        g_hash_table_unref (current_attributes);

        return changes;
}

#if GLIB_CHECK_VERSION (2, 32, 0)

#define hash_table_contains g_hash_table_contains
#define hash_table_add g_hash_table_add

#else

static gboolean
hash_table_contains (GHashTable *table,
                     gpointer    key)
{
  return g_hash_table_lookup_extended (table, key, NULL, NULL);
}

static void
hash_table_add (GHashTable *table,
                gpointer    key)
{
  g_hash_table_replace (table, key, key);
}

#endif

static gboolean
is_read_only (const gchar *changed_element,
              const gchar *changed_attribute)
{
        static GHashTable *readonly_props = NULL;
        static gsize readonly_props_loaded = 0;

        if (g_once_init_enter (&readonly_props_loaded)) {
                readonly_props = g_hash_table_new (g_str_hash,
                                                   g_str_equal);

                hash_table_add (readonly_props, (gpointer)  "@id");
                hash_table_add (readonly_props, (gpointer)  "@parentID");
                hash_table_add (readonly_props, (gpointer)  "@refID");
                hash_table_add (readonly_props, (gpointer)  "@restricted");
                hash_table_add (readonly_props, (gpointer)  "@searchable");
                hash_table_add (readonly_props, (gpointer)  "@childCount");
                hash_table_add (readonly_props, (gpointer)  "searchClass");
                hash_table_add (readonly_props, (gpointer)  "searchClass@name");
                hash_table_add (readonly_props, (gpointer)  "searchClass@includeDerived");
                hash_table_add (readonly_props, (gpointer)  "createClass");
                hash_table_add (readonly_props, (gpointer)  "createClass@name");
                hash_table_add (readonly_props, (gpointer)  "createClass@includeDerived");
                hash_table_add (readonly_props, (gpointer)  "writeStatus");
                hash_table_add (readonly_props, (gpointer)  "res@importUri");
                hash_table_add (readonly_props, (gpointer)  "storageTotal");
                hash_table_add (readonly_props, (gpointer)  "storageUsed");
                hash_table_add (readonly_props, (gpointer)  "storageFree");
                hash_table_add (readonly_props, (gpointer)  "storageMaxPartition");
                hash_table_add (readonly_props, (gpointer)  "storageMedium");
                hash_table_add (readonly_props, (gpointer)  "playbackCount");
                hash_table_add (readonly_props, (gpointer)  "srsRecordScheduleID");
                hash_table_add (readonly_props, (gpointer)  "srsRecordTaskID");
                hash_table_add (readonly_props, (gpointer)  "price");
                hash_table_add (readonly_props, (gpointer)  "price@currency");
                hash_table_add (readonly_props, (gpointer)  "payPerView");
                hash_table_add (readonly_props, (gpointer)  "dateTimeRange");
                hash_table_add (readonly_props, (gpointer)
                                "dateTimeRange@daylightSaving");
                hash_table_add (readonly_props, (gpointer)  "signalStrength");
                hash_table_add (readonly_props, (gpointer)  "signalLocked");
                hash_table_add (readonly_props, (gpointer)  "tuned");
                hash_table_add (readonly_props, (gpointer)  "containerUpdateID");
                hash_table_add (readonly_props, (gpointer)  "objectUpdateID");
                hash_table_add (readonly_props, (gpointer)  "totalDeletedChildCount");
                hash_table_add (readonly_props, (gpointer)  "res@updateCount");
                g_once_init_leave (&readonly_props_loaded, 1);
        }
        if (changed_element != NULL) {
                if (changed_attribute != NULL) {
                        gchar *test_prop = g_strdup_printf ("%s@%s",
                                                            changed_element,
                                                            changed_attribute);
                        gboolean result = hash_table_contains (readonly_props,
                                                               test_prop);

                        g_free (test_prop);
                        if (result)
                                return TRUE;
                        test_prop = g_strdup_printf ("@%s", changed_attribute);
                        result = hash_table_contains (readonly_props,
                                                      test_prop);
                        g_free (test_prop);
                        if (result)
                                return TRUE;
                }

                return hash_table_contains (readonly_props, changed_element);
        }

        return FALSE;
}

static gboolean
is_any_change_read_only (xmlNodePtr current_node,
                         xmlNodePtr new_node)
{
        GList *changes = get_toplevel_changes (current_node, new_node);
        GList *iter;
        gboolean read_only = FALSE;

        for (iter = changes; iter != NULL; iter = iter->next) {
                NodeDiff *diff = (NodeDiff *) iter->data;

                if (is_read_only (diff->node_name,
                                  diff->attribute_name)) {
                        read_only = TRUE;

                        break;
                }
        }

        if (changes != NULL)
                g_list_free_full (changes, (GDestroyNotify) node_diff_free);
        return read_only;
}

static GUPnPDIDLLiteFragmentResult
apply_temporary_modification (DocNode    *modified,
                              xmlNodePtr  current_node,
                              xmlNodePtr  new_node,
                              XSDData    *xsd_data)
{
        xmlNodePtr mod_cur_node = xml_util_find_node (modified->node,
                                                      current_node);
        xmlNodePtr new_node_copy = xml_util_copy_node (new_node);

        if (mod_cur_node == NULL) {
                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_UNKNOWN_ERROR;
        }

        xmlUnlinkNode (new_node_copy);
        mod_cur_node = xmlReplaceNode (mod_cur_node, new_node_copy);
        xmlUnlinkNode (mod_cur_node);
        xmlFreeNode (mod_cur_node);

        if (!xsd_data_validate_doc (xsd_data, modified->doc))
                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_NEW_INVALID;

        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK;
}

static GUPnPDIDLLiteFragmentResult
apply_temporary_addition (DocNode    *modified,
                          xmlNodePtr  sibling,
                          xmlNodePtr  new_node,
                          XSDData    *xsd_data)
{
        xmlNodePtr mod_sibling;
        xmlNodePtr new_node_copy = xml_util_copy_node (new_node);

        if (sibling->doc == modified->doc)
                mod_sibling = sibling;
        else
                mod_sibling = xml_util_find_node (modified->node, sibling);

        if (mod_sibling == NULL)
                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_UNKNOWN_ERROR;

        xmlUnlinkNode (new_node_copy);

        if (xmlAddNextSibling (mod_sibling, new_node_copy) == NULL) {
                xmlFreeNode (new_node_copy);

                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_UNKNOWN_ERROR;
        }

        if (!xsd_data_validate_doc (xsd_data, modified->doc))
                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_NEW_INVALID;

        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK;
}

static GUPnPDIDLLiteFragmentResult
apply_temporary_removal (DocNode    *modified,
                         xmlNodePtr  current_node,
                         XSDData    *xsd_data)
{
        xmlNodePtr mod_cur_node = xml_util_find_node (modified->node,
                                                      current_node);

        if (mod_cur_node == NULL)
                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_UNKNOWN_ERROR;

        xmlUnlinkNode (mod_cur_node);
        xmlFreeNode (mod_cur_node);
        if (!xsd_data_validate_doc (xsd_data, modified->doc))
                /* not sure if this is correct */
                return GUPNP_DIDL_LITE_FRAGMENT_RESULT_REQUIRED_TAG;

        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK;
}

typedef struct {
        gboolean required;
        GHashTable* required_dep_props; /* string set */
        GHashTable* required_indep_props; /* string to indep prop */
} IndependentProperty;

static void
independent_property_free (IndependentProperty *indep)
{
        if (indep != NULL) {
                g_hash_table_unref (indep->required_dep_props);
                g_hash_table_unref (indep->required_indep_props);
                g_slice_free (IndependentProperty, indep);
        }
}

static IndependentProperty *
independent_property_new (gboolean required)
{
        IndependentProperty *indep = g_slice_new (IndependentProperty);

        indep->required = required;
        indep->required_dep_props = g_hash_table_new_full (g_str_hash,
                                                           g_str_equal,
                                                           g_free,
                                                           NULL);
        indep->required_indep_props = g_hash_table_new_full
                                   (g_str_hash,
                                    g_str_equal,
                                    g_free,
                                    (GDestroyNotify) independent_property_free);

        return indep;
}

static void
insert_indep_prop (GHashTable          *props,
                   const gchar         *name,
                   IndependentProperty *prop)
{
        g_hash_table_insert (props, g_strdup (name), prop);
}

static void
insert_indep_prop_to_indep (IndependentProperty *prop,
                            const gchar         *name,
                            IndependentProperty *req_prop)
{
        insert_indep_prop (prop->required_indep_props, name, req_prop);
}

static void
add_dep_prop (IndependentProperty *indep,
              const gchar         *name)
{
        hash_table_add (indep->required_dep_props, g_strdup (name));
}

static IndependentProperty *
create_prop_with_required_dep_props (gboolean     required,
                                     const gchar *dep_prop,
                                     ...)
{
        IndependentProperty *indep = independent_property_new (required);

        if (dep_prop != NULL) {
                va_list var_args;
                const gchar *name = dep_prop;

                va_start (var_args, dep_prop);
                do {
                        add_dep_prop (indep, name);
                        name = va_arg (var_args, gchar *);
                } while (name != NULL);
                va_end (var_args);
        }

        return indep;
}

static IndependentProperty *
create_foreign_metadata_props (void)
{
        IndependentProperty *fm = independent_property_new (FALSE);
        IndependentProperty *other;

        add_dep_prop (fm, "type");

        other = independent_property_new (TRUE);
        insert_indep_prop_to_indep (fm, "fmId", other);

        other = independent_property_new (TRUE);
        insert_indep_prop_to_indep (fm, "fmClass", other);

        other = independent_property_new (TRUE);
        insert_indep_prop_to_indep (fm, "fmProvider", other);

        other = independent_property_new (TRUE);
        add_dep_prop (other, "xmlFlag");
        insert_indep_prop_to_indep (fm, "fmBody", other);

        return fm;
}

static GHashTable *
get_required_properties (void)
{
        static GHashTable *required_props = NULL;
        static gsize required_props_loaded = 0;

        if (g_once_init_enter (&required_props_loaded)) {
                required_props = g_hash_table_new_full
                                   (g_str_hash,
                                    g_str_equal,
                                    g_free,
                                    (GDestroyNotify) independent_property_free);

                insert_indep_prop (required_props,
                                   "",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "id",
                                         "parentID",
                                         "restricted",
                                         NULL));

                insert_indep_prop (required_props,
                                   "title",
                                   independent_property_new (TRUE));
                insert_indep_prop (required_props,
                                   "class",
                                   independent_property_new (TRUE));

                insert_indep_prop (required_props,
                                   "res",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "protocolInfo",
                                         NULL));
                insert_indep_prop (required_props,
                                   "programID",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "type",
                                         NULL));
                insert_indep_prop (required_props,
                                   "seriesID",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "type",
                                         NULL));
                insert_indep_prop (required_props,
                                   "channelID",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "type",
                                         NULL));
                insert_indep_prop (required_props,
                                   "programCode",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "type",
                                         NULL));
                insert_indep_prop (required_props,
                                   "channelGroupName",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "id",
                                         NULL));
                insert_indep_prop (required_props,
                                   "price",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "currency",
                                         NULL));
                insert_indep_prop (required_props,
                                   "desc",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "nameSpace",
                                         NULL));
                insert_indep_prop (required_props,
                                   "deviceUDN",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "serviceType",
                                         "serviceId",
                                         NULL));
                insert_indep_prop (required_props,
                                   "stateVariableCollection",
                                   create_prop_with_required_dep_props
                                        (FALSE,
                                         "serviceName",
                                         "rcsInstanceType",
                                         NULL));
                insert_indep_prop (required_props,
                                   "foreignMetadata",
                                   create_foreign_metadata_props ());
                g_once_init_leave (&required_props_loaded, 1);
        }

        return required_props;
}

static gboolean
is_required (const xmlChar *changed_element,
             const xmlChar *changed_attribute)
{
        GHashTable *required_props = get_required_properties ();

        if (changed_element != NULL) {
                IndependentProperty *toplevel_prop = g_hash_table_lookup
                                        (required_props,
                                         "");
                IndependentProperty *this_prop = g_hash_table_lookup
                                        (required_props,
                                         (gpointer) changed_element);

                if (changed_attribute != NULL) {
                        if (hash_table_contains
                                        (toplevel_prop->required_dep_props,
                                         changed_attribute))
                                return TRUE;
                        if (hash_table_contains (this_prop->required_dep_props,
                                                 changed_attribute))
                                return TRUE;
                }
                if (hash_table_contains (toplevel_prop->required_indep_props,
                                         changed_element))
                                return TRUE;
                /* TODO: check if changed element is not a required
                 * property of its parent element. That needs some
                 * additions in IndepependentProperty.
                 */
        }

        return FALSE;
}

static GUPnPDIDLLiteFragmentResult
new_doc_is_valid_modification (DocNode   *modified,
                               xmlDocPtr  current_doc,
                               xmlDocPtr  new_doc,
                               XSDData   *xsd_data)
{
        xmlNodePtr current_node = current_doc->children->children;
        xmlNodePtr new_node = new_doc->children->children;
        xmlNodePtr last_sibling = NULL;

        while (current_node != NULL && new_node != NULL) {
                GUPnPDIDLLiteFragmentResult result;
                xmlNodePtr temp_current_node = current_node;
                xmlNodePtr temp_new_node = new_node;

                last_sibling = new_node;
                /* We can't put this line into for instruction,
                 * because new_node could be unlinked from its
                 * document and put into another one in
                 * apply_temporary_modification. We have to get its
                 * sibling before that could happen.
                 */
                new_node = new_node->next;
                current_node = current_node->next;
                if (xml_util_node_deep_equal (temp_current_node, temp_new_node))
                        /* This is just a context, skip the checks. */
                        continue;
                if (xmlStrcmp (temp_current_node->name, temp_new_node->name))
                        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_NEW_INVALID;
                if (is_any_change_read_only (temp_current_node, temp_new_node))
                        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_READONLY_TAG;
                result = apply_temporary_modification (modified,
                                                       temp_current_node,
                                                       temp_new_node,
                                                       xsd_data);
                if (result != GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK)
                        return result;
        }
        if (last_sibling == NULL) {
                if (modified->node->children != NULL)
                        last_sibling = modified->node->last;
                else
                        /* We expect that modified object has some
                         * required tags like <upnp:class> or
                         * <dc:title>.
                         */
                        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_UNKNOWN_ERROR;
        }
        /* If there are some more nodes in current fragment then it
         * means they are going to be removed. Check against required
         * or read-only tag removal.
         */
        while (current_node != NULL) {
                GUPnPDIDLLiteFragmentResult result;
                xmlNodePtr temp_node = current_node;

                current_node = current_node->next;
                /* TODO: should we check if there are some readonly
                 * attributes when we remove whole element?
                 */
                if (is_read_only ((gchar *) temp_node->name, NULL))
                        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_READONLY_TAG;
                /* We don't check for required attributes or
                 * subelements, because most of them are required only
                 * when the element exists. And we are removing this
                 * one.
                 */
                if (is_required (temp_node->name, NULL))
                        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_REQUIRED_TAG;
                result = apply_temporary_removal (modified,
                                                  temp_node,
                                                  xsd_data);

                if (result != GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK)
                        return result;
        }
        /* If there are some more nodes in new fragment then it means
         * they are going to be added. Check against read-only tags
         * addition and general sanity check.
         */
        while (new_node != NULL) {
                GUPnPDIDLLiteFragmentResult result;
                xmlNodePtr temp_node;

                if (is_read_only ((gchar *) new_node->name, NULL))
                        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_READONLY_TAG;
                /* TODO: We probably should check if newly added node
                 * has all required properties. Maybe XSD check could
                 * do that for us.
                 */
                temp_node = new_node;
                new_node = new_node->next;
                result = apply_temporary_addition (modified,
                                                   last_sibling,
                                                   temp_node,
                                                   xsd_data);
                if (result != GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK)
                        return result;
        }

        return GUPNP_DIDL_LITE_FRAGMENT_RESULT_OK;
}

static gchar *
fix_fragment (const gchar *fragment)
{
        return g_strdup_printf
                    ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                     "<DIDLLiteFragment\n"
                     "xmlns:dc=\"http://purl.org/dc/elements/1.1/\"\n"
                     "xmlns=\"urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/\"\n"
                     "xmlns:upnp=\"urn:schemas-upnp-org:metadata-1-0/upnp/\"\n"
                     "xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"\n"
                     ">%s</DIDLLiteFragment>\n",
                     fragment);
}

static gboolean
is_current_doc_part_of_original_doc (DocNode   *original,
                                     xmlDocPtr  current_doc)
{
        xmlNodePtr current_node = current_doc->children->children;
        xmlNodePtr this_node;

        /* No current node means that we want to add new elements to
           the document. */
        if (current_node == NULL)
                return TRUE;

        this_node = xml_util_find_node (original->node, current_node);

        if (this_node == NULL)
                return FALSE;

        for (current_node = current_node->next, this_node = this_node->next;
             current_node != NULL && this_node != NULL;
             current_node = current_node->next, this_node = this_node->next)
                if (!xml_util_node_deep_equal (current_node, this_node))
                        return FALSE;

        return TRUE;
}

GUPnPDIDLLiteFragmentResult
fragment_util_check_fragments (DocNode     *original,
                               DocNode     *modified,
                               const gchar *current_fragment,
                               const gchar *new_fragment,
                               XSDData     *xsd_data)
{
        gchar *fixed_current_fragment = fix_fragment (current_fragment);
        gchar *fixed_new_fragment = fix_fragment (new_fragment);
        xmlDocPtr current_doc = xmlReadDoc (BAD_CAST (fixed_current_fragment),
                                            NULL,
                                            NULL,
                                            XML_PARSE_NONET);
        xmlDocPtr new_doc = xmlReadDoc (BAD_CAST (fixed_new_fragment),
                                        NULL,
                                        NULL,
                                        XML_PARSE_NONET);
        GUPnPDIDLLiteFragmentResult result;

        if (current_doc == NULL || current_doc->children == NULL) {
                result = GUPNP_DIDL_LITE_FRAGMENT_RESULT_CURRENT_BAD_XML;

                goto out;
        }

        if (new_doc == NULL || new_doc->children == NULL) {
                result = GUPNP_DIDL_LITE_FRAGMENT_RESULT_NEW_BAD_XML;

                goto out;
        }

        /* Assuming current_doc->children is non-NULL. */
        if (current_doc->children->children != NULL) {
            /* If the child element is title or class,
             * it must not be set to empty or removed.
             */
            if (g_strrstr ((char *) current_doc->children->children->name,
                           "title") != NULL ||
                g_strrstr ((char *) current_doc->children->children->name,
                           "class") != NULL) {
                /* If the new tag has no corresponding title or class element */
                if (new_doc->children->children == NULL) {
                    result = GUPNP_DIDL_LITE_FRAGMENT_RESULT_REQUIRED_TAG;

                    goto out;
                }
                /* If the new tag has an empty value for title or class */
                if (new_doc->children->children->children == NULL) {
                    result = GUPNP_DIDL_LITE_FRAGMENT_RESULT_REQUIRED_TAG;

                    goto out;
                }
            }
        }

        if (!is_current_doc_part_of_original_doc (original, current_doc)) {
                result = GUPNP_DIDL_LITE_FRAGMENT_RESULT_CURRENT_INVALID;

                goto out;
        }

        result = new_doc_is_valid_modification (modified,
                                                current_doc,
                                                new_doc,
                                                xsd_data);

 out:
        if (new_doc != NULL)
                xmlFreeDoc (new_doc);
        if (current_doc != NULL)
                xmlFreeDoc (current_doc);
        g_free (fixed_new_fragment);
        g_free (fixed_current_fragment);

        return result;
}

static const gchar *
get_data_dir (void)
{
        const gchar *datadir = g_getenv ("GUPNP_AV_DATADIR");

        if (datadir == NULL)
                /* that's a macro defined by -DDATADIR=foo */
                datadir = DATADIR;

        return datadir;
}

static gchar *
get_xsd_path (const gchar *path)
{
        return g_strdup_printf ("%s" G_DIR_SEPARATOR_S "%s",
                                get_data_dir (),
                                path);
}

static xmlParserInputPtr
our_own_loader (const char       *url,
                const char       *id,
                xmlParserCtxtPtr  context)
{
        gchar *basename;
        gchar *path;
        xmlParserInputPtr input;

        g_debug ("URL: %s, ID: %s.", url, id);

        basename = g_path_get_basename (url);
        path = get_xsd_path (basename);
        g_debug ("BASENAME: %s, PATH: %s", basename, path);
        input = xmlNewInputFromFile (context, path);

        g_free (basename);
        g_free (path);

        return input;
}

XSDData *
fragment_util_get_didl_lite_xsd_data (void)
{
        gchar *path = get_xsd_path ("didl-lite-v2.xsd");
        xmlExternalEntityLoader original_loader = xmlGetExternalEntityLoader ();
        XSDData *xsd_data;

        xmlSetExternalEntityLoader (our_own_loader);
        xsd_data = xsd_data_new (path);
        xmlSetExternalEntityLoader (original_loader);

        g_free (path);

        return xsd_data;
}

gboolean
fragment_util_apply_modification (xmlNodePtr *node_ptr,
                                  DocNode    *modified)
{
        xmlNodePtr node_copy;
        xmlNodePtr old;

        if (node_ptr == NULL || *node_ptr == NULL)
                return FALSE;

        node_copy = xml_util_copy_node (modified->node);

        if (node_copy == NULL)
                return FALSE;

        old = xmlReplaceNode (*node_ptr, node_copy);

        if (old == NULL)
                return FALSE;

        *node_ptr = node_copy;
        xmlFreeNode (old);

        return TRUE;
}
