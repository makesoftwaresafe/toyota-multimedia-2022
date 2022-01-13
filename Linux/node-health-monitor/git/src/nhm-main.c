/* NHM - NodeHealthMonitor
 *
 * Copyright (C) 2013 Continental Automotive Systems, Inc.
 *
 * This Source Code Form is subject to the terms of the
 * Mozilla Public License, v. 2.0. If a copy of the MPL
 * was not distributed with this file, You can obtain
 * one at http://mozilla.org/MPL/2.0/.
 *
 * Author: Jean-Pierre Bogler <Jean-Pierre.Bogler@continental-corporation.com>
 */

/**
 * SECTION:nhm-main
 * @title: NodeHealthMonitor (NHM)
 * @short_description: Supervises the node's health
 *
 * The Node Health Monitor will usually be started by systemd and will interact
 * with application plug-ins to inform it that a component has failed in the
 * system. He will be responsible for:
 *
 *   - providing an interface with which plug-ins can register failures
 *     - name of the failing service, used to identify and track failures
 *   - tracking failure statistics over multiple LCs for system and components
 *      - name of failing service used to identify/track component failures
 *      - NHM will maintain a count of the number of failures in the current
 *        life cycle as well as statistics on number of failures in last X life
 *        cycles (i.e. 3 failures in last 32 life cycles)
 *  - observe the life cycle accordingly to catch unexpected system restarts
 *  - provide an interface for plug-ins to read system/component error counts
 *  - provide an interface for plug-ins to request a node restart
 *
 * Additionally the Node Health Monitor will test a number of product defined
 * criteria with the aim to ensure that userland is stable and functional.
 * It will be able to validate that:
 *
 *   - defined file is present
 *   - defined processes are still running
 *   - a user defined process can be executed with an expected result
 *   - communication on defined dbus (bus address) is possible
 *
 * If the NHM believes that there is an issue with user land
 * then it will initiate a system restart.
 */

/******************************************************************************
*
* Header includes
*
******************************************************************************/

/* Own header files */
#include "inc/NodeHealthMonitor.h"
#include "nhm-systemd.h"
#include "nhm-helper.h"

/* System header files                                                      */
#include <stdio.h>                          /* FILE, write, read            */
#include <string.h>                         /* Use strlen                   */
#include <stdlib.h>                         /* Use strtol                   */
#include <signal.h>                         /* Define SIGTERM               */
#include <errno.h>                          /* Use errno                    */
#include <glib-unix.h>                      /* Catch SIGTERM                */
#include <gio/gio.h>                        /* GIO for dbus                 */
#include <glib-2.0/glib.h>                  /* GLIB for lists, arrays, etc. */
#include <dlt/dlt.h>                        /* DLT Log'n'Trace              */
#include <systemd/sd-daemon.h>              /* systemd WDOG                 */
#include <NodeStateTypes.h>                 /* NSM types for dbus comm.     */
#include <persistence_client_library.h>     /* Init/DeInit PCL              */
#include <persistence_client_library_key.h> /* Access persistent data       */


/* Generated D-Bus interface header files                                 */
#include <gen/nhm-dbus-info.h>        /* Own dbus info interface          */
#include <gen/nsm-dbus-consumer.h>    /* Consumer interface of the NSM    */
#include <gen/nsm-dbus-lc-control.h>  /* LC Control interface of the NSM  */
#include <gen/nsm-dbus-lc-consumer.h> /* LC Consumer interface of the NSM */


/******************************************************************************
*
* Constants, types and defines
*
******************************************************************************/

/* File to manage data */
#define NHM_LC_DATA_FILE (DATADIR"lcdata")

/* Persistence IDs to manage shutdown flag */
#define NHM_SHUTDOWN_FLAG_LDBID 0xFF
#define NHM_SHUTDOWN_FLAG_NAME  "PKV_NHM_SHUTDOWN_FLAG"

/* File to load config from (flag 'CONFDIR' comes from Makefile) */
#define NHM_CFG_FILE (CONFDIR"node-health-monitor.conf")

/* Definitions for NSM connection */
#define NHM_LC_CLIENT_OBJ     "/org/genivi/NodeHealthMonitor/LifecycleClient"
#define NHM_LC_CLIENT_TIMEOUT 1000

#define NHM_DOES_NOT_USE_PCL /* this patch for the malfunction avoidance of PCL */

/**
 * NhmNodeState:
 * @NHM_NODESTATE_NOTSET:   Default value to init. variables.
 * @NHM_NODESTATE_STARTED:  NHM writes this value when it starts.
 * @NHM_NODESTATE_SHUTDOWN: NHM writes this value when it is shut down.
 *
 * NhmNodeState is used to determine if the system has been shut down correctly.
 */
typedef enum
{
  NHM_NODESTATE_NOTSET,
  NHM_NODESTATE_STARTED,
  NHM_NODESTATE_SHUTDOWN
} NhmNodeState;

/**
 * NhmCurrentFailedApp:
 * @name: Name of the failed app.
 *
 * Info for currently failed app, used to create list of currently failed apps
 */
typedef struct
{
  gchar *name;
} NhmCurrentFailedApp;

/**
 * NhmFailedApp:
 * @name:      Name of the failed app.
 * @failcount: Number of times, the app. switched from running to failed.
 *
 * Info for a failed app, used to create list of failed apps in a LC.
 */
typedef struct
{
  gchar *name;
  guint  failcount;
} NhmFailedApp;

/**
 * NhmLcInfo:
 * @start_state: State which was found in flag file, when NHM started.
 * @failed_apps: List of failed apps in the LC.
 *
 * Info for a LC. Used to create an array with info. for multiple LCs.
 */
typedef struct
{
  NhmNodeState  start_state;
  GSList       *failed_apps;
} NhmLcInfo;

/**
 * NhmMonitoredDbus:
 * @bus_addr: Bus address of the observed dbus.
 * @bus_conn: Connection to bus, created at startup.
 *
 * Used to create an array of monitored busses, based on the config.
 */
typedef struct
{
  gchar           *bus_addr;
  GDBusConnection *bus_conn;
} NhmCheckedDbus;

/******************************************************************************
*
* Prototypes for file local functions (see implementation for description)
*
******************************************************************************/

/* Functions to free occupied memory */
static void                  nhm_main_free_lc_info             (gpointer                lc_info);
static void                  nhm_main_free_failed_app          (gpointer                failed_app);
static void                  nhm_main_free_current_failed_app  (gpointer                failed_app);
static void                  nhm_main_free_checked_dbus        (gpointer                checked_dbus);
static void                  nhm_main_free_nhm_objects         (void);
static void                  nhm_main_free_nsm_objects         (void);
static void                  nhm_main_free_config_objects      (void);
static void                  nhm_main_free_check_objects       (void);

/* Functions to find apps. */
static NhmFailedApp         *nhm_main_find_failed_app          (NhmLcInfo              *lc_info,
                                                                const gchar            *search_app);
static NhmCurrentFailedApp  *nhm_main_find_current_failed_app  (const gchar            *search_app);

/* Helper functions for dbus callbacks */
static void                  nhm_main_check_failed_app_restart (void);
static NhmErrorStatus_e      nhm_main_request_restart          (NsmRestartReason_e      restart_reason,
                                                                guint                   restart_type);
static void                  nhm_main_register_app_status      (const gchar            *name,
                                                                NhmAppStatus_e          status);

/* Callbacks for D-Bus interfaces */
static gboolean              nhm_main_read_statistics_cb       (NhmDbusInfo            *object,
                                                                GDBusMethodInvocation  *invocation,
                                                                const gchar            *app_name,
                                                                gpointer                user_data);
static gboolean              nhm_main_register_app_status_cb   (NhmDbusInfo            *object,
                                                                GDBusMethodInvocation *invocation,
                                                                const gchar           *app_name,
                                                                gint                   app_status,
                                                                gpointer               user_data);
static gboolean              nhm_main_request_node_restart_cb  (NhmDbusInfo           *object,
                                                                GDBusMethodInvocation *invocation,
                                                                const gchar           *app_name,
                                                                gpointer               user_data);
static gboolean              nhm_main_lc_request_cb            (NsmDbusLcConsumer     *object,
                                                                GDBusMethodInvocation *invocation,
                                                                const guint            shutdown_type,
                                                                const guint            request_id,
                                                                gpointer               user_data);
/* Watchdog and other callbacks */
static void                  nhm_main_start_wdog               (void);
static gboolean              nhm_main_timer_wdog_cb            (gpointer               user_data);
static gboolean              nhm_main_on_sigterm               (gpointer               user_data);

/* Bus connection functions and callbacks */
static gboolean              nhm_main_connect_to_nsm           (void);
static void                  nhm_main_bus_acquired_cb          (GDBusConnection       *connection,
                                                                const gchar           *bus_name,
                                                                gpointer               user_data);
static void                  nhm_main_name_acquired_cb         (GDBusConnection       *connection,
                                                                const gchar           *bus_name,
                                                                gpointer               user_data);
static void                  nhm_main_name_lost_cb             (GDBusConnection       *connection,
                                                                const gchar           *bus_name,
                                                                gpointer               user_data);

/* Functions for userland checks */
static gboolean              nhm_does_file_exist                (gchar                *file_name);
static gboolean              nhm_main_is_process_running        (gchar                *prog);
static gboolean              nhm_main_is_process_ok             (gchar                *process);
static gboolean              nhm_main_is_dbus_alive             (NhmCheckedDbus       *checked_dbus);
static gboolean              nhm_main_timer_userland_check_cb   (gpointer              user_data);

