/* NHM - NodeHealthMonitor
 *
 * Copyright (C) 2013 Continental Automotive Systems, Inc.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public License,
 * v. 2.0. If a copy of the MPL was not distributed with this file, You can
 * obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Author: Jean-Pierre Bogler <Jean-Pierre.Bogler@continental-corporation.com>
 */

/**
 * SECTION:nhm-systemd
 * @title: NodeHealthMonitor (NHM) systemd observation
 * @short_description: Use systemd to check for failed units
 *
 * This section is for the observation of systemd. The code will monitor the
 * "ActiveState" of unit objects on their dbus interface. Based on the
 * transitions of the "ActiveState", failing units can be identified.
 */


/*******************************************************************************
*
* Header includes
*
*******************************************************************************/

/* System header files                 */
#include <stdio.h>       /* NULL       */
#include <gio/gio.h>     /* Use gdbus  */
#include <dlt/dlt.h>     /* DLT traces */

/* Component header files                        */
#include "nhm-systemd.h" /* Own header           */
#include "nhm-helper.h"  /* NHM helper functions */


/*******************************************************************************
*
* Constants, types and defines
*
*******************************************************************************/

#define NHM_SYSTEMD_BUS_NAME "org.freedesktop.systemd1"
#define NHM_SYSTEMD_MNGR_IF  "org.freedesktop.systemd1.Manager"
#define NHM_SYSTEMD_UNIT_IF  "org.freedesktop.systemd1.Unit"
#define NHM_SYSTEMD_PROP_IF  "org.freedesktop.DBus.Properties"
#define NHM_SYSTEMD_OBJ_PATH "/org/freedesktop/systemd1"


/**
 * NhmActiveState:
 * @NHM_ACTIVE_STATE_UNKNOWN:      Init. val. and to handle systemd updates
 * @NHM_ACTIVE_STATE_ACTIVE:       Unit state is active
 * @NHM_ACTIVE_STATE_RELOADING:    Unit state is reloading
 * @NHM_ACTIVE_STATE_INACTIVE:     Unit is inactive
 * @NHM_ACTIVE_STATE_FAILED:       Unit is in a failed state
 * @NHM_ACTIVE_STATE_ACTIVATING:   Unit is currently being activated
 * @NHM_ACTIVE_STATE_DEACTIVATING: Unit is currently being deactivated
 * @NHM_ACTIVE_STATE_LAST:         Last value of enumeration
 *
 * The enumeration is used to convert string values, which systemd delivers for
 * the ActiveState, to numeric values.
 */
typedef enum
{
  NHM_ACTIVE_STATE_UNKNOWN,
  NHM_ACTIVE_STATE_ACTIVE,
  NHM_ACTIVE_STATE_RELOADING,
  NHM_ACTIVE_STATE_INACTIVE,
  NHM_ACTIVE_STATE_FAILED,
  NHM_ACTIVE_STATE_ACTIVATING,
  NHM_ACTIVE_STATE_DEACTIVATING,
  NHM_ACTIVE_STATE_LAST
} NhmActiveState;


/**
 * NhmSystemdUnit:
 * @name:         Name of the unit
 * @path:         Path to find unit on dbus
 * @active_state: Active state of the unit
 * @sig_sub_id:   Subscription ID for PropertiesChanged signal
 *
 * The structure is used to create a list of observed units.
 */
typedef struct
{
  gchar          *name;
  gchar          *path;
  NhmActiveState  active_state;
  guint           sig_sub_id;
} NhmSystemdUnit;


/**
 * NhmSystemdAppStatusChange:
 * @do_callback: Defines if 'app_status' should be send
 * @next_status: Status to which app changed.
 *
 * The structure is used in 'nhm_systemd_active_state_map' to define the new
 * app_status after a transition and whether a callback should be made.
 */
typedef struct
{
  gboolean       do_callback;
  NhmAppStatus_e next_status;
} NhmSystemdAppStatusChange;


