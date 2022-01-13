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
 * (https://gitlab.freedesktop.org/gstreamer/gst-template/blob/master/gst-plugin/src/gstplugin.c).
 * The example code is dual licensed as MIT and LGPL as above license statement.
 * ADIT selects the MIT license and licenses all extensions under MIT license only.
 */

#include "config.h"

#include <stdint.h>
#include <stdio.h>
#include <dlfcn.h>
#include "rate_core_if.h"

#include <gst/gst.h>
#include <gst/base/gstadapter.h>
#include "gstaditaudioresample.h"

GST_DEBUG_CATEGORY_STATIC (gst_aditaudioresample_debug);
#define GST_CAT_DEFAULT gst_aditaudioresample_debug

#define INPUT_FORMAT_BYTE 2
#define FRAME_CALCULATION 50

/* Filter signals and args */
enum
{
    /* FILL ME */
    LAST_SIGNAL
};

enum
{
    PROP_0,
    PROP_SILENT
};

/* the capabilities of the inputs and outputs.
 *
 * describe the real formats here.
 */
static GstStaticPadTemplate sink_factory = GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "audio/x-raw, "
        "format = (string) {S16LE}, "
        "rate = (int) {8000, 12000, 16000, 22050, 24000, 32000, 44100, 48000}, "
        "channels = (int) {2}, "
        "layout = (string) {interleaved}"
    )
    );

static GstStaticPadTemplate src_factory = GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS (
        "audio/x-raw, "
        "format = (string) {S16LE}, "
        "rate = (int) {8000, 12000, 16000, 22050, 24000, 32000, 44100, 48000}, "
        "channels = (int) {2}, "
        "layout = (string) {interleaved}"
    )
    );

#define gst_aditaudioresample_parent_class parent_class
G_DEFINE_TYPE (Gstaditaudioresample, gst_aditaudioresample, GST_TYPE_ELEMENT);

static gboolean gst_aditaudioresample_query (GstPad *pad, GstObject * parent, GstQuery * query);
static gboolean gst_aditaudioresample_sink_event (GstPad * pad, GstObject * parent, GstEvent * event);
static GstFlowReturn gst_aditaudioresample_chain (GstPad * pad, GstObject * parent, GstBuffer * buf);

static void
gst_aditaudioresample_class_init (GstaditaudioresampleClass * klass)
{
    GstElementClass *gstelement_class;

    gstelement_class = (GstElementClass *) klass;

    gst_element_class_set_details_simple(gstelement_class,
        "aditaudioresample",
        "Filter/Converter/Audio",
        "Resamples audio",
        "Tsunashima <<ytsunashima@adit-jv.jp.com>>");

    gst_element_class_add_static_pad_template (gstelement_class,
        &src_factory);
    gst_element_class_add_static_pad_template (gstelement_class,
        &sink_factory);
}

static void
gst_aditaudioresample_init (Gstaditaudioresample * filter)
{
    struct adit_swsrc_core_ops *ops;
    int (*entry_func)(unsigned int version, void **objp,
        struct adit_swsrc_core_ops **ops);
    void *dlobj = dlopen ("libadit-swsrc.so", RTLD_NOW);

    if (dlobj == NULL) {
        GST_ERROR ("failed to load swsrc library\n");
        return;
    } else {
        filter->pfile = dlobj;
        GST_DEBUG ("success to load swsrc library:%p\n", dlobj);
        entry_func = dlsym (dlobj, "adit_swsrc_core_open");
        if (entry_func == NULL) {
            GST_ERROR ("failed to entry swsrc function\n");
            return;
        } else {
            entry_func (ADIT_RATE_CORE_IF_VERSION, &(filter->swsrc_context), &ops);
        }
    }
    filter->adapter = gst_adapter_new ();
    filter->ops = ops;

    filter->sinkpad = gst_pad_new_from_static_template (&sink_factory, "sink");
    gst_pad_set_event_function (filter->sinkpad,
        GST_DEBUG_FUNCPTR(gst_aditaudioresample_sink_event));
    gst_pad_set_query_function (filter->sinkpad,
        GST_DEBUG_FUNCPTR(gst_aditaudioresample_query));
    gst_pad_set_chain_function (filter->sinkpad,
        GST_DEBUG_FUNCPTR(gst_aditaudioresample_chain));
    GST_PAD_SET_PROXY_CAPS (filter->sinkpad);
    gst_element_add_pad (GST_ELEMENT (filter), filter->sinkpad);

    filter->srcpad = gst_pad_new_from_static_template (&src_factory, "src");

    gst_pad_use_fixed_caps (filter->srcpad);
    gst_element_add_pad (GST_ELEMENT (filter), filter->srcpad);

    filter->caps_nego_done = FALSE;
    filter->sink_rate = 0;
    filter->source_rate = 0;
    filter->channels = 2;
    filter->sink_frames = 0;
    filter->source_frames = 0;
}

