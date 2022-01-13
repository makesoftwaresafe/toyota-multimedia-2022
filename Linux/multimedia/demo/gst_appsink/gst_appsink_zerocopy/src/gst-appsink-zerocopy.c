#define GST_USE_UNSTABLE_API

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <gst/allocators/gstdmabuf.h>
#include <gst/app/gstappsink.h>
#include <pthread.h>
#include <semaphore.h>
#include <assert.h>

#include <GLES/gl.h>
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>

#include <drm.h>
#include <xf86drm.h>
#include <drm_fourcc.h>

#include "ivi-application-client-protocol.h"

#include <wayland-client.h>
#include <wayland-egl.h>
#include <wayland-cursor.h>

#define NANOSEC_PER_SEC    1000000000L

#ifdef DEBUG
# define error(FORMAT, ...)	fprintf(stderr, "[Error]: %s: " FORMAT "\n", __func__, ## __VA_ARGS__)
# define debug(FORMAT, ...)	fprintf(stderr, "[Debug]: %s: " FORMAT "\n", __func__, ## __VA_ARGS__)
#else
# define error(FORMAT, ...) fprintf(stderr, "[Error]: %s: " FORMAT "\n", __func__, ## __VA_ARGS__)
/*# define debug(FORMAT, ...)	fprintf(stderr, "[Debug]: %s: " FORMAT "\n", __func__, ## __VA_ARGS__)*/
# define debug(FORMAT, ...) do { } while (0)
#endif

#define IVI_SURFACE_ID 6000

struct display {
    struct wl_display* display;
    struct wl_registry* registry;
    struct wl_compositor *compositor;
    struct wl_surface *surface;
    struct {
        EGLDisplay dpy;
        EGLContext ctx;
        EGLConfig conf;
        EGLSurface surf;
        EGLImageKHR img;
        PFNEGLCREATEIMAGEKHRPROC create_image;
        PFNEGLDESTROYIMAGEKHRPROC destroy_image;
        PFNGLEGLIMAGETARGETTEXTURE2DOESPROC image_texture_2d;
    } egl;
	struct {
        GLuint vertex_shader;
        GLuint fragment_shader;
        GLuint program_object;
        GLuint texture;
    } gl;
    struct wl_egl_window *native;
    struct ivi_application *ivi_application;
    struct ivi_surface *ivi_surface;
};

struct gstApplContext
{
    GstElement *queue;
    GstElement *queue_2;
    GMainLoop *loop;
    GstElement *pipeline;
    GstElement *src;
    GstElement *demux;
    GstElement *videoparse;
    GstElement *videodec;
    GstElement *sink;
    GstElement *vsp;
    GstBus *bus;
    GstVideoMeta *vmeta;
    struct display *disp;
    sem_t rndr_start_obj;
    sem_t thread_sync_obj;
    GstBuffer *buffer;
    GstSample *sample;
    int dma_fd;
    int width;
    int height;
    char exit_render_thread;
};

static const gchar *vertex_shader_str =
    "attribute vec4 a_position;   \n"
    "attribute vec2 a_texCoord;   \n"
    "varying vec2 v_texCoord;     \n"
    "void main()                  \n"
    "{                            \n"
    "   gl_Position = a_position; \n"
    "   v_texCoord = a_texCoord;  \n"
    "}                            \n";

static const gchar *fragment_shader_str =
    "#ifdef GL_ES                                          \n"
    "precision mediump float;                              \n"
    "#endif                                                \n"
    "varying vec2 v_texCoord;                              \n"
    "uniform sampler2D tex;                                \n"
    "const float PI = 3.1415926535;                        \n"
    "void main()                                           \n"
    "{                                                     \n"
    "  float aperture = 178.0;                             \n"
    "  float apertureHalf = 0.5 * aperture * (PI / 180.0); \n"
    "  float maxFactor = sin(apertureHalf);                \n"
    "  vec2 uv;                                            \n"
    "  vec2 xy = 2.0 * v_texCoord.xy - 1.0;                \n"
    "  float d = length(xy);                               \n"
    "  if (d < (2.0-maxFactor))                            \n"
    "  {                                                   \n"
    "     d = length(xy * maxFactor);                      \n"
    "     float z = sqrt(1.0 - d * d);                     \n"
    "     float r = atan(d, z) / PI;                       \n"
    "     float phi = atan(xy.y, xy.x);                    \n"
    "     uv.x = r * cos(phi) + 0.5;                       \n"
    "     uv.y = r * sin(phi) + 0.5;                       \n"
    "     gl_FragColor = texture2D(tex, uv);               \n"
    "  }                                                   \n"
    "  else                                                \n"
    "  {                                                   \n"
    "     gl_FragColor = vec4(0.0,0.0,0.0,1.0);            \n"
    "  }                                                   \n"
    "}                                                     \n";