/* Array defines new 'app_status' after a transition of the 'active_state' */
static const
NhmSystemdAppStatusChange
nhm_systemd_active_state_map[NHM_ACTIVE_STATE_LAST][NHM_ACTIVE_STATE_LAST] =
{
  /* NHM_ACTIVE_STATE_UNKNOWN -> New state                                 */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { TRUE,  NhmAppStatus_Ok         },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { TRUE,  NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { TRUE,  NhmAppStatus_Failed     }},

  /* NHM_ACTIVE_STATE_ACTIVE -> New state                                  */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { TRUE,  NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { TRUE,  NhmAppStatus_Failed     }},

  /* NHM_ACTIVE_STATE_RELOADING -> New state                               */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { TRUE,  NhmAppStatus_Ok         },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { TRUE,  NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { TRUE,  NhmAppStatus_Failed     }},

  /* NHM_ACTIVE_STATE_INACTIVE -> New state                                */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { TRUE,  NhmAppStatus_Ok         },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { TRUE,  NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { TRUE,  NhmAppStatus_Failed     }},

  /* NHM_ACTIVE_STATE_FAILED -> New state                                  */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { TRUE,  NhmAppStatus_Ok         },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { TRUE,  NhmAppStatus_Restarting },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { FALSE, NhmAppStatus_Failed     }},

  /* NHM_ACTIVE_STATE_ACTIVATING -> New state                              */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { TRUE,  NhmAppStatus_Ok         },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { TRUE,  NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { TRUE,  NhmAppStatus_Failed     }},

  /* NHM_ACTIVE_STATE_DEACTIVATING -> New state                            */
  /* NHM_ACTIVE_STATE_UNKNOWN         */ {{ FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVE          */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_RELOADING       */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_INACTIVE        */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_FAILED          */  { TRUE,  NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_ACTIVATING      */  { FALSE, NhmAppStatus_Failed     },
  /* NHM_ACTIVE_STATE_DEACTIVATING    */  { FALSE, NhmAppStatus_Failed     }}
};


/*******************************************************************************
*
* Prototypes for file local functions (see implementation for description)
*
*******************************************************************************/

/* Helper functions */
static NhmActiveState nhm_systemd_active_state_string_to_enum  (const gchar          *string);
static void           nhm_systemd_unit_active_state_changed    (NhmSystemdUnit       *unit,
                                                                NhmActiveState        new_state);
static gint           nhm_systemd_find_unit_by_name            (gconstpointer         u1,
                                                                gconstpointer         u2);
static void           nhm_systemd_free_unit                    (gpointer              unit);

static NhmActiveState nhm_systemd_unit_get_active_state        (NhmSystemdUnit       *unit);

/* Signal registration functions */
static guint          nhm_systemd_subscribe_properties_changed (const NhmSystemdUnit *unit);

/* Signal handler functions */
static void           nhm_systemd_unit_active_state_changed    (NhmSystemdUnit       *unit,
                                                                NhmActiveState        new_state);
static void           nhm_systemd_unit_properties_changed      (GDBusConnection      *connection,
                                                                const gchar          *sender_name,
                                                                const gchar          *object_path,
                                                                const gchar          *interface_name,
                                                                const gchar          *signal_name,
                                                                GVariant             *parameters,
                                                                gpointer              user_data);
static void           nhm_systemd_unit_added                   (GDBusConnection      *connection,
                                                                const gchar          *sender_name,
                                                                const gchar          *object_path,
                                                                const gchar          *interface_name,
                                                                const gchar          *signal_name,
                                                                GVariant             *parameters,
                                                                gpointer              user_data);
static void           nhm_systemd_unit_removed                 (GDBusConnection      *connection,
                                                                const gchar          *sender_name,
                                                                const gchar          *object_path,
                                                                const gchar          *interface_name,
                                                                const gchar          *signal_name,
                                                                GVariant             *parameters,
                                                                gpointer              user_data);
static void           nhm_systemd_unit_properties_changed      (GDBusConnection      *connection,
                                                                const gchar          *sender_name,
                                                                const gchar          *object_path,
                                                                const gchar          *interface_name,
                                                                const gchar          *signal_name,
                                                                GVariant             *parameters,
                                                                gpointer              user_data);


/*******************************************************************************
*
* Local variables and constants
*
*******************************************************************************/

/* Bus connection and callback set on observation start */
static NhmSystemdAppStatusCb  nhm_systemd_app_status_cb   = NULL;
static GDBusConnection       *nhm_systemd_conn            = NULL;

/* List of units known by us (based on NhmSystemdUnit) */
static GSList                *nhm_systemd_observed_units  = NULL;

/* Signals registered at systemd */
static guint                  nhm_systemd_unit_add_sig_id   = 0;
static guint                  nhm_systemd_unit_rem_sig_id   = 0;
static gboolean               nhm_systemd_events_subscribed = FALSE;


/*******************************************************************************
*
* Local (static) functions
*
*******************************************************************************/


/**
 * nhm_systemd_active_state_string_to_enum:
 * @string: String representing 'active_state' of a systemd unit
 * @return: Enumerated equivalent for 'active_state' string.
 *
 * The function converts the passed 'active_state' string to the equivalent
 * enumerated value. If the string isn't a known ActiveState,
 * 'NHM_ACTIVE_STATE_UNKNOWN' is returned.
 */
static NhmActiveState
nhm_systemd_active_state_string_to_enum(const gchar *string)
{
  NhmActiveState state = NHM_ACTIVE_STATE_UNKNOWN;

  if(g_strcmp0(string, "active") == 0)
  {
    state = NHM_ACTIVE_STATE_ACTIVE;
  }
  else if(g_strcmp0(string, "inactive") == 0)
  {
    state = NHM_ACTIVE_STATE_INACTIVE;
  }
  else if(g_strcmp0(string, "activating") == 0)
  {
    state = NHM_ACTIVE_STATE_ACTIVATING;
  }
  else if(g_strcmp0(string, "deactivating") == 0)
  {
    state = NHM_ACTIVE_STATE_DEACTIVATING;
  }
  else if(g_strcmp0(string, "failed") == 0)
  {
    state = NHM_ACTIVE_STATE_FAILED;
  }
  else if(g_strcmp0(string, "reloading") == 0)
  {
    state = NHM_ACTIVE_STATE_RELOADING;
  }
  else
  {
    state = NHM_ACTIVE_STATE_UNKNOWN;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to convert 'ActiveState'.");
            DLT_STRING("Error: Unknown string.");
            DLT_STRING("String:"); DLT_STRING(string));
  }

  return state;
}


