/*
 * GStreamer
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2019 Yuki Tsunashima <ytsunashima@jp.adit-jv.com>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * This code was derived from the GStreamer example plugin code
 * (https://gitlab.freedesktop.org/gstreamer/gst-template/blob/master/gst-plugin/src/gstplugin.h).
 * The example code is dual licensed as MIT and LGPL as above license statement.
 * ADIT selects the MIT license and licenses all extensions under MIT license only.
 */

#ifndef __GST_ADITAUDIORESAMPLE_H__
#define __GST_ADITAUDIORESAMPLE_H__

#include <gst/gst.h>


G_BEGIN_DECLS

/* #defines don't like whitespacey bits */
#define GST_TYPE_ADITAUDIORESAMPLE \
  (gst_aditaudioresample_get_type())
#define GST_ADITAUDIORESAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_ADITAUDIORESAMPLE,Gstaditaudioresample))
#define GST_ADITAUDIORESAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_ADITAUDIORESAMPLE,GstaditaudioresampleClass))
#define GST_IS_ADITAUDIORESAMPLE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_ADITAUDIORESAMPLE))
#define GST_IS_ADITAUDIORESAMPLE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_ADITAUDIORESAMPLE))

typedef struct _Gstaditaudioresample      Gstaditaudioresample;
typedef struct _GstaditaudioresampleClass GstaditaudioresampleClass;

struct GstAdapter;

struct _Gstaditaudioresample
{
  GstElement element;

  GstAdapter *adapter;

  GstPad *sinkpad, *srcpad;

  gboolean caps_nego_done;

  gpointer *pfile;

  int sink_rate;
  int source_rate;
  int channels;
  int sink_frames;
  int source_frames;
  struct rate_core_init_info core_info;

  struct rate_core_convert_info info;

  struct adit_swsrc_core_ops *ops;

  void *swsrc_context;
};

struct _GstaditaudioresampleClass 
{
  GstElementClass parent_class;
};

GType gst_aditaudioresample_get_type (void);

G_END_DECLS

#endif /* __GST_ADITAUDIORESAMPLE_H__ */
