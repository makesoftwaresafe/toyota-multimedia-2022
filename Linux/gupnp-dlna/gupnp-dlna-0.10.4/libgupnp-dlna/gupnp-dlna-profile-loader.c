/*
 * Copyright (C) 2010 Nokia Corporation.
 * Copyright (C) 2012, 2013 Intel Corporation.
 *
 * Authors: Arun Raghavan <arun.raghavan@collabora.co.uk>
 *          Krzesimir Nowak <krnowak@openismus.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif /* HAVE_CONFIG_H_ */
#include <glib/gstdio.h>
#include <libxml/xmlreader.h>
#include <libxml/relaxng.h>
#include "gupnp-dlna-profile-loader.h"
#include "gupnp-dlna-profile-private.h"
#include "gupnp-dlna-utils.h"
#include "gupnp-dlna-value-list-private.h"
#include "gupnp-dlna-value-type.h"
#include "gupnp-dlna-restriction-private.h"

G_DEFINE_TYPE (GUPnPDLNAProfileLoader, gupnp_dlna_profile_loader, G_TYPE_OBJECT)

#define DLNA_DATA_DIR DATA_DIR G_DIR_SEPARATOR_S "dlna-profiles"
#define NODE_TYPE_ELEMENT_START 1
#define NODE_TYPE_TEXT 3
#define NODE_TYPE_ELEMENT_END 15

enum {
        PROP_0,

        PROP_RELAXED_MODE,
        PROP_EXTENDED_MODE
};

typedef enum {
        GUPNP_DLNA_PARSED_ELEMENT_RESTRICTIONS,
        GUPNP_DLNA_PARSED_ELEMENT_RESTRICTION,
        GUPNP_DLNA_PARSED_ELEMENT_FIELD,
        GUPNP_DLNA_PARSED_ELEMENT_PARENT,
        GUPNP_DLNA_PARSED_ELEMENT_DLNA_PROFILE,
        GUPNP_DLNA_PARSED_ELEMENT_INVALID
} GUPnPDLNAParsedElement;

typedef enum {
        GUPNP_DLNA_RESTRICTION_TYPE_AUDIO,
        GUPNP_DLNA_RESTRICTION_TYPE_CONTAINER,
        GUPNP_DLNA_RESTRICTION_TYPE_IMAGE,
        GUPNP_DLNA_RESTRICTION_TYPE_VIDEO,
        GUPNP_DLNA_RESTRICTION_TYPE_INVALID
} GUPnPDLNARestrictionType;

typedef struct {
        GList *audios;
        GList *containers;
        GList *images;
        GList *videos;
} GUPnPDLNAProfileData;

typedef struct {
        gchar              *name;
        GUPnPDLNAValueList *list;
} GUPnPDLNANameValueListPair;

typedef struct {
        GList *name_list_pairs;
        GList *parents;
} GUPnPDLNARestrictionData;

typedef struct {
        GUPnPDLNARestriction     *restriction;
        GUPnPDLNARestrictionType  type;
} GUPnPDLNADescription;

struct _GUPnPDLNAProfileLoaderPrivate {
        /* parser part */
        GHashTable *restrictions;
        GHashTable *profile_ids;
        GHashTable *files_hash;
        gboolean    relaxed_mode;
        gboolean    extended_mode;
        /* loader part */
        GHashTable *descriptions;
        GList      *tags_stack;
        GList      *dlna_profile_data_stack;
        GList      *restriction_data_stack;
        char       *dlna_profile_dir;
};

static GUPnPDLNANameValueListPair *
gupnp_dlna_name_value_list_pair_new (const gchar        *name,
                                     GUPnPDLNAValueList *list)
{
        GUPnPDLNANameValueListPair *pair =
                                       g_slice_new (GUPnPDLNANameValueListPair);

        pair->name = g_strdup (name);
        pair->list = list;

        return pair;
}

static void
gupnp_dlna_name_value_list_pair_free (GUPnPDLNANameValueListPair *pair)
{
        if (pair == NULL)
                return;
        g_free (pair->name);
        gupnp_dlna_value_list_free (pair->list);
        g_slice_free (GUPnPDLNANameValueListPair, pair);
}

static GUPnPDLNADescription *
gupnp_dlna_description_new (GUPnPDLNARestriction     *restriction,
                            GUPnPDLNARestrictionType  type)
{
        GUPnPDLNADescription *description = g_slice_new (GUPnPDLNADescription);

        description->restriction = restriction;
        description->type = type;

        return description;
}

static void
gupnp_dlna_description_free (GUPnPDLNADescription *description)
{
        if (description == NULL)
                return;

        gupnp_dlna_restriction_free (description->restriction);
        g_slice_free (GUPnPDLNADescription, description);
}

static GUPnPDLNAProfileData*
gupnp_dlna_profile_data_new (void)
{
        GUPnPDLNAProfileData* data = g_slice_new (GUPnPDLNAProfileData);

        data->audios = NULL;
        data->containers = NULL;
        data->images = NULL;
        data->videos = NULL;

        return data;
}

static void
gupnp_dlna_profile_data_free (GUPnPDLNAProfileData *data)
{
        if (data == NULL)
                return;

        gupnp_dlna_utils_free_restrictions (data->audios);
        gupnp_dlna_utils_free_restrictions (data->containers);
        gupnp_dlna_utils_free_restrictions (data->images);
        gupnp_dlna_utils_free_restrictions (data->videos);
        g_slice_free (GUPnPDLNAProfileData, data);
}

static GUPnPDLNARestrictionData *
gupnp_dlna_restriction_data_new (void)
{
        GUPnPDLNARestrictionData* data = g_slice_new (GUPnPDLNARestrictionData);

        data->name_list_pairs = NULL;
        data->parents = NULL;

        return data;
}

static void
gupnp_dlna_restriction_data_free (GUPnPDLNARestrictionData *data)
{
        if (data == NULL)
                return;

        if (data->name_list_pairs != NULL)
                g_list_free_full
                 (data->name_list_pairs,
                  (GDestroyNotify) gupnp_dlna_name_value_list_pair_free);
        if (data->parents != NULL)
                g_list_free_full (data->parents,
                                  (GDestroyNotify) gupnp_dlna_restriction_free);
        g_slice_free (GUPnPDLNARestrictionData, data);
}

