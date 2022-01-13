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

#include "extractgeneric.h"

/**
 * SECTION:plugins
 * @title: Plugins
 * @short_description: Plugins for cache conversion.
 * @include: libmediaart/mediaart.h
 *
 * Plugins are provided to allow different systems to make use of
 * existing file format conversion APIs. By default, a GdkPixbuf and
 * Qt implementation are provided. This API allows new implementations
 * to be provided.
 *
 **/

/**
 * media_art_plugin_init:
 * @max_width: The maximum width that an image is allowed to be
 *
 * This function facilitates a plugin&apos;s need to create any
 * internal caches before anything else is done. This function must
 * exist in each plugin and is called immediately after the plugin is
 * loaded. Conversely, media_art_plugin_shutdown() is called before
 * tear down of the plugin system or plugin itself.
 *
 * Since: 0.1.0
 */
void
media_art_plugin_init (gint max_width)
{
	/* Initialize something */
}

/**
 * media_art_plugin_shutdown:
 *
 * This function facilitates a plugin&apos;s need to clean up any
 * internal caches. This function must exist in each plugin and is
 * called immediately after the plugin is loaded. Conversely,
 * media_art_plugin_init() is called after the plugin is loaded.
 *
 * Since: 0.1.0
 */
void
media_art_plugin_shutdown (void)
{
	/* Shutdown something */
}

/**
 * media_art_file_to_jpeg:
 * @filename: Original file name (not URI) to convert
 * @target: Output file name (not URI) to save converted content to
 * @error: A #GError to use upon failure, or %NULL
 *
 * Save @filename to @target as JPEG format. The @filename may not be
 * a JPEG in the first place.
 *
 * Returns: %TRUE if conversion was successful, otherwise %FALSE is
 * returned if @error is set.
 *
 * Since: 0.1.0
 */
gboolean
media_art_file_to_jpeg (const gchar  *filename,
                        const gchar  *target,
                        GError      **error)
{
	return FALSE;
}

/**
 * media_art_buffer_to_jpeg:
 * @buffer: Raw buffer representing content to save
 * @len: Length of @buffer to use
 * @buffer_mime: MIME type for @buffer
 * @target: Output file name (not URI) to save converted content to
 * @error: A #GError to use upon failure, or %NULL
 *
 * This function performs the same operation as
 * media_art_file_to_jpeg() with the exception that a raw @buffer can
 * be used instead providing @len and the @buffer_mime too.
 *
 * Returns: %TRUE if conversion was successful, otherwise %FALSE is
 * returned if @error is set.
 *
 * Since: 0.1.0
 */
gboolean
media_art_buffer_to_jpeg (const unsigned char  *buffer,
                          size_t                len,
                          const gchar          *buffer_mime,
                          const gchar          *target,
                          GError              **error)
{
	return FALSE;
}
