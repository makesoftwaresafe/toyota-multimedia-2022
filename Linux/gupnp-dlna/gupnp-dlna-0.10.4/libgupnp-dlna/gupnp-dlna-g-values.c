/*
 * Copyright (C) 2012, 2013 Intel Corporation.
 *
 * Authors: Krzesimir Nowak <krnowak@openismus.com>
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

#include "gupnp-dlna-g-values-private.h"

G_DEFINE_BOXED_TYPE (GUPnPDLNAFraction,
                     gupnp_dlna_fraction,
                     gupnp_dlna_fraction_copy,
                     gupnp_dlna_fraction_free)

G_DEFINE_BOXED_TYPE (GUPnPDLNAFractionRange,
                     gupnp_dlna_fraction_range,
                     gupnp_dlna_fraction_range_copy,
                     gupnp_dlna_fraction_range_free)

G_DEFINE_BOXED_TYPE (GUPnPDLNAIntRange,
                     gupnp_dlna_int_range,
                     gupnp_dlna_int_range_copy,
                     gupnp_dlna_int_range_free)

GUPnPDLNAFractionRange *
gupnp_dlna_fraction_range_new_take (GUPnPDLNAFraction *min,
                                    GUPnPDLNAFraction *max)
{
        GUPnPDLNAFractionRange *range;

        g_return_val_if_fail (min != NULL, NULL);
        g_return_val_if_fail (max != NULL, NULL);

        range = g_slice_new (GUPnPDLNAFractionRange);
        range->min = min;
        range->max = max;

        return range;
}

GUPnPDLNAIntRange *
gupnp_dlna_int_range_new (gint min,
                          gint max)
{
        GUPnPDLNAIntRange *range;

        g_return_val_if_fail (min <= max, NULL);

        range = g_slice_new (GUPnPDLNAIntRange);
        range->min = min;
        range->max = max;

        return range;
}

/**
 * gupnp_dlna_fraction_copy:
 * @fraction: (transfer none): A fraction to copy.
 *
 * Returns: (transfer full): A copy of @fraction.
 */
GUPnPDLNAFraction *
gupnp_dlna_fraction_copy (GUPnPDLNAFraction *fraction)
{
        GUPnPDLNAFraction *dup;

        if (fraction == NULL)
                return NULL;

        dup = g_slice_new (GUPnPDLNAFraction);
        dup->numerator = fraction->numerator;
        dup->denominator = fraction->denominator;

        return dup;
}

/**
 * gupnp_dlna_fraction_free:
 * @fraction: A fraction to free.
 *
 * Frees @fraction.
 */
void
gupnp_dlna_fraction_free (GUPnPDLNAFraction *fraction)
{
        if (fraction == NULL)
                return;

        g_slice_free (GUPnPDLNAFraction, fraction);
}

/**
 * gupnp_dlna_fraction_get_numerator:
 * @fraction: A fraction.
 *
 * Returns: @fraction's numerator.
 */
gint
gupnp_dlna_fraction_get_numerator (GUPnPDLNAFraction *fraction)
{
        g_return_val_if_fail (fraction != NULL, 0);

        return fraction->numerator;
}

/**
 * gupnp_dlna_fraction_get_denominator:
 * @fraction: A fraction.
 *
 * Returns: @fraction's denominator.
 */
gint
gupnp_dlna_fraction_get_denominator (GUPnPDLNAFraction *fraction)
{
        g_return_val_if_fail (fraction != NULL, 0);

        return fraction->denominator;
}

/**
 * gupnp_dlna_fraction_range_copy:
 * @range: (transfer none): A range to copy.
 *
 * Returns: (transfer full): A copy of @range.
 */
GUPnPDLNAFractionRange *
gupnp_dlna_fraction_range_copy (GUPnPDLNAFractionRange *range)
{
        if (range == NULL)
                return NULL;

        return gupnp_dlna_fraction_range_new_take
                                        (gupnp_dlna_fraction_copy (range->min),
                                         gupnp_dlna_fraction_copy (range->max));
}

/**
 * gupnp_dlna_fraction_range_free:
 * @range: A range to free.
 *
 * Frees @range.
 */
void
gupnp_dlna_fraction_range_free (GUPnPDLNAFractionRange *range)
{
        if (range == NULL)
                return;

        gupnp_dlna_fraction_free (range->min);
        gupnp_dlna_fraction_free (range->max);
        g_slice_free (GUPnPDLNAFractionRange, range);
}

/**
 * gupnp_dlna_fraction_range_get_min:
 * @range: A range.
 *
 * Returns: (transfer none): Minimum value of @range.
 */
GUPnPDLNAFraction *
gupnp_dlna_fraction_range_get_min (GUPnPDLNAFractionRange *range)
{
        g_return_val_if_fail (range != NULL, NULL);

        return range->min;
}

/**
 * gupnp_dlna_fraction_range_get_max:
 * @range: A range.
 *
 * Returns: (transfer none): Maximum value of @range.
 */
GUPnPDLNAFraction *
gupnp_dlna_fraction_range_get_max (GUPnPDLNAFractionRange *range)
{
        g_return_val_if_fail (range != NULL, NULL);

        return range->max;
}

/**
 * gupnp_dlna_int_range_copy:
 * @range: (transfer none): A range to copy.
 *
 * Returns: (transfer full): A copy of @range.
 */
GUPnPDLNAIntRange *
gupnp_dlna_int_range_copy (GUPnPDLNAIntRange *range)
{
        if (range == NULL)
                return NULL;

        return gupnp_dlna_int_range_new (range->min, range->max);
}

/**
 * gupnp_dlna_int_range_free:
 * @range: A range to free.
 *
 * Frees @range.
 */
void
gupnp_dlna_int_range_free (GUPnPDLNAIntRange *range)
{
        if (range == NULL)
                return;

        g_slice_free (GUPnPDLNAIntRange, range);
}

/**
 * gupnp_dlna_int_range_get_min:
 * @range: A range.
 *
 * Returns: Minimum value of @range.
 */
gint
gupnp_dlna_int_range_get_min (GUPnPDLNAIntRange *range)
{
        g_return_val_if_fail (range != NULL, 0);

        return range->min;
}

/**
 * gupnp_dlna_int_range_get_max:
 * @range: A range.
 *
 * Returns: Maximum value of @range.
 */
gint
gupnp_dlna_int_range_get_max (GUPnPDLNAIntRange *range)
{
        g_return_val_if_fail (range != NULL, 0);

        return range->max;
}
