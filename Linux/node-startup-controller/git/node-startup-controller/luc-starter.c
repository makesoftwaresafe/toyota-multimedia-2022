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

#include <glib-object.h>
#include <gio/gio.h>

#include <dlt/dlt.h>

#include <common/nsm-lifecycle-control-dbus.h>

#include <node-startup-controller/job-manager.h>
#include <node-startup-controller/luc-starter.h>
#include <node-startup-controller/node-startup-controller-service.h>



/**
 * SECTION: luc-starter
 * @title: LUCStarter
 * @short_description: Starts the applications registered with the Last User Context.
 * @stability: Internal
 *
 * The #LUCStarter is a component of the #NodeStartupControllerApplication which manages 
 * the start of the Last User Context (LUC) applications.
 * 
 * When the #NodeStartupControllerApplication starts, it asks the #LUCStarter to start
 * the LUC applications. To start the LUC applications, the #LUCStarter will do the
 * following in order:
 *
 * 1. Checks with the Node State Manager (NSM) if starting the LUC applications
 *    is required. If it is not required, it notifies the groups of current applications
 *    that the start of the LUC has been processed, which never happens because starting
 *    the LUC is not required.
 *
 * If starting the LUC is required:
 *
 * 2. Removes the current user context.
 *
 * 3. Reads the LUC using the node_startup_controller_service_read_luc().
 *
 * 4. Starts the LUC applications asynchronously and in prioritised groups. The group of
 *    applications, which belong to the most prioritised LUC type, start first and then
 *    the next group of applications are started in the order of the prioritized types
 *    set at build-time, then in numerical order.
 *    When an application is started, the #LUCStarter keeps it in a table, associates
 *    this application with its #GCancellable (so it is possible to cancel each
 *    respective application if the start of the LUC is cancelled), and start the service
 *    calling the start method to #JobManager and passing its #GCancellable.
 *
 * 5. Notifies the groups of applications that the start of the LUC has been processed.
 *    
 */



DLT_IMPORT_CONTEXT (controller_context);



/* signal identifiers */
enum
{
  SIGNAL_LUC_GROUPS_STARTED,
  LAST_SIGNAL,
};



/* property identifiers */
enum
{
  PROP_0,
  PROP_JOB_MANAGER,
  PROP_NODE_STARTUP_CONTROLLER,
};


static void luc_starter_constructed               (GObject      *object);
static void luc_starter_finalize                  (GObject      *object);
static void luc_starter_get_property              (GObject      *object,
                                                   guint         prop_id,
                                                   GValue       *value,
                                                   GParamSpec   *pspec);
static void luc_starter_set_property              (GObject      *object,
                                                   guint         prop_id,
                                                   const GValue *value,
                                                   GParamSpec   *pspec);
static gint luc_starter_compare_luc_types         (gconstpointer a,
                                                   gconstpointer b,
                                                   gpointer      user_data);
static void luc_starter_start_next_group          (LUCStarter   *starter);
static void luc_starter_start_app                 (const gchar  *app,
                                                   LUCStarter   *starter);
static void luc_starter_start_app_finish          (JobManager   *manager,
                                                   const gchar  *unit,
                                                   const gchar  *result,
                                                   GError       *error,
                                                   gpointer      user_data);
static void luc_starter_cancel_start              (const gchar  *app,
                                                   GCancellable *cancellable,
                                                   gpointer      user_data);
static void luc_starter_check_luc_required_finish (GObject      *object,
                                                   GAsyncResult *res,
                                                   gpointer      user_data);
static void luc_starter_start_groups_for_real     (LUCStarter   *starter);



struct _LUCStarterClass
{
  GObjectClass __parent__;
};

struct _LUCStarter
{
  GObject                        __parent__;

  JobManager                    *job_manager;
  NodeStartupControllerService  *node_startup_controller;
  NSMLifecycleControl           *nsm_lifecycle_control;

  GArray                        *prioritised_types;

  GArray                        *start_order;
  GHashTable                    *start_groups;

  GHashTable                    *cancellables;
};



static guint luc_starter_signals[LAST_SIGNAL];



G_DEFINE_TYPE (LUCStarter, luc_starter, G_TYPE_OBJECT);



