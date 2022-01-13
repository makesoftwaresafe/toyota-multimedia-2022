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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#include "config.h"

#include <string.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>
#include <gio/gio.h>

#include "cache.h"

/**
 * SECTION:cache
 * @title: Cache
 * @short_description: Caching and lookup of stored media art.
 * @include: libmediaart/mediaart.h
 *
 * These functions give you access to the media art that has been
 * extracted and saved. There are normally two places the media art
 * will be located in. These locations store symlinks not real copies
 * of the content:
 * <itemizedlist>
 *   <listitem>
 *     <para>The user's XDG_CACHE_HOME directory (usually
 * <filename>~/.cache/media-art/</filename>)</para>
 *   </listitem>
 *   <listitem>
 *     <para>The local file system's top level
 * <filename>.mediaartlocal</filename> directory (for example
 * <filename>/media/martyn/pendrive/.mediaartlocal/</filename>)</para>
 *   </listitem>
 * </itemizedlist>
 *
 * To find the media art for a given media file, use the function
 * media_art_get_file() (you can also use media_art_get_path(), which
 * does the same thing but for path strings instead of #GFile
 * objects).
 *
 * If media art for the file is not found in the cache, these
 * functions will return %NULL. You may find some embedded media art
 * upon loading the file, and you can use media_art_process_buffer()
 * to convert it to the correct format and save it in the cache for
 * next time. The media_art_process_file() function also supports
 * searching for external media art images using a basic heuristic.
 **/

static gboolean
media_art_strip_find_next_block (const gchar    *original,
                                 const gunichar  open_char,
                                 const gunichar  close_char,
                                 gint           *open_pos,
                                 gint           *close_pos)
{
	const gchar *p1, *p2;

	if (open_pos) {
		*open_pos = -1;
	}

	if (close_pos) {
		*close_pos = -1;
	}

	p1 = g_utf8_strchr (original, -1, open_char);
	if (p1) {
		if (open_pos) {
			*open_pos = p1 - original;
		}

		p2 = g_utf8_strchr (g_utf8_next_char (p1), -1, close_char);
		if (p2) {
			if (close_pos) {
				*close_pos = p2 - original;
			}

			return TRUE;
		}
	}

	return FALSE;
}

/**
 * media_art_strip_invalid_entities:
 * @original: original string
 *
 * Strip a albumname or artistname string to prepare it for calculating the
 * media art path with it. Certain characters and charactersets will be stripped
 * and a newly allocated string returned which you must free with g_free().
 *
 * This functions is used internally by media_art_get_file() and
 * media_art_get_path(). You will not normally need to call it yourself.
 *
 * This function provides the following features:
 * 1. Invalid characters include: ()[]<>{}_!@#$^&*+=|\/"'?~;
 * 2. Text inside brackets of (), {}, [] and <> pairs are removed.
 * 3. Multiples of space characters are removed.
 *
 * Returns: @original stripped of invalid characters which must be
 * freed. On error or if @original is empty, %NULL is returned.
 *
 * Since: 0.2.0
 */
gchar *
media_art_strip_invalid_entities (const gchar *original)
{
	GString *str_no_blocks;
	gchar **strv;
	gchar *str;
	gboolean blocks_done = FALSE;
	const gchar *p;
	const gchar *invalid_chars = "()[]<>{}_!@#$^&*+=|\\/\"'?~";
	const gchar *invalid_chars_delimiter = "*";
	const gchar *convert_chars = "\t";
	const gchar *convert_chars_delimiter = " ";
	const gunichar blocks[5][2] = {
		{ '(', ')' },
		{ '{', '}' },
		{ '[', ']' },
		{ '<', '>' },
		{  0,   0  }
	};

	g_return_val_if_fail (original != NULL, NULL);

	str_no_blocks = g_string_new ("");

	p = original;

	while (!blocks_done) {
		gint pos1, pos2, i;

		pos1 = -1;
		pos2 = -1;

		for (i = 0; blocks[i][0] != 0; i++) {
			gint start, end;

			/* Go through blocks, find the earliest block we can */
			if (media_art_strip_find_next_block (p, blocks[i][0], blocks[i][1], &start, &end)) {
				if (pos1 == -1 || start < pos1) {
					pos1 = start;
					pos2 = end;
				}
			}
		}

		/* If either are -1 we didn't find any */
		if (pos1 == -1) {
			/* This means no blocks were found */
			g_string_append (str_no_blocks, p);
			blocks_done = TRUE;
		} else {
			/* Append the test BEFORE the block */
			if (pos1 > 0) {
				g_string_append_len (str_no_blocks, p, pos1);
			}

			p = g_utf8_next_char (p + pos2);

			/* Do same again for position AFTER block */
			if (*p == '\0') {
				blocks_done = TRUE;
			}
		}
	}

	/* Now convert chars to lower case */
	str = g_utf8_strdown (str_no_blocks->str, -1);
	g_string_free (str_no_blocks, TRUE);

	/* Now strip invalid chars */
	g_strdelimit (str, invalid_chars, *invalid_chars_delimiter);
	strv = g_strsplit (str, invalid_chars_delimiter, -1);
	g_free (str);
	str = g_strjoinv (NULL, strv);
	g_strfreev (strv);

	/* Now convert chars */
	g_strdelimit (str, convert_chars, *convert_chars_delimiter);
	strv = g_strsplit (str, convert_chars_delimiter, -1);
	g_free (str);
	str = g_strjoinv (convert_chars_delimiter, strv);
	g_strfreev (strv);

	while (g_strrstr (str, "  ") != NULL) {
		/* Now remove double spaces */
		strv = g_strsplit (str, "  ", -1);
		g_free (str);
		str = g_strjoinv (" ", strv);
		g_strfreev (strv);
	}

	/* Now strip leading/trailing white space */
	g_strstrip (str);

	return str;
}

