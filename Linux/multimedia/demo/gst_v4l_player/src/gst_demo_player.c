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

#include <stdio.h>

#include <string.h>


#include <ilm_client.h>
#include <ilm_control.h>
#include <wayland-client.h>
#include <wayland-egl.h>
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

/* Commandline options */
static gboolean run_guidelines_thread = FALSE;
static int video_layer_id = 3000;
static int video_surface_id = 40;
static int guidelines_layer_id = 7;
static int guidelines_surface_id = 40;
static int pixel_format = 9;
static gboolean force_memcpy = FALSE;

inline GLint glGetInteger(GLenum name) {
    GLint i;
    glGetIntegerv(name,&i);
    return i;
}

extern const struct wl_interface serverinfo_interface;
struct serverinfo;

struct serverinfo_listener {
  void (*connection_id)(void *data,
            struct serverinfo *serverinfo,
            uint32_t connection_id);
};

static inline int
serverinfo_add_listener(struct serverinfo *serverinfo,
      const struct serverinfo_listener *listener, void *data)
{
  return wl_proxy_add_listener((struct wl_proxy *) serverinfo,
             (void (**)(void)) listener, data);
}

#define SERVERINFO_GET_CONNECTION_ID  0

static inline void
serverinfo_get_connection_id(struct serverinfo *serverinfo)
{
  wl_proxy_marshal((struct wl_proxy *) serverinfo,
       SERVERINFO_GET_CONNECTION_ID);
}

#define __ADIT_UNUSED(u) (void)(u)

/* PRQA: Lint Message 160,826: deactivation because of GStreamer macro code*/
/*lint -save -e160 -e826*/

typedef struct
{
  GstElement *queue;
  GstElement *src;
  GstElement *sink;
  GMainLoop *loop;
  GstElement *bin;
} player;

typedef struct
{
  /* Wayland stuff */
  guint surface_id;
  guint layer_id;
  struct wl_display* wl_display;
  struct wl_compositor* wl_compositor;
  struct wl_egl_window *wl_egl_window;
  struct serverinfo* wl_ext_serverinfo;
  guint32 wl_connect_id;
  guint32 wl_mask;
  struct wl_registry* wl_registry;
  struct wl_surface* wl_surface;

  EGLDisplay eglDisplay;
  EGLSurface eglWindowSurface;
  EGLContext eglContext;
  EGLConfig eglConfig;
} glDemo;

static const EGLint gl_context_attribs[] =
{
    EGL_CONTEXT_CLIENT_VERSION, 2,
    EGL_NONE
};

void demo_init_egl (glDemo *egl_info)
{
  EGLint s_configAttribs[] =
      {
          EGL_RED_SIZE,         8,
          EGL_GREEN_SIZE,       8,
          EGL_BLUE_SIZE,        8,
          EGL_ALPHA_SIZE,       8,
          EGL_DEPTH_SIZE,       EGL_DONT_CARE,
          EGL_STENCIL_SIZE,     EGL_DONT_CARE,
          EGL_SURFACE_TYPE,     EGL_WINDOW_BIT,
          EGL_SAMPLES,          glGetInteger(GL_MAX_SAMPLES_EXT),
          EGL_RENDERABLE_TYPE,  EGL_OPENVG_BIT,
          EGL_NONE
      };

  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;
  EGLBoolean res;

  egl_info->eglDisplay = eglGetDisplay( (EGLNativeDisplayType)egl_info->wl_display);

  s_configAttribs[16] =  EGL_NONE;
  res = eglBindAPI(EGL_OPENGL_ES_API);
  if (!res)
  {
    printf ("eglBind\n");
    return;
  }


  res = eglInitialize(egl_info->eglDisplay, &majorVersion, &minorVersion);
  if (!res)
  {
    printf("eglInitialize\n");
    return;
  }

  res = eglGetConfigs(egl_info->eglDisplay, NULL, 0, &numConfigs);
  if (!res)
  {
    printf("eglGetConfigs\n");
    return;
  }

  res = eglChooseConfig(egl_info->eglDisplay, s_configAttribs, &egl_info->eglConfig, 1, &numConfigs);
  if (!res)
  {
    printf("eglChoseConfig\n");
    return;
  }


  egl_info->eglWindowSurface = eglCreateWindowSurface(egl_info->eglDisplay, egl_info->eglConfig, (EGLNativeWindowType)egl_info->wl_egl_window, NULL);

  egl_info->eglContext = eglCreateContext(egl_info->eglDisplay,
      egl_info->eglConfig,
      EGL_NO_CONTEXT,
      gl_context_attribs);

  res = eglMakeCurrent(egl_info->eglDisplay,
      egl_info->eglWindowSurface,
      egl_info->eglWindowSurface,
      egl_info->eglContext);
  if (!res)
  {
    printf("eglMakeCurrent\n");
    return;
  }

  res = eglSwapInterval(egl_info->eglDisplay,1);
  if (!res)
  {
    printf("eglMakeCurrent\n");
    return;
  }
}