/* Functions to read and write run time data */
static void                  nhm_main_write_data                (void);
static void                  nhm_main_read_data                 (void);
static NhmNodeState          nhm_main_read_shutdown_flag        (void);
static gboolean              nhm_main_write_shutdown_flag       (NhmNodeState          flagval);

/* Functions to import and process configuration */
static void                  nhm_main_load_config               (void);
static guint                 nhm_main_config_load_uint          (GKeyFile             *file,
                                                                 gchar                *group,
                                                                 gchar                *key,
                                                                 guint                 defval);
static gchar               **nhm_main_config_load_string_array  (GKeyFile             *file,
                                                                 gchar                *group,
                                                                 gchar                *key,
                                                                 gchar               **defval);
static void                  nhm_main_prepare_checks            (void);


/******************************************************************************
*
* Local variables and constants
*
******************************************************************************/

/* Skeleton and proxy objects and connection for D-Bus */
static GDBusConnection   *nsmbusconn           = NULL;
static NsmDbusConsumer   *dbus_consumer_obj    = NULL;
static NsmDbusLcControl  *dbus_lc_control_obj  = NULL;
static NhmDbusInfo       *dbus_nhm_info_obj    = NULL;
static NsmDbusLcConsumer *dbus_lc_consumer_obj = NULL;

/* Variables to control the main loop */
static gint               mainreturn           = 0;
static GMainLoop         *mainloop             = NULL;

/* Run time data. Array for life cycles and list for the current LC */
static GPtrArray         *nodeinfo             = NULL;
static GSList            *current_failed_apps  = NULL;

/* Variables to handle configured checks */
static GPtrArray         *checked_dbusses      = NULL;

/* Variables to read the configuration */
static gchar            **no_restart_apps      = NULL;
static guint              max_lc_count         = 0;
static guint              max_failed_apps      = 0;

static guint              ul_chk_interval      = 0;
static gchar            **monitored_files      = NULL;
static gchar            **monitored_procs      = NULL;
static gchar            **monitored_progs      = NULL;
static gchar            **monitored_dbus       = NULL;


/******************************************************************************
*
* Local (static) functions
*
******************************************************************************/

/**
 * nhm_main_free_checked_dbus:
 * @checked_dbus: Pointer to 'NhmCheckedDbus' object.
 *
 * Frees the memory occupied by a 'NhmCheckedDbus' object.
 * It is used as 'free func' for the array 'checked_dbusses'.
 *
 */
static void
nhm_main_free_checked_dbus(gpointer checked_dbus)
{
  NhmCheckedDbus *bus = (NhmCheckedDbus*) checked_dbus;

  g_free(bus->bus_addr);

  if(bus->bus_conn != NULL)
  {
    g_object_unref(bus->bus_conn);
  }

  g_free(checked_dbus);
}

/**
 * nhm_main_free_current_failed_app:
 * @failed_app: Pointer to 'NhmCurrentFailedApp' object.
 *
 * Frees the memory occupied by a 'NhmCurrentFailedApp' object.
 * Can be used in 'g_slist_free_full' to free list of current failed apps.
 *
 */
static void
nhm_main_free_current_failed_app(gpointer failed_app)
{
  g_free(((NhmCurrentFailedApp*) failed_app)->name);
  g_free(failed_app);
}


/**
 * nhm_main_free_failed_app:
 * @failed_app: Pointer to 'NhmFailedApp' object.
 *
 * Frees the memory occupied by a 'NhmFailedApp' object.
 * Can be used in 'g_slist_free_full' to free the list of failed apps.
 */
static void
nhm_main_free_failed_app(gpointer failed_app)
{
  g_free(((NhmFailedApp*) failed_app)->name);
  g_free(failed_app);
}


/**
 * nhm_main_free_lc_info:
 * @lcinfo: Pointer to 'NhmLcInfo' object.
 *
 * Frees the memory occupied by a 'NhmLcInfo' object.
 * It is used as 'free func' for the array 'nodeinfo'.
 */
static void
nhm_main_free_lc_info(gpointer lcinfo)
{
  g_slist_free_full(((NhmLcInfo*) lcinfo)->failed_apps,
                    &nhm_main_free_failed_app);
  g_free(lcinfo);
}


/**
 * nhm_main_find_failed_app:
 * @lcinfo:  Pointer to the life cycle in which the app. should be searched.
 * @appname: Name of the app. that is searched for.
 *
 * The function searches in the failed app. list
 * of the passed LC for an app. with the passed name.
 *
 * Return value: Ptr. to the app. info of searched app.
 *               %NULL if the app. is not found.
 */
static NhmFailedApp*
nhm_main_find_failed_app(NhmLcInfo   *lcinfo,
                         const gchar *appname)
{
  GSList       *list = NULL;
  NhmFailedApp *app  = NULL;

  /* Loop through the list of failed apps. until app is found or list ends */
  for(list = lcinfo->failed_apps;
      (list != NULL) && (app == NULL);
      list = g_slist_next(list))
  {
    /* App. found, if the app. name of the stored object equals passed name */
    app =   (g_strcmp0(((NhmFailedApp*) list->data)->name, appname) == 0)
          ? (NhmFailedApp*) list->data : NULL;
  }

  return app;
}


/**
 * nhm_main_find_current_failed_app:
 * @appname: Name of the app. that is searched for.
 *
 * Searches in the currently failed app. list for an app. with the passed name.
 *
 * Return value: Pointer to the current failed app. info.
 * of the searched app. or %NULL if app. is not found.
 */
static NhmCurrentFailedApp*
nhm_main_find_current_failed_app(const gchar *appname)
{
  GSList              *list = NULL;
  NhmCurrentFailedApp *app  = NULL;

  /* Loop through currently failed apps. until app is found or list ends */
  for(list = current_failed_apps;
      (list != NULL) && (app == NULL);
      list = g_slist_next(list))
  {
    app =   (g_strcmp0(((NhmCurrentFailedApp*) list->data)->name, appname) == 0)
          ? (NhmCurrentFailedApp*) list->data : NULL;
  }

  return app;
}


/**
 * nhm_main_request_restart:
 * @restart_reason: Reason for the restart request
 * @restart_type:  Type of the desired restart (NSM_SHUTDOWNTYPE_*)
 *
 * The function is called from 'nhm_main_check_failed_app_restart'
 * and 'nhm_main_request_node_restart_cb' if a node restart should be
 * requested at the NSM.
 *
 * Return value:
 *
 *   NhmErrorStatus_Ok:                 NSM accepted the restart request.
 *                                      Restart should be ongoing.
 *   NhmErrorStatus_RestartNotPossible: NSM rejected the restart request.
 *   NhmErrorStatus_Error:              Could not communicate to NSM via D-Bus.
 */
static NhmErrorStatus_e
nhm_main_request_restart(NsmRestartReason_e restart_reason,
                         guint              restart_type)
{
  NhmErrorStatus_e  retval     = NhmErrorStatus_Error;
  NsmErrorStatus_e  nsm_retval = NsmErrorStatus_NotSet;
  GError           *error      = NULL;

  /* Trace message and send restart request to NSM */
  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Sending restart request to NSM.");
          DLT_STRING("RestartReason:"); DLT_INT(restart_reason);
          DLT_STRING("RestartType:"  ); DLT_UINT(restart_type ));

  (void) nsm_dbus_lc_control_call_request_node_restart_sync(dbus_lc_control_obj,
                                                            (gint) restart_reason,
                                                            restart_type,
                                                            (gint*) &nsm_retval,
                                                            NULL,
                                                            &error);
  if(error == NULL) /* Evaluate the calls result. */
  {
    if(nsm_retval == NsmErrorStatus_Ok)
    {
      retval = NhmErrorStatus_Ok; /* The NSM accepted the RestartRequest */
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: NSM accepted the restart request."));
    }
    else
    {
      retval = NhmErrorStatus_RestartNotPossible; /* The NSM rejected the RestartRequest */
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: NSM rejected the restart request.");
              DLT_STRING("Return value:"); DLT_INT(nsm_retval));
    }
  }
  else
  {
    /* Error: D-Bus communication failed. */
    retval = NhmErrorStatus_Error;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Sending restart request to NSM failed.");
            DLT_STRING("Error: D-Bus communication to NSM failed.");
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);
  }

  return retval;
}


/**
 * nhm_main_check_failed_app_restart:
 *
 * The function is called from 'nhm_main_register_app_status_cb' whenever
 * an application failed. It determines the number of failed applications
 * in the current LC. If the number is higher than the configured value,
 * the function requests a node restart at the NSM.
 */
static void
nhm_main_check_failed_app_restart(void)
{
  guint failed_app_cnt = 0;

  /* If the failed app. observation is active */
  if(max_failed_apps != 0)
  {
    /* Get the amount of currently failed apps. and compare to the max. value */
    failed_app_cnt =   (current_failed_apps != NULL)
                     ? g_slist_length(current_failed_apps) : 0;

    if(failed_app_cnt >= max_failed_apps)
    {
      /* The amount of failed applications is too high. Request a node restart. */
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: Amount of failed apps too high.");
              DLT_STRING("FailCount:"); DLT_UINT(failed_app_cnt);
              DLT_STRING("Limit:"    ); DLT_UINT(max_failed_apps));

      (void) nhm_main_request_restart(NsmRestartReason_ApplicationFailure,
                                      NSM_SHUTDOWNTYPE_NORMAL);
    }
  }
}


