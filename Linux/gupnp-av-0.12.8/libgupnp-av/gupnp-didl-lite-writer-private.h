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

#ifndef __GUPNP_DIDL_LITE_WRITER_PRIVATE_H__
#define __GUPNP_DIDL_LITE_WRITER_PRIVATE_H__

#include "gupnp-didl-lite-writer.h"
#include "gupnp-didl-lite-container.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL GUPnPDIDLLiteItem *
gupnp_didl_lite_writer_add_container_child_item
                                        (GUPnPDIDLLiteWriter    *writer,
                                         GUPnPDIDLLiteContainer *container);

G_GNUC_INTERNAL void
gupnp_didl_lite_writer_filter_tags      (GUPnPDIDLLiteWriter   *writer,
                                         const char            *filter);

G_END_DECLS

#endif /* __GUPNP_DIDL_LITE_WRITER_PRIVATE_H__ */
