/**
* \file: gst_viv_buffer.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* Vivante YUV texture buffer for Gstreamer framework.
* This buffer is used to pass data between the gstreamer and VIVANTE
* graphics hardware
*
* \component: gst_viv_demo
*
* \author: Michael Methner ADITG/SW1 mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/



#include "gst_viv_buffer.h"

GST_DEBUG_CATEGORY_STATIC( gst_viv_buffer_debug);
#define GST_CAT_DEFAULT gst_viv_buffer_debug

GST_BOILERPLATE(GstVivBuffer, gst_viv_buffer, GstBuffer, GST_TYPE_BUFFER);

static GstBufferClass *viv_buffer_parent_class = NULL;



/**
* \func gst_viv_buffer_finalize
* This function is called when the recount drops to 0 by the gstreamer
* framework. The function calls the finalize method of the buffer object
* if set.
*
* \param buffer "This" pointer
*
*/
static void
gst_viv_buffer_finalize(GstVivBuffer * buffer)
{
  gboolean recycled = FALSE;

  g_return_if_fail(buffer != NULL);

  if (NULL != buffer->finalize)
    {
      recycled = (buffer->finalize)(buffer, buffer->parent);
    }

  if (!recycled)
    {
      GST_DEBUG("Buffer not recycled: %p", buffer);
      if (buffer->parent)
        {
          gst_object_unref(buffer->parent);
          buffer->parent = NULL;

        }
      if(GST_BUFFER_CAPS(buffer))
          {
          gst_caps_unref(GST_BUFFER_CAPS(buffer));
          GST_BUFFER_CAPS(buffer) = NULL;
          }
      /* TBD: free memory here */
    }
  return;
}



/**
* \func gst_viv_buffer_base_init
* Initialize the class
*
* \param gclass Pointer to GstVivBufferClass
* *
*/
static void
gst_viv_buffer_base_init(gpointer gclass)
{
  /* this function is intentionally blank */
  /* do not delete, it used by the get_type function */
  gclass = gclass;
}


/**
* \func gst_viv_buffer_init
* Initialize the buffer
*
* \param buffer "This" pointer
* \param klass Pointer to GstVivBufferClass
* *
*/
static void
gst_viv_buffer_init(GstVivBuffer * buffer, GstVivBufferClass * klass)
{
  klass = klass;

  GST_BUFFER(buffer)->data = NULL;
  buffer->parent = NULL;
  buffer->finalize = NULL;
  buffer->texName = 0;
}


/**
* \func gst_viv_buffer_class_init
* Initialize the buffer class
*
* \param klass Pointer to GstVivBufferClass
* *
*/
static void
gst_viv_buffer_class_init(GstVivBufferClass * klass)
{
  GST_DEBUG_CATEGORY_INIT(gst_viv_buffer_debug, "gst_viv_buffer", 0,
      "GstBuffer with Vivante YUV texture");

  GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS(klass);

  viv_buffer_parent_class = g_type_class_peek_parent(klass);

  mini_object_class->finalize
      = (GstMiniObjectFinalizeFunction) gst_viv_buffer_finalize;

  parent_class = g_type_class_peek_parent(klass);
}



/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "gst_viv_buffer"
#endif