/************************** Wayland ****************************************/
static void serverinfo_cb_impl__demo(void *data, struct serverinfo *pServerinfo, uint32_t client_handle)
{
  __ADIT_UNUSED (pServerinfo);

    glDemo * sink = (glDemo *)data;
    sink->wl_connect_id = client_handle;
}

struct serverinfo_listener serverinfo_cb__demo = {
    serverinfo_cb_impl__demo
};

static const struct wl_interface *types__demo[] = {
        NULL,
};

static const struct wl_message serverinfo_requests__demo[] = {
        { "get_connection_id", "", types__demo + 0 },
};

static const struct wl_message serverinfo_events__demo[] = {
        { "connection_id", "u", types__demo + 0 },
};
const struct wl_interface serverinfo_interface__demo = {
        "serverinfo", 1,
        G_N_ELEMENTS(serverinfo_requests__demo), serverinfo_requests__demo,
        G_N_ELEMENTS(serverinfo_events__demo), serverinfo_events__demo,
};


void
RegistryHandleGlobal__demo(void* data, struct wl_registry* registry, uint32_t name,
    const char* interface, uint32_t version)
{
  __ADIT_UNUSED(version);
  glDemo *sink = (glDemo *)data;

  if (!strcmp(interface, "wl_compositor"))
    {
      sink->wl_compositor = (struct wl_compositor*) (wl_registry_bind(registry,
          name, &wl_compositor_interface, 1));
      GST_LOG("sink: %p\n sink->wl_compositor: %p", sink, sink->wl_compositor);
    }

  if (!strcmp(interface, "serverinfo"))
    {
      sink->wl_ext_serverinfo = (struct serverinfo*) wl_registry_bind(registry,
          name, &serverinfo_interface__demo, 1);
      serverinfo_add_listener(sink->wl_ext_serverinfo, &serverinfo_cb__demo, data);
      serverinfo_get_connection_id(sink->wl_ext_serverinfo);
      GST_LOG("sink->wl_ext_serverinfo: %p", sink->wl_ext_serverinfo);
    }
}


static struct wl_registry_listener registryListener__demo = {
    RegistryHandleGlobal__demo,
    NULL
};

gboolean
demo_create_wl_context(glDemo * sink)
{
    if (ILM_FAILED == ilm_init())
      {
        GST_ERROR("ilm_init failed");
        return FALSE;
      }

    sink->wl_display = wl_display_connect(NULL);
    if (sink->wl_display == NULL)
    {
        GST_ERROR("wl_display_connect failed");
        return FALSE;
    }

    sink->wl_registry = wl_display_get_registry(sink->wl_display);
    wl_registry_add_listener(sink->wl_registry, &registryListener__demo, sink);
    wl_display_dispatch(sink->wl_display);

    wl_display_roundtrip(sink->wl_display);
    if(sink->wl_compositor == NULL)
    {
      return FALSE;
    }

    sink->wl_surface = wl_compositor_create_surface(sink->wl_compositor);
    if(sink->wl_surface == NULL)
    {
        GST_ERROR("wl_compositor_create_surface failed");
        return FALSE;
    }

    sink->wl_egl_window = wl_egl_window_create(sink->wl_surface, 800, 480);
    if (sink->wl_egl_window == NULL)
    {
      GST_ERROR("wl_egl_window_create failed");
      return FALSE;
    }

    return TRUE;
}

gboolean
demo_create_ilm_context(glDemo * sink)
{
/* Modification for APX: We allocate surfaces the size of the video and
 * we set the destination as smaller. We rely on the layer manager to perform
 * the scaling. */
  guint32 native_ilm_handle;
  ilmErrorTypes error;
  struct wl_object* p_obj;


   p_obj = (struct wl_object*)sink->wl_surface;
   native_ilm_handle = (sink->wl_connect_id << 16) | (uint32_t)p_obj->id;

   error = ilm_surfaceCreate( (t_ilm_nativehandle) native_ilm_handle, 800, 480,
           ILM_PIXELFORMAT_RGBA_8888, (t_ilm_surface*)&(sink->surface_id));

   if (error == ILM_FAILED)
   {
       GST_ERROR("ilm_surfaceCreate failed");
       return FALSE;
   }

   error = ilm_surfaceSetDestinationRectangle((t_ilm_surface)sink->surface_id,
       0,0,800,480);
   if (error == ILM_FAILED)
   {
       GST_ERROR("ilm_surfaceSetDestinationRectangle failed");
       return FALSE;
   }

   error = ilm_surfaceSetSourceRectangle((t_ilm_surface)sink->surface_id,
       0, 0, 800, 480);
   if (error == ILM_FAILED)
   {
       GST_ERROR("ilm_surfaceSetSourceRectangle failed");
       return FALSE;
   }

   error = ilm_layerAddSurface((t_ilm_layer)sink->layer_id, (t_ilm_surface)sink->surface_id);
   if (error == ILM_FAILED)
   {
       GST_ERROR("ilm_layerAddSurface failed");
       return FALSE;
   }

   error = ilm_surfaceSetVisibility((t_ilm_surface)sink->surface_id, ILM_TRUE);
   if (error == ILM_FAILED)
   {
       GST_ERROR("ilm_surfaceSetVisibility failed");
       return FALSE;
   }

   error = ilm_commitChanges();
   if (error == ILM_FAILED)
   {
       GST_ERROR("ilm_commitChanges failed");
       return FALSE;
   }

   return TRUE;
}