/**
 * nhm_systemd_subscribe_properties_changed:
 * @unit:   Unit for which properties changed should be processed
 * @return: Subscription ID. Necessary to unsubscribe.
 *
 * The function registers for the PropertiesChanged signal of the passed unit.
 */
static guint
nhm_systemd_subscribe_properties_changed(const NhmSystemdUnit *unit)
{
  guint rval = 0;

  rval =
      g_dbus_connection_signal_subscribe(nhm_systemd_conn,
                                         NHM_SYSTEMD_BUS_NAME,
                                         NHM_SYSTEMD_PROP_IF,
                                         "PropertiesChanged",
                                         unit->path,
                                         NULL,
                                         G_DBUS_SIGNAL_FLAGS_NONE,
                                         &nhm_systemd_unit_properties_changed,
                                         (gpointer) unit,
                                         NULL);
  return rval;
}


/**
 * nhm_systemd_find_unit_by_name:
 * @u1:     Pointer to unit item from list.
 * @u2:     Pointer to searched unit item.
 * @return: 0, if unit names equal.
 *
 * The function is used by 'custom finds' to find a unit based on it name.
 */
static gint
nhm_systemd_find_unit_by_name(gconstpointer u1, gconstpointer u2)
{
  return g_strcmp0(((NhmSystemdUnit*) u1)->name,
                   ((NhmSystemdUnit*) u2)->name);
}


