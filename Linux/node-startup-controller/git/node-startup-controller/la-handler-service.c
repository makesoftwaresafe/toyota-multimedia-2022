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

#include <glib-object.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <common/la-handler-dbus.h>
#include <common/nsm-consumer-dbus.h>
#include <common/nsm-enum-types.h>
#include <common/shutdown-client.h>
#include <common/shutdown-consumer-dbus.h>

#include <node-startup-controller/job-manager.h>
#include <node-startup-controller/la-handler-service.h>



/**
 * SECTION: la-handler-service
 * @title: LAHandlerService
 * @short_description: Handles registration of legacy apps with the Node State Manager.
 * @stability: Internal
 * 
 * The #LAHandlerService contains two hash tables to map the shutdown clients it
 * registers with the Node State Manager and vice versa.
 *
 * The #LAHandlerService class provides an internal D-Bus interface which contains a
 * %Register method for the #legacy-app-handler helper binary to communicate with.
 *
 * When it receives a %Register method call (which specifies a unit name, shutdown mode
 * and timeout), it handles the "handle-register" signal by doing the following:
 *
 * 1. Looks for a pre-existing #ShutdownClient by its unit name. If it already exists,
 *    it adds the shutdown mode to whatever shutdown modes are already registered and
 *    sets the timeout to the new value.
 *
 * 2. Create a #ShutdownClient with unit name, bus name, object path, shutdown mode,
 *    timeout and a #ShutdownConsumerSkeleton, where the bus name is the Node
 *    Startup Controller's bus name, and the object path is unique and decided by the
 *    #LAHandlerService.
 *
 * 3. Register the "handle-lifecycle-request" signal handler to the
 *    #ShutdownConsumerSkeleton's %LifecycleRequest method, taking the #ShutdownClient as
 *    userdata.
 *
 * 4. Export the shutdown client's #ShutdownConsumerSkeleton using the client's object
 *    path.
 *
 * 5. Register the #ShutdownClient with the Node State Manager.
 *
 * When it receives a %LifecycleRequest method call, it does the following:
 *
 * 1. Looks up the unit name of the #ShutdownClient passed as userdata. If it is not
 *    found then it returns an error to the Node State Manager.
 *
 * 2. Tells the Job Manager to stop the #ShutdownClient's systemd unit.
 *
 * 3. Returns the #NSMErrorStatus NSM_ERROR_STATUS_PENDING to the Node State Manger,
 *    which tells The Node State Manager to wait %timeout seconds for a replying method
 *    call to the Node State Manager with the %LifecycleRequestComplete method.
 *
 * 4. After the #JobManager has stopped the unit, it checks if the #JobManager failed to
 *    stop the unit and calls the Node State Manager with %LifecycleRequestComplete to
 *    inform it about the success or failure of stopping the unit.
 */



DLT_IMPORT_CONTEXT (la_handler_context);



/* property identifiers */
enum
{
  PROP_0,
  PROP_CONNECTION,
  PROP_JOB_MANAGER,
};



typedef struct _LAHandlerServiceData LAHandlerServiceData;



static void                  la_handler_service_constructed                              (GObject               *object);
static void                  la_handler_service_finalize                                 (GObject               *object);
static void                  la_handler_service_get_property                             (GObject               *object,
                                                                                          guint                  prop_id,
                                                                                          GValue                *value,
                                                                                          GParamSpec            *pspec);
static void                  la_handler_service_set_property                             (GObject               *object,
                                                                                          guint                  prop_id,
                                                                                          const GValue          *value,
                                                                                          GParamSpec            *pspec);
static gboolean              la_handler_service_handle_register                          (LAHandler             *interface,
                                                                                          GDBusMethodInvocation *invocation,
                                                                                          const gchar           *unit,
                                                                                          NSMShutdownType        mode,
                                                                                          guint                  timeout,
                                                                                          LAHandlerService      *service);
