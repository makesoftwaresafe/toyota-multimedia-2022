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

#include <libxml/parser.h>
#include <libxml/xmlschemas.h>

#include "xsd-data.h"

struct _XSDData {
        xmlSchemaPtr schema;
        xmlSchemaValidCtxtPtr valid_context;
};

XSDData *
xsd_data_new (const gchar *xsd_file)
{
        XSDData *xsd_data = g_slice_new0 (XSDData);
        gboolean failed = TRUE;
        xmlSchemaParserCtxtPtr context = xmlSchemaNewParserCtxt (xsd_file);

        if (context == NULL)
                /* unable to create a parser context for the schema */
                goto out;
        xsd_data->schema = xmlSchemaParse (context);
        if (xsd_data->schema == NULL)
                /* the schema itself is not valid */
                goto out;
        xsd_data->valid_context = xmlSchemaNewValidCtxt (xsd_data->schema);
        if (xsd_data->valid_context == NULL)
                /* unable to create a validation context for the schema */
                goto out;
        failed = FALSE;
 out:
        if (context != NULL)
                xmlSchemaFreeParserCtxt (context);
        if (failed) {
                xsd_data_free (xsd_data);
                xsd_data = NULL;
        }

        return xsd_data;
}

void
xsd_data_free (XSDData *xsd_data)
{
        if (xsd_data == NULL)
                return;
        if (xsd_data->valid_context != NULL)
                xmlSchemaFreeValidCtxt (xsd_data->valid_context);
        if (xsd_data->schema != NULL)
                xmlSchemaFree (xsd_data->schema);
        g_slice_free (XSDData, xsd_data);
}

gboolean
xsd_data_validate_doc (XSDData *xsd_data,
                       xmlDoc  *doc)
{
        static xmlSAXHandler empty_handler = {
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                NULL,
                XML_SAX2_MAGIC,
                NULL,
                NULL,
                NULL,
                NULL
        };
        xmlChar *dump = NULL;
        int size = 0;
        xmlParserInputBufferPtr buffer = NULL;
        gboolean result = FALSE;

        if (xsd_data == NULL)
                return TRUE;

        xmlDocDumpMemory (doc, &dump, &size);
        if (dump == NULL)
                goto out;
        g_debug ("Doc dump:\n%s", dump);
        buffer = xmlParserInputBufferCreateMem ((char *) dump,
                                                   size,
                                                   XML_CHAR_ENCODING_NONE);
        if (buffer == NULL)
                goto out;
        if (!xmlSchemaValidateStream (xsd_data->valid_context,
                                      buffer,
                                      XML_CHAR_ENCODING_NONE,
                                      &empty_handler,
                                      NULL))
                result = TRUE;
 out:
        /* Commented out, because it crashes because of double free. I
         * suppose that it is freed by xmlSchemaValidateStream.
         */
        /*
        if (buffer)
                xmlFreeParserInputBuffer (buffer);
        */
        if (dump != NULL)
                xmlFree (dump);
        return result;
}
