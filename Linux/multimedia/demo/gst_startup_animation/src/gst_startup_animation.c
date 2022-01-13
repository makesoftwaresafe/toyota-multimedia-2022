/**
* \file: main.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* Testapplication for video and audio playback using gstreamer
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2013 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
* 0.2 Michael Methner Ported from Gen2 to Gen3
***********************************************************************/


#include <gst/gst.h>

#include <glib.h>

#include <mcheck.h>

#include <signal.h>
#include <glib-unix.h>

// #define USB_KEYBOARD_SUPPORT

#ifdef USB_KEYBOARD_SUPPORT
#include <linux/input.h>
#endif

#include <string.h>


/* PRQA: Lint Message 160,826: deactivation because of GStreamer macro code*/
/*lint -save -e160 -e826*/


typedef struct
{
  GstElement *filesrc;
  GstElement *demux;
  GstElement *vpu;
  GstElement *queue;
  GstElement *display;
  GstElement *pipeline;

  char * video[2];

  GMainLoop *loop;
} player;




static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  bus = bus;

  player *play = (player *) data;
  switch (GST_MESSAGE_TYPE(msg))
    {
  case GST_MESSAGE_EOS:
    {
    gst_element_set_state(play->pipeline, GST_STATE_NULL);
    gst_element_set_state(play->pipeline, GST_STATE_READY);
    break;
    }
  case GST_MESSAGE_ERROR:
    {
      gchar *debug;
      GError *error;


      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);
      g_printerr("Error: %s\n", error->message);
      g_error_free(error);

      g_main_loop_quit(play->loop);
      break;
    }

  case GST_MESSAGE_STATE_CHANGED:
    {
      GstState old, new_state, pending;
      gst_message_parse_state_changed(msg, &old, &new_state, &pending);
      printf("state-changed: %s, old: %d new: %d pending: %d\n",GST_ELEMENT_NAME(GST_MESSAGE_SRC(msg)), old,new_state,pending);
     }
    break;
  default:

    g_print("Message: %s\n",gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
    g_print("Structure: %s\n",
        gst_structure_to_string(gst_message_get_structure(msg)));

    break;
    }
  return TRUE;
}





void print_keys( void )
{
  g_print("______________________________\n");
  g_print("Commands:\n");
  g_print("1\t: Play first video\n");
  g_print("2\t: Play second video\n");
  g_print("q/Q\t: Quit\n");
  g_print("______________________________\n");
}


///////////////////////////////////////////////////////////////////
//
// This is the function that should react on the trigger from the
// silverbox to play a video
//
///////////////////////////////////////////////////////////////////
void
play_video(player *play, int video)
{
  gst_element_set_state(play->pipeline, GST_STATE_NULL);
  g_object_set(play->filesrc,"location",play->video[video], NULL);
  gst_element_set_state(play->pipeline, GST_STATE_PLAYING);
}



void
process_hotkey(player *play, char key)
{
  switch (key)
    {
  case '1':
    play_video(play, 0);
    break;
  case '2':
    play_video(play, 1);
    break;
  case 'q':
  case 'Q':
    g_print("Quit\n");
    gst_element_set_state(play->pipeline, GST_STATE_NULL);
    if (play->loop)
      g_main_loop_quit(play->loop);
    break;
    }
}


gboolean
stdin_handler(GIOChannel *source, GIOCondition condition, gpointer data)
{
  source = source;

  player *play;
  play = (player*)data;

  if (condition == G_IO_IN)
    {
      char key;
      key = getchar();
      process_hotkey(play, key);
     }
  return TRUE;
}

void
videoFirstFrameRendered(GstElement * element, GstPad *pad, gpointer user_data)
{
  g_print("first frame rendered: element: %p pad: %p user_data: %p\n",
      element, pad, user_data);
}

