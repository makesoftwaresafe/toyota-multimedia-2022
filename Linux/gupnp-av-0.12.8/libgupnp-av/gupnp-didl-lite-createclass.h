/*
 * Copyright (C) 2012 Intel Corporation.
 * Copyright (C) 2009 Nokia Corporation.
 *
 * Author: Christophe Guiraud <christophe.guiraud@intel.com>
 * Author: Zeeshan Ali (Khattak) <zeeshanak@gnome.org>
 *                               <zeeshan.ali@nokia.com>
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

#ifndef __GUPNP_DIDL_LITE_CREATE_CLASS_H__
#define __GUPNP_DIDL_LITE_CREATE_CLASS_H__

#include <glib-object.h>
#include <libxml/tree.h>
#include "gupnp-didl-lite-container.h"

G_BEGIN_DECLS

GType
gupnp_didl_lite_create_class_get_type (void) G_GNUC_CONST;

#define GUPNP_TYPE_DIDL_LITE_CREATE_CLASS \
                (gupnp_didl_lite_create_class_get_type ())
#define GUPNP_DIDL_LITE_CREATE_CLASS(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CREATE_CLASS, \
                 GUPnPDIDLLiteCreateClass))
#define GUPNP_DIDL_LITE_CREATE_CLASS_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CREATE_CLASS, \
                 GUPnPDIDLLiteCreateClassClass))
#define GUPNP_IS_DIDL_LITE_CREATE_CLASS(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CREATE_CLASS))
#define GUPNP_IS_DIDL_LITE_CREATE_CLASS_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CREATE_CLASS))
#define GUPNP_DIDL_LITE_CREATE_CLASS_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CREATE_CLASS, \
                 GUPnPDIDLLiteCreateClassClass))

typedef struct _GUPnPDIDLLiteCreateClassPrivate GUPnPDIDLLiteCreateClassPrivate;

/**
 * GUPnPDIDLLiteCreateClass:
 **/
typedef struct {
        GObject parent;

        /*< private >*/
        GUPnPDIDLLiteCreateClassPrivate *priv;
} GUPnPDIDLLiteCreateClass;

typedef struct {
        GObjectClass parent_class;

        /* future padding */
        void (* _gupnp_reserved1) (void);
        void (* _gupnp_reserved2) (void);
        void (* _gupnp_reserved3) (void);
        void (* _gupnp_reserved4) (void);
} GUPnPDIDLLiteCreateClassClass;

const char *
gupnp_didl_lite_create_class_get_content
                                    (GUPnPDIDLLiteCreateClass *create_class);

void
gupnp_didl_lite_create_class_set_content
                                    (GUPnPDIDLLiteCreateClass *create_class,
                                     const char               *content);

gboolean
gupnp_didl_lite_create_class_get_include_derived
                                    (GUPnPDIDLLiteCreateClass *create_class);

void
gupnp_didl_lite_create_class_set_include_derived
                                    (GUPnPDIDLLiteCreateClass *create_class,
                                     gboolean                 include_derived);

const char *
gupnp_didl_lite_create_class_get_friendly_name
                                    (GUPnPDIDLLiteCreateClass *create_class);

void
gupnp_didl_lite_create_class_set_friendly_name
                                    (GUPnPDIDLLiteCreateClass *create_class,
                                     const char               *friendly_name);

xmlNode *
gupnp_didl_lite_create_class_get_xml_node
                                    (GUPnPDIDLLiteCreateClass *create_class);

G_END_DECLS

#endif /* __GUPNP_DIDL_LITE_CREATE_CLASS_H__ */
