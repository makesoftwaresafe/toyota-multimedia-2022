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

#include <gst/allocators/gstdmabuf.h>

#include "wldmabuf.h"
#include "wlbuffer.h"
#include "wlvideoformat.h"
#include "linux-dmabuf-unstable-v1-client-protocol.h"
#include <errno.h>
#include <string.h>

extern int errno;

GST_DEBUG_CATEGORY_EXTERN (gstwayland_debug);
#define GST_CAT_DEFAULT gstwayland_debug

typedef struct
{
  struct wl_buffer *wbuf;
  gboolean done;
  GstWaylandSink *sink;
} ConstructBufferData;

static void
linux_dmabuf_buffer_created (void *data,
    struct zwp_linux_buffer_params_v1 *params, struct wl_buffer *buffer)
{
  ConstructBufferData *d = data;

  GST_DEBUG_OBJECT (d->sink, "wl_buffer %p created", buffer);
  d->wbuf = buffer;
  zwp_linux_buffer_params_v1_destroy (params);
  d->done = TRUE;
}

static void
linux_dmabuf_buffer_failed (void *data, 
    struct zwp_linux_buffer_params_v1 *params)
{
  ConstructBufferData *d = data;

  GST_DEBUG_OBJECT (d->sink, "failed to create wl_buffer");
  d->wbuf = NULL;
  zwp_linux_buffer_params_v1_destroy (params);
  d->done = TRUE;
}

static const struct zwp_linux_buffer_params_v1_listener buffer_params_listener = {
  linux_dmabuf_buffer_created,
  linux_dmabuf_buffer_failed,
};

struct wl_buffer *
gst_wl_dmabuf_construct_wl_buffer (GstWaylandSink * sink, GstBuffer * buf,
    const GstVideoInfo * info)
{
  struct zwp_linux_buffer_params_v1 *params;
  ConstructBufferData data;
  GstVideoMeta *vidmeta;
  gint i, n_planes, retval, display_error_code;
  GstMemory *mem;

  GST_DEBUG_OBJECT (sink, "Creating wl_buffer for buffer %p", buf);

  mem = gst_buffer_peek_memory (buf, 0);

  data.sink = sink;

  /* initialize wbuf to null */
  data.wbuf = NULL;

  params = zwp_linux_dmabuf_v1_create_params (sink->display->dmabuf);
  zwp_linux_buffer_params_v1_add_listener (params, &buffer_params_listener, &data);

  wl_proxy_set_queue((struct wl_proxy*)params,sink->display->dmabuf_own_queue);

  vidmeta = gst_buffer_get_video_meta (buf);
  n_planes = vidmeta ? vidmeta->n_planes : GST_VIDEO_INFO_N_PLANES (info);

  for (i = 0; i < n_planes; i++) {
    guint offset, stride, mem_idx, length;
    gsize skip;

    offset = GST_VIDEO_INFO_PLANE_OFFSET (info, i);
    stride = GST_VIDEO_INFO_PLANE_STRIDE (info, i);
    if (gst_buffer_find_memory (buf, offset, 1, &mem_idx, &length, &skip)) {
      GstMemory *m = gst_buffer_peek_memory (buf, mem_idx);
      gint fd = gst_dmabuf_memory_get_fd (m);
      zwp_linux_buffer_params_v1_add (params, fd, i, m->offset + skip,
          stride, 0, 0);
    } else {
      GST_ERROR_OBJECT (mem->allocator, "memory does not seem to contain "
          "enough data for the specified format");
      goto done;
    }
  }

  zwp_linux_buffer_params_v1_create (params, info->width, info->height,
      gst_video_format_to_wl_dmabuf_format (info->finfo->format),
      ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT);
  wl_display_flush (sink->display->display);

  data.done = FALSE;
  while (!data.done) {
    while (wl_display_prepare_read_queue (sink->display->display, sink->display->dmabuf_own_queue) != 0) {
      wl_display_dispatch_queue_pending (sink->display->display, sink->display->dmabuf_own_queue);
      if (data.done)
        goto done;
    }

    wl_display_flush (sink->display->display);
    retval = poll(&sink->display->pfd, 1, 1000);
    if (retval == 0) {
      /* timeout of 1 second happened so print message return null */
      wl_display_cancel_read (sink->display->display);
      GST_ERROR_OBJECT(sink, "timeout while waiting for zwp_linux_buffer_params_v1 event");
      goto error;
    } else if (retval < 0) {
      wl_display_cancel_read (sink->display->display);
      GST_ERROR_OBJECT(sink, "Error during poll errno:%d errmsg:%s", errno, strerror(errno));
      goto error;
    } else if(sink->display->pfd.revents & POLLIN) {
      wl_display_read_events (sink->display->display);
      wl_display_dispatch_queue_pending (sink->display->display, sink->display->dmabuf_own_queue);
      if (data.done)
        goto done;
    }

    display_error_code = wl_display_get_error(sink->display->display);
    if (display_error_code) {
      GST_ERROR("Wayland display:Fatal error occurred with error code %d",display_error_code);
      goto error;
    }
  }

error:
  zwp_linux_buffer_params_v1_destroy (params);

done:
  return data.wbuf;
}