/**
 * nhm_main_read_statistics_cb:
 * @object:     Pointer to NhmDbusInfo object
 * @invocation: Pointer to D-Bus invocation of this call
 * @app_name:   This will be the name of the application for which the calling
 *              application wants to know the failure count for. If this value
 *              is an empty string the NHM will return the failure statistics
 *              for the whole node.
 * @user_data:  Pointer to optional user data
 *
 * This function is called from dbus to read the failure count of either a
 * particular app. or of the node itself. Returns the following values on dbus:
 *
 * CurrentFailCount: If there is an app. name:
 *                   How many times the app has failed in the current lifecycle
 *                   If app. name is empty:
 *                   How many apps currently 'failed' in the current lifecycle
 *
 * TotalFailures:    If there is an app. name:
 *                   Number of times that app has failed in the last X LCs.
 *                   If app. name is empty:
 *                   Number of times node failed to shutdown in the last X LCs.
 *
 * TotalLifecycles:  Number of lifecycles used to collect data.
 *
 * Return value: Always %TRUE. Method has been processed.
 */
static gboolean
nhm_main_read_statistics_cb(NhmDbusInfo           *object,
                            GDBusMethodInvocation *invocation,
                            const gchar           *app_name,
                            gpointer               user_data)
{
  guint         lc_idx           = 0;
  NhmLcInfo    *lc_info          = NULL;
  NhmFailedApp *app_info         = NULL;
  guint         current_fail_cnt = 0;
  guint         total_failures   = 0;

  /* Check if the node statistics should be retrieved (empty AppName) */
  if(strlen(app_name) == 0)
  {
    /* Node statistics requested. Store number of currently failed apps. */
    current_fail_cnt =   (current_failed_apps != NULL)
                       ? g_slist_length(current_failed_apps) : 0;

    /* Loop through all life cycles and sum up failed shut downs */
    for(lc_idx = 0; (lc_idx < nodeinfo->len) && (lc_idx <= max_lc_count); lc_idx++)
    {
      lc_info         = (NhmLcInfo*) g_ptr_array_index(nodeinfo, lc_idx);
      total_failures += (lc_info->start_state != NHM_NODESTATE_SHUTDOWN) ? 1 : 0;
    }
  }
  else
  {
    /* App. statistics requested. Get fail count for app. in current LC */
    lc_info  = (NhmLcInfo*) g_ptr_array_index(nodeinfo, 0);
    app_info = nhm_main_find_failed_app(lc_info, app_name);
    current_fail_cnt = (app_info != NULL) ? app_info->failcount : 0;

    /* Init. total fail cnt with current fail cnt. Add fail cnt of previous LCs */
    total_failures = current_fail_cnt;
    for(lc_idx = 1; (lc_idx < nodeinfo->len) && (lc_idx <= max_lc_count); lc_idx++)
    {
      lc_info  = (NhmLcInfo*) g_ptr_array_index(nodeinfo, lc_idx);
      app_info = nhm_main_find_failed_app(lc_info, app_name);
      total_failures += (app_info != NULL) ? app_info->failcount : 0;
    }
  }

  /* Complete D-Bus call. Send return to D-Bus caller. */
  nhm_dbus_info_complete_read_statistics(object,
                                         invocation,
                                         current_fail_cnt,
                                         total_failures,
                                         lc_idx,
                                         (gint) NhmErrorStatus_Ok);

  return TRUE;
}



/**
 * nhm_main_register_app_status:
 * @name:    This is the unit name of the application that has failed
 * @status:  This can be used to specify the status of the application that has failed.
 *           It will be based upon the enum NHM_ApplicationStatus_e.
 *
 * The function is called via the dbus interface or from the systemd
 * observation when either a NHM client wants to register a failed app.
 * or the state of a systemd unit changed. The NHM will maintain an internal
 * list of the applications that are currently in a failed state.
 * Additionally it will maintain a count of the currently failed applications
 * that can be used to trigger a system restart if the value gets too high.
 * The NHM will also call the NSM method SetAppHealthStatus which will allow
 * the NSM to disable any sessions that might have been enabled by the failed
 * application and send out the signal 'AppHealthStatus'.
 */
static void
nhm_main_register_app_status(const gchar    *name,
                             NhmAppStatus_e  status)
{
  GError              *error          = NULL;
  NsmErrorStatus_e     nsm_rval       = NsmErrorStatus_NotSet;
  NhmLcInfo           *lc_info        = NULL;
  NhmFailedApp        *app_info       = NULL;
  NhmCurrentFailedApp *app_on_list    = NULL;
  gboolean             app_running    = FALSE;

  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Processing 'RegisterAppStatus' call");
          DLT_STRING("AppName:"); DLT_STRING(name),
          DLT_STRING("Status:");  DLT_INT(status));

  /* Forward the call to the NSM who will process it for its own purposes. */
  app_running = (status == NhmAppStatus_Ok);
  (void) nsm_dbus_lc_control_call_set_app_health_status_sync(dbus_lc_control_obj,
                                                             name,
                                                             app_running,
                                                             (gint*) &nsm_rval,
                                                             NULL,
                                                             &error);
  if(error != NULL) /* Check for D-Bus errors */
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to forward app. status to NSM.");
            DLT_STRING("Error: D-Bus communication to NSM failed.");
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);
  }

  /* Transparently emit 'AppStatus' signal */
  nhm_dbus_info_emit_app_health_status(dbus_nhm_info_obj, name, status);

  /* Start internal processing. Check if app. is on current failed list. */
  app_on_list = nhm_main_find_current_failed_app(name);

  if((app_on_list == NULL) && (status == NhmAppStatus_Failed))
  {
    /* App. not on list and the new status is failed. Add it to the list! */
    app_on_list         = g_new(NhmCurrentFailedApp, 1);
    app_on_list->name   = g_strdup(name);
    current_failed_apps = g_slist_append(current_failed_apps, app_on_list);

    /* Try to get the app. in the list of failed apps. of the current LC */
    lc_info  = (NhmLcInfo*) g_ptr_array_index(nodeinfo, 0);
    app_info = nhm_main_find_failed_app(lc_info, name);

    if(app_info == NULL)
    {
      /* Failed app has not been on list. Init. error count with 1 and add it */
      app_info             = g_new(NhmFailedApp, 1);
      app_info->name       = g_strdup(name);
      app_info->failcount  = 0;
      lc_info->failed_apps = g_slist_append(lc_info->failed_apps, app_info);
    }

    app_info->failcount++; /* increase fail count (either of old or new app.) */

    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_INFO,
            DLT_STRING("NHM: Updated error count for application.");
            DLT_STRING("AppName:");    DLT_STRING(app_info->name);
            DLT_STRING("Fail count:"); DLT_UINT(app_info->failcount));

    nhm_main_write_data();
    nhm_main_check_failed_app_restart();
  }
  else
  {
    /* The app is on the list, but not failed anymore. Remove it! */
    if((app_on_list != NULL) && (status != NhmAppStatus_Failed))
    {
      nhm_main_free_current_failed_app(app_on_list);
      current_failed_apps = g_slist_remove(current_failed_apps, app_on_list);
    }
  }
}


/**
 * nhm_main_register_app_status_cb:
 * @object:      Pointer to NhmDbusInfo object
 * @invocation:  Pointer to D-Bus invocation of this call
 * @app_name:    This is the unit name of the application that has failed.
 * @app_status:  This can be used to specify the status of the application
 *               that has failed.
 *               It will be based upon the enum NHM_ApplicationStatus_e.
 * @user_data:   Pointer to optional user data
 *
 * This function is called from dbus when a NHM client wants to register
 * that an application has failed or recovered from a previous failure.
 *
 * Return value: Always %TRUE. Method has been processed.
 */
static gboolean
nhm_main_register_app_status_cb(NhmDbusInfo           *object,
                                GDBusMethodInvocation *invocation,
                                const gchar           *app_name,
                                gint                   app_status,
                                gpointer               user_data)
{

  nhm_main_register_app_status(app_name, (NhmAppStatus_e) app_status);
  nhm_dbus_info_complete_register_app_status(object, invocation);

  return TRUE;
}


/**
 * nhm_main_request_node_restart_cb:
 * @object:     Pointer to NhmDbusInfo object
 * @invocation: Pointer to D-Bus invocation of this call
 * @app_name:   This is the unit name of the application that has failed.
 * @user_data:  Pointer to optional user data
 *
 * This function is called from dbus when a NHM client wants to request a
 * node restart if a critical application can not be recovered. The NHM
 * will have the possibility to internally evaluate whether the failed
 * application is important enough to warrant the restarting of the node.
 * The NHM will then forward the request to the NSM who will evaluate
 * whether a restart is allowed at the current time.
 *
 * Return value: Always %TRUE. Method has been processed.
 */
static gboolean
nhm_main_request_node_restart_cb(NhmDbusInfo           *object,
                                 GDBusMethodInvocation *invocation,
                                 const gchar           *app_name,
                                 gpointer               user_data)
{
  NhmErrorStatus_e retval = NhmErrorStatus_Error;

  /* Check if the app. is on the black list "no_restart_apps" */
  if(nhm_helper_str_in_strv(app_name, no_restart_apps) == FALSE)
  {
    /* The app is not on the black list. Forward the request to the NSM. */
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_INFO,
            DLT_STRING("NHM: Restart request from app. accepted.");
            DLT_STRING("AppName:"); DLT_STRING(app_name));

    retval = nhm_main_request_restart(NsmRestartReason_ApplicationFailure,
                                      NSM_SHUTDOWNTYPE_NORMAL);
  }
  else
  {
    /* The app is on the black list (no_restart_apps). Return an error. */
    retval = NhmErrorStatus_RestartNotPossible;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_INFO,
            DLT_STRING("NHM: Restart request from app. rejected.");
            DLT_STRING("AppName:"); DLT_STRING(app_name));
  }

  /* Complete D-Bus call. Send return to D-Bus caller. */
  nhm_dbus_info_complete_request_node_restart(object,
                                              invocation,
                                              (gint) retval);

  return TRUE;
}


