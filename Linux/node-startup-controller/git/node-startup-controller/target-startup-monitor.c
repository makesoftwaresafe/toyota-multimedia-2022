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

#include <common/nsm-enum-types.h>
#include <common/nsm-lifecycle-control-dbus.h>

#include <node-startup-controller/target-startup-monitor.h>
#include <node-startup-controller/systemd-unit-dbus.h>



/**
 * SECTION: target-startup-monitor
 * @title: TargetStartupMonitor
 * @short_description: Listens to JobRemoved signals and sets the node state.
 * @stability: Internal
 * 
 * The #TargetStartupMonitor class is responsible for setting the state of the
 * Node State Manager using states from #NSMNodeState. It sets the node state
 * under the following circumstances:
 * 
 * 1. The Node Startup Controller is starting up. In the GENIVI lifecycle subsystem,
 *    this means that all mandatory applications have been started - set the state to
 *    %NSM_NODE_STATE_BASE_RUNNING.
 * 
 * 2. The #TargetStartupMonitor receives a "JobRemoved" signal from "focussed.target",
 *    "unfocussed.target" or "lazy.target" indicating that this unit has changed its
 *     active state to "active" then:
 *
 *     - If the systemd unit "focussed.target" has started - set the state to
 *       %NSM_NODE_STATE_LUC_RUNNING.
 * 
 *     - If the systemd unit "unfocussed.target" has started - set the state to
 *       %NSM_NODE_STATE_FULLY_RUNNING.
 * 
 *     - If the systemd unit "lazy.target" has started - set the state to
 *       %NSM_NODE_STATE_FULLY_OPERATIONAL.
 */



DLT_IMPORT_CONTEXT (controller_context);



/* property identifier */
enum
{
  PROP_0,
  PROP_SYSTEMD_MANAGER,
};



typedef struct _GetUnitData GetUnitData;



static void     target_startup_monitor_finalize                (GObject              *object);
static void     target_startup_monitor_constructed             (GObject              *object);
static void     target_startup_monitor_get_property            (GObject              *object,
                                                                guint                 prop_id,
                                                                GValue               *value,
                                                                GParamSpec           *pspec);
static void     target_startup_monitor_set_property            (GObject              *object,
                                                                guint                 prop_id,
                                                                const GValue         *value,
                                                                GParamSpec           *pspec);
static void     target_startup_monitor_job_removed             (SystemdManager       *manager,
                                                                guint                 id,
                                                                const gchar          *job_name,
                                                                const gchar          *unit,
                                                                const gchar          *result,
                                                                TargetStartupMonitor *monitor);
static void     target_startup_monitor_get_unit_finish         (GObject              *object,
                                                                GAsyncResult         *res,
                                                                gpointer              user_data);
static void     target_startup_monitor_unit_proxy_new_finish   (GObject              *object,
                                                                GAsyncResult         *res,
                                                                gpointer              user_data);
static void     target_startup_monitor_set_node_state          (TargetStartupMonitor *monitor,
                                                                NSMNodeState          state);
static void     target_startup_monitor_set_node_state_finish   (GObject              *object,
                                                                GAsyncResult         *res,
                                                                gpointer              user_data);



struct _TargetStartupMonitorClass
{
  GObjectClass __parent__;
};

struct _TargetStartupMonitor
{
  GObject              __parent__;

  SystemdManager      *systemd_manager;

  NSMLifecycleControl *nsm_lifecycle_control;

  /* map of systemd target names to corresponding node states */
  GHashTable          *targets_to_states;
};

struct _GetUnitData
{
  TargetStartupMonitor *monitor;
  gchar                *unit_name;
  gchar                *object_path;
};



G_DEFINE_TYPE (TargetStartupMonitor, target_startup_monitor, G_TYPE_OBJECT);



static void
target_startup_monitor_class_init (TargetStartupMonitorClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = target_startup_monitor_finalize;
  gobject_class->constructed = target_startup_monitor_constructed;
  gobject_class->get_property = target_startup_monitor_get_property;
  gobject_class->set_property = target_startup_monitor_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_SYSTEMD_MANAGER,
                                   g_param_spec_object ("systemd-manager",
                                                        "systemd-manager",
                                                        "systemd-manager",
                                                        TYPE_SYSTEMD_MANAGER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));
}



