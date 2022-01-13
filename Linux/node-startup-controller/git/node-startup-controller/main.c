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

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <glib.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <node-startup-controller/la-handler-service.h>
#include <node-startup-controller/node-startup-controller-application.h>
#include <node-startup-controller/node-startup-controller-dbus.h>
#include <node-startup-controller/node-startup-controller-service.h>
#include <node-startup-controller/systemd-manager-dbus.h>
#include <node-startup-controller/target-startup-monitor.h>



DLT_DECLARE_CONTEXT (controller_context);
DLT_DECLARE_CONTEXT (la_handler_context);



static void
unregister_dlt (void)
{
  DLT_UNREGISTER_CONTEXT (controller_context);
  DLT_UNREGISTER_CONTEXT (la_handler_context);
  DLT_UNREGISTER_APP ();
}



int
main (int    argc,
      char **argv)
{
  NodeStartupControllerApplication *application;
  NodeStartupControllerService     *node_startup_controller;
  TargetStartupMonitor             *target_startup_monitor;
  LAHandlerService                 *la_handler_service;
  GDBusConnection                  *connection;
  SystemdManager                   *systemd_manager;
  JobManager                       *job_manager;
  GMainLoop                        *main_loop;
  GError                           *error = NULL;

  /* register the application and context in DLT */
  DLT_REGISTER_APP ("NSC", "GENIVI Node Startup Controller");
  DLT_REGISTER_CONTEXT (controller_context, "CTRL",
                        "Context of the Node Startup Controller itself");
  DLT_REGISTER_CONTEXT (la_handler_context, "LAH",
                        "Context of the Legacy Application Handler that hooks legacy "
                        "applications up with the shutdown concept of the Node State "
                        "Manager");

  /* have DLT unregistered at exit */
  atexit (unregister_dlt);

  /* initialize the GType type system */
  g_type_init ();

  /* attempt to connect to D-Bus */
  connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM, NULL, &error);
  if (connection == NULL)
    {
      DLT_LOG (controller_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to connect to the system bus:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);

      return EXIT_FAILURE;
    }

  /* attempt to connect to the systemd manager */
  systemd_manager =
    systemd_manager_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                            G_DBUS_PROXY_FLAGS_NONE,
                                            "org.freedesktop.systemd1",
                                            "/org/freedesktop/systemd1",
                                            NULL, &error);
  if (systemd_manager == NULL)
    {
      DLT_LOG (controller_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to connect to the systemd manager:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* subscribe to the systemd manager */
  if (!systemd_manager_call_subscribe_sync (systemd_manager, NULL, &error))
    {
      DLT_LOG (controller_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to subscribe to the systemd manager:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* instantiate the node startup controller service implementation */
  node_startup_controller = node_startup_controller_service_new (connection);

  /* attempt to start the node startup controller service */
  if (!node_startup_controller_service_start_up (node_startup_controller, &error))
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to start the node startup controller service:"),
               DLT_STRING (error->message));

      /* clean up */
      g_error_free (error);
      g_object_unref (node_startup_controller);
      g_object_unref (systemd_manager);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* instantiate the job manager */
  job_manager = job_manager_new (connection, systemd_manager);

  /* instantiate the legacy app handler */
  la_handler_service = la_handler_service_new (connection, job_manager);

  /* start the legacy app handler */
  if (!la_handler_service_start (la_handler_service, &error))
    {
      DLT_LOG (controller_context, DLT_LOG_FATAL,
               DLT_STRING ("Failed to start the legacy app handler service:"),
               DLT_STRING (error->message));

      /* clean up */
      g_clear_error (&error);
      g_object_unref (la_handler_service);
      g_object_unref (job_manager);
      g_object_unref (node_startup_controller);
      g_object_unref (systemd_manager);
      g_object_unref (connection);

      return EXIT_FAILURE;
    }

  /* create the main loop */
  main_loop = g_main_loop_new (NULL, FALSE);

  /* create the target startup monitor */
  target_startup_monitor = target_startup_monitor_new (systemd_manager);

  /* create and run the main application */
  application = node_startup_controller_application_new (main_loop, connection,
                                                         job_manager, la_handler_service,
                                                         node_startup_controller);

  /* run the main loop */
  g_main_loop_run (main_loop);
  g_main_loop_unref (main_loop);

  /* release allocated objects */
  g_object_unref (application);
  g_object_unref (target_startup_monitor);
  g_object_unref (systemd_manager);
  g_object_unref (job_manager);
  g_object_unref (node_startup_controller);
  g_object_unref (connection);

  return EXIT_SUCCESS;
}