/**
 * nhm_does_file_exist:
 * @file_name: The full path to a file
 *
 * Tests if a file exists.
 *
 * Return value: If the file exists %TRUE, otherwise %FALSE.
 */
static gboolean
nhm_does_file_exist(gchar* file_name)
{
  return g_file_test(file_name, G_FILE_TEST_EXISTS);
}


/**
 * nhm_main_is_process_running:
 * @process: Full path to the executable of the process
 *
 * The function checks if a process is running (process info in '/proc').
 *
 * Return value: %TRUE, if process is currently running,
 *               otherwise %FALSE;
 */
static gboolean
nhm_main_is_process_running(gchar* prog)
{
  GDir        *root_dir   = NULL;
  const gchar *proc_dir   = NULL;
  gchar       *exe_link   = NULL;
  gchar       *prog_name  = NULL;
  gboolean     found_prog = FALSE;
  GError      *error      = NULL;

  /* Open 'proc' directory in rootfs */
  root_dir = g_dir_open("/proc/", 0, &error);

  if(error == NULL)
  {
    /* Proc directory opened. Check every file/folder inside */
    for(proc_dir = g_dir_read_name(root_dir);
        (proc_dir != NULL) && (found_prog == FALSE);
        proc_dir = g_dir_read_name(root_dir))
    {
      /* For every file/folder in 'proc' create path to exe link */
      exe_link = g_strconcat("/proc/", proc_dir, "/exe", NULL);

      /* Check if 'exe' link points to searched executable */
      prog_name = g_file_read_link(exe_link, NULL);
      if(prog_name != NULL)
      {
        found_prog = (g_strcmp0(prog_name, prog) == 0);
        g_free(prog_name);
      }

      g_free(exe_link);
    }

    g_dir_close(root_dir);
  }
  else
  {
    /* 'proc' directory could not be opened. Return an error. */
    found_prog = FALSE;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Program check failed. Proc folder not readable.");
            DLT_STRING("Error: Proc folder not readable.");
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);
  }

  return found_prog;
}


/**
 * nhm_main_is_process_ok:
 * @process: Full path to the executable of the process
 *
 * The function checks if a user defined process returns valid (0).
 *
 * Return value: %TRUE, if process is returned with 0, otherwise %FALSE;
 */
static gboolean
nhm_main_is_process_ok(gchar *process)
{
  gchar    *argv[]      = {process, NULL};
  GError   *error       = NULL;
  gint      proc_retval = 0;
  gboolean  proc_ok     = FALSE;

  /* Start the process.*/
  (void) g_spawn_sync(NULL,
                     argv,
                     NULL,
                     G_SPAWN_STDOUT_TO_DEV_NULL|G_SPAWN_STDERR_TO_DEV_NULL,
                     NULL,
                     NULL,
                     NULL,
                     NULL,
                     &proc_retval,
                     &error);

  /* Check for errors of prg execution. */
  if(error == NULL)
  {
    /* Prog. executed. Check that return value is '0' */
    proc_ok = (proc_retval == 0);
  }
  else
  {
    /* Prog. execution failed. */
    proc_ok = FALSE;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Process check failed.");
            DLT_STRING("Error: Monitored process not started.");
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);
  }

  return proc_ok;
}


/**
 * nhm_main_is_dbus_alive:
 * @checked_dbus: Observed bus, created at startup.
 *
 * The function checks if a user defined dbus exists (bus address valid)
 * and if there is response when calling "Ping" on org.freedesktop.DBus.
 *
 * Return value: %TRUE, if checks succeed, otherwise %FALSE;
 */
static gboolean
nhm_main_is_dbus_alive(NhmCheckedDbus *checked_dbus)
{
  GError   *error       = NULL;
  GVariant *dbus_return = NULL;
  gboolean  retval      = FALSE;

  /* Step 1: Check if connection already was opened. If not, open it */
  if(checked_dbus->bus_conn == NULL)
  {
    checked_dbus->bus_conn = g_dbus_connection_new_for_address_sync(checked_dbus->bus_addr,
                                                                      G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT
                                                                    | G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,
                                                                    NULL,
                                                                    NULL,
                                                                    &error);
    if(error == NULL)
    {
      retval = TRUE;
      g_dbus_connection_set_exit_on_close(checked_dbus->bus_conn, FALSE);
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: D-Bus observation failed.");
              DLT_STRING("Error: Failed to connect to observed bus.");
              DLT_STRING("Bus address:"); DLT_STRING(checked_dbus->bus_addr);
              DLT_STRING("Reason:");      DLT_STRING(error->message));
      g_error_free(error);
    }
  }
  else
  {
    retval = TRUE;
  }

  /* Step 2: If there is an open conn., call method of default dbus object */
  if(retval == TRUE)
  {
    dbus_return = g_dbus_connection_call_sync(checked_dbus->bus_conn,
                                              "org.freedesktop.DBus",
                                              "/org/freedesktop/DBus",
                                              "org.freedesktop.DBus",
                                              "GetId",
                                              NULL,
                                              NULL,
                                              G_DBUS_CALL_FLAGS_NONE,
                                              -1,
                                              NULL,
                                              &error);
    if(error == NULL)
    {
      retval = TRUE;
      g_variant_unref(dbus_return);
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: D-Bus observation failed.");
              DLT_STRING("Error: Failed to call dbus method.");
              DLT_STRING("Bus address:"); DLT_STRING(checked_dbus->bus_addr);
              DLT_STRING("Reason:");      DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  return retval;
}


/**
 * nhm_main_start_wdog:
 *
 * The function reads the watchdog timeout value from the environment and starts
 * a cyclic timer if a timeout is configured.
 */
static void
nhm_main_start_wdog(void)
{
  const gchar *wdog_str_us = NULL;
  gchar       *endptr      = NULL;
  guint64      wdog_us     = 0;
  guint        wdog_ms     = 0;

  wdog_str_us = g_getenv("WATCHDOG_USEC");

  if(wdog_str_us != NULL)
  {
    errno = 0;
    wdog_us = g_ascii_strtoull(wdog_str_us, &endptr, 10);

    if(    (*endptr == '\0'       )   /* Parsed to end of string */
        && (endptr  != wdog_str_us)   /* String was not empty    */
        && (errno   == 0          ))  /* No error (overflow) set */
    {
      wdog_us /= 2000; /* convert us to ms and get half timeout */
      wdog_ms = (wdog_us > G_MAXUINT) ? G_MAXUINT : (guint) wdog_us;

      (void) g_timeout_add(wdog_ms, &nhm_main_timer_wdog_cb, NULL);

      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: Started watchdog timer.");
              DLT_STRING("Cycle:"); DLT_UINT(wdog_ms); DLT_STRING("ms"));
    }
    else
    {
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Watchdog initialization failed.");
              DLT_STRING("Error: Failed to parse WATCHDOG_USEC.");
              DLT_STRING("Value:"); DLT_STRING(wdog_str_us));
    }
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_WARN,
            DLT_STRING("NHM: Watchdog timeout not configured."));
  }
}


/**
 * nhm_main_timer_wdog_cb:
 * @user_data: Optional user data
 *
 * The function is a timer callback, used to trigger the watchdog periodically.
 *
 * Return value: Always %TRUE to keep timer for callback alive.
 */
static gboolean
nhm_main_timer_wdog_cb(gpointer user_data)
{
  (void) sd_notify(0, "WATCHDOG=1");
  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Triggered systemd WDOG."));

  return TRUE;
}


/**
 * nhm_main_timer_userland_check_cb:
 * @user_data: Optional user data
 *
 * The function is a timer callback, called periodically to perform 'userland'
 * checks. If a user land check fails, the function resets the system!
 *
 * Return value: Always %TRUE to keep timer for callback alive.
 */