static void
target_startup_monitor_init (TargetStartupMonitor *monitor)
{
  GError *error = NULL;

  /* create proxy to talk to the Node State Manager's lifecycle control */
  monitor->nsm_lifecycle_control =
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

  /* set the initial state to base running, which means that
   * the mandatory.target has been started (this is done before the
   * node startup controller itself is brought up) */
  target_startup_monitor_set_node_state (monitor, NSM_NODE_STATE_BASE_RUNNING);

  /* create the table of targets and their node states */
  monitor->targets_to_states = g_hash_table_new (g_str_hash, g_str_equal);
  g_hash_table_insert (monitor->targets_to_states, "focussed.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_LUC_RUNNING));
  g_hash_table_insert (monitor->targets_to_states, "unfocussed.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_FULLY_RUNNING));
  g_hash_table_insert (monitor->targets_to_states, "lazy.target",
                       GUINT_TO_POINTER (NSM_NODE_STATE_FULLY_OPERATIONAL));
}



static void
target_startup_monitor_constructed (GObject *object)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  g_signal_connect (monitor->systemd_manager, "job-removed",
                    G_CALLBACK (target_startup_monitor_job_removed), monitor);
}



static void
target_startup_monitor_finalize (GObject *object)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  /* release the mapping of systemd targets to node states */
  g_hash_table_destroy (monitor->targets_to_states);

  /* release the systemd manager */
  g_signal_handlers_disconnect_matched (monitor->systemd_manager,
                                        G_SIGNAL_MATCH_DATA,
                                        0, 0, NULL, NULL, monitor);
  g_object_unref (monitor->systemd_manager);

  if (monitor->nsm_lifecycle_control != NULL)
    g_object_unref (monitor->nsm_lifecycle_control);

  (*G_OBJECT_CLASS (target_startup_monitor_parent_class)->finalize) (object);
}



