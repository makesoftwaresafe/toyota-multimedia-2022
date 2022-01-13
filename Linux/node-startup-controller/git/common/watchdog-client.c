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

#include <systemd/sd-daemon.h>

#include <glib-object.h>
#include <gio/gio.h>

#include <common/watchdog-client.h>



/**
 * SECTION: watchdog-client
 * @title: WatchdogClient
 * @short_description: Notifies the systemd watchdog in regular intervals.
 * @stability: Internal
 * 
 * The #WatchdogClient notifies systemd's watchdog in a regular interval that
 * is specified upon construction. If the unit file associated with the
 * application has %WatchdogSec set then systemd will restart the application
 * if it does not update the watchdog timestamp in this interval (e.g. if it
 * has crashed or is stuck in an infinite loop).
 *
 * In order to avoid problems with delays it is recommended to notify the
 * systemd watchdog twice in the %WatchdogSec interval, so usually the
 * value passed to #watchdog_client_new will be half of %WatchdogSec.
 */



/* property identifiers */
enum
{
  PROP_0,
  PROP_TIMEOUT,
};



static void     watchdog_client_constructed  (GObject      *object);
static void     watchdog_client_finalize     (GObject      *object);
static void     watchdog_client_get_property (GObject      *object,
                                              guint         prop_id,
                                              GValue       *value,
                                              GParamSpec   *pspec);
static void     watchdog_client_set_property (GObject      *object,
                                              guint         prop_id,
                                              const GValue *value,
                                              GParamSpec   *pspec);
static gboolean watchdog_client_timeout      (gpointer      user_data);



struct _WatchdogClientClass
{
  GObjectClass __parent__;
};

struct _WatchdogClient
{
  GObject          __parent__;

  guint            timeout;
  guint            timeout_id;
};



G_DEFINE_TYPE (WatchdogClient, watchdog_client, G_TYPE_OBJECT);



static void
watchdog_client_class_init (WatchdogClientClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->constructed = watchdog_client_constructed;
  gobject_class->finalize = watchdog_client_finalize;
  gobject_class->get_property = watchdog_client_get_property;
  gobject_class->set_property = watchdog_client_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_TIMEOUT,
                                   g_param_spec_uint ("timeout",
                                                      "timeout",
                                                      "timeout",
                                                      0, G_MAXUINT, 120,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));
}



static void
watchdog_client_init (WatchdogClient *client)
{
}



static void
watchdog_client_constructed (GObject *object)
{
  WatchdogClient *client = WATCHDOG_CLIENT (object);

  /* trigger a systemd watchdog timestap update now */
  watchdog_client_timeout (client);

  /* schedule a regular timeout to update the systemd watchdog timestamp */
  client->timeout_id = g_timeout_add_seconds_full (G_PRIORITY_DEFAULT,
                                                   client->timeout,
                                                   watchdog_client_timeout,
                                                   g_object_ref (client),
                                                   (GDestroyNotify) g_object_unref);
}



static void
watchdog_client_finalize (GObject *object)
{
  WatchdogClient *client = WATCHDOG_CLIENT (object);

  /* drop the watchdog timeout */
  if (client->timeout_id > 0)
    g_source_remove (client->timeout_id);

  (*G_OBJECT_CLASS (watchdog_client_parent_class)->finalize) (object);
}



static void
watchdog_client_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  WatchdogClient *client = WATCHDOG_CLIENT (object);

  switch (prop_id)
    {
    case PROP_TIMEOUT:
      g_value_set_uint (value, client->timeout);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
watchdog_client_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  WatchdogClient *client = WATCHDOG_CLIENT (object);

  switch (prop_id)
    {
    case PROP_TIMEOUT:
      client->timeout = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static gboolean
watchdog_client_timeout (gpointer user_data)
{
  sd_notify (0, "WATCHDOG=1");
  return TRUE;
}



/**
 * watchdog_client_new:
 * @timeout: The amount of time to wait in between notifications to systemd's watchdog.
 * 
 * Creates a new watchdog and starts notifying systemd's watchdog every @timeout seconds.
 * 
 * Returns: A new instance of #WatchdogClient.
 */
WatchdogClient *
watchdog_client_new (guint timeout)
{
  return g_object_new (TYPE_WATCHDOG_CLIENT, "timeout", timeout, NULL);
}