static gchar *
media_art_checksum_for_data (GChecksumType  checksum_type,
                             const guchar  *data,
                             gsize          length)
{
	GChecksum *checksum;
	gchar *retval;

	checksum = g_checksum_new (checksum_type);
	if (!checksum) {
		return NULL;
	}

	g_checksum_update (checksum, data, length);
	retval = g_strdup (g_checksum_get_string (checksum));
	g_checksum_free (checksum);

	return retval;
}

/**
 * media_art_get_file:
 * @artist: (allow-none): the artist
 * @title: (allow-none): the title
 * @prefix: (allow-none): the prefix for cache files, for example "album"
 * @cache_file: (out) (transfer full) (allow-none): a pointer to a
 * #GFile which represents the cached file for media art, or %NULL
 * a #GFile representing the user&apos;s cache path, or %NULL
 * #GFile representing the location of the local media art
 *
 * Gets the files pointing to cache files suitable for storing the media
 * art provided by the @artist, @title and @file arguments. @cache_file
 * will point to a location in the XDG user cache directory..
 *
 * The @cache_file relates to a symlink stored in XDG cache directories
 * for the user. A @cache_file would be expected to look like
 * <filename>file:///home/martyn/.cache/media-art/...</filename>. This
 * is normally the location that is most useful (assuming the cache
 * has been extracted in the first place).
 *
 * When done, both #GFile<!-- -->s must be freed with g_object_unref() if
 * non-%NULL.
 *
 * This operation should not use i/o, but it depends on the backend
 * GFile implementation.
 *
 * Returns: %TRUE if @cache_file was returned, otherwise %FALSE.
 *
 * Since: 0.2.0
 */
gboolean
media_art_get_file (const gchar  *artist,
                    const gchar  *title,
                    const gchar  *prefix,
                    GFile       **cache_file)
{
	const gchar *space_checksum = "7215ee9c7d9dc229d2921a40e899ec5f";
	const gchar *a, *b;

	gchar *art_filename;
	gchar *dir, *filename;
	gchar *artist_down, *title_down;
	gchar *artist_stripped, *title_stripped;
	gchar *artist_norm, *title_norm;
	gchar *artist_checksum = NULL, *title_checksum = NULL;

	/* http://live.gnome.org/MediaArtStorageSpec */

	if (cache_file) {
		*cache_file = NULL;
	}

	/* Rules:
	 * 1. artist OR title must be non-NULL.
	 * 2. cache_file must be non-NULL
	 */
	g_return_val_if_fail (artist != NULL || title != NULL, FALSE);
	g_return_val_if_fail (!G_IS_FILE (cache_file), FALSE);

	if (artist) {
		artist_stripped = media_art_strip_invalid_entities (artist);
		artist_norm = g_utf8_normalize (artist_stripped, -1, G_NORMALIZE_NFKD);
		artist_down = g_utf8_strdown (artist_norm, -1);
		artist_checksum = media_art_checksum_for_data (G_CHECKSUM_MD5,
		                                               (const guchar *) artist_down,
		                                               strlen (artist_down));
	}

	if (title) {
		title_stripped = media_art_strip_invalid_entities (title);
		title_norm = g_utf8_normalize (title_stripped, -1, G_NORMALIZE_NFKD);
		title_down = g_utf8_strdown (title_norm, -1);
		title_checksum = media_art_checksum_for_data (G_CHECKSUM_MD5,
		                                              (const guchar *) title_down,
		                                              strlen (title_down));
	}

	dir = g_build_filename (g_get_user_cache_dir (),
	                        "media-art",
	                        NULL);

	if (artist) {
		a = artist_checksum;
		b = title ? title_checksum : space_checksum;
	} else {
		a = title_checksum;
		b = space_checksum;
	}

	art_filename = g_strdup_printf ("%s-%s-%s.jpeg", prefix ? prefix : "album", a, b);

	if (artist) {
		g_free (artist_checksum);
		g_free (artist_stripped);
		g_free (artist_down);
		g_free (artist_norm);
	}

	if (title) {
		g_free (title_checksum);
		g_free (title_stripped);
		g_free (title_down);
		g_free (title_norm);
	}

	if (cache_file) {
		filename = g_build_filename (dir, art_filename, NULL);
		*cache_file = g_file_new_for_path (filename);
		g_free (filename);
	}

	g_free (dir);
	g_free (art_filename);

	return TRUE;
}

