/**
* \file: gst_viv_v4l_m2m_buffer.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* \component: gst_v4l_csc
*
* \author: Michael Methner ADITG/SW1 mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/

#ifndef _GST_V4L_M2M_BUFFER_H_
#define _GST_V4L_M2M_BUFFER_H_

#include <gst/gst.h>

G_BEGIN_DECLS


GType gst_v4l_m2m_buffer_get_type (void);


typedef struct _GstV4lM2mBuffer GstV4lM2mBuffer;
typedef struct _GstV4lM2mBufferClass GstV4lM2mBufferClass;

struct _GstV4lM2mBuffer
{
  GstBuffer buffer;

  GstElement *parent;
  gboolean (*finalize)(GstV4lM2mBuffer *buffer, GstElement *parent);

  gint index;
};


struct _GstV4lM2mBufferClass
{
  GstBufferClass klass;
};



#define GST_TYPE_V4L_M2M_BUFFER (gst_v4l_m2m_buffer_get_type())

#define GST_IS_V4L_M2M_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_V4L_M2M_BUFFER))
#define GST_V4L_M2M_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_V4L_M2M_BUFFER, GstV4lM2mBuffer))
#define GST_V4L_M2M_BUFFER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_V4L_M2M_BUFFER, GstV4lM2mBufferClass))


G_END_DECLS

#endif /* _GST_V4L_M2M_BUFFER_H_ */
