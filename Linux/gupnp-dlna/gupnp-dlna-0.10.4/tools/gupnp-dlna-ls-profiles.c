/* GUPnPDLNA
 * gupnp-dlna-ls-profiles.c
 *
 * Copyright (C) 2010 Nokia Corporation
 * Copyright (C) 2010 Collabora Multimedia
 * Copyright (C) 2012 Intel Corporation
 *
 * Authors: Parthasarathi Susarla <partha.susarla@collabora.co.uk>
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
#endif

#include <locale.h>
#include <stdlib.h>

#include <glib.h>
#include <glib-object.h>

#include <libgupnp-dlna/gupnp-dlna-profile.h>
#include <libgupnp-dlna/gupnp-dlna-profile-guesser.h>

static gboolean relaxed = FALSE;

static void
print_profile (GUPnPDLNAProfile *profile)
{
        g_print ("%s %-30s%-35s\n",
                 gupnp_dlna_profile_get_extended (profile) ? "*" : " ",
                 gupnp_dlna_profile_get_name (profile),
                 gupnp_dlna_profile_get_mime (profile));
}

int
main (int argc, char **argv)
{
        GError *err = NULL;
        const GList *profiles = NULL;
        GUPnPDLNAProfileGuesser *guesser;

        GOptionEntry options[] = {
                {"relaxed", 'r', 0, G_OPTION_ARG_NONE, &relaxed,
                 "Read profiles in relaxed mode", NULL},
                {NULL}
        };

        GOptionContext *ctx;

#if !GLIB_CHECK_VERSION(2,35,0)
        g_type_init ();
#endif

        setlocale (LC_ALL, "");

        ctx = g_option_context_new (" - program to list all the DLNA profiles supported by gupnp-dlna");
        g_option_context_add_main_entries (ctx, options, NULL);
        if (!g_option_context_parse (ctx, &argc, &argv, &err)) {

                g_print ("Error initializing: %s\n", err->message);
                g_error_free (err);
                exit (1);
        }

        g_option_context_free (ctx);

        guesser = gupnp_dlna_profile_guesser_new (relaxed, TRUE);
        profiles = gupnp_dlna_profile_guesser_list_profiles (guesser);

        g_print ("  %-30s%s\n", "Name", "MIME type");
        g_print ("---------------------------------------------------"
                         "---------------------\n");
g_list_foreach ((GList *) profiles, (GFunc) print_profile, NULL);
        g_print ("\nProfiles with a '*' against their name are extended "
                 "(non-standard) profiles.\n\n");

        g_object_unref (guesser);
        gupnp_dlna_profile_guesser_cleanup ();

        return 0;
}