/**
 * media_art_get_path:
 * @artist: (allow-none): the artist
 * @title: (allow-none): the title
 * @prefix: (allow-none): the prefix, for example "album"
 * @cache_path: (out) (transfer full) (allow-none): a string
 * representing the path to the cache for this media art
 * path or %NULL
 *
 * This function calls media_art_get_file() by creating a #GFile for
 * @uri and passing the same arguments to media_art_get_file(). For more
 * details about what this function does, see media_art_get_file().
 *
 * Get the path to media art for a given resource. Newly allocated
 * data returned in @cache_path must be freed with g_free().
 *
 * Returns: %TRUE if @cache_path was returned, otherwise %FALSE.
 *
 * Since: 0.2.0
 */
gboolean
media_art_get_path (const gchar  *artist,
                    const gchar  *title,
                    const gchar  *prefix,
                    gchar       **cache_path)
{
	GFile *cache_file = NULL;

	/* Rules:
	 * 1. artist OR title must be non-NULL.
	 * 2. cache_file must be non-NULL
	 */
	g_return_val_if_fail (artist != NULL || title != NULL, FALSE);
	g_return_val_if_fail (cache_path != NULL, FALSE);

	media_art_get_file (artist, title, prefix, cache_path ? &cache_file : NULL);
	if (cache_path) {
		*cache_path = cache_file ? g_file_get_path (cache_file) : NULL;
	}

	return TRUE;
}

/**
 * media_art_remove:
 * @artist: artist the media art belongs to
 * @album: (allow-none): album the media art belongs or %NULL
 * @cancellable: (allow-none): optional #GCancellable object, %NULL to ignore.
 * @error: location to store the error occurring, or %NULL to ignore
 *
 * Removes media art for given album/artist provided.
 *
 * If @artist and @album are %NULL, ALL media art cache is removed.
 *
 * Returns: #TRUE on success, otherwise #FALSE where @error will be set.
 *
 * Since: 0.2.0
 */
gboolean
media_art_remove (const gchar   *artist,
                  const gchar   *album,
                  GCancellable  *cancellable,
                  GError       **error)
{
	GError *local_error = NULL;
	const gchar *name;
	GDir *dir;
	gchar *dirname;
	gboolean success = TRUE;

	g_return_val_if_fail (artist != NULL && artist[0] != '\0', FALSE);

	dirname = g_build_filename (g_get_user_cache_dir (), "media-art", NULL);

	dir = g_dir_open (dirname, 0, &local_error);
	if (!dir || local_error) {
		/* Nothing to do if there is no directory in the first place. */
		g_debug ("Removing media-art for artist:'%s', album:'%s': directory could not be opened, %s",
		         artist, album, local_error ? local_error->message : "no error given");

		g_clear_error (&local_error);

		if (dir) {
			g_dir_close (dir);
		}
		g_free (dirname);

		/* We wanted to remove media art, so if there is no
		 * media art, the caller has achieved what they wanted.
		 */
		return TRUE;
	}

	/* NOTE: We expect to not find some of these paths for
	 * artist/album conbinations, so don't error in those
	 * cases...
	 */
	if (artist || album) {
		gchar *target = NULL;
		gint removed = 0;

		/* The get_path API does stripping itself */
		media_art_get_path (artist, album, "album", &target);

		if (target) {
			if (g_unlink (target) != 0) {
				g_debug ("Could not delete file '%s'", target);
			} else {
				g_message ("Removed media-art for artist:'%s', album:'%s': deleting file '%s'",
				           artist, album, target);
				removed++;
			}

			g_free (target);
		}

		/* Add the album path also (to which the symlinks are made) */
		if (album) {
			media_art_get_path (NULL, album, "album", &target);
			if (target) {
				if (g_unlink (target) != 0) {
					g_debug ("Could not delete file '%s'", target);
				} else {
					g_message ("Removed media-art for album:'%s': deleting file '%s'",
					           album, target);
					removed++;
				}

				g_free (target);
			}
		}

		success = removed > 0;
	} else {
		for (name = g_dir_read_name (dir);
		     name != NULL;
		     name = g_dir_read_name (dir)) {
			gchar *target;

			target = g_build_filename (dirname, name, NULL);

			if (g_unlink (target) != 0) {
				g_warning ("Could not delete file '%s'", target);
				success = FALSE;
			} else {
				g_message ("Removing all media-art: deleted file '%s'", target);
			}

			g_free (target);
		}
	}

	if (!success) {
		g_set_error_literal (error,
		                     G_IO_ERROR,
		                     G_IO_ERROR_FAILED,
		                     _("Could not remove one or more files from media art cache"));
	}

	g_dir_close (dir);
	g_free (dirname);

	return success;
}

