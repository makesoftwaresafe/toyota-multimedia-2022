/******************************************************************************
 * \file dynamic_plugin_test.c
 *
 * \brief GStreamer test application to test dynamic changing of element in a
 *        playing pipeline
 *
 * \author Gautham Kantharaju / RBEI / ECF3 / Gautham.Kantharaju@in.bosch.com
 *
 * \copyright (c) 2007 - 2014 ADIT Corporation
 *
 *****************************************************************************/
#include "dynamic_plugin_test.h"

GstElement *v4lsrc;
GstElement *v4lqueue;
GstElement *waylandqueue;

GstElement *v4lsink;
GstElement *waylandsink;
GstElement *pipeline;
GstPad *v4lsrcpad;
GstPad *waylandsrcpad;
GMainLoop *loop;

void
change_graphic_bg(gint bckground)
{
  if (FRAMEBUFFER_BCKGROUND == bckground)
    {
      system("systemctl disable layer-management-wayland.service");
      system("systemctl stop layer-management-wayland.service");
    }
  else if (WAYLAND_BCKGROUND == bckground)
    {
      //system("source /opt/platform/switch-graphics-backend.sh wayland");
      system("systemctl enable layer-management-wayland.service");
      system("systemctl start layer-management-wayland.service");

#if 1
      system("export XDG_RUNTIME_DIR=/tmp");
      system("export LM_PLUGIN_PATH=/usr/lib/layermanager");
      system("export LD_LIBRARY_PATH=/usr/lib/gstreamer-0.10");
#endif

      printf("Need to export wayland related env variables \n");

      system("LayerManagerControl create layer 3000 1024 768");
      system("LayerManagerControl set screen 0 render order 3000");
      system("LayerManagerControl set screen 1 render order 3000");
      system("LayerManagerControl set layer 3000 render order 10");
      system("LayerManagerControl set layer 3000 visibility 1");
    }
  else
    {
      printf("Nothing to be done \n");
    }
}

static void
unlink_remove_v4lsink()
{
  GstPad *sinkpad;

  /* Unlink the v4lqueue and v4lsink */
  gst_element_unlink(v4lqueue, v4lsink);

  /**
   * EOS event, flushing out the data
   * from v4lsink element
   */
  sinkpad = gst_element_get_static_pad(v4lsink, "sink");

  printf("start send flush_start v4lsink\n");
  gst_pad_send_event(sinkpad,gst_event_new_flush_start());
  printf("start send flush_stop v4lsink\n");

  gst_pad_send_event(sinkpad,gst_event_new_flush_stop());
  printf("start send eos v4lsink\n");
  gst_pad_send_event(sinkpad, gst_event_new_eos());
  printf("stop send eos v4lsink\n");


  gst_element_set_state(GST_ELEMENT(v4lsink), GST_STATE_NULL);

  /* remove v4lsink from bin */
  gst_bin_remove(GST_BIN(pipeline), v4lsink);

  gst_object_unref(sinkpad);
 }


static void
pad_block_v4l_cb(GstPad *pad, gboolean blocked, gpointer user_data)
{
  printf("v4l pad blocked pad=%p \t blocked=%d \t user_data=%p \n",
         pad, blocked, user_data);
  unlink_remove_v4lsink();
}

static void
unlink_remove_waylandsink()
{
  GstPad *sinkpad;

  /* Unlink the waylandqueue and waylandsink */
  gst_element_unlink(waylandqueue, waylandsink);

  /**
   * EOS event, flushing out the data
   * from waylandsink(fakesink) element
   */
  sinkpad = gst_element_get_static_pad(waylandsink, "sink");
  gst_pad_send_event(sinkpad, gst_event_new_eos());

  gst_element_set_state(GST_ELEMENT(waylandsink), GST_STATE_NULL);

  /* remove waylandsink from bin */
  gst_bin_remove(GST_BIN(pipeline), waylandsink);
  gst_object_unref(sinkpad);
}

