/* GStreamer Wayland video sink
 *
 * Copyright (C) 2014-2015 Collabora Ltd.
 * Copyright (C) 2016 Renesas Electronics Corporation
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA.
 *
 * Based on wldmabufsink by Collabora Ltd.
 *
 */

#ifndef __GST_WL_DMABUF_H__
#define __GST_WL_DMABUF_H__

#include <gst/gst.h>
#include "gstwaylandsink.h"

G_BEGIN_DECLS

struct wl_buffer *
gst_wl_dmabuf_construct_wl_buffer (GstWaylandSink * sink, GstBuffer * buf,
    const GstVideoInfo * info);
G_END_DECLS

#endif /* __GST_WL_DMABUF_H__ */