static gboolean
nhm_main_timer_userland_check_cb(gpointer user_data)
{
  guint    check_idx = 0;
  gboolean ul_ok     = TRUE;

  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Userland check started."));

  /* Check if monitored files exist */
  if(monitored_files != NULL)
  {
    for(check_idx = 0;
        (check_idx < g_strv_length(monitored_files)) && (ul_ok == TRUE);
        check_idx++)
    {
      ul_ok = nhm_does_file_exist(monitored_files[check_idx]);
    }

    if(ul_ok == FALSE)
    {
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: Userland check failed.");
              DLT_STRING("Reason: Monitored file does not exist.");
              DLT_STRING("File name:");
              DLT_STRING(monitored_files[check_idx - 1]));
    }
  }

  /* Check if monitored programs re running */
  if(ul_ok == TRUE)
  {
    if(monitored_progs != NULL)
    {
      for(check_idx = 0;
          (check_idx < g_strv_length(monitored_progs)) && (ul_ok == TRUE);
          check_idx++)
      {
        ul_ok = nhm_main_is_process_running(monitored_progs[check_idx]);
      }

      if(ul_ok == FALSE)
      {
        DLT_LOG(nhm_helper_trace_ctx,
                DLT_LOG_INFO,
                DLT_STRING("NHM: Userland check failed.");
                DLT_STRING("Reason: Monitored program not running.");
                DLT_STRING("Prog name:");
                DLT_STRING(monitored_progs[check_idx - 1]));
      }
    }
  }

  /* Check if monitored processes return valid */
  if(ul_ok == TRUE)
  {
    if(monitored_procs != NULL)
    {
      for(check_idx = 0;
          (check_idx < g_strv_length(monitored_procs)) && (ul_ok == TRUE);
          check_idx++)
      {
        ul_ok = nhm_main_is_process_ok(monitored_procs[check_idx]);
      }

      if(ul_ok == FALSE)
      {
        DLT_LOG(nhm_helper_trace_ctx,
                DLT_LOG_INFO,
                DLT_STRING("NHM: Userland check failed.");
                DLT_STRING("Reason: Monitored proc. returned invalid.");
                DLT_STRING("Proc name:");
                DLT_STRING(monitored_procs[check_idx - 1]));
      }
    }
  }

  /* Check if monitored dbusses are alive */
  if(ul_ok == TRUE)
  {
    if(checked_dbusses != NULL)
    {
      for(check_idx = 0;
          (check_idx < checked_dbusses->len) && (ul_ok == TRUE);
          check_idx++)
      {
        if(nhm_main_is_dbus_alive((NhmCheckedDbus*)
            g_ptr_array_index(checked_dbusses, check_idx)) == FALSE)
        {
          ul_ok = FALSE;
          DLT_LOG(nhm_helper_trace_ctx,
                  DLT_LOG_INFO,
                  DLT_STRING("NHM: Userland check failed.");
                  DLT_STRING("Reason: Monitored dbus returned invalid.");
                  DLT_STRING("Bus name:");
                  DLT_STRING(((NhmCheckedDbus*)
                     g_ptr_array_index(checked_dbusses, check_idx))->bus_addr));
        }
      }
    }
  }

  /* Print outcome of userland check */
  if(ul_ok == TRUE)
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_INFO,
            DLT_STRING("NHM: Userland check successfully finished."));
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_INFO,
            DLT_STRING("NHM: Userland check failed. Restarting system."));
  }

  return TRUE;
}


/**
 * nhm_main_lc_request_cb:
 * @object:        NSM LifecycleConsumer object
 * @invocation:    Invocation object of dbus
 * @shutdown_type: Type of the shut down (NSM_SHUTDOWN_TYPE_*)
 * @request_id:    ID of shut down request. Used for async. completion call
 * @user_data:     Optionally user data
 *
 * The function is a dbus callback, called by the NSM to shut down the system.
 * Because NHM usually is the last 'shutdown client', write the shut down flag.
 *
 * Return value: Always %TRUE to inform dbus that call has been processed.
 */
static gboolean
nhm_main_lc_request_cb(NsmDbusLcConsumer     *object,
                       GDBusMethodInvocation *invocation,
                       const guint            shutdown_type,
                       const guint            request_id,
                       gpointer               user_data)
{
  NsmErrorStatus_e error = NsmErrorStatus_NotSet;

  if(shutdown_type != NSM_SHUTDOWNTYPE_RUNUP)
  {
    error =   (nhm_main_write_shutdown_flag(NHM_NODESTATE_SHUTDOWN) == TRUE)
            ? NsmErrorStatus_Ok : NsmErrorStatus_Error;
  }
  else
  {
    error =   (nhm_main_write_shutdown_flag(NHM_NODESTATE_STARTED) == TRUE)
            ? NsmErrorStatus_Ok : NsmErrorStatus_Error;
  }

  /* Inform NSM that shut down request was successfully processed */
  nsm_dbus_lc_consumer_complete_lifecycle_request(object,
                                                  invocation,
                                                  (gint) error);

  return TRUE;
}


/**
 * nhm_main_bus_acquired_cb:
 * @connection: Connection, which was acquired
 * @bus_name:   Bus name
 * @user_data:  Optionally user data
 *
 * The function is called when a connection to the D-Bus could be established.
 */
static void
nhm_main_bus_acquired_cb(GDBusConnection *connection,
                         const gchar     *bus_name,
                         gpointer         user_data)
{
  GError    *error   = NULL;
  NhmLcInfo *lc_info = NULL;

  /* "Protect" NHM from exit when conn. fails. Important to monitor dbus */
  g_dbus_connection_set_exit_on_close(connection, FALSE);

  /* The first array element always has to be the current LC. Create it! */
  nodeinfo = g_ptr_array_new_with_free_func(&nhm_main_free_lc_info);
  lc_info = g_new(NhmLcInfo, 1);

  /* Load shutdown flag. Indicates if last LC was regular shut down */
  lc_info->start_state = nhm_main_read_shutdown_flag();
  lc_info->failed_apps = NULL;
  g_ptr_array_add(nodeinfo, (gpointer) lc_info);

  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Previous shutdown was");
          DLT_STRING(  (lc_info->start_state == NHM_NODESTATE_SHUTDOWN)
                      ? "complete" : "incomplete"));

  /* Read data of prev. LCs. They are added to 'nodeinfo' after current LC */
  nhm_main_read_data();

  /* Create skeleton object, register signals and export interfaces */
  dbus_nhm_info_obj = nhm_dbus_info_skeleton_new();

  (void) g_signal_connect(dbus_nhm_info_obj,
                          "handle-register-app-status",
                          G_CALLBACK(nhm_main_register_app_status_cb),
                          NULL);

  (void) g_signal_connect(dbus_nhm_info_obj,
                          "handle-read-statistics",
                          G_CALLBACK(nhm_main_read_statistics_cb),
                          NULL);

  (void) g_signal_connect(dbus_nhm_info_obj,
                          "handle-request-node-restart",
                          G_CALLBACK(nhm_main_request_node_restart_cb),
                          NULL);

  (void) g_dbus_interface_skeleton_export(G_DBUS_INTERFACE_SKELETON(dbus_nhm_info_obj),
                                          connection,
                                          NHM_INFO_OBJECT,
                                          &error);
  if(error != NULL)
  {
    /* Critical error: The interface could not be exported. Stop the NHM. */
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Bus acquired actions failed.");
            DLT_STRING("Error: Could not export D-Bus object.");
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);

    mainreturn = EXIT_FAILURE;
    g_main_loop_quit(mainloop);
  }
}


/**
 * nhm_main_name_acquired_cb:
 * @connection: Connection over which the bus name was acquired
 * @app_name:   Acquired bus name
 * @user_data:  Optionally user data
 *
 * The function is called when the "bus name" could be acquired on the D-Bus.
 */
static void
nhm_main_name_acquired_cb(GDBusConnection *connection,
                          const gchar     *app_name,
                          gpointer         user_data)
{
  /* The NHM is now completely functional. Reset the shutdown flag */
  (void) nhm_main_write_shutdown_flag(NHM_NODESTATE_STARTED);

  /* Write initial state of current LC (no apps failed) and last prev. LCs*/
  nhm_main_write_data();

  /* If a user land check is configured, install the timer */
  if(ul_chk_interval != 0)
  {
    g_timeout_add_seconds(ul_chk_interval,
                          &nhm_main_timer_userland_check_cb,
                          NULL);
  }

  if(nhm_systemd_connect(&nhm_main_register_app_status) == FALSE)
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_WARN,
            DLT_STRING("NHM: Systemd observation could not be started."));
  }

  /* Inform systemd that we started up and start timer for systemd WDOG */
  (void) sd_notify (0, "READY=1");
  nhm_main_start_wdog();

  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Successfully obtained D-Bus name."));
}


/**
 * nhm_main_name_lost_cb:
 * @connection:      Connection. If it is NULL, no D-Bus connection could be
 *                   established. Otherwise the bus name was lost.
 * @param app_name:  Bus name
 * @param user_data: Optionally user data
 *
 * The function is called if either no connection to D-Bus could be established
 * or the bus name could not be acquired.
 */
static void
nhm_main_name_lost_cb(GDBusConnection *connection,
                      const gchar     *app_name,
                      gpointer         user_data)
{
  /* If the connection pointer is NULL connection has been lost */
  if(connection == NULL)
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: D-Bus connection failed."));
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: D-Bus name not obtained or lost."));
  }

  mainreturn = EXIT_FAILURE;
  g_main_loop_quit(mainloop); /* Cancel loop in all cases */
}


/**
 * nhm_main_convert_version_string:
 * @version_string: String containing the package version. The @version_string
 *                  can consist of up to four positive numbers <= 255, seperated
 *                  by dots (e.g. "1.21.3.4").
 *
 * The function will convert a version string (four positive numbers <= 255,
 * seperated by dots) to a 32 Bit value.
 *
 * Return value: Numeric value representing the version number.
 */
static guint32
nhm_main_convert_version_string(const gchar *version_string)
{
  gchar** version_split = NULL;
  guint   version_idx   = 0;
  guint32 version       = 0;

 version_split = g_strsplit(version_string, ".", 4);

 for(version_idx = 0; version_idx < g_strv_length(version_split); version_idx++)
 {
   version |= (strtol(version_split[version_idx], NULL, 10)
               << (24 -(8 * version_idx)));
 }

 g_strfreev(version_split);

 return version;
}


