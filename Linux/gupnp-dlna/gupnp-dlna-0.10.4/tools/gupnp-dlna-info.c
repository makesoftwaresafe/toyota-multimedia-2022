/* GUPnPDLNA
 * gupnp-dlna-info.c
 *
 * Copyright (C) 2010 Nokia Corporation
 * Copyright (C) 2010 Collabora Multimedia
 * Copyright (C) 2012, 2013 Intel Corporation
 *
 * Authors: Parthasarathi Susarla <partha.susarla@collabora.co.uk>
 *          Krzesimir Nowak <krnowak@openismus.com>
 *
 * Based on 'gst-discoverer.c' by
 * Edward Hervey <edward.hervey@collabora.co.uk>
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>

#include <libgupnp-dlna/gupnp-dlna-profile.h>
#include <libgupnp-dlna/gupnp-dlna-profile-guesser.h>


static gboolean async = FALSE;
static gint timeout = 10;
static guint files_to_guess = 0;

typedef struct
{
        GUPnPDLNAProfileGuesser *guesser;
        int argc;
        char **argv;
        GMainLoop *ml;
} PrivStruct;

static void
print_dlna_profile (GUPnPDLNAProfile *profile,
                    const gchar      *uri,
                    GError           *err)
{
        g_print ("\nURI: %s\n", uri);
        if (err) {
                g_print ("Failed to guess: %s\n", err->message);
        } else if (profile == NULL) {
                g_print ("No error reported, but no matching profile found.\n");
        } else {
                g_print ("Profile Name: %s\n", gupnp_dlna_profile_get_name (profile));
                g_print ("Profile MIME: %s\n", gupnp_dlna_profile_get_mime (profile));
        }
        return;
}

static void
guesser_done (GUPnPDLNAProfileGuesser *guesser G_GNUC_UNUSED,
              GUPnPDLNAInformation    *info,
              GUPnPDLNAProfile        *profile,
              GError                  *err,
              GMainLoop               *ml)
{
        const gchar *uri = gupnp_dlna_information_get_uri (info);

        print_dlna_profile (profile, uri, err);
        --files_to_guess;
        if (!files_to_guess)
                g_main_loop_quit (ml);
        return;
}

static gboolean
is_uri (const gchar *filename)
{
        gchar *uri = g_uri_parse_scheme (filename);
        gboolean result = (uri != NULL);

        g_free (uri);

        return result;
}

static void
process_file (GUPnPDLNAProfileGuesser *guesser,
              const gchar             *filename);

static void
process_directory (GUPnPDLNAProfileGuesser *guesser,
                   const gchar             *directory)
{
        GError *err = NULL;
        GDir *dir = g_dir_open (directory, 0, &err);

        if (dir) {
                const gchar *entry;

                while ((entry = g_dir_read_name (dir))) {
                        gchar *entry_path;

                        entry_path = g_build_filename (directory,
                                                       entry,
                                                       NULL);
                        process_file (guesser, entry_path);
                        g_free (entry_path);
                }

                g_dir_close (dir);
        } else {
                g_warning ("Couldn't open directory: %s.\n",
                           err->message);
                g_error_free (err);
                err = NULL;
        }
}

static void
process_file (GUPnPDLNAProfileGuesser *guesser,
              const gchar             *filename)
{
        gchar *uri;

        if(!is_uri (filename)) {
                gchar *abs_path;
                GError *err = NULL;

                if (g_file_test (filename, G_FILE_TEST_IS_DIR)) {
                        process_directory (guesser, filename);
                        return;
                }

                if (!g_path_is_absolute (filename)) {
                        gchar *cur_dir = g_get_current_dir ();

                        abs_path = g_build_filename (cur_dir, filename, NULL);
                        g_free (cur_dir);
                } else {
                        abs_path = g_strdup (filename);
                }

                uri = g_filename_to_uri (abs_path, NULL, &err);
                g_free (abs_path);

                if (err) {
                        g_warning ("Couldn't convert filename (%s) to URI: %s\n",
                                   filename,
                                   err->message);
                        g_error_free (err);
                        err = NULL;
                        return;
                }
        } else {
                uri = g_strdup (filename);
        }

        if (async) {
                GError *err = NULL;

                if (!gupnp_dlna_profile_guesser_guess_profile_async (guesser, uri, timeout, &err)) {
                        const gchar *message;

                        if (err) {
                                message = err->message;
                        } else {
                                message = "Unknown error, probably programming mistake";
                        }
                        g_warning ("Unable to queue file for guessing: %s\n",
                                   message);
                        if (err) {
                                g_error_free (err);
                        }
                } else {
                        ++files_to_guess;
                }
        } else {
                GError *err = NULL;
                GUPnPDLNAProfile *profile = gupnp_dlna_profile_guesser_guess_profile_sync (guesser, uri, timeout, NULL, &err);

                if (err) {
                        g_warning ("Unable to read file: %s\n",
                                   err->message);
                        g_error_free (err);
                        err = NULL;
                } else {
                        print_dlna_profile (profile, uri, err);
                }
        }
        g_free (uri);
}

static gboolean
async_idle_loop (PrivStruct *ps)
{
        gint iter;

        for (iter = 1; iter < ps->argc; iter++)
                process_file (ps->guesser, ps->argv[iter]);

        /* No files added to queue, exit program */
        if (files_to_guess == 0) {
                g_main_loop_quit (ps->ml);
        }

        return FALSE;
}