static void
luc_starter_class_init (LUCStarterClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = luc_starter_constructed;
  gobject_class->finalize = luc_starter_finalize;
  gobject_class->get_property = luc_starter_get_property;
  gobject_class->set_property = luc_starter_set_property;

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

  g_object_class_install_property (gobject_class,
                                   PROP_NODE_STARTUP_CONTROLLER,
                                   g_param_spec_object ("node-startup-controller",
                                                        "node-startup-controller",
                                                        "node-startup-controller",
                                                        TYPE_NODE_STARTUP_CONTROLLER_SERVICE,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  luc_starter_signals[SIGNAL_LUC_GROUPS_STARTED] =
    g_signal_new ("luc-groups-started",
                  TYPE_LUC_STARTER,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
luc_starter_init (LUCStarter *starter)
{
  /* allocate data structures for the start order and groups */
  starter->start_order = g_array_new (FALSE, TRUE, sizeof (gint));
  starter->start_groups = g_hash_table_new_full (g_direct_hash, g_direct_equal,
                                                 NULL, (GDestroyNotify) g_ptr_array_free);

  /* allocate a mapping of app names to correspoding cancellables */
  starter->cancellables = g_hash_table_new_full (g_str_hash, g_str_equal,
                                                 g_free, (GDestroyNotify) g_object_unref);
}



static void
luc_starter_constructed (GObject *object)
{
  LUCStarter *starter = LUC_STARTER (object);
  GError     *error = NULL;
  gchar     **types;
  guint       n;
  gint        type;

  /* connect to the node state manager */
  starter->nsm_lifecycle_control =
    nsm_lifecycle_control_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                                  G_DBUS_PROXY_FLAGS_NONE,
                                                  "org.genivi.NodeStateManager",
                                                  "/org/genivi/NodeStateManager/LifecycleControl",
                                                  NULL, &error);
  if (error != NULL)
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to connect to the NSM lifecycle control:"),
               DLT_STRING (error->message));
      g_error_free (error);
    }

  /* parse the prioritised LUC types defined at build-time */
  types = g_strsplit (PRIORITISED_LUC_TYPES, ",", -1);
  starter->prioritised_types = g_array_new (FALSE, TRUE, sizeof (gint));
  for (n = 0; types != NULL && types[n] != NULL; n++)
    {
      type = strtol (types[n], NULL, 10);
      g_array_append_val (starter->prioritised_types, type);
    }
  g_strfreev (types);
}



static void
luc_starter_finalize (GObject *object)
{
  LUCStarter *starter = LUC_STARTER (object);

  /* release NSMLifecycleControl */
  if (starter->nsm_lifecycle_control != NULL)
    g_object_unref (starter->nsm_lifecycle_control);

  /* release start order, and groups */
  g_array_free (starter->start_order, TRUE);
  g_hash_table_unref (starter->start_groups);

  /* release the cancellables */
  g_hash_table_unref (starter->cancellables);

  /* free the prioritised types array */
  g_array_free (starter->prioritised_types, TRUE);

  /* release the job manager */
  g_object_unref (starter->job_manager);

  /* release the node startup controller service */
  g_object_unref (starter->node_startup_controller);

  (*G_OBJECT_CLASS (luc_starter_parent_class)->finalize) (object);
}



static void
luc_starter_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  LUCStarter *starter = LUC_STARTER (object);

  switch (prop_id)
    {
    case PROP_JOB_MANAGER:
      g_value_set_object (value, starter->job_manager);
      break;
    case PROP_NODE_STARTUP_CONTROLLER:
      g_value_set_object (value, starter->node_startup_controller);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
luc_starter_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  LUCStarter *starter = LUC_STARTER (object);

  switch (prop_id)
    {
    case PROP_JOB_MANAGER:
      starter->job_manager = g_value_dup_object (value);
      break;
    case PROP_NODE_STARTUP_CONTROLLER:
      starter->node_startup_controller = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gint
luc_starter_compare_luc_types (gconstpointer a,
                               gconstpointer b,
                               gpointer      user_data)
{
  LUCStarter *starter = LUC_STARTER (user_data);
  guint       n;
  gint        type;
  gint        type_a = *(gint *)a;
  gint        type_b = *(gint *)b;
  gint        pos_a = G_MAXINT;
  gint        pos_b = G_MAXINT;

  /* try to find the first type in the prioritised types list */
  for (n = 0; pos_a == G_MAXINT && n < starter->prioritised_types->len; n++)
    {
      type = g_array_index (starter->prioritised_types, gint, n);
      if (type == type_a)
        pos_a = n;
    }

  /* try to find the second type in the prioritised types list */
  for (n = 0; pos_b == G_MAXINT && n < starter->prioritised_types->len; n++)
    {
      type = g_array_index (starter->prioritised_types, gint, n);
      if (type == type_b)
        pos_b = n;
    }

  /* this statement has the following effect when sorting:
   * - a and b are prioritised     -> prioritization order is preserved
   * - a is prioritised, b isn't   -> negative return value, a comes first
   * - b is prioritised, a isn't   -> positive return value, b comes first
   * - neither a nor b prioritised -> return value is 0, arbitrary order
   */
  return pos_a - pos_b;
}



static void
luc_starter_start_next_group (LUCStarter *starter)
{
  GPtrArray *apps;
  gint       group;

  g_return_if_fail (IS_LUC_STARTER (starter));
  g_return_if_fail (starter->start_order->len > 0);

  /* fetch the next group */
  group = g_array_index (starter->start_order, gint, 0);

  DLT_LOG (controller_context, DLT_LOG_INFO,
           DLT_STRING ("Starting LUC group:"), DLT_INT (group));

  /* look up the apps for the group */
  apps = g_hash_table_lookup (starter->start_groups, GINT_TO_POINTER (group));
  if (apps != NULL)
    {
      /* launch all the applications in the group asynchronously */
      g_ptr_array_foreach (apps, (GFunc) luc_starter_start_app, starter);
    }
}



static void
luc_starter_start_app (const gchar *app,
                       LUCStarter  *starter)
{
  GCancellable *cancellable;

  g_return_if_fail (app != NULL && *app != '\0');
  g_return_if_fail (IS_LUC_STARTER (starter));

  DLT_LOG (controller_context, DLT_LOG_INFO,
           DLT_STRING ("Starting LUC app:"), DLT_STRING (app));

  /* create the new cancellable */
  cancellable = g_cancellable_new ();

  /* store an association between each app and its cancellable, so that it is possible to
   * call g_cancellable_cancel() for each respective app. */
  g_hash_table_insert (starter->cancellables, g_strdup (app), cancellable);

  /* start the service, passing the cancellable */
  job_manager_start (starter->job_manager, app, cancellable,
                     luc_starter_start_app_finish,
                     g_object_ref (starter));
}



static void
luc_starter_start_app_finish (JobManager  *manager,
                              const gchar *unit,
                              const gchar *result,
                              GError      *error,
                              gpointer     user_data)
{
  LUCStarter *starter = LUC_STARTER (user_data);
  GPtrArray  *apps;
  gboolean    app_found = FALSE;
  guint       n;
  gint        group;

  g_return_if_fail (IS_JOB_MANAGER (manager));
  g_return_if_fail (unit != NULL && *unit != '\0');
  g_return_if_fail (IS_LUC_STARTER (user_data));
  g_return_if_fail (starter->start_order->len > 0);

  DLT_LOG (controller_context, DLT_LOG_INFO,
           DLT_STRING ("Finished starting LUC app:"), DLT_STRING (unit));

  /* respond to errors */
  if (error != NULL)
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to start LUC application:"),
               DLT_STRING ("unit"), DLT_STRING (unit),
               DLT_STRING ("error message"), DLT_STRING (error->message));
    }

  /* get the current start group */
  group = g_array_index (starter->start_order, gint, 0);

  /* look up the apps for this group */
  apps = g_hash_table_lookup (starter->start_groups, GINT_TO_POINTER (group));
  if (apps != NULL)
    {
      /* try to find the current app in the group */
      for (n = 0; !app_found && n < apps->len; n++)
        {
          if (g_strcmp0 (unit, g_ptr_array_index (apps, n)) == 0)
            app_found = TRUE;
        }

      /* remove the app from the group */
      if (app_found)
        g_ptr_array_remove_index (apps, n-1);

      /* check if this was the last app in the group to be started */
      if (apps->len == 0)
        {
          DLT_LOG (controller_context, DLT_LOG_INFO,
                   DLT_STRING ("Finished starting LUC group:"), DLT_INT (group));

          /* remove the group from the groups and the order */
          g_hash_table_remove (starter->start_groups, GINT_TO_POINTER (group));
          g_array_remove_index (starter->start_order, 0);

          /* check if we have more groups to start */
          if (starter->start_order->len > 0)
            {
              /* we do, so start the next group now */
              luc_starter_start_next_group (starter);
            }
          else
            {
              /* no, we are finished; notify others */
              g_signal_emit (starter, luc_starter_signals[SIGNAL_LUC_GROUPS_STARTED],
                             0, NULL);
            }
        }
    }

  /* remove the association between an app and its cancellable, because the app has
   * finished starting, so cannot be cancelled any more */
  g_hash_table_remove (starter->cancellables, unit);

  /* release the LUCStarter because the operation is finished */
  g_object_unref (starter);
}



static void
luc_starter_cancel_start (const gchar  *app,
                          GCancellable *cancellable,
                          gpointer      user_data)
{
  g_cancellable_cancel (cancellable);
}



static void
luc_starter_check_luc_required_finish (GObject      *object,
                                       GAsyncResult *res,
                                       gpointer      user_data)
{
  NSMLifecycleControl *nsm_lifecycle_control = NSM_LIFECYCLE_CONTROL (object);
  LUCStarter          *starter = LUC_STARTER (user_data);
  gboolean             luc_required = TRUE;
  GError              *error = NULL;

  g_return_if_fail (IS_NSM_LIFECYCLE_CONTROL (nsm_lifecycle_control));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (IS_LUC_STARTER (starter));

  /* finish the checking for reloading the LUC */
  if (!nsm_lifecycle_control_call_check_luc_required_finish (nsm_lifecycle_control,
                                                             &luc_required, res, &error))
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to check whether the LUC is required:"),
               DLT_STRING (error->message));
      g_clear_error (&error);

      DLT_LOG (controller_context, DLT_LOG_INFO,
               DLT_STRING ("Assuming that we should start the LUC"));

      /* start all the LUC groups now */
      luc_starter_start_groups_for_real (starter);
    }
  else
    {
      /* check whether we need to start the LUC or not */
      if (luc_required)
        {
          DLT_LOG (controller_context, DLT_LOG_INFO,
                   DLT_STRING ("LUC is required, starting it now"));

          /* start all the LUC groups now */
          luc_starter_start_groups_for_real (starter);
        }
      else
        {
          /* LUC is not required, log this information */
          DLT_LOG (controller_context, DLT_LOG_INFO, DLT_STRING ("LUC is not required"));

          /* notify others that we have started the LUC groups; we haven't
           * in this case but the call of luc_starter_start_groups() may
           * still want to be notified that the call has been processed */
          g_signal_emit (starter, luc_starter_signals[SIGNAL_LUC_GROUPS_STARTED],
                         0, NULL);
        }
    }
}



