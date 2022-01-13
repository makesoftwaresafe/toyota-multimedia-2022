/**
* \file: gst_apx_sink_render_util.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* Render video images using wayland and APX
*
* \component: multimedia/gst
*
* \author: Jens Georg ADITG/SWG jgeorg@de.adit-jv.com
*
* \copyright: (c) 2014 ADIT Corporation
*
* \history
* 0.1 Jens Georg Initial version
***********************************************************************/

#include <memory.h>

#include <gst/video/video.h>

#include <gst/fsl/gstbufmeta.h>

/* Extension of apx sink plugin to support mfw_v4lsrc */
#include <gst/video/video.h>
#include <linux/videodev2.h>
#include <gst/base/gstpushsrc.h>
#include <stdint.h>

#include <apx.h>

#include "gstapx.h"
#include "gst_apx_render_util.h"
#include "gst_apx_buffer.h"

#define MAX_APX_BUFFERS 20

/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

/* Forward declarations */
static inline gboolean
render_hardware_buffer(GstApxSink *sink, guint8* log, guint8* phy, struct apx_buffer **out);

static inline gboolean
render_software_buffer(GstApxSink *sink, GstBuffer *buf, struct apx_buffer **out);

static inline void
copy_plane(guint8 *dst, guint8 *src, unsigned src_height, unsigned dst_height,
           unsigned src_stride, unsigned dst_stride);

static int on_apx_buffer_unlocked(struct apx_buffer *buffer, void *data)
{
    if (data == NULL)
    {
        apx_buffer_destroy(buffer, NULL);
    }
    else
    {
        gst_buffer_unref (GST_BUFFER(data));
    }

	return 1;
}

static void on_destroy_func(void *data)
{
    if (GST_IS_BUFFER(data))
        gst_buffer_unref(GST_BUFFER(data));
}

gboolean
gst_apx_render_buffer(GstApxSink *sink, GstBuffer *buf, gboolean lock)
{
    GstBufferMeta * meta;
    struct apx_buffer *apx_buf = NULL;
    gboolean retval = FALSE;

    meta = GST_BUFFER_META(buf->_gst_reserved[G_N_ELEMENTS(buf->_gst_reserved)-1]);

    if (G_UNLIKELY(sink->force_memcpy))
    {
        retval = render_software_buffer(sink, buf, &apx_buf);
        if (retval && lock)
            apx_buffer_set_unlock_callback(apx_buf, on_apx_buffer_unlocked, NULL, NULL);
    }
    else if (meta && meta->physical_data)
    {
        retval = render_hardware_buffer(sink, GST_BUFFER_DATA(buf), meta->physical_data, &apx_buf);
        if (retval && lock) {
            apx_buffer_set_unlock_callback(apx_buf, on_apx_buffer_unlocked, gst_buffer_ref (buf),
                                           (ApxCallbackDestroyNotify) on_destroy_func);
        }
    }
    else if (GST_IS_APX_BUFFER(buf))
    {
		apx_buf = GST_APX_BUFFER(buf)->apx_buffer;
        GST_APX_BUFFER(buf)->apx_buffer = NULL;
        if (lock)
            apx_buffer_set_unlock_callback(apx_buf, on_apx_buffer_unlocked, NULL, NULL);
		retval = TRUE;
    }
    else
    {
        retval = render_software_buffer(sink, buf, &apx_buf);
        if (retval && lock)
            apx_buffer_set_unlock_callback(apx_buf, on_apx_buffer_unlocked, NULL, NULL);
    }

    if (retval && apx_buf)
    {
        if (apx_buffer_commit (&sink->apx_a, apx_buf, sink->wl_surface, 0, 0,
                               sink->display_width, sink->display_height,
                               sink->wl_display) < 0)
        {
            __apx_log_errmem("APX_SINK: Failed to commit APX buffer\n");
            retval = FALSE;
        }
    }
    else
    {
        /* Do nothing */
    }

    return retval && (apx_buf != NULL);
}

/* ================= Utility functions ====================== */

static inline void
copy_plane(guint8 *dst,
           guint8 *src,
           unsigned src_height,
           unsigned dst_height,
           unsigned src_stride,
           unsigned dst_stride)
{
  unsigned height;
  unsigned i = 0;

  height = MIN(src_height, dst_height);

  if (G_LIKELY(dst_stride == src_stride))
  {
    memcpy(dst, src, dst_stride * height);
  }
  else
  {
    for (i = 0; i < height; i++)
    {
      memcpy(&(dst[i * dst_stride]), &(src[i * src_stride]), src_stride);
    }
  }
}

/**
 * render_software_buffer_apx:
 * @sink: a #GstApxSink
 * @buf: a #GstBuffer to render
 *
 * Fallback rendering method using memcpy to a buffer allocated by
 * apx_buffer_create
 */
