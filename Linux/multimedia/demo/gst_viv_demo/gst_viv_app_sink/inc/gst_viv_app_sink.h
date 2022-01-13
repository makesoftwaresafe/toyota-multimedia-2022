/**
* \file: gst_viv_app_sink.h
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
* \copyright: (c) 2003 - 2011 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/

#ifndef __GST_VIV_APP_SINK_H__
#define __GST_VIV_APP_SINK_H__

#include <gst/video/gstvideosink.h>
#include "gst_viv_buffer.h"

#include <GLES2/gl2.h>


G_BEGIN_DECLS


#define GST_TYPE_VIV_APP_SINK \
  (gst_viv_app_sink_get_type())
#define GST_VIV_APP_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_VIV_APP_SINK,GstVivAppSink))
#define GST_VIV_APP_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_VIV_APP_SINK,GstVivAppSinkClass))
#define GST_IS_VIV_APP_SINK(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_VIV_APP_SINK))
#define GST_IS_VIV_APP_SINK_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_VIV_APP_SINK))





typedef struct _GstVivAppSink GstVivAppSink;
typedef struct _GstVivAppSinkClass GstVivAppSinkClass;

struct _GstVivAppSink
{
  GstVideoSink videosink;

  GMutex *pool_lock;
  GCond *pool_data;
  GSList *buffer_pool;

  gint width;
  gint height;
  GstVivBuffer * buffer;
};


struct _GstVivAppSinkClass
{
  GstVideoSink parent_class;
  void (*render) (GstElement *element, GstPad *pad);
};


GType gst_viv_app_sink_get_type(void);

void gst_viv_app_add_buffer(GstVivAppSink * sink, void * ptr, GLuint texName, gint size);
GLuint gst_viv_app_get_current_texname(GstVivAppSink * sink);


G_END_DECLS

#endif /* __GST_VIV_APP_SINK_H__ */
