/**
* \file: gstxsubparse.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* DivX XSUB subtitle decoder
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

#include <ctype.h>
#include <string.h>

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>
#include <gst/video/video.h>

#include "gstxsubparse.h"

#include "xsub.h"

/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

static GstStaticPadTemplate sink_template =
    GST_STATIC_PAD_TEMPLATE ("sink",
        GST_PAD_SINK,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ("video/x-avi-unknown") //, fourcc={(fourcc)DXSB, (fourcc)DXSA}")
        );

static GstStaticPadTemplate src_template =
    GST_STATIC_PAD_TEMPLATE ("src",
        GST_PAD_SRC,
        GST_PAD_ALWAYS,
        GST_STATIC_CAPS ("subtitle/x-xsub, " \
            "has-alpha=(boolean) {true, false}, "\
            "width=(int)[0,max], height=(int)[0,max]")
        );

GST_DEBUG_CATEGORY_STATIC (gst_xsubparse_debug);
#define GST_CAT_DEFAULT gst_xsubparse_debug

/* PRQA: Lint Message 19, 123, 144, 751: deactivation because macro is related to GStreamer framework */
/*lint -e19 -e123 -e144 -e751 */
GST_BOILERPLATE(GstXSUBParse, gst_xsubparse, GstBaseParse, GST_TYPE_BASE_PARSE);
/*lint +e19 +e123 +e144 +e751 */

/* GstBaseParse vfuncs */

static gboolean
gst_xsubparse_set_sink_caps (GstBaseParse *object, GstCaps *caps);

static GstFlowReturn
gst_xsubparse_parse_frame (GstBaseParse *object, GstBaseParseFrame *frame);

static void
gst_xsubparse_base_init (gpointer klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  gst_element_class_set_details_simple(element_class,
      "XSUB parser",
      "Codec/Parser/Video",
      "DivX XSUB and XSUB+ subtitle parser",
      "Jens Georg <jgeorg@adit-jv.com>");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&sink_template));

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_template));
}

static void
gst_xsubparse_class_init (GstXSUBParseClass *klass)
{
  GstBaseParseClass *baseparse_class =
      GST_BASE_PARSE_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_xsubparse_debug, "xsubparse", 0,
      "XSUB DivX subtitle parser");

  baseparse_class->set_sink_caps = GST_DEBUG_FUNCPTR(gst_xsubparse_set_sink_caps);
  baseparse_class->parse_frame = GST_DEBUG_FUNCPTR(gst_xsubparse_parse_frame);
}

static void
gst_xsubparse_init (GstXSUBParse *parser, GstXSUBParseClass *klass)
{
  __ADIT_UNUSED (parser);
  __ADIT_UNUSED (klass);
}

static gboolean
gst_xsubparse_set_sink_caps (GstBaseParse *object, GstCaps *caps)
{
  GstVideoFormat format;
  int width = 0;
  int height = 0;
  GstStructure *s;
  guint32 fourcc;
  gboolean has_alpha = FALSE;
  GstCaps *srccaps;

  gst_video_format_parse_caps(caps, &format, &width, &height);
  if (width == 0 || height == 0)
  {
    width = 640;
    height = 480;
    GST_INFO_OBJECT (object, "Did not receive dimensions from demuxer, assuming 640x480");
  }

  s = gst_caps_get_structure(caps, 0);
  if (!gst_structure_get_fourcc(s, "fourcc", &fourcc))
    return FALSE;

  if (fourcc == GST_MAKE_FOURCC('D', 'X', 'S', 'A'))
  {
    has_alpha = TRUE;
  }
  else if (fourcc == GST_MAKE_FOURCC('D', 'X', 'S', 'B'))
  {
    has_alpha = FALSE;
  }
  else
    return FALSE;

  srccaps = gst_caps_new_simple ("subtitle/x-xsub",
      "has-alpha", G_TYPE_BOOLEAN, has_alpha,
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      NULL);

  gst_pad_set_caps (GST_BASE_PARSE_SRC_PAD (object), srccaps);

  return TRUE;
}

static GstFlowReturn
gst_xsubparse_parse_frame (GstBaseParse *object, GstBaseParseFrame *frame)
{
  GstCaps *padcaps;
  guint8 duration[28];
  GstClockTime start;
  GstClockTime buffer_duration;
  char *incaps;
  char *outcaps;

  padcaps = gst_pad_get_caps (GST_BASE_PARSE_SRC_PAD(object));

  memset (duration, 0, sizeof(duration));
  memcpy (duration, GST_BUFFER_DATA(frame->buffer), sizeof(duration) - 1);

  incaps = gst_caps_to_string (GST_BUFFER_CAPS (frame->buffer));
  outcaps = gst_caps_to_string (padcaps);
  GST_TRACE_OBJECT (object, "Incoming buffer had caps %s, changing to %s",
      incaps, outcaps);
  g_free (incaps);
  g_free (outcaps);

  frame->buffer = gst_buffer_make_metadata_writable(frame->buffer);

  gst_buffer_set_caps (frame->buffer, padcaps);

  if (gst_xsub_parse_duration (duration, &start, &buffer_duration) == FALSE)
  {
      GST_WARNING ("Subtitle buffer had invalid timestamp; discarding\n");

      return GST_FLOW_ERROR;
  }

  GST_BUFFER_TIMESTAMP(frame->buffer) = start;
  GST_BUFFER_DURATION(frame->buffer) = buffer_duration;

  return GST_FLOW_OK;
}

/*lint -e826 -e160*/