static void
luc_starter_start_groups_for_real (LUCStarter *starter)
{
  GVariantIter iter;
  GPtrArray   *group_apps;
  GVariant    *context;
  GError      *error = NULL;
  GList       *groups;
  GList       *lp;
  gchar      **apps;
  guint        n;
  gint         group;
  gint         type;

  g_return_if_fail (IS_LUC_STARTER (starter));

  /* log prioritised LUC types */
  DLT_LOG (controller_context, DLT_LOG_INFO, DLT_STRING ("Prioritised LUC types:"));
  for (n = 0; n < starter->prioritised_types->len; n++)
    {
      DLT_LOG (controller_context, DLT_LOG_INFO,
               DLT_INT (g_array_index (starter->prioritised_types, gint, n)));
    }

  /* clear the start order */
  if (starter->start_order->len > 0)
    g_array_remove_range (starter->start_order, 0, starter->start_order->len);

  /* clear the start groups */
  g_hash_table_remove_all (starter->start_groups);

  /* clear the mapping between apps and their cancellables */
  g_hash_table_remove_all (starter->cancellables);

  /* get the current last user context */
  context = node_startup_controller_service_read_luc (starter->node_startup_controller,
                                                      &error);
  if (error != NULL)
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to read the last user context:"),
               DLT_STRING (error->message));
      g_error_free (error);

      /* notify others that we are finished starting the groups, even if
       * that failed */
      g_signal_emit (starter, luc_starter_signals[SIGNAL_LUC_GROUPS_STARTED],
                     0, NULL);

      return;
    }

  /* create groups for all types in the LUC */
  g_variant_iter_init (&iter, context);
  while (g_variant_iter_loop (&iter, "{i^as}", &type, &apps))
    {
      group_apps = g_ptr_array_new_with_free_func (g_free);

      for (n = 0; apps != NULL && apps[n] != NULL; n++)
        g_ptr_array_add (group_apps, g_strdup (apps[n]));

      g_hash_table_insert (starter->start_groups, GINT_TO_POINTER (type), group_apps);
    }

  /* release the last user context */
  g_variant_unref (context);

  /* generate the start order by sorting the LUC types according to
   * the prioritised types */
  groups = g_hash_table_get_keys (starter->start_groups);
  for (lp = groups; lp != NULL; lp = lp->next)
    {
      group = GPOINTER_TO_INT (lp->data);
      g_array_append_val (starter->start_order, group);
    }
  g_array_sort_with_data (starter->start_order, luc_starter_compare_luc_types, starter);

  DLT_LOG (controller_context, DLT_LOG_INFO, DLT_STRING ("LUC start groups (ordered):"));
  for (n = 0; n < starter->start_order->len; n++)
    {
      DLT_LOG (controller_context, DLT_LOG_INFO,
               DLT_INT (g_array_index (starter->start_order, gint, n)));
    }

  if (starter->start_order->len > 0)
    luc_starter_start_next_group (starter);
}