static void                  la_handler_service_handle_register_finish                   (GObject               *object,
                                                                                          GAsyncResult          *res,
                                                                                          gpointer               user_data);
static gboolean              la_handler_service_handle_consumer_lifecycle_request        (ShutdownConsumer      *consumer,
                                                                                          GDBusMethodInvocation *invocation,
                                                                                          guint                  request,
                                                                                          guint                  request_id,
                                                                                          ShutdownClient        *client);
static void                  la_handler_service_handle_consumer_lifecycle_request_finish (JobManager            *manager,
                                                                                          const gchar           *unit,
                                                                                          const gchar           *result,
                                                                                          GError                *error,
                                                                                          gpointer               user_data);
static LAHandlerServiceData *la_handler_service_data_new                                 (LAHandlerService      *service,
                                                                                          GDBusMethodInvocation *invocation,
                                                                                          guint                  request_id);
static void                  la_handler_service_data_unref                               (LAHandlerServiceData  *data);



struct _LAHandlerServiceClass
{
  GObjectClass __parent__;
};

struct _LAHandlerService
{
  GObject          __parent__;

  GDBusConnection *connection;
  LAHandler       *interface;
  JobManager      *job_manager;

  /* Associations of shutdown clients and their units */
  GHashTable      *units_to_clients;
  GHashTable      *clients_to_units;

  const gchar     *prefix;
  guint            index;

  /* connection to the NSM consumer interface */
  NSMConsumer     *nsm_consumer;
};

struct _LAHandlerServiceData
{
  GDBusMethodInvocation *invocation;
  LAHandlerService      *service;
  guint                  request_id;
};



G_DEFINE_TYPE (LAHandlerService, la_handler_service, G_TYPE_OBJECT);



static void
la_handler_service_class_init (LAHandlerServiceClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = la_handler_service_constructed;
  gobject_class->finalize = la_handler_service_finalize;
  gobject_class->get_property = la_handler_service_get_property;
  gobject_class->set_property = la_handler_service_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_CONNECTION,
                                   g_param_spec_object ("connection",
                                                        "connection",
                                                        "connection",
                                                        G_TYPE_DBUS_CONNECTION,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
  g_object_class_install_property (gobject_class,
                                   PROP_JOB_MANAGER,
                                   g_param_spec_object ("job-manager",
                                                        "Job Manager",
                                                        "The internal handler of Start()"
                                                        " and Stop() jobs",
                                                        TYPE_JOB_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
la_handler_service_constructed (GObject *object)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);
  GError           *error = NULL;

  /* connect to the node state manager */
  service->nsm_consumer =
    nsm_consumer_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM, G_DBUS_PROXY_FLAGS_NONE,
                                         "org.genivi.NodeStateManager",
                                         "/org/genivi/NodeStateManager/Consumer",
                                          NULL, &error);
  if (error != NULL)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to connect to the NSM consumer:"),
               DLT_STRING (error->message));
      g_error_free (error);
    }
}



static void
la_handler_service_init (LAHandlerService *service)
{
  service->interface = la_handler_skeleton_new ();

  /* the number that follows the prefix in the shutdown client's object path,
   * making every shutdown client unique */
  service->index = 1;

  /* the string that precedes the index in the shutdown client's object path */
  service->prefix = "/org/genivi/NodeStartupController1/ShutdownConsumer";

  /* initialize the association of shutdown client to units */
  service->units_to_clients = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                     (GDestroyNotify) g_free,
                                                     (GDestroyNotify) g_object_unref);
  service->clients_to_units = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                     (GDestroyNotify) g_object_unref,
                                                     (GDestroyNotify) g_free);

  /* implement the Register() handler */
  g_signal_connect (service->interface, "handle-register",
                    G_CALLBACK (la_handler_service_handle_register),
                    service);
}



