/*
 * Copyright (C) 2012 Intel Corporation.
 *
 * Authors: Regis Merlino <regis.merlino@intel.com>
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
 * SECTION:gupnp-feature-list-parser
 * @short_description: FeatureList state variable XML parser
 *
 * #GUPnPFeatureListParser parses XML strings from ContentDirectory
 * FeatureList state variable.
 *
 */

#include <string.h>
#include "gupnp-feature-list-parser.h"
#include "gupnp-av.h"
#include "xml-util.h"

/* GUPnPFeatureListParser */
G_DEFINE_TYPE (GUPnPFeatureListParser,
               gupnp_feature_list_parser,
               G_TYPE_OBJECT);

static void
gupnp_feature_list_parser_init (G_GNUC_UNUSED GUPnPFeatureListParser *parser)
{
        /* Nothing to do here */
}

static void
gupnp_feature_list_parser_finalize (GObject *object)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (gupnp_feature_list_parser_parent_class);
        object_class->finalize (object);
}

static void
gupnp_feature_list_parser_class_init (GUPnPFeatureListParserClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gupnp_feature_list_parser_finalize;
}

/**
 * gupnp_feature_list_parser_new:
 *
 * Return value: A new #GUPnPFeatureListParser object.
 **/
GUPnPFeatureListParser *
gupnp_feature_list_parser_new (void)
{
        return g_object_new (GUPNP_TYPE_FEATURE_LIST_PARSER, NULL);
}

static char *
get_feature_object_ids (xmlNode *feature)
{
        xmlNode    *element;
        char       *object_ids = NULL;
        const char *content;

        object_ids = (char *) g_malloc0(1);

        for (element = feature->children; element; element = element->next) {
                gsize new_size;

                if (g_ascii_strcasecmp ((char *) element->name,
                                        "objectIDs") == 0) {
                        content = (const char *) xmlNodeGetContent (element);

                        new_size = strlen (object_ids) + strlen (content) + 1;
                        if (*object_ids)
                                new_size++;

                        object_ids = g_realloc (object_ids, new_size);

                        if (*object_ids)
                                strcat (object_ids, ",");
                        strcat (object_ids, content);
                }
        }

        return object_ids;
}

/**
 * gupnp_feature_list_parser_parse_text:
 * @parser: A #GUPnPFeatureListParser
 * @text: The feature list string to be parsed
 * @error: The location where to store the error information if any, or NULL
 *
 * Parses @text and returns the list of available features.
 * If an error occured @error will be set.
 *
 * Return value: (transfer full) (element-type GUPnPFeature): The list of
 * features or %NULL if an error occured.
 **/
GList *
gupnp_feature_list_parser_parse_text
                                 (G_GNUC_UNUSED GUPnPFeatureListParser *parser,
                                  const char             *text,
                                  GError                **error)
{
        xmlDoc       *doc;
        xmlNode      *element;
        GList        *feature_list = NULL;

        doc = xmlRecoverMemory (text, strlen (text));
        if (doc == NULL) {
                g_set_error (error,
                             GUPNP_XML_ERROR,
                             GUPNP_XML_ERROR_PARSE,
                             "Could not parse FeatureList XML:\n%s", text);

                return NULL;
        }

        /* Get a pointer to root element */
        element = xml_util_get_element ((xmlNode *) doc, "Features", NULL);
        if (element == NULL) {
                g_set_error (error,
                             GUPNP_XML_ERROR,
                             GUPNP_XML_ERROR_NO_NODE,
                             "No 'Features' node in the XML:\n%s",
                             text);
                xmlFreeDoc (doc);

                return NULL;
        }

        for (element = element->children; element; element = element->next) {
                GUPnPFeature *feature;
                const char   *name;
                const char   *version;
                char         *object_ids;

                if (g_ascii_strcasecmp ((char *) element->name,
                                        "Feature") == 0) {
                        name = xml_util_get_attribute_content (element, "name");
                        version = xml_util_get_attribute_content (element,
                                                                  "version");
                        if (!name || !version) {
                                g_set_error (error,
                                             GUPNP_XML_ERROR,
                                             GUPNP_XML_ERROR_INVALID_ATTRIBUTE,
                                             "Invalid attributes in 'Feature' "
                                             "node in the XML:\n%s",
                                             text);

                                xmlFreeDoc (doc);
                                if (feature_list)
                                        g_list_free_full (feature_list,
                                                          g_object_unref);

                                return NULL;
                        }

                        object_ids = get_feature_object_ids (element);

                        feature = g_object_new (GUPNP_TYPE_FEATURE,
                                                "name", name,
                                                "version", version,
                                                "object-ids", object_ids,
                                                NULL);

                        feature_list = g_list_append (feature_list, feature);

                        g_free (object_ids);
                }
        }

        xmlFreeDoc (doc);

        return feature_list;
}
