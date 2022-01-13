/*
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
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <gst/gst.h>

#include <gst/video/video.h>
#include <gst/video/videooverlay.h>

#include <gst/wayland/wayland.h>

#include <glib-unix.h>

#include <wayland-client.h>

#include <linux/input.h>

#include <sys/types.h>
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define INFINITE_LOOP_PLAYBACK          -1

typedef struct
{
  GMainLoop *loop;
  GSource *source;
  GstElement *pipeline;

  gint fullscreen;
  gboolean nloop;

  struct wl_display *display;
  struct wl_event_queue *queue;
  struct wl_registry *registry;
  struct wl_compositor *compositor;
  struct wl_shell *shell;
  struct wl_shm *shm;
  struct wl_seat *seat;
  struct wl_pointer *pointer;
  struct wl_touch *touch;
  GList *surfaces;
  GList *shell_surfaces;
  GList *outputs;

  struct wl_surface *focused_surface;
  gint min_refresh;
  gint64 frame_cnt;

  guint signal_watch_id;
} GstWlDemo;

static void
pointer_handle_enter (void *data, struct wl_pointer *pointer,
    uint32_t serial, struct wl_surface *surface, wl_fixed_t sx, wl_fixed_t sy)
{
  GstWlDemo *priv = data;

  priv->focused_surface = surface;
}

static void
pointer_handle_leave (void *data, struct wl_pointer *pointer,
    uint32_t serial, struct wl_surface *surface)
{
  GstWlDemo *priv = data;

  priv->focused_surface = NULL;
}

static void
pointer_handle_motion (void *data, struct wl_pointer *pointer,
    uint32_t time, wl_fixed_t sx, wl_fixed_t sy)
{
}

static void
pointer_handle_button (void *data, struct wl_pointer *wl_pointer,
    uint32_t serial, uint32_t time, uint32_t button, uint32_t state)
{
  GstWlDemo *priv = data;
  struct wl_shell_surface *shell_surface;

  if (!priv->focused_surface)
    return;

  shell_surface = wl_surface_get_user_data (priv->focused_surface);

  if (button == BTN_LEFT && state == WL_POINTER_BUTTON_STATE_PRESSED)
    wl_shell_surface_move (shell_surface, priv->seat, serial);
}

static void
pointer_handle_axis (void *data, struct wl_pointer *wl_pointer,
    uint32_t time, uint32_t axis, wl_fixed_t value)
{
}

static const struct wl_pointer_listener pointer_listener = {
  pointer_handle_enter,
  pointer_handle_leave,
  pointer_handle_motion,
  pointer_handle_button,
  pointer_handle_axis,
};

static void
touch_handle_down (void *data, struct wl_touch *wl_touch,
    uint32_t serial, uint32_t time, struct wl_surface *surface,
    int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
  GstWlDemo *priv = data;
  struct wl_shell_surface *shell_surface;

  shell_surface = wl_surface_get_user_data (surface);

  wl_shell_surface_move (shell_surface, priv->seat, serial);
}

static void
touch_handle_up (void *data, struct wl_touch *wl_touch,
    uint32_t serial, uint32_t time, int32_t id)
{
}

static void
touch_handle_motion (void *data, struct wl_touch *wl_touch,
    uint32_t time, int32_t id, wl_fixed_t x_w, wl_fixed_t y_w)
{
}

static void
touch_handle_frame (void *data, struct wl_touch *wl_touch)
{
}

static void
touch_handle_cancel (void *data, struct wl_touch *wl_touch)
{
}

static const struct wl_touch_listener touch_listener = {
  touch_handle_down,
  touch_handle_up,
  touch_handle_motion,
  touch_handle_frame,
  touch_handle_cancel,
};

static void
seat_handle_capabilities (void *data, struct wl_seat *seat,
    enum wl_seat_capability caps)
{
  GstWlDemo *priv = data;

  if ((caps & WL_SEAT_CAPABILITY_POINTER) && !priv->pointer) {
    priv->pointer = wl_seat_get_pointer (seat);
    wl_proxy_set_queue ((struct wl_proxy *) priv->pointer, priv->queue);
    wl_pointer_add_listener (priv->pointer, &pointer_listener, priv);
  } else if (!(caps & WL_SEAT_CAPABILITY_POINTER) && priv->pointer) {
    wl_pointer_destroy (priv->pointer);
    priv->pointer = NULL;
  }

  if ((caps & WL_SEAT_CAPABILITY_TOUCH) && !priv->touch) {
    priv->touch = wl_seat_get_touch (seat);
    wl_touch_set_user_data (priv->touch, priv);
    wl_proxy_set_queue ((struct wl_proxy *) priv->touch, priv->queue);
    wl_touch_add_listener (priv->touch, &touch_listener, priv);
  } else if (!(caps & WL_SEAT_CAPABILITY_TOUCH) && priv->touch) {
    wl_touch_destroy (priv->touch);
    priv->touch = NULL;
  }
}

static const struct wl_seat_listener seat_listener = {
  seat_handle_capabilities,
};

static void
handle_ping (void *data, struct wl_shell_surface *shell_surface,
    uint32_t serial)
{
  wl_shell_surface_pong (shell_surface, serial);
}

static void
handle_configure (void *data, struct wl_shell_surface *shell_surface,
    uint32_t edges, int32_t width, int32_t height)
{
}

static void
handle_popup_done (void *data, struct wl_shell_surface *shell_surface)
{
}

static const struct wl_shell_surface_listener shell_surface_listener = {
  handle_ping,
  handle_configure,
  handle_popup_done
};

static void
shm_format (void *data, struct wl_shm *wl_shm, uint32_t format)
{
  GST_DEBUG ("supported format=%08x", format);
}

static const struct wl_shm_listener shm_listener = {
  shm_format
};

static void
display_handle_geometry (void *data, struct wl_output *wl_output, int x, int y,
    int physical_width, int physical_height, int subpixel, const char *make,
    const char *model, int transform)
{
}

static void
display_handle_mode (void *data, struct wl_output *wl_output, uint32_t flags,
    int width, int height, int refresh)
{
  GstWlDemo *priv = data;

  if (flags & WL_OUTPUT_MODE_CURRENT && priv->min_refresh > refresh)
    priv->min_refresh = refresh;
}

static void
display_handle_done (void *data, struct wl_output *wl_output)
{
}

static void
display_handle_scale (void *data, struct wl_output *wl_output, int32_t scale)
{
}

static const struct wl_output_listener output_listener = {
  display_handle_geometry,
  display_handle_mode,
  display_handle_done,
  display_handle_scale
};

static void
registry_handle_global (void *data, struct wl_registry *registry,
    uint32_t id, const char *interface, uint32_t version)
{
  GstWlDemo *priv = data;
  struct wl_output *output;

  if (g_strcmp0 (interface, "wl_compositor") == 0) {
    priv->compositor = wl_registry_bind (registry, id, &wl_compositor_interface,
        MIN (version, 3));
  } else if (g_strcmp0 (interface, "wl_shell") == 0) {
    priv->shell = wl_registry_bind (registry, id, &wl_shell_interface, 1);
  } else if (g_strcmp0 (interface, "wl_shm") == 0) {
    priv->shm = wl_registry_bind (registry, id, &wl_shm_interface, 1);
  } else if (g_strcmp0 (interface, "wl_seat") == 0) {
    priv->seat = wl_registry_bind (registry, id, &wl_seat_interface, 1);
    wl_proxy_set_queue ((struct wl_proxy *) priv->seat, priv->queue);
    wl_seat_add_listener (priv->seat, &seat_listener, priv);
  } else if (g_strcmp0 (interface, "wl_output") == 0) {
    output = wl_registry_bind (registry, id, &wl_output_interface, 1);
    wl_proxy_set_queue ((struct wl_proxy *) output, priv->queue);
    wl_output_add_listener (output, &output_listener, priv);
    priv->outputs = g_list_append (priv->outputs, output);
  }
}

static const struct wl_registry_listener registry_listener = {
  registry_handle_global
};

static gboolean
setup_surface (GstWlDemo * priv, struct wl_surface *surface,
    struct wl_shell_surface *shell_surface, gint width, gint height)
{
  char filename[1024];
  static int cnt = 0;
  int fd;
  GstVideoInfo vinfo;
  void *data;
  struct wl_shm_pool *shm_pool;
  struct wl_buffer *wlbuffer;

  /*
   * waylandsink creates a wl_subsurface from an external wl_surface passed by
   * the application and attaches buffers from the upstream to
   * the wl_subsurface. A wl_subsurface becomes visible by mapping
   * its parent wl_surface, so we have to draw the wl_surface that will be passed
   * to waylandsink.
   */
  /* Transparently draw the area of the same size as the video resolution
     by using a shm buffer. */
  gst_video_info_set_format (&vinfo, GST_VIDEO_FORMAT_BGRA, width, height);

  snprintf (filename, 1024, "%s/%s-demo-%d-%s", g_get_user_runtime_dir (),
      "wayland-shm", cnt++, "XXXXXX");

  fd = mkstemp (filename);
  if (fd < 0) {
    g_printerr ("temp file %s creation failed: %s\n", filename,
        strerror (errno));
    return FALSE;
  }

  if (ftruncate (fd, vinfo.size) < 0) {
    g_printerr ("ftruncate failed: %s\n", strerror (errno));
    close (fd);
    return FALSE;
  }

  data = mmap (NULL, vinfo.size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  if (data == MAP_FAILED) {
    g_printerr ("mmap failed: %s\n", strerror (errno));
    close (fd);
    return FALSE;
  }

  memset (data, 0, vinfo.size);

  munmap (data, vinfo.size);

  shm_pool = wl_shm_create_pool (priv->shm, fd, vinfo.size);
  wlbuffer =
      wl_shm_pool_create_buffer (shm_pool, 0, GST_VIDEO_INFO_WIDTH (&vinfo),
      GST_VIDEO_INFO_HEIGHT (&vinfo),
      GST_VIDEO_INFO_PLANE_STRIDE (&vinfo, 0), WL_SHM_FORMAT_ARGB8888);
  wl_shm_pool_destroy (shm_pool);
  unlink (filename);
  close (fd);

  wl_proxy_set_queue ((struct wl_proxy *) shell_surface, priv->queue);

  wl_shell_surface_add_listener (shell_surface, &shell_surface_listener, priv);
  wl_shell_surface_set_toplevel (shell_surface);

  wl_surface_set_user_data (surface, shell_surface);

  if (priv->fullscreen != -1) {
    struct wl_event_queue *queue;
    struct wl_region *region;
    struct wl_output *output;

    region = wl_compositor_create_region (priv->compositor);
    wl_region_add (region, 0, 0, width, height);
    wl_surface_set_opaque_region (surface, region);
    wl_region_destroy (region);

    output = g_list_nth_data (priv->outputs, priv->fullscreen);
    if (!output) {
      g_printerr
          ("failed to get wl_output object, so could not set fullscreen\n");
      return FALSE;
    }

    queue = wl_display_create_queue (priv->display);

    wl_shell_surface_set_fullscreen (shell_surface,
        WL_SHELL_SURFACE_FULLSCREEN_METHOD_DEFAULT, 0, output);
    wl_display_roundtrip_queue (priv->display, queue);

    wl_event_queue_destroy (queue);
  }

  wl_surface_attach (surface, wlbuffer, 0, 0);
  wl_surface_damage (surface, 0, 0, GST_VIDEO_INFO_WIDTH (&vinfo),
      GST_VIDEO_INFO_HEIGHT (&vinfo));
  wl_surface_commit (surface);
  wl_display_flush (priv->display);

  return TRUE;
}

