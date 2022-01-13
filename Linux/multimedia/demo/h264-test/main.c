/**
 * \file: hud.cpp
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * Sample application for H.264 decoding, similar to AAuto/CarPlay
 *
 * \component: main.c
 *
 * \author: Jens Georg <jgeorg@de.adit-jv.com>
 *
 * \copyright (c) 2016 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 ***********************************************************************/

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gio/gio.h>
#include <gio/gunixinputstream.h>
#include <stdio.h>

int frame_counter = 0;
GstElement *appsrc = NULL;
GDataInputStream *stream = NULL;

void on_data (GObject *obj,
              GAsyncResult *res,
              gpointer user_data)
{
    GError *error = NULL;
    g_data_input_stream_read_line_finish(stream, res, NULL, &error);
    if (error == NULL) {
        char *file = g_strdup_printf("h264-frame%05d.h264", frame_counter);
        g_print ("Pushing frame %s...", file);
        char *data = NULL;
        gssize size = 0;
        if (g_file_get_contents(file, &data, &size, &error))
        {
            GstBuffer *b = gst_buffer_new();
            GST_BUFFER_MALLOCDATA(b) = data;
            GST_BUFFER_DATA(b) = data;
            GST_BUFFER_SIZE(b) = size;
            gst_app_src_push_buffer(GST_APP_SRC(appsrc), b);
        }
        g_free(file);
        g_print("Done.");
        frame_counter++;
    }

    g_data_input_stream_read_line_async(stream, G_PRIORITY_DEFAULT, NULL, on_data, NULL);
}

int main(int argc, char *argv[])
{
    gst_init(&argc, &argv);

    appsrc = gst_element_factory_make ("appsrc", NULL);
    GstCaps *caps = gst_caps_from_string ("video/x-h264,width=800,height=480,framerate=(fraction)0/1,profile=constrained-baseline");
    g_object_set(G_OBJECT(appsrc),
                 "caps", caps,
                 "is-live", TRUE,
                 "do-timestamp", FALSE,
                 "block", FALSE,
                 "min-percent", 0,
                 "max-bytes", (guint64)100000,
                 NULL);
    GstElement *bin = gst_parse_bin_from_description("vpudec low-latency=true frame-plus=2 framedrop=false framerate-nu=30 dis-reorder=true ! multifilesink location=output-dump-%05d.raw sync=false qos=false max-lateness=3000000000", TRUE, NULL);
    GstElement *pipeline = gst_pipeline_new("Example pipeline");
    gst_bin_add_many(GST_BIN(pipeline), appsrc, bin, NULL);
    gst_element_link(appsrc, bin);

    gst_element_set_state(pipeline, GST_STATE_PLAYING);

    g_print ("Waiting for input");
    stream = g_data_input_stream_new (g_unix_input_stream_new (fileno(stdin), FALSE));
    g_data_input_stream_read_line_async(stream, G_PRIORITY_DEFAULT, NULL, on_data, NULL);

    GMainLoop *loop = g_main_loop_new (NULL, FALSE);
    g_main_loop_run (loop);

    return 0;
}
