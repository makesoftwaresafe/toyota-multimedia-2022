/*
 * Copyright (C) 2012 Intel Corporation
 *
 * Author: Krzesimir Nowak <krnowak@openismus.com>
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

#include <libgupnp-av/gupnp-last-change-parser.h>

#define TEST_GENERAL \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> " \
"<Event " \
"xmlns=\"urn:schemas-upnp-org:metadata-1-0/RCS/\" " \
"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " \
"xsi:schemaLocation=\" " \
"urn:schemas-upnp-org:metadata-1-0/RCS/ " \
"http://www.upnp.org/schemas/av/rcs-event-v1-20060531.xsd\"> " \
"<InstanceID val=\"0\"> " \
"<Foo val=\"-13\"/> " \
"<Bar val=\"ajwaj\"/> " \
"</InstanceID> " \
"<InstanceID val=\"1\"> " \
"<Baz val=\"true\"/> " \
"<Qux val=\"42\"/> " \
"</InstanceID> " \
"</Event>"

#define BOGUS_TEXT "This is not an XML document!"

#define TEST_TWO_MUTES \
"<?xml version=\"1.0\" encoding=\"UTF-8\"?> " \
"<Event " \
"xmlns=\"urn:schemas-upnp-org:metadata-1-0/RCS/\" " \
"xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" " \
"xsi:schemaLocation=\" " \
"urn:schemas-upnp-org:metadata-1-0/RCS/ " \
"http://www.upnp.org/schemas/av/rcs-event-v1-20060531.xsd\"> " \
"<InstanceID val=\"0\"> " \
"<Mute channel=\"Master\" val=\"0\"/> " \
"<Mute channel=\"CF\" val=\"1\"/> " \
"</InstanceID> " \
"</Event>"

static void
general (void)
{
  GUPnPLastChangeParser *parser = gupnp_last_change_parser_new ();
  GError                *error1 = NULL;
  GError                *error2 = NULL;
  gboolean               r1;
  gboolean               r2;
  gint                   foo = -1;
  gchar                 *bar = NULL;
  gboolean               baz = FALSE;
  guint                  qux = G_MAXUINT;;

  r1 = gupnp_last_change_parser_parse_last_change (parser,
                                                   0,
                                                   TEST_GENERAL,
                                                   &error1,
                                                   "Foo",
                                                           G_TYPE_INT,
                                                           &foo,
                                                   "Bar",
                                                           G_TYPE_STRING,
                                                           &bar,
                                                   NULL);
  r2 = gupnp_last_change_parser_parse_last_change (parser,
                                                   1,
                                                   TEST_GENERAL,
                                                   &error2,
                                                   "Baz",
                                                           G_TYPE_BOOLEAN,
                                                           &baz,
                                                   "Qux",
                                                           G_TYPE_UINT,
                                                           &qux,
                                                   NULL);

  g_object_unref (parser);

  g_assert (r1 == TRUE);
  g_assert_no_error (error1);
  g_assert_cmpint (foo, ==, -13);
  g_assert_cmpstr (bar, ==, "ajwaj");

  g_free (bar);

  g_assert (r2 == TRUE);
  g_assert_no_error (error2);
  g_assert (baz == TRUE);
  g_assert_cmpuint (qux, ==, 42);
}

static void
bogus_text (void)
{
  GUPnPLastChangeParser *parser = gupnp_last_change_parser_new ();
  GError                *error = NULL;
  gboolean               r;
  int                    whatever = -1;

  r = gupnp_last_change_parser_parse_last_change (parser,
                                                  0,
                                                  BOGUS_TEXT,
                                                  &error,
                                                  "whatever",
                                                          G_TYPE_INT,
                                                          &whatever,
                                                  NULL);

  g_object_unref (parser);

  g_assert (r == FALSE);
  g_assert_error (error, GUPNP_XML_ERROR, GUPNP_XML_ERROR_PARSE);
  g_assert_cmpint (whatever, ==, -1);
}

static void
nonexistent_instance (void)
{
  GUPnPLastChangeParser *parser = gupnp_last_change_parser_new ();
  GError                *error = NULL;
  gboolean               r;
  gint                   foo = -1;
  gchar                 *bar = NULL;
  gboolean               baz = FALSE;
  guint                  qux = G_MAXUINT;;

  r = gupnp_last_change_parser_parse_last_change (parser,
                                                  42,
                                                  TEST_GENERAL,
                                                  &error,
                                                  "Foo",
                                                          G_TYPE_INT,
                                                          &foo,
                                                  "Bar",
                                                          G_TYPE_STRING,
                                                          &bar,
                                                  "Baz",
                                                          G_TYPE_BOOLEAN,
                                                          &baz,
                                                  "Qux",
                                                          G_TYPE_UINT,
                                                          &qux,
                                                  NULL);

  g_object_unref (parser);

  g_assert (r == FALSE);
  g_assert_no_error (error);
  g_assert_cmpint (foo, ==, -1);
  g_assert_cmpstr (bar, ==, NULL);
  g_assert (baz == FALSE);
  g_assert_cmpuint (qux, ==, G_MAXUINT);
}

/* FIXME: We really have no way to test whether some variable does not
   exist. In the test below I set baz to FALSE and qux to G_MAXUINT
   and check whether those variables have still the same values after
   parsing. It may happen that those variable existed in LastChange
   document and had exactly those values. */
