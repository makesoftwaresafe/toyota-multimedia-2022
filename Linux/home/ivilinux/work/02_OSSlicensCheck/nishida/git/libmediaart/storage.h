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
 */

#ifndef __LIBMEDIAART_STORAGE_H__
#define __LIBMEDIAART_STORAGE_H__

#include <glib-object.h>
#include <gio/gio.h>

#if !defined (__LIBMEDIAART_INSIDE__) && !defined (LIBMEDIAART_COMPILATION)
#error "Only <libmediaart/mediaart.h> must be included directly."
#endif

G_BEGIN_DECLS

/**
 * StorageType:
 * @STORAGE_REMOVABLE: Storage is a removable media
 * @STORAGE_OPTICAL: Storage is an optical disc
 *
 * Flags specifying properties of the type of storage.
 *
 * Since: 0.2.0
 */
typedef enum {
	STORAGE_REMOVABLE = 1 << 0,
	STORAGE_OPTICAL   = 1 << 1
} StorageType;

/**
 * STORAGE_TYPE_IS_REMOVABLE:
 * @type: Mask of StorageType flags
 *
 * Check if the given storage type is marked as being removable media.
 *
 * Returns: %TRUE if the storage is marked as removable media, %FALSE otherwise
 *
 * Since: 0.2.0
 */
#define STORAGE_TYPE_IS_REMOVABLE(type) ((type & STORAGE_REMOVABLE) ? TRUE : FALSE)

/**
 * STORAGE_TYPE_IS_OPTICAL:
 * @type: Mask of StorageType flags
 *
 * Check if the given storage type is marked as being optical disc
 *
 * Returns: %TRUE if the storage is marked as optical disc, %FALSE otherwise
 *
 * Since: 0.2.0
 */
#define STORAGE_TYPE_IS_OPTICAL(type) ((type & STORAGE_OPTICAL) ? TRUE : FALSE)


#define TYPE_STORAGE                 (storage_get_type ())
#define STORAGE(o)                   (G_TYPE_CHECK_INSTANCE_CAST ((o), TYPE_STORAGE, Storage))
#define STORAGE_CLASS(k)             (G_TYPE_CHECK_CLASS_CAST ((k), TYPE_STORAGE, StorageClass))
#define IS_STORAGE(o)                (G_TYPE_CHECK_INSTANCE_TYPE ((o), TYPE_STORAGE))
#define IS_STORAGE_CLASS(k)          (G_TYPE_CHECK_CLASS_TYPE ((k), TYPE_STORAGE))
#define STORAGE_GET_CLASS(o)         (G_TYPE_INSTANCE_GET_CLASS ((o), TYPE_STORAGE, StorageClass))

typedef struct _Storage Storage;
typedef struct _StorageClass StorageClass;

/**
 * Storage:
 * @parent: parent object
 *
 * A storage API for using mount points and devices
 **/
struct _Storage {
	GObject parent;
};

/**
 * StorageClass:
 * @parent_class: parent object class
 *
 * A storage class for #Storage.
 **/
struct _StorageClass {
	GObjectClass parent_class;
};

GType        storage_get_type                 (void) G_GNUC_CONST;
Storage *    storage_new                      (void);
GSList *     storage_get_device_roots         (Storage     *storage,
                                               StorageType  type,
                                               gboolean     exact_match);
GSList *     storage_get_device_uuids         (Storage     *storage,
                                               StorageType  type,
                                               gboolean     exact_match);
const gchar *storage_get_mount_point_for_uuid (Storage     *storage,
                                               const gchar *uuid);
StorageType  storage_get_type_for_uuid        (Storage     *storage,
                                               const gchar *uuid);
const gchar *storage_get_uuid_for_file        (Storage     *storage,
                                               GFile       *file);

G_END_DECLS

#endif /* __LIBMEDIAART_STORAGE_H__ */