static void
gupnp_dlna_profile_data_stack_free (GList* stack)
{
        g_list_free_full (stack,
                          (GDestroyNotify) gupnp_dlna_profile_data_free);
}

static void
gupnp_dlna_restriction_data_stack_free (GList* stack)
{
        g_list_free_full (stack,
                          (GDestroyNotify) gupnp_dlna_restriction_data_free);
}

static void
push_tag (GUPnPDLNAProfileLoader *loader,
          GUPnPDLNAParsedElement  element)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        gint raw_element = (gint) element;

        priv->tags_stack = g_list_prepend (priv->tags_stack,
                                           GINT_TO_POINTER (raw_element));
}

static void
pop_tag (GUPnPDLNAProfileLoader *loader)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        priv->tags_stack = g_list_delete_link (priv->tags_stack,
                                               priv->tags_stack);
}

static GUPnPDLNAParsedElement
top_tag (GUPnPDLNAProfileLoader *loader)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        if (priv->tags_stack != NULL) {
                gint top_raw = GPOINTER_TO_INT (priv->tags_stack->data);

                return (GUPnPDLNAParsedElement) top_raw;
        }

        return GUPNP_DLNA_PARSED_ELEMENT_INVALID;
}

static void
pre_field (GUPnPDLNAProfileLoader *loader)
{
        push_tag (loader, GUPNP_DLNA_PARSED_ELEMENT_FIELD);
}

static GUPnPDLNAValueType *
value_type_from_string (const gchar *type)
{
        if (!g_strcmp0 (type, "boolean"))
                return gupnp_dlna_value_type_bool ();
        else if (!g_strcmp0 (type, "float")) {
                g_warning ("'float' data type is not yet supported.");

                return NULL;
        } else if (!g_strcmp0 (type, "fourcc")) {
                g_warning ("'fourcc' data type is not yet supported.");

                return NULL;
        } else if (!g_strcmp0 (type, "fraction"))
                return gupnp_dlna_value_type_fraction ();
        else if (!g_strcmp0 (type, "int"))
                return gupnp_dlna_value_type_int ();
        else if (!g_strcmp0 (type, "string"))
                return gupnp_dlna_value_type_string ();
        g_critical ("Unknown value type: %s", type);

        return NULL;
}

static void
append_value_to_list (GUPnPDLNAFieldValue *value,
                      GUPnPDLNAValueList  *list)
{
        if (value == NULL)
                return;

        switch (value->type) {
        case GUPNP_DLNA_FIELD_VALUE_TYPE_RANGE:
                if (!gupnp_dlna_value_list_add_range (list,
                                                      value->value.range.min,
                                                      value->value.range.max))
                        g_warning ("Failed to add range value (%s, %s).",
                                   value->value.range.min,
                                   value->value.range.max);
                break;
        case GUPNP_DLNA_FIELD_VALUE_TYPE_SINGLE:
                if (!gupnp_dlna_value_list_add_single (list,
                                                       value->value.single))
                        g_warning ("Failed to add single value (%s).",
                                   value->value.single);

                break;
        default:
                g_critical ("Unknown field value type: %d", (gint) value->type);
        }
}

static void
post_field (GUPnPDLNAProfileLoader *loader,
            const gchar            *name,
            const gchar            *type,
            GList                  *values)
{
        GUPnPDLNAProfileLoaderPrivate *priv;
        GUPnPDLNARestrictionData *restriction_data;
        GUPnPDLNANameValueListPair *pair;
        GUPnPDLNAValueList *value_list;
        GUPnPDLNAValueType* value_type;
        GList *iter;

        pop_tag (loader);

        if (name == NULL || type == NULL)
                return;

        value_type = value_type_from_string (type);

        if (value_type == NULL)
                return;

        priv = loader->priv;
        restriction_data =
                (GUPnPDLNARestrictionData *) priv->restriction_data_stack->data;
        value_list = gupnp_dlna_value_list_new (value_type);

        for (iter = values; iter != NULL; iter = iter->next) {
                GUPnPDLNAFieldValue *field_value =
                                        (GUPnPDLNAFieldValue *) iter->data;

                append_value_to_list (field_value, value_list);
        }

        pair = gupnp_dlna_name_value_list_pair_new (name, value_list);
        restriction_data->name_list_pairs = g_list_prepend
                                        (restriction_data->name_list_pairs,
                                         pair);
}

static void
merge_restrictions (GUPnPDLNAProfileLoader *loader,
                    GUPnPDLNADescription   *description)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNAProfileData* data =
                   (GUPnPDLNAProfileData *) priv->dlna_profile_data_stack->data;
        GList **target_list;
        GUPnPDLNARestriction *copy;

        if (description == NULL || description->restriction == NULL)
                return;

        switch (description->type) {
        case GUPNP_DLNA_RESTRICTION_TYPE_AUDIO:
                target_list = &data->audios;

                break;
        case GUPNP_DLNA_RESTRICTION_TYPE_CONTAINER:
                target_list = &data->containers;

                break;
        case GUPNP_DLNA_RESTRICTION_TYPE_IMAGE:
                target_list = &data->images;

                break;
        case GUPNP_DLNA_RESTRICTION_TYPE_VIDEO:
                target_list = &data->videos;

                break;
        case GUPNP_DLNA_RESTRICTION_TYPE_INVALID:
        default:
                g_assert_not_reached ();
        }

        copy = gupnp_dlna_restriction_copy (description->restriction);
        *target_list = g_list_prepend (*target_list, copy);
}

static void
merge_restrictions_if_in_dlna_profile (GUPnPDLNAProfileLoader *loader,
                                       GUPnPDLNADescription   *description)
{
        GUPnPDLNAParsedElement element = top_tag (loader);

        if (element == GUPNP_DLNA_PARSED_ELEMENT_DLNA_PROFILE)
                merge_restrictions (loader, description);
}

