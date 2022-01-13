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

#ifndef __LIBMEDIAART_CACHE_H__
#define __LIBMEDIAART_CACHE_H__

#include <glib.h>
#include <gio/gio.h>

#if !defined (__LIBMEDIAART_INSIDE__) && !defined (LIBMEDIAART_COMPILATION)
#error "Only <libmediaart/mediaart.h> must be included directly."
#endif

G_BEGIN_DECLS

gchar *  media_art_strip_invalid_entities (const gchar          *original);

gboolean media_art_get_path               (const gchar          *artist,
                                           const gchar          *title,
                                           const gchar          *prefix,
                                           gchar               **cache_path);
gboolean media_art_get_file               (const gchar          *artist,
                                           const gchar          *title,
                                           const gchar          *prefix,
                                           GFile               **cache_file);

gboolean media_art_remove                 (const gchar          *artist,
                                           const gchar          *album,
                                           GCancellable         *cancellable,
                                           GError              **error);
void     media_art_remove_async           (const gchar          *artist,
                                           const gchar          *album,
                                           gint                  io_priority,
                                           GObject              *source_object,
                                           GCancellable         *cancellable,
                                           GAsyncReadyCallback   callback,
                                           gpointer              user_data);
gboolean media_art_remove_finish          (GObject              *source_object,
                                           GAsyncResult         *result,
                                           GError              **error);

G_END_DECLS

#endif /* __LIBMEDIAART_CACHE_H__ */
