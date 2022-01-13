/**
 * \file: avatp_avtp.h
 *
 * \version: $Id:$
 *
 * \release: $Name:$
 *
 * Logging mechanism macros.
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

#ifndef __AVATP_LOG_H__
#define __AVATP_LOG_H__

/* use the gstreamer logging mechanisms */
#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN( gst_avatp_debug);
#define GST_CAT_DEFAULT gst_avatp_debug

#define AVATP_WARNING(format...) 	GST_WARNING(format)
#define AVATP_INFO(format...)		GST_INFO(format)
#define AVATP_DEBUG(format...)		GST_DEBUG(format)

#endif /* __AVATP_LOG_H__ */