static inline gboolean
render_software_buffer(GstApxSink *sink, GstBuffer *buf, struct apx_buffer **apx_buf)
{
    guint8 *p[3] = {NULL, NULL, NULL};
    unsigned stride = 0, w = 0, h = 0;

    if (sink->apx_format != APX_PIXELFORMAT_RGBA_8888 &&
            sink->apx_format != APX_PIXELFORMAT_RGB_565 &&
            sink->apx_format != APX_PIXELFORMAT_YUY2 &&
            sink->apx_format != APX_PIXELFORMAT_UYVY &&
            sink->apx_format != APX_PIXELFORMAT_YV12 &&
            sink->apx_format != APX_PIXELFORMAT_NV12)
    {
        GST_ERROR_OBJECT(sink, "Unsupported pixel format for software buffers");

        return FALSE;
    }

    GST_TRACE_OBJECT(sink,
                     "memcpy rendering for buffer with caps %"GST_PTR_FORMAT,
                     GST_BUFFER_CAPS(buf));

	*apx_buf = apx_buffer_create_unbuffered (&sink->apx_a, sink->video_width,
											 sink->video_height, sink->apx_format);
    if (!*apx_buf)
    {
        GST_ERROR_OBJECT (sink, "Failed to create APX buffer");
        return FALSE;
    }

    if (apx_buffer_map(*apx_buf, (void **)p) < 0)
    {
        GST_ERROR_OBJECT (sink, "Failed to map APX buffer!");
        return FALSE;
    }

    apx_buffer_get_stride(*apx_buf, &stride);
    apx_buffer_get_size(*apx_buf, &w, &h);

    /* Single plane video formats */
    copy_plane(p[0], GST_BUFFER_DATA(buf), sink->video_height,
               h, sink->strides[0], stride);
    if (sink->apx_format == APX_PIXELFORMAT_NV12) {
        copy_plane(p[1], GST_BUFFER_DATA(buf) + w * h,
                   sink->video_height/2, h / 2, sink->strides[1], stride);
    }

    if (sink->apx_format == APX_PIXELFORMAT_YV12)
    {
        copy_plane(p[1], GST_BUFFER_DATA(buf) + (w * h) + (w * h / 4),
                   sink->video_height/2, h / 2, sink->strides[2], stride / 2);
        copy_plane(p[2], GST_BUFFER_DATA(buf) + w * h,
                   sink->video_height/2, h / 2, sink->strides[1], stride / 2);
    }

    apx_buffer_unmap (*apx_buf);

    return TRUE;
}

static inline gboolean
render_hardware_buffer(GstApxSink *sink, guint8* logical, guint8* phy,
                       struct apx_buffer **apx_buf)
{
    guint8 *p[3] = { NULL, NULL, NULL };
    guint8 *l[3] = { logical, NULL, NULL };

    *apx_buf = (struct apx_buffer *) g_hash_table_lookup (sink->buffers, phy);
    if (*apx_buf != NULL)
    {
        GST_TRACE_OBJECT(sink, "re-using old buffer %p -> %p", phy, apx_buf);

        return TRUE;
    }

    if (G_UNLIKELY (g_hash_table_size (sink->buffers) == MAX_APX_BUFFERS))
    {
        GST_ERROR_OBJECT(sink, "Got too many different buffers");

        return FALSE;
    }

    // We need to compute the various YUV pointers.
    /* FIXME: According to APX, these offsets are only used for checking the
       alignment... */
    switch (sink->apx_format) {
    case APX_PIXELFORMAT_YUY2:
    case APX_PIXELFORMAT_UYVY:
        // One plane.
        p[0] = phy;
        p[1] = p[2] = NULL;
        break;

    case APX_PIXELFORMAT_NV12:
        // Two planes.
        p[0] = phy;
        p[1] = (void *)(phy + sink->video_height * sink->strides[0]);
        p[2] = NULL;
        break;

    case APX_PIXELFORMAT_YV12:
        p[0] = phy;
        p[1] = (void *)(phy + sink->video_height * sink->strides[0]);
        p[2] = (void *)(p[1] + (sink->video_height / 2) * sink->strides[1]);
        break;
    default:
        GST_ERROR("Bad format %i!", sink->apx_format);
        return FALSE;
    }

    *apx_buf = apx_buffer_wrap(&sink->apx_a, (void **)p, (void **)l,
                               sink->video_width, sink->video_height,
                               sink->strides[0], sink->apx_format);

    if (!*apx_buf) {
        GST_ERROR("apx_buffer_wrap error");
        return FALSE;
    }

    g_hash_table_insert (sink->buffers, phy, *apx_buf);
    GST_TRACE_OBJECT(sink, "Added %p -> %p to buffer cache", phy, *apx_buf);

    return TRUE;
}

/*lint +e826 +e160*/