static gboolean
bus_call (GstBus     *bus,
          GstMessage *msg,
          gpointer    data)
{
  struct gstApplContext* gst_ctx =
          (struct gstApplContext *) data;
  GMainLoop *loop = (GMainLoop *) gst_ctx->loop;

  bus = bus;

  switch (GST_MESSAGE_TYPE (msg)) {

    case GST_MESSAGE_EOS:
      g_print ("End of stream\n");
      g_main_loop_quit (loop);
      break;

    case GST_MESSAGE_ERROR: {
      gchar  *debug;
      GError *error;

      gst_message_parse_error (msg, &error, &debug);
      g_free (debug);

      g_printerr ("Error: %s\n", error->message);
      g_error_free (error);

      g_main_loop_quit (loop);
      break;
    }
    default:
      break;
  }

  return TRUE;
}

static gint
get_dmabuf_fd (GstBuffer *buffer)
{
    GstMemory *mem = gst_buffer_peek_memory (buffer, 0);
    if (!gst_is_dmabuf_memory (mem)) {
        error("captured buffer is no dma\n");
        return -1;
    }

    return gst_dmabuf_memory_get_fd (mem);
}

static GstFlowReturn
on_new_sample_from_sink (GstElement* object, gpointer user_data)
{
    struct gstApplContext* gst_ctx =
            (struct gstApplContext *) user_data;
    GstSample *sample;
    GstBuffer *buffer;

    /* no need to process the buffer after
     * render thread is exited */
    if(gst_ctx->exit_render_thread)
        return GST_FLOW_CUSTOM_SUCCESS;

    sem_wait(&gst_ctx->thread_sync_obj);

    sample = gst_app_sink_pull_sample((GstAppSink*)((void*)object));
    buffer = gst_sample_get_buffer(sample);
    gst_ctx->vmeta = gst_buffer_get_video_meta(buffer);

    gst_ctx->dma_fd = get_dmabuf_fd(buffer);
    if (gst_ctx->dma_fd < 0) {
        error("dma_fd is invalid\n");
        return GST_FLOW_ERROR;
    }
    else
        debug("dma_fd:%d\n", gst_ctx->dma_fd);

    gst_buffer_ref(buffer);
    gst_ctx->buffer = buffer;
    gst_ctx->sample = sample;
    sem_post(&gst_ctx->rndr_start_obj);

    return GST_FLOW_CUSTOM_SUCCESS;
}

static void
registry_handle_global(void *data, struct wl_registry *registry,
               uint32_t name, const char *interface, uint32_t version)
{
    struct display *d = (struct display *)data;
        version = version;

    if (strcmp(interface, "wl_compositor") == 0) {
        d->compositor = (struct wl_compositor*)
        wl_registry_bind(registry, name,
                &wl_compositor_interface, 1);
    } else if (strcmp(interface, "ivi_application") == 0) {
        d->ivi_application = (struct ivi_application*)
        wl_registry_bind(registry, name,
                &ivi_application_interface, 1);
    }
}

static void
registry_handle_global_remove(void *data, struct wl_registry *registry,
            uint32_t name)
{
    data = data;
    registry = registry;
    name = name;
}

static const struct wl_registry_listener registry_listener = {
	registry_handle_global,
	registry_handle_global_remove
};

void init_wayland(struct display *disp)
{
    disp->display = wl_display_connect(NULL);
    assert(disp->display);

    disp->registry = wl_display_get_registry(disp->display);
    wl_registry_add_listener(disp->registry,
            &registry_listener, disp);
    wl_display_roundtrip(disp->display);
}