/**
 * nhm_main_write_data:
 *
 * The function writes the information about the previous life cycles to a
 * file in a binary format. The content of the file is structured as follows:
 *
 * Ver.   | # of Lc | state  | # of apps | # app chars | app name    | app fails
 * 4 Byte | 4 Byte  | 4 Byte | 4 Byte    | 4 Byte      | # app chars | 4 Byte
 *                                       | # app chars | app name    | app fails
 *                                       | 4 Byte      | # app chars | 4 Byte
 *                                                                  ...
 *                  | state  | # of apps | # app chars | app name    | app fails
 *                  | 4 Byte | 4 Byte    | 4 Byte      | # app chars | 4 Byte
 *                                       | # app chars | app name    | app fails
 *                                       | 4 Byte      | # app chars | 4 Byte
 *                                                                  ...
 *
 * At the start of the file, the version of the NHM is stored. The version is
 * followed by the number of stored life cycles. For each life cycle, the first
 * information is the shutdown state. After that, the number of failed apps. in
 * the life cycle is stored. For each failed app., the app. name is stored
 * (str. length and string) and the fail count.
 */
static void
nhm_main_write_data(void)
{
  FILE         *file          = NULL;
  guint         lc_idx        = 0;
  NhmLcInfo    *lc_info       = NULL;
  GSList       *app_list      = NULL;
  NhmFailedApp *app_info      = NULL;
  guint         lc_list_size  = 0;
  guint         app_list_size = 0;
  guint         app_name_len  = 0;
  guint         nhm_version   = 0;

  file = fopen(NHM_LC_DATA_FILE, "w"); /* Open file to store data */

  if(file != NULL)
  {
    /* Store the NHM version */
    nhm_version = nhm_main_convert_version_string(VERSION);
    fwrite((void*) &nhm_version, sizeof(nhm_version), 1, file);

    /* Store # of LCs (amount required by config. or at least what we have) */
    lc_list_size = MIN(nodeinfo->len, max_lc_count);
    fwrite((void*) &lc_list_size, sizeof(lc_list_size), 1, file);

    for(lc_idx = 0; lc_idx < lc_list_size; lc_idx++)
    {
      /* For every LC, store it's info. Start with the 'shutdown state'. */
      lc_info = (NhmLcInfo*) g_ptr_array_index(nodeinfo, lc_idx);
      fwrite((void*) &(lc_info->start_state), sizeof(lc_info->start_state), 1, file);

      /* Store the number of failed apps. that occured in this life cycle */
      app_list      = lc_info->failed_apps;
      app_list_size = (app_list != NULL) ? g_slist_length(app_list) : 0;
      fwrite((void*) &app_list_size, sizeof(app_list_size), 1, file);

      for(app_list = lc_info->failed_apps; app_list != NULL; app_list = g_slist_next(app_list))
      {
        /* For every app store app. name (length and string) and fail count */
        app_info     = (NhmFailedApp*) app_list->data;
        app_name_len = strlen((char*) app_info->name) + 1;
        fwrite((void*) &app_name_len, sizeof(app_name_len), 1, file);
        fwrite((void*) app_info->name, app_name_len, 1, file);
        fwrite((void*) &(app_info->failcount), sizeof(app_info->failcount), 1, file);
      }
    }

    fclose(file); /* close the file */
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Write LcData failed.");
            DLT_STRING("Error: Failed to open file.");
            DLT_STRING("File:"); DLT_STRING(NHM_LC_DATA_FILE));
  }
}


/**
 * nhm_main_read_data:
 *
 * The function reads the information about the previous life cycles from a
 * binary file. For the structure of the file see 'nhm_main_write_data'.
 * For each LC a 'NhmLcInfo' structure is allocated. For every app. in each
 * life cycle, a 'NHM__tstAppInfo' structure and storage to store the app.
 * name are allocated. Remember to free this memory!
 */
static void
nhm_main_read_data(void)
{
  FILE         *file          = NULL;
  guint         lc_idx        = 0;
  NhmLcInfo    *lc_Info       = NULL;
  GSList       *app_list      = NULL;
  NhmFailedApp *app_info      = NULL;
  guint         lc_list_size  = 0;
  guint         app_idx       = 0;
  guint         app_list_size = 0;
  guint         app_name_len  = 0;
  guint         nhm_version   = 0;

  file = fopen(NHM_LC_DATA_FILE, "r"); /* Open file to read data */

  if(file != NULL)
  {
    /* Read NHM version */
    fread(&nhm_version, 4, 1, file);

    /* Read # of LCs. Load configured amount or at least the stored ones */
    fread((void*) &lc_list_size, sizeof(lc_list_size), 1, file);
    lc_list_size = MIN(lc_list_size, max_lc_count);

    for(lc_idx = 0; lc_idx < lc_list_size; lc_idx++)
    {
      /* Create a new LC. Read its 'shutdown' flag */
      lc_Info = g_new(NhmLcInfo, 1);
      fread((void*) &(lc_Info->start_state), sizeof(lc_Info->start_state), 1, file);

      /* Create a new app. list for the LC. Read number of stored apps. */
      app_list = NULL;
      fread((void*) &app_list_size, sizeof(app_list_size), 1, file);

      for(app_idx = 0; app_idx < app_list_size; app_idx++)
      {
        /* For each stored app. Create a new app. info */
        app_info = g_new(NhmFailedApp, 1);

        /* Read the app. name length, allocate storage for string and read it */
        fread((void*) &app_name_len, sizeof(app_name_len), 1, file);

        app_info->name = g_new(gchar, app_name_len);
        fread((void*) app_info->name, app_name_len, 1, file);

        /* Read the apps. fail count */
        fread((void*) &(app_info->failcount), sizeof(app_info->failcount), 1, file);

        /* Append the new app. info to the app. list */
        app_list = g_slist_append(app_list, app_info);
      }

      /* Assign the recently read app. list to LC and store new LC in array. */
      lc_Info->failed_apps = app_list;
      g_ptr_array_add(nodeinfo, lc_Info);
    }

    fclose(file); /* Close the file */
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Read LcData failed.");
            DLT_STRING("Error: Failed to open file.");
            DLT_STRING("File:"); DLT_STRING(NHM_LC_DATA_FILE));
  }
}


/**
 * nhm_main_write_shutdown_flag:
 * @node_state: Value that should be written to the shutdown flag
 *
 * The function writes the persistent shut down flag
 */
static gboolean
nhm_main_write_shutdown_flag(NhmNodeState node_state)
{
  int      persval = 0;
  gboolean retval  = FALSE;

#ifdef NHM_DOES_NOT_USE_PCL
  /* this patch for the malfunction avoidance of PCL */
  persval = sizeof(node_state);
#else
  persval = pclKeyWriteData(NHM_SHUTDOWN_FLAG_LDBID,
                            NHM_SHUTDOWN_FLAG_NAME,
                            0,
                            0,
                            (unsigned char*) &node_state,
                            sizeof(node_state));
#endif

  if(persval == sizeof(node_state))
  {
    retval = TRUE;
  }
  else
  {
    /* Error: Did not write expected amount of bytes or got an error (< 0) */
    retval = FALSE;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to write 'shutdown flag'.");
            DLT_STRING("Error: Unexpected return from PCL.");
            DLT_STRING("Database ID:"); DLT_INT(NHM_SHUTDOWN_FLAG_LDBID);
            DLT_STRING("Key:");         DLT_STRING(NHM_SHUTDOWN_FLAG_NAME);
            DLT_STRING("User:");        DLT_INT(0);
            DLT_STRING("Seat:");        DLT_INT(0);
            DLT_STRING("Return:");      DLT_INT(persval));
  }

  return retval;
}


/**
 * nhm_main_read_shutdown_flag:
 *
 * The function reads the persistent shut down flag.
 *
 * Return value: Value read from shut down flag file.
 */
static NhmNodeState
nhm_main_read_shutdown_flag(void)
{
  NhmNodeState retval  = NHM_NODESTATE_NOTSET;
  int          persval = 0;

#ifdef NHM_DOES_NOT_USE_PCL
  /* this patch for the malfunction avoidance of PCL */
  retval = NHM_NODESTATE_SHUTDOWN;
  persval = sizeof(retval);
#else
  persval = pclKeyReadData(NHM_SHUTDOWN_FLAG_LDBID,
                           NHM_SHUTDOWN_FLAG_NAME,
                           0,
                           0,
                           (unsigned char*) &retval,
                           sizeof(retval));
#endif

  if(persval != sizeof(retval))
  {
    /* Error: Did not read expected amount of bytes or got an error (< 0) */
    retval = NHM_NODESTATE_NOTSET;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to read 'shutdown flag'.");
            DLT_STRING("Error: Unexpected return from PCL.");
            DLT_STRING("Database ID:"); DLT_INT(NHM_SHUTDOWN_FLAG_LDBID);
            DLT_STRING("Key:");         DLT_STRING(NHM_SHUTDOWN_FLAG_NAME);
            DLT_STRING("User:");        DLT_INT(0);
            DLT_STRING("Seat:");        DLT_INT(0);
            DLT_STRING("Return:");      DLT_INT(persval));
  }

  return retval;
}


/**
 * nhm_main_config_load_uint:
 * @file:   Pointer to key file.
 * @group:  Group name, in which key resists
 * @key:    Key name
 * @defval: Default return, if value can't be read.
 *
 * The function loads an unsigned integer from the given file, group and key.
 * If the key is not accessible or the read value is negative, the default
 * value is returned.
 *
 * Return value: Unsigned integer loaded from config file or default value.
 */
