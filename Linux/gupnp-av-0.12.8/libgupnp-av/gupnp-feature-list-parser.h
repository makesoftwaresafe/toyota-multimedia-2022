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

#ifndef __GUPNP_FEATURE_LIST_PARSER_H_
#define __GUPNP_FEATURE_LIST_PARSER_H_

#include <glib-object.h>
#include "gupnp-feature.h"

G_BEGIN_DECLS

GType
gupnp_feature_list_parser_get_type (void) G_GNUC_CONST;

#define GUPNP_TYPE_FEATURE_LIST_PARSER \
                (gupnp_feature_list_parser_get_type ())
#define GUPNP_FEATURE_LIST_PARSER(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 GUPNP_TYPE_FEATURE_LIST_PARSER, \
                 GUPnPFeatureListParser))
#define GUPNP_FEATURE_LIST_PARSER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 GUPNP_TYPE_FEATURE_LIST_PARSER, \
                 GUPnPFeatureListParserClass))
#define GUPNP_IS_FEATURE_LIST_PARSER(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 GUPNP_TYPE_FEATURE_LIST_PARSER))
#define GUPNP_IS_FEATURE_LIST_PARSER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 GUPNP_TYPE_FEATURE_LIST_PARSER))
#define GUPNP_FEATURE_LIST_PARSER_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 GUPNP_TYPE_FEATURE_LIST_PARSER, \
                 GUPnPFeatureListParserClass))

typedef struct {
        GObject parent;
} GUPnPFeatureListParser;

typedef struct {
        GObjectClass parent_class;
} GUPnPFeatureListParserClass;

GUPnPFeatureListParser *
gupnp_feature_list_parser_new        (void);

GList *
gupnp_feature_list_parser_parse_text (GUPnPFeatureListParser *parser,
                                      const char             *text,
                                      GError                 **error);

G_END_DECLS

#endif /* __GUPNP_FEATURE_LIST_PARSER_H_ */
