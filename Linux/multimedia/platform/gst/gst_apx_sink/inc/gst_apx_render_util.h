/**
* \file: gst_apx_render_util.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* Draw a GstBuffer using APX
*
* \component: gst_apx_sink
*
* \author: Jens Georg ADITG/SWG jgeorg@de.adit-jv.com
*
* \copyright: (c) 2014 ADIT Corporation
*
* \history
* 0.1 Jens Georg Initial version
*
***********************************************************************/

#ifndef __GST_APX_RENDER_UTIL_H__
#define __GST_APX_RENDER_UTIL_H__

#include <glib.h>

#include <gst/gst.h>

#include "gst_apx_sink.h"

G_BEGIN_DECLS

G_GNUC_INTERNAL gboolean
gst_apx_render_buffer(GstApxSink *sink, GstBuffer *buf, gboolean lock);

G_END_DECLS

#endif
