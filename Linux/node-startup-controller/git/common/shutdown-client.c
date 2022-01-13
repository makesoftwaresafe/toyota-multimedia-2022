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

#include <common/nsm-enum-types.h>
#include <common/shutdown-client.h>
#include <common/shutdown-consumer-dbus.h>



/**
 * SECTION: shutdown-client
 * @title: ShutdownClient
 * @short_description: A class to bundle information about a #ShutdownConsumer.
 * @stability: Internal
 * 
 * The #ShutdownClient is a container for a #ShutdownConsumerSkeleton if it is being used
 * by the #LAHandlerService in the Node Startup Controller, and a #ShutdownConsumerProxy 
 * if it is being used by the #NSMConsumerService in the Node State Manager Dummy.
 * 
 * In addition, it contains the following information:
 *
 * * The bus name, the name that the controlling process owns on the system bus.
 *
 * * The object path, a unique D-Bus object path which, together with the bus name,
 *   is used by the Node State Manager to identify the #ShutdownConsumer.
 *
 * * The shutdown mode, an #NSMShutdownType.
 *
 * * The timeout, the amount of time the Node State Manager will wait before deciding
 *                that this #ShutdownClient is not responding.
 *
 * The #LAHandlerService uses it to identify which #ShutdownConsumer was told to shut
 * down, which it will then use to stop the associated systemd unit.
 *
 * The #NSMConsumerService uses it to locate the #ShutdownConsumer, shut it down with the
 * appropriate mode, and wait for an appropriate length of time when expecting a response.
 */



/* property identifiers */
enum
{
  PROP_0,
  PROP_BUS_NAME,
  PROP_OBJECT_PATH,
  PROP_SHUTDOWN_MODE,
  PROP_TIMEOUT,
  PROP_CONSUMER,
};



static void shutdown_client_finalize     (GObject      *object);
static void shutdown_client_get_property (GObject      *object,
                                          guint         prop_id,
                                          GValue       *value,
                                          GParamSpec   *pspec);
static void shutdown_client_set_property (GObject      *object,
                                          guint         prop_id,
                                          const GValue *value,
                                          GParamSpec   *pspec);



struct _ShutdownClientClass
{
  GObjectClass __parent__;
};

struct _ShutdownClient
{
  GObject           __parent__;

  gchar            *bus_name;
  gchar            *object_path;
  NSMShutdownType   shutdown_mode;
  guint             timeout;

  ShutdownConsumer *consumer;
};



G_DEFINE_TYPE (ShutdownClient, shutdown_client, G_TYPE_OBJECT);



static void
shutdown_client_class_init (ShutdownClientClass *klass)
{
  GObjectClass *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = shutdown_client_finalize;
  gobject_class->get_property = shutdown_client_get_property;
  gobject_class->set_property = shutdown_client_set_property;

  /**
   * ShutdownClient:bus-name:
   * 
   * The bus name which this shutdown consumer is registered with.
   * e.g. org.genivi.NodeStartupController1.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_BUS_NAME,
                                   g_param_spec_string ("bus-name",
                                                        "bus-name",
                                                        "bus-name",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * ShutdownClient:object-path:
   * 
   * The D-Bus object path this shutdown consumer uses.
   * e.g. /org/genivi/NodeStartupController1/ShutdownConsumer/1
   */
  g_object_class_install_property (gobject_class,
                                   PROP_OBJECT_PATH,
                                   g_param_spec_string ("object-path",
                                                        "object-path",
                                                        "object-path",
                                                        NULL,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_CONSTRUCT_ONLY |
                                                        G_PARAM_STATIC_STRINGS));

  /**
   * ShutdownClient:shutdown-mode:
   * 
   * The shutdown mode of this shutdown client.
   * For a full list of shutdown modes, see #NSMShutdownType.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_SHUTDOWN_MODE,
                                   g_param_spec_flags ("shutdown-mode",
                                                       "shutdown-mode",
                                                       "shutdown-mode",
                                                       TYPE_NSM_SHUTDOWN_TYPE,
                                                       NSM_SHUTDOWN_TYPE_NOT,
                                                       G_PARAM_READWRITE |
                                                       G_PARAM_CONSTRUCT_ONLY |
                                                       G_PARAM_STATIC_STRINGS));

  /**
   * ShutdownClient:timeout:
   * 
   * The amount of time the Node State Manager will wait before deciding that the
   * shutdown client is not responding.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_TIMEOUT,
                                   g_param_spec_uint ("timeout",
                                                      "timeout",
                                                      "timeout",
                                                      0, G_MAXUINT, 120,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY |
                                                      G_PARAM_STATIC_STRINGS));

  /**
   * ShutdownClient:consumer:
   * 
   * The #ShutdownConsumerSkeleton or #ShutdownConsumerProxy for this shutdown client.
   */
  g_object_class_install_property (gobject_class,
                                   PROP_CONSUMER,
                                   g_param_spec_object ("consumer",
                                                        "consumer",
                                                        "consumer",
                                                        TYPE_SHUTDOWN_CONSUMER,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS));

}



