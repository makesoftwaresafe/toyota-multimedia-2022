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
#include <glib-object.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <node-startup-controller/glib-extensions.h>
#include <node-startup-controller/node-startup-controller-dbus.h>
#include <node-startup-controller/node-startup-controller-service.h>



/**
 * SECTION: node-startup-controller-service
 * @title: NodeStartupControllerService
 * @short_description: Implements the <link linkend="gdbus-org.genivi.NodeStartupController1.NodeStartupController">public D-Bus interface</link>
 *                     of the Node Startup Controller.
 * @stability: Internal
 *
 * The #NodeStartupControllerService implements the methods of the
 * org.genivi.NodeStartupController1.NodeStartupController D-Bus interface which is
 * represented by the #NodeStartupControllerSkeleton class. It does this by implementing
 * handlers for the signals:
 *
 *  * "handle-register-with-luc" which represents the %RegisterWithLUC method.
 *
 *  * "handle-begin-lucregistration" which represents the %BeginLUCRegistration method.
 *
 *  * "handle-finish-lucregistration" which represents the %FinishLUCRegistration method.
 *
 * The specification for the D-Bus interface can be found at
 * #gdbus-org.genivi.NodeStartupController1.NodeStartupController.
 *
 * When the #NodeStartupControllerService receives a "handle-begin-luc-registration"
 * signal, it creates an empty #GVariant (of type "a{ias}", a dictionary of LUC types as
 * integers to groups of units as string arrays) which it uses to contain the new
 * candidate for the Last User Context, and allows other methods to be called
 * successfully.
 *
 * When it receives a "handle-register-with-luc" signal and it has already handled
 * "handle-begin-lucregistration", it adds the #GVariant that it received with the signal
 * to the new candidate. If a new LUC type is specified, it will add the whole group,
 * and if new units are added to an existing group, they get added at the end. If a unit
 * exists in both the received #GVariant and in the new candidate then the unit will be
 * added at the end.
 *
 * When it receives a "handle-finish-lucregistration" and it has already handled
 * "handle-begin-lucregistration", it writes the new candidate by calling
 * node_startup_controller_service_write_luc(), then deletes the candidate so that
 * "handle-begin-lucregistration" can be called again.
 */



DLT_IMPORT_CONTEXT (controller_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
};



static void     node_startup_controller_service_finalize                       (GObject                      *object);
static void     node_startup_controller_service_get_property                   (GObject                      *object,
                                                                                guint                         prop_id,
                                                                                GValue                       *value,
                                                                                GParamSpec                   *pspec);
static void     node_startup_controller_service_set_property                   (GObject                      *object,
                                                                                guint                         prop_id,
                                                                                const GValue                 *value,
                                                                                GParamSpec                   *pspec);
static gboolean node_startup_controller_service_handle_begin_luc_registration  (NodeStartupController        *interface,
                                                                                GDBusMethodInvocation        *invocation,
                                                                                NodeStartupControllerService *service);
static gboolean node_startup_controller_service_handle_finish_luc_registration (NodeStartupController        *interface,
                                                                                GDBusMethodInvocation        *invocation,
                                                                                NodeStartupControllerService *service);
static gboolean node_startup_controller_service_handle_register_with_luc       (NodeStartupController        *interface,
                                                                                GDBusMethodInvocation        *invocation,
                                                                                GVariant                     *apps,
                                                                                NodeStartupControllerService *service);



struct _NodeStartupControllerServiceClass
{
  GObjectClass __parent__;
};

struct _NodeStartupControllerService
{
  GObject                __parent__;

  GDBusConnection       *connection;
  NodeStartupController *interface;

  GVariant              *current_user_context;
  gboolean               started_registration;
};



G_DEFINE_TYPE (NodeStartupControllerService,
               node_startup_controller_service,
               G_TYPE_OBJECT);