/************************** Wayland/END **************************************/


static gboolean
bus_call(GstBus *bus, GstMessage *msg, gpointer data)
{
  __ADIT_UNUSED(bus);

  player *play = (player *) data;
  switch (GST_MESSAGE_TYPE(msg))
    {
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
      g_main_loop_quit(play->loop);
      break;
    }
  case GST_MESSAGE_STATE_CHANGED:
    {
      if (GST_MESSAGE_SRC(msg) != GST_OBJECT(play->bin))
        return TRUE;

      GstState old, new_state, pending;
      gst_message_parse_state_changed(msg, &old, &new_state, &pending);
      if (GST_STATE_TRANSITION(old, new_state) == GST_STATE_CHANGE_PAUSED_TO_PLAYING)
        {
          GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(play->bin), GST_DEBUG_GRAPH_SHOW_ALL, "gst_demo_player");
        }
    }
    break;
  default:
    break;
    }
  return TRUE;
}


void
toggle_pause(player *play)
{
  GstState current_state;
  gst_element_get_state(play->bin, &current_state, NULL, GST_CLOCK_TIME_NONE);


  if (current_state == GST_STATE_NULL)
  {
	  g_print ("==> Going to Playing\n");
    gst_element_set_state (play->bin, GST_STATE_PLAYING);
  }
  else if (current_state == GST_STATE_PLAYING) {
	  g_print ("==> Going to pause\n");
    gst_element_set_state (play->bin, GST_STATE_NULL);
  } else {
	  g_print ("Weird state: %d\n", current_state);
  }
}


void print_keys( void )
{
  g_print("______________________________\n");
  g_print("Commands:\n");
  g_print("o/O\t: play/pause\n");
  g_print("q/Q\t: Quit\n");
  g_print("______________________________\n");
}

void
process_hotkey(player *play, char key)
{
  switch (key)
    {
  case 'q':
  case 'Q':
    g_print("Quit\n");
    gst_element_set_state(play->bin, GST_STATE_NULL);
    if (play->loop)
      g_main_loop_quit(play->loop);
    break;
  case 'o':
  case 'O':
    toggle_pause (play);
    break;
   default:
    break;
    }
}


gboolean
stdin_handler(GIOChannel *source, GIOCondition condition, gpointer data)
{
  __ADIT_UNUSED(source);

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

static char *fragment_shader =
"void main()"
"{"
"    gl_FragColor = vec4(0.0, 1.0, 0.0, 1.0);"
"}";

static char *vertex_shader =
"attribute vec2 a_pos;"

"void main()"
"{"
"    gl_Position = vec4(a_pos, 0.0, 1.0);"
"}";


float vertices[] = {
     0.0f,  0.5f, // Vertex 1 (X, Y)
     0.5f, -0.5f, // Vertex 2 (X, Y)
    -0.5f, -0.5f,  // Vertex 3 (X, Y)
};

void *gles_thread_func(void *data)
{
    GLenum res;
    GLuint fragmentShader, vertexShader, shaderProgram, vbo;
    GLint status, posAttrib;

    glDemo *p = (glDemo *)data;

    demo_create_wl_context(p);
    demo_create_ilm_context(p);
    demo_init_egl(p);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, (const GLchar **)&vertex_shader, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buffer[512];
        glGetShaderInfoLog(vertexShader, 512, NULL, buffer);
        g_print ("Failed to compile vertex shader:\n%s\n", buffer);

        return NULL;
    }

    res = glGetError();
    if (res != GL_NO_ERROR)
    {
        g_print("0.0 Error in gl operation: 0x%x\n", res);
        return NULL;
    }

    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, (const GLchar **)&fragment_shader, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buffer[512];
        glGetShaderInfoLog(fragmentShader, 512, NULL, buffer);
        g_print ("Failed to compile fragment shader:\n%s\n", buffer);

        return NULL;
    }

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glGetShaderiv(shaderProgram, GL_LINK_STATUS, &status);
    if (status != GL_TRUE)
    {
        char buffer[1024];
        glGetShaderInfoLog(shaderProgram, 1024, NULL, buffer);
        g_print ("Failed to link shader:\n%s\n", buffer);

        return NULL;
    }
    glUseProgram(shaderProgram);

    posAttrib = glGetAttribLocation(shaderProgram, "a_pos");
    glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 0, NULL);
    glEnableVertexAttribArray(posAttrib);

    g_print ("GLES thread up and running!\n");
    while (TRUE)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        glClearColor(1.0f, 1.0f, 1.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glDrawArrays(GL_TRIANGLES, 0, G_N_ELEMENTS(vertices));
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        eglSwapBuffers(p->eglDisplay, p->eglWindowSurface);
    }

    return NULL;
}