static void
shutdown_client_init (ShutdownClient *client)
{
}



static void
shutdown_client_finalize (GObject *object)
{
  ShutdownClient *client = SHUTDOWN_CLIENT (object);

  /* free bus name and object path */
  g_free (client->bus_name);
  g_free (client->object_path);

  /* release the consumer, if we have one */
  if (client->consumer != NULL)
    {
      if (IS_SHUTDOWN_CONSUMER_SKELETON (client->consumer))
        g_dbus_interface_skeleton_unexport (G_DBUS_INTERFACE_SKELETON (client->consumer));
      g_object_unref (client->consumer);
    }

  (*G_OBJECT_CLASS (shutdown_client_parent_class)->finalize) (object);
}



static void
shutdown_client_get_property (GObject    *object,
                              guint       prop_id,
                              GValue     *value,
                              GParamSpec *pspec)
{
  ShutdownClient *client = SHUTDOWN_CLIENT (object);

  switch (prop_id)
    {
    case PROP_BUS_NAME:
      g_value_set_string (value, client->bus_name);
      break;
    case PROP_OBJECT_PATH:
      g_value_set_string (value, client->object_path);
      break;
    case PROP_SHUTDOWN_MODE:
      g_value_set_flags (value, client->shutdown_mode);
      break;
    case PROP_TIMEOUT:
      g_value_set_uint (value, client->timeout);
      break;
    case PROP_CONSUMER:
      g_value_set_object (value, client->consumer);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
shutdown_client_set_property (GObject      *object,
                              guint         prop_id,
                              const GValue *value,
                              GParamSpec   *pspec)
{
  ShutdownClient *client = SHUTDOWN_CLIENT (object);

  switch (prop_id)
    {
    case PROP_BUS_NAME:
      client->bus_name = g_value_dup_string (value);
      break;
    case PROP_OBJECT_PATH:
      client->object_path = g_value_dup_string (value);
      break;
    case PROP_SHUTDOWN_MODE:
      client->shutdown_mode = g_value_get_flags (value);
      break;
    case PROP_TIMEOUT:
      client->timeout = g_value_get_uint (value);
      break;
    case PROP_CONSUMER:
      shutdown_client_set_consumer (client, g_value_get_object (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



/**
 * shutdown_client_new:
 * @bus_name:      The bus name which this shutdown consumer is registered with.
 *                 e.g. org.genivi.NodeStartupController1.
 * @object_path:   The D-Bus object path this shutdown consumer uses.
 *                 e.g. /org/genivi/NodeStartupController1/ShutdownConsumer/1
 * @shutdown_mode: The shutdown mode of this shutdown client.
 *                 For a full list of shutdown modes, see #NSMShutdownType.
 * @timeout:       The amount of time the Node State Manager will wait before deciding
 *                 that the shutdown client is not responding.
 * 
 * Creates a new shutdown client.
 * 
 * Returns: A new #ShutdownClient object.
 */
ShutdownClient *
shutdown_client_new (const gchar    *bus_name,
                     const gchar    *object_path,
                     NSMShutdownType shutdown_mode,
                     guint           timeout)
{
  return g_object_new (TYPE_SHUTDOWN_CLIENT,
                       "bus-name", bus_name,
                       "object-path", object_path,
                       "shutdown-mode", shutdown_mode,
                       "timeout", timeout,
                       NULL);
}



/**
 * shutdown_client_get_bus_name:
 * @client: The #ShutdownClient.
 * 
 * Retrieves the bus name of a #ShutdownClient.
 * 
 * Returns: The bus name for this #ShutdownClient.
 */
const gchar *
shutdown_client_get_bus_name (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NULL);
  return client->bus_name;
}



/**
 * shutdown_client_get_object_path:
 * @client: The #ShutdownClient.
 * 
 * Retrieves the object path of a #ShutdownClient.
 * 
 * Returns: The object path for this #ShutdownClient.
 */
const gchar *
shutdown_client_get_object_path (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NULL);
  return client->object_path;
}



/**
 * shutdown_client_get_shutdown_mode:
 * @client: The #ShutdownClient.
 * 
 * Retrieves the %shutdown_mode of a #ShutdownClient.
 * 
 * Returns: The %shutdown_mode for this #ShutdownClient.
 */
NSMShutdownType
shutdown_client_get_shutdown_mode (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NSM_SHUTDOWN_TYPE_NOT);
  return client->shutdown_mode;
}


/**
 * shutdown_client_set_shutdown_mode:
 * @client:        The #ShutdownClient.
 * @shutdown_mode: The #NSMShutdownType for the @client.
 * 
 * Sets the shutdown_mode of @client.
 */
void
shutdown_client_set_shutdown_mode (ShutdownClient *client,
                                   NSMShutdownType shutdown_mode)
{
  g_return_if_fail (IS_SHUTDOWN_CLIENT (client));

  /* do nothing if the shutdown_mode value is not changing */
  if (client->shutdown_mode == shutdown_mode)
    return;

  /* update the shutdown_mode property */
  client->shutdown_mode = shutdown_mode;

  /* notify the callers that this property has changed */
  g_object_notify (G_OBJECT (client), "shutdown-mode");
}


/**
 * shutdown_client_get_timeout:
 * @client: The #ShutdownClient.
 * 
 * Retrieves the %timeout of a #ShutdownClient.
 * 
 * Returns: The %timeout for this #ShutdownClient.
 */
guint
shutdown_client_get_timeout (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), 0);
  return client->timeout;
}


/**
 * shutdown_client_set_timeout:
 * @client: The #ShutdownClient.
 * @timeout: The timeout for @client.
 * 
 * Sets the timeout of @client.
 */
void
shutdown_client_set_timeout (ShutdownClient *client,
                             guint           timeout)
{
  g_return_if_fail (IS_SHUTDOWN_CLIENT (client));

  /* do nothing if the timeout value is not changing */
  if (client->timeout == timeout)
    return;

  /* update the timeout property */
  client->timeout = timeout;

  /* notify the callers that this property has changed */
  g_object_notify (G_OBJECT (client), "timeout");
}


/**
 * shutdown_client_get_consumer:
 * @client: The #ShutdownClient
 * 
 * Retrieves the %consumer of a #ShutdownClient.
 * 
 *Note: Ownership of the #ShutdownConsumer is not transferred. The object returned by this
 * function should not be released.
 * 
 * Returns: The #ShutdownConsumer for this #ShutdownClient.
 */
ShutdownConsumer *
shutdown_client_get_consumer (ShutdownClient *client)
{
  g_return_val_if_fail (IS_SHUTDOWN_CLIENT (client), NULL);
  return client->consumer;
}


/**
 * shutdown_client_set_consumer:
 * @client:   The #ShutdownClient.
 * @consumer: The #ShutdownConsumer for @client.
 * 
 * Sets the consumer of @client.
 */
void
shutdown_client_set_consumer (ShutdownClient   *client,
                              ShutdownConsumer *consumer)
{
  g_return_if_fail (IS_SHUTDOWN_CLIENT (client));
  g_return_if_fail (consumer == NULL || IS_SHUTDOWN_CONSUMER (consumer));

  /* release the existing consumer if we have one */
  if (client->consumer != NULL)
    {
      /* also, if we are trying to reset the same consumer, ignore */
      if (client->consumer == consumer)
        return;

      /* release and reset the consumer member */
      g_object_unref (client->consumer);
      client->consumer = NULL;
    }

  /* grab a reference on the new consumer */
  if (consumer != NULL)
    client->consumer = g_object_ref (consumer);

  /* notify others that the property has changed */
  g_object_notify (G_OBJECT (client), "consumer");
}