/**
 * nhm_systemd_free_unit:
 * @unit: Pointer to unit item.
 *
 * The function frees the memory occupied by a unit object and its members.
 * It also unregisters for the PropertiesChnaged signal of the unit.
 */
static void
nhm_systemd_free_unit(gpointer unit)
{
  NhmSystemdUnit *u = (NhmSystemdUnit*) unit;

  g_free(u->name);
  g_free(u->path);

  g_dbus_connection_signal_unsubscribe(nhm_systemd_conn, u->sig_sub_id);

  g_free(u);
}


/**
 * nhm_systemd_unit_active_state_changed:
 * @unit:      Pointer to unit item, whose 'ActiveState' changed.
 * @new_state: New 'ActiveState' of the unit.
 *
 * The function is called when the 'ActiveState' property of a unit changed.
 */
static void
nhm_systemd_unit_active_state_changed(NhmSystemdUnit *unit,
                                      NhmActiveState  new_state)
{
  const NhmSystemdAppStatusChange *status_change = NULL;

  status_change = &nhm_systemd_active_state_map[unit->active_state][new_state];

  if(status_change->do_callback == TRUE)
  {
    nhm_systemd_app_status_cb(unit->name, status_change->next_status);
  }

  unit->active_state = new_state;
}


/**
 * nhm_systemd_unit_get_active_state:
 * @unit:   Pointer to unit item, whose 'ActiveState' property should be read.
 * @return: ActiveState read from unit properties and converted to the
 *          equivalent enumeration.
 *
 * The function is called to retrieve value of the 'ActiveState' property of a
 * unit.
 */
static NhmActiveState
nhm_systemd_unit_get_active_state(NhmSystemdUnit *unit)
{
  GError         *error   = NULL;
  GVariant       *propval = NULL;
  const gchar    *state   = NULL;
  NhmActiveState  retval  = NHM_ACTIVE_STATE_UNKNOWN;

  propval = g_dbus_connection_call_sync(nhm_systemd_conn,
                                        NHM_SYSTEMD_BUS_NAME,
                                        unit->path,
                                        NHM_SYSTEMD_PROP_IF,
                                        "Get",
                                        g_variant_new("(ss)",
                                                      NHM_SYSTEMD_UNIT_IF,
                                                      "ActiveState"),
                                        (GVariantType*) "(v)",
                                        G_DBUS_CALL_FLAGS_NONE,
                                        -1,
                                        NULL,
                                        &error);

  if(error == NULL)
  {
    g_variant_get_child(propval, 0, "&s", &state);
    retval = nhm_systemd_active_state_string_to_enum(state);
    g_variant_unref(propval);
  }
  else
  {
    retval = NHM_ACTIVE_STATE_UNKNOWN;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to get unit property 'ActiveState'.");
            DLT_STRING("Error: D-Bus communication failed.");
            DLT_STRING("Reason:"); DLT_STRING(error->message));
    g_error_free(error);
  }

  return retval;
}


/**
 * nhm_systemd_unit_added:
 * @connection:     Connection on which signal occurred.
 * @sender_name:    (Unique) Sender name of signal.
 * @object_path:    Object that send the signal.
 * @interface_name: Interface from where signal comes from.
 * @signal_name:    Name of the signal.
 * @parameters:     Parameters of dbus signal.
 * @user_data:      Optional user data (not used)
 *
 * Called when the "UnitAdded" signal from org.freedesktop.systemd1
 * arrives. The new unit will be added to the internal unit list and
 * its initial state will be retrieved.
 */
