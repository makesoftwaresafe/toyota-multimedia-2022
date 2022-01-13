/**
* \file: gst_rawvideo_src.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* filereader for YUV raw files with support for gst_pad_alloc
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/

#ifndef __GST_RAWVIDEO_SRC_H__
#define __GST_RAWVIDEO_SRC_H__

#include <gst/base/gstpushsrc.h>


G_BEGIN_DECLS


#define GST_TYPE_RAWVIDEO_SRC \
  (gst_rawvideo_src_get_type())
#define GST_RAWVIDEO_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_RAWVIDEO_SRC,GstRawvideoSrc))
#define GST_VIV_APP_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_RAWVIDEO_SRC,GstRawvideoSrcClass))
#define GST_IS_RAWVIDEO_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_RAWVIDEO_SRC))
#define GST_IS_RAWVIDEO_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_RAWVIDEO_SRC))



typedef struct _GstRawvideoSrc GstRawvideoSrc;
typedef struct _GstRawvideoSrcClass GstRawvideoSrcClass;

struct _GstRawvideoSrc
{
  GstPushSrc element;
  gint width;
  gint height;
  float bitwidth;
  gint format;
  gint framerate_nominator;
  gint framerate_denominator;

  FILE * fid;
  gchar *filename;
  GstClockTime next_timestamp;
  GstCaps * caps;
};


struct _GstRawvideoSrcClass
{
  GstPushSrcClass  parent_class;
};


GType gst_viv_app_sink_get_type(void);

G_END_DECLS

#endif /* __GST_RAWVIDEO_SRC_H__ */
