/* vi:set et ai sw=2 sts=2 ts=2: */
/* -
 * Copyright (c) 2012 GENIVI.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>

#include <node-startup-controller/glib-extensions.h>



/**
 * SECTION: glib-extensions
 * @title: GLib extensions
 * @short_description: Auxiliary functions which are not provided by GLib.
 * @stability: Internal
 *
 * Auxiliary functions related to the GLib library but not provided by GLib.
 */



/**
 * g_variant_lookup_value_with_int_key:
 * @dictionary: A dictionary #GVariant.
 * @key: The key to lookup in the @dictionary.
 * @expected_type: A #GVariantType to check that the value corresponding to the @key has
 * the correct type. 
 *
 * Looks up a value in a dictionary #GVariant using an integer @key.
 * This function only works with dictionaries of the type a{ias}.
 * In the event that dictionary has the type a{ias}, the @key is found and the value
 * belonging to this @key has the correct #GVariantType, then the value is returned.
 * Otherwise the returned value is %NULL.
 *
 * Returns: The value associated with the the dictionary key, or %NULL.
 */
GVariant *
g_variant_lookup_value_with_int_key (GVariant           *dictionary,
                                     const gint          key,
                                     const GVariantType *expected_type)
{
  GVariantIter iter;
  GVariant    *value = NULL;
  gint32       current_key;


  g_return_val_if_fail (dictionary != NULL, NULL);
  g_return_val_if_fail (expected_type != NULL, NULL);
  
  if (!g_variant_is_of_type (dictionary, G_VARIANT_TYPE ("a{ias}")))
    return NULL;

  g_variant_iter_init (&iter, dictionary);
  while (g_variant_iter_loop (&iter, "{i@as}", &current_key, &value))
    {
      if (current_key == key)
        {
          if (value != NULL && g_variant_is_of_type (value, expected_type))
            return value;
          else
            return NULL;
        }
    }

  return NULL;
}



/**
 * g_variant_string_array_has_string:
 * @array: A #GVariant holding an array of strings.
 * @str: A string to check for in @array.
 *
 * Checks if @array includes the string @str.
 *
 * Returns: TRUE if @array includes the string @str, otherwise FALSE.
 */
gboolean
g_variant_string_array_has_string (GVariant    *array,
                                   const gchar *str)
{
  gboolean found = FALSE;
  gchar   *current_str;
  guint    n;

  for (n = 0; array != NULL && !found && n < g_variant_n_children (array); n++)
    {
      g_variant_get_child (array, n, "&s", &current_str);
      if (g_strcmp0 (str, current_str) == 0)
        found = TRUE;
    }

  return found;
}



/**
 * g_int_pointer_compare:
 * @a: A #gconstpointer.
 * @b: Another #gconstpointer.
 *
 * Compares @a and @b, assuming that they are integers represented as pointers. Returns
 * a negative value if @a is less than @b, zero if they are equal and a positive value if
 * @b is greater than @a.
 *
 * Returns: A negative value if @a is lesser, a positive value if @a is greater, and zero
 * if @a is equal to @b.
 */
gint
g_int_pointer_compare (gconstpointer a, gconstpointer b)
{
  return GPOINTER_TO_INT (a) - GPOINTER_TO_INT (b);
}
