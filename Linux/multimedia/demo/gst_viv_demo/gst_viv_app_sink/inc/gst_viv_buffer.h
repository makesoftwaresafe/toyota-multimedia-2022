/**
* \file: gst_viv_buffer.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* Vivante YUV Buffer for Gstreamer framework.
* This buffer is used to pass data between the gstreamer  and
* vivante graphics core
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SW1 mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/

#ifndef _GST_VIV_BUFFER_H_
#define _GST_VIV_BUFFER_H_

#include <gst/gst.h>
#include <GLES2/gl2.h>

G_BEGIN_DECLS


GType gst_viv_buffer_get_type (void);


typedef struct _GstVivBuffer GstVivBuffer;
typedef struct _GstVivBufferClass GstVivBufferClass;

struct _GstVivBuffer
{
  GstBuffer buffer;

  GstElement *parent;
  gboolean (*finalize)(GstVivBuffer *buffer, GstElement *parent);

  GLuint texName;
};


struct _GstVivBufferClass
{
  GstBufferClass klass;
};



#define GST_TYPE_VIV_BUFFER (gst_viv_buffer_get_type())

#define GST_IS_VIV_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_VIV_BUFFER))
#define GST_VIV_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_VIV_BUFFER, GstVivBuffer))
#define GST_VIV_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_VIV_BUFFER, GstVivBufferClass))


G_END_DECLS

#endif /* _GST_VIV_BUFFER_H_ */
