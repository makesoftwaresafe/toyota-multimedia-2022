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


typedef enum {
  GST_PLAY_FLAG_VIDEO         = (1 << 0),
  GST_PLAY_FLAG_AUDIO         = (1 << 1),
  GST_PLAY_FLAG_TEXT          = (1 << 2),
  GST_PLAY_FLAG_VIS           = (1 << 3),
  GST_PLAY_FLAG_SOFT_VOLUME   = (1 << 4),
  GST_PLAY_FLAG_NATIVE_AUDIO  = (1 << 5),
  GST_PLAY_FLAG_NATIVE_VIDEO  = (1 << 6),
  GST_PLAY_FLAG_DOWNLOAD      = (1 << 7),
  GST_PLAY_FLAG_BUFFERING     = (1 << 8)
} GstPlayFlags;



typedef struct
{
  GstElement *playbin;
  GstElement *display;
  GstElement *audiosink;
  GstElement *textsink;
  GMainLoop *loop;
  GList *playlist;
  gint filecount;
  gboolean shuffle;
  gboolean error;
  gdouble rate;
  gboolean endless_loop;
} player;


void
playback_rate(player *play, gdouble rate);


void
play_next(player *play)
{
  GList *next_pos;

  gst_element_set_state(play->playbin, GST_STATE_NULL);

  if (play->shuffle)
    {
      guint length = g_list_length(g_list_first(play->playlist));
      next_pos = g_list_nth(g_list_first(play->playlist), ((guint)rand()) % length);
    }
  else
    {
      next_pos = g_list_next(play->playlist);
      if (!next_pos)
        {
          if(!play->endless_loop)
            {
              g_main_loop_quit(play->loop);
              return;
            }
          next_pos = g_list_first(play->playlist);
        }
      play->playlist = next_pos;
    }

  play->filecount++;
  g_print("#%d playing: %s\n",play->filecount,(char *)(next_pos->data));

  g_object_set(play->playbin, "uri",
      next_pos->data, NULL);

  gst_element_set_state(play->playbin, GST_STATE_PLAYING);

}


void
play_prev(player *play)
{
  GList *prev_pos;

  gst_element_set_state(play->playbin, GST_STATE_NULL);

  if (play->shuffle)
    {
      guint length = g_list_length(g_list_first(play->playlist));
      prev_pos = g_list_nth(g_list_first(play->playlist), ((guint)rand()) % length);
    }
  else
    {
      prev_pos = g_list_previous(play->playlist);
      if (!prev_pos)
        {
          prev_pos = g_list_last(play->playlist);
        }
      play->playlist = prev_pos;
    }

  play->filecount++;
  g_print("#%d playing: %s\n",play->filecount,(char *)(prev_pos->data));

  g_object_set(play->playbin, "uri",
      prev_pos->data, NULL);

  gst_element_set_state(play->playbin, GST_STATE_PLAYING);
}





static void
print_tag(const GstTagList * list, const gchar * tag, gpointer user_data)
{
  user_data = user_data;
  int i, num;

  num = gst_tag_list_get_tag_size(list, tag);
  for (i = 0; i < num; ++i)
    {
      const GValue *val;
      val = gst_tag_list_get_value_index(list, tag, i);

      g_print("tag: \"%s\" ", tag);

      if (G_VALUE_HOLDS(val, G_TYPE_STRING))
        {
          g_print("string: \"%s\"\n", g_value_get_string(val));
        }
      else if (G_VALUE_HOLDS(val, G_TYPE_UINT))
        {
          g_print("uint: \"%u\"\n", g_value_get_uint(val));
        }
      else if (G_VALUE_HOLDS(val, G_TYPE_DOUBLE))
        {
          g_print("double: \"%f\"\n", g_value_get_double(val));
        }
      else if (G_VALUE_HOLDS(val, G_TYPE_BOOLEAN))
        {
          g_print("boolean: \"%u\"\n", g_value_get_boolean(val));
        }
      else if (G_VALUE_HOLDS(val, GST_TYPE_BUFFER))
        {
          g_print("GstBuffer\n");
        }
      else if (G_VALUE_HOLDS(val, GST_TYPE_DATE))
        {
          g_print("date: (dd.mm.yyyy) %02u.%02u.%u\n",
              g_date_get_day(gst_value_get_date(val)),
              g_date_get_month(gst_value_get_date(val)),
              g_date_get_year(gst_value_get_date(val))
                  );
        }
      else
        {
          g_print("unkown type: \"%s\"\n", G_VALUE_TYPE_NAME (val));
        }
    }
}



