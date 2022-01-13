/**
* \file: gst_apx_sink.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* Render video images using wayland, GLESv2 and Vivante Extensions
* for mapping YUV data.
* *
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
* 0.2 Gautham Kantharaju Extended to support mfw_v4lsrc source plugin
*
***********************************************************************/

#include "gstapx.h"
#include "gst_apx_sink.h"
#include "gst_apx_buffer.h"
#include "gst_apx_render_util.h"
#include "gst_apx_sink_wl_if.h"

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/poll.h>

#include <glib.h>

#include <gst/video/video.h>

#include <apx.h>

GST_DEBUG_CATEGORY (gst_apx_sink_debug);

/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

#define DEFAULT_DISPLAY_WIDTH 1024
#define DEFAULT_DISPLAY_HEIGHT 768
#define DEFAULT_LAYER_ID 3000
#define DEFAULT_SURFACE_ID 40

#define RGB_SUBPICTURE_FORMAT "subpicture/x-raw-rgb"

enum
{
  SIGNAL_FIRST_VIDEOFRAME_RENDERED,
  LAST_SIGNAL
};

static guint gst_apx_sink_signals[LAST_SIGNAL] = { 0 };


enum
{
  PROP_DISPLAY_WIDTH = 1,
  PROP_DISPLAY_HEIGHT,
  PROP_X_OFFSET,
  PROP_Y_OFFSET,
  PROP_LAYER_ID,
  PROP_SURFACE_ID,
  PROP_FORCE_MEMCPY,
  PROP_FORCE_ASPECT_RATIO,
  PROP_WAYLAND_DISPLAY
};

gboolean
gst_apx_sink_setcaps(GstPad *pad, GstCaps *caps);

#define MAX_ERRMEM_MESSAGE_LENGTH 256

void __apx_log_errmem(const char *message, ...)
{
  va_list args;
  int ret = 0;
  int fd = -1;
  char *msg = NULL;
  const char *process = NULL;

  process = g_get_prgname();

  /* Only log for procvideo */
  if (!strstr(process, "procvideo_out"))
      return;

  fd = open("/dev/errmem", O_WRONLY);
  if (fd >= 0)
  {
    msg = (char *)calloc(MAX_ERRMEM_MESSAGE_LENGTH, sizeof(char));
    if (msg)
    {
/* PRQA: Lint Message 530: va_list args gets initialized via va_start */
/*lint -save -e530*/
      va_start(args, message);
      ret = vsnprintf(msg, MAX_ERRMEM_MESSAGE_LENGTH, message, args);
      va_end(args);
/*lint -restore*/

      if (ret > 0 && ret < MAX_ERRMEM_MESSAGE_LENGTH)
      {
        write(fd, msg, sizeof(char) * (ret + 1));
      }
      else if (ret > -1)
      {
        write(fd, msg, MAX_ERRMEM_MESSAGE_LENGTH);
      }
      else
      {
        const char errmsg[] = "APX_SINK: Failed to vsnprintf log message\n";
        write(fd, errmsg, sizeof(errmsg));
      }
      free(msg);
    }
    else
    {
      const char errmsg[] = "APX_SINK: Failed to allocate memory for message\n";
      write(fd, errmsg, sizeof(errmsg));
    }
    close(fd);
  }
}


/* We support the artificial type "subpicture/x-raw-rgb" to fool playbin2
 * into accepting us as a text-sink */
static GstStaticPadTemplate sink_factory =
  GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        GST_VIDEO_CAPS_YUV ("{ YV12, YUY2, YUYV, UYVY, NV12 }") ";"
        GST_VIDEO_CAPS_BGRA ";"
        GST_VIDEO_CAPS_RGB_16 ";"
        RGB_SUBPICTURE_FORMAT
        )
  );



/* PRQA: Lint Message 19, 123, 144, 751: deactivation because macro is related to GStreamer framework */
/*lint -e19 -e123 -e144 -e751 */
GST_BOILERPLATE (GstApxSink,gst_apx_sink, GstVideoSink,
                GST_TYPE_VIDEO_SINK);
/*lint +e19 +e123 +e144 +e751 */

static void gst_apx_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_apx_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn
gst_apx_sink_change_state (GstElement *element, GstStateChange transition);

static GstFlowReturn gst_apx_sink_show_frame (GstVideoSink * vsink, GstBuffer * buf);
static GstFlowReturn gst_apx_sink_preroll(GstBaseSink *sink, GstBuffer *buf);

