/*
 * Copyright (C) 2009 Nokia Corporation.
 * Copyright (C) 2012 Intel Corporation
 *
 * Authors: Zeeshan Ali (Khattak) <zeeshan.ali@nokia.com>
 *                                <zeeshanak@gnome.org>
 *          Christophe Guiraud <christophe.guiraud@intel.com>
 *          Krzesimir Nowak <krnowak@openismus.com>
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

#ifndef __GUPNP_DIDL_LITE_CONTAINER_H__
#define __GUPNP_DIDL_LITE_CONTAINER_H__

#include <glib-object.h>
#include "gupnp-didl-lite-object.h"

G_BEGIN_DECLS

GType
gupnp_didl_lite_container_get_type (void) G_GNUC_CONST;

#define GUPNP_TYPE_DIDL_LITE_CONTAINER \
                (gupnp_didl_lite_container_get_type ())
#define GUPNP_DIDL_LITE_CONTAINER(obj) \
                (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CONTAINER, \
                 GUPnPDIDLLiteContainer))
#define GUPNP_DIDL_LITE_CONTAINER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_CAST ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CONTAINER, \
                 GUPnPDIDLLiteContainerClass))
#define GUPNP_IS_DIDL_LITE_CONTAINER(obj) \
                (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CONTAINER))
#define GUPNP_IS_DIDL_LITE_CONTAINER_CLASS(obj) \
                (G_TYPE_CHECK_CLASS_TYPE ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CONTAINER))
#define GUPNP_DIDL_LITE_CONTAINER_GET_CLASS(obj) \
                (G_TYPE_INSTANCE_GET_CLASS ((obj), \
                 GUPNP_TYPE_DIDL_LITE_CONTAINER, \
                 GUPnPDIDLLiteContainerClass))

typedef struct {
        GUPnPDIDLLiteObject parent;
} GUPnPDIDLLiteContainer;

typedef struct {
        GUPnPDIDLLiteObjectClass parent_class;

        /* future padding */
        void (* _gupnp_reserved1) (void);
        void (* _gupnp_reserved2) (void);
        void (* _gupnp_reserved3) (void);
        void (* _gupnp_reserved4) (void);
} GUPnPDIDLLiteContainerClass;

gboolean
gupnp_didl_lite_container_get_searchable
                                        (GUPnPDIDLLiteContainer *container);

gint
gupnp_didl_lite_container_get_child_count
                                        (GUPnPDIDLLiteContainer *container);

guint
gupnp_didl_lite_container_get_container_update_id
                                        (GUPnPDIDLLiteContainer *container);

gboolean
gupnp_didl_lite_container_container_update_id_is_set
                                        (GUPnPDIDLLiteContainer *container);

guint
gupnp_didl_lite_container_get_total_deleted_child_count
                                        (GUPnPDIDLLiteContainer *container);

gboolean
gupnp_didl_lite_container_total_deleted_child_count_is_set
                                        (GUPnPDIDLLiteContainer *container);

GList *
gupnp_didl_lite_container_get_create_classes
                                        (GUPnPDIDLLiteContainer *container);

GList *
gupnp_didl_lite_container_get_create_classes_full
                                        (GUPnPDIDLLiteContainer *container);
GList *
gupnp_didl_lite_container_get_search_classes
                                        (GUPnPDIDLLiteContainer *container);

void
gupnp_didl_lite_container_set_searchable
                                        (GUPnPDIDLLiteContainer *container,
                                         gboolean                searchable);

void
gupnp_didl_lite_container_set_child_count
                                        (GUPnPDIDLLiteContainer *container,
                                         gint                    child_count);

void
gupnp_didl_lite_container_set_container_update_id
                                        (GUPnPDIDLLiteContainer *container,
                                         guint                   update_id);

void
gupnp_didl_lite_container_unset_container_update_id
                                        (GUPnPDIDLLiteContainer *container);

void
gupnp_didl_lite_container_set_total_deleted_child_count
                                        (GUPnPDIDLLiteContainer *container,
                                         guint                   count);

void
gupnp_didl_lite_container_unset_total_deleted_child_count
                                        (GUPnPDIDLLiteContainer *container);

void
gupnp_didl_lite_container_add_create_class
                                        (GUPnPDIDLLiteContainer *container,
                                         const char             *create_class);

void
gupnp_didl_lite_container_add_create_class_full
                                        (GUPnPDIDLLiteContainer *container,
                                         const char             *create_class,
                                         gboolean                include_derived);

void
gupnp_didl_lite_container_add_search_class
                                        (GUPnPDIDLLiteContainer *container,
                                         const char             *search_class);
void
gupnp_didl_lite_container_add_search_class_full
                                        (GUPnPDIDLLiteContainer *container,
                                         const char             *search_class,
                                         gboolean                include_derived);

gint64
gupnp_didl_lite_container_get_storage_used
                                       (GUPnPDIDLLiteContainer *container);

void
gupnp_didl_lite_container_set_storage_used
                                       (GUPnPDIDLLiteContainer *container,
                                        gint64                  storage_used);

G_END_DECLS

#endif /* __GUPNP_DIDL_LITE_CONTAINER_H__ */