static void
init_egl(struct display *disp)
{
    EGLint config_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };

    static const EGLint context_attribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL_NONE
    };

    EGLint major, minor, count;
    EGLBoolean ret;

    disp->egl.dpy = eglGetDisplay(disp->display);
    assert(disp->egl.dpy);

    ret = eglInitialize(disp->egl.dpy, &major, &minor);
    assert(ret == EGL_TRUE);
    ret = eglBindAPI(EGL_OPENGL_ES_API);
    assert(ret == EGL_TRUE);

    ret = eglChooseConfig(disp->egl.dpy, config_attribs,
            &disp->egl.conf, 1, &count);
    assert(ret && count >= 1);

    disp->egl.ctx = eglCreateContext(disp->egl.dpy,
                        disp->egl.conf,
                        EGL_NO_CONTEXT, context_attribs);
    assert(disp->egl.ctx);

    eglSwapInterval(disp->egl.dpy, 1);

    disp->egl.create_image =
            (void *) eglGetProcAddress("eglCreateImageKHR");
    assert(disp->egl.create_image);

    disp->egl.image_texture_2d =
            (void *) eglGetProcAddress("glEGLImageTargetTexture2DOES");
    assert(disp->egl.image_texture_2d);

    disp->egl.destroy_image =
            (void *) eglGetProcAddress("eglDestroyImageKHR");
    assert(disp->egl.destroy_image);
}

static void
create_surface(struct gstApplContext* gst_ctx)
{
    struct display* disp = gst_ctx->disp;
    uint32_t id_ivisurf = IVI_SURFACE_ID +
            (uint32_t)getpid();
    EGLBoolean ret;

    disp->surface = wl_compositor_create_surface(disp->compositor);
    assert(disp->surface);

    disp->native = wl_egl_window_create(disp->surface,
            gst_ctx->width, gst_ctx->height);
    assert(disp->native);

    disp->egl.surf = eglCreateWindowSurface(disp->egl.dpy,
            disp->egl.conf,
            (NativeWindowType)disp->native, NULL);

    disp->ivi_surface = ivi_application_surface_create(disp->ivi_application,
            id_ivisurf, disp->surface);

    if (disp->ivi_surface == NULL) {
        error("Failed to create ivi_client_surface\n");
        abort();
    }

    ret = eglMakeCurrent(disp->egl.dpy, disp->egl.surf,
            disp->egl.surf, disp->egl.ctx);
    assert(ret == EGL_TRUE);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(disp->egl.dpy, disp->egl.surf);
}

void deinit(struct display *disp)
{
    glDeleteShader(disp->gl.vertex_shader);
    glDeleteShader(disp->gl.fragment_shader);
    glDeleteProgram(disp->gl.program_object);
    glDeleteTextures(1, &disp->gl.texture);

    eglMakeCurrent(disp->egl.dpy, EGL_NO_SURFACE,
            EGL_NO_SURFACE, EGL_NO_CONTEXT);
    wl_egl_window_destroy(disp->native);
    ivi_surface_destroy(disp->ivi_surface);
    wl_surface_destroy(disp->surface);

    eglTerminate(disp->egl.dpy);
    eglReleaseThread();

    if (disp->ivi_application)
        ivi_application_destroy(disp->ivi_application);
    if (disp->compositor)
        wl_compositor_destroy(disp->compositor);
    if (disp->registry)
        wl_registry_destroy(disp->registry);

    wl_display_flush(disp->display);
    wl_display_disconnect(disp->display);
}

static int
drm_fourcc_from_gst_format(GstVideoFormat format)
{
    switch (format) {
        case GST_VIDEO_FORMAT_RGB16:
        case GST_VIDEO_FORMAT_BGR16:
            debug("DRM_FORMAT_RGB565\n");
            return DRM_FORMAT_RGB565;

        case GST_VIDEO_FORMAT_RGB:
        case GST_VIDEO_FORMAT_BGR:
            debug("DRM_FORMAT_RGB565\n");
            return DRM_FORMAT_BGR888;

        case GST_VIDEO_FORMAT_RGBA:
        case GST_VIDEO_FORMAT_RGBx:
        case GST_VIDEO_FORMAT_BGRA:
        case GST_VIDEO_FORMAT_BGRx:
        case GST_VIDEO_FORMAT_ARGB:
        case GST_VIDEO_FORMAT_xRGB:
        case GST_VIDEO_FORMAT_ABGR:
        case GST_VIDEO_FORMAT_xBGR:
        case GST_VIDEO_FORMAT_AYUV:
            debug("DRM_FORMAT_ABGR8888\n");
            return DRM_FORMAT_ARGB8888;

        case GST_VIDEO_FORMAT_GRAY8:
            debug("DRM_FORMAT_R8\n");
            return DRM_FORMAT_R8;

        case GST_VIDEO_FORMAT_YUY2:
        case GST_VIDEO_FORMAT_UYVY:
        case GST_VIDEO_FORMAT_GRAY16_LE:
        case GST_VIDEO_FORMAT_GRAY16_BE:
            debug("DRM_FORMAT_GR88\n");
            return DRM_FORMAT_GR88;

        case GST_VIDEO_FORMAT_I420:
        case GST_VIDEO_FORMAT_YV12:
        case GST_VIDEO_FORMAT_Y41B:
        case GST_VIDEO_FORMAT_Y42B:
        case GST_VIDEO_FORMAT_Y444:
            debug("DRM_FORMAT_R8\n");
            return DRM_FORMAT_R8;

        default:
            error("Unsupported format for DMABuf format:%s.",
                    gst_video_format_to_string(format));
            return -1;
    }
}