static void
nhm_systemd_unit_added(GDBusConnection *connection,
                       const gchar     *sender_name,
                       const gchar     *object_path,
                       const gchar     *interface_name,
                       const gchar     *signal_name,
                       GVariant        *parameters,
                       gpointer         user_data)
{
  NhmSystemdUnit *unit       = NULL;
  const gchar    *param_type = NULL;
  NhmSystemdUnit  search_unit;
  GSList         *list_item  = NULL;

  param_type = g_variant_get_type_string(parameters);

  if(g_strcmp0(param_type, "(so)") == 0)
  {
    g_variant_get_child(parameters, 0, "&s", &search_unit.name);

    if(g_str_has_suffix(search_unit.name, ".service") == TRUE)
    {
      list_item = g_slist_find_custom(nhm_systemd_observed_units,
                                      &search_unit,
                                      &nhm_systemd_find_unit_by_name);
      if(list_item == NULL)
      {
        unit = g_new(NhmSystemdUnit, 1);
        unit->name = g_strdup(search_unit.name);

        g_variant_get_child(parameters, 1, "o", &unit->path);

        unit->active_state = NHM_ACTIVE_STATE_UNKNOWN;
        unit->sig_sub_id   = nhm_systemd_subscribe_properties_changed(unit);

        nhm_systemd_observed_units = g_slist_prepend(nhm_systemd_observed_units,
                                                     unit);

        DLT_LOG(nhm_helper_trace_ctx,
                DLT_LOG_INFO,
                DLT_STRING("NHM: Systemd unit added.");
                DLT_STRING("Name:");   DLT_STRING(unit->name));
      }
    }
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to process 'UnitAdded' signal.");
            DLT_STRING("Error: Invalid parameter type.");
            DLT_STRING("Type:"); DLT_STRING(param_type));
  }
}


/**
 * nhm_systemd_unit_removed:
 * @connection:     Connection on which signal occurred.
 * @sender_name:    (Unique) Sender name of signal.
 * @object_path:    Object that send the signal.
 * @interface_name: Interface from where signal comes from.
 * @signal_name:    Name of the signal.
 * @parameters:     Parameters of dbus signal.
 * @user_data:      Optional user data (not used)
 *
 * Called when the "UnitRemoved" signal from org.freedesktop.systemd1
 * arrives. The unit will be removed from the internal unit list.
 */
static void
nhm_systemd_unit_removed(GDBusConnection *connection,
                         const gchar     *sender_name,
                         const gchar     *object_path,
                         const gchar     *interface_name,
                         const gchar     *signal_name,
                         GVariant        *parameters,
                         gpointer         user_data)
{
  GSList         *list_item  = NULL;
  const gchar    *param_type = NULL;
  NhmSystemdUnit  search_unit;

  param_type = g_variant_get_type_string(parameters);

  if(g_strcmp0(param_type, "(so)") == 0)
  {
    g_variant_get_child(parameters, 0, "&s", &search_unit.name);

    if(g_str_has_suffix(search_unit.name, ".service") == TRUE)
    {
      list_item = g_slist_find_custom(nhm_systemd_observed_units,
                                      &search_unit,
                                      &nhm_systemd_find_unit_by_name);
      if(list_item != NULL)
      {
        DLT_LOG(nhm_helper_trace_ctx,
                DLT_LOG_INFO,
                DLT_STRING("NHM: Systemd unit removed.");
                DLT_STRING("Name:");   DLT_STRING(search_unit.name));

        nhm_systemd_free_unit(list_item->data);
        nhm_systemd_observed_units = g_slist_remove(nhm_systemd_observed_units,
                                                    list_item->data);
      }
    }
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to process 'UnitRemoved' signal.");
            DLT_STRING("Error: Invalid parameter type.");
            DLT_STRING("Type:"); DLT_STRING(param_type));
  }
}


