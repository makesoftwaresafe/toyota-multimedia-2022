/**
* \file: gst_v4l_m2m_buffer.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* \component: gst_v4l_csc
*
* \author: Michael Methner ADITG/SW1 mmethner@de.adit-jv.com
*
* \copyright: (c) 2003 - 2012 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
***********************************************************************/



#include "gst_v4l_m2m_buffer.h"

/* PRQA: Lint Message 826: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826*/

GST_DEBUG_CATEGORY_STATIC( gst_v4l_m2m_buffer_debug);
#define GST_CAT_DEFAULT gst_v4l_m2m_buffer_debug

/* PRQA: Lint Message 19, 123, 144, 751, 160: deactivation because macro is related to GStreamer framework */
/*lint -e19 -e123 -e144 -e751 -e160 */
GST_BOILERPLATE(GstV4lM2mBuffer, gst_v4l_m2m_buffer, GstBuffer, GST_TYPE_BUFFER);
/*lint +e19 +e123 +e144 +e751 +e160 */

static GstBufferClass *v4l_m2m_buffer_parent_class = NULL;



/**
* \func gst_v4l_m2m_buffer_finalize
* This function is called when the recount drops to 0 by the gstreamer
* framework. The function calls the finalize method of the buffer object
* if set.
*
* \param buffer "This" pointer
*
*/
static void
gst_v4l_m2m_buffer_finalize(GstV4lM2mBuffer * buffer)
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
    }
  return;
}



/**
* \func gst_v4l_m2m_buffer_base_init
* Initialize the class
*
* \param gclass Pointer to GstV4lM2mBufferClass
* *
*/
static void
gst_v4l_m2m_buffer_base_init(gpointer gclass)
{
  /* this function is intentionally blank */
  /* do not delete, it used by the get_type function */
  gclass = gclass;
}


/**
* \func gst_v4l_m2m_buffer_init
* Initialize the buffer
*
* \param buffer "This" pointer
* \param klass Pointer to GstV4lM2mBufferClass
* *
*/
static void
gst_v4l_m2m_buffer_init(GstV4lM2mBuffer * buffer, GstV4lM2mBufferClass * klass)
{
  klass = klass;
  buffer = buffer;
}


/**
* \func gst_v4l_m2m_buffer_class_init
* Initialize the buffer class
*
* \param klass Pointer to GstV4lM2mBufferClass
* *
*/
static void
gst_v4l_m2m_buffer_class_init(GstV4lM2mBufferClass * klass)
{
  GST_DEBUG_CATEGORY_INIT(gst_v4l_m2m_buffer_debug, "gst_v4l_m2m_buffer", 0,
      "GstBuffer with v4l index");

  GstMiniObjectClass *mini_object_class = GST_MINI_OBJECT_CLASS(klass);

  v4l_m2m_buffer_parent_class = g_type_class_peek_parent(klass);

  mini_object_class->finalize
      = (GstMiniObjectFinalizeFunction) gst_v4l_m2m_buffer_finalize;

  parent_class = g_type_class_peek_parent(klass);
}



/*lint +e826*/