static GstBusSyncReply
bus_sync_handler (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GstWlDemo *priv = user_data;

  if (gst_is_wayland_display_handle_need_context_message (message)) {
    GstContext *context;

    context = gst_wayland_display_handle_context_new (priv->display);
    gst_element_set_context (GST_ELEMENT (GST_MESSAGE_SRC (message)), context);

    goto drop;
  } else if (gst_is_video_overlay_prepare_window_handle_message (message)) {
    GstPad *pad;
    GstCaps *caps;
    GstStructure *structure;
    struct wl_surface *surface;
    struct wl_shell_surface *shell_surface;
    gint width, height;

    if (!g_str_has_prefix (GST_MESSAGE_SRC_NAME (message), "waylandsink"))
      return GST_BUS_PASS;

    pad = gst_element_get_static_pad ((GstElement *) GST_MESSAGE_SRC (message),
        "sink");
    caps = gst_pad_get_current_caps (pad);
    structure = gst_caps_get_structure (caps, 0);
    gst_structure_get_int (structure, "width", &width);
    gst_structure_get_int (structure, "height", &height);
    gst_caps_unref (caps);
    gst_object_unref (pad);

    surface = wl_compositor_create_surface (priv->compositor);
    shell_surface = wl_shell_get_shell_surface (priv->shell, surface);

    if (!setup_surface (priv, surface, shell_surface, width, height)) {
      wl_shell_surface_destroy (shell_surface);
      wl_surface_destroy (surface);
      return GST_BUS_PASS;
    }

    priv->surfaces = g_list_append (priv->surfaces, surface);
    priv->shell_surfaces = g_list_append (priv->shell_surfaces, shell_surface);

    gst_video_overlay_set_window_handle (GST_VIDEO_OVERLAY (GST_MESSAGE_SRC
            (message)), (guintptr) surface);
    gst_video_overlay_set_render_rectangle (GST_VIDEO_OVERLAY (GST_MESSAGE_SRC
            (message)), 0, 0, width, height);

    goto drop;
  }

  return GST_BUS_PASS;

drop:
  gst_message_unref (message);
  return GST_BUS_DROP;
}

