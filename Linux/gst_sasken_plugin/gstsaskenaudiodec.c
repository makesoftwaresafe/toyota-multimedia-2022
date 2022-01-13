/*
 * Copyright (C) 2005 Thomas Vander Stichele <thomas@apestaart.org>
 * Copyright (C) 2005 Ronald S. Bultje <rbultje@ronald.bitfreak.net>
 * Copyright (C) 2017 by Sasken Technologies Ltd ( Formerly Sasken
 * Communication Technologies Limited)
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Alternatively, the contents of this file may be used under the
 * GNU Lesser General Public License Version 2.1 (the "LGPL"), in
 * which case the following provisions apply instead of the ones
 * mentioned above:
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */


/* SECTION:element-sasken_audiodec
 *
 * The sasken_audiodec GStreamer plugin is wrapper over
 * sasken audio Decoders. Decoder interfaces will
 *  be directly called from the Gstreamer plugin.
 * There is no OMX layer in between Gstreamer plugin and Encoder.
 * It Decode encoded audio data into PCM with Sasken audio decoders.
 *
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gstsaskenwma10prodec.h"
#include "gstsaskenmp3dec.h"
#include "gstsaskensbcdec.h"
#include "gstsaskenaacdec.h"
#include "gstsaskenflacdec.h"
#include "gstsaskenvorbisdec.h"
#include "gstsaskenwmalosslessdec.h"

static GKeyFile *config = NULL;

struct TypeOffest
{
  GType (*get_type) (void);
  glong offset;
};

static const struct TypeOffest base_types[] = {
  {gst_saskenwma10prodec_get_type,
      G_STRUCT_OFFSET (Gstsaskenwma10prodecClass, lib_name)},
  {gst_saskenwmalossless_get_type,
      G_STRUCT_OFFSET (GstsaskenwmalosslessClass, lib_name)},
  {gst_sasken_aacdec_get_type,
      G_STRUCT_OFFSET (GstsaskenaacdecClass, lib_name)},
  {gst_saskenflacdec_get_type,
      G_STRUCT_OFFSET (GstsaskenflacdecClass, lib_name)},
  {gst_saskenmp3dec_get_type,
      G_STRUCT_OFFSET (Gstsaskenmp3decClass, lib_name)},
  {gst_saskensbcdec_get_type,
      G_STRUCT_OFFSET (GstsaskensbcdecClass, lib_name)},
  {gst_saskenvorbisdec_get_type,
      G_STRUCT_OFFSET (GstsaskenvorbisdecClass, lib_name)}
};

typedef GType (*GGetTypeFunction) (void);

static const GGetTypeFunction types[] = {
  gst_saskenwmalossless_get_type,
  gst_saskenwma10prodec_get_type,
  gst_sasken_aacdec_get_type,
  gst_saskenflacdec_get_type,
  gst_saskenmp3dec_get_type,
  gst_saskensbcdec_get_type,
  gst_saskenvorbisdec_get_type
};

static void
_class_init (gpointer g_class, gpointer data)
{
  const gchar *element_name = data;
  gchar *lib_name = NULL;
  int i;
  gchar **element_lib_name = NULL;

  if (!element_name)
    return;

  /* Dynamic loading of libraries.  */

  for (i = 0; i < G_N_ELEMENTS (base_types); i++) {
    GType gtype = base_types[i].get_type ();

    if (G_TYPE_CHECK_CLASS_TYPE (g_class, gtype)) {
      element_lib_name = (gchar **) (((guint8 *) g_class)
          + base_types[i].offset);
      lib_name = g_key_file_get_string (config, element_name, "lib-name", NULL);
      g_assert (lib_name != NULL);
      if (element_lib_name != NULL) {
        *element_lib_name = lib_name;
      }
      break;
    }
  }
}