static void
node_startup_controller_service_class_init (NodeStartupControllerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = node_startup_controller_service_finalize;
  gobject_class->get_property = node_startup_controller_service_get_property;
  gobject_class->set_property = node_startup_controller_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
node_startup_controller_service_init (NodeStartupControllerService *service)
{
  service->interface = node_startup_controller_skeleton_new ();

  /* initially, no registration is assumed to have been started */
  service->started_registration = FALSE;

  /* reset current user context */
  service->current_user_context = NULL;

  /* implement the RegisterWithLUC() handler */
  g_signal_connect (service->interface, "handle-register-with-luc",
                    G_CALLBACK (node_startup_controller_service_handle_register_with_luc),
                    service);

  /* implement the BeginLUCRegistration() handler */
  g_signal_connect (service->interface, "handle-begin-lucregistration",
                    G_CALLBACK (node_startup_controller_service_handle_begin_luc_registration),
                    service);

  /* implement the FinishLUCRegistration() handler */
  g_signal_connect (service->interface, "handle-finish-lucregistration",
                    G_CALLBACK (node_startup_controller_service_handle_finish_luc_registration),
                    service);
}



static void
node_startup_controller_service_finalize (GObject *object)
{
  NodeStartupControllerService *service = NODE_STARTUP_CONTROLLER_SERVICE (object);

  /* release the D-Bus connection object */
  g_object_unref (service->connection);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (service->interface));
  g_object_unref (service->interface);

  /* release the current user context */
  if (service->current_user_context != NULL)
    g_variant_unref (service->current_user_context);

  (*G_OBJECT_CLASS (node_startup_controller_service_parent_class)->finalize) (object);
}



static void
node_startup_controller_service_get_property (GObject    *object,
                                              guint       prop_id,
                                              GValue     *value,
                                              GParamSpec *pspec)
{
  NodeStartupControllerService *service = NODE_STARTUP_CONTROLLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
node_startup_controller_service_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec)
{
  NodeStartupControllerService *service = NODE_STARTUP_CONTROLLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
node_startup_controller_service_handle_begin_luc_registration (NodeStartupController        *interface,
                                                               GDBusMethodInvocation        *invocation,
                                                               NodeStartupControllerService *service)
{
  GVariantBuilder builder;

  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (service), FALSE);

  /* mark the last user context registration as started */
  service->started_registration = TRUE;

  /* initialize the current user context */
  g_variant_builder_init (&builder, G_VARIANT_TYPE ("a{ias}"));
  service->current_user_context = g_variant_builder_end (&builder);

  /* notify the caller that we have handled the method call */
  g_dbus_method_invocation_return_value (invocation, NULL);
  return TRUE;
}



static gboolean
node_startup_controller_service_handle_finish_luc_registration (NodeStartupController        *interface,
                                                                GDBusMethodInvocation        *invocation,
                                                                NodeStartupControllerService *service)
{
  GError *error = NULL;

  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (service), FALSE);

  /* check if last user context registration started */
  if (!service->started_registration)
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to finish the LUC registration:"),
               DLT_STRING ("the registration sequence was not started properly"));

      /* notify the caller that we have handled the method call */
      g_dbus_method_invocation_return_value (invocation, NULL);
      return TRUE;
    }

  /* write the last user context in a file */
  node_startup_controller_service_write_luc (service, &error);
  if (error != NULL)
   {
     DLT_LOG (controller_context, DLT_LOG_ERROR,
              DLT_STRING ("Failed to finish the LUC registration:"),
              DLT_STRING (error->message));
     g_error_free (error);
   }

  /* mark the last user context registration as finished */
  service->started_registration = FALSE;

  /* clear the current user context */
  g_variant_unref (service->current_user_context);
  service->current_user_context = NULL;

  /* notify the caller that we have handled the register request */
  g_dbus_method_invocation_return_value (invocation, NULL);
  return TRUE;
}