/*
 * TODO: Currently Single planar formats only supported
 * And Work is in progress for the multi-planar
 * formats
 * */
EGLImageKHR create_eglimage(struct gstApplContext* gst_ctx)
{
    struct display* disp = gst_ctx->disp;
    GstVideoMeta *vmeta = gst_ctx->vmeta;
    uint32_t n_planes = vmeta->n_planes;
    EGLint attribs[30];
    int fourcc;
    int atti = 0;

    fourcc = drm_fourcc_from_gst_format(vmeta->format);

    debug("planes:%d width:%d height:%d dma_fd:%d fourcc:0x%x\n", n_planes,
            vmeta->width, vmeta->height, gst_ctx->dma_fd,
            fourcc);

    attribs[atti++] = EGL_WIDTH;
    attribs[atti++] = vmeta->width;
    attribs[atti++] = EGL_HEIGHT;
    attribs[atti++] = vmeta->height;
    attribs[atti++] = EGL_LINUX_DRM_FOURCC_EXT;
    attribs[atti++] = fourcc;

    if (n_planes > 0) {
        attribs[atti++] = EGL_DMA_BUF_PLANE0_FD_EXT;
        attribs[atti++] = gst_ctx->dma_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE0_OFFSET_EXT;
        attribs[atti++] = vmeta->offset[0];
        attribs[atti++] = EGL_DMA_BUF_PLANE0_PITCH_EXT;
        attribs[atti++] = vmeta->stride[0];
    }

    if (n_planes > 1) {
        attribs[atti++] = EGL_DMA_BUF_PLANE1_FD_EXT;
        attribs[atti++] = gst_ctx->dma_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE1_OFFSET_EXT;
        attribs[atti++] = vmeta->offset[1];
        attribs[atti++] = EGL_DMA_BUF_PLANE1_PITCH_EXT;
        attribs[atti++] = vmeta->stride[1];
    }

    if (n_planes > 2) {
        attribs[atti++] = EGL_DMA_BUF_PLANE2_FD_EXT;
        attribs[atti++] = gst_ctx->dma_fd;
        attribs[atti++] = EGL_DMA_BUF_PLANE2_OFFSET_EXT;
        attribs[atti++] = vmeta->offset[2];
        attribs[atti++] = EGL_DMA_BUF_PLANE2_PITCH_EXT;
        attribs[atti++] = vmeta->stride[2];
    }

    attribs[atti++] = EGL_NONE;

    return disp->egl.create_image(disp->egl.dpy, EGL_NO_CONTEXT,
            EGL_LINUX_DMA_BUF_EXT, NULL, attribs);
}

void render_frame(struct display *disp)
{
    GLfloat vVertices[] = { -1.0f, 1.0f, 1.0f, // Position 0
            0.0f, 0.0f, // TexCoord 0
            -1.0f, -1.0f, 1.0f, // Position 1
            0.0f, 1.0f, // TexCoord 1
            1.0f, -1.0f, 1.0f, // Position 2
            1.0f, 1.0f, // TexCoord 2
            1.0f, 1.0f, 1.0f, // Position 3, skewed a bit
            1.0f, 0.0f // TexCoord 3
    };
    GLushort indices[] = { 0, 1, 2, 0, 2, 3 };

    glClear(GL_COLOR_BUFFER_BIT);
    glUseProgram(disp->gl.program_object);

    // Load the vertex position
    GLint positionLoc = glGetAttribLocation(disp->gl.program_object, "a_position");
    glVertexAttribPointer(positionLoc, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), vVertices);
    // Load the texture coordinate
    GLint texCoordLoc = glGetAttribLocation(disp->gl.program_object, "a_texCoord");
    glVertexAttribPointer(texCoordLoc, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), &vVertices[3]);
    glEnableVertexAttribArray(positionLoc);
    glEnableVertexAttribArray(texCoordLoc);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, disp->gl.texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    disp->egl.image_texture_2d(GL_TEXTURE_2D, disp->egl.img);

    //Set the texture sampler to texture unit 0
    GLint tex = glGetUniformLocation(disp->gl.program_object, "tex");
    glUniform1i(tex, 0);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, indices);

    eglSwapBuffers(disp->egl.dpy, disp->egl.surf);
}