/* GstElement vmethod implementations */

static gboolean
gst_aditaudioresample_query (GstPad *pad, GstObject * parent, GstQuery * query)
{
    gboolean ret = FALSE;

    GST_DEBUG ("name=%s\n", GST_QUERY_TYPE_NAME(query));
    switch (GST_QUERY_TYPE (query)) {
        case GST_QUERY_CAPS:
        {
            GstCaps *caps;
            caps = gst_pad_get_pad_template_caps(pad);
            if (caps) {
                gst_query_set_caps_result (query, caps);
                gst_caps_unref (caps);
                ret = TRUE;
            } else {
                GST_ERROR ("caps is null");
            }
            break;
        }
        case GST_QUERY_ACCEPT_CAPS:
        {
            GstCaps *caps;
            gst_query_parse_accept_caps (query, &caps);
            if (gst_caps_is_fixed(caps)) {
                gst_query_set_accept_caps_result (query,TRUE);
                ret = TRUE;
            } else {
                GST_ERROR ("caps is not fixed");
            }
            break;
        }
        default:
            ret = gst_pad_query_default (pad, parent, query);
        break;
    }
    return ret;
}

/* this function handles sink events */
static gboolean
gst_aditaudioresample_filter_setcaps (Gstaditaudioresample *filter,
		GstCaps *caps)
{
    GstCaps *srccaps, *newcaps;
    GstStructure *s = gst_caps_get_structure(caps, 0);

    /* no passthrough, setup internal conversion */
    gst_structure_get_int (s, "rate", &filter->sink_rate);
    gst_structure_get_int (s, "channels", &filter->channels);

    if (filter->channels != 2) {
        GST_ERROR ("only 2 channels are allowed for input\n");
	return FALSE;
    }
    if (filter->sink_rate !=  8000 &&
        filter->sink_rate != 12000 &&
        filter->sink_rate != 16000 &&
        filter->sink_rate != 22050 &&
        filter->sink_rate != 24000 &&
        filter->sink_rate != 32000 &&
        filter->sink_rate != 44100 &&
        filter->sink_rate != 48000) {
        GST_ERROR ("sink rate not supported %d\n", filter->sink_rate);
        return FALSE;
    }

    srccaps = gst_pad_get_allowed_caps (filter->srcpad);

    newcaps = gst_caps_copy_nth (srccaps, 0);

    s = gst_caps_get_structure (newcaps, 0);
    gst_structure_get_int (s, "rate", &filter->source_rate);
    gst_caps_unref (srccaps);
    if (filter->source_rate !=  8000 &&
        filter->source_rate != 12000 &&
        filter->source_rate != 16000 &&
        filter->source_rate != 22050 &&
        filter->source_rate != 24000 &&
        filter->source_rate != 32000 &&
        filter->source_rate != 44100 &&
        filter->source_rate != 48000) {
        GST_ERROR ("source rate not supported\n");
        gst_caps_unref (newcaps);
        return FALSE;
    }

    if (!gst_pad_set_caps (filter->srcpad, newcaps)) {
        GST_ERROR ("gst-pad set caps failed \n");
        gst_caps_unref (newcaps);
        return FALSE;
    } else {
        filter->caps_nego_done = TRUE;
        filter->sink_frames = filter->sink_rate/ FRAME_CALCULATION;
        filter->source_frames = filter->source_rate/ FRAME_CALCULATION;
        //Configure src init info
        struct rate_core_init_info cinfo =
        {
            .in =
            { .freq = filter->sink_rate, .period_size = filter->sink_frames,
                .pitch_frames_max = 0 },
            .out =
              { .freq = filter->source_rate, .period_size = filter->source_frames,
                .pitch_frames_max = 0 },
            .channels = filter->channels
        };
        filter->ops->init(filter->swsrc_context, &cinfo);
    }
    gst_caps_unref (newcaps);
    return TRUE;
}

