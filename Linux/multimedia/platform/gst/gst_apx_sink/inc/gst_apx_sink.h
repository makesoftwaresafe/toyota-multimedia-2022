/**
* \file: gst_apx_sink.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* Render video images using wayland, GLESv2 and Vivante Extensions
* for mapping YUV data.
*
* \component: multimedia/gst
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2015 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
* 0.2 Gautham Kantharaju Extended to support mfw_v4lsrc source plugin
* 0.3 Emre Ucan Modified to use compositor-shim
*
***********************************************************************/

#ifndef __gst_apx_sink_H__
#define __gst_apx_sink_H__

#include <gst/video/gstvideosink.h>

#include <wayland-client.h>

#include <compositor-shim.h>

#include <apx.h>

G_BEGIN_DECLS


#define GST_TYPE_APX_SINK \
  (gst_apx_sink_get_type())
#define GST_APX_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_APX_SINK,GstApxSink))
#define GST_APX_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_APX_SINK,GstApxSinkClass))
#define GST_IS_APX_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_APX_SINK))
#define GST_IS_APX_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_APX_SINK))


typedef struct _GstApxSink GstApxSink;
typedef struct _GstApxSinkClass GstApxSinkClass;

#define MAX_BUFFER 20

struct _GstApxSink
{
  GstVideoSink videosink;
  gint video_width;
  gint video_height;
  float pixel_aspect_ratio;
  GstVideoFormat pixel_format;
  guint32 strides[3];

  guint display_width;
  guint display_height;
  gint x_offset;
  gint y_offset;
  guint crop_left;
  guint crop_top;
  guint crop_right;
  guint crop_bottom;
  guint surface_id;
  guint layer_id;
  gboolean force_memcpy;
  gboolean force_aspect_ratio;

  gboolean is_firstframe;

  /* wayland internals */
  struct wl_display* wl_display;

  struct wl_compositor* wl_compositor;
  struct wl_registry* wl_registry;
  struct wl_surface* wl_surface;

  /* Compositor Shim data structures */
  struct compositor_shim_context* shim_context;
  struct compositor_shim_surface_context shim_surface_context;


	/* APX */
  struct apx apx_a;
  apxPixelFormat apx_format;
  gboolean apx_init;

  GHashTable *buffers;
  GstBuffer *preroll;
  gboolean compositor_shim_wait_server;
};


struct _GstApxSinkClass
{
  GstVideoSink parent_class;
  void (*first_videoframe_rendered) (GstElement *element, GstPad *pad);

};


GType gst_apx_sink_get_type(void);


G_END_DECLS

#endif /* __gst_apx_sink_H__ */