GLuint load_shader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    // Create the shader object
    shader = glCreateShader(type);
    if (shader == 0)
        return 0;
    // Load the shader source
    glShaderSource(shader, 1, &shaderSrc, NULL);
    // Compile the shader
    glCompileShader(shader);
    // Check the compile status
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char* infoLog = (char*)malloc (sizeof(char) * infoLen );
            glGetShaderInfoLog(shader, infoLen, NULL, infoLog);
            error("Error compiling shader:%s\n",infoLog);
            free(infoLog);
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

void init_gl(struct display *disp)
{
    GLint linked;

    // load vertext/fragment shader
    disp->gl.vertex_shader = load_shader(GL_VERTEX_SHADER, vertex_shader_str);
    disp->gl.fragment_shader = load_shader(GL_FRAGMENT_SHADER, fragment_shader_str);

    // Create the program object
    disp->gl.program_object = glCreateProgram();
    if (disp->gl.program_object == 0)
    {
       error("error program object\n");
       return;
    }

    glAttachShader(disp->gl.program_object, disp->gl.vertex_shader);
    glAttachShader(disp->gl.program_object, disp->gl.fragment_shader);
    // Bind vPosition to attribute 0
    glBindAttribLocation(disp->gl.program_object, 0, "a_position");
    // Link the program
    glLinkProgram(disp->gl.program_object);
    // Check the link status
    glGetProgramiv(disp->gl.program_object, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(disp->gl.program_object, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char* infoLog = (char*)malloc(sizeof(char) * infoLen);
            glGetProgramInfoLog(disp->gl.program_object, infoLen, NULL, infoLog);
            error("Error linking program:%s\n", infoLog);
            free(infoLog);
        }
        glDeleteProgram(disp->gl.program_object);
    }

    glGenTextures(1, &disp->gl.texture);

    return;
}

void* render_thread(void * arg)
{
    struct gstApplContext* gst_ctx =
            (struct gstApplContext *) arg;
    struct display* disp;
    struct timespec tspec;
    int ret;

    disp = calloc(1, sizeof(struct display));
    if (NULL == disp)
    {
        error("calloc failed for size:%ld\n",
                sizeof(struct display));
        return NULL;
    }
    gst_ctx->disp = disp;

    init_wayland(disp);
    init_egl(disp);
    create_surface(gst_ctx);
    init_gl(disp);
    sem_post(&gst_ctx->thread_sync_obj);

    while(!gst_ctx->exit_render_thread) {
        clock_gettime(CLOCK_REALTIME, &tspec);
        tspec.tv_nsec += 2000;
        if (tspec.tv_nsec > NANOSEC_PER_SEC)
        {
            tspec.tv_nsec -= NANOSEC_PER_SEC;
            tspec.tv_sec++;
        }

        errno = 0;
        ret = sem_timedwait(&gst_ctx->rndr_start_obj,
                &tspec);

        if((errno == ETIMEDOUT)||
            (errno == EINTR)||
            (-1 == ret)) {
            continue;
        }

        disp->egl.img = create_eglimage(gst_ctx);
        if (EGL_NO_IMAGE_KHR == disp->egl.img)
        {
            error("create_eglimage failed EGLError:0x%x\n",
                  eglGetError());
            sem_post(&gst_ctx->thread_sync_obj);
            break;
        }

        render_frame(disp);
        disp->egl.destroy_image(disp->egl.dpy, disp->egl.img);
        gst_buffer_unref(gst_ctx->buffer);
        gst_sample_unref(gst_ctx->sample);
        sem_post(&gst_ctx->thread_sync_obj);
    }

    deinit(disp);
    free(disp);

    return NULL;
}

