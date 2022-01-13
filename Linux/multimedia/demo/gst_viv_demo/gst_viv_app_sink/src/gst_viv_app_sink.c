/**
* \file: gst_viv_app_sink.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* appsink used for Vivante Texture rendering. Supports gst_pad_alloc
* and provides texture names to rendering application
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
*
***********************************************************************/
#include "gst_viv_app_sink.h"

#include <glib.h>
#include <string.h>
#include <stdio.h>


#ifndef PACKAGE
#define PACKAGE "gst_viv_app_sink"
#endif


enum
{
  SIGNAL_RENDER,
  LAST_SIGNAL
};


static guint gst_viv_app_sink_signals[LAST_SIGNAL] = { 0 };




static GstStaticPadTemplate sink_factory =
  GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS_ANY
  );


GST_DEBUG_CATEGORY_STATIC (gst_viv_app_sink_debug);
#define GST_CAT_DEFAULT gst_viv_app_sink_debug



GST_BOILERPLATE (GstVivAppSink,gst_viv_app_sink, GstVideoSink,
                GST_TYPE_VIDEO_SINK);


static void gst_viv_app_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_viv_app_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn
gst_viv_app_sink_change_state (GstElement *element, GstStateChange transition);

static GstFlowReturn gst_viv_app_sink_show_frame (GstVideoSink * vsink, GstBuffer * buf);

static gboolean gst_viv_app_sink_event(GstBaseSink * sink, GstEvent *event);


static void
gst_viv_app_sink_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "gst_viv_app_sink",
    "Sink/Video/Device",
    "gst_viv_app_sink",
    "Michael Methner mmethner@de.adit-jv.com");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));

}


static void
gst_viv_app_sink_class_init (GstVivAppSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *gstelement_class;
  GstBaseSinkClass *gstbasesink_class;
  GstVideoSinkClass *gstvideosink_class;

  gobject_class = (GObjectClass *) klass;
  gstelement_class = (GstElementClass *) klass;
  gstbasesink_class = (GstBaseSinkClass *) klass;
  gstvideosink_class = (GstVideoSinkClass *) klass;

  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_viv_app_sink_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_viv_app_sink_get_property);

  gstelement_class->change_state = GST_DEBUG_FUNCPTR (gst_viv_app_sink_change_state);

  gstbasesink_class->event = GST_DEBUG_FUNCPTR (gst_viv_app_sink_event);

  gstvideosink_class->show_frame = GST_DEBUG_FUNCPTR (gst_viv_app_sink_show_frame);

  gst_viv_app_sink_signals[SIGNAL_RENDER] =
       g_signal_new ("render",
           G_TYPE_FROM_CLASS (klass),
           G_SIGNAL_RUN_LAST,
           G_STRUCT_OFFSET (GstVivAppSinkClass, render),
           NULL, NULL,
           gst_marshal_VOID__OBJECT,
           G_TYPE_NONE, 1,
           GST_TYPE_PAD);
}





/**
 * \func gst_viv_app_sink_finalize_buffer
 *
 * This function is called whenever the reference count of a GstVivBuffer
 * drops to zero. The function increments the refcount again and pushes it
 * into the sink queue for reuse
 *
 * \param buffer Buffer to be recycled
 * \param element Pointer the decoder which has created the buffer
 *
 * \return True on success, false on failure
 *
 */
gboolean
gst_viv_app_sink_finalize_buffer(GstVivBuffer * buffer, GstElement * element)
{
  gboolean recycled = FALSE;
  GstVivAppSink * sink = GST_VIV_APP_SINK(element);

  GST_LOG("lock: buf: %p data: %p",buffer, GST_BUFFER(buffer)->data);
  g_mutex_lock(sink->pool_lock);
  sink->buffer_pool = g_slist_append(sink->buffer_pool, buffer);
  g_cond_signal(sink->pool_data);
  g_mutex_unlock(sink->pool_lock);
  GST_LOG("unlock");
  gst_buffer_ref(GST_BUFFER(buffer));
  recycled = TRUE;

  return recycled;
}

/**
 * \func gst_viv_app_sink_buffer_new
 *
 * Creates new vivbuffer.
 * Set the finalize fncptr of the buffer to returned to the sink
 * queue when unreferenced
 *
 * \param decoder "this" pointer
 *
 * \return True on success, false on failure
 *
 */
static GstVivBuffer *
gst_viv_app_buffer_new(GstVivAppSink *sink)
{
  GstVivBuffer *buffer;
  buffer = GST_VIV_BUFFER(gst_mini_object_new (GST_TYPE_VIV_BUFFER));

  buffer->parent = gst_object_ref(sink);
  buffer->finalize = &gst_viv_app_sink_finalize_buffer;

  GST_LOG("created new GstVivBuffer: %p", buffer);
  return buffer;
}


