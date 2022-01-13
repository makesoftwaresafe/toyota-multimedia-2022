/**
* \file: gstxsubsink.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* DivX XSUB subtitle sink.
* *
* \component: gst_xsubdec
*
* \author: Jens Georg ADITG/SWG jgeorg@de.adit-jv.com
*
* \copyright: (c) 2013 ADIT Corporation
*
* \history
* 0.1 Jens Georg Initial version
*
***********************************************************************/

#include <gst/gst.h>

#include "gstxsubsink.h"
#include "xsub.h"

/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

GST_DEBUG_CATEGORY_STATIC (gst_xsubsink_debug);
#define GST_CAT_DEFAULT gst_xsubsink_debug

/* PRQA: Lint Message 19, 123, 144, 751: deactivation because macro is related to GStreamer framework */
/*lint -e19 -e123 -e144 -e751 */
GST_BOILERPLATE(GstXSUBSink, gst_xsubsink, GstBin, GST_TYPE_BIN);
/*lint +e19 +e123 +e144 +e751 */

#define GST_XSUBSINK_DEFAULT_LAYER 3001
#define GST_XSUBSINK_DEFAULT_SURFACE 41

enum
{
  PROP_LAYER_ID = 1,
  PROP_SURFACE_ID
};

static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-avi-unknown, fourcc= {(fourcc)DXSB, (fourcc)DXSA }; "
                     "subtitle/x-xsub, has-alpha={TRUE, FALSE}, width=[1,max], height=[1,max]")
);

static void
gst_xsubsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void
gst_xsubsink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static GstStateChangeReturn
gst_xsubsink_change_state (GstElement *element, GstStateChange transition);

static void
gst_xsubsink_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
      "XSUB display sink",
      "Sink/Video",
      "DivX XSUB and XSUB+ subtitle sink",
      "Jens Georg <jgeorg@adit-jv.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_factory));
}

static void
gst_xsubsink_class_init (GstXSUBSinkClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_xsubsink_debug, "xsubsink", 0,
      "XSUB DivX subtitle sink");

  gobject_class->set_property = gst_xsubsink_set_property;
  gobject_class->get_property = gst_xsubsink_get_property;

  gstelement_class->change_state = GST_DEBUG_FUNCPTR(gst_xsubsink_change_state);

  g_object_class_install_property (gobject_class,
      PROP_LAYER_ID,
      g_param_spec_uint ("layer-id",
                         "layer id",
                         "Id of LayerManager layer to use",
                         0,
                         G_MAXUINT,
                         GST_XSUBSINK_DEFAULT_LAYER,
                         G_PARAM_CONSTRUCT |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class,
      PROP_SURFACE_ID,
      g_param_spec_uint ("surface-id",
                         "surface id",
                         "Id of LayerManager surface to use",
                         0,
                         G_MAXUINT,
                         GST_XSUBSINK_DEFAULT_SURFACE,
                         G_PARAM_CONSTRUCT |
                         G_PARAM_READWRITE |
                         G_PARAM_STATIC_STRINGS));
}

static void
gst_xsubsink_init (GstXSUBSink *sink, GstXSUBSinkClass *gclass)
{
  GstPad *sinkpad;
  GstPad *ghost;

  __ADIT_UNUSED (gclass);

  GST_DEBUG_OBJECT (sink, "XSUB subtitle sink created");

  sink->decode = gst_element_factory_make ("xsubdecode", "decode");

#ifndef HOST_COMPILE
  sink->sink = gst_element_factory_make ("gst_wayland_gles_sink", "sink");
  gst_bin_add_many (GST_BIN(sink), sink->decode, sink->sink, NULL);
  gst_element_link (sink->decode, sink->sink);
#else
  {
    GstElement *csp;

    csp = gst_element_factory_make ("ffmpegcolorspace", "colorspace");
    sink->sink = gst_element_factory_make ("autovideosink", "sink");
    gst_bin_add_many (GST_BIN (sink), sink->decode, csp, sink->sink, NULL);
    gst_element_link_many (sink->decode, csp, sink->sink, NULL);
  }
#endif


  /* Add ghostpad to bin */
  sinkpad = gst_element_get_static_pad (sink->decode, "sink");
  ghost = gst_ghost_pad_new ("sink", sinkpad);
  gst_pad_set_active (ghost, TRUE);
  gst_element_add_pad (GST_ELEMENT(sink), ghost);
  gst_object_unref (sinkpad);
}

static void
gst_xsubsink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstXSUBSink *sink = GST_XSUBSINK (object);

  switch (prop_id)
  {
    case PROP_LAYER_ID:
      sink->layer_id = g_value_get_uint(value);
      break;
    case PROP_SURFACE_ID:
      sink->surface_id = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_xsubsink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstXSUBSink *sink = GST_XSUBSINK (object);

  switch (prop_id)
  {
    case PROP_LAYER_ID:
      g_value_set_uint(value, sink->layer_id);
      break;
    case PROP_SURFACE_ID:
      g_value_set_uint(value, sink->surface_id);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static GstStateChangeReturn
gst_xsubsink_change_state (GstElement *element, GstStateChange transition)
{
  GstXSUBSink *sink = GST_XSUBSINK (element);

#ifdef HOST_COMPILE
  __ADIT_UNUSED(sink);
#endif

  switch (transition)
  {
    case GST_STATE_CHANGE_NULL_TO_READY:
#ifndef HOST_COMPILE
      /* Forward properties to Wayland sink which is setting up its
       * layer manager connection in this state change.
       */
      g_object_set(sink->sink,
          "layer-id", sink->layer_id,
          "surface-id", sink->surface_id, NULL);
#else
      /* Do nothing, prevent the ugly warning that these properties
       * don't exist on host.
       */
#endif
      break;
    default:
      break;
  }

  return GST_ELEMENT_CLASS(parent_class)->change_state (element, transition);
}
/*lint -e826 -e160*/