static gboolean
node_startup_controller_service_handle_register_with_luc (NodeStartupController        *interface,
                                                          GDBusMethodInvocation        *invocation,
                                                          GVariant                     *apps,
                                                          NodeStartupControllerService *service)
{
  GVariantBuilder dict_builder;
  GHashTableIter  hiter;
  GVariantIter    viter;
  GHashTable     *table;
  GPtrArray      *apps_array;
  GVariant       *current_context;
  GVariant       *current_apps;
  GVariant       *new_apps;
  gpointer        key;
  GList          *lp;
  GList          *luc_types;
  gchar          *app;
  gchar          *debug_text = NULL;
  guint           n;
  gint            luc_type;

  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (service), FALSE);

  /* check if last user context registration started */
  if (!service->started_registration)
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to register apps with the LUC:"),
               DLT_STRING ("the registration sequence was not started properly"));

      /* notify the caller that we have handled the register request */
      g_dbus_method_invocation_return_value (invocation, NULL);
      return TRUE;
    }

  /* create a hash table to merge the current context and the newly registered apps */
  table = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                 NULL, (GDestroyNotify) g_ptr_array_unref);

  /* obtain the current content of the last user context */
  current_context = g_variant_ref (service->current_user_context);

  /* prepare app lists for all LUC types present in the current context */
  g_variant_iter_init (&viter, current_context);
  while (g_variant_iter_loop (&viter, "{ias}", &luc_type, NULL))
    {
      g_hash_table_insert (table, GINT_TO_POINTER (luc_type),
                           g_ptr_array_new_with_free_func (g_free));
    }

  /* add app lists for LUC types that are needed for the newly registered apps */
  g_variant_iter_init (&viter, apps);
  while (g_variant_iter_loop (&viter, "{ias}", &luc_type, NULL))
    {
      g_hash_table_insert (table, GINT_TO_POINTER (luc_type),
                           g_ptr_array_new_with_free_func (g_free));
    }

  /* we now have a hash table that has all LUC types involved in the
   * current context and in the newly registered apps */

  /* fill the app lists for each LUC type involved, make sure that newly registered
   * apps are added at the end so that they are "prioritized" */
  g_hash_table_iter_init (&hiter, table);
  while (g_hash_table_iter_next (&hiter, (gpointer) &key, (gpointer) &apps_array))
    {
      /* get apps currently registered for the LUC type */
      current_apps = g_variant_lookup_value_with_int_key (current_context,
                                                          GPOINTER_TO_INT (key),
                                                          G_VARIANT_TYPE_STRING_ARRAY);

      /* get apps to be registered for the LUC type now */
      new_apps = g_variant_lookup_value_with_int_key (apps,
                                                      GPOINTER_TO_INT (key),
                                                      G_VARIANT_TYPE_STRING_ARRAY);

      /* add all currently registered apps unless they are to be registered now.
       * this is because we want apps to be registered now to be moved to the end
       * of the lists */
      for (n = 0; current_apps != NULL && n < g_variant_n_children (current_apps); n++)
        {
          g_variant_get_child (current_apps, n, "&s", &app);
          if (!g_variant_string_array_has_string (new_apps, app))
            g_ptr_array_add (apps_array, g_strdup (app));
        }

      /* add all newly registered apps at the end now */
      for (n = 0; new_apps != NULL && n < g_variant_n_children (new_apps); n++)
        {
          g_variant_get_child (new_apps, n, "&s", &app);
          g_ptr_array_add (apps_array, g_strdup (app));
        }

      /* release app lists for this LUC type */
      if (current_apps != NULL)
        g_variant_unref (current_apps);
      if (new_apps != NULL)
        g_variant_unref (new_apps);
    }

  /* construct a new dictionary variant for the new LUC */
  g_variant_builder_init (&dict_builder, G_VARIANT_TYPE ("a{ias}"));

  /* copy LUC types and corresponding apps over to the new context.
   * make sure the order in which we add LUC types to the context
   * dict is always the same. this is helpful for testing */
  luc_types = g_hash_table_get_keys (table);
  luc_types = g_list_sort (luc_types, (GCompareFunc) g_int_pointer_compare);
  for (lp = luc_types; lp != NULL; lp = lp->next)
    {
      /* get the apps list registered for this LUC type */
      apps_array = g_hash_table_lookup (table, lp->data);

      /* NULL-terminate the pointer so that we can treat it as a gchar ** */
      g_ptr_array_add (apps_array, NULL);

      /* add the LUC type and its apps to the new context */
      g_variant_builder_add (&dict_builder, "{i^as}",
                             GPOINTER_TO_INT (lp->data), apps_array->pdata);
    }

  /* free the LUC types and our LUC type to apps mapping */
  g_list_free (luc_types);
  g_hash_table_unref (table);

  /* free the last user context */
  g_variant_unref (service->current_user_context);

  /* apply the new last user context */
  service->current_user_context = g_variant_builder_end (&dict_builder);

  /* log the new last user context */
  debug_text = g_variant_print (service->current_user_context, TRUE);
  DLT_LOG (controller_context, DLT_LOG_INFO,
           DLT_STRING ("Updated LUC to:"), DLT_STRING (debug_text));
  g_free (debug_text);

  /* release the current context */
  g_variant_unref (current_context);

  /* notify the caller that we have handled the register request */
  g_dbus_method_invocation_return_value (invocation, NULL);

  return TRUE;
}



/**
 * node_startup_controller_service_new:
 * @connection: A connection to the system bus.
 * 
 * Creates a new #NodeStartupControllerService object.
 * 
 * Returns: A new #NodeStartupControllerService.
 */