static GOptionEntry entries[] = {
    { "guidelines", 'g', 0, G_OPTION_ARG_NONE, &run_guidelines_thread, "Simulate guiding lines in a thread", NULL },
    { "video-layer-id", 'l', 0, G_OPTION_ARG_INT, &video_layer_id, "Layer id of video", NULL },
    { "video-surface-id", 's', 0, G_OPTION_ARG_INT, &video_surface_id, "Surface id of video", NULL },
    { "guideline-layer-id", 'm', 0, G_OPTION_ARG_INT, &guidelines_layer_id, "Layer id of guidelines", NULL },
    { "guideline-surface-id", 't', 0, G_OPTION_ARG_INT, &guidelines_surface_id, "Surface id of guidelines", NULL },
    { "pixel-format", 'p', 0, G_OPTION_ARG_INT, &pixel_format, "pixel format (6 = UYVY, 7 = YUYV/YUY2, 9 = NV12", NULL },
    { "force-memcpy", 'c', 0, G_OPTION_ARG_NONE, &force_memcpy, "Force memcpy rendering instead of buffer mapping", NULL },
    { NULL }
};

int
main(int argc, char *argv[])
{
  glDemo demo;
  GstBus * bus;
  player play;
  GOptionContext *context;
  GError *error = NULL;

  memset(&demo, 0, sizeof(demo));

  g_print("GStreamer Demo Player\n");
  print_keys();

  context = g_option_context_new ("- Simulate rearview camera use-case");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gst_init_get_option_group ());

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
      g_print ("Problem parsing commandline: %s\n", error->message);
      g_clear_error (&error);

      exit (1);
  }

  demo.layer_id = guidelines_layer_id;
  demo.surface_id = guidelines_surface_id;

  play.loop = g_main_loop_new (NULL, FALSE);
  play.sink = gst_element_factory_make ("gst_apx_sink", "sink");
  play.src = gst_element_factory_make ("mfw_v4lsrc", "src");
  play.queue = gst_element_factory_make ("queue", "queue");
  play.bin = gst_pipeline_new ("v4l");

  g_object_set (G_OBJECT(play.src), "pix-fmt", pixel_format, NULL);
  /* Set in separate call because g_object_set is atomic and older versions of
     the sink don't support this property */
  g_object_set (G_OBJECT(play.sink), "force-memcpy", force_memcpy, NULL);
  g_object_set (G_OBJECT(play.sink),
                  "layer-id", video_layer_id,
                  "surface-id", video_surface_id,
                  NULL);

  gst_bin_add_many (GST_BIN(play.bin), play.src, play.queue, play.sink, NULL);
  gst_element_link_many (play.src, play.queue, play.sink, NULL);

  GIOChannel * gio_read = g_io_channel_unix_new(fileno(stdin));
  g_io_add_watch(gio_read, G_IO_IN, stdin_handler, &play);

  bus = gst_pipeline_get_bus(GST_PIPELINE(play.bin));
  gst_bus_add_watch(bus, bus_call, &play);
  gst_object_unref(bus);

  gst_element_set_state(play.bin, GST_STATE_PLAYING);


  g_unix_signal_add (SIGINT, (GSourceFunc)g_main_loop_quit, play.loop);
  g_unix_signal_add (SIGTERM, (GSourceFunc)g_main_loop_quit, play.loop);

  if (run_guidelines_thread &&
      !g_thread_create(gles_thread_func, &demo, FALSE, NULL))
  {
    g_print("Failed to create thread\n");
  }

  g_print("Running...\n");
  g_main_loop_run(play.loop);

  g_print("Returned, stopping playback\n");
  gst_element_set_state(play.bin, GST_STATE_NULL);

  g_print("Deleting playbin\n");
  gst_object_unref(GST_OBJECT(play.bin));
  gst_object_unref(GST_OBJECT(bus));

  g_io_channel_unref (gio_read);

  return 0;
}

/*lint -restore*/

/* PRQA: Lint Message 749: deactivation because not all values of the GstPlayFlags enum are used*/
/*lint -save -e749 */
