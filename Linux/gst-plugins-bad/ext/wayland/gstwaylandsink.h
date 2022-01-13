/*
 * GStreamer Wayland video sink
 * Copyright (C) 2011 Intel Corporation
 * Copyright (C) 2011 Sreerenj Balachandran <sreerenj.balachandran@intel.com>
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
 */

#ifndef __GST_WAYLAND_VIDEO_SINK_H__
#define __GST_WAYLAND_VIDEO_SINK_H__

#include <gst/gst.h>
#include <gst/video/video.h>

#include <wayland-client.h>

#include "wldisplay.h"
#include "wlwindow.h"

G_BEGIN_DECLS

#define GST_TYPE_WAYLAND_SINK \
	    (gst_wayland_sink_get_type())
#define GST_WAYLAND_SINK(obj) \
	    (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_WAYLAND_SINK,GstWaylandSink))
#define GST_WAYLAND_SINK_CLASS(klass) \
	    (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_WAYLAND_SINK,GstWaylandSinkClass))
#define GST_IS_WAYLAND_SINK(obj) \
	    (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_WAYLAND_SINK))
#define GST_IS_WAYLAND_SINK_CLASS(klass) \
	    (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_WAYLAND_SINK))
#define GST_WAYLAND_SINK_GET_CLASS(inst) \
        (G_TYPE_INSTANCE_GET_CLASS ((inst), GST_TYPE_WAYLAND_SINK, GstWaylandSinkClass))

#ifndef GST_CAPS_FEATURE_DMABUF
#define GST_CAPS_FEATURE_DMABUF   "memory:DMABuf"
#endif

#define GST_WAYLANDSINK_SUPPORTED_INPUT_FORMATS \
    "{ BGRx, BGRA, RGBx, xBGR, xRGB, RGBA, ABGR, ARGB, RGB, BGR, " \
    " RGB16, BGR16, YUY2, YVYU, UYVY, AYUV, NV12, NV21, NV16, " \
    " YUV9, YVU9, Y41B, I420, YV12, Y42B, v308 }"

#define GST_WAYLANDSINK_DMA_MAKE_SURFACE_CAPS               \
    GST_VIDEO_CAPS_MAKE_WITH_FEATURES(          \
    GST_CAPS_FEATURE_DMABUF, GST_WAYLANDSINK_SUPPORTED_INPUT_FORMATS)

typedef struct _GstWaylandSink GstWaylandSink;
typedef struct _GstWaylandSinkClass GstWaylandSinkClass;

struct _GstWaylandSink
{
  GstVideoSink parent;

  GMutex display_lock;
  GstWlDisplay *display;
  GstWlWindow *window;
  GstBufferPool *pool;

  gboolean video_info_changed;
  GstVideoInfo video_info;

  gchar *display_name;

  gboolean redraw_pending;
  GMutex render_lock;
  GstBuffer *last_buffer;

  gboolean use_subsurface;

  struct wl_event_queue *frame_queue;
  struct wl_display *display_wrapper;
  guint surface_id;
  gboolean is_firstframe;
};

struct _GstWaylandSinkClass
{
  GstVideoSinkClass parent;
  void (*first_videoframe_rendered) (GstElement *element, GstPad *pad);
};

GType gst_wayland_sink_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif /* __GST_WAYLAND_VIDEO_SINK_H__ */