static void
la_handler_service_finalize (GObject *object)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  /* release the interface skeleton */
  g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (service->interface));

  /* release the NSM consumer service object, if there is one */
  if (service->nsm_consumer != NULL)
    g_object_unref (service->nsm_consumer);

  /* release the interface skeleton */
  g_signal_handlers_disconnect_matched (service->interface,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, service);
  g_object_unref (service->interface);

  /* release the job manager skeleton */
  g_object_unref (service->job_manager);

  /* release the D-Bus connection object */
  if (service->connection != NULL)
    g_object_unref (service->connection);

  /* release the shutdown clients */
  g_hash_table_unref (service->units_to_clients);
  g_hash_table_unref (service->clients_to_units);

  (*G_OBJECT_CLASS (la_handler_service_parent_class)->finalize) (object);
}



static void
la_handler_service_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      g_value_set_object (value, service->connection);
      break;
    case PROP_JOB_MANAGER:
      g_value_set_object (value, service->job_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
la_handler_service_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  LAHandlerService *service = LA_HANDLER_SERVICE (object);

  switch (prop_id)
    {
    case PROP_CONNECTION:
      service->connection = g_value_dup_object (value);
      break;
    case PROP_JOB_MANAGER:
      service->job_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
la_handler_service_handle_register (LAHandler             *interface,
                                    GDBusMethodInvocation *invocation,
                                    const gchar           *unit,
                                    NSMShutdownType        shutdown_mode,
                                    guint                  timeout,
                                    LAHandlerService      *service)
{
  ShutdownConsumer *consumer;
  ShutdownClient   *client;
  GError           *error = NULL;
  const gchar      *existing_bus_name;
  const gchar      *existing_object_path;
  gchar            *bus_name;
  gchar            *object_path;

  g_return_val_if_fail (IS_LA_HANDLER (interface), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (unit != NULL && *unit != '\0', FALSE);
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (service), FALSE);

  if (shutdown_mode != NSM_SHUTDOWN_TYPE_NORMAL
      && shutdown_mode != NSM_SHUTDOWN_TYPE_FAST
      && shutdown_mode != (NSM_SHUTDOWN_TYPE_NORMAL | NSM_SHUTDOWN_TYPE_FAST))
    {
      /* the shutdown mode is invalid */
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to register legacy application: "
                           "invalid shutdown mode"), DLT_INT (shutdown_mode));
      la_handler_complete_register (interface, invocation);
      return TRUE;
    }

  /* find out if we have a shutdown client for this unit already */
  client = g_hash_table_lookup (service->units_to_clients, unit);
  if (client != NULL)
    {
      /* there already is a shutdown client for the unit, so simply
       * re-register its client with the new shutdown mode and timeout */

      /* extract information from the client */
      existing_bus_name = shutdown_client_get_bus_name (client);
      existing_object_path = shutdown_client_get_object_path (client);

      /* temporarily store a reference to the legacy app handler service object
       * in the invocation object */
      g_object_set_data_full (G_OBJECT (invocation), "la-handler-service",
                              g_object_ref (service), (GDestroyNotify) g_object_unref);

      /* re-register the shutdown consumer with the NSM Consumer */
      nsm_consumer_call_register_shutdown_client (service->nsm_consumer,
                                                  existing_bus_name, existing_object_path,
                                                  shutdown_mode, timeout, NULL,
                                                  la_handler_service_handle_register_finish,
                                                  invocation);
    }
  else
    {
      /* create a new shutdown client and consumer for the unit */
      bus_name = "org.genivi.NodeStartupController1";
      object_path = g_strdup_printf ("%s/%u", service->prefix, service->index);
      client = shutdown_client_new (bus_name, object_path, shutdown_mode, timeout);
      consumer = shutdown_consumer_skeleton_new ();
      shutdown_client_set_consumer (client, consumer);

      /* remember the legacy app handler service object in shutdown client */
      g_object_set_data_full (G_OBJECT (client), "la-handler-service",
                              g_object_ref (service), (GDestroyNotify) g_object_unref);

      /* implement the LifecycleRequest method of the shutdown consumer */
      g_signal_connect (consumer, "handle-lifecycle-request",
                        G_CALLBACK (la_handler_service_handle_consumer_lifecycle_request),
                        client);

      /* associate the shutdown client with the unit name */
      g_hash_table_insert (service->units_to_clients, g_strdup (unit),
                           g_object_ref (client));
      g_hash_table_insert (service->clients_to_units, g_object_ref (client),
                           g_strdup (unit));

      /* export the shutdown consumer on the bus */
      g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (consumer),
                                        service->connection, object_path, &error);
      if (error != NULL)
        {
          DLT_LOG (la_handler_context, DLT_LOG_ERROR,
                   DLT_STRING ("Failed to export shutdown consumer on the bus:"),
                   DLT_STRING (error->message));
          g_error_free (error);
        }

      /* temporarily store a reference to the legacy app handler service object
       * in the invocation object */
      g_object_set_data_full (G_OBJECT (invocation), "la-handler-service",
                              g_object_ref (service), (GDestroyNotify) g_object_unref);

      /* register the shutdown consumer with the NSM Consumer */
      nsm_consumer_call_register_shutdown_client (service->nsm_consumer,
                                                  bus_name, object_path,
                                                  shutdown_mode, timeout, NULL,
                                                  la_handler_service_handle_register_finish,
                                                  invocation);

      /* free strings and release the shutdown consumer */
      g_free (object_path);
      g_object_unref (consumer);

      /* increment the counter for our shutdown consumer object paths */
      service->index++;
    }

  return TRUE;
}