typedef struct {
	gchar *artist;
	gchar *album;
} RemoveData;

static RemoveData *
remove_data_new (const gchar *artist,
                 const gchar *album)
{
	RemoveData *data;

	data = g_slice_new0 (RemoveData);
	data->artist = g_strdup (artist);
	data->album = g_strdup (album);

	return data;
}

static void
remove_data_free (RemoveData *data)
{
	if (!data) {
		return;
	}

	g_free (data->artist);
	g_free (data->album);
	g_slice_free (RemoveData, data);
}

static void
remove_thread (GTask        *task,
               gpointer      source_object,
               gpointer      task_data,
               GCancellable *cancellable)
{
	RemoveData *data = task_data;
	GError *error = NULL;
	gboolean success = FALSE;

	if (!g_cancellable_set_error_if_cancelled (cancellable, &error)) {
		success = media_art_remove (data->artist,
		                            data->album,
		                            cancellable,
		                            &error);
	}

	if (error) {
		g_task_return_error (task, error);
	} else {
		g_task_return_boolean (task, success);
	}
}

/**
 * media_art_remove_async:
 * @artist: artist the media art belongs to
 * @album: (allow-none): album the media art belongs or %NULL
 * @source_object: (allow-none): the #GObject this task belongs to,
 * can be %NULL.
 * @io_priority: the [I/O priority][io-priority] of the request
 * @cancellable: (allow-none): optional #GCancellable object, %NULL to
 * ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the
 * request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Removes media art for given album/artist provided. Precisely the
 * same operation as media_art_remove() is performing, but
 * asynchronously.
 *
 * When all i/o for the operation is finished the @callback will be
 * called.
 *
 * In case of a partial error the callback will be called with any
 * succeeding items and no error, and on the next request the error
 * will be reported. If a request is cancelled the callback will be
 * called with %G_IO_ERROR_CANCELLED.
 *
 * During an async request no other sync and async calls are allowed,
 * and will result in %G_IO_ERROR_PENDING errors.
 *
 * Any outstanding i/o request with higher priority (lower numerical
 * value) will be executed before an outstanding request with lower
 * priority. Default priority is %G_PRIORITY_DEFAULT.
 *
 * Since: 0.7.0
 */
void
media_art_remove_async (const gchar           *artist,
                        const gchar           *album,
                        gint                   io_priority,
                        GObject               *source_object,
                        GCancellable          *cancellable,
                        GAsyncReadyCallback    callback,
                        gpointer               user_data)
{
	GTask *task;

	task = g_task_new (source_object, cancellable, callback, user_data);
	g_task_set_task_data (task, remove_data_new (artist, album), (GDestroyNotify) remove_data_free);
	g_task_set_priority (task, io_priority);
	g_task_run_in_thread (task, remove_thread);
	g_object_unref (task);
}

/**
 * media_art_remove_finish:
 * @source_object: (allow-none): the #GObject this task belongs to,
 * can be %NULL.
 * @result: a #GAsyncResult.
 * @error: a #GError location to store the error occurring, or %NULL
 * to ignore.
 *
 * Finishes the asynchronous operation started with
 * media_art_remove_async().
 *
 * Returns: %TRUE on success, otherwise %FALSE when @error will be set.
 *
 * Since: 0.7.0
 **/
gboolean
media_art_remove_finish (GObject       *source_object,
                         GAsyncResult  *result,
                         GError       **error)
{
	g_return_val_if_fail (g_task_is_valid (result, source_object), FALSE);

	return g_task_propagate_boolean (G_TASK (result), error);
}