static void
collect_parents (GUPnPDLNAProfileLoader *loader,
                 GUPnPDLNADescription   *description)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNARestrictionData *data =
                (GUPnPDLNARestrictionData *) priv->restriction_data_stack->data;

        if (description != NULL && description->restriction != NULL) {
                /* Collect parents in a list - we'll
                 * coalesce them later */
                GUPnPDLNARestriction *copy =
                         gupnp_dlna_restriction_copy (description->restriction);

                data->parents = g_list_prepend (data->parents, copy);
        }
}

static void
collect_parents_if_in_restriction (GUPnPDLNAProfileLoader *loader,
                                   GUPnPDLNADescription   *description)
{
        GUPnPDLNAParsedElement element = top_tag (loader);

        if (element == GUPNP_DLNA_PARSED_ELEMENT_RESTRICTION)
                collect_parents (loader, description);
}

static void
pre_parent (GUPnPDLNAProfileLoader *loader)
{
        push_tag (loader, GUPNP_DLNA_PARSED_ELEMENT_PARENT);
}

static void
post_parent (GUPnPDLNAProfileLoader *loader,
             const gchar            *parent)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        pop_tag (loader);

        if (parent != NULL) {
                GUPnPDLNADescription *description = g_hash_table_lookup
                                        (priv->descriptions,
                                         parent);

                merge_restrictions_if_in_dlna_profile (loader, description);
                collect_parents_if_in_restriction (loader, description);
        }
}

static void
pre_restriction (GUPnPDLNAProfileLoader *loader)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNARestrictionData *data = gupnp_dlna_restriction_data_new ();

        push_tag (loader, GUPNP_DLNA_PARSED_ELEMENT_RESTRICTION);

        priv->restriction_data_stack = g_list_prepend
                                        (priv->restriction_data_stack,
                                         data);
}

static GUPnPDLNARestrictionType
restriction_type_from_string (const gchar *type)
{
        if (!g_strcmp0 (type, "audio"))
                return GUPNP_DLNA_RESTRICTION_TYPE_AUDIO;
        else if (!g_strcmp0 (type, "container"))
                return GUPNP_DLNA_RESTRICTION_TYPE_CONTAINER;
        else if (!g_strcmp0 (type, "image"))
                return GUPNP_DLNA_RESTRICTION_TYPE_IMAGE;
        else if (!g_strcmp0 (type, "video"))
                return GUPNP_DLNA_RESTRICTION_TYPE_VIDEO;

        return GUPNP_DLNA_RESTRICTION_TYPE_INVALID;
}

static void
post_restriction (GUPnPDLNAProfileLoader *loader,
                  const gchar            *restriction_type,
                  const gchar            *id,
                  const gchar            *name)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNARestrictionData *data =
                (GUPnPDLNARestrictionData *) priv->restriction_data_stack->data;
        GUPnPDLNARestriction *restriction;
        GUPnPDLNADescription *description;
        GUPnPDLNARestrictionType type;
        GList *iter;

        pop_tag (loader);

        /* If this is NULL then it means that 'used' attribute was
           different from relaxed_mode setting. In this case we just
           ignore it.
         */
        if (restriction_type == NULL)
                goto out;

        restriction = gupnp_dlna_restriction_new (name);

        for (iter = data->name_list_pairs; iter != NULL; iter = iter->next) {
                GUPnPDLNANameValueListPair *pair =
                                      (GUPnPDLNANameValueListPair *) iter->data;

                if (gupnp_dlna_restriction_add_value_list (restriction,
                                                           pair->name,
                                                           pair->list))
                        pair->list = NULL;
        }

        type = restriction_type_from_string (restriction_type);

        if (type == GUPNP_DLNA_RESTRICTION_TYPE_INVALID) {
                g_warning ("Support for '%s' restrictions not yet implemented.",
                           restriction_type);
                goto out;
        }

        iter = data->parents = g_list_reverse (data->parents);
        for (iter = data->parents; iter != NULL; iter = iter->next) {
                /* Merge all the parent caps. The child overrides parent
                 * attributes */
                GUPnPDLNARestriction *parent =
                                        GUPNP_DLNA_RESTRICTION (iter->data);

                gupnp_dlna_restriction_merge (restriction, parent);
                iter->data = NULL;
        }

        description = gupnp_dlna_description_new (restriction, type);
        merge_restrictions_if_in_dlna_profile (loader, description);
        if (id != NULL)
                g_hash_table_replace (priv->descriptions,
                                      g_strdup (id),
                                      description);
        else
                gupnp_dlna_description_free (description);

 out:
        gupnp_dlna_restriction_data_free (data);
        priv->restriction_data_stack = g_list_delete_link
                                        (priv->restriction_data_stack,
                                         priv->restriction_data_stack);
}

static void
pre_restrictions (GUPnPDLNAProfileLoader *loader)
{
        push_tag (loader, GUPNP_DLNA_PARSED_ELEMENT_RESTRICTIONS);
}

static void
post_restrictions (GUPnPDLNAProfileLoader *loader)
{
        pop_tag (loader);
}

static void
pre_dlna_profile (GUPnPDLNAProfileLoader *loader)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNAProfileData* data = gupnp_dlna_profile_data_new ();

        push_tag (loader, GUPNP_DLNA_PARSED_ELEMENT_DLNA_PROFILE);
        priv->dlna_profile_data_stack = g_list_prepend
                                        (priv->dlna_profile_data_stack,
                                         data);
}

static GList *
copy_restrictions_list (GList *list)
{
        GList *dup = NULL;
        GList *iter;

        for (iter = list; iter != NULL; iter = iter->next) {
                GUPnPDLNARestriction *restriction =
                                        GUPNP_DLNA_RESTRICTION (iter->data);
                GUPnPDLNARestriction *copy =
                                      gupnp_dlna_restriction_copy (restriction);

                if (copy)
                        dup = g_list_prepend (dup, copy);
        }

        return dup;
}

