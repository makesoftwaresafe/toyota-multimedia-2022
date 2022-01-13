/**
* \file: gst_apx_buffer.h
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

#ifndef __GST_APX_BUFFER_H__
#define __GST_APX_BUFFER_H__

#include <gst/gst.h>
#include <apx.h>

G_BEGIN_DECLS

#define GST_APX_BUFFER_NATIVE(buf) (GST_APX_BUFFER((buf))->apx_buffer)

typedef struct _GstApxBuffer GstApxBuffer;

GType gst_apx_buffer_get_type (void);
#define GST_TYPE_APX_BUFFER (gst_apx_buffer_get_type())
#define GST_IS_APX_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_APX_BUFFER))
#define GST_APX_BUFFER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_APX_BUFFER, GstApxBuffer))

struct _GstApxBuffer
{
  GstBuffer buffer;

  struct apx_buffer *apx_buffer;
  struct apx *apx;
  apxPixelFormat format;
};

GstBuffer *
gst_apx_buffer_new (struct apx *apx, GstCaps *caps);

GstBuffer *
gst_apx_buffer_new_from_buffer (GstBuffer *buffer);

G_END_DECLS

#endif /* __GST_APX_BUFFER_H__ */