static GstFlowReturn gst_apx_sink_buffer_alloc(GstBaseSink *pad,
    guint64 offset,
    guint size,
    GstCaps *caps,
    GstBuffer **buf);


static void
gst_apx_sink_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "gst_apx_sink",
    "Sink/Video/Device",
    "gst_apx_sink",
    "Jens Georg <jgeorg@de.adit-jv.com> et al.");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

}

static void
gst_apx_sink_finalize (GObject *object)
{
    gst_apx_sink_wl_if_close (GST_APX_SINK (object));
    g_hash_table_unref (GST_APX_SINK(object)->buffers);
}

static void
gst_apx_sink_class_init (GstApxSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *gstvideosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstvideosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_apx_sink_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_apx_sink_get_property);
  gobject_class->finalize = GST_DEBUG_FUNCPTR (gst_apx_sink_finalize);

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_apx_sink_change_state);

  gstbasesink_class->buffer_alloc = GST_DEBUG_FUNCPTR(gst_apx_sink_buffer_alloc);
  gstbasesink_class->preroll = GST_DEBUG_FUNCPTR(gst_apx_sink_preroll);

  gstvideosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_apx_sink_show_frame);

  g_object_class_install_property (gobject_class, PROP_DISPLAY_WIDTH,
         g_param_spec_uint ("display-width", "display-width", "Display width in pixel",
             0,4096,DEFAULT_DISPLAY_WIDTH,
             G_PARAM_READWRITE |
             G_PARAM_CONSTRUCT |
             G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_DISPLAY_HEIGHT,
         g_param_spec_uint ("display-height", "display-height", "Display height in pixel",
             0,4096,DEFAULT_DISPLAY_HEIGHT,
             G_PARAM_READWRITE |
             G_PARAM_CONSTRUCT |
             G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (gobject_class, PROP_LAYER_ID,
         g_param_spec_uint ("layer-id", "layer-id", "layer-id",
             0,G_MAXUINT,DEFAULT_LAYER_ID,
             G_PARAM_READWRITE |
             G_PARAM_CONSTRUCT |
             G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (gobject_class, PROP_SURFACE_ID,
         g_param_spec_uint ("surface-id", "surface-id", "surface-id",
             0,G_MAXUINT,DEFAULT_SURFACE_ID,
             G_PARAM_READWRITE |
             G_PARAM_CONSTRUCT |
             G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (gobject_class, PROP_FORCE_MEMCPY,
         g_param_spec_boolean ("force-memcpy", "force-memcpy", "force-memcpy",
             FALSE,
             G_PARAM_READWRITE |
             G_PARAM_CONSTRUCT |
             G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (gobject_class, PROP_X_OFFSET,
       g_param_spec_int ("x-offset", "x-offset", "x-offset",
           -4096, 4096, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

   g_object_class_install_property (gobject_class, PROP_Y_OFFSET,
       g_param_spec_int ("y-offset", "y-offset", "y-offset",
           -4096, 4096, 0, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

   g_object_class_install_property (gobject_class, PROP_FORCE_ASPECT_RATIO,
       g_param_spec_boolean ("force-aspect-ratio", "force-aspect-ratio",
                             "force-aspect-ratio", FALSE,
                             G_PARAM_READWRITE | G_PARAM_CONSTRUCT |
                             G_PARAM_STATIC_STRINGS));

   g_object_class_install_property (gobject_class, PROP_WAYLAND_DISPLAY,
       g_param_spec_pointer ("wl-display", "wl-display", "wl-display",
                             G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

   gst_apx_sink_signals[SIGNAL_FIRST_VIDEOFRAME_RENDERED] =
        g_signal_new ("first-videoframe-rendered",
            G_TYPE_FROM_CLASS (klass),
            G_SIGNAL_RUN_LAST,
 /* PRQA: Lint Message 413: deactivation because struct offset mechanism of GObject throws the finding */
 /*lint  -e413*/
            G_STRUCT_OFFSET (GstApxSinkClass, first_videoframe_rendered),
/*lint  +e413*/
            NULL, NULL,
            gst_marshal_VOID__OBJECT,
            G_TYPE_NONE, 1,
            GST_TYPE_PAD);
}

/* Need this for GDestroyNotify as apx_buffer_destroy takes two parameters */
static void
gst_apx_buf_free (struct apx_buffer *buf)
{
    apx_buffer_destroy (buf, NULL);
}

static void
gst_apx_sink_init (GstApxSink * videosink,
    GstApxSinkClass * gclass)
{
  __ADIT_UNUSED (gclass);

  g_object_set(G_OBJECT(videosink), "max-lateness", (gint64)-1, NULL );
  videosink->video_width=0;
  videosink->video_height=0;
  videosink->pixel_aspect_ratio = 1;
  videosink->force_aspect_ratio = FALSE;

  gst_pad_set_setcaps_function(GST_BASE_SINK_PAD(videosink), GST_DEBUG_FUNCPTR(
      gst_apx_sink_setcaps));

  videosink->is_firstframe = TRUE;
  videosink->buffers = g_hash_table_new_full (g_direct_hash,
                                              g_direct_equal,
                                              NULL,
                                              (GDestroyNotify) gst_apx_buf_free);

  videosink->compositor_shim_wait_server = FALSE;
  videosink->shim_context = NULL;
  videosink->apx_init = FALSE;
  gst_apx_sink_wl_if_init(videosink);
}

static void
gst_apx_sink_set_property(GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstApxSink * sink = GST_APX_SINK(object);
  switch (prop_id)
    {
  case PROP_DISPLAY_WIDTH:
    sink->display_width = g_value_get_uint(value);
    break;
  case PROP_DISPLAY_HEIGHT:
    sink->display_height = g_value_get_uint(value);
    break;
  case PROP_X_OFFSET:
    sink->x_offset = g_value_get_int(value);
    break;
  case PROP_Y_OFFSET:
    sink->y_offset = g_value_get_int(value);
    break;
  case PROP_SURFACE_ID:
    sink->surface_id = g_value_get_uint(value);
    break;
  case PROP_LAYER_ID:
    sink->layer_id = g_value_get_uint(value);
    break;
  case PROP_FORCE_MEMCPY:
    sink->force_memcpy = g_value_get_boolean(value);
    break;
  case PROP_FORCE_ASPECT_RATIO:
    sink->force_aspect_ratio = g_value_get_boolean(value);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
    }
}


static void
gst_apx_sink_get_property(GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstApxSink * sink = GST_APX_SINK(object);
  switch (prop_id)
    {
  case PROP_DISPLAY_WIDTH:
    g_value_set_uint(value,sink->display_width);
    break;
  case PROP_DISPLAY_HEIGHT:
    g_value_set_uint(value,sink->display_height);
    break;
  case PROP_X_OFFSET:
    g_value_set_int(value, sink->x_offset);
    break;
  case PROP_Y_OFFSET:
    g_value_set_int(value, sink->y_offset);
    break;
  case PROP_SURFACE_ID:
    g_value_set_uint(value,sink->surface_id);
    break;
  case PROP_LAYER_ID:
    g_value_set_uint(value,sink->layer_id);
    break;
  case PROP_FORCE_MEMCPY:
    g_value_set_boolean(value, sink->force_memcpy);
    break;
  case PROP_FORCE_ASPECT_RATIO:
    g_value_set_boolean(value, sink->force_aspect_ratio);
    break;
  case PROP_WAYLAND_DISPLAY:
    g_value_set_pointer(value, sink->wl_display);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
    break;
    }
}

gboolean
gst_apx_sink_draw(GstApxSink * sink, GstBuffer *buffer, gboolean lock)
{
    if (sink->is_firstframe && !gst_apx_sink_wl_if_create_apx_context(sink)) {
        GST_ERROR("createAPXContext failed\");");
        __apx_log_errmem("APX_SINK: Failed to create XP context\n");
        return FALSE;
    }

    return gst_apx_render_buffer(sink, buffer, lock);
}

static GstStateChangeReturn
gst_apx_sink_change_state(GstElement *element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    GstApxSink * sink = GST_APX_SINK(element);

    GST_LOG("switch state: transition: %d current_state: %d", transition,element->current_state);

    switch (transition)
    {
        case GST_STATE_CHANGE_NULL_TO_READY:
            if (sink->shim_context == NULL)
            {
                GST_ERROR("gst_apx_sink_wl_init failed");

                return GST_STATE_CHANGE_FAILURE;
            }
            sink->is_firstframe = TRUE;
            break;
        case GST_STATE_CHANGE_READY_TO_PAUSED:
            break;
        case GST_STATE_CHANGE_PAUSED_TO_PLAYING:
            break;
        default:
            break;
  }

  ret = GST_ELEMENT_CLASS (parent_class)->change_state(element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
  {
    GST_ERROR("state change failure");
    __apx_log_errmem("APX_SINK: State change from parent class failed\n");
    return ret;
  }

  switch (transition)
  {
    case GST_STATE_CHANGE_READY_TO_NULL:
      {
        gst_apx_sink_wl_if_stop(sink);
        /* Clear buffer cache on state change, we will most likely receive
           new hardware pointers */
        g_hash_table_remove_all (sink->buffers);
      }
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      break;
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    default:
      break;
  }
  return ret;
}

gboolean
gst_apx_sink_setcaps(GstPad *pad, GstCaps *caps)
{
  gboolean retval = TRUE;
  gint par_n;
  gint par_d;
  GstStructure *s;
  GstElement * element = GST_PAD_PARENT(pad);
  GstApxSink * sink = GST_APX_SINK(element);
  GstVideoFormat format;
  gint crop_top;
  gint crop_bottom;
  gint crop_left;
  gint crop_right;
  gint i;

  sink->apx_format = APX_PIXEL_FORMAT_UNKNOWN;
  s = gst_caps_get_structure(caps, 0);
  gst_video_format_parse_caps (caps, &format, &sink->video_width, &sink->video_height);
  if (format != GST_VIDEO_FORMAT_UNKNOWN)
  {
    sink->apx_format = gst_to_apx(format);
    sink->pixel_format = format;
  }
  else
  {
    gboolean has_fourcc = FALSE;
    guint32 fourcc;

    has_fourcc = gst_structure_get_fourcc(s, "format", &fourcc);
    if (g_strcmp0(gst_structure_get_name(s), RGB_SUBPICTURE_FORMAT) == 0)
    {
      sink->apx_format = APX_PIXELFORMAT_RGBA_8888;
    }
    else if (has_fourcc && fourcc == GST_MAKE_FOURCC('Y','U','Y','V'))
    {
      /* gst_video_format_parse_caps does not handle this YUY2 alias */
      sink->apx_format = APX_PIXELFORMAT_YUY2;
    }
    sink->pixel_format = apx_to_gst(sink->apx_format);
  }

  if (sink->apx_format == APX_PIXEL_FORMAT_UNKNOWN)
  {
    GST_ERROR_OBJECT (sink, "Could not find compatible color format for caps %"GST_PTR_FORMAT, caps);
        retval = FALSE;
  }

  sink->video_height = sink->video_height - sink->video_height % 4;

  /* Let's see whether we get some cropping */
  if (gst_structure_get_int(s, "crop-top", &crop_top))
  {
    sink->crop_top = crop_top;
  }

  if (gst_structure_get_int(s, "crop-bottom", &crop_bottom))
  {
    sink->crop_bottom = crop_bottom;
  }

  if (gst_structure_get_int(s, "crop-left", &crop_left))
  {
    sink->crop_left = crop_left;
  }

  if (gst_structure_get_int(s, "crop-right", &crop_right))
  {
    sink->crop_right = crop_right;
  }


  if(gst_structure_get_fraction(s, "pixel-aspect-ratio",
        &par_n,
        &par_d))
  {
      sink->pixel_aspect_ratio = (float)par_n / (float)par_d;
  }
  else
  {
      sink->pixel_aspect_ratio = 1.0f;
  }

  if (sink->force_aspect_ratio)
  {
      float target_aspect = 0.0f;
      float src_aspect = 0.0f;

      src_aspect = (sink->video_width * 1.0f) / sink->video_height;
      target_aspect = (sink->display_width * 1.0f) / sink->display_height;

      GST_TRACE_OBJECT(sink, "PAR: %f", sink->pixel_aspect_ratio);

      GST_TRACE_OBJECT(sink, "dimensions: src %dx%d, dest: %dx%d, offsets: %u %u",
                       sink->video_width, sink->video_height,
                       sink->display_width, sink->display_height,
                       sink->x_offset, sink->y_offset);

      GST_TRACE_OBJECT(sink, "src aspect: %f, target aspect: %f",
                         src_aspect, target_aspect);

      if (src_aspect < target_aspect)
      {
          guint new_width = (guint)(((float) sink->display_height) * src_aspect);
          sink->x_offset += (sink->display_width - new_width) / 2;
          /* keep the height, adjust the width */
          sink->display_width = new_width;
      }
      else
      {
          gint new_height = (gint)(((float) sink->display_width) / src_aspect);
          /* keep the width, adjust the height */
          sink->y_offset += (sink->display_height - new_height) / 2;
          sink->display_height = new_height;
      }

      GST_TRACE_OBJECT(sink, "after correction: %d x %d",
                         sink->display_width, sink->display_height);
  }

  /* For max. 3 planes we get the row stride values. For formats with less
     then 3 planes, the values are identical to the last valid planes; we only
     use the valid stride in the mapping later on anyway. */
  for (i = 0; i < 3; i++)
  {
      sink->strides[i] = gst_video_format_get_row_stride(sink->pixel_format,
                                                  i, sink->video_width);
  }

  return retval;
}

static void
wayland_sync_callback(void *data, struct wl_callback *callback, uint32_t serial)
{
	int *done = data;
	(void)serial;

	*done = 1;
	wl_callback_destroy(callback);
}

static const struct wl_callback_listener wayland_sync_callback_listener = {
	wayland_sync_callback
};

static int wait_for_wayland_sync(GstApxSink *sink, int *done, struct wl_callback* sync_callback)
{
	struct wl_display *display;
	struct pollfd pfd[1];
	int ret = 0;

	if(!sink || !done)
		return -1;

	display =  sink->wl_display;
	if(!display)
		return -1;

	pfd[0].fd = wl_display_get_fd(display);
	pfd[0].events = POLLIN;

	while (1) {
		/* Mark this thread as a reading thread */
		while (wl_display_prepare_read(display) < 0) {
			if (wl_display_dispatch_pending(display) < 0) {
				GST_ERROR("wl_display_dispatch_pending failed");
				goto err;
			}
		}

		if (*done) {
			GST_INFO("done event was already dispatched");
			wl_display_cancel_read(display);
			break;
		}

		/* Flush events before going to blocking wait */
		if (wl_display_flush(display) < 0) {
			GST_ERROR("wl_display_flush failed\n");
		    wl_display_cancel_read(display);
		    goto err;
		}

		/* Check for Wayland event (not necessarily input event) */
		ret = poll(pfd, 1, -1);
		if(ret <= 0) {
			/* cancel in read if no event is pending */
			wl_display_cancel_read(display);
		} else {
			if (wl_display_read_events(display) < 0 ) {
				GST_ERROR("wl_display_read_events failed");
		        goto err;
			}

		    if (wl_display_dispatch_pending(display) < 0) {
		        GST_ERROR("wl_display_dispatch_pending failed");
		        goto err;
		    }

		    if (*done) {
		        GST_INFO("got done event after read events");
		        break;
		    }
		}
	}

	return 0;

err:
	if (sync_callback) {
		wl_callback_destroy(sync_callback);
	}

	return -1;
}

static GstFlowReturn
gst_apx_sink_show_frame(GstVideoSink * vsink, GstBuffer * buf)
{
  GstApxSink * sink = GST_APX_SINK(vsink);
  gboolean preroll = buf == sink->preroll;

  /* Buffer has been already rendered for some reason */
  if (GST_IS_APX_BUFFER(buf) && !GST_APX_BUFFER_NATIVE(buf))
    return GST_FLOW_OK;

  GST_LOG("start: buf: %p data: %p size:%d timestamp: %" GST_TIME_FORMAT " duration: %" GST_TIME_FORMAT,
          buf, buf->data, buf->size, GST_TIME_ARGS(buf->timestamp), GST_TIME_ARGS(buf->duration));

  if (!gst_apx_sink_draw(sink, buf, !preroll))
    return GST_FLOW_ERROR;

  if(sink->is_firstframe)
  {
	struct wl_callback* sync_callback = NULL;
	int ret = 0;
	int done = 0;

	sync_callback = wl_display_sync(sink->wl_display);
	if (sync_callback == NULL) {
		GST_ERROR("wl_display_sync failed!!!");
	}
	else{
		wl_callback_add_listener(sync_callback, &wayland_sync_callback_listener, &done);
		ret = wait_for_wayland_sync(sink, &done, sync_callback);
		if (-1 == ret) {
			GST_ERROR("wait_for_wayland_sync failed!!!");
		}
	}

    sink->is_firstframe = FALSE;
    g_signal_emit(sink,
        gst_apx_sink_signals[SIGNAL_FIRST_VIDEOFRAME_RENDERED],
        0,
        GST_BASE_SINK_PAD(sink));
  }

  return GST_FLOW_OK;
}

static GstFlowReturn gst_apx_sink_buffer_alloc(GstBaseSink *sink,
    guint64 offset,
    guint size,
    GstCaps *caps,
    GstBuffer **buf)
{
  GstBuffer *tmp = NULL;
  GstApxSink *self = GST_APX_SINK(sink);

  if (self->shim_context == NULL)
  {
      GST_ERROR("Element not connected to wayland/LayerManager, cannot allocate buffer");
      return GST_FLOW_ERROR;
  }

  if (G_LIKELY(self->force_memcpy == FALSE))
  {
    tmp = gst_apx_buffer_new (&self->apx_a, caps);
  }

  /* Apparently something is odd; let's use a malloc buffer then. */
  if (tmp != NULL && GST_BUFFER_SIZE(tmp) != size)
  {
    GST_LOG_OBJECT(sink, "Size mismatch: Requested %u, APX delivered %u",
        size, GST_BUFFER_SIZE(tmp));
    gst_buffer_unref(tmp);
    tmp = NULL;
  }

  if (tmp == NULL)
  {
    GST_LOG_OBJECT(sink, "Could not allocate proper hardware buffer, delegating to parent class");
    /* Could not allocate proper hardware buffer, delegate to parent class */
    GST_BASE_SINK_CLASS(parent_class)->buffer_alloc(sink, offset, size, caps, buf);
  }
  else
  {
    GST_BUFFER_OFFSET(tmp) = offset;
    *buf = tmp;
  }

  return GST_FLOW_OK;
}

static GstFlowReturn gst_apx_sink_preroll(GstBaseSink *sink, GstBuffer *buffer)
{
    GstApxSink *self = GST_APX_SINK(sink);
    self->preroll = buffer;

    GstFlowReturn ret = GST_BASE_SINK_CLASS(parent_class)->preroll(sink, buffer);

    self->preroll = NULL;

    return ret;
}

apxPixelFormat gst_to_apx(GstVideoFormat format)
{
  switch (format)
  {
    case GST_VIDEO_FORMAT_BGRA:
      return APX_PIXELFORMAT_RGBA_8888;
    case GST_VIDEO_FORMAT_RGB16:
      return APX_PIXELFORMAT_RGB_565;
    case GST_VIDEO_FORMAT_YV12:
      return APX_PIXELFORMAT_YV12;
    case GST_VIDEO_FORMAT_NV12:
      return APX_PIXELFORMAT_NV12;
    case GST_VIDEO_FORMAT_YUY2:
      return APX_PIXELFORMAT_YUY2;
    case GST_VIDEO_FORMAT_UYVY:
      return APX_PIXELFORMAT_UYVY;
    default:
      return APX_PIXEL_FORMAT_UNKNOWN;
  }
}

GstVideoFormat apx_to_gst(apxPixelFormat format)
{
  switch (format)
  {
    case APX_PIXELFORMAT_RGBA_8888:
      return GST_VIDEO_FORMAT_RGBA;
    case APX_PIXELFORMAT_RGB_565:
      return GST_VIDEO_FORMAT_RGB16;
    case APX_PIXELFORMAT_YV12:
      return GST_VIDEO_FORMAT_YV12;
    case APX_PIXELFORMAT_NV12:
      return GST_VIDEO_FORMAT_NV12;
    case APX_PIXELFORMAT_YUY2:
      return GST_VIDEO_FORMAT_YUY2;
    case APX_PIXELFORMAT_UYVY:
      return GST_VIDEO_FORMAT_UYVY;
    default:
      return GST_VIDEO_FORMAT_UNKNOWN;
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_apx_sink_debug, "gst_apx_sink",
      0, "Adit APX Video display");

  return gst_element_register (plugin, "gst_apx_sink", GST_RANK_PRIMARY,
      GST_TYPE_APX_SINK);
 }


GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gst_apx_sink",
    "GST APX sink",
    plugin_init,
    "0.0.1",
    "Proprietary",
    "ADIT",
    "http://TBD/"
)



/*lint +e826 +e160*/
