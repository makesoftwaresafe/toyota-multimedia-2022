/**
* \file: main.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* Testapplication to demonstrate rendering of YUV video stream from
* gst_rawvideo_src with Vivante GL extension
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/


#include "EGL/egl.h"
#include "EGL/eglvivante.h"
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>

#include <gst/gst.h>
#include <glib.h>
#include <gst_viv_app_sink.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/time.h>


#define WIDTH  1024
#define HEIGHT 768


EGLDisplay eglDisplay = EGL_NO_DISPLAY;
EGLConfig eglConfig;
EGLContext eglContext = EGL_NO_CONTEXT;
EGLSurface eglWindowSurface = EGL_NO_SURFACE;
GLuint ShaderProgs;

GLuint texCoordLoc;
GLuint samplerLoc;
GLuint vPosLoc;
GLuint programObject;


int eglOpen(void)
{
  EGLBoolean ret;
  static const EGLint gl_context_attribs[] =
    {
      EGL_CONTEXT_CLIENT_VERSION, 2,
      EGL_NONE
    };
  
  static const EGLint s_configAttribs[] =
    {
      EGL_RED_SIZE, 5,
      EGL_GREEN_SIZE, 6,
      EGL_BLUE_SIZE, 5,
      EGL_ALPHA_SIZE, EGL_DONT_CARE,
      EGL_NONE
    };

  EGLint numConfigs;
  EGLint majorVersion;
  EGLint minorVersion;

  EGLNativeDisplayType native_display = (EGLNativeDisplayType)fbGetDisplay(NULL);
  EGLNativeWindowType  native_window  = (EGLNativeWindowType)fbCreateWindow(native_display, 0, 1, WIDTH, HEIGHT);

  eglBindAPI(EGL_OPENGL_ES_API);
  eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
  eglInitialize(eglDisplay, &majorVersion, &minorVersion);
  eglGetConfigs(eglDisplay, NULL, 0, &numConfigs);
  eglChooseConfig(eglDisplay, s_configAttribs, &eglConfig, 1, &numConfigs);

  eglWindowSurface = eglCreateWindowSurface(eglDisplay, eglConfig, native_window, NULL);
  if (eglWindowSurface == EGL_NO_SURFACE)
    {
      printf("eglCreateWindowSurface failed\n");
      return -1;
    }

  eglContext = eglCreateContext(eglDisplay, eglConfig, eglContext, gl_context_attribs);
  if (eglContext == EGL_NO_CONTEXT)
    {
      printf("eglCreateContext failed\n");
      return -1;
    }


  ret = eglMakeCurrent(eglDisplay, eglWindowSurface, eglWindowSurface, eglContext);
  if (ret == EGL_FALSE)
    {
      printf("eglMakeCurrent failed\n");
      return -1;
    }
  return 0;
}


void eglClose(void)
{
  eglMakeCurrent(eglDisplay, NULL, NULL, NULL);
  eglDestroyContext(eglDisplay, eglContext);
  eglDestroySurface(eglDisplay, eglWindowSurface);
  eglTerminate(eglDisplay);
}


GLuint LoadShader(const char *shaderSrc, GLenum type)
{
  GLuint shader;
  GLint compiled;

  shader = glCreateShader(type);
  if(shader==0)
    return 0;

  glShaderSource(shader, 1, &shaderSrc, NULL);
  glCompileShader(shader);
  glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

  if(!compiled)
    {
    GLint infoLen=0;
    
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
    if(infoLen>0)
      {
        char *infoLog = malloc(sizeof(char) * infoLen);
        glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
        printf("shader compile error:\n%s\n", infoLog);
        free(infoLog);
        glDeleteShader(shader);
        return 0;
    }
  }
  return shader;
}


int initShaders(void)
{
  GLuint vertexShader;
  GLuint fragmentShader;
  GLint linked;

  const char vShaderStr[] =
      "attribute vec4 vPosition;    \n"
      "attribute vec2 a_texCoord;   \n"
      "varying vec2 v_texCoord;     \n"
      "void main()                  \n"
      "{                            \n"
      "   gl_Position = vPosition;  \n"
      "   v_texCoord = a_texCoord;  \n"
      "}                            \n";

  const char fShaderStr[] =
      "precision mediump float;                            \n"
      "varying vec2 v_texCoord;                            \n"
      "uniform sampler2D s_texture;                        \n"
      "void main()                                         \n"
      "{                                                   \n"
      "  gl_FragColor = texture2D( s_texture, v_texCoord );\n"
      "}\n";

  vertexShader = LoadShader(vShaderStr, GL_VERTEX_SHADER);
  fragmentShader = LoadShader(fShaderStr, GL_FRAGMENT_SHADER);

  programObject = glCreateProgram();

  if(programObject==0)
    return -1;
  
  glAttachShader(programObject, vertexShader);
  glAttachShader(programObject, fragmentShader);

  glBindAttribLocation(programObject, 0, "vPosition");
  glBindAttribLocation(programObject, 1, "a_texCoord");

  glLinkProgram(programObject);

  glGetProgramiv(programObject, GL_LINK_STATUS, &linked);

  if(!linked){
    GLint infoLen=0;
    
    glGetShaderiv(programObject, GL_INFO_LOG_LENGTH, &infoLen);
    if(infoLen>0){
      char *infoLog = malloc(sizeof(char) * infoLen);
      
      glGetShaderInfoLog(programObject, infoLen, NULL, infoLog);
      printf("Error linking shader:\n%s\n", infoLog);
      free(infoLog);
      return -1;
    }
  }
  ShaderProgs = programObject;

  glClearColor(1.0f, 0.0f, 0.0f, 1.0f);

  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,
                    GL_NEAREST);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                    GL_NEAREST);

  glEnable(GL_TEXTURE_2D);

  return 0;
}


unsigned long long to_usec(struct timeval * ts)
{
  return ts->tv_sec * 1000000.0f + ts->tv_usec;
}


void
renderCallback(GstElement * element, GstPad *pad, gpointer user_data)
{
  pad = pad;
  element = element;
  GstVivAppSink * sink = GST_VIV_APP_SINK(user_data);
  GLuint texName = gst_viv_app_get_current_texname(sink);

  static struct timeval start;
  static gint cnt = 0;
  cnt++;

  static gboolean firstRun = TRUE;
  if(firstRun)
    {
      eglMakeCurrent(eglDisplay, eglWindowSurface, eglWindowSurface, eglContext);
      firstRun = FALSE;
      gettimeofday(&start, NULL);
    }

  if((cnt % 30) == 0)
    {
      struct timeval stop;
      gettimeofday(&stop, NULL);
      float framerate = (float)cnt/ (float)(to_usec(&stop)-to_usec(&start)) * 1000000.0f;
      printf("framerate:%f\n",framerate);
    }

  GLfloat vVertices[] = { -0.5f, -0.5f, 0.0f,
                           0.5f, -0.5f, 0.0f,
                          -0.5f,  0.5f, 0.0f,
                           0.5f,  0.5f, 0.0f };
  GLfloat tVertices[] = { 0.0f, 1.0f,
                          1.0f, 1.0f,
                          0.0f, 0.0f,
                          1.0f, 0.0f };


  glViewport(0, 0, WIDTH, HEIGHT);
  glClear(GL_COLOR_BUFFER_BIT);
  glUseProgram(ShaderProgs);

  vPosLoc = glGetAttribLocation ( programObject,"vPosition");
  texCoordLoc = glGetAttribLocation ( programObject, "a_texCoord" );
  samplerLoc = glGetUniformLocation ( programObject, "s_texture" );

  glVertexAttribPointer(vPosLoc, 3, GL_FLOAT, GL_FALSE, 0, vVertices);
  glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 0, tVertices);

  glEnableVertexAttribArray(vPosLoc);
  glEnableVertexAttribArray (texCoordLoc);
  glUniform1i ( samplerLoc, 0 );
  glEnable(GL_TEXTURE_2D);

  glBindTexture(GL_TEXTURE_2D, texName);
  glTexDirectInvalidateVIV(GL_TEXTURE_2D);

  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glFinish();
  eglSwapBuffers(eglDisplay, eglWindowSurface);
}



int main(int argc, char *argv[])
{
  GLuint gl_format = GL_VIV_UYVY;
  guint gst_format = GST_STR_FOURCC("UYVY");
  float bitwidth = 2; /* YUY2 and UYVY requires two bits per pixel,
                         change to 1.5 for YV12, NV12 and NV21*/

  gst_init(&argc, &argv);
  if(argc != 4)
    {
      printf("syntax: %s <filename> <width> <height>\n",argv[0]);
      return -1;
    }

  int i;
  int width = atoi(argv[2]);
  int height = atoi(argv[3]);

  if(eglOpen()<0)
    {
      printf("could not open egl\n");
      return -1;
    }

  if(initShaders()<0)
    {
      printf("could not init shader\n");
      return -1;
    }

  GLuint texName;
  GLvoid * imgPtr[3] = {NULL};

  GMainLoop * loop = g_main_loop_new (NULL, FALSE);
  GstElement * src = gst_element_factory_make("rawvideo_src", "src");
  GstElement * sink = gst_element_factory_make("gst_viv_app_sink", "sink");
  GstElement * queue = gst_element_factory_make("queue", "queue");

  GstElement * pipeline = gst_pipeline_new ("pipeline");

  g_signal_connect(sink, "render",
      G_CALLBACK (renderCallback), sink);

  for(i=0;i<4;i++)
    {
      glGenTextures(1, &texName);
      glBindTexture(GL_TEXTURE_2D, texName);
      glTexDirectVIV( GL_TEXTURE_2D, width, height, gl_format, &imgPtr[0]);
      printf("texName: %d imgPtr: %p %p %p\n", texName, imgPtr[0], imgPtr[1],imgPtr[2]);
      gst_viv_app_add_buffer(GST_VIV_APP_SINK(sink), imgPtr[0], texName, width*height*bitwidth);
    }

  eglMakeCurrent(eglDisplay, NULL, NULL, NULL);

  /* set file location */
  g_object_set (G_OBJECT (src), "location", argv[1], NULL);

  /* width in pixel */
  g_object_set (G_OBJECT (src), "width", width, NULL);

  /* height  in pixel */
  g_object_set (G_OBJECT (src), "height", height, NULL);
  g_object_set (G_OBJECT (src), "framerate-nominator", 30, NULL);
  g_object_set (G_OBJECT (src), "framerate-denominator", 1, NULL);
  g_object_set (G_OBJECT (src), "format", gst_format, NULL);


  gst_bin_add_many (GST_BIN (pipeline),src, queue, sink, NULL);
  gst_element_link_many (src, queue, sink,NULL);

  gst_element_set_state (pipeline, GST_STATE_PLAYING);

  g_main_loop_run (loop);

  eglClose();
  return 0;
}
