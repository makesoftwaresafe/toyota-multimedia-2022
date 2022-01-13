/**
* \file: gstxsubparse.h
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

#ifndef __GST_XSUBPARSE_H__
#define __GST_XSUBPARSE_H__

#include <gst/gst.h>
#include <gst/base/gstbaseparse.h>

G_BEGIN_DECLS

GType gst_xsubparse_get_type (void);

#define GST_TYPE_XSUBPARSE (gst_xsubparse_get_type())
#define GST_XSUBPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_XSUBPARSE, GstXSUBParse))
#define GST_XSUBPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST(klass),GST_TYPE_XSUBPARSE,GstXSUBParseClass))
#define GST_IS_XSUBPARSE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_XSUBPARSE))
#define GST_IS_XSUBPARSE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_XSUBPARSE))

typedef struct _GstXSUBParse GstXSUBParse;
typedef struct _GstXSUBParseClass GstXSUBParseClass;

struct _GstXSUBParse
{
  GstBaseParse parent;

  GstTagList *language;
};

struct _GstXSUBParseClass
{
  GstBaseParseClass parent_class;
};

G_END_DECLS

#endif /* __GST_XSUBPARSE_H__ */
