/**
* \file: gst_apx_buffer.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* Custom GstBuffer class for APX
*
* \component: gst_apx_sink
*
* \author: Jens Georg ADITG/SWG jgeorg@de.adit-jv.com
*
* \copyright: (c) 2013 ADIT Corporation
*
* \history
* 0.1 Jens Georg Initial version
*
***********************************************************************/

#include <string.h>

#include <gst/video/video.h>

#include "gstapx.h"
#include "gst_apx_buffer.h"


/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

static GstBufferClass *apx_buffer_parent_class = NULL;

static void
gst_apx_buffer_finalize (GstApxBuffer *buffer)
{
  if (buffer->apx_buffer)
  {
    struct apx_buffer *tmp = buffer->apx_buffer;
    buffer->apx_buffer = NULL;

    apx_buffer_destroy(tmp, NULL);
  }
}

static void
gst_apx_buffer_class_init (gpointer g_class, gpointer class_data)
{
  GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS (g_class);
  (void) class_data;

  apx_buffer_parent_class = g_type_class_peek_parent (g_class);

  mini_object_class->finalize = (GstMiniObjectFinalizeFunction)
      gst_apx_buffer_finalize;
}

GType
gst_apx_buffer_get_type (void)
{
  static GType _gst_apx_buffer_type = G_TYPE_NONE;

  if (G_UNLIKELY (_gst_apx_buffer_type == G_TYPE_NONE)) {
    static const GTypeInfo apx_buffer_info = {
      sizeof (GstBufferClass),
      NULL,
      NULL,
      gst_apx_buffer_class_init,
      NULL,
      NULL,
      sizeof (GstApxBuffer),
      0,
      NULL,
      NULL
    };
    _gst_apx_buffer_type = g_type_register_static (GST_TYPE_BUFFER,
        "GstApxBuffer", &apx_buffer_info, (GTypeFlags)0);
  }
  return _gst_apx_buffer_type;
}

GstBuffer *
gst_apx_buffer_new (struct apx *apx, GstCaps *caps)
{
  GstApxBuffer *buf;
  GstVideoFormat format;
  int w;
  int h;
  void *p[] = {NULL, NULL, NULL };
  unsigned stride;

  buf = GST_APX_BUFFER(gst_mini_object_new(GST_TYPE_APX_BUFFER));

  gst_video_format_parse_caps(caps, &format, &w, &h);

  buf->format = gst_to_apx (format);
  buf->apx = apx;
  buf->apx_buffer = apx_buffer_create_unbuffered(apx, w, h, buf->format);
  apx_buffer_map(buf->apx_buffer, p);
  apx_buffer_get_stride(buf->apx_buffer, &stride);
  GST_BUFFER_DATA(buf) = p[0];
  GST_BUFFER_SIZE(buf) = h * stride;

  /* The following values are guaranteed by the APX API */
  if (format == GST_VIDEO_FORMAT_NV12)
  {
    GST_BUFFER_SIZE(buf) += (h/2) * stride;
  }
  else if (format == GST_VIDEO_FORMAT_YV12)
  {
    /* two planes of h/2 x stride/2 */
    GST_BUFFER_SIZE(buf) += h * stride / 2;
  }

  gst_buffer_set_caps (GST_BUFFER(buf), caps);
  GST_TRACE("Created APX buffer for caps %"GST_PTR_FORMAT" with size %u",
      caps, GST_BUFFER_SIZE(buf));

  return (GstBuffer *) buf;
}

/*lint +e826 +e160*/
