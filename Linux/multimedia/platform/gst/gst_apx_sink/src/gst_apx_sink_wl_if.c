/**
* \file: gst_apx_sink_wl_if.c
*
* \version: $Id:$
*
* \release: $Name:$
*
* Render video images using wayland, GLESv2 and Vivante Extensions
* for mapping YUV data.
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


#include <glib-object.h>

#include "gstapx.h"
#include "gst_apx_sink_wl_if.h"

#include <wayland-client.h>
#include <apx.h>
#include <string.h>

/* PRQA: Lint Message 826, 160: deactivation because casting mechanism of GObject throws the finding */
/*lint -e826 -e160*/

void
RegistryHandleGlobal(void* data, struct wl_registry* registry, uint32_t name,
    const char* interface, uint32_t version)
{
  __ADIT_UNUSED(version);
  GstApxSink *sink = GST_APX_SINK(data);

  if (apx_global_handler(&sink->apx_a, registry, name, interface, version) < 0) {
      GST_ERROR_OBJECT(sink, "apx_global_handler error");
      g_assert_not_reached ();
  }

  if (!strcmp(interface, "wl_compositor"))
    {
      sink->wl_compositor = (struct wl_compositor*) (wl_registry_bind(registry,
          name, &wl_compositor_interface, 1));
      GST_LOG("sink: %p\n sink->wl_compositor: %p", sink, sink->wl_compositor);
    }
}

static struct wl_registry_listener registryListener = {
    RegistryHandleGlobal,
    NULL
};

static gboolean
create_wl_surface(GstApxSink *sink)
{
    sink->wl_surface = wl_compositor_create_surface(sink->wl_compositor);
    if(sink->wl_surface == NULL)
    {
        GST_ERROR("wl_compositor_create_surface failed");
        __apx_log_errmem("APX_SINK: wl_compositor_create_surface failed");
        return FALSE;
    }

    return TRUE;
}

static gboolean
create_wl_context(GstApxSink * sink)
{
    sink->wl_display = wl_display_connect(NULL);
    if (sink->wl_display == NULL)
    {
        GST_ERROR_OBJECT(sink, "wl_display_connect failed");
        __apx_log_errmem("APX_SINK: wl_display_connect failed");
        return FALSE;
    }

    GST_TRACE_OBJECT(sink, "Successfully connected to wayland display %p", sink->wl_display);

    sink->wl_registry = wl_display_get_registry(sink->wl_display);
    wl_registry_add_listener(sink->wl_registry, &registryListener, sink);
    wl_display_dispatch(sink->wl_display);

    wl_display_roundtrip(sink->wl_display);
    if(sink->wl_compositor == NULL)
    {
      GST_ERROR_OBJECT(sink, "Failed to get compositor interface");
      __apx_log_errmem("APX_SINK: Failed to get compositor interface");
      return FALSE;
    }

    sink->shim_context = compositor_shim_initialize (sink->wl_display);
    if (sink->shim_context == NULL)
    {
        GST_ERROR_OBJECT(sink, "compositor_shim_initialize error");
        return FALSE;
    }

    // APX
    if (apx_init(&sink->apx_a) < 0) {
        GST_ERROR_OBJECT(sink, "apx_init error");
        __apx_log_errmem("APX_SINK: apx_init error");
        sink->apx_init = FALSE;
        return FALSE;
    }
    else
        sink->apx_init = TRUE;

    return TRUE;
}