static gboolean
plugin_init (GstPlugin * plugin)
{
  gboolean ret = FALSE;
  GError *err = NULL;
  gchar **elements;
  gint i = 0;
  gchar *env_config_dir;
  gchar *config_dirs[2] = {NULL, NULL};
  gsize n_elements;
  static const gchar *config_name[] = { "gstsaskenplugin.conf", NULL};
  static const gchar *env_config_name[] =
      { "GST_SASKEN_PLUGIN_CONFIG_DIR", NULL };

  /* Set default path of audio configuration file "gstsaskenplugin.conf" */
  g_setenv (*env_config_name, "/usr/share", FALSE);

  /* Read configuration file gstsaskenplugin.conf from the preferred
   * configuration directories */
  env_config_dir = g_strdup (g_getenv (*env_config_name));
  config_dirs[0] = env_config_dir;
  config         = g_key_file_new ();

  if (!g_key_file_load_from_dirs (config, *config_name,
          (const gchar **) config_dirs, NULL, G_KEY_FILE_NONE, &err)) {

    GST_ERROR ("Failed to load configuration file: %s", config_name[0]);
    g_error_free (err);
    goto done;
  }

  /* Initialize all types */
  for (i = 0; i < G_N_ELEMENTS (types); i++)
    types[i] ();

  /* All Decoder Elements created Dynamically. */

  elements = g_key_file_get_groups (config, &n_elements);
  for (i = 0; i < n_elements; i++) {
    GTypeQuery type_query;
    GTypeInfo type_info = { 0, };
    GType type, subtype;
    gchar *type_name, *lib_name;
    gint rank = GST_RANK_PRIMARY;       /*Default value of rank */

    GST_DEBUG ("Registering element '%s'", elements[i]);
    err = NULL;

    if (!(type_name =
            g_key_file_get_string (config, elements[i], "type-name", &err))) {
      GST_ERROR
          ("Unable to read 'type-name' configuration for element '%s': %s",
          elements[i], err->message);
      g_error_free (err);
      continue;
    }

    /* Bug #40061[SWGIII-22661]: Added a check for decoder components by
     * checking for the suffix "dec" in the group ID of each element of the
     * configuration file */
    if (!g_str_has_suffix(elements[i], "dec")) {
      g_free (type_name);
      continue;
    }
    type = g_type_from_name (type_name);
    if (type == G_TYPE_INVALID) {
      GST_ERROR ("Invalid type name '%s' for element '%s'", type_name,
          elements[i]);
      g_free (type_name);
      continue;
    }

    if (!g_type_is_a (type, GST_TYPE_ELEMENT)) {
      GST_ERROR ("Type '%s' is no GstElement subtype for element '%s'",
          type_name, elements[i]);
      g_free (type_name);
      continue;
    }

    g_free (type_name);

    /* And now some sanity checking */
    err = NULL;
    if (!(lib_name =
        g_key_file_get_string (config, elements[i], "lib-name", &err))) {
      GST_ERROR ("Unable to read 'lib-name' configuration for element '%s': %s",
          elements[i], err->message);
      g_error_free (err);
      continue;
    }

    if (!g_file_test (lib_name, G_FILE_TEST_IS_REGULAR)) {
      GST_ERROR ("Library '%s' does not exist for element '%s'", lib_name,
          elements[i]);
      g_free (lib_name);
      continue;
    }
    g_free (lib_name);
    err = NULL;

    rank = g_key_file_get_integer (config, elements[i], "rank", &err);

    if (err != NULL) {
      GST_ERROR ("No rank set for element '%s': %s", elements[i], err->message);
      g_error_free (err);
      continue;
    }

    /* And now register the type, all other configuration will
     * be handled by the type itself */
    g_type_query (type, &type_query);
    memset (&type_info, 0, sizeof (type_info));
    type_info.class_size = type_query.class_size;
    type_info.instance_size = type_query.instance_size;
    type_info.class_init = _class_init;
    type_info.class_data = g_strdup (elements[i]);
    type_name = g_strdup_printf ("%s-%s", g_type_name (type), elements[i]);

    if (g_type_from_name (type_name) != G_TYPE_INVALID) {
      GST_ERROR ("Type '%s' already exists for element '%s'", type_name,
          elements[i]);
      g_free (type_name);
      continue;
    }

    subtype = g_type_register_static (type, type_name, &type_info, 0);
    g_free (type_name);
    ret = gst_element_register (plugin, elements[i], rank, subtype);

    if (ret != TRUE) {
      GST_ERROR ("Error while registering element '%s' Type '%s'",
              elements[i], type_name);
      return ret;
    }
  }

done:
  g_free (env_config_dir);
  return TRUE;
}

#ifndef PACKAGE
#define PACKAGE "sasken_audiodec"
#endif

/******************************************************************************
 * Gstreamer looks for this structure to register sasken audio decoders
 *****************************************************************************/
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    sasken_audiodec,
    "Decodes the encoded audio stream",
    plugin_init, "0.0.0.10", "LGPL", "sasken_audiodec", "www.sasken.com")

/*****************************************************************************/