static void
merge_base_restrictions (GUPnPDLNAProfileData *data,
                         GUPnPDLNAProfile     *profile)
{
        GList *audio_restrictions =
                            gupnp_dlna_profile_get_audio_restrictions (profile);
        GList *container_restrictions =
                        gupnp_dlna_profile_get_container_restrictions (profile);
        GList *image_restrictions =
                            gupnp_dlna_profile_get_image_restrictions (profile);
        GList *video_restrictions =
                            gupnp_dlna_profile_get_video_restrictions (profile);

        if (audio_restrictions != NULL) {
                GList *copy = copy_restrictions_list (audio_restrictions);

                data->audios = g_list_concat (copy, data->audios);
        }
        if (container_restrictions != NULL) {
                GList *copy = copy_restrictions_list (container_restrictions);

                data->containers = g_list_concat (copy, data->containers);
        }
        if (image_restrictions != NULL) {
                GList *copy = copy_restrictions_list (image_restrictions);

                data->images = g_list_concat (copy, data->images);
        }
        if (video_restrictions != NULL) {
                GList *copy = copy_restrictions_list (video_restrictions);

                data->videos = g_list_concat (copy, data->videos);
        }
}

static gboolean
restrictions_list_is_empty (GList *list)
{
        GList *iter;

        for (iter = list; iter != NULL; iter = iter->next) {
                GUPnPDLNARestriction *restriction =
                                            GUPNP_DLNA_RESTRICTION (iter->data);

                if (restriction != NULL &&
                    !gupnp_dlna_restriction_is_empty (restriction))
                        return FALSE;
        }

        return TRUE;
}

static GUPnPDLNAProfile *
create_profile (GUPnPDLNAProfileLoader *loader,
                GUPnPDLNAProfile       *base,
                const gchar            *name,
                const gchar            *mime,
                gboolean                extended)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNAProfileData *data =
                   (GUPnPDLNAProfileData *) priv->dlna_profile_data_stack->data;
        GList *audio_restrictions = NULL;
        GList *container_restrictions = NULL;
        GList *image_restrictions = NULL;
        GList *video_restrictions = NULL;

        /* Inherit from base profile, if it exists */
        if (base != NULL)
                merge_base_restrictions (data, base);

        /* The merged caps will be our new GUPnPDLNAProfile */
        if (!restrictions_list_is_empty (data->audios)) {
                audio_restrictions = g_list_reverse (data->audios);
                data->audios = NULL;
        }
        if (!restrictions_list_is_empty (data->containers)) {
                container_restrictions = g_list_reverse (data->containers);
                data->containers = NULL;
        }
        if (!restrictions_list_is_empty (data->images)) {
                image_restrictions = g_list_reverse (data->images);
                data->images = NULL;
        }
        if (!restrictions_list_is_empty (data->videos)) {
                video_restrictions = g_list_reverse (data->videos);
                data->videos = NULL;
        }

        return gupnp_dlna_profile_new (name,
                                       mime,
                                       audio_restrictions,
                                       container_restrictions,
                                       image_restrictions,
                                       video_restrictions,
                                       extended);
}

static void
post_dlna_profile (GUPnPDLNAProfileLoader *loader)
{
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        GUPnPDLNAProfileData *data =
                   (GUPnPDLNAProfileData *) priv->dlna_profile_data_stack->data;

        pop_tag (loader);
        gupnp_dlna_profile_data_free (data);
        priv->dlna_profile_data_stack = g_list_delete_link
                                        (priv->dlna_profile_data_stack,
                                         priv->dlna_profile_data_stack);
}

static GList *
cleanup (GUPnPDLNAProfileLoader *loader G_GNUC_UNUSED,
         GList *profiles)
{
        /* Now that we're done loading profiles, remove all profiles
         * with no name which are only used for inheritance and not
         * matching. */
        GList *iter = profiles;

        while (iter != NULL) {
                GList *next = iter->next;
                GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (iter->data);
                const gchar *name = gupnp_dlna_profile_get_name (profile);

                if (name == NULL || name[0] == '\0') {
                        profiles = g_list_delete_link (profiles, iter);
                        g_object_unref (profile);
                } else {
                        /* TODO: simplify restrictions in profile if
                         * possible.
                         */
                }

                iter = next;
        }

        for (iter = profiles; iter != NULL; iter = iter->next) {
                GUPnPDLNAProfile *profile = GUPNP_DLNA_PROFILE (iter->data);
                gchar *acaps = gupnp_dlna_utils_restrictions_list_to_string
                          (gupnp_dlna_profile_get_audio_restrictions (profile));
                gchar *ccaps = gupnp_dlna_utils_restrictions_list_to_string
                      (gupnp_dlna_profile_get_container_restrictions (profile));
                gchar *icaps = gupnp_dlna_utils_restrictions_list_to_string
                          (gupnp_dlna_profile_get_image_restrictions (profile));
                gchar *vcaps = gupnp_dlna_utils_restrictions_list_to_string
                          (gupnp_dlna_profile_get_video_restrictions (profile));

                g_debug ("Loaded profile: %s\nMIME: %s\naudio caps: %s\n"
                         "container caps: %s\nimage caps: %s\nvideo caps: %s\n",
                         gupnp_dlna_profile_get_name (profile),
                         gupnp_dlna_profile_get_mime (profile),
                         acaps,
                         ccaps,
                         icaps,
                         vcaps);
                g_free (acaps);
                g_free (ccaps);
                g_free (icaps);
                g_free (vcaps);
        }

        return profiles;
}