static void
target_startup_monitor_get_property (GObject    *object,
                                     guint       prop_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  switch (prop_id)
    {
    case PROP_SYSTEMD_MANAGER:
      g_value_set_object (value, monitor->systemd_manager);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
target_startup_monitor_set_property (GObject      *object,
                                     guint         prop_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  TargetStartupMonitor *monitor = TARGET_STARTUP_MONITOR (object);

  switch (prop_id)
    {
    case PROP_SYSTEMD_MANAGER:
      monitor->systemd_manager = g_value_dup_object (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
target_startup_monitor_job_removed (SystemdManager       *manager,
                                    guint                 id,
                                    const gchar          *job_name,
                                    const gchar          *unit,
                                    const gchar          *result,
                                    TargetStartupMonitor *monitor)
{
  GetUnitData *data;

  g_return_if_fail (IS_SYSTEMD_MANAGER (manager));
  g_return_if_fail (job_name != NULL && *job_name != '\0');
  g_return_if_fail (unit != NULL && *unit != '\0');
  g_return_if_fail (result != NULL && *result != '\0');
  g_return_if_fail (IS_TARGET_STARTUP_MONITOR (monitor));

  /* check if the unit corresponds to one which has to be monitored */
  if (g_hash_table_lookup_extended (monitor->targets_to_states, unit, NULL, NULL))
    {
      /* create a temporary struct to bundle information about the unit */
      data = g_slice_new0 (GetUnitData);
      data->monitor = g_object_ref (monitor);
      data->unit_name = g_strdup (unit);

      /* ask systemd to return the object path for this unit */
      systemd_manager_call_get_unit (monitor->systemd_manager, unit, NULL,
                                     target_startup_monitor_get_unit_finish, data);
    }
}



static void
target_startup_monitor_get_unit_finish (GObject      *object,
                                        GAsyncResult *res,
                                        gpointer      user_data)
{
  GetUnitData *data = user_data;
  GError      *error = NULL;
  gchar       *object_path;

  g_return_if_fail (IS_SYSTEMD_MANAGER (object));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  /* finish obtaining the object path for the unit from systemd */
  if (!systemd_manager_call_get_unit_finish (SYSTEMD_MANAGER (object), &object_path,
                                             res, &error))
    {
      /* there was an error, log it */
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to get a unit from systemd:"),
               DLT_STRING ("unit"), DLT_STRING (data->unit_name),
               DLT_STRING ("error message"), DLT_STRING (error->message));
      g_error_free (error);

      /* release the get unit data */
      g_object_unref (data->monitor);
      g_free (data->unit_name);
      g_slice_free (GetUnitData, data);
    }
  else
    {
      DLT_LOG (controller_context, DLT_LOG_INFO,
               DLT_STRING ("Creating D-Bus proxy:"),
               DLT_STRING ("object path"), DLT_STRING (object_path));

      /* remember the object path */
      data->object_path = object_path;

      /* create a proxy for this unit D-Bus object */
      systemd_unit_proxy_new (g_dbus_proxy_get_connection (G_DBUS_PROXY (data->monitor->systemd_manager)),
                              G_DBUS_PROXY_FLAGS_NONE,
                              "org.freedesktop.systemd1",
                              object_path,
                              NULL,
                              target_startup_monitor_unit_proxy_new_finish,
                              data);
    }
}



static void
target_startup_monitor_unit_proxy_new_finish (GObject      *object,
                                              GAsyncResult *res,
                                              gpointer      user_data)
{
  GetUnitData *data = user_data;
  SystemdUnit *unit;
  const gchar *state;
  gpointer     node_state;
  GError      *error = NULL;

  g_return_if_fail (G_IS_ASYNC_RESULT (res));
  g_return_if_fail (data != NULL);

  /* finish creating the proxy for this systemd unit */
  unit = systemd_unit_proxy_new_finish (res, &error);
  if (error != NULL)
    {
      /* there was an error, log it */
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to create D-Bus proxy:"),
               DLT_STRING ("object path"), DLT_STRING (data->object_path),
               DLT_STRING ("error message"), DLT_STRING (error->message));
      g_error_free (error);
    }
  else
    {
      /* query the unit for its current active state */
      state = systemd_unit_get_active_state (unit);

      /* log the the active state has changed */
      DLT_LOG (controller_context, DLT_LOG_INFO,
               DLT_STRING ("Active state of unit changed:"),
               DLT_STRING ("unit"), DLT_STRING (data->unit_name),
               DLT_STRING ("state"), DLT_STRING (state));

      /* check if the new state is active */
      if (g_strcmp0 (state, "active") == 0)
        {
          /* look up the node state corresponding to this unit, if there is one */
          if (g_hash_table_lookup_extended (data->monitor->targets_to_states,
                                            data->unit_name, NULL, &node_state))
            {
              /* we do have a state for this unit, so apply it now */
              target_startup_monitor_set_node_state (data->monitor,
                                                     GPOINTER_TO_UINT (node_state));
            }
        }

      /* release the unit proxy */
      g_object_unref (unit);
    }

  /* free the get unit data */
  g_object_unref (data->monitor);
  g_free (data->unit_name);
  g_free (data->object_path);
  g_slice_free (GetUnitData, data);
}



static void
target_startup_monitor_set_node_state (TargetStartupMonitor *monitor,
                                       NSMNodeState          state)
{
  g_return_if_fail (IS_TARGET_STARTUP_MONITOR (monitor));

  /* set node state in the Node State Manager */
  nsm_lifecycle_control_call_set_node_state (monitor->nsm_lifecycle_control,
                                             (gint) state, NULL,
                                             target_startup_monitor_set_node_state_finish,
                                             NULL);
}



static void
target_startup_monitor_set_node_state_finish (GObject      *object,
                                              GAsyncResult *res,
                                              gpointer      user_data)
{
  NSMLifecycleControl *nsm_lifecycle_control = NSM_LIFECYCLE_CONTROL (object);
  GError              *error = NULL;
  NSMErrorStatus       error_code = NSM_ERROR_STATUS_OK;

  g_return_if_fail (IS_NSM_LIFECYCLE_CONTROL (nsm_lifecycle_control));
  g_return_if_fail (G_IS_ASYNC_RESULT (res));

  /* finish setting the node state in the NSM */
  if (!nsm_lifecycle_control_call_set_node_state_finish (nsm_lifecycle_control,
                                                         (gint *) &error_code, res,
                                                         &error))
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to set the node state:"),
               DLT_STRING (error->message));
      g_error_free (error);
    }
  else if (error_code != NSM_ERROR_STATUS_OK)
    {
      DLT_LOG (controller_context, DLT_LOG_ERROR,
               DLT_STRING ("Failed to set the node state:"),
               DLT_STRING ("error code"), DLT_UINT (error_code));
    }
}



/**
 * target_startup_monitor_new:
 * @systemd_manager: An interface to the systemd manager created with
 * systemd_manager_proxy_new_for_bus_sync()
 * 
 * Creates a new target startup monitor and begins listening to %JobRemoved signals from
 * systemd.
 * 
 * Returns: A new instance of the #TargetStartupMonitor.
 */
TargetStartupMonitor *
target_startup_monitor_new (SystemdManager *systemd_manager)
{
  g_return_val_if_fail (IS_SYSTEMD_MANAGER (systemd_manager), NULL);

  return g_object_new (TYPE_TARGET_STARTUP_MONITOR,
                       "systemd-manager", systemd_manager,
                       NULL);
}
