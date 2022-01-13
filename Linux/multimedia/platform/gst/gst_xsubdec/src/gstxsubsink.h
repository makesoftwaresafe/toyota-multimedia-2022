/**
* \file: gstxsubsink.h
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

#ifndef __GST_XSUBSINK_H__
#define __GST_XSUBSINK_H__

#include <gst/gst.h>

G_BEGIN_DECLS

GType gst_xsubsink_get_type (void);

#define GST_TYPE_XSUBSINK (gst_xsubsink_get_type())
#define GST_XSUBSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_XSUBSINK, GstXSUBSink))
#define GST_XSUBSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST(klass),GST_TYPE_XSUBSINK,GstXSUBSinkClass))
#define GST_IS_XSUBSINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_XSUBSINK))
#define GST_IS_XSUBSINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_XSUBSINK))

typedef struct _GstXSUBSink GstXSUBSink;
typedef struct _GstXSUBSinkClass GstXSUBSinkClass;

struct _GstXSUBSink
{
  GstBin parent;
  GstElement *decode, *sink;

  guint surface_id;
  guint layer_id;
};

struct _GstXSUBSinkClass
{
  GstBinClass parent_class;
};

G_END_DECLS

#endif /* __GST_XSUBSINK_H__ */