static gboolean
bus_async_handler (GstBus * bus, GstMessage * message, gpointer user_data)
{
  GstWlDemo *priv = user_data;

  switch (GST_MESSAGE_TYPE (message)) {
    case GST_MESSAGE_ERROR:{
      GError *err;
      gchar *debug;

      gst_message_parse_error (message, &err, &debug);
      g_print ("Error: %s\n", err->message);
      g_error_free (err);
      g_free (debug);

      goto quit;
    }
    case GST_MESSAGE_APPLICATION:
    {
      const GstStructure *s;

      s = gst_message_get_structure (message);

      if (gst_structure_has_name (s, "GstWaylandDemoInterrupt"))
        goto quit;
    }
      break;
    case GST_MESSAGE_EOS:
      if (priv->nloop > 1 || priv->nloop == INFINITE_LOOP_PLAYBACK) {
        if (!gst_element_seek (priv->pipeline,
                1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH,
                GST_SEEK_TYPE_SET, 0,
                GST_SEEK_TYPE_NONE, GST_CLOCK_TIME_NONE)) {
          g_printerr ("seek event sending failed\n");
          goto quit;
        }

        if (priv->nloop != INFINITE_LOOP_PLAYBACK)
          priv->nloop--;
        break;
      }

      goto quit;
    default:
      break;
  }

  return TRUE;

quit:
  g_main_loop_quit (priv->loop);
  g_source_destroy (priv->source);
  priv->source = NULL;
  return TRUE;
}

