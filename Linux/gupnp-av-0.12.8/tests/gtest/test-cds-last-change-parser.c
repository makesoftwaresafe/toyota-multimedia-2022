/*
 * Copyright (C) 2012 Intel Corporation
 *
 * Author: Jens Georg <jensg@openismus.com>
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

#include <stdio.h>
#include <string.h>

#include <libgupnp-av/gupnp-cds-last-change-parser.h>

typedef struct {
        GUPnPCDSLastChangeEvent event;
        const char *object_id;
        const char *parent_id;
        const char *class;
        guint32 update_id;
        gboolean is_st_update;
} TestReferenceEntry;

typedef struct {
    const char *file_name;
    guint entry_count;
    TestReferenceEntry data[100];
} TestReference;

TestReference test_reference_data[] = {
    {
            "00-cds-last-change.xml",
            0,
            {}
    },
    {
            "01-cds-last-change.xml",
            8,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "s002",
                            "s001",
                            "object.container.album",
                            213,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "s001",
                            NULL,
                            NULL,
                            214,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "s003",
                            "s001",
                            "object.item.audioItem",
                            215,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "s004",
                            "s002",
                            "object.item.audioItem",
                            216,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED,
                            "s003",
                            NULL,
                            NULL,
                            217,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "s001",
                            NULL,
                            NULL,
                            218,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "s004",
                            NULL,
                            NULL,
                            219,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_ST_DONE,
                            "s001",
                            NULL,
                            NULL,
                            219,
                            FALSE
                    }
            }
    },
    {
            "02-cds-last-change.xml",
            2,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Album001",
                            "0",
                            "object.container.album.musicAlbum",
                            101,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "0",
                            NULL,
                            NULL,
                            102,
                            FALSE
                    }
            }
    },
    {
            "03-cds-last-change.xml",
            4,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Album001",
                            "0",
                            "object.container.album.musicAlbum",
                            101,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "0",
                            NULL,
                            NULL,
                            102,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song001",
                            "Album001",
                            "object.item.audioItem",
                            103,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            104,
                            FALSE
                    }
            }
    },
    {
            "04-cds-last-change.xml",
            8,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Album001",
                            "0",
                            "object.container.album.musicAlbum",
                            101,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "0",
                            NULL,
                            NULL,
                            102,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song001",
                            "Album001",
                            "object.item.audioItem",
                            103,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            104,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song002",
                            "Album001",
                            "object.item.audioItem",
                            105,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            106,
                            FALSE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED,
                            "Song001",
                            NULL,
                            NULL,
                            107,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            108,
                            FALSE
                    }
            }
    },
    {
            "05-cds-last-change.xml",
            8,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Album001",
                            "0",
                            "object.container.album.musicAlbum",
                            101,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "0",
                            NULL,
                            NULL,
                            102,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song001",
                            "Album001",
                            "object.item.audioItem",
                            103,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            104,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song002",
                            "Album001",
                            "object.item.audioItem",
                            105,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            106,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED,
                            "Song001",
                            NULL,
                            NULL,
                            107,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            108,
                            FALSE
                    }
            }
    },
    {
            "06-cds-last-change.xml",
            2,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song003",
                            "Album001",
                            "object.item.audioItem",
                            109,
                            FALSE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            110,
                            FALSE
                    }
            }
    },
    {
            "07-cds-last-change.xml",
            6,
            {
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_ADDED,
                            "Song003",
                            "Album001",
                            "object.item.audioItem",
                            234,
                            TRUE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            235,
                            TRUE
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Song001",
                            NULL,
                            NULL,
                            236,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_REMOVED,
                            "Song002",
                            NULL,
                            NULL,
                            237,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_OBJECT_MODIFIED,
                            "Album001",
                            NULL,
                            NULL,
                            238,
                            TRUE,
                    },
                    {
                            GUPNP_CDS_LAST_CHANGE_EVENT_ST_DONE,
                            "Album001",
                            NULL,
                            NULL,
                            238,
                            FALSE,
                    }
            }
    }
};

static void
compare_entries (const char *file, GList *entries)
{
        guint reference_size, i, j;
        GList *it = entries;

        reference_size = G_N_ELEMENTS (test_reference_data);

        /* Find reference entries */
        for (i = 0; i < reference_size; i++) {
                if (strcmp (test_reference_data[i].file_name, file) != 0)
                        continue;
                g_assert_cmpint (g_list_length (entries),
                                 ==,
                                 test_reference_data[i].entry_count);
                for (j = 0;
                     j < test_reference_data[i].entry_count;
                     j++, it = it->next) {
                        TestReferenceEntry* entry;
                        entry = &test_reference_data[i].data[j];
                        g_assert_cmpstr (entry->class, ==,
                                         gupnp_cds_last_change_entry_get_class (it->data));
                        g_assert_cmpstr (entry->parent_id, ==,
                                         gupnp_cds_last_change_entry_get_parent_id (it->data));
                        g_assert_cmpuint (entry->update_id, ==,
                                          gupnp_cds_last_change_entry_get_update_id (it->data));
                        g_assert_cmpstr (entry->object_id, ==,
                                         gupnp_cds_last_change_entry_get_object_id (it->data));
                        g_assert_cmpint (entry->event, ==,
                                         gupnp_cds_last_change_entry_get_event (it->data));
                        g_assert_cmpint (entry->is_st_update, ==,
                                         gupnp_cds_last_change_entry_is_subtree_update (it->data));
                }

                return;
        }

        g_message ("No reference entry found for file %s", file);
        g_assert_not_reached ();
}

static void
test_parse_spec_samples (void)
{
        GDir *dir;
        GError *error = NULL;
        const char *file;
        char *data_path;
        GUPnPCDSLastChangeParser *parser;

        parser = gupnp_cds_last_change_parser_new ();
        data_path = g_build_filename (DATA_PATH,
                                      "data",
                                      "cds-last-change",
                                      NULL);

        dir = g_dir_open (data_path, 0, &error);
        g_assert_no_error (error);
        while ((file = g_dir_read_name (dir)) != NULL) {
                char *contents;
                gsize length;
                char *file_path;
                GList *entries;

                if (g_str_has_prefix (file, ".") ||
                    g_str_has_suffix (file, "~"))
                        continue;

                file_path = g_build_filename (data_path, file, NULL);

                g_file_get_contents (file_path, &contents, &length, &error);
                g_assert_no_error (error);
                g_free (file_path);

                entries = gupnp_cds_last_change_parser_parse (parser,
                                                              contents,
                                                              &error);
                g_assert_no_error (error);
                compare_entries (file, entries);
                g_list_free_full (entries,
                                  (GDestroyNotify) gupnp_cds_last_change_entry_unref);
                g_free (contents);
        }

        g_object_unref (parser);
        g_dir_close (dir);
        g_free (data_path);
}

int
main (int argc, char *argv[])
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
        g_type_init ();
#endif
        g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/cds-last-change/parse-spec-samples",
                         test_parse_spec_samples);

        return g_test_run ();
}