static guint
nhm_main_config_load_uint(GKeyFile *file,
                          gchar    *group,
                          gchar    *key,
                          guint     defval)
{
  GError *error  = NULL;
  gint    retval = 0;

  /* Load value from key */
  retval = g_key_file_get_integer(file, group, key, &error);

  if(error == NULL)
  {
    /* Value from config could be read. Check if it is negative. */
    if(retval >= 0)
    {
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: Loaded config value.");
              DLT_STRING("Group:"); DLT_STRING(group);
              DLT_STRING("Key:");   DLT_STRING(key);
              DLT_STRING("Value:"); DLT_INT(retval));
    }
    else
    {
      /* Error. A negative number has been read. Use default value. */
      retval = (gint) defval;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to load config value.");
              DLT_STRING("Error: Value out of range.");
              DLT_STRING("Group:"); DLT_STRING(group);
              DLT_STRING("Key:");   DLT_STRING(key));
    }
  }
  else
  {
    /* Error. Failed to load the value. Print error and use default value. */
    retval = (gint) defval;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to load config value.");
            DLT_STRING("Error: Could not get integer.");
            DLT_STRING("Group:");  DLT_STRING(group);
            DLT_STRING("Key:");    DLT_STRING(key);
            DLT_STRING("Reason:"); DLT_STRING(error->message ));
    g_error_free(error);
  }

  return (guint) retval;
}


/**
 * nhm_main_config_load_string_array:
 * @file:   Pointer to key file.
 * @group:  Group name, in which key resists
 * @key:    Key name
 * @defval: Default return, if value can't be read. May be NULL.
 *
 * The function loads a string list from the given file, group and key.
 * If the key is not accessible, the default value is returned. In all
 * cases, the returned string array has to be freed with 'g_strfreev'.
 *
 *  Return value: Ptr. to a string array. Has to be freed with 'g_strfreev'.
 */
static gchar**
nhm_main_config_load_string_array(GKeyFile *file,
                                  gchar    *group,
                                  gchar    *key,
                                  gchar   **defval)
{
  /* Function local variables */
  GError  *error       = NULL;
  gchar  **retval      = NULL;
  gsize    list_size   = 0;
  gchar   *loaded_list = NULL;

  /* Load value from key */
  retval = g_key_file_get_string_list(file, group, key, &list_size, &error);

  if(error == NULL)
  {
    loaded_list = g_strjoinv(";", retval);

    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_INFO,
            DLT_STRING("NHM: Loaded config value.");
            DLT_STRING("Group:"); DLT_STRING(group);
            DLT_STRING("Key:");   DLT_STRING(key);
            DLT_STRING("Value:"); DLT_STRING(loaded_list));
  }
  else
  {
    /* Error. Failed to load the value. Print error and use default value. */
    retval = g_strdupv(defval);
    loaded_list = g_strjoinv(";", retval);

    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to load config value.");
            DLT_STRING("Error: Could not get string.");
            DLT_STRING("Group:");  DLT_STRING(group);
            DLT_STRING("Key:");    DLT_STRING(key);
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);
  }

  if(strlen(loaded_list) == 0)
  {
    g_strfreev(retval);
    retval = NULL;
  }

  g_free(loaded_list);

  return retval;
}


/**
 * nhm_main_load_config:
 *
 * The function loads the configuration.
 * If a value is not accessable, default values are used.
 */
static void
nhm_main_load_config(void)
{
  GKeyFile *file  = NULL;
  GError   *error = NULL;

  /* Variables to configure default values on error */
  gchar *def_no_restart_apps[] = {"", NULL};
  gchar *def_monitored_files[] = {"", NULL};
  gchar *def_monitored_progs[] = {"", NULL};
  gchar *def_monitored_procs[] = {"", NULL};
  gchar *def_monitored_dbus[]  = {"", NULL};

  /* Open the key file */
  file = g_key_file_new();

  (void) g_key_file_load_from_file(file, NHM_CFG_FILE, G_KEY_FILE_NONE, &error);

  if(error == NULL)
  {
    /* Key file could be opened. Load config values */
    max_lc_count    = nhm_main_config_load_uint        (file,
                                                        "node",
                                                        "historic_lc_count",
                                                        0);
    max_failed_apps = nhm_main_config_load_uint        (file,
                                                        "node",
                                                        "max_failed_apps",
                                                        0);
    no_restart_apps = nhm_main_config_load_string_array(file,
                                                        "node",
                                                        "no_restart_apps",
                                                        def_no_restart_apps);
    ul_chk_interval = nhm_main_config_load_uint        (file,
                                                        "userland",
                                                        "ul_chk_interval",
                                                        0);
    monitored_files = nhm_main_config_load_string_array(file,
                                                        "userland",
                                                        "monitored_files",
                                                        def_monitored_files);
    monitored_progs = nhm_main_config_load_string_array(file,
                                                        "userland",
                                                        "monitored_progs",
                                                        def_monitored_progs);
    monitored_procs = nhm_main_config_load_string_array(file,
                                                        "userland",
                                                        "monitored_procs",
                                                        def_monitored_procs);
    monitored_dbus  = nhm_main_config_load_string_array(file,
                                                        "userland",
                                                        "monitored_dbus",
                                                        def_monitored_dbus);
  }
  else
  {
    /* Error. Key file could not be opened. Use default values for settings. */
    max_lc_count    = 0;
    max_failed_apps = 0;
    no_restart_apps = NULL;
    ul_chk_interval = 0;
    monitored_files = NULL;
    monitored_progs = NULL;
    monitored_procs = NULL;

    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to open configuration.");
            DLT_STRING("Error: Loading key file failed.");
            DLT_STRING("File:");    DLT_STRING(NHM_CFG_FILE);
            DLT_STRING("Reason:");  DLT_STRING(error->message));
    g_error_free(error);
  }

  g_key_file_free(file);
}


/**
 * nhm_main_prepare_checks:
 *
 * Create variables to be able to run checks.
 */
static void
nhm_main_prepare_checks(void)
{
  guint32          bus_idx      = 0;
  NhmCheckedDbus  *checked_dbus = NULL;

  if(monitored_dbus != NULL)
  {
    /* Create new array with free function */
    checked_dbusses =
        g_ptr_array_new_with_free_func(&nhm_main_free_checked_dbus);

    for(bus_idx = 0; bus_idx < g_strv_length(monitored_dbus); bus_idx++)
    {
      checked_dbus = g_new(NhmCheckedDbus, 1);
      checked_dbus->bus_addr = g_strdup(monitored_dbus[bus_idx]);
      checked_dbus->bus_conn = NULL;

      g_ptr_array_add(checked_dbusses, (gpointer) checked_dbus);
    }
  }
}

/**
 * nhm_main_connect_to_nsm:
 *
 * The function "connects" the NHM, to the NSM.
 * Therefore, the following things are done:
 *
 *   1. Bus connection to the NSM's bus is obtained
 *   2. "LifecycleControl"   proxy is created
 *   3. "NodeStateConsumer"  proxy is created
 *   4. "LifecycleConsumer"  skeleton is created and exported.
 *   5. The NHM registers as shut down client at the NSM.
 *
 * Return value:  %TRUE:  All points above succeeded. NHM is connected to NSM.
 *                %FALSE: The NHM could not connect to the NSM.
 */