static void
la_handler_service_handle_register_finish (GObject      *object,
                                           GAsyncResult *res,
                                           gpointer      user_data)
{
  GDBusMethodInvocation *invocation = G_DBUS_METHOD_INVOCATION (user_data);
  LAHandlerService      *service;
  NSMConsumer           *nsm_consumer = NSM_CONSUMER (object);
  GError                *error = NULL;
  gint                   error_code;

  g_return_if_fail (IS_NSM_CONSUMER (nsm_consumer));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation));

  /* finish registering the shutdown client */
  nsm_consumer_call_register_shutdown_client_finish (nsm_consumer, &error_code, res,
                                                     &error);
  if (error != NULL)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to register a shutdown consumer:"),
               DLT_STRING (error->message));
      g_error_free (error);
    }

  /* retrieve the LAHandlerService from the invocation object */
  service = g_object_get_data (G_OBJECT (invocation), "la-handler-service");

  /* notify the caller that we have handled the registration request */
  la_handler_complete_register (service->interface, invocation);
}



static gboolean
la_handler_service_handle_consumer_lifecycle_request (ShutdownConsumer      *consumer,
                                                      GDBusMethodInvocation *invocation,
                                                      guint                  request,
                                                      guint                  request_id,
                                                      ShutdownClient        *client)
{
  LAHandlerServiceData *data;
  LAHandlerService     *service;
  gchar                *unit_name;

  g_return_val_if_fail (IS_SHUTDOWN_CONSUMER (consumer), FALSE);
  g_return_val_if_fail (G_IS_DBUS_METHOD_INVOCATION (invocation), FALSE);
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), FALSE);

  /* get the service from the shutdown client */
  service = g_object_get_data (G_OBJECT (client), "la-handler-service");

  /* look up the unit name associated with this shutdown client */
  unit_name = g_hash_table_lookup (service->clients_to_units, client);

  if (unit_name != NULL)
    {
      data = la_handler_service_data_new (service, NULL, request_id);

      /* stop this unit now */
      job_manager_stop (service->job_manager, unit_name, NULL,
                        la_handler_service_handle_consumer_lifecycle_request_finish,
                        data);

      /* let the NSM know that we are working on this request */
      shutdown_consumer_complete_lifecycle_request (consumer, invocation,
                                                    NSM_ERROR_STATUS_RESPONSE_PENDING);
    }
  else
    {
      /* NSM asked us to shutdown a shutdown consumer we did not register;
       * make it aware by returning an error */
      shutdown_consumer_complete_lifecycle_request (consumer, invocation,
                                                    NSM_ERROR_STATUS_ERROR);
    }

  return TRUE;
}