/**
 * nhm_systemd_unit_properties_changed:
 * @connection:     Connection on which signal occurred.
 * @sender_name:    (Unique) Sender name of signal.
 * @object_path:    Object that send the signal.
 * @interface_name: Interface from where signal comes from.
 * @signal_name:    Name of the signal.
 * @parameters:     Parameters of dbus signal.
 * @user_data:      User data. Pointer to unit for which properties changed
 *                  signal had been registered.
 *
 * Called when the "PropertiesChanged" signal from the "Properties" interface
 * of a systemd unit arrives. Depending on the changed property, the change
 * will be processed further in the related sub functions.
 */
static void
nhm_systemd_unit_properties_changed(GDBusConnection *connection,
                                    const gchar     *sender_name,
                                    const gchar     *object_path,
                                    const gchar     *interface_name,
                                    const gchar     *signal_name,
                                    GVariant        *parameters,
                                    gpointer         user_data)
{
  NhmSystemdUnit  *unit         = (NhmSystemdUnit*) user_data;
  const gchar     *inv_props    = NULL;
  NhmActiveState   active_state = NHM_ACTIVE_STATE_UNKNOWN;
  const gchar     *param_type   = NULL;

  param_type = g_variant_get_type_string(parameters);

  if(g_strcmp0(param_type, "(sa{sv}as)") == 0)
  {
    GVariant *value;
    GVariantIter iter;
    GVariant *entry;

    value = g_variant_get_child_value(parameters, 1);
    g_variant_iter_init(&iter, value);

    while((entry = g_variant_iter_next_value(&iter)))
    {
      GVariant *entry_key;

      entry_key = g_variant_get_child_value(entry, 0);
      if (strcmp(g_variant_get_string(entry_key, NULL), "ActiveState") == 0)
      {
        /* found ActiveState key */
        g_variant_unref(entry_key);
        break;
      }
      g_variant_unref(entry_key);
      g_variant_unref(entry);
    }
    g_variant_unref(value);

    if (entry == NULL)
    {
      return; /* not matched ActiveState on 'PropertiesChanged' signal. */
    }

    value = g_variant_get_child_value(entry, 1);
    g_variant_unref(entry);

    if (g_variant_is_of_type(value, G_VARIANT_TYPE_VARIANT))
    {
      GVariant *tmp;

      tmp = g_variant_get_variant(value);
      g_variant_unref(value);

      if (!g_variant_is_of_type(tmp, G_VARIANT_TYPE("s")))
      {
        g_variant_unref(tmp);
        tmp = NULL;
      }

      value = tmp;
    }

    if(value != NULL)
    {
      inv_props = g_variant_get_string(value, NULL);
      g_variant_unref(value);

      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_INFO,
              DLT_STRING("NHM: ActiveState:"); DLT_STRING(inv_props);
              DLT_STRING(", Unit:"); DLT_STRING(unit->name));

      active_state = nhm_systemd_active_state_string_to_enum(inv_props);

      if(active_state != unit->active_state)
      {
        nhm_systemd_unit_active_state_changed(unit, active_state);
      }
    }
    else
    {
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: not matched VariantTypeString on ActiveState value."));
    }
  }
  else
  {
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to process 'PropertiesChanged' signal.");
            DLT_STRING("Error: Invalid parameter type.");
            DLT_STRING("Type:"); DLT_STRING(param_type));
  }
}


/*******************************************************************************
*
* Interfaces. Exported functions. See Header for detailed description.
*
*******************************************************************************/

/**
 * nhm_systemd_connect:
 * @app_status_cb: Callback that should be called, if the "ActiveState" of
 *                 a systemd Service changes between valid and invalid states.
 *
 * The NHM main process can start the systemd observation whith this function.
 */