NodeStartupControllerService *
node_startup_controller_service_new (GDBusConnection *connection)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);

  return g_object_new (TYPE_NODE_STARTUP_CONTROLLER_SERVICE,
                       "connection", connection,
                       NULL);
}



/**
 * node_startup_controller_service_start_up:
 * @service: A #NodeStartupControllerService.
 * @error: The location of an error (if raised), %NULL otherwise.
 * 
 * Exports the #NodeStartupController interface skeleton on the system bus, which makes
 * the %BeginLUCRegistration, %FinishLUCRegistration and %RegisterWithLUC methods
 * available.
 * 
 * Returns: %TRUE if it successfully started up and exported its interface skeleton,
 * %FALSE otherwise.
 */
gboolean
node_startup_controller_service_start_up (NodeStartupControllerService *service,
                                          GError                      **error)
{
  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.NodeStartupController1.NodeStartupController service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/NodeStartupController1/NodeStartupController",
                                           error);
}



/**
 * node_startup_controller_service_read_luc:
 * @service: A #NodeStartupControllerService.
 * @error: The location of the error raised, or %NULL.
 * 
 * Reads the Last User Context from the file whose location is defined by the environment
 * variable %LUC_PATH, or if not, the build-time definition of %LUC_PATH.
 * 
 * Returns: A #GVariant of the form "a{ias}" which contains the Last User Context if
 * successfully read. In case of failure, %NULL is returned and the error is set.
 */
GVariant *
node_startup_controller_service_read_luc (NodeStartupControllerService *service,
                                          GError                      **error)
{
  const gchar *luc_path;
  GVariant    *context;
  GFile       *luc_file;
  char        *data;
  gsize        data_len;

  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (service), NULL);
  g_return_val_if_fail ((error == NULL || *error == NULL), NULL);

  /* check which configuration file to use; the LUC_PATH environment variable
   * has priority over the build-time LUC_PATH definition */
  luc_path = g_getenv ("LUC_PATH");
  if (luc_path == NULL)
    luc_path = LUC_PATH;

  /* initialize the GFile */
  luc_file = g_file_new_for_path (luc_path);

  /* read the contents of the file */
  if (!g_file_load_contents (luc_file, NULL, &data, &data_len, NULL, error))
    {
      g_object_unref (luc_file);
      return NULL;
    }

  /* store the contents of the file in a GVariant */
  context = g_variant_new_from_data (G_VARIANT_TYPE ("a{ias}"), data, data_len,
                                     TRUE, g_free, data);

  g_object_unref (luc_file);

  return context;
}



/**
 * node_startup_controller_service_write_luc:
 * @service: A #NodeStartupControllerService.
 * @error: The location of the error raised, or %NULL.
 * 
 * Atomically writes the Last User Context stored in @service to the file whose location
 * is defined by the environment variable %LUC_PATH, or if not, the build-time definition
 * of %LUC_PATH.
 */
void
node_startup_controller_service_write_luc (NodeStartupControllerService *service,
                                           GError                      **error)
{
  const gchar *luc_path;
  GError      *err = NULL;
  GFile       *luc_file;
  GFile       *luc_dir;

  g_return_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (service));
  g_return_if_fail (error == NULL || *error == NULL);

  /* check which configuration file to use; the LUC_PATH environment variable
   * has priority over the build-time LUC_PATH definition */
  luc_path = g_getenv ("LUC_PATH");
  if (luc_path == NULL)
    luc_path = LUC_PATH;

  /* initialize the GFiles */
  luc_file = g_file_new_for_path (luc_path);
  luc_dir = g_file_get_parent (luc_file);

  /* make sure the last user context's directory exists */
  if (!g_file_make_directory_with_parents (luc_dir, NULL, &err))
    {
      if (err->domain == G_IO_ERROR && err->code == G_IO_ERROR_EXISTS)
        {
          /* clear the error for reuse */
          g_clear_error (&err);
        }
      else
        {
          /* let the caller know there was a problem */
          g_propagate_error (error, err);
          g_object_unref (luc_file);
          g_object_unref (luc_dir);
          return;
        }
    }

  /* replace the contents with that of the file. g_file_replace_contents
   * guarantees atomic overwriting and can make backups */
  g_file_replace_contents (luc_file, g_variant_get_data (service->current_user_context),
                           g_variant_get_size (service->current_user_context), NULL,
                           TRUE, G_FILE_CREATE_NONE, NULL, NULL, error);

  /* release the GFiles */
  g_object_unref (luc_file);
  g_object_unref (luc_dir);
}
