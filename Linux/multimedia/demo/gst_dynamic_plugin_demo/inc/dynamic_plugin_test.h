/******************************************************************************
 * \file: dynamic_plugin_test.h
 *
 * \Description: Function, defines and enum declarations
 *
 * \component: Gstreamer
 *
 * \author: Gautham Kantharaju / RBEI / ECF3 / Gautham.Kantharaju@in.bosch.com
 *
 * \copyright: (c) 2007 - 2014 ADIT Corporation
 *****************************************************************************/

#ifndef _DYNAMIC_PLUGIN_TEST_
#define _DYNAMIC_PLUGIN_TEST_

#include <gst/gst.h>
#include <glib.h>
#include <gst/base/gstbasesink.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <signal.h>
#include <stdbool.h>

#define DEBUG

#ifdef DEBUG
#  define    debug(string, args...) g_print(string, ##args)
#else
#  define    debug(string, args...)
#endif

#define FRAMEBUFFER_BCKGROUND   0
#define WAYLAND_BCKGROUND       1

#define ONLY_CHANGE_BCKGROUND   0
#define INIT_LAYER_MANAGER      1

enum
{
  PIX_FMT_RGB565,  /* 0: To capture RGB565 color-format image       */
  PIX_FMT_BGR24,   /* 1: To capture BGR24 color-format image        */
  PIX_FMT_RGB24,   /* 2: To capture RGB24 color-format image        */
  PIX_FMT_BGR32,   /* 3: To capture BGR32 color-format image        */
  PIX_FMT_RGB32,   /* 4: To capture RGB32 color-format image        */
  PIX_FMT_YUV422P, /* 5: To capture YUV422P color-format image      */
  PIX_FMT_UYVY,    /* 6: To capture UYVY color-format image         */
  PIX_FMT_YUYV,    /* 7: To capture YUYV color-format image         */
  PIX_FMT_YUV420,  /* 8: To capture YUV420(YU12) color-format image */
  PIX_FMT_NV12     /* 9: To capture NV12 color-format image         */
};

#endif /* _DYNAMIC_PLUGIN_TEST_ */

/************************************************************************
 * <put this to end of file>
 * \history
 * <history item>
 * <history item>
 * <history item>
 *
 ***********************************************************************/