typedef struct _GstWaylandEventSource
{
  GSource source;
  GPollFD pfd;
  struct wl_display *display;
  struct wl_event_queue *queue;
  gboolean reading;
} GstWaylandEventSource;

static gboolean
event_source_prepare (GSource * source, gint * timeout)
{
  GstWaylandEventSource *wl_source = (GstWaylandEventSource *) source;

  *timeout = -1;

  if (wl_source->reading)
    return FALSE;

  while (wl_display_prepare_read_queue (wl_source->display,
          wl_source->queue) != 0) {
    wl_display_dispatch_queue_pending (wl_source->display, wl_source->queue);
  }
  wl_display_flush (wl_source->display);

  wl_source->reading = TRUE;

  return FALSE;
}

static gboolean
event_source_check (GSource * source)
{
  GstWaylandEventSource *wl_source = (GstWaylandEventSource *) source;

  return ! !(wl_source->pfd.revents & G_IO_IN);
}

static gboolean
event_source_dispatch (GSource * source, GSourceFunc callback,
    gpointer user_data)
{
  GstWaylandEventSource *wl_source = (GstWaylandEventSource *) source;

  wl_display_read_events (wl_source->display);
  wl_display_dispatch_queue_pending (wl_source->display, wl_source->queue);

  wl_source->reading = FALSE;

  return TRUE;
}

