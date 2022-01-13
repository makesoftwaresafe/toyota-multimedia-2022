/**
* \file: xsub.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* DivX XSUB subtitle handling
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

#ifndef __XSUB_H__
#define __XSUB_H__

#ifndef __ADIT_UNUSED
#define __ADIT_UNUSED(x) (void)(x)
#endif

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "xsubdecode"
#endif

#include <gst/gst.h>

G_BEGIN_DECLS

gboolean
gst_xsub_parse_duration (guint8 *buffer, GstClockTime *start, GstClockTime *duration);

G_END_DECLS

#endif /* __XSUB_H__ */