static gboolean
gst_aditaudioresample_sink_event (GstPad * pad, GstObject * parent, GstEvent * event)
{
    GST_DEBUG ("Event:%s %d\n", GST_EVENT_TYPE_NAME (event),
        GST_EVENT_TYPE (event));
    Gstaditaudioresample *filter;
    gboolean ret = FALSE;

    filter = GST_ADITAUDIORESAMPLE (parent);

    switch (GST_EVENT_TYPE (event)) {
        case GST_EVENT_CAPS:
        {
            GstCaps * caps;

            gst_event_parse_caps (event, &caps);
            ret = gst_aditaudioresample_filter_setcaps (filter, caps);
            break;
        }
        case GST_EVENT_EOS:
        {
            if (filter->ops->close (filter->swsrc_context) == 0) {
                /**
                 *In case of failed sending all gst_adapter's buffer,
                 *execute gst_adapter_flush and flush remaining
                 *gst_adapter's buffer
                 */
                if (filter->adapter && (gst_adapter_available(filter->adapter) != 0))
                    gst_adapter_flush (filter->adapter, gst_adapter_available(filter->adapter));
                ret = gst_pad_push_event (filter->srcpad, event);
	    }
            break;
        }
        case GST_EVENT_SEGMENT:
        {
            if (filter->adapter && (gst_adapter_available(filter->adapter) != 0))
                gst_adapter_flush (filter->adapter, gst_adapter_available(filter->adapter));
            if (filter->caps_nego_done == FALSE) {
                GST_ERROR ("Negotiations not done\n");
            } else {
                GstSegment sink_segment;
                gst_event_copy_segment (event, &sink_segment);
                if (sink_segment.format == GST_FORMAT_TIME) {
                    ret = gst_pad_push_event (filter->srcpad, event);
                } else {
                    gst_event_unref (event);
                    ret = TRUE;
                }
            }
            break;
        }
        default:
            ret = gst_pad_event_default (pad, parent, event);
        break;
    }
    return ret;
}

/* chain function
 * this function does the actual processing
 */
static GstFlowReturn
gst_aditaudioresample_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
    GstAdapter *adapter;
    Gstaditaudioresample *filter;
    gsize sink_buffer_byte;
    gsize source_buffer_byte;
    GstFlowReturn ret = GST_FLOW_OK;
    filter = GST_ADITAUDIORESAMPLE (parent);
    adapter = filter->adapter;
    gst_adapter_push(adapter, buf);
    (void) pad;

    sink_buffer_byte = filter->sink_frames* filter->channels* INPUT_FORMAT_BYTE;
    source_buffer_byte = filter->source_frames* filter->channels* INPUT_FORMAT_BYTE;

    while ((gst_adapter_available (adapter) >= sink_buffer_byte) && ret == GST_FLOW_OK) {
        gpointer ibuf = gst_adapter_take (adapter, sink_buffer_byte);
        if (ibuf == NULL) {
            GST_WARNING ("Invalid buffer returned\n");
            continue;
        }

        GstBuffer *obuf;
        GstMapInfo out_info;

        obuf = gst_buffer_new_allocate (NULL, source_buffer_byte, NULL);
        gst_buffer_map (obuf, &out_info, GST_MAP_READWRITE);

        struct rate_core_convert_info rinfo = {
            .in = {.buf = (int16_t *)ibuf, .frames = filter->sink_frames,
                .pitch_frames = 0},
            .out = {.buf = (int16_t *)out_info.data,
                .frames = filter->source_frames,
                .pitch_frames = 0}
        };

        if (filter->ops == NULL){
            GST_ERROR ("adit_swsrc's ops doesn't set");
            ret = GST_FLOW_ERROR;
        } else {
            filter->ops->convert_s16 (filter->swsrc_context, &rinfo);
            gst_buffer_ref (obuf);
            ret = gst_pad_push (filter->srcpad, obuf);
            g_free (ibuf);
            gst_buffer_unmap (obuf, &out_info);
            gst_buffer_unref (obuf);
        }
    }

    return ret;
  
}


/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
aditaudioresample_init (GstPlugin * aditaudioresample)
{
    GST_DEBUG_CATEGORY_INIT (gst_aditaudioresample_debug, "aditaudioresample",
        0, "ADIT SW-SRC based audio resample plugin");

    return gst_element_register (aditaudioresample, "aditaudioresample", GST_RANK_NONE,
        GST_TYPE_ADITAUDIORESAMPLE);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "aditaudioresample"
#endif

/* gstreamer looks for this structure to register aditaudioresamples
 *
 * exchange the string 'Template aditaudioresample' with your aditaudioresample description
 */
GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    aditaudioresample,
    "ADIT SW SRC based audio resample",
    aditaudioresample_init,
    VERSION,
    "MIT/X11",
    "GStreamer",
    "http://gstreamer.net/"
)