static void
add_link_wayland()
{
  waylandsink = gst_element_factory_make("gst_apx_sink", "apxsink");

  /* Switch to wayland background to create wayland element */
  if (!waylandsink)
    {
      printf("Failed to create waylandsink element \n");
      exit(EXIT_FAILURE);
    }

  g_object_set(GST_ELEMENT(waylandsink), "sync", FALSE, NULL);

  /* Waylandsink to bin */
  if (TRUE != (gst_bin_add(GST_BIN(pipeline), waylandsink)))
    printf("Could not add waylandsink element to bin \n");

  if (TRUE != (gst_element_sync_state_with_parent(waylandsink)))
    printf("Wayland sink state not synched with parent \n");

  /* Link v4lsrc - waylandsink */
  if (TRUE != (gst_element_link(waylandqueue, waylandsink)))
    printf("V4Lsrc and Waylandsink linked... \n");
}

static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  printf("\n\nbus_call: %s\n\n bus=%p \t data=%p\n",
         GST_MESSAGE_TYPE_NAME(msg), bus, data);
  GstState oldstate;
      GstState newstate;
      GstState pending;

  switch (GST_MESSAGE_TYPE(msg))
    {
  case GST_MESSAGE_STREAM_STATUS:
    {
      GstStreamStatusType type;
      GstElement *owner;
      gst_message_parse_stream_status      (msg, &type,&owner);
      printf("streamstatus type: %d owner: %s\n",type,GST_OBJECT_NAME(owner));
      break;
    }
  case GST_MESSAGE_STATE_CHANGED:

    gst_message_parse_state_changed (msg,&oldstate,&newstate,&pending);
    printf("state-changed: %s old: %d new: %d pending: %d\n",
        GST_MESSAGE_SRC_NAME(msg),oldstate,newstate,pending);
    break;
  case GST_MESSAGE_EOS:
    printf("End of stream\n");
    break;

  case GST_MESSAGE_ERROR:
    {
      gchar *debug;
      GError *error;

      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);
      g_printerr("Error: %s\n", error->message);
      g_error_free(error);

      // TBD: error handling can be optimized for msg->src. In case of v4lsink
      // being the source switch to gst_apx_sink may solve the issue
      g_main_loop_quit(loop);
      break;
    }
  default:
    break;
    }

  return TRUE;
}


void
print_usage(char *argv)
{
  printf("Usage: %s <camera_device> <pixel_format> <display_device>\n", argv);
  printf ("Pass camera and display device information \n");
  printf ("Pass camera and display device information \n");
  printf("V4Lsink used, pix_fmt supported values [6, 9] \n");
  printf("Wayalndsink used, pix_fmt supported values [6, 7, 9] \n");
}


void
v4lBufferErrorCB(GstElement * element, GstPad *pad, gpointer user_data)
{
  printf("v4lBufferErrorCB: element: %p pad: %p user_data: %p\n", element, pad,
      user_data);
}


void
v4lBufferErrorDoneCB(GstElement * element, GstPad *pad, gpointer user_data)
{
  printf("v4lBufferErrorDoneCB: element: %p pad: %p user_data: %p\n", element,
      pad, user_data);
}


static void
pad_unblock_cb(GstPad *pad, gboolean blocked, gpointer user_data)
{
  printf("unblocked pad=%p \t blocked=%d \t user_data=%p \n",
         pad, blocked, user_data);
}


static void
pad_block_cb(GstPad *pad, gboolean blocked, gpointer user_data)
{
  printf ("blocked pad=%p \t blocked=%d \t user_data=%p \n",
          pad, blocked, user_data);

  unlink_remove_waylandsink();

  add_link_wayland();

  /* unblock the v4lsrc pad */
  if (TRUE != (gst_pad_set_blocked_async(pad, FALSE, pad_unblock_cb, NULL)))
    {
      debug("V4lsrc pad not be unblocked \n");
    }
}


static void * start_layermanager(void *arg)
{
  arg = arg;
  printf("threaded layermanager start\n");
  change_graphic_bg(WAYLAND_BCKGROUND);
  printf("threaded layermanager finished\n");

  return NULL;
}