/**
 * luc_starter_new:
 * @job_manager: A #JobManager object.
 * @node_startup_controller: A #NodeStartupControllerService object.
 *
 * Creates a new #LUCStarter object.
 *
 * Returns: A new instance of the #LUCStarter.
 */
LUCStarter *
luc_starter_new (JobManager                   *job_manager,
                 NodeStartupControllerService *node_startup_controller)
{
  g_return_val_if_fail (IS_JOB_MANAGER (job_manager), NULL);
  g_return_val_if_fail (IS_NODE_STARTUP_CONTROLLER_SERVICE (node_startup_controller), NULL);

  return g_object_new (TYPE_LUC_STARTER,
                       "job-manager", job_manager,
                       "node-startup-controller", node_startup_controller,
                       NULL);
}



/**
 * luc_starter_start_groups:
 * @starter: A #LUCStarter object.
 *
 * Checks with the NSM whether to start the LUC applications or not. If it is required to
 * start the LUC or the NSM is unavailable, it will start the LUC.
 */
void
luc_starter_start_groups (LUCStarter *starter)
{
  g_return_if_fail (IS_LUC_STARTER (starter));

  /* check whether the NSMLifecycleProxy is available or not */
  if (starter->nsm_lifecycle_control != NULL)
    {
      /* check with NSM whether to start the LUC */
      nsm_lifecycle_control_call_check_luc_required (starter->nsm_lifecycle_control, NULL,
                                                     luc_starter_check_luc_required_finish,
                                                     starter);
    }
  else
    {
      DLT_LOG (controller_context, DLT_LOG_WARN,
               DLT_STRING ("NSM unavailable, starting the LUC unconditionally"));

      /* start all the LUC groups now */
      luc_starter_start_groups_for_real (starter);
    }
}



/**
 * luc_starter_cancel:
 * @starter: A #LUCStarter object.
 *
 * Cancel the start of the LUC.
 *
 */
void
luc_starter_cancel (LUCStarter *starter)
{
  g_hash_table_foreach (starter->cancellables, (GHFunc) luc_starter_cancel_start, NULL);
}
