/**
* \file: gst_rawvideo_src.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* filereader for YUV raw files with support for gst_pad_alloc
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SWG mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
*
***********************************************************************/
#include "gst_rawvideo_src.h"
#include <glib.h>
#include <glib-object.h>
#include <string.h>
#include <stdio.h>


enum
{
  PROP_LOCATION = 1,
  PROP_WIDTH,
  PROP_HEIGHT,
  PROP_FORMAT,
  PROP_FRAMERATE_NOMINATOR,
  PROP_FRAMERATE_DENOMINATOR,
};



#ifndef PACKAGE
#define PACKAGE "gst_rawvideo_src"
#endif



static GstStaticPadTemplate src_factory =
  GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("video/x-raw-yuv, "
        "framerate = (fraction) [ 0, MAX ], "
        "width = (int) [ 1, MAX ], " "height = (int) [ 1, MAX ]")
  );


GST_DEBUG_CATEGORY_STATIC (gst_rawvideo_src_debug);
#define GST_CAT_DEFAULT gst_rawvideo_src_debug



GST_BOILERPLATE (GstRawvideoSrc,gst_rawvideo_src, GstPushSrc,
                GST_TYPE_PUSH_SRC);


static void gst_rawvideo_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_rawvideo_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);




static void
gst_rawvideo_src_base_init (gpointer gclass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (gclass);

  gst_element_class_set_details_simple(element_class,
    "gst_rawvideo_src",
    "Src/Video",
    "gst_rawvideo_src",
    "Michael Methner mmethner@de.adit-jv.com");

  gst_element_class_add_pad_template (element_class,
      gst_static_pad_template_get (&src_factory));
}

GstFlowReturn gst_rawvideo_src_create(GstPushSrc *src, GstBuffer **buf);
gboolean      gst_rawvideo_src_start(GstBaseSrc *src);
gboolean      gst_rawvideo_src_stop(GstBaseSrc *src);
GstCaps*      gst_rawvideo_src_get_caps(GstBaseSrc *src);