static void
la_handler_service_handle_consumer_lifecycle_request_finish (JobManager  *manager,
                                                             const gchar *unit,
                                                             const gchar *result,
                                                             GError      *error,
                                                             gpointer     user_data)
{
  LAHandlerServiceData *data = (LAHandlerServiceData *)user_data;
  GError               *err = NULL;
  gint                  error_status = NSM_ERROR_STATUS_OK;
  gint                  status = NSM_ERROR_STATUS_OK;

  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (unit != NULL && *unit != '\0');
  g_return_if_fail (result != NULL && *result != '\0');
  g_return_if_fail (data != NULL);

  /* log that we are completing a lifecycle request */
  DLT_LOG (la_handler_context, DLT_LOG_INFO,
           DLT_STRING ("Completing a lifecycle request:"),
           DLT_STRING ("request id"), DLT_UINT (data->request_id));

  /* log an error if shutting down the consumer has failed */
  if (error != NULL)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to shut down a shutdown consumer:"),
               DLT_STRING (error->message));

      /* send an error back to the NSM */
      status = NSM_ERROR_STATUS_ERROR;
    }

  /* log an error if systemd failed to stop the consumer */
  if (g_strcmp0 (result, "failed") == 0)
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to shutdown a shutdown consumer"));

      /* send an error back to the NSM */
      status = NSM_ERROR_STATUS_ERROR;
    }

  /* let the NSM know that we have handled the lifecycle request */
  if (!nsm_consumer_call_lifecycle_request_complete_sync (data->service->nsm_consumer,
                                                          data->request_id, status,
                                                          &error_status, NULL, &err))
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to notify NSM about completed lifecycle request:"),
               DLT_STRING ("request id"), DLT_UINT (data->request_id),
               DLT_STRING ("error message"), DLT_STRING (err->message));
      g_error_free (err);
    }
  else if (error_status == NSM_ERROR_STATUS_OK)
    {
      DLT_LOG (la_handler_context, DLT_LOG_INFO,
               DLT_STRING ("Successfully notified NSM about completed "
                           "lifecycle request:"),
               DLT_STRING ("request id"), DLT_UINT (data->request_id));
    }
  else
    {
      DLT_LOG (la_handler_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to notify NSM about completed lifecycle request:"),
               DLT_STRING ("request id"), DLT_UINT (data->request_id),
               DLT_STRING ("error status"), DLT_INT (error_status));
    }

  la_handler_service_data_unref (data);
}



static LAHandlerServiceData *
la_handler_service_data_new (LAHandlerService      *service,
                             GDBusMethodInvocation *invocation,
                             guint                  request_id)
{
  LAHandlerServiceData *data;

  data = g_slice_new0 (LAHandlerServiceData);
  if (service != NULL)
    data->service = g_object_ref (service);
  if (invocation != NULL)
    data->invocation = g_object_ref (invocation);
  data->request_id = request_id;

  return data;
}



static void
la_handler_service_data_unref (LAHandlerServiceData *data)
{
  if (data == NULL)
    return;

  if (data->invocation != NULL)
    g_object_unref (data->invocation);
  if (data->service != NULL)
    g_object_unref (data->service);
  g_slice_free (LAHandlerServiceData, data);
}



/**
 * la_handler_service_new:
 * @connection: A connection to the system bus.
 * @job_manager: A reference to the #JobManager object.
 * 
 * Creates a new #LAHandlerService object.
 * 
 * Returns: A new instance of the #LAHandlerService.
 */
