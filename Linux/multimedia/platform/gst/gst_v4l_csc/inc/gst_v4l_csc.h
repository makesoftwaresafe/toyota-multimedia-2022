/**
* \file: gst_caps_filter.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* V4L mem2mem based color conversion
*
* \component: multimedia/gst
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/

#ifndef __gst_v4l_csc_filter_H__
#define __gst_v4l_csc_filter_H__

#include <gst/gst.h>


G_BEGIN_DECLS

#define GST_TYPE_V4L_CSC \
  (gst_v4l_csc_get_type())
#define GST_V4L_CSC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_V4L_CSC,GstV4lCsc))
#define GST_V4L_CSC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_V4L_CSC,GstV4lCscClass))
#define GST_IS_V4L_CSC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_V4L_CSC))
#define GST_IS_V4L_CSC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_V4L_CSC))





typedef struct _GstV4lCsc GstV4lCsc;
typedef struct _GstV4lCscClass GstV4lCscClass;
typedef struct ColorPorps_t ColorProps;

struct ColorPorps_t
{
  gint   hue;
  gint   hue_offset;
  gint   brightness_offset;
  guint8 saturation;
  guint8 brightness;
  guint8 contrast;
  guint8 saturation_offset;
  gboolean update_cprops;
};

struct _GstV4lCsc
{
  GstElement element;

  GstPad * sinkpad;
  GstPad * srcpad;

  int fd;

  int sink_width;
  int sink_height;
  int sink_format;

  gint framerate_n;
  gint framerate_d;
  gint par_n;
  gint par_d;

  int src_width;
  int src_height;
  int src_format;
  float src_bpp;

  guint8 * sink_buffer_ptr;
  guint32 sink_buffer_size;

  guint8 * src_buffer_ptr;
  guint32 src_buffer_size;
  GstCaps * src_caps;

  gboolean init;
  gboolean running;

  gboolean pass_through;
  gboolean passthrough_mode;

  gboolean use_pad_alloc;
  gint num_output_frames;
  gchar * devicename;

  GMutex *pool_lock;
  GCond *pool_data;
  GSList *buffer_pool;

  gboolean hflip;
  gboolean vflip;

  ColorProps CProps;
};


struct _GstV4lCscClass
{
  GstElementClass parent_class;
};


GType gst_v4l_csc_get_type(void);



G_END_DECLS

#endif