gpointer mainLoopfunc(gpointer data)
{
  printf ("data = %p\n", data);
  g_main_loop_run(loop);
  return NULL;
}


int
main(int argc, char *argv[])
{
  GstBus *bus;
  GstElement *tee;
  GstPadTemplate *tee_src_pad_template;
  GstPad *tee_v4l_pad;
  GstPad *tee_wayland_pad;
  GstPad *v4l_pad;
  GstPad *wayland_pad;
  GstElementClass *klass;

  gst_init(&argc, &argv);

  if (argc != 4)
    {
      print_usage(argv[0]);
      exit(EXIT_FAILURE);
    }

  loop = g_main_loop_new(NULL, FALSE);

  pipeline = gst_pipeline_new("pipeline");

  /* we add a message handler */
  bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
  gst_bus_add_watch(bus, bus_call, loop);

  v4lsrc = gst_element_factory_make("mfw_v4lsrc", "v4lsource");

  tee = gst_element_factory_make("tee", "tee");

  v4lqueue = gst_element_factory_make("queue", "v4lqueue");
  waylandqueue = gst_element_factory_make("queue", "waylandqueue");
  waylandsink = gst_element_factory_make("fakesink", "fakesink");

  /**
   * Switch back to framebuffer graphic background,
   * since v4lsink is functional early during start-up
   */
  change_graphic_bg(FRAMEBUFFER_BCKGROUND);

  v4lsink = gst_element_factory_make("mfw_v4lsink", "v4lsink");

  if (!pipeline || !v4lsrc || !tee || !v4lqueue || !v4lsink || !waylandqueue
      || !waylandsink)
    {
      printf("Failed to create gst element \n");
      exit(EXIT_FAILURE);
    }

  /* mfw_v4lsrc property setting */
  g_object_set(G_OBJECT(v4lsrc), "device", argv[1], NULL);
  g_object_set(G_OBJECT(v4lsrc), "pix_fmt", atoi(argv[2]), NULL);

  g_object_set(G_OBJECT(v4lsink), "device", argv[3], NULL);
  g_object_set(G_OBJECT(v4lsink), "disp-type", "lvds", NULL);
  g_object_set(G_OBJECT(v4lsink), "disp-width", 800, NULL);
  g_object_set(G_OBJECT(v4lsink), "disp-height", 480, NULL);

  g_signal_connect(G_OBJECT(v4lsrc), "v4l-buf-flag-error",
      G_CALLBACK(v4lBufferErrorCB), NULL);

  g_signal_connect(G_OBJECT(v4lsrc), "v4l-buf-flag-ok",
      G_CALLBACK(v4lBufferErrorDoneCB), NULL);

  g_object_set(G_OBJECT(v4lqueue), "leaky", 2, NULL);
  g_object_set(G_OBJECT(v4lqueue), "max-size-buffers", 2, NULL);

  g_object_set(G_OBJECT(waylandqueue), "leaky", 2, NULL);
  g_object_set(G_OBJECT(waylandqueue), "max-size-buffers", 2, NULL);

  gst_bin_add_many(GST_BIN(pipeline), v4lsrc, tee, v4lqueue, v4lsink,
      waylandqueue, waylandsink, NULL);

  if (!gst_element_link_many(v4lsrc, tee, NULL))
    {
      gst_object_unref(pipeline);
      g_critical("Unable to link mfw_v4lsrc to tee. check your caps.");
      return 0;
    }

  /* Link the mfw_v4lsink */
  if (!gst_element_link_many(v4lqueue, v4lsink, NULL))
    {
      gst_object_unref(pipeline);
      g_critical("Unable to link mfw_v4lsrc->tee->v4lqueue->mfw_v4lsink");
      return 0;
    }

  /* Link the fakesink */
  if (!gst_element_link_many(waylandqueue, waylandsink, NULL))
    {
      gst_object_unref(pipeline);
      g_critical("Unable to link, mfw_v4lsrc->tee->waylandqueue->fakesink");
      return 0;
    }

  /* Manually link the Tee, which has "Request" pads */
  klass = GST_ELEMENT_GET_CLASS(tee);
  if (!(tee_src_pad_template = gst_element_class_get_pad_template(klass,
      "src%d")))
    {
      gst_object_unref(pipeline);
      g_critical("Unable to get pad template");
      return 0;
    }

  tee_v4l_pad = gst_element_request_pad(tee, tee_src_pad_template, NULL, NULL);
  printf("Obtained request pad %s for v4lsink branch.\n",
      gst_pad_get_name(tee_v4l_pad));
  v4l_pad = gst_element_get_static_pad(v4lqueue, "sink");

  tee_wayland_pad = gst_element_request_pad(tee, tee_src_pad_template, NULL,
      NULL);
  printf("Obtained request pad %s for gst_apx_sink branch.\n",
      gst_pad_get_name(tee_wayland_pad));
  wayland_pad = gst_element_get_static_pad(waylandqueue, "sink");

  /* Link the tee to the v4lqueue */
  if (gst_pad_link(tee_v4l_pad, v4l_pad) != GST_PAD_LINK_OK)
    {
      g_critical("Tee for v4lqueue could not be linked.\n");
      gst_object_unref(pipeline);
      return 0;
    }

  /* Link the tee to the waylandqueue */
  if (gst_pad_link(tee_wayland_pad, wayland_pad) != GST_PAD_LINK_OK)
    {
      g_critical("Tee for waylandqueue could not be linked.\n");
      gst_object_unref(pipeline);
      return 0;
    }

  GThread * mainLoopThread = g_thread_new("MainLoop Thread", mainLoopfunc, loop);

  GstStateChangeReturn sc_return = gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
  printf("set state to playing %d\n",sc_return);

  pthread_t thread = 0;
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  pthread_create(&thread, &attr, start_layermanager, NULL);

  /**
   * Sleep for few secs, before dynamically switching the sink
   * element from fakesink to gst_apx_sink
   */
  sleep(5);

  GstState state;
  GstState pending;
  gst_element_get_state(pipeline, &state, &pending,GST_CLOCK_TIME_NONE);
  printf("\n\n\n\nstate: %d pending: %d\n\n\n\n ", state, pending);
  if(pending != GST_STATE_VOID_PENDING )
    {
      /* abort the early pipeline because it is not playing, this may cause flickering if state is
       * changing to playing between get_state and set_state */
      printf("v4lsink error case\n");
      gst_element_set_state(pipeline, GST_STATE_NULL);

      unlink_remove_v4lsink();
      unlink_remove_waylandsink();
      add_link_wayland();
      gst_element_set_state(pipeline, GST_STATE_PLAYING);
    }
  else
    {

      GstPad * srcpad = gst_element_get_static_pad(waylandqueue, "src");
      if (gst_pad_set_blocked_async(srcpad, TRUE, pad_block_cb, loop) != TRUE)
        debug("V4lsrc pad not be blocked \n");
      sleep(2);

      printf("++++++++ unlink_remove_v4lsink +++++++++++\n");
      /* disabling mfw_v4lsink */
      srcpad = gst_element_get_static_pad(v4lqueue, "src");

      _gst_debug_bin_to_dot_file(GST_BIN(pipeline), GST_DEBUG_GRAPH_SHOW_ALL,
          "file.dot");

      // pad_block_v4l_cb(NULL,TRUE, NULL);

      printf("srcpad %p\n", srcpad);
      if (gst_pad_set_blocked_async(srcpad, TRUE, pad_block_v4l_cb, loop)
          != TRUE)
        debug("V4lsrc pad not be blocked \n");

    }

  // wait for finished mainLoop and then exit
  g_thread_join(mainLoopThread);
  gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_NULL);

  gst_object_unref(v4l_pad);
  gst_object_unref(wayland_pad);
  gst_object_unref(GST_OBJECT(pipeline));
  gst_object_unref(bus);
  g_main_loop_unref(loop);

  exit(EXIT_SUCCESS);
}