static void
gst_rawvideo_src_class_init (GstRawvideoSrcClass * klass)
{
  GObjectClass * gobject_class = (GObjectClass *) klass;
  GstBaseSrcClass * basesrc_class = (GstBaseSrcClass *) klass;
  GstPushSrcClass * pushsrc_class = (GstPushSrcClass *) klass;

  pushsrc_class->create = GST_DEBUG_FUNCPTR (gst_rawvideo_src_create);
  basesrc_class->start = GST_DEBUG_FUNCPTR (gst_rawvideo_src_start);
  basesrc_class->stop = GST_DEBUG_FUNCPTR (gst_rawvideo_src_stop);

  basesrc_class->get_caps = GST_DEBUG_FUNCPTR (gst_rawvideo_src_get_caps);
  gobject_class->set_property = GST_DEBUG_FUNCPTR (gst_rawvideo_src_set_property);
  gobject_class->get_property = GST_DEBUG_FUNCPTR (gst_rawvideo_src_get_property);

  g_object_class_install_property (gobject_class, PROP_LOCATION,
      g_param_spec_string ("location", "filename",
          "filename to read", NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS ));

  g_object_class_install_property (gobject_class, PROP_WIDTH,
        g_param_spec_int ("width", "width", "Image width in pixel",
            0,4096,320, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_HEIGHT,
        g_param_spec_int ("height", "height", "Image height in pixel",
            0,4096,240, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FRAMERATE_NOMINATOR,
        g_param_spec_uint ("framerate-nominator", "framerate-nominator", "framerate-nominator",
            0,G_MAXUINT,30, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FRAMERATE_DENOMINATOR,
          g_param_spec_uint ("framerate-denominator", "framerate-denominator", "framerate-denominator",
              0,G_MAXUINT,1, G_PARAM_READWRITE));

  g_object_class_install_property (gobject_class, PROP_FORMAT,
        g_param_spec_uint ("format", "format", "Pixel format",
            0,G_MAXUINT,GST_STR_FOURCC("I420"), G_PARAM_READWRITE));
}


GstFlowReturn gst_rawvideo_src_create(GstPushSrc *src, GstBuffer **buf)
{
  GstRawvideoSrc * element = GST_RAWVIDEO_SRC(src);
  gint size = element->width*element->height*element->bitwidth;

  if(!element->caps)
    element->caps = gst_rawvideo_src_get_caps(GST_BASE_SRC(src));

  GstFlowReturn retval = gst_pad_alloc_buffer(GST_BASE_SRC_PAD(src),
       0, size ,  gst_caps_ref(element->caps), buf);

  if(retval==GST_FLOW_OK)
    {

      (*buf)->timestamp = element->next_timestamp;
      element->next_timestamp = element->next_timestamp +
          GST_SECOND * element->framerate_denominator / element->framerate_nominator ;

      gint ret = fread((*buf)->data,sizeof(char),size,element->fid );
      if(ret!=size)
        {
          GST_LOG("seeking to beginning of file");
          rewind(element->fid);
          gint ret2 = fread((*buf)->data,sizeof(char),size,element->fid );
          if(ret2!=size)
            {
              GST_ERROR("could not read file");
              retval = GST_FLOW_ERROR;
            }
        }
    }
  return retval;
}

GstCaps*      gst_rawvideo_src_get_caps(GstBaseSrc *src)
{
  GstRawvideoSrc * element = GST_RAWVIDEO_SRC(src);

  GstCaps *caps
      = gst_caps_new_simple("video/x-raw-yuv",
          "format", GST_TYPE_FOURCC, element->format,
          "width", G_TYPE_INT, element->width,
          "height", G_TYPE_INT, element->height,
          "framerate", GST_TYPE_FRACTION, element->framerate_nominator,
          element->framerate_denominator, NULL);

  return caps;
}

gboolean gst_rawvideo_src_start(GstBaseSrc *src)
{
  gboolean retval = FALSE;
  GstRawvideoSrc * element = GST_RAWVIDEO_SRC(src);

  element->fid = fopen(element->filename,"rb");
  if(element->fid)
    {
      retval = TRUE;
    }
  else
    {
      GST_ERROR("could not open file");
    }
  return retval;
}


gboolean      gst_rawvideo_src_stop(GstBaseSrc *src)
{
  GstRawvideoSrc * element = GST_RAWVIDEO_SRC(src);
  if(element->fid)
    {
      fclose(element->fid);
      element->fid = NULL;
    }

  if(element->caps)
    {
      gst_caps_unref(element->caps);
      element->caps = NULL;
    }

  return TRUE;
}



static void
gst_rawvideo_src_init (GstRawvideoSrc * src,
    GstRawvideoSrcClass * gclass)
{
  gclass = gclass;
  src->fid =  NULL;
  src->filename = NULL;
  src->height = 2048;
  src->width = 2048;
  src->bitwidth = 1.5;
  src->format = GST_STR_FOURCC("I420");

  src->framerate_nominator = 30;
  src->framerate_denominator = 1;

  src->next_timestamp = 0;
}



static void
gst_rawvideo_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstRawvideoSrc * element = GST_RAWVIDEO_SRC(object);
  switch (prop_id)
  {
  case PROP_HEIGHT:
      element->height = g_value_get_int(value);
      break;
  case PROP_WIDTH:
      element->width = g_value_get_int(value);
      break;
  case PROP_LOCATION:
      if(element->filename)
        {
          g_free(element->filename);
        }
      element->filename = g_strdup(g_value_get_string (value));
      break;
  case PROP_FORMAT:
    {
      guint fourcc =  g_value_get_uint(value);
      if(fourcc == GST_STR_FOURCC("I420") ||
         fourcc == GST_STR_FOURCC("YV12") ||
         fourcc == GST_STR_FOURCC("NV12") ||
         fourcc == GST_STR_FOURCC("NV21"))
      {
        element->format = fourcc;
        element->bitwidth = 1.5;
      }
      else if ( fourcc == GST_STR_FOURCC("YUY2") ||
                fourcc == GST_STR_FOURCC("UYVY"))
      {
          element->format = fourcc;
          element->bitwidth = 2;
      }
      else
      {
          GST_ERROR("unsupported fourcc code");
      }
    }
    break;
  case PROP_FRAMERATE_NOMINATOR:
      element->framerate_nominator = g_value_get_uint(value);
      break;
  case PROP_FRAMERATE_DENOMINATOR:
      element->framerate_denominator = g_value_get_uint(value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


static void
gst_rawvideo_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstRawvideoSrc * element = GST_RAWVIDEO_SRC(object);
  switch (prop_id)
  {
    case PROP_WIDTH:
        g_value_set_int(value, element->width);
        break;
    case PROP_HEIGHT:
        g_value_set_int(value, element->height);
        break;
    case PROP_LOCATION:
          g_value_set_string(value, element->filename);
        break;
    case PROP_FORMAT:
        g_value_set_uint(value,element->format);
        break;
    case PROP_FRAMERATE_NOMINATOR:
        g_value_set_uint(value,element->framerate_nominator);
        break;
    case PROP_FRAMERATE_DENOMINATOR:
        g_value_set_int(value,element->framerate_denominator);
        break;
    default:
          G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
          break;
  }
}



static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_rawvideo_src_debug, "rawvideo_src",
      0, "Rawvideo Src");

  return gst_element_register (plugin, "rawvideo_src", GST_RANK_PRIMARY,
      GST_TYPE_RAWVIDEO_SRC);
 }


GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    "rawvideo_src",
    "Rawvideo src",
    plugin_init,
    "0.0.1",
    "Proprietary",
    "ADIT",
    "http://TBD/"
)


