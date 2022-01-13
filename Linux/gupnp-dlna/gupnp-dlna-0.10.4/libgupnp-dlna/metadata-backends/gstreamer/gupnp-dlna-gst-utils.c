/*
 * Copyright (C) 2013 Intel Corporation.
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

#include "gupnp-dlna-gst-utils.h"
#include "gupnp-dlna-gst-information.h"

/**
 * gupnp_dlna_gst_utils_information_from_discoverer_info:
 * @info: A #GstDiscovererInfo object.
 *
 * This utility function creates a #GUPnPDLNAInformation from
 * #GstDiscovererInfo. Its purpose is mainly to pass its result to
 * gupnp_dlna_profile_guesser_guess_profile_from_info() to avoid
 * rediscovering of a URI. Created #GUPnPDLNAInformation takes a
 * reference on @info.
 *
 * Returns: (transfer full): A #GUPnPDLNAInformation. Unref it when
 * done with it.
 */
GUPnPDLNAInformation *
gupnp_dlna_gst_utils_information_from_discoverer_info (GstDiscovererInfo *info)
{
        g_return_val_if_fail (GST_IS_DISCOVERER_INFO (info), NULL);

        return GUPNP_DLNA_INFORMATION
                            (gupnp_dlna_gst_information_new_from_discoverer_info
                                        (gst_discoverer_info_get_uri (info),
                                         info));
}