static void
event_source_finalize (GSource * source)
{
  GstWaylandEventSource *wl_source = (GstWaylandEventSource *) source;

  if (wl_source->reading) {
    wl_display_cancel_read (wl_source->display);
    wl_source->reading = FALSE;
  }
}

static GSourceFuncs GstWaylandEventSourceFuncs = {
  event_source_prepare,
  event_source_check,
  event_source_dispatch,
  event_source_finalize
};

static void
setup_framerate_adjustment (const GValue * item, gpointer user_data)
{
  GstWlDemo *priv = user_data;
  GstElement *elem;
  GstElement *peer_elem;
  GstPad *pad;
  GstPad *peer_pad;
  GstCaps *caps;

  elem = g_value_get_object (item);

  if (g_str_has_prefix (GST_ELEMENT_NAME (elem), "videorate")) {
    /* Get the element immediately after this videorate */
    pad = gst_element_get_static_pad (elem, "src");
    peer_pad = gst_pad_get_peer (pad);
    peer_elem = gst_pad_get_parent_element (peer_pad);
    gst_object_unref (pad);
    gst_object_unref (peer_pad);

    caps = gst_caps_new_simple ("video/x-raw", "framerate",
        GST_TYPE_FRACTION, priv->min_refresh / 1000, 1, NULL);

    gst_element_unlink (elem, peer_elem);
    gst_element_link_filtered (elem, peer_elem, caps);
    gst_object_unref (peer_elem);
  }
}

static GstPadProbeReturn
cb_have_data (GstPad * pad, GstPadProbeInfo * info, gpointer user_data)
{
  GstWlDemo *priv = user_data;
  time_t t;
  struct tm *tm;
  gint64 cur_time;
  gint64 framesinsec;
  static gint64 prev_time = 0;
  static gint64 prev_cnt = 0;

  priv->frame_cnt++;

  if (prev_time == 0) {
    prev_time = g_get_monotonic_time ();
    return GST_PAD_PROBE_OK;
  }

  cur_time = g_get_monotonic_time ();

  if (cur_time - prev_time >= 1000000) {
    framesinsec = priv->frame_cnt - prev_cnt;
    t = time (NULL);
    tm = localtime (&t);

    g_print ("FPS: %3ld  TIME %02d:%02d:%02d\n", framesinsec, tm->tm_hour,
        tm->tm_min, tm->tm_sec);

    prev_cnt = priv->frame_cnt;
    prev_time = cur_time;
  }

  return GST_PAD_PROBE_OK;
}

static gboolean
sigint_handler (gpointer user_data)
{
  GstWlDemo *priv = user_data;

  gst_element_post_message (GST_ELEMENT (priv->pipeline),
      gst_message_new_application (GST_OBJECT (priv->pipeline),
          gst_structure_new ("GstWaylandDemoInterrupt", "message",
              G_TYPE_STRING, "Pipeline interrupted", NULL)));

  priv->signal_watch_id = 0;

  return FALSE;
}