/* Main */
int
main (int argc,
      char **argv)
{
        GUPnPDLNAProfileGuesser *guesser;
        gboolean relaxed_mode = FALSE;
        gboolean extended_mode = FALSE;
        GError *err = NULL;

        GOptionEntry options[] = {
                {"timeout", 't', 0, G_OPTION_ARG_INT, &timeout,
                 "Specify timeout (in seconds, defaults to 10)", "T"},
                {"async", 'a', 0, G_OPTION_ARG_NONE, &async,
                 "Run asynchronously", NULL},
                {"relaxed mode", 'r', 0, G_OPTION_ARG_NONE, &relaxed_mode,
                 "Enable Relaxed mode", NULL},
                {"extended mode", 'e', 0, G_OPTION_ARG_NONE, &extended_mode,
                 "Enable extended mode", NULL},
                {NULL}
        };

        GOptionContext *ctx;

#if !GLIB_CHECK_VERSION(2,35,0)
        g_type_init ();
#endif

        setlocale (LC_ALL, "");

        ctx = g_option_context_new (" - program to extract DLNA and related metadata");
        g_option_context_add_main_entries (ctx, options, NULL);

        if (!g_option_context_parse (ctx, &argc, &argv, &err)) {

                g_print ("Error initializing: %s\n", err->message);
                g_error_free (err);
                exit (1);
        }

        g_option_context_free (ctx);

        if (argc < 2) {
                g_print ("usage:%s <files>\n", argv[0]);
                return -1;
        }

        /* Option was passed in seconds, but guesser takes
           miliseconds. */
        timeout *= 1000;

        guesser = gupnp_dlna_profile_guesser_new (relaxed_mode,
                                                  extended_mode);
        if (guesser == NULL) {
                g_print ("Failed to create meta-data guesser\n");
                exit (1);
        }

        if (async == FALSE) {
                gint iter;

                for (iter = 1; iter < argc; ++iter)
                        process_file (guesser, argv[iter]);
        } else {
                PrivStruct *ps = g_slice_new0 (PrivStruct);
                GMainLoop *ml = g_main_loop_new (NULL, FALSE);

                ps->guesser = guesser;
                ps->argc = argc;
                ps->argv = argv;
                ps->ml = ml;

                g_idle_add ((GSourceFunc) async_idle_loop, ps);

                g_signal_connect (guesser, "done",
                                  G_CALLBACK (guesser_done), ml);

                g_main_loop_run (ml);
                g_main_loop_unref (ml);
                g_slice_free (PrivStruct, ps);
        }
        g_object_unref (guesser);
        gupnp_dlna_profile_guesser_cleanup ();
        return 0;
}
