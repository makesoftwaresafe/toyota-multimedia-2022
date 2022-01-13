/**
* \file: gst_apx_sink_wl_if.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* Render video images using wayland, GLESv2 and Vivante Extensions
* for mapping YUV data.
*
* \component: multimedia/gst
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/


#ifndef _GST_APX_SINK_WL_IF_H_
#define _GST_APX_SINK_WL_IF_H_

#include "gst_apx_sink.h"

gboolean
gst_apx_sink_wl_if_init(GstApxSink * sink);

gboolean
gst_apx_sink_wl_if_commit_buffer(GstApxSink * sink);

void
gst_apx_sink_wl_if_stop(GstApxSink *sink);

void
gst_apx_sink_wl_if_close(GstApxSink * sink);

gboolean
gst_apx_sink_wl_if_create_apx_context(GstApxSink * sink);

gboolean
gst_apx_sink_wl_if_draw_buffer(GstApxSink *sink, GstBuffer *buf);

#endif