void gst_viv_app_add_buffer(GstVivAppSink *sink, void * ptr, GLuint texName, gint size)
{
  GstVivBuffer * buffer = gst_viv_app_buffer_new(sink);
  GST_BUFFER(buffer)->data= ptr;
  GST_BUFFER(buffer)->size = size;
  buffer->texName = texName;

  g_mutex_lock(sink->pool_lock);
  sink->buffer_pool = g_slist_append(sink->buffer_pool, buffer);
  g_cond_signal(sink->pool_data);
  g_mutex_unlock(sink->pool_lock);
}


GstFlowReturn gst_viv_app_sink_buffer_alloc_func(GstPad *pad,
                                                         guint64 offset,
                                                         guint size,
                                                         GstCaps *caps,
                                                         GstBuffer **buf)
{
  size = size;
  GstVivAppSink * sink = GST_VIV_APP_SINK(GST_PAD_PARENT(pad));
  GstVivBuffer * vivBuffer = NULL;

  g_mutex_lock(sink->pool_lock);
  while (!sink->buffer_pool)
    {
      GST_LOG("waiting for buffer ...");
      g_cond_wait(sink->pool_data, sink->pool_lock);
    }
  GST_LOG("got buffer");

  vivBuffer = (GstVivBuffer *) sink->buffer_pool->data;
  sink->buffer_pool = g_slist_delete_link(sink->buffer_pool,
      sink->buffer_pool);

  g_mutex_unlock(sink->pool_lock);
  *buf = GST_BUFFER(vivBuffer);
  (*buf)->offset = offset;
  (*buf)->caps = caps;
  return GST_FLOW_OK;
}


static void
gst_viv_app_sink_init (GstVivAppSink * videosink,
    GstVivAppSinkClass * gclass)
{
  gclass = gclass;
  gst_pad_set_bufferalloc_function (GST_BASE_SINK_PAD(videosink),&gst_viv_app_sink_buffer_alloc_func);

  videosink->pool_lock = g_mutex_new();
  videosink->pool_data = g_cond_new();
  videosink->buffer_pool = NULL;

  g_object_set(G_OBJECT(videosink), "max-lateness", (gint64)-1, NULL );

  videosink->buffer = NULL;
}



static void
gst_viv_app_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  value = value;
  switch (prop_id) {
   
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_viv_app_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  value = value;
  switch (prop_id) {
    default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
          break;
  }
}


static GstStateChangeReturn
gst_viv_app_sink_change_state(GstElement *element, GstStateChange transition)
{
  GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
  GST_LOG("switch state: transition: %d current_state: %d", transition,element->current_state);

  switch (transition)
  {
    case GST_STATE_CHANGE_NULL_TO_READY:
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
    return ret;
  }

  switch (transition)
  {
    case GST_STATE_CHANGE_READY_TO_NULL:
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


static gboolean
gst_viv_app_sink_event(GstBaseSink * sink, GstEvent *event)
{
  sink = sink;
  GST_LOG("received: %s", GST_EVENT_TYPE_NAME(event));

  return TRUE;
}



GLuint gst_viv_app_get_current_texname(GstVivAppSink * sink)
{
  GLuint texname = 0;
  if(sink->buffer)
    {
      texname = sink->buffer->texName;
    }
  return texname;
}


static GstFlowReturn
gst_viv_app_sink_show_frame(GstVideoSink * vsink, GstBuffer * buf)
{
  GST_LOG("start: buf: %p data: %p size:%d timestamp: %" GST_TIME_FORMAT " duration: %" GST_TIME_FORMAT "",
          buf, buf->data, buf->size, GST_TIME_ARGS(buf->timestamp), GST_TIME_ARGS(buf->duration));

  GST_ERROR("%s", gst_caps_to_string (buf->caps));
  GstVivAppSink * sink = GST_VIV_APP_SINK(vsink);

  if(sink->buffer)
    {
      gst_buffer_unref(GST_BUFFER(sink->buffer));
    }

  sink->buffer = GST_VIV_BUFFER(buf);
  gst_buffer_ref(GST_BUFFER(sink->buffer));

  g_signal_emit(vsink,
      gst_viv_app_sink_signals[SIGNAL_RENDER],
      0,
      GST_BASE_SINK_PAD(vsink));

  return GST_FLOW_OK;
}


static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_viv_app_sink_debug, "gst_viv_app_sink",
      0, "Adit SVG Display");

  return gst_element_register (plugin, "gst_viv_app_sink", GST_RANK_NONE,
      GST_TYPE_VIV_APP_SINK);
 }


GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "gst_viv_app_sink",
    "GST Vivante App sink",
    plugin_init,
    "0.0.1",
    "Proprietary",
    "ADIT",
    "http://TBD/"
)


