/**
 * \file: gstavatp.c
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * AVATP Gstreamer plugin definition.
 *
 * \component: gst-plugins-avatp
 *
 * \author: Jakob Harder / ADIT / SW1 / jharder@de.adit-jv.com
 *
 * \copyright (c) 2012 Advanced Driver Information Technology.
 * This code is developed by Advanced Driver Information Technology.
 * Copyright of Advanced Driver Information Technology, Bosch, and DENSO.
 * All rights reserved.
 *
 * \history
 * 0.1 - Jakob Harder - initial version
 *
 ***********************************************************************/

#include <gst/gst.h>
#include "gstavatpsink.h"
#include "gstavatpsrc.h"


#ifndef PACKAGE
#define PACKAGE "gst_avatp"
#endif

GST_DEBUG_CATEGORY(gst_avatp_debug);

static gboolean avatp_init(GstPlugin *plugin)
{
    if (!gst_element_register(plugin, "avatpsrc", GST_RANK_NONE,
            GST_TYPE_AVATPSRC))
        return FALSE;

    if (!gst_element_register(plugin, "avatpsink", GST_RANK_NONE,
            GST_TYPE_AVATPSINK))
        return FALSE;

	GST_DEBUG_CATEGORY_INIT(gst_avatp_debug, "gst_avatp", 0,
			"AVATP");

    return TRUE;
}

#if GST_CHECK_VERSION(1,0,0)
#define GST_AVATP_PLUGIN_NAME avatp
#else
#define GST_AVATP_PLUGIN_NAME "avatp"
#endif

GST_PLUGIN_DEFINE(
        GST_VERSION_MAJOR, GST_VERSION_MINOR,
        GST_AVATP_PLUGIN_NAME,
        "stream RTP media via AVATP",
        avatp_init,
        "0.2.1",
        "Proprietary",
        "ADIT",
        "ADIT")