static gboolean
nhm_main_connect_to_nsm(void)
{
  GError           *error      = NULL;
  const gchar      *bus_name   = NULL;
  NsmErrorStatus_e  nsm_retval = NsmErrorStatus_NotSet;
  gboolean          retval     = FALSE;

  /* Step 1: Connect to dbus of the NSM */
  nsmbusconn = g_bus_get_sync((GBusType) NSM_BUS_TYPE, NULL, &error);

  if(error == NULL)
  {
    retval = TRUE;

    /* "Protect" NHM from exit when conn. fails. Important to monitor dbus */
    g_dbus_connection_set_exit_on_close(nsmbusconn, FALSE);
  }
  else
  {
    retval = FALSE;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to connect to NSM.");
            DLT_STRING("Error: Get connection failed.");
            DLT_STRING("Bus type:"); DLT_INT(NSM_BUS_TYPE);
            DLT_STRING("Reason:");   DLT_STRING(error->message));
    g_error_free(error);
  }

  /* Step 2: Create life cycle control proxy to call 'RequestNodeRestart'. */
  if(retval == TRUE)
  {
    dbus_lc_control_obj =
        nsm_dbus_lc_control_proxy_new_sync(nsmbusconn,
                                           G_DBUS_PROXY_FLAGS_NONE,
                                           NSM_BUS_NAME,
                                           NSM_LIFECYCLE_OBJECT,
                                           NULL,
                                           &error);
    if(error == NULL)
    {
      retval = TRUE;
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to connect to NSM.");
              DLT_STRING("Error: Could not create LcControl proxy.");
              DLT_STRING("Reason:"); DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Step 3: Create node state consumer proxy to register as LC client. */
  if(retval == TRUE)
  {
    dbus_consumer_obj =
        nsm_dbus_consumer_proxy_new_sync(nsmbusconn,
                                         G_DBUS_PROXY_FLAGS_NONE,
                                         NSM_BUS_NAME,
                                         NSM_CONSUMER_OBJECT,
                                         NULL,
                                         &error);
    if(error == NULL)
    {
      retval = TRUE;
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to connect to NSM.");
              DLT_STRING("Error: Could not create Consumer proxy.");
              DLT_STRING("Reason:"); DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Step 4: Create and export life cycle client interface. */
  if(retval == TRUE)
  {
    dbus_lc_consumer_obj = nsm_dbus_lc_consumer_skeleton_new();

    (void) g_dbus_interface_skeleton_export(
                                G_DBUS_INTERFACE_SKELETON(dbus_lc_consumer_obj),
                                nsmbusconn,
                                NHM_LC_CLIENT_OBJ,
                                &error);
    if(error == NULL)
    {
      retval = TRUE;

      (void) g_signal_connect(dbus_lc_consumer_obj,
                              "handle-lifecycle-request",
                              G_CALLBACK(nhm_main_lc_request_cb),
                              NULL);
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to connect to NSM.");
              DLT_STRING("Error: Could not export LC consumer object.");
              DLT_STRING("Reason:");  DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Step 5: Register as life cycle client. */
  if(retval == TRUE)
  {
    bus_name = g_dbus_connection_get_unique_name(nsmbusconn);

    (void) nsm_dbus_consumer_call_register_shutdown_client_sync(
                                                        dbus_consumer_obj,
                                                        bus_name,
                                                        NHM_LC_CLIENT_OBJ,
                                                        NSM_SHUTDOWNTYPE_FAST
                                                      | NSM_SHUTDOWNTYPE_NORMAL,
                                                        NHM_LC_CLIENT_TIMEOUT,
                                                        (gint*) &nsm_retval,
                                                        NULL,
                                                        &error);
    if(error == NULL)
    {
      if(nsm_retval == NsmErrorStatus_Ok)
      {
        retval = TRUE;
        DLT_LOG(nhm_helper_trace_ctx,
                DLT_LOG_INFO,
                DLT_STRING("NHM: Successfully connected to NSM"));
      }
      else
      {
        retval = FALSE;
        DLT_LOG(nhm_helper_trace_ctx,
                DLT_LOG_ERROR,
                DLT_STRING("NHM: Failed to connect to NSM.");
                DLT_STRING("Error: Unexpected return from NSM.");
                DLT_STRING("Return:");  DLT_INT(nsm_retval));
      }
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to connect to NSM.");
              DLT_STRING("Error: Could not call NSM client registration.");
              DLT_STRING("Reason:");  DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  return retval;
}


/**
 * nhm_main_free_nhm_objects:
 *
 * Destroy all objects that could have been created during mainloop run.
 */
static void
nhm_main_free_nhm_objects(void)
{
  /* Free the skeleton object (if there was one) */
  if(dbus_nhm_info_obj != NULL)
  {
    g_object_unref(dbus_nhm_info_obj);
    dbus_nhm_info_obj = NULL;
  }

  /* Free the list of currently failed apps */
  g_slist_free_full(current_failed_apps, &nhm_main_free_current_failed_app);
  current_failed_apps = NULL;

  /* Free the array of life cycle info */
  if(nodeinfo != NULL)
  {
    g_ptr_array_unref(nodeinfo);
    nodeinfo = NULL;
  }
}


/**
 * nhm_main_free_nsm_objects:
 *
 * Tries to destroy all objects that could have been created
 * in 'nhm_main_connect_to_nsm'.
 */
static void
nhm_main_free_nsm_objects(void)
{
  /* Free NSM bus connection */
  if(nsmbusconn != NULL)
  {
    g_object_unref(nsmbusconn);
    nsmbusconn = NULL;
  }

  /* Free LifecycleControl proxy */
  if(dbus_lc_control_obj != NULL)
  {
    g_object_unref(dbus_lc_control_obj);
    dbus_lc_control_obj = NULL;
  }

  /* Free NodeStateConsumer proxy */
  if(dbus_consumer_obj != NULL)
  {
    g_object_unref(dbus_consumer_obj);
    dbus_consumer_obj = NULL;
  }

  /* Free LifecycleConsumerSkeleton */
  if(dbus_lc_consumer_obj != NULL)
  {
    g_object_unref(dbus_lc_consumer_obj);
    dbus_lc_consumer_obj = NULL;
  }
}


/**
 * nhm_main_free_config_objects:
 *
 * Destroys all objects that have been created when loading the config.
 */
static void
nhm_main_free_config_objects(void)
{
  /* g_strfreev can hanlde NULL on its own */
  g_strfreev(no_restart_apps);
  no_restart_apps = NULL;

  g_strfreev(monitored_files);
  monitored_files = NULL;

  g_strfreev(monitored_progs);
  monitored_progs = NULL;

  g_strfreev(monitored_procs);
  monitored_procs = NULL;

  g_strfreev(monitored_dbus);
  monitored_dbus = NULL;
}


/**
 * nhm_main_free_check_objects:
 *
 * Destroys all objects that have been created for checks.
 */
static void
nhm_main_free_check_objects(void)
{
  if(checked_dbusses != NULL)
  {
    g_ptr_array_unref(checked_dbusses);
  }
}


/**
 * nhm_main_init:
 *
 * Initializes the file local variables used by the NHM.
 */
static void
nhm_main_init(void)
{
  /* main control */
  mainreturn           = EXIT_FAILURE;
  mainloop             = NULL;

  /* dbus connection */
  nsmbusconn           = NULL;
  dbus_nhm_info_obj    = NULL;
  dbus_lc_consumer_obj = NULL;
  dbus_lc_control_obj  = NULL;
  dbus_consumer_obj    = NULL;

  /* run time data */
  nodeinfo             = NULL;
  current_failed_apps  = NULL;
  checked_dbusses      = NULL;

  /* config stuff */
  max_lc_count         = 0;
  max_failed_apps      = 0;
  no_restart_apps      = NULL;

  ul_chk_interval      = 0;
  monitored_files      = NULL;
  monitored_procs      = NULL;
  monitored_procs      = NULL;
  monitored_dbus       = NULL;
}


/**
 * nhm_main_on_sigterm:
 * @user_data: Optional userdata, configured when signal was registered.
 *
 * Callback when the SIGTERM signal arrives.
 */
static gboolean
nhm_main_on_sigterm(gpointer user_data)
{
  mainreturn = EXIT_SUCCESS;

  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: Received SIGTERM. Going to shut down"));

  g_main_loop_quit(mainloop);

  return TRUE;
}


/******************************************************************************
*
* Interfaces. Exported functions. See Header for detailed description.
*
******************************************************************************/

/**
 *
 * Main function of the executable. Starts the NHM, connects to D-Bus,
 * publishes the interface and waits for calls.
 *
 * @return EXIT_SUCCESS: The executable ended without errors.
 *         EXIT_FAILURE: There was an error.
 */
int
main(void)
{
  int pcl_ret = 0;

  /* Register NHM for DLT */
  DLT_REGISTER_APP("NHM", "Node Health Monitor");
  DLT_REGISTER_CONTEXT(nhm_helper_trace_ctx, "016", "Context for the NHM");

  /* Print first msg. to show that NHM is going to start */
  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: NodeHealthMonitor started."),
          DLT_STRING("Version:"), DLT_STRING(VERSION ));

#if !GLIB_CHECK_VERSION(2,36,0)
    /* Initialize glib for using "g" types. Only necessary until version 2.36 */
    g_type_init();
#endif

  /* Initialize components variables */
  nhm_main_init();

  /* Initialize the PCL */
  pcl_ret = pclInitLibrary("node-health-monitor",
                           PCL_SHUTDOWN_TYPE_FAST | PCL_SHUTDOWN_TYPE_NORMAL);

  if(pcl_ret < 0)
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_WARN,
            DLT_STRING("NHM: PCL could not be initialized.");
            DLT_STRING("Return:"); DLT_INT(pcl_ret));
  }

  /* Load config and prepare checks. Default config used in case of errors */
  nhm_main_load_config();
  nhm_main_prepare_checks();

  /* Connect to NSM, before offering services. Don't start if it doesn't work */
  if(nhm_main_connect_to_nsm() == TRUE)
  {
    mainloop = g_main_loop_new(NULL, FALSE);

    (void) g_bus_own_name((GBusType) NHM_BUS_TYPE,
                          NHM_BUS_NAME,
                          G_BUS_NAME_OWNER_FLAGS_NONE,
                          &nhm_main_bus_acquired_cb,
                          &nhm_main_name_acquired_cb,
                          &nhm_main_name_lost_cb,
                          NULL,
                          NULL);

    /* Add source to catch SIGTERM signal */
    g_unix_signal_add(SIGTERM, &nhm_main_on_sigterm, NULL);

    /* Blocking function, returns in case of an error or if app. shuts down */
    g_main_loop_run(mainloop);

    /* Disconnect from systemd observation */
    nhm_systemd_disconnect();

    /* Free objects created during main loop run */
    nhm_main_free_nhm_objects();

    /* Free resources of the main loop */
    g_main_loop_unref(mainloop);
  }
  else
  {
    mainreturn = EXIT_FAILURE;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to start NodeHealthMonitor.");
            DLT_STRING("Error:"); DLT_STRING("Could not connect to NSM"));
  }

  /* Free objects created by NSM connection */
  nhm_main_free_nsm_objects();

  /* Free objects used by checks */
  nhm_main_free_check_objects();

  /* Free objects created by config. load */
  nhm_main_free_config_objects();

  /* Deinitialize the PCL */
  pcl_ret = pclDeinitLibrary();

  if(pcl_ret < 0)
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_WARN,
            DLT_STRING("NHM: PCL could not be deinitialized."),
            DLT_STRING("Return:"); DLT_INT(pcl_ret));
  }

  DLT_LOG(nhm_helper_trace_ctx,
          DLT_LOG_INFO,
          DLT_STRING("NHM: NodeHealthMonitor stopped."));

  /* Unregister NSM from DLT */
  DLT_UNREGISTER_CONTEXT(nhm_helper_trace_ctx);
  DLT_UNREGISTER_APP();

  return mainreturn;
}
