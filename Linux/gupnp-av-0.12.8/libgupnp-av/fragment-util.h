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

#ifndef __FRAGMENT_UTIL_H__
#define __FRAGMENT_UTIL_H__

#include <glib.h>
#include <libxml/tree.h>

#include "xsd-data.h"
#include "gupnp-av-enums.h"

G_BEGIN_DECLS

typedef struct {
        xmlDocPtr doc;
        xmlNodePtr node;
} DocNode;

G_GNUC_INTERNAL XSDData *
fragment_util_get_didl_lite_xsd_data    (void);

G_GNUC_INTERNAL GUPnPDIDLLiteFragmentResult
fragment_util_check_fragments           (DocNode     *original,
                                         DocNode     *modified,
                                         const gchar *current_fragment,
                                         const gchar *new_fragment,
                                         XSDData     *xsd_data);

G_GNUC_INTERNAL gboolean
fragment_util_apply_modification        (xmlNodePtr *node_ptr,
                                         DocNode    *modified);

G_END_DECLS

#endif /* __FRAGMENT_UTIL_H__ */