static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  bus = bus;

  player *play = (player *) data;
  switch (GST_MESSAGE_TYPE(msg))
    {
  case GST_MESSAGE_EOS:
    {
    play_next(play);
    break;
    }
  case GST_MESSAGE_ERROR:
    {
      gchar *debug;
      GError *error;
      FILE * fid;

      gst_message_parse_error(msg, &error, &debug);
      g_free(debug);
      g_printerr("Error: %s\n", error->message);

      fid = fopen("last_error_message.txt","wt");
      if( fid)
        {
        fprintf(fid,"%s",error->message);
        fclose(fid);
        }

      g_error_free(error);
      play->error = TRUE;
      g_main_loop_quit(play->loop);
      break;
    }
  case GST_MESSAGE_TAG:
    {
      GstTagList *tags = NULL;
      gst_message_parse_tag(msg, &tags);
      gst_tag_list_foreach (tags, print_tag, NULL);
      gst_tag_list_free(tags);
      break;
    }
  case GST_MESSAGE_STATE_CHANGED:
    {
      if (GST_MESSAGE_SRC(msg) != GST_OBJECT(play->playbin))
        return TRUE;

      GstState old, new_state, pending;
      gst_message_parse_state_changed(msg, &old, &new_state, &pending);
      if (GST_STATE_TRANSITION(old, new_state) == GST_STATE_CHANGE_PAUSED_TO_PLAYING)
        {
          GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(play->playbin), GST_DEBUG_GRAPH_SHOW_ALL, "gst_demo_player");
        }
    }
    break;
  default:
    /*
    g_print("Message: %s\n",gst_message_type_get_name(GST_MESSAGE_TYPE(msg)));
    g_print("Structure: %s\n",
        gst_structure_to_string(gst_message_get_structure(msg)));
    */
    break;
    }
  return TRUE;
}



void
playback_rate(player *play, gdouble rate)
{
  if(rate == play->rate)
    {
    g_print("error: same rate\n");
    /* do not set to current rate */
    return;
    }
  play->rate = rate;

  GstFormat fmt = GST_FORMAT_TIME;
  gint64    cur_pos;
  GstBuffer *last_buffer = NULL;

  /* get last displayed buffer for exact segment configuration */
  g_object_get(G_OBJECT(play->display), "last-buffer", &last_buffer, NULL);
  if(last_buffer)
    {
      cur_pos = GST_BUFFER_TIMESTAMP(last_buffer);
      gst_buffer_unref(last_buffer);
    }
  else
    {
    gst_element_query_position (play->display, &fmt, &cur_pos);
    }
  g_print("playback_rate: %.2f  %" GST_TIME_FORMAT "\n", rate, GST_TIME_ARGS(cur_pos));

  GstSeekFlags flags = GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_KEY_UNIT;
  if( rate > 1.1f || rate < 0.0f )
    {
    flags = flags | GST_SEEK_FLAG_SKIP;
    }

  if(rate < 0.0f)
    {
    gst_element_seek (play->playbin, rate, GST_FORMAT_TIME,
        flags,
        GST_SEEK_TYPE_SET, 0,
        GST_SEEK_TYPE_SET, cur_pos);
    }
  else
    {
    gst_element_seek (play->playbin, rate, GST_FORMAT_TIME,
        flags,
        GST_SEEK_TYPE_SET, cur_pos,
        GST_SEEK_TYPE_SET, -1 );
    }
}

void
seek (player *play, gint64 new_pos)
{
  if (new_pos < 0)
    new_pos = 0;

  gst_element_seek (play->playbin, 1.0, GST_FORMAT_TIME, GST_SEEK_FLAG_FLUSH |  GST_SEEK_FLAG_KEY_UNIT , GST_SEEK_TYPE_SET,
                    new_pos, GST_SEEK_TYPE_NONE,  (gint64)GST_CLOCK_TIME_NONE);
}

void
relative_seek (player *play, double displacement)
{
  g_print("relative seek: %.2f\n", displacement);
  GstFormat fmt = GST_FORMAT_TIME;
  gint64    cur_pos;
  gst_element_query_position (play->playbin, &fmt, &cur_pos);

  double new_pos_sec = cur_pos * (1.0 / GST_SECOND) + displacement;
  seek (play, (gint64)(new_pos_sec * GST_SECOND));
}