void pad_added(GstElement* object,
                                   GstPad* arg0,
                                   gpointer user_data)
{
  object = object;
  arg0 = arg0;
  player *play = user_data;
  gchar * name = gst_pad_get_name(arg0);
  printf("%s: %s\n",__FUNCTION__,name);

  if(0==strncmp(name,"video",sizeof("video")-1))
  {
      gst_element_link(play->demux,play->vpu);
  }
  g_free(name);

}
void pad_removed(GstElement* object,
                                     GstPad* arg0,
                                     gpointer user_data)
{
  object = object;
  arg0 = arg0;


  player *play = user_data;
  printf("%s\n",__FUNCTION__);
  gchar * name = gst_pad_get_name(arg0);
  if(0==strncmp(name,"video",sizeof("video")-1))
  {
      gst_element_unlink(play->demux,play->vpu);
  }
  g_free(name);
}



int
main(int argc, char *argv[])
{
  GstBus * bus;
  player play;

  g_print("GStreamer Startup Animation Demo\n");
  print_keys();

  /* Initialisation */
  gst_init(&argc, &argv);

  /* Check input arguments */
  if (argc != 3)
    {
      g_printerr("Usage: %s <filename_1> <filename_2>\n", argv[0]);
      return -1;
    }
  play.video[0] = argv[1];
  play.video[1] = argv[2];

  play.loop = g_main_loop_new(NULL, FALSE);
  play.filesrc = gst_element_factory_make("filesrc", "filesrc");
  play.demux = gst_element_factory_make("qtdemux", "demux");
  play.vpu = gst_element_factory_make("vpudec", "vpudec");
  play.queue = gst_element_factory_make("queue", "queue");
  play.display = gst_element_factory_make("gst_apx_sink", "display");
  play.pipeline = gst_pipeline_new ("pipeline");

  /*play.display = gst_element_factory_make("fakesink", "display"); */

  if(play.loop == NULL || play.filesrc == NULL || play.demux == NULL ||
      play.vpu == NULL || play.queue == NULL || play.display == NULL)
    {
    g_print("could not create all elements\n");
    return -1;
    }


  g_object_set(play.filesrc, "typefind", FALSE, NULL);
  g_object_set(play.queue, "max-size-buffers", 4, NULL);
  g_signal_connect(play.display,
      "first-videoframe-rendered",
      G_CALLBACK (videoFirstFrameRendered), NULL);

  gst_bin_add_many (GST_BIN(play.pipeline), play.filesrc, play.demux, play.vpu, play.queue, play.display, NULL);
  GstCaps * caps = gst_caps_from_string("video/quicktime, variant=(string)iso");

  gst_element_link_filtered(play.filesrc, play.demux,caps);
  // gst_element_link_many (play.filesrc, play.demux, NULL);
  gst_element_link_many (play.vpu, play.queue, play.display, NULL);

  g_signal_connect(play.demux,
      "pad-added",
      G_CALLBACK (pad_added), (gpointer)&play);

  g_signal_connect(play.demux,
      "pad-removed",
      G_CALLBACK (pad_removed), (gpointer)&play);


  bus = gst_pipeline_get_bus(GST_PIPELINE(play.pipeline));
  gst_bus_add_watch(bus, bus_call, &play);
  gst_object_unref(bus);

  g_unix_signal_add (SIGINT, (GSourceFunc)g_main_loop_quit, play.loop);
  g_unix_signal_add (SIGTERM, (GSourceFunc)g_main_loop_quit, play.loop);

  GIOChannel * gio_read = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(gio_read, G_IO_IN, stdin_handler, &play);

  ///////////////////////////////////////////////////////////////////
  //
  // SIGNAL here to silverbox that we are ready to playback a file
  //
  ///////////////////////////////////////////////////////////////////

  g_print("Running...\n");

  g_main_loop_run(play.loop);

  g_print("Returned, stopping playback\n");
  gst_element_set_state(play.pipeline, GST_STATE_NULL);

  g_print("Deleting playbin\n");
  gst_object_unref(GST_OBJECT(play.pipeline));

  gst_object_unref(GST_OBJECT(bus));

  g_io_channel_unref (gio_read);

  return 0;
}

/*lint -restore*/

/* PRQA: Lint Message 749: deactivation because not all values of the GstPlayFlags enum are used*/
/*lint -save -e749 */
