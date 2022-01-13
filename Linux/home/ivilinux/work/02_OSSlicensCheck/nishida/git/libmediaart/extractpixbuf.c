/*
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 *
 * Authors:
 * Philip Van Hoof <philip@codeminded.be>
 */

#include <gdk-pixbuf/gdk-pixbuf.h>

#include "extractgeneric.h"

static gint max_width_in_bytes = 0;

void
media_art_plugin_init (gint max_width)
{
	g_return_if_fail (max_width >= 0);

	max_width_in_bytes = max_width;
}

void
media_art_plugin_shutdown (void)
{
}

gboolean
media_art_file_to_jpeg (const gchar  *filename,
                        const gchar  *target,
                        GError      **error)
{
	GdkPixbuf *pixbuf;
	GError *local_error = NULL;

	/* TODO: Add resizing support */

	pixbuf = gdk_pixbuf_new_from_file (filename, &local_error);

	if (local_error) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	gdk_pixbuf_save (pixbuf, target, "jpeg", &local_error, NULL);
	g_object_unref (pixbuf);

	if (local_error) {
		g_propagate_error (error, local_error);
		return FALSE;
	}

	return TRUE;
}

static void
size_prepared_cb (GdkPixbufLoader *loader,
                  gint             width,
                  gint             height,
                  gpointer         user_data)
{
	gfloat scale;

	if (max_width_in_bytes < 1 || width <= max_width_in_bytes) {
		return;
	}

	g_debug ("Resizing media art to %d width", max_width_in_bytes);

	scale = width / (gfloat) max_width_in_bytes;

	gdk_pixbuf_loader_set_size (loader, (gint) (width / scale), (gint) (height / scale));
}

gboolean
media_art_buffer_to_jpeg (const unsigned char  *buffer,
                          size_t                len,
                          const gchar          *buffer_mime,
                          const gchar          *target,
                          GError              **error)
{
	GError *local_error = NULL;

	if (max_width_in_bytes < 0) {
		g_debug ("Not saving album art from buffer, disabled in config");
		return TRUE;
	}

	/* FF D8 FF are the three first bytes of JPeg images */
	if (max_width_in_bytes == 0 &&
	    (g_strcmp0 (buffer_mime, "image/jpeg") == 0 ||
	     g_strcmp0 (buffer_mime, "JPG") == 0) &&
	    (buffer && len > 2 && buffer[0] == 0xff && buffer[1] == 0xd8 && buffer[2] == 0xff)) {
		g_debug ("Saving album art using raw data as uri:'%s'", target);
		if (!g_file_set_contents (target, (const gchar *) buffer, (gssize) len, error)) {
			return FALSE;
		}
	} else {
		GdkPixbuf *pixbuf;
		GdkPixbufLoader *loader;

		g_debug ("Saving album art using GdkPixbufLoader for uri:'%s' (max width:%d)",
		         target,
		         max_width_in_bytes);

		loader = gdk_pixbuf_loader_new ();
		if (max_width_in_bytes > 0) {
			g_signal_connect (loader,
			                  "size-prepared",
			                  G_CALLBACK (size_prepared_cb),
			                  NULL);
		}

		if (!gdk_pixbuf_loader_write (loader, buffer, len, &local_error)) {
			g_warning ("Could not write with GdkPixbufLoader when setting media art, %s",
			           local_error ? local_error->message : "no error given");

			g_propagate_error (error, local_error);
			gdk_pixbuf_loader_close (loader, NULL);
			g_object_unref (loader);

			return FALSE;
		}

		pixbuf = gdk_pixbuf_loader_get_pixbuf (loader);

		if (pixbuf == NULL) {
			g_warning ("Could not get pixbuf from GdkPixbufLoader when setting media art");

			/* FIXME: Set error here */

			gdk_pixbuf_loader_close (loader, NULL);
			g_object_unref (loader);

			return FALSE;
		}

		if (!gdk_pixbuf_save (pixbuf, target, "jpeg", &local_error, NULL)) {
			g_warning ("Could not save GdkPixbuf when setting media art, %s",
			           local_error ? local_error->message : "no error given");

			g_propagate_error (error, local_error);
			gdk_pixbuf_loader_close (loader, NULL);
			g_object_unref (loader);

			return FALSE;
		}

		if (!gdk_pixbuf_loader_close (loader, &local_error)) {
			g_warning ("Could not close GdkPixbufLoader when setting media art, %s",
			           local_error ? local_error->message : "no error given");

			g_propagate_error (error, local_error);

			return FALSE;
		}

		g_object_unref (loader);
	}

	return TRUE;
}