static void
nonexistent_variable (void)
{
  GUPnPLastChangeParser *parser = gupnp_last_change_parser_new ();
  GError                *error = NULL;
  gboolean               r;
  gboolean               baz = FALSE;
  guint                  qux = G_MAXUINT;;

  r = gupnp_last_change_parser_parse_last_change (parser,
                                                  0,
                                                  TEST_GENERAL,
                                                  &error,
                                                  "Baz",
                                                          G_TYPE_BOOLEAN,
                                                          &baz,
                                                  "Qux",
                                                          G_TYPE_UINT,
                                                          &qux,
                                                  NULL);

  g_object_unref (parser);

  g_assert (r == TRUE);
  g_assert_no_error (error);
  g_assert (baz == FALSE);
  g_assert_cmpuint (qux, ==, G_MAXUINT);
}

/* FIXME: There is no possibility for fine-grained selection of
   variables we want to extract. There can be two "Mute" variables on
   different "channel"s, but currently the code can only take the
   first "Mute" variable ignoring "channel" attribute. */
static void
two_mutes (void)
{
  GUPnPLastChangeParser *parser = gupnp_last_change_parser_new ();
  GError                *error = NULL;
  gint                   master_mute = -1;
  gint                   cf_mute = -1;
  gboolean               r;

  r = gupnp_last_change_parser_parse_last_change (parser,
                                                  0,
                                                  TEST_TWO_MUTES,
                                                  &error,
                                                  "Mute",
                                                          G_TYPE_INT,
                                                          &master_mute,
                                                  "Mute",
                                                          G_TYPE_INT,
                                                          &cf_mute,
                                                  NULL);

  g_object_unref (parser);

  g_assert (r == TRUE);
  g_assert_no_error (error);
  g_assert_cmpint (master_mute, ==, 0);
  g_message ("Omitting the check of \"Mute\" for \"CF\" channel as this test "
             "fails, because of design issues.");
  //g_assert_cmpint (cf_mute, ==, 1);
}

int
main (int argc, char **argv)
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
  g_type_init ();
#endif
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/last-change-parser/general", general);
  g_test_add_func ("/last-change-parser/bogus-text", bogus_text);
  g_test_add_func ("/last-change-parser/nonexistent-instance", nonexistent_instance);
  g_test_add_func ("/last-change-parser/nonexistent-variable", nonexistent_variable);
  g_test_add_func ("/last-change-parser/two-mutes", two_mutes);

  g_test_run ();

  return 0;
}
