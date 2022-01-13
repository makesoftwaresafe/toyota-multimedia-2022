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

#ifndef __GUPNP_CDS_LAST_CHANGE_PARSER_H__
#define __GUPNP_CDS_LAST_CHANGE_PARSER_H__

#include <glib-object.h>

G_BEGIN_DECLS

GType
gupnp_cds_last_change_parser_get_type (void) G_GNUC_CONST;

GType
gupnp_cds_last_change_entry_get_type (void) G_GNUC_CONST;

#define GUPNP_TYPE_CDS_LAST_CHANGE_PARSER \
                (gupnp_cds_last_change_parser_get_type ())
#define GUPNP_CDS_LAST_CHANGE_PARSER(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 GUPNP_TYPE_CDS_LAST_CHANGE_PARSER, \
                 GUPnPCDSLastChangeParser))
#define GUPNP_CDS_LAST_CHANGE_PARSER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 GUPNP_TYPE_CDS_LAST_CHANGE_PARSER, \
                 GUPnPCDSLastChangeParserClass))
#define GUPNP_IS_CDS_LAST_CHANGE_PARSER(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 GUPNP_TYPE_CDS_LAST_CHANGE_PARSER))
#define GUPNP_IS_CDS_LAST_CHANGE_PARSER_CLASS(klass) \
                (G_TYPE_CHECK_CLASS_TYPE ((klass), \
                 GUPNP_TYPE_CDS_LAST_CHANGE_PARSER))
#define GUPNP_CDS_LAST_CHANGE_PARSER_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 GUPNP_TYPE_CDS_LAST_CHANGE_PARSER, \
                 GUPnPCDSLastChangeParserClass))

typedef struct _GUPnPCDSLastChangeParser GUPnPCDSLastChangeParser;
typedef struct _GUPnPCDSLastChangeParserClass GUPnPCDSLastChangeParserClass;
typedef struct _GUPnPCDSLastChangeParserPrivate GUPnPCDSLastChangeParserPrivate;
typedef struct _GUPnPCDSLastChangeEntry GUPnPCDSLastChangeEntry;

struct _GUPnPCDSLastChangeParser {
        GObject parent;

        GUPnPCDSLastChangeParserPrivate *priv;
};

struct _GUPnPCDSLastChangeParserClass {
        GObjectClass parent_class;
};

/**
 * GUPnPCDSLastChangeEvent:
 * @GUPNP_CDS_LAST_CHANGE_EVENT_INVALID: Invalid #GUPnPCDSLastChangeEntry.
 * @GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED: The #GUPnPCDSLastChangeEntry is
 * an object added event.
 * @GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED: The #GUPnPCDSLastChangeEntry
 * is an object removal event.
 * @GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED: The #GUPnPCDSLastChangeEntry
 * is an object modification event.
 * @GUPNP_CDS_LAST_CHANGE_EVENT_ST_DONE: The #GUPnPCDSLastChangeEntry is a
 * subtree update done event.
 */
typedef enum GUPnPCDSLastChangeEvent {
        GUPNP_CDS_LAST_CHANGE_EVENT_INVALID,
        GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
        GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED,
        GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
        GUPNP_CDS_LAST_CHANGE_EVENT_ST_DONE
} GUPnPCDSLastChangeEvent;

GUPnPCDSLastChangeParser *
gupnp_cds_last_change_parser_new (void);

GList *
gupnp_cds_last_change_parser_parse (GUPnPCDSLastChangeParser *parser,
                                    const char               *last_change,
                                    GError                  **error);

GUPnPCDSLastChangeEntry *
gupnp_cds_last_change_entry_ref    (GUPnPCDSLastChangeEntry *entry);
void
gupnp_cds_last_change_entry_unref  (GUPnPCDSLastChangeEntry *entry);

GUPnPCDSLastChangeEvent
gupnp_cds_last_change_entry_get_event         (GUPnPCDSLastChangeEntry *entry);

const char *
gupnp_cds_last_change_entry_get_object_id     (GUPnPCDSLastChangeEntry *entry);

const char *
gupnp_cds_last_change_entry_get_parent_id     (GUPnPCDSLastChangeEntry *entry);

const char *
gupnp_cds_last_change_entry_get_class         (GUPnPCDSLastChangeEntry *entry);

gboolean
gupnp_cds_last_change_entry_is_subtree_update (GUPnPCDSLastChangeEntry *entry);

guint32
gupnp_cds_last_change_entry_get_update_id    (GUPnPCDSLastChangeEntry *entry);

G_END_DECLS

#endif /* __GUPNP_CDS_LAST_CHANGE_PARSER_H__ */