static void
shell_surface_destroy (struct wl_shell_surface *shell_surface)
{
  wl_shell_surface_destroy (shell_surface);
}

static void
surface_destroy (struct wl_surface *surface)
{
  wl_surface_destroy (surface);
}

static void
output_destroy (struct wl_output *output)
{
  wl_output_destroy (output);
}

int
main (int argc, char **argv)
{
  GstWaylandEventSource *wl_source;
  GstWlDemo *priv;
  GOptionContext *context;
  GstBus *bus;
  GstIterator *it;
  GstElement *elem;
  GstPad *pad;
  GstStateChangeReturn state_ret;
  GTimer *timer = NULL;
  GError *error = NULL;
  guint bus_watch_id;
  gint fullscreen = -1;
  gint nloop = 0;
  gchar *measuring_pad = NULL;
  gchar *elem_name;
  gchar *pad_name;
  gdouble elapsed;
  gchar **argvn;
  gint i;
  int ret = EXIT_FAILURE;
  GOptionEntry options[] = {
    {"fullscreen", 'f', 0, G_OPTION_ARG_INT, &fullscreen,
        "Display in fullscreen mode on dest_num output [-f dest_num]", NULL},
    {"loop", 'l', 0, G_OPTION_ARG_INT, &nloop,
        "Loop Playback for loop_count [-l loop_count]", NULL},
    {"fps", 'p', 0, G_OPTION_ARG_STRING, &measuring_pad,
          "Framerate display (specify a measuring point as regular format [-p element:pad])",
        NULL},
    {NULL}
  };

  gst_init (&argc, &argv);

  context = g_option_context_new ("PIPELINE-DESCRIPTION");
  g_option_context_add_main_entries (context, options, NULL);
  g_option_context_add_group (context, gst_init_get_option_group ());
  if (!g_option_context_parse (context, &argc, &argv, &error)) {
    g_printerr ("option parsing failed: %s\n",
        (error) ? error->message : "Unknown error");
    return EXIT_FAILURE;
  }
  g_option_context_free (context);

  priv = g_slice_new0 (GstWlDemo);

  priv->fullscreen = fullscreen;
  priv->nloop = nloop;
  priv->min_refresh = G_MAXINT;

  priv->loop = g_main_loop_new (NULL, FALSE);

  /* Construct a pipeline from the result of parsing argv
     similarly to gst-launch. */
  argvn = g_new0 (char *, argc);
  memcpy (argvn, argv + 1, sizeof (char *) * (argc - 1));
  error = NULL;
  priv->pipeline = gst_parse_launchv ((const gchar **) argvn, &error);
  g_free (argvn);

  if (!priv->pipeline) {
    g_printerr ("pipeline could not be constructed: %s\n",
        (error) ? error->message : "Unknown error");
    return EXIT_FAILURE;
  }

  priv->signal_watch_id =
      g_unix_signal_add (SIGINT, (GSourceFunc) sigint_handler, priv);

  bus = gst_pipeline_get_bus (GST_PIPELINE (priv->pipeline));
  gst_bus_set_sync_handler (bus, bus_sync_handler, priv, NULL);
  bus_watch_id = gst_bus_add_watch (bus, bus_async_handler, priv);
  gst_object_unref (bus);

  priv->display = wl_display_connect (NULL);
  if (!priv->display) {
    g_printerr ("display connection failed\n");
    goto leave;
  }

  priv->queue = wl_display_create_queue (priv->display);

  priv->registry = wl_display_get_registry (priv->display);
  wl_proxy_set_queue ((struct wl_proxy *) priv->registry, priv->queue);
  wl_registry_add_listener (priv->registry, &registry_listener, priv);

  /* Need 2 roundtrips to do all the global objects processing. */
  for (i = 0; i < 2; i++)
    wl_display_roundtrip_queue (priv->display, priv->queue);

  priv->source = g_source_new (&GstWaylandEventSourceFuncs,
      sizeof (GstWaylandEventSource));
  wl_source = (GstWaylandEventSource *) priv->source;

  wl_source->pfd.fd = wl_display_get_fd (priv->display);
  wl_source->pfd.events = G_IO_IN | G_IO_ERR | G_IO_HUP;
  g_source_add_poll (priv->source, &wl_source->pfd);

  wl_source->display = priv->display;
  wl_source->queue = priv->queue;
  wl_source->reading = FALSE;
  g_source_attach (priv->source, NULL);
  g_source_unref (priv->source);

  /* Setup the framerate adjustment by videorate. */
  it = gst_bin_iterate_elements (GST_BIN (priv->pipeline));
  gst_iterator_foreach (it, setup_framerate_adjustment, priv);
  gst_iterator_free (it);

  if (measuring_pad) {
    elem_name = strtok (measuring_pad, ":");
    pad_name = strtok (NULL, ":");
    if (!elem_name || !pad_name) {
      g_printerr ("tokens extraction failed\n");
      goto leave;
    }

    elem = gst_bin_get_by_name (GST_BIN (priv->pipeline), elem_name);
    if (!elem) {
      g_printerr ("failed to get the element by name\n");
      goto leave;
    }

    pad = gst_element_get_static_pad (elem, pad_name);
    if (!pad) {
      g_printerr ("failed to get the static pad by name\n");
      gst_object_unref (elem);
      goto leave;
    }

    gst_pad_add_probe (pad, GST_PAD_PROBE_TYPE_BUFFER,
        (GstPadProbeCallback) cb_have_data, priv, NULL);

    gst_object_unref (pad);
    gst_object_unref (elem);

    /* To calculate the average framerate throughout the playback. */
    timer = g_timer_new ();
  }

  state_ret = gst_element_set_state (priv->pipeline, GST_STATE_PAUSED);
  while (state_ret == GST_STATE_CHANGE_ASYNC) {
    state_ret = gst_element_get_state (priv->pipeline, NULL, NULL,
        GST_CLOCK_TIME_NONE);
  }

  if (timer)
    g_timer_start (timer);

  gst_element_set_state (priv->pipeline, GST_STATE_PLAYING);

  g_main_loop_run (priv->loop);

  if (timer) {
    g_timer_stop (timer);
    elapsed = g_timer_elapsed (timer, NULL);
    g_timer_destroy (timer);
    g_print ("Avg. FPS: %.2lf\n", priv->frame_cnt / elapsed);
  }

  gst_element_set_state (priv->pipeline, GST_STATE_NULL);

  ret = 0;

leave:
  if (priv->source)
    g_source_destroy (priv->source);

  gst_object_unref (priv->pipeline);

  if (priv->shell_surfaces) {
    g_list_foreach (priv->shell_surfaces, (GFunc) shell_surface_destroy, NULL);
    g_list_free (priv->shell_surfaces);
  }

  if (priv->surfaces) {
    g_list_foreach (priv->surfaces, (GFunc) surface_destroy, NULL);
    g_list_free (priv->surfaces);
  }

  if (priv->outputs) {
    g_list_foreach (priv->outputs, (GFunc) output_destroy, NULL);
    g_list_free (priv->outputs);
  }

  if (priv->shm)
    wl_shm_destroy (priv->shm);
  if (priv->shell)
    wl_shell_destroy (priv->shell);
  if (priv->registry)
    wl_registry_destroy (priv->registry);
  if (priv->compositor)
    wl_compositor_destroy (priv->compositor);
  if (priv->queue)
    wl_event_queue_destroy (priv->queue);
  if (priv->display)
    wl_display_disconnect (priv->display);

  g_source_remove (bus_watch_id);
  if (priv->signal_watch_id > 0)
    g_source_remove (priv->signal_watch_id);
  g_main_loop_unref (priv->loop);
  g_slice_free (GstWlDemo, priv);

  if (measuring_pad)
    g_free (measuring_pad);

  return ret;
}