void
toggle_pause(player *play)
{
  GstState current_state;
  gst_element_get_state(play->playbin, &current_state, NULL, GST_CLOCK_TIME_NONE);


  if (current_state == GST_STATE_PAUSED)
  {
    gst_element_set_state (play->playbin, GST_STATE_PLAYING);
  }
  else if (current_state == GST_STATE_PLAYING) {
    gst_element_set_state (play->playbin, GST_STATE_PAUSED);
  }
}


void print_keys( void )
{
  g_print("______________________________\n");
  g_print("Commands:\n");
  g_print("1\t: Seek -60 seconds\n");
  g_print("2\t: Seek -10 seconds\n");
  g_print("3\t: Seek -2 seconds\n");
  g_print("4\t: Seek 2 seconds\n");
  g_print("5\t: Seek 10 seconds\n");
  g_print("6\t: Seek 60 seconds\n");
  g_print("\n");
  g_print("a/A\t: 30x rewind\n");
  g_print("s/S\t: 10x rewind\n");
  g_print("d/D\t: 2x rewind\n");
  g_print("f/F\t: 1x rewind\n");
  g_print("g/G\t: 1x forward\n");
  g_print("h/H\t: 2x fast forward\n");
  g_print("j/J\t: 10x fast forward\n");
  g_print("k/K\t: 30x fast forward\n");
  g_print("\n");
  g_print("i/I\t: play previous\n");

  g_print("o/O\t: play/pause\n");
  g_print("p/P\t: play next\n");

  g_print("n/N\t: Volume -\n");
  g_print("m/M:\t: Volume +\n");

  g_print("x/X\t: Next subtitle\n");

  g_print("q/Q\t: Quit\n");
  g_print("______________________________\n");
}

gboolean
query_position(player *play)
{
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 pos, len;

  if (gst_element_query_position (play->playbin, &fmt, &pos) &&
      gst_element_query_duration (play->playbin, &fmt, &len))
    {
      g_print ("Time: %" GST_TIME_FORMAT " ", GST_TIME_ARGS (pos));
      g_print (" / %" GST_TIME_FORMAT, GST_TIME_ARGS (len));
      g_print ("\r");
    }

  /* call me again */
  return TRUE;
}

