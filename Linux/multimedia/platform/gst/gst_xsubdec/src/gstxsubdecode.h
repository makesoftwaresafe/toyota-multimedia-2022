/**
* \file: gstxsubdecode.h
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

#ifndef __GST_XSUBDECODE_H__
#define __GST_XSUBDECODE_H__

#include <gst/gst.h>

G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_XSUBDECODE \
  (gst_xsubdecode_get_type())
#define GST_XSUBDECODE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_XSUBDECODE,GstXSUBDecode))
#define GST_XSUBDECODE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_XSUBDECODE,GstXSUBDecodeClass))
#define GST_IS_XSUBDECODE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_XSUBDECODE))
#define GST_IS_XSUBDECODE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_XSUBDECODE))

typedef struct _GstXSUBDecode      GstXSUBDecode;
typedef struct _GstXSUBDecodeClass GstXSUBDecodeClass;

struct _GstXSUBDecode
{
  GstElement element;

  GstPad *sinkpad, *srcpad;

  int plane_width;
  int plane_height;
  int plane_size;
  int stride;
  gboolean has_alpha;
  GstTagList *language;
  GstClock *clock;

  gboolean silent;
  gboolean debug_dump_decoded_package;
  gboolean debug_show_background;
  guint bad_buffer_count;
};

struct _GstXSUBDecodeClass 
{
  GstElementClass parent_class;
};

GType gst_xsubdecode_get_type (void);

G_END_DECLS

#endif /* __GST_XSUBDECODE_H__ */