LAHandlerService *
la_handler_service_new (GDBusConnection *connection,
                        JobManager      *job_manager)
{
  g_return_val_if_fail (G_IS_DBUS_CONNECTION (connection), NULL);
  g_return_val_if_fail (IS_JOB_MANAGER (job_manager), NULL);

  return g_object_new (LA_HANDLER_TYPE_SERVICE,
                       "connection", connection,
                       "job-manager", job_manager,
                       NULL);
}



/**
 * la_handler_service_start:
 * @service: A #LAHandlerService.
 * @error:   Return location for error or %NULL.
 * 
 * Makes @service export its #LAHandler interface so that it is available to the
 * #legacy-app-handler helper binary.
 * 
 * Returns: %TRUE if the interface was exported successfully, otherwise %FALSE with @error
 * set.
 */
gboolean
la_handler_service_start (LAHandlerService *service,
                           GError         **error)
{
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (service), FALSE);
  g_return_val_if_fail (error == NULL || *error == NULL, FALSE);

  /* announce the org.genivi.NodeStartupController1.LegacyAppHandler service on the bus */
  return g_dbus_interface_skeleton_export (G_DBUS_INTERFACE_SKELETON (service->interface),
                                           service->connection,
                                           "/org/genivi/NodeStartupController1/LegacyAppHandler",
                                           error);
}



/**
 * la_handler_service_get_nsm_consumer:
 * @service: A #LAHandlerService.
 * 
 * Retrieves the #NSMConsumer stored in @service.
 * 
 * Returns: A proxy of the Node State Manager's #NSMConsumer interface.
 */
NSMConsumer *
la_handler_service_get_nsm_consumer (LAHandlerService *service)
{
  g_return_val_if_fail (LA_HANDLER_IS_SERVICE (service), NULL);

  return service->nsm_consumer;
}



/**
 * la_handler_service_deregister_consumers:
 * @service: A #LAHandlerService.
 * 
 * Unregisters every #ShutdownClient from the Node State Manager.
 * This method is typically used when the #LAHandlerService is about to shut down.
 */
void
la_handler_service_deregister_consumers (LAHandlerService *service)
{
  GHashTableIter  iter;
  ShutdownClient *client;
  const gchar    *bus_name;
  const gchar    *object_path;
  const gchar    *unit;
  GError         *error = NULL;
  gint            error_code;
  gint            shutdown_mode;

  g_return_if_fail (LA_HANDLER_IS_SERVICE (service));

  g_hash_table_iter_init (&iter, service->clients_to_units);
  while (g_hash_table_iter_next (&iter, (gpointer *)&client, (gpointer *)&unit))
    {
      /* extract data from the current client */
      bus_name = shutdown_client_get_bus_name (client);
      object_path = shutdown_client_get_object_path (client);
      shutdown_mode = shutdown_client_get_shutdown_mode (client);

      /* unregister the shutdown client */
      nsm_consumer_call_un_register_shutdown_client_sync (service->nsm_consumer,
                                                          bus_name, object_path,
                                                          shutdown_mode, &error_code,
                                                          NULL, &error);

      if (error != NULL)
        {
          DLT_LOG (la_handler_context, DLT_LOG_ERROR,
                   DLT_STRING ("Failed to unregister shutdown client:"),
                   DLT_STRING ("object path"), DLT_STRING (object_path),
                   DLT_STRING ("unit"), DLT_STRING (unit),
                   DLT_STRING ("error message"), DLT_STRING (error->message));
          g_error_free (error);
        }
      else if (error_code != NSM_ERROR_STATUS_OK)
        {
          DLT_LOG (la_handler_context, DLT_LOG_ERROR,
                   DLT_STRING ("Failed to unregister shutdown client:"),
                   DLT_STRING ("object path"), DLT_STRING (object_path),
                   DLT_STRING ("unit"), DLT_STRING (unit),
                   DLT_STRING ("error code"), DLT_INT (error_code));
        }
    }
}