void
process_hotkey(player *play, char key)
{
  switch (key)
    {
  case '1':
    relative_seek(play, -60);
    break;
  case '2':
    relative_seek(play, -10);
    break;
  case '3':
    relative_seek(play, -2);
    break;
  case '4':
    relative_seek(play, 2);
    break;
  case '5':
    relative_seek(play, 10);
    break;
  case '6':
    relative_seek(play, 60);
    break;
  case 'q':
  case 'Q':
    g_print("Quit\n");
    gst_element_set_state(play->playbin, GST_STATE_NULL);
    if (play->loop)
      g_main_loop_quit(play->loop);
    break;
  case 'a':
  case 'A':
    playback_rate(play, -30);
    break;
  case 's':
  case 'S':
    playback_rate(play, -10);
    break;
  case 'd':
  case 'D':
    playback_rate(play, -2);
    break;
  case 'f':
  case 'F':
    playback_rate(play, -1);
    break;
  case 'g':
  case 'G':
    playback_rate(play, 1);
    break;
  case 'h':
  case 'H':
    playback_rate(play, 2);
    break;
  case 'j':
  case 'J':
    playback_rate(play, 10);
    break;
  case 'k':
  case 'K':
    playback_rate(play, 30);
    break;
  case 'o':
  case 'O':
    toggle_pause(play);
    break;
  case 'i':
  case 'I':
    play_prev(play);
    break;
  case 'p':
  case 'P':
    play_next(play);
    break;
  case 'r':
  case 'R':
    play->shuffle = !play->shuffle;
    g_print("Shuffle: %d\n", play->shuffle);
    break;
  case 'n':
  case 'N':
  case 'M':
  case 'm':
    {
      gdouble volume;
      g_object_get(G_OBJECT(play->playbin), "volume", &volume, NULL);
      if (key == 'M' || key == 'm')
        {
          volume += 0.1;
        }
      else
        {
          volume -= 0.1;
        }
      g_print("volume: %f", volume);
      g_object_set(G_OBJECT(play->playbin), "volume", volume, NULL);
      break;
    }

  case 'x':
  case 'X':
    {
      int current_text, n_texts, i;

      g_object_get(G_OBJECT(play->playbin),
                   "current-text", &current_text,
                   "n-text", &n_texts,
                   NULL);

      for (i = 0; i < n_texts; i++)
      {
        GstTagList *tags;

        g_signal_emit_by_name (G_OBJECT(play->playbin), "get-text-tags", i, &tags);
        if (tags)
        {
          char *lang;
          if (!gst_tag_list_get_string(tags, GST_TAG_LANGUAGE_CODE, &lang))
            lang = g_strdup ("(unknown)");

          g_print ("Subtitle %d has language code %s\n", i,
              lang);
          g_free(lang);
        }
      }

      current_text ++;
      current_text %= n_texts;
      g_object_set(G_OBJECT(play->playbin), "current-text", current_text, NULL);
      g_print ("Setting subtitle to %d\n", current_text);
      break;
    }

   default:
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


#ifdef USB_KEYBOARD_SUPPORT

gpointer
usb_key_reader(gpointer data)
{
  player * play = (player *) data;

  printf("keyreader\n");

  static FILE * fid = NULL;
  if (!fid)
    {
      fid = fopen("/dev/input/by-id/usb-_USB_Keyboard-event-kbd", "rb");
    }

  if (fid)
    {
    gboolean quit = FALSE;
    do
      {
      struct input_event data;
      int size = fread(&data, 1, sizeof(data), fid);

      if (size == sizeof(data) &&data.type == EV_KEY && data.value == 1)
        {
        printf("pressed: code: %d\n", data.code);
        switch(data.code)
        {
        case KEY_1:
          relative_seek(play, -60);
          break;
        case KEY_2:
          relative_seek(play, -10);
          break;
        case KEY_3:
          //relative_seek(play, -2);
          break;
        case KEY_4:
          //relative_seek(play, 2);
          break;
        case KEY_5:
          relative_seek(play, 10);
          break;
        case KEY_6:
          relative_seek(play, 60);
          break;
        case KEY_Q:
          //gst_element_set_state (play->playbin, GST_STATE_NULL);
          //if (play->loop)
          //     g_main_loop_quit (play->loop);
          break;
        case KEY_A:
          //playback_rate(play, -30);
          break;
        case KEY_S:
          //playback_rate(play, -10);
          break;
        case KEY_D:
          //playback_rate(play, -2);
          break;
        case KEY_F:
          //playback_rate(play, -1);
          break;
        case KEY_G:
          //playback_rate(play, 1);
          break;
        case  KEY_H:
          //playback_rate(play, 2);
          break;
        case KEY_J:
          //playback_rate(play, 10);
          break;
        case KEY_K:
          playback_rate(play, 30);
          break;
        case KEY_O:
          toggle_pause(play);
          break;
        case KEY_I:
          play_prev(play);
          break;
        case KEY_P:
          play_next(play);
          break;
        case KEY_7:
          g_object_set(G_OBJECT(play->display), "scale_mode", 0, NULL );
          break;
        case KEY_8:
            g_object_set(G_OBJECT(play->display), "scale_mode", 1, NULL );
            break;
        case KEY_9:
          g_object_set(G_OBJECT(play->display), "size-x", 800, NULL );
          g_object_set(G_OBJECT(play->display), "size-y", 480, NULL );
          g_object_set(G_OBJECT(play->display), "scale_mode", 2, NULL );
          break;
        case KEY_0:
          g_object_set(G_OBJECT(play->display), "size-x", 0, NULL );
          g_object_set(G_OBJECT(play->display), "size-y", 0, NULL );
          g_object_set(G_OBJECT(play->display), "scale_mode", 2, NULL );
          break;
        case KEY_R:
          play->shuffle = !play->shuffle;
          g_print("Shuffle: %d\n",play->shuffle);
          break;
        case KEY_N:
        case KEY_M:
          {
          gdouble volume;
          g_object_get(G_OBJECT(play->playbin), "volume", &volume, NULL);
          if(KEY_M == data.code)
            {
            volume += 0.1;
            }
          else
            {
            volume -= 0.1;
            }
          g_print("volume: %f",volume);
          g_object_set(G_OBJECT(play->playbin), "volume", volume, NULL);
          break;
          }
        }
        }
     } while(quit == FALSE);
    }
  else
    {
    printf    ("could not open usb keyboard\n");
    }
  return NULL;
}

#endif

void
videoFirstFrameRendered(GstElement * element, GstPad *pad, gpointer user_data)
{
  g_print("first frame rendered: element: %p pad: %p user_data: %p\n",
      element, pad, user_data);
}


int
main(int argc, char *argv[])
{
  GstBus * bus;
  player play;
  play.playlist = NULL;
  play.filecount = 1;
  play.rate = 1.0f;
  play.endless_loop = FALSE;
  play.error = FALSE;

  g_print("GStreamer Demo Player\n");
  print_keys();

  /* Initialisation */
  gst_init(&argc, &argv);

  /* Check input arguments */
  if (argc < 2)
    {
      g_printerr("Usage: %s < filename>\n", argv[0]);
      g_printerr("Usage: %s < filename_1> ... < filename_n>\n", argv[0]);
      return -1;
    }

  gint i;
  for(i=1;i<argc;i++)
    {
    play.playlist = g_list_append(play.playlist, argv[i]);
    }


  play.shuffle = FALSE;
  play.loop = g_main_loop_new(NULL, FALSE);
  play.display = gst_element_factory_make("gst_apx_sink", "display");
  /*play.display = gst_element_factory_make("fakesink", "display"); */

  play.audiosink = gst_element_factory_make("alsasink", "audio");
  /*play.audiosink = gst_element_factory_make("fakesink", "audio"); */
  // play.textsink = gst_element_factory_make("xsubsink", "text");

  play.playbin = gst_element_factory_make("playbin2", "play");

  if(play.loop == NULL || play.display == NULL || play.audiosink == NULL ||
      play.playbin == NULL )
    {
    g_print("could not create all elements\n");
    return -1;
    }

  GstPlayFlags flags = GST_PLAY_FLAG_NATIVE_VIDEO
                      /* | GST_PLAY_FLAG_NATIVE_AUDIO */
                      | GST_PLAY_FLAG_VIDEO
                      | GST_PLAY_FLAG_AUDIO
                      | GST_PLAY_FLAG_TEXT
                      | GST_PLAY_FLAG_SOFT_VOLUME;

  g_object_set(G_OBJECT(play.playbin), "audio-sink", play.audiosink, NULL);
  g_object_set(G_OBJECT(play.playbin), "video-sink", play.display, NULL);
  // g_object_set(G_OBJECT(play.playbin), "text-sink", play.textsink, NULL);
  g_object_set(G_OBJECT(play.playbin), "uri", argv[1], NULL);
  // g_object_set(G_OBJECT(play.playbin), "volume", 0.1, NULL);
  g_object_set(G_OBJECT(play.playbin), "flags",flags, NULL);

  /* play only native video, otherwise playbin will insert ffmpegcolorspace
   * into queue which will perform memcpy operations and mix up the passing
   * back of the image buffer to the video decoder
   */

  g_object_set(G_OBJECT(play.audiosink), "device", "entertainment_main", NULL );


  g_signal_connect(play.display,
      "first-videoframe-rendered",
      G_CALLBACK (videoFirstFrameRendered), NULL);

  if (!play.loop || !play.playbin || !play.display || !play.audiosink /* || !play.textsink */)
    {
      g_printerr("One element could not be created. Exiting.\n");
      return -1;
    }

  GIOChannel * gio_read = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(gio_read, G_IO_IN, stdin_handler, &play);

#ifdef USB_KEYBOARD_SUPPORT
  GThread * key_thread = g_thread_create(usb_key_reader, &play, FALSE, NULL);
#endif

  bus = gst_pipeline_get_bus(GST_PIPELINE(play.playbin));
  gst_bus_add_watch(bus, bus_call, &play);
  gst_object_unref(bus);

  g_print("Now playing: %s\n", argv[1]);
  gst_element_set_state(play.playbin, GST_STATE_PLAYING);

  g_timeout_add (100, (GSourceFunc) query_position, &play);

  g_unix_signal_add (SIGINT, (GSourceFunc)g_main_loop_quit, play.loop);
  g_unix_signal_add (SIGTERM, (GSourceFunc)g_main_loop_quit, play.loop);

  g_print("Running...\n");
  g_main_loop_run(play.loop);

  g_print("Returned, stopping playback\n");
  gst_element_set_state(play.playbin, GST_STATE_NULL);

  g_print("Deleting playbin\n");
  gst_object_unref(GST_OBJECT(play.playbin));

  gst_object_unref(GST_OBJECT(bus));

  g_io_channel_unref (gio_read);

  g_list_free(play.playlist);

  int ret = 0;
  if(play.error) /* got error during playback ? */
    {
      ret = -1;
    }

  return ret;
}

/*lint -restore*/

/* PRQA: Lint Message 749: deactivation because not all values of the GstPlayFlags enum are used*/
/*lint -save -e749 */