static void
on_pad_added (GstElement *element,
              GstPad     *pad,
              gpointer    data)
{
    element = element;
    pad = pad;
    struct gstApplContext *gst_ctx =
            (struct gstApplContext *)data;
    gchar *name = gst_pad_get_name(pad);
    printf("%s: %s\n",__FUNCTION__,name);

    if (0 == strncmp(name, "video", sizeof("video")-1))
    {
        if (gst_ctx->queue)
            gst_element_link(gst_ctx->demux, gst_ctx->queue);
        else if (gst_ctx->videoparse)
            gst_element_link(gst_ctx->demux, gst_ctx->videoparse);
    }
    g_free(name);
}

void pad_removed(GstElement *element,
                 GstPad *pad,
                 gpointer data)
{
    element = element;
    struct gstApplContext *gst_ctx =
            (struct gstApplContext *)data;
    gchar * name = gst_pad_get_name(pad);
    printf("%s: %s\n",__FUNCTION__, name);

    if (0 == strncmp(name, "video", sizeof("video")-1))
    {
        if (gst_ctx->queue)
            gst_element_unlink(gst_ctx->demux, gst_ctx->queue);
        else if (gst_ctx->videoparse)
            gst_element_unlink(gst_ctx->demux, gst_ctx->videoparse);
    }
    g_free(name);
}

static void usage(char *argv[])
{
    printf("usage:%s [decode/capture] [stream-location/video-node]"
           "[width] [height]\n", argv[0]);
    printf("ex1:%s decode /opt/platform/unit_tests/01_189_libx264_1920x1080_6M.mp4 "
           "1920 1080\n", argv[0]);
    printf("ex2:%s capture /dev/video1 1920 1080\n", argv[0]);
    exit(0);
}

