/*
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

#include <libgupnp-av/gupnp-didl-lite-object.h>
#include <libgupnp-av/gupnp-didl-lite-writer.h>

static void
namespace_getters (void)
{
  GUPnPDIDLLiteWriter *writer = gupnp_didl_lite_writer_new (NULL);
  GUPnPDIDLLiteObject *object = GUPNP_DIDL_LITE_OBJECT (gupnp_didl_lite_writer_add_item (writer));
  xmlNs *namespace;

  namespace = gupnp_didl_lite_object_get_upnp_namespace (object);
  g_assert (namespace != NULL);
  g_assert_cmpstr ((char *) namespace->prefix, ==, "upnp");

  namespace = gupnp_didl_lite_object_get_dlna_namespace (object);
  g_assert (namespace != NULL);
  g_assert_cmpstr ((char *) namespace->prefix, ==, "dlna");


  namespace = gupnp_didl_lite_object_get_dc_namespace (object);
  g_assert (namespace != NULL);
  g_assert_cmpstr ((char *) namespace->prefix, ==, "dc");

  namespace = gupnp_didl_lite_object_get_pv_namespace (object);
  g_assert (namespace != NULL);
  g_assert_cmpstr ((char *) namespace->prefix, ==, "pv");
}

int
main (int argc, char **argv)
{
#if !GLIB_CHECK_VERSION (2, 35, 0)
  g_type_init ();
#endif
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/didl-lite-object/namespace-getters", namespace_getters);

  g_test_run ();

  return 0;
}