gboolean
nhm_systemd_connect(NhmSystemdAppStatusCb app_status_cb)
{
  GVariantIter    iter;
  GError         *error          = NULL;
  GVariant       *manager_return = NULL;
  GVariant       *unit_array     = NULL;
  GVariant       *unit           = NULL;
  gboolean        retval         = FALSE;
  gchar          *unit_name      = NULL;
  const gchar    *active_state   = NULL;
  NhmSystemdUnit *new_unit       = NULL;

  /* Initialize local variables */
  nhm_systemd_app_status_cb     = NULL;
  nhm_systemd_conn              = NULL;
  nhm_systemd_observed_units    = NULL;
  nhm_systemd_events_subscribed = FALSE;
  nhm_systemd_unit_add_sig_id   = 0;
  nhm_systemd_unit_rem_sig_id   = 0;

  /* Step 1: Save function to call if app status changes. */
  if(app_status_cb != NULL)
  {
    retval = TRUE;
    nhm_systemd_app_status_cb = app_status_cb;
  }
  else
  {
    retval = FALSE;
    DLT_LOG(nhm_helper_trace_ctx,
            DLT_LOG_ERROR,
            DLT_STRING("NHM: Failed to connect to systemd dbus.");
            DLT_STRING("Error: Invalid callback passed."));
  }

  /* Step 2: Connect to the system bus */
  if(retval == TRUE)
  {
    nhm_systemd_conn = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);

    if(error == NULL)
    {
      retval = TRUE;
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to connect to systemd dbus.");
              DLT_STRING("Error: D-Bus connection failed.");
              DLT_STRING("Reason:"); DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Step 3: Register for signals, if units are removed/added */
  if(retval == TRUE)
  {
    nhm_systemd_unit_add_sig_id =
        g_dbus_connection_signal_subscribe(nhm_systemd_conn,
                                           NHM_SYSTEMD_BUS_NAME,
                                           NHM_SYSTEMD_MNGR_IF,
                                           "UnitNew",
                                           NHM_SYSTEMD_OBJ_PATH,
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           &nhm_systemd_unit_added,
                                           NULL,
                                           NULL);

    nhm_systemd_unit_rem_sig_id =
        g_dbus_connection_signal_subscribe(nhm_systemd_conn,
                                           NHM_SYSTEMD_BUS_NAME,
                                           NHM_SYSTEMD_MNGR_IF,
                                           "UnitRemoved",
                                           NHM_SYSTEMD_OBJ_PATH,
                                           NULL,
                                           G_DBUS_SIGNAL_FLAGS_NONE,
                                           &nhm_systemd_unit_removed,
                                           NULL,
                                           NULL);
  }

  /* Step 4: Subscribe. Without, PropertiesChanged isn't send */
  if(retval == TRUE)
  {
    manager_return = g_dbus_connection_call_sync(nhm_systemd_conn,
                                                 NHM_SYSTEMD_BUS_NAME,
                                                 NHM_SYSTEMD_OBJ_PATH,
                                                 NHM_SYSTEMD_MNGR_IF,
                                                 "Subscribe",
                                                 NULL,
                                                 NULL,
                                                 G_DBUS_CALL_FLAGS_NONE,
                                                 -1,
                                                 NULL,
                                                 &error);
    if(error == NULL)
    {
      retval = TRUE;
      nhm_systemd_events_subscribed = TRUE;
      g_variant_unref(manager_return);
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to subscribe to systemd signals.");
              DLT_STRING("Error: D-Bus connection failed.");
              DLT_STRING("Reason:"); DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Step 5: Retrieve all currently known service units from systemd. */
  if(retval == TRUE)
  {
    manager_return =
        g_dbus_connection_call_sync(nhm_systemd_conn,
                                    NHM_SYSTEMD_BUS_NAME,
                                    NHM_SYSTEMD_OBJ_PATH,
                                    NHM_SYSTEMD_MNGR_IF,
                                    "ListUnits",
                                    NULL,
                                    (GVariantType*) "(a(ssssssouso))",
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);

    if(error == NULL)
    {
      retval = TRUE;

      unit_array = g_variant_get_child_value(manager_return, 0);
      g_variant_iter_init (&iter, unit_array);

      while((unit = g_variant_iter_next_value(&iter)))
      {
        /* Return for a unit is of type '(ssssssouso)' with member #:
         * 0: Unit name. 3: Active state. 6: Object path */
        g_variant_get_child(unit, 0, "s", &unit_name);

        if(g_str_has_suffix(unit_name, ".service") == TRUE)
        {
          new_unit = g_new(NhmSystemdUnit, 1);
          new_unit->name = unit_name;

          g_variant_get_child(unit, 3, "&s", &active_state);

          new_unit->active_state =
              nhm_systemd_active_state_string_to_enum(active_state);

          g_variant_get_child(unit, 6, "o", &new_unit->path);

          new_unit->sig_sub_id =
              nhm_systemd_subscribe_properties_changed(new_unit);

          nhm_systemd_observed_units =
              g_slist_append(nhm_systemd_observed_units, new_unit);
        }
        else
        {
          g_free(unit_name);
        }
      }

      g_variant_unref(unit_array);
      g_variant_unref(manager_return);
    }
    else
    {
      retval = FALSE;
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to retrieve unit list from systemd.");
              DLT_STRING("Error: D-Bus communication failed.");
              DLT_STRING("Reason:"); DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Step 6: If there was a problem, destroy what has been created */
  if(retval == FALSE)
  {
    nhm_systemd_disconnect();
  }

  return retval;
}


/**
 * nhm_systemd_disconnect:
 *
 * The NHM main process should call this function, when systemd observation is
 * no longer needed.
 */
void
nhm_systemd_disconnect(void)
{
  GVariant *manager_return = NULL;
  GError   *error          = NULL;

  /* Reset callback */
  nhm_systemd_app_status_cb = NULL;

  /* Unsubscribe from systemd signals */
  if(nhm_systemd_events_subscribed == TRUE)
  {
    manager_return =
        g_dbus_connection_call_sync(nhm_systemd_conn,
                                    NHM_SYSTEMD_BUS_NAME,
                                    NHM_SYSTEMD_OBJ_PATH,
                                    NHM_SYSTEMD_MNGR_IF,
                                    "Unsubscribe",
                                    NULL,
                                    NULL,
                                    G_DBUS_CALL_FLAGS_NONE,
                                    -1,
                                    NULL,
                                    &error);
    if(error == NULL)
    {
      nhm_systemd_events_subscribed = FALSE;
      g_variant_unref(manager_return);
    }
    else
    {
      DLT_LOG(nhm_helper_trace_ctx,
              DLT_LOG_ERROR,
              DLT_STRING("NHM: Failed to Unsubscribe from systemd.");
              DLT_STRING("Error: D-Bus communication failed.");
              DLT_STRING("Reason:"); DLT_STRING(error->message));
      g_error_free(error);
    }
  }

  /* Unregister unit add and remove signals */
  if(nhm_systemd_unit_add_sig_id != 0)
  {
    g_dbus_connection_signal_unsubscribe(nhm_systemd_conn,
                                         nhm_systemd_unit_add_sig_id);
    nhm_systemd_unit_add_sig_id = 0;
  }

  if(nhm_systemd_unit_rem_sig_id != 0)
  {
    g_dbus_connection_signal_unsubscribe(nhm_systemd_conn,
                                         nhm_systemd_unit_rem_sig_id);
    nhm_systemd_unit_rem_sig_id = 0;
  }

  /* Destroy list of units (incl. unregister prop. changed) */
  if(nhm_systemd_observed_units != NULL)
  {
    g_slist_free_full(nhm_systemd_observed_units, &nhm_systemd_free_unit);
    nhm_systemd_observed_units = NULL;
  }

  if(nhm_systemd_conn != NULL)
  {
    g_object_unref(nhm_systemd_conn);
    nhm_systemd_conn = NULL;
  }
}
