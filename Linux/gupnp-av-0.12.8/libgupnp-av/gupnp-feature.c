/*
 * Copyright (C) 2012 Intel Corporation
 *
 * Authors: Regis Merlino <regis.merlino@intel.com>
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
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:gupnp-feature
 * @short_description: ContentDirectory feature
 *
 * #GUPnPFeature respresent a Feature element.
 */

#include "gupnp-feature.h"

G_DEFINE_TYPE (GUPnPFeature,
               gupnp_feature,
               G_TYPE_OBJECT);

struct _GUPnPFeaturePrivate {
        char *name;
        char *version;
        char *object_ids;
};

enum {
        PROP_0,
        PROP_NAME,
        PROP_VERSION,
        PROP_OBJECT_IDS
};

static void
gupnp_feature_init (GUPnPFeature *object)
{
        object->priv = G_TYPE_INSTANCE_GET_PRIVATE (object,
                                                    GUPNP_TYPE_FEATURE,
                                                    GUPnPFeaturePrivate);
}

static void
gupnp_feature_finalize (GObject *object)
{
        GObjectClass        *object_class;
        GUPnPFeaturePrivate *priv;

        priv = GUPNP_FEATURE (object)->priv;
        g_free (priv->name);
        g_free (priv->version);
        g_free (priv->object_ids);

        object_class = G_OBJECT_CLASS (gupnp_feature_parent_class);
        object_class->finalize (object);
}

static void
gupnp_feature_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
        GUPnPFeature *feature;

        feature = GUPNP_FEATURE (object);

        switch (property_id) {
        case PROP_NAME:
                g_value_set_string (value, gupnp_feature_get_name (feature));
                break;
        case PROP_VERSION:
                g_value_set_string (value, gupnp_feature_get_version (feature));
                break;
        case PROP_OBJECT_IDS:
                g_value_set_string (value,
                                    gupnp_feature_get_object_ids (feature));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_feature_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
        GUPnPFeature *feature;

        feature = GUPNP_FEATURE (object);

        switch (property_id) {
        case PROP_NAME:
                feature->priv->name = g_value_dup_string (value);
                break;
        case PROP_VERSION:
                feature->priv->version = g_value_dup_string (value);
                break;
        case PROP_OBJECT_IDS:
                feature->priv->object_ids = g_value_dup_string (value);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
                break;
        }
}

static void
gupnp_feature_class_init (GUPnPFeatureClass *klass)
{
        GObjectClass *object_class;

        object_class = G_OBJECT_CLASS (klass);

        object_class->get_property = gupnp_feature_get_property;
        object_class->set_property = gupnp_feature_set_property;
        object_class->finalize = gupnp_feature_finalize;

        g_type_class_add_private (klass, sizeof (GUPnPFeaturePrivate));

        /**
         * GUPnPFeature:name:
         *
         * The name of this feature.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_NAME,
                 g_param_spec_string ("name",
                                      "Name",
                                      "The name of this feature.",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPFeature:version:
         *
         * The version of this feature.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_VERSION,
                 g_param_spec_string ("version",
                                      "Version",
                                      "The version of this feature.",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));

        /**
         * GUPnPFeature:object-ids:
         *
         * The object IDs related to this feature.
         **/
        g_object_class_install_property
                (object_class,
                 PROP_OBJECT_IDS,
                 g_param_spec_string ("object-ids",
                                      "Object IDs",
                                      "The object IDs of this feature.",
                                      NULL,
                                      G_PARAM_READWRITE |
                                      G_PARAM_CONSTRUCT_ONLY |
                                      G_PARAM_STATIC_NAME |
                                      G_PARAM_STATIC_NICK |
                                      G_PARAM_STATIC_BLURB));
}

/**
 * gupnp_feature_get_name:
 * @feature: #GUPnPFeature
 *
 * Get the name of the @feature.
 *
 * Return value: The name of the @feature.
 **/
const char *
gupnp_feature_get_name (GUPnPFeature *feature)
{
        return feature->priv->name;
}

/**
 * gupnp_feature_get_version:
 * @feature: #GUPnPFeature
 *
 * Get the version of the @feature.
 *
 * Return value: The version of the @feature.
 **/
const char *
gupnp_feature_get_version (GUPnPFeature *feature)
{
        return feature->priv->version;
}

/**
 * gupnp_feature_get_object_ids:
 * @feature: #GUPnPFeature
 *
 * Get the object IDs related to the @feature.
 *
 * Return value: The object IDs related to the @feature.
 **/
const char *
gupnp_feature_get_object_ids (GUPnPFeature *feature)
{
        return feature->priv->object_ids;
}