int main(int argc, char *argv[])
{
    struct gstApplContext gst_ctx = {0};
    pthread_t thrd_id;
    pthread_attr_t attr;
    int retval;
    uint8_t decode = 0;
    uint8_t capture = 0;
    GstCaps *caps;
    GstCaps *caps_1;

    if (argc < 5)
        usage(argv);

    if (!strcmp(argv[1], "decode"))
        decode = 1;
    else if (!strcmp(argv[1], "capture"))
        capture = 1;
    else
        usage(argv);

    gst_ctx.width = atoi(argv[3]);
    gst_ctx.height = atoi(argv[4]);

    sem_init(&gst_ctx.rndr_start_obj, 0, 0);
    sem_init(&gst_ctx.thread_sync_obj, 0, 0);

    pthread_attr_init(&attr);
    retval = pthread_create(&thrd_id, &attr, render_thread, &gst_ctx);
    if (retval < 0) {
        error("render thread creation failed\n");
        goto failure;
    }

    gst_init(&argc, &argv);
    gst_ctx.loop = g_main_loop_new (NULL, FALSE);
    gst_ctx.pipeline = gst_pipeline_new("pipeline");

    /* we add a message handler */
    gst_ctx.bus = gst_pipeline_get_bus((GstPipeline*)((void*)gst_ctx.pipeline));
    gst_bus_add_watch(gst_ctx.bus, bus_call, &gst_ctx);

    /*check for appsink*/
    gst_ctx.sink = gst_element_factory_make("appsink", "sink");
    if (!gst_ctx.sink) {
        error("appsink is not available, can't continue!!!\n");
        goto failure;
    }

    g_object_set (G_OBJECT(gst_ctx.sink), "emit-signals",
            TRUE, "sync", FALSE, NULL);
    g_signal_connect (gst_ctx.sink, "new-sample",
            G_CALLBACK (on_new_sample_from_sink), &gst_ctx);

    caps = gst_caps_new_simple ("video/x-raw",
                    "format", G_TYPE_STRING, "BGRA",
                    "width", G_TYPE_INT, gst_ctx.width,
                    "height", G_TYPE_INT, gst_ctx.height,
                    NULL);

    if (decode) {
        gst_ctx.src = gst_element_factory_make("filesrc", "src");
        g_object_set(G_OBJECT(gst_ctx.src), "location", argv[2], NULL);
        gst_ctx.demux = gst_element_factory_make("qtdemux", "demux");
        gst_ctx.videoparse = gst_element_factory_make("h264parse", "videoparse");

        if (!gst_ctx.src || !gst_ctx.demux || !gst_ctx.videoparse) {
            error("basic decode elements failed!!!\n");
        }

        gst_ctx.videodec = gst_element_factory_make("omxh264dec", "videodec");
        if (gst_ctx.videodec) { /*omxh264dec*/
            gst_ctx.queue = gst_element_factory_make("queue", "queue");
            g_object_set(G_OBJECT(gst_ctx.queue), "max-size-buffers", 4, NULL);

            gst_ctx.queue_2 = gst_element_factory_make("queue", "queue_2");

            gst_ctx.vsp = gst_element_factory_make("vspfilter", "vspfilter");

            if ( !gst_ctx.queue || !gst_ctx.queue_2 || !gst_ctx.vsp) {
                error("decode elements related to omxh264dec failed!!!\n");
            }

            gst_bin_add_many((GstBin*)((void*)gst_ctx.pipeline), gst_ctx.src, gst_ctx.demux,
                    gst_ctx.queue, gst_ctx.videoparse, gst_ctx.videodec, gst_ctx.queue_2,
                    gst_ctx.vsp, gst_ctx.sink, NULL);

            caps_1 = gst_caps_from_string("video/quicktime, variant=(string)iso");
            gst_element_link_filtered(gst_ctx.src, gst_ctx.demux, caps_1);

            gst_element_link_filtered(gst_ctx.vsp, gst_ctx.sink, caps);

            gst_element_link_many(gst_ctx.queue, gst_ctx.videoparse,
                    gst_ctx.videodec, gst_ctx.queue_2,gst_ctx.vsp, gst_ctx.sink, NULL);

            gst_caps_unref (caps);
            gst_caps_unref (caps_1);
        }
        else { /*mfxdecode*/
            gst_ctx.videodec = gst_element_factory_make("mfxdecode", "videodec");
            if (gst_ctx.videodec) {
                gst_bin_add_many((GstBin*)((void*)gst_ctx.pipeline), gst_ctx.src, gst_ctx.demux,
                        gst_ctx.videoparse, gst_ctx.videodec, gst_ctx.sink, NULL);
                gst_element_link(gst_ctx.src, gst_ctx.demux);
                if (!gst_element_link_many(gst_ctx.videoparse, gst_ctx.videodec,
                        gst_ctx.sink, NULL))
                    error("gst_element_link_many failed!!!\n");
            }
            else
                error("no valid decoder available!!!\n");
        }
    }
    else if (capture) {
        gst_ctx.src = gst_element_factory_make("v4l2src", "src");
        g_object_set(G_OBJECT(gst_ctx.src), "device", argv[2], NULL);
        g_object_set(G_OBJECT(gst_ctx.src), "io-mode", 4 /*dmabuf*/, NULL);

        gst_ctx.queue = gst_element_factory_make("queue", "queue");
        gst_ctx.vsp = gst_element_factory_make("vspfilter", "vspfilter");

        if (!gst_ctx.src || !gst_ctx.queue || !gst_ctx.vsp) {
            g_print("capture elements failed!!!\n");
        }

        gst_bin_add_many((GstBin*)((void*)gst_ctx.pipeline), gst_ctx.src, gst_ctx.queue,
                gst_ctx.vsp, gst_ctx.sink, NULL);
        caps_1 = gst_caps_from_string("video/x-raw, format=RGB16, interlace-mode=interleaved");
        gst_element_link_filtered(gst_ctx.src, gst_ctx.queue, caps_1);
        gst_element_link_filtered(gst_ctx.vsp, gst_ctx.sink, caps);

        gst_element_link_many(gst_ctx.queue, gst_ctx.vsp, gst_ctx.sink, NULL);

        gst_caps_unref (caps);
        gst_caps_unref (caps_1);
    }

    if (gst_ctx.demux) {
        g_signal_connect (gst_ctx.demux, "pad-added",
                G_CALLBACK (on_pad_added), &gst_ctx);
        g_signal_connect (gst_ctx.demux, "pad-removed",
                G_CALLBACK (pad_removed), &gst_ctx);
    }

    gst_element_set_state ((GstElement*)((void*)gst_ctx.pipeline), GST_STATE_PLAYING);

    g_main_loop_run (gst_ctx.loop);

    gst_element_set_state (gst_ctx.pipeline, GST_STATE_NULL);

failure:
    gst_ctx.exit_render_thread = 1;
    pthread_join(thrd_id, NULL);
    gst_object_unref (gst_ctx.bus);
    gst_object_unref ((GstObject*)((void*)gst_ctx.pipeline));
    g_main_loop_unref (gst_ctx.loop);

    exit(EXIT_SUCCESS);
}
