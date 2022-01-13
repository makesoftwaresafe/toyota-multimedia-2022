/**
* \file: gstapx.h
*
* \version: $Id:$
*
* \release: $Name:$
*
* Render videos using APX pixmaps
*
* \component: multimedia/gst
*
* \author: Jens Georg <jgeorg@de.adit-jv.com>
*
* \copyright: (c) 2003 - 2013 ADIT Corporation
*
* \history
* 0.1 Michael Methner Initial version
* 0.2 Gautham Kantharaju Extended to support mfw_v4lsrc source plugin
*
***********************************************************************/
#ifndef __GST_APX_H__
#define __GST_APX_H__

#include <gst/gst.h>
#include <gst/video/video.h>
#include <apx.h>

#ifndef __ADIT_UNUSED
# define __ADIT_UNUSED(x) ((void)(x))
#endif /* __ADIT_UNUSED */

#ifndef PACKAGE
#define PACKAGE "gst_apx_sink"
#endif /* PACKAGE */

GST_DEBUG_CATEGORY_EXTERN(gst_apx_sink_debug);
#define GST_CAT_DEFAULT gst_apx_sink_debug

GstVideoFormat apx_to_gst(apxPixelFormat format);
apxPixelFormat gst_to_apx(GstVideoFormat format);

#ifndef g_clear_pointer
#define g_clear_pointer(p,f) \
    G_STMT_START { \
        void *__ptr = *(p); \
        *(p) = NULL; \
        if (__ptr) (f)(__ptr); \
    } G_STMT_END
#endif

void __apx_log_errmem(const char *message, ...);

#endif /* __GST_APX_H__ */