static void
gupnp_dlna_profile_loader_get_property (GObject    *object,
                                        guint       prop_id,
                                        GValue     *value,
                                        GParamSpec *pspec)
{
        GUPnPDLNAProfileLoader *loader = GUPNP_DLNA_PROFILE_LOADER (object);
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        switch (prop_id) {
        case PROP_RELAXED_MODE:
                g_value_set_boolean (value, priv->relaxed_mode);
                break;

        case PROP_EXTENDED_MODE:
                g_value_set_boolean (value, priv->extended_mode);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gupnp_dlna_profile_loader_set_property (GObject      *object,
                                        guint         prop_id,
                                        const GValue *value,
                                        GParamSpec   *pspec)
{
        GUPnPDLNAProfileLoader *loader = GUPNP_DLNA_PROFILE_LOADER (object);
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        switch (prop_id) {
        case PROP_RELAXED_MODE:
                priv->relaxed_mode = g_value_get_boolean (value);
                break;

        case PROP_EXTENDED_MODE:
                priv->extended_mode = g_value_get_boolean (value);
                break;

        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
gupnp_dlna_profile_loader_dispose (GObject *object)
{
        GUPnPDLNAProfileLoader *loader = GUPNP_DLNA_PROFILE_LOADER (object);
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        g_clear_pointer (&priv->restrictions, g_hash_table_unref);
        g_clear_pointer (&priv->profile_ids, g_hash_table_unref);
        g_clear_pointer (&priv->files_hash, g_hash_table_unref);

        g_clear_pointer (&priv->descriptions, g_hash_table_unref);
        g_clear_pointer (&priv->tags_stack, g_list_free);
        g_clear_pointer (&priv->dlna_profile_data_stack,
                         gupnp_dlna_profile_data_stack_free);
        g_clear_pointer (&priv->restriction_data_stack,
                         gupnp_dlna_restriction_data_stack_free);
        g_clear_pointer (&priv->dlna_profile_dir, g_free);

        G_OBJECT_CLASS (gupnp_dlna_profile_loader_parent_class)->dispose
                                        (object);
}

static void
gupnp_dlna_profile_loader_class_init (GUPnPDLNAProfileLoaderClass *loader_class)
{
        GObjectClass *object_class = G_OBJECT_CLASS (loader_class);
        GParamSpec *spec;

        object_class->get_property = gupnp_dlna_profile_loader_get_property;
        object_class->set_property = gupnp_dlna_profile_loader_set_property;
        object_class->dispose = gupnp_dlna_profile_loader_dispose;

        spec = g_param_spec_boolean ("relaxed-mode",
                                     "Relaxed mode",
                                     "Whether loader loads relaxed profiles",
                                     TRUE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_property (object_class,
                                         PROP_RELAXED_MODE,
                                         spec);

        spec = g_param_spec_boolean ("extended-mode",
                                     "Extended mode",
                                     "Whether loader loads extended profiles",
                                     TRUE,
                                     G_PARAM_READWRITE |
                                     G_PARAM_CONSTRUCT_ONLY |
                                     G_PARAM_STATIC_STRINGS);

        g_object_class_install_property (object_class,
                                         PROP_EXTENDED_MODE,
                                         spec);

        g_type_class_add_private (loader_class,
                                  sizeof (GUPnPDLNAProfileLoaderPrivate));
}

static void
gupnp_dlna_profile_loader_init (GUPnPDLNAProfileLoader *self)
{
        GUPnPDLNAProfileLoaderPrivate *priv = G_TYPE_INSTANCE_GET_PRIVATE
                                        (self,
                                         GUPNP_TYPE_DLNA_PROFILE_LOADER,
                                         GUPnPDLNAProfileLoaderPrivate);

        priv->restrictions = g_hash_table_new_full (g_str_hash,
                                                    g_str_equal,
                                                    g_free,
                                                    NULL);
        priv->profile_ids = g_hash_table_new_full (g_str_hash,
                                                   g_str_equal,
                                                   g_free,
                                                   g_object_unref);
        priv->files_hash = g_hash_table_new_full (g_str_hash,
                                                  g_str_equal,
                                                  g_free,
                                                  NULL);

        priv->descriptions = g_hash_table_new_full
                                 (g_str_hash,
                                  g_str_equal,
                                  g_free,
                                  (GDestroyNotify) gupnp_dlna_description_free);
        priv->tags_stack = NULL;
        priv->dlna_profile_data_stack = NULL;
        priv->restriction_data_stack = NULL;

        self->priv = priv;
}

static GUPnPDLNAFieldValue *
get_value (xmlTextReaderPtr reader)
{
        GUPnPDLNAFieldValue *value = NULL;
        xmlChar *curr = xmlTextReaderName (reader);
        int ret = 1;

        /* This function may be called with reader pointing to a <field> or
         * the element just below a <field>. In the former case, we move the
         * cursor forward and then continue processing. */
        if (xmlStrEqual (curr, BAD_CAST ("field")))
                ret = xmlTextReaderRead (reader);
        xmlFree (curr);

        while (ret == 1) {
                xmlChar *tag = xmlTextReaderName (reader);

                if (xmlTextReaderNodeType (reader) == NODE_TYPE_ELEMENT_START &&
                    xmlStrEqual (tag, BAD_CAST ("value"))) {
                        /* <value> */
                        gint node_type;

                        /* Note: This assumes you won't have a comment in the
                         *       middle of your text */
                        do {
                                ret = xmlTextReaderRead (reader);
                                node_type = xmlTextReaderNodeType (reader);
                        } while (ret == 1 &&
                                 node_type != NODE_TYPE_TEXT &&
                                 node_type != NODE_TYPE_ELEMENT_END);

                        /* We're now at the real text between a <value> and a
                         * </value> */

                        if (node_type == NODE_TYPE_TEXT) {
                                xmlChar *string = xmlTextReaderValue (reader);

                                value = gupnp_dlna_field_value_new_single
                                                            ((gchar *) string);
                                if (string)
                                        xmlFree (string);
                        }
                }

                if (xmlTextReaderNodeType (reader) == NODE_TYPE_ELEMENT_END &&
                    xmlStrEqual (tag, BAD_CAST ("value"))) {
                        /* </value> */
                        xmlFree (tag);

                        break;
                }

                xmlFree (tag);
                ret = xmlTextReaderRead (reader);
        }

        if (!value)
                g_warning ("Empty <value>s are illegal");

        return value;
}

static GUPnPDLNAFieldValue *
get_range (xmlTextReaderPtr reader)
{
        xmlChar *min = xmlTextReaderGetAttribute (reader, BAD_CAST ("min"));
        xmlChar *max = xmlTextReaderGetAttribute (reader, BAD_CAST ("max"));
        GUPnPDLNAFieldValue *value = gupnp_dlna_field_value_new_range
                                        ((gchar *) min,
                                         (gchar *) max);

        if (min)
                xmlFree (min);
        if (max)
                xmlFree (max);

        return value;
}

static void
process_field (GUPnPDLNAProfileLoader *loader,
               xmlTextReaderPtr        reader)
{
        xmlChar *name = NULL;
        xmlChar *type = NULL;
        xmlChar *used;
        GList *values = NULL;
        gboolean done = FALSE;
        gboolean skip = FALSE;
        GUPnPDLNAFieldValue *value = NULL;
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        pre_field (loader);

        /*
         * Parse the 'used' attribute and figure out the mode we
         * need to follow.
         */
        used = xmlTextReaderGetAttribute (reader, BAD_CAST ("used"));
        if (used) {
                if ((priv->relaxed_mode == FALSE) &&
                    xmlStrEqual (used, BAD_CAST ("in-relaxed"))) {
                        skip = TRUE;
                } else if ((priv->relaxed_mode == TRUE) &&
                           (xmlStrEqual (used, BAD_CAST ("in-strict")))) {
                        skip = TRUE;
                }

                xmlFree (used);
        }

        if (!skip) {
                name = xmlTextReaderGetAttribute (reader, BAD_CAST ("name"));
                type = xmlTextReaderGetAttribute (reader, BAD_CAST ("type"));
        }

        /* I don't like it - we should check done first, then try to
         * read next tag. But this is how it was done in original
         * code, so I am not touching it for now - krnowak.
         */
        while ((xmlTextReaderRead (reader) == 1) && !done) {
                xmlChar *tag = xmlTextReaderName (reader);

                switch (xmlTextReaderNodeType (reader)) {
                case NODE_TYPE_ELEMENT_START:
                        if (skip)
                                break;

                        if (xmlStrEqual (tag, BAD_CAST ("range"))) {
                                /* <range> */
                                value = get_range (reader);
                        } else if (xmlStrEqual (tag, BAD_CAST ("value"))) {
                                /* <value> */
                                value = get_value (reader);
                        }

                        if (value)
                                values = g_list_prepend (values, value);
                        break;

                case NODE_TYPE_ELEMENT_END:
                        if (xmlStrEqual (tag, BAD_CAST ("field")))
                                /* </field> */
                                done = TRUE;

                        break;

                default:
                        break;
                }

                xmlFree (tag);
        }

        if (values)
                values = g_list_reverse (values);

        post_field (loader, (gchar *)name, (gchar *)type, values);

        if (name)
                xmlFree (name);
        if (type)
                xmlFree (type);
        if (values) {
                g_list_free_full (values,
                                  (GDestroyNotify) gupnp_dlna_field_value_free);
        }
}

static void
process_parent (GUPnPDLNAProfileLoader *loader,
                xmlTextReaderPtr        reader)
{
        xmlChar *parent = NULL;
        xmlChar *used = NULL;
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        pre_parent (loader);

        /*
         * Check to see if we need to follow any relaxed/strict mode
         * restrictions.
         */
        used = xmlTextReaderGetAttribute (reader, BAD_CAST ("used"));
        if (used) {
                gboolean go_out = FALSE;

                if ((priv->relaxed_mode == FALSE) &&
                    xmlStrEqual (used, BAD_CAST ("in-relaxed"))) {
                        go_out = TRUE;
                } else if ((priv->relaxed_mode == TRUE) &&
                           (xmlStrEqual (used, BAD_CAST ("in-strict")))) {
                        go_out = TRUE;
                }
                xmlFree (used);
                if (go_out)
                        goto out;
        }

        parent = xmlTextReaderGetAttribute (reader, BAD_CAST ("name"));

        if (!g_hash_table_contains (priv->restrictions, parent))
                g_warning ("Could not find parent restriction: %s", parent);

 out:
        post_parent (loader, (gchar*) parent);

        if (parent)
                xmlFree (parent);
}

static void
process_restriction (GUPnPDLNAProfileLoader *loader,
                     xmlTextReaderPtr        reader)
{
        xmlChar *used;
        gboolean done = FALSE;
        gboolean skip = FALSE;
        xmlChar *id = NULL;
        xmlChar *type = NULL;
        gchar *name = NULL;
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        pre_restriction (loader);
        /*
         * First we parse the 'used' attribute and figure out
         * the mode we need to comply to.
         */
        used = xmlTextReaderGetAttribute (reader, BAD_CAST ("used"));
        if (used) {
                if ((priv->relaxed_mode == FALSE) &&
                    xmlStrEqual (used, BAD_CAST ("in-relaxed"))) {
                        skip = TRUE;
                } else if ((priv->relaxed_mode == TRUE) &&
                           (xmlStrEqual (used, BAD_CAST ("in-strict")))) {
                        skip = TRUE;
                }
                xmlFree (used);
        }

        if (!skip) {
                id = xmlTextReaderGetAttribute (reader, BAD_CAST ("id"));
                type = xmlTextReaderGetAttribute (reader, BAD_CAST ("type"));
        }

        /* I don't like it - we should check done first, then try to
         * read next tag. But this is how it was done in original
         * code, so I am not touching it for now - krnowak.
         */
        while ((xmlTextReaderRead (reader) == 1) && !done) {
                xmlChar *tag = xmlTextReaderName (reader);

                switch (xmlTextReaderNodeType (reader)) {
                case NODE_TYPE_ELEMENT_START:
                        if (skip)
                                break;

                        if (xmlStrEqual (tag, BAD_CAST ("field"))) {
                                /* <field> */
                                xmlChar *field;

                                field = xmlTextReaderGetAttribute
                                        (reader,
                                         BAD_CAST ("name"));

                                if (xmlStrEqual (field, BAD_CAST ("name"))) {
                                        GUPnPDLNAFieldValue *value = get_value
                                                (reader);

                                        if (value) {
                                                /* get_value returns
                                                   single type
                                                   value. */
                                                name =
                                                 g_strdup (value->value.single);
                                                gupnp_dlna_field_value_free
                                                        (value);
                                        }
                                } else
                                        process_field (loader, reader);

                                xmlFree (field);
                        } else if (xmlStrEqual (tag, BAD_CAST ("parent"))) {
                                /* <parent> */
                                process_parent (loader, reader);
                        }

                        break;

                case NODE_TYPE_ELEMENT_END:
                        if (xmlStrEqual (tag, BAD_CAST ("restriction")))
                                /* </restriction> */
                                done = TRUE;

                        break;

                default:
                        break;
                }

                xmlFree (tag);
        }

        if (skip)
                goto out;

        if (id)
                g_hash_table_add (priv->restrictions, g_strdup ((gchar *) id));

out:
        post_restriction (loader, (gchar *) type, (gchar *) id, name);
        if (type)
                xmlFree (type);
        if (id)
                xmlFree (id);
        g_free (name);
}

static void
process_restrictions (GUPnPDLNAProfileLoader *loader,
                      xmlTextReaderPtr        reader)
{
        gboolean done = FALSE;

        pre_restrictions (loader);

        while (!done && xmlTextReaderRead (reader) == 1) {
                xmlChar *tag = xmlTextReaderName (reader);

                switch (xmlTextReaderNodeType (reader)) {
                case NODE_TYPE_ELEMENT_START:
                        if (xmlStrEqual (tag, BAD_CAST ("restriction"))) {
                                /* <restriction> */
                                process_restriction (loader, reader);
                        }

                        break;

                case NODE_TYPE_ELEMENT_END:
                        if (xmlStrEqual (tag, BAD_CAST ("restrictions")))
                                /* </restrictions> */
                                done = TRUE;

                default:
                        break;
                }

                xmlFree (tag);
        }

        post_restrictions (loader);
}

static void
process_dlna_profile (GUPnPDLNAProfileLoader  *loader,
                      xmlTextReaderPtr         reader,
                      GList                  **profiles)
{
        GUPnPDLNAProfile *profile = NULL;
        GUPnPDLNAProfile *base = NULL;
        xmlChar *name;
        xmlChar *mime;
        xmlChar *id;
        xmlChar *base_profile;
        xmlChar *extended;
        gboolean done = FALSE;
        gboolean is_extended = FALSE;
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;

        pre_dlna_profile (loader);

        name = xmlTextReaderGetAttribute (reader, BAD_CAST ("name"));
        mime = xmlTextReaderGetAttribute (reader, BAD_CAST ("mime"));
        extended = xmlTextReaderGetAttribute (reader, BAD_CAST ("extended"));
        id = xmlTextReaderGetAttribute (reader, BAD_CAST ("id"));
        base_profile = xmlTextReaderGetAttribute (reader,
                                                  BAD_CAST ("base-profile"));

        if (!name) {
                g_assert (mime == NULL);

                /* We need a non-NULL string to not trigger asserts in the
                 * places these are used. Profiles without names are used
                 * only for inheritance, not for actual matching. */
                name = xmlStrdup (BAD_CAST (""));
                mime = xmlStrdup (BAD_CAST (""));
        }

        if (extended && xmlStrEqual (extended, BAD_CAST ("true"))) {
                /* If we're not in extended mode, skip this profile */
                if (!priv->extended_mode)
                        goto out;

                is_extended = TRUE;
        }

        /* I don't like it - we should check done first, then try to
         * read next tag. But this is how it was done in original
         * code, so I am not touching it for now - krnowak.
         */
        while ((xmlTextReaderRead (reader) == 1) && !done) {
                xmlChar *tag = xmlTextReaderName (reader);

                switch (xmlTextReaderNodeType (reader)) {
                case NODE_TYPE_ELEMENT_START:
                        if (xmlStrEqual (tag, BAD_CAST ("restriction")))
                                process_restriction (loader, reader);
                        else if (xmlStrEqual (tag, BAD_CAST ("parent")))
                                process_parent (loader, reader);
                        break;

                case NODE_TYPE_ELEMENT_END:
                        if (xmlStrEqual (tag, BAD_CAST ("dlna-profile")))
                                done = TRUE;

                default:
                        break;
                }

                xmlFree (tag);
        }

        if (base_profile) {
                base = g_hash_table_lookup (priv->profile_ids, base_profile);
                if (!base)
                        g_warning ("Invalid base-profile reference");
        }


        profile = create_profile (loader,
                                  base,
                                  (gchar *) name,
                                  (gchar *) mime,
                                  is_extended);

        *profiles = g_list_prepend (*profiles, profile);

        if (id) {
                g_hash_table_replace (priv->profile_ids,
                                      g_strdup ((gchar *) id),
                                      g_object_ref (profile));
        }

out:
        post_dlna_profile (loader);

        if (id)
                xmlFree (id);
        if (mime)
                xmlFree (mime);
        if (name)
                xmlFree (name);
        if (extended)
                xmlFree (extended);
        if (base_profile)
                xmlFree (base_profile);
}

static void
gupnp_dlna_profile_loader_get_from_file (GUPnPDLNAProfileLoader  *loader,
                                         const char              *file_name,
                                         GList                  **profiles);

static void
process_include (GUPnPDLNAProfileLoader  *loader,
                 xmlTextReaderPtr         reader,
                 GList                  **profiles)
{
        xmlChar *path;
        gchar *g_path;

        path = xmlTextReaderGetAttribute (reader, BAD_CAST ("ref"));
        g_path = g_strdup ((gchar *) path);
        if (path)
                xmlFree (path);

        if (!g_path_is_absolute (g_path)) {
                gchar *tmp = g_build_filename (loader->priv->dlna_profile_dir,
                                               g_path,
                                               NULL);
                g_free (g_path);
                g_path = tmp;
        }

        gupnp_dlna_profile_loader_get_from_file (loader, g_path, profiles);
        g_free (g_path);
}

/* This can go away once we have a glib function to canonicalize paths (see
 * https://bugzilla.gnome.org/show_bug.cgi?id=111848
 *
 * The implementation is not generic enough, but sufficient for our use. The
 * idea is taken from Tristan Van Berkom's comment in the bug mentioned above:
 *
 *   1. cd dirname(path)
 *   2. absdir = $CWD
 *   3. cd $OLDPWD
 *   4. abspath = absdir + basename(path)
 */
static gchar *
canonicalize_path_name (const char *path)
{
        gchar *dir_name = NULL, *file_name = NULL, *abs_dir = NULL,
              *old_dir = NULL, *ret = NULL;

        if (g_path_is_absolute (path))
                return g_strdup (path);

        old_dir = g_get_current_dir ();
        dir_name = g_path_get_dirname (path);

        if (g_chdir (dir_name) < 0) {
                ret = g_strdup (path);
                goto out;
        }

        abs_dir = g_get_current_dir ();
        g_chdir (old_dir);

        file_name = g_path_get_basename (path);
        ret = g_build_filename (abs_dir, file_name, NULL);

out:
        g_free (dir_name);
        g_free (file_name);
        g_free (abs_dir);
        g_free (old_dir);

        return ret;
}

static void
gupnp_dlna_profile_loader_get_from_file (GUPnPDLNAProfileLoader  *loader,
                                         const char              *file_name,
                                         GList                  **profiles)
{
        gchar *path = canonicalize_path_name (file_name);
        xmlTextReaderPtr reader = NULL;
        xmlRelaxNGParserCtxtPtr rngp = NULL;
        xmlRelaxNGPtr rngs = NULL;
        GUPnPDLNAProfileLoaderPrivate *priv = loader->priv;
        char *rng_path = NULL;

        if (g_hash_table_contains (priv->files_hash, path))
                goto out;
        else
                g_hash_table_add (priv->files_hash, g_strdup (path));

        reader = xmlNewTextReaderFilename (path);
        if (!reader)
                goto out;

        /* Load the schema for validation */
        rng_path = g_build_filename (loader->priv->dlna_profile_dir,
                                     "dlna-profiles.rng",
                                     NULL);
        rngp = xmlRelaxNGNewParserCtxt (rng_path);
        g_free (rng_path);

        if (!rngp)
                goto out;
        rngs = xmlRelaxNGParse (rngp);
        if (!rngs)
                goto out;
        if (xmlTextReaderRelaxNGSetSchema (reader, rngs) < 0)
                goto out;

        while (xmlTextReaderRead (reader) == 1) {
                xmlChar *tag = xmlTextReaderName (reader);

                switch (xmlTextReaderNodeType (reader)) {
                        case NODE_TYPE_ELEMENT_START:
                                if (xmlStrEqual (tag, BAD_CAST ("include"))) {
                                        /* <include> */
                                        process_include (loader,
                                                         reader,
                                                         profiles);
                                } else if (xmlStrEqual (tag,
                                        BAD_CAST ("restrictions"))) {
                                        /* <restrictions> */
                                        process_restrictions (loader,
                                                              reader);
                                } else if (xmlStrEqual (tag,
                                        BAD_CAST ("dlna-profile"))) {
                                        /* <dlna-profile> */
                                        process_dlna_profile (loader,
                                                              reader,
                                                              profiles);

                                }

                                break;

                        default:
                                break;
                }

                xmlFree (tag);
        }


out:
        g_free (path);
        if (reader)
                xmlFreeTextReader (reader);
        if (rngs)
                xmlRelaxNGFree (rngs);
        if (rngp)
                xmlRelaxNGFreeParserCtxt (rngp);
}

static GList *
gupnp_dlna_profile_loader_get_from_dir (GUPnPDLNAProfileLoader *loader,
                                        const gchar            *profile_dir)
{
        GDir *dir;
        GList *profiles = NULL;

        g_debug ("Loading DLNA profiles from %s", profile_dir);

        if ((dir = g_dir_open (profile_dir, 0, NULL))) {
                const gchar *entry;

                while ((entry = g_dir_read_name (dir))) {
                        gchar *path = g_build_filename (profile_dir,
                                                        entry,
                                                        NULL);

                        if (g_str_has_suffix (entry, ".xml") &&
                            g_file_test (path, G_FILE_TEST_IS_REGULAR)) {
                                gupnp_dlna_profile_loader_get_from_file
                                        (loader,
                                         path,
                                         &profiles);
                        }

                        g_free (path);
                }

                g_dir_close (dir);
        }

        return profiles;
}

GUPnPDLNAProfileLoader *
gupnp_dlna_profile_loader_new (gboolean relaxed_mode,
                               gboolean extended_mode)
{
        return GUPNP_DLNA_PROFILE_LOADER (g_object_new
                                        (GUPNP_TYPE_DLNA_PROFILE_LOADER,
                                         "relaxed-mode", relaxed_mode,
                                         "extended-mode", extended_mode,
                                         NULL));
}

GList *
gupnp_dlna_profile_loader_get_from_disk (GUPnPDLNAProfileLoader *loader)
{
        GList *profiles;

        g_return_val_if_fail (GUPNP_IS_DLNA_PROFILE_LOADER (loader), NULL);

        if (loader->priv->dlna_profile_dir == NULL) {
                char **env = NULL;
                const char *profile_dir = NULL;

                env = g_get_environ ();
                profile_dir = g_environ_getenv (env, "GUPNP_DLNA_PROFILE_DIR");
                if (profile_dir != NULL && g_path_is_absolute (profile_dir)) {
                        loader->priv->dlna_profile_dir = g_strdup (profile_dir);
                } else {
                        loader->priv->dlna_profile_dir = g_strdup (DLNA_DATA_DIR);
                }


                g_strfreev (env);
        }

        profiles = gupnp_dlna_profile_loader_get_from_dir
                                        (loader,
                                         loader->priv->dlna_profile_dir);

        profiles = g_list_reverse (profiles);

        return cleanup (loader, profiles);
}