gboolean
gst_apx_sink_wl_if_create_apx_context(GstApxSink * sink)
{
    if (!create_wl_surface (sink)) {
        GST_ERROR_OBJECT (sink, "Failed to create new Wayland surface");

        return FALSE;
    }
/* Modification for APX: We allocate surfaces the size of the video and
 * we set the destination as smaller. We rely on the layer manager to perform
 * the scaling. */
	uint32_t width = sink->display_width, height = sink->display_height;
	uint32_t src_x = sink->crop_left, src_y = sink->crop_top, dest_x, dest_y;
	uint32_t src_width, src_height;
	uint32_t dst_width, dst_height;
	int adapter_error = 0;

	/* Simulate negative X and Y offsets by calculating a different source
	 * rectangle
	 */
	if (sink->x_offset >= 0)
	{
	  dest_x = sink->x_offset;
	  dst_width = width;
	}
	else
	{
	  src_x += ((guint)(-sink->x_offset)) * sink->video_width / sink->display_width;
	  dest_x = 0;
	  dst_width = width + sink->x_offset;
	}

	if (sink->y_offset >= 0)
	{
	  dest_y = sink->y_offset;
	  dst_height = height;
	}
	else
	{
	  src_y += ((guint)(-sink->y_offset)) * sink->video_height / sink->display_height;
	  dest_y = 0;
	  dst_height = height + sink->y_offset;
	}

	src_width = (sink->video_width - src_x) - sink->crop_right;
	src_height = (sink->video_height - src_y) - sink->crop_bottom;

	GST_DEBUG_OBJECT(sink, "Source rectangle: %d %d %d %d",
	    src_x, src_y, src_width, src_height);
	GST_DEBUG_OBJECT(sink, "Destination rectangle: %d %d %d %d",
	    dest_x, dest_y, dst_width, dst_height);

	sink->shim_surface_context.wlSurface = sink->wl_surface;

	sink->shim_surface_context.surface_id = sink->surface_id;
	sink->shim_surface_context.layer_id = sink->layer_id;

	sink->shim_surface_context.origSourceWidth = sink->video_width;
	sink->shim_surface_context.origSourceHeight = sink->video_height;
	sink->shim_surface_context.sourceWidth = src_width;
	sink->shim_surface_context.sourceHeight = src_height;
	sink->shim_surface_context.destWidth = dst_width;
	sink->shim_surface_context.destHeight = dst_height;

	sink->shim_surface_context.sourceX = src_x;
	sink->shim_surface_context.sourceY = src_y;
	sink->shim_surface_context.destX = dest_x;
	sink->shim_surface_context.destY = dest_y;

	sink->shim_surface_context.visibility = 1;
	sink->shim_surface_context.opacity = 1.0f;

	if (sink->layer_id > 0)
	{
		adapter_error = compositor_shim_surface_configure (sink->shim_context, &sink->shim_surface_context,
													   ADAPTER_CONFIGURATION_ALL);
	}
	else
	{
		adapter_error = compositor_shim_surface_configure (sink->shim_context, &sink->shim_surface_context,
													   ADAPTER_CONFIGURATION_CREATE);
	}
	if (adapter_error != 0)
	{
		GST_ERROR_OBJECT(sink, "compositor_shim_surface_configure() failed");
	    return FALSE;
	}

   return TRUE;
}

gboolean
gst_apx_sink_wl_if_init(GstApxSink * sink)
{
  if (compositor_shim_wait_server () < 0) {
      GST_ERROR_OBJECT(sink, "Wayland Server does not respond");
      sink->compositor_shim_wait_server = FALSE;
      gst_apx_sink_wl_if_close(sink);
      return FALSE;
  }
  else
      sink->compositor_shim_wait_server = TRUE;

  if (!create_wl_context(sink))
    {
      GST_ERROR_OBJECT(sink, "createWlContext failed");
      gst_apx_sink_wl_if_close(sink);
      return FALSE;
    }

  return TRUE;
}

static void
destroy_wl_surface (GstApxSink *sink)
{
    int error;

    if (NULL != sink->shim_surface_context.shim_ctx) {
        error = compositor_shim_surface_destroy(&sink->shim_surface_context);
        if (error != 0)
        {
            GST_ERROR_OBJECT(sink, "compositor_shim_surface_destroy failed");
        }
    }

    /* PRQA: Lint Message 751 disabled because of GLib assertion mechanism throws the finding */
/*lint -e751 */
    g_clear_pointer (&(sink->wl_surface), wl_surface_destroy);
/*lint +e751 */
}

void
gst_apx_sink_wl_if_stop(GstApxSink *sink)
{
    destroy_wl_surface (sink);
}

void
gst_apx_sink_wl_if_close(GstApxSink * sink)
{
  destroy_wl_surface (sink);

  /* Compositor */
  if (NULL != sink->shim_context)
      compositor_shim_terminate(sink->shim_context);

  /*
   *  Need to call ilm_destroy() in-order
   *  to balance out ilm_init() called
   *  from compositor_shim_wait_server()
   */
  if (sink->compositor_shim_wait_server) {
      compositor_shim_close_server();
      sink->compositor_shim_wait_server = FALSE;
  }

  if (sink->apx_init) {
      if (apx_deinit(&sink->apx_a) < 0) {
          GST_ERROR("apx_deinit failed");
	  }

      sink->apx_init = FALSE;
  }

  /* PRQA: Lint Message 751 disabled because of GLib assertion mechanism throws the finding */
/*lint -e751 */
  g_clear_pointer (&(sink->wl_compositor), wl_compositor_destroy);
  g_clear_pointer (&(sink->wl_registry), wl_registry_destroy);
  g_clear_pointer (&(sink->wl_display), wl_display_disconnect);
/*lint +e751 */
}

/*lint +e826 +e160*/
